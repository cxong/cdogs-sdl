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

	// Check if closest player is too far away, and follow him/her if so
	// Also calculate the squad number, so we have a formation
	// i.e. bigger squad number members follow at a further distance
	TActor *closestPlayer = NULL;
	TActor *closestEnemy;
	int minDistance2 = -1;
	int distanceTooFarFromPlayer = 8;
	int minEnemyDistance;
	int i;
	int squadNumber = 0;
	for (i = 0; i < gOptions.numPlayers; i++)
	{
		if (!IsPlayerAlive(i))
		{
			continue;
		}
		if (gPlayerDatas[i].inputDevice != INPUT_DEVICE_AI)
		{
			TActor *p = CArrayGet(&gActors, gPlayerIds[i]);
			int distance2 = DistanceSquared(
				Vec2iFull2Real(actor->Pos), Vec2iFull2Real(p->Pos));
			if (!closestPlayer || distance2 < minDistance2)
			{
				minDistance2 = distance2;
				closestPlayer = p;
			}
		}
		else if (actor->pData->playerIndex > i)
		{
			squadNumber++;
		}
	}
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
	closestEnemy = AIGetClosestVisibleEnemy(actor->Pos, actor->flags, 1);
	if (closestEnemy)
	{
		minEnemyDistance = CHEBYSHEV_DISTANCE(
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
	if (closestPlayer &&
		minDistance2 > 4*4*16*16/3/3*(squadNumber+1)*(squadNumber+1))
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
		actor->aiContext->Delay++;
		if (actor->aiContext->Delay >= STUCK_TICKS)
		{
			// We've been stuck for too long
			// Pathfind around it
			actor->aiContext->IsStuckTooLong = true;
			cmd = AIGoto(actor, realPos, !actor->aiContext->IsStuckTooLong);
		}
		else
		{
			cmd = AIGoto(
				actor, Vec2iNew(o->tileItem.x, o->tileItem.y), true) |
				CMD_BUTTON1;
		}
	}
	else if (!Vec2iEqual(tilePos, actor->aiContext->LastTile))
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
static bool TryCompleteNearbyObjective(
	TActor *actor, const TActor *closestPlayer,
	const int distanceTooFarFromPlayer, int *cmdOut)
{
	const Vec2i actorRealPos = Vec2iFull2Real(actor->Pos);
	int closestObjectiveDistance = -1;
	Vec2i closestObjectivePos = Vec2iZero();
	bool closestObjectiveDestructible = false;
	AIState closestObjectiveState = AI_STATE_IDLE;
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
		AIState objectiveState = STATE_IDLE;
		switch (o->Type)
		{
		case OBJ_JEWEL:	// fallthrough
		case OBJ_KEYCARD_YELLOW:	// fallthrough
		case OBJ_KEYCARD_GREEN:	// fallthrough
		case OBJ_KEYCARD_BLUE:	// fallthrough
		case OBJ_KEYCARD_RED:	// fallthrough
			isObjective = true;
			isDestructible = false;
			objectiveState = AI_STATE_COLLECT;
			break;
		case OBJ_NONE:
			if (o->tileItem.flags & TILEITEM_OBJECTIVE)
			{
				// Destructible objective; go towards it and fire
				isObjective = true;
				isDestructible = true;
				objectiveState = AI_STATE_DESTROY;
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
				// TODO: have a "going to objective" state
				closestObjectiveDistance = objDistance;
				closestObjectivePos = objPos;
				closestObjectiveDestructible = isDestructible;
				closestObjectiveState = objectiveState;
			}
		}
	}
	if (closestObjectiveDistance != -1)
	{
		actor->aiContext->State = closestObjectiveState;
		*cmdOut = SmartGoto(
			actor, closestObjectivePos, closestObjectiveDistance); 
		if (closestObjectiveDestructible)
		{
			*cmdOut |= CMD_BUTTON1;
		}
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
	// We can complete an objective if it is both:
	// - Close enough from the lead player
	// - In line of sight from us
	if (!IsPosCloseEnoughToPlayer(
		objRealPos, player, distanceTooFarFromPlayer))
	{
		return false;
	}
	return AIHasClearPath(actorRealPos, objRealPos, false);
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
