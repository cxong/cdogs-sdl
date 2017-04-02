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

    Copyright (c) 2013-2014, 2016 Cong Xu
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
static int gAreGoodGuysPresent = 0;


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

static bool IsCloseToPlayer(const Vec2i fullPos, const int fullDistance)
{
	TActor *closestPlayer = AIGetClosestPlayer(fullPos);
	return closestPlayer && CHEBYSHEV_DISTANCE(
		fullPos.x, fullPos.y,
		closestPlayer->Pos.x, closestPlayer->Pos.y) < fullDistance;
}

static bool CanSeeAPlayer(const TActor *a)
{
	const Vec2i realPos = Vec2iFull2Real(a->Pos);
	CA_FOREACH(const PlayerData, p, gPlayerDatas)
		if (!IsPlayerAlive(p))
		{
			continue;
		}
		const TActor *player = ActorGetByUID(p->ActorUID);
		const Vec2i playerRealPos = Vec2iFull2Real(player->Pos);
		// Can see player if:
		// - Clear line of sight, and
		// - If they are close, or if facing and they are not too far
		if (!AIHasClearShot(realPos, playerRealPos))
		{
			continue;
		}
		const int distance = CHEBYSHEV_DISTANCE(
			realPos.x, realPos.y, playerRealPos.x, playerRealPos.y);
		const bool isClose = distance < 16 * 4;
		const bool isNotTooFar = distance < 16 * 30;
		if (isClose ||
			(isNotTooFar && AIIsFacing(a, player->Pos, a->direction)))
		{
			return true;
		}
	CA_FOREACH_END()
	return false;
}


static bool IsPosOK(TActor *actor, Vec2i pos)
{
	const Vec2i realPos = Vec2iFull2Real(pos);
	if (IsCollisionDiamond(&gMap, realPos, actor->tileItem.size))
	{
		return false;
	}
	const CollisionParams params =
	{
		TILEITEM_IMPASSABLE, CalcCollisionTeam(true, actor),
		IsPVP(gCampaign.Entry.Mode)
	};
	if (OverlapGetFirstItem(
		&actor->tileItem, pos, actor->tileItem.size, params))
	{
		return false;
	}
	return true;
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
		ActorCanFire(actor) &&
		roll < bot->probabilityToShoot)
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
void CommandBadGuys(int ticks)
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
		if (!actor->isInUse)
		{
			continue;
		}
		const CharBot *bot = ActorGetCharacter(actor)->bot;
		if (!(actor->PlayerUID >= 0 || (actor->flags & FLAGS_PRISONER)))
		{
			if ((actor->flags & (FLAGS_VICTIM | FLAGS_GOOD_GUY)) != 0)
			{
				gAreGoodGuysPresent = 1;
			}

			count++;
			int cmd = 0;

			// Wake up if it can see a player
			if ((actor->flags & FLAGS_SLEEPING) &&
				actor->aiContext->Delay == 0)
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
				if (!IsCloseToPlayer(actor->Pos, (40 * 16) << 8))
				{
					actor->flags |= FLAGS_SLEEPING;
					ActorSetAIState(actor, AI_STATE_IDLE);
				}
			}

			if (!actor->dead && !(actor->flags & FLAGS_SLEEPING))
			{
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
						const Vec2i exitPos = MapGetExitPos(&gMap);
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
					else
					{
						cmd = 0;
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
							ActorGetGun(actor)->lock = 40;
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
			}
			actor->aiContext->Delay =
				MAX(0, actor->aiContext->Delay - ticks);
			CommandActor(actor, cmd, ticks);
		}
		else if ((actor->flags & FLAGS_PRISONER) != 0)
		{
			CommandActor(actor, 0, ticks);
		}
	CA_FOREACH_END()
	if (gMission.missionData->Enemies.size > 0 &&
		gMission.missionData->EnemyDensity > 0 &&
		count < MAX(1, (gMission.missionData->EnemyDensity * ConfigGetInt(&gConfig, "Game.EnemyDensity")) / 100))
	{
		NActorAdd aa = NActorAdd_init_default;
		aa.UID = ActorsGetNextUID();
		aa.CharId = CharacterStoreGetRandomBaddieId(
			&gCampaign.Setting.characters);
		aa.Direction = rand() % DIRECTION_COUNT;
		const Character *c =
			CArrayGet(&gCampaign.Setting.characters.OtherChars, aa.CharId);
		aa.Health = CharacterGetStartingHealth(c, true);
		aa.FullPos = PlaceAwayFromPlayers(&gMap, true);
		GameEvent e = GameEventNew(GAME_EVENT_ACTOR_ADD);
		e.u.ActorAdd = aa;
		GameEventsEnqueue(&gGameEvents, e);
		gBaddieCount++;
	}
}
static int Follow(TActor *a)
{
	// If we are a rescue objective and we are in the exit
	// area, stop following and stay in the rescue area
	const Character *ch = ActorGetCharacter(a);
	const CharacterStore *store = &gCampaign.Setting.characters;
	if (CharacterIsPrisoner(store, ch) && CanCompleteMission(&gMission) &&
		MapIsTileInExit(&gMap, &a->tileItem))
	{
		a->flags &= ~FLAGS_FOLLOWER;
		a->flags |= FLAGS_RESCUED;
		return 0;
	}
	else if (IsCloseToPlayer(a->Pos, 32 << 8))
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

void InitializeBadGuys(void)
{
	CA_FOREACH(Objective, o, gMission.missionData->Objectives)
		if (o->Type == OBJECTIVE_KILL &&
			gMission.missionData->SpecialChars.size > 0)
		{
			for (; o->placed < o->Count; o->placed++)
			{
				NActorAdd aa = NActorAdd_init_default;
				aa.UID = ActorsGetNextUID();
				aa.CharId = CharacterStoreGetRandomSpecialId(
					&gCampaign.Setting.characters);
				aa.TileItemFlags = ObjectiveToTileItem(_ca_index);
				aa.Direction = rand() % DIRECTION_COUNT;
				const Character *c =
					CArrayGet(&gCampaign.Setting.characters.OtherChars, aa.CharId);
				aa.Health = CharacterGetStartingHealth(c, true);
				aa.FullPos = PlaceAwayFromPlayers(&gMap, false);
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
				aa.TileItemFlags = ObjectiveToTileItem(_ca_index);
				aa.Direction = rand() % DIRECTION_COUNT;
				const Character *c =
					CArrayGet(&gCampaign.Setting.characters.OtherChars, aa.CharId);
				aa.Health = CharacterGetStartingHealth(c, true);
				if (MapHasLockedRooms(&gMap))
				{
					aa.FullPos = PlacePrisoner(&gMap);
				}
				else
				{
					aa.FullPos = PlaceAwayFromPlayers(&gMap, false);
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
	gAreGoodGuysPresent = 0;
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
		aa.FullPos = PlaceAwayFromPlayers(&gMap, true);
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
