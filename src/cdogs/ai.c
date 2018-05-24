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

    Copyright (c) 2013-2014, 2016-2017 Cong Xu
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

#include "actor_placement.h"
#include "ai_utils.h"
#include "collision/collision.h"
#include "config.h"
#include "defs.h"
#include "actors.h"
#include "events.h"
#include "game_events.h"
#include "gamedata.h"
#include "handle_game_events.h"
#include "mission.h"
#include "net_util.h"
#include "sys_specifics.h"
#include "utils.h"

static int gBaddieCount = 0;
static bool sAreGoodGuysPresent = false;


static bool IsFacingPlayer(TActor *actor, direction_e d)
{
	CA_FOREACH(const PlayerData, p, gPlayerDatas)
		if (!IsPlayerAlive(p))
		{
			continue;
		}
		const TActor *player = ActorGetByUID(p->ActorUID);
		if (AIIsFacing(actor, player->Pos, d))
		{
			return true;
		}
	CA_FOREACH_END()
	return false;
}


#define Distance(a,b) CHEBYSHEV_DISTANCE(a->x, a->y, b->x, b->y)

static bool IsCloseToPlayer(const struct vec2 pos, const float distance)
{
	const TActor *closestPlayer = AIGetClosestPlayer(pos);
	const float distance2 = distance * distance;
	return closestPlayer &&
		svec2_distance_squared(pos, closestPlayer->Pos) < distance2;
}

static bool CanSeeAPlayer(const TActor *a)
{
	CA_FOREACH(const PlayerData, p, gPlayerDatas)
		if (!IsPlayerAlive(p))
		{
			continue;
		}
		const TActor *player = ActorGetByUID(p->ActorUID);
		// Can see player if:
		// - Clear line of sight, and
		// - If they are close, or if facing and they are not too far
		if (!AIHasClearShot(a->Pos, player->Pos))
		{
			continue;
		}
		const float distance2 =
			svec2_distance_squared(a->Pos, player->Pos);
		const bool isClose = distance2 < SQUARED(16 * 4);
		const bool isNotTooFar = distance2 < SQUARED(16 * 30);
		if (isClose ||
			(isNotTooFar && AIIsFacing(a, player->Pos, a->direction)))
		{
			return true;
		}
	CA_FOREACH_END()
	return false;
}


static bool IsPosOK(const TActor *actor, const struct vec2 pos)
{
	if (IsCollisionDiamond(&gMap, pos, actor->thing.size))
	{
		return false;
	}
	const CollisionParams params =
	{
		THING_IMPASSABLE, CalcCollisionTeam(true, actor),
		IsPVP(gCampaign.Entry.Mode)
	};
	if (OverlapGetFirstItem(
		&actor->thing, pos, actor->thing.size, params))
	{
		return false;
	}
	return true;
}

#define STEPSIZE    4

static bool IsDirectionOK(const TActor *a, const int dir)
{
	switch (dir) {
	case DIRECTION_UP:
		return IsPosOK(a, svec2_add(a->Pos, svec2(0, -STEPSIZE)));
	case DIRECTION_UPLEFT:
		return
			IsPosOK(a, svec2_add(a->Pos, svec2(-STEPSIZE, -STEPSIZE))) ||
			IsPosOK(a, svec2_add(a->Pos, svec2(-STEPSIZE, 0))) ||
			IsPosOK(a, svec2_add(a->Pos, svec2(0, -STEPSIZE)));
	case DIRECTION_LEFT:
		return
			IsPosOK(a, svec2_add(a->Pos, svec2(-STEPSIZE, 0)));
	case DIRECTION_DOWNLEFT:
		return
			IsPosOK(a, svec2_add(a->Pos, svec2(-STEPSIZE, STEPSIZE))) ||
			IsPosOK(a, svec2_add(a->Pos, svec2(-STEPSIZE, 0))) ||
			IsPosOK(a, svec2_add(a->Pos, svec2(0, STEPSIZE)));
	case DIRECTION_DOWN:
		return
			IsPosOK(a, svec2_add(a->Pos, svec2(0, STEPSIZE)));
	case DIRECTION_DOWNRIGHT:
		return
			IsPosOK(a, svec2_add(a->Pos, svec2(STEPSIZE, STEPSIZE))) ||
			IsPosOK(a, svec2_add(a->Pos, svec2(STEPSIZE, 0))) ||
			IsPosOK(a, svec2_add(a->Pos, svec2(0, STEPSIZE)));
	case DIRECTION_RIGHT:
		return
			IsPosOK(a, svec2_add(a->Pos, svec2(STEPSIZE, 0)));
	case DIRECTION_UPRIGHT:
		return
			IsPosOK(a, svec2_add(a->Pos, svec2(STEPSIZE, -STEPSIZE))) ||
			IsPosOK(a, svec2_add(a->Pos, svec2(STEPSIZE, 0))) ||
			IsPosOK(a, svec2_add(a->Pos, svec2(0, -STEPSIZE)));
	}
	return 0;
}


static int BrightWalk(TActor * actor, int roll)
{
	const CharBot *bot = ActorGetCharacter(actor)->bot;
	if (!!(actor->flags & FLAGS_VISIBLE) && roll < bot->probabilityToTrack)
	{
		actor->flags &= ~FLAGS_DETOURING;
		return AIHuntClosest(actor);
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
	const CharBot *bot = ActorGetCharacter(actor)->bot;
	if ((actor->flags & FLAGS_VISIBLE) != 0 &&
		ActorCanFireWeapon(actor, ACTOR_GET_WEAPON(actor)) &&
		roll < bot->probabilityToShoot)
	{
		if ((actor->flags & FLAGS_GOOD_GUY) != 0)
			return 1;	//!FacingPlayer( actor);
		else if (sAreGoodGuysPresent)
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


static bool DidPlayerShoot(void)
{
	CA_FOREACH(const PlayerData, p, gPlayerDatas)
		if (!IsPlayerAlive(p))
		{
			continue;
		}
		const TActor *player = ActorGetByUID(p->ActorUID);
		if (player->lastCmd & CMD_BUTTON1)
		{
			return true;
		}
	CA_FOREACH_END()
	return false;
}

static int Follow(TActor *a);
static int GetCmd(TActor *actor, const int delayModifier, const int rollLimit);
int AICommand(const int ticks)
{
	int count = 0;
	int delayModifier;
	int rollLimit;

	switch (ConfigGetEnum(&gConfig, "Game.Difficulty"))
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

	CA_FOREACH(TActor, actor, gActors)
		if (!actor->isInUse || actor->PlayerUID >= 0 || actor->dead)
		{
			continue;
		}
		int cmd = 0;
		if (!(actor->flags & FLAGS_PRISONER))
		{
			if (actor->flags & (FLAGS_VICTIM | FLAGS_GOOD_GUY))
			{
				sAreGoodGuysPresent = true;
			}
			cmd = GetCmd(actor, delayModifier, rollLimit);
			actor->aiContext->Delay = MAX(0, actor->aiContext->Delay - ticks);
		}
		CommandActor(actor, cmd, ticks);
		actor->aiContext->LastCmd = cmd;
		count++;
	CA_FOREACH_END()
	return count;
}
static int GetCmd(TActor *actor, const int delayModifier, const int rollLimit)
{
	const CharBot *bot = ActorGetCharacter(actor)->bot;

	int cmd = 0;

	// Wake up if it can see a player
	if ((actor->flags & FLAGS_SLEEPING) && actor->aiContext->Delay == 0)
	{
		if (CanSeeAPlayer(actor))
		{
			actor->flags &= ~FLAGS_SLEEPING;
			ActorSetAIState(actor, AI_STATE_NONE);
		}
		actor->aiContext->Delay = bot->actionDelay * delayModifier;
		// Randomly change direction
		int newDir = (int)actor->direction + ((rand() % 2) * 2 - 1);
		if (newDir < (int)DIRECTION_UP)
		{
			newDir = (int)DIRECTION_UPLEFT;
		}
		if (newDir == (int)DIRECTION_COUNT)
		{
			newDir = (int)DIRECTION_UP;
		}
		cmd = DirectionToCmd((int)newDir);
	}
	// Go to sleep if the player's too far away
	if (!(actor->flags & FLAGS_SLEEPING) &&
		actor->aiContext->Delay == 0 &&
		!(actor->flags & FLAGS_AWAKEALWAYS))
	{
		if (!IsCloseToPlayer(actor->Pos, 40 * 16))
		{
			actor->flags |= FLAGS_SLEEPING;
			ActorSetAIState(actor, AI_STATE_IDLE);
		}
	}

	if (actor->flags & FLAGS_SLEEPING)
	{
		return cmd;
	}

	bool bypass = false;
	const int roll = rand() % rollLimit;
	if (actor->flags & FLAGS_FOLLOWER)
	{
		cmd = Follow(actor);
	}
	else if (!!(actor->flags & FLAGS_SNEAKY) &&
		!!(actor->flags & FLAGS_VISIBLE) &&
		DidPlayerShoot())
	{
		cmd = AIHuntClosest(actor) | CMD_BUTTON1;
		if (actor->flags & FLAGS_RUNS_AWAY)
		{
			// Turn back and shoot for running away characters
			cmd = AIReverseDirection(cmd);
		}
		bypass = true;
		ActorSetAIState(actor, AI_STATE_HUNT);
	}
	else if (actor->flags & FLAGS_DETOURING)
	{
		cmd = BrightWalk(actor, roll);
		ActorSetAIState(actor, AI_STATE_TRACK);
	}
	else if (actor->flags & FLAGS_RESCUED)
	{
		// If we haven't completed all objectives, act as follower
		if (!CanCompleteMission(&gMission))
		{
			cmd = Follow(actor);
		}
		else
		{
			// Run towards exit
			const struct vec2 exitPos = MapGetExitPos(&gMap);
			cmd = AIGoto(actor, exitPos, false);
		}
	}
	else if (actor->aiContext->Delay > 0)
	{
		cmd = actor->lastCmd & ~CMD_BUTTON1;
	}
	else
	{
		if (roll < bot->probabilityToTrack)
		{
			cmd = AIHuntClosest(actor);
			ActorSetAIState(actor, AI_STATE_HUNT);
		}
		else if (roll < bot->probabilityToMove)
		{
			cmd = DirectionToCmd(rand() & 7);
			ActorSetAIState(actor, AI_STATE_TRACK);
		}
		actor->aiContext->Delay = bot->actionDelay * delayModifier;
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
				for (int j = 0; j < 10; j++)
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
				cmd |= AIReverseDirection(AIHuntClosest(actor));
			}
			ActorSetAIState(actor, AI_STATE_HUNT);
		}
		else
		{
			if ((actor->flags & FLAGS_VISIBLE) == 0)
			{
				// I think this is some hack to make sure invisible enemies don't fire so much
				ACTOR_GET_WEAPON(actor)->lock = 40;
			}
			if (cmd && !IsDirectionOK(actor, CmdToDirection(cmd)) &&
				(actor->flags & FLAGS_DETOURING) == 0)
			{
				Detour(actor);
				cmd = 0;
				ActorSetAIState(actor, AI_STATE_TRACK);
			}
		}
	}
	return cmd;
}
static int Follow(TActor *a)
{
	// If we are a rescue objective and we are in the exit
	// area, stop following and stay in the rescue area
	const Character *ch = ActorGetCharacter(a);
	const CharacterStore *store = &gCampaign.Setting.characters;
	if (CharacterIsPrisoner(store, ch) && CanCompleteMission(&gMission) &&
		MapIsTileInExit(&gMap, &a->thing))
	{
		a->flags &= ~FLAGS_FOLLOWER;
		a->flags |= FLAGS_RESCUED;
		return 0;
	}
	else if (IsCloseToPlayer(a->Pos, 32))
	{
		ActorSetAIState(a, AI_STATE_IDLE);
		return 0;
	}
	else
	{
		ActorSetAIState(a, AI_STATE_FOLLOW);
		return AIGoto(a, AIGetClosestPlayerPos(a->Pos), true);
	}
}

void AICommandLast(const int ticks)
{
	CA_FOREACH(TActor, actor, gActors)
		if (!actor->isInUse || actor->PlayerUID >= 0 || actor->dead ||
			(actor->flags & FLAGS_PRISONER))
		{
			continue;
		}
		const int cmd = actor->aiContext->LastCmd;
		actor->aiContext->Delay = MAX(0, actor->aiContext->Delay - ticks);
		CommandActor(actor, cmd, ticks);
	CA_FOREACH_END()
}

void AIAddRandomEnemies(const int enemies, const Mission *m)
{
	if (m->Enemies.size > 0 && m->EnemyDensity > 0 &&
		enemies < MAX(1, (m->EnemyDensity * ConfigGetInt(&gConfig, "Game.EnemyDensity")) / 100))
	{
		NActorAdd aa = NActorAdd_init_default;
		aa.UID = ActorsGetNextUID();
		aa.CharId = CharacterStoreGetRandomBaddieId(
			&gCampaign.Setting.characters);
		aa.Direction = rand() % DIRECTION_COUNT;
		const Character *c =
			CArrayGet(&gCampaign.Setting.characters.OtherChars, aa.CharId);
		aa.Health = CharacterGetStartingHealth(c, true);
		aa.Pos = PlaceAwayFromPlayers(&gMap, true, PLACEMENT_ACCESS_ANY);
		GameEvent e = GameEventNew(GAME_EVENT_ACTOR_ADD);
		e.u.ActorAdd = aa;
		GameEventsEnqueue(&gGameEvents, e);
		gBaddieCount++;
	}
}

void InitializeBadGuys(void)
{
	CA_FOREACH(Objective, o, gMission.missionData->Objectives)
		const PlacementAccessFlags paFlags =
			ObjectiveGetPlacementAccessFlags(o);
		if (o->Type == OBJECTIVE_KILL &&
			gMission.missionData->SpecialChars.size > 0)
		{
			for (; o->placed < o->Count; o->placed++)
			{
				NActorAdd aa = NActorAdd_init_default;
				aa.UID = ActorsGetNextUID();
				aa.CharId = CharacterStoreGetRandomSpecialId(
					&gCampaign.Setting.characters);
				aa.ThingFlags = ObjectiveToThing(_ca_index);
				aa.Direction = rand() % DIRECTION_COUNT;
				const Character *c =
					CArrayGet(&gCampaign.Setting.characters.OtherChars, aa.CharId);
				aa.Health = CharacterGetStartingHealth(c, true);
				aa.Pos = PlaceAwayFromPlayers(&gMap, false, paFlags);
				GameEvent e = GameEventNew(GAME_EVENT_ACTOR_ADD);
				e.u.ActorAdd = aa;
				GameEventsEnqueue(&gGameEvents, e);

				// Process the events that actually place the actors
				HandleGameEvents(&gGameEvents, NULL, NULL, NULL);
			}
		}
		else if (o->Type == OBJECTIVE_RESCUE)
		{
			for (; o->placed < o->Count; o->placed++)
			{
				NActorAdd aa = NActorAdd_init_default;
				aa.UID = ActorsGetNextUID();
				aa.CharId = CharacterStoreGetPrisonerId(
					&gCampaign.Setting.characters, 0);
				aa.ThingFlags = ObjectiveToThing(_ca_index);
				aa.Direction = rand() % DIRECTION_COUNT;
				const Character *c =
					CArrayGet(&gCampaign.Setting.characters.OtherChars, aa.CharId);
				aa.Health = CharacterGetStartingHealth(c, true);
				if (MapHasLockedRooms(&gMap))
				{
					aa.Pos = PlacePrisoner(&gMap);
				}
				else
				{
					aa.Pos = PlaceAwayFromPlayers(&gMap, false, paFlags);
				}
				GameEvent e = GameEventNew(GAME_EVENT_ACTOR_ADD);
				e.u.ActorAdd = aa;
				GameEventsEnqueue(&gGameEvents, e);

				// Process the events that actually place the actors
				HandleGameEvents(&gGameEvents, NULL, NULL, NULL);
			}
		}
	CA_FOREACH_END()

	gBaddieCount = gMission.index * 4;
	sAreGoodGuysPresent = false;
}

void CreateEnemies(void)
{
	if (gCampaign.Setting.characters.baddieIds.size == 0)
	{
		return;
	}

	const int density =
		gMission.missionData->EnemyDensity *
		ConfigGetInt(&gConfig, "Game.EnemyDensity");
	for (int i = 0; i < density / 100; i++)
	{
		NActorAdd aa = NActorAdd_init_default;
		aa.UID = ActorsGetNextUID();
		aa.CharId = CharacterStoreGetRandomBaddieId(
			&gCampaign.Setting.characters);
		aa.Pos = PlaceAwayFromPlayers(&gMap, true, PLACEMENT_ACCESS_ANY);
		aa.Direction = rand() % DIRECTION_COUNT;
		const Character *c =
			CArrayGet(&gCampaign.Setting.characters.OtherChars, aa.CharId);
		aa.Health = CharacterGetStartingHealth(c, true);
		GameEvent e = GameEventNew(GAME_EVENT_ACTOR_ADD);
		e.u.ActorAdd = aa;
		GameEventsEnqueue(&gGameEvents, e);
		gBaddieCount++;

		// Process the events that actually place the actors
		HandleGameEvents(&gGameEvents, NULL, NULL, NULL);
	}
}
