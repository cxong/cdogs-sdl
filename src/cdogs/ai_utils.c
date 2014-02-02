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

    Copyright (c) 2013-2014, Cong Xu
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
#include <math.h>

#include "algorithms.h"
#include "AStar.h"
#include "collision.h"
#include "map.h"
#include "objs.h"


TActor *AIGetClosestPlayer(Vec2i pos)
{
	int i;
	int minDistance = -1;
	TActor *closestPlayer = NULL;
	for (i = 0; i < gOptions.numPlayers; i++)
	{
		if (IsPlayerAlive(i))
		{
			TActor *p = gPlayers[i];
			Vec2i pPos = Vec2iFull2Real(Vec2iNew(p->x, p->y));
			int distance = CHEBYSHEV_DISTANCE(pos.x, pos.y, pPos.x, pPos.y);
			if (!closestPlayer || distance < minDistance)
			{
				closestPlayer = p;
				minDistance = distance;
			}
		}
	}
	return closestPlayer;
}

static TActor *AIGetClosestActor(Vec2i from, int (*compFunc)(TActor *))
{
	// Search all the actors and find the closest one that
	// satisfies the condition
	TActor *a;
	TActor *closest = NULL;
	int minDistance = -1;
	for (a = ActorList(); a; a = a->next)
	{
		int distance;
		// Never target invulnerables or civilians
		if (a->flags & (FLAGS_INVULNERABLE | FLAGS_PENALTY))
		{
			continue;
		}
		if (compFunc(a))
		{
			distance = CHEBYSHEV_DISTANCE(from.x, from.y, a->x, a->y);
			if (!closest || distance < minDistance)
			{
				minDistance = distance;
				closest = a;
			}
		}
	}
	return closest;
}

static int IsGood(TActor *a)
{
	return a->pData || (a->flags & FLAGS_GOOD_GUY);
}
static int IsBad(TActor *a)
{
	return !IsGood(a);
}
TActor *AIGetClosestEnemy(Vec2i from, int flags, int isPlayer)
{
	if (!isPlayer && !(flags & FLAGS_GOOD_GUY))
	{
		// we are bad; look for good guys
		return AIGetClosestActor(from, IsGood);
	}
	else
	{
		// we are good; look for bad guys
		return AIGetClosestActor(from, IsBad);
	}
}

static int IsGoodAndVisible(TActor *a)
{
	return IsGood(a) && (a->flags & FLAGS_VISIBLE);
}
static int IsBadAndVisible(TActor *a)
{
	return IsBad(a) && (a->flags & FLAGS_VISIBLE);
}
TActor *AIGetClosestVisibleEnemy(Vec2i from, int flags, int isPlayer)
{
	if (!isPlayer && !(flags & FLAGS_GOOD_GUY))
	{
		// we are bad; look for good guys
		return AIGetClosestActor(from, IsGoodAndVisible);
	}
	else
	{
		// we are good; look for bad guys
		return AIGetClosestActor(from, IsBadAndVisible);
	}
}

Vec2i AIGetClosestPlayerPos(Vec2i pos)
{
	TActor *closestPlayer = AIGetClosestPlayer(pos);
	if (closestPlayer)
	{
		return Vec2iFull2Real(Vec2iNew(closestPlayer->x, closestPlayer->y));
	}
	else
	{
		return pos;
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

static bool IsBlocked(void *data, Vec2i pos)
{
	return MapGetTile(data, pos)->flags & MAPTILE_IS_WALL;
}
int AIHasClearLine(Vec2i from, Vec2i to)
{
	// Find all tiles that overlap with the line (from, to)
	// Uses a modified version of Xiaolin Wu's algorithm
	HasClearLineData data;
	data.IsBlocked = IsBlocked;
	data.size = gMap.Size;
	data.tileSize = Vec2iNew(TILE_WIDTH, TILE_HEIGHT);
	data.data = &gMap;
	
	// Hack: since HasClearLine() is buggy, double-check with
	// Bresenham line algorithm
	return
		HasClearLine(from, to, &data) &&
		HasClearLineBresenham(from, to, &data);
}

TObject *AIGetObjectRunningInto(TActor *a, int cmd)
{
	// Check the position just in front of the character;
	// check if there's a (non-dangerous) object in front of it
	Vec2i frontPos = Vec2iFull2Real(Vec2iNew(a->x, a->y));
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
	item = GetItemOnTileInCollision(
		&a->tileItem,
		frontPos,
		TILEITEM_IMPASSABLE,
		CalcCollisionTeam(1, a),
		gCampaign.Entry.mode == CAMPAIGN_MODE_DOGFIGHT);
	if (!item || item->kind != KIND_OBJECT)
	{
		return NULL;
	}
	return item->data;
}


typedef struct
{
	Vec2i Goal;
	ASPath Path;
	int PathIndex;
	int IsFollowing;
} AIGotoContext;
typedef struct
{
	Map *Map;
} AStarContext;
static int IsWallOrLockedDoor(Map *map, Vec2i pos)
{
	int tileFlags = MapGetTile(map, pos)->flags;
	if (tileFlags & MAPTILE_IS_WALL)
	{
		return 1;
	}
	else if (tileFlags & MAPTILE_NO_WALK)
	{
		return !!(MapGetDoorKeycardFlag(map, pos) & ~gMission.flags);
	}
	return 0;
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
		if (y < 0 || y >= gMap.Size.y)
		{
			continue;
		}
		for (x = v->x - 1; x <= v->x + 1; x++)
		{
			float cost;
			Vec2i neighbor;
			neighbor.x = x;
			neighbor.y = y;
			if (x < 0 || x >= gMap.Size.x)
			{
				continue;
			}
			if (x == v->x && y == v->y)
			{
				continue;
			}
			if (IsWallOrLockedDoor(c->Map, Vec2iNew(x, y)))
			{
				continue;
			}
			// if we're moving diagonally,
			// need to check the axis-aligned neighbours are also clear
			if (IsWallOrLockedDoor(c->Map, Vec2iNew(v->x, y)) ||
				IsWallOrLockedDoor(c->Map, Vec2iNew(x, v->y)))
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
static ASPathNodeSource cPathNodeSource =
{
	sizeof(Vec2i), AddTileNeighbors, AStarHeuristic, NULL, NULL
};
static int AIGotoDirect(Vec2i a, Vec2i p)
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
	Vec2i *pathTile = ASPathGetNode(c->Path, c->PathIndex);
	c->IsFollowing = 1;
	// Check if we need to follow the next step in the path
	// Note: need to make sure the actor is fully within the current tile
	// otherwise it may get stuck at corners
	if (Vec2iEqual(currentTile, *pathTile) &&
		IsTileItemInsideTile(i, currentTile))
	{
		c->PathIndex++;
		pathTile = ASPathGetNode(c->Path, c->PathIndex);
		c->IsFollowing = 0;
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
		c->PathIndex >= (int)ASPathGetCount(c->Path) - 1) // at end of path
	{
		return 0;
	}
	// Check if we're too far from the current start of the path
	pathTile = ASPathGetNode(c->Path, c->PathIndex);
	if (CHEBYSHEV_DISTANCE(
		currentTile.x, currentTile.y, pathTile->x, pathTile->y) > 2)
	{
		return 0;
	}
	// Check if we're too far from the end of the path
	pathEnd = ASPathGetNode(c->Path, ASPathGetCount(c->Path) - 1);
	if (CHEBYSHEV_DISTANCE(
		goalTile.x, goalTile.y, pathEnd->x, pathEnd->y) > 0)
	{
		return 0;
	}
	return 1;
}
int AIGoto(TActor *actor, Vec2i p)
{
	Vec2i a = Vec2iFull2Real(Vec2iNew(actor->x, actor->y));
	Vec2i currentTile = Vec2iToTile(a);
	Vec2i goalTile = Vec2iToTile(p);
	AIGotoContext *c = actor->aiContext;

	// If we are currently following an A* path,
	// and it is still valid, keep following it until
	// we have reached a new tile
	if (c && c->IsFollowing && AStarCloseToPath(c, currentTile, goalTile))
	{
		return AStarFollow(c, currentTile, &actor->tileItem, a);
	}
	else if (AIHasClearLine(a, p))
	{
		// Simple case: if there's a clear line between AI and target,
		// walk straight towards it
		return AIGotoDirect(a, p);
	}
	else
	{
		// We need to recalculate A*
		AStarContext ac;
		ac.Map = &gMap;
		if (!c)
		{
			CCALLOC(actor->aiContext, sizeof(AIGotoContext));
			c = actor->aiContext;
		}
		c->Goal = goalTile;
		c->PathIndex = 1;	// start navigating to the next path node
		c->Path = ASPathCreate(
			&cPathNodeSource, &ac, &currentTile, &c->Goal);

		// In case we can't calculate A* for some reason,
		// try simple navigation again
		if (ASPathGetCount(c->Path) <= 1)
		{
			debug(
				D_MAX,
				"Error: can't calculate path from {%d, %d} to {%d, %d}",
				currentTile.x, currentTile.y,
				goalTile.x, goalTile.y);
			assert(0);
			return AIGotoDirect(a, p);
		}

		return AStarFollow(c, currentTile, &actor->tileItem, a);
	}
}

int AIHunt(TActor *actor)
{
	int cmd = 0;
	int dx, dy;
	Vec2i targetPos = Vec2iNew(actor->x, actor->y);
	if (!(actor->pData || (actor->flags & FLAGS_GOOD_GUY)))
	{
		targetPos = AIGetClosestPlayerPos(Vec2iNew(actor->x, actor->y));
	}

	if (actor->flags & FLAGS_VISIBLE)
	{
		TActor *a = AIGetClosestEnemy(
			Vec2iNew(actor->x, actor->y), actor->flags, !!actor->pData);
		if (a)
		{
			targetPos.x = a->x;
			targetPos.y = a->y;
		}
	}

	dx = abs(targetPos.x - actor->x);
	dy = abs(targetPos.y - actor->y);

	if (2 * dx > dy)
	{
		if (actor->x < targetPos.x)			cmd |= CMD_RIGHT;
		else if (actor->x > targetPos.x)	cmd |= CMD_LEFT;
	}
	if (2 * dy > dx)
	{
		if (actor->y < targetPos.y)			cmd |= CMD_DOWN;
		else if (actor->y > targetPos.y)	cmd |= CMD_UP;
	}
	// If it's a coward, reverse directions...
	if (actor->flags & FLAGS_RUNS_AWAY)
	{
		cmd = AIReverseDirection(cmd);
	}

	return cmd;
}
