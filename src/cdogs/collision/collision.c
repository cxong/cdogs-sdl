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

    Copyright (c) 2013-2015, 2017-2018 Cong Xu
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
#include "campaigns.h"
#include "config.h"
#include "minkowski_hex.h"
#include "objs.h"


static void TileCacheInit(CArray *tc)
{
	CArrayInit(tc, sizeof(struct vec2i));
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
	CArray *tc, const struct vec2i v, const bool addAdjacents);
static void TileCacheAdd(CArray *tc, const struct vec2i v)
{
	TileCacheAddImpl(tc, v, true);
}
static void TileCacheAddImpl(
	CArray *tc, const struct vec2i v, const bool addAdjacents)
{
	if (!MapIsTileIn(&gMap, v))
	{
		return;
	}
	// Add tile in y/x order
	CA_FOREACH(const struct vec2i, t, *tc)
		if (t->y > v.y || (t->y == v.y && t->x > v.x))
		{
			CArrayInsert(tc, _ca_index, &v);
			break;
		}
		else if (svec2i_is_equal(*t, v))
		{
			// Don't add the same tile twice
			return;
		}
	CA_FOREACH_END()
	CArrayPushBack(tc, &v);

	// Also add the adjacencies for the tile
	if (addAdjacents)
	{
		struct vec2i dv;
		for (dv.y = -1; dv.y <= 1; dv.y++)
		{
			for (dv.x = -1; dv.x <= 1; dv.x++)
			{
				if (svec2i_is_zero(dv))
				{
					continue;
				}
				const struct vec2i dtv = svec2i_add(v, dv);
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

bool IsCollisionWithWall(const struct vec2 pos, const struct vec2i size2)
{
	const struct vec2i size = svec2i_scale_divide(size2, 2);
	if (pos.x - size.x < 0 ||
		pos.y - size.y < 0 ||
		pos.x + size.x >= gMap.Size.x * TILE_WIDTH ||
		pos.y + size.y >= gMap.Size.y * TILE_HEIGHT)
	{
		return true;
	}
	if (HitWall((int)pos.x - size.x,	(int)pos.y - size.y) ||
		HitWall((int)pos.x - size.x,	(int)pos.y) ||
		HitWall((int)pos.x - size.x,	(int)pos.y + size.y) ||
		HitWall((int)pos.x,				(int)pos.y + size.y) ||
		HitWall((int)pos.x + size.x,	(int)pos.y + size.y) ||
		HitWall((int)pos.x + size.x,	(int)pos.y) ||
		HitWall((int)pos.x + size.x,	(int)pos.y - size.y) ||
		HitWall((int)pos.x,				(int)pos.y - size.y))
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
bool IsCollisionDiamond(
	const Map *map, const struct vec2 pos, const struct vec2i size2)
{
	const struct vec2i mapSize =
		svec2i(map->Size.x * TILE_WIDTH, map->Size.y * TILE_HEIGHT);
	const struct vec2i size = svec2i_scale_divide(size2, 2);
	if (pos.x - size.x < 0 || pos.x + size.x >= mapSize.x ||
		pos.y - size.y < 0 || pos.y + size.y >= mapSize.y)
	{
		return true;
	}

	// Only support wider-than-taller collision diamonds for now
	CASSERT(size.x >= size.y, "not implemented, taller than wider diamond");
	const float gradient = (float)size.y / size.x;

	// Now we need to check in a diamond pattern that the boundary does not
	// collide
	// Top to right
	for (int i = 0; i < size.x; i++)
	{
		const float y = (-size.x + i) * gradient;
		const struct vec2 p = svec2_add(pos, svec2((float)i, y));
		if (HitWall(p.x, p.y))
		{
			return true;
		}
	}
	// Right to bottom
	for (int i = 0; i < size.x; i++)
	{
		const float y = i * gradient;
		const struct vec2 p = svec2_add(
			pos, svec2(size.x - (float)i, y));
		if (HitWall(p.x, p.y))
		{
			return true;
		}
	}
	// Bottom to left
	for (int i = 0; i < size.x; i++)
	{
		const float y = (size.x - i) * gradient;
		const struct vec2 p = svec2_add(pos, svec2((float)-i, y));
		if (HitWall(p.x, p.y))
		{
			return true;
		}
	}
	// Left to top
	for (int i = 0; i < size.x; i++)
	{
		const float y = -i * gradient;
		const struct vec2 p = svec2_add(
			pos, svec2((float)-size.x + i, y));
		if (HitWall(p.x, p.y))
		{
			return true;
		}
	}
	return false;
}

bool AABBOverlap(
	const struct vec2 pos1, const struct vec2 pos2,
	const struct vec2i size1, const struct vec2i size2)
{
	// Use Minkowski addition to check overlap of two rects
	const struct vec2 d = svec2(
		fabsf(pos1.x - pos2.x), fabsf(pos1.y - pos2.y));
	const struct vec2i r = svec2i_scale_divide(svec2i_add(size1, size2), 2);
	return d.x < r.x && d.y < r.y;
}

static bool CollisionIsOnSameTeam(
	const Thing *i, const CollisionTeam team, const bool isPVP)
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
	const CollisionParams params, const Thing *a, const Thing *b);

static void AddPosToTileCache(void *data, struct vec2i pos);
static bool CheckOverlaps(
	const Thing *item, const struct vec2 pos, const struct vec2 vel,
	const struct vec2i size,
	const CollisionParams params, CollideItemFunc func, void *data,
	CheckWallFunc checkWallFunc, CollideWallFunc wallFunc, void *wallData,
	const struct vec2i tilePos);
void OverlapThings(
	const Thing *item, const struct vec2 pos, const struct vec2i size,
	const CollisionParams params, CollideItemFunc func, void *data,
	CheckWallFunc checkWallFunc, CollideWallFunc wallFunc, void *wallData)
{
	TileCacheReset(&gCollisionSystem.tileCache);
	// Add all the tiles along the motion path
	AlgoLineDrawData drawData;
	drawData.Draw = AddPosToTileCache;
	drawData.data = &gCollisionSystem.tileCache;
	BresenhamLineDraw(
		svec2i_assign_vec2(pos), svec2i_assign_vec2(svec2_add(pos, item->Vel)), &drawData);

	// Check collisions with all tiles in the cache
	CA_FOREACH(const struct vec2i, dtv, gCollisionSystem.tileCache)
		if (!CheckOverlaps(
			item, pos, item->Vel, size, params, func, data,
			checkWallFunc, wallFunc, wallData,
			*dtv))
		{
			return;
		}
	CA_FOREACH_END()
}
static void AddPosToTileCache(void *data, struct vec2i pos)
{
	CArray *tileCache = data;
	const struct vec2i tv = Vec2iToTile(pos);
	TileCacheAdd(tileCache, tv);
}
static bool CheckOverlaps(
	const Thing *item, const struct vec2 pos, const struct vec2 vel,
	const struct vec2i size,
	const CollisionParams params, CollideItemFunc func, void *data,
	CheckWallFunc checkWallFunc, CollideWallFunc wallFunc, void *wallData,
	const struct vec2i tilePos)
{
	struct vec2 colA, colB, normal;
	// Check item collisions
	if (func != NULL)
	{
		const CArray *tileThings = &MapGetTile(&gMap, tilePos)->things;
		CA_FOREACH(const ThingId, tid, *tileThings)
			Thing *ti = ThingIdGetThing(tid);
			if (!CheckParams(params, item, ti))
			{
				continue;
			}
			if (!MinkowskiHexCollide(
				pos, vel, size, ti->Pos, ti->Vel,
				ti->size, &colA, &colB, &normal))
			{
				continue;
			}
			// Collision callback and check continue
			if (!func(ti, data, colA, colB, normal))
			{
				return false;
			}
		CA_FOREACH_END()
	}
	// Check wall collisions
	if (checkWallFunc != NULL && wallFunc != NULL && checkWallFunc(tilePos))
	{
		// Hack: bullets always considered 0x0 when colliding with walls
		// TODO: bullet size for walls
		const struct vec2i sizeForWall =
			item->kind == KIND_MOBILEOBJECT ? svec2i_zero() : size;
		if (MinkowskiHexCollide(
			pos, vel, sizeForWall,
			Vec2CenterOfTile(tilePos), svec2_zero(),
			TILE_SIZE, &colA, &colB, &normal) &&
			!wallFunc(tilePos, wallData, colA, normal))
		{
			return false;
		}
	}
	return true;
}

static bool OverlapGetFirstItemCallback(
	Thing *ti, void *data, const struct vec2 colA, const struct vec2 colB,
	const struct vec2 normal);
Thing *OverlapGetFirstItem(
	const Thing *item, const struct vec2 pos, const struct vec2i size,
	const CollisionParams params)
{
	Thing *firstItem = NULL;
	OverlapThings(
		item, pos, size, params, OverlapGetFirstItemCallback, &firstItem,
		NULL, NULL, NULL);
	return firstItem;
}
static bool OverlapGetFirstItemCallback(
	Thing *ti, void *data, const struct vec2 colA, const struct vec2 colB,
	const struct vec2 normal)
{
	UNUSED(colA);
	UNUSED(colB);
	UNUSED(normal);
	Thing **pFirstItem = data;
	// Store the first item in custom data and return
	*pFirstItem = ti;
	return false;
}

static bool CheckParams(
	const CollisionParams params, const Thing *a, const Thing *b)
{
	// Don't collide if items are on the same team
	if (CollisionIsOnSameTeam(b, params.Team, params.IsPVP))
	{
		return false;
	}
	// No same-item collision
	if (a == b) return false;
	if (params.ThingMask != 0 && !(b->flags & params.ThingMask))
	{
		return false;
	}

	return true;
}

void GetWallBouncePosVel(
	const struct vec2 pos, const struct vec2 vel, const struct vec2 colPos,
	const struct vec2 colNormal, struct vec2 *outPos, struct vec2 *outVel)
{
	const struct vec2 velBeforeCol = svec2_subtract(colPos, pos);
	const struct vec2 velAfterCol = svec2_subtract(vel, velBeforeCol);

	// If normal is zero, this means we were in collision from the start
	if (svec2_is_zero(colNormal))
	{
		// Reverse the vector
		*outPos = svec2_subtract(pos, vel);
		*outVel = svec2_scale(vel, -1);
		return;
	}

	// Reflect the out position by the collision normal about the collision pos
	const struct vec2 velReflected = svec2(
		colNormal.x == 0 ? velAfterCol.x : colNormal.x * fabsf(velAfterCol.x),
		colNormal.y == 0 ? velAfterCol.y : colNormal.y * fabsf(velAfterCol.y));
	*outPos = svec2_add(colPos, velReflected);

	// Out velocity follows the collision normal
	*outVel = svec2(
		colNormal.x == 0 ? vel.x : colNormal.x * fabsf(vel.x),
		colNormal.y == 0 ? vel.y : colNormal.y * fabsf(vel.y));
}
