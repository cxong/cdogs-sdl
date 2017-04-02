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
#pragma once

#include "actors.h"
#include "map.h"

typedef struct
{
	AllyCollision allyCollision;
	// Cache of tiles to check for potential collisions, of tile coords
	CArray tileCache;	// of Vec2i
} CollisionSystem;

extern CollisionSystem gCollisionSystem;

void CollisionSystemInit(CollisionSystem *cs);
void CollisionSystemReset(CollisionSystem *cs);
void CollisionSystemTerminate(CollisionSystem *cs);

#define HitWall(x, y) (MapGetTile(&gMap, Vec2iNew((x)/TILE_WIDTH, (y)/TILE_HEIGHT))->flags & MAPTILE_NO_WALK)
#define ShootWall(x, y) (MapGetTile(&gMap, Vec2iNew((x)/TILE_WIDTH, (y)/TILE_HEIGHT))->flags & MAPTILE_NO_SHOOT)

// Which "team" the actor's on, for collision
// Actors on the same team don't have to collide
typedef enum
{
	COLLISIONTEAM_NONE,
	COLLISIONTEAM_GOOD,
	COLLISIONTEAM_BAD
} CollisionTeam;

CollisionTeam CalcCollisionTeam(const bool isActor, const TActor *actor);

// Parameters that determine whether two objects can collide
// TODO: replace with single layer mask
typedef struct
{
	int TileItemMask;
	CollisionTeam Team;
	bool IsPVP;
} CollisionParams;

bool IsCollisionWithWall(const Vec2i pos, const Vec2i fullSize);
// Check collision of an object with a diamond shape
bool IsCollisionDiamond(const Map *map, const Vec2i pos, const Vec2i fullSize);

// Get all TTileItem that overlap with a target TTileItem, with callback.
// The callback returns bool continue, as multiple callbacks can result.
typedef bool (*CollideItemFunc)(
	TTileItem *, void *, const Vec2i, const Vec2i, const Vec2i);
typedef bool (*CheckWallFunc)(const Vec2i);
typedef bool (*CollideWallFunc)(const Vec2i, void *, const Vec2i, const Vec2i);
void OverlapTileItems(
	const TTileItem *item, const Vec2i pos, const Vec2i size,
	const CollisionParams params, CollideItemFunc func, void *data,
	CheckWallFunc checkWallFunc, CollideWallFunc wallFunc, void *wallData);
// Get the first TTileItem that overlaps
TTileItem *OverlapGetFirstItem(
	const TTileItem *item, const Vec2i pos, const Vec2i size,
	const CollisionParams params);

bool AABBOverlap(
	const Vec2i pos1, const Vec2i pos2, const Vec2i size1, const Vec2i size2);

// Resolve wall bounces; all coordinates in full
void GetWallBouncePosVelFull(
	const Vec2i pos, const Vec2i vel, const Vec2i colPos,
	const Vec2i colNormal, Vec2i *outPos, Vec2i *outVel);
