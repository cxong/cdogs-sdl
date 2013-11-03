/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (C) 1995 Ronny Wester
    Copyright (C) 2003 Jeremy Chin
    Copyright (C) 2003-2007 Lucas Martin-King

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    This file incorporates work covered by the following copyright and
    permission notice:

    Copyright (c) 2013, Cong Xu
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.
    Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
    ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
    LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
    INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
    CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
    ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGE.
*/
#include "game.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <SDL.h>
#include "SDL_mutex.h"

#include "config.h"
#include "draw.h"
#include "hud.h"
#include "joystick.h"
#include "music.h"
#include "objs.h"
#include "palette.h"
#include "pic_manager.h"
#include "actors.h"
#include "pics.h"
#include "text.h"
#include "gamedata.h"
#include "ai.h"
#include "input.h"
#include "automap.h"
#include "mission.h"
#include "sounds.h"
#include "triggers.h"
#include "keyboard.h"
#include "blit.h"
#include "utils.h"

#include "drawtools.h" /* for Draw_Box and Draw_Point */

#define FPS_FRAMELIMIT       70
#define CLOCK_LIMIT       2100

#define FPS_TARGET	30

#define SWITCH_TURNLIMIT     10

#define PICKUP_LIMIT         350

#define SPLIT_X  300
#define SPLIT_Y  180

static Uint32 ticks_now;
static Uint32 ticks_then;

static int frames = 0;

static int screenShaking = 0;

long oldtime;

// This is referenced from CDOGS.C to determine time bonus
int missionTime;

#define MICROSECS_PER_SEC 1000000
#define MILLISECS_PER_SEC 1000

void PlayerSpecialCommands(TActor *actor, int cmd, struct PlayerData *data)
{
	int isDirectionCmd = cmd & (CMD_LEFT | CMD_RIGHT | CMD_UP | CMD_DOWN);
	assert(actor);
	if (!((cmd | actor->lastCmd) & CMD_BUTTON2))
	{
		actor->flags &= ~FLAGS_SPECIAL_USED;
	}

	if ((cmd & CMD_BUTTON2) &&
		isDirectionCmd &&
		actor->dx == 0 && actor->dy == 0)
	{
		if (gConfig.Game.SwitchMoveStyle == SWITCHMOVE_SLIDE)
		{
			SlideActor(actor, cmd);
		}
		if (gConfig.Game.SwitchMoveStyle != SWITCHMOVE_NONE)
		{
			actor->flags |= FLAGS_SPECIAL_USED;
		}
	}
	else if (
		(actor->lastCmd & CMD_BUTTON2) &&
		!(cmd & CMD_BUTTON2) &&
		!(actor->flags & FLAGS_SPECIAL_USED) &&
		!(gConfig.Game.SwitchMoveStyle == SWITCHMOVE_SLIDE && isDirectionCmd) &&
		data->weaponCount > 1)
	{
		int i;
		for (i = 0; i < data->weaponCount; i++)
		{
			if (actor->weapon.gun == (gun_e)data->weapons[i])
			{
				break;
			}
		}
		i++;
		if (i >= data->weaponCount)
		{
			i = 0;
		}
		actor->weapon.gun = data->weapons[i];
		SoundPlayAt(
			&gSoundDevice,
			SND_SWITCH,
			Vec2iNew(actor->tileItem.x, actor->tileItem.y));
	}
}

static void Ticks_Update(void)
{
	static int init = 0;

	if (init == 0) {
		ticks_then = SDL_GetTicks();
		ticks_now = SDL_GetTicks();
		init = 1;
	} else {
		ticks_then = ticks_now;
		ticks_now = SDL_GetTicks();
	}

	return;
}

static int Ticks_TimeElapsed(Uint32 msec)
{
	static Uint32 old_ticks = 0;

	if (old_ticks == 0) {
		old_ticks = ticks_now;
		return 0;
	} else {
		if (ticks_now - old_ticks > msec) {
			old_ticks = ticks_now;
			return 1;
		} else {
			return 0;
		}
	}
}

static void Ticks_FrameEnd(void)
{
	Uint32 now = SDL_GetTicks();
	Uint32 ticksSpent = now - ticks_now;
	Uint32 ticksIdeal = 16;
	if (ticksSpent < ticksIdeal)
	{
		Uint32 ticksToDelay = ticksIdeal - ticksSpent;
		SDL_Delay(ticksToDelay);
		debug(D_VERBOSE, "Delaying %u ticks_now %u now %u\n", ticksToDelay, ticks_now, now);
	}
}


void DoBuffer(
	DrawBuffer *b, Vec2i center, int w, Vec2i noise, Vec2i offset)
{
	DrawBufferSetFromMap(
		b, gMap, Vec2iAdd(center, noise), w, Vec2iNew(X_TILES, Y_TILES));
	LineOfSight(center, b, MAPTILE_IS_SHADOW);
	FixBuffer(b, MAPTILE_IS_SHADOW);
	DrawBufferDraw(b, offset);
}

void ShakeScreen(int amount)
{
	screenShaking = (screenShaking + amount) * gConfig.Graphics.ShakeMultiplier;

	/* So we don't shake too much :) */
	if (screenShaking > 100)
		screenShaking = 100;
}

void BlackLine(void)
{
	int i;
	Uint32 *p = gGraphicsDevice.buf;
	Uint32 black = PixelFromColor(&gGraphicsDevice, colorBlack);

	p += (gGraphicsDevice.cachedConfig.ResolutionWidth / 2) - 1;
	for (i = 0; i < gGraphicsDevice.cachedConfig.ResolutionHeight; i++)
	{
		*p++ = black;
		*p = black;
		p += gGraphicsDevice.cachedConfig.ResolutionWidth - 1;
	}
}

int IsSingleScreen(TTileItem *p1, TTileItem *p2, int splitScreenAlways)
{
	return
		!splitScreenAlways &&
		abs(p1->x - p2->x) < SPLIT_X && abs(p1->y - p2->y) < SPLIT_Y;
}

Vec2i DrawScreen(
	DrawBuffer *b, TActor *player1, TActor *player2, Vec2i lastPosition)
{
	Vec2i noise = Vec2iZero();
	Vec2i centerOffset = Vec2iNew(-TILE_WIDTH / 2 - 8, -TILE_HEIGHT / 2 - 4);
	int i;

	if (screenShaking)
	{
		noise.x = rand() & 7;
		noise.y = rand() & 7;
	}

	for (i = 0; i < GraphicsGetScreenSize(&gGraphicsDevice.cachedConfig); i++)
	{
		gGraphicsDevice.buf[i] = PixelFromColor(&gGraphicsDevice, colorBlack);
	}

	GraphicsResetBlitClip(&gGraphicsDevice);
	if (player1 && player2)
	{
		if (IsSingleScreen(
				&player1->tileItem, &player2->tileItem,
				gConfig.Interface.SplitscreenAlways))
		{
			// One screen
			lastPosition.x = (player1->tileItem.x + player2->tileItem.x) / 2;
			lastPosition.y = (player1->tileItem.y + player2->tileItem.y) / 2;

			DrawBufferSetFromMap(
				b, gMap,
				Vec2iAdd(lastPosition, noise),
				X_TILES,
				Vec2iNew(X_TILES, Y_TILES));
			LineOfSight(
				Vec2iNew(player1->tileItem.x, player1->tileItem.y),
				b,
				MAPTILE_IS_SHADOW);
			LineOfSight(
				Vec2iNew(player2->tileItem.x, player2->tileItem.y),
				b,
				MAPTILE_IS_SHADOW2);
			FixBuffer(b, MAPTILE_IS_SHADOW | MAPTILE_IS_SHADOW2);
			DrawBufferDraw(b, centerOffset);
			SoundSetEars(lastPosition);
		}
		else
		{
			Vec2i center = Vec2iNew(player1->tileItem.x, player1->tileItem.y);
			GraphicsSetBlitClip(
				&gGraphicsDevice,
				0,
				0,
				(gGraphicsDevice.cachedConfig.ResolutionWidth / 2) - 1,
				gGraphicsDevice.cachedConfig.ResolutionHeight - 1);
			DoBuffer(
				b,
				center,
				X_TILES_HALF,
				noise,
				centerOffset);
			SoundSetLeftEar(center);

			center = Vec2iNew(player2->tileItem.x, player2->tileItem.y);
			GraphicsSetBlitClip(
				&gGraphicsDevice,
				(gGraphicsDevice.cachedConfig.ResolutionWidth / 2) + 1,
				0,
				gGraphicsDevice.cachedConfig.ResolutionWidth - 1,
				gGraphicsDevice.cachedConfig.ResolutionHeight - 1);
			centerOffset.x +=
				(gGraphicsDevice.cachedConfig.ResolutionWidth / 2) + 1;
			DoBuffer(
				b,
				center,
				X_TILES_HALF,
				noise,
				centerOffset);
			SoundSetRightEar(center);
			lastPosition.x = player1->tileItem.x;
			lastPosition.y = player1->tileItem.y;
			BlackLine();
		}
	}
	else if (player1)
	{
		Vec2i center = Vec2iNew(player1->tileItem.x, player1->tileItem.y);
		DoBuffer(b, center, X_TILES, noise, centerOffset);
		SoundSetEars(center);
		lastPosition.x = player1->tileItem.x;
		lastPosition.y = player1->tileItem.y;
	}
	else if (player2)
	{
		Vec2i center = Vec2iNew(player2->tileItem.x, player2->tileItem.y);
		DoBuffer(b, center, X_TILES, noise, centerOffset);
		SoundSetEars(center);
		lastPosition.x = player2->tileItem.x;
		lastPosition.y = player2->tileItem.y;
	}
	else
	{
		DoBuffer(b, lastPosition, X_TILES, noise, centerOffset);
	}
	GraphicsResetBlitClip(&gGraphicsDevice);
	return lastPosition;
}

static void DrawExitArea(void)
{
	int left, right, top, bottom;
	int x, y;

	left = gMission.exitLeft / TILE_WIDTH;
	right = gMission.exitRight / TILE_WIDTH;
	top = gMission.exitTop / TILE_HEIGHT;
	bottom = gMission.exitBottom / TILE_HEIGHT;

	for (x = left; x <= right; x++)
		ChangeFloor(x, top, gMission.exitPic, gMission.exitShadow);
	for (x = left; x <= right; x++)
		ChangeFloor(x, bottom, gMission.exitPic,
			    gMission.exitShadow);
	for (y = top + 1; y < bottom; y++)
		ChangeFloor(left, y, gMission.exitPic,
			    gMission.exitShadow);
	for (y = top + 1; y < bottom; y++)
		ChangeFloor(right, y, gMission.exitPic,
			    gMission.exitShadow);
}

static void MissionUpdateObjectives(void)
{
	int i, left;
	char s[4];
	int allDone = 1;
	static int completed = 0;
	int x, y;

	x = 5;
	y = gGraphicsDevice.cachedConfig.ResolutionHeight - 5 - CDogsTextHeight();
	for (i = 0; i < gMission.missionData->objectiveCount; i++) {
		if (gMission.missionData->objectives[i].type ==
		    OBJECTIVE_INVESTIGATE)
			gMission.objectives[i].done = ExploredPercentage();

		if (gMission.missionData->objectives[i].required > 0)
		{
			y += 3;
			// Objective color dot
			Draw_Rect(x, y, 2, 2, gMission.objectives[i].color);
			y -= 3;

			left = gMission.objectives[i].required - gMission.objectives[i].done;

			if (left > 0) {
				if ((gMission.missionData->objectives[i].flags & OBJECTIVE_UNKNOWNCOUNT) == 0) {
					sprintf(s, "%d", left);
				} else {
					strcpy(s, "?");
				}
				CDogsTextStringAt(x + 5, y, s);
				allDone = 0;
			} else {
				CDogsTextStringAt(x + 5, y, "Done");
			}
			x += 30;
		}
	}

	if (allDone && !completed)
	{
		completed = 1;
		DrawExitArea();
	}
	else if (!allDone)
	{
		completed = 0;
	}
}

int HandleKey(int cmd, int *isPaused)
{
	if (IsAutoMapEnabled(gCampaign.Entry.mode))
	{
		int hasDisplayedAutomap = 0;
		while (KeyIsDown(
			&gInputDevices.keyboard, gConfig.Input.PlayerKeys[0].Keys.map) ||
			(cmd & CMD_BUTTON3) != 0)
		{
			int cmd1 = 0, cmd2 = 0;
			if (!hasDisplayedAutomap)
			{
				AutomapDraw(0);
				BlitFlip(&gGraphicsDevice, &gConfig.Graphics);
				hasDisplayedAutomap = 1;
			}
			SDL_Delay(10);
			InputPoll(&gInputDevices, SDL_GetTicks());
			cmd1 = InputGetGameCmd(
				&gInputDevices, &gConfig.Input, 0, Vec2iZero());
			cmd2 = InputGetGameCmd(
				&gInputDevices, &gConfig.Input, 1, Vec2iZero());
			cmd = cmd1 | cmd2;
		}
	}

	if (*isPaused && AnyButton(cmd))
	{
		*isPaused = 0;
	}

	if (KeyIsPressed(&gInputDevices.keyboard, SDLK_ESCAPE) ||
		JoyIsPressed(&gInputDevices.joysticks.joys[0], CMD_BUTTON4) ||
		JoyIsPressed(&gInputDevices.joysticks.joys[1], CMD_BUTTON4))
	{
		if (!*isPaused)
		{
			*isPaused = 1;
		}
		return 1;
	}
	else if (KeyGetPressed(&gInputDevices.keyboard))
	{
		*isPaused = 0;
	}

	if (!*isPaused && !(SDL_GetAppState() & SDL_APPINPUTFOCUS))
	{
		*isPaused = 1;
	}

	return 0;
}

Vec2i GetPlayerCenter(GraphicsDevice *device, int player)
{
	Vec2i center;
	int players = 1;
	int screenW = device->cachedConfig.ResolutionWidth;
	if (gOptions.twoPlayers && gPlayer1 && gPlayer2)
	{
		players = 2;
	}
	
	if (players == 2 &&
		IsSingleScreen(
			&gPlayer1->tileItem, &gPlayer2->tileItem,
			gConfig.Interface.SplitscreenAlways))
	{
		Vec2i pCenter = Vec2iNew(
			(gPlayer1->tileItem.x + gPlayer2->tileItem.x) / 2,
			(gPlayer1->tileItem.y + gPlayer2->tileItem.y) / 2);
		Vec2i screenCenter = Vec2iNew(
			screenW / 2,
			device->cachedConfig.ResolutionHeight / 2);
		int factor = 1;
		TTileItem *pTileItem =
			player == 0 ? &gPlayer1->tileItem : &gPlayer2->tileItem;
		Vec2i p = Vec2iNew(pTileItem->x, pTileItem->y);
		assert((player == 0 || player == 1) && "invalid player index");
		center = Vec2iAdd(
			Vec2iScaleDiv(Vec2iAdd(p, Vec2iScale(pCenter, -1)), factor),
			screenCenter);
		printf("%d,%d\n", center.x, center.y);
	}
	else
	{
		center.y = device->cachedConfig.ResolutionHeight / 2;
		if (players == 1)
		{
			center.x = screenW / 2;
		}
		else
		{
			assert(players == 2 && "invalid number of players");
			if (player == 0)
			{
				center.x = screenW / 4;
			}
			else
			{
				assert(player == 1 && "invalid player");
				center.x = screenW * 3 / 4;
			}
		}
	}
	return center;
}

int gameloop(void)
{
	DrawBuffer buffer;
	int is_esc_pressed = 0;
	int isDone = 0;
	int isPaused = 0;
	HUD hud;
	Vec2i lastPosition = Vec2iZero();

	DrawBufferInit(&buffer, Vec2iNew(X_TILES, Y_TILES));
	HUDInit(&hud, &gConfig.Interface, &gGraphicsDevice, &gMission);

	if (MusicGetStatus(&gSoundDevice) != MUSIC_OK)
	{
		HUDDisplayMessage(&hud, MusicGetErrorMessage(&gSoundDevice));
	}

	missionTime = 0;
	InputInit(&gInputDevices, PicManagerGetOldPic(&gPicManager, 340));
	while (!isDone)
	{
		int cmd1 = 0, cmd2 = 0;
		int ticks = 1;
		Ticks_Update();

		MusicSetPlaying(&gSoundDevice, SDL_GetAppState() & SDL_APPINPUTFOCUS);
		InputPoll(&gInputDevices, ticks_now);
		if (gPlayer1 && !gPlayer2->dead)
		{
			cmd1 = InputGetGameCmd(
				&gInputDevices,
				&gConfig.Input,
				0,
				GetPlayerCenter(&gGraphicsDevice, 0));
		}
		if (gPlayer2 && !gPlayer2->dead)
		{
			cmd2 = InputGetGameCmd(
				&gInputDevices,
				&gConfig.Input,
				1,
				GetPlayerCenter(&gGraphicsDevice, 1));
		}
		is_esc_pressed = HandleKey(cmd1 | cmd2, &isPaused);
		if (is_esc_pressed && isPaused)
		{
			isDone = 1;
		}

		if (!isPaused)
		{
			if (!gConfig.Game.SlowMotion || (frames & 1) == 0)
			{
				UpdateAllActors(ticks);
				UpdateMobileObjects(&gMobObjList, ticks);

				if (gPlayer1)
				{
					PlayerSpecialCommands(gPlayer1, cmd1, &gPlayer1Data);
					CommandActor(gPlayer1, cmd1, ticks);
				}
				if (gPlayer2)
				{
					PlayerSpecialCommands(gPlayer2, cmd2, &gPlayer2Data);
					CommandActor(gPlayer2, cmd2, ticks);
				}

				if (gOptions.badGuys)
				{
					CommandBadGuys(ticks);
				}

				UpdateWatches();
			}

			missionTime += ticks;
			if ((gPlayer1 || gPlayer2) && IsMissionComplete(&gMission))
			{
				if (gMission.pickupTime == PICKUP_LIMIT)
				{
					SoundPlay(&gSoundDevice, SND_DONE);
				}
				gMission.pickupTime -= ticks;
				if (gMission.pickupTime <= 0)
				{
					isDone = 1;
				}
			}
			else
			{
				gMission.pickupTime = PICKUP_LIMIT;
			}
		}

		if (HasObjectives(gCampaign.Entry.mode))
		{
			MissionUpdateObjectives();
		}

		if (!gPlayer1 && !gPlayer2)
		{
			isDone = 1;
		}

		lastPosition = DrawScreen(&buffer, gPlayer1, gPlayer2, lastPosition);

		if (screenShaking) {
			screenShaking -= ticks;
			if (screenShaking < 0)
				screenShaking = 0;
		}

		debug(D_VERBOSE, "frames... %d\n", frames);

		if (Ticks_TimeElapsed(MILLISECS_PER_SEC))
		{
			frames = 0;
		}

		HUDUpdate(&hud, ticks_now - ticks_then);
		HUDDraw(&hud, isPaused);
		if (ConfigIsMouseUsed(&gConfig.Input))
		{
			MouseDraw(&gInputDevices.mouse);
		}

		BlitFlip(&gGraphicsDevice, &gConfig.Graphics);

		Ticks_FrameEnd();
	}
	DrawBufferTerminate(&buffer);

	return !is_esc_pressed;
}
