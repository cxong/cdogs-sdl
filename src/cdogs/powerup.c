/*
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
#include "powerup.h"

#include <math.h>

#include "ai_utils.h"
#include "ammo.h"
#include "gamedata.h"
#include "game_events.h"


#define TIME_DECAY_EXPONENT 1.04
#define HEALTH_W 6
#define HEALTH_H 6
#define MAX_TILES_PER_PICKUP 625

void PowerupSpawnerInit(PowerupSpawner *p, Map *map)
{
	memset(p, 0, sizeof *p);
	p->map = map;
	p->timer = 0;
	p->numPickups = 0;
	p->pickupsSpawned = 0;
	p->Enabled = true;
}
void PowerupSpawnerTerminate(PowerupSpawner *p)
{
	CFREE(p->Data);
}

static bool TryPlacePickup(PowerupSpawner *p);
void PowerupSpawnerUpdate(PowerupSpawner *p, const int ticks)
{
	// Don't spawn pickups if not allowed
	if (!p->Enabled)
	{
		return;
	}

	double scalar = p->RateScaleFunc(p->Data);
	// Scale down over time
	scalar *= pow(TIME_DECAY_EXPONENT, p->pickupsSpawned);

	p->timeUntilNextSpawn = (int)floor(scalar * p->SpawnTime);

	// Update time
	p->timer += ticks;

	// Attempt to add pickup if time reached, and we haven't placed too many
	if (p->timer >= p->timeUntilNextSpawn &&
		p->map->NumExplorableTiles / MAX_TILES_PER_PICKUP + 1 > p->numPickups)
	{
		p->timer -= p->timeUntilNextSpawn;

		if (TryPlacePickup(p))
		{
			p->pickupsSpawned++;
			p->numPickups++;
		}
	}
}
static bool TryPlacePickup(PowerupSpawner *p)
{
	const Vec2i size = Vec2iNew(HEALTH_W, HEALTH_H);
	// Attempt to place one in out-of-sight area
	for (int i = 0; i < 100; i++)
	{
		const Vec2i v = MapGenerateFreePosition(p->map, size);
		const Vec2i fullpos = Vec2iReal2Full(v);
		const TActor *closestPlayer = AIGetClosestPlayer(fullpos);
		if (!Vec2iIsZero(v) &&
			(!closestPlayer || CHEBYSHEV_DISTANCE(
			fullpos.x, fullpos.y,
			closestPlayer->Pos.x, closestPlayer->Pos.y) >= 256 * 150))
		{
			p->PlaceFunc(v, p->Data);
			return true;
		}
	}
	// Attempt to place one anyway
	for (int i = 0; i < 100; i++)
	{
		const Vec2i v = MapGenerateFreePosition(p->map, size);
		if (!Vec2iIsZero(v))
		{
			p->PlaceFunc(v, p->Data);
			return true;
		}
	}
	return false;
}

void PowerupSpawnerRemoveOne(PowerupSpawner *p)
{
	CASSERT(p->numPickups > 0, "unexpectedly removing powerup");
	p->numPickups--;
}


#define HEALTH_SPAWN_TIME (20 * FPS_FRAMELIMIT)

static double HealthScale(void *data);
static void HealthPlace(const Vec2i pos, void *data);
void HealthSpawnerInit(PowerupSpawner *p, Map *map)
{
	PowerupSpawnerInit(p, map);
	p->Enabled =
		AreHealthPickupsAllowed(gCampaign.Entry.Mode) &&
		ConfigGetBool(&gConfig, "Game.HealthPickups");
	p->SpawnTime = HEALTH_SPAWN_TIME;
	p->RateScaleFunc = HealthScale;
	p->PlaceFunc = HealthPlace;
	
	// Update once
	PowerupSpawnerUpdate(p, 0);
}
static double HealthScale(void *data)
{
	UNUSED(data);
	// Update time until next spawn based on:
	// Damage taken (find player with lowest health)
	int minHealth = ModeMaxHealth(gCampaign.Entry.Mode);
	int maxHealth = minHealth;
	for (int i = 0; i < (int)gPlayerDatas.size; i++)
	{
		const PlayerData *p = CArrayGet(&gPlayerDatas, i);
		if (!IsPlayerAlive(p))
		{
			continue;
		}
		const TActor *player = CArrayGet(&gActors, p->Id);
		minHealth = MIN(minHealth, player->health);
	}
	// Double spawn rate if near 0 health
	return (minHealth + maxHealth) / (maxHealth * 2.0);
}
static void HealthPlace(const Vec2i pos, void *data)
{
	UNUSED(data);
	GameEvent e = GameEventNew(GAME_EVENT_ADD_PICKUP);
	e.u.AddPickup.Pos = pos;
	e.u.AddPickup.PickupClassId = StrPickupClassId("health");
	e.u.AddPickup.IsRandomSpawned = true;
	GameEventsEnqueue(&gGameEvents, e);
}


#define AMMO_SPAWN_TIME (20 * FPS_FRAMELIMIT)

static double AmmoScale(void *data);
static void AmmoPlace(const Vec2i pos, void *data);
void AmmoSpawnerInit(PowerupSpawner *p, Map *map, const int ammoId)
{
	PowerupSpawnerInit(p, map);
	// TODO: disable ammo spawners unless classic mode
	p->Enabled =
		!IsPVP(gCampaign.Entry.Mode) && ConfigGetBool(&gConfig, "Game.Ammo");
	p->SpawnTime = AMMO_SPAWN_TIME;
	p->RateScaleFunc = AmmoScale;
	p->PlaceFunc = AmmoPlace;
	CMALLOC(p->Data, sizeof ammoId);
	*(int *)p->Data = ammoId;

	// Update once
	PowerupSpawnerUpdate(p, 0);
}
static double AmmoScale(void *data)
{
	const int ammoId = *(int *)data;
	// Update time until next spawn based on:
	// Ammo left (find player with lowest ammo)
	// But make sure at least one player has a gun that uses this ammo
	int minVal = AmmoGetById(&gAmmo, ammoId)->Max;
	int maxVal = minVal;
	int numPlayersWithAmmo = 0;
	for (int i = 0; i < (int)gPlayerDatas.size; i++)
	{
		const PlayerData *p = CArrayGet(&gPlayerDatas, i);
		if (!IsPlayerAlive(p))
		{
			continue;
		}
		const TActor *player = CArrayGet(&gActors, p->Id);
		for (int j = 0; j < (int)player->guns.size; j++)
		{
			const Weapon *w = CArrayGet(&player->guns, j);
			if (w->Gun->AmmoId == ammoId)
			{
				numPlayersWithAmmo++;
			}
		}
		minVal = MIN(minVal, *(int *)CArrayGet(&player->ammo, ammoId));
	}
	if (numPlayersWithAmmo > 0)
	{
		// Double spawn rate if near 0 ammo
		return (minVal + maxVal) / (maxVal * 2.0) / numPlayersWithAmmo;
	}
	else
	{
		// No players have guns that use this ammo; spawn very slowly
		return 5.0;
	}
}
static void AmmoPlace(const Vec2i pos, void *data)
{
	const int ammoId = *(int *)data;
	GameEvent e = GameEventNew(GAME_EVENT_ADD_PICKUP);
	e.u.AddPickup.Pos = pos;
	const Ammo *a = AmmoGetById(&gAmmo, ammoId);
	char buf[256];
	sprintf(buf, "ammo_%s", a->Name);
	e.u.AddPickup.PickupClassId = StrPickupClassId(buf);
	e.u.AddPickup.IsRandomSpawned = true;
	GameEventsEnqueue(&gGameEvents, e);
}
