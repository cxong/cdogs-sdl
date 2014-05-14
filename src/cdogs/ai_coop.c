/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.

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
#include "ai_coop.h"

#include "ai_utils.h"

// How many ticks to stay in one confusion state
#define CONFUSION_STATE_TICKS_MIN 25
#define CONFUSION_STATE_TICKS_RANGE 25


static int AICoopGetCmdNormal(TActor *actor);
int AICoopGetCmd(TActor *actor, const int ticks)
{
	// Create AI context if it isn't there already
	// This is because co-op AIs don't have the character bot property
	// TODO: don't require this lazy initialisation
	if (actor->aiContext == NULL)
	{
		actor->aiContext = AIContextNew();
	}

	int cmd = 0;

	// Special decision tree for confusion
	// - If confused, randomly:
	//   - Perform the right action (reverse directions)
	//   - Perform a random command (except for switching weapons)
	// - And never slide
	if (actor->confused)
	{
		AIConfusionState *s = &actor->aiContext->ConfusionState;
		// Check delay and change state
		actor->aiContext->Delay = MAX(0, actor->aiContext->Delay - ticks);
		if (actor->aiContext->Delay == 0)
		{
			actor->aiContext->Delay =
				CONFUSION_STATE_TICKS_MIN +
				(rand() % CONFUSION_STATE_TICKS_RANGE);
			if (s->Type == AI_CONFUSION_CONFUSED)
			{
				s->Type = AI_CONFUSION_CORRECT;
			}
			else
			{
				actor->aiContext->State = AI_STATE_CONFUSED;
				s->Type = AI_CONFUSION_CONFUSED;
				// Generate the confused action
				s->Cmd = rand() &
					(CMD_LEFT | CMD_RIGHT | CMD_UP | CMD_DOWN |
					CMD_BUTTON1 | CMD_BUTTON2);
			}
		}
		// Choose confusion action based on state
		switch (s->Type)
		{
		case AI_CONFUSION_CONFUSED:
			// Use canned random command
			// TODO: add state for this action
			cmd = s->Cmd;
			break;
		case AI_CONFUSION_CORRECT:
			// Reverse directions so they are correct
			cmd = CmdGetReverse(AICoopGetCmdNormal(actor));
			break;
		default:
			CASSERT(false, "Unknown state");
			break;
		}
		// Don't slide
		if ((cmd & CMD_BUTTON2) &&
			(cmd & (CMD_LEFT | CMD_RIGHT | CMD_UP | CMD_DOWN)))
		{
			cmd &= ~CMD_BUTTON2;
		}
	}
	else
	{
		// Act normally
		cmd = AICoopGetCmdNormal(actor);
	}
	return cmd;
}

static int SmartGoto(TActor *actor, Vec2i pos, int minDistance2);
static bool TryCompleteNearbyObjective(
	TActor *actor, const TActor *closestPlayer,
	const int distanceTooFarFromPlayer, int *cmdOut);
static int AICoopGetCmdNormal(TActor *actor)
{
	// Use decision tree to command the AI
	// - If too far away from nearest player
	//   - Go to nearest player
	// - else
	//   - If closest enemy is close enough
	//     - Attack enemy
	//   - else
	//     - Go to nearest player

	// Follow the closest player with a lower ID
	TActor *closestPlayer = NULL;
	int minDistance2 = -1;
	for (int i = 0; i < actor->pData->playerIndex; i++)
	{
		if (!IsPlayerAlive(i))
		{
			continue;
		}
		TActor *p = CArrayGet(&gActors, gPlayerIds[i]);
		int distance2 = DistanceSquared(
			Vec2iFull2Real(actor->Pos), Vec2iFull2Real(p->Pos));
		if (!closestPlayer || distance2 < minDistance2)
		{
			minDistance2 = distance2;
			closestPlayer = p;
		}
	}

	// Set distance we want to stay within the lead player
	int distanceTooFarFromPlayer = 8;
	// If player is exiting, we want to be very close to the player
	if (closestPlayer && closestPlayer->action == ACTORACTION_EXITING)
	{
		distanceTooFarFromPlayer = 2;
	}
	if (closestPlayer &&
		minDistance2 > distanceTooFarFromPlayer*distanceTooFarFromPlayer*16*16)
	{
		actor->aiContext->State = AI_STATE_FOLLOW;
		return SmartGoto(actor, Vec2iFull2Real(closestPlayer->Pos), minDistance2);
	}

	// Check if closest enemy is close enough, and visible
	TActor *closestEnemy =
		AIGetClosestVisibleEnemy(actor->Pos, actor->flags, 1);
	if (closestEnemy)
	{
		int minEnemyDistance = CHEBYSHEV_DISTANCE(
			actor->Pos.x, actor->Pos.y,
			closestEnemy->Pos.x, closestEnemy->Pos.y);
		// Also only engage if there's a clear shot
		if (minEnemyDistance > 0 && minEnemyDistance < ((12 * 16) << 8) &&
			AIHasClearShot(
			Vec2iFull2Real(actor->Pos), Vec2iFull2Real(closestEnemy->Pos)))
		{
			actor->aiContext->State = AI_STATE_HUNT;
			int cmd = AIHunt(actor);
			// only fire if gun is ready
			if (actor->weapon.lock <= 0)
			{
				cmd |= CMD_BUTTON1;
			}
			return cmd;
		}
	}

	// Look for objectives nearby to complete
	int cmd;
	if (TryCompleteNearbyObjective(
		actor, closestPlayer, distanceTooFarFromPlayer, &cmd))
	{
		return cmd;
	}

	// Otherwise, just go towards the closest player as long as we don't
	// run into them
	if (closestPlayer && minDistance2 > 4*4*16*16/3/3)
	{
		actor->aiContext->State = AI_STATE_FOLLOW;
		return SmartGoto(actor, Vec2iFull2Real(closestPlayer->Pos), minDistance2);
	}

	actor->aiContext->State = AI_STATE_IDLE;
	return 0;
}
// Number of ticks to persist in trying to destroy an obstruction
// before giving up and going around
#define STUCK_TICKS 70
// Goto with extra smarts:
// - If clear path, slide
// - If non-dangerous object blocking, shoot at it
// - If stuck for a long time, pathfind around obstructing object
static int SmartGoto(TActor *actor, Vec2i realPos, int minDistance2)
{
	int cmd = AIGoto(actor, realPos, true);
	// Try to slide if there is a clear path and we are far enough away
	if ((cmd & (CMD_LEFT | CMD_RIGHT | CMD_UP | CMD_DOWN)) &&
		AIHasClearPath(
			Vec2iFull2Real(actor->Pos),
			realPos,
			!actor->aiContext->IsStuckTooLong) &&
		minDistance2 > 7 * 7 * 16 * 16)
	{
		cmd |= CMD_BUTTON2;
	}
	// If running into safe object, and we're being blocked, shoot at it
	TObject *o = AIGetObjectRunningInto(actor, cmd);
	Vec2i tilePos = Vec2iToTile(Vec2iFull2Real(actor->Pos));
	if (o && !(o->flags & OBJFLAG_DANGEROUS) &&
		Vec2iEqual(tilePos, actor->aiContext->LastTile))
	{
		cmd = AIGoto(
			actor, Vec2iNew(o->tileItem.x, o->tileItem.y), true) |
			CMD_BUTTON1;
	}
	// Check if we are stuck
	if (Vec2iEqual(tilePos, actor->aiContext->LastTile))
	{
		actor->aiContext->Delay++;
		if (actor->aiContext->Delay >= STUCK_TICKS)
		{
			// We've been stuck for too long
			// Pathfind around it
			actor->aiContext->IsStuckTooLong = true;
			cmd = AIGoto(actor, realPos, !actor->aiContext->IsStuckTooLong);
		}
	}
	else
	{
		actor->aiContext->Delay = 0;
		actor->aiContext->IsStuckTooLong = false;
	}
	actor->aiContext->LastTile = tilePos;
	return cmd;
}
static bool CanGetObjective(
	const Vec2i objRealPos, const Vec2i actorRealPos, const TActor *player,
	const int distanceTooFarFromPlayer);
static int GotoObjective(TActor *actor, int objDistance);
static bool TryCompleteNearbyObjective(
	TActor *actor, const TActor *closestPlayer,
	const int distanceTooFarFromPlayer, int *cmdOut)
{
	AIContext *context = actor->aiContext;
	AIObjectiveState *objState = &context->ObjectiveState;
	const Vec2i actorRealPos = Vec2iFull2Real(actor->Pos);

	// Check to see if we are already attempting to complete an objective,
	// and that it hasn't been updated since we last checked.
	// If so keep following the path
	if (context->State == AI_STATE_NEXT_OBJECTIVE)
	{
		bool hasNoUpdates = true;
		if (objState->Type == AI_OBJECTIVE_TYPE_KEY)
		{
			hasNoUpdates = objState->LastDone == KeycardCount(gMission.flags);
		}
		else if (objState->Type == AI_OBJECTIVE_TYPE_NORMAL)
		{
			hasNoUpdates =
				objState->Obj && objState->LastDone == objState->Obj->done;
		}
		if (hasNoUpdates)
		{
			*cmdOut = GotoObjective(actor, 0);
			return true;
		}
	}

	// First, check if mission complete; if so go to exit
	if (CanCompleteMission(&gMission))
	{
		context->State = AI_STATE_NEXT_OBJECTIVE;
		objState->Type = AI_OBJECTIVE_TYPE_EXIT;
		objState->Goal = MapGetExitPos(&gMap);
		*cmdOut = GotoObjective(actor, 0);
		return true;
	}

	int closestObjectiveDistance = -1;
	const struct Objective *closestObjective = NULL;
	Vec2i closestObjectivePos = Vec2iZero();
	bool closestObjectiveDestructible = false;
	AIObjectiveType closestType = AI_OBJECTIVE_TYPE_NORMAL;

	// Look for pickups and destructibles
	for (int i = 0; i < (int)gObjs.size; i++)
	{
		const TObject *o = CArrayGet(&gObjs, i);
		if (!o->isInUse)
		{
			continue;
		}
		const Vec2i objPos = Vec2iNew(o->tileItem.x, o->tileItem.y);
		bool isObjective = false;
		bool isDestructible = false;
		AIObjectiveType type = AI_OBJECTIVE_TYPE_NORMAL;
		switch (o->Type)
		{
		case OBJ_KEYCARD_YELLOW:	// fallthrough
		case OBJ_KEYCARD_GREEN:	// fallthrough
		case OBJ_KEYCARD_BLUE:	// fallthrough
		case OBJ_KEYCARD_RED:	// fallthrough
			type = AI_OBJECTIVE_TYPE_KEY;	// fallthrough
		case OBJ_JEWEL:
			isObjective = true;
			isDestructible = false;
			break;
		case OBJ_NONE:
			if (o->tileItem.flags & TILEITEM_OBJECTIVE)
			{
				// Destructible objective; go towards it and fire
				isObjective = true;
				isDestructible = true;
			}
			break;
		default:
			// do nothing
			break;
		}
		if (isObjective && CanGetObjective(
			objPos, actorRealPos, closestPlayer, distanceTooFarFromPlayer))
		{
			const int objDistance = DistanceSquared(actorRealPos, objPos);
			if (closestObjectiveDistance == -1 ||
				closestObjectiveDistance > objDistance)
			{
				closestObjectiveDistance = objDistance;
				closestType = type;
				if (type == AI_OBJECTIVE_TYPE_NORMAL)
				{
					int objective = ObjectiveFromTileItem(o->tileItem.flags);
					closestObjective =
						CArrayGet(&gMission.Objectives, objective);
				}
				closestObjectivePos = objPos;
				closestObjectiveDestructible = isDestructible;
			}
		}
	}

	// Look for kill or rescue objectives
	for (int i = 0; i < (int)gActors.size; i++)
	{
		const TActor *a = CArrayGet(&gActors, i);
		if (!a->isInUse)
		{
			continue;
		}
		const TTileItem *ti = &a->tileItem;
		if (!(ti->flags & TILEITEM_OBJECTIVE))
		{
			continue;
		}
		int objective = ObjectiveFromTileItem(ti->flags);
		MissionObjective *mo =
			CArrayGet(&gMission.missionData->Objectives, objective);
		if (mo->Type != OBJECTIVE_KILL &&
			mo->Type != OBJECTIVE_RESCUE)
		{
			continue;
		}
		// Only rescue those that need to be rescued
		if (mo->Type == OBJECTIVE_RESCUE && !(a->flags & FLAGS_PRISONER))
		{
			continue;
		}
		const Vec2i objPos = Vec2iNew(ti->x, ti->y);
		if (CanGetObjective(
			objPos, actorRealPos, closestPlayer, distanceTooFarFromPlayer))
		{
			const int objDistance = DistanceSquared(actorRealPos, objPos);
			if (closestObjectiveDistance == -1 ||
				closestObjectiveDistance > objDistance)
			{
				closestObjectiveDistance = objDistance;
				closestType = AI_OBJECTIVE_TYPE_NORMAL;
				closestObjective = CArrayGet(&gMission.Objectives, objective);
				closestObjectivePos = objPos;
				closestObjectiveDestructible = false;
			}
		}
	}

	// Look for explore objectives
	for (int i = 0; i < (int)gMission.missionData->Objectives.size; i++)
	{
		MissionObjective *mo = CArrayGet(&gMission.missionData->Objectives, i);
		if (mo->Type != OBJECTIVE_INVESTIGATE)
		{
			continue;
		}
		struct Objective *o = CArrayGet(&gMission.Objectives, i);
		if (o->done >= mo->Required)
		{
			continue;
		}
		// Find the nearest unexplored tile
		// Search using an expanding box pattern
		const Vec2i actorTile = Vec2iToTile(actorRealPos);
		for (int radius = 2; ; radius++)
		{
			Vec2i tile;
			for (tile.x = actorTile.x - radius;
				tile.x <= actorTile.x + radius;
				tile.x++)
			{
				if (tile.x < 0)
				{
					continue;
				}
				if (tile.x >= gMap.Size.x)
				{
					break;
				}
				// Check top and bottom of box

			}
		}
	}

	if (closestObjectiveDistance != -1)
	{
		context->State = AI_STATE_NEXT_OBJECTIVE;
		objState->Type = closestType;
		objState->IsDestructible = closestObjectiveDestructible;
		if (closestType == AI_OBJECTIVE_TYPE_KEY)
		{
			objState->LastDone = KeycardCount(gMission.flags);
		}
		else if (closestType == AI_OBJECTIVE_TYPE_NORMAL)
		{
			objState->Obj = closestObjective;
			objState->LastDone = closestObjective->done;
		}
		objState->Goal = closestObjectivePos;
		*cmdOut = GotoObjective(actor, closestObjectiveDistance);
		return true;
	}
	return false;
}
static bool IsPosCloseEnoughToPlayer(
	const Vec2i realPos, const TActor *player,
	const int distanceTooFarFromPlayer);
static bool CanGetObjective(
	const Vec2i objRealPos, const Vec2i actorRealPos, const TActor *player,
	const int distanceTooFarFromPlayer)
{
	// Check if we can complete an objective.
	// If we don't need to follow any player, then only check that we can
	// pathfind to it.
	if (!player)
	{
		return AIHasPath(actorRealPos, objRealPos, true);
	}
	// If we need to stick close to a player, then check that it is both:
	// - Close enough from the lead player
	// - In line of sight from us
	if (!IsPosCloseEnoughToPlayer(
		objRealPos, player, distanceTooFarFromPlayer))
	{
		return false;
	}
	return AIHasClearPath(actorRealPos, objRealPos, true);
}
static bool IsPosCloseEnoughToPlayer(
	const Vec2i realPos, const TActor *player,
	const int distanceTooFarFromPlayer)
{
	if (!player)
	{
		return true;
	}
	return DistanceSquared(realPos, Vec2iFull2Real(player->Pos)) <
		distanceTooFarFromPlayer*distanceTooFarFromPlayer * 16 * 16;
}
// Navigate to the current objective
// If the objective is destructible and it makes sense to shoot it,
// shoot at it as well
static int GotoObjective(TActor *actor, int objDistance)
{
	const AIObjectiveState *objState = &actor->aiContext->ObjectiveState;
	Vec2i goal = objState->Goal;
	int cmd = SmartGoto(actor, goal, objDistance);
	if (objState->Type == AI_OBJECTIVE_TYPE_NORMAL &&
		objState->IsDestructible &&
		actor->weapon.lock <= 0 &&
		AIHasClearShot(Vec2iFull2Real(actor->Pos), goal))
	{
		cmd |= CMD_BUTTON1;
	}
	return cmd;
}

gun_e AICoopSelectWeapon(int player, int weapons[GUN_COUNT])
{
	// Weapon preferences, for different player indices
#define NUM_PREFERRED_WEAPONS 5
	gun_e preferredWeapons[][NUM_PREFERRED_WEAPONS] =
	{
		{ GUN_MG, GUN_SHOTGUN, GUN_POWERGUN, GUN_SNIPER, GUN_FLAMER },
		{ GUN_FLAMER, GUN_SHOTGUN, GUN_MG, GUN_POWERGUN, GUN_SNIPER },
		{ GUN_SHOTGUN, GUN_MG, GUN_POWERGUN, GUN_FLAMER, GUN_SNIPER },
		{ GUN_POWERGUN, GUN_SNIPER, GUN_MG, GUN_SHOTGUN, GUN_FLAMER }
	};
	int i;

	// First try to select an available weapon
	for (i = 0; i < NUM_PREFERRED_WEAPONS; i++)
	{
		gun_e preferredWeapon = preferredWeapons[player][i];
		if (weapons[preferredWeapon])
		{
			return preferredWeapon;
		}
	}

	// Preferred weapons unavailable;
	// give up and select most preferred weapon anyway
	return preferredWeapons[player][0];
}
