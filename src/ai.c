/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (C) 1995 Ronny Wester
    Copyright (C) 2003 Jeremy Chin 
    Copyright (C) 2003 Lucas Martin-King 

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

-------------------------------------------------------------------------------

 ai.c - enemy and friend AI routines

*/


#include <stdlib.h>
#include "defs.h"
#include "actors.h"
#include "gamedata.h"
#include "mission.h"

#ifndef __typeof__ /* VC7 doesn't have this */
	#define __typeof__ typeof
#endif /* __typeof__ */

#define max(a,b) ({__typeof__(a) __a = (a); __typeof__(b) __b = (b); (__a > __b) ? __a : __b;})
#define min(a,b) ({__typeof__(a) __a = (a); __typeof__(b) __b = (b); (__a < __b) ? __a : __b;})

static int baddieCount = 0;
static int goodGuysPresent = 0;


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


#define Distance(a,b)  max( abs( a->x - b->x), abs( a->y - b->y))

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
	if (CheckWall(x, y, actor->tileItem.w, actor->tileItem.h))
		return NO;
	if (CheckTileItemCollision(&actor->tileItem, x >> 8, y >> 8,
				   TILEITEM_IMPASSABLE) != NULL)
		return NO;
	return YES;
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
	    roll < characterDesc[actor->character].probabilityToTrack) {
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
	    actor->gunLock <= 0 &&
	    roll < characterDesc[actor->character].probabilityToShoot) {
		if ((actor->flags & FLAGS_GOOD_GUY) != 0)
			return 1;	//!FacingPlayer( actor);
		else if (goodGuysPresent)
			return 1;
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

static void PlaceBaddie(TActor * actor)
{
	int x, y;

	actor->health = (actor->health * gOptions.npcHp) / 100;

	if (actor->health <= 0)
		actor->health = 1;

	do {
		do {
			actor->x = ((rand() % (XMAX * TILE_WIDTH)) << 8);
			actor->y = ((rand() % (YMAX * TILE_HEIGHT)) << 8);
		}
		while ((gPlayer1 && Distance(actor, gPlayer1) < 256 * 150)
		       || (gPlayer2
			   && Distance(actor, gPlayer2) < 256 * 150));
		x = actor->x;
		y = actor->y;
		actor->x = actor->y = 0;
	}
	while (!MoveActor(actor, x, y));

	if ((actor->flags & FLAGS_AWAKEALWAYS) != 0)
		actor->flags &= ~FLAGS_SLEEPING;
	else if ((actor->flags & FLAGS_SLEEPALWAYS) == 0 &&
		 rand() % 100 < baddieCount)
		actor->flags &= ~FLAGS_SLEEPING;
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


void CommandBadGuys(void)
{
	TActor *actor;
	int roll, cmd;
	int count = 0;
	int character;
	int bypass;
	int delayModifier;
	int rollLimit;

	switch (gOptions.difficulty) {
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
	while (actor) {
		if ((actor->flags & (FLAGS_PLAYERS | FLAGS_PRISONER)) == 0) {
			if ((actor->
			     flags & (FLAGS_VICTIM | FLAGS_GOOD_GUY)) != 0)
				goodGuysPresent = 1;

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
					    characterDesc[actor->
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
				} else if (actor->flags & FLAGS_DETOURING)
					cmd = BrightWalk(actor, roll);
				else if (actor->delay > 0) {
					actor->delay--;
					cmd =
					    (actor->
					     lastCmd & ~CMD_BUTTON1);
				} else {
					if (roll <
					    characterDesc[actor->
							  character].
					    probabilityToTrack)
						cmd = Hunt(actor);
					else if (roll <
						 characterDesc[actor->
							       character].
						 probabilityToMove)
						cmd =
						    DirectionToCmd(rand() &
								   7);
					else
						cmd = 0;
					actor->delay =
					    characterDesc[actor->
							  character].
					    actionDelay * delayModifier;
				}
				if (!bypass) {
					if (WillFire(actor, roll))
						cmd = CMD_BUTTON1;
					else {
						if ((actor->
						     flags & FLAGS_VISIBLE)
						    == 0)
							actor->gunLock =
							    40;
						if (cmd
						    && !DirectionOK(actor,
								    CmdToDirection
								    (cmd))
						    && (actor->
							flags &
							FLAGS_DETOURING) ==
						    0) {
							Detour(actor);
							cmd = 0;
						}
					}
				}
			}
			CommandActor(actor, cmd);
			actor->flags &= ~FLAGS_VISIBLE;
		} else if ((actor->flags & FLAGS_PRISONER) != 0)
			CommandActor(actor, 0);

		actor = actor->next;
	}
	if (gMission.missionData->baddieCount > 0 &&
	    gMission.missionData->baddieDensity > 0 &&
	    count < max(1,
			(gMission.missionData->baddieDensity *
			 gOptions.density) / 100)) {
		character =
		    CHARACTER_OTHERS +
		    rand() % gMission.missionData->baddieCount;
		character = min(character, CHARACTER_COUNT);
		PlaceBaddie(AddActor(character));
		baddieCount++;
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
					    gMission.missionData->
					    baddieCount +
					    rand() %
					    gMission.missionData->
					    specialCount;
					actor = AddActor(character);
					actor->tileItem.flags |=
					    ObjectiveToTileItem(i);
					PlaceBaddie(actor);
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
				gPrisoner->tileItem.flags |=
				    ObjectiveToTileItem(i);
				if (HasHighAccess())
					PlacePrisoner(gPrisoner);
				else
					PlaceBaddie(gPrisoner);
			} else {
				// This is an error!
				gMission.objectives[i].count = 0;
				gMission.objectives[i].required = 0;
			}
		}

	baddieCount = gMission.index * 4;
	goodGuysPresent = 0;
}

void CreateCharacters(void)
{
	int i, character;

	if (gMission.missionData->baddieCount <= 0)
		return;

	for (i = 0;
	     i < max(1,
		     (gMission.missionData->baddieDensity *
		      gOptions.density) / 100); i++) {
		character =
		    CHARACTER_OTHERS +
		    rand() % gMission.missionData->baddieCount;
		character = min(character, CHARACTER_COUNT);
		PlaceBaddie(AddActor(character));
		baddieCount++;
	}
}
