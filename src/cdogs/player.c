/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (c) 2014, Cong Xu
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
#include "player_template.h"


CArray gPlayerDatas;


void PlayerDataInit(CArray *p)
{
	CArrayInit(p, sizeof(PlayerData));
}

PlayerData *PlayerDataAdd(CArray *p, const bool isLocal)
{
	PlayerData d;
	memset(&d, 0, sizeof d);
	Character c;
	memset(&c, 0, sizeof c);

	int i = (int)p->size;
	d.IsLocal = isLocal;
	// Set default player 1 controls, as it's used in menus
	if (i == 0)
	{
		d.inputDevice = INPUT_DEVICE_KEYBOARD;
		d.deviceIndex = 0;
	}

	// load from template if available
	if ((int)gPlayerTemplates.size > i)
	{
		const PlayerTemplate *t = CArrayGet(&gPlayerTemplates, i);
		strcpy(d.name, t->name);
		c.looks = t->Looks;
	}
	else
	{
		switch (i)
		{
		case 0:
			strcpy(d.name, "Jones");
			c.looks.face = FACE_JONES;
			c.looks.skin = SHADE_SKIN;
			c.looks.arm = SHADE_BLUE;
			c.looks.body = SHADE_BLUE;
			c.looks.leg = SHADE_BLUE;
			c.looks.hair = SHADE_RED;
			break;
		case 1:
			strcpy(d.name, "Ice");
			c.looks.face = FACE_ICE;
			c.looks.skin = SHADE_DARKSKIN;
			c.looks.arm = SHADE_RED;
			c.looks.body = SHADE_RED;
			c.looks.leg = SHADE_RED;
			c.looks.hair = SHADE_RED;
			break;
		case 2:
			strcpy(d.name, "Delta");
			c.looks.face = FACE_WARBABY;
			c.looks.skin = SHADE_SKIN;
			c.looks.arm = SHADE_GREEN;
			c.looks.body = SHADE_GREEN;
			c.looks.leg = SHADE_GREEN;
			c.looks.hair = SHADE_RED;
			break;
		case 3:
			strcpy(d.name, "Hans");
			c.looks.face = FACE_HAN;
			c.looks.skin = SHADE_ASIANSKIN;
			c.looks.arm = SHADE_YELLOW;
			c.looks.body = SHADE_YELLOW;
			c.looks.leg = SHADE_YELLOW;
			c.looks.hair = SHADE_GOLDEN;
			break;
		default:
			// Set up player N template
			sprintf(d.name, "Player %d", i);
			c.looks.face = FACE_JONES;
			c.looks.skin = SHADE_SKIN;
			c.looks.arm = SHADE_BLUE;
			c.looks.body = SHADE_BLUE;
			c.looks.leg = SHADE_BLUE;
			c.looks.hair = SHADE_RED;
			break;
		}
	}

	// weapons
	switch (i)
	{
	case 0:
		d.weapons[0] = StrGunDescription("Shotgun");
		d.weapons[1] = StrGunDescription("Machine gun");
		d.weapons[2] = StrGunDescription("Shrapnel bombs");
		break;
	case 1:
		d.weapons[0] = StrGunDescription("Powergun");
		d.weapons[1] = StrGunDescription("Flamer");
		d.weapons[2] = StrGunDescription("Grenades");
		break;
	case 2:
		d.weapons[0] = StrGunDescription("Sniper rifle");
		d.weapons[1] = StrGunDescription("Knife");
		d.weapons[2] = StrGunDescription("Molotovs");
		break;
	case 3:
		d.weapons[0] = StrGunDescription("Machine gun");
		d.weapons[1] = StrGunDescription("Flamer");
		d.weapons[2] = StrGunDescription("Dynamite");
		break;
	default:
		d.weapons[0] = StrGunDescription("Shotgun");
		d.weapons[1] = StrGunDescription("Machine gun");
		d.weapons[2] = StrGunDescription("Shrapnel bombs");
		break;
	}
	d.weaponCount = 3;
	d.playerIndex = i;

	c.speed = 256;
	c.maxHealth = 200;

	CArrayPushBack(p, &d);
	CArrayPushBack(&gCampaign.Setting.characters.Players, &c);
	CASSERT(p->size == gCampaign.Setting.characters.Players.size,
		"Player data and character sizes inconsistent");
	return CArrayGet(p, (int)p->size - 1);
}

void PlayerDataReset(CArray *p)
{
	for (int i = 0; i < (int)gPlayerDatas.size; i++)
	{
		PlayerData *pd = CArrayGet(p, i);
		pd->totalScore = 0;
		pd->missions = 0;
	}
}

void PlayerDataTerminate(CArray *p)
{
	CArrayTerminate(p);
}

int GetNumPlayers(const bool alive, const bool human, const bool local)
{
	int numPlayers = 0;
	for (int i = 0; i < (int)gPlayerDatas.size; i++)
	{
		const PlayerData *p = CArrayGet(&gPlayerDatas, i);
		if ((!alive || IsPlayerAlive(p)) &&
			(!human || p->inputDevice != INPUT_DEVICE_AI) &&
			(!local || p->IsLocal))
		{
			numPlayers++;
		}
	}
	return numPlayers;
}

const PlayerData *GetFirstPlayer(
	const bool alive, const bool human, const bool local)
{
	for (int i = 0; i < (int)gPlayerDatas.size; i++)
	{
		const PlayerData *p = CArrayGet(&gPlayerDatas, i);
		if ((!alive || IsPlayerAlive(p)) &&
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
	if (player->Id == -1)
	{
		return false;
	}
	const TActor *p = CArrayGet(&gActors, player->Id);
	return !p->dead;
}
bool IsPlayerHumanAndAlive(const PlayerData *player)
{
	return IsPlayerAlive(player) && player->inputDevice != INPUT_DEVICE_AI;
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
	const bool humansOnly = GetNumPlayers(true, true, false) > 0;
	for (int i = 0; i < (int)gPlayerDatas.size; i++)
	{
		const PlayerData *p = CArrayGet(&gPlayerDatas, i);
		if (!p->IsLocal)
		{
			continue;
		}
		if (humansOnly ? IsPlayerHumanAndAlive(p) : IsPlayerAlive(p))
		{
			const TActor *player = CArrayGet(&gActors, p->Id);
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
