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

    Copyright (c) 2013-2014, Cong Xu
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

#include <cdogs/actors.h>
#include <cdogs/ai.h>
#include <cdogs/ai_coop.h>
#include <cdogs/automap.h>
#include <cdogs/config.h>
#include <cdogs/draw.h>
#include <cdogs/events.h>
#include <cdogs/game_events.h>
#include <cdogs/health_pickup.h>
#include <cdogs/hud.h>
#include <cdogs/joystick.h>
#include <cdogs/mission.h>
#include <cdogs/music.h>
#include <cdogs/objs.h>
#include <cdogs/palette.h>
#include <cdogs/pic_manager.h>
#include <cdogs/pics.h>
#include <cdogs/screen_shake.h>
#include <cdogs/text.h>
#include <cdogs/triggers.h>

#include <cdogs/drawtools.h> /* for Draw_Box and Draw_Point */

#include "handle_game_events.h"

#define MAX_FRAMESKIP	(FPS_FRAMELIMIT / 5)

#define SPLIT_PADDING 40


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

	if ((cmd & CMD_BUTTON2) && isDirectionCmd)
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


static void DoBuffer(
	DrawBuffer *b, Vec2i center, int w, Vec2i noise, Vec2i offset)
{
	DrawBufferSetFromMap(b, &gMap, Vec2iAdd(center, noise), w);
	DrawBufferLOS(b, center);
	FixBuffer(b);
	DrawBufferDraw(b, offset, NULL);
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

Vec2i DrawScreen(DrawBuffer *b, Vec2i lastPosition, ScreenShake shake)
{
	Vec2i centerOffset = Vec2iZero();
	int i;
	int numPlayersAlive = GetNumPlayersAlive();
	int w = gGraphicsDevice.cachedConfig.ResolutionWidth;
	int h = gGraphicsDevice.cachedConfig.ResolutionHeight;

	for (i = 0; i < GraphicsGetScreenSize(&gGraphicsDevice.cachedConfig); i++)
	{
		gGraphicsDevice.buf[i] = PixelFromColor(&gGraphicsDevice, colorBlack);
	}

	Vec2i noise = ScreenShakeGetDelta(shake);

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
				b, &gMap, Vec2iAdd(lastPosition, noise), X_TILES);
			for (i = 0; i < MAX_PLAYERS; i++)
			{
				if (IsPlayerAlive(i))
				{
					DrawBufferLOS(
						b,
						Vec2iNew(
							gPlayers[i]->tileItem.x, gPlayers[i]->tileItem.y));
				}
			}
			FixBuffer(b);
			DrawBufferDraw(b, centerOffset, NULL);
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

static int HandleKey(int cmd, int *isPaused, int *hasUsedMap, bool showExit)
{
	if (IsAutoMapEnabled(gCampaign.Entry.mode))
	{
		int hasDisplayedAutomap = 0;
		while (KeyIsDown(
			&gEventHandlers.keyboard, gConfig.Input.PlayerKeys[0].Keys.map) ||
			(cmd & CMD_BUTTON3) != 0)
		{
			int i;
			*hasUsedMap = 1;
			if (!hasDisplayedAutomap)
			{
				AutomapDraw(0, showExit);
				BlitFlip(&gGraphicsDevice, &gConfig.Graphics);
				hasDisplayedAutomap = 1;
			}
			SDL_Delay(10);
			EventPoll(&gEventHandlers, SDL_GetTicks());
			cmd = 0;
			for (i = 0; i < MAX_PLAYERS; i++)
			{
				cmd |= GetGameCmd(
					&gEventHandlers, &gConfig.Input,
					&gPlayerDatas[i], Vec2iZero());
			}
		}
	}

	if (*isPaused && AnyButton(cmd))
	{
		*isPaused = 0;
	}

	if (KeyIsPressed(&gEventHandlers.keyboard, SDLK_ESCAPE) ||
		JoyIsPressed(&gEventHandlers.joysticks.joys[0], CMD_BUTTON4))
	{
		return 1;
	}
	else if (KeyGetPressed(&gEventHandlers.keyboard) ||
		JoyGetPressed(&gEventHandlers.joysticks.joys[0]))
	{
		*isPaused = 0;
	}

#ifndef RUN_WITHOUT_APP_FOCUS
	if (!*isPaused && !(SDL_GetAppState() & SDL_APPINPUTFOCUS))
	{
		*isPaused = 1;
	}	
#endif

	return 0;
}

Vec2i GetPlayerCenter(GraphicsDevice *device, DrawBuffer *b, int player)
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
	// Add draw buffer offset
	center = Vec2iMinus(center, Vec2iNew(b->dx, b->dy));
	return center;
}

static void MissionUpdateObjectives(struct MissionOptions *mo, Map *map);
int gameloop(void)
{
	DrawBuffer buffer;
	int is_esc_pressed = 0;
	int isPaused = 0;
	HUD hud;
	Vec2i lastPosition = Vec2iZero();
	Uint32 ticksNow;
	Uint32 ticksThen;
	Uint32 ticksElapsed = 0;
	Uint32 ticksElapsedDraw = 0;
	int frames = 0;
	int framesSkipped = 0;
	ScreenShake shake = ScreenShakeZero();
	HealthPickups hp;

	DrawBufferInit(&buffer, Vec2iNew(X_TILES, Y_TILES), &gGraphicsDevice);
	HUDInit(&hud, &gConfig.Interface, &gGraphicsDevice, &gMission);
	GameEventsInit(&gGameEvents);
	HealthPickupsInit(&hp, &gMap, gPlayers);

	if (MusicGetStatus(&gSoundDevice) != MUSIC_OK)
	{
		HUDDisplayMessage(&hud, MusicGetErrorMessage(&gSoundDevice), 140);
	}

	gMission.time = 0;
	gMobileObjId = 0;
	gMission.pickupTime = 0;
	gMission.state = MISSION_STATE_PLAY;
	gMission.isDone = false;
	Pic *crosshair = PicManagerGetPic(&gPicManager, "crosshair");
	crosshair->offset.x = -crosshair->size.x / 2;
	crosshair->offset.y = -crosshair->size.y / 2;
	EventReset(&gEventHandlers, crosshair);

	GameEvent start;
	start.Type = GAME_EVENT_GAME_START;
	GameEventsEnqueue(&gGameEvents, start);

	// Check if mission is done already
	MissionSetMessageIfComplete(&gMission);
	ticksNow = SDL_GetTicks();
	while (!gMission.isDone)
	{
		int cmds[MAX_PLAYERS];
		int cmdAll = 0;
		int ticks = 1;
		int i;
		int allPlayersDestroyed = 1;
		int hasUsedMap = 0;
		Uint32 ticksBeforeMap;
		ticksThen = ticksNow;
		ticksNow = SDL_GetTicks();
		ticksElapsed += ticksNow - ticksThen;
		ticksElapsedDraw += ticksNow - ticksThen;
		if (ticksElapsed < 1000 / FPS_FRAMELIMIT)
		{
			SDL_Delay(1);
			debug(D_VERBOSE, "Delaying 1 ticksNow %u elapsed %u\n", ticksNow, ticksElapsed);
			continue;
		}

#ifndef RUN_WITHOUT_APP_FOCUS
		MusicSetPlaying(&gSoundDevice, SDL_GetAppState() & SDL_APPINPUTFOCUS);
#endif
		// Only process input every 2 frames
		if ((frames & 1) == 0)
		{
			EventPoll(&gEventHandlers, ticksNow);
			for (i = 0; i < MAX_PLAYERS; i++)
			{
				if (IsPlayerAlive(i))
				{
					cmds[i] = GetGameCmd(
						&gEventHandlers,
						&gConfig.Input,
						&gPlayerDatas[i],
						GetPlayerCenter(&gGraphicsDevice, &buffer, i));
					cmdAll |= cmds[i];
				}
			}
		}
		ticksBeforeMap = SDL_GetTicks();
		is_esc_pressed = HandleKey(
			cmdAll, &isPaused, &hasUsedMap, hud.showExit);
		if (is_esc_pressed)
		{
			if (isPaused)
			{
				GameEvent e;
				e.Type = GAME_EVENT_MISSION_END;
				GameEventsEnqueue(&gGameEvents, e);
				// Also explicitly set done
				// otherwise game will not quit immediately
				gMission.isDone = true;
			}
			else
			{
				isPaused = 1;
			}
		}
		if (hasUsedMap)
		{
			// Map keeps the game paused, reset the time elapsed counter
			ticksNow += SDL_GetTicks() - ticksBeforeMap;
		}
		// Cheat: special weapon activation
		if (IsPlayerAlive(0))
		{
			const char *pulserifle = "sgodc";
			const char *heatseeker = "miaon";
			int isMatch = 1;
			for (i = 0; i < (int)strlen(pulserifle); i++)
			{
				if (gEventHandlers.keyboard.pressedKeysBuffer[i] !=
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
				KeyInit(&gEventHandlers.keyboard);
				cmdAll = 0;
				memset(cmds, 0, sizeof cmds);
			}
			isMatch = 1;
			for (i = 0; i < (int)strlen(heatseeker); i++)
			{
				if (gEventHandlers.keyboard.pressedKeysBuffer[i] !=
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
				KeyInit(&gEventHandlers.keyboard);
				cmdAll = 0;
				memset(cmds, 0, sizeof cmds);
			}
		}

		if (!isPaused)
		{
			if (!gConfig.Game.SlowMotion || (frames & 1) == 0)
			{
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

				UpdateWatches(&gMap.triggers);

				HealthPickupsUpdate(&hp, ticks);

				bool isMissionComplete =
					GetNumPlayersAlive() > 0 && IsMissionComplete(&gMission);
				if (gMission.state == MISSION_STATE_PLAY && isMissionComplete)
				{
					GameEvent e;
					e.Type = GAME_EVENT_MISSION_PICKUP;
					GameEventsEnqueue(&gGameEvents, e);
				}
				if (gMission.state == MISSION_STATE_PICKUP &&
					!isMissionComplete)
				{
					GameEvent e;
					e.Type = GAME_EVENT_MISSION_INCOMPLETE;
					GameEventsEnqueue(&gGameEvents, e);
				}
				if (gMission.state == MISSION_STATE_PICKUP &&
					gMission.pickupTime + PICKUP_LIMIT <= gMission.time)
				{
					GameEvent e;
					e.Type = GAME_EVENT_MISSION_END;
					GameEventsEnqueue(&gGameEvents, e);
				}

				HandleGameEvents(
					&gGameEvents, &hud, &shake, &hp, &gEventHandlers);
			}

			gMission.time += ticks;

			if (HasObjectives(gCampaign.Entry.mode))
			{
				MissionUpdateObjectives(&gMission, &gMap);
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
				GameEvent e;
				e.Type = GAME_EVENT_MISSION_END;
				GameEventsEnqueue(&gGameEvents, e);
			}
		}

		ticksElapsed -= 1000 / FPS_FRAMELIMIT;
		frames++;
		if (frames > FPS_FRAMELIMIT)
		{
			frames = 0;
		}
		// frame skip
		if (ticksElapsed > 1000 / FPS_FRAMELIMIT &&
			framesSkipped < MAX_FRAMESKIP)
		{
			framesSkipped++;
			continue;
		}
		framesSkipped = 0;

		// Don't update HUD if paused, only draw
		if (!isPaused)
		{
			shake = ScreenShakeUpdate(shake, ticks);

			HUDUpdate(&hud, ticksElapsedDraw);
		}

		lastPosition = DrawScreen(&buffer, lastPosition, shake);

		debug(D_VERBOSE, "frames... %d\n", frames);

		HUDDraw(&hud, isPaused);
		if (GameIsMouseUsed(gPlayerDatas))
		{
			MouseDraw(&gEventHandlers.mouse);
		}

		BlitFlip(&gGraphicsDevice, &gConfig.Graphics);

		ticksElapsedDraw = 0;
	}
	GameEventsTerminate(&gGameEvents);
	HUDTerminate(&hud);
	DrawBufferTerminate(&buffer);

	return
		gMission.state == MISSION_STATE_PICKUP &&
		gMission.pickupTime + PICKUP_LIMIT <= gMission.time;
}

static void MissionUpdateObjectives(struct MissionOptions *mo, Map *map)
{
	for (int i = 0; i < (int)mo->missionData->Objectives.size; i++)
	{
		MissionObjective *mobj = CArrayGet(&mo->missionData->Objectives, i);
		struct Objective *o = CArrayGet(&mo->Objectives, i);
		if (mobj->Type == OBJECTIVE_INVESTIGATE)
		{
			o->done = MapGetExploredPercentage(map);
			MissionSetMessageIfComplete(mo);
		}
	}
}
