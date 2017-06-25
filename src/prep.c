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

    Copyright (c) 2013-2017 Cong Xu
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
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <SDL_mouse.h>

#include <cdogs/ai_coop.h>
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

#include "autosave.h"
#include "password.h"
#include "player_select_menus.h"
#include "namegen.h"
#include "weapon_menu.h"


typedef struct
{
	MenuSystem ms;
	GameLoopResult (*checkFunc)(void *);
	void *data;
} ScreenWaitData;
static void ScreenWaitTerminate(GameLoopData *data);
static GameLoopResult ScreenWaitUpdate(GameLoopData *data);
static void ScreenWaitDraw(GameLoopData *data);
static GameLoopData ScreenWait(
	const char *message, GameLoopResult (*checkFunc)(void *), void *data)
{
	ScreenWaitData *swData;
	CMALLOC(swData, sizeof *swData);
	MenuSystemInit(
		&swData->ms, &gEventHandlers, &gGraphicsDevice,
		Vec2iZero(),
		gGraphicsDevice.cachedConfig.Res);
	swData->ms.allowAborts = true;
	swData->ms.root = swData->ms.current =
		MenuCreateNormal("", message, MENU_TYPE_NORMAL, 0);
	MenuAddExitType(&swData->ms, MENU_TYPE_RETURN);
	swData->checkFunc = checkFunc;
	swData->data = data;

	return GameLoopDataNew(
		swData, ScreenWaitTerminate, NULL, NULL,
		NULL, ScreenWaitUpdate, ScreenWaitDraw);
}
static void ScreenWaitTerminate(GameLoopData *data)
{
	ScreenWaitData *swData = data->Data;

	MenuSystemTerminate(&swData->ms);
	CFREE(swData->data);
	CFREE(swData);
}
static GameLoopResult ScreenWaitUpdate(GameLoopData *data)
{
	ScreenWaitData *swData = data->Data;

	const GameLoopResult result = swData->checkFunc(swData->data);
	if (result != UPDATE_RESULT_DRAW)
	{
		return result;
	}
	return MenuUpdate(&swData->ms);
}
static void ScreenWaitDraw(GameLoopData *data)
{
	const ScreenWaitData *swData = data->Data;

	MenuDraw(&swData->ms);
}

static GameLoopResult CheckCampaignDefComplete(void *data);
GameLoopData ScreenWaitForCampaignDef(void)
{
	char buf[256];
	char ipbuf[256];
	enet_address_get_host_ip(&gNetClient.peer->address, ipbuf, sizeof ipbuf);
	sprintf(buf, "Connecting to %s:%u...",
		ipbuf, gNetClient.peer->address.port);
	return ScreenWait(buf, CheckCampaignDefComplete, NULL);
}
static GameLoopResult CheckCampaignDefComplete(void *data)
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
		return UPDATE_RESULT_EXIT;
	}
	return UPDATE_RESULT_DRAW;
}


static void NumPlayersTerminate(GameLoopData *data);
static void NumPlayersOnExit(GameLoopData *data);
static GameLoopResult NumPlayersUpdate(GameLoopData *data);
static void NumPlayersDraw(GameLoopData *data);
GameLoopData NumPlayersSelection(
	GraphicsDevice *graphics, EventHandlers *handlers)
{
	MenuSystem *ms;
	CMALLOC(ms, sizeof *ms);
	MenuSystemInit(
		ms, handlers, graphics,
		Vec2iZero(),
		graphics->cachedConfig.Res);
	ms->allowAborts = true;
	ms->root = ms->current = MenuCreateNormal(
		"",
		"Select number of players",
		MENU_TYPE_NORMAL,
		0);
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
		ms, NumPlayersTerminate, NULL, NumPlayersOnExit,
		NULL, NumPlayersUpdate, NumPlayersDraw);
}
static void NumPlayersTerminate(GameLoopData *data)
{
	CFREE(data->Data);
}
static void NumPlayersOnExit(GameLoopData *data)
{
	MenuSystem *ms = data->Data;
	if (ms->hasAbort)
	{
		// TODO: pop menu
		CampaignUnload(&gCampaign);
	}
	MenuSystemTerminate(ms);
}
static GameLoopResult NumPlayersUpdate(GameLoopData *data)
{
	MenuSystem *ms = data->Data;
	const GameLoopResult result = MenuUpdate(ms);
	if (result == UPDATE_RESULT_EXIT && !ms->hasAbort)
	{
		const int numPlayers = ms->current->u.returnCode;
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
		HandleGameEvents(&gGameEvents, NULL, NULL, NULL);
		// This also causes the client to send player data to the server

		// Switch to player selection
		GameLoopChange(data, PlayerSelection());
		return UPDATE_RESULT_OK;
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
				SoundPlay(&gSoundDevice, StrSound("hahaha"));
				break;
			}
		}
		if (MouseIsPressed(&handlers->mouse, SDL_BUTTON_LEFT) &&
			PlayerTrySetUnusedInputDevice(p, INPUT_DEVICE_MOUSE, 0))
		{
			SoundPlay(&gSoundDevice, StrSound("hahaha"));
			continue;
		}
		for (int j = 0; j < (int)handlers->joysticks.size; j++)
		{
			const Joystick *joy = CArrayGet(&handlers->joysticks, j);
			if (JoyIsPressed(joy->id, CMD_BUTTON1) &&
				PlayerTrySetUnusedInputDevice(p, INPUT_DEVICE_JOYSTICK, joy->id))
			{
				SoundPlay(&gSoundDevice, StrSound("hahaha"));
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
static GameLoopResult PlayerSelectionUpdate(GameLoopData *data);
static void PlayerSelectionDraw(GameLoopData *data);
GameLoopData PlayerSelection(void)
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
			&data->menus[idx], GetNumPlayers(PLAYER_ANY, false, true),
			idx, p->UID,
			&gEventHandlers, &gGraphicsDevice, &data->g);
	}

	return GameLoopDataNew(
		data, PlayerSelectionTerminate, NULL, PlayerSelectionOnExit,
		NULL, PlayerSelectionUpdate, PlayerSelectionDraw);
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

		if (gCampaign.IsClient)
		{
			// If connecting to a server, we've already received the mission index
			// Do nothing
		}
		else if (IsPasswordAllowed(gCampaign.Entry.Mode))
		{
			MissionSave m;
			AutosaveLoadMission(&gAutosave, &m, gCampaign.Entry.Path);
			gCampaign.MissionIndex = EnterPassword(&gGraphicsDevice, &m);
		}
		else
		{
			gCampaign.MissionIndex = 0;
		}
	}
	else
	{
		CampaignUnload(&gCampaign);
	}
}
static GameLoopResult PlayerSelectionUpdate(GameLoopData *data)
{
	PlayerSelectionData *pData = data->Data;

	if (GetNumPlayers(PLAYER_ANY, false, true) == 0)
	{
		pData->waitResult = EVENT_WAIT_OK;
		return UPDATE_RESULT_EXIT;
	}

	// Check if anyone pressed escape
	int cmds[MAX_LOCAL_PLAYERS];
	memset(cmds, 0, sizeof cmds);
	GetPlayerCmds(&gEventHandlers, &cmds);
	if (EventIsEscape(&gEventHandlers, cmds, GetMenuCmd(&gEventHandlers)))
	{
		pData->waitResult = EVENT_WAIT_CANCEL;
		return UPDATE_RESULT_EXIT;
	}

	// Menu input
	int idx = 0;
	for (int i = 0; i < (int)gPlayerDatas.size; i++, idx++)
	{
		const PlayerData *p = CArrayGet(&gPlayerDatas, i);
		if (!p->IsLocal)
		{
			idx--;
			continue;
		}
		if (p->inputDevice != INPUT_DEVICE_UNSET &&
			!MenuIsExit(&pData->menus[idx].ms) && cmds[idx])
		{
			MenuProcessCmd(&pData->menus[idx].ms, cmds[idx]);
		}
	}

	// Conditions for exit: at least one player has selected "Done",
	// and no other players, if any, are still selecting their player
	// The "players" with no input device are turned into AIs
	bool hasAtLeastOneInput = false;
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
		return UPDATE_RESULT_EXIT;
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
			Vec2i center = Vec2iZero();
			const char *prompt = "Press Fire to join...";
			const Vec2i offset = Vec2iScaleDiv(FontStrSize(prompt), -2);
			switch (GetNumPlayers(false, false, true))
			{
			case 1:
				// Center of screen
				center = Vec2iNew(w / 2, h / 2);
				break;
			case 2:
				// Side by side
				center = Vec2iNew(idx * w / 2 + w / 4, h / 2);
				break;
			case 3:
			case 4:
				// Four corners
				center = Vec2iNew(
					(idx & 1) * w / 2 + w / 4, (idx / 2) * h / 2 + h / 4);
				break;
			default:
				CASSERT(false, "not implemented");
				break;
			}
			FontStr(prompt, Vec2iAdd(center, offset));
		}
	}

	BlitUpdateFromBuf(&gGraphicsDevice, gGraphicsDevice.screen);
}

typedef struct
{
	MenuSystem ms;
	const CArray *weapons;
	CArray allowed;	// of bool
} GameOptionsData;
static menu_t *MenuCreateAllowedWeapons(
	const char *name, GameOptionsData *data);
static void GameOptionsTerminate(GameLoopData *data);
static void GameOptionsOnEnter(GameLoopData *data);
static void GameOptionsOnExit(GameLoopData *data);
static GameLoopResult GameOptionsUpdate(GameLoopData *data);
static void GameOptionsDraw(GameLoopData *data);
GameLoopData GameOptions(const GameMode gm)
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
		ms, &gEventHandlers, &gGraphicsDevice, Vec2iZero(), Vec2iNew(w, h));
	ms->align = MENU_ALIGN_CENTER;
	ms->allowAborts = true;
	ms->root = MenuCreateNormal(
		"",
		"",
		MENU_TYPE_OPTIONS,
		0);
#define I(_config)\
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
		I("Game.Ammo");
		I("Game.RandomSeed");
		MenuAddSubmenu(ms->root, MenuCreateSeparator(""));
		I("StartServer");
		break;
	case GAME_MODE_DOGFIGHT:
		I("Dogfight.PlayerHP");
		I("Dogfight.FirstTo");
		I("Game.HealthPickups");
		I("Game.Ammo");
		I("Game.RandomSeed");
		MenuAddSubmenu(ms->root,
			MenuCreateAllowedWeapons("Weapons...", data));
		MenuAddSubmenu(ms->root, MenuCreateSeparator(""));
		I("StartServer");
		break;
	case GAME_MODE_DEATHMATCH:
		I("Game.PlayerHP");
		I("Deathmatch.Lives");
		I("Game.HealthPickups");
		I("Game.Ammo");
		I("Game.RandomSeed");
		MenuAddSubmenu(ms->root,
			MenuCreateAllowedWeapons("Weapons...", data));
		MenuAddSubmenu(ms->root, MenuCreateSeparator(""));
		I("StartServer");
		break;
	case GAME_MODE_QUICK_PLAY:
		I("QuickPlay.MapSize");
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
		ms, GameOptionsTerminate, GameOptionsOnEnter, GameOptionsOnExit,
		NULL, GameOptionsUpdate, GameOptionsDraw);
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

	CampaignAndMissionSetup(&gCampaign, &gMission);

	gData->ms.current = gData->ms.root;
	// Select "Done"
	gData->ms.root->u.normal.index =
		(int)gData->ms.root->u.normal.subMenus.size - 1;
}
static void GameOptionsOnExit(GameLoopData *data)
{
	GameOptionsData *gData = data->Data;

	const bool ok = !gData->ms.hasAbort;
	if (!ok)
	{
		CampaignUnload(&gCampaign);
	}
	else
	{
		if (!ConfigApply(&gConfig))
		{
			LOG(LM_MAIN, LL_ERROR, "Failed to apply config; reset to last used");
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
	}
}
static GameLoopResult GameOptionsUpdate(GameLoopData *data)
{
	GameOptionsData *gData = data->Data;

	if (!IsGameOptionsNeeded(gCampaign.Entry.Mode))
	{
		return UPDATE_RESULT_EXIT;
	}
	return MenuUpdate(&gData->ms);
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
		const GunDescription **g = CArrayGet(data->weapons, i);
		MenuAddSubmenu(m,
			MenuCreateOptionToggle((*g)->name, CArrayGet(&data->allowed, i)));
	}
	MenuAddSubmenu(m, MenuCreateSeparator(""));
	MenuAddSubmenu(m, MenuCreateBack("Done"));
	return m;
}

static void RemoveUnavailableWeapons(PlayerData *data, const CArray *weapons);
typedef struct
{
	WeaponMenu menus[MAX_LOCAL_PLAYERS];
	EventWaitResult waitResult;
} PlayerEquipData;
static void PlayerEquipTerminate(GameLoopData *data);
static void PlayerEquipOnExit(GameLoopData *data);
static GameLoopResult PlayerEquipUpdate(GameLoopData *data);
static void PlayerEquipDraw(GameLoopData *data);
GameLoopData PlayerEquip(void)
{
	PlayerEquipData *data;
	CCALLOC(data, sizeof *data);
	data->waitResult = EVENT_WAIT_CONTINUE;
	for (int i = 0, idx = 0; i < (int)gPlayerDatas.size; i++, idx++)
	{
		PlayerData *p = CArrayGet(&gPlayerDatas, i);
		if (!p->IsLocal)
		{
			idx--;
			continue;
		}

		// Remove unavailable weapons from players inventories
		RemoveUnavailableWeapons(p, &gMission.Weapons);

		WeaponMenuCreate(
			&data->menus[idx], GetNumPlayers(PLAYER_ANY, false, true),
			idx, p->UID,
			&gEventHandlers, &gGraphicsDevice);
		// For AI players, pre-pick their weapons and go straight to menu end
		if (p->inputDevice == INPUT_DEVICE_AI)
		{
			const int lastMenuIndex =
				(int)data->menus[idx].ms.root->u.normal.subMenus.size - 1;
			data->menus[idx].ms.current = CArrayGet(
				&data->menus[idx].ms.root->u.normal.subMenus, lastMenuIndex);
			AICoopSelectWeapons(p, idx, &gMission.Weapons);
		}
	}

	return GameLoopDataNew(
		data, PlayerEquipTerminate, NULL, PlayerEquipOnExit,
		NULL, PlayerEquipUpdate, PlayerEquipDraw);
}
static bool HasWeapon(const CArray *weapons, const GunDescription *w);
static void RemoveUnavailableWeapons(PlayerData *data, const CArray *weapons)
{
	for (int i = data->weaponCount - 1; i >= 0; i--)
	{
		if (!HasWeapon(weapons, data->weapons[i]))
		{
			for (int j = i + 1; j < data->weaponCount; j++)
			{
				data->weapons[j - 1] = data->weapons[j];
			}
			data->weaponCount--;
		}
	}
}
static bool HasWeapon(const CArray *weapons, const GunDescription *w)
{
	for (int i = 0; i < (int)weapons->size; i++)
	{
		const GunDescription **g = CArrayGet(weapons, i);
		if (w == *g)
		{
			return true;
		}
	}
	return false;
}
static void PlayerEquipTerminate(GameLoopData *data)
{
	PlayerEquipData *pData = data->Data;

	for (int i = 0; i < GetNumPlayers(PLAYER_ANY, false, true); i++)
	{
		MenuSystemTerminate(&pData->menus[i].ms);
	}
	CFREE(pData);
}
static void PlayerEquipOnExit(GameLoopData *data)
{
	PlayerEquipData *pData = data->Data;

	if (pData->waitResult == EVENT_WAIT_OK)
	{
		for (int i = 0, idx = 0; i < (int)gPlayerDatas.size; i++, idx++)
		{
			const PlayerData *p = CArrayGet(&gPlayerDatas, i);
			if (!p->IsLocal)
			{
				idx--;
				continue;
			}
			NPlayerData pd = NMakePlayerData(p);
			// Update player definitions
			if (gCampaign.IsClient)
			{
				NetClientSendMsg(&gNetClient, GAME_EVENT_PLAYER_DATA, &pd);
			}
			else
			{
				NetServerSendMsg(
					&gNetServer, NET_SERVER_BCAST, GAME_EVENT_PLAYER_DATA, &pd);
			}
		}
	}
	else
	{
		CampaignUnload(&gCampaign);
	}
}
static GameLoopResult PlayerEquipUpdate(GameLoopData *data)
{
	PlayerEquipData *pData = data->Data;

	// If no human players, don't show equip screen
	if (GetNumPlayers(PLAYER_ANY, false, true) == 0)
	{
		pData->waitResult = EVENT_WAIT_OK;
		return UPDATE_RESULT_EXIT;
	}

	// Check if anyone pressed escape
	int cmds[MAX_LOCAL_PLAYERS];
	memset(cmds, 0, sizeof cmds);
	GetPlayerCmds(&gEventHandlers, &cmds);
	if (EventIsEscape(&gEventHandlers, cmds, GetMenuCmd(&gEventHandlers)))
	{
		pData->waitResult = EVENT_WAIT_CANCEL;
		return UPDATE_RESULT_EXIT;
	}

	// Update menus
	int idx = 0;
	for (int i = 0; i < (int)gPlayerDatas.size; i++, idx++)
	{
		const PlayerData *p = CArrayGet(&gPlayerDatas, i);
		if (!p->IsLocal)
		{
			idx--;
			continue;
		}
		if (!MenuIsExit(&pData->menus[idx].ms))
		{
			MenuProcessCmd(&pData->menus[idx].ms, cmds[idx]);
		}
		else if (p->weaponCount == 0)
		{
			// Check exit condition; must have selected at least one weapon
			// Otherwise reset the current menu
			pData->menus[idx].ms.current = pData->menus[idx].ms.root;
		}
	}

	bool isDone = true;
	for (int i = 0; i < GetNumPlayers(PLAYER_ANY, false, true); i++)
	{
		if (strcmp(pData->menus[i].ms.current->name, "(End)") != 0)
		{
			isDone = false;
		}
	}
	if (isDone)
	{
		pData->waitResult = EVENT_WAIT_OK;
		return UPDATE_RESULT_EXIT;
	}

	return UPDATE_RESULT_DRAW;
}
static void PlayerEquipDraw(GameLoopData *data)
{
	const PlayerEquipData *pData = data->Data;
	BlitClearBuf(&gGraphicsDevice);
	for (int i = 0; i < GetNumPlayers(PLAYER_ANY, false, true); i++)
	{
		MenuDisplay(&pData->menus[i].ms);
	}
	BlitUpdateFromBuf(&gGraphicsDevice, gGraphicsDevice.screen);
}

static GameLoopResult CheckGameStart(void *data);
GameLoopData ScreenWaitForGameStart(void)
{
	gNetClient.Ready = true;
	if (!gMission.HasStarted)
	{
		// Tell server we're ready
		NetClientSendMsg(&gNetClient, GAME_EVENT_CLIENT_READY, NULL);
	}
	return ScreenWait("Waiting for game start...", CheckGameStart, NULL);
}
static GameLoopResult CheckGameStart(void *data)
{
	UNUSED(data);
	if (!gCampaign.IsClient || gMission.HasStarted)
	{
		return UPDATE_RESULT_EXIT;
	}
	// Check disconnections
	if (!NetClientIsConnected(&gNetClient))
	{
		CampaignUnload(&gCampaign);
		return UPDATE_RESULT_EXIT;
	}
	return UPDATE_RESULT_DRAW;
}
