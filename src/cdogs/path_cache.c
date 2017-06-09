/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
	Copyright (c) 2014, 2016 Cong Xu
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
#include "path_cache.h"

#include <math.h>
#include <time.h>

#include "ai_utils.h"
#include "log.h"

#define PATH_CACHE_MAX 128

PathCache gPathCache;


static CachedPath CachedPathCopy(CachedPath *c)
{
	CachedPath copy;
	memcpy(&copy, c, sizeof *c);
	(*copy.refs)++;
	return copy;
}
void CachedPathDestroy(CachedPath *c)
{
	if (c->Path == NULL && c->refs == NULL)
	{
		return;
	}
	(*c->refs)--;
	CASSERT(*c->refs >= 0, "out of sync ref count");
	if (*c->refs == 0)
	{
		ASPathDestroy(c->Path);
		CFREE(c->refs);
	}
}

static bool CachedPathMatches(
	const CachedPath *c, const Vec2i from, const Vec2i to)
{
	return Vec2iEqual(c->from, from) && Vec2iEqual(c->to, to);
}


void PathCacheInit(PathCache *pc, Map *m)
{
	CArrayInit(&pc->paths, sizeof(CachedPath));
	pc->head = 0;
	pc->map = m;
}
void PathCacheTerminate(PathCache *pc)
{
	PathCacheClear(pc);
	CArrayTerminate(&pc->paths);
}

void PathCacheClear(PathCache *pc)
{
	CA_FOREACH(CachedPath, c, pc->paths)
		CachedPathDestroy(c);
	CA_FOREACH_END()
	CArrayClear(&pc->paths);
	pc->head = 0;
}

typedef struct
{
	Map *Map;
	TileSelectFunc IsTileOk;
} AStarContext;
static void AddTileNeighbors(
	ASNeighborList neighbors, void *node, void *context);
static float AStarHeuristic(void *fromNode, void *toNode, void *context);
static ASPathNodeSource cPathNodeSource =
{
	sizeof(Vec2i), AddTileNeighbors, AStarHeuristic, NULL, NULL
};
CachedPath PathCacheCreate(
	PathCache *pc, Vec2i from, Vec2i to,
	const bool ignoreObjects, const bool cache)
{
	// Search through existing cache for path
	CA_FOREACH(CachedPath, c, pc->paths)
		if (CachedPathMatches(c, from, to))
		{
			LOG(LM_PATH, LL_TRACE, "cached path (%d, %d) to (%d, %d)...",
				from.x, from.y, to.x, to.y);
			return CachedPathCopy(c);
		}
	CA_FOREACH_END()

	LOG(LM_PATH, LL_TRACE, "find path (%d, %d) to (%d, %d)...",
		from.x, from.y, to.x, to.y);
	const clock_t start = clock();

	// Cached path not found; find the path now
	CachedPath cp;
	AStarContext ac;
	ac.Map = pc->map;
	ac.IsTileOk = ignoreObjects ? IsTileWalkable : IsTileWalkableAroundObjects;
	cp.Path = ASPathCreate(&cPathNodeSource, &ac, &from, &to);
	CMALLOC(cp.refs, sizeof *cp.refs);
	(*cp.refs) = 1;
	cp.from = from;
	cp.to = to;
	// Cache the path, optionally
	if (cache)
	{
		(*cp.refs)++;
		// Add to the cache if we are under the max size
		if ((int)pc->paths.size < PATH_CACHE_MAX)
		{
			CArrayPushBack(&pc->paths, &cp);
		}
		else
		{
			// Replace the oldest cached path with this one
			CachedPath *oldest = CArrayGet(&pc->paths, pc->head);
			CachedPathDestroy(oldest);
			memcpy(oldest, &cp, sizeof cp);
			// Move the head
			pc->head++;
			if (pc->head == pc->paths.size)
			{
				pc->head = 0;
			}
		}
		LOG(LM_PATH, LL_TRACE, "Cached %d paths", (int)pc->paths.size);
	}
	const clock_t diff = clock() - start;
	const int ms = diff * 1000 / CLOCKS_PER_SEC;
	LOG(LM_PATH, LL_DEBUG, "Pathfind time %dms", ms);
	return cp;
}

static void AddTileNeighbors(
	ASNeighborList neighbors, void *node, void *context)
{
	Vec2i *v = node;
	int y;
	AStarContext *c = context;
	for (y = v->y - 1; y <= v->y + 1; y++)
	{
		int x;
		if (y < 0 || y >= c->Map->Size.y)
		{
			continue;
		}
		for (x = v->x - 1; x <= v->x + 1; x++)
		{
			float cost;
			Vec2i neighbor;
			neighbor.x = x;
			neighbor.y = y;
			if (x < 0 || x >= c->Map->Size.x)
			{
				continue;
			}
			if (x == v->x && y == v->y)
			{
				continue;
			}
			// if we're moving diagonally,
			// need to check the axis-aligned neighbours are also clear
			if (!c->IsTileOk(c->Map, Vec2iNew(x, y)) ||
				!c->IsTileOk(c->Map, Vec2iNew(v->x, y)) ||
				!c->IsTileOk(c->Map, Vec2iNew(x, v->y)))
			{
				continue;
			}
			// Calculate cost of direction
			// Note that there are different horizontal and vertical costs,
			// due to the tiles being non-square
			// Slightly prefer axes instead of diagonals
			if (x != v->x && y != v->y)
			{
				cost = TILE_WIDTH * 1.1f;
			}
			else if (x != v->x)
			{
				cost = TILE_WIDTH;
			}
			else
			{
				cost = TILE_HEIGHT;
			}
			ASNeighborListAdd(neighbors, &neighbor, cost);
		}
	}
}
static float AStarHeuristic(void *fromNode, void *toNode, void *context)
{
	// Simple Euclidean
	Vec2i *v1 = fromNode;
	Vec2i *v2 = toNode;
	UNUSED(context);
	return (float)sqrt(DistanceSquared(
		Vec2iCenterOfTile(*v1), Vec2iCenterOfTile(*v2)));
}
