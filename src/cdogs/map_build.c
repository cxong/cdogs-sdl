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

    Copyright (c) 2013-2014, 2017-2018 Cong Xu
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
#include "map_build.h"

#include "collision/collision.h"
#include "door.h"
#include "log.h"
#include "map_cave.h"
#include "map_classic.h"
#include "map_static.h"
#include "net_util.h"
#include "objs.h"

#define COLLECTABLE_W 4
#define COLLECTABLE_H 3
#define EXIT_WIDTH  8
#define EXIT_HEIGHT 8


static void MapSetupTilesAndWalls(MapBuilder *mb);
static void MapSetupDoors(MapBuilder *mb);
static void DebugPrintMap(const MapBuilder *mb);
static void MapAddDrains(MapBuilder *mb);
void MapBuild(Map *m, const Mission *mission, const CampaignOptions *co)
{
	MapBuilder mb;
	MapBuilderInit(&mb, m, mission, co);
	MapInit(mb.Map, mb.mission->Size);

	switch (mb.mission->Type)
	{
	case MAPTYPE_CLASSIC:
		MapClassicLoad(&mb);
		break;
	case MAPTYPE_STATIC:
		MapStaticLoad(&mb);
		break;
	case MAPTYPE_CAVE:
		MapCaveLoad(&mb);
		break;
	default:
		CASSERT(false, "unknown map type");
		break;
	}
	MapSetAccess(mb.Map, &mb.iMap);

	DebugPrintMap(&mb);

	MapSetupTilesAndWalls(&mb);
	MapSetupDoors(&mb);

	if (mb.mission->Type == MAPTYPE_CLASSIC)
	{
		MapAddDrains(&mb);
	}

	// Set exit now since we have set up all the tiles
	if (svec2i_is_zero(mb.Map->ExitStart) && svec2i_is_zero(mb.Map->ExitEnd))
	{
		MapGenerateRandomExitArea(mb.Map);
	}

	// Count total number of reachable tiles, for explored %
	mb.Map->NumExplorableTiles = 0;
	struct vec2i v;
	for (v.y = 0; v.y < mb.Map->Size.y; v.y++)
	{
		for (v.x = 0; v.x < mb.Map->Size.x; v.x++)
		{
			if (TileCanWalk(MapGetTile(mb.Map, v)))
			{
				mb.Map->NumExplorableTiles++;
			}
		}
	}

	if (!co->IsClient)
	{
		MapLoadDynamic(&mb);
	}
	MapBuilderTerminate(&mb);
}
static int MapGetAccessFlags(const MapBuilder *mb, const struct vec2i v);
static void MapSetupDoors(MapBuilder *mb)
{
	struct vec2i v;
	for (v.x = 0; v.x < mb->Map->Size.x; v.x++)
	{
		for (v.y = 0; v.y < mb->Map->Size.y; v.y++)
		{
			// Check if this is the start of a door group
			// Top or left-most door
			if ((IMapGet(mb, v) & MAP_MASKACCESS) == MAP_DOOR &&
				(IMapGet(mb, svec2i(v.x - 1, v.y)) & MAP_MASKACCESS) != MAP_DOOR &&
				(IMapGet(mb, svec2i(v.x, v.y - 1)) & MAP_MASKACCESS) != MAP_DOOR)
			{
				MapAddDoorGroup(mb, v, MapGetAccessFlags(mb, v));
			}
		}
	}
}
static int MapGetAccessFlags(const MapBuilder *mb, const struct vec2i v)
{
	int flags = 0;
	flags = MAX(flags, AccessCodeToFlags(IMapGet(mb, v)));
	flags = MAX(flags, AccessCodeToFlags(IMapGet(mb, svec2i(v.x - 1, v.y))));
	flags = MAX(flags, AccessCodeToFlags(IMapGet(mb, svec2i(v.x + 1, v.y))));
	flags = MAX(flags, AccessCodeToFlags(IMapGet(mb, svec2i(v.x, v.y - 1))));
	flags = MAX(flags, AccessCodeToFlags(IMapGet(mb, svec2i(v.x, v.y + 1))));
	return flags;
}
static void DebugPrintMap(const MapBuilder *mb)
{
	if (LogModuleGetLevel(LM_MAP) > LL_TRACE)
	{
		return;
	}
	char *buf;
	CCALLOC(buf, mb->Map->Size.x + 1);
	char *bufP = buf;
	struct vec2i v;
	for (v.y = 0; v.y < mb->Map->Size.y; v.y++)
	{
		for (v.x = 0; v.x < mb->Map->Size.x; v.x++)
		{
			switch (IMapGet(mb, v) & MAP_MASKACCESS)
			{
			case MAP_FLOOR:
				*bufP++ = '.';
				break;
			case MAP_WALL:
				*bufP++ = '#';
				break;
			case MAP_DOOR:
				*bufP++ = '+';
				break;
			case MAP_ROOM:
				*bufP++ = '-';
				break;
			case MAP_NOTHING:
				*bufP++ = ' ';
				break;
			case MAP_SQUARE:
				*bufP++ = '_';
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
	CFREE(buf);
}

void MapBuilderInit(
	MapBuilder *mb, Map *m, const Mission *mission, const CampaignOptions *co)
{
	memset(mb, 0, sizeof *mb);
	mb->Map = m;
	mb->mission = mission;
	mb->co = co;

	const int mapSize = mission->Size.x * mission->Size.y;
	CArrayInit(&mb->iMap, sizeof(unsigned short));
	CArrayResize(&mb->iMap, mapSize, NULL);
	CArrayFillZero(&mb->iMap);
	CArrayInit(&mb->leaveFree, sizeof(bool));
	CArrayResize(&mb->leaveFree, mapSize, &gFalse);
}
void MapBuilderTerminate(MapBuilder *mb)
{
	CArrayTerminate(&mb->iMap);
	CArrayTerminate(&mb->leaveFree);
}

unsigned short IMapGet(const MapBuilder *mb, const struct vec2i pos)
{
	if (pos.x < 0 || pos.x >= mb->Map->Size.x ||
		pos.y < 0 || pos.y >= mb->Map->Size.y)
	{
		return MAP_NOTHING;
	}
	return *(unsigned short *)CArrayGet(
		&mb->iMap, pos.y * mb->Map->Size.x + pos.x);
}
void IMapSet(MapBuilder *mb, struct vec2i pos, unsigned short v)
{
	*(unsigned short *)CArrayGet(&mb->iMap, pos.y *mb->Map->Size.x + pos.x) = v;
}

static int MapGetNumWallsAdjacentTile(const Map *map, const struct vec2i v);
static int MapGetNumWallsAroundTile(const Map *map, const struct vec2i v);
bool MapTryPlaceOneObject(
	MapBuilder *mb, const struct vec2i v, const MapObject *mo,
	const int extraFlags, const bool isStrictMode)
{
	// Don't place ammo spawners if ammo is disabled
	if (!ConfigGetBool(&gConfig, "Game.Ammo") &&
		mo->Type == MAP_OBJECT_TYPE_PICKUP_SPAWNER &&
		mo->u.PickupClass->Type == PICKUP_AMMO)
	{
		return false;
	}
	const Tile *t = MapGetTile(mb->Map, v);

	const bool isEmpty = TileIsClear(t);
	if (isStrictMode && !MapObjectIsTileOKStrict(
			mb, mo, IMapGet(mb, v) & MAP_MASKACCESS, v, isEmpty,
			IMapGet(mb, svec2i(v.x, v.y - 1)),
			IMapGet(mb, svec2i(v.x, v.y + 1)),
			MapGetNumWallsAdjacentTile(mb->Map, v),
			MapGetNumWallsAroundTile(mb->Map, v)))
	{
		return 0;
	}
	else if (!MapObjectIsTileOK(
		mo, IMapGet(mb, v), isEmpty, IMapGet(mb, svec2i(v.x, v.y - 1))))
	{
		return 0;
	}

	if (mo->Flags & (1 << PLACEMENT_FREE_IN_FRONT))
	{
		MapBuilderSetLeaveFree(mb, svec2i(v.x, v.y + 1), true);
	}

	NMapObjectAdd amo = NMapObjectAdd_init_default;
	amo.UID = ObjsGetNextUID();
	strcpy(amo.MapObjectClass, mo->Name);
	amo.Pos = Vec2ToNet(MapObjectGetPlacementPos(mo, v));
	amo.ThingFlags = MapObjectGetFlags(mo) | extraFlags;
	amo.Health = mo->Health;
	ObjAdd(amo);
	return true;
}
// Adjacent means to the left, right, above or below
static int MapGetNumWallsAdjacentTile(const Map *map, const struct vec2i v)
{
	int count = 0;
	if (v.x > 0 && v.y > 0 && v.x < map->Size.x - 1 && v.y < map->Size.y - 1)
	{
		if (!TileCanWalk(MapGetTile(map, svec2i(v.x - 1, v.y))))
		{
			count++;
		}
		if (!TileCanWalk(MapGetTile(map, svec2i(v.x + 1, v.y))))
		{
			count++;
		}
		if (!TileCanWalk(MapGetTile(map, svec2i(v.x, v.y - 1))))
		{
			count++;
		}
		if (!TileCanWalk(MapGetTile(map, svec2i(v.x, v.y + 1))))
		{
			count++;
		}
	}
	return count;
}
// Around means the 8 tiles surrounding the tile
static int MapGetNumWallsAroundTile(const Map *map, const struct vec2i v)
{
	int count = MapGetNumWallsAdjacentTile(map, v);
	if (v.x > 0 && v.y > 0 && v.x < map->Size.x - 1 && v.y < map->Size.y - 1)
	{
		// Having checked the adjacencies, check the diagonals
		if (!TileCanWalk(MapGetTile(map, svec2i(v.x - 1, v.y - 1))))
		{
			count++;
		}
		if (!TileCanWalk(MapGetTile(map, svec2i(v.x + 1, v.y + 1))))
		{
			count++;
		}
		if (!TileCanWalk(MapGetTile(map, svec2i(v.x + 1, v.y - 1))))
		{
			count++;
		}
		if (!TileCanWalk(MapGetTile(map, svec2i(v.x - 1, v.y + 1))))
		{
			count++;
		}
	}
	return count;
}

static void AddObjectives(MapBuilder *mb);
static void AddKeys(MapBuilder *mb);
void MapLoadDynamic(MapBuilder *mb)
{
	if (mb->mission->Type == MAPTYPE_STATIC)
	{
		MapStaticLoadDynamic(mb);
	}

	// Add map objects
	CA_FOREACH(const MapObjectDensity, mod, mb->mission->MapObjectDensities)
		for (int j = 0;
			j < (mod->Density * mb->Map->Size.x * mb->Map->Size.y) / 1000;
			j++)
		{
			MapTryPlaceOneObject(
				mb, MapGetRandomTile(mb->Map), mod->M, 0, true);
		}
	CA_FOREACH_END()

	if (HasObjectives(gCampaign.Entry.Mode))
	{
		AddObjectives(mb);
	}

	if (AreKeysAllowed(gCampaign.Entry.Mode))
	{
		AddKeys(mb);
	}
}
static bool MapTryPlaceBlowup(MapBuilder *mb, const int objective);
static int MapTryPlaceCollectible(MapBuilder *mb, const int objective);
static void AddObjectives(MapBuilder *mb)
{
	// Try to add the objectives
	// If we are unable to place them all, make sure to reduce the totals
	// in case we create missions that are impossible to complete
	CA_FOREACH(Objective, o, mb->mission->Objectives)
		if (o->Type != OBJECTIVE_COLLECT && o->Type != OBJECTIVE_DESTROY)
		{
			continue;
		}
		if (o->Type == OBJECTIVE_COLLECT)
		{
			for (int i = o->placed; i < o->Count; i++)
			{
				if (MapTryPlaceCollectible(mb, _ca_index))
				{
					o->placed++;
				}
			}
		}
		else if (o->Type == OBJECTIVE_DESTROY)
		{
			for (int i = o->placed; i < o->Count; i++)
			{
				if (MapTryPlaceBlowup(mb, _ca_index))
				{
					o->placed++;
				}
			}
		}
		o->Count = o->placed;
		if (o->Count < o->Required)
		{
			o->Required = o->Count;
		}
	CA_FOREACH_END()
}
static int MapTryPlaceCollectible(MapBuilder *mb, const int objective)
{
	const Objective *o = CArrayGet(&mb->mission->Objectives, objective);
	const bool hasLockedRooms =
		(o->Flags & OBJECTIVE_HIACCESS) && MapHasLockedRooms(mb->Map);
	const bool noaccess = o->Flags & OBJECTIVE_NOACCESS;
	int i = (noaccess || hasLockedRooms) ? 1000 : 100;

	while (i)
	{
		const struct vec2 v = MapGetRandomPos(mb->Map);
		const struct vec2i size = svec2i(COLLECTABLE_W, COLLECTABLE_H);
		if (!IsCollisionWithWall(v, size))
		{
			if ((!hasLockedRooms || MapPosIsInLockedRoom(mb->Map, v)) &&
				(!noaccess || !MapPosIsInLockedRoom(mb->Map, v)))
			{
				MapPlaceCollectible(mb->mission, objective, v);
				return 1;
			}
		}
		i--;
	}
	return 0;
}
static void MapPlaceCard(
	MapBuilder *mb, const int keyIndex, const int mapAccess);
static void AddKeys(MapBuilder *mb)
{
	if (mb->Map->keyAccessCount >= 5)
	{
		MapPlaceCard(mb, 3, MAP_ACCESS_BLUE);
	}
	if (mb->Map->keyAccessCount >= 4)
	{
		MapPlaceCard(mb, 2, MAP_ACCESS_GREEN);
	}
	if (mb->Map->keyAccessCount >= 3)
	{
		MapPlaceCard(mb, 1, MAP_ACCESS_YELLOW);
	}
	if (mb->Map->keyAccessCount >= 2)
	{
		MapPlaceCard(mb, 0, 0);
	}
}
static void MapPlaceCard(
	MapBuilder *mb, const int keyIndex, const int mapAccess)
{
	for (;;)
	{
		const struct vec2i v = MapGetRandomTile(mb->Map);
		const Tile *t = MapGetTile(mb->Map, v);
		const unsigned short iMap = IMapGet(mb, v);
		const Tile *tBelow = MapGetTile(mb->Map, svec2i(v.x, v.y + 1));
		if (TileIsClear(t) &&
			(iMap & 0xF00) == mapAccess &&
			(iMap & MAP_MASKACCESS) == MAP_ROOM &&
			TileIsClear(tBelow))
		{
			MapPlaceKey(mb, v, keyIndex);
			return;
		}
	}
}
typedef struct
{
	const Objective *o;
	int objective;
} TryPlaceOneBlowupData;
static bool TryPlaceOneBlowup(
	MapBuilder *mb, const struct vec2i tilePos, void *data);
static bool MapTryPlaceBlowup(MapBuilder *mb, const int objective)
{
	TryPlaceOneBlowupData data;
	data.o = CArrayGet(&mb->mission->Objectives, objective);
	const PlacementAccessFlags paFlags =
		ObjectiveGetPlacementAccessFlags(data.o);
	data.objective = objective;
	return MapPlaceRandomTile(mb, paFlags, TryPlaceOneBlowup, &data);
}
static bool TryPlaceOneBlowup(
	MapBuilder *mb, const struct vec2i tilePos, void *data)
{
	const TryPlaceOneBlowupData *pData = data;
	return MapTryPlaceOneObject(
		mb, tilePos, pData->o->u.MapObject,
		ObjectiveToThing(pData->objective), true);
}

void MapBuilderSetLeaveFree(
	MapBuilder *mb, const struct vec2i tile, const bool value)
{
	CArraySet(&mb->leaveFree, tile.y * mb->Map->Size.x + tile.x, &value);
}
bool MapBuilderIsLeaveFree(const MapBuilder *mb, const struct vec2i tile)
{
	return *(const bool *)CArrayGet(
		&mb->leaveFree, tile.y * mb->Map->Size.x + tile.x);
}

bool MapObjectIsTileOKStrict(
	const MapBuilder *mb, const MapObject *obj, const unsigned short tileAccess,
	const struct vec2i tile, const bool isEmpty,
	const unsigned short tileAbove, const unsigned short tileBelow,
	const int numWallsAdjacent, const int numWallsAround)
{
	if (!MapObjectIsTileOK(obj, tileAccess, isEmpty, tileAbove))
	{
		return 0;
	}
	if (MapBuilderIsLeaveFree(mb, tile))
	{
		return false;
	}

	if (obj->Flags & (1 << PLACEMENT_OUTSIDE) && tileAccess == MAP_ROOM)
	{
		return false;
	}
	if ((obj->Flags & (1 << PLACEMENT_INSIDE)) && tileAccess != MAP_ROOM)
	{
		return false;
	}
	if ((obj->Flags & (1 << PLACEMENT_NO_WALLS)) && numWallsAround != 0)
	{
		return false;
	}
	if ((obj->Flags & (1 << PLACEMENT_ONE_WALL)) && numWallsAdjacent != 1)
	{
		return false;
	}
	if ((obj->Flags & (1 << PLACEMENT_ONE_OR_MORE_WALLS)) && numWallsAdjacent < 1)
	{
		return false;
	}
	if ((obj->Flags & (1 << PLACEMENT_FREE_IN_FRONT)) &&
		(tileBelow & MAP_MASKACCESS) != MAP_FLOOR &&
		(tileBelow & MAP_MASKACCESS) != MAP_SQUARE &&
		(tileBelow & MAP_MASKACCESS) != MAP_ROOM)
	{
		return false;
	}

	return true;
}


// TODO: remove
unsigned short MapGetTileType(const Map *map, const struct vec2i pos)
{
	const Tile *tile = MapGetTile(map, pos);
	unsigned short ti = MAP_NOTHING;
	if (tile->Class->IsFloor)
	{
		ti = MAP_FLOOR;
	}
	else if (tile->Class->IsWall)
	{
		ti = MAP_WALL;
	}
	else if (tile->Class->IsDoor)
	{
		ti = MAP_DOOR | MapGetAccessLevel(map, pos);
	}
	else if (TileCanWalk(tile))
	{
		ti = MAP_ROOM;
	}
	return ti;
}

static void MapSetupTile(MapBuilder *mb, const struct vec2i pos);

// TODO: change static map building
void MapBuildTile(
	Map *m, const Mission *mission, const struct vec2i pos,
	unsigned short tileType)
{
	MapBuilder mb;
	MapBuilderInit(&mb, m, mission, NULL);
	// Update the tile as well, plus neighbours as they may be affected
	// by shadows etc. especially walls
	IMapSet(&mb, pos, tileType);
	MapSetupTile(&mb, pos);
	struct vec2i v;
	for (v.y = pos.y - 2; v.y <= pos.y + 2; v.y++)
	{
		for (v.x = pos.x - 2; v.x <= pos.x + 2; v.x++)
		{
			if (!MapIsTileIn(m, v))
			{
				continue;
			}
			IMapSet(&mb, v, MapGetTileType(m, v));
		}
	}
	for (v.y = pos.y - 1; v.y <= pos.y + 1; v.y++)
	{
		for (v.x = pos.x - 1; v.x <= pos.x + 1; v.x++)
		{
			if (!MapIsTileIn(m, v))
			{
				continue;
			}
			MapSetupTile(&mb, v);
		}
	}
	MapSetAccess(mb.Map, &mb.iMap);
	MapBuilderTerminate(&mb);
}

static bool MapTileIsNormalFloor(const MapBuilder *mb, const struct vec2i pos)
{
	// Normal floor tiles can be replaced randomly with
	// special floor tiles such as drainage
	switch (IMapGet(mb, pos) & MAP_MASKACCESS)
	{
	case MAP_FLOOR:
	case MAP_SQUARE:
		{
			const Tile *tAbove = MapGetTile(mb->Map, svec2i(pos.x, pos.y - 1));
			if (!tAbove || TileIsOpaque(tAbove))
			{
				break;
			}
			return true;
		}
	default:
		break;
	}
	return false;
}

static void MapSetupTilesAndWalls(MapBuilder *mb)
{
	// Pre-load the tile pics that this map will use
	// TODO: multiple styles and colours
	// Walls
	for (int i = 0; i < WALL_TYPE_COUNT; i++)
	{
		PicManagerGenerateMaskedStylePic(
			&gPicManager, "wall", mb->mission->WallStyle, IntWallType(i),
			mb->mission->WallMask, mb->mission->AltMask);
	}
	// Floors
	for (int i = 0; i < FLOOR_TYPES; i++)
	{
		PicManagerGenerateMaskedStylePic(
			&gPicManager, "tile", mb->mission->FloorStyle, IntTileType(i),
			mb->mission->FloorMask, mb->mission->AltMask);
	}
	// Rooms
	for (int i = 0; i < ROOMFLOOR_TYPES; i++)
	{
		PicManagerGenerateMaskedStylePic(
			&gPicManager, "tile", mb->mission->RoomStyle, IntTileType(i),
			mb->mission->RoomMask, mb->mission->AltMask);
	}

	struct vec2i v;
	for (v.x = 0; v.x < mb->Map->Size.x; v.x++)
	{
		for (v.y = 0; v.y < mb->Map->Size.y; v.y++)
		{
			MapSetupTile(mb, v);
		}
	}

	// Randomly change normal floor tiles to alternative floor tiles
	for (int i = 0; i < mb->Map->Size.x*mb->Map->Size.y / 22; i++)
	{
		const struct vec2i pos = MapGetRandomTile(mb->Map);
		if (MapTileIsNormalFloor(mb, pos))
		{
			MapGetTile(mb->Map, pos)->Class = TileClassesGetMaskedTile(
				&gTileClasses, &gPicManager, &gTileFloor,
				mb->mission->FloorStyle,
				"alt1",
				mb->mission->FloorMask, mb->mission->AltMask
			);
		}
	}
	for (int i = 0; i < mb->Map->Size.x*mb->Map->Size.y / 16; i++)
	{
		const struct vec2i pos = MapGetRandomTile(mb->Map);
		if (MapTileIsNormalFloor(mb, pos))
		{
			MapGetTile(mb->Map, pos)->Class = TileClassesGetMaskedTile(
				&gTileClasses, &gPicManager, &gTileFloor,
				mb->mission->FloorStyle,
				"alt2",
				mb->mission->FloorMask, mb->mission->AltMask
			);
		}
	}
}
static const char *MapGetWallPic(const MapBuilder *m, const struct vec2i pos);
// Set tile properties for a map tile
static void MapSetupTile(MapBuilder *mb, const struct vec2i pos)
{
	const Tile *tAbove = MapGetTile(mb->Map, svec2i(pos.x, pos.y - 1));
	const bool canSeeTileAbove = !(tAbove != NULL && TileIsOpaque(tAbove));
	Tile *t = MapGetTile(mb->Map, pos);
	if (!t)
	{
		return;
	}
	switch (IMapGet(mb, pos) & MAP_MASKACCESS)
	{
	case MAP_FLOOR:
	case MAP_SQUARE:
		t->Class = TileClassesGetMaskedTile(
			&gTileClasses, &gPicManager,
			&gTileFloor,
			mb->mission->FloorStyle, canSeeTileAbove ? "normal" : "shadow",
			mb->mission->FloorMask, mb->mission->AltMask);
		break;

	case MAP_ROOM:
	case MAP_DOOR:
		t->Class = TileClassesGetMaskedTile(
			&gTileClasses, &gPicManager, &gTileFloor, mb->mission->RoomStyle,
			canSeeTileAbove ? "normal" : "shadow",
			mb->mission->RoomMask, mb->mission->AltMask);
		break;

	case MAP_WALL:
		t->Class = TileClassesGetMaskedTile(
			&gTileClasses, &gPicManager, &gTileWall,
			mb->mission->WallStyle, MapGetWallPic(mb, pos),
			mb->mission->WallMask, mb->mission->AltMask);
		break;

	case MAP_NOTHING:
		t->Class = &gTileNothing;
		break;
	}
}
static int W(const MapBuilder *mb, const int x, const int y);
static const char *MapGetWallPic(const MapBuilder *m, const struct vec2i pos)
{
	const int x = pos.x;
	const int y = pos.y;
	if (W(m, x - 1, y) && W(m, x + 1, y) && W(m, x, y + 1) && W(m, x, y - 1))
	{
		return "x";
	}
	if (W(m, x - 1, y) && W(m, x + 1, y) && W(m, x, y + 1))
	{
		return "nt";
	}
	if (W(m, x - 1, y) && W(m, x + 1, y) && W(m, x, y - 1))
	{
		return "st";
	}
	if (W(m, x - 1, y) && W(m, x, y + 1) && W(m, x, y - 1))
	{
		return "et";
	}
	if (W(m, x + 1, y) && W(m, x, y + 1) && W(m, x, y - 1))
	{
		return "wt";
	}
	if (W(m, x + 1, y) && W(m, x, y + 1))
	{
		return "nw";
	}
	if (W(m, x + 1, y) && W(m, x, y - 1))
	{
		return "sw";
	}
	if (W(m, x - 1, y) && W(m, x, y + 1))
	{
		return "ne";
	}
	if (W(m, x - 1, y) && W(m, x, y - 1))
	{
		return "se";
	}
	if (W(m, x - 1, y) && W(m, x + 1, y))
	{
		return "h";
	}
	if (W(m, x, y + 1) && W(m, x, y - 1))
	{
		return "v";
	}
	if (W(m, x, y + 1))
	{
		return "n";
	}
	if (W(m, x, y - 1))
	{
		return "s";
	}
	if (W(m, x + 1, y))
	{
		return "w";
	}
	if (W(m, x - 1, y))
	{
		return "e";
	}
	return "o";
}
static int W(const MapBuilder *mb, const int x, const int y)
{
	return IMapGet(mb, svec2i(x, y)) == MAP_WALL;
}

static bool MapIsValidStartForWall(
	const MapBuilder *mb, const struct vec2i pos, const unsigned short tileType,
	const int pad)
{
	if (pos.x == 0 || pos.y == 0 ||
		pos.x == mb->Map->Size.x - 1 || pos.y ==  mb->Map->Size.y - 1)
	{
		return false;
	}
	struct vec2i d;
	for (d.x = pos.x - pad; d.x <= pos.x + pad; d.x++)
	{
		for (d.y = pos.y - pad; d.y <= pos.y + pad; d.y++)
		{
			if (IMapGet(mb, d) != tileType)
			{
				return false;
			}
		}
	}
	return true;
}

struct vec2i MapGetRoomSize(const RoomParams r, const int doorMin)
{
	// Work out dimensions of room
	// make sure room is large enough to accommodate doors
	const int roomMin = MAX(r.Min, doorMin + 4);
	const int roomMax = MAX(r.Max, doorMin + 4);
	return svec2i(
		RAND_INT(roomMin, roomMax + 1), RAND_INT(roomMin, roomMax + 1));
}

void MapMakeRoom(
	MapBuilder *mb, const struct vec2i pos, const struct vec2i size,
	const bool walls)
{
	struct vec2i v;
	// Set the perimeter walls and interior
	// If the tile is a room interior already, do not turn it into a wall
	// This is due to overlapping rooms
	for (v.y = pos.y; v.y < pos.y + size.y; v.y++)
	{
		for (v.x = pos.x; v.x < pos.x + size.x; v.x++)
		{
			if (v.y == pos.y || v.y == pos.y + size.y - 1 ||
				v.x == pos.x || v.x == pos.x + size.x - 1)
			{
				if (walls && IMapGet(mb, v) != MAP_ROOM)
				{
					IMapSet(mb, v, MAP_WALL);
				}
			}
			else
			{
				IMapSet(mb, v, MAP_ROOM);
			}
		}
	}
	// Check perimeter again; if there are walls where both sides contain
	// rooms, remove the wall as the rooms have merged
	for (v.y = pos.y; v.y < pos.y + size.y; v.y++)
	{
		for (v.x = pos.x; v.x < pos.x + size.x; v.x++)
		{
			if (v.y == pos.y || v.y == pos.y + size.y - 1 ||
				v.x == pos.x || v.x == pos.x + size.x - 1)
			{
				if (((IMapGet(mb, svec2i(v.x + 1, v.y)) & MAP_MASKACCESS) == MAP_ROOM &&
					(IMapGet(mb, svec2i(v.x - 1, v.y)) & MAP_MASKACCESS) == MAP_ROOM) ||
					((IMapGet(mb, svec2i(v.x, v.y + 1)) & MAP_MASKACCESS) == MAP_ROOM &&
					(IMapGet(mb, svec2i(v.x, v.y - 1)) & MAP_MASKACCESS) == MAP_ROOM))
				{
					IMapSet(mb, v, MAP_ROOM);
				}
			}
		}
	}
}

void MapMakeRoomWalls(MapBuilder *mb, const RoomParams r)
{
	int count = 0;
	for (int i = 0; i < 100 && count < r.Walls; i++)
	{
		if (!MapTryBuildWall(mb, MAP_ROOM, MAX(r.WallPad, 1), r.WallLength))
		{
			continue;
		}
		count++;
	}
}

static void MapGrowWall(
	MapBuilder *mb, struct vec2i pos,
	const unsigned short tileType, const int pad, const int d, int length);
bool MapTryBuildWall(
	MapBuilder *mb, const unsigned short tileType, const int pad,
	const int wallLength)
{
	const struct vec2i v = MapGetRandomTile(mb->Map);
	if (MapIsValidStartForWall(mb, v, tileType, pad))
	{
		IMapSet(mb, v, MAP_WALL);
		MapGrowWall(mb, v, tileType, pad, rand() & 3, wallLength);
		return true;
	}
	return false;
}
static void MapGrowWall(
	MapBuilder *mb, struct vec2i pos,
	const unsigned short tileType, const int pad, const int d, int length)
{
	int l;
	struct vec2i v;

	if (length <= 0)
		return;

	switch (d) {
		case 0:
			if (pos.y < 2 + pad)
			{
				return;
			}
			// Check tiles above
			// xxxxx
			//  xxx
			//   o
			for (v.y = pos.y - 2; v.y > pos.y - 2 - pad; v.y--)
			{
				int level = v.y - (pos.y - 2);
				for (v.x = pos.x - 1 - level; v.x <= pos.x + 1 + level; v.x++)
				{
					if (IMapGet(mb, v) != tileType)
					{
						return;
					}
				}
			}
			pos.y--;
			break;
		case 1:
			// Check tiles to the right
			//   x
			//  xx
			// oxx
			//  xx
			//   x
			for (v.x = pos.x + 2; v.x < pos.x + 2 + pad; v.x++)
			{
				int level = v.x - (pos.x + 2);
				for (v.y = pos.y - 1 - level; v.y <= pos.y + 1 + level; v.y++)
				{
					if (IMapGet(mb, v) != tileType)
					{
						return;
					}
				}
			}
			pos.x++;
			break;
		case 2:
			// Check tiles below
			//   o
			//  xxx
			// xxxxx
			for (v.y = pos.y + 2; v.y < pos.y + 2 + pad; v.y++)
			{
				int level = v.y - (pos.y + 2);
				for (v.x = pos.x - 1 - level; v.x <= pos.x + 1 + level; v.x++)
				{
					if (IMapGet(mb, v) != tileType)
					{
						return;
					}
				}
			}
			pos.y++;
			break;
		case 4:
			if (pos.x < 2 + pad)
			{
				return;
			}
			// Check tiles to the left
			// x
			// xx
			// xxo
			// xx
			// x
			for (v.x = pos.x - 2; v.x > pos.x - 2 - pad; v.x--)
			{
				int level = v.x - (pos.x - 2);
				for (v.y = pos.y - 1 - level; v.y <= pos.y + 1 + level; v.y++)
				{
					if (IMapGet(mb, v) != tileType)
					{
						return;
					}
				}
			}
			pos.x--;
			break;
	}
	IMapSet(mb, pos, MAP_WALL);
	length--;
	if (length > 0 && (rand() & 3) == 0)
	{
		// Randomly try to grow the wall in a different direction
		l = rand() % length;
		MapGrowWall(mb, pos, tileType, pad, rand() & 3, l);
		length -= l;
	}
	// Keep growing wall in same direction
	MapGrowWall(mb, pos, tileType, pad, d, length);
}

void MapSetRoomAccessMask(
	MapBuilder *mb, const struct vec2i pos, const struct vec2i size,
	const unsigned short accessMask)
{
	struct vec2i v;
	for (v.y = pos.y + 1; v.y < pos.y + size.y - 1; v.y++)
	{
		for (v.x = pos.x + 1; v.x < pos.x + size.x - 1; v.x++)
		{
			if ((IMapGet(mb, v) & MAP_MASKACCESS) == MAP_ROOM)
			{
				IMapSet(mb, v, MAP_ROOM | accessMask);
			}
		}
	}
}

static void AddOverlapRooms(
	MapBuilder *mb, const Rect2i room, CArray *overlapRooms, CArray *rooms,
	const unsigned short accessMask);
void MapSetRoomAccessMaskOverlap(
	MapBuilder *mb, CArray *rooms, const unsigned short accessMask)
{
	CArray overlapRooms;
	CArrayInit(&overlapRooms, sizeof(Rect2i));
	const Rect2i room = *(const Rect2i *)CArrayGet(rooms, 0);
	CArrayPushBack(&overlapRooms, &room);
	CA_FOREACH(const Rect2i, r, overlapRooms)
		AddOverlapRooms(mb, *r, &overlapRooms, rooms, accessMask);
	CA_FOREACH_END()
}
static void AddOverlapRooms(
	MapBuilder *mb, const Rect2i room, CArray *overlapRooms, CArray *rooms,
	const unsigned short accessMask)
{
	// Find all rooms that overlap with a room, and move it to the overlap
	// rooms array, setting access mask as we go
	CA_FOREACH(const Rect2i, r, *rooms)
		if (Rect2iOverlap(room, *r))
		{
			LOG(LM_MAP, LL_TRACE,
				"Room overlap {%d, %d (%dx%d)} {%d, %d (%dx%d)} access(%d)",
				room.Pos.x, room.Pos.y, room.Size.x, room.Size.y,
				r->Pos.x, r->Pos.y, r->Size.x, r->Size.y,
				accessMask);
			MapSetRoomAccessMask(mb, r->Pos, r->Size, accessMask);
			CArrayPushBack(overlapRooms, r);
			CArrayDelete(rooms, _ca_index);
			_ca_index--;
		}
	CA_FOREACH_END()
}

static bool TryPlaceDoorTile(
	MapBuilder *mb, const struct vec2i v, const struct vec2i d,
	const unsigned short t);
void MapPlaceDoors(
	MapBuilder *mb, struct vec2i pos, struct vec2i size,
	int hasDoors, int doors[4], int doorMin, int doorMax,
	unsigned short accessMask)
{
	int i;
	unsigned short doorTile = hasDoors ? MAP_DOOR : MAP_ROOM;
	struct vec2i v;

	// Set access mask
	MapSetRoomAccessMask(mb, pos, size, accessMask);

	// Set the doors
	if (doors[0])
	{
		int doorSize = MIN(
			(doorMax > doorMin ? (rand() % (doorMax - doorMin + 1)) : 0) + doorMin,
			size.y - 4);
		for (i = -doorSize / 2; i < (doorSize + 1) / 2; i++)
		{
			v = svec2i(pos.x, pos.y + size.y / 2 + i);
			if (!TryPlaceDoorTile(mb, v, svec2i(1, 0), doorTile))
			{
				break;
			}
		}
	}
	if (doors[1])
	{
		int doorSize = MIN(
			(doorMax > doorMin ? (rand() % (doorMax - doorMin + 1)) : 0) + doorMin,
			size.y - 4);
		for (i = -doorSize / 2; i < (doorSize + 1) / 2; i++)
		{
			v = svec2i(pos.x + size.x - 1, pos.y + size.y / 2 + i);
			if (!TryPlaceDoorTile(mb, v, svec2i(1, 0), doorTile))
			{
				break;
			}
		}
	}
	if (doors[2])
	{
		int doorSize = MIN(
			(doorMax > doorMin ? (rand() % (doorMax - doorMin + 1)) : 0) + doorMin,
			size.x - 4);
		for (i = -doorSize / 2; i < (doorSize + 1) / 2; i++)
		{
			v = svec2i(pos.x + size.x / 2 + i, pos.y);
			if (!TryPlaceDoorTile(mb, v, svec2i(0, 1), doorTile))
			{
				break;
			}
		}
	}
	if (doors[3])
	{
		int doorSize = MIN(
			(doorMax > doorMin ? (rand() % (doorMax - doorMin + 1)) : 0) + doorMin,
			size.x - 4);
		for (i = -doorSize / 2; i < (doorSize + 1) / 2; i++)
		{
			v = svec2i(pos.x + size.x / 2 + i, pos.y + size.y - 1);
			if (!TryPlaceDoorTile(mb, v, svec2i(0, 1), doorTile))
			{
				break;
			}
		}
	}
}
static bool TryPlaceDoorTile(
	MapBuilder *mb, const struct vec2i v, const struct vec2i d,
	const unsigned short t)
{
	if (IMapGet(mb, v) == MAP_WALL &&
		IMapGet(mb, svec2i_add(v, d)) != MAP_WALL &&
		IMapGet(mb, svec2i_subtract(v, d)) != MAP_WALL)
	{
		IMapSet(mb, v, t);
		return true;
	}
	return false;
}

bool MapIsAreaInside(const Map *map, const struct vec2i pos, const struct vec2i size)
{
	return pos.x >= 0 && pos.y >= 0 &&
		pos.x + size.x < map->Size.x && pos.y + size.y < map->Size.y;
}

bool MapIsAreaClear(
	const MapBuilder *mb, const struct vec2i pos, const struct vec2i size)
{
	if (!MapIsAreaInside(mb->Map, pos, size))
	{
		return false;
	}
	struct vec2i v;
	for (v.y = pos.y; v.y < pos.y + size.y; v.y++)
	{
		for (v.x = pos.x; v.x < pos.x + size.x; v.x++)
		{
			if (IMapGet(mb, v) != MAP_FLOOR)
			{
				return false;
			}
		}
	}

	return true;
}
static bool MapTileIsPartOfRoom(const MapBuilder *mb, const struct vec2i pos)
{
	struct vec2i v2;
	bool isRoom = false;
	bool isFloor = false;
	// Find whether a wall tile is part of a room perimeter
	// The surrounding tiles must have normal floor and room tiles
	// to be a perimeter
	for (v2.y = pos.y - 1; v2.y <= pos.y + 1; v2.y++)
	{
		for (v2.x = pos.x - 1; v2.x <= pos.x + 1; v2.x++)
		{
			if ((IMapGet(mb, v2) & MAP_MASKACCESS) == MAP_ROOM)
			{
				isRoom = true;
			}
			else if ((IMapGet(mb, v2) & MAP_MASKACCESS) == MAP_FLOOR)
			{
				isFloor = true;
			}
		}
	}
	return isRoom && isFloor;
}
bool MapIsAreaClearOrRoom(
	const MapBuilder *mb, const struct vec2i pos, const struct vec2i size)
{
	if (!MapIsTileIn(mb->Map, pos))
	{
		return false;
	}

	struct vec2i v;
	for (v.y = pos.y; v.y < pos.y + size.y; v.y++)
	{
		for (v.x = pos.x; v.x < pos.x + size.x; v.x++)
		{
			unsigned short tile = IMapGet(mb, v) & MAP_MASKACCESS;
			switch (tile)
			{
			case MAP_FLOOR:	// fallthrough
			case MAP_ROOM:
				break;
			case MAP_WALL:
				// Check if this wall is part of a room
				if (!MapTileIsPartOfRoom(mb, v))
				{
					return false;
				}
				break;
			default:
				return false;
			}
		}
	}

	return true;
}
bool MapIsAreaClearOrWall(
	const MapBuilder *mb, struct vec2i pos, struct vec2i size)
{
	if (!MapIsTileIn(mb->Map, pos))
	{
		return false;
	}

	struct vec2i v;
	for (v.y = pos.y; v.y < pos.y + size.y; v.y++)
	{
		for (v.x = pos.x; v.x < pos.x + size.x; v.x++)
		{
			switch (IMapGet(mb, v) & MAP_MASKACCESS)
			{
			case MAP_FLOOR:
				break;
			case MAP_WALL:
				// need to check if this is not a room wall
				if (MapTileIsPartOfRoom(mb, v))
				{
					return false;
				}
				break;
			default:
				return false;
			}
		}
	}

	return true;
}
// Find the size of the passage created by the overlap of two rooms
// To find whether an overlap is valid,
// collect the perimeter walls that overlap
// Two possible cases where there is a valid overlap:
// - if there are exactly two overlapping perimeter walls
//   i.e.
//          X
// room 2 XXXXXX
//        X X     <-- all tiles between either belong to room 1 or room 2
//     XXXXXX
//        X    room 1
//
// - if the collection of overlapping tiles are contiguous
//   i.e.
//        X room 1 X
//     XXXXXXXXXXXXXXX
//     X     room 2  X
//
// In both cases, the overlap is valid if all tiles in between are room or
// perimeter tiles. The size of the passage is given by the largest difference
// in the x or y coordinates between the first and last intersection tiles,
// minus 1
bool MapGetRoomOverlapSize(
	const MapBuilder *mb,
	const struct vec2i pos,
	const struct vec2i size,
	unsigned short *overlapAccess)
{
	struct vec2i v;
	int numOverlaps = 0;
	struct vec2i overlapMin = svec2i_zero();
	struct vec2i overlapMax = svec2i_zero();

	if (!MapIsTileIn(mb->Map, pos))
	{
		return 0;
	}

	// Find perimeter tiles that overlap
	for (v.y = pos.y; v.y < pos.y + size.y; v.y++)
	{
		for (v.x = pos.x; v.x < pos.x + size.x; v.x++)
		{
			// only check perimeter
			if (v.x != pos.x && v.x != pos.x + size.x - 1 &&
				v.y != pos.y && v.y != pos.y + size.y - 1)
			{
				continue;
			}
			switch (IMapGet(mb, v))
			{
			case MAP_WALL:
				// Check if this wall is part of a room
				if (MapTileIsPartOfRoom(mb, v))
				{
					// Get the access level of the room
					struct vec2i v2;
					for (v2.y = v.y - 1; v2.y <= v.y + 1; v2.y++)
					{
						for (v2.x = v.x - 1; v2.x <= v.x + 1; v2.x++)
						{
							if ((IMapGet(mb, v2) & MAP_MASKACCESS) == MAP_ROOM)
							{
								if (overlapAccess != NULL)
								{
									*overlapAccess |=
										IMapGet(mb, v2) & MAP_ACCESSBITS;
								}
							}
						}
					}
					if (numOverlaps == 0)
					{
						overlapMin = overlapMax = v;
					}
					else
					{
						overlapMin = svec2i_min(overlapMin, v);
						overlapMax = svec2i_max(overlapMax, v);
					}
					numOverlaps++;
				}
				break;
			default:
				break;
			}
		}
	}
	if (numOverlaps < 2)
	{
		return 0;
	}

	// Now check that all tiles between the first and last tiles are room or
	// perimeter tiles
	for (v.y = overlapMin.y; v.y <= overlapMax.y; v.y++)
	{
		for (v.x = overlapMin.x; v.x <= overlapMax.x; v.x++)
		{
			switch (IMapGet(mb, v) & MAP_MASKACCESS)
			{
			case MAP_ROOM:
				break;
			case MAP_WALL:
				// Check if this wall is part of a room
				if (!MapTileIsPartOfRoom(mb, v))
				{
					return 0;
				}
				break;
			default:
				// invalid tile type
				return 0;
			}
		}
	}

	return MAX(overlapMax.x - overlapMin.x, overlapMax.y - overlapMin.y) - 1;
}
// Check that this area does not overlap two or more "walls"
bool MapIsLessThanTwoWallOverlaps(
	const MapBuilder *mb, struct vec2i pos, struct vec2i size)
{
	if (!MapIsTileIn(mb->Map, pos))
	{
		return false;
	}

	struct vec2i v;
	int numOverlaps = 0;
	struct vec2i overlapMin = svec2i_zero();
	struct vec2i overlapMax = svec2i_zero();
	for (v.y = pos.y; v.y < pos.y + size.y; v.y++)
	{
		for (v.x = pos.x; v.x < pos.x + size.x; v.x++)
		{
			// only check perimeter
			if (v.x != pos.x && v.x != pos.x + size.x - 1 &&
				v.y != pos.y && v.y != pos.y + size.y - 1)
			{
				continue;
			}
			switch (IMapGet(mb, v))
			{
			case MAP_WALL:
				// Check if this wall is part of a room
				if (!MapTileIsPartOfRoom(mb, v))
				{
					if (numOverlaps == 0)
					{
						overlapMin = overlapMax = v;
					}
					else
					{
						overlapMin = svec2i_min(overlapMin, v);
						overlapMax = svec2i_max(overlapMax, v);
					}
					numOverlaps++;
				}
				break;
			default:
				break;
			}
		}
	}
	if (numOverlaps < 2)
	{
		return true;
	}

	// Now check that all tiles between the first and last tiles are
	// pillar tiles
	for (v.y = overlapMin.y; v.y <= overlapMax.y; v.y++)
	{
		for (v.x = overlapMin.x; v.x <= overlapMax.x; v.x++)
		{
			switch (IMapGet(mb, v) & MAP_MASKACCESS)
			{
			case MAP_WALL:
				// Check if this wall is not part of a room
				if (MapTileIsPartOfRoom(mb, v))
				{
					return false;
				}
				break;
			default:
				// invalid tile type
				return false;
			}
		}
	}

	return true;
}

void MapMakeSquare(MapBuilder *mb, struct vec2i pos, struct vec2i size)
{
	struct vec2i v;
	for (v.y = pos.y; v.y < pos.y + size.y; v.y++)
	{
		for (v.x = pos.x; v.x < pos.x + size.x; v.x++)
		{
			IMapSet(mb, v, MAP_SQUARE);
		}
	}
}
void MapMakePillar(MapBuilder *mb, struct vec2i pos, struct vec2i size)
{
	struct vec2i v;
	for (v.y = pos.y; v.y < pos.y + size.y; v.y++)
	{
		for (v.x = pos.x; v.x < pos.x + size.x; v.x++)
		{
			IMapSet(mb, v, MAP_WALL);
		}
	}
}

unsigned short GenerateAccessMask(int *accessLevel)
{
	unsigned short accessMask = 0;
	switch (rand() % 20)
	{
	case 0:
		if (*accessLevel >= 4)
		{
			accessMask = MAP_ACCESS_RED;
			*accessLevel = 5;
		}
		break;
	case 1:
	case 2:
		if (*accessLevel >= 3)
		{
			accessMask = MAP_ACCESS_BLUE;
			if (*accessLevel < 4)
			{
				*accessLevel = 4;
			}
		}
		break;
	case 3:
	case 4:
	case 5:
		if (*accessLevel >= 2)
		{
			accessMask = MAP_ACCESS_GREEN;
			if (*accessLevel < 3)
			{
				*accessLevel = 3;
			}
		}
		break;
	case 6:
	case 7:
	case 8:
	case 9:
		if (*accessLevel >= 1)
		{
			accessMask = MAP_ACCESS_YELLOW;
			if (*accessLevel < 2)
			{
				*accessLevel = 2;
			}
		}
		break;
	}
	return accessMask;
}

void MapGenerateRandomExitArea(Map *map)
{
	const Tile *t = NULL;
	for (int i = 0; i < 10000 && (t == NULL || !TileCanWalk(t)); i++)
	{
		map->ExitStart.x = (rand() % (abs(map->Size.x) - EXIT_WIDTH - 1));
		map->ExitEnd.x = map->ExitStart.x + EXIT_WIDTH + 1;
		map->ExitStart.y = (rand() % (abs(map->Size.y) - EXIT_HEIGHT - 1));
		map->ExitEnd.y = map->ExitStart.y + EXIT_HEIGHT + 1;
		// Check that the exit area is walkable
		const struct vec2i center = svec2i(
			(map->ExitStart.x + map->ExitEnd.x) / 2,
			(map->ExitStart.y + map->ExitEnd.y) / 2);
		t = MapGetTile(map, center);
	}
}

static void MapAddDrains(MapBuilder *mb)
{
	// Randomly add drainage tiles for classic map type;
	// For other map types drains are regular map objects
	const MapObject *drain = StrMapObject("drain0");
	for (int i = 0; i < mb->Map->Size.x*mb->Map->Size.y / 45; i++)
	{
		// Make sure drain tiles aren't next to each other
		struct vec2i v = MapGetRandomTile(mb->Map);
		v.x &= 0xFFFFFE;
		v.y &= 0xFFFFFE;
		if (MapTileIsNormalFloor(mb, v))
		{
			MapTryPlaceOneObject(mb, v, drain, 0, false);
		}
	}
}
