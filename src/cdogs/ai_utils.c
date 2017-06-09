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
#include "ai_utils.h"

#include <assert.h>

#include "algorithms.h"
#include "collision/collision.h"
#include "gamedata.h"
#include "map.h"
#include "objs.h"
#include "path_cache.h"
#include "weapon.h"


TActor *AIGetClosestPlayer(Vec2i fullpos)
{
	int minDistance = -1;
	TActor *closestPlayer = NULL;
	CA_FOREACH(const PlayerData, pd, gPlayerDatas)
		if (!IsPlayerAlive(pd))
		{
			continue;
		}
		TActor *p = ActorGetByUID(pd->ActorUID);
		const int distance = CHEBYSHEV_DISTANCE(
			fullpos.x, fullpos.y, p->Pos.x, p->Pos.y);
		if (!closestPlayer || distance < minDistance)
		{
			closestPlayer = p;
			minDistance = distance;
		}
	CA_FOREACH_END()
	return closestPlayer;
}

static TActor *AIGetClosestActor(
	const Vec2i fromPos, const TActor *from,
	bool (*compFunc)(const TActor *, const TActor *))
{
	// Search all the actors and find the closest one that
	// satisfies the condition
	TActor *closest = NULL;
	int minDistance = -1;
	CA_FOREACH(TActor, a, gActors)
		if (!a->isInUse || a->dead)
		{
			continue;
		}
		// Never target invulnerables or civilians
		if (a->flags & (FLAGS_INVULNERABLE | FLAGS_PENALTY))
		{
			continue;
		}
		if (compFunc(a, from))
		{
			int distance = CHEBYSHEV_DISTANCE(
				fromPos.x, fromPos.y, a->Pos.x, a->Pos.y);
			if (!closest || distance < minDistance)
			{
				minDistance = distance;
				closest = a;
			}
		}
	CA_FOREACH_END()
	return closest;
}

static bool IsGood(const TActor *a, const TActor *b)
{
	UNUSED(b);
	return a->PlayerUID >= 0 || (a->flags & FLAGS_GOOD_GUY);
}
static bool IsBad(const TActor *a, const TActor *b)
{
	return !IsGood(a, b);
}
static bool IsDifferent(const TActor *a, const TActor *b)
{
	return a != b;
}
const TActor *AIGetClosestEnemy(
	const Vec2i from, const TActor *a, const int flags)
{
	if (IsPVP(gCampaign.Entry.Mode))
	{
		// free for all; look for anybody else
		return AIGetClosestActor(from, a, IsDifferent);
	}
	else if ((!a || a->PlayerUID < 0) && !(flags & FLAGS_GOOD_GUY))
	{
		// we are bad; look for good guys
		return AIGetClosestActor(from, a, IsGood);
	}
	else
	{
		// we are good; look for bad guys
		return AIGetClosestActor(from, a, IsBad);
	}
}

static bool IsGoodAndVisible(const TActor *a, const TActor *b)
{
	return IsGood(a, b) && (a->flags & FLAGS_VISIBLE);
}
static bool IsBadAndVisible(const TActor *a, const TActor *b)
{
	return IsBad(a, b) && (a->flags & FLAGS_VISIBLE);
}
const TActor *AIGetClosestVisibleEnemy(
	const TActor *from, const bool isPlayer)
{
	if (IsPVP(gCampaign.Entry.Mode))
	{
		// free for all; look for anybody
		return AIGetClosestActor(from->Pos, from, IsDifferent);
	}
	else if (!isPlayer && !(from->flags & FLAGS_GOOD_GUY))
	{
		// we are bad; look for good guys
		return AIGetClosestActor(from->Pos, from, IsGoodAndVisible);
	}
	else
	{
		// we are good; look for bad guys
		return AIGetClosestActor(from->Pos, from, IsBadAndVisible);
	}
}

Vec2i AIGetClosestPlayerPos(Vec2i pos)
{
	TActor *closestPlayer = AIGetClosestPlayer(pos);
	if (closestPlayer)
	{
		return Vec2iFull2Real(closestPlayer->Pos);
	}
	else
	{
		return Vec2iFull2Real(pos);
	}
}

int AIReverseDirection(int cmd)
{
	if (cmd & (CMD_LEFT | CMD_RIGHT))
	{
		cmd ^= CMD_LEFT | CMD_RIGHT;
	}
	if (cmd & (CMD_UP | CMD_DOWN))
	{
		cmd ^= CMD_UP | CMD_DOWN;
	}
	return cmd;
}

typedef bool (*IsBlockedFunc)(void *, Vec2i);
static bool AIHasClearLine(
	Vec2i from, Vec2i to, IsBlockedFunc isBlockedFunc);
static bool IsPosNoWalk(void *data, Vec2i pos);
static bool IsPosNoWalkAroundObjects(void *data, Vec2i pos);
bool AIHasClearPath(
	const Vec2i from, const Vec2i to, const bool ignoreObjects)
{
	IsBlockedFunc f = ignoreObjects ? IsPosNoWalk : IsPosNoWalkAroundObjects;
	return AIHasClearLine(from, to, f);
}
static bool AIHasClearLine(
	Vec2i from, Vec2i to, IsBlockedFunc isBlockedFunc)
{
	// Find all tiles that overlap with the line (from, to)
	// Uses Bresenham that crosses interiors of tiles
	HasClearLineData data;
	data.IsBlocked = isBlockedFunc;
	data.data = &gMap;

	return HasClearLineXiaolinWu(from, to, &data);
}
static bool IsTileWalkableOrOpenable(Map *map, Vec2i pos);
bool IsTileWalkable(Map *map, const Vec2i pos)
{
	if (!IsTileWalkableOrOpenable(map, pos))
	{
		return false;
	}
	// Check if tile has a dangerous (explosive) item on it
	// For AI, we don't want to shoot it, so just walk around
	Tile *t = MapGetTile(map, pos);
	CA_FOREACH(ThingId, tid, t->things)
		// Only look for explosive objects
		if (tid->Kind != KIND_OBJECT)
		{
			continue;
		}
		const TObject *o = CArrayGet(&gObjs, tid->Id);
		if (ObjIsDangerous(o))
		{
			return false;
		}
	CA_FOREACH_END()
	return true;
}
static bool IsPosNoWalk(void *data, Vec2i pos)
{
	return !IsTileWalkable(data, Vec2iToTile(pos));
}
bool IsTileWalkableAroundObjects(Map *map, const Vec2i pos)
{
	if (!IsTileWalkableOrOpenable(map, pos))
	{
		return false;
	}
	// Check if tile has any item on it
	Tile *t = MapGetTile(map, pos);
	CA_FOREACH(ThingId, tid, t->things)
		if (tid->Kind == KIND_OBJECT)
		{
			// Check that the object has hitbox - i.e. health > 0
			const TObject *o = CArrayGet(&gObjs, tid->Id);
			if (o->Health > 0)
			{
				return false;
			}
		}
		else if (tid->Kind == KIND_CHARACTER)
		{
			switch (gCollisionSystem.allyCollision)
			{
			case ALLYCOLLISION_NORMAL:
				return false;
			case ALLYCOLLISION_REPEL:
				// TODO: implement
				// Need to know collision team of player
				// to know if collision will result in repelling
				break;
			case ALLYCOLLISION_NONE:
				continue;
			default:
				CASSERT(false, "unknown collision type");
				break;
			}
		}
	CA_FOREACH_END()
	return true;
}
static bool IsPosNoWalkAroundObjects(void *data, Vec2i pos)
{
	return !IsTileWalkableAroundObjects(data, Vec2iToTile(pos));
}
static bool IsTileWalkableOrOpenable(Map *map, Vec2i pos)
{
	const Tile *tile = MapGetTile(map, pos);
	if (tile == NULL)
	{
		return false;
	}
	const int tileFlags = tile->flags;
	if (!(tileFlags & MAPTILE_NO_WALK))
	{
		return true;
	}
	if (tileFlags & MAPTILE_OFFSET_PIC)
	{
		// A door; check if we can open it
		int keycard = MapGetDoorKeycardFlag(map, pos);
		if (!keycard)
		{
			// Unlocked door
			return true;
		}
		return !!(keycard & gMission.KeyFlags);
	}
	// Otherwise, we cannot walk over this tile
	return false;
}
static bool IsPosNoSee(void *data, Vec2i pos);
bool AIHasClearShot(const Vec2i from, const Vec2i to)
{
	// Perform 4 line tests - above, below, left and right
	// This is to account for possible positions for the muzzle
	Vec2i fromOffset = from;

	const int pad = 2;
	fromOffset.x = from.x - (ACTOR_W + pad) / 2;
	if (Vec2iToTile(fromOffset).x >= 0 &&
		!AIHasClearLine(fromOffset, to, IsPosNoSee))
	{
		return false;
	}
	fromOffset.x = from.x + (ACTOR_W + pad) / 2;
	if (Vec2iToTile(fromOffset).x < gMap.Size.x &&
		!AIHasClearLine(fromOffset, to, IsPosNoSee))
	{
		return false;
	}
	fromOffset.x = from.x;
	fromOffset.y = from.y - (ACTOR_H + pad) / 2;
	if (Vec2iToTile(fromOffset).y >= 0 &&
		!AIHasClearLine(fromOffset, to, IsPosNoSee))
	{
		return false;
	}
	fromOffset.y = from.y + (ACTOR_H + pad) / 2;
	if (Vec2iToTile(fromOffset).y < gMap.Size.y &&
		!AIHasClearLine(fromOffset, to, IsPosNoSee))
	{
		return false;
	}
	return true;
}
static bool IsPosNoSee(void *data, Vec2i pos)
{
	return MapGetTile(data, Vec2iToTile(pos))->flags & MAPTILE_NO_SEE;
}

TObject *AIGetObjectRunningInto(TActor *a, int cmd)
{
	// Check the position just in front of the character;
	// check if there's a (non-dangerous) object in front of it
	Vec2i frontPos = Vec2iFull2Real(a->Pos);
	TTileItem *item;
	if (cmd & CMD_LEFT)
	{
		frontPos.x--;
	}
	else if (cmd & CMD_RIGHT)
	{
		frontPos.x++;
	}
	if (cmd & CMD_UP)
	{
		frontPos.y--;
	}
	else if (cmd & CMD_DOWN)
	{
		frontPos.y++;
	}
	const CollisionParams params =
	{
		TILEITEM_IMPASSABLE, CalcCollisionTeam(true, a),
		IsPVP(gCampaign.Entry.Mode)
	};
	item = OverlapGetFirstItem(
		&a->tileItem, Vec2iReal2Full(frontPos), a->tileItem.size, params);
	if (!item || item->kind != KIND_OBJECT)
	{
		return NULL;
	}
	return CArrayGet(&gObjs, item->id);
}

bool AIIsFacing(const TActor *a, const Vec2i targetFull, const direction_e d)
{
	const bool isUpperOrLowerOctants =
		abs(a->Pos.x - targetFull.x) < abs(a->Pos.y - targetFull.y);
	const bool isRight = a->Pos.x < targetFull.x;
	const bool isAbove = a->Pos.y > targetFull.y;
	switch (d)
	{
	case DIRECTION_UP:
		return isAbove && isUpperOrLowerOctants;
	case DIRECTION_UPLEFT:
		return !isRight && isAbove;
	case DIRECTION_LEFT:
		return !isRight && !isUpperOrLowerOctants;
	case DIRECTION_DOWNLEFT:
		return !isRight && !isAbove;
	case DIRECTION_DOWN:
		return !isAbove && isUpperOrLowerOctants;
	case DIRECTION_DOWNRIGHT:
		return isRight && !isAbove;
	case DIRECTION_RIGHT:
		return isRight && !isUpperOrLowerOctants;
	case DIRECTION_UPRIGHT:
		return isRight && isAbove;
	default:
		CASSERT(false, "invalid direction");
		break;
	}
	return false;
}


// Use pathfinding to check that there is a path between
// source and destination tiles
bool AIHasPath(const Vec2i from, const Vec2i to, const bool ignoreObjects)
{
	// Quick first test: check there is a clear path
	if (AIHasClearPath(from, to, ignoreObjects))
	{
		return true;
	}
	// Pathfind
	const Vec2i fromTile = Vec2iToTile(from);
	const Vec2i toTile = MapSearchTileAround(
		&gMap, Vec2iToTile(to),
		ignoreObjects ? IsTileWalkable : IsTileWalkableAroundObjects);
	CachedPath path = PathCacheCreate(
		&gPathCache, fromTile, toTile, ignoreObjects, true);
	const size_t pathCount = ASPathGetCount(path.Path);
	CachedPathDestroy(&path);
	return pathCount >= 1;
}

int AIGotoDirect(const Vec2i a, const Vec2i p)
{
	int cmd = 0;

	if (a.x < p.x)		cmd |= CMD_RIGHT;
	else if (a.x > p.x)	cmd |= CMD_LEFT;

	if (a.y < p.y)		cmd |= CMD_DOWN;
	else if (a.y > p.y)	cmd |= CMD_UP;

	return cmd;
}

// Follow the current A* path
static int AStarFollow(
	AIGotoContext *c, Vec2i currentTile, TTileItem *i, Vec2i a)
{
	Vec2i *pathTile = ASPathGetNode(c->Path.Path, c->PathIndex);
	c->IsFollowing = 1;
	// Check if we need to follow the next step in the path
	// Note: need to make sure the actor is fully within the current tile
	// otherwise it may get stuck at corners
	if (Vec2iEqual(currentTile, *pathTile) &&
		IsTileItemInsideTile(i, currentTile))
	{
		c->PathIndex++;
		pathTile = ASPathGetNode(c->Path.Path, c->PathIndex);
	}
	// Go directly to the center of the next tile
	return AIGotoDirect(a, Vec2iCenterOfTile(*pathTile));
}
// Check that we are still close to the start of the A* path,
// and the end of the path is close to our goal
static int AStarCloseToPath(
	AIGotoContext *c, Vec2i currentTile, Vec2i goalTile)
{
	Vec2i *pathTile;
	Vec2i *pathEnd;
	if (!c ||
		c->PathIndex >= (int)ASPathGetCount(c->Path.Path) - 1) // at end of path
	{
		return 0;
	}
	// Check if we're too far from the current start of the path
	pathTile = ASPathGetNode(c->Path.Path, c->PathIndex);
	if (CHEBYSHEV_DISTANCE(
		currentTile.x, currentTile.y, pathTile->x, pathTile->y) > 2)
	{
		return 0;
	}
	// Check if we're too far from the end of the path
	pathEnd = ASPathGetNode(c->Path.Path, ASPathGetCount(c->Path.Path) - 1);
	if (CHEBYSHEV_DISTANCE(
		goalTile.x, goalTile.y, pathEnd->x, pathEnd->y) > 0)
	{
		return 0;
	}
	return 1;
}
int AIGoto(TActor *actor, Vec2i p, bool ignoreObjects)
{
	Vec2i a = Vec2iFull2Real(actor->Pos);
	Vec2i currentTile = Vec2iToTile(a);
	Vec2i goalTile = Vec2iToTile(p);
	AIGotoContext *c = &actor->aiContext->Goto;

	CASSERT(c != NULL, "no AI context");

	// If we are already there, go directly to the goal
	// This can happen if AI is trying to track the player,
	// but the player has died, for example.
	if (Vec2iEqual(currentTile, goalTile))
	{
		return AIGotoDirect(a, p);
	}

	// If we are currently following an A* path,
	// and it is still valid, keep following it until
	// we have reached a new tile
	if (c && c->IsFollowing && AStarCloseToPath(c, currentTile, goalTile))
	{
		return AStarFollow(c, currentTile, &actor->tileItem, a);
	}
	else if (AIHasClearPath(a, p, ignoreObjects))
	{
		// Simple case: if there's a clear line between AI and target,
		// walk straight towards it
		return AIGotoDirect(a, p);
	}
	else
	{
		// We need to recalculate A*

		// First, if the goal tile is blocked itself,
		// find a nearby tile that can be walked to
		c->Goal = MapSearchTileAround(
			&gMap, goalTile,
			ignoreObjects ? IsTileWalkable : IsTileWalkableAroundObjects);

		c->PathIndex = 1;	// start navigating to the next path node
		CachedPathDestroy(&c->Path);
		c->Path = PathCacheCreate(
			&gPathCache, currentTile, c->Goal, ignoreObjects, true);

		// In case we can't calculate A* for some reason,
		// try simple navigation again
		if (ASPathGetCount(c->Path.Path) <= 1)
		{
			debug(
				D_MAX,
				"Error: can't calculate path from {%d, %d} to {%d, %d}",
				currentTile.x, currentTile.y,
				goalTile.x, goalTile.y);
			return AIGotoDirect(a, p);
		}

		return AStarFollow(c, currentTile, &actor->tileItem, a);
	}
}

// Hunt moves an Actor towards a target, using the most efficient direction.
// That is, given the following octant:
//            x  A      xxxx
//          x       xxxx
//        x     xxxx
//      x   xxxx      B
//    x  xxx
//  xxxxxxxxxxxxxxxxxxxxxxx
// Those in slice A will move down-left and those in slice B will move left.
int AIHunt(TActor *actor, Vec2i targetPos)
{
	const Vec2i fullPos = Vec2iAdd(actor->Pos, ActorGetGunMuzzleOffset(actor));
	const int dx = abs(targetPos.x - fullPos.x);
	const int dy = abs(targetPos.y - fullPos.y);

	int cmd = 0;
	if (2 * dx > dy)
	{
		if (fullPos.x < targetPos.x)		cmd |= CMD_RIGHT;
		else if (fullPos.x > targetPos.x)	cmd |= CMD_LEFT;
	}
	if (2 * dy > dx)
	{
		if (fullPos.y < targetPos.y)		cmd |= CMD_DOWN;
		else if (fullPos.y > targetPos.y)	cmd |= CMD_UP;
	}
	// If it's a coward, reverse directions...
	if (actor->flags & FLAGS_RUNS_AWAY)
	{
		cmd = AIReverseDirection(cmd);
	}

	return cmd;
}
int AIHuntClosest(TActor *actor)
{
	Vec2i targetPos = actor->Pos;
	if (!(actor->PlayerUID >= 0 || (actor->flags & FLAGS_GOOD_GUY)))
	{
		targetPos = AIGetClosestPlayerPos(actor->Pos);
	}

	if (actor->flags & FLAGS_VISIBLE)
	{
		const TActor *a =
			AIGetClosestVisibleEnemy(actor, actor->PlayerUID >= 0);
		if (a)
		{
			targetPos = a->Pos;
		}
	}
	return AIHunt(actor, targetPos);
}

// Move away from the target
// Usually used for a simple flee
int AIRetreatFrom(TActor *actor, const Vec2i from)
{
	return AIReverseDirection(AIHunt(actor, from));
}

// Track moves an Actor towards a target, but in such a fashion that the Actor
// will come into 8-axis alignment with the target soonest.
// That is, given the following octant:
//            x  A      xxxx
//          x       xxxx
//        x     xxxx
//      x   xxxx      B
//    x  xxx
//  xxxxxxxxxxxxxxxxxxxxxxx
// Those in slice A will move left and those in slice B will move down-left.
int AITrack(TActor *actor, const Vec2i targetPos)
{
	const Vec2i fullPos = Vec2iAdd(actor->Pos, ActorGetGunMuzzleOffset(actor));
	const int dx = abs(targetPos.x - fullPos.x);
	const int dy = abs(targetPos.y - fullPos.y);

	int cmd = 0;
	// Terminology: imagine the compass directions sliced into 16 equal parts,
	// and labelled in bearing/clock order. That is, the slices on the right
	// half are labelled 1-8.
	// In order to construct the movement, note that:
	// - If the target is to our left, we need to move left...
	// - Except if they are in slice 2 or 7
	// Slice 2 and 7 (and 10 and 15) can be found by (dy < 2dx && dy > dx)
	// - call this X-exception
	// Repeat this for all 4 cardinal directions.
	// Furthermore, give a bit of leeway for the 8 axes so we don't
	// fluctuate between perpendicular movement vectors.
	const bool xException = dy < 2 * dx && dy > (int)(dx * 1.1);
	if (!xException)
	{
		if (fullPos.x - targetPos.x < (int)(dy * 0.1))			cmd |= CMD_RIGHT;
		else if (fullPos.x - targetPos.x > -(int)(dy * 0.1))	cmd |= CMD_LEFT;
	}
	const bool yException = dx < 2 * dy && dx > (int)(dy * 1.1);
	if (!yException)
	{
		if (fullPos.y - targetPos.y < (int)(dx * 0.1))			cmd |= CMD_DOWN;
		else if (fullPos.y - targetPos.y > -(int)(dx * 0.1))	cmd |= CMD_UP;
	}

	return cmd;
}
