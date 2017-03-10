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
#pragma once

#include "actors.h"
#include "map.h"

typedef struct
{
	AllyCollision allyCollision;
} CollisionSystem;

extern CollisionSystem gCollisionSystem;

void CollisionSystemInit(CollisionSystem *cs);

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

bool IsCollisionWithWall(const Vec2i pos, const Vec2i fullSize);
// Check collision of an object with a diamond shape
bool IsCollisionDiamond(const Map *map, const Vec2i pos, const Vec2i fullSize);
bool CollisionIsOnSameTeam(
	const TTileItem *i, const CollisionTeam team, const bool isPVP);

// Get all TTileItem that collide with a target TTileItem, with callback.
// The callback returns bool continue, as multiple callbacks can result.
typedef bool (*CollideItemFunc)(TTileItem *, void *);
void CollideTileItems(
	const TTileItem *item, const Vec2i pos,
	const int mask, const CollisionTeam team, const bool isPVP,
	CollideItemFunc func, void *data);
// Get the first TTileItem in collision
TTileItem *CollideGetFirstItem(
	const TTileItem *item, const Vec2i pos,
	const int mask, const CollisionTeam team, const bool isPVP);
// Get the first TTileItem that overlaps an area
// This disregards original position
TTileItem *OverlapGetFirstItem(
	const TTileItem *item, const Vec2i pos, const Vec2i size,
	const int mask, const CollisionTeam team, const bool isPVP);

bool AreasCollide(
	const Vec2i pos1, const Vec2i pos2, const Vec2i size1, const Vec2i size2);

// Resolve wall bounces
Vec2i GetWallBounceFullPos(
	const Vec2i startFull, const Vec2i newFull, Vec2i *velFull);
