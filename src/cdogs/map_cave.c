/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (c) 2016, Cong Xu
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

#include "actors.h"
#include "campaigns.h"
#include "events.h"
#include "game_events.h"
#include "gamedata.h"
#include "handle_game_events.h"
#include "log.h"
#include "map_build.h"
#include "net_util.h"


static void CaveRep(Map *map, const int r1, const int r2);
static void MapFloodFill(
	CArray *fl, const Vec2i size, const int idx, const int elem);
static void AddCorridor(
	Map *map, const Vec2i v1, const Vec2i v2, const Vec2i dInit,
	const unsigned short tile);
void MapCaveLoad(Map *map, const struct MissionOptions *mo)
{
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
			const int idx = v.x + v.y * map->Size.y;
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
