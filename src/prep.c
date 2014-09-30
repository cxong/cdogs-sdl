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
#include "prep.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <SDL_timer.h>

#include <cdogs/proto/client.pb.h>
#include <cdogs/ai_coop.h>
#include <cdogs/actors.h>
#include <cdogs/blit.h>
#include <cdogs/config.h>
#include <cdogs/draw.h>
#include <cdogs/files.h>
#include <cdogs/font.h>
#include <cdogs/grafx.h>
#include <cdogs/input.h>
#include <cdogs/joystick.h>
#include <cdogs/keyboard.h>
#include <cdogs/music.h>
#include <cdogs/net_client.h>
#include <cdogs/net_server.h>
#include <cdogs/pic_manager.h>
#include <cdogs/sounds.h>
#include <cdogs/utils.h>

#include "player_select_menus.h"
#include "namegen.h"
#include "weapon_menu.h"


static void CheckCampaignDefComplete(menu_t *menu, void *data);
bool ScreenWaitForCampaignDef(void)
{
	MenuSystem ms;
	MenuSystemInit(
		&ms, &gEventHandlers, &gGraphicsDevice,
		Vec2iZero(),
		gGraphicsDevice.cachedConfig.Res);
	ms.allowAborts = true;
	char buf[256];
	uint32_t ip = gNetClient.peer->address.host;
	sprintf(buf, "Connecting to %d.%d.%d.%d:%d...",
		ip & 0xff, (ip >> 8) & 0xff, (ip >> 16) & 0xff, ip >> 24,
		gNetClient.peer->address.port);
	ms.root = ms.current = MenuCreateNormal("", buf, MENU_TYPE_NORMAL, 0);
	MenuAddExitType(&ms, MENU_TYPE_RETURN);
	MenuSetPostUpdateFunc(ms.root, CheckCampaignDefComplete, &ms);

	MenuLoop(&ms);
	const bool ok = !ms.hasAbort;
	MenuSystemTerminate(&ms);
	return ok;
}
static void CheckCampaignDefComplete(menu_t *menu, void *data)
{
	UNUSED(data);
	if (gCampaign.IsLoaded)
	{
		if (gCampaign.IsError)
		{
			// Failed to load campaign; signal that we want to abort
			MenuSystem *ms = data;
			ms->hasAbort = true;
		}
		CASSERT(gCampaign.IsClient, "campaign is not client");
		// Hack to force the menu to exit
		menu->type = MENU_TYPE_RETURN;
	}
}


static void CheckRemotePlayersComplete(menu_t *menu, void *data);
bool ScreenWaitForRemotePlayers(void)
{
	MenuSystem ms;
	MenuSystemInit(
		&ms, &gEventHandlers, &gGraphicsDevice,
		Vec2iZero(),
		gGraphicsDevice.cachedConfig.Res);
	ms.allowAborts = true;
	ms.root = ms.current = MenuCreateNormal(
		"",
		"Waiting for server players...",
		MENU_TYPE_NORMAL,
		0);
	MenuAddExitType(&ms, MENU_TYPE_RETURN);
	MenuSetPostUpdateFunc(ms.root, CheckRemotePlayersComplete, NULL);

	MenuLoop(&ms);
	const bool ok = !ms.hasAbort;
	MenuSystemTerminate(&ms);
	return ok;
}
static void CheckRemotePlayersComplete(menu_t *menu, void *data)
{
	UNUSED(data);
	if (gPlayerDatas.size > 0)
	{
		// Hack to force the menu to exit
		menu->type = MENU_TYPE_RETURN;
	}
}

static void CheckNewPlayersComplete(menu_t *menu, void *data);
bool ScreenWaitForNewPlayers(void)
{
	MenuSystem ms;
	MenuSystemInit(
		&ms, &gEventHandlers, &gGraphicsDevice,
		Vec2iZero(),
		gGraphicsDevice.cachedConfig.Res);
	ms.allowAborts = true;
	ms.root = ms.current = MenuCreateNormal(
		"",
		"Registering players...",
		MENU_TYPE_NORMAL,
		0);
	MenuAddExitType(&ms, MENU_TYPE_RETURN);
	MenuSetPostUpdateFunc(ms.root, CheckNewPlayersComplete, NULL);

	MenuLoop(&ms);
	const bool ok = !ms.hasAbort;
	MenuSystemTerminate(&ms);
	return ok;
}
static void CheckNewPlayersComplete(menu_t *menu, void *data)
{
	UNUSED(data);
	// Check that we have any local player data
	// Since we receive local player data in one go, this is adequate.
	for (int i = 0; i < (int)gPlayerDatas.size; i++)
	{
		const PlayerData *p = CArrayGet(&gPlayerDatas, i);
		if (p->IsLocal)
		{
			// Hack to force the menu to exit
			menu->type = MENU_TYPE_RETURN;
			break;
		}
	}
}


bool NumPlayersSelection(
	campaign_mode_e mode, GraphicsDevice *graphics, EventHandlers *handlers)
{
	MenuSystem ms;
	MenuSystemInit(
		&ms, handlers, graphics,
		Vec2iZero(),
		graphics->cachedConfig.Res);
	ms.allowAborts = true;
	ms.root = ms.current = MenuCreateNormal(
		"",
		"Select number of players",
		MENU_TYPE_NORMAL,
		0);
	for (int i = 0; i < MAX_LOCAL_PLAYERS; i++)
	{
		char buf[2];
		if (mode == CAMPAIGN_MODE_DOGFIGHT && i == 0)
		{
			// At least two players for dogfights
			continue;
		}
		sprintf(buf, "%d", i + 1);
		MenuAddSubmenu(ms.current, MenuCreateReturn(buf, i + 1));
	}
	MenuAddExitType(&ms, MENU_TYPE_RETURN);

	MenuLoop(&ms);
	const bool ok = !ms.hasAbort;
	if (ok)
	{
		const int numPlayers = ms.current->u.returnCode;
		for (int i = 0; i < (int)gPlayerDatas.size; i++)
		{
			const PlayerData *p = CArrayGet(&gPlayerDatas, i);
			CASSERT(!p->IsLocal, "unexpected local player");
		}
		if (NetClientIsConnected(&gNetClient))
		{
			// Tell the server that we want to add new players
			NetMsgNewPlayers np;
			np.ClientId = gNetClient.ClientId;
			np.NumPlayers = numPlayers;
			NetClientSendMsg(&gNetClient, CLIENT_MSG_NEW_PLAYERS, &np);
		}
		else
		{
			// We are the server, just add the players
			for (int i = 0; i < numPlayers; i++)
			{
				PlayerData *p = PlayerDataAdd(&gPlayerDatas);
				PlayerDataSetLocalDefaults(p, i);
				p->inputDevice = INPUT_DEVICE_UNSET;
			}
		}
	}
	MenuSystemTerminate(&ms);
	return ok;
}


static void AssignPlayerInputDevices(
	EventHandlers *handlers, const InputConfig *inputConfig)
{
	bool assignedKeyboards[MAX_KEYBOARD_CONFIGS];
	bool assignedMouse = false;
	bool assignedJoysticks[MAX_JOYSTICKS];
	memset(assignedKeyboards, 0, sizeof assignedKeyboards);
	memset(assignedJoysticks, 0, sizeof assignedJoysticks);

	for (int i = 0; i < (int)gPlayerDatas.size; i++)
	{
		PlayerData *p = CArrayGet(&gPlayerDatas, i);
		if (!p->IsLocal)
		{
			continue;
		}
		if (p->inputDevice != INPUT_DEVICE_UNSET)
		{
			// Find all the assigned devices
			switch (p->inputDevice)
			{
			case INPUT_DEVICE_KEYBOARD:
				assignedKeyboards[p->deviceIndex] = true;
				break;
			case INPUT_DEVICE_MOUSE:
				assignedMouse = true;
				break;
			case INPUT_DEVICE_JOYSTICK:
				assignedJoysticks[p->deviceIndex] = true;
				break;
			default:
				// do nothing
				break;
			}
			continue;
		}

		// Try to assign devices to players
		// For each unassigned player, check if any device has button 1 pressed
		for (int j = 0; j < MAX_KEYBOARD_CONFIGS; j++)
		{
			if (KeyIsPressed(
				&handlers->keyboard,
				inputConfig->PlayerKeys[j].Keys.button1) &&
				!assignedKeyboards[j])
			{
				PlayerSetInputDevice(p, INPUT_DEVICE_KEYBOARD, j);
				assignedKeyboards[j] = true;
				SoundPlay(&gSoundDevice, StrSound("hahaha"));
				break;
			}
		}
		if (MouseIsPressed(&handlers->mouse, SDL_BUTTON_LEFT) &&
			!assignedMouse)
		{
			PlayerSetInputDevice(p, INPUT_DEVICE_MOUSE, 0);
			assignedMouse = true;
			SoundPlay(&gSoundDevice, StrSound("hahaha"));
			continue;
		}
		for (int j = 0; j < handlers->joysticks.numJoys; j++)
		{
			if (JoyIsPressed(
				&handlers->joysticks.joys[j], CMD_BUTTON1) &&
				!assignedJoysticks[j])
			{
				PlayerSetInputDevice(p, INPUT_DEVICE_JOYSTICK, j);
				assignedJoysticks[j] = true;
				SoundPlay(&gSoundDevice, StrSound("hahaha"));
				break;
			}
		}
	}
}

typedef struct
{
	PlayerSelectMenu menus[MAX_LOCAL_PLAYERS];
	NameGen g;
	char prefixes[CDOGS_PATH_MAX];
	char suffixes[CDOGS_PATH_MAX];
	char suffixnames[CDOGS_PATH_MAX];
	bool IsOK;
} PlayerSelectionData;
static GameLoopResult PlayerSelectionUpdate(void *data);
static void PlayerSelectionDraw(void *data);
bool PlayerSelection(void)
{
	PlayerSelectionData data;
	memset(&data, 0, sizeof data);
	data.IsOK = true;
	GetDataFilePath(data.prefixes, "data/prefixes.txt");
	GetDataFilePath(data.suffixes, "data/suffixes.txt");
	GetDataFilePath(data.suffixnames, "data/suffixnames.txt");
	NameGenInit(&data.g, data.prefixes, data.suffixes, data.suffixnames);

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
			&data.menus[idx], GetNumPlayers(false, false, true), idx, i,
			&gEventHandlers, &gGraphicsDevice, &gConfig.Input, &data.g);
	}

	NetServerOpen(&gNetServer);
	GameLoopData gData = GameLoopDataNew(
		&data, PlayerSelectionUpdate,
		&data, PlayerSelectionDraw);
	GameLoop(&gData);

	if (data.IsOK)
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
				PlayerSetInputDevice(p, INPUT_DEVICE_AI, 0);
			}
		}
	}

	for (int i = 0; i < GetNumPlayers(false, false, true); i++)
	{
		MenuSystemTerminate(&data.menus[i].ms);
	}
	NameGenTerminate(&data.g);
	return data.IsOK;
}
static GameLoopResult PlayerSelectionUpdate(void *data)
{
	PlayerSelectionData *pData = data;

	// Check if anyone pressed escape
	int cmds[MAX_LOCAL_PLAYERS];
	memset(cmds, 0, sizeof cmds);
	GetPlayerCmds(&gEventHandlers, &cmds);
	if (EventIsEscape(&gEventHandlers, cmds, GetMenuCmd(&gEventHandlers)))
	{
		pData->IsOK = false;
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
		return UPDATE_RESULT_EXIT;
	}

	AssignPlayerInputDevices(&gEventHandlers, &gConfig.Input);

	return UPDATE_RESULT_DRAW;
}
static void PlayerSelectionDraw(void *data)
{
	const PlayerSelectionData *pData = data;

	GraphicsBlitBkg(&gGraphicsDevice);
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
}

typedef struct
{
	WeaponMenu menus[MAX_LOCAL_PLAYERS];
	bool IsOK;
} PlayerEquipData;
static GameLoopResult PlayerEquipUpdate(void *data);
static void PlayerEquipDraw(void *data);
bool PlayerEquip(void)
{
	PlayerEquipData data;
	memset(&data, 0, sizeof data);
	data.IsOK = true;
	for (int i = 0, idx = 0; i < (int)gPlayerDatas.size; i++, idx++)
	{
		PlayerData *p = CArrayGet(&gPlayerDatas, i);
		if (!p->IsLocal)
		{
			idx--;
			continue;
		}
		WeaponMenuCreate(
			&data.menus[idx], GetNumPlayers(false, false, true), idx, i,
			&gEventHandlers, &gGraphicsDevice, &gConfig.Input);
		// For AI players, pre-pick their weapons and go straight to menu end
		if (p->inputDevice == INPUT_DEVICE_AI)
		{
			const int lastMenuIndex =
				(int)data.menus[idx].ms.root->u.normal.subMenus.size - 1;
			data.menus[idx].ms.current = CArrayGet(
				&data.menus[idx].ms.root->u.normal.subMenus, lastMenuIndex);
			p->weapons[0] = AICoopSelectWeapon(
				idx, &gMission.missionData->Weapons);
			p->weaponCount = 1;
			// TODO: select more weapons, or select weapons based on mission
		}
	}

	GameLoopData gData = GameLoopDataNew(
		&data, PlayerEquipUpdate, &data, PlayerEquipDraw);
	GameLoop(&gData);

	for (int i = 0, idx = 0; i < (int)gPlayerDatas.size; i++, idx++)
	{
		PlayerData *p = CArrayGet(&gPlayerDatas, i);
		if (!p->IsLocal)
		{
			idx--;
			continue;
		}
		// Ready player definitions
		p->IsUsed = true;
		if (gCampaign.IsClient)
		{
			NetClientSendMsg(&gNetClient, CLIENT_MSG_PLAYER_DATA, p);
		}
		else
		{
			NetServerBroadcastMsg(&gNetServer, SERVER_MSG_PLAYER_DATA, p);
		}
	}

	for (int i = 0; i < GetNumPlayers(false, false, true); i++)
	{
		MenuSystemTerminate(&data.menus[i].ms);
	}

	return data.IsOK;
}
static GameLoopResult PlayerEquipUpdate(void *data)
{
	PlayerEquipData *pData = data;

	// Check if anyone pressed escape
	int cmds[MAX_LOCAL_PLAYERS];
	memset(cmds, 0, sizeof cmds);
	GetPlayerCmds(&gEventHandlers, &cmds);
	if (EventIsEscape(&gEventHandlers, cmds, GetMenuCmd(&gEventHandlers)))
	{
		pData->IsOK = false;
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
	for (int i = 0; i < GetNumPlayers(false, false, true); i++)
	{
		if (strcmp(pData->menus[i].ms.current->name, "(End)") != 0)
		{
			isDone = false;
		}
	}
	if (isDone)
	{
		return UPDATE_RESULT_EXIT;
	}

	return UPDATE_RESULT_DRAW;
}
static void PlayerEquipDraw(void *data)
{
	const PlayerEquipData *pData = data;
	GraphicsBlitBkg(&gGraphicsDevice);
	for (int i = 0; i < GetNumPlayers(false, false, true); i++)
	{
		MenuDisplay(&pData->menus[i].ms);
	}
}
