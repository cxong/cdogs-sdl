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

#include <cdogs/actors.h>
#include <cdogs/ai.h>
#include <cdogs/automap.h>
#include <cdogs/config.h>
#include <cdogs/draw.h>
#include <cdogs/game_events.h>
#include <cdogs/hud.h>
#include <cdogs/joystick.h>
#include <cdogs/mission.h>
#include <cdogs/music.h>
#include <cdogs/objs.h>
#include <cdogs/palette.h>
#include <cdogs/pic_manager.h>
#include <cdogs/pics.h>
#include <cdogs/text.h>
#include <cdogs/triggers.h>

#include <cdogs/drawtools.h> /* for Draw_Box and Draw_Point */

#define FPS_FRAMELIMIT       70
#define CLOCK_LIMIT       2100

#define FPS_TARGET	30

#define SWITCH_TURNLIMIT     10

#define PICKUP_LIMIT         350

#define SPLIT_PADDING 40

static Uint32 ticks_now;
static Uint32 ticks_then;

static int frames = 0;

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

int GetShakeAmount(int oldShake, int amount)
{
	int shake = (oldShake + amount) * gConfig.Graphics.ShakeMultiplier;

	/* So we don't shake too much :) */
	if (shake > 100)
	{
		shake = 100;
	}

	return shake;
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

void GetBoundingRectangle(Vec2i *min, Vec2i *max)
{
	int isFirst = 1;
	int i;
	*min = Vec2iZero();
	*max = Vec2iZero();
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		if (gPlayers[i] && !gPlayers[i]->dead)
		{
			TTileItem *p = &gPlayers[i]->tileItem;
			if (isFirst)
			{
				*min = *max = Vec2iNew(p->x, p->y);
			}
			else
			{
				if (p->x < min->x)	min->x = p->x;
				if (p->y < min->y)	min->y = p->y;
				if (p->x > max->x)	max->x = p->x;
				if (p->y > max->y)	max->y = p->y;
			}
			isFirst = 0;
		}
	}
}

int IsSingleScreen(GraphicsConfig *config, int splitScreenAlways)
{
	Vec2i min;
	Vec2i max;
	if (splitScreenAlways)
	{
		return 0;
	}
	GetBoundingRectangle(&min, &max);
	return
		max.x - min.x < config->ResolutionWidth - SPLIT_PADDING &&
		max.y - min.y < config->ResolutionHeight - SPLIT_PADDING;
}

static Vec2i GetPlayersMidpoint(void)
{
	// for all surviving players, find bounding rectangle, and get center
	Vec2i min;
	Vec2i max;
	GetBoundingRectangle(&min, &max);
	return Vec2iNew((min.x + max.x) / 2, (min.y + max.y) / 2);
}

Vec2i DrawScreen(DrawBuffer *b, Vec2i lastPosition, int shakeAmount)
{
	Vec2i noise = Vec2iZero();
	Vec2i centerOffset = Vec2iNew(-TILE_WIDTH / 2 - 8, -TILE_HEIGHT / 2 - 4);
	int i;
	int numPlayersAlive = GetNumPlayersAlive();

	if (shakeAmount)
	{
		noise.x = rand() & 7;
		noise.y = rand() & 7;
	}

	for (i = 0; i < GraphicsGetScreenSize(&gGraphicsDevice.cachedConfig); i++)
	{
		gGraphicsDevice.buf[i] = PixelFromColor(&gGraphicsDevice, colorBlack);
	}

	GraphicsResetBlitClip(&gGraphicsDevice);
	if (numPlayersAlive == 0)
	{
		DoBuffer(b, lastPosition, X_TILES, noise, centerOffset);
	}
	else
	{
		if (numPlayersAlive == 1)
		{
			TActor *p = GetFirstAlivePlayer();
			Vec2i center = Vec2iNew(p->tileItem.x, p->tileItem.y);
			DoBuffer(b, center, X_TILES, noise, centerOffset);
			SoundSetEars(center);
			lastPosition = center;
		}
		else if (IsSingleScreen(
				&gGraphicsDevice.cachedConfig,
				gConfig.Interface.SplitscreenAlways))
		{
			// One screen
			lastPosition = GetPlayersMidpoint();

			DrawBufferSetFromMap(
				b, gMap,
				Vec2iAdd(lastPosition, noise),
				X_TILES,
				Vec2iNew(X_TILES, Y_TILES));
			for (i = 0; i < MAX_PLAYERS; i++)
			{
				if (gPlayers[i] && !gPlayers[i]->dead)
				{
					int shadowFlag =
						i == 0 ? MAPTILE_IS_SHADOW : MAPTILE_IS_SHADOW2;
					LineOfSight(
						Vec2iNew(
							gPlayers[i]->tileItem.x, gPlayers[i]->tileItem.y),
						b,
						shadowFlag);
				}
			}
			FixBuffer(b, MAPTILE_IS_SHADOW | MAPTILE_IS_SHADOW2);
			DrawBufferDraw(b, centerOffset);
			SoundSetEars(lastPosition);
		}
		else if (numPlayersAlive == 2 && gOptions.numPlayers == 2)
		{
			// side-by-side split
			for (i = 0; i < 2; i++)
			{
				Vec2i center = Vec2iNew(
					gPlayers[i]->tileItem.x, gPlayers[i]->tileItem.y);
				int clipLeft = i == 0 ? 0 :
					(gGraphicsDevice.cachedConfig.ResolutionWidth / 2) + 1;
				int clipRight = i == 0 ?
					(gGraphicsDevice.cachedConfig.ResolutionWidth / 2) - 1 : 0;
				GraphicsSetBlitClip(
					&gGraphicsDevice,
					clipLeft,
					0,
					clipRight,
					gGraphicsDevice.cachedConfig.ResolutionHeight - 1);
				DoBuffer(b, center, X_TILES_HALF, noise, centerOffset);
				if (i == 0)
				{
					SoundSetLeftEar(center);
					lastPosition = center;
				}
				else
				{
					SoundSetRightEar(center);
				}
			}
			BlackLine();
		}
		else
		{
			// 4 player split screen
			assert(0 && "not implemented yet");
		}
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
	int players = 0;
	int screenW = device->cachedConfig.ResolutionWidth;
	int i;
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		if (gPlayers[i] && !gPlayers[i]->dead)
		{
			players++;
		}
	}
	
	if (IsSingleScreen(
			&device->cachedConfig,
			gConfig.Interface.SplitscreenAlways))
	{
		Vec2i pCenter = GetPlayersMidpoint();
		Vec2i screenCenter = Vec2iNew(
			screenW / 2,
			device->cachedConfig.ResolutionHeight / 2);
		int factor = 1;
		TTileItem *pTileItem = &gPlayers[player]->tileItem;
		Vec2i p = Vec2iNew(pTileItem->x, pTileItem->y);
		assert((player == 0 || player == 1) && "invalid player index");
		center = Vec2iAdd(
			Vec2iScaleDiv(Vec2iAdd(p, Vec2iScale(pCenter, -1)), factor),
			screenCenter);
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

void HandleGameEvents(GameEventStore *store, int *shakeAmount)
{
	int i;
	for (i = 0; i < store->count; i++)
	{
		switch (store->events[i].Type)
		{
		case GAME_EVENT_SCREEN_SHAKE:
			*shakeAmount = GetShakeAmount(
				*shakeAmount, store->events[i].u.ShakeAmount);
			break;
		default:
			assert(0 && "unknown game event");
			break;
		}
	}
	GameEventsClear(store);
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
	GameEventsInit(&gGameEvents);

	if (MusicGetStatus(&gSoundDevice) != MUSIC_OK)
	{
		HUDDisplayMessage(&hud, MusicGetErrorMessage(&gSoundDevice));
	}

	missionTime = 0;
	InputInit(&gInputDevices, PicManagerGetOldPic(&gPicManager, 340));
	while (!isDone)
	{
		int cmds[MAX_PLAYERS];
		int cmdAll = 0;
		int ticks = 1;
		int i;
		int shakeAmount = 0;
		Ticks_Update();

		MusicSetPlaying(&gSoundDevice, SDL_GetAppState() & SDL_APPINPUTFOCUS);
		InputPoll(&gInputDevices, ticks_now);
		for (i = 0; i < MAX_PLAYERS; i++)
		{
			if (gPlayers[i] && !gPlayers[i]->dead)
			{
				cmds[i] = InputGetGameCmd(
					&gInputDevices,
					&gConfig.Input,
					i,
					GetPlayerCenter(&gGraphicsDevice, i));
				cmdAll |= cmds[i];
			}
		}
		is_esc_pressed = HandleKey(cmdAll, &isPaused);
		if (is_esc_pressed)
		{
			if (isPaused)
			{
				isDone = 1;
			}
			else
			{
				isPaused = 1;
			}
		}

		if (!isPaused)
		{
			if (!gConfig.Game.SlowMotion || (frames & 1) == 0)
			{
				HandleGameEvents(&gGameEvents, &shakeAmount);
				UpdateAllActors(ticks);
				UpdateMobileObjects(&gMobObjList, ticks);

				for (i = 0; i < MAX_PLAYERS; i++)
				{
					if (gPlayers[i])
					{
						PlayerSpecialCommands(
							gPlayers[i], cmds[i], &gPlayerDatas[i]);
						CommandActor(gPlayers[i], cmds[i], ticks);
					}
				}

				if (gOptions.badGuys)
				{
					CommandBadGuys(ticks);
				}

				UpdateWatches();
			}

			missionTime += ticks;
			if (GetNumPlayersAlive() > 0 && IsMissionComplete(&gMission))
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

		if (GetNumPlayersAlive() == 0)
		{
			isDone = 1;
		}

		lastPosition = DrawScreen(&buffer, lastPosition, shakeAmount);

		shakeAmount -= ticks;
		if (shakeAmount < 0)
		{
			shakeAmount = 0;
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
	GameEventsTerminate(&gGameEvents);
	DrawBufferTerminate(&buffer);

	return !is_esc_pressed;
}
