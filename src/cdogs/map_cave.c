/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (c) 2016-2017 Cong Xu
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
#include "map_cave.h"

#include "algorithms.h"
#include "map_build.h"


static void CaveRep(Map *map, const int r1, const int r2);
static void LinkDisconnectedAreas(Map *map);
static void FixCorridors(Map *map, const int corridorWidth);
static void PlaceSquares(Map *map, const int squares);
void MapCaveLoad(
	Map *map, const struct MissionOptions *mo, const CampaignOptions* co)
{
	// Re-seed RNG so results are consistent
	CampaignSeedRandom(co);

	const Mission *m = mo->missionData;

	// Randomly set a percentage of the tiles as walls
	for (int i = 0;
		i < m->u.Cave.FillPercent*map->Size.x*map->Size.y / 100;
		i++)
	{
		const Vec2i pos = Vec2iNew(i % map->Size.x, i / map->Size.x);
		IMapSet(map, pos, MAP_WALL);
	}
	// Shuffle
	CArrayShuffle(&map->iMap);
	// Repetitions
	for (int i = 0; i < m->u.Cave.Repeat; i++)
	{
		CaveRep(map, m->u.Cave.R1, m->u.Cave.R2);
	}

	LinkDisconnectedAreas(map);

	FixCorridors(map, m->u.Cave.CorridorWidth);

	PlaceSquares(map, m->u.Cave.Squares);
}

// Perform one generation of cellular automata
// If the number of walls within 1 distance is at least R1, OR
// if the number of walls within 2 distance is at most R2, then the tile
// becomes a wall; otherwise it is a floor
static int CountTilesAround(
	const Map *map, const Vec2i pos, const int radius,
	const unsigned short tile);
static void CaveRep(Map *map, const int r1, const int r2)
{
	CArray buf;
	CArrayInit(&buf, map->iMap.elemSize);
	const unsigned short floor = MAP_FLOOR;
	CArrayResize(&buf, map->iMap.size, &floor);
	Vec2i v;
	for (v.y = 0; v.y < map->Size.y; v.y++)
	{
		for (v.x = 0; v.x < map->Size.x; v.x++)
		{
			const int idx = v.x + v.y * map->Size.x;
			unsigned short *tile = CArrayGet(&buf, idx);
			if (CountTilesAround(map, v, 1, MAP_WALL) >= r1 ||
				CountTilesAround(map, v, 2, MAP_WALL) <= r2)
			{
				*tile = MAP_WALL;
			}
			else
			{
				*tile = MAP_FLOOR;
			}
		}
	}
	CArrayCopy(&map->iMap, &buf);
	CArrayTerminate(&buf);
}
static int CountTilesAround(
	const Map *map, const Vec2i pos, const int radius,
	const unsigned short tile)
{
	int c = 0;
	for (int x = pos.x - radius; x <= pos.x + radius; x++)
	{
		for (int y = pos.y - radius; y <= pos.y + radius; y++)
		{
			// Also count edge of maps
			if (x < 0 || x >= map->Size.x || y < 0 || y >= map->Size.y ||
				IMapGet(map, Vec2iNew(x, y)) == tile)
			{
				c++;
			}
		}
	}
	return c;
}

static void MapFloodFill(
	CArray *fl, const Vec2i size, const int idx, const int elem);
static void AddCorridor(
	Map *map, const Vec2i v1, const Vec2i v2, const Vec2i dInit,
	const unsigned short tile);
static void LinkDisconnectedAreas(Map *map)
{
	// Use flood fill to identify disconnected areas
	CArray fl;
	CArrayInit(&fl, sizeof(int));
	const int zero = 0;
	CArrayResize(&fl, map->iMap.size, &zero);
	// First copy across the wall tiles (as -1)
	CA_FOREACH(const unsigned short, tile, map->iMap)
		if (*tile == MAP_WALL)
		{
			*(int *)CArrayGet(&fl, _ca_index) = -1;
		}
	CA_FOREACH_END()
	// Then perform flood fill conditionally on the flood layer
	int idx = 1;
	CA_FOREACH(int, i, fl)
		if (*i == 0)
		{
			MapFloodFill(&fl, map->Size, _ca_index, idx);
			idx++;
		}
	CA_FOREACH_END()

	const int numAreas = idx - 1;
	// Connect the disconnected areas, first to second, second to third etc.
	// Select random tile from each area, using index shuffle
	CArray areaTiles;
	CArrayInit(&areaTiles, sizeof(int));
	CA_FOREACH(int, i, fl)
		UNUSED(i);
		CArrayPushBack(&areaTiles, &_ca_index);
	CA_FOREACH_END()
	CArrayShuffle(&areaTiles);
	CArray areaStarts;
	CArrayInit(&areaStarts, sizeof(int));
	CArrayResize(&areaStarts, numAreas, &zero);
	CA_FOREACH(int, areaIdx, areaTiles)
		const int tile = *(int *)CArrayGet(&fl, *areaIdx) - 1;
		if (tile >= 0 && tile < numAreas)
		{
			int *areaStart = CArrayGet(&areaStarts, tile);
			if (*areaStart == 0)
			{
				*areaStart = *areaIdx;
			}
		}
	CA_FOREACH_END()
	CArrayTerminate(&fl);
	CArrayTerminate(&areaTiles);
	// Connect consecutive pairs of tiles
	for (int i = 0; i < (int)areaStarts.size - 1; i++)
	{
		const int *a1 = CArrayGet(&areaStarts, i);
		const int *a2 = CArrayGet(&areaStarts, i + 1);
		const Vec2i v1 = Vec2iNew(*a1 % map->Size.x, *a1 / map->Size.x);
		const Vec2i v2 = Vec2iNew(*a2 % map->Size.x, *a2 / map->Size.x);
		const Vec2i delta = Vec2iNew(abs(v1.x - v2.x), abs(v1.y - v2.y));
		const int dx = delta.x > delta.y ? 1 : 0;
		const int dy = 1 - dx;
		AddCorridor(map, v1, v2, Vec2iNew(dx, dy), MAP_FLOOR);
	}
	CArrayTerminate(&areaStarts);
}

static void MapFloodFill(
	CArray *fl, const Vec2i size, const int idx, const int elem)
{
	CArray indices;
	CArrayInit(&indices, sizeof(int));
	CArrayPushBack(&indices, &idx);
	const int floodTile = *(int *)CArrayGet(fl, idx);
	*(int *)CArrayGet(fl, idx) = elem;
	CA_FOREACH(int, i, indices)
		const int x = *i % size.x;
		const int y = *i / size.x;
		int next;
		// top
		next = (y - 1)*size.x + x;
		if (y > 0 && *(int *)CArrayGet(fl, next) == floodTile)
		{
			CArrayPushBack(&indices, &next);
			*(int *)CArrayGet(fl, next) = elem;
		}
		// bottom
		next = (y + 1)*size.x + x;
		if (y < size.y - 1 && *(int *)CArrayGet(fl, next) == floodTile)
		{
			CArrayPushBack(&indices, &next);
			*(int *)CArrayGet(fl, next) = elem;
		}
		// left
		next = y*size.x + x - 1;
		if (x > 0 && *(int *)CArrayGet(fl, next) == floodTile)
		{
			CArrayPushBack(&indices, &next);
			*(int *)CArrayGet(fl, next) = elem;
		}
		// right
		next = y*size.x + x + 1;
		if (x < size.x - 1 && *(int *)CArrayGet(fl, next) == floodTile)
		{
			CArrayPushBack(&indices, &next);
			*(int *)CArrayGet(fl, next) = elem;
		}
	CA_FOREACH_END()
	CArrayTerminate(&indices);
}

// Add an S-shaped corridor from one point to another, filling it with a
// certain tile value. The corridor starts in a specific direction d, then
// makes a turn in the middle, then turns back to the original direction.
static void AddCorridor(
	Map *map, const Vec2i v1, const Vec2i v2, const Vec2i dInit,
	const unsigned short tile)
{
	Vec2i dAlt;
	// Location of the turn
	Vec2i half;
	Vec2i start = v1;
	Vec2i end = v2;
	Vec2i d = dInit;
	if (d.x > 0)
	{
		// horizontal
		d = Vec2iNew(1, 0);
		if (start.x > end.x)
		{
			// Swap
			const Vec2i tmp = start;
			start = end;
			end = tmp;
		}
		dAlt = Vec2iNew(0, 1);
		half = Vec2iNew((end.x - start.x) / 2 + start.x, end.y + 1);
		if (end.y < start.y)
		{
			dAlt.y = -1;
			half.y = end.y - 1;
		}
	}
	else
	{
		// vertical
		d = Vec2iNew(0, 1);
		if (start.y > end.y)
		{
			// Swap
			const Vec2i tmp = start;
			start = end;
			end = tmp;
		}
		dAlt = Vec2iNew(1, 0);
		half = Vec2iNew(end.x + 1, (end.y - start.y) / 2 + start.y);
		if (end.x < start.x)
		{
			dAlt.x = -1;
			half.x = end.x - 1;
		}
	}
	// Initial direction
	Vec2i v = start;
	for (; v.x != half.x && v.y != half.y; v = Vec2iAdd(v, d))
	{
		IMapSet(map, v, tile);
	}
	// Turn
	for (; v.x != end.x && v.y != end.y; v = Vec2iAdd(v, dAlt))
	{
		IMapSet(map, v, tile);
	}
	// Finish
	for (; v.x != end.x || v.y != end.y; v = Vec2iAdd(v, d))
	{
		IMapSet(map, v, tile);
	}
	IMapSet(map, end, tile);
}

static bool CheckCorridorsAroundTile(
	const Map *map, const int corridorWidth, const Vec2i v);
// Make sure corridors are wide enough
static void FixCorridors(Map *map, const int corridorWidth)
{
	// Find all instances where a corridor is too narrow
	// Do this by drawing a line from each wall tile to its surrounding square
	// area, and that this line of tiles that each wall tile is either
	// - blocked by a neighbouring wall tile, or
	// - only consists of floor tiles
	// If not, then replace that wall with a floor tile
	Vec2i v;
	for (v.y = corridorWidth; v.y < map->Size.y - corridorWidth; v.y++)
	{
		for (v.x = corridorWidth; v.x < map->Size.x - corridorWidth; v.x++)
		{
			const int idx = v.x + v.y * map->Size.x;
			unsigned short *tile = CArrayGet(&map->iMap, idx);
			// Make sure that the tile is a wall
			if (*tile != MAP_WALL)
			{
				continue;
			}
			if (!CheckCorridorsAroundTile(map, corridorWidth, v))
			{
				// Corridor checks failed; replace with a floor tile
				*tile = MAP_FLOOR;
			}
		}
	}
}
typedef struct
{
	const Map *M;
	int Counter;
	bool IsFirstWall;
	bool AreAllFloors;
} FixCorridorOnTileData;
static void FixCorridorOnTile(void *data, Vec2i v);
static bool CheckCorridorsAroundTile(
	const Map *map, const int corridorWidth, const Vec2i v)
{
	AlgoLineDrawData data;
	data.Draw = FixCorridorOnTile;
	FixCorridorOnTileData onTileData;
	onTileData.M = map;
	data.data = &onTileData;
	Vec2i v1;
	for (v1.y = v.y - corridorWidth; v1.y <= v.y + corridorWidth; v1.y++)
	{
		for (v1.x = v.x - corridorWidth; v1.x <= v.x + corridorWidth; v1.x++)
		{
			// only draw out to edges
			if (v1.y != v.y - corridorWidth && v1.y != v.y + corridorWidth &&
				v1.x != v.x - corridorWidth && v1.x != v.x + corridorWidth)
			{
				continue;
			}
			onTileData.Counter = 0;
			onTileData.IsFirstWall = false;
			onTileData.AreAllFloors = true;
			BresenhamLineDraw(v, v1, &data);
			if (!onTileData.IsFirstWall && !onTileData.AreAllFloors)
			{
				return false;
			}
		}
	}
	return true;
}
static void FixCorridorOnTile(void *data, Vec2i v)
{
	FixCorridorOnTileData *onTileData = data;
	if (onTileData->Counter == 1)
	{
		// This is the first tile after the starting tile
		// If this tile is a wall, result is good, and we don't care about
		// the rest of the tiles anymore
		onTileData->IsFirstWall = IMapGet(onTileData->M, v) == MAP_WALL;
	}
	if (onTileData->Counter > 0)
	{
		if (IMapGet(onTileData->M, v) != MAP_FLOOR)
		{
			onTileData->AreAllFloors = false;
		}
	}
	onTileData->Counter++;
}

static bool MapIsAreaClearForCaveSquare(
	const Map *map, const Vec2i pos, const Vec2i size);
static void PlaceSquares(Map *map, const int squares)
{
	// Place empty square areas on the map
	// This can only be done if at least one tile in the square is a floor type
	int count = 0;
	for (int i = 0; i < 1000 && count < squares; i++)
	{
		const Vec2i v = MapGetRandomTile(map);
		const Vec2i size = Vec2iNew(rand() % 9 + 8, rand() % 9 + 8);
		if (!MapIsAreaClearForCaveSquare(map, v, size))
		{
			continue;
		}
		MapMakeSquare(map, v, size);
		count++;
	}
}
static bool MapIsAreaClearForCaveSquare(
	const Map *map, const Vec2i pos, const Vec2i size)
{
	if (!MapIsAreaInside(map, pos, size))
	{
		return false;
	}

	// For area to be clear, it must have:
	// - At least one floor tile
	// - No square tiles
	Vec2i v;
	bool hasFloor = false;
	for (v.y = pos.y; v.y < pos.y + size.y; v.y++)
	{
		for (v.x = pos.x; v.x < pos.x + size.x; v.x++)
		{
			switch (IMapGet(map, v))
			{
				case MAP_FLOOR:
					hasFloor = true;
					break;
				case MAP_WALL:
					break;
				default:
					// Any other tile type is disallowed
					return false;
			}
		}
	}
	return hasFloor;
}
