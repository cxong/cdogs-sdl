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

    Copyright (c) 2013, Cong Xu
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
#include "ai.h"

#include <assert.h>
#include <stdlib.h>

#include "collision.h"
#include "config.h"
#include "defs.h"
#include "actors.h"
#include "gamedata.h"
#include "mission.h"
#include "sys_specifics.h"
#include "utils.h"

static int gBaddieCount = 0;
static int gAreGoodGuysPresent = 0;


static int Facing(TActor * a, TActor * a2)
{
	switch (a->direction) {
	case DIRECTION_UP:
		return (a->y > a2->y);
	case DIRECTION_UPLEFT:
		return (a->y > a2->y && a->x > a2->x);
	case DIRECTION_LEFT:
		return a->x > a2->x;
	case DIRECTION_DOWNLEFT:
		return (a->y < a2->y && a->x > a2->x);
	case DIRECTION_DOWN:
		return a->y < a2->y;
	case DIRECTION_DOWNRIGHT:
		return (a->y < a2->y && a->x < a2->x);
	case DIRECTION_RIGHT:
		return a->x < a2->x;
	case DIRECTION_UPRIGHT:
		return (a->y > a2->y && a->x < a2->x);
	default:
		// should nver get here
		assert(0);
		break;
	}
	return NO;
}


static int FacingPlayer(TActor * actor)
{
	if (gPlayer1 && !gPlayer1->dead && Facing(actor, gPlayer1))
		return YES;
	if (gPlayer2 && !gPlayer2->dead && Facing(actor, gPlayer2))
		return YES;
	return NO;
}


#define Distance(a,b) CHEBYSHEV_DISTANCE(a->x, a->y, b->x, b->y)

/*
TActor *TrackOtherAICharacter( TActor *actor )
{
  TActor *a;

  a = ActorList();
  while (a)
  {
    if (a != actor && a->health > 0 &&
        Distance( a, actor) < 100 << 8)
    {
      if ((a->flags & (FLAGS_PLAYERS | FLAGS_GOOD_GUY)) != 0 &&
          (actor->flags & (FLAGS_PLAYERS | FLAGS_GOOD_GUY)) == 0)
        return a;
      if ((a->flags & (FLAGS_PLAYERS | FLAGS_GOOD_GUY)) == 0 &&
          (actor->flags & (FLAGS_PLAYERS | FLAGS_GOOD_GUY)) != 0)
        return a;
    }
    a = a->next;
  }
  return NULL;
}
*/

static void GetTargetCoords(TActor * actor, int *x, int *y)
{
	if (gPlayer1 && gPlayer2) {
		if (Distance(actor, gPlayer1) < Distance(actor, gPlayer2)) {
			*x = gPlayer1->x;
			*y = gPlayer1->y;
		} else {
			*x = gPlayer2->x;
			*y = gPlayer2->y;
		}
	} else if (gPlayer1) {
		*x = gPlayer1->x;
		*y = gPlayer1->y;
	} else if (gPlayer2) {
		*x = gPlayer2->x;
		*y = gPlayer2->y;
	} else {
		*x = actor->x;
		*y = actor->y;
	}
}

static int CloseToPlayer(TActor * actor)
{
	if (gPlayer1 && Distance(gPlayer1, actor) < 32 << 8)
		return 1;
	if (gPlayer2 && Distance(gPlayer2, actor) < 32 << 8)
		return 1;
	return 0;
}

static int Follow(TActor * actor)
{
	int cmd = 0;
	int x, y;

	GetTargetCoords(actor, &x, &y);
	x >>= 8;
	y >>= 8;

	if ((actor->x >> 8) < x - 1)
		cmd |= CMD_RIGHT;
	else if ((actor->x >> 8) > x + 1)
		cmd |= CMD_LEFT;

	if ((actor->y >> 8) < y - 1)
		cmd |= CMD_DOWN;
	else if ((actor->y >> 8) > y + 1)
		cmd |= CMD_UP;

	return cmd;
}


static int Hunt(TActor * actor)
{
	int cmd = 0;
	int x, y, dx, dy;

	x = y = 0;

//  TActor *a;

	if ((actor->flags & (FLAGS_PLAYERS | FLAGS_GOOD_GUY)) == 0)
		GetTargetCoords(actor, &x, &y);

/*
  if ((actor->flags & FLAGS_VISIBLE) != 0)
  {
    a = TrackOtherAICharacter( actor);
    if (a)
    {
      x = a->x;
      y = a->y;
    }
  }
*/

	dx = abs(x - actor->x);
	dy = abs(y - actor->y);

	if (2 * dx > dy) {
		if (actor->x < x)
			cmd |= CMD_RIGHT;
		else if (actor->x > x)
			cmd |= CMD_LEFT;
	}
	if (2 * dy > dx) {
		if (actor->y < y)
			cmd |= CMD_DOWN;
		else if (actor->y > y)
			cmd |= CMD_UP;
	}
	// If it's a coward, reverse directions...
	if ((actor->flags & FLAGS_RUNS_AWAY) != 0) {
		if ((cmd & (CMD_LEFT | CMD_RIGHT)) != 0)
			cmd ^= (CMD_LEFT | CMD_RIGHT);
		if ((cmd & (CMD_UP | CMD_DOWN)) != 0)
			cmd ^= (CMD_UP | CMD_DOWN);
	}

	return cmd;
}


static int PositionOK(TActor * actor, int x, int y)
{
	Vec2i realPos = Vec2iScaleDiv(Vec2iNew(x, y), 256);
	Vec2i size = Vec2iNew(actor->tileItem.w, actor->tileItem.h);
	if (IsCollisionWithWall(realPos, size))
	{
		return 0;
	}
	if (GetItemOnTileInCollision(
		&actor->tileItem, realPos, TILEITEM_IMPASSABLE))
	{
		return 0;
	}
	return 1;
}

#define STEPSIZE    1024

static int DirectionOK(TActor * actor, int dir)
{
	switch (dir) {
	case DIRECTION_UP:
		return PositionOK(actor, actor->x, actor->y - STEPSIZE);
	case DIRECTION_UPLEFT:
		return PositionOK(actor, actor->x - STEPSIZE,
				  actor->y - STEPSIZE)
		    || PositionOK(actor, actor->x - STEPSIZE, actor->y)
		    || PositionOK(actor, actor->x, actor->y - STEPSIZE);
	case DIRECTION_LEFT:
		return PositionOK(actor, actor->x - STEPSIZE, actor->y);
	case DIRECTION_DOWNLEFT:
		return PositionOK(actor, actor->x - STEPSIZE,
				  actor->y + STEPSIZE)
		    || PositionOK(actor, actor->x - STEPSIZE, actor->y)
		    || PositionOK(actor, actor->x, actor->y + STEPSIZE);
	case DIRECTION_DOWN:
		return PositionOK(actor, actor->x, actor->y + STEPSIZE);
	case DIRECTION_DOWNRIGHT:
		return PositionOK(actor, actor->x + STEPSIZE,
				  actor->y + STEPSIZE)
		    || PositionOK(actor, actor->x + STEPSIZE, actor->y)
		    || PositionOK(actor, actor->x, actor->y + STEPSIZE);
	case DIRECTION_RIGHT:
		return PositionOK(actor, actor->x + STEPSIZE, actor->y);
	case DIRECTION_UPRIGHT:
		return PositionOK(actor, actor->x + STEPSIZE,
				  actor->y - STEPSIZE)
		    || PositionOK(actor, actor->x + STEPSIZE, actor->y)
		    || PositionOK(actor, actor->x, actor->y - STEPSIZE);
	}
	return NO;
}


static int BrightWalk(TActor * actor, int roll)
{
	if ((actor->flags & FLAGS_VISIBLE) != 0 &&
	    roll < gCharacterDesc[actor->character].probabilityToTrack) {
		actor->flags &= ~FLAGS_DETOURING;
		return Hunt(actor);
	}

	if (actor->flags & FLAGS_TRYRIGHT) {
		if (DirectionOK(actor, (actor->direction + 7) % 8)) {
			actor->direction = (actor->direction + 7) % 8;
			actor->turns--;
			if (actor->turns == 0)
				actor->flags &= ~FLAGS_DETOURING;
		} else if (!DirectionOK(actor, actor->direction)) {
			actor->direction = (actor->direction + 1) % 8;
			actor->turns++;
			if (actor->turns == 4) {
				actor->flags &=
				    ~(FLAGS_DETOURING | FLAGS_TRYRIGHT);
				actor->turns = 0;
			}
		}
	} else {
		if (DirectionOK(actor, (actor->direction + 1) % 8)) {
			actor->direction = (actor->direction + 1) % 8;
			actor->turns--;
			if (actor->turns == 0)
				actor->flags &= ~FLAGS_DETOURING;
		} else if (!DirectionOK(actor, actor->direction)) {
			actor->direction = (actor->direction + 7) % 8;
			actor->turns++;
			if (actor->turns == 4) {
				actor->flags &=
				    ~(FLAGS_DETOURING | FLAGS_TRYRIGHT);
				actor->turns = 0;
			}
		}
	}
	return DirectionToCmd(actor->direction);
}

static int WillFire(TActor * actor, int roll)
{
	if ((actor->flags & FLAGS_VISIBLE) != 0 &&
		WeaponCanFire(&actor->weapon) &&
		roll < gCharacterDesc[actor->character].probabilityToShoot)
	{
		if ((actor->flags & FLAGS_GOOD_GUY) != 0)
			return 1;	//!FacingPlayer( actor);
		else if (gAreGoodGuysPresent)
		{
			return 1;
		}
		else
			return FacingPlayer(actor);
	}
	return 0;
}

void Detour(TActor * actor)
{
	actor->flags |= FLAGS_DETOURING;
	actor->turns = 1;
	if (actor->flags & FLAGS_TRYRIGHT)
		actor->direction =
		    (CmdToDirection(actor->lastCmd) + 1) % 8;
	else
		actor->direction =
		    (CmdToDirection(actor->lastCmd) + 7) % 8;
}

static int IsActorPositionValid(TActor *actor)
{
	Vec2i pos = Vec2iNew(actor->x, actor->y);
	actor->x = actor->y = 0;
	return MoveActor(actor, pos.x, pos.y);
}

static int TryPlaceBaddie(TActor *actor)
{
	int hasPlaced = 0;
	int i;
	for (i = 0; i < 100; i++)	// Don't try forever trying to place baddie
	{
		// Try spawning out of players' sights
		do
		{
			actor->x = (rand() % (XMAX * TILE_WIDTH)) << 8;
			actor->y = (rand() % (YMAX * TILE_HEIGHT)) << 8;
		}
		while ((gPlayer1 && Distance(actor, gPlayer1) < 256 * 150) ||
			(gPlayer2 && Distance(actor, gPlayer2) < 256 * 150));
		if (IsActorPositionValid(actor))
		{
			hasPlaced = 1;
			break;
		}
	}
	if (!hasPlaced)
	{
		// Keep trying, but this time try spawning anywhere, even close to player
		for (i = 0; i < 100; i++)	// Don't try forever trying to place baddie
		{
			actor->x = (rand() % (XMAX * TILE_WIDTH)) << 8;
			actor->y = (rand() % (YMAX * TILE_HEIGHT)) << 8;
			if (IsActorPositionValid(actor))
			{
				hasPlaced = 1;
				break;
			}
		}
	}

	actor->health = (actor->health * gConfig.Game.NonPlayerHP) / 100;
	if (actor->health <= 0)
	{
		actor->health = 1;
	}
	if (actor->flags & FLAGS_AWAKEALWAYS)
	{
		actor->flags &= ~FLAGS_SLEEPING;
	}
	else if (!(actor->flags & FLAGS_SLEEPALWAYS) &&
		rand() % 100 < gBaddieCount)
	{
		actor->flags &= ~FLAGS_SLEEPING;
	}

	return hasPlaced;
}

static void PlacePrisoner(TActor * actor)
{
	int x, y;

	do {
		do {
			actor->x = ((rand() % (XMAX * TILE_WIDTH)) << 8);
			actor->y = ((rand() % (YMAX * TILE_HEIGHT)) << 8);
		}
		while (!IsHighAccess(actor->x >> 8, actor->y >> 8));
		x = actor->x;
		y = actor->y;
		actor->x = actor->y = 0;
	}
	while (!MoveActor(actor, x, y));
}


void CommandBadGuys(int ticks)
{
	TActor *actor;
	int roll, cmd;
	int count = 0;
	int character;
	int bypass;
	int delayModifier;
	int rollLimit;

	switch (gConfig.Game.Difficulty)
	{
	case DIFFICULTY_VERYEASY:
		delayModifier = 4;
		rollLimit = 300;
		break;
	case DIFFICULTY_EASY:
		delayModifier = 2;
		rollLimit = 200;
		break;
	case DIFFICULTY_HARD:
		delayModifier = 1;
		rollLimit = 75;
		break;
	case DIFFICULTY_VERYHARD:
		delayModifier = 1;
		rollLimit = 50;
		break;
	default:
		delayModifier = 1;
		rollLimit = 100;
		break;
	}

	actor = ActorList();
	while (actor != NULL)
	{
		if ((actor->flags & (FLAGS_PLAYERS | FLAGS_PRISONER)) == 0)
		{
			if ((actor->flags & (FLAGS_VICTIM | FLAGS_GOOD_GUY)) != 0)
			{
				gAreGoodGuysPresent = 1;
			}

			count++;
			cmd = 0;
			if (!actor->dead &&
			    (actor->flags & FLAGS_SLEEPING) == 0) {
				bypass = 0;
				roll = rand() % rollLimit;
				if ((actor->flags & FLAGS_FOLLOWER) != 0) {
					if (CloseToPlayer(actor))
						cmd = 0;
					else
						cmd = Follow(actor);
					actor->delay =
					    gCharacterDesc[actor->
							  character].
					    actionDelay;
				} else if ((actor->flags & FLAGS_SNEAKY) !=
					   0
					   && (actor->
					       flags & FLAGS_VISIBLE) != 0
					   &&
					   ((gPlayer1
					     && (gPlayer1->
						 lastCmd & CMD_BUTTON1) !=
					     0) || (gPlayer2
						    && (gPlayer2->
							lastCmd &
							CMD_BUTTON1) !=
						    0))) {
					cmd = Hunt(actor) | CMD_BUTTON1;
					bypass = 1;
				}
				else if (actor->flags & FLAGS_DETOURING)
				{
					cmd = BrightWalk(actor, roll);
				}
				else if (actor->delay > 0)
				{
					actor->delay = MAX(0, actor->delay - ticks);
					cmd = actor->lastCmd & ~CMD_BUTTON1;
				}
				else
				{
					if (roll <
						gCharacterDesc[actor->character].probabilityToTrack)
					{
						cmd = Hunt(actor);
					}
					else if (roll <
						gCharacterDesc[actor->character].probabilityToMove)
					{
						cmd = DirectionToCmd(rand() & 7);
					}
					else
					{
						cmd = 0;
					}
					actor->delay =
						gCharacterDesc[actor->character].actionDelay *
						delayModifier;
				}
				if (!bypass)
				{
					if (WillFire(actor, roll))
					{
						cmd |= CMD_BUTTON1;
					}
					else
					{
						if ((actor->flags & FLAGS_VISIBLE) == 0)
						{
							// I think this is some hack to make sure invisible enemies don't fire so much
							actor->weapon.lock = 40;
						}
						if (cmd && !DirectionOK(actor, CmdToDirection(cmd)) &&
							(actor->flags & FLAGS_DETOURING) == 0)
						{
							Detour(actor);
							cmd = 0;
						}
					}
				}
			}
			CommandActor(actor, cmd, ticks);
			actor->flags &= ~FLAGS_VISIBLE;
		}
		else if ((actor->flags & FLAGS_PRISONER) != 0)
		{
			CommandActor(actor, 0, ticks);
		}

		actor = actor->next;
	}
	if (gMission.missionData->baddieCount > 0 &&
		gMission.missionData->baddieDensity > 0 &&
		count < MAX(1, (gMission.missionData->baddieDensity * gConfig.Game.EnemyDensity) / 100))
	{
		TActor *baddie;
		character =
		    CHARACTER_OTHERS +
		    rand() % gMission.missionData->baddieCount;
		character = MIN(character, CHARACTER_COUNT);
		baddie = AddActor(character);
		if (!TryPlaceBaddie(baddie))
		{
			RemoveActor(baddie);
		}
		else
		{
			gBaddieCount++;
		}
	}
}

void InitializeBadGuys(void)
{
	int i, j;
	int character;
	TActor *actor;

	if (gMission.missionData->specialCount > 0) {
		for (i = 0; i < gMission.missionData->objectiveCount; i++)
			if (gMission.missionData->objectives[i].type ==
			    OBJECTIVE_KILL) {
				for (j = 0;
				     j < gMission.objectives[i].count;
				     j++) {
					character =
						CHARACTER_OTHERS +
						gMission.missionData->baddieCount +
						rand() %
						gMission.missionData->specialCount;
					actor = AddActor(character);
					actor->tileItem.flags |= ObjectiveToTileItem(i);
					if (!TryPlaceBaddie(actor))
					{
						RemoveActor(actor);
					}
				}
			}
	}

	for (i = 0; i < gMission.missionData->objectiveCount; i++)
		if (gMission.missionData->objectives[i].type ==
		    OBJECTIVE_RESCUE) {
			if (!gPrisoner) {
				gMission.objectives[i].count = 1;
				gMission.objectives[i].required = 1;
				gPrisoner = AddActor(CHARACTER_PRISONER);
				gPrisoner->tileItem.flags |= ObjectiveToTileItem(i);
				if (HasLockedRooms())
				{
					PlacePrisoner(gPrisoner);
				}
				else
				{
					if (!TryPlaceBaddie(gPrisoner))
					{
						// Can't place prisoner when it's the objective
						// Fatal error, can't recover
						printf("Cannot place prisoner!\n");
						assert(0);
						exit(1);
					}
				}
			}
			else
			{
				// This is an error!
				gMission.objectives[i].count = 0;
				gMission.objectives[i].required = 0;
			}
		}

	gBaddieCount = gMission.index * 4;
	gAreGoodGuysPresent = 0;
}

void CreateEnemies(void)
{
	int i, character;

	if (gMission.missionData->baddieCount <= 0)
		return;

	for (i = 0;
		i < MAX(1, (gMission.missionData->baddieDensity * gConfig.Game.EnemyDensity) / 100);
		i++)
	{
		TActor *enemy;
		character = CHARACTER_OTHERS + rand() % gMission.missionData->baddieCount;
		character = MIN(character, CHARACTER_COUNT);
		enemy = AddActor(character);
		if (!TryPlaceBaddie(enemy))
		{
			RemoveActor(enemy);
		}
		else
		{
			gBaddieCount++;
		}
	}
}
