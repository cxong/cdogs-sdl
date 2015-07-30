/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (c) 2014-2015, Cong Xu
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
	p->Char.looks = pd.Looks;
	CharacterSetColors(&p->Char);
	p->weaponCount = pd.Weapons_count;
	for (int i = 0; i < (int)pd.Weapons_count; i++)
	{
		p->weapons[i] = StrGunDescription(pd.Weapons[i]);
	}
	p->Lives = pd.Lives;
	p->score = pd.Score;
	p->totalScore = pd.Score;
	p->kills = pd.Kills;
	p->suicides = pd.Suicides;
	p->friendlies = pd.Friendlies;
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
		pd.Looks = t->Looks;
	}
	else
	{
		switch (idx)
		{
		case 0:
			strcpy(pd.Name, "Jones");
			pd.Looks.Face = FACE_JONES;
			pd.Looks.Skin = SHADE_SKIN;
			pd.Looks.Arm = SHADE_BLUE;
			pd.Looks.Body = SHADE_BLUE;
			pd.Looks.Leg = SHADE_BLUE;
			pd.Looks.Hair = SHADE_RED;
			break;
		case 1:
			strcpy(pd.Name, "Ice");
			pd.Looks.Face = FACE_ICE;
			pd.Looks.Skin = SHADE_DARKSKIN;
			pd.Looks.Arm = SHADE_RED;
			pd.Looks.Body = SHADE_RED;
			pd.Looks.Leg = SHADE_RED;
			pd.Looks.Hair = SHADE_RED;
			break;
		case 2:
			strcpy(pd.Name, "Delta");
			pd.Looks.Face = FACE_WARBABY;
			pd.Looks.Skin = SHADE_SKIN;
			pd.Looks.Arm = SHADE_GREEN;
			pd.Looks.Body = SHADE_GREEN;
			pd.Looks.Leg = SHADE_GREEN;
			pd.Looks.Hair = SHADE_RED;
			break;
		case 3:
			strcpy(pd.Name, "Hans");
			pd.Looks.Face = FACE_HAN;
			pd.Looks.Skin = SHADE_ASIANSKIN;
			pd.Looks.Arm = SHADE_YELLOW;
			pd.Looks.Body = SHADE_YELLOW;
			pd.Looks.Leg = SHADE_YELLOW;
			pd.Looks.Hair = SHADE_GOLDEN;
			break;
		default:
			// Set up player N template
			sprintf(pd.Name, "Player %d", idx);
			pd.Looks.Face = FACE_JONES;
			pd.Looks.Skin = SHADE_SKIN;
			pd.Looks.Arm = SHADE_BLUE;
			pd.Looks.Body = SHADE_BLUE;
			pd.Looks.Leg = SHADE_BLUE;
			pd.Looks.Hair = SHADE_RED;
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

	pd.Score = 0;
	pd.Kills = 0;
	pd.Suicides = 0;
	pd.Friendlies = 0;

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
	for (int i = 0; i < (int)gPlayerDatas.size; i++)
	{
		PlayerData *p = CArrayGet(&gPlayerDatas, i);
		if (p->UID == uid) return p;
	}
	return NULL;
}

int GetNumPlayers(
	const PlayerAliveOptions alive, const bool human, const bool local)
{
	int numPlayers = 0;
	for (int i = 0; i < (int)gPlayerDatas.size; i++)
	{
		const PlayerData *p = CArrayGet(&gPlayerDatas, i);
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
	}
	return numPlayers;
}

bool AreAllPlayersDeadAndNoLives(void)
{
	for (int i = 0; i < (int)gPlayerDatas.size; i++)
	{
		const PlayerData *p = CArrayGet(&gPlayerDatas, i);
		if (IsPlayerAlive(p) || p->Lives > 0)
		{
			return false;
		}
	}
	return true;
}

const PlayerData *GetFirstPlayer(
	const bool alive, const bool human, const bool local)
{
	for (int i = 0; i < (int)gPlayerDatas.size; i++)
	{
		const PlayerData *p = CArrayGet(&gPlayerDatas, i);
		if ((!alive || IsPlayerAliveOrDying(p)) &&
			(!human || p->inputDevice != INPUT_DEVICE_AI) &&
			(!local || p->IsLocal))
		{
			return p;
		}
	}
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
	for (int i = 0; i < (int)gPlayerDatas.size; i++)
	{
		const PlayerData *p = CArrayGet(&gPlayerDatas, i);
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
	}
}

// Get the number of players that use this ammo
int PlayersNumUseAmmo(const int ammoId)
{
	int numPlayersWithAmmo = 0;
	for (int i = 0; i < (int)gPlayerDatas.size; i++)
	{
		const PlayerData *p = CArrayGet(&gPlayerDatas, i);
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
	}
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
	p->score += points;
	p->totalScore += points;
}

void PlayerSetInputDevice(
	PlayerData *p, const input_device_e d, const int idx)
{
	p->inputDevice = d;
	p->deviceIndex = idx;
}
