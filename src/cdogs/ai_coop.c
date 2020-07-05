/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.

	Copyright (c) 2013-2015, 2017-2018, 2020 Cong Xu
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

#define DISTANCE_TO_KEEP_OUT_OF_WAY SQUARED(4 * 16)

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
			actor->aiContext->Delay = CONFUSION_STATE_TICKS_MIN +
									  (rand() % CONFUSION_STATE_TICKS_RANGE);
			if (s->Type == AI_CONFUSION_CONFUSED)
			{
				s->Type = AI_CONFUSION_CORRECT;
			}
			else
			{
				ActorSetAIState(actor, AI_STATE_CONFUSED);
				s->Type = AI_CONFUSION_CONFUSED;
				// Generate the confused action
				s->Cmd = rand() & (CMD_LEFT | CMD_RIGHT | CMD_UP | CMD_DOWN |
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

static int SmartGoto(
	TActor *actor, const struct vec2 pos, const float minDistance2);
static bool TryCompleteNearbyObjective(
	TActor *actor, const TActor *closestPlayer,
	const float distanceTooFarFromPlayer, int *cmdOut);
static int AICoopGetCmdNormal(TActor *actor)
{
	// Use decision tree to command the AI
	// - Move away from dangerous bullets
	// - Check weapons and ammo
	// - If too far away from nearest player
	//   - Go to nearest player
	// - else if blocking a nearby player
	//   - move out of the way
	// - else if closest enemy is close enough
	//   - Attack enemy
	// - else
	//   - Go to nearest player if too far, or away if too close
	const struct vec2i actorTilePos = Vec2ToTile(actor->Pos);

	// Look for dangerous bullets in a 1-tile radius
	// These are bullets with the "HurtAlways" property true
	struct vec2i v;
	struct vec2 dangerBulletPos = svec2_zero();
	for (v.x = actorTilePos.x - 1;
		 v.x <= actorTilePos.x + 1 && svec2_is_zero(dangerBulletPos); v.x++)
	{
		for (v.y = actorTilePos.y - 1;
			 v.y <= actorTilePos.y + 1 && svec2_is_zero(dangerBulletPos);
			 v.y++)
		{
			const Tile *t = MapGetTile(&gMap, v);
			if (t == NULL)
				continue;
			CA_FOREACH(const ThingId, tid, t->things)
			// Only look for bullets
			if (tid->Kind != KIND_MOBILEOBJECT)
				continue;
			const TMobileObject *mo = CArrayGet(&gMobObjs, tid->Id);
			if (mo->bulletClass->HurtAlways)
			{
				dangerBulletPos = mo->thing.Pos;
				break;
			}
			CA_FOREACH_END()
		}
	}
	// Run away if dangerous bullet found
	if (!svec2_is_zero(dangerBulletPos))
	{
		return AIRetreatFrom(actor, dangerBulletPos);
	}

	// Check the weapon for ammo
	int lowAmmoGun = -1;
	if (gCampaign.Setting.Ammo && actor->aiContext->OnGunId == -1)
	{
		// Check all our weapons
		// Prefer guns using ammo
		// This is because unlimited ammo guns tend to be worse
		int mostPreferredWeaponWithAmmo = -1;
		bool mostPreferredWeaponHasAmmo = false;
		for (int i = 0; i < MAX_GUNS; i++)
		{
			const Weapon *w = &actor->guns[i];
			if (w->Gun == NULL)
			{
				continue;
			}
			const int ammoAmount = ActorWeaponGetAmmo(actor, w->Gun);
			if ((mostPreferredWeaponWithAmmo == -1 && ammoAmount != 0) ||
				(!mostPreferredWeaponHasAmmo && ammoAmount > 0))
			{
				// Note: -1 means the gun has unlimited ammo
				mostPreferredWeaponWithAmmo = i;
				mostPreferredWeaponHasAmmo = ammoAmount > 0;
			}
			if (w->Gun->AmmoId != -1)
			{
				const Ammo *ammo = AmmoGetById(&gAmmo, w->Gun->AmmoId);
				if (lowAmmoGun == -1 && ammoAmount < ammo->Amount)
				{
					lowAmmoGun = i;
				}
			}
		}

		// If we're out of ammo, switch to one with ammo
		// Prioritise our selection based on weapon order, i.e.
		// prefer first weapon over second over third etc.
		if (mostPreferredWeaponWithAmmo != -1 &&
			mostPreferredWeaponWithAmmo != actor->gunIndex)
		{
			return actor->lastCmd == CMD_BUTTON2 ? 0 : CMD_BUTTON2;
		}
	}

	// If we're over a weapon pickup, check if we want to pick it up
	// Pick up if we have:
	// - a weapon that is low on ammo and don't yet have the gun, or
	// - if we have a free slot
	if (actor->aiContext->OnGunId != -1)
	{
		const WeaponClass *wc = IdWeaponClass(actor->aiContext->OnGunId);
		if ((lowAmmoGun != -1 && !ActorHasGun(actor, wc)) ||
			(wc->IsGrenade ? ActorGetNumGrenades(actor) < MAX_GRENADES
						   : ActorGetNumGuns(actor) < MAX_GUNS))
		{
			actor->aiContext->OnGunId = -1;
			// Pick it up
			// TODO: this will not guarantee the new gun will replace
			// the low ammo gun; weapon management?
			return actor->lastCmd == CMD_BUTTON2 ? 0 : CMD_BUTTON2;
		}
		actor->aiContext->OnGunId = -1;
	}

	// Find nearby players, and move out of the way if any of them are facing
	// directly at us
	// Follow the closest player with a lower UID
	const TActor *closestPlayer = NULL;
	float minDistance2 = -1;
	if (!IsPVP(gCampaign.Entry.Mode))
	{
		for (int uid = 0; uid < actor->PlayerUID; uid++)
		{
			const PlayerData *pd = PlayerDataGetByUID(uid);
			if (pd == NULL || !IsPlayerAlive(pd))
				continue;
			const TActor *p = ActorGetByUID(pd->ActorUID);
			const float distance2 = svec2_distance_squared(actor->Pos, p->Pos);
			if (!closestPlayer || distance2 < minDistance2)
			{
				minDistance2 = distance2;
				closestPlayer = p;
			}
			// TODO: AIIsFacing might be too generous?
			if (distance2 < DISTANCE_TO_KEEP_OUT_OF_WAY &&
				AIIsFacing(p, actor->Pos, p->direction))
			{
				// Move out of the way
				return AIMoveAwayFromLine(actor->Pos, p->Pos, p->direction);
			}
		}
	}

	// Set distance we want to stay within the lead player
	float distanceTooFarFromPlayer = 8.0f;
	// If player is exiting, we want to be very close to the player
	if (closestPlayer && closestPlayer->action == ACTORACTION_EXITING)
	{
		distanceTooFarFromPlayer = 2.0f;
	}
	if (closestPlayer && minDistance2 > SQUARED(distanceTooFarFromPlayer * 16))
	{
		ActorSetAIState(actor, AI_STATE_FOLLOW);
		return SmartGoto(actor, closestPlayer->Pos, minDistance2);
	}

	// Check if closest enemy is close enough, and visible
	const TActor *closestEnemy = AIGetClosestVisibleEnemy(actor, true);
	if (closestEnemy)
	{
		const float minEnemyDistance = CHEBYSHEV_DISTANCE(
			actor->Pos.x, actor->Pos.y, closestEnemy->Pos.x,
			closestEnemy->Pos.y);
		// Also only engage if there's a clear shot
		if (minEnemyDistance > 0 && minEnemyDistance < 12 * 16 &&
			AIHasClearShot(actor->Pos, closestEnemy->Pos))
		{
			ActorSetAIState(actor, AI_STATE_HUNT);
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
			return AIAttack(actor, closestEnemy->Pos);
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
	// run into them, but keep away if we're too close
	if (closestPlayer)
	{
		if (minDistance2 > SQUARED(2 * 16))
		{
			ActorSetAIState(actor, AI_STATE_FOLLOW);
			return SmartGoto(actor, closestPlayer->Pos, minDistance2);
		}
		else if (minDistance2 < SQUARED(4 * 16 / 3))
		{
			return CmdGetReverse(AIGotoDirect(actor->Pos, closestPlayer->Pos));
		}
	}

	ActorSetAIState(actor, AI_STATE_IDLE);
	return 0;
}
// Number of ticks to persist in trying to destroy an obstruction
// before giving up and going around
#define STUCK_TICKS 70
// Goto with extra smarts:
// - If clear path, slide
// - If non-dangerous object blocking, shoot at it
// - If stuck for a long time, pathfind around obstructing object
static int SmartGoto(
	TActor *actor, const struct vec2 pos, const float minDistance2)
{
	int cmd = AIGoto(actor, pos, true);
	// Try to slide if there is a clear path and we are far enough away
	if (CMD_HAS_DIRECTION(cmd) &&
		AIHasClearPath(actor->Pos, pos, !actor->aiContext->IsStuckTooLong) &&
		minDistance2 > SQUARED(7 * 16))
	{
		cmd |= CMD_BUTTON2;
	}
	// If running into safe object, and we're being blocked, shoot at it
	const TObject *o = AIGetObjectRunningInto(actor, cmd);
	const struct vec2i tilePos = Vec2ToTile(actor->Pos);
	if (o && ObjIsDangerous(o) &&
		svec2i_is_equal(tilePos, actor->aiContext->LastTile))
	{
		cmd = AIGoto(actor, o->thing.Pos, true);
		if (ACTOR_GET_WEAPON(actor)->lock <= 0)
		{
			cmd |= CMD_BUTTON1;
		}
	}
	// Check if we are stuck
	if (svec2i_is_equal(tilePos, actor->aiContext->LastTile))
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
			cmd = AIGoto(actor, pos, !actor->aiContext->IsStuckTooLong);
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
	float Distance2;
	union {
		const Objective *Objective;
		int UID;
	} u;
	struct vec2 Pos;
	bool IsDestructible;
	AIObjectiveType Type;
} ClosestObjective;
static void FindObjectivesSortedByDistance(
	CArray *objectives, const TActor *actor, const TActor *closestPlayer);
static bool CanGetObjective(
	const struct vec2 objPos, const struct vec2 actorPos, const TActor *player,
	const float distanceTooFarFromPlayer);
static int GotoObjective(TActor *actor, const float objDistance2);
static bool TryCompleteNearbyObjective(
	TActor *actor, const TActor *closestPlayer,
	const float distanceTooFarFromPlayer, int *cmdOut)
{
	AIContext *context = actor->aiContext;
	AIObjectiveState *objState = &context->ObjectiveState;

	// Check to see if we are already attempting to complete an objective,
	// and that it hasn't been updated since we last checked.
	// If so keep following the path
	if (context->State == AI_STATE_NEXT_OBJECTIVE)
	{
		bool hasNoUpdates = true;
		switch (objState->Type)
		{
		case AI_OBJECTIVE_TYPE_KEY:
			hasNoUpdates =
				objState->LastDone == KeycardCount(gMission.KeyFlags);
			break;
		case AI_OBJECTIVE_TYPE_NORMAL:
			hasNoUpdates =
				objState->u.Obj && objState->LastDone == objState->u.Obj->done;
			break;
		case AI_OBJECTIVE_TYPE_KILL: {
			const TActor *target = ActorGetByUID(objState->u.UID);
			hasNoUpdates = target->health > 0;
			// Update target position
			objState->Goal = target->thing.Pos;
		}
		break;
		case AI_OBJECTIVE_TYPE_PICKUP: {
			const Pickup *p = PickupGetByUID(objState->u.UID);
			hasNoUpdates = p != NULL && p->isInUse;
		}
		break;
		default:
			// Do nothing
			break;
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
	// if so (and there's a path) go to an exit
	if (CanCompleteMission(&gMission))
	{
		for (int i = 0; i < (int)gMap.exits.size; i++)
		{
			const struct vec2 exitPos = MapGetExitPos(&gMap, i);
			if (CanGetObjective(
					exitPos, actor->Pos, closestPlayer,
					distanceTooFarFromPlayer))
			{
				ActorSetAIState(actor, AI_STATE_NEXT_OBJECTIVE);
				objState->Type = AI_OBJECTIVE_TYPE_EXIT;
				objState->Goal = exitPos;
				*cmdOut = GotoObjective(actor, 0);
				return true;
			}
		}
	}

	// Find all the objective/key locations, sort according to distance
	// TODO: reuse this array, don't recreate it
	CArray objectives;
	FindObjectivesSortedByDistance(&objectives, actor, closestPlayer);

	// Starting from the closest objectives, find one we can go to
	CA_FOREACH(ClosestObjective, c, objectives)
	if (CanGetObjective(
			c->Pos, actor->Pos, closestPlayer, distanceTooFarFromPlayer))
	{
		ActorSetAIState(actor, AI_STATE_NEXT_OBJECTIVE);
		objState->Type = c->Type;
		objState->IsDestructible = c->IsDestructible;
		switch (c->Type)
		{
		case AI_OBJECTIVE_TYPE_KEY:
			objState->LastDone = KeycardCount(gMission.KeyFlags);
			break;
		case AI_OBJECTIVE_TYPE_NORMAL:
			objState->u.Obj = c->u.Objective;
			objState->LastDone = c->u.Objective->done;
			break;
		case AI_OBJECTIVE_TYPE_KILL:
			objState->u.UID = c->u.UID;
			break;
		case AI_OBJECTIVE_TYPE_PICKUP:
			objState->u.UID = c->u.UID;
			break;
		default:
			// Do nothing
			break;
		}
		objState->Goal = c->Pos;
		*cmdOut = GotoObjective(actor, c->Distance2);
		return true;
	}
	CA_FOREACH_END()
	return false;
}
static bool OnClosestPickupGun(
	ClosestObjective *co, const Pickup *p, const TActor *actor,
	const TActor *closestPlayer);
static int CompareClosestObjective(const void *v1, const void *v2);
static void FindObjectivesSortedByDistance(
	CArray *objectives, const TActor *actor, const TActor *closestPlayer)
{
	CArrayInit(objectives, sizeof(ClosestObjective));

	// If PVP, find the closest enemy and go to them
	if (IsPVP(gCampaign.Entry.Mode))
	{
		const TActor *closestEnemy = AIGetClosestVisibleEnemy(actor, true);
		if (closestEnemy != NULL)
		{
			ClosestObjective co;
			memset(&co, 0, sizeof co);
			co.Pos = closestEnemy->thing.Pos;
			co.IsDestructible = false;
			co.Type = AI_OBJECTIVE_TYPE_KILL;
			co.Distance2 = svec2_distance_squared(actor->Pos, co.Pos);
			co.u.UID = closestEnemy->uid;
			CArrayPushBack(objectives, &co);
		}
	}

	// Look for pickups
	CA_FOREACH(const Pickup, p, gPickups)
	if (!p->isInUse)
	{
		continue;
	}
	ClosestObjective co;
	memset(&co, 0, sizeof co);
	co.Pos = p->thing.Pos;
	co.IsDestructible = false;
	co.Type = AI_OBJECTIVE_TYPE_NORMAL;
	switch (p->class->Type)
	{
	case PICKUP_KEYCARD:
		co.Type = AI_OBJECTIVE_TYPE_KEY;
		break;
	case PICKUP_JEWEL:
		break;
	case PICKUP_HEALTH:
		// Pick up if we are on low health, and lower than lead player
		if (actor->health > ModeMaxHealth(gCampaign.Entry.Mode) / 4)
		{
			continue;
		}
		co.Type = AI_OBJECTIVE_TYPE_PICKUP;
		co.u.UID = p->UID;
		break;
	case PICKUP_AMMO: {
		// Pick up if we use this ammo, have less ammo than starting,
		// and lower than lead player, who uses the ammo
		const int ammoId = p->class->u.Ammo.Id;
		const Ammo *ammo = AmmoGetById(&gAmmo, ammoId);
		const int ammoAmount = *(int *)CArrayGet(&actor->ammo, ammoId);
		if (!ActorUsesAmmo(actor, ammoId) || ammoAmount > ammo->Amount ||
			(closestPlayer != NULL && ActorUsesAmmo(closestPlayer, ammoId) &&
			 ammoAmount > *(int *)CArrayGet(&closestPlayer->ammo, ammoId)))
		{
			continue;
		}
		co.Type = AI_OBJECTIVE_TYPE_PICKUP;
		co.u.UID = p->UID;
	}
	break;
	case PICKUP_GUN:
		if (!OnClosestPickupGun(&co, p, actor, closestPlayer))
		{
			continue;
		}
		break;
	default:
		// Not something we want to pick up
		continue;
	}
	// Check if the pickup is actually accessible
	// This is because random spawning may cause some pickups to be spawned
	// in inaccessible areas
	if (!TileCanWalk(MapGetTile(&gMap, Vec2ToTile(co.Pos))))
	{
		continue;
	}
	co.Distance2 = svec2_distance_squared(actor->Pos, co.Pos);
	if (co.Type == AI_OBJECTIVE_TYPE_NORMAL)
	{
		const int objective = ObjectiveFromThing(p->thing.flags);
		co.u.Objective =
			CArrayGet(&gMission.missionData->Objectives, objective);
	}
	CArrayPushBack(objectives, &co);
	CA_FOREACH_END()

	// Look for destructibles
	CA_FOREACH(const TObject, o, gObjs)
	if (!o->isInUse)
	{
		continue;
	}
	ClosestObjective co;
	memset(&co, 0, sizeof co);
	co.Pos = o->thing.Pos;
	co.IsDestructible = true;
	co.Type = AI_OBJECTIVE_TYPE_NORMAL;
	if (!(o->thing.flags & THING_OBJECTIVE))
	{
		continue;
	}
	// Destructible objective; go towards it and fire
	co.Distance2 = svec2_distance_squared(actor->Pos, co.Pos);
	if (co.Type == AI_OBJECTIVE_TYPE_NORMAL)
	{
		const int objective = ObjectiveFromThing(o->thing.flags);
		co.u.Objective =
			CArrayGet(&gMission.missionData->Objectives, objective);
	}
	CArrayPushBack(objectives, &co);
	CA_FOREACH_END()

	// Look for kill or rescue objectives
	CA_FOREACH(const TActor, a, gActors)
	if (!a->isInUse)
	{
		continue;
	}
	const Thing *ti = &a->thing;
	if (!(ti->flags & THING_OBJECTIVE))
	{
		continue;
	}
	const int objective = ObjectiveFromThing(ti->flags);
	const Objective *o =
		CArrayGet(&gMission.missionData->Objectives, objective);
	if (o->Type != OBJECTIVE_KILL && o->Type != OBJECTIVE_RESCUE)
	{
		continue;
	}
	// Only rescue those that need to be rescued
	if (o->Type == OBJECTIVE_RESCUE && !(a->flags & FLAGS_PRISONER))
	{
		continue;
	}
	ClosestObjective co;
	memset(&co, 0, sizeof co);
	co.Pos = ti->Pos;
	co.IsDestructible = false;
	co.Type = AI_OBJECTIVE_TYPE_NORMAL;
	co.Distance2 = svec2_distance_squared(actor->Pos, co.Pos);
	co.u.Objective = o;
	CArrayPushBack(objectives, &co);
	CA_FOREACH_END()

	// Look for explore objectives
	CA_FOREACH(const Objective, o, gMission.missionData->Objectives)
	if (o->Type != OBJECTIVE_INVESTIGATE)
	{
		continue;
	}
	if (ObjectiveIsComplete(o) || MapGetExploredPercentage(&gMap) == 100)
	{
		continue;
	}
	// Find the nearest unexplored tile
	const struct vec2i actorTile = Vec2ToTile(actor->Pos);
	const struct vec2i unexploredTile =
		MapSearchTileAround(&gMap, actorTile, MapTileIsUnexplored);
	ClosestObjective co;
	memset(&co, 0, sizeof co);
	co.Pos = Vec2CenterOfTile(unexploredTile);
	co.IsDestructible = false;
	co.Type = AI_OBJECTIVE_TYPE_NORMAL;
	co.Distance2 = svec2_distance_squared(actor->Pos, co.Pos);
	co.u.Objective = o;
	CArrayPushBack(objectives, &co);
	CA_FOREACH_END()

	// Sort according to distance
	qsort(
		objectives->data, objectives->size, objectives->elemSize,
		CompareClosestObjective);
}
static bool OnClosestPickupGun(
	ClosestObjective *co, const Pickup *p, const TActor *actor,
	const TActor *closestPlayer)
{
	if (!gCampaign.Setting.Ammo)
	{
		return false;
	}
	const WeaponClass *pickupGun = IdWeaponClass(p->class->u.GunId);
	const int weaponIndexStart = pickupGun->IsGrenade ? MAX_GUNS : 0;
	const int weaponIndexEnd = pickupGun->IsGrenade ? MAX_WEAPONS : MAX_GUNS;
	// Pick up if:
	// - we have a gun with less ammo than starting,
	// - and lower than lead player, who uses the ammo,
	// - or if there is a free weapon slot,
	// - and the gun is the same type (normal, grenade)
	bool hasGunLowOnAmmo = false;
	for (int i = weaponIndexStart; i < weaponIndexEnd; i++)
	{
		if (actor->guns[i].Gun == NULL)
		{
			hasGunLowOnAmmo = true;
			break;
		}
		const int ammoId = actor->guns[i].Gun->AmmoId;
		if (ammoId < 0)
		{
			continue;
		}
		const Ammo *ammo = AmmoGetById(&gAmmo, ammoId);
		const int ammoAmount = *(int *)CArrayGet(&actor->ammo, ammoId);
		if (ammoAmount >= ammo->Amount ||
			(closestPlayer != NULL && ActorUsesAmmo(closestPlayer, ammoId) &&
			 ammoAmount > *(int *)CArrayGet(&closestPlayer->ammo, ammoId)))
		{
			continue;
		}
		hasGunLowOnAmmo = true;
	}

	if (!hasGunLowOnAmmo)
	{
		return false;
	}

	if (ActorHasGun(actor, IdWeaponClass(p->class->u.GunId)))
	{
		return false;
	}

	co->Type = AI_OBJECTIVE_TYPE_PICKUP;
	co->u.UID = p->UID;
	return true;
}
static int CompareClosestObjective(const void *v1, const void *v2)
{
	const ClosestObjective *c1 = v1;
	const ClosestObjective *c2 = v2;
	if (c1->Distance2 < c2->Distance2)
	{
		return -1;
	}
	else if (c1->Distance2 > c2->Distance2)
	{
		return 1;
	}
	return 0;
}
static bool IsPosCloseEnoughToPlayer(
	const struct vec2 pos, const TActor *player,
	const float distanceTooFarFromPlayer);
static bool CanGetObjective(
	const struct vec2 objPos, const struct vec2 actorPos, const TActor *player,
	const float distanceTooFarFromPlayer)
{
	// Check if we can complete an objective.
	// If we don't need to follow any player, then only check that we can
	// pathfind to it.
	if (!player)
	{
		return AIHasPath(actorPos, objPos, true);
	}
	// If we need to stick close to a player, then check that it is both:
	// - Close enough from the lead player
	// - In line of sight from us
	if (!IsPosCloseEnoughToPlayer(objPos, player, distanceTooFarFromPlayer))
	{
		return false;
	}
	return AIHasClearPath(actorPos, objPos, true);
}
static bool IsPosCloseEnoughToPlayer(
	const struct vec2 pos, const TActor *player,
	const float distanceTooFarFromPlayer)
{
	if (!player)
	{
		return true;
	}
	const float distanceFromPlayer2 = svec2_distance_squared(pos, player->Pos);
	const float distanceMax2 = SQUARED(distanceTooFarFromPlayer * 16);
	return distanceFromPlayer2 < distanceMax2;
}
// Navigate to the current objective
// If the objective is destructible and it makes sense to shoot it,
// shoot at it as well
static int GotoObjective(TActor *actor, const float objDistance2)
{
	const AIObjectiveState *objState = &actor->aiContext->ObjectiveState;
	const struct vec2 goal = objState->Goal;
	int cmd = 0;
	// Check to see if we need to go any closer to the objective
	const bool isDestruction =
		objState->Type == AI_OBJECTIVE_TYPE_NORMAL && objState->IsDestructible;
	if (!isDestruction ||
		svec2_distance_squared(actor->Pos, goal) > SQUARED(3 * 16) ||
		!AIHasClearShot(actor->Pos, goal))
	{
		cmd = SmartGoto(actor, goal, objDistance2);
	}
	else if (isDestruction && ACTOR_GET_WEAPON(actor)->lock <= 0)
	{
		cmd = AIHunt(actor, goal);
		if (AIHasClearShot(actor->Pos, goal))
		{
			cmd |= CMD_BUTTON1;
		}
	}
	return cmd;
}

static bool PlayerHasWeapon(const PlayerData *p, const WeaponClass *wc);
void AICoopSelectWeapons(
	PlayerData *p, const int player, const CArray *weapons)
{
	// Select two weapons from available ones
	// Offset the starting index a bit so each AI uses different guns
	// TODO: select grenades
	const int startIdx = (player * MAX_GUNS) % weapons->size;
	int gunCount = 0;
	for (int i = 0; gunCount < MAX_GUNS; i++)
	{
		const int idx = (startIdx + i) % weapons->size;
		if (idx == startIdx && i > 0)
		{
			// Not enough weapons
			break;
		}
		const WeaponClass **wc = CArrayGet(weapons, idx);
		if ((*wc)->IsGrenade)
		{
			continue;
		}
		p->guns[gunCount] = *wc;
		gunCount++;
	}

	if (gCampaign.Setting.Ammo)
	{
		// Select pistol as an infinite-ammo backup
		const WeaponClass *pistol = StrWeaponClass("Pistol");
		if (!PlayerHasWeapon(p, pistol))
		{
			if (gunCount == MAX_GUNS)
			{
				// Player has full weapons; replace last gun
				p->guns[MAX_GUNS - 1] = pistol;
			}
			else
			{
				// Add the pistol
				p->guns[gunCount] = pistol;
				gunCount++;
			}
		}
	}
}
static bool PlayerHasWeapon(const PlayerData *p, const WeaponClass *wc)
{
	for (int i = 0; i < MAX_WEAPONS; i++)
	{
		const WeaponClass *wc2 = p->guns[i];
		if (wc == wc2)
		{
			return true;
		}
	}
	return false;
}

void AICoopOnPickupGun(TActor *a, const int gunId)
{
	// Remember that we are over a gun pickup;
	// we will act upon it later (during GetCmd)
	a->aiContext->OnGunId = gunId;
}
