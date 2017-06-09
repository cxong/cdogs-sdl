/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
	Copyright (c) 2014, Cong Xu
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
#pragma once

#include "AStar.h"
#include "c_array.h"
#include "map.h"
#include "vector.h"

// Ref-counted path reference
// Once refs reaches zero, can then free the path
typedef struct
{
	ASPath Path;
	int *refs;
	Vec2i from;
	Vec2i to;
} CachedPath;

typedef struct
{
	CArray paths;	// of CachedPath
	size_t head;
	Map *map;
} PathCache;

// Cache of A* paths so similar paths don't need to be recalculated
// Mainly to work around AI repeating the same pathfinds rapidly
// Note: lifetime managed by Map
extern PathCache gPathCache;

void CachedPathDestroy(CachedPath *c);

void PathCacheInit(PathCache *pc, Map *m);
void PathCacheTerminate(PathCache *pc);

// Clear all entries in cache
// This is done when the underlying map changes, changing paths
// e.g. keys
void PathCacheClear(PathCache *pc);

CachedPath PathCacheCreate(
	PathCache *pc, Vec2i from, Vec2i to,
	const bool ignoreObjects, const bool cache);