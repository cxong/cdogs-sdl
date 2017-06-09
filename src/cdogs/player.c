/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (c) 2014-2016, Cong Xu
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
#include "player.h"

#include "actors.h"
#include "log.h"
#include "net_client.h"
#include "player_template.h"


CArray gPlayerDatas;
// Templates stored here as it is used by player
// TODO: 
CArray gPlayerTemplates;


void PlayerDataInit(CArray *p)
{
	CArrayInit(p, sizeof(PlayerData));
}

void PlayerDataAddOrUpdate(const NPlayerData pd)
{
	PlayerData *p = PlayerDataGetByUID(pd.UID);
	if (p == NULL)
	{
		PlayerData pNew;
		memset(&pNew, 0, sizeof pNew);
		CArrayPushBack(&gPlayerDatas, &pNew);
		p = CArrayGet(&gPlayerDatas, (int)gPlayerDatas.size - 1);

		// Set defaults
		p->ActorUID = -1;
		p->IsLocal =
			(int)pd.UID >= gNetClient.FirstPlayerUID &&
			(int)pd.UID < gNetClient.FirstPlayerUID + MAX_LOCAL_PLAYERS;
		p->inputDevice = INPUT_DEVICE_UNSET;

		p->Char.speed = 256;

		LOG(LM_MAIN, LL_INFO, "add default player UID(%u) local(%s)",
			pd.UID, p->IsLocal ? "true" : "false");
	}

	p->UID = pd.UID;

	strcpy(p->name, pd.Name);
	p->Char.Class = StrCharacterClass(pd.CharacterClass);
	p->Char.Colors = Net2CharColors(pd.Colors);
	p->weaponCount = pd.Weapons_count;
	for (int i = 0; i < (int)pd.Weapons_count; i++)
	{
		p->weapons[i] = StrGunDescription(pd.Weapons[i]);
	}
	p->Lives = pd.Lives;
	p->Stats = pd.Stats;
	p->Totals = pd.Totals;
	p->Char.maxHealth = pd.MaxHealth;
	p->lastMission = pd.LastMission;

	// Ready players as well
	p->Ready = true;

	LOG(LM_MAIN, LL_INFO, "update player UID(%d) maxHealth(%d)",
		p->UID, p->Char.maxHealth);
}

static void PlayerTerminate(PlayerData *p);
void PlayerRemove(const int uid)
{
	// Find the player so we can remove by index
	PlayerData *p = NULL;
	int i;
	for (i = 0; i < (int)gPlayerDatas.size; i++)
	{
		PlayerData *pi = CArrayGet(&gPlayerDatas, i);
		if (pi->UID == uid)
		{
			p = pi;
			break;
		}
	}
	if (p == NULL)
	{
		return;
	}
	if (p->ActorUID >= 0)
	{
		ActorDestroy(ActorGetByUID(p->ActorUID));
	}
	PlayerTerminate(p);
	CArrayDelete(&gPlayerDatas, i);

	LOG(LM_MAIN, LL_INFO, "remove player UID(%d)", uid);
}

static void PlayerTerminate(PlayerData *p)
{
	CFREE(p->Char.bot);
}

NPlayerData PlayerDataDefault(const int idx)
{
	NPlayerData pd = NPlayerData_init_default;

	// load from template if available
	if ((int)gPlayerTemplates.size > idx)
	{
		const PlayerTemplate *t = CArrayGet(&gPlayerTemplates, idx);
		strcpy(pd.Name, t->name);
		strcpy(pd.CharacterClass, t->Class->Name);
		pd.Colors = CharColors2Net(t->Colors);
	}
	else
	{
		switch (idx)
		{
		case 0:
			strcpy(pd.Name, "Jones");
			strcpy(pd.CharacterClass, "Jones");
			pd.Colors.Skin = Color2Net(colorSkin);
			pd.Colors.Arms = Color2Net(colorLightBlue);
			pd.Colors.Body = Color2Net(colorLightBlue);
			pd.Colors.Legs = Color2Net(colorLightBlue);
			pd.Colors.Hair = Color2Net(colorLightBlue);
			break;
		case 1:
			strcpy(pd.Name, "Ice");
			strcpy(pd.CharacterClass, "Ice");
			pd.Colors.Skin = Color2Net(colorDarkSkin);
			pd.Colors.Arms = Color2Net(colorRed);
			pd.Colors.Body = Color2Net(colorRed);
			pd.Colors.Legs = Color2Net(colorRed);
			pd.Colors.Hair = Color2Net(colorBlack);
			break;
		case 2:
			strcpy(pd.Name, "Warbaby");
			strcpy(pd.CharacterClass, "WarBaby");
			pd.Colors.Skin = Color2Net(colorSkin);
			pd.Colors.Arms = Color2Net(colorGreen);
			pd.Colors.Body = Color2Net(colorGreen);
			pd.Colors.Legs = Color2Net(colorGreen);
			pd.Colors.Hair = Color2Net(colorRed);
			break;
		case 3:
			strcpy(pd.Name, "Han");
			strcpy(pd.CharacterClass, "Dragon");
			pd.Colors.Skin = Color2Net(colorAsianSkin);
			pd.Colors.Arms = Color2Net(colorYellow);
			pd.Colors.Body = Color2Net(colorYellow);
			pd.Colors.Legs = Color2Net(colorYellow);
			pd.Colors.Hair = Color2Net(colorYellow);
			break;
		default:
			// Set up player N template
			sprintf(pd.Name, "Player %d", idx);
			strcpy(pd.CharacterClass, "Jones");
			pd.Colors.Skin = Color2Net(colorSkin);
			pd.Colors.Arms = Color2Net(colorLightBlue);
			pd.Colors.Body = Color2Net(colorLightBlue);
			pd.Colors.Legs = Color2Net(colorLightBlue);
			pd.Colors.Hair = Color2Net(colorLightBlue);
			break;
		}
	}

	// weapons
	switch (idx)
	{
	case 0:
		strcpy(pd.Weapons[0], "Shotgun");
		strcpy(pd.Weapons[1], "Machine gun");
		strcpy(pd.Weapons[2], "Shrapnel bombs");
		break;
	case 1:
		strcpy(pd.Weapons[0], "Powergun");
		strcpy(pd.Weapons[1], "Flamer");
		strcpy(pd.Weapons[2], "Grenades");
		break;
	case 2:
		strcpy(pd.Weapons[0], "Sniper rifle");
		strcpy(pd.Weapons[1], "Knife");
		strcpy(pd.Weapons[2], "Molotovs");
		break;
	case 3:
		strcpy(pd.Weapons[0], "Machine gun");
		strcpy(pd.Weapons[1], "Flamer");
		strcpy(pd.Weapons[2], "Dynamite");
		break;
	default:
		strcpy(pd.Weapons[0], "Shotgun");
		strcpy(pd.Weapons[1], "Machine gun");
		strcpy(pd.Weapons[2], "Shrapnel bombs");
		break;
	}
	pd.Weapons_count = 3;

	pd.MaxHealth = 200;

	return pd;
}

NPlayerData PlayerDataMissionReset(const PlayerData *p)
{
	NPlayerData pd = NMakePlayerData(p);
	pd.Lives = ModeLives(gCampaign.Entry.Mode);

	memset(&pd.Stats, 0, sizeof pd.Stats);

	pd.LastMission = gCampaign.MissionIndex;
	pd.MaxHealth = ModeMaxHealth(gCampaign.Entry.Mode);
	return pd;
}

void PlayerDataTerminate(CArray *p)
{
	for (int i = 0; i < (int)p->size; i++)
	{
		PlayerTerminate(CArrayGet(p, i));
	}
	CArrayTerminate(p);
}

PlayerData *PlayerDataGetByUID(const int uid)
{
	if (uid == -1)
	{
		return NULL;
	}
	CA_FOREACH(PlayerData, p, gPlayerDatas)
		if (p->UID == uid) return p;
	CA_FOREACH_END()
	return NULL;
}

int GetNumPlayers(
	const PlayerAliveOptions alive, const bool human, const bool local)
{
	int numPlayers = 0;
	CA_FOREACH(const PlayerData, p, gPlayerDatas)
		bool life = false;
		switch (alive)
		{
		case PLAYER_ANY: life = true; break;
		case PLAYER_ALIVE: life = IsPlayerAlive(p); break;
		case PLAYER_ALIVE_OR_DYING: life = IsPlayerAliveOrDying(p); break;
		}
		if (life &&
			(!human || p->inputDevice != INPUT_DEVICE_AI) &&
			(!local || p->IsLocal))
		{
			numPlayers++;
		}
	CA_FOREACH_END()
	return numPlayers;
}

int GetNumPlayersScreen(const PlayerData **p)
{
	const bool humanOnly =
		IsPVP(gCampaign.Entry.Mode) ||
		!ConfigGetBool(&gConfig, "Interface.SplitscreenAI");
	const int n = GetNumPlayers(PLAYER_ALIVE_OR_DYING, humanOnly, true);
	if (n > 0 && p != NULL)
	{
		*p = GetFirstPlayer(true, humanOnly, true);
	}
	return n;
}

bool AreAllPlayersDeadAndNoLives(void)
{
	CA_FOREACH(const PlayerData, p, gPlayerDatas)
		if (IsPlayerAlive(p) || p->Lives > 0)
		{
			return false;
		}
	CA_FOREACH_END()
	return true;
}

const PlayerData *GetFirstPlayer(
	const bool alive, const bool human, const bool local)
{
	CA_FOREACH(const PlayerData, p, gPlayerDatas)
		if ((!alive || IsPlayerAliveOrDying(p)) &&
			(!human || p->inputDevice != INPUT_DEVICE_AI) &&
			(!local || p->IsLocal))
		{
			return p;
		}
	CA_FOREACH_END()
	return NULL;
}

bool IsPlayerAlive(const PlayerData *player)
{
	if (player->ActorUID == -1)
	{
		return false;
	}
	const TActor *p = ActorGetByUID(player->ActorUID);
	return !p->dead;
}
bool IsPlayerHuman(const PlayerData *player)
{
	return player->inputDevice != INPUT_DEVICE_AI;
}
bool IsPlayerHumanAndAlive(const PlayerData *player)
{
	return IsPlayerAlive(player) && IsPlayerHuman(player);
}
bool IsPlayerAliveOrDying(const PlayerData *player)
{
	if (player->ActorUID == -1)
	{
		return false;
	}
	const TActor *p = ActorGetByUID(player->ActorUID);
	return p->dead <= DEATH_MAX;
}
bool IsPlayerScreen(const PlayerData *p)
{
	const bool humanOnly =
		IsPVP(gCampaign.Entry.Mode) ||
		!ConfigGetBool(&gConfig, "Interface.SplitscreenAI");
	const bool humanOrScreen = !humanOnly || p->inputDevice != INPUT_DEVICE_AI;
	return p->IsLocal && humanOrScreen && IsPlayerAliveOrDying(p);
}

Vec2i PlayersGetMidpoint(void)
{
	// for all surviving players, find bounding rectangle, and get center
	Vec2i min;
	Vec2i max;
	PlayersGetBoundingRectangle(&min, &max);
	return Vec2iScaleDiv(Vec2iAdd(min, max), 2);
}

void PlayersGetBoundingRectangle(Vec2i *min, Vec2i *max)
{
	bool isFirst = true;
	*min = Vec2iZero();
	*max = Vec2iZero();
	const bool humansOnly =
		GetNumPlayers(PLAYER_ALIVE_OR_DYING, true, false) > 0;
	CA_FOREACH(const PlayerData, p, gPlayerDatas)
		if (!p->IsLocal)
		{
			continue;
		}
		if (humansOnly ? IsPlayerHumanAndAlive(p) : IsPlayerAlive(p))
		{
			const TActor *player = ActorGetByUID(p->ActorUID);
			const TTileItem *ti = &player->tileItem;
			if (isFirst)
			{
				*min = *max = Vec2iNew(ti->x, ti->y);
			}
			else
			{
				if (ti->x < min->x)	min->x = ti->x;
				if (ti->y < min->y)	min->y = ti->y;
				if (ti->x > max->x)	max->x = ti->x;
				if (ti->y > max->y)	max->y = ti->y;
			}
			isFirst = false;
		}
	CA_FOREACH_END()
}

// Get the number of players that use this ammo
int PlayersNumUseAmmo(const int ammoId)
{
	int numPlayersWithAmmo = 0;
	CA_FOREACH(const PlayerData, p, gPlayerDatas)
		if (!IsPlayerAlive(p))
		{
			continue;
		}
		const TActor *player = ActorGetByUID(p->ActorUID);
		for (int j = 0; j < (int)player->guns.size; j++)
		{
			const Weapon *w = CArrayGet(&player->guns, j);
			if (w->Gun->AmmoId == ammoId)
			{
				numPlayersWithAmmo++;
			}
		}
	CA_FOREACH_END()
	return numPlayersWithAmmo;
}

bool PlayerIsLocal(const int uid)
{
	const PlayerData *p = PlayerDataGetByUID(uid);
	return p != NULL && p->IsLocal;
}

void PlayerScore(PlayerData *p, const int points)
{
	if (p == NULL)
	{
		return;
	}
	p->Stats.Score += points;
	p->Totals.Score += points;
}

bool PlayerTrySetInputDevice(
	PlayerData *p, const input_device_e d, const int idx)
{
	if (p->inputDevice == d && p->deviceIndex == idx)
	{
		return false;
	}
	p->inputDevice = d;
	p->deviceIndex = idx;
	LOG(LM_MAIN, LL_DEBUG, "playerUID(%d) assigned input device(%d %d)",
		p->UID, (int)d, idx);
	return true;
}

bool PlayerTrySetUnusedInputDevice(
	PlayerData *p, const input_device_e d, const int idx)
{
	// Check that player's input device is unassigned
	if (p->inputDevice != INPUT_DEVICE_UNSET) return false;
	// Check that no players use this input device
	CA_FOREACH(const PlayerData, pOther, gPlayerDatas)
		if (pOther->inputDevice == d && pOther->deviceIndex == idx)
		{
			return false;
		}
	CA_FOREACH_END()
	return PlayerTrySetInputDevice(p, d, idx);
}
