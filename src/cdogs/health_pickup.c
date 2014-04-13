/*
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
#include "health_pickup.h"

#include <math.h>

#include "ai_utils.h"


#define SPAWN_TIME (20 * FPS_FRAMELIMIT)
#define TIME_DECAY_EXPONENT 1.04
#define HEALTH_W 6
#define HEALTH_H 6
#define MAX_TILES_PER_PICKUP 625

void HealthPickupsInit(HealthPickups *h, Map *map)
{
	h->map = map;
	h->timer = 0;
	h->numPickups = 0;
	h->pickupsSpawned = 0;

	// Update once
	HealthPickupsUpdate(h, 0);
}

static bool TryPlacePickup(HealthPickups *h);
void HealthPickupsUpdate(HealthPickups *h, int ticks)
{
	// Don't spawn pickups if not allowed
	if (!AreHealthPickupsAllowed(gCampaign.Entry.mode) ||
		!gConfig.Game.HealthPickups)
	{
		return;
	}

	double scalar = 1.0;
	// Update time until next spawn based on:
	// Damage taken (find player with lowest health)
	int minHealth = 200 * gConfig.Game.PlayerHP / 100;
	int maxHealth = 200 * gConfig.Game.PlayerHP / 100;
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		if (IsPlayerAlive(i))
		{
			TActor *player = CArrayGet(&gActors, gPlayerIds[i]);
			minHealth = MIN(minHealth, player->health);
		}
	}
	// Double spawn rate if near 0 health
	scalar *= (minHealth + maxHealth) / (maxHealth * 2.0);

	// Scale down over time
	scalar *= pow(TIME_DECAY_EXPONENT, h->pickupsSpawned);

	h->timeUntilNextSpawn = (int)floor(scalar * SPAWN_TIME);

	// Update time
	h->timer += ticks;

	// Attempt to add health if time reached, and we haven't placed too many
	if (h->timer >= h->timeUntilNextSpawn &&
		h->map->NumExplorableTiles / MAX_TILES_PER_PICKUP + 1 > h->numPickups)
	{
		h->timer = 0;

		if (TryPlacePickup(h))
		{
			h->pickupsSpawned++;
			h->numPickups++;
		}
	}
}
static bool TryPlacePickup(HealthPickups *h)
{
	Vec2i size = Vec2iNew(HEALTH_W, HEALTH_H);
	// Attempt to place one in unexplored area
	for (int i = 0; i < 100; i++)
	{
		Vec2i v = MapGenerateFreePosition(h->map, size);
		if (!Vec2iEqual(v, Vec2iZero()) &&
			!MapGetTile(h->map, Vec2iToTile(v))->isVisited)
		{
			MapPlaceHealth(v);
			return true;
		}
	}
	// Attempt to place one in out-of-sight area
	for (int i = 0; i < 100; i++)
	{
		Vec2i v = MapGenerateFreePosition(h->map, size);
		Vec2i fullpos = Vec2iReal2Full(v);
		TActor *closestPlayer = AIGetClosestPlayer(fullpos);
		if (!Vec2iEqual(v, Vec2iZero()) &&
			(!closestPlayer || CHEBYSHEV_DISTANCE(
			fullpos.x, fullpos.y,
			closestPlayer->Pos.x, closestPlayer->Pos.y) >= 256 * 150))
		{
			MapPlaceHealth(v);
			return true;
		}
	}
	// Attempt to place one anyway
	for (int i = 0; i < 100; i++)
	{
		Vec2i v = MapGenerateFreePosition(h->map, size);
		if (!Vec2iEqual(v, Vec2iZero()))
		{
			MapPlaceHealth(v);
			return true;
		}
	}
	return false;
}

void HealthPickupsRemoveOne(HealthPickups *h)
{
	CASSERT(h->numPickups > 0, "unexpectedly removing health pickup");
	h->numPickups--;
}
