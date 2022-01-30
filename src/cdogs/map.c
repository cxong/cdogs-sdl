/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.
	Copyright (C) 1995 Ronny Wester
	Copyright (C) 2003 Jeremy Chin
	Copyright (C) 2003-2007 Lucas Martin-King

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

	This file incorporates work covered by the following copyright and
	permission notice:

	Copyright (c) 2013-2022 Cong Xu
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
#include "map.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "actors.h"
#include "algorithms.h"
#include "ammo.h"
#include "collision/collision.h"
#include "config.h"
#include "door.h"
#include "game_events.h"
#include "gamedata.h"
#include "log.h"
#include "los.h"
#include "map_build.h"
#include "map_cave.h"
#include "map_classic.h"
#include "map_static.h"
#include "mission.h"
#include "net_util.h"
#include "objs.h"
#include "pic_manager.h"
#include "pickup.h"
#include "sounds.h"
#include "utils.h"

Map gMap;

const char *IMapTypeStr(IMapType t)
{
	switch (t)
	{
		T2S(MAP_FLOOR, "Floor");
		T2S(MAP_WALL, "Wall");
		T2S(MAP_DOOR, "Door");
		T2S(MAP_ROOM, "Room");
		T2S(MAP_NOTHING, "Nothing");
		T2S(MAP_SQUARE, "Square");
	default:
		return "";
	}
}
IMapType StrIMapType(const char *s)
{
	S2T(MAP_FLOOR, "Floor");
	S2T(MAP_WALL, "Wall");
	S2T(MAP_DOOR, "Door");
	S2T(MAP_ROOM, "Room");
	S2T(MAP_NOTHING, "Nothing");
	S2T(MAP_SQUARE, "Square");
	return MAP_FLOOR;
}

uint16_t GetAccessMask(const int k)
{
	if (k == -1)
	{
		return 0;
	}
	return MAP_ACCESS_YELLOW << k;
}

Tile *MapGetTile(const Map *map, const struct vec2i pos)
{
	if (!MapIsTileIn(map, pos))
	{
		return NULL;
	}
	return CArrayGet(&map->Tiles, pos.y * map->Size.x + pos.x);
}

bool MapIsTileIn(const Map *map, const struct vec2i pos)
{
	// Check that the tile pos is within the interior of the map
	return Rect2iIsInside(Rect2iNew(svec2i_zero(), map->Size), pos);
}
static bool MapIsPosIn(const Map *map, const struct vec2 pos)
{
	// Check that the pos is within the interior of the map
	return pos.x >= 0 && pos.y >= 0 && MapIsTileIn(map, Vec2ToTile(pos));
}

// If thing is in the exit-index exit
// If exit = -1 then check any exit
// Returns the exit index or -1 if not in any exit
int MapIsTileInExit(const Map *map, const Thing *ti, const int exit)
{
	if (exit < 0)
	{
		for (int i = 0; i < (int)map->exits.size; i++)
		{
			if (MapIsTileInExit(map, ti, i) != -1)
			{
				return i;
			}
		}
		return -1;
	}
	const struct vec2i tilePos = Vec2ToTile(ti->Pos);
	const Exit *e = CArrayGet(&map->exits, exit);
	// Outer edge is also in exit
	const Rect2i r = Rect2iNew(e->R.Pos, svec2i_add(e->R.Size, svec2i_one()));
	return Rect2iIsInside(r, tilePos) ? exit : -1;
}

static Tile *MapGetTileOfItem(Map *map, Thing *t)
{
	const struct vec2i pos = Vec2ToTile(t->Pos);
	return MapGetTile(map, pos);
}

static void AddItemToTile(Thing *t, Tile *tile);
bool MapTryMoveThing(Map *map, Thing *t, const struct vec2 pos)
{
	// Check if we can move to new position
	if (!MapIsPosIn(map, pos))
	{
		return false;
	}
	t->LastPos = t->Pos;
	// When first initialised, position is -1
	const bool doRemove = t->Pos.x >= 0 && t->Pos.y >= 0;
	const struct vec2i t1 = Vec2ToTile(t->Pos);
	const struct vec2i t2 = Vec2ToTile(pos);
	// If we'll be in the same tile, do nothing
	if (svec2i_is_equal(t1, t2) && doRemove)
	{
		t->Pos = pos;
		return true;
	}
	// Moving; remove from old tile...
	if (doRemove)
	{
		MapRemoveThing(map, t);
	}
	// ...move and add to new tile
	t->Pos = pos;
	AddItemToTile(t, MapGetTile(map, t2));
	return true;
}
static void AddItemToTile(Thing *t, Tile *tile)
{
	ThingId tid;
	tid.Id = t->id;
	tid.Kind = t->kind;
	CASSERT(tid.Id >= 0, "invalid ThingId");
	CASSERT(tid.Kind >= 0 && tid.Kind <= KIND_PICKUP, "unknown thing kind");
	CArrayPushBack(&tile->things, &tid);
}

void MapRemoveThing(Map *map, Thing *t)
{
	if (!MapIsPosIn(map, t->Pos))
	{
		return;
	}
	Tile *tile = MapGetTileOfItem(map, t);
	CA_FOREACH(ThingId, tid, tile->things)
	if (tid->Id == t->id && tid->Kind == t->kind)
	{
		CArrayDelete(&tile->things, _ca_index);
		return;
	}
	CA_FOREACH_END()
	CASSERT(false, "Did not find element to delete");
}

struct vec2i MapGetRandomTile(const Map *map)
{
	return svec2i(rand() % map->Size.x, rand() % map->Size.y);
}

struct vec2 MapGetRandomPos(const Map *map)
{
	for (;;)
	{
		const struct vec2 pos = svec2(
			RAND_FLOAT(0, map->Size.x * TILE_WIDTH),
			RAND_FLOAT(0, map->Size.y * TILE_HEIGHT));
		// RAND_FLOAT can sometimes produce the max size
		if (pos.x < map->Size.x * TILE_WIDTH &&
			pos.y < map->Size.y * TILE_HEIGHT)
		{
			return pos;
		}
	}
}

static void MapChangeFloor(
	Map *map, const struct vec2i pos, const TileClass *normal,
	const TileClass *shadow)
{
	const Tile *tAbove = MapGetTile(map, svec2i(pos.x, pos.y - 1));
	const int canSeeTileAbove = !(pos.y > 0 && TileIsOpaque(tAbove));
	Tile *t = MapGetTile(map, pos);
	if (t == NULL || t->Class->Type != TILE_CLASS_FLOOR)
	{
		return;
	}
	if (!canSeeTileAbove)
	{
		t->Class = shadow;
	}
	else
	{
		t->Class = normal;
	}
}

bool MapHasExits(const Map *m)
{
	return m->exits.size > 0 && !IsPVP(gCampaign.Entry.Mode);
}

// Change the perimeter of tiles around the exit area
void MapShowExitArea(Map *map, const int i)
{
	const Exit *exit = CArrayGet(&map->exits, i);
	const int left = exit->R.Pos.x;
	const int right = left + exit->R.Size.x;
	const int top = exit->R.Pos.y;
	const int bottom = top + exit->R.Size.y;

	const TileClass *exitClass = TileClassesGetExit(
		map->TileClasses, &gPicManager, gMission.missionData->ExitStyle, false);
	const TileClass *exitShadowClass = TileClassesGetExit(
		map->TileClasses, &gPicManager, gMission.missionData->ExitStyle, true);

	struct vec2i v;
	v.y = top;
	for (v.x = left; v.x <= right; v.x++)
	{
		MapChangeFloor(map, v, exitClass, exitShadowClass);
	}
	v.y = bottom;
	for (v.x = left; v.x <= right; v.x++)
	{
		MapChangeFloor(map, v, exitClass, exitShadowClass);
	}
	v.x = left;
	for (v.y = top + 1; v.y < bottom; v.y++)
	{
		MapChangeFloor(map, v, exitClass, exitShadowClass);
	}
	v.x = right;
	for (v.y = top + 1; v.y < bottom; v.y++)
	{
		MapChangeFloor(map, v, exitClass, exitShadowClass);
	}
}

struct vec2 MapGetExitPos(const Map *m, const int i)
{
	const Exit *exit = CArrayGet(&m->exits, i);
	return svec2_assign_vec2i(Vec2iCenterOfTile(Rect2iCenter(exit->R)));
}

bool MapHasLockedRooms(const Map *map)
{
	CA_FOREACH(const uint16_t, a, map->access)
	if (*a != 0)
	{
		return true;
	}
	CA_FOREACH_END()
	return false;
}

uint16_t MapGetAccessLevel(const Map *map, const struct vec2i pos)
{
	if (!MapIsTileIn(map, pos))
	{
		return 0;
	}
	const uint16_t t =
		*(uint16_t *)CArrayGet(&map->access, pos.y * map->Size.x + pos.x);
	return AccessCodeToFlags(t);
}

static bool MapTileIsInLockedRoom(const Map *map, const struct vec2i tilePos)
{
	return MapGetAccessLevel(map, tilePos) != 0;
}

bool MapPosIsInLockedRoom(const Map *map, const struct vec2 pos)
{
	const struct vec2i tilePos = Vec2ToTile(pos);
	return MapTileIsInLockedRoom(map, tilePos);
}

void MapPlaceCollectible(
	const Mission *m, const int objective, const struct vec2 pos)
{
	const Objective *o = CArrayGet(&m->Objectives, objective);
	GameEvent e = GameEventNew(GAME_EVENT_ADD_PICKUP);
	strcpy(e.u.AddPickup.PickupClass, o->u.Pickup->Name);
	e.u.AddPickup.ThingFlags = ObjectiveToThing(objective);
	e.u.AddPickup.Pos = Vec2ToNet(pos);
	GameEventsEnqueue(&gGameEvents, e);
}

struct vec2 MapGenerateFreePosition(Map *map, const struct vec2i size)
{
	for (int i = 0; i < 100; i++)
	{
		const struct vec2 v = MapGetRandomPos(map);
		if (!IsCollisionWithWall(v, size))
		{
			return v;
		}
	}
	return svec2_zero();
}

void MapPlaceKey(
	MapBuilder *mb, const struct vec2i tilePos, const int keyIndex)
{
	GameEvent e = GameEventNew(GAME_EVENT_ADD_PICKUP);
	strcpy(
		e.u.AddPickup.PickupClass,
		KeyPickupClass(mb->mission->KeyStyle, keyIndex)->Name);
	e.u.AddPickup.Pos = Vec2ToNet(Vec2CenterOfTile(tilePos));
	GameEventsEnqueue(&gGameEvents, e);
}

static int GetPlacementRetries(
	const Map *map, const PlacementAccessFlags paFlags, bool *locked,
	bool *unlocked)
{
	// Try more times if we need to place in a locked room or unlocked place
	*locked = paFlags == PLACEMENT_ACCESS_LOCKED && MapHasLockedRooms(map);
	*unlocked = paFlags == PLACEMENT_ACCESS_NOT_LOCKED;
	return (*locked || *unlocked) ? 1000 : 100;
}

bool MapPlaceRandomTile(
	MapBuilder *mb, const PlacementAccessFlags paFlags,
	bool (*tryPlaceFunc)(MapBuilder *, const struct vec2i, void *), void *data)
{
	// Try a bunch of times to place something on a random tile
	bool locked, unlocked;
	const int retries =
		GetPlacementRetries(mb->Map, paFlags, &locked, &unlocked);
	for (int i = 0; i < retries; i++)
	{
		const struct vec2i tilePos = MapGetRandomTile(mb->Map);
		const bool isInLocked = MapTileIsInLockedRoom(mb->Map, tilePos);
		if ((!locked || isInLocked) && (!unlocked || !isInLocked))
		{
			if (tryPlaceFunc(mb, tilePos, data))
			{
				return true;
			}
		}
	}
	return false;
}
bool MapPlaceRandomPos(
	const Map *map, const PlacementAccessFlags paFlags,
	bool (*tryPlaceFunc)(const Map *, const struct vec2, void *), void *data)
{
	// Try a bunch of times to place something at a random location
	bool locked, unlocked;
	const int retries = GetPlacementRetries(map, paFlags, &locked, &unlocked);
	for (int i = 0; i < retries; i++)
	{
		const struct vec2 v = MapGetRandomPos(map);
		const bool isInLocked = MapPosIsInLockedRoom(map, v);
		if ((!locked || isInLocked) && (!unlocked || !isInLocked))
		{
			if (tryPlaceFunc(map, v, data))
			{
				return true;
			}
		}
	}
	return false;
}

// TODO: use enum instead of flag for map access
uint16_t AccessCodeToFlags(const uint16_t code)
{
	if (code & MAP_ACCESS_RED)
		return FLAGS_KEYCARD_RED;
	if (code & MAP_ACCESS_BLUE)
		return FLAGS_KEYCARD_BLUE;
	if (code & MAP_ACCESS_GREEN)
		return FLAGS_KEYCARD_GREEN;
	if (code & MAP_ACCESS_YELLOW)
		return FLAGS_KEYCARD_YELLOW;
	return 0;
}

// Need to check the flags around the door tile because it's the
// triggers that contain the right flags
// TODO: refactor door
int MapGetDoorKeycardFlag(Map *map, struct vec2i pos)
{
	int l = MapGetAccessLevel(map, pos);
	if (l)
		return l;
	l = MapGetAccessLevel(map, svec2i(pos.x - 1, pos.y));
	if (l)
		return l;
	l = MapGetAccessLevel(map, svec2i(pos.x + 1, pos.y));
	if (l)
		return l;
	l = MapGetAccessLevel(map, svec2i(pos.x, pos.y - 1));
	if (l)
		return l;
	return MapGetAccessLevel(map, svec2i(pos.x, pos.y + 1));
}

void MapTerminate(Map *map)
{
	CA_FOREACH(Trigger *, t, map->triggers)
	TriggerTerminate(*t);
	CA_FOREACH_END()
	CArrayTerminate(&map->triggers);
	CArrayTerminate(&map->exits);
	struct vec2i v;
	for (v.y = 0; v.y < map->Size.y; v.y++)
	{
		for (v.x = 0; v.x < map->Size.x; v.x++)
		{
			Tile *t = MapGetTile(map, v);
			TileDestroy(t);
		}
	}
	CArrayTerminate(&map->Tiles);
	TileClassesTerminate(map->TileClasses);
	LOSTerminate(&map->LOS);
	CArrayTerminate(&map->access);
	PathCacheTerminate(&gPathCache);
}

void MapInit(Map *map, const struct vec2i size)
{
	MapTerminate(map);

	// Init map
	memset(map, 0, sizeof *map);
	map->TileClasses = TileClassesNew();
	CArrayInit(&map->Tiles, sizeof(Tile));
	map->Size = size;
	LOSInit(map);
	CArrayInitFillZero(&map->access, sizeof(uint16_t), size.x * size.y);
	CArrayInit(&map->triggers, sizeof(Trigger *));
	CArrayInit(&map->exits, sizeof(Exit));
	PathCacheInit(&gPathCache, map);

	struct vec2i v;
	for (v.y = 0; v.y < map->Size.y; v.y++)
	{
		for (v.x = 0; v.x < map->Size.x; v.x++)
		{
			Tile t;
			TileInit(&t);
			CArrayPushBack(&map->Tiles, &t);
		}
	}
}

void MapPrintDebug(const Map *m)
{
	if (LogModuleGetLevel(LM_MAP) > LL_TRACE)
	{
		return;
	}
	char *buf;
	CCALLOC(buf, m->Size.x + 1);
	char *bufP = buf;
	struct vec2i v;
	for (v.y = 0; v.y < m->Size.y; v.y++)
	{
		for (v.x = 0; v.x < m->Size.x; v.x++)
		{
			const TileClass *t = MapGetTile(m, v)->Class;
			switch (t->Type)
			{
			case TILE_CLASS_FLOOR:
				*bufP++ = t->IsRoom ? '-' : '.';
				break;
			case TILE_CLASS_WALL:
				*bufP++ = '#';
				break;
			case TILE_CLASS_DOOR:
				*bufP++ = '+';
				break;
			case TILE_CLASS_NOTHING:
				*bufP++ = ' ';
				break;
			default:
				*bufP++ = '?';
				break;
			}
		}
		LOG(LM_MAP, LL_TRACE, buf);
		*buf = '\0';
		bufP = buf;
	}
	LOG_FLUSH();
	CFREE(buf);
}

bool MapIsPosOKForPlayer(
	const Map *map, const struct vec2 pos, const bool allowAllTiles)
{
	if (!MapIsTileAreaClear(map, pos, svec2i(ACTOR_W, ACTOR_H)))
	{
		return false;
	}
	// Don't put players in locked rooms
	if (MapPosIsInLockedRoom(map, pos))
	{
		return false;
	}
	const struct vec2i tilePos = Vec2ToTile(pos);
	const Tile *tile = MapGetTile(map, tilePos);
	if (tile->Class->Type == TILE_CLASS_FLOOR)
	{
		return true;
	}
	else if (allowAllTiles)
	{
		return TileCanWalk(tile);
	}
	return false;
}

// Check if the target position is completely clear
// This includes collisions that make the target illegal, such as walls
// But it also includes item collisions, whether or not the collisions
// are legal, e.g. item pickups, friendly collisions
bool MapIsTileAreaClear(
	const Map *map, const struct vec2 pos, const struct vec2i size)
{
	// Wall collision
	if (IsCollisionWithWall(pos, size))
	{
		return false;
	}

	// Item collision
	const struct vec2i tv = Vec2ToTile(pos);
	struct vec2i dv;
	// Check collisions with all other items on this tile, in all 8 directions
	for (dv.y = -1; dv.y <= 1; dv.y++)
	{
		for (dv.x = -1; dv.x <= 1; dv.x++)
		{
			const struct vec2i dtv = svec2i_add(tv, dv);
			if (!MapIsTileIn(map, dtv))
			{
				continue;
			}
			const CArray *tileThings = &MapGetTile(map, dtv)->things;
			if (tileThings == NULL)
			{
				continue;
			}
			for (int i = 0; i < (int)tileThings->size; i++)
			{
				const Thing *ti = ThingIdGetThing(CArrayGet(tileThings, i));
				if (AABBOverlap(pos, ti->Pos, size, ti->size))
				{
					if (ti->kind == KIND_OBJECT)
					{
						const TObject *tobj = CArrayGet(&gObjs, ti->id);
						if (tobj->Health <= 0)
						{
							continue;
						}
					}
					return false;
				}
			}
		}
	}

	return true;
}

void MapMarkAsVisited(Map *map, struct vec2i pos)
{
	Tile *t = MapGetTile(map, pos);
	if (!t->isVisited && TileCanWalk(t))
	{
		map->tilesSeen++;
	}
	t->isVisited = true;
}

void MapMarkAllAsVisited(Map *map)
{
	struct vec2i pos;
	for (pos.y = 0; pos.y < map->Size.y; pos.y++)
	{
		for (pos.x = 0; pos.x < map->Size.x; pos.x++)
		{
			MapMarkAsVisited(map, pos);
		}
	}
}

int MapGetExploredPercentage(Map *map)
{
	return (100 * map->tilesSeen) / map->NumExplorableTiles;
}

void MapUpdate(Map *map)
{
	CA_FOREACH(Tile, t, map->Tiles)
	TileUpdate(t);
	CA_FOREACH_END()
}

struct vec2i MapSearchTileAround(
	Map *map, struct vec2i start, TileSelectFunc func)
{
	if (func(map, start))
	{
		return start;
	}
	// Search using an expanding box pattern around the goal
	for (int radius = 1; radius < MAX(map->Size.x, map->Size.y); radius++)
	{
		struct vec2i tile;
		for (tile.x = start.x - radius; tile.x <= start.x + radius; tile.x++)
		{
			if (tile.x < 0)
				continue;
			if (tile.x >= map->Size.x)
				break;
			for (tile.y = start.y - radius; tile.y <= start.y + radius;
				 tile.y++)
			{
				if (tile.y < 0)
					continue;
				if (tile.y >= map->Size.y)
					break;
				// Check box; don't check inside
				if (tile.x != start.x - radius && tile.x != start.x + radius &&
					tile.y != start.y - radius && tile.y != start.y + radius)
				{
					continue;
				}
				if (func(map, tile))
				{
					return tile;
				}
			}
		}
	}
	// Should never reach this point; something is very wrong
	CASSERT(false, "failed to find tile around tile");
	return svec2i_zero();
}
bool MapTileIsUnexplored(Map *map, struct vec2i tile)
{
	const Tile *t = MapGetTile(map, tile);
	return !t->isVisited && TileCanWalk(t);
}

// Only creates the trigger, but does not place it
Trigger *MapNewTrigger(Map *map)
{
	Trigger *t = TriggerNew();
	CArrayPushBack(&map->triggers, &t);
	t->id = map->triggerId++;
	return t;
}
