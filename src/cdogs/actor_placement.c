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


static NVec2i PlaceActor(Map *map)
{
	Vec2i pos;

	if (HasExit(gCampaign.Entry.Mode))
	{
		// First, try to place at least half the map away from the exit
		const int halfMap =
			MAX(map->Size.x * TILE_WIDTH, map->Size.y * TILE_HEIGHT) / 2;
		const Vec2i exitPos = MapGetExitPos(map);
		// Don't try forever trying to place
		for (int i = 0; i < 100; i++)
		{
			const Vec2i realPos = Vec2iNew(
				rand() % (map->Size.x * TILE_WIDTH),
				rand() % (map->Size.y * TILE_HEIGHT));
			pos = Vec2iFull2Real(realPos);
			if (abs(realPos.x - exitPos.x) > halfMap &&
				abs(realPos.y - exitPos.y) > halfMap &&
				MapIsTileAreaClear(map, pos, Vec2iNew(ACTOR_W, ACTOR_H)))
			{
				return Vec2i2Net(pos);
			}
		}
	}

	// Try to place randomly
	do
	{
		pos.x = ((rand() % (map->Size.x * TILE_WIDTH)) << 8);
		pos.y = ((rand() % (map->Size.y * TILE_HEIGHT)) << 8);
	} while (!MapIsFullPosOKforPlayer(map, pos, false) ||
		!MapIsTileAreaClear(map, pos, Vec2iNew(ACTOR_W, ACTOR_H)));
	return Vec2i2Net(pos);
}

static NVec2i PlaceActorNear(
	Map *map, const Vec2i nearPos, const bool allowAllTiles)
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
	pos = Vec2iAdd(nearPos, Vec2iNew(dx, dy));\
	if (MapIsFullPosOKforPlayer(map, pos, allowAllTiles) && \
		MapIsTileAreaClear(map, pos, Vec2iNew(ACTOR_W, ACTOR_H)))\
	{\
		return Vec2i2Net(pos);\
	}
	int dx = 0;
	int dy = 0;
	Vec2i pos;
	TRY_LOCATION();
	int inc = 1 << 8;
	for (int radius = 12 << 8;; radius += 12 << 8)
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

NVec2i PlaceAwayFromPlayers(Map *map, const bool giveUp)
{
	// Don't try forever trying to place
	for (int i = 0; i < 100; i++)
	{
		// Try spawning out of players' sights
		const Vec2i pos = Vec2iReal2Full(Vec2iNew(
			rand() % (map->Size.x * TILE_WIDTH),
			rand() % (map->Size.y * TILE_HEIGHT)));
		const TActor *closestPlayer = AIGetClosestPlayer(pos);
		if (closestPlayer && CHEBYSHEV_DISTANCE(
			pos.x, pos.y,
			closestPlayer->Pos.x, closestPlayer->Pos.y) >= 256 * 150 &&
			MapIsTileAreaClear(map, pos, Vec2iNew(ACTOR_W, ACTOR_H)))
		{
			return Vec2i2Net(pos);
		}
	}
	// Keep trying, but this time try spawning anywhere,
	// even close to player
	for (int i = 0; i < 10000 || !giveUp; i++)
	{
		const Vec2i pos = Vec2iReal2Full(Vec2iNew(
			rand() % (map->Size.x * TILE_WIDTH),
			rand() % (map->Size.y * TILE_HEIGHT)));
		if (MapIsTileAreaClear(map, pos, Vec2iNew(ACTOR_W, ACTOR_H)))
		{
			return Vec2i2Net(pos);
		}
	}
	// Uh oh
	return Vec2i2Net(Vec2iZero());
}

NVec2i PlacePrisoner(Map *map)
{
	Vec2i fullPos;
	do
	{
		do
		{
			fullPos.x = ((rand() % (map->Size.x * TILE_WIDTH)) << 8);
			fullPos.y = ((rand() % (map->Size.y * TILE_HEIGHT)) << 8);
		} while (!MapPosIsInLockedRoom(map, Vec2iFull2Real(fullPos)));
	} while (!MapIsTileAreaClear(map, fullPos, Vec2iNew(ACTOR_W, ACTOR_H)));
	return Vec2i2Net(fullPos);
}

Vec2i PlacePlayer(
	Map *map, const PlayerData *p, const Vec2i firstPos, const bool pumpEvents)
{
	NActorAdd aa = NActorAdd_init_default;
	aa.UID = ActorsGetNextUID();
	aa.Health = p->Char.maxHealth;
	aa.PlayerUID = p->UID;

	if (IsPVP(gCampaign.Entry.Mode))
	{
		// In a PVP mode, always place players apart
		aa.FullPos = PlaceAwayFromPlayers(&gMap, false);
	}
	else if (
		ConfigGetEnum(&gConfig, "Interface.Splitscreen") == SPLITSCREEN_NEVER &&
		!Vec2iIsZero(firstPos))
	{
		// If never split screen, try to place players near the first player
		aa.FullPos = PlaceActorNear(map, firstPos, true);
	}
	else if (gMission.missionData->Type == MAPTYPE_STATIC &&
		!Vec2iIsZero(gMission.missionData->u.Static.Start))
	{
		// place players near the start point
		Vec2i startPoint = Vec2iReal2Full(Vec2iCenterOfTile(
			gMission.missionData->u.Static.Start));
		aa.FullPos = PlaceActorNear(map, startPoint, true);
	}
	else
	{
		aa.FullPos = PlaceActor(map);
	}

	GameEvent e = GameEventNew(GAME_EVENT_ACTOR_ADD);
	e.u.ActorAdd = aa;
	GameEventsEnqueue(&gGameEvents, e);

	if (pumpEvents)
	{
		// Process the events that actually place the players
		HandleGameEvents(&gGameEvents, NULL, NULL, NULL);
	}

	return Vec2iNew(aa.FullPos.x, aa.FullPos.y);
}
