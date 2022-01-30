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

	Copyright (c) 2013-2014, 2017-2022 Cong Xu
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

#include "actors.h"
#include "collision/collision.h"
#include "door.h"
#include "log.h"
#include "map_cave.h"
#include "map_classic.h"
#include "map_interior.h"
#include "map_static.h"
#include "net_util.h"
#include "objs.h"

#define COLLECTABLE_W 4
#define COLLECTABLE_H 3
#define EXIT_WIDTH 8
#define EXIT_HEIGHT 8

static void MapSetupTilesAndWalls(MapBuilder *mb);
static void MapSetupDoors(MapBuilder *mb);
static void MapAddDrains(MapBuilder *mb);
static void MapGenerateRandomExitArea(Map *map, const int mission);
void MapBuild(
	Map *m, const Mission *mission, const bool loadDynamic, const int missionIndex, const GameMode mode, const CharacterStore *characters)
{
	MapBuilder mb;
	MapBuilderInit(&mb, m, mission, mode, characters);
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
	case MAPTYPE_INTERIOR:
		MapInteriorLoad(&mb, missionIndex);
		break;
	default:
		CASSERT(false, "unknown map type");
		break;
	}
	CArrayCopy(&mb.Map->access, &mb.access);

	MapSetupTilesAndWalls(&mb);
	MapSetupDoors(&mb);
	MapPrintDebug(mb.Map);

	// Set exit now since we have set up all the tiles
	switch (mb.mission->Type)
	{
	case MAPTYPE_CLASSIC:
		MapAddDrains(&mb);
		if (HasExit(gCampaign.Entry.Mode) && mb.mission->u.Classic.ExitEnabled)
		{
			MapGenerateRandomExitArea(mb.Map, missionIndex);
		}
		break;
	case MAPTYPE_STATIC:
		break;
	case MAPTYPE_CAVE:
		if (HasExit(gCampaign.Entry.Mode) && mb.mission->u.Cave.ExitEnabled)
		{
			MapGenerateRandomExitArea(mb.Map, missionIndex);
		}
		break;
	case MAPTYPE_INTERIOR:
		MapAddDrains(&mb);
		break;
	default:
		CASSERT(false, "unknown map type");
		break;
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

	if (loadDynamic)
	{
		MapLoadDynamic(&mb);
		ActorsPilotVehicles();
	}
	MapBuilderTerminate(&mb);
}
void SetupWallTileClasses(Map *m, PicManager *pm, const TileClass *base)
{
	for (int i = 0; i < WALL_TYPE_COUNT; i++)
	{
		PicManagerGenerateMaskedStylePic(
			pm, "wall", base->Style, IntWallType(i), base->Mask, base->MaskAlt,
			false);
		TileClassesAdd(
			m->TileClasses, pm, base, base->Style, IntWallType(i), base->Mask,
			base->MaskAlt);
	}
}
void SetupFloorTileClasses(Map *m, PicManager *pm, const TileClass *base)
{
	for (int i = 0; i < FLOOR_TYPES; i++)
	{
		PicManagerGenerateMaskedStylePic(
			pm, "tile", base->Style, IntTileType(i), base->Mask, base->MaskAlt,
			false);
		TileClassesAdd(
			m->TileClasses, pm, base, base->Style, IntTileType(i), base->Mask,
			base->MaskAlt);
	}
}
void SetupDoorTileClasses(Map *m, PicManager *pm, const TileClass *base)
{
	const char *doorStyles[] = {"yellow", "green", "blue",	"red",
								"open",	  "wall",  "normal"};
	for (int i = 0; i < (int)(sizeof doorStyles / sizeof doorStyles[0]); i++)
	{
		for (DoorType type = DOORTYPE_H; type < DOORTYPE_COUNT; type++)
		{
			DoorAddClass(m->TileClasses, pm, base, doorStyles[i], type);
		}
	}
}
static bool MapBuilderIsDoor(const MapBuilder *mb, const struct vec2i v);
static int MapGetAccessFlags(const MapBuilder *mb, const struct vec2i v);
static void MapSetupDoors(MapBuilder *mb)
{
	// Mark doors as set up as we go
	CArray doorsSetup;
	CArrayInitFillZero(&doorsSetup, sizeof(bool), mb->Map->Size.x * mb->Map->Size.y);
	
	RECT_FOREACH(Rect2iNew(svec2i_zero(), mb->Map->Size))
	// Check if this door tile hasn't been set up yet
	const int idx = _v.x + _v.y * mb->Map->Size.x;
	if (MapBuilderIsDoor(mb, _v) && !*(bool *)CArrayGet(&doorsSetup, idx))
	{
		const struct vec2i groupSize = MapAddDoorGroup(mb, _v, MapGetAccessFlags(mb, _v));
		for (int dx = 0; dx < groupSize.x; dx++)
		{
			for (int dy = 0; dy < groupSize.y; dy++)
			{
				const int idx2 = _v.x + dx + (_v.y + dy) * mb->Map->Size.x;
				CArraySet(&doorsSetup, idx2, &gTrue);
			}
		}
	}
	RECT_FOREACH_END()
	
	CArrayTerminate(&doorsSetup);
}
static bool MapBuilderIsDoor(const MapBuilder *mb, const struct vec2i v)
{
	const TileClass *t = MapBuilderGetTile(mb, v);
	return t != NULL && t->Type == TILE_CLASS_DOOR;
}
static int MapGetAccessFlags(const MapBuilder *mb, const struct vec2i v)
{
	int flags = 0;
	flags = MAX(flags, AccessCodeToFlags(MapBuildGetAccess(mb, v)));
	flags = MAX(
		flags, AccessCodeToFlags(MapBuildGetAccess(mb, svec2i(v.x - 1, v.y))));
	flags = MAX(
		flags, AccessCodeToFlags(MapBuildGetAccess(mb, svec2i(v.x + 1, v.y))));
	flags = MAX(
		flags, AccessCodeToFlags(MapBuildGetAccess(mb, svec2i(v.x, v.y - 1))));
	flags = MAX(
		flags, AccessCodeToFlags(MapBuildGetAccess(mb, svec2i(v.x, v.y + 1))));
	return flags;
}

void MapBuilderInit(
	MapBuilder *mb, Map *m, const Mission *mission, const GameMode mode, const CharacterStore *characters)
{
	memset(mb, 0, sizeof *mb);
	mb->Map = m;
	mb->mission = mission;
	mb->mode = mode;
	mb->characters = characters;

	const int mapSize = mission->Size.x * mission->Size.y;
	CArrayInitFillZero(&mb->access, sizeof(uint16_t), mapSize);
	CArrayInitFill(&mb->tiles, sizeof(TileClass), mapSize, &gTileNothing);
	CArrayInitFillZero(&mb->leaveFree, sizeof(bool), mapSize);
}
void MapBuilderTerminate(MapBuilder *mb)
{
	CArrayTerminate(&mb->access);
	CArrayTerminate(&mb->tiles);
	CArrayTerminate(&mb->leaveFree);
}

uint16_t MapBuildGetAccess(const MapBuilder *mb, const struct vec2i pos)
{
	if (!MapIsTileIn(mb->Map, pos))
	{
		return 0;
	}
	return *(uint16_t *)CArrayGet(
		&mb->access, pos.y * mb->Map->Size.x + pos.x);
}
void MapBuildSetAccess(MapBuilder *mb, struct vec2i pos, const uint16_t v)
{
	*(uint16_t *)CArrayGet(&mb->access, pos.y * mb->Map->Size.x + pos.x) = v;
}
const TileClass *MapBuilderGetTile(
	const MapBuilder *mb, const struct vec2i pos)
{
	if (!MapIsTileIn(mb->Map, pos))
	{
		return NULL;
	}
	return CArrayGet(&mb->tiles, pos.y * mb->Map->Size.x + pos.x);
}
void MapBuilderSetTile(MapBuilder *mb, struct vec2i pos, const TileClass *t)
{
	if (t != NULL)
	{
		*(TileClass *)CArrayGet(&mb->tiles, pos.y * mb->Map->Size.x + pos.x) =
			*t;
	}
}

static bool IsTileOKStrict(
	const MapObject *obj, const Tile *tile, const Tile *tileAbove,
	const Tile *tileBelow, const bool isLeaveFree, const int numWallsAdjacent,
	const int numWallsAround);
static int MapGetNumWallsAdjacentTile(const Map *map, const struct vec2i v);
static int MapGetNumWallsAroundTile(const Map *map, const struct vec2i v);
bool MapTryPlaceOneObject(
	MapBuilder *mb, const struct vec2i v, const MapObject *mo,
	const int extraFlags, const bool isStrictMode)
{
	// Don't place ammo spawners if ammo is disabled
	if (!gCampaign.Setting.Ammo &&
		mo->Type == MAP_OBJECT_TYPE_PICKUP_SPAWNER &&
		mo->u.PickupClass->Type == PICKUP_AMMO)
	{
		return false;
	}
	const Tile *t = MapGetTile(mb->Map, v);
	const Tile *tAbove = MapGetTile(mb->Map, svec2i(v.x, v.y - 1));
	const Tile *tBelow = MapGetTile(mb->Map, svec2i(v.x, v.y + 1));
	if (isStrictMode &&
		!IsTileOKStrict(
			mo, t, tAbove, tBelow, MapBuilderIsLeaveFree(mb, v),
			MapGetNumWallsAdjacentTile(mb->Map, v),
			MapGetNumWallsAroundTile(mb->Map, v)))
	{
		return false;
	}
	else if (!MapObjectIsTileOK(mo, t, tAbove))
	{
		return false;
	}

	if (mo->Flags & (1 << PLACEMENT_FREE_IN_FRONT))
	{
		MapBuilderSetLeaveFree(mb, svec2i(v.x, v.y + 1), true);
	}

	NMapObjectAdd amo = NMapObjectAdd_init_default;
	amo.UID = ObjsGetNextUID();
	strcpy(amo.MapObjectClass, mo->Name);
	amo.has_Pos = true;
	amo.Pos = Vec2ToNet(MapObjectGetPlacementPos(mo, v));
	amo.ThingFlags = MapObjectGetFlags(mo) | extraFlags;
	amo.Health = mo->Health;
	ObjAdd(amo);
	return true;
}
static bool IsTileOKStrict(
	const MapObject *obj, const Tile *tile, const Tile *tileAbove,
	const Tile *tileBelow, const bool isLeaveFree, const int numWallsAdjacent,
	const int numWallsAround)
{
	if (!MapObjectIsTileOK(obj, tile, tileAbove))
	{
		return false;
	}
	if (isLeaveFree)
	{
		return false;
	}

	if (obj->Flags & (1 << PLACEMENT_OUTSIDE) && tile->Class->IsRoom)
	{
		return false;
	}
	if ((obj->Flags & (1 << PLACEMENT_INSIDE)) && !tile->Class->IsRoom)
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
	if ((obj->Flags & (1 << PLACEMENT_ONE_OR_MORE_WALLS)) &&
		numWallsAdjacent < 1)
	{
		return false;
	}
	if ((obj->Flags & (1 << PLACEMENT_FREE_IN_FRONT)) &&
		!TileIsClear(tileBelow))
	{
		return false;
	}

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
		 j < (mod->Density * mb->Map->Size.x * mb->Map->Size.y) / 1000; j++)
	{
		MapTryPlaceOneObject(mb, MapGetRandomTile(mb->Map), mod->M, 0, true);
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
		if (t->Class->IsRoom && TileIsClear(t) && TileCanWalk(t) &&
			MapBuildGetAccess(mb, v) == mapAccess &&
			// Ensure keys are visible, not hidden behind walls
			TileIsClear(MapGetTile(mb->Map, svec2i(v.x, v.y + 1))))
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
		mb, tilePos, pData->o->u.MapObject, ObjectiveToThing(pData->objective),
		true);
}

void MapBuilderSetLeaveFree(
	MapBuilder *mb, const struct vec2i tile, const bool value)
{
	if (!MapIsTileIn(mb->Map, tile))
		return;
	CArraySet(&mb->leaveFree, tile.y * mb->Map->Size.x + tile.x, &value);
}
bool MapBuilderIsLeaveFree(const MapBuilder *mb, const struct vec2i tile)
{
	if (!MapIsTileIn(mb->Map, tile))
		return false;
	return *(bool *)CArrayGet(
		&mb->leaveFree, tile.y * mb->Map->Size.x + tile.x);
}

static void MapSetupTile(MapBuilder *mb, const struct vec2i pos);

void MapBuildTile(
	MapBuilder *mb, const struct vec2i pos, const TileClass *tile)
{
	// Load tiles from +2 perimeter
	RECT_FOREACH(Rect2iNew(svec2i_subtract(pos, svec2i(2, 2)), svec2i(5, 5)))
	MapStaticLoadTile(mb, _v);
	RECT_FOREACH_END()
	// Update the tile as well, plus neighbours as they may be affected
	// by shadows etc. especially walls
	MapBuilderSetTile(mb, pos, tile);
	MapSetupTile(mb, pos);
	RECT_FOREACH(Rect2iNew(svec2i_subtract(pos, svec2i(1, 1)), svec2i(3, 3)))
	MapSetupTile(mb, _v);
	RECT_FOREACH_END()
	CArrayCopy(&mb->Map->access, &mb->access);
}

static bool MapTileIsNormalFloor(const MapBuilder *mb, const struct vec2i pos)
{
	// Normal floor tiles can be replaced randomly with
	// special floor tiles such as drainage
	const TileClass *tile = MapBuilderGetTile(mb, pos);
	if (tile->Type != TILE_CLASS_FLOOR || tile->IsRoom)
	{
		return false;
	}
	const TileClass *tAbove = MapBuilderGetTile(mb, svec2i(pos.x, pos.y - 1));
	if (!tAbove || tAbove->Type != TILE_CLASS_FLOOR || tAbove->IsRoom)
	{
		return false;
	}
	return true;
}

static void MapSetupTilesAndWalls(MapBuilder *mb)
{
	RECT_FOREACH(Rect2iNew(svec2i_zero(), mb->Map->Size))
	MapSetupTile(mb, _v);
	RECT_FOREACH_END()

	if (mb->mission->Type != MAPTYPE_STATIC || mb->mission->u.Static.AltFloorsEnabled)
	{
		// Randomly change normal floor tiles to alternative floor tiles
		for (int i = 0; i < mb->Map->Size.x * mb->Map->Size.y / 22; i++)
		{
			const struct vec2i pos = MapGetRandomTile(mb->Map);
			if (MapTileIsNormalFloor(mb, pos))
			{
				Tile *t = MapGetTile(mb->Map, pos);
				t->Class = TileClassesGetMaskedTile(
					mb->Map->TileClasses,
					t->Class, t->Class->Style, "alt1", t->Class->Mask,
					t->Class->MaskAlt);
			}
		}
		for (int i = 0; i < mb->Map->Size.x * mb->Map->Size.y / 16; i++)
		{
			const struct vec2i pos = MapGetRandomTile(mb->Map);
			if (MapTileIsNormalFloor(mb, pos))
			{
				Tile *t = MapGetTile(mb->Map, pos);
				t->Class = TileClassesGetMaskedTile(
					mb->Map->TileClasses,
					t->Class, t->Class->Style, "alt2", t->Class->Mask,
					t->Class->MaskAlt);
			}
		}
	}
}
static const char *MapGetWallPic(const MapBuilder *m, const struct vec2i pos);
// Set tile properties for a map tile
static void MapSetupTile(MapBuilder *mb, const struct vec2i pos)
{
	if (!MapIsTileIn(mb->Map, pos))
		return;
	const Tile *tAbove = MapGetTile(mb->Map, svec2i(pos.x, pos.y - 1));
	const bool canSeeTileAbove = !(tAbove != NULL && TileIsOpaque(tAbove));
	Tile *t = MapGetTile(mb->Map, pos);
	if (!t)
	{
		return;
	}
	const TileClass *tc = MapBuilderGetTile(mb, pos);
	if (tc->Type == TILE_CLASS_FLOOR)
	{
		t->Class = TileClassesGetMaskedTile(
			mb->Map->TileClasses,
			tc, tc->Style, canSeeTileAbove ? "normal" : "shadow", tc->Mask,
			tc->MaskAlt);
	}
	else if (tc->Type == TILE_CLASS_WALL)
	{
		t->Class = TileClassesGetMaskedTile(
			mb->Map->TileClasses,
			tc, tc->Style, MapGetWallPic(mb, pos), tc->Mask, tc->MaskAlt);
	}
	else if (tc->Type == TILE_CLASS_DOOR)
	{
		t->Class = TileClassesGetMaskedTile(
			mb->Map->TileClasses,
			tc, tc->Style, "normal_h", tc->Mask, tc->MaskAlt);
	}
	else if (tc->Type == TILE_CLASS_NOTHING)
	{
		t->Class = &gTileNothing;
	}
	else
	{
		CASSERT(false, "cannot setup tile");
		t->Class = &gTileNothing;
	}
}
static bool W(const MapBuilder *mb, const int x, const int y);
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
static bool W(const MapBuilder *mb, const int x, const int y)
{
	const struct vec2i v = svec2i(x, y);
	if (!MapIsTileIn(mb->Map, v))
		return false;
	const TileClass *tc = MapBuilderGetTile(mb, v);
	return tc->Type == TILE_CLASS_WALL;
}

static bool MapIsValidStartForWall(
	const MapBuilder *mb, const struct vec2i pos, const bool isRoom,
	const int pad)
{
	if (!MapIsTileIn(mb->Map, pos) || pos.x == 0 || pos.y == 0 ||
		pos.x == mb->Map->Size.x - 1 || pos.y == mb->Map->Size.y - 1)
	{
		return false;
	}
	struct vec2i d;
	for (d.x = pos.x - pad; d.x <= pos.x + pad; d.x++)
	{
		for (d.y = pos.y - pad; d.y <= pos.y + pad; d.y++)
		{
			const TileClass *t = MapBuilderGetTile(mb, d);
			if (t == NULL || t->Type != TILE_CLASS_FLOOR ||
				t->IsRoom != isRoom)
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
	const int roomMin = MAX(r.Min, doorMin + 2);
	const int roomMax = MAX(r.Max, doorMin + 2);
	return svec2i(RAND_INT(roomMin, roomMax), RAND_INT(roomMin, roomMax));
}

static bool MapBuilderGetIsRoom(const MapBuilder *mb, const struct vec2i pos);
void MapMakeRoom(
	MapBuilder *mb, const struct vec2i pos, const struct vec2i size,
	const bool walls, const TileClass *wall, const TileClass *room,
	const bool removeInterRoomWalls)
{
	struct vec2i v;
	// Set the perimeter walls and interior
	// If the tile is a room interior already, do not turn it into a wall
	// This is due to overlapping rooms
	for (v.y = pos.y; v.y < pos.y + size.y; v.y++)
	{
		for (v.x = pos.x; v.x < pos.x + size.x; v.x++)
		{
			if (v.y == pos.y || v.y == pos.y + size.y - 1 || v.x == pos.x ||
				v.x == pos.x + size.x - 1)
			{
				const TileClass *t = MapBuilderGetTile(mb, v);
				if (walls && t != NULL && !t->IsRoom)
				{
					MapBuilderSetTile(mb, v, wall);
				}
			}
			else
			{
				MapBuilderSetTile(mb, v, room);
			}
		}
	}
	if (removeInterRoomWalls)
	{

		// Check perimeter again; if there are walls where both sides contain
		// rooms, remove the wall as the rooms have merged
		for (v.y = pos.y; v.y < pos.y + size.y; v.y++)
		{
			for (v.x = pos.x; v.x < pos.x + size.x; v.x++)
			{
				if (v.y == pos.y || v.y == pos.y + size.y - 1 ||
					v.x == pos.x || v.x == pos.x + size.x - 1)
				{
					if ((MapBuilderGetIsRoom(mb, svec2i(v.x + 1, v.y)) &&
						 MapBuilderGetIsRoom(mb, svec2i(v.x - 1, v.y))) ||
						(MapBuilderGetIsRoom(mb, svec2i(v.x, v.y + 1)) &&
						 MapBuilderGetIsRoom(mb, svec2i(v.x, v.y - 1))))
					{
						MapBuilderSetTile(mb, v, room);
					}
				}
			}
		}
	}
}
static bool MapBuilderGetIsRoom(const MapBuilder *mb, const struct vec2i pos)
{
	const TileClass *t = MapBuilderGetTile(mb, pos);
	return t != NULL && t->Type == TILE_CLASS_FLOOR && t->IsRoom;
}

void MapMakeRoomWalls(
	MapBuilder *mb, const RoomParams r, const TileClass *wall, const Rect2i room)
{
	int count = 0;
	for (int i = 0; i < 100 && count < r.Walls; i++)
	{
		if (!MapTryBuildWall(mb, true, MAX(r.WallPad, 1), r.WallLength, wall, room))
		{
			continue;
		}
		count++;
	}
}

static void MapGrowWall(
	MapBuilder *mb, struct vec2i pos, const bool isRoom, const int pad,
	const int d, int length, const TileClass *wall);
bool MapTryBuildWall(
	MapBuilder *mb, const bool isRoom, const int pad, const int wallLength,
	const TileClass *wall, const Rect2i r)
{
	const struct vec2i v =
		Rect2iIsZero(r) ?
		MapGetRandomTile(mb->Map) :
		svec2i_add(r.Pos, svec2i(rand() % r.Size.x, rand() % r.Size.y));
	if (MapIsValidStartForWall(mb, v, isRoom, pad))
	{
		MapBuilderSetTile(mb, v, wall);
		MapGrowWall(mb, v, isRoom, pad, rand() & 3, wallLength, wall);
		return true;
	}
	return false;
}
static bool MapGrowWallCheck(
	const MapBuilder *mb, const struct vec2i v, const bool isRoom);
static void MapGrowWall(
	MapBuilder *mb, struct vec2i pos, const bool isRoom, const int pad,
	const int d, int length, const TileClass *wall)
{
	int l;
	struct vec2i v;

	if (length <= 0)
		return;

	switch (d)
	{
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
				if (!MapGrowWallCheck(mb, v, isRoom))
					return;
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
				if (!MapGrowWallCheck(mb, v, isRoom))
					return;
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
				if (!MapGrowWallCheck(mb, v, isRoom))
					return;
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
				if (!MapGrowWallCheck(mb, v, isRoom))
					return;
			}
		}
		pos.x--;
		break;
	}
	MapBuilderSetTile(mb, pos, wall);
	length--;
	if (length > 0 && (rand() & 3) == 0)
	{
		// Randomly try to grow the wall in a different direction
		l = rand() % length;
		MapGrowWall(mb, pos, isRoom, pad, rand() & 3, l, wall);
		length -= l;
	}
	// Keep growing wall in same direction
	MapGrowWall(mb, pos, isRoom, pad, d, length, wall);
}
static bool MapGrowWallCheck(
	const MapBuilder *mb, const struct vec2i v, const bool isRoom)
{
	if (!MapIsTileIn(mb->Map, v))
		return true;
	const TileClass *t = MapBuilderGetTile(mb, v);
	if (t == NULL || t->Type != TILE_CLASS_FLOOR || t->IsRoom != isRoom)
	{
		return false;
	}
	return true;
}

void MapSetRoomAccessMask(
	MapBuilder *mb, const Rect2i r, const uint16_t accessMask)
{
	RECT_FOREACH(r)
	MapBuildSetAccess(mb, _v, accessMask);
	RECT_FOREACH_END()
}

static void AddOverlapRooms(
	MapBuilder *mb, const Rect2i room, CArray *overlapRooms, CArray *rooms,
	const uint16_t accessMask);
void MapSetRoomAccessMaskOverlap(
	MapBuilder *mb, CArray *rooms, const uint16_t accessMask)
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
	const uint16_t accessMask)
{
	// Find all rooms that overlap with a room, and move it to the overlap
	// rooms array, setting access mask as we go
	CA_FOREACH(const Rect2i, r, *rooms)
	if (Rect2iOverlap(room, *r))
	{
		LOG(LM_MAP, LL_TRACE,
			"Room overlap {%d, %d (%dx%d)} {%d, %d (%dx%d)} access(%d)",
			room.Pos.x, room.Pos.y, room.Size.x, room.Size.y, r->Pos.x,
			r->Pos.y, r->Size.x, r->Size.y, accessMask);
		MapSetRoomAccessMask(mb, *r, accessMask);
		CArrayPushBack(overlapRooms, r);
		CArrayDelete(rooms, _ca_index);
		_ca_index--;
	}
	CA_FOREACH_END()
}

static void PlaceDoors(MapBuilder *mb, const int doorSize, const int roomDim, const struct vec2i doorStart, const struct vec2i d, const bool randomPos, const TileClass *tile);
void MapPlaceDoors(
	MapBuilder *mb, const Rect2i r, const bool hasDoors, const bool doors[4],
	const int doorMin, const int doorMax, const uint16_t accessMask, const bool randomPos,
	const TileClass *door, const TileClass *floor)
{
	const TileClass *tile = hasDoors ? door : floor;

	MapSetRoomAccessMask(mb, r, accessMask);

	// Set the doors
	for (int i = 0; i < 4; i++)
	{
		if (!doors[i])
		{
			continue;
		}
		const int doorSize = doorMax > doorMin ? RAND_INT(doorMin, doorMax) : doorMin;
		int roomDim;
		struct vec2i d;
		struct vec2i doorStart;
		switch (i)
		{
			case 0:
				// left
				roomDim = r.Size.y;
				d = svec2i(1, 0);
				doorStart = r.Pos;
				break;
			case 1:
				// right
				roomDim = r.Size.y;
				d = svec2i(1, 0);
				doorStart = svec2i(r.Pos.x + r.Size.x - 1, r.Pos.y);
				break;
			case 2:
				// top
				roomDim = r.Size.x;
				d = svec2i(0, 1);
				doorStart = r.Pos;
				break;
			case 3:
				// bottom:
				roomDim = r.Size.x;
				d = svec2i(0, 1);
				doorStart = svec2i(r.Pos.x, r.Pos.y + r.Size.y - 1);
				break;
			default:
				CASSERT(false, "unexpected side index");
				return;
		}
		PlaceDoors(mb, doorSize, roomDim, doorStart, d, randomPos, tile);
	}
}
static bool TryPlaceDoorTile(
	MapBuilder *mb, const struct vec2i v, const struct vec2i d,
	const TileClass *tile);
static void PlaceDoors(MapBuilder *mb, const int doorSize, const int roomDim, const struct vec2i doorStart, const struct vec2i d, const bool randomPos, const TileClass *tile)
{
	const struct vec2i dAcross = svec2i_subtract(svec2i_one(), d);
	const int size = MIN(doorSize, roomDim - 2);
	struct vec2i start = doorStart;
	if (randomPos)
	{
		start = svec2i_add(start, svec2i_scale(dAcross, (float)RAND_INT(1, roomDim - size - 1)));
	}
	else
	{
		start = svec2i_add(start, svec2i_scale(dAcross, (float)((roomDim - size) / 2)));
	}
	for (int i = 0; i < size; i++)
	{
		const struct vec2i v = svec2i_add(start, svec2i_scale(dAcross, (float)i));
		if (!TryPlaceDoorTile(mb, v, d, tile))
		{
			break;
		}
	}
}
static bool TryPlaceDoorTile(
	MapBuilder *mb, const struct vec2i v, const struct vec2i d,
	const TileClass *tile)
{
	if (MapBuilderGetTile(mb, v)->Type == TILE_CLASS_WALL &&
		MapBuilderGetTile(mb, svec2i_add(v, d))->Type != TILE_CLASS_WALL &&
		MapBuilderGetTile(mb, svec2i_subtract(v, d))->Type != TILE_CLASS_WALL)
	{
		MapBuilderSetTile(mb, v, tile);
		return true;
	}
	return false;
}

bool MapIsAreaInside(
	const Map *map, const struct vec2i pos, const struct vec2i size)
{
	return pos.x >= 0 && pos.y >= 0 && pos.x + size.x < map->Size.x &&
		   pos.y + size.y < map->Size.y;
}

bool MapBuilderIsAreaFunc(
	const MapBuilder *mb, const struct vec2i pos, const struct vec2i size,
	bool (*func)(const MapBuilder *, const struct vec2i))
{
	if (!MapIsAreaInside(mb->Map, pos, size))
	{
		return false;
	}
	RECT_FOREACH(Rect2iNew(pos, size))
	if (!func(mb, _v))
		return false;
	RECT_FOREACH_END()
	return true;
}

static bool IsClear(const MapBuilder *mb, const struct vec2i pos)
{
	return MapBuilderGetTile(mb, pos)->Type == TILE_CLASS_FLOOR;
}
bool MapIsAreaClear(
	const MapBuilder *mb, const struct vec2i pos, const struct vec2i size)
{
	return MapBuilderIsAreaFunc(mb, pos, size, IsClear);
}
static bool AreaHasRoomAndFloor(const MapBuilder *mb, const struct vec2i pos)
{
	// Find whether a wall tile is part of a room perimeter
	// The surrounding tiles must have normal floor and room tiles
	// to be a perimeter
	bool hasRoom = false;
	bool hasFloor = false;
	RECT_FOREACH(Rect2iNew(svec2i_subtract(pos, svec2i_one()), svec2i(3, 3)))
	const TileClass *t = MapBuilderGetTile(mb, _v);
	if (t == NULL || t->Type != TILE_CLASS_FLOOR)
		continue;
	if (t->IsRoom)
		hasRoom = true;
	else
		hasFloor = true;
	RECT_FOREACH_END()
	return hasRoom && hasFloor;
}
static bool IsClearOrRoom(const MapBuilder *mb, const struct vec2i pos)
{
	const TileClass *tile = MapBuilderGetTile(mb, pos);
	if (tile->Type == TILE_CLASS_FLOOR)
		return true;
	// Check if this wall is part of a room
	if (tile->Type != TILE_CLASS_WALL)
		return false;
	return AreaHasRoomAndFloor(mb, pos);
}
bool MapIsAreaClearOrRoom(
	const MapBuilder *mb, const struct vec2i pos, const struct vec2i size)
{
	return MapBuilderIsAreaFunc(mb, pos, size, IsClearOrRoom);
}
static bool IsClearOrWall(const MapBuilder *mb, const struct vec2i pos)
{
	const TileClass *tile = MapBuilderGetTile(mb, pos);
	if (tile->Type == TILE_CLASS_FLOOR && !tile->IsRoom)
		return true;
	// Check if this wall is part of a room
	if (tile->Type != TILE_CLASS_WALL)
		return false;
	return AreaHasRoomAndFloor(mb, pos);
}
bool MapIsAreaClearOrWall(
	const MapBuilder *mb, struct vec2i pos, struct vec2i size)
{
	return MapBuilderIsAreaFunc(mb, pos, size, IsClearOrWall);
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
	const MapBuilder *mb, const Rect2i r, uint16_t *overlapAccess)
{
	int numOverlaps = 0;
	struct vec2i overlapMin = svec2i_zero();
	struct vec2i overlapMax = svec2i_zero();

	// Find perimeter tiles that overlap
	RECT_FOREACH(r)
	// only check perimeter
	if (_v.x != r.Pos.x && _v.x != r.Pos.x + r.Size.x - 1 && _v.y != r.Pos.y &&
		_v.y != r.Pos.y + r.Size.y - 1)
	{
		continue;
	}
	if (!MapIsTileIn(mb->Map, _v))
	{
		continue;
	}
	const TileClass *tile = MapBuilderGetTile(mb, _v);
	// Check if this wall is part of a room
	if (tile->Type != TILE_CLASS_WALL || !AreaHasRoomAndFloor(mb, _v))
	{
		continue;
	}
	// Get the access level of the room
	struct vec2i v2;
	for (v2.y = _v.y - 1; v2.y <= _v.y + 1; v2.y++)
	{
		for (v2.x = _v.x - 1; v2.x <= _v.x + 1; v2.x++)
		{
			const TileClass *t = MapBuilderGetTile(mb, v2);
			if (t != NULL && t->IsRoom)
			{
				if (overlapAccess != NULL)
				{
					*overlapAccess |= MapBuildGetAccess(mb, v2);
				}
			}
		}
	}
	if (numOverlaps == 0)
	{
		overlapMin = overlapMax = _v;
	}
	else
	{
		overlapMin = svec2i_min(overlapMin, _v);
		overlapMax = svec2i_max(overlapMax, _v);
	}
	numOverlaps++;
	RECT_FOREACH_END()
	if (numOverlaps < 2)
	{
		return 0;
	}

	// Now check that all tiles between the first and last tiles are room or
	// perimeter tiles
	struct vec2i v;
	for (v.y = overlapMin.y; v.y <= overlapMax.y; v.y++)
	{
		for (v.x = overlapMin.x; v.x <= overlapMax.x; v.x++)
		{
			const TileClass *tile = MapBuilderGetTile(mb, v);
			if (tile->Type == TILE_CLASS_WALL)
			{
				// Check if this wall is part of a room
				if (!AreaHasRoomAndFloor(mb, v))
				{
					return 0;
				}
			}
			else if (!tile->IsRoom)
			{
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
			if (v.x != pos.x && v.x != pos.x + size.x - 1 && v.y != pos.y &&
				v.y != pos.y + size.y - 1)
			{
				continue;
			}
			if (MapBuilderGetTile(mb, v)->Type != TILE_CLASS_WALL)
			{
				continue;
			}
			// Check if this wall is part of a room
			if (!AreaHasRoomAndFloor(mb, v))
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
			if (MapBuilderGetTile(mb, v)->Type != TILE_CLASS_WALL)
			{
				// invalid tile type
				return false;
			}
			// Check if this wall is not part of a room
			if (AreaHasRoomAndFloor(mb, v))
			{
				return false;
			}
		}
	}

	return true;
}

void MapFillRect(MapBuilder *mb, const Rect2i r, const TileClass *edge, const TileClass *fill)
{
	RECT_FOREACH(r)
	const TileClass *tc = Rect2iIsAtEdge(r, _v) ? edge : fill;
	MapBuilderSetTile(mb, _v, tc);
	RECT_FOREACH_END()
}

uint16_t GenerateAccessMask(int *accessLevel)
{
	uint16_t accessMask = 0;
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
	if (*accessLevel < 1)
	{
		*accessLevel = 1;
	}
	return accessMask;
}

static void MapGenerateRandomExitArea(Map *map, const int mission)
{
	const Tile *t = NULL;
	Exit exit;
	exit.Mission = mission + 1;
	exit.Hidden = false;
	for (int i = 0; i < 10000 && (t == NULL || !TileCanWalk(t)); i++)
	{
		exit.R.Pos.x = (rand() % (abs(map->Size.x) - EXIT_WIDTH - 1));
		exit.R.Size.x = EXIT_WIDTH + 1;
		exit.R.Pos.y = (rand() % (abs(map->Size.y) - EXIT_HEIGHT - 1));
		exit.R.Size.y = EXIT_HEIGHT + 1;
		// Check that the exit area is walkable
		t = MapGetTile(map, Rect2iCenter(exit.R));
	}
	CArrayPushBack(&map->exits, &exit);
}

static void MapAddDrains(MapBuilder *mb)
{
	// Randomly add drainage tiles for classic map type;
	// For other map types drains are regular map objects
	const MapObject *drain = StrMapObject("drain0");
	if (drain == NULL)
	{
		return;
	}
	for (int i = 0; i < mb->Map->Size.x * mb->Map->Size.y / 45; i++)
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
