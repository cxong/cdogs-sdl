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
#include "gamedata.h"
#include "pickup.h"

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
				AIContextSetState(actor->aiContext, AI_STATE_CONFUSED);
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
		if ((cmd & CMD_BUTTON2) && CMD_HAS_DIRECTION(cmd))
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

	// First, check the weapon
	// If we're out of ammo, switch to one with ammo
	// Prioritise our selection based on weapon order, i.e.
	// prefer first weapon over second over third etc.
	if (ConfigGetBool(&gConfig, "Game.Ammo"))
	{
		int mostPreferredWeaponWithAmmo = 0;
		for (int i = 0; i < (int)actor->guns.size; i++)
		{
			const Weapon *w = CArrayGet(&actor->guns, i);
			const bool hasAmmo = ActorGunGetAmmo(actor, w) != 0;
			if (hasAmmo)
			{
				break;
			}
			mostPreferredWeaponWithAmmo++;
		}
		if (mostPreferredWeaponWithAmmo != actor->gunIndex)
		{
			return actor->lastCmd == CMD_BUTTON2 ? 0 : CMD_BUTTON2;
		}
	}

	const Vec2i actorRealPos = Vec2iFull2Real(actor->Pos);

	// Follow the closest player with a lower ID
	const TActor *closestPlayer = NULL;
	int minDistance2 = -1;
	if (!IsPVP(gCampaign.Entry.Mode))
	{
		for (int i = 0; i < actor->playerIndex; i++)
		{
			const PlayerData *pd = CArrayGet(&gPlayerDatas, i);
			if (!IsPlayerAlive(pd))
			{
				continue;
			}
			const TActor *p = CArrayGet(&gActors, pd->Id);
			const int distance2 =
				DistanceSquared(actorRealPos, Vec2iFull2Real(p->Pos));
			if (!closestPlayer || distance2 < minDistance2)
			{
				minDistance2 = distance2;
				closestPlayer = p;
			}
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
		AIContextSetState(actor->aiContext, AI_STATE_FOLLOW);
		return SmartGoto(actor, Vec2iFull2Real(closestPlayer->Pos), minDistance2);
	}

	// Check if closest enemy is close enough, and visible
	const TActor *closestEnemy = AIGetClosestVisibleEnemy(actor, true);
	if (closestEnemy)
	{
		int minEnemyDistance = CHEBYSHEV_DISTANCE(
			actor->Pos.x, actor->Pos.y,
			closestEnemy->Pos.x, closestEnemy->Pos.y);
		// Also only engage if there's a clear shot
		if (minEnemyDistance > 0 && minEnemyDistance < ((12 * 16) << 8) &&
			AIHasClearShot(actorRealPos, Vec2iFull2Real(closestEnemy->Pos)))
		{
			AIContextSetState(actor->aiContext, AI_STATE_HUNT);
			if (closestEnemy->uid != actor->aiContext->EnemyId)
			{
				// Engaging new enemy now
				actor->aiContext->EnemyId = closestEnemy->uid;
				actor->aiContext->GunRangeScalar = 1.0;
			}
			else
			{
				// Still attacking the same enemy; inch closer
				actor->aiContext->GunRangeScalar *= 0.99;
			}
			// Move to the ideal distance for the weapon
			int cmd = 0;
			const int gunRange = GunGetRange(ActorGetGun(actor)->Gun);
			const int distanceSquared = DistanceSquared(
				Vec2iFull2Real(actor->Pos), Vec2iFull2Real(closestEnemy->Pos));
			const bool canFire = ActorGetGun(actor)->lock <= 0;
			if ((double)distanceSquared >
				SQUARED(gunRange * 3 / 4) * actor->aiContext->GunRangeScalar)
			{
				// Move towards the enemy, fire if able
				// But don't bother firing if too far away

#define MINIMUM_GUN_DISTANCE 30
				// Cap the minimum distance for moving away; if we are extremely
				// close to the target, fire anyway regardless of the gun range.
				// This is to prevent us from not firing with a melee-range gun.
				if (canFire &&
					(distanceSquared < SQUARED(gunRange * 2) ||
					distanceSquared > SQUARED(MINIMUM_GUN_DISTANCE)))
				{
					cmd = AIHunt(actor, closestEnemy->Pos) | CMD_BUTTON1;
				}
				else
				{
					// Track so that we end up in a favorable angle
					cmd = AITrack(actor, closestEnemy->Pos);
				}
			}
			else if ((double)distanceSquared <
				SQUARED(gunRange * 3) * actor->aiContext->GunRangeScalar &&
				!canFire)
			{
				// Move away from the enemy because we're too close
				// Only move away if we can't fire; otherwise turn to fire
				cmd = AIRetreatFrom(actor, closestEnemy->Pos);
			}
			else if (canFire)
			{
				// We're not too close and not too far; fire if able
				cmd = AIHunt(actor, closestEnemy->Pos) | CMD_BUTTON1;
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
		AIContextSetState(actor->aiContext, AI_STATE_FOLLOW);
		return SmartGoto(actor, Vec2iFull2Real(closestPlayer->Pos), minDistance2);
	}

	AIContextSetState(actor->aiContext, AI_STATE_IDLE);
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
	if (CMD_HAS_DIRECTION(cmd) &&
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
	if (o && ObjIsDangerous(o) &&
		Vec2iEqual(tilePos, actor->aiContext->LastTile))
	{
		cmd = AIGoto(
			actor, Vec2iNew(o->tileItem.x, o->tileItem.y), true);
		if (ActorGetGun(actor)->lock <= 0)
		{
			cmd |= CMD_BUTTON1;
		}
	}
	// Check if we are stuck
	if (Vec2iEqual(tilePos, actor->aiContext->LastTile))
	{
		actor->aiContext->Delay++;
		if (actor->aiContext->Delay >= STUCK_TICKS)
		{
			// We've been stuck for too long
			// Pathfind around it
			// Make sure to reset the A* path if we are first realising
			// we are stuck
			if (!actor->aiContext->IsStuckTooLong)
			{
				actor->aiContext->Goto.IsFollowing = false;
			}
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
typedef struct
{
	int Distance;
	const ObjectiveDef *Objective;
	Vec2i Pos;
	bool IsDestructible;
	AIObjectiveType Type;
} ClosestObjective;
static void FindObjectivesSortedByDistance(
	CArray *objectives, const Vec2i actorRealPos);
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

	// If PVP, just find the closest enemy and go to them
	if (IsPVP(gCampaign.Entry.Mode))
	{
		const TActor *closestEnemy =
			AIGetClosestVisibleEnemy(actor, true);
		if (closestEnemy == NULL)
		{
			return false;
		}
		objState->Type = AI_OBJECTIVE_TYPE_NORMAL;
		objState->Goal = Vec2iFull2Real(closestEnemy->Pos);
		*cmdOut = GotoObjective(actor, 0);
		return true;
	}

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
			int cmd = GotoObjective(actor, 0);
			// If we can't go to the objective,
			// one possibility is that we've reached where the objective was,
			// but have nothing to do.
			// Remember that actor objectives can move around!
			if (CMD_HAS_DIRECTION(cmd))
			{
				*cmdOut = cmd;
				return true;
			}
		}
	}

	// First, check if mission complete;
	// if so (and there's a path) go to exit
	const Vec2i exitPos = MapGetExitPos(&gMap);
	if (CanCompleteMission(&gMission) && CanGetObjective(
		exitPos, actorRealPos, closestPlayer, distanceTooFarFromPlayer))
	{
		AIContextSetState(actor->aiContext, AI_STATE_NEXT_OBJECTIVE);
		objState->Type = AI_OBJECTIVE_TYPE_EXIT;
		objState->Goal = exitPos;
		*cmdOut = GotoObjective(actor, 0);
		return true;
	}

	// Find all the objective/key locations, sort according to distance
	// TODO: reuse this array, don't recreate it
	CArray objectives;
	FindObjectivesSortedByDistance(&objectives, actorRealPos);

	// Starting from the closest objectives, find one we can go to
	for (int i = 0; i < (int)objectives.size; i++)
	{
		ClosestObjective *c = CArrayGet(&objectives, i);
		if (CanGetObjective(
			c->Pos, actorRealPos, closestPlayer, distanceTooFarFromPlayer))
		{
			AIContextSetState(actor->aiContext, AI_STATE_NEXT_OBJECTIVE);
			objState->Type = c->Type;
			objState->IsDestructible = c->IsDestructible;
			if (c->Type == AI_OBJECTIVE_TYPE_KEY)
			{
				objState->LastDone = KeycardCount(gMission.flags);
			}
			else if (c->Type == AI_OBJECTIVE_TYPE_NORMAL)
			{
				objState->Obj = c->Objective;
				objState->LastDone = c->Objective->done;
			}
			objState->Goal = c->Pos;
			*cmdOut = GotoObjective(actor, c->Distance);
			return true;
		}
	}
	return false;
}
static int CompareClosestObjective(const void *v1, const void *v2);
static void FindObjectivesSortedByDistance(
	CArray *objectives, const Vec2i actorRealPos)
{
	CArrayInit(objectives, sizeof(ClosestObjective));

	// Look for pickups
	for (int i = 0; i < (int)gPickups.size; i++)
	{
		const Pickup *p = CArrayGet(&gPickups, i);
		if (!p->isInUse)
		{
			continue;
		}
		ClosestObjective co;
		memset(&co, 0, sizeof co);
		co.Pos = Vec2iNew(p->tileItem.x, p->tileItem.y);
		co.IsDestructible = false;
		co.Type = AI_OBJECTIVE_TYPE_NORMAL;
		bool isObjective = false;
		switch (p->class->Type)
		{
		case PICKUP_KEYCARD:	// fallthrough
			co.Type = AI_OBJECTIVE_TYPE_KEY;	// fallthrough
		case PICKUP_JEWEL:
			isObjective = true;
			break;
		default:
			// do nothing
			break;
		}
		if (!isObjective)
		{
			continue;
		}
		co.Distance = DistanceSquared(actorRealPos, co.Pos);
		if (co.Type == AI_OBJECTIVE_TYPE_NORMAL)
		{
			const int objective = ObjectiveFromTileItem(p->tileItem.flags);
			co.Objective =
				CArrayGet(&gMission.Objectives, objective);
		}
		CArrayPushBack(objectives, &co);
	}

	// Look for destructibles
	for (int i = 0; i < (int)gObjs.size; i++)
	{
		const TObject *o = CArrayGet(&gObjs, i);
		if (!o->isInUse)
		{
			continue;
		}
		ClosestObjective co;
		memset(&co, 0, sizeof co);
		co.Pos = Vec2iNew(o->tileItem.x, o->tileItem.y);
		co.IsDestructible = true;
		co.Type = AI_OBJECTIVE_TYPE_NORMAL;
		if (!(o->tileItem.flags & TILEITEM_OBJECTIVE))
		{
			continue;
		}
		// Destructible objective; go towards it and fire
		co.Distance = DistanceSquared(actorRealPos, co.Pos);
		if (co.Type == AI_OBJECTIVE_TYPE_NORMAL)
		{
			const int objective = ObjectiveFromTileItem(o->tileItem.flags);
			co.Objective =
				CArrayGet(&gMission.Objectives, objective);
		}
		CArrayPushBack(objectives, &co);
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
		ClosestObjective co;
		memset(&co, 0, sizeof co);
		co.Pos = Vec2iNew(ti->x, ti->y);
		co.IsDestructible = false;
		co.Type = AI_OBJECTIVE_TYPE_NORMAL;
		co.Distance = DistanceSquared(actorRealPos, co.Pos);
		co.Objective = CArrayGet(&gMission.Objectives, objective);
		CArrayPushBack(objectives, &co);
	}

	// Look for explore objectives
	for (int i = 0; i < (int)gMission.missionData->Objectives.size; i++)
	{
		MissionObjective *mo = CArrayGet(&gMission.missionData->Objectives, i);
		if (mo->Type != OBJECTIVE_INVESTIGATE)
		{
			continue;
		}
		const ObjectiveDef *o = CArrayGet(&gMission.Objectives, i);
		if (o->done >= mo->Required || MapGetExploredPercentage(&gMap) == 100)
		{
			continue;
		}
		// Find the nearest unexplored tile
		const Vec2i actorTile = Vec2iToTile(actorRealPos);
		const Vec2i unexploredTile = MapSearchTileAround(
			&gMap, actorTile, MapTileIsUnexplored);
		ClosestObjective co;
		memset(&co, 0, sizeof co);
		co.Pos = Vec2iCenterOfTile(unexploredTile);
		co.IsDestructible = false;
		co.Type = AI_OBJECTIVE_TYPE_NORMAL;
		co.Distance = DistanceSquared(actorRealPos, co.Pos);
		co.Objective = o;
		CArrayPushBack(objectives, &co);
	}

	// Sort according to distance
	qsort(
		objectives->data,
		objectives->size, objectives->elemSize, CompareClosestObjective);
}
static int CompareClosestObjective(const void *v1, const void *v2)
{
	const ClosestObjective *c1 = v1;
	const ClosestObjective *c2 = v2;
	if (c1->Distance < c2->Distance)
	{
		return -1;
	}
	else if (c1->Distance > c2->Distance)
	{
		return 1;
	}
	return 0;
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
	const Vec2i goal = objState->Goal;
	int cmd = 0;
	// Check to see if we need to go any closer to the objective
	const bool isDestruction =
		objState->Type == AI_OBJECTIVE_TYPE_NORMAL && objState->IsDestructible;
	if (!isDestruction ||
		DistanceSquared(Vec2iFull2Real(actor->Pos), goal) > 3 * 16 * 16 ||
		!AIHasClearShot(Vec2iFull2Real(actor->Pos), goal))
	{
		cmd = SmartGoto(actor, goal, objDistance);
	}
	else if (isDestruction && ActorGetGun(actor)->lock <= 0)
	{
		cmd = AIHunt(actor, Vec2iReal2FullCentered(goal));
		if (AIHasClearShot(Vec2iFull2Real(actor->Pos), goal))
		{
			 cmd |= CMD_BUTTON1;
		}
	}
	return cmd;
}

static bool PlayerHasWeapon(const PlayerData *p, const GunDescription *w);
void AICoopSelectWeapons(
	PlayerData *p, const int player, const CArray *weapons)
{
	p->weaponCount = 0;

	// Select two weapons from available ones
	// Offset the starting index a bit so each AI uses different guns
	const int startIdx = (player * 3) % weapons->size;
	for (int i = 0; i < MAX_WEAPONS; i++)
	{
		const int idx = (startIdx + i) % weapons->size;
		if (idx == startIdx && i > 0)
		{
			// Not enough weapons
			break;
		}
		const GunDescription **g = CArrayGet(weapons, idx);
		p->weapons[i] = *g;
		p->weaponCount++;
	}

	if (ConfigGetBool(&gConfig, "Game.Ammo"))
	{
		// Select pistol as an infinite-ammo backup
		const GunDescription *pistol = StrGunDescription("Pistol");
		if (!PlayerHasWeapon(p, pistol))
		{
			if (p->weaponCount == MAX_WEAPONS)
			{
				// Player has full weapons; replace last weapon
				p->weapons[MAX_WEAPONS - 1] = pistol;
			}
			else
			{
				// Add the pistol
				p->weapons[p->weaponCount] = pistol;
				p->weaponCount++;
			}
		}
	}
}
static bool PlayerHasWeapon(const PlayerData *p, const GunDescription *w)
{
	for (int i = 0; i < MAX_WEAPONS; i++)
	{
		const GunDescription *g = p->weapons[i];
		if (w == g)
		{
			return true;
		}
	}
	return false;
}
