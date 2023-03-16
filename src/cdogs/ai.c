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

	Copyright (c) 2013-2014, 2016-2017, 2020-2021 Cong Xu
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
#include "actors.h"
#include "ai_utils.h"
#include "collision/collision.h"
#include "config.h"
#include "defs.h"
#include "events.h"
#include "game_events.h"
#include "gamedata.h"
#include "handle_game_events.h"
#include "mission.h"
#include "net_util.h"
#include "sys_specifics.h"
#include "utils.h"

#define AI_WAKE_SOUND_RANGE (8 * TILE_WIDTH)
#define AI_WAKE_SOUND_RANGE_INDIRECT (4 * TILE_WIDTH)

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

#define Distance(a, b) CHEBYSHEV_DISTANCE(a->x, a->y, b->x, b->y)

static bool IsCloseToPlayer(const struct vec2 pos, const float distance)
{
	const TActor *closestPlayer = AIGetClosestPlayer(pos);
	const float distance2 = distance * distance;
	return closestPlayer &&
		   svec2_distance_squared(pos, closestPlayer->Pos) < distance2;
}

static bool CanSeeActor(const TActor *a, const TActor *target)
{
	// Can see if:
	// - They are close
	// - Or if they can see them with line of sight
	const float distance2 = svec2_distance_squared(a->Pos, target->Pos);
	const bool isClose = distance2 < SQUARED(16 * 2);
	return isClose || AICanSee(a, target->Pos, a->direction);
}

static bool CanSeeAPlayer(const TActor *a)
{
	CA_FOREACH(const PlayerData, p, gPlayerDatas)
	if (!IsPlayerAlive(p))
	{
		continue;
	}
	const TActor *player = ActorGetByUID(p->ActorUID);
	if (CanSeeActor(a, player))
	{
		return true;
	}
	CA_FOREACH_END()
	return false;
}

static bool CanSeeActorBeingAttacked(const TActor *a)
{
	CA_FOREACH(const TActor, target, gActors)
	if (!target->isInUse)
	{
		continue;
	}
	// Use grimacing as a proxy to being attacked / being aggressive
	if (!ActorIsGrimacing(target) && target->dead == 0)
	{
		continue;
	}
	if (CanSeeActor(a, target))
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
	const CollisionParams params = {
		THING_IMPASSABLE, CalcCollisionTeam(true, actor),
		IsPVP(gCampaign.Entry.Mode), false};
	if (OverlapGetFirstItem(
			&actor->thing, pos, actor->thing.size, actor->thing.Vel, params))
	{
		return false;
	}
	return true;
}

#define STEPSIZE 4

static bool IsDirectionOK(const TActor *a, const int dir)
{
	switch (dir)
	{
	case DIRECTION_UP:
		return IsPosOK(a, svec2_add(a->Pos, svec2(0, -STEPSIZE)));
	case DIRECTION_UPLEFT:
		return IsPosOK(a, svec2_add(a->Pos, svec2(-STEPSIZE, -STEPSIZE))) ||
			   IsPosOK(a, svec2_add(a->Pos, svec2(-STEPSIZE, 0))) ||
			   IsPosOK(a, svec2_add(a->Pos, svec2(0, -STEPSIZE)));
	case DIRECTION_LEFT:
		return IsPosOK(a, svec2_add(a->Pos, svec2(-STEPSIZE, 0)));
	case DIRECTION_DOWNLEFT:
		return IsPosOK(a, svec2_add(a->Pos, svec2(-STEPSIZE, STEPSIZE))) ||
			   IsPosOK(a, svec2_add(a->Pos, svec2(-STEPSIZE, 0))) ||
			   IsPosOK(a, svec2_add(a->Pos, svec2(0, STEPSIZE)));
	case DIRECTION_DOWN:
		return IsPosOK(a, svec2_add(a->Pos, svec2(0, STEPSIZE)));
	case DIRECTION_DOWNRIGHT:
		return IsPosOK(a, svec2_add(a->Pos, svec2(STEPSIZE, STEPSIZE))) ||
			   IsPosOK(a, svec2_add(a->Pos, svec2(STEPSIZE, 0))) ||
			   IsPosOK(a, svec2_add(a->Pos, svec2(0, STEPSIZE)));
	case DIRECTION_RIGHT:
		return IsPosOK(a, svec2_add(a->Pos, svec2(STEPSIZE, 0)));
	case DIRECTION_UPRIGHT:
		return IsPosOK(a, svec2_add(a->Pos, svec2(STEPSIZE, -STEPSIZE))) ||
			   IsPosOK(a, svec2_add(a->Pos, svec2(STEPSIZE, 0))) ||
			   IsPosOK(a, svec2_add(a->Pos, svec2(0, -STEPSIZE)));
	}
	return 0;
}

static int BrightWalk(TActor *actor, int roll)
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
			if (actor->turns == 4)
			{
				actor->flags &= ~(FLAGS_DETOURING | FLAGS_TRYRIGHT);
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
			if (actor->turns == 4)
			{
				actor->flags &= ~(FLAGS_DETOURING | FLAGS_TRYRIGHT);
				actor->turns = 0;
			}
		}
	}
	return DirectionToCmd(actor->direction);
}

static int WillFire(TActor *actor, int roll)
{
	const CharBot *bot = ActorGetCharacter(actor)->bot;
	if ((actor->flags & FLAGS_VISIBLE) != 0 &&
		roll < bot->probabilityToShoot &&
		ActorGetCanFireBarrel(actor, ACTOR_GET_WEAPON(actor)) >= 0)
	{
		if ((actor->flags & FLAGS_GOOD_GUY) != 0)
			return 1; //! FacingPlayer( actor);
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

void Detour(TActor *actor)
{
	actor->flags |= FLAGS_DETOURING;
	actor->turns = 1;
	if (actor->flags & FLAGS_TRYRIGHT)
		actor->direction = (CmdToDirection(actor->lastCmd) + 1) % 8;
	else
		actor->direction = (CmdToDirection(actor->lastCmd) + 7) % 8;
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

static bool IsAIEnabled(const TActor *a)
{
	return a->isInUse && a->PlayerUID < 0 && !a->dead &&
		   !ActorGetCharacter(a)->Class->Vehicle;
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
	if (!IsAIEnabled(actor))
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

	// Wake up if it can see a player or someone dying
	if ((actor->flags & FLAGS_SLEEPING) && (actor->flags & FLAGS_VISIBLE) &&
		actor->aiContext->Delay == 0 &&
		(CanSeeAPlayer(actor) || CanSeeActorBeingAttacked(actor)))
	{
		AIWake(actor, delayModifier);
	}
	// Fully wake up
	if ((actor->flags & FLAGS_WAKING) && actor->aiContext->Delay == 0)
	{
		actor->flags &= ~FLAGS_WAKING;
	}
	// Go to sleep if the player's too far away
	if (!(actor->flags & FLAGS_SLEEPING) && actor->aiContext->Delay == 0 &&
		!(actor->flags & FLAGS_AWAKEALWAYS))
	{
		if (!IsCloseToPlayer(actor->Pos, 40 * 16))
		{
			actor->flags |= FLAGS_SLEEPING;
			actor->flags &= ~FLAGS_WAKING;
			ActorSetAIState(actor, AI_STATE_IDLE);
		}
	}

	// Don't do anything if the AI is sleeping or waking
	if (actor->flags & (FLAGS_SLEEPING | FLAGS_WAKING))
	{
		return cmd;
	}

	bool bypass = false;
	const int roll = rand() % rollLimit;
	if (actor->flags & FLAGS_FOLLOWER)
	{
		cmd = Follow(actor);
	}
	else if (
		!!(actor->flags & FLAGS_SNEAKY) && !!(actor->flags & FLAGS_VISIBLE) &&
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
		if (!CanCompleteMission(&gMission) || gMap.exits.size > 1)
		{
			cmd = Follow(actor);
		}
		else
		{
			// Run towards the exit if there is one
			if (gMap.exits.size == 1)
			{
				const struct vec2 exitPos = MapGetExitPos(&gMap, 0);
				cmd = AIGoto(actor, exitPos, false);
			}
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
					direction_e d = (direction_e)(rand() % DIRECTION_COUNT);
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
				// I think this is some hack to make sure invisible enemies
				// don't fire so much
				Weapon *w = ACTOR_GET_WEAPON(actor);
				for (int i = 0; i < WeaponClassNumBarrels(w->Gun); i++)
				{
					w->barrels[i].lock = 40;
				}
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
void AIWake(TActor *a, const int delayModifier)
{
	if (!a->aiContext || !(a->flags & FLAGS_SLEEPING))
		return;
	a->flags &= ~FLAGS_SLEEPING;
	a->flags |= FLAGS_WAKING;
	ActorSetAIState(a, AI_STATE_NONE);
	const CharBot *bot = ActorGetCharacter(a)->bot;
	if (bot == NULL)
		return;
	a->aiContext->Delay = bot->actionDelay * delayModifier;

	// Don't play alert sound for invisible enemies
	if (!(a->flags & FLAGS_SEETHROUGH))
	{
		GameEvent es = GameEventNew(GAME_EVENT_SOUND_AT);
		CharacterClassGetSound(
			ActorGetCharacter(a)->Class, es.u.SoundAt.Sound, "alert");
		es.u.SoundAt.Pos = Vec2ToNet(a->thing.Pos);
		GameEventsEnqueue(&gGameEvents, es);
	}
}
static int Follow(TActor *a)
{
	// If we are a rescue objective and we are in the same exit as another
	// player, stop following and stay in the exit
	const Character *ch = ActorGetCharacter(a);
	const CharacterStore *store = &gCampaign.Setting.characters;
	if (CharacterIsPrisoner(store, ch) && CanCompleteMission(&gMission))
	{
		const int exit = MapIsTileInExit(&gMap, &a->thing, -1);
		if (exit != -1)
		{
			CA_FOREACH(const PlayerData, pd, gPlayerDatas)
			if (!IsPlayerAlive(pd))
			{
				continue;
			}
			const TActor *p = ActorGetByUID(pd->ActorUID);
			const int playerExit = MapIsTileInExit(&gMap, &p->thing, -1);
			if (playerExit == exit)
			{
				a->flags &= ~FLAGS_FOLLOWER;
				a->flags |= FLAGS_RESCUED;
				return 0;
			}
			CA_FOREACH_END()
		}
	}
	if (IsCloseToPlayer(a->Pos, 32))
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
	if (!IsAIEnabled(actor))
	{
		continue;
	}
	const int cmd = actor->aiContext->LastCmd;
	actor->aiContext->Delay = MAX(0, actor->aiContext->Delay - ticks);
	CommandActor(actor, cmd, ticks);
	CA_FOREACH_END()
}

void AIWakeOnSoundAt(const struct vec2 pos)
{
	CA_FOREACH(TActor, actor, gActors)
	if (!actor->isInUse || actor->PlayerUID >= 0 || actor->dead ||
		!(actor->flags & FLAGS_SLEEPING) || (actor->flags & FLAGS_DEAF))
	{
		continue;
	}
	const float d =
		CHEBYSHEV_DISTANCE(pos.x, pos.y, actor->Pos.x, actor->Pos.y);
	if (d > AI_WAKE_SOUND_RANGE)
	{
		continue;
	}
	if (d > AI_WAKE_SOUND_RANGE_INDIRECT &&
		!AIHasClearView(actor, pos, AI_WAKE_SOUND_RANGE_INDIRECT))
	{
		continue;
	}
	AIWake(actor, 1);
	CA_FOREACH_END()
}

void AIAddRandomEnemies(const int enemies, const Mission *m)
{
	if (m->Enemies.size > 0 && m->EnemyDensity > 0 &&
		enemies < MAX(1, (m->EnemyDensity *
						  ConfigGetInt(&gConfig, "Game.EnemyDensity")) /
							 100))
	{
		const int charId =
			CharacterStoreGetRandomBaddieId(&gCampaign.Setting.characters);
		const Character *c =
			CArrayGet(&gCampaign.Setting.characters.OtherChars, charId);
		GameEvent e = GameEventNewActorAdd(
			PlaceAwayFromPlayers(&gMap, true, PLACEMENT_ACCESS_ANY), c, NULL);
		e.u.ActorAdd.CharId = charId;
		GameEventsEnqueue(&gGameEvents, e);
		gBaddieCount++;
	}
}

void InitializeBadGuys(void)
{
	CA_FOREACH(Objective, o, gMission.missionData->Objectives)
	const PlacementAccessFlags paFlags = ObjectiveGetPlacementAccessFlags(o);
	if (o->Type == OBJECTIVE_KILL &&
		gMission.missionData->SpecialChars.size > 0)
	{
		const int charId =
			CharacterStoreGetRandomSpecialId(&gCampaign.Setting.characters);
		const Character *c =
			CArrayGet(&gCampaign.Setting.characters.OtherChars, charId);
		for (; o->placed < o->Count; o->placed++)
		{
			GameEvent e = GameEventNewActorAdd(
				PlaceAwayFromPlayers(&gMap, false, paFlags), c, NULL);
			e.u.ActorAdd.CharId = charId;
			e.u.ActorAdd.ThingFlags = ObjectiveToThing(_ca_index);
			GameEventsEnqueue(&gGameEvents, e);

			// Process the events that actually place the actors
			HandleGameEvents(&gGameEvents, NULL, NULL, NULL, NULL);
		}
	}
	else if (o->Type == OBJECTIVE_RESCUE)
	{
		const int charId =
			CharacterStoreGetPrisonerId(&gCampaign.Setting.characters, 0);
		const Character *c =
			CArrayGet(&gCampaign.Setting.characters.OtherChars, charId);
		for (; o->placed < o->Count; o->placed++)
		{
			GameEvent e = GameEventNewActorAdd(
				MapHasLockedRooms(&gMap)
					? PlacePrisoner(&gMap)
					: PlaceAwayFromPlayers(&gMap, false, paFlags),
				c, NULL);
			e.u.ActorAdd.CharId = charId;
			e.u.ActorAdd.ThingFlags = ObjectiveToThing(_ca_index);
			GameEventsEnqueue(&gGameEvents, e);

			// Process the events that actually place the actors
			HandleGameEvents(&gGameEvents, NULL, NULL, NULL, NULL);
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

	const int density = gMission.missionData->EnemyDensity *
						ConfigGetInt(&gConfig, "Game.EnemyDensity");
	for (int i = 0; i < density / 100; i++)
	{
		const int charId =
			CharacterStoreGetRandomBaddieId(&gCampaign.Setting.characters);
		const Character *c =
			CArrayGet(&gCampaign.Setting.characters.OtherChars, charId);
		GameEvent e = GameEventNewActorAdd(
			PlaceAwayFromPlayers(&gMap, true, PLACEMENT_ACCESS_ANY), c, NULL);
		e.u.ActorAdd.CharId = charId;
		GameEventsEnqueue(&gGameEvents, e);
		gBaddieCount++;

		// Process the events that actually place the actors
		HandleGameEvents(&gGameEvents, NULL, NULL, NULL, NULL);
	}
}
