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

    Copyright (c) 2013-2015, 2017 Cong Xu
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
#include "collision.h"

#include "actors.h"
#include "algorithms.h"
#include "config.h"
#include "minkowski_hex.h"

#define TILE_CACHE_TILE 1
#define TILE_CACHE_ADJACENT 2


static void TileCacheInit(CArray *tc)
{
	CArrayInit(tc, sizeof(Vec2i));
}
static void TileCacheReset(CArray *tc)
{
	CArrayClear(tc);
}
static void TileCacheTerminate(CArray *tc)
{
	CArrayTerminate(tc);
}
static void TileCacheAddImpl(
	CArray *tc, const Vec2i v, const bool addAdjacents);
static void TileCacheAdd(CArray *tc, const Vec2i v)
{
	TileCacheAddImpl(tc, v, true);
}
static void TileCacheAddImpl(
	CArray *tc, const Vec2i v, const bool addAdjacents)
{
	if (!MapIsTileIn(&gMap, v))
	{
		return;
	}
	// Add tile in y/x order
	CA_FOREACH(const Vec2i, t, *tc)
		if (t->y > v.y || (t->y == v.y && t->x > v.x))
		{
			CArrayInsert(tc, _ca_index, &v);
			break;
		}
		else if (Vec2iEqual(*t, v))
		{
			// Don't add the same tile twice
			return;
		}
	CA_FOREACH_END()
	CArrayPushBack(tc, &v);

	// Also add the adjacencies for the tile
	if (addAdjacents)
	{
		Vec2i dv;
		for (dv.y = -1; dv.y <= 1; dv.y++)
		{
			for (dv.x = -1; dv.x <= 1; dv.x++)
			{
				if (Vec2iIsZero(dv))
				{
					continue;
				}
				const Vec2i dtv = Vec2iAdd(v, dv);
				if (!MapIsTileIn(&gMap, dtv))
				{
					continue;
				}
				TileCacheAddImpl(tc, dtv, false);
			}
		}
	}
}


CollisionSystem gCollisionSystem;

void CollisionSystemInit(CollisionSystem *cs)
{
	CollisionSystemReset(cs);
	TileCacheInit(&cs->tileCache);
}
void CollisionSystemReset(CollisionSystem *cs)
{
	cs->allyCollision = ConfigGetEnum(&gConfig, "Game.AllyCollision");
}
void CollisionSystemTerminate(CollisionSystem *cs)
{
	TileCacheTerminate(&cs->tileCache);
}

CollisionTeam CalcCollisionTeam(const bool isActor, const TActor *actor)
{
	// Need to have prisoners collide with everything otherwise they will not
	// be "rescued"
	// Also need victims to collide with everyone
	if (!isActor || (actor->flags & (FLAGS_PRISONER | FLAGS_VICTIM)) ||
		IsPVP(gCampaign.Entry.Mode))
	{
		return COLLISIONTEAM_NONE;
	}
	if (actor->PlayerUID >= 0 || (actor->flags & FLAGS_GOOD_GUY))
	{
		return COLLISIONTEAM_GOOD;
	}
	return COLLISIONTEAM_BAD;
}

bool IsCollisionWithWall(const Vec2i pos, const Vec2i fullSize)
{
	Vec2i size = Vec2iScaleDiv(fullSize, 2);
	if (pos.x - size.x < 0 ||
		pos.y - size.y < 0 ||
		pos.x + size.x >= gMap.Size.x * TILE_WIDTH ||
		pos.y + size.y >= gMap.Size.y * TILE_HEIGHT)
	{
		return true;
	}
	if (HitWall(pos.x - size.x,	pos.y - size.y) ||
		HitWall(pos.x - size.x,	pos.y) ||
		HitWall(pos.x - size.x,	pos.y + size.y) ||
		HitWall(pos.x,			pos.y + size.y) ||
		HitWall(pos.x + size.x,	pos.y + size.y) ||
		HitWall(pos.x + size.x,	pos.y) ||
		HitWall(pos.x + size.x,	pos.y - size.y) ||
		HitWall(pos.x,			pos.y - size.y))
	{
		return true;
	}
	return false;
}

// Check collision with a diamond shape
// This means that the bounding box could be in collision, but the bounding
// "radius" is not. The diamond is expressed with a single "radius" - that is,
// the diamond is the same height and width.
// This arrangement is used so that axis movement can slide off corners by
// moving in a diagonal direction.
// E.g. this is not a collision:
//       x
//     x   x
//   x       x
// x           x
//   x       x
//     x   x wwwww
//       x   w
//           w
// Where 'x' denotes the bounding diamond, and 'w' represents a wall corner.
bool IsCollisionDiamond(const Map *map, const Vec2i pos, const Vec2i fullSize)
{
	const Vec2i mapSize =
		Vec2iNew(map->Size.x * TILE_WIDTH, map->Size.y * TILE_HEIGHT);
	const Vec2i size = Vec2iScaleDiv(fullSize, 2);
	if (pos.x - size.x < 0 || pos.x + size.x >= mapSize.x ||
		pos.y - size.y < 0 || pos.y + size.y >= mapSize.y)
	{
		return true;
	}

	// Only support wider-than-taller collision diamonds for now
	CASSERT(size.x >= size.y, "not implemented, taller than wider diamond");
	const double gradient = (double)size.y / size.x;

	// Now we need to check in a diamond pattern that the boundary does not
	// collide
	// Top to right
	for (int i = 0; i < size.x; i++)
	{
		const int y = (int)Round((-size.x + i)* gradient);
		const Vec2i p = Vec2iAdd(pos, Vec2iNew(i, y));
		if (HitWall(p.x, p.y))
		{
			return true;
		}
	}
	// Right to bottom
	for (int i = 0; i < size.x; i++)
	{
		const int y = (int)Round(i * gradient);
		const Vec2i p = Vec2iAdd(pos, Vec2iNew(size.x - i, y));
		if (HitWall(p.x, p.y))
		{
			return true;
		}
	}
	// Bottom to left
	for (int i = 0; i < size.x; i++)
	{
		const int y = (int)Round((size.x - i) * gradient);
		const Vec2i p = Vec2iAdd(pos, Vec2iNew(-i, y));
		if (HitWall(p.x, p.y))
		{
			return true;
		}
	}
	// Left to top
	for (int i = 0; i < size.x; i++)
	{
		const int y = (int)Round(-i * gradient);
		const Vec2i p = Vec2iAdd(pos, Vec2iNew(-size.x + i, y));
		if (HitWall(p.x, p.y))
		{
			return true;
		}
	}
	return false;
}

bool AABBOverlap(
	const Vec2i pos1, const Vec2i pos2, const Vec2i size1, const Vec2i size2)
{
	// Use Minkowski addition to check overlap of two rects
	const Vec2i d = Vec2iNew(abs(pos1.x - pos2.x), abs(pos1.y - pos2.y));
	const Vec2i r = Vec2iScaleDiv(Vec2iAdd(size1, size2), 2);
	return d.x < r.x && d.y < r.y;
}

static bool CollisionIsOnSameTeam(
	const TTileItem *i, const CollisionTeam team, const bool isPVP)
{
	if (gCollisionSystem.allyCollision == ALLYCOLLISION_NORMAL)
	{
		return false;
	}
	CollisionTeam itemTeam = COLLISIONTEAM_NONE;
	if (i->kind == KIND_CHARACTER)
	{
		const TActor *a = CArrayGet(&gActors, i->id);
		itemTeam = CalcCollisionTeam(1, a);
	}
	return
		team != COLLISIONTEAM_NONE &&
		itemTeam != COLLISIONTEAM_NONE &&
		team == itemTeam &&
		!isPVP;
}

static bool CheckParams(
	const CollisionParams params, const TTileItem *a, const TTileItem *b);

static void AddPosToTileCache(void *data, Vec2i pos);
static bool CheckOverlaps(
	const TTileItem *item, const Vec2i pos, const Vec2i vel, const Vec2i size,
	const CollisionParams params, CollideItemFunc func, void *data,
	const CArray *tileThings);
void OverlapTileItems(
	const TTileItem *item, const Vec2i pos, const Vec2i size,
	const CollisionParams params, CollideItemFunc func, void *data)
{
	TileCacheReset(&gCollisionSystem.tileCache);
	// Add all the tiles along the motion path
	AlgoLineDrawData drawData;
	drawData.Draw = AddPosToTileCache;
	drawData.data = &gCollisionSystem.tileCache;
	const Vec2i vel = Vec2iFull2Real(item->VelFull);
	BresenhamLineDraw(pos, Vec2iAdd(pos, vel), &drawData);

	// Check collisions with all tiles in the cache
	CA_FOREACH(const Vec2i, dtv, gCollisionSystem.tileCache)
		const CArray *tileThings = &MapGetTile(&gMap, *dtv)->things;
		if (!CheckOverlaps(
			item, pos, vel, size, params, func, data, tileThings))
		{
			return;
		}
	CA_FOREACH_END()
}
static void AddPosToTileCache(void *data, Vec2i pos)
{
	CArray *tileCache = data;
	const Vec2i tv = Vec2iToTile(pos);
	TileCacheAdd(tileCache, tv);
}
static bool CheckOverlaps(
	const TTileItem *item, const Vec2i pos, const Vec2i vel, const Vec2i size,
	const CollisionParams params, CollideItemFunc func, void *data,
	const CArray *tileThings)
{
	CA_FOREACH(const ThingId, tid, *tileThings)
		TTileItem *ti = ThingIdGetTileItem(tid);
		if (!CheckParams(params, item, ti))
		{
			continue;
		}
		Vec2i collideA, collideB;
		if (!MinkowskiHexCollide(
			pos, vel, size,
			Vec2iNew(ti->x, ti->y), Vec2iFull2Real(ti->VelFull), ti->size,
			&collideA, &collideB))
		{
			continue;
		}
		// Collision callback and check continue
		if (!func(ti, data, collideA, collideB))
		{
			return false;
		}
	CA_FOREACH_END()
	return true;
}

static bool OverlapGetFirstItemCallback(
	TTileItem *ti, void *data, const Vec2i collideA, const Vec2i collideB);
TTileItem *OverlapGetFirstItem(
	const TTileItem *item, const Vec2i pos, const Vec2i size,
	const CollisionParams params)
{
	TTileItem *firstItem = NULL;
	OverlapTileItems(
		item, pos, size, params, OverlapGetFirstItemCallback, &firstItem);
	return firstItem;
}
static bool OverlapGetFirstItemCallback(
	TTileItem *ti, void *data, const Vec2i collideA, const Vec2i collideB)
{
	UNUSED(collideA);
	UNUSED(collideB);
	TTileItem **pFirstItem = data;
	// Store the first item in custom data and return
	*pFirstItem = ti;
	return false;
}

static bool CheckParams(
	const CollisionParams params, const TTileItem *a, const TTileItem *b)
{
	// Don't collide if items are on the same team
	if (CollisionIsOnSameTeam(b, params.Team, params.IsPVP))
	{
		return false;
	}
	// No same-item collision
	if (a == b) return false;
	if (params.TileItemMask != 0 && !(b->flags & params.TileItemMask))
	{
		return false;
	}

	return true;
}

Vec2i GetWallBounceFullPos(
	const Vec2i startFull, const Vec2i newFull, Vec2i *velFull)
{
	CASSERT(velFull != NULL, "need velocity for wall bouncing");
	Vec2i newReal = Vec2iFull2Real(newFull);
	if (!ShootWall(newReal.x, newReal.y))
	{
		return newFull;
	}
	Vec2i startRealPos = Vec2iFull2Real(startFull);
	Vec2i bounceFull = startFull;
	if (!ShootWall(startRealPos.x, newReal.y))
	{
		bounceFull.y = newFull.y;
		velFull->x *= -1;
	}
	else if (!ShootWall(newReal.x, startRealPos.y))
	{
		bounceFull.x = newFull.x;
		velFull->y *= -1;
	}
	else
	{
		*velFull = Vec2iScale(*velFull, -1);
		// Keep bouncing back if it's inside a wall
		// However, do not bounce more than half a tile's size
		if (!Vec2iIsZero(*velFull))
		{
			Vec2i bounceReal = newReal;
			const int maxBounces = MAX(
				velFull->x / 256 / TILE_WIDTH,
				velFull->y / 256 / TILE_HEIGHT);
			for (int i = 0;
				i < maxBounces &&
				MapIsRealPosIn(&gMap, bounceReal) &&
				ShootWall(bounceReal.x, bounceReal.y);
				i++)
			{
				bounceFull = Vec2iAdd(bounceFull, *velFull);
				bounceReal = Vec2iFull2Real(bounceFull);
			}
			// If still colliding wall or outside map,
			// can't recover from this point; zero velocity and return
			if (!MapIsRealPosIn(&gMap, bounceReal) ||
				ShootWall(bounceReal.x, bounceReal.y))
			{
				*velFull = Vec2iZero();
				return startFull;
			}
		}
	}
	return bounceFull;
}
