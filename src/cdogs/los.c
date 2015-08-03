/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
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
#include "los.h"

#include "algorithms.h"
#include "game_events.h"
#include "net_util.h"


void LOSInit(Map *map, const Vec2i size)
{
	CArrayInit(&map->LOS.LOS, sizeof(bool));
	CArrayInit(&map->LOS.Explored, sizeof(bool));
	Vec2i v;
	for (v.y = 0; v.y < size.y; v.y++)
	{
		for (v.x = 0; v.x < size.x; v.x++)
		{
			const bool f = false;
			CArrayPushBack(&map->LOS.LOS, &f);
			CArrayPushBack(&map->LOS.Explored, &f);
		}
	}
}
void LOSTerminate(LineOfSight *los)
{
	CArrayTerminate(&los->LOS);
	CArrayTerminate(&los->Explored);
}

// Reset lines of sight by setting all cells to unseen
void LOSReset(LineOfSight *los)
{
	CArrayFillZero(&los->LOS);
	CArrayFillZero(&los->Explored);
}
typedef struct
{
	Map *Map;
	Vec2i Center;
	int SightRange2;
	bool Explore;
} LOSData;
// Calculate LOS cells from a certain start position
// Sight range based on config
static void SetLOSVisible(Map *map, const Vec2i pos, const bool explore);
static bool IsNextTileBlockedAndSetVisibility(void *data, Vec2i pos);
static void SetObstructionVisible(
	Map *map, const Vec2i pos, const bool explore);
void LOSCalcFrom(Map *map, const Vec2i pos, const bool explore)
{
	// Perform LOS by casting rays from the centre to the edges, terminating
	// whenever an obstruction or out-of-range is reached.

	CArrayFillZero(&map->LOS.Explored);

	// First mark center tile and all adjacent tiles as visible
	// +-+-+-+
	// |V|V|V|
	// +-+-+-+
	// |V|C|V|
	// +-+-+-+
	// |V|V|V|  (C=center, V=visible)
	// +-+-+-+
	Vec2i end;
	for (end.x = pos.x - 1; end.x <= pos.x + 1; end.x++)
	{
		for (end.y = pos.y - 1; end.y <= pos.y + 1; end.y++)
		{
			SetLOSVisible(map, end, explore);
		}
	}

	const int sightRange = ConfigGetInt(&gConfig, "Game.SightRange");
	if (sightRange == 0) return;

	// Limit the perimeter to the sight range
	const Vec2i origin = Vec2iNew(pos.x - sightRange, pos.y - sightRange);
	const Vec2i perimSize = Vec2iScale(Vec2iMinus(pos, origin), 2);

	LOSData data;
	data.Map = map;
	data.Center = pos;
	data.SightRange2 = sightRange * sightRange;
	data.Explore = explore;

	// Start from the top-left cell, and proceed clockwise around
	end = origin;
	HasClearLineData lineData;
	lineData.IsBlocked = IsNextTileBlockedAndSetVisibility;
	lineData.data = &data;
	// Top edge
	for (; end.x < origin.x + perimSize.x; end.x++)
	{
		HasClearLineXiaolinWu(pos, end, &lineData);
	}
	// right edge
	for (; end.y < origin.y + perimSize.y; end.y++)
	{
		HasClearLineXiaolinWu(pos, end, &lineData);
	}
	// bottom edge
	for (; end.x > origin.x; end.x--)
	{
		HasClearLineXiaolinWu(pos, end, &lineData);
	}
	// left edge
	for (; end.y > origin.y; end.y--)
	{
		HasClearLineXiaolinWu(pos, end, &lineData);
	}

	// Second pass: make any non-visible obstructions that are adjacent to
	// visible non-obstructions visible too
	// This is to ensure runs of walls stay visible
	for (end.y = origin.y; end.y < origin.y + perimSize.y; end.y++)
	{
		for (end.x = origin.x; end.x < origin.x + perimSize.x; end.x++)
		{
			const Tile *tile = MapGetTile(map, end);
			if (!tile || !(tile->flags & MAPTILE_NO_SEE))
			{
				continue;
			}
			// Check sight range
			if (DistanceSquared(pos, end) >= data.SightRange2)
			{
				continue;
			}
			SetObstructionVisible(map, end, explore);
		}
	}

	// Find all the newly visible tiles and set events for them
	GameEvent e = GameEventNew(GAME_EVENT_EXPLORE_TILES);
	e.u.ExploreTiles.Runs_count = 0;
	e.u.ExploreTiles.Runs[0].Run = 0;
	bool run = false;
	for (end.y = 0; end.y < map->Size.y; end.y++)
	{
		for (end.x = 0; end.x < map->Size.x; end.x++)
		{
			if (LOSAddRun(
				&e.u.ExploreTiles, &run, end,
				*((bool *)CArrayGet(&map->LOS.Explored, end.y * map->Size.x + end.x))))
			{
				GameEventsEnqueue(&gGameEvents, e);
				e.u.ExploreTiles.Runs_count = 0;
				e.u.ExploreTiles.Runs[0].Run = 0;
				run = false;
			}
		}
	}
	if (e.u.ExploreTiles.Runs_count > 0)
	{
		GameEventsEnqueue(&gGameEvents, e);
	}
	CArrayFillZero(&map->LOS.Explored);
}
static void SetLOSVisible(Map *map, const Vec2i pos, const bool explore)
{
	const Tile *t = MapGetTile(map, pos);
	if (t == NULL) return;
	*((bool *)CArrayGet(&map->LOS.LOS, pos.y * map->Size.x + pos.x)) = true;
	if (!t->isVisited && explore)
	{
		// Cache the newly explored tile
		*((bool *)CArrayGet(&map->LOS.Explored, pos.y * map->Size.x + pos.x)) = true;
	}
}
static bool IsNextTileBlockedAndSetVisibility(void *data, Vec2i pos)
{
	LOSData *lData = data;
	// Check sight range
	if (DistanceSquared(lData->Center, pos) >= lData->SightRange2) return true;
	// Check map range
	const Tile *t = MapGetTile(lData->Map, pos);
	if (t == NULL) return true;
	SetLOSVisible(lData->Map, pos, lData->Explore);
	// Check if this tile is an obstruction
	return t->flags & MAPTILE_NO_SEE;
}
static bool IsTileVisibleNonObstruction(Map *map, const Vec2i pos);
static void SetObstructionVisible(
	Map *map, const Vec2i pos, const bool explore)
{
	Vec2i d;
	for (d.x = -1; d.x < 2; d.x++)
	{
		for (d.y = -1; d.y < 2; d.y++)
		{
			if (IsTileVisibleNonObstruction(map, Vec2iAdd(pos, d)))
			{
				SetLOSVisible(map, pos, explore);
				return;
			}
		}
	}
}
static bool IsTileVisibleNonObstruction(Map *map, const Vec2i pos)
{
	const Tile *t = MapGetTile(map, pos);
	if (t == NULL) return false;
	return !(t->flags & MAPTILE_NO_SEE) && LOSTileIsVisible(map, pos);
}

bool LOSAddRun(
	NExploreTiles *runs, bool *run, const Vec2i tile, const bool explored)
{
	if (explored)
	{
		if (!*run)
		{
			// Start of new run
			runs->Runs_count++;
			runs->Runs[runs->Runs_count - 1].Tile = Vec2i2Net(tile);
			runs->Runs[runs->Runs_count - 1].Run = 0;
		}
		runs->Runs[runs->Runs_count - 1].Run++;
		*run = true;
	}
	else
	{
		// End of run
		// If we have too many runs, send off the event and start a new one
		if (runs->Runs_count == sizeof runs->Runs / sizeof runs->Runs[0])
		{
			return true;
		}
		*run = false;
	}
	return false;
}

bool LOSTileIsVisible(Map *map, const Vec2i pos)
{
	if (MapGetTile(map, pos) == NULL) return false;
	return *((bool *)CArrayGet(&map->LOS.LOS, pos.y * map->Size.x + pos.x));
}
