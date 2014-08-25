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
#include <cdogs/particle.h>
#include <cdogs/pic_manager.h>
#include <cdogs/pics.h>
#include <cdogs/screen_shake.h>
#include <cdogs/triggers.h>

#include <cdogs/drawtools.h> /* for Draw_Line */

#include "handle_game_events.h"

#define SPLIT_PADDING 40

static void PlayerSpecialCommands(TActor *actor, const int cmd)
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
		ActorTrySwitchGun(actor))
	{
		SoundPlayAt(
			&gSoundDevice,
			gSoundDevice.switchSound,
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
	PlayersGetBoundingRectangle(&min, &max);
	return
		max.x - min.x < config->Res.x - SPLIT_PADDING &&
		max.y - min.y < config->Res.y - SPLIT_PADDING;
}

Vec2i DrawScreen(DrawBuffer *b, Vec2i lastPosition, ScreenShake shake)
{
	Vec2i centerOffset = Vec2iZero();
	int i;
	int numPlayersAlive = GetNumPlayersAlive();
	int w = gGraphicsDevice.cachedConfig.Res.x;
	int h = gGraphicsDevice.cachedConfig.Res.y;

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
			lastPosition = PlayersGetMidpoint();

			DrawBufferSetFromMap(
				b, &gMap, Vec2iAdd(lastPosition, noise), X_TILES);
			for (i = 0; i < MAX_PLAYERS; i++)
			{
				if (IsPlayerAlive(i))
				{
					TActor *player = CArrayGet(&gActors, gPlayerIds[i]);
					DrawBufferLOS(
						b, Vec2iNew(player->tileItem.x, player->tileItem.y));
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
				TActor *player = CArrayGet(&gActors, gPlayerIds[i]);
				Vec2i center = Vec2iNew(
					player->tileItem.x, player->tileItem.y);
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
				TActor *player = CArrayGet(&gActors, i);
				center = Vec2iNew(player->tileItem.x, player->tileItem.y);
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

Vec2i GetPlayerCenter(GraphicsDevice *device, DrawBuffer *b, int player)
{
	Vec2i center = Vec2iZero();
	int w = device->cachedConfig.Res.x;
	int h = device->cachedConfig.Res.y;

	if (GetNumPlayersAlive() == 1 ||
		IsSingleScreen(
			&device->cachedConfig,
			gConfig.Interface.Splitscreen))
	{
		Vec2i pCenter = PlayersGetMidpoint();
		Vec2i screenCenter = Vec2iNew(w / 2, device->cachedConfig.Res.y / 2);
		TActor *actor = CArrayGet(&gActors, gPlayerIds[player]);
		TTileItem *pTileItem = &actor->tileItem;
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
typedef struct
{
	struct MissionOptions *m;
	Map *map;
	DrawBuffer buffer;
	HUD hud;
	int frames;
	// TODO: turn the following into a screen system?
	bool isPaused;
	bool isMap;
	int cmds[MAX_PLAYERS];
	HealthPickups hp;
	ScreenShake shake;
	Vec2i lastPosition;
	GameLoopData loop;
} RunGameData;
static void RunGameInput(void *data);
static GameLoopResult RunGameUpdate(void *data);
static void RunGameDraw(void *data);
bool RunGame(struct MissionOptions *m, Map *map)
{
	RunGameData data;
	memset(&data, 0, sizeof data);
	data.m = m;
	data.map = map;
	data.lastPosition = Vec2iZero();

	DrawBufferInit(&data.buffer, Vec2iNew(X_TILES, Y_TILES), &gGraphicsDevice);
	HUDInit(&data.hud, &gConfig.Interface, &gGraphicsDevice, m);
	HealthPickupsInit(&data.hp, map);
	GameEventsInit(&gGameEvents);
	data.shake = ScreenShakeZero();

	if (MusicGetStatus(&gSoundDevice) != MUSIC_OK)
	{
		HUDDisplayMessage(&data.hud, MusicGetErrorMessage(&gSoundDevice), 140);
	}

	m->time = 0;
	m->pickupTime = 0;
	m->state = MISSION_STATE_PLAY;
	m->isDone = false;
	Pic *crosshair = PicManagerGetPic(&gPicManager, "crosshair");
	crosshair->offset.x = -crosshair->size.x / 2;
	crosshair->offset.y = -crosshair->size.y / 2;
	EventReset(&gEventHandlers, crosshair);

	GameEvent start;
	start.Type = GAME_EVENT_GAME_START;
	GameEventsEnqueue(&gGameEvents, start);

	data.loop = GameLoopDataNew(
		&data, RunGameUpdate, &data, RunGameDraw);
	data.loop.InputData = &data;
	data.loop.InputFunc = RunGameInput;
	data.loop.FPS = FPS_FRAMELIMIT;
	data.loop.InputEverySecondFrame = true;
	GameLoop(&data.loop);

	GameEventsTerminate(&gGameEvents);
	HUDTerminate(&data.hud);
	DrawBufferTerminate(&data.buffer);

	return
		m->state == MISSION_STATE_PICKUP &&
		m->pickupTime + PICKUP_LIMIT <= m->time;
}
static void RunGameInput(void *data)
{
	RunGameData *rData = data;

	if (gEventHandlers.HasQuit)
	{
		rData->m->isDone = true;
		return;
	}

	memset(rData->cmds, 0, sizeof rData->cmds);
	int cmdAll = 0;
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		if (!IsPlayerAlive(i))
		{
			continue;
		}
		rData->cmds[i] = GetGameCmd(
			&gEventHandlers,
			&gConfig.Input,
			&gPlayerDatas[i],
			GetPlayerCenter(&gGraphicsDevice, &rData->buffer, i));
		cmdAll |= rData->cmds[i];
	}

	// Check if automap key is pressed by any player
	rData->isMap =
		IsAutoMapEnabled(gCampaign.Entry.Mode) &&
		(KeyIsDown(&gEventHandlers.keyboard, gConfig.Input.PlayerKeys[0].Keys.map) ||
		(cmdAll & CMD_BUTTON3));

	// Check if escape was pressed
	// If the game was not paused, enter pause mode
	// If the game was paused, exit the game
	if (AnyButton(cmdAll))
	{
		rData->isPaused = false;
	}
	else if (KeyIsPressed(&gEventHandlers.keyboard, SDLK_ESCAPE) ||
		JoyIsPressed(&gEventHandlers.joysticks.joys[0], CMD_BUTTON4))
	{
		// Escape pressed
		if (rData->isPaused)
		{
			// Exit
			GameEvent e;
			e.Type = GAME_EVENT_MISSION_END;
			GameEventsEnqueue(&gGameEvents, e);
			// Also explicitly set done
			// otherwise game will not quit immediately
			rData->m->isDone = true;
		}
		else
		{
			// Pause the game
			rData->isPaused = true;
		}
	}
}
static GameLoopResult RunGameUpdate(void *data)
{
	RunGameData *rData = data;

	// Detect exit
	if (rData->m->isDone)
	{
		return UPDATE_RESULT_EXIT;
	}

	// Don't update if the game has paused or has automap shown
	if (rData->isPaused || rData->isMap)
	{
		return UPDATE_RESULT_DRAW;
	}

	// If slow motion, update every other frame
	if (gConfig.Game.SlowMotion && (rData->loop.Frames & 1))
	{
		return UPDATE_RESULT_OK;
	}

	// Update all the things in the game
	const int ticksPerFrame = 1;

	for (int i = 0; i < gOptions.numPlayers; i++)
	{
		if (!IsPlayerAlive(i))
		{
			continue;
		}
		TActor *player = CArrayGet(&gActors, gPlayerIds[i]);
		if (gPlayerDatas[i].inputDevice == INPUT_DEVICE_AI)
		{
			rData->cmds[i] = AICoopGetCmd(player, ticksPerFrame);
		}
		PlayerSpecialCommands(player, rData->cmds[i]);
		CommandActor(player, rData->cmds[i], ticksPerFrame);
	}

	if (gOptions.badGuys)
	{
		CommandBadGuys(ticksPerFrame);
	}

	// If split screen never and players are too close to the
	// edge of the screen, forcefully pull them towards the center
	if (gConfig.Interface.Splitscreen == SPLITSCREEN_NEVER &&
		IsSingleScreen(
		&gGraphicsDevice.cachedConfig,
		gConfig.Interface.Splitscreen))
	{
		const int w = gGraphicsDevice.cachedConfig.Res.x;
		const int h = gGraphicsDevice.cachedConfig.Res.y;
		const Vec2i screen = Vec2iAdd(
			PlayersGetMidpoint(), Vec2iNew(-w / 2, -h / 2));
		for (int i = 0; i < gOptions.numPlayers; i++)
		{
			if (!IsPlayerAlive(i))
			{
				continue;
			}
			const TActor *p = CArrayGet(&gActors, gPlayerIds[i]);
			const int pad = SPLIT_PADDING;
			GameEvent ei;
			ei.Type = GAME_EVENT_ACTOR_IMPULSE;
			ei.u.ActorImpulse.Id = p->tileItem.id;
			ei.u.ActorImpulse.Vel = p->Vel;
			if (screen.x + pad > p->tileItem.x && p->Vel.x < 256)
			{
				ei.u.ActorImpulse.Vel.x = 256 - p->Vel.x;
			}
			else if (screen.x + w - pad < p->tileItem.x && p->Vel.x > -256)
			{
				ei.u.ActorImpulse.Vel.x = -256 - p->Vel.x;
			}
			if (screen.y + pad > p->tileItem.y && p->Vel.y < 256)
			{
				ei.u.ActorImpulse.Vel.y = 256 - p->Vel.y;
			}
			else if (screen.y + h - pad < p->tileItem.y && p->Vel.y > -256)
			{
				ei.u.ActorImpulse.Vel.y = -256 - p->Vel.y;
			}
			if (!Vec2iEqual(ei.u.ActorImpulse.Vel, p->Vel))
			{
				GameEventsEnqueue(&gGameEvents, ei);
			}
		}
	}

	UpdateAllActors(ticksPerFrame);
	UpdateMobileObjects(ticksPerFrame);
	ParticlesUpdate(&gParticles, ticksPerFrame);

	UpdateWatches(&rData->map->triggers);

	HealthPickupsUpdate(&rData->hp, ticksPerFrame);

	const bool isMissionComplete =
		GetNumPlayersAlive() > 0 && IsMissionComplete(rData->m);
	if (rData->m->state == MISSION_STATE_PLAY && isMissionComplete)
	{
		GameEvent e;
		e.Type = GAME_EVENT_MISSION_PICKUP;
		GameEventsEnqueue(&gGameEvents, e);
	}
	if (rData->m->state == MISSION_STATE_PICKUP && !isMissionComplete)
	{
		GameEvent e;
		e.Type = GAME_EVENT_MISSION_INCOMPLETE;
		GameEventsEnqueue(&gGameEvents, e);
	}
	if (rData->m->state == MISSION_STATE_PICKUP &&
		rData->m->pickupTime + PICKUP_LIMIT <= rData->m->time)
	{
		GameEvent e;
		e.Type = GAME_EVENT_MISSION_END;
		GameEventsEnqueue(&gGameEvents, e);
	}

	HandleGameEvents(
		&gGameEvents, &rData->hud, &rData->shake, &rData->hp, &gEventHandlers);

	rData->m->time += ticksPerFrame;

	if (HasObjectives(gCampaign.Entry.Mode))
	{
		MissionUpdateObjectives(rData->m, rData->map);
	}

	// Check that all players have been destroyed
	// Note: there's a period of time where players are dying
	// Wait until after this period before ending the game
	bool allPlayersDestroyed = true;
	for (int i = 0; i < gOptions.numPlayers; i++)
	{
		if (gPlayerIds[i] != -1)
		{
			allPlayersDestroyed = false;
			break;
		}
	}
	if (allPlayersDestroyed)
	{
		GameEvent e;
		e.Type = GAME_EVENT_MISSION_END;
		GameEventsEnqueue(&gGameEvents, e);
	}

	rData->shake = ScreenShakeUpdate(rData->shake, ticksPerFrame);

	HUDUpdate(&rData->hud, 1000 / rData->loop.FPS);

	return UPDATE_RESULT_DRAW;
}
static void RunGameDraw(void *data)
{
	RunGameData *rData = data;

	// Draw everything
	rData->lastPosition =
		DrawScreen(&rData->buffer, rData->lastPosition, rData->shake);

	HUDDraw(&rData->hud, rData->isPaused);
	if (GameIsMouseUsed(gPlayerDatas))
	{
		MouseDraw(&gEventHandlers.mouse);
	}

	// Draw automap if enabled
	if (rData->isMap)
	{
		AutomapDraw(0, rData->hud.showExit);
	}
}

static void MissionUpdateObjectives(struct MissionOptions *mo, Map *map)
{
	for (int i = 0; i < (int)mo->missionData->Objectives.size; i++)
	{
		MissionObjective *mobj = CArrayGet(&mo->missionData->Objectives, i);
		struct Objective *o = CArrayGet(&mo->Objectives, i);
		if (mobj->Type == OBJECTIVE_INVESTIGATE)
		{
			int update = MapGetExploredPercentage(map) - o->done;
			if (update > 0)
			{
				GameEvent e;
				e.Type = GAME_EVENT_UPDATE_OBJECTIVE;
				e.u.UpdateObjective.ObjectiveIndex = i;
				e.u.UpdateObjective.Update = update;
				GameEventsEnqueue(&gGameEvents, e);
			}
		}
	}
}
