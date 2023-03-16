/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.

	Copyright (c) 2014-2017, 2020-2021 Cong Xu
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
#include "actor_placement.h"

#include "actors.h"
#include "ai_utils.h"
#include "game_events.h"
#include "gamedata.h"
#include "handle_game_events.h"

static struct vec2 PlacePlayerSimple(const Map *map)
{
	const int halfMap =
		MAX(map->Size.x * TILE_WIDTH, map->Size.y * TILE_HEIGHT) / 2;
	int attemptsAwayFromExits = 0;

	struct vec2 pos = svec2_zero();
	bool ok = false;
	for (int j = 0; j < 10000 && !ok; j++)
	{
		pos = MapGetRandomPos(map);
		ok = MapIsPosOKForPlayer(map, pos, false);
		if (!ok)
			continue;
		if (attemptsAwayFromExits < 100)
		{
			attemptsAwayFromExits++;
			// Try to place at least half the map away from any exits
			for (int i = 0; i < (int)map->exits.size; i++)
			{
				const struct vec2 exitPos = MapGetExitPos(map, i);
				if (fabsf(pos.x - exitPos.x) > halfMap &&
					fabsf(pos.y - exitPos.y) > halfMap)
				{
					ok = false;
					break;
				}
			}
		}
	}
	return pos;
}

static struct vec2 PlaceActorNear(
	const Map *map, const struct vec2 nearPos, const bool allowAllTiles)
{
	// Try a concentric rhombus pattern, clockwise from right
	// That is, start by checking right, below, left, above,
	// then continue with radius 2 right, below-right, below, below-left...
	// (start from S:)
	//      4
	//  9 3 S 1 5
	//    8 2 6
	//      7
#define TRY_LOCATION()                                                        \
	pos = svec2_add(nearPos, svec2(dx, dy));                                  \
	if (MapIsPosOKForPlayer(map, pos, allowAllTiles))                         \
	{                                                                         \
		return pos;                                                \
	}
	float dx = 0;
	float dy = 0;
	struct vec2 pos;
	TRY_LOCATION();
	const float inc = 1;
	for (float radius = 12;; radius += 12)
	{
		// Going from right to below
		for (dx = radius, dy = 0; dy < radius; dx -= inc, dy += inc)
		{
			TRY_LOCATION();
		}
		// below to left
		for (dx = 0, dy = radius; dy > 0; dx -= inc, dy -= inc)
		{
			TRY_LOCATION();
		}
		// left to above
		for (dx = -radius, dy = 0; dx < 0; dx += inc, dy -= inc)
		{
			TRY_LOCATION();
		}
		// above to right
		for (dx = 0, dy = -radius; dy < 0; dx += inc, dy += inc)
		{
			TRY_LOCATION();
		}
	}
}

static bool TryPlaceOneAwayFromPlayers(
	const Map *map, const struct vec2 pos, void *data);
struct vec2 PlaceAwayFromPlayers(
	const Map *map, const bool giveUp, const PlacementAccessFlags paFlags)
{
	struct vec2 out;
	if (MapPlaceRandomPos(map, paFlags, TryPlaceOneAwayFromPlayers, &out))
	{
		return out;
	}

	// Keep trying, but this time try spawning anywhere,
	// even close to player
	for (int i = 0; i < 10000 || !giveUp; i++)
	{
		const struct vec2 pos = MapGetRandomPos(map);
		if (MapIsTileAreaClear(map, pos, svec2i(ACTOR_W, ACTOR_H)))
		{
			return pos;
		}
	}

	// Uh oh
	// TODO: scan map for a safe position, to use as default
	return svec2(TILE_WIDTH * 3 / 2, TILE_HEIGHT * 3 / 2);
}
static bool TryPlaceOneAwayFromPlayers(
	const Map *map, const struct vec2 pos, void *data)
{
	struct vec2 *out = data;
	// Try spawning out of players' sights
	*out = pos;

	const TActor *closestPlayer = AIGetClosestPlayer(pos);
	if ((closestPlayer == NULL || CHEBYSHEV_DISTANCE(
									  pos.x, pos.y, closestPlayer->Pos.x,
									  closestPlayer->Pos.y) >= 150) &&
		MapIsTileAreaClear(map, pos, svec2i(ACTOR_W, ACTOR_H)))
	{
		*out = pos;
		return true;
	}
	return false;
}

struct vec2 PlacePrisoner(const Map *map)
{
	struct vec2 pos;
	do
	{
		do
		{
			pos = MapGetRandomPos(map);
		} while (!MapPosIsInLockedRoom(map, pos));
	} while (!MapIsTileAreaClear(map, pos, svec2i(ACTOR_W, ACTOR_H)));
	return pos;
}

struct vec2 PlacePlayer(
	const Map *map, const PlayerData *p, const struct vec2 firstPos,
	const bool pumpEvents)
{
	struct vec2 pos;
	if (IsPVP(gCampaign.Entry.Mode))
	{
		// In a PVP mode, always place players apart
		pos = PlaceAwayFromPlayers(map, false, PLACEMENT_ACCESS_ANY);
	}
	else if (
		ConfigGetEnum(&gConfig, "Interface.Splitscreen") ==
			SPLITSCREEN_NEVER &&
		!svec2_is_zero(firstPos))
	{
		// If never split screen, try to place players near the first player
		pos = PlaceActorNear(map, firstPos, true);
	}
	else if (!svec2i_is_zero(map->start))
	{
		// place players near the start point
		const struct vec2 startPoint = Vec2CenterOfTile(map->start);
		pos = PlaceActorNear(map, startPoint, true);
	}
	else
	{
		pos = PlacePlayerSimple(map);
	}
	GameEvent e = GameEventNewActorAdd(pos, &p->Char, p);
	e.u.ActorAdd.Direction = DIRECTION_DOWN;
	e.u.ActorAdd.PlayerUID = p->UID;
	Ammo2Net(&e.u.ActorAdd.Ammo_count, e.u.ActorAdd.Ammo, &p->ammo);
	GameEventsEnqueue(&gGameEvents, e);

	if (pumpEvents)
	{
		// Process the events that actually place the players
		HandleGameEvents(&gGameEvents, NULL, NULL, NULL, NULL);
	}

	return NetToVec2(e.u.ActorAdd.Pos);
}
