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
#include "ai.h"

#include <assert.h>
#include <stdlib.h>

#include "ai_utils.h"
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


static int IsFacing(TActor *a, TActor *a2, direction_e d)
{
	switch (d)
	{
	case DIRECTION_UP:
		return (a->Pos.y > a2->Pos.y);
	case DIRECTION_UPLEFT:
		return (a->Pos.y > a2->Pos.y && a->Pos.x > a2->Pos.x);
	case DIRECTION_LEFT:
		return a->Pos.x > a2->Pos.x;
	case DIRECTION_DOWNLEFT:
		return (a->Pos.y < a2->Pos.y && a->Pos.x > a2->Pos.x);
	case DIRECTION_DOWN:
		return a->Pos.y < a2->Pos.y;
	case DIRECTION_DOWNRIGHT:
		return (a->Pos.y < a2->Pos.y && a->Pos.x < a2->Pos.x);
	case DIRECTION_RIGHT:
		return a->Pos.x < a2->Pos.x;
	case DIRECTION_UPRIGHT:
		return (a->Pos.y > a2->Pos.y && a->Pos.x < a2->Pos.x);
	default:
		// should nver get here
		assert(0);
		break;
	}
	return 0;
}


static int IsFacingPlayer(TActor *actor, direction_e d)
{
	int i;
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		if (IsPlayerAlive(i) &&
			IsFacing(actor, CArrayGet(&gActors, gPlayerIds[i]), d))
		{
			return 1;
		}
	}
	return 0;
}


#define Distance(a,b) CHEBYSHEV_DISTANCE(a->x, a->y, b->x, b->y)

static int IsCloseToPlayer(Vec2i pos)
{
	TActor *closestPlayer = AIGetClosestPlayer(pos);
	if (closestPlayer && CHEBYSHEV_DISTANCE(
		pos.x, pos.y, closestPlayer->Pos.x, closestPlayer->Pos.y) < (32 << 8))
	{
		return 1;
	}
	return 0;
}


static bool IsPosOK(TActor *actor, Vec2i pos)
{
	Vec2i realPos = Vec2iFull2Real(pos);
	Vec2i size = Vec2iNew(actor->tileItem.w, actor->tileItem.h);
	if (IsCollisionWallOrEdge(&gMap, realPos, size))
	{
		return 0;
	}
	if (GetItemOnTileInCollision(
		&actor->tileItem, realPos, TILEITEM_IMPASSABLE,
		CalcCollisionTeam(1, actor),
		gCampaign.Entry.mode == CAMPAIGN_MODE_DOGFIGHT))
	{
		return 0;
	}
	return 1;
}

#define STEPSIZE    1024

static bool IsDirectionOK(TActor *a, int dir)
{
	switch (dir) {
	case DIRECTION_UP:
		return IsPosOK(a, Vec2iAdd(a->Pos, Vec2iNew(0, -STEPSIZE)));
	case DIRECTION_UPLEFT:
		return
			IsPosOK(a, Vec2iAdd(a->Pos, Vec2iNew(-STEPSIZE, -STEPSIZE))) ||
			IsPosOK(a, Vec2iAdd(a->Pos, Vec2iNew(-STEPSIZE, 0))) ||
			IsPosOK(a, Vec2iAdd(a->Pos, Vec2iNew(0, -STEPSIZE)));
	case DIRECTION_LEFT:
		return IsPosOK(a, Vec2iAdd(a->Pos, Vec2iNew(-STEPSIZE, 0)));
	case DIRECTION_DOWNLEFT:
		return
			IsPosOK(a, Vec2iAdd(a->Pos, Vec2iNew(-STEPSIZE, STEPSIZE))) ||
			IsPosOK(a, Vec2iAdd(a->Pos, Vec2iNew(-STEPSIZE, 0))) ||
			IsPosOK(a, Vec2iAdd(a->Pos, Vec2iNew(0, STEPSIZE)));
	case DIRECTION_DOWN:
		return IsPosOK(a, Vec2iAdd(a->Pos, Vec2iNew(0, STEPSIZE)));
	case DIRECTION_DOWNRIGHT:
		return
			IsPosOK(a, Vec2iAdd(a->Pos, Vec2iNew(STEPSIZE, STEPSIZE))) ||
			IsPosOK(a, Vec2iAdd(a->Pos, Vec2iNew(STEPSIZE, 0))) ||
			IsPosOK(a, Vec2iAdd(a->Pos, Vec2iNew(0, STEPSIZE)));
	case DIRECTION_RIGHT:
		return IsPosOK(a, Vec2iAdd(a->Pos, Vec2iNew(STEPSIZE, 0)));
	case DIRECTION_UPRIGHT:
		return
			IsPosOK(a, Vec2iAdd(a->Pos, Vec2iNew(STEPSIZE, -STEPSIZE))) ||
			IsPosOK(a, Vec2iAdd(a->Pos, Vec2iNew(STEPSIZE, 0))) ||
			IsPosOK(a, Vec2iAdd(a->Pos, Vec2iNew(0, -STEPSIZE)));
	}
	return 0;
}


static int BrightWalk(TActor * actor, int roll)
{
	if (!!(actor->flags & FLAGS_VISIBLE) &&
		roll < actor->character->bot.probabilityToTrack)
	{
		actor->flags &= ~FLAGS_DETOURING;
		return AIHunt(actor);
	}

	if (actor->flags & FLAGS_TRYRIGHT)
	{
		if (IsDirectionOK(actor, (actor->direction + 7) % 8))
		{
			actor->direction = (actor->direction + 7) % 8;
			actor->turns--;
			if (actor->turns == 0)
			{
				actor->flags &= ~FLAGS_DETOURING;
			}
		}
		else if (!IsDirectionOK(actor, actor->direction))
		{
			actor->direction = (actor->direction + 1) % 8;
			actor->turns++;
			if (actor->turns == 4) {
				actor->flags &=
				    ~(FLAGS_DETOURING | FLAGS_TRYRIGHT);
				actor->turns = 0;
			}
		}
	}
	else
	{
		if (IsDirectionOK(actor, (actor->direction + 1) % 8))
		{
			actor->direction = (actor->direction + 1) % 8;
			actor->turns--;
			if (actor->turns == 0)
				actor->flags &= ~FLAGS_DETOURING;
		}
		else if (!IsDirectionOK(actor, actor->direction))
		{
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
		roll < actor->character->bot.probabilityToShoot)
	{
		if ((actor->flags & FLAGS_GOOD_GUY) != 0)
			return 1;	//!FacingPlayer( actor);
		else if (gAreGoodGuysPresent)
		{
			return 1;
		}
		else
		{
			return IsFacingPlayer(actor, actor->direction);
		}
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

static bool IsActorPositionValid(TActor *actor)
{
	Vec2i pos = actor->Pos;
	actor->Pos = Vec2iZero();
	return TryMoveActor(actor, pos);
}

static void PlaceBaddie(TActor *actor)
{
	int hasPlaced = 0;
	int i;
	for (i = 0; i < 100; i++)	// Don't try forever trying to place baddie
	{
		// Try spawning out of players' sights
		actor->Pos.x = (rand() % (gMap.Size.x * TILE_WIDTH)) << 8;
		actor->Pos.y = (rand() % (gMap.Size.y * TILE_HEIGHT)) << 8;
		TActor *closestPlayer = AIGetClosestPlayer(actor->Pos);
		if (closestPlayer && CHEBYSHEV_DISTANCE(
			actor->Pos.x, actor->Pos.y,
			closestPlayer->Pos.x, closestPlayer->Pos.y) >= 256 * 150 &&
			IsActorPositionValid(actor))
		{
			hasPlaced = 1;
			break;
		}
	}
	// Keep trying, but this time try spawning anywhere, even close to player
	while (!hasPlaced)
	{
		actor->Pos.x = (rand() % (gMap.Size.x * TILE_WIDTH)) << 8;
		actor->Pos.y = (rand() % (gMap.Size.y * TILE_HEIGHT)) << 8;
		if (IsActorPositionValid(actor))
		{
			hasPlaced = 1;
			break;
		}
	}

	ActorInit(actor);
	if (!(actor->flags & FLAGS_SLEEPALWAYS) &&
		rand() % 100 < gBaddieCount)
	{
		actor->flags &= ~FLAGS_SLEEPING;
	}
}

static void PlacePrisoner(TActor *actor)
{
	do
	{
		do
		{
			actor->Pos.x = ((rand() % (gMap.Size.x * TILE_WIDTH)) << 8);
			actor->Pos.y = ((rand() % (gMap.Size.y * TILE_HEIGHT)) << 8);
		}
		while (!MapPosIsHighAccess(
			&gMap, actor->Pos.x >> 8, actor->Pos.y >> 8));
	}
	while (!IsActorPositionValid(actor));
}


static bool DidPlayerShoot(void)
{
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		if (IsPlayerAlive(i))
		{
			TActor *player = CArrayGet(&gActors, gPlayerIds[i]);
			return player->lastCmd & CMD_BUTTON1;
		}
	}
	return 0;
}

void CommandBadGuys(int ticks)
{
	int roll, cmd;
	int count = 0;
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

	for (int i = 0; i < (int)gActors.size; i++)
	{
		TActor *actor = CArrayGet(&gActors, i);
		if (!actor->isInUse)
		{
			continue;
		}
		if (!(actor->pData || (actor->flags & FLAGS_PRISONER)))
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
				if (!!(actor->flags & FLAGS_FOLLOWER))
				{
					if (IsCloseToPlayer(actor->Pos))
					{
						cmd = 0;
					}
					else
					{
						cmd = AIGoto(actor, AIGetClosestPlayerPos(
							Vec2iFull2Real(actor->Pos)));
					}
					actor->delay = actor->character->bot.actionDelay;
				}
				else if (!!(actor->flags & FLAGS_SNEAKY) &&
					!!(actor->flags & FLAGS_VISIBLE) &&
					DidPlayerShoot())
				{
					cmd = AIHunt(actor) | CMD_BUTTON1;
					if (actor->flags & FLAGS_RUNS_AWAY)
					{
						// Turn back and shoot for running away characters
						cmd = AIReverseDirection(cmd);
					}
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
					if (roll < actor->character->bot.probabilityToTrack)
					{
						cmd = AIHunt(actor);
					}
					else if (roll < actor->character->bot.probabilityToMove)
					{
						cmd = DirectionToCmd(rand() & 7);
					}
					else
					{
						cmd = 0;
					}
					actor->delay = actor->character->bot.actionDelay * delayModifier;
				}
				if (!bypass)
				{
					if (WillFire(actor, roll))
					{
						cmd |= CMD_BUTTON1;
						if (!!(actor->flags & FLAGS_FOLLOWER) &&
							(actor->flags & FLAGS_GOOD_GUY))
						{
							// Shoot in a random direction away
							int i;
							for (i = 0; i < 10; i++)
							{
								direction_e d =
									(direction_e)(rand() % DIRECTION_COUNT);
								if (!IsFacingPlayer(actor, d))
								{
									cmd = DirectionToCmd(d) | CMD_BUTTON1;
									break;
								}
							}
						}
						if (actor->flags & FLAGS_RUNS_AWAY)
						{
							// Turn back and shoot for running away characters
							cmd |= AIReverseDirection(AIHunt(actor));
						}
					}
					else
					{
						if ((actor->flags & FLAGS_VISIBLE) == 0)
						{
							// I think this is some hack to make sure invisible enemies don't fire so much
							actor->weapon.lock = 40;
						}
						if (cmd && !IsDirectionOK(actor, CmdToDirection(cmd)) &&
							(actor->flags & FLAGS_DETOURING) == 0)
						{
							Detour(actor);
							cmd = 0;
						}
					}
				}
			}
			CommandActor(actor, cmd, ticks);
		}
		else if ((actor->flags & FLAGS_PRISONER) != 0)
		{
			CommandActor(actor, 0, ticks);
		}
	}
	if (gMission.missionData->Enemies.size > 0 &&
		gMission.missionData->EnemyDensity > 0 &&
		count < MAX(1, (gMission.missionData->EnemyDensity * gConfig.Game.EnemyDensity) / 100))
	{
		Character *character = CharacterStoreGetRandomBaddie(
			&gCampaign.Setting.characters);
		TActor *baddie = CArrayGet(&gActors, ActorAdd(character, NULL));
		PlaceBaddie(baddie);
		gBaddieCount++;
	}
}

void InitializeBadGuys(void)
{
	int i;
	TActor *actor;
	for (i = 0; i < (int)gMission.missionData->Objectives.size; i++)
	{
		MissionObjective *mobj =
			CArrayGet(&gMission.missionData->Objectives, i);
		struct Objective *obj = CArrayGet(&gMission.Objectives, i);
		if (mobj->Type == OBJECTIVE_KILL &&
			gMission.missionData->SpecialChars.size > 0)
		{
			for (; obj->placed < mobj->Count; obj->placed++)
			{
				int id = ActorAdd(CharacterStoreGetRandomSpecial(
					&gCampaign.Setting.characters), NULL);
				actor = CArrayGet(&gActors, id);
				actor->tileItem.flags |= ObjectiveToTileItem(i);
				PlaceBaddie(actor);
			}
		}
		else if (mobj->Type == OBJECTIVE_RESCUE)
		{
			for (; obj->placed < mobj->Count; obj->placed++)
			{
				int id = ActorAdd(CharacterStoreGetPrisoner(
					&gCampaign.Setting.characters, 0), NULL);
				actor = CArrayGet(&gActors, id);
				actor->tileItem.flags |= ObjectiveToTileItem(i);
				if (MapHasLockedRooms(&gMap))
				{
					PlacePrisoner(actor);
				}
				else
				{
					PlaceBaddie(actor);
				}
			}
		}
	}

	gBaddieCount = gMission.index * 4;
	gAreGoodGuysPresent = 0;
}

void CreateEnemies(void)
{
	int i;
	if (gCampaign.Setting.characters.baddieCount <= 0)
	{
		return;
	}

	for (i = 0;
		i < MAX(1, (gMission.missionData->EnemyDensity * gConfig.Game.EnemyDensity) / 100);
		i++)
	{
		int id = ActorAdd(CharacterStoreGetRandomBaddie(
			&gCampaign.Setting.characters), NULL);
		TActor *enemy = CArrayGet(&gActors, id);
		PlaceBaddie(enemy);
		gBaddieCount++;
	}
}
