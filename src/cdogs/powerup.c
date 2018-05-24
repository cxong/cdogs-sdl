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

#include <float.h>
#include <math.h>

#include "ai_utils.h"
#include "ammo.h"
#include "gamedata.h"
#include "game_events.h"
#include "net_util.h"
#include "pickup.h"


#define TIME_DECAY_EXPONENT 1.04
#define HEALTH_W 6
#define HEALTH_H 6
#define MAX_TILES_PER_PICKUP 300

void PowerupSpawnerInit(PowerupSpawner *p, Map *map)
{
	memset(p, 0, sizeof *p);
	p->map = map;
	p->timer = 0;
	p->numPickups = 0;
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
	if (scalar == DBL_MAX)
	{
		p->timeUntilNextSpawn = INT_MAX;
	}
	else
	{
		// Scale down over time
		scalar *= pow(TIME_DECAY_EXPONENT, p->numPickups);

		p->timeUntilNextSpawn = (int)floor(scalar * p->SpawnTime);
	}

	// Update time
	p->timer += ticks;

	// Attempt to add pickup if time reached, and we haven't placed too many
	if (p->timer >= p->timeUntilNextSpawn &&
		p->map->NumExplorableTiles / MAX_TILES_PER_PICKUP + 1 > p->numPickups)
	{
		p->timer -= p->timeUntilNextSpawn;

		if (TryPlacePickup(p))
		{
			p->numPickups++;
		}
	}
}
static bool TryPlacePickup(PowerupSpawner *p)
{
	const struct vec2i size = svec2i(HEALTH_W, HEALTH_H);
	// Attempt to place one in out-of-sight area
	for (int i = 0; i < 100; i++)
	{
		const struct vec2 v = MapGenerateFreePosition(p->map, size);
		const TActor *closestPlayer = AIGetClosestPlayer(v);
		if (!svec2_is_zero(v) &&
			(!closestPlayer ||
			svec2_distance_squared(v, closestPlayer->Pos) >= SQUARED(150)))
		{
			p->PlaceFunc(v, p->Data);
			return true;
		}
	}
	// Attempt to place one anyway
	for (int i = 0; i < 100; i++)
	{
		const struct vec2 v = MapGenerateFreePosition(p->map, size);
		if (!svec2_is_zero(v))
		{
			p->PlaceFunc(v, p->Data);
			return true;
		}
	}
	return false;
}

void PowerupSpawnerRemoveOne(PowerupSpawner *p)
{
	p->numPickups = MAX(0, p->numPickups - 1);
}


#define HEALTH_SPAWN_TIME (20 * FPS_FRAMELIMIT)

static double HealthScale(void *data);
static void HealthPlace(const struct vec2 pos, void *data);
void HealthSpawnerInit(PowerupSpawner *p, Map *map)
{
	PowerupSpawnerInit(p, map);
	p->Enabled =
		AreHealthPickupsAllowed(gCampaign.Entry.Mode) &&
		ConfigGetBool(&gConfig, "Game.HealthPickups") &&
		!gCampaign.IsClient;
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
	CA_FOREACH(const PlayerData, p, gPlayerDatas)
		if (!IsPlayerAlive(p))
		{
			continue;
		}
		const TActor *player = ActorGetByUID(p->ActorUID);
		minHealth = MIN(minHealth, player->health);
	CA_FOREACH_END()
	// Double spawn rate if near 0 health
	return (minHealth + maxHealth) / (maxHealth * 2.0);
}
static void HealthPlace(const struct vec2 pos, void *data)
{
	UNUSED(data);
	GameEvent e = GameEventNew(GAME_EVENT_ADD_PICKUP);
	e.u.AddPickup.UID = PickupsGetNextUID();
	e.u.AddPickup.Pos = Vec2ToNet(pos);
	strcpy(e.u.AddPickup.PickupClass, "health");
	e.u.AddPickup.IsRandomSpawned = true;
	e.u.AddPickup.SpawnerUID = -1;
	e.u.AddPickup.ThingFlags = 0;
	GameEventsEnqueue(&gGameEvents, e);
}


#define AMMO_SPAWN_TIME (20 * FPS_FRAMELIMIT)

static double AmmoScale(void *data);
static void AmmoPlace(const struct vec2 pos, void *data);
void AmmoSpawnerInit(PowerupSpawner *p, Map *map, const int ammoId)
{
	PowerupSpawnerInit(p, map);
	// TODO: disable ammo spawners unless classic mode
	p->Enabled =
		ConfigGetBool(&gConfig, "Game.Ammo") &&
		!gCampaign.IsClient;
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
	// Make sure at least one player has a gun that uses this ammo
	const int numPlayersWithAmmo = PlayersNumUseAmmo(ammoId);
	if (numPlayersWithAmmo == 0)
	{
		// No players have guns that use this ammo; never spawn
		return DBL_MAX;
	}

	// Update time until next spawn based on:
	// Ammo left (find player with lowest ammo)
	int minVal = AmmoGetById(&gAmmo, ammoId)->Max;
	int maxVal = minVal;
	CA_FOREACH(const PlayerData, p, gPlayerDatas)
		if (!IsPlayerAlive(p))
		{
			continue;
		}
		const TActor *player = ActorGetByUID(p->ActorUID);
		minVal = MIN(minVal, *(int *)CArrayGet(&player->ammo, ammoId));
	CA_FOREACH_END()
	// 10-fold spawn rate if near 0 ammo
	return (minVal * 9.0 + maxVal) / (maxVal * 10.0) / numPlayersWithAmmo;
}
static void AmmoPlace(const struct vec2 pos, void *data)
{
	const int ammoId = *(int *)data;
	GameEvent e = GameEventNew(GAME_EVENT_ADD_PICKUP);
	e.u.AddPickup.UID = PickupsGetNextUID();
	e.u.AddPickup.Pos = Vec2ToNet(pos);
	const Ammo *a = AmmoGetById(&gAmmo, ammoId);
	sprintf(e.u.AddPickup.PickupClass, "ammo_%s", a->Name);
	e.u.AddPickup.IsRandomSpawned = true;
	e.u.AddPickup.SpawnerUID = -1;
	e.u.AddPickup.ThingFlags = 0;
	GameEventsEnqueue(&gGameEvents, e);
}
