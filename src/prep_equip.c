/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.
	Copyright (c) 2013-2018, 2020-2023 Cong Xu
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
#include "prep_equip.h"

#include <cdogs/net_client.h>
#include <cdogs/net_server.h>
#include <cdogs/player.h>

#include "autosave.h"
#include "prep.h"
#include "equip_menu.h"

static void AddPlayerWeapons(CArray *weapons, const WeaponClass **guns);
static void RemoveUnavailableWeapons(
	const WeaponClass **guns, const CArray *weapons);
static void AddDefaultGuns(PlayerData *p, const int idx, const CArray *guns);
typedef struct
{
	EquipMenu menus[MAX_LOCAL_PLAYERS];
	EventWaitResult waitResult;
} PlayerEquipData;
static void PlayerEquipTerminate(GameLoopData *data);
static void PlayerEquipOnExit(GameLoopData *data);
static GameLoopResult PlayerEquipUpdate(GameLoopData *data, LoopRunner *l);
static void PlayerEquipDraw(GameLoopData *data);
GameLoopData *PlayerEquip(void)
{
	PlayerEquipData *data;
	CCALLOC(data, sizeof *data);
	const CampaignSave *save = AutosaveGetCampaign(&gAutosave, gCampaign.Entry.Path);
	const Mission *m = CampaignGetCurrentMission(&gCampaign);
	if (save != NULL && gCampaign.MissionIndex > 0)
	{
		PlayerSavesApply(&save->Players, m->WeaponPersist);
	}
	data->waitResult = EVENT_WAIT_CONTINUE;
	for (int i = 0, idx = 0; i < (int)gPlayerDatas.size; i++, idx++)
	{
		PlayerData *p = CArrayGet(&gPlayerDatas, i);
		if (!p->IsLocal)
		{
			idx--;
			continue;
		}

		CArray weapons;
		CArrayInit(&weapons, sizeof(WeaponClass *));
		CArrayCopy(&weapons, &gMission.Weapons);
		if (gMission.missionData->WeaponPersist)
		{
			// Add player's current weapons into available weapons
			AddPlayerWeapons(&weapons, p->guns);
		}
		else
		{
			// Remove unavailable weapons from players inventories
			RemoveUnavailableWeapons(p->guns, &weapons);
		}

		// Add default guns if the player has no weapons
		if (PlayerGetNumWeapons(p) == 0)
		{
			AddDefaultGuns(p, idx, &weapons);
		}

		// Get previous mission's weapons
		const CArray *prevWeapons = NULL;
		if (gCampaign.Entry.Mode == GAME_MODE_NORMAL &&
			gCampaign.MissionIndex > 0)
		{
			const Mission *prevMission = CArrayGet(
				&gCampaign.Setting.Missions, gCampaign.MissionIndex - 1);
			prevWeapons = &prevMission->Weapons;
		}
		
		// Special case: reset player lives
		// Player.Lives is modified dynamically in game, so if we
		// are replaying, we need to reset if we have no player save
		if (save == NULL || gCampaign.MissionIndex == 0)
		{
			p->Lives = CampaignGetLives(&gCampaign);
		}

		EquipMenuCreate(
			&data->menus[idx], &weapons, prevWeapons,
			GetNumPlayers(PLAYER_ANY, false, true), idx, p->UID,
			&gEventHandlers, &gGraphicsDevice);
		CArrayTerminate(&weapons);
	}

	return GameLoopDataNew(
		data, PlayerEquipTerminate, NULL, PlayerEquipOnExit, NULL,
		PlayerEquipUpdate, PlayerEquipDraw);
}
static bool HasWeapon(const CArray *weapons, const WeaponClass *wc);
static void RemoveUnavailableWeapons(
	const WeaponClass **guns, const CArray *weapons)
{
	for (int i = 0; i < MAX_WEAPONS; i++)
	{
		if (!HasWeapon(weapons, guns[i]))
		{
			guns[i] = NULL;
		}
	}
}
static void AddPlayerWeapons(CArray *weapons, const WeaponClass **guns)
{
	for (int i = 0; i < MAX_WEAPONS; i++)
	{
		if (guns[i] != NULL && !HasWeapon(weapons, guns[i]))
		{
			CArrayPushBack(weapons, &guns[i]);
		}
	}
}
static void AddDefaultGuns(PlayerData *p, const int idx, const CArray *guns)
{
	const char *defaultGuns[MAX_LOCAL_PLAYERS][MAX_WEAPONS] = {
		{"Shotgun", "Machine gun", "Knife", "Shrapnel bombs"},
		{"Powergun", "Flamer", "Knife", "Grenades"},
		{"Sniper rifle", "Pulse rifle", "Knife", "Molotovs"},
		{"Machine gun", "Flamer", "Knife", "Dynamite"},
	};

	if (!gCampaign.Setting.BuyAndSell)
	{
		// Attempt to give the player default guns; if the guns are missing,
		// find guns available in the mission Look for the default guns
		for (int i = 0; i < MAX_WEAPONS; i++)
		{
			const WeaponClass *defaultWC = NULL;
			CA_FOREACH(const WeaponClass *, wc, *guns)
			const char *gunName = defaultGuns[idx][i];
			if (strcmp((*wc)->name, gunName) == 0)
			{
				defaultWC = *wc;
			}
			CA_FOREACH_END()
			if (defaultWC != NULL)
			{
				PlayerAddWeapon(p, defaultWC);
			}
		}
		// Look for guns available in the mission
		CA_FOREACH(const WeaponClass *, wc, *guns)
		PlayerAddWeapon(p, *wc);
		CA_FOREACH_END()
	}
	PlayerAddMinimalWeapons(p);
}
static bool HasWeapon(const CArray *weapons, const WeaponClass *wc)
{
	CA_FOREACH(const WeaponClass *, wc2, *weapons)
	if (wc == *wc2)
	{
		return true;
	}
	CA_FOREACH_END()
	return false;
}
static void PlayerEquipTerminate(GameLoopData *data)
{
	PlayerEquipData *pData = data->Data;

	for (int i = 0; i < GetNumPlayers(PLAYER_ANY, false, true); i++)
	{
		EquipMenuTerminate(&pData->menus[i]);
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
					&gNetServer, NET_SERVER_BCAST, GAME_EVENT_PLAYER_DATA,
					&pd);
			}
		}
	}
	else
	{
		CampaignUnload(&gCampaign);
	}
}
static GameLoopResult PlayerEquipUpdate(GameLoopData *data, LoopRunner *l)
{
	PlayerEquipData *pData = data->Data;

	// If no human players, don't show equip screen
	if (GetNumPlayers(PLAYER_ANY, false, true) == 0 ||
		gCampaign.Setting.SkipWeaponMenu)
	{
		pData->waitResult = EVENT_WAIT_OK;
		goto bail;
	}

	// Check if anyone pressed escape
	int cmds[MAX_LOCAL_PLAYERS];
	memset(cmds, 0, sizeof cmds);
	GetPlayerCmds(&gEventHandlers, &cmds);
	if (EventIsEscape(
			&gEventHandlers, cmds, GetMenuCmd(&gEventHandlers, false)))
	{
		pData->waitResult = EVENT_WAIT_CANCEL;
		goto bail;
	}

	// Update menus
	bool isDone = true;
	for (int i = 0; i < GetNumPlayers(PLAYER_ANY, false, true); i++)
	{
		EquipMenuUpdate(&pData->menus[i], cmds[i]);
		isDone = isDone && EquipMenuIsDone(&pData->menus[i]);
	}
	if (isDone)
	{
		pData->waitResult = EVENT_WAIT_OK;
		goto bail;
	}

	return UPDATE_RESULT_DRAW;

bail:
	if (pData->waitResult == EVENT_WAIT_OK)
	{
		LoopRunnerChange(l, ScreenWaitForGameStart());
	}
	else
	{
		LoopRunnerPop(l);
	}
	return UPDATE_RESULT_OK;
}
static void PlayerEquipDraw(GameLoopData *data)
{
	const PlayerEquipData *pData = data->Data;
	BlitClearBuf(&gGraphicsDevice);
	for (int i = 0; i < GetNumPlayers(PLAYER_ANY, false, true); i++)
	{
		EquipMenuDraw(&pData->menus[i]);
	}
	BlitUpdateFromBuf(&gGraphicsDevice, gGraphicsDevice.screen);
}
