/*
 C-Dogs SDL
 A port of the legendary (and fun) action/arcade cdogs.
 
 Copyright (c) 2014-2017 Cong Xu
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


static NVec2 PlaceActor(Map *map)
{
	struct vec2 pos;

	if (HasExit(gCampaign.Entry.Mode))
	{
		// First, try to place at least half the map away from the exit
		const int halfMap =
			MAX(map->Size.x * TILE_WIDTH, map->Size.y * TILE_HEIGHT) / 2;
		const struct vec2 exitPos = MapGetExitPos(map);
		// Don't try forever trying to place
		for (int i = 0; i < 100; i++)
		{
			pos = MapGetRandomPos(map);
			if (fabsf(pos.x - exitPos.x) > halfMap &&
				fabsf(pos.y - exitPos.y) > halfMap &&
				MapIsTileAreaClear(map, pos, svec2i(ACTOR_W, ACTOR_H)))
			{
				return Vec2ToNet(pos);
			}
		}
	}

	// Try to place randomly
	do
	{
		pos = MapGetRandomPos(map);
	} while (!MapIsPosOKForPlayer(map, pos, false) ||
		!MapIsTileAreaClear(map, pos, svec2i(ACTOR_W, ACTOR_H)));
	return Vec2ToNet(pos);
}

static NVec2 PlaceActorNear(
	Map *map, const struct vec2 nearPos, const bool allowAllTiles)
{
	// Try a concentric rhombus pattern, clockwise from right
	// That is, start by checking right, below, left, above,
	// then continue with radius 2 right, below-right, below, below-left...
	// (start from S:)
	//      4
	//  9 3 S 1 5
	//    8 2 6
	//      7
#define TRY_LOCATION()\
	pos = svec2_add(nearPos, svec2(dx, dy));\
	if (MapIsPosOKForPlayer(map, pos, allowAllTiles) && \
		MapIsTileAreaClear(map, pos, svec2i(ACTOR_W, ACTOR_H)))\
	{\
		return Vec2ToNet(pos);\
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
	Map *map, const struct vec2 pos, void *data);
NVec2 PlaceAwayFromPlayers(
	Map *map, const bool giveUp, const PlacementAccessFlags paFlags)
{
	NVec2 out;
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
			out = Vec2ToNet(pos);
			return out;
		}
	}

	// Uh oh
	// TODO: scan map for a safe position, to use as default
	return Vec2ToNet(svec2(TILE_WIDTH * 3 / 2, TILE_HEIGHT * 3 / 2));
}
static bool TryPlaceOneAwayFromPlayers(
	Map *map, const struct vec2 pos, void *data)
{
	NVec2 *out = data;
	// Try spawning out of players' sights
	*out = Vec2ToNet(pos);

	const TActor *closestPlayer = AIGetClosestPlayer(pos);
	if ((closestPlayer == NULL || CHEBYSHEV_DISTANCE(
			pos.x, pos.y,
			closestPlayer->Pos.x, closestPlayer->Pos.y) >= 150) &&
		MapIsTileAreaClear(map, pos, svec2i(ACTOR_W, ACTOR_H)))
	{
		*out = Vec2ToNet(pos);
		return true;
	}
	return false;
}

NVec2 PlacePrisoner(Map *map)
{
	struct vec2 pos;
	do
	{
		do
		{
			pos = MapGetRandomPos(map);
		} while (!MapPosIsInLockedRoom(map, pos));
	} while (!MapIsTileAreaClear(map, pos, svec2i(ACTOR_W, ACTOR_H)));
	return Vec2ToNet(pos);
}

struct vec2 PlacePlayer(
	Map *map, const PlayerData *p, const struct vec2 firstPos,
	const bool pumpEvents)
{
	NActorAdd aa = NActorAdd_init_default;
	aa.UID = ActorsGetNextUID();
	aa.Health = p->Char.maxHealth;
	aa.PlayerUID = p->UID;

	if (IsPVP(gCampaign.Entry.Mode))
	{
		// In a PVP mode, always place players apart
		aa.Pos = PlaceAwayFromPlayers(&gMap, false, PLACEMENT_ACCESS_ANY);
	}
	else if (
		ConfigGetEnum(&gConfig, "Interface.Splitscreen") == SPLITSCREEN_NEVER &&
		!svec2_is_zero(firstPos))
	{
		// If never split screen, try to place players near the first player
		aa.Pos = PlaceActorNear(map, firstPos, true);
	}
	else if (gMission.missionData->Type == MAPTYPE_STATIC &&
		!svec2i_is_zero(gMission.missionData->u.Static.Start))
	{
		// place players near the start point
		const struct vec2 startPoint = Vec2CenterOfTile(
			gMission.missionData->u.Static.Start);
		aa.Pos = PlaceActorNear(map, startPoint, true);
	}
	else
	{
		aa.Pos = PlaceActor(map);
	}

	GameEvent e = GameEventNew(GAME_EVENT_ACTOR_ADD);
	e.u.ActorAdd = aa;
	GameEventsEnqueue(&gGameEvents, e);

	if (pumpEvents)
	{
		// Process the events that actually place the players
		HandleGameEvents(&gGameEvents, NULL, NULL, NULL);
	}

	return svec2(aa.Pos.x, aa.Pos.y);
}
