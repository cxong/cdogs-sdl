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

	Copyright (c) 2013-2018, 2020-2022 Cong Xu
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
#include "prep.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL_mouse.h>

#include <cdogs/actors.h>
#include <cdogs/blit.h>
#include <cdogs/config_io.h>
#include <cdogs/draw/draw.h>
#include <cdogs/files.h>
#include <cdogs/font.h>
#include <cdogs/grafx.h>
#include <cdogs/handle_game_events.h>
#include <cdogs/joystick.h>
#include <cdogs/keyboard.h>
#include <cdogs/log.h>
#include <cdogs/music.h>
#include <cdogs/net_client.h>
#include <cdogs/net_server.h>
#include <cdogs/pic_manager.h>
#include <cdogs/sounds.h>
#include <cdogs/utils.h>

#include "briefing_screens.h"
#include "game.h"
#include "loading_screens.h"
#include "namegen.h"
#include "password.h"
#include "player_select_menus.h"

typedef struct
{
	MenuSystem ms;
	GameLoopResult (*checkFunc)(void *, LoopRunner *l);
	void *data;
} ScreenWaitData;
static void ScreenWaitTerminate(GameLoopData *data);
static GameLoopResult ScreenWaitUpdate(GameLoopData *data, LoopRunner *l);
static void ScreenWaitDraw(GameLoopData *data);
static GameLoopData *ScreenWait(
	const char *message, GameLoopResult (*checkFunc)(void *, LoopRunner *l),
	void *data)
{
	ScreenWaitData *swData;
	CMALLOC(swData, sizeof *swData);
	MenuSystemInit(
		&swData->ms, &gEventHandlers, &gGraphicsDevice, svec2i_zero(),
		gGraphicsDevice.cachedConfig.Res);
	swData->ms.allowAborts = true;
	swData->ms.root = swData->ms.current =
		MenuCreateNormal("", message, MENU_TYPE_NORMAL, 0);
	MenuAddExitType(&swData->ms, MENU_TYPE_RETURN);
	swData->checkFunc = checkFunc;
	swData->data = data;

	return GameLoopDataNew(
		swData, ScreenWaitTerminate, NULL, NULL, NULL, ScreenWaitUpdate,
		ScreenWaitDraw);
}
static void ScreenWaitTerminate(GameLoopData *data)
{
	ScreenWaitData *swData = data->Data;

	MenuSystemTerminate(&swData->ms);
	CFREE(swData->data);
	CFREE(swData);
}
static GameLoopResult ScreenWaitUpdate(GameLoopData *data, LoopRunner *l)
{
	ScreenWaitData *swData = data->Data;

	const GameLoopResult result = swData->checkFunc(swData->data, l);
	if (result != UPDATE_RESULT_DRAW)
	{
		return result;
	}
	const GameLoopResult menuResult = MenuUpdate(&swData->ms);
	if (menuResult == UPDATE_RESULT_OK)
	{
		LoopRunnerPop(l);
	}
	return menuResult;
}
static void ScreenWaitDraw(GameLoopData *data)
{
	const ScreenWaitData *swData = data->Data;

	MenuDraw(&swData->ms);
}

static GameLoopResult CheckCampaignDefComplete(void *data, LoopRunner *l);
GameLoopData *ScreenWaitForCampaignDef(void)
{
	char buf[256];
	char ipbuf[64];
	enet_address_get_host_ip(&gNetClient.peer->address, ipbuf, sizeof ipbuf);
	sprintf(
		buf, "Connecting to %s:%u...", ipbuf, gNetClient.peer->address.port);
	return ScreenWait(buf, CheckCampaignDefComplete, NULL);
}
static GameLoopResult CheckCampaignDefComplete(void *data, LoopRunner *l)
{
	UNUSED(data);
	if (gCampaign.IsLoaded || gCampaign.IsError)
	{
		if (gCampaign.IsError)
		{
			// Failed to load campaign
			CampaignUnload(&gCampaign);
		}
		else
		{
			CASSERT(gCampaign.IsClient, "campaign is not client");
		}
		LoopRunnerPop(l);
		return UPDATE_RESULT_OK;
	}
	return UPDATE_RESULT_DRAW;
}

static void NumPlayersTerminate(GameLoopData *data);
static void NumPlayersOnEnter(GameLoopData *data);
static GameLoopResult NumPlayersUpdate(GameLoopData *data, LoopRunner *l);
static void NumPlayersDraw(GameLoopData *data);
GameLoopData *NumPlayersSelection(
	GraphicsDevice *graphics, EventHandlers *handlers)
{
	MenuSystem *ms;
	CMALLOC(ms, sizeof *ms);
	MenuSystemInit(
		ms, handlers, graphics, svec2i_zero(), graphics->cachedConfig.Res);
	ms->allowAborts = true;
	ms->root = ms->current =
		MenuCreateNormal("", "Select number of players", MENU_TYPE_NORMAL, 0);
	MenuAddSubmenu(ms->current, MenuCreateReturn("(No local players)", 0));
	for (int i = 0; i < MAX_LOCAL_PLAYERS; i++)
	{
		char buf[2];
		sprintf(buf, "%d", i + 1);
		MenuAddSubmenu(ms->current, MenuCreateReturn(buf, i + 1));
	}
	MenuAddExitType(ms, MENU_TYPE_RETURN);
	// Select 1 player default
	ms->current->u.normal.index = 1;

	return GameLoopDataNew(
		ms, NumPlayersTerminate, NumPlayersOnEnter, NULL, NULL,
		NumPlayersUpdate, NumPlayersDraw);
}
static void NumPlayersTerminate(GameLoopData *data)
{
	MenuSystem *ms = data->Data;
	MenuSystemTerminate(ms);
	CFREE(data->Data);
}
static void NumPlayersOnEnter(GameLoopData *data)
{
	UNUSED(data);
	LoadingScreenDraw(&gLoadingScreen, "Loading...", 1.0f);
	MusicPlayFromChunk(
		&gSoundDevice.music, MUSIC_BRIEFING,
		&gCampaign.Setting.CustomSongs[MUSIC_BRIEFING]);
}
static GameLoopResult NumPlayersUpdate(GameLoopData *data, LoopRunner *l)
{
	int numPlayers = 0;
	GameLoopResult result = UPDATE_RESULT_DRAW;
	bool hasAbort = false;
	if (gEventHandlers.DemoQuitTimer > 0)
	{
		// Select random number of players for demo
		numPlayers = RAND_INT(1, 5);
		result = UPDATE_RESULT_OK;
	}
	else
	{
		MenuSystem *ms = data->Data;
		result = MenuUpdate(ms);
		numPlayers = ms->current->u.returnCode;
		hasAbort = ms->hasAbort;
	}

	if (result == UPDATE_RESULT_OK)
	{
		if (hasAbort)
		{
			CampaignUnload(&gCampaign);
			LoopRunnerPop(l);
		}
		else
		{
			CA_FOREACH(const PlayerData, p, gPlayerDatas)
			CASSERT(!p->IsLocal, "unexpected local player");
			CA_FOREACH_END()
			// Add the players
			for (int i = 0; i < numPlayers; i++)
			{
				GameEvent e = GameEventNew(GAME_EVENT_PLAYER_DATA);
				e.u.PlayerData = PlayerDataDefault(i);
				e.u.PlayerData.UID = gNetClient.FirstPlayerUID + i;
				GameEventsEnqueue(&gGameEvents, e);
			}
			// Process the events to force add the players
			HandleGameEvents(&gGameEvents, NULL, NULL, NULL, NULL);
			// This also causes the client to send player data to the server

			// Switch to player selection
			LoopRunnerChange(l, PlayerSelection());
		}
	}
	return result;
}
static void NumPlayersDraw(GameLoopData *data)
{
	MenuDraw(data->Data);
}

static void AssignPlayerInputDevices(EventHandlers *handlers)
{
	CA_FOREACH(PlayerData, p, gPlayerDatas)
	if (!p->IsLocal)
	{
		continue;
	}

	// Try to assign devices to players
	// For each unassigned player, check if any device has button 1 pressed
	// If so, and that input device wasn't used, assign it to that player
	for (int j = 0; j < MAX_KEYBOARD_CONFIGS; j++)
	{
		char buf[256];
		sprintf(buf, "Input.PlayerCodes%d.button1", j);
		if (KeyIsPressed(&handlers->keyboard, ConfigGetInt(&gConfig, buf)) &&
			PlayerTrySetUnusedInputDevice(p, INPUT_DEVICE_KEYBOARD, j))
		{
			MenuPlaySound(MENU_SOUND_START);
			break;
		}
	}
	for (int j = 0; j < (int)handlers->joysticks.size; j++)
	{
		const Joystick *joy = CArrayGet(&handlers->joysticks, j);
		if (JoyIsPressed(joy->id, CMD_BUTTON1) &&
			PlayerTrySetUnusedInputDevice(p, INPUT_DEVICE_JOYSTICK, joy->id))
		{
			MenuPlaySound(MENU_SOUND_START);
			break;
		}
	}
	CA_FOREACH_END()
}

typedef struct
{
	PlayerSelectMenu menus[MAX_LOCAL_PLAYERS];
	NameGen g;
	char prefixes[CDOGS_PATH_MAX];
	char suffixes[CDOGS_PATH_MAX];
	char suffixnames[CDOGS_PATH_MAX];
	EventWaitResult waitResult;
} PlayerSelectionData;
static void PlayerSelectionTerminate(GameLoopData *data);
static void PlayerSelectionOnExit(GameLoopData *data);
static GameLoopResult PlayerSelectionUpdate(GameLoopData *data, LoopRunner *l);
static void PlayerSelectionDraw(GameLoopData *data);
GameLoopData *PlayerSelection(void)
{
	PlayerSelectionData *data;
	CCALLOC(data, sizeof *data);
	data->waitResult = EVENT_WAIT_CONTINUE;
	GetDataFilePath(data->prefixes, "data/prefixes.txt");
	GetDataFilePath(data->suffixes, "data/suffixes.txt");
	GetDataFilePath(data->suffixnames, "data/suffixnames.txt");
	NameGenInit(&data->g, data->prefixes, data->suffixes, data->suffixnames);

	// Create selection menus for each local player
	for (int i = 0, idx = 0; i < (int)gPlayerDatas.size; i++, idx++)
	{
		PlayerData *p = CArrayGet(&gPlayerDatas, i);
		if (!p->IsLocal)
		{
			idx--;
			continue;
		}
		PlayerSelectMenusCreate(
			&data->menus[idx], GetNumPlayers(PLAYER_ANY, false, true), idx,
			p->UID, &gEventHandlers, &gGraphicsDevice, &data->g);
	}

	return GameLoopDataNew(
		data, PlayerSelectionTerminate, NULL, PlayerSelectionOnExit, NULL,
		PlayerSelectionUpdate, PlayerSelectionDraw);
}
static void PlayerSelectionTerminate(GameLoopData *data)
{
	PlayerSelectionData *pData = data->Data;

	for (int i = 0; i < GetNumPlayers(PLAYER_ANY, false, true); i++)
	{
		MenuSystemTerminate(&pData->menus[i].ms);
	}
	NameGenTerminate(&pData->g);
	CFREE(pData);
}
static void PlayerSelectionOnExit(GameLoopData *data)
{
	PlayerSelectionData *pData = data->Data;
	if (pData->waitResult == EVENT_WAIT_OK)
	{
		for (int i = 0, idx = 0; i < (int)gPlayerDatas.size; i++, idx++)
		{
			PlayerData *p = CArrayGet(&gPlayerDatas, i);
			if (!p->IsLocal)
			{
				idx--;
				continue;
			}

			// For any player slots not picked, turn them into AIs
			if (p->inputDevice == INPUT_DEVICE_UNSET)
			{
				PlayerTrySetInputDevice(p, INPUT_DEVICE_AI, 0);
			}
		}

		if (!gCampaign.IsClient)
		{
			gCampaign.MissionIndex = 0;
		}
	}
	else
	{
		CampaignUnload(&gCampaign);
	}
}
static GameLoopResult PlayerSelectionUpdate(GameLoopData *data, LoopRunner *l)
{
	PlayerSelectionData *pData = data->Data;

	if (GetNumPlayers(PLAYER_ANY, false, true) == 0)
	{
		pData->waitResult = EVENT_WAIT_OK;
		LoopRunnerPop(l);
		return UPDATE_RESULT_OK;
	}

	// Check if anyone pressed escape
	int cmds[MAX_LOCAL_PLAYERS];
	memset(cmds, 0, sizeof cmds);
	GetPlayerCmds(&gEventHandlers, &cmds);
	if (EventIsEscape(
			&gEventHandlers, cmds, GetMenuCmd(&gEventHandlers, false)))
	{
		pData->waitResult = EVENT_WAIT_CANCEL;
		LoopRunnerPop(l);
		return UPDATE_RESULT_OK;
	}

	// Menu input
	int idx = 0;
	const bool useMenuCmd = GetNumPlayers(PLAYER_ANY, true, true);
	for (int i = 0; i < (int)gPlayerDatas.size; i++, idx++)
	{
		const PlayerData *p = CArrayGet(&gPlayerDatas, i);
		if (!p->IsLocal)
		{
			idx--;
			continue;
		}
		if (p->inputDevice != INPUT_DEVICE_UNSET)
		{
			MenuSystem *ms = &pData->menus[idx].ms;
			if (ms->current->customPostUpdateFunc)
			{
				ms->current->customPostUpdateFunc(
					ms->current, ms->current->customPostUpdateData);
			}
			MenuUpdateMouse(ms);
			if (useMenuCmd)
			{
				cmds[idx] |=
					GetMenuCmd(&gEventHandlers, ms->current->mouseHover);
			}
			if (!MenuIsExit(ms) && cmds[idx])
			{
				MenuProcessCmd(ms, cmds[idx]);
			}
		}
	}

	// Conditions for exit: at least one player has selected "Done",
	// and no other players, if any, are still selecting their player
	// The "players" with no input device are turned into AIs
	// If in demo mode, all players are AI
	bool hasAtLeastOneInput = gEventHandlers.DemoQuitTimer > 0;
	bool isDone = true;
	idx = 0;
	for (int i = 0; i < (int)gPlayerDatas.size; i++, idx++)
	{
		const PlayerData *p = CArrayGet(&gPlayerDatas, i);
		if (!p->IsLocal)
		{
			idx--;
			continue;
		}
		if (p->inputDevice != INPUT_DEVICE_UNSET)
		{
			hasAtLeastOneInput = true;
			if (strcmp(pData->menus[idx].ms.current->name, "Done") != 0)
			{
				isDone = false;
			}
		}
	}
	if (isDone && hasAtLeastOneInput)
	{
		pData->waitResult = EVENT_WAIT_OK;
		if (!gCampaign.IsClient && CanLevelSelect(gCampaign.Entry.Mode))
		{
			LoopRunnerChange(l, LevelSelection(&gGraphicsDevice));
		}
		else
		{
			LoopRunnerChange(l, GameOptions(gCampaign.Entry.Mode));
		}
		return UPDATE_RESULT_OK;
	}

	AssignPlayerInputDevices(&gEventHandlers);

	return UPDATE_RESULT_DRAW;
}
static void PlayerSelectionDraw(GameLoopData *data)
{
	const PlayerSelectionData *pData = data->Data;

	BlitClearBuf(&gGraphicsDevice);
	const int w = gGraphicsDevice.cachedConfig.Res.x;
	const int h = gGraphicsDevice.cachedConfig.Res.y;
	int idx = 0;
	for (int i = 0; i < (int)gPlayerDatas.size; i++, idx++)
	{
		const PlayerData *p = CArrayGet(&gPlayerDatas, i);
		if (!p->IsLocal)
		{
			idx--;
			continue;
		}
		if (p->inputDevice != INPUT_DEVICE_UNSET)
		{
			MenuDisplay(&pData->menus[idx].ms);
		}
		else
		{
			struct vec2i center = svec2i_zero();
			const char *prompt =
				"Press Fire to choose input device and join...";
			const struct vec2i offset =
				svec2i_scale_divide(FontStrSize(prompt), -2);
			switch (GetNumPlayers(false, false, true))
			{
			case 1:
				// Center of screen
				center = svec2i(w / 2, h / 2);
				break;
			case 2:
				// Side by side
				center = svec2i(idx * w / 2 + w / 4, h / 2);
				break;
			case 3:
			case 4:
				// Four corners
				center = svec2i(
					(idx & 1) * w / 2 + w / 4, (idx / 2) * h / 2 + h / 4);
				break;
			default:
				CASSERT(false, "not implemented");
				break;
			}
			FontStr(prompt, svec2i_add(center, offset));
		}
	}

	BlitUpdateFromBuf(&gGraphicsDevice, gGraphicsDevice.screen);
}

typedef struct
{
	MenuSystem ms;
	const CArray *weapons;
	CArray allowed; // of bool
} GameOptionsData;
static menu_t *MenuCreateAllowedWeapons(
	const char *name, GameOptionsData *data);
static void GameOptionsTerminate(GameLoopData *data);
static void GameOptionsOnEnter(GameLoopData *data);
static GameLoopResult GameOptionsUpdate(GameLoopData *data, LoopRunner *l);
static void GameOptionsDraw(GameLoopData *data);
GameLoopData *GameOptions(const GameMode gm)
{
	// Create selection menus
	const int w = gGraphicsDevice.cachedConfig.Res.x;
	const int h = gGraphicsDevice.cachedConfig.Res.y;
	GameOptionsData *data;
	CMALLOC(data, sizeof *data);
	MenuSystem *ms = &data->ms;
	data->weapons = &gMission.Weapons;
	CArrayInit(&data->allowed, sizeof(bool));
	for (int i = 0; i < (int)data->weapons->size; i++)
	{
		const bool f = true;
		CArrayPushBack(&data->allowed, &f);
	}
	MenuSystemInit(
		ms, &gEventHandlers, &gGraphicsDevice, svec2i_zero(), svec2i(w, h));
	ms->align = MENU_ALIGN_CENTER;
	ms->allowAborts = true;
	ms->root = MenuCreateNormal("", "", MENU_TYPE_OPTIONS, 0);
#define I(_config)                                                            \
	MenuAddConfigOptionsItem(ms->root, ConfigGet(&gConfig, _config));
	switch (gm)
	{
	case GAME_MODE_NORMAL:
		I("Game.Difficulty");
		I("Game.EnemyDensity");
		I("Game.NonPlayerHP");
		I("Game.PlayerHP");
		I("Game.Lives");
		I("Game.HealthPickups");
		I("Game.RandomSeed");
		MenuAddSubmenu(ms->root, MenuCreateSeparator(""));
		I("StartServer");
		break;
	case GAME_MODE_DOGFIGHT:
		I("Dogfight.PlayerHP");
		I("Dogfight.FirstTo");
		I("Game.HealthPickups");
		I("Game.RandomSeed");
		MenuAddSubmenu(ms->root, MenuCreateAllowedWeapons("Weapons...", data));
		MenuAddSubmenu(ms->root, MenuCreateSeparator(""));
		I("StartServer");
		break;
	case GAME_MODE_DEATHMATCH:
		I("Game.PlayerHP");
		I("Deathmatch.Lives");
		I("Game.HealthPickups");
		I("Game.RandomSeed");
		MenuAddSubmenu(ms->root, MenuCreateAllowedWeapons("Weapons...", data));
		MenuAddSubmenu(ms->root, MenuCreateSeparator(""));
		I("StartServer");
		break;
	case GAME_MODE_QUICK_PLAY:
		I("QuickPlay.WallCount");
		I("QuickPlay.WallLength");
		I("QuickPlay.RoomCount");
		I("QuickPlay.SquareCount");
		I("QuickPlay.EnemyCount");
		I("QuickPlay.EnemySpeed");
		I("QuickPlay.EnemyHealth");
		I("QuickPlay.EnemiesWithExplosives");
		I("QuickPlay.ItemCount");
		// Force start server to be false - we don't support multiplayer with
		// this game mode
		ConfigGet(&gConfig, "StartServer")->u.Bool.Value = false;
		break;
	default:
		CASSERT(false, "unknown game mode");
		break;
	}
#undef I
	MenuAddSubmenu(ms->root, MenuCreateSeparator(""));
	MenuAddSubmenu(ms->root, MenuCreateReturn("Done", 0));
	MenuAddExitType(ms, MENU_TYPE_RETURN);

	return GameLoopDataNew(
		ms, GameOptionsTerminate, GameOptionsOnEnter, NULL, NULL,
		GameOptionsUpdate, GameOptionsDraw);
}
static void GameOptionsTerminate(GameLoopData *data)
{
	GameOptionsData *gData = data->Data;

	MenuSystemTerminate(&gData->ms);
	CArrayTerminate(&gData->allowed);
	CFREE(gData);
}
static void GameOptionsOnEnter(GameLoopData *data)
{
	GameOptionsData *gData = data->Data;

	if (!gCampaign.IsQuit)
	{
		MissionOptionsTerminate(&gMission);
		CampaignAndMissionSetup(&gCampaign, &gMission);
	}

	gData->ms.current = gData->ms.root;
	// Select "Done"
	gData->ms.root->u.normal.index =
		(int)gData->ms.root->u.normal.subMenus.size - 1;
}
static GameLoopResult GameOptionsUpdate(GameLoopData *data, LoopRunner *l)
{
	GameOptionsData *gData = data->Data;

	// Check end conditions:
	// - Campaign not loaded
	// - Campaign complete
	// - Mission quit
	// - No options needed
	// - Demo mode
	// - Menu complete
	const GameLoopResult result = MenuUpdate(&gData->ms);
	const bool isQuit = !gCampaign.IsLoaded || gCampaign.IsComplete ||
						gCampaign.IsQuit || gData->ms.hasAbort ||
						gMission.missionData == NULL;
	const bool isDone = !IsGameOptionsNeeded(gCampaign.Entry.Mode) ||
						result == UPDATE_RESULT_OK ||
						gEventHandlers.DemoQuitTimer > 0;
	if (isQuit || isDone)
	{
		if (isQuit)
		{
			MissionOptionsTerminate(&gMission);
			CampaignUnload(&gCampaign);
			LoopRunnerPop(l);
		}
		else
		{
			if (!ConfigApply(&gConfig, NULL))
			{
				LOG(LM_MAIN, LL_ERROR,
					"Failed to apply config; reset to last used");
				ConfigResetChanged(&gConfig);
			}
			else
			{
				// Save options for later
				ConfigSave(&gConfig, GetConfigFilePath(CONFIG_FILE));
			}

			// Set allowed weapons
			// First check if the player has unwittingly disabled all weapons
			// if so, enable all weapons
			bool allDisabled = true;
			for (int i = 0, j = 0; i < (int)gData->allowed.size; i++, j++)
			{
				const bool *allowed = CArrayGet(&gData->allowed, i);
				if (*allowed)
				{
					allDisabled = false;
					break;
				}
			}
			if (!allDisabled)
			{
				for (int i = 0, j = 0; i < (int)gData->allowed.size; i++, j++)
				{
					const bool *allowed = CArrayGet(&gData->allowed, i);
					if (!*allowed)
					{
						CArrayDelete(&gMission.Weapons, j);
						j--;
					}
				}
			}

			gCampaign.OptionsSet = true;

			// If enabled, start net server
			if (!gCampaign.IsClient && ConfigGetBool(&gConfig, "StartServer"))
			{
				NetServerOpen(&gNetServer);
			}
			LoopRunnerPush(
				l, ScreenMissionBriefing(&gCampaign.Setting, &gMission));
		}
		return UPDATE_RESULT_OK;
	}
	return UPDATE_RESULT_DRAW;
}
static void GameOptionsDraw(GameLoopData *data)
{
	const GameOptionsData *gData = data->Data;

	MenuDraw(&gData->ms);
}
static menu_t *MenuCreateAllowedWeapons(
	const char *name, GameOptionsData *data)
{
	// Create a menu to choose allowable weapons for this map
	// The weapons will be chosen from the available weapons
	menu_t *m =
		MenuCreateNormal(name, "Available Weapons:", MENU_TYPE_OPTIONS, 0);
	m->customPostInputData = data;
	for (int i = 0; i < (int)data->weapons->size; i++)
	{
		const WeaponClass **wc = CArrayGet(data->weapons, i);
		MenuAddSubmenu(
			m,
			MenuCreateOptionToggle((*wc)->name, CArrayGet(&data->allowed, i)));
	}
	MenuAddSubmenu(m, MenuCreateSeparator(""));
	MenuAddSubmenu(m, MenuCreateBack("Done"));
	return m;
}

static GameLoopResult CheckGameStart(void *data, LoopRunner *l);
GameLoopData *ScreenWaitForGameStart(void)
{
	return ScreenWait("Waiting for game start...", CheckGameStart, NULL);
}
static GameLoopResult CheckGameStart(void *data, LoopRunner *l)
{
	UNUSED(data);
	if (!gNetClient.Ready)
	{
		// Tell server we're ready
		NetClientSendMsg(&gNetClient, GAME_EVENT_CLIENT_READY, NULL);
	}
	gNetClient.Ready = true;
	if (!gCampaign.IsClient || gMission.HasStarted)
	{
		goto bail;
	}
	// Check disconnections
	if (!NetClientIsConnected(&gNetClient))
	{
		CampaignUnload(&gCampaign);
		goto bail;
	}
	return UPDATE_RESULT_DRAW;

bail:
	if (!gCampaign.IsLoaded)
	{
		LoopRunnerPop(l);
	}
	else
	{
		LoopRunnerChange(l, RunGame(&gCampaign, &gMission, &gMap));
	}
	return UPDATE_RESULT_OK;
}
