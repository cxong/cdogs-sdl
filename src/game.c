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
#include <cdogs/ai_coop.h>
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


static void DoBuffer(
	DrawBuffer *b, Vec2i center, int w, Vec2i noise, Vec2i offset)
{
	DrawBufferSetFromMap(
		b, gMap, Vec2iAdd(center, noise), w, Vec2iNew(X_TILES, Y_TILES));
	LineOfSight(center, b);
	FixBuffer(b);
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

int IsSingleScreen(GraphicsConfig *config, SplitscreenStyle splitscreenStyle)
{
	Vec2i min;
	Vec2i max;
	if (splitscreenStyle == SPLITSCREEN_ALWAYS)
	{
		return 0;
	}
	PlayersGetBoundingRectangle(gPlayers, &min, &max);
	return
		max.x - min.x < config->ResolutionWidth - SPLIT_PADDING &&
		max.y - min.y < config->ResolutionHeight - SPLIT_PADDING;
}

Vec2i DrawScreen(DrawBuffer *b, Vec2i lastPosition, int shakeAmount)
{
	Vec2i noise = Vec2iZero();
	Vec2i centerOffset = Vec2iNew(-TILE_WIDTH / 2 - 8, -TILE_HEIGHT / 2 - 4);
	int i;
	int numPlayersAlive = GetNumPlayersAlive();
	int w = gGraphicsDevice.cachedConfig.ResolutionWidth;
	int h = gGraphicsDevice.cachedConfig.ResolutionHeight;

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
				gConfig.Interface.Splitscreen))
		{
			// One screen
			lastPosition = PlayersGetMidpoint(gPlayers);

			DrawBufferSetFromMap(
				b, gMap,
				Vec2iAdd(lastPosition, noise),
				X_TILES,
				Vec2iNew(X_TILES, Y_TILES));
			for (i = 0; i < MAX_PLAYERS; i++)
			{
				if (IsPlayerAlive(i))
				{
					LineOfSight(
						Vec2iNew(
							gPlayers[i]->tileItem.x, gPlayers[i]->tileItem.y),
						b);
				}
			}
			FixBuffer(b);
			DrawBufferDraw(b, centerOffset);
			SoundSetEars(lastPosition);
		}
		else if (gOptions.numPlayers == 2)
		{
			assert(numPlayersAlive == 2);
			// side-by-side split
			for (i = 0; i < 2; i++)
			{
				Vec2i center = Vec2iNew(
					gPlayers[i]->tileItem.x, gPlayers[i]->tileItem.y);
				Vec2i centerOffsetPlayer = centerOffset;
				int clipLeft = (i & 1) ? w / 2 : 0;
				int clipRight = (i & 1) ? w - 1 : (w / 2) - 1;
				GraphicsSetBlitClip(
					&gGraphicsDevice, clipLeft, 0, clipRight, h - 1);
				if (i == 1)
				{
					centerOffsetPlayer.x += w / 2;
				}
				DoBuffer(b, center, X_TILES_HALF, noise, centerOffsetPlayer);
				if (i == 0)
				{
					SoundSetLeftEars(center);
					lastPosition = center;
				}
				else
				{
					SoundSetRightEars(center);
				}
			}
			Draw_Line(w / 2 - 1, 0, w / 2 - 1, h - 1, colorBlack);
			Draw_Line(w / 2, 0, w / 2, h - 1, colorBlack);
		}
		else if (gOptions.numPlayers >= 3 && gOptions.numPlayers <= 4)
		{
			// 4 player split screen
			for (i = 0; i < 4; i++)
			{
				Vec2i center;
				Vec2i centerOffsetPlayer = centerOffset;
				int clipLeft = (i & 1) ? w / 2 : 0;
				int clipTop = (i < 2) ? 0 : h / 2 - 1;
				int clipRight = (i & 1) ? w - 1 : (w / 2) - 1;
				int clipBottom = (i < 2) ? h / 2 : h - 1;
				if (!IsPlayerAlive(i))
				{
					continue;
				}
				center = Vec2iNew(
					gPlayers[i]->tileItem.x, gPlayers[i]->tileItem.y);
				GraphicsSetBlitClip(
					&gGraphicsDevice,
					clipLeft, clipTop, clipRight, clipBottom);
				if (i & 1)
				{
					centerOffsetPlayer.x += w / 2;
				}
				if (i < 2)
				{
					centerOffsetPlayer.y -= h / 4;
				}
				else
				{
					centerOffsetPlayer.y += h / 4;
				}
				DoBuffer(b, center, X_TILES_HALF, noise, centerOffsetPlayer);

				// Set the sound "ears"
				// If any player is dead, that ear reverts to the other ear
				// of the same side of the remaining player
				// If both players of one side are dead, those ears revert
				// to any of the other remaining players
				switch (i)
				{
				case 0:
					if (IsPlayerAlive(0))
					{
						SoundSetLeftEar1(center);
						if (!IsPlayerAlive(2))
						{
							SoundSetLeftEar2(center);
						}
						else if (!IsPlayerAlive(1) && !IsPlayerAlive(3))
						{
							SoundSetRightEars(center);
						}
					}
					break;
				case 1:
					if (IsPlayerAlive(1))
					{
						SoundSetRightEar1(center);
						if (!IsPlayerAlive(3))
						{
							SoundSetRightEar2(center);
						}
						else if (!IsPlayerAlive(0) && !IsPlayerAlive(2))
						{
							SoundSetLeftEars(center);
						}
					}
					break;
				case 2:
					if (IsPlayerAlive(2))
					{
						SoundSetLeftEar2(center);
						if (!IsPlayerAlive(0))
						{
							SoundSetLeftEar1(center);
						}
						else if (!IsPlayerAlive(1) && !IsPlayerAlive(3))
						{
							SoundSetRightEars(center);
						}
					}
					break;
				case 3:
					if (IsPlayerAlive(3))
					{
						SoundSetRightEar2(center);
						if (!IsPlayerAlive(1))
						{
							SoundSetRightEar1(center);
						}
						else if (!IsPlayerAlive(0) && !IsPlayerAlive(2))
						{
							SoundSetLeftEars(center);
						}
					}
					break;
				default:
					assert(0 && "unknown error");
					break;
				}
				if (IsPlayerAlive(i))
				{
					lastPosition = center;
				}
			}
			Draw_Line(w / 2 - 1, 0, w / 2 - 1, h - 1, colorBlack);
			Draw_Line(w / 2, 0, w / 2, h - 1, colorBlack);
			Draw_Line(0, h / 2 - 1, w - 1, h / 2 - 1, colorBlack);
			Draw_Line(0, h / 2, w - 1, h / 2, colorBlack);
		}
		else
		{
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
			int i;
			if (!hasDisplayedAutomap)
			{
				AutomapDraw(0);
				BlitFlip(&gGraphicsDevice, &gConfig.Graphics);
				hasDisplayedAutomap = 1;
			}
			SDL_Delay(10);
			InputPoll(&gInputDevices, SDL_GetTicks());
			cmd = 0;
			for (i = 0; i < MAX_PLAYERS; i++)
			{
				cmd |= GetGameCmd(
					&gInputDevices, &gConfig.Input,
					&gPlayerDatas[i], Vec2iZero());
			}
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
	Vec2i center = Vec2iZero();
	int w = device->cachedConfig.ResolutionWidth;
	int h = device->cachedConfig.ResolutionHeight;

	if (GetNumPlayersAlive() == 1 ||
		IsSingleScreen(
			&device->cachedConfig,
			gConfig.Interface.Splitscreen))
	{
		Vec2i pCenter = PlayersGetMidpoint(gPlayers);
		Vec2i screenCenter = Vec2iNew(
			w / 2,
			device->cachedConfig.ResolutionHeight / 2);
		TTileItem *pTileItem = &gPlayers[player]->tileItem;
		Vec2i p = Vec2iNew(pTileItem->x, pTileItem->y);
		center = Vec2iAdd(
			Vec2iAdd(p, Vec2iScale(pCenter, -1)), screenCenter);
	}
	else
	{
		if (gOptions.numPlayers == 2)
		{
			center.x = player == 0 ? w / 4 : w * 3 / 4;
			center.y = h / 2;
		}
		else if (gOptions.numPlayers >= 3 && gOptions.numPlayers <= 4)
		{
			center.x = (player & 1) ? w * 3 / 4 : w / 4;
			center.y = (player >= 2) ? h * 3 / 4 : h / 4;
		}
		else
		{
			assert(0 && "invalid number of players");
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
	gMission.pickupTime = PICKUP_LIMIT;
	InputInit(&gInputDevices, PicManagerGetOldPic(&gPicManager, 340));
	while (!isDone)
	{
		int cmds[MAX_PLAYERS];
		int cmdAll = 0;
		int ticks = 1;
		int i;
		int shakeAmount = 0;
		int allPlayersDestroyed = 1;
		Ticks_Update();

		MusicSetPlaying(&gSoundDevice, SDL_GetAppState() & SDL_APPINPUTFOCUS);
		InputPoll(&gInputDevices, ticks_now);
		for (i = 0; i < MAX_PLAYERS; i++)
		{
			if (IsPlayerAlive(i))
			{
				cmds[i] = GetGameCmd(
					&gInputDevices,
					&gConfig.Input,
					&gPlayerDatas[i],
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
		// Cheat: special weapon activation
		if (IsPlayerAlive(0))
		{
			const char *pulserifle = "sgodc";
			const char *heatseeker = "miaon";
			int isMatch = 1;
			for (i = 0; i < (int)strlen(pulserifle); i++)
			{
				if (gInputDevices.keyboard.pressedKeysBuffer[i] !=
					pulserifle[i])
				{
					isMatch = 0;
					break;
				}
			}
			if (isMatch)
			{
				gPlayers[0]->weapon = WeaponCreate(GUN_PULSERIFLE);
				SoundPlay(&gSoundDevice, SND_HAHAHA);
				// Reset to prevent last key from being processed as
				// normal player commands
				KeyInit(&gInputDevices.keyboard);
				cmdAll = 0;
				memset(cmds, 0, sizeof cmds);
			}
			isMatch = 1;
			for (i = 0; i < (int)strlen(heatseeker); i++)
			{
				if (gInputDevices.keyboard.pressedKeysBuffer[i] !=
					heatseeker[i])
				{
					isMatch = 0;
					break;
				}
			}
			if (isMatch)
			{
				gPlayers[0]->weapon = WeaponCreate(GUN_HEATSEEKER);
				SoundPlay(&gSoundDevice, SND_HAHAHA);
				// Reset to prevent last key from being processed as
				// normal player commands
				KeyInit(&gInputDevices.keyboard);
				cmdAll = 0;
				memset(cmds, 0, sizeof cmds);
			}
		}

		if (!isPaused)
		{
			if (!gConfig.Game.SlowMotion || (frames & 1) == 0)
			{
				HandleGameEvents(&gGameEvents, &shakeAmount);

				for (i = 0; i < gOptions.numPlayers; i++)
				{
					if (gPlayers[i])
					{
						if (gPlayerDatas[i].inputDevice == INPUT_DEVICE_AI)
						{
							cmds[i] = AICoopGetCmd(gPlayers[i]);
						}
						PlayerSpecialCommands(
							gPlayers[i], cmds[i], &gPlayerDatas[i]);
						CommandActor(gPlayers[i], cmds[i], ticks);
					}
				}

				if (gOptions.badGuys)
				{
					CommandBadGuys(ticks);
				}
				
				// If split screen never and players are too close to the
				// edge of the screen, forcefully pull them towards the center
				if (gConfig.Interface.Splitscreen == SPLITSCREEN_NEVER &&
					IsSingleScreen(
						&gGraphicsDevice.cachedConfig,
						gConfig.Interface.Splitscreen))
				{
					int w = gGraphicsDevice.cachedConfig.ResolutionWidth;
					int h = gGraphicsDevice.cachedConfig.ResolutionHeight;
					Vec2i screen = Vec2iAdd(
						PlayersGetMidpoint(gPlayers),
						Vec2iNew(-w / 2, -h / 2));
					for (i = 0; i < gOptions.numPlayers; i++)
					{
						TActor *p = gPlayers[i];
						int pad = SPLIT_PADDING;
						if (!IsPlayerAlive(i))
						{
							break;
						}
						if (screen.x + pad > p->tileItem.x)
						{
							p->dx = MAX(p->dx, 256);
						}
						if (screen.x + w - pad < p->tileItem.x)
						{
							p->dx = MIN(p->dx, -256);
						}
						if (screen.y + pad > p->tileItem.y)
						{
							p->dy = MAX(p->dy, 256);
						}
						if (screen.y + h - pad < p->tileItem.y)
						{
							p->dy = MIN(p->dy, -256);
						}
					}
				}
				
				UpdateAllActors(ticks);
				UpdateMobileObjects(&gMobObjList, ticks);

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

		// Check that all players have been destroyed
		// Note: there's a period of time where players are dying
		// Wait until after this period before ending the game
		for (i = 0; i < gOptions.numPlayers; i++)
		{
			if (gPlayers[i])
			{
				allPlayersDestroyed = 0;
				break;
			}
		}
		if (allPlayersDestroyed)
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
		if (GameIsMouseUsed(gPlayerDatas))
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
