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

    Copyright (c) 2013-2017, Cong Xu
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

#include <cdogs/actor_placement.h>
#include <cdogs/actors.h>
#include <cdogs/ai.h>
#include <cdogs/ai_coop.h>
#include <cdogs/automap.h>
#include <cdogs/camera.h>
#include <cdogs/draw/drawtools.h>
#include <cdogs/events.h>
#include <cdogs/grafx_bg.h>
#include <cdogs/handle_game_events.h>
#include <cdogs/log.h>
#include <cdogs/los.h>
#include <cdogs/music.h>
#include <cdogs/net_client.h>
#include <cdogs/net_server.h>
#include <cdogs/objs.h>

#include "briefing_screens.h"
#include "hiscores.h"
#include "screens_end.h"


static void PlayerSpecialCommands(TActor *actor, const int cmd)
{
	if ((cmd & CMD_BUTTON2) && CMD_HAS_DIRECTION(cmd))
	{
		if (ConfigGetEnum(&gConfig, "Game.SwitchMoveStyle") == SWITCHMOVE_SLIDE)
		{
			SlideActor(actor, cmd);
		}
	}
	else if (
		(actor->lastCmd & CMD_BUTTON2) &&
		!(cmd & CMD_BUTTON2) &&
		!actor->specialCmdDir &&
		!actor->CanPickupSpecial &&
		!(ConfigGetEnum(&gConfig, "Game.SwitchMoveStyle") == SWITCHMOVE_SLIDE && CMD_HAS_DIRECTION(cmd)) &&
		ActorCanSwitchGun(actor))
	{
		GameEvent e = GameEventNew(GAME_EVENT_ACTOR_SWITCH_GUN);
		e.u.ActorSwitchGun.UID = actor->uid;
		e.u.ActorSwitchGun.GunIdx = (actor->gunIndex + 1) % actor->guns.size;
		GameEventsEnqueue(&gGameEvents, e);
	}
}


// TODO: reimplement in camera
struct vec2i GetPlayerCenter(
	GraphicsDevice *device, const Camera *camera,
	const PlayerData *pData, const int playerIdx)
{
	if (pData->ActorUID < 0)
	{
		// Player is dead
		return svec2i_zero();
	}
	struct vec2i center = svec2i_zero();
	int w = device->cachedConfig.Res.x;
	int h = device->cachedConfig.Res.y;

	if (GetNumPlayers(PLAYER_ANY, true, true) == 1 ||
		GetNumPlayers(PLAYER_ANY, false , true) == 1 ||
		CameraIsSingleScreen())
	{
		const struct vec2 pCenter = camera->lastPosition;
		const struct vec2i screenCenter =
			svec2i(w / 2, device->cachedConfig.Res.y / 2);
		const TActor *actor = ActorGetByUID(pData->ActorUID);
		const struct vec2 p = actor->tileItem.Pos;
		center = svec2i_add(
			svec2i_assign_vec2(svec2_subtract(p, pCenter)), screenCenter);
	}
	else
	{
		const int numLocalPlayers = GetNumPlayers(PLAYER_ANY, false, true);
		if (numLocalPlayers == 2)
		{
			center.x = playerIdx == 0 ? w / 4 : w * 3 / 4;
			center.y = h / 2;
		}
		else if (numLocalPlayers >= 3 && numLocalPlayers <= 4)
		{
			center.x = (playerIdx & 1) ? w * 3 / 4 : w / 4;
			center.y = (playerIdx >= 2) ? h * 3 / 4 : h / 4;
		}
		else
		{
			CASSERT(false, "invalid number of players");
		}
	}
	return center;
}

typedef struct
{
	const CampaignOptions *co;
	struct MissionOptions *m;
	Map *map;
	Camera Camera;
	int frames;
	// TODO: turn the following into a screen system?
	input_device_e pausingDevice;	// INPUT_DEVICE_UNSET if not paused
	bool controllerUnplugged;
	bool isMap;
	int cmds[MAX_LOCAL_PLAYERS];
	int lastCmds[MAX_LOCAL_PLAYERS];
	// Only update AI every 4 ticks
	int aiUpdateCounter;
	PowerupSpawner healthSpawner;
	CArray ammoSpawners;	// of PowerupSpawner
} RunGameData;
static void RunGameTerminate(GameLoopData *data);
static void RunGameOnEnter(GameLoopData *data);
static void RunGameOnExit(GameLoopData *data);
static void RunGameInput(GameLoopData *data);
static GameLoopResult RunGameUpdate(GameLoopData *data, LoopRunner *l);
static void RunGameDraw(GameLoopData *data);
GameLoopData *RunGame(
	const CampaignOptions *co, struct MissionOptions *m, Map *map)
{
	RunGameData *data;
	CCALLOC(data, sizeof *data);
	data->co = co;
	data->m = m;
	data->map = map;
	GameLoopData *g = GameLoopDataNew(
		data, RunGameTerminate, RunGameOnEnter, RunGameOnExit,
		RunGameInput, RunGameUpdate, RunGameDraw);
	g->FPS = ConfigGetInt(&gConfig, "Game.FPS");
	g->InputEverySecondFrame = true;
	return g;
}
static void RunGameTerminate(GameLoopData *data)
{
	RunGameData *rData = data->Data;

	CFREE(rData);
}
static void RunGameOnEnter(GameLoopData *data)
{
	RunGameData *rData = data->Data;

	// Clear the background
	DrawRectangle(
		&gGraphicsDevice, svec2i_zero(), gGraphicsDevice.cachedConfig.Res,
		colorBlack, 0);
	BlitUpdateFromBuf(&gGraphicsDevice, gGraphicsDevice.bkg);

	MapLoad(rData->map, rData->m, rData->co);

	// Seed random if PVP mode (otherwise players will always spawn in same
	// position)
	if (IsPVP(rData->co->Entry.Mode))
	{
		srand((unsigned int)time(NULL));
	}

	if (!rData->co->IsClient)
	{
		MapLoadDynamic(rData->map, rData->m, &rData->co->Setting.characters);

		// For PVP modes, mark all map as explored
		if (IsPVP(rData->co->Entry.Mode))
		{
			MapMarkAllAsVisited(rData->map);
		}

		// Reset players for the mission
		CA_FOREACH(const PlayerData, p, gPlayerDatas)
			// Only reset for local players; for remote ones wait for the
			// client ready message
			if (!p->IsLocal) continue;
			GameEvent e = GameEventNew(GAME_EVENT_PLAYER_DATA);
			e.u.PlayerData = PlayerDataMissionReset(p);
			GameEventsEnqueue(&gGameEvents, e);
		CA_FOREACH_END()
		// Process the events to force add the players
		HandleGameEvents(&gGameEvents, NULL, NULL, NULL);

		// Note: place players first,
		// as bad guys are placed away from players
		struct vec2 firstPos = svec2_zero();
		CA_FOREACH(const PlayerData, p, gPlayerDatas)
			if (!p->Ready) continue;
			firstPos = PlacePlayer(&gMap, p, firstPos, true);
		CA_FOREACH_END()
		if (!IsPVP(rData->co->Entry.Mode))
		{
			InitializeBadGuys();
			CreateEnemies();
		}
	}

	CameraInit(&rData->Camera);
	// If there are no players, show the full map before starting
	if (GetNumPlayers(PLAYER_ANY, false, true) == 0)
	{
		LOSSetAllVisible(&rData->map->LOS);
		rData->Camera.lastPosition =
			Vec2CenterOfTile(svec2i_scale_divide(rData->map->Size, 2));
		rData->Camera.FollowNextPlayer = true;
	}
	HealthSpawnerInit(&rData->healthSpawner, rData->map);
	CArrayInit(&rData->ammoSpawners, sizeof(PowerupSpawner));
	for (int i = 0; i < AmmoGetNumClasses(&gAmmo); i++)
	{
		PowerupSpawner ps;
		AmmoSpawnerInit(&ps, rData->map, i);
		CArrayPushBack(&rData->ammoSpawners, &ps);
	}

	rData->m->state = MISSION_STATE_WAITING;
	rData->m->isDone = false;
	rData->m->DoneCounter = 0;
	Pic *crosshair = PicManagerGetPic(&gPicManager, "crosshair");
	crosshair->offset.x = -crosshair->size.x / 2;
	crosshair->offset.y = -crosshair->size.y / 2;
	EventReset(
		&gEventHandlers, crosshair,
		PicManagerGetPic(&gPicManager, "crosshair_trail"));

	NetServerSendGameStartMessages(&gNetServer, NET_SERVER_BCAST);
	GameEvent start = GameEventNew(GAME_EVENT_GAME_START);
	GameEventsEnqueue(&gGameEvents, start);
}
static void RunGameOnExit(GameLoopData *data)
{
	RunGameData *rData = data->Data;

	LOG(LM_MAIN, LL_INFO, "Game finished");

	// Flush events
	HandleGameEvents(&gGameEvents, NULL, NULL, NULL);

	PowerupSpawnerTerminate(&rData->healthSpawner);
	CA_FOREACH(PowerupSpawner, a, rData->ammoSpawners)
		PowerupSpawnerTerminate(a);
	CA_FOREACH_END()
	CArrayTerminate(&rData->ammoSpawners);
	CameraTerminate(&rData->Camera);

	// Draw background
	GrafxRedrawBackground(&gGraphicsDevice, rData->Camera.lastPosition);
	// Clear other texures
	BlitClearBuf(&gGraphicsDevice);
	BlitUpdateFromBuf(&gGraphicsDevice, gGraphicsDevice.hud);
	if (gGraphicsDevice.cachedConfig.SecondWindow)
	{
		BlitUpdateFromBuf(&gGraphicsDevice, gGraphicsDevice.hud2);
	}

	// Unready all the players
	CA_FOREACH(PlayerData, p, gPlayerDatas)
		p->Ready = false;
	CA_FOREACH_END()
	gNetClient.Ready = false;

	// Calculate remaining health and survived
	CA_FOREACH(PlayerData, p, gPlayerDatas)
		p->survived = IsPlayerAlive(p);
		if (IsPlayerAlive(p))
		{
			const TActor *player = ActorGetByUID(p->ActorUID);
			p->hp = player->health;
		}
	CA_FOREACH_END()

	MusicPlayMenu(&gSoundDevice);
}
static void RunGameInput(GameLoopData *data)
{
	RunGameData *rData = data->Data;

	if (gEventHandlers.HasQuit)
	{
		GameEvent e = GameEventNew(GAME_EVENT_MISSION_END);
		e.u.MissionEnd.IsQuit = true;
		GameEventsEnqueue(&gGameEvents, e);
		return;
	}

	int lastCmdAll = 0;
	for (int i = 0; i < MAX_LOCAL_PLAYERS; i++)
	{
		rData->lastCmds[i] = rData->cmds[i];
		lastCmdAll |= rData->lastCmds[i];
	}
	memset(rData->cmds, 0, sizeof rData->cmds);
	int cmdAll = 0;
	int idx = 0;
	input_device_e pausingDevice = INPUT_DEVICE_UNSET;
	input_device_e firstPausingDevice = INPUT_DEVICE_UNSET;
	if (GetNumPlayers(PLAYER_ANY, false, true) == 0)
	{
		// If no players, allow default keyboard to control camera
		rData->cmds[0] = GetKeyboardCmd(
			&gEventHandlers.keyboard, 0, false);
		firstPausingDevice = INPUT_DEVICE_KEYBOARD;
	}
	else
	{
		for (int i = 0; i < (int)gPlayerDatas.size; i++, idx++)
		{
			const PlayerData *p = CArrayGet(&gPlayerDatas, i);
			if (!p->IsLocal)
			{
				idx--;
				continue;
			}
			if (firstPausingDevice == INPUT_DEVICE_UNSET)
			{
				firstPausingDevice = p->inputDevice;
			}
			rData->cmds[idx] = GetGameCmd(
				&gEventHandlers,
				p,
				GetPlayerCenter(&gGraphicsDevice, &rData->Camera, p, idx));
			cmdAll |= rData->cmds[idx];

			// Only allow the first player to escape
			// Use keypress otherwise the player will quit immediately
			if (idx == 0 &&
				(rData->cmds[idx] & CMD_ESC) && !(rData->lastCmds[idx] & CMD_ESC))
			{
				pausingDevice = p->inputDevice;
			}
		}
	}
	if (KeyIsPressed(&gEventHandlers.keyboard, SDL_SCANCODE_ESCAPE))
	{
		pausingDevice = INPUT_DEVICE_KEYBOARD;
	}

	// Check if any controllers are unplugged
	rData->controllerUnplugged = false;
	CA_FOREACH(const PlayerData, p, gPlayerDatas)
		if (p->inputDevice == INPUT_DEVICE_UNSET && p->IsLocal)
		{
			rData->controllerUnplugged = true;
			break;
		}
	CA_FOREACH_END()

	// Check if:
	// - escape was pressed, or
	// - window lost focus
	// - controller unplugged
	// If the game was not paused, enter pause mode
	// If the game was paused and escape was pressed, exit the game
	if (AnyButton(cmdAll))
	{
		rData->pausingDevice = INPUT_DEVICE_UNSET;
	}
	else if (rData->controllerUnplugged || gEventHandlers.HasLostFocus)
	{
		// Pause the game
		rData->pausingDevice = firstPausingDevice;
		rData->isMap = false;
	}
	else if (pausingDevice != INPUT_DEVICE_UNSET)
	{
		if (rData->pausingDevice != INPUT_DEVICE_UNSET)
		{
			// Already paused; exit
			GameEvent e = GameEventNew(GAME_EVENT_MISSION_END);
			e.u.MissionEnd.IsQuit = true;
			GameEventsEnqueue(&gGameEvents, e);
			// Need to unpause to process the quit
			rData->pausingDevice = INPUT_DEVICE_UNSET;
			rData->controllerUnplugged = false;
		}
		else
		{
			// Pause the game
			rData->pausingDevice = pausingDevice;
			rData->isMap = false;
		}
	}

	const bool paused =
		rData->pausingDevice != INPUT_DEVICE_UNSET ||
		rData->controllerUnplugged;
	if (!paused)
	{
		// Check if automap key is pressed by any player
		// Toggle
		if (IsAutoMapEnabled(gCampaign.Entry.Mode) &&
			(KeyIsPressed(&gEventHandlers.keyboard, ConfigGetInt(&gConfig, "Input.PlayerCodes0.map")) ||
			((cmdAll & CMD_MAP) && !(lastCmdAll & CMD_MAP))))
		{
			rData->isMap = !rData->isMap;
			SoundPlay(
				&gSoundDevice,
				StrSound(rData->isMap ? "map_open" : "map_close"));
		}
	}

	CameraInput(&rData->Camera, rData->cmds[0], rData->lastCmds[0]);
}
static void NextLoop(RunGameData *rData, LoopRunner *l);
static void CheckMissionCompletion(const struct MissionOptions *mo);
static GameLoopResult RunGameUpdate(GameLoopData *data, LoopRunner *l)
{
	RunGameData *rData = data->Data;

	// Detect exit
	if (rData->m->isDone)
	{
		rData->m->DoneCounter--;
		if (rData->m->DoneCounter <= 0)
		{
			NextLoop(rData, l);
			return UPDATE_RESULT_OK;
		}
		else
		{
			return UPDATE_RESULT_DRAW;
		}
	}

	// Check if game can begin
	if (!rData->m->HasBegun && MissionCanBegin())
	{
		GameEvent begin = GameEventNew(GAME_EVENT_GAME_BEGIN);
		begin.u.GameBegin.MissionTime = gMission.time;
		GameEventsEnqueue(&gGameEvents, begin);
	}

	// Set mission complete and display exit if it is complete
	MissionSetMessageIfComplete(rData->m);

	// If we're not hosting a net game,
	// don't update if the game has paused or has automap shown
	// Important: don't consider paused if we are trying to quit
	const bool paused =
		rData->pausingDevice != INPUT_DEVICE_UNSET ||
		rData->controllerUnplugged ||
		rData->isMap;
	if (!gCampaign.IsClient &&
		!ConfigGetBool(&gConfig, "StartServer") &&
		paused &&
		!gEventHandlers.HasQuit)
	{
		return UPDATE_RESULT_DRAW;
	}

	// Update all the things in the game
	const int ticksPerFrame = 1;

	if (gPlayerDatas.size > 0)
	{
		LOSReset(&gMap.LOS);
		for (int i = 0, idx = 0; i < (int)gPlayerDatas.size; i++, idx++)
		{
			const PlayerData *p = CArrayGet(&gPlayerDatas, i);
			if (p->ActorUID == -1) continue;
			TActor *player = ActorGetByUID(p->ActorUID);
			if (player->dead > DEATH_MAX) continue;
			// Calculate LOS for all players alive or dying
			LOSCalcFrom(
				&gMap, Vec2ToTile(player->tileItem.Pos), !gCampaign.IsClient);

			if (player->dead) continue;

			// Only handle inputs/commands for local players
			if (!p->IsLocal)
			{
				idx--;
				continue;
			}
			if (p->inputDevice == INPUT_DEVICE_AI)
			{
				rData->cmds[idx] = AICoopGetCmd(player, ticksPerFrame);
			}
			PlayerSpecialCommands(player, rData->cmds[idx]);
			CommandActor(player, rData->cmds[idx], ticksPerFrame);
		}
	}

	if (!gCampaign.IsClient)
	{
		rData->aiUpdateCounter -= ticksPerFrame;
		if (rData->aiUpdateCounter <= 0)
		{
			const int enemies = AICommand(ticksPerFrame);
			AIAddRandomEnemies(enemies, rData->m->missionData);
			rData->aiUpdateCounter = 4;
		}
		else
		{
			AICommandLast(ticksPerFrame);
		}
	}

	// If split screen never and players are too close to the
	// edge of the screen, forcefully pull them towards the center
	if (ConfigGetEnum(&gConfig, "Interface.Splitscreen") == SPLITSCREEN_NEVER &&
		GetNumPlayers(PLAYER_ALIVE_OR_DYING, true, true) > 1 &&
		!IsPVP(gCampaign.Entry.Mode))
	{
		const int w = gGraphicsDevice.cachedConfig.Res.x;
		const int h = gGraphicsDevice.cachedConfig.Res.y;
		const struct vec2i screen = svec2i_add(
			svec2i_assign_vec2(PlayersGetMidpoint()), svec2i(-w / 2, -h / 2));
		CA_FOREACH(const PlayerData, pd, gPlayerDatas)
			if (!pd->IsLocal || !IsPlayerAlive(pd))
			{
				continue;
			}
			const TActor *p = ActorGetByUID(pd->ActorUID);
			const int pad = CAMERA_SPLIT_PADDING;
			struct vec2 vel = svec2_zero();
			if (screen.x + pad > p->tileItem.Pos.x && p->tileItem.Vel.x < 1)
			{
				vel.x = screen.x + pad - p->tileItem.Pos.x;
			}
			else if (screen.x + w - pad < p->tileItem.Pos.x &&
				p->tileItem.Vel.x > -1)
			{
				vel.x = screen.x + w - pad - p->tileItem.Pos.x;
			}
			if (screen.y + pad > p->tileItem.Pos.y && p->tileItem.Vel.y < 1)
			{
				vel.y = screen.y + pad - p->tileItem.Pos.y;
			}
			else if (screen.y + h - pad < p->tileItem.Pos.y &&
				p->tileItem.Vel.y > -1)
			{
				vel.y = screen.y + h - pad - p->tileItem.Pos.y;
			}
			if (!svec2_is_zero(vel))
			{
				GameEvent ei = GameEventNew(GAME_EVENT_ACTOR_IMPULSE);
				ei.u.ActorImpulse.UID = p->uid;
				ei.u.ActorImpulse.Vel = Vec2ToNet(svec2_scale(vel, 0.25f));
				ei.u.ActorImpulse.Pos = Vec2ToNet(svec2_zero());
				GameEventsEnqueue(&gGameEvents, ei);
				LOG(LM_MAIN, LL_TRACE,
					"playerUID(%d) pos(%f, %f) screen(%d, %d) impulse(%f, %f)",
					p->uid, p->tileItem.Pos.x, p->tileItem.Pos.y,
					screen.x, screen.y,
					ei.u.ActorImpulse.Vel.x, ei.u.ActorImpulse.Vel.y);
			}
		CA_FOREACH_END()
	}

	UpdateAllActors(ticksPerFrame);
	UpdateObjects(ticksPerFrame);
	UpdateMobileObjects(ticksPerFrame);
	ParticlesUpdate(&gParticles, ticksPerFrame);

	UpdateWatches(&rData->map->triggers, ticksPerFrame);

	PowerupSpawnerUpdate(&rData->healthSpawner, ticksPerFrame);
	CA_FOREACH(PowerupSpawner, a, rData->ammoSpawners)
		PowerupSpawnerUpdate(a, ticksPerFrame);
	CA_FOREACH_END()

	if (!gCampaign.IsClient)
	{
		CheckMissionCompletion(rData->m);
	}
	else if (!NetClientIsConnected(&gNetClient))
	{
		// Check if disconnected from server; end mission
		const NMissionEnd me = NMissionEnd_init_zero;
		MissionDone(&gMission, me);
	}

	HandleGameEvents(
		&gGameEvents, &rData->Camera,
		&rData->healthSpawner, &rData->ammoSpawners);

	rData->m->time += ticksPerFrame;

	CameraUpdate(&rData->Camera, ticksPerFrame, 1000 / data->FPS);

	return UPDATE_RESULT_DRAW;
}
static void NextLoop(RunGameData *rData, LoopRunner *l)
{
	// Find the next screen to switch to
	const bool hasLocalPlayers = GetNumPlayers(PLAYER_ANY, false, true) > 0;
	const int survivingPlayers =
		GetNumPlayers(PLAYER_ALIVE, false, false);
	const bool survivedAndCompletedObjectives =
		survivingPlayers > 0 && MissionAllObjectivesComplete(&gMission);

	// Switch to a score screen if there are local players and we haven't quit
	const bool showScores = !gMission.IsQuit && hasLocalPlayers;
	if (showScores)
	{
		switch (rData->co->Entry.Mode)
		{
		case GAME_MODE_DOGFIGHT:
			LoopRunnerChange(l, ScreenDogfightScores());
			break;
		case GAME_MODE_DEATHMATCH:
			LoopRunnerChange(l, ScreenDeathmatchFinalScores());
			break;
		default:
			// In co-op (non-PVP) modes, at least one player must survive
			LoopRunnerChange(l, ScreenMissionSummary(
				rData->co, &gMission, survivedAndCompletedObjectives));
			break;
		}
	}
	else
	{
		LoopRunnerChange(l, HighScoresScreen(&gCampaign, &gGraphicsDevice));
	}
}
static void CheckMissionCompletion(const struct MissionOptions *mo)
{
	// Check if we need to update explore objectives
	CA_FOREACH(const Objective, o, mo->missionData->Objectives)
		if (o->Type != OBJECTIVE_INVESTIGATE) continue;
		const int update = MapGetExploredPercentage(&gMap) - o->done;
		if (update > 0 && !gCampaign.IsClient)
		{
			GameEvent e = GameEventNew(GAME_EVENT_OBJECTIVE_UPDATE);
			e.u.ObjectiveUpdate.ObjectiveId = _ca_index;
			e.u.ObjectiveUpdate.Count = update;
			GameEventsEnqueue(&gGameEvents, e);
		}
	CA_FOREACH_END()

	const bool isMissionComplete =
		GetNumPlayers(PLAYER_ALIVE_OR_DYING, false, false) > 0 && IsMissionComplete(mo);
	if (mo->state == MISSION_STATE_PLAY && isMissionComplete)
	{
		GameEvent e = GameEventNew(GAME_EVENT_MISSION_PICKUP);
		GameEventsEnqueue(&gGameEvents, e);
	}
	if (mo->state == MISSION_STATE_PICKUP && !isMissionComplete)
	{
		GameEvent e = GameEventNew(GAME_EVENT_MISSION_INCOMPLETE);
		GameEventsEnqueue(&gGameEvents, e);
	}
	if (mo->state == MISSION_STATE_PICKUP &&
		mo->pickupTime + PICKUP_LIMIT <= mo->time)
	{
		GameEvent e = GameEventNew(GAME_EVENT_MISSION_END);
		GameEventsEnqueue(&gGameEvents, e);
	}

	// Check that all players have been destroyed
	// If the server has no players at all, wait for a player to join
	if (gPlayerDatas.size > 0)
	{
		// Note: there's a period of time where players are dying
		// Wait until after this period before ending the game
		bool allPlayersDestroyed = true;
		CA_FOREACH(const PlayerData, p, gPlayerDatas)
			if (p->ActorUID != -1)
			{
				allPlayersDestroyed = false;
				break;
			}
		CA_FOREACH_END()
		if (allPlayersDestroyed && AreAllPlayersDeadAndNoLives())
		{
			GameEvent e = GameEventNew(GAME_EVENT_MISSION_END);
			e.u.MissionEnd.Delay = GAME_OVER_DELAY;
			GameEventsEnqueue(&gGameEvents, e);
		}
	}
}
static void RunGameDraw(GameLoopData *data)
{
	RunGameData *rData = data->Data;

	// Draw game layer
	BlitClearBuf(&gGraphicsDevice);
	CameraDraw(&rData->Camera, rData->Camera.HUD.DrawData);
	BlitUpdateFromBuf(&gGraphicsDevice, gGraphicsDevice.screen);

	// Draw HUD layer
	BlitClearBuf(&gGraphicsDevice);
	CameraDrawMode(&rData->Camera);
	HUDDraw(
		&rData->Camera.HUD, rData->pausingDevice, rData->controllerUnplugged,
		rData->Camera.NumViews);
	const bool isMouse = GameIsMouseUsed();
	SDL_SetRelativeMouseMode(isMouse);
	if (isMouse)
	{
		MouseDraw(&gEventHandlers.mouse);
	}
	// Draw automap if enabled
	if (rData->isMap)
	{
		AutomapDraw(0, rData->Camera.HUD.showExit);
	}
	BlitUpdateFromBuf(&gGraphicsDevice, gGraphicsDevice.hud);

	if (gGraphicsDevice.cachedConfig.SecondWindow)
	{
		BlitClearBuf(&gGraphicsDevice);
		if (IsAutoMapEnabled(gCampaign.Entry.Mode))
		{
			AutomapDraw(0, rData->Camera.HUD.showExit);
		}
		BlitUpdateFromBuf(&gGraphicsDevice, gGraphicsDevice.hud2);
	}
}
