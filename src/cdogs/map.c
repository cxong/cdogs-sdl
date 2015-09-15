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

    Copyright (c) 2013-2015, Cong Xu
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
#include <string.h>
#include <stdlib.h>

#include "algorithms.h"
#include "ammo.h"
#include "collision.h"
#include "config.h"
#include "door.h"
#include "game_events.h"
#include "gamedata.h"
#include "los.h"
#include "map_build.h"
#include "map_classic.h"
#include "map_static.h"
#include "net_util.h"
#include "pic_manager.h"
#include "pickup.h"
#include "objs.h"
#include "sounds.h"
#include "actors.h"
#include "mission.h"
#include "utils.h"

#define KEY_W 9
#define KEY_H 5
#define COLLECTABLE_W 4
#define COLLECTABLE_H 3

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


unsigned short GetAccessMask(int k)
{
	if (k == -1)
	{
		return 0;
	}
	return MAP_ACCESS_YELLOW << k;
}

Tile *MapGetTile(Map *map, Vec2i pos)
{
	if (pos.x < 0 || pos.x >= map->Size.x || pos.y < 0 || pos.y >= map->Size.y)
	{
		return NULL;
	}
	return CArrayGet(&map->Tiles, pos.y * map->Size.x + pos.x);
}

bool MapIsTileIn(const Map *map, const Vec2i pos)
{
	// Check that the tile pos is within the interior of the map
	return !(pos.x < 0 || pos.y < 0 ||
		pos.x > map->Size.x - 1 || pos.y > map->Size.y - 1);
}
bool MapIsRealPosIn(const Map *map, const Vec2i realPos)
{
	// Check that the real pos is within the interior of the map
	// Note: can't use Vec2iToTile as division will cause small
	// negative values to appear to be 0 i.e. valid
	return !(realPos.x < 0 || realPos.y < 0 ||
		realPos.x / TILE_WIDTH > map->Size.x - 1 ||
		realPos.y / TILE_HEIGHT > map->Size.y - 1);
}

bool MapIsTileInExit(const Map *map, const TTileItem *ti)
{
	return
		ti->x / TILE_WIDTH >= map->ExitStart.x &&
		ti->x / TILE_WIDTH <= map->ExitEnd.x &&
		ti->y / TILE_HEIGHT >= map->ExitStart.y &&
		ti->y / TILE_HEIGHT <= map->ExitEnd.y;
}

static Tile *MapGetTileOfItem(Map *map, TTileItem *t)
{
	Vec2i pos = Vec2iToTile(Vec2iNew(t->x, t->y));
	return MapGetTile(map, pos);
}

static void AddItemToTile(TTileItem *t, Tile *tile);
bool MapTryMoveTileItem(Map *map, TTileItem *t, Vec2i pos)
{
	// Check if we can move to new position
	if (!MapIsRealPosIn(map, pos))
	{
		return false;
	}
	// When first initialised, position is -1
	bool doRemove = t->x >= 0 && t->y >= 0;
	Vec2i t1 = Vec2iToTile(Vec2iNew(t->x, t->y));
	Vec2i t2 = Vec2iToTile(pos);
	// If we'll be in the same tile, do nothing
	if (Vec2iEqual(t1, t2) && doRemove)
	{
		t->x = pos.x;
		t->y = pos.y;
		return true;
	}
	// Moving; remove from old tile...
	if (doRemove)
	{
		MapRemoveTileItem(map, t);
	}
	// ...move and add to new tile
	t->x = pos.x;
	t->y = pos.y;
	AddItemToTile(t, MapGetTile(map, t2));
	return true;
}
static void AddItemToTile(TTileItem *t, Tile *tile)
{
	// Lazy initialisation
	if (tile->things.elemSize == 0)
	{
		CArrayInit(&tile->things, sizeof(ThingId));
	}
	ThingId tid;
	tid.Id = t->id;
	tid.Kind = t->kind;
	CASSERT(tid.Id >= 0, "invalid ThingId");
	CASSERT(tid.Kind >= 0 && tid.Kind <= KIND_PICKUP, "unknown thing kind");
	CArrayPushBack(&tile->things, &tid);
}

void MapRemoveTileItem(Map *map, TTileItem *t)
{
	if (!MapIsRealPosIn(map, Vec2iNew(t->x, t->y)))
	{
		return;
	}
	Tile *tile = MapGetTileOfItem(map, t);
	for (int i = 0; i < (int)tile->things.size; i++)
	{
		ThingId *tid = CArrayGet(&tile->things, i);
		if (tid->Id == t->id && tid->Kind == t->kind)
		{
			CArrayDelete(&tile->things, i);
			return;
		}
	}
	CASSERT(false, "Did not find element to delete");
}

static Vec2i GuessCoords(Map *map)
{
	return Vec2iNew(rand() % map->Size.x, rand() % map->Size.y);
}

static Vec2i GuessPixelCoords(Map *map)
{
	return Vec2iNew(
		rand() % (map->Size.x * TILE_WIDTH),
		rand() % (map->Size.y * TILE_HEIGHT));
}

unsigned short IMapGet(const Map *map, const Vec2i pos)
{
	if (pos.x < 0 || pos.x >= map->Size.x || pos.y < 0 || pos.y >= map->Size.y)
	{
		return MAP_NOTHING;
	}
	return *(unsigned short *)CArrayGet(
		&map->iMap, pos.y * map->Size.x + pos.x);
}
void IMapSet(Map *map, Vec2i pos, unsigned short v)
{
	*(unsigned short *)CArrayGet(&map->iMap, pos.y * map->Size.x + pos.x) = v;
}

void MapChangeFloor(
	Map *map, const Vec2i pos, NamedPic *normal, NamedPic *shadow)
{
	Tile *tAbove = MapGetTile(map, Vec2iNew(pos.x, pos.y - 1));
	int canSeeTileAbove = !(pos.y > 0 && !TileCanSee(tAbove));
	Tile *t = MapGetTile(map, pos);
	if (t->flags & MAPTILE_IS_DRAINAGE)
	{
		return;
	}
	switch (IMapGet(map, pos) & MAP_MASKACCESS)
	{
	case MAP_FLOOR:
	case MAP_SQUARE:
	case MAP_ROOM:
		if (!canSeeTileAbove)
		{
			t->pic = shadow;
		}
		else
		{
			t->pic = normal;
		}
		break;
	default:
		// do nothing
		break;
	}
}

void MapShowExitArea(Map *map)
{
	const int left = map->ExitStart.x;
	const int right = map->ExitEnd.x;
	const int top = map->ExitStart.y;
	const int bottom = map->ExitEnd.y;

	Vec2i v;
	v.y = top;
	for (v.x = left; v.x <= right; v.x++)
	{
		MapChangeFloor(map, v, gMission.exitPic, gMission.exitShadow);
	}
	v.y = bottom;
	for (v.x = left; v.x <= right; v.x++)
	{
		MapChangeFloor(map, v, gMission.exitPic, gMission.exitShadow);
	}
	v.x = left;
	for (v.y = top + 1; v.y < bottom; v.y++)
	{
		MapChangeFloor(map, v, gMission.exitPic, gMission.exitShadow);
	}
	v.x = right;
	for (v.y = top + 1; v.y < bottom; v.y++)
	{
		MapChangeFloor(map, v, gMission.exitPic, gMission.exitShadow);
	}
}

Vec2i MapGetExitPos(const Map *m)
{
	return Vec2iCenterOfTile(
		Vec2iScaleDiv(Vec2iAdd(m->ExitStart, m->ExitEnd), 2));
}

// Adjacent means to the left, right, above or below
static int MapGetNumWallsAdjacentTile(Map *map, Vec2i v)
{
	int count = 0;
	if (v.x > 0 && v.y > 0 && v.x < map->Size.x - 1 && v.y < map->Size.y - 1)
	{
		if (!TileCanWalk(MapGetTile(map, Vec2iNew(v.x - 1, v.y))))
		{
			count++;
		}
		if (!TileCanWalk(MapGetTile(map, Vec2iNew(v.x + 1, v.y))))
		{
			count++;
		}
		if (!TileCanWalk(MapGetTile(map, Vec2iNew(v.x, v.y - 1))))
		{
			count++;
		}
		if (!TileCanWalk(MapGetTile(map, Vec2iNew(v.x, v.y + 1))))
		{
			count++;
		}
	}
	return count;
}

// Around means the 8 tiles surrounding the tile
static int MapGetNumWallsAroundTile(Map *map, Vec2i v)
{
	int count = MapGetNumWallsAdjacentTile(map, v);
	if (v.x > 0 && v.y > 0 && v.x < map->Size.x - 1 && v.y < map->Size.y - 1)
	{
		// Having checked the adjacencies, check the diagonals
		if (!TileCanWalk(MapGetTile(map, Vec2iNew(v.x - 1, v.y - 1))))
		{
			count++;
		}
		if (!TileCanWalk(MapGetTile(map, Vec2iNew(v.x + 1, v.y + 1))))
		{
			count++;
		}
		if (!TileCanWalk(MapGetTile(map, Vec2iNew(v.x + 1, v.y - 1))))
		{
			count++;
		}
		if (!TileCanWalk(MapGetTile(map, Vec2iNew(v.x - 1, v.y + 1))))
		{
			count++;
		}
	}
	return count;
}

bool MapTryPlaceOneObject(
	Map *map, const Vec2i v, const MapObject *mo, const int extraFlags,
	const bool isStrictMode)
{
	// Don't place ammo spawners if ammo is disabled
	if (!ConfigGetBool(&gConfig, "Game.Ammo") &&
		mo->Type == MAP_OBJECT_TYPE_PICKUP_SPAWNER &&
		mo->u.PickupClass->Type == PICKUP_AMMO)
	{
		return false;
	}
	Vec2i realPos = Vec2iCenterOfTile(v);
	int tileFlags = 0;
	Tile *t = MapGetTile(map, v);
	unsigned short iMap = IMapGet(map, v);

	const bool isEmpty = TileIsClear(t);
	if (isStrictMode && !MapObjectIsTileOKStrict(
			mo, iMap, isEmpty,
			IMapGet(map, Vec2iNew(v.x, v.y - 1)),
			IMapGet(map, Vec2iNew(v.x, v.y + 1)),
			MapGetNumWallsAdjacentTile(map, v),
			MapGetNumWallsAroundTile(map, v)))
	{
		return 0;
	}
	else if (!MapObjectIsTileOK(
		mo, iMap, isEmpty, IMapGet(map, Vec2iNew(v.x, v.y - 1))))
	{
		return 0;
	}

	if (mo->Flags & (1 << PLACEMENT_FREE_IN_FRONT))
	{
		IMapSet(map, Vec2iNew(v.x, v.y + 1), IMapGet(map, Vec2iNew(v.x, v.y + 1)) | MAP_LEAVEFREE);
	}

	// For on-wall objects, set their position to the top of the tile
	// This guarantees that they are drawn last
	if (mo->Flags & (1 << PLACEMENT_ON_WALL))
	{
		realPos.y -= TILE_HEIGHT / 2 + 1;
	}

	if (MapObjectIsWreck(mo))
	{
		tileFlags |= TILEITEM_IS_WRECK;
	}
	else if (!(mo->Flags & (1 << PLACEMENT_ON_WALL)))
	{
		// TODO: any situation where these two do not coincide?
		tileFlags |= TILEITEM_IMPASSABLE;
		tileFlags |= TILEITEM_CAN_BE_SHOT;
	}

	NMapObjectAdd amo = NMapObjectAdd_init_default;
	amo.UID = ObjsGetNextUID();
	strcpy(amo.MapObjectClass, mo->Name);
	amo.Pos = Vec2i2Net(realPos);
	amo.TileItemFlags = tileFlags | extraFlags;
	amo.Health = mo->Health;
	ObjAdd(amo);
	return true;
}

void MapPlaceWreck(Map *map, const Vec2i v, const MapObject *mo)
{
	Tile *t = MapGetTile(map, v);
	unsigned short iMap = IMapGet(map, v);
	if (!MapObjectIsTileOK(
		mo, iMap, TileIsClear(t), IMapGet(map, Vec2iNew(v.x, v.y - 1))))
	{
		return;
	}
	NMapObjectAdd amo = NMapObjectAdd_init_default;
	amo.UID = ObjsGetNextUID();
	strcpy(amo.MapObjectClass, mo->Name);
	amo.Pos = Vec2i2Net(Vec2iCenterOfTile(v));
	amo.TileItemFlags = TILEITEM_IS_WRECK;
	// Set health to 0 to force into a wreck
	amo.Health = 0;
	ObjAdd(amo);
}

int MapHasLockedRooms(Map *map)
{
	return map->keyAccessCount > 1;
}

// TODO: rename this function
int MapPosIsHighAccess(Map *map, int x, int y)
{
	Vec2i tilePos = Vec2iToTile(Vec2iNew(x, y));
	return IMapGet(map, tilePos) & MAP_ACCESSBITS;
}

void MapPlaceCollectible(
	const struct MissionOptions *mo, const int objective, const Vec2i realPos)
{
	const ObjectiveDef *o = CArrayGet(&mo->Objectives, objective);
	GameEvent e = GameEventNew(GAME_EVENT_ADD_PICKUP);
	e.u.AddPickup.UID = PickupsGetNextUID();
	strcpy(e.u.AddPickup.PickupClass, o->pickupClass->Name);
	e.u.AddPickup.IsRandomSpawned = false;
	e.u.AddPickup.SpawnerUID = -1;
	e.u.AddPickup.TileItemFlags = ObjectiveToTileItem(objective);
	e.u.AddPickup.Pos = Vec2i2Net(realPos);
	GameEventsEnqueue(&gGameEvents, e);
}
static int MapTryPlaceCollectible(
	Map *map, const Mission *mission, const struct MissionOptions *mo,
	const int objective)
{
	const MissionObjective *mobj = CArrayGet(&mission->Objectives, objective);
	const bool hasLockedRooms =
		(mobj->Flags & OBJECTIVE_HIACCESS) && MapHasLockedRooms(map);
	int noaccess = mobj->Flags & OBJECTIVE_NOACCESS;
	int i = (noaccess || hasLockedRooms) ? 1000 : 100;

	while (i)
	{
		Vec2i v = GuessPixelCoords(map);
		Vec2i size = Vec2iNew(COLLECTABLE_W, COLLECTABLE_H);
		if (!IsCollisionWithWall(v, size))
		{
			if ((!hasLockedRooms || MapPosIsHighAccess(map, v.x, v.y)) &&
				(!noaccess || !MapPosIsHighAccess(map, v.x, v.y)))
			{
				MapPlaceCollectible(mo, objective, v);
				return 1;
			}
		}
		i--;
	}
	return 0;
}

Vec2i MapGenerateFreePosition(Map *map, Vec2i size)
{
	for (int i = 0; i < 100; i++)
	{
		Vec2i v = GuessPixelCoords(map);
		if (!IsCollisionWithWall(v, size))
		{
			return v;
		}
	}
	return Vec2iZero();
}

static bool MapTryPlaceBlowup(
	Map *map, const Mission *mission, const struct MissionOptions *mo,
	const int objective)
{
	const MissionObjective *mobj = CArrayGet(&mission->Objectives, objective);
	const bool hasLockedRooms =
		(mobj->Flags & OBJECTIVE_HIACCESS) && MapHasLockedRooms(map);
	int noaccess = mobj->Flags & OBJECTIVE_NOACCESS;
	int i = (noaccess || hasLockedRooms) ? 1000 : 100;

	while (i > 0)
	{
		Vec2i v = GuessCoords(map);
		if ((!hasLockedRooms || (IMapGet(map, v) >> 8)) &&
			(!noaccess || (IMapGet(map, v) >> 8) == 0))
		{
			const ObjectiveDef *o = CArrayGet(&mo->Objectives, objective);
			if (MapTryPlaceOneObject(
					map,
					v,
					o->blowupObject,
					ObjectiveToTileItem(objective), 1))
			{
				return 1;
			}
		}
		i--;
	}
	return 0;
}

void MapPlaceKey(
	Map *map, const struct MissionOptions *mo, const Vec2i pos,
	const int keyIndex)
{
	UNUSED(map);
	GameEvent e = GameEventNew(GAME_EVENT_ADD_PICKUP);
	e.u.AddPickup.UID = PickupsGetNextUID();
	strcpy(
		e.u.AddPickup.PickupClass,
		KeyPickupClass(mo->keyStyle, keyIndex)->Name);
	e.u.AddPickup.IsRandomSpawned = false;
	e.u.AddPickup.SpawnerUID = -1;
	e.u.AddPickup.TileItemFlags = 0;
	e.u.AddPickup.Pos = Vec2i2Net(Vec2iCenterOfTile(pos));
	GameEventsEnqueue(&gGameEvents, e);
}

static void MapPlaceCard(Map *map, int keyIndex, int map_access)
{
	for (;;)
	{
		Vec2i v = GuessCoords(map);
		Tile *t;
		Tile *tBelow;
		unsigned short iMap;
		t = MapGetTile(map, v);
		iMap = IMapGet(map, v);
		tBelow = MapGetTile(map, Vec2iNew(v.x, v.y + 1));
		if (TileIsClear(t) &&
			(iMap & 0xF00) == map_access &&
			(iMap & MAP_MASKACCESS) == MAP_ROOM &&
			TileIsClear(tBelow))
		{
			MapPlaceKey(map, &gMission, v, keyIndex);
			return;
		}
	}
}

static int AccessCodeToFlags(int code)
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

static int MapGetAccessLevel(Map *map, int x, int y)
{
	return AccessCodeToFlags(IMapGet(map, Vec2iNew(x, y)));
}

// Need to check the flags around the door tile because it's the
// triggers that contain the right flags
// TODO: refactor door
int MapGetDoorKeycardFlag(Map *map, Vec2i pos)
{
	int l = MapGetAccessLevel(map, pos.x, pos.y);
	if (l) return l;
	l = MapGetAccessLevel(map, pos.x - 1, pos.y);
	if (l) return l;
	l = MapGetAccessLevel(map, pos.x + 1, pos.y);
	if (l) return l;
	l = MapGetAccessLevel(map, pos.x, pos.y - 1);
	if (l) return l;
	return MapGetAccessLevel(map, pos.x, pos.y + 1);
}

static int MapGetAccessFlags(Map *map, int x, int y)
{
	int flags = 0;
	flags = MAX(flags, AccessCodeToFlags(IMapGet(map, Vec2iNew(x, y))));
	flags = MAX(flags, AccessCodeToFlags(IMapGet(map, Vec2iNew(x - 1, y))));
	flags = MAX(flags, AccessCodeToFlags(IMapGet(map, Vec2iNew(x + 1, y))));
	flags = MAX(flags, AccessCodeToFlags(IMapGet(map, Vec2iNew(x, y - 1))));
	flags = MAX(flags, AccessCodeToFlags(IMapGet(map, Vec2iNew(x, y + 1))));
	return flags;
}

static void MapSetupDoors(Map *map, const Mission *m, int floor, int room)
{
	Vec2i v;
	for (v.x = 0; v.x < map->Size.x; v.x++)
	{
		for (v.y = 0; v.y < map->Size.y; v.y++)
		{
			// Check if this is the start of a door group
			// Top or left-most door
			if ((IMapGet(map, v) & MAP_MASKACCESS) == MAP_DOOR &&
				(IMapGet(map, Vec2iNew(v.x - 1, v.y)) & MAP_MASKACCESS) != MAP_DOOR &&
				(IMapGet(map, Vec2iNew(v.x, v.y - 1)) & MAP_MASKACCESS) != MAP_DOOR)
			{
				MapAddDoorGroup(
					map, m, v, floor, room,
					MapGetAccessFlags(map, v.x, v.y));
			}
		}
	}
}

void MapTerminate(Map *map)
{
	for (int i = 0; i < (int)map->triggers.size; i++)
	{
		TriggerTerminate(*(Trigger **)CArrayGet(&map->triggers, i));
	}
	CArrayTerminate(&map->triggers);
	Vec2i v;
	for (v.y = 0; v.y < map->Size.y; v.y++)
	{
		for (v.x = 0; v.x < map->Size.x; v.x++)
		{
			Tile *t = MapGetTile(map, v);
			TileDestroy(t);
		}
	}
	CArrayTerminate(&map->Tiles);
	CArrayTerminate(&map->iMap);
	LOSTerminate(&map->LOS);
	PathCacheTerminate(&gPathCache);
}
void MapLoad(
	Map *map, const struct MissionOptions *mo, const CampaignOptions *co)
{
	MapTerminate(map);

	// Init map
	memset(map, 0, sizeof *map);
	CArrayInit(&map->Tiles, sizeof(Tile));
	CArrayInit(&map->iMap, sizeof(unsigned short));
	const Mission *mission = mo->missionData;
	map->Size = mission->Size;
	LOSInit(map, map->Size);
	CArrayInit(&map->triggers, sizeof(Trigger *));
	PathCacheInit(&gPathCache, map);

	Vec2i v;
	for (v.y = 0; v.y < map->Size.y; v.y++)
	{
		for (v.x = 0; v.x < map->Size.x; v.x++)
		{
			Tile t;
			unsigned short tI = MAP_FLOOR;
			TileInit(&t);
			CArrayPushBack(&map->Tiles, &t);
			CArrayPushBack(&map->iMap, &tI);
		}
	}

	if (mission->Type == MAPTYPE_CLASSIC)
	{
		MapClassicLoad(map, mission, co);
	}
	else
	{
		MapStaticLoad(map, mo);
	}

	MapSetupTilesAndWalls(map, mission);
	const int floor = mission->FloorStyle % FLOOR_STYLE_COUNT;
	const int room = mission->RoomStyle % ROOM_STYLE_COUNT;
	MapSetupDoors(map, mission, floor, room);

	// Set exit now since we have set up all the tiles
	if (Vec2iIsZero(map->ExitStart) && Vec2iIsZero(map->ExitEnd))
	{
		MapGenerateRandomExitArea(map);
	}

	// Count total number of reachable tiles, for explored %
	map->NumExplorableTiles = 0;
	for (v.y = 0; v.y < map->Size.y; v.y++)
	{
		for (v.x = 0; v.x < map->Size.x; v.x++)
		{
			if (!(MapGetTile(map, v)->flags & MAPTILE_NO_WALK))
			{
				map->NumExplorableTiles++;
			}
		}
	}
}

static void AddObjectives(Map *map, const struct MissionOptions *mo);
static void AddKeys(Map *map);
void MapLoadDynamic(
	Map *map, const struct MissionOptions *mo, const CharacterStore *store)
{
	const Mission *mission = mo->missionData;

	if (mission->Type == MAPTYPE_STATIC)
	{
		MapStaticLoadDynamic(map, mo, store);
	}

	// Add map objects
	for (int i = 0; i < (int)mission->MapObjectDensities.size; i++)
	{
		const MapObjectDensity *mod =
			CArrayGet(&mission->MapObjectDensities, i);
		for (int j = 0;
			j < (mod->Density * map->Size.x * map->Size.y) / 1000;
			j++)
		{
			MapTryPlaceOneObject(
				map,
				Vec2iNew(rand() % map->Size.x, rand() % map->Size.y),
				mod->M,
				0,
				true);
		}
	}

	if (HasObjectives(gCampaign.Entry.Mode))
	{
		AddObjectives(map, mo);
	}

	if (AreKeysAllowed(gCampaign.Entry.Mode))
	{
		AddKeys(map);
	}
}
static void AddObjectives(Map *map, const struct MissionOptions *mo)
{
	// Try to add the objectives
	// If we are unable to place them all, make sure to reduce the totals
	// in case we create missions that are impossible to complete
	for (int i = 0, j = 0; i < (int)mo->missionData->Objectives.size; i++)
	{
		MissionObjective *mobj = CArrayGet(&mo->missionData->Objectives, i);
		if (mobj->Type != OBJECTIVE_COLLECT && mobj->Type != OBJECTIVE_DESTROY)
		{
			continue;
		}
		ObjectiveDef *obj = CArrayGet(&mo->Objectives, i);
		if (mobj->Type == OBJECTIVE_COLLECT)
		{
			for (j = obj->placed; j < mobj->Count; j++)
			{
				if (MapTryPlaceCollectible(map, mo->missionData, mo, i))
				{
					obj->placed++;
				}
			}
		}
		else if (mobj->Type == OBJECTIVE_DESTROY)
		{
			for (j = obj->placed; j < mobj->Count; j++)
			{
				if (MapTryPlaceBlowup(map, mo->missionData, mo, i))
				{
					obj->placed++;
				}
			}
		}
		mobj->Count = obj->placed;
		if (mobj->Count < mobj->Required)
		{
			mobj->Required = mobj->Count;
		}
	}
}
static void AddKeys(Map *map)
{
	if (map->keyAccessCount >= 5)
	{
		MapPlaceCard(map, 3, MAP_ACCESS_BLUE);
	}
	if (map->keyAccessCount >= 4)
	{
		MapPlaceCard(map, 2, MAP_ACCESS_GREEN);
	}
	if (map->keyAccessCount >= 3)
	{
		MapPlaceCard(map, 1, MAP_ACCESS_YELLOW);
	}
	if (map->keyAccessCount >= 2)
	{
		MapPlaceCard(map, 0, 0);
	}
}

bool MapIsFullPosOKforPlayer(
	const Map *map, const Vec2i pos, const bool allowAllTiles)
{
	Vec2i tilePos = Vec2iToTile(Vec2iFull2Real(pos));
	unsigned short tile = IMapGet(map, tilePos);
	if (tile == MAP_FLOOR)
	{
		return true;
	}
	else if (allowAllTiles)
	{
		return tile == MAP_SQUARE || tile == MAP_ROOM;
	}
	return false;
}

// Check if the target position is completely clear
// This includes collisions that make the target illegal, such as walls
// But it also includes item collisions, whether or not the collisions
// are legal, e.g. item pickups, friendly collisions
bool MapIsTileAreaClear(Map *map, const Vec2i fullPos, const Vec2i size)
{
	const Vec2i realPos = Vec2iFull2Real(fullPos);

	// Wall collision
	if (IsCollisionWithWall(realPos, size))
	{
		return false;
	}

	// Item collision
	const Vec2i tv = Vec2iToTile(realPos);
	Vec2i dv;
	// Check collisions with all other items on this tile, in all 8 directions
	for (dv.y = -1; dv.y <= 1; dv.y++)
	{
		for (dv.x = -1; dv.x <= 1; dv.x++)
		{
			const Vec2i dtv = Vec2iAdd(tv, dv);
			if (!MapIsTileIn(map, dtv))
			{
				continue;
			}
			const CArray *tileThings = &MapGetTile(map, dtv)->things;
			for (int i = 0; i < (int)tileThings->size; i++)
			{
				const TTileItem *ti =
					ThingIdGetTileItem(CArrayGet(tileThings, i));
				if (AreasCollide(
						realPos, Vec2iNew(ti->x, ti->y), size, ti->size))
				{
					return false;
				}
			}
		}
	}

	return true;
}

void MapMarkAsVisited(Map *map, Vec2i pos)
{
	Tile *t = MapGetTile(map, pos);
	if (!t->isVisited && !(t->flags & MAPTILE_NO_WALK))
	{
		map->tilesSeen++;
	}
	t->isVisited = true;
}

void MapMarkAllAsVisited(Map *map)
{
	Vec2i pos;
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

Vec2i MapSearchTileAround(Map *map, Vec2i start, TileSelectFunc func)
{
	if (func(map, start))
	{
		return start;
	}
	// Search using an expanding box pattern around the goal
	for (int radius = 1; radius < MAX(map->Size.x, map->Size.y); radius++)
	{
		Vec2i tile;
		for (tile.x = start.x - radius;
			tile.x <= start.x + radius;
			tile.x++)
		{
			if (tile.x < 0) continue;
			if (tile.x >= map->Size.x) break;
			for (tile.y = start.y - radius;
				tile.y <= start.y + radius;
				tile.y++)
			{
				if (tile.y < 0) continue;
				if (tile.y >= map->Size.y) break;
				// Check box; don't check inside
				if (tile.x != start.x - radius &&
					tile.x != start.x + radius &&
					tile.y != start.y - radius &&
					tile.y != start.y + radius)
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
	return Vec2iZero();
}
bool MapTileIsUnexplored(Map *map, Vec2i tile)
{
	const Tile *t = MapGetTile(map, tile);
	return !t->isVisited && !(t->flags & MAPTILE_NO_WALK);
}

// Only creates the trigger, but does not place it
Trigger *MapNewTrigger(Map *map)
{
	Trigger *t = TriggerNew();
	CArrayPushBack(&map->triggers, &t);
	t->id = map->triggerId++;
	return t;
}
