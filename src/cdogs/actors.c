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

	Copyright (c) 2013-2021 Cong Xu
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
#include "actors.h"

#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "actor_fire.h"
#include "actor_placement.h"
#include "ai.h"
#include "ai_coop.h"
#include "ai_utils.h"
#include "ammo.h"
#include "character.h"
#include "collision/collision.h"
#include "config.h"
#include "damage.h"
#include "defs.h"
#include "draw/drawtools.h"
#include "events.h"
#include "game.h"
#include "game_events.h"
#include "gamedata.h"
#include "log.h"
#include "material.h"
#include "mission.h"
#include "pic_manager.h"
#include "pickup.h"
#include "sounds.h"
#include "thing.h"
#include "triggers.h"
#include "utils.h"

#define FOOTSTEP_MAX_ANIM_SPEED 2
#define REPEL_STRENGTH 0.06f
#define SLIDE_LOCK 50
#define SLIDE_X (TILE_WIDTH / 3)
#define SLIDE_Y (TILE_HEIGHT / 3)
#define VEL_DECAY_X (TILE_WIDTH * 2 / 256.0f)
#define VEL_DECAY_Y (TILE_WIDTH * 2 / 256.0f) // Note: deliberately tile width
#define SOUND_LOCK_WEAPON_CLICK 20
#define DRAW_RADIAN_SPEED (MPI / 16)
// Percent of health considered low; bleed and flash HUD if low
#define LOW_HEALTH_PERCENTAGE 25
#define GORE_EMITTER_MAX_SPEED 0.25f
#define CHATTER_SHOW_SECONDS 2
#define CHATTER_SWITCH_GUN                                                    \
	45 // TODO: based on clock time instead of game ticks
#define GRIMACE_PERIOD 20
#define GRIMACE_HIT_TICKS 39
#define GRIMACE_MELEE_TICKS 19
#define DAMAGE_TEXT_DISTANCE_RESET_THRESHOLD (ACTOR_W / 2)
#define FOOTPRINT_MAX 8

CArray gPlayerIds;

CArray gActors;
static unsigned int sActorUIDs = 0;

void ActorSetState(TActor *actor, const ActorAnimation state)
{
	actor->anim = AnimationGetActorAnimation(state);
}

static void ActorUpdateWeapon(TActor *a, Weapon *w, const int ticks);
static void CheckPickups(TActor *actor);
void UpdateActorState(TActor *actor, int ticks)
{
	ActorUpdateWeapon(actor, ACTOR_GET_GUN(actor), ticks);
	ActorUpdateWeapon(actor, ACTOR_GET_GRENADE(actor), ticks);

	// If we're ready to pick up, always check the pickups
	if (actor->PickupAll && !gCampaign.IsClient)
	{
		CheckPickups(actor);
	}
	// Stop picking up to prevent multiple pickups
	// (require repeated key presses)
	actor->PickupAll = false;

	if (actor->health > 0)
	{
		actor->flamed = MAX(0, actor->flamed - ticks);
		if (actor->poisoned)
		{
			if ((actor->poisoned & 7) == 0)
			{
				InjureActor(actor, 1);
			}
			actor->poisoned = MAX(0, actor->poisoned - ticks);
		}
		actor->petrified = MAX(0, actor->petrified - ticks);
		actor->confused = MAX(0, actor->confused - ticks);
	}

	actor->slideLock = MAX(0, actor->slideLock - ticks);

	ThingUpdate(&actor->thing, ticks);

	actor->stateCounter = MAX(0, actor->stateCounter - ticks);
	if (actor->stateCounter > 0)
	{
		return;
	}

	if (actor->health <= 0)
	{
		actor->dead++;
		actor->MoveVel = svec2_zero();
		actor->stateCounter = 4;
		actor->thing.flags = 0;
		return;
	}

	// Draw rotation interpolation
	const float targetRadians = (float)dir2radians[actor->direction];
	if (actor->DrawRadians - targetRadians > MPI)
	{
		actor->DrawRadians -= 2 * MPI;
	}
	if (actor->DrawRadians - targetRadians < -MPI)
	{
		actor->DrawRadians += 2 * MPI;
	}
	const float dr = actor->DrawRadians - targetRadians;
	if (dr < 0)
	{
		actor->DrawRadians += (float)MIN(DRAW_RADIAN_SPEED * ticks, -dr);
	}
	else if (dr > 0)
	{
		actor->DrawRadians -= (float)MIN(DRAW_RADIAN_SPEED * ticks, dr);
	}
	
	const struct vec2i tilePos = Vec2ToTile(actor->Pos);
	const Tile *t = MapGetTile(&gMap, tilePos);

	// Footstep sounds
	// Step on 2 and 6
	// TODO: custom animation and footstep frames
	const int frame = AnimationGetFrame(&actor->anim);
	const bool isFootstepFrame = actor->anim.Type == ACTORANIMATION_WALKING && (frame == 2 || frame == 6) && actor->anim.newFrame;
	if (isFootstepFrame)
	{

		if (ConfigGetBool(&gConfig, "Sound.Footsteps"))
		{
			GameEvent e = GameEventNew(GAME_EVENT_SOUND_AT);
			const CharacterClass *cc = ActorGetCharacter(actor)->Class;
			MatGetFootstepSound(cc, t, e.u.SoundAt.Sound);
			e.u.SoundAt.Pos = Vec2ToNet(actor->thing.Pos);
			e.u.SoundAt.Distance = cc->FootstepsDistancePlus;
			GameEventsEnqueue(&gGameEvents, e);
		}

		// See if we've stepped on something that leaves footprints
		const color_t footprintMask = MatGetFootprintMask(t);
		if (footprintMask.a != 0)
		{
			actor->footprintMask = footprintMask;
			actor->footprintCounter = FOOTPRINT_MAX;
		}

		// Footprint particle
		if (actor->footprintCounter > 0)
		{
			GameEvent e = GameEventNew(GAME_EVENT_ADD_PARTICLE);
			e.u.AddParticle.Class =
				StrParticleClass(&gParticleClasses, "footprint");
			const struct vec2 footOffset = svec2_scale(
				Vec2FromRadiansScaled(actor->DrawRadians + MPI_2),
				frame == 2 ? 3.f : -3.f);
			e.u.AddParticle.Pos = svec2_subtract(actor->thing.Pos, footOffset);
			e.u.AddParticle.Angle = actor->DrawRadians;
			e.u.AddParticle.Mask = actor->footprintMask;
			// Fade footprint away gradually
			e.u.AddParticle.Mask.a = (uint8_t)MIN(
				actor->footprintCounter * e.u.AddParticle.Mask.a /
					FOOTPRINT_MAX,
				255);
			GameEventsEnqueue(&gGameEvents, e);
			actor->footprintCounter--;
		}
	}

	// Damage when on special tiles
	if (!gCampaign.IsClient &&
		actor->anim.Type == ACTORANIMATION_WALKING ? isFootstepFrame : (gMission.time % FPS_FRAMELIMIT) == 0)
	{
	   const BulletClass *b = MatGetDamageBullet(t);
	   if (b != NULL)
	   {
		   GameEvent e = GameEventNew(GAME_EVENT_THING_DAMAGE);
		   e.u.ThingDamage.UID = actor->uid;
		   e.u.ThingDamage.Kind = KIND_CHARACTER;
		   BulletToDamageEvent(b, &e);
		   GameEventsEnqueue(&gGameEvents, e);
	   }
	}

	// Animation
	float animTicks = 1;
	if (actor->anim.Type == ACTORANIMATION_WALKING)
	{
		// Update walk animation based on actor speed
		animTicks =
			MIN(svec2_length(svec2_add(actor->MoveVel, actor->thing.Vel)),
				FOOTSTEP_MAX_ANIM_SPEED);
	}
	animTicks *= ticks;
	AnimationUpdate(&actor->anim, animTicks);

	// Chatting
	actor->ChatterCounter = MAX(0, actor->ChatterCounter - ticks);
	if (actor->ChatterCounter == 0)
	{
		// Stop chatting
		strcpy(actor->Chatter, "");
	}

	actor->grimaceCounter = MAX(0, actor->grimaceCounter - ticks);
}
static void ActorUpdateWeapon(TActor *a, Weapon *w, const int ticks)
{
	if (w->Gun == NULL)
	{
		return;
	}
	WeaponUpdate(w, ticks);
	ActorFireUpdate(w, a, ticks);
	for (int i = 0; i < WeaponClassNumBarrels(w->Gun); i++)
	{
		if (WeaponBarrelIsOverheating(w, i))
		{
			AddParticle ap;
			memset(&ap, 0, sizeof ap);
			ap.Pos = svec2_add(a->Pos, ActorGetMuzzleOffset(a, w, i));
			ap.Z = WeaponClassGetMuzzleHeight(w->Gun, w->barrels[i].state, i) /
				   Z_FACTOR;
			ap.Mask = colorWhite;
			EmitterUpdate(&a->barrelSmoke, &ap, ticks);
		}
	}
}

static struct vec2 GetConstrainedPos(
	const Map *map, const struct vec2 from, const struct vec2 to,
	const struct vec2i size);
static void OnMove(TActor *a);
bool TryMoveActor(TActor *actor, struct vec2 pos)
{
	CASSERT(
		!svec2_is_nearly_equal(actor->Pos, pos, EPSILON_POS),
		"trying to move to same position");

	// Don't check collisions for pilots
	if (actor->vehicleUID == -1)
	{
		actor->hasCollided = true;
		actor->CanPickupSpecial = false;

		const struct vec2 oldPos = actor->Pos;
		pos = GetConstrainedPos(&gMap, actor->Pos, pos, actor->thing.size);
		if (svec2_is_nearly_equal(oldPos, pos, EPSILON_POS))
		{
			return false;
		}

		// Check for object collisions
		const CollisionParams params = {
			THING_IMPASSABLE, CalcCollisionTeam(true, actor),
			IsPVP(gCampaign.Entry.Mode), false};
		Thing *target = OverlapGetFirstItem(
			&actor->thing, pos, actor->thing.size, actor->thing.Vel, params);
		if (target)
		{
			Weapon *gun = ACTOR_GET_WEAPON(actor);
			const TObject *object = target->kind == KIND_OBJECT
										? CArrayGet(&gObjs, target->id)
										: NULL;
			const int barrel = ActorGetCanFireBarrel(actor, gun);
			// Check for melee damage if we are the owner of the actor
			const bool checkMelee =
				(!gCampaign.IsClient && actor->PlayerUID < 0) ||
				ActorIsLocalPlayer(actor->uid);
			// TODO: support melee weapons on multi guns
			const BulletClass *b = WeaponClassGetBullet(gun->Gun, barrel);
			if (checkMelee && barrel >= 0 && !WeaponClassCanShoot(gun->Gun) &&
				actor->health > 0 && b &&
				(!object ||
				 (((b->Hit.Object.Hit && target->kind == KIND_OBJECT) ||
				   (b->Hit.Flesh.Hit && target->kind == KIND_CHARACTER)) &&
				  !ObjIsDangerous(object))))
			{
				if (CanHit(b, actor->flags, actor->uid, target))
				{
					// Tell the server that we want to melee something
					GameEvent e = GameEventNew(GAME_EVENT_ACTOR_MELEE);
					e.u.Melee.UID = actor->uid;
					strcpy(e.u.Melee.BulletClass, b->Name);
					e.u.Melee.TargetKind = target->kind;
					switch (target->kind)
					{
					case KIND_CHARACTER:
						e.u.Melee.TargetUID =
							((const TActor *)CArrayGet(&gActors, target->id))
								->uid;
						e.u.Melee.HitType = HIT_FLESH;
						break;
					case KIND_OBJECT:
						e.u.Melee.TargetUID =
							((const TObject *)CArrayGet(&gObjs, target->id))
								->uid;
						e.u.Melee.HitType = HIT_OBJECT;
						break;
					default:
						CASSERT(false, "cannot damage target kind");
						break;
					}
					if (gun->barrels[barrel].soundLock > 0)
					{
						e.u.Melee.HitType = (int)HIT_NONE;
					}
					GameEventsEnqueue(&gGameEvents, e);
					WeaponBarrelOnFire(gun, barrel);

					// Only set grimace when counter 0 so that the actor
					// alternates their grimace
					if (actor->grimaceCounter == 0)
					{
						actor->grimaceCounter = GRIMACE_MELEE_TICKS;
					}
				}
				return false;
			}

			const struct vec2 yPos = svec2(actor->Pos.x, pos.y);
			if (OverlapGetFirstItem(
					&actor->thing, yPos, actor->thing.size, svec2_zero(),
					params))
			{
				pos.y = actor->Pos.y;
			}
			const struct vec2 xPos = svec2(pos.x, actor->Pos.y);
			if (OverlapGetFirstItem(
					&actor->thing, xPos, actor->thing.size, svec2_zero(),
					params))
			{
				pos.x = actor->Pos.x;
			}
			if (pos.x != actor->Pos.x && pos.y != actor->Pos.y)
			{
				// Both x-only or y-only movement are viable,
				// i.e. we are colliding corner vs corner
				// Arbitrarily choose x-only movement
				pos.y = actor->Pos.y;
			}
			if ((pos.x == actor->Pos.x && pos.y == actor->Pos.y) ||
				IsCollisionWithWall(pos, actor->thing.size))
			{
				return false;
			}
		}
	}

	actor->Pos = pos;
	OnMove(actor);

	actor->hasCollided = false;
	return true;
}
// Get a movement position that is constrained by collisions
// May return a position that is the same as the 'from', that is, we cannot
// move in the direction specified.
static struct vec2 GetConstrainedPos(
	const Map *map, const struct vec2 from, const struct vec2 to,
	const struct vec2i size)
{
	// Check collision with wall
	if (!IsCollisionWithWall(to, size))
	{
		// Not in collision; just return where we wanted to go
		return to;
	}

	CASSERT(size.x >= size.y, "tall collision not supported");
	const struct vec2 dv = svec2_subtract(to, from);

	// If moving diagonally, use rectangular bounds and
	// try to move in only x or y directions
	if (!nearly_equal(dv.x, 0.0f, EPSILON_POS) &&
		!nearly_equal(dv.y, 0.0f, EPSILON_POS))
	{
		// X-only movement
		const struct vec2 xVec = svec2(to.x, from.y);
		if (!IsCollisionWithWall(xVec, size))
		{
			return xVec;
		}
		// Y-only movement
		const struct vec2 yVec = svec2(from.x, to.y);
		if (!IsCollisionWithWall(yVec, size))
		{
			return yVec;
		}
		// If we're still stuck, we're possibly stuck on a corner which is not
		// in collision with a diamond but is colliding with the box.
		// If so try x- or y-only movement, but with the benefit of diamond
		// slipping.
		const struct vec2 xPos = GetConstrainedPos(map, from, xVec, size);
		if (!svec2_is_nearly_equal(xPos, from, EPSILON_POS))
		{
			return xPos;
		}
		const struct vec2 yPos = GetConstrainedPos(map, from, yVec, size);
		if (!svec2_is_nearly_equal(yPos, from, EPSILON_POS))
		{
			return yPos;
		}
	}

	// Now check diagonal movement, if we were moving in an x- or y-
	// only direction
	// Note: we're moving at extra speed because dx/dy are only magnitude 1;
	// if we divide then we get 0 which ruins the logic
	if (nearly_equal(dv.x, 0.0f, EPSILON_POS))
	{
		// Moving up or down; try moving to the left or right diagonally
		// Scale X movement because our diamond is wider than tall, so we
		// may need to scale the diamond wider.
		const int xScale =
			size.x > size.y ? (int)ceil((double)size.x / size.y) : 1;
		const struct vec2 diag1Vec =
			svec2_add(from, svec2(-dv.y * xScale, dv.y));
		if (!IsCollisionDiamond(map, diag1Vec, size))
		{
			return diag1Vec;
		}
		const struct vec2 diag2Vec =
			svec2_add(from, svec2(dv.y * xScale, dv.y));
		if (!IsCollisionDiamond(map, diag2Vec, size))
		{
			return diag2Vec;
		}
	}
	else if (nearly_equal(dv.y, 0.0f, EPSILON_POS))
	{
		// Moving left or right; try moving up or down diagonally
		const struct vec2 diag1Vec = svec2_add(from, svec2(dv.x, -dv.x));
		if (!IsCollisionDiamond(map, diag1Vec, size))
		{
			return diag1Vec;
		}
		const struct vec2 diag2Vec = svec2_add(from, svec2(dv.x, dv.x));
		if (!IsCollisionDiamond(map, diag2Vec, size))
		{
			return diag2Vec;
		}
	}

	// All alternative movements are in collision; don't move
	return from;
}

void ActorMove(const NActorMove am)
{
	TActor *a = ActorGetByUID(am.UID);
	if (a == NULL || !a->isInUse)
		return;
	a->Pos = NetToVec2(am.Pos);
	a->MoveVel = NetToVec2(am.MoveVel);
	OnMove(a);
}
static void CheckTrigger(const TActor *a, const Map *map);
static void CheckRescue(const TActor *a);
static void OnMove(TActor *a)
{
	MapTryMoveThing(&gMap, &a->thing, a->Pos);
	if (MapIsTileInExit(&gMap, &a->thing, -1) != -1)
	{
		a->action = ACTORACTION_EXITING;
	}
	else
	{
		a->action = ACTORACTION_MOVING;
	}

	if (!gCampaign.IsClient)
	{
		CheckTrigger(a, &gMap);

		CheckPickups(a);

		CheckRescue(a);
	}
}
static void CheckTrigger(const TActor *a, const Map *map)
{
	// Don't let sleeping AI actors trigger tiles
	if (a->PlayerUID < 0 && a->flags & FLAGS_SLEEPING)
	{
		return;
	}
	const struct vec2i tilePos = Vec2ToTile(a->Pos);
	const bool showLocked = ActorIsLocalPlayer(a->uid);
	const Tile *t = MapGetTile(map, tilePos);
	const int keyFlags =
		(a->flags & FLAGS_UNLOCK_DOORS) ? -1 : gMission.KeyFlags;
	CA_FOREACH(Trigger *, tp, t->triggers)
	if (!TriggerTryActivate(*tp, keyFlags, tilePos) && (*tp)->isActive &&
		TriggerCannotActivate(*tp) && showLocked)
	{
		TriggerSetCannotActivate(*tp);
		GameEvent s = GameEventNew(GAME_EVENT_ADD_PARTICLE);
		s.u.AddParticle.Class =
			StrParticleClass(&gParticleClasses, "locked_text");
		s.u.AddParticle.Pos = Vec2CenterOfTile(tilePos);
		s.u.AddParticle.Z = (BULLET_Z * 2) * Z_FACTOR;
		sprintf(s.u.AddParticle.Text, "locked");
		GameEventsEnqueue(&gGameEvents, s);
	}
	CA_FOREACH_END()
}
// Check if the player can pickup any item
static bool CheckPickupFunc(
	Thing *ti, void *data, const struct vec2 colA, const struct vec2 colB,
	const struct vec2 normal);
static void CheckPilot(const TActor *a, const CollisionParams params);
static void CheckPickups(TActor *actor)
{
	// NPCs can't pickup
	if (actor->PlayerUID < 0)
	{
		return;
	}
	const CollisionParams params = {
		0, CalcCollisionTeam(true, actor), IsPVP(gCampaign.Entry.Mode), false};
	OverlapThings(
		&actor->thing, actor->Pos, actor->thing.Vel, actor->thing.size, params,
		CheckPickupFunc, actor, NULL, NULL, NULL);
	if (actor->PickupAll)
	{
		const CollisionParams paramsPilot = {
			THING_IMPASSABLE | THING_CAN_BE_SHOT, params.Team, params.IsPVP,
			true};
		CheckPilot(actor, paramsPilot);
	}
}
static bool CheckPickupFunc(
	Thing *ti, void *data, const struct vec2 colA, const struct vec2 colB,
	const struct vec2 normal)
{
	UNUSED(colA);
	UNUSED(colB);
	UNUSED(normal);
	// Always return true, as we can pickup multiple items in one go
	if (ti->kind != KIND_PICKUP)
		return true;
	TActor *a = data;
	PickupPickup(a, CArrayGet(&gPickups, ti->id), a->PickupAll);
	return true;
}
static void CheckPilot(const TActor *a, const CollisionParams params)
{
	const Thing *ti = OverlapGetFirstItem(
		&a->thing, a->Pos, a->thing.size, a->thing.Vel, params);
	if (ti == NULL || ti->kind != KIND_CHARACTER)
		return;
	const TActor *vehicle = CArrayGet(&gActors, ti->id);
	if (vehicle->pilotUID >= 0)
		return;

	GameEvent e = GameEventNew(GAME_EVENT_ACTOR_PILOT);
	e.u.Pilot.On = true;
	e.u.Pilot.UID = a->uid;
	e.u.Pilot.VehicleUID = vehicle->uid;
	GameEventsEnqueue(&gGameEvents, e);
}
static void CheckRescue(const TActor *a)
{
	// NPCs can't rescue
	if (a->PlayerUID < 0)
		return;

		// Check an area slightly bigger than the actor's size for rescue
		// objectives
#define RESCUE_CHECK_PAD 2
	const CollisionParams params = {
		THING_IMPASSABLE, CalcCollisionTeam(true, a),
		IsPVP(gCampaign.Entry.Mode), false};
	const Thing *target = OverlapGetFirstItem(
		&a->thing, a->Pos,
		svec2i_add(a->thing.size, svec2i(RESCUE_CHECK_PAD, RESCUE_CHECK_PAD)),
		a->thing.Vel, params);
	if (target != NULL && target->kind == KIND_CHARACTER)
	{
		TActor *other = CArrayGet(&gActors, target->id);
		CASSERT(other->isInUse, "Cannot find nonexistent player");
		if (other->flags & FLAGS_PRISONER)
		{
			other->flags &= ~FLAGS_PRISONER;
			GameEvent e = GameEventNew(GAME_EVENT_RESCUE_CHARACTER);
			e.u.Rescue.UID = other->uid;
			GameEventsEnqueue(&gGameEvents, e);
			UpdateMissionObjective(
				&gMission, other->thing.flags, OBJECTIVE_RESCUE, 1);
		}
	}
}

void ActorHeal(TActor *actor, int health)
{
	actor->health += health;
	actor->health = MIN(actor->health, ActorGetCharacter(actor)->maxHealth);
	AddParticle ap;
	memset(&ap, 0, sizeof ap);
	ap.Pos = actor->Pos;
	ap.Z = 10;
	for (int i = 0; i < MAX(health / 20, 1); i++)
	{
		ap.Vel = svec2(RAND_FLOAT(-0.2f, 0.2f), RAND_FLOAT(-0.2f, 0.2f));
		EmitterStart(&actor->healEffect, &ap);
	}
}

void InjureActor(TActor *actor, int injury)
{
	const int lastHealth = actor->health;
	actor->health -= injury;
	LOG(LM_ACTOR, LL_DEBUG, "actor uid(%d) injured %d -(%d)-> %d", actor->uid,
		lastHealth, injury, actor->health);
	if (lastHealth > 0 && actor->health <= 0)
	{
		actor->stateCounter = 0;
		GameEvent es = GameEventNew(GAME_EVENT_SOUND_AT);
		CharacterClassGetSound(
			ActorGetCharacter(actor)->Class, es.u.SoundAt.Sound, "die");
		es.u.SoundAt.Pos = Vec2ToNet(actor->thing.Pos);
		GameEventsEnqueue(&gGameEvents, es);
		if (actor->PlayerUID >= 0)
		{
			es = GameEventNew(GAME_EVENT_SOUND_AT);
			strcpy(es.u.SoundAt.Sound, "hahaha");
			es.u.SoundAt.Pos = Vec2ToNet(actor->thing.Pos);
			GameEventsEnqueue(&gGameEvents, es);
		}
		if (actor->thing.flags & THING_OBJECTIVE)
		{
			UpdateMissionObjective(
				&gMission, actor->thing.flags, OBJECTIVE_KILL, 1);
			// If we've killed someone we have rescued, deduct from the
			// rescue objective
			if (!(actor->flags & FLAGS_PRISONER))
			{
				UpdateMissionObjective(
					&gMission, actor->thing.flags, OBJECTIVE_RESCUE, -1);
			}
		}
	}
}

void ActorAddAmmo(TActor *actor, const int ammoId, const int amount)
{
	int *ammo = CArrayGet(&actor->ammo, ammoId);
	*ammo += amount;
	const int ammoMax = AmmoGetById(&gAmmo, ammoId)->Max;
	*ammo = CLAMP(*ammo, 0, ammoMax);
}

bool ActorUsesAmmo(const TActor *actor, const int ammoId)
{
	for (int i = 0; i < MAX_WEAPONS; i++)
	{
		const WeaponClass *wc = actor->guns[i].Gun;
		if (wc == NULL)
		{
			continue;
		}
		for (int j = 0; j < WeaponClassNumBarrels(wc); j++)
		{
			if (WC_BARREL_ATTR(*wc, AmmoId, j) == ammoId)
			{
				return true;
			}
		}
	}
	return false;
}

void ActorReplaceGun(const NActorReplaceGun rg)
{
	TActor *a = ActorGetByUID(rg.UID);
	if (a == NULL || !a->isInUse)
		return;
	const WeaponClass *wc = StrWeaponClass(rg.Gun);
	CASSERT(wc != NULL, "cannot find gun");
	// If player already has gun, don't do anything
	if (ActorFindGun(a, wc) >= 0)
	{
		return;
	}
	LOG(LM_ACTOR, LL_DEBUG, "actor uid(%d) replacing gun(%s) idx(%d)",
		(int)rg.UID, rg.Gun, rg.GunIdx);
	Weapon w = WeaponCreate(wc);
	memcpy(&a->guns[rg.GunIdx], &w, sizeof w);
	// Switch immediately to picked up gun
	const PlayerData *p = PlayerDataGetByUID(a->PlayerUID);
	if (wc->Type == GUNTYPE_GRENADE && PlayerHasGrenadeButton(p))
	{
		a->grenadeIndex = rg.GunIdx - MAX_GUNS;
	}
	else
	{
		a->gunIndex = rg.GunIdx;
	}

	SoundPlayAt(&gSoundDevice, wc->SwitchSound, a->Pos);
}

int ActorFindGun(const TActor *a, const WeaponClass *wc)
{
	for (int i = 0; i < MAX_WEAPONS; i++)
	{
		if (a->guns[i].Gun == wc)
		{
			return i;
		}
	}
	return -1;
}

int ActorGetNumWeapons(const TActor *a)
{
	int count = 0;
	for (int i = 0; i < MAX_WEAPONS; i++)
	{
		if (a->guns[i].Gun != NULL)
		{
			count++;
		}
	}
	return count;
}
int ActorGetNumGuns(const TActor *a)
{
	int count = 0;
	for (int i = 0; i < MAX_GUNS; i++)
	{
		if (a->guns[i].Gun != NULL)
		{
			count++;
		}
	}
	return count;
}
int ActorGetNumGrenades(const TActor *a)
{
	int count = 0;
	for (int i = MAX_GUNS; i < MAX_WEAPONS; i++)
	{
		if (a->guns[i].Gun != NULL)
		{
			count++;
		}
	}
	return count;
}

static void ActorSetChatter(TActor *a, const char *text, const int count)
{
	strcpy(a->Chatter, text);
	a->ChatterCounter = count;
}

// Set AI state and possibly say something based on the state
void ActorSetAIState(TActor *actor, const AIState s)
{
	if (AIContextSetState(actor->aiContext, s) &&
		AIContextShowChatter(ConfigGetEnum(&gConfig, "Interface.AIChatter")))
	{
		ActorSetChatter(
			actor, AIStateGetChatterText(actor->aiContext->State),
			CHATTER_SHOW_SECONDS * ConfigGetInt(&gConfig, "Game.FPS"));
	}
}

void ActorPilot(const NActorPilot ap)
{
	TActor *pilot = ActorGetByUID(ap.UID);
	TActor *vehicle = ActorGetByUID(ap.VehicleUID);
	if (ap.On)
	{
		CASSERT(pilot->vehicleUID == -1, "already piloting");
		pilot->vehicleUID = vehicle->uid;
		CASSERT(vehicle->pilotUID == -1, "already has pilot");
		vehicle->pilotUID = pilot->uid;
		char buf[256];
		CharacterClassGetSound(
			ActorGetCharacter(vehicle)->Class, buf, "alert");
		SoundPlayAt(&gSoundDevice, StrSound(buf), vehicle->Pos);
	}
	else
	{
		CASSERT(pilot->vehicleUID != -1, "not piloting");
		pilot->vehicleUID = -1;
		CASSERT(vehicle->pilotUID != -1, "doesn't have pilot");
		vehicle->pilotUID = -1;
		char buf[256];
		MatGetFootstepSound(ActorGetCharacter(pilot)->Class, NULL, buf);
		SoundPlayAt(&gSoundDevice, StrSound(buf), vehicle->Pos);
	}
}

static void FireWeapon(TActor *a, Weapon *w)
{
	if (w->Gun == NULL)
	{
		return;
	}
	const int barrel = ActorGetCanFireBarrel(a, w);
	if (barrel == -1)
	{
		const int unlockedBarrel = WeaponGetUnlockedBarrel(w);
		if (unlockedBarrel >= 0 && gCampaign.Setting.Ammo)
		{
			CASSERT(
				ActorWeaponGetAmmo(a, w->Gun, unlockedBarrel) == 0,
				"should be out of ammo");
			// Play a clicking sound if this weapon is out of ammo
			if (w->clickLock <= 0)
			{
				GameEvent es = GameEventNew(GAME_EVENT_SOUND_AT);
				strcpy(es.u.SoundAt.Sound, "click");
				es.u.SoundAt.Pos = Vec2ToNet(a->Pos);
				GameEventsEnqueue(&gGameEvents, es);
				w->clickLock = SOUND_LOCK_WEAPON_CLICK;
			}
		}
		return;
	}
	ActorFireBarrel(w, a, barrel);
	const int ammoId = WC_BARREL_ATTR(*(w->Gun), AmmoId, barrel);
	if (a->PlayerUID >= 0 && gCampaign.Setting.Ammo && ammoId >= 0)
	{
		GameEvent e = GameEventNew(GAME_EVENT_ACTOR_USE_AMMO);
		e.u.UseAmmo.UID = a->uid;
		e.u.UseAmmo.PlayerUID = a->PlayerUID;
		e.u.UseAmmo.Ammo.Id = ammoId;
		e.u.UseAmmo.Ammo.Amount = 1;
		GameEventsEnqueue(&gGameEvents, e);
	}
	const TActor *firingActor = ActorGetByUID(a->pilotUID);
	const int cost = WC_BARREL_ATTR(*(w->Gun), Cost, barrel);
	if (firingActor->PlayerUID >= 0 && cost != 0)
	{
		// Classic C-Dogs score consumption
		GameEvent e = GameEventNew(GAME_EVENT_SCORE);
		e.u.Score.PlayerUID = firingActor->PlayerUID;
		e.u.Score.Score = -cost;
		GameEventsEnqueue(&gGameEvents, e);
	}
}

static bool ActorTryChangeDirection(
	TActor *actor, const int cmd, const int prevCmd)
{
	const bool willChangeDirecton =
		!actor->petrified && CMD_HAS_DIRECTION(cmd) &&
		(!(cmd & CMD_BUTTON2) ||
		 ConfigGetEnum(&gConfig, "Game.SwitchMoveStyle") !=
			 SWITCHMOVE_STRAFE) &&
		(!(prevCmd & CMD_BUTTON1) ||
		 ConfigGetEnum(&gConfig, "Game.FireMoveStyle") != FIREMOVE_STRAFE);
	const direction_e dir = CmdToDirection(cmd);
	if (willChangeDirecton && dir != actor->direction)
	{
		GameEvent e = GameEventNew(GAME_EVENT_ACTOR_DIR);
		e.u.ActorDir.UID = actor->uid;
		e.u.ActorDir.Dir = (int32_t)dir;
		GameEventsEnqueue(&gGameEvents, e);
		// Change direction immediately because this affects shooting
		actor->direction = dir;
	}
	return willChangeDirecton;
}

static bool ActorTryShoot(TActor *actor, const int cmd)
{
	const bool willShoot = !actor->petrified && (cmd & CMD_BUTTON1);
	if (willShoot)
	{
		FireWeapon(actor, ACTOR_GET_GUN(actor));
	}
	else
	{
		// Stop firing barrel and restore ready state
		const Weapon *w = ACTOR_GET_GUN(actor);
		for (int i = 0; i < WeaponClassNumBarrels(w->Gun); i++)
		{
			if (w->barrels[i].state == GUNSTATE_FIRING)
			{
				GameEvent e = GameEventNew(GAME_EVENT_GUN_STATE);
				e.u.GunState.ActorUID = actor->uid;
				e.u.GunState.Barrel = i;
				e.u.GunState.State = GUNSTATE_READY;
				GameEventsEnqueue(&gGameEvents, e);
			}
		}
	}
	return willShoot;
}

static bool TryGrenade(TActor *a, const int cmd)
{
	const bool willGrenade = !a->petrified && (cmd & CMD_GRENADE);
	if (willGrenade)
	{
		FireWeapon(a, ACTOR_GET_GRENADE(a));
	}
	return willGrenade;
}

static bool ActorTryMove(TActor *actor, int cmd, int hasShot, int ticks);
void CommandActor(TActor *actor, int cmd, int ticks)
{
	// If this is a pilot, command the vehicle instead
	if (actor->vehicleUID != -1)
	{
		TActor *vehicle = ActorGetByUID(actor->vehicleUID);
		CommandActor(vehicle, cmd, ticks);
	}
	else if (actor->pilotUID == -1)
	{
		// If this is a vehicle and there's no pilot, do nothing
	}
	else
	{

		if (actor->confused)
		{
			cmd = CmdGetReverse(cmd);
		}

		if (actor->health > 0)
		{
			const bool hasChangedDirection =
				ActorTryChangeDirection(actor, cmd, actor->lastCmd);
			const bool hasShot = ActorTryShoot(actor, cmd);
			const bool hasGrenaded = TryGrenade(actor, cmd);
			const bool hasMoved = ActorTryMove(actor, cmd, hasShot, ticks);
			ActorAnimation anim = actor->anim.Type;
			// Idle if player hasn't done anything
			if (!(hasChangedDirection || hasShot || hasGrenaded || hasMoved))
			{
				anim = ACTORANIMATION_IDLE;
			}
			else if (hasMoved)
			{
				anim = ACTORANIMATION_WALKING;
			}
			else
			{
				anim = ACTORANIMATION_STAND;
			}
			if (actor->anim.Type != anim)
			{
				GameEvent e = GameEventNew(GAME_EVENT_ACTOR_STATE);
				e.u.ActorState.UID = actor->uid;
				e.u.ActorState.State = (int32_t)anim;
				GameEventsEnqueue(&gGameEvents, e);
			}
		}
	}

	actor->specialCmdDir = CMD_HAS_DIRECTION(cmd);
	if ((cmd & CMD_BUTTON2) && !actor->specialCmdDir)
	{
		// Special: pick up things that can only be picked up on demand
		if (!actor->PickupAll && !(actor->lastCmd & CMD_BUTTON2) &&
			actor->vehicleUID == -1)
		{
			GameEvent e = GameEventNew(GAME_EVENT_ACTOR_PICKUP_ALL);
			e.u.ActorPickupAll.UID = actor->uid;
			e.u.ActorPickupAll.PickupAll = true;
			GameEventsEnqueue(&gGameEvents, e);
		}
	}

	actor->lastCmd = cmd;
}
static bool ActorTryMove(TActor *actor, int cmd, int hasShot, int ticks)
{
	const bool canMoveWhenShooting =
		ConfigGetEnum(&gConfig, "Game.FireMoveStyle") != FIREMOVE_STOP ||
		!hasShot ||
		(ConfigGetEnum(&gConfig, "Game.SwitchMoveStyle") ==
			 SWITCHMOVE_STRAFE &&
		 (cmd & CMD_BUTTON2));
	const bool willMove =
		!actor->petrified && CMD_HAS_DIRECTION(cmd) && canMoveWhenShooting;
	actor->MoveVel = svec2_zero();
	if (willMove)
	{
		const float moveAmount = ActorGetCharacter(actor)->speed * ticks;
		struct vec2 moveVel = svec2_zero();
		if (cmd & CMD_LEFT)
			moveVel.x--;
		else if (cmd & CMD_RIGHT)
			moveVel.x++;
		if (cmd & CMD_UP)
			moveVel.y--;
		else if (cmd & CMD_DOWN)
			moveVel.y++;
		if (!svec2_is_zero(moveVel))
		{
			actor->MoveVel = svec2_scale(svec2_normalize(moveVel), moveAmount);
		}
	}

	// If we have changed our move commands, send the move event
	if (cmd != actor->lastCmd || actor->hasCollided)
	{
		GameEvent e = GameEventNew(GAME_EVENT_ACTOR_MOVE);
		e.u.ActorMove.UID = actor->uid;
		e.u.ActorMove.Pos = Vec2ToNet(actor->Pos);
		e.u.ActorMove.MoveVel = Vec2ToNet(actor->MoveVel);
		GameEventsEnqueue(&gGameEvents, e);
	}

	return willMove || !svec2_is_zero(actor->thing.Vel);
}

void SlideActor(TActor *actor, int cmd)
{
	// Check that actor can slide
	if (actor->slideLock > 0)
	{
		return;
	}

	if (actor->petrified)
		return;

	if (actor->confused)
	{
		cmd = CmdGetReverse(cmd);
	}

	GameEvent e = GameEventNew(GAME_EVENT_ACTOR_SLIDE);
	e.u.ActorSlide.UID = actor->uid;
	struct vec2 vel = svec2_zero();
	if (cmd & CMD_LEFT)
		vel.x = -SLIDE_X;
	else if (cmd & CMD_RIGHT)
		vel.x = SLIDE_X;
	if (cmd & CMD_UP)
		vel.y = -SLIDE_Y;
	else if (cmd & CMD_DOWN)
		vel.y = SLIDE_Y;
	e.u.ActorSlide.Vel = Vec2ToNet(vel);
	GameEventsEnqueue(&gGameEvents, e);

	actor->slideLock = SLIDE_LOCK;
}

static void ActorAddBloodSplatters(
	TActor *a, const int power, const float mass, const struct vec2 hitVector);

static void ActorUpdatePosition(TActor *actor, int ticks);
static void ActorDie(TActor *actor);
void UpdateAllActors(const int ticks)
{
	CA_FOREACH(TActor, actor, gActors)
	if (!actor->isInUse)
	{
		continue;
	}
	// Update pilot/vehicle statuses
	if (actor->vehicleUID != -1)
	{
		const TActor *vehicle = ActorGetByUID(actor->vehicleUID);
		if (vehicle->dead)
		{
			actor->vehicleUID = -1;
		}
	}
	if (actor->pilotUID != -1 && actor->pilotUID != actor->uid)
	{
		const TActor *pilot = ActorGetByUID(actor->pilotUID);
		if (pilot->dead)
		{
			actor->pilotUID = -1;
		}
	}
	ActorUpdatePosition(actor, ticks);
	UpdateActorState(actor, ticks);
	const NamedSprites *deathSprites = CharacterClassGetDeathSprites(
		ActorGetCharacter(actor)->Class, &gPicManager);
	if (actor->dead && (deathSprites == NULL ||
						actor->dead - 1 > (int)deathSprites->pics.size))
	{
		if (!gCampaign.IsClient)
		{
			ActorDie(actor);
		}
		continue;
	}
	// Find actors that are on the same team and colliding,
	// and repel them
	if (!gCampaign.IsClient &&
		gCollisionSystem.allyCollision == ALLYCOLLISION_REPEL)
	{
		const CollisionParams params = {
			THING_IMPASSABLE, COLLISIONTEAM_NONE, IsPVP(gCampaign.Entry.Mode),
			false};
		const Thing *collidingItem = OverlapGetFirstItem(
			&actor->thing, actor->Pos, actor->thing.size, actor->thing.Vel,
			params);
		if (collidingItem && collidingItem->kind == KIND_CHARACTER)
		{
			TActor *collidingActor = CArrayGet(&gActors, collidingItem->id);
			if (CalcCollisionTeam(1, collidingActor) ==
				CalcCollisionTeam(1, actor))
			{
				struct vec2 v =
					svec2_subtract(actor->Pos, collidingActor->Pos);
				if (svec2_is_zero(v))
				{
					v = svec2(1, 0);
				}
				v = svec2_scale(svec2_normalize(v), REPEL_STRENGTH);
				GameEvent e = GameEventNew(GAME_EVENT_ACTOR_IMPULSE);
				e.u.ActorImpulse.UID = actor->uid;
				e.u.ActorImpulse.Vel = Vec2ToNet(v);
				e.u.ActorImpulse.Pos = Vec2ToNet(actor->Pos);
				GameEventsEnqueue(&gGameEvents, e);
				e.u.ActorImpulse.UID = collidingActor->uid;
				e.u.ActorImpulse.Vel = Vec2ToNet(svec2_scale(v, -1));
				e.u.ActorImpulse.Pos = Vec2ToNet(collidingActor->Pos);
				GameEventsEnqueue(&gGameEvents, e);
			}
		}
	}
	// If low on health, bleed
	if (ActorIsLowHealth(actor))
	{
		actor->bleedCounter -= ticks;
		if (actor->bleedCounter <= 0)
		{
			ActorAddBloodSplatters(actor, 1, 1.0f, svec2_zero());
			actor->bleedCounter += ActorGetHealthPercent(actor);
		}
	}
	CA_FOREACH_END()
}
static void CheckManualPickups(TActor *a);
static void ActorUpdatePosition(TActor *actor, int ticks)
{
	struct vec2 newPos;
	// If piloting a vehicle, set position to that of vehicle
	if (actor->vehicleUID != -1)
	{
		const TActor *vehicle = ActorGetByUID(actor->vehicleUID);
		newPos = vehicle->Pos;
	}
	else
	{
		newPos = svec2_add(actor->Pos, actor->MoveVel);
		if (!svec2_is_zero(actor->thing.Vel))
		{
			newPos =
				svec2_add(newPos, svec2_scale(actor->thing.Vel, (float)ticks));

			for (int i = 0; i < ticks; i++)
			{
				if (actor->thing.Vel.x > FLT_EPSILON)
				{
					actor->thing.Vel.x =
						MAX(0, actor->thing.Vel.x - VEL_DECAY_X);
				}
				else if (actor->thing.Vel.x < -FLT_EPSILON)
				{
					actor->thing.Vel.x =
						MIN(0, actor->thing.Vel.x + VEL_DECAY_X);
				}
				if (actor->thing.Vel.y > FLT_EPSILON)
				{
					actor->thing.Vel.y =
						MAX(0, actor->thing.Vel.y - VEL_DECAY_Y);
				}
				else if (actor->thing.Vel.y < FLT_EPSILON)
				{
					actor->thing.Vel.y =
						MIN(0, actor->thing.Vel.y + VEL_DECAY_Y);
				}
			}
		}
	}

	if (!svec2_is_nearly_equal(actor->Pos, newPos, EPSILON_POS))
	{
		TryMoveActor(actor, newPos);
	}
	// Check if we're standing over any manual pickups
	CheckManualPickups(actor);
}
// Check if the actor is over any manual pickups
static bool CheckManualPickupFunc(
	Thing *ti, void *data, const struct vec2 colA, const struct vec2 colB,
	const struct vec2 normal);
// Check if the actor is over any unpiloted vehicles
static void CheckManualPilot(TActor *a, const CollisionParams params);
static void CheckManualPickups(TActor *a)
{
	// NPCs can't pickup
	if (a->PlayerUID < 0)
		return;
	const CollisionParams params = {
		0, CalcCollisionTeam(true, a), IsPVP(gCampaign.Entry.Mode), false};
	OverlapThings(
		&a->thing, a->Pos, a->thing.Vel, a->thing.size, params,
		CheckManualPickupFunc, a, NULL, NULL, NULL);
	const CollisionParams paramsPilot = {
		THING_IMPASSABLE | THING_CAN_BE_SHOT, params.Team, params.IsPVP, true};
	CheckManualPilot(a, paramsPilot);
}
static bool CheckManualPickupFunc(
	Thing *ti, void *data, const struct vec2 colA, const struct vec2 colB,
	const struct vec2 normal)
{
	UNUSED(colA);
	UNUSED(colB);
	UNUSED(normal);
	TActor *a = data;
	if (ti->kind != KIND_PICKUP)
		return true;
	const Pickup *p = CArrayGet(&gPickups, ti->id);
	if (!PickupIsManual(p))
		return true;
	// "Say" that the weapon must be picked up using a command
	const PlayerData *pData = PlayerDataGetByUID(a->PlayerUID);
	if (pData->IsLocal && IsPlayerHuman(pData))
	{
		char buttonName[64];
		strcpy(buttonName, "");
		InputGetButtonName(
			pData->inputDevice, pData->deviceIndex, CMD_BUTTON2, buttonName);
		// TODO: PickupGetName
		const char *pickupName;
		switch (p->class->Type)
		{
		case PICKUP_AMMO:
			pickupName = AmmoGetById(&gAmmo, p->class->u.Ammo.Id)->Name;
			break;
		case PICKUP_GUN:
			pickupName = IdWeaponClass(p->class->u.GunId)->name;
			break;
		default:
			CASSERT(false, "unknown pickup type");
			pickupName = "???";
			break;
		}
		char buf[256];
		sprintf(buf, "%s to pick up\n%s", buttonName, pickupName);
		ActorSetChatter(a, buf, 2);
	}
	// If co-op AI, alert it so it can try to pick the gun up
	if (a->aiContext != NULL)
	{
		AICoopOnPickupGun(a, p->class->u.GunId);
	}
	a->CanPickupSpecial = true;
	return false;
}
static void CheckManualPilot(TActor *a, const CollisionParams params)
{
	if (a->vehicleUID >= 0)
	{
		return;
	}
	const Thing *ti = OverlapGetFirstItem(
		&a->thing, a->Pos, a->thing.size, a->thing.Vel, params);
	if (ti == NULL || ti->kind != KIND_CHARACTER)
		return;
	const TActor *vehicle = CArrayGet(&gActors, ti->id);
	if (vehicle->pilotUID >= 0)
		return;
	// "Say" that we can pilot using a command
	const PlayerData *pData = PlayerDataGetByUID(a->PlayerUID);
	if (pData->IsLocal && IsPlayerHuman(pData))
	{
		char buttonName[64];
		strcpy(buttonName, "");
		InputGetButtonName(
			pData->inputDevice, pData->deviceIndex, CMD_BUTTON2, buttonName);
		const char *vehicleName = ActorGetCharacter(vehicle)->Class->Name;
		char buf[256];
		sprintf(buf, "%s to pilot\n%s", buttonName, vehicleName);
		ActorSetChatter(a, buf, 2);
	}
	// TODO: co-op AI pilot
	a->CanPickupSpecial = true;
}
static void ActorAddAmmoPickup(const TActor *actor);
static void ActorAddGunPickup(const TActor *actor);
static void ActorDie(TActor *actor)
{
	const Character *c = ActorGetCharacter(actor);
	GameEvent e;
	if (!gCampaign.IsClient)
	{
		if (c->Drop)
		{
			e = GameEventNew(GAME_EVENT_ADD_PICKUP);
			strcpy(e.u.AddPickup.PickupClass, c->Drop->Name);
			e.u.AddPickup.Pos = Vec2ToNet(actor->Pos);
			GameEventsEnqueue(&gGameEvents, e);
		}
		else
		{
			// Add an ammo pickup of the actor's gun
			if (gCampaign.Setting.Ammo)
			{
				ActorAddAmmoPickup(actor);
			}

			ActorAddGunPickup(actor);
		}
	}

	// Add corpse
	if (ConfigGetEnum(&gConfig, "Graphics.Gore") != GORE_NONE)
	{
		GameEvent ea = GameEventNew(GAME_EVENT_MAP_OBJECT_ADD);
		ea.u.MapObjectAdd.UID = ObjsGetNextUID();
		const MapObject *corpse = StrMapObject(c->Class->Corpse);
		if (!corpse)
		{
			corpse = GetRandomBloodPool();
			ea.u.MapObjectAdd.Mask = Color2Net(c->Class->BloodColor);
		}
		strcpy(ea.u.MapObjectAdd.MapObjectClass, corpse->Name);
		ea.u.MapObjectAdd.Pos = Vec2ToNet(actor->Pos);
		ea.u.MapObjectAdd.ThingFlags = MapObjectGetFlags(corpse);
		ea.u.MapObjectAdd.Health = corpse->Health;
		GameEventsEnqueue(&gGameEvents, ea);
	}

	e = GameEventNew(GAME_EVENT_ACTOR_DIE);
	e.u.ActorDie.UID = actor->uid;
	GameEventsEnqueue(&gGameEvents, e);
}
static bool IsUnarmedBot(const TActor *actor);
static void ActorAddAmmoPickup(const TActor *actor)
{
	if (IsUnarmedBot(actor))
	{
		return;
	}

	// Add ammo pickups for each of the actor's guns
	for (int i = 0; i < MAX_WEAPONS; i++)
	{
		const Weapon *w = &actor->guns[i];
		if (w->Gun == NULL)
		{
			continue;
		}

		for (int j = 0; j < WeaponClassNumBarrels(w->Gun); j++)
		{
			const int ammoId = WC_BARREL_ATTR(*(w->Gun), AmmoId, j);
			// Check if the actor's gun has ammo at all
			if (ammoId < 0)
			{
				continue;
			}
			// Don't spawn ammo if no players use it
			if (PlayersNumUseAmmo(ammoId) == 0)
			{
				continue;
			}

			GameEvent e = GameEventNew(GAME_EVENT_ADD_PICKUP);
			const Ammo *a = AmmoGetById(&gAmmo, ammoId);
			sprintf(e.u.AddPickup.PickupClass, "ammo_%s", a->Name);
			// Add a little random offset so the pickups aren't all together
			const struct vec2 offset = svec2(
				(float)RAND_INT(-TILE_WIDTH, TILE_WIDTH) / 2,
				(float)RAND_INT(-TILE_HEIGHT, TILE_HEIGHT) / 2);
			e.u.AddPickup.Pos = Vec2ToNet(svec2_add(actor->Pos, offset));
			GameEventsEnqueue(&gGameEvents, e);
		}
	}
}
static bool HasGunPickups(const WeaponClass *wc, const int n);
static void ActorAddGunPickup(const TActor *actor)
{
	if (IsUnarmedBot(actor))
	{
		return;
	}

	// Select a valid gun to drop
	for (int i = 0; i < MAX_WEAPONS; i++)
	{
		const WeaponClass *wc = actor->guns[i].Gun;
		if (wc == NULL)
			continue;
		if (!wc->CanDrop)
			continue;
		if (wc->DropGun)
		{
			wc = StrWeaponClass(wc->DropGun);
			CASSERT(wc != NULL, "Cannot find gun to drop");
		}
		// Don't drop gun if there's gun pickups for this already
		if (HasGunPickups(wc, 2))
			continue;
		PickupAddGun(wc, actor->Pos);
		break;
	}
}
static bool HasGunPickups(const WeaponClass *wc, const int n)
{
	const int wcId = WeaponClassId(wc);
	int count = 0;
	CA_FOREACH(const Pickup, p, gPickups)
	if (!p->isInUse)
	{
		continue;
	}
	if (p->class->Type == PICKUP_GUN && p->class->u.GunId == wcId)
	{
		count++;
		if (count >= n)
		{
			return true;
		}
	}
	CA_FOREACH_END()
	return false;
}
static bool IsUnarmedBot(const TActor *actor)
{
	// Note: if the actor is AI with no shooting time,
	// then it's an unarmed actor
	const Character *c = ActorGetCharacter(actor);
	return c->bot != NULL && c->bot->probabilityToShoot == 0;
}

static void VehicleTakePilot(const TActor *vehicle);
// Check whether actors are overlapping with unpiloted vehicles,
// and automatically pilot them
// Only called at start of mission
void ActorsPilotVehicles(void)
{
	CA_FOREACH(const TActor, vehicle, gActors)
	if (!vehicle->isInUse)
	{
		continue;
	}
	if (vehicle->pilotUID == -1)
	{
		VehicleTakePilot(vehicle);
	}
	CA_FOREACH_END()
}
static void VehicleTakePilot(const TActor *vehicle)
{
	CA_FOREACH(const TActor, pilot, gActors)
	if (!pilot->isInUse)
	{
		continue;
	}
	if (pilot->pilotUID == pilot->uid &&
		AABBOverlap(
			vehicle->Pos, pilot->Pos, vehicle->thing.size, pilot->thing.size))
	{
		GameEvent e = GameEventNew(GAME_EVENT_ACTOR_PILOT);
		e.u.Pilot.On = true;
		e.u.Pilot.UID = pilot->uid;
		e.u.Pilot.VehicleUID = vehicle->uid;
		GameEventsEnqueue(&gGameEvents, e);
		break;
	}
	CA_FOREACH_END()
}

void ActorsInit(void)
{
	CArrayInit(&gActors, sizeof(TActor));
	CArrayReserve(&gActors, 64);
	sActorUIDs = 0;
}
void ActorsTerminate(void)
{
	CA_FOREACH(TActor, a, gActors)
	if (!a->isInUse)
		continue;
	ActorDestroy(a);
	CA_FOREACH_END()
	CArrayTerminate(&gActors);
}
int ActorsGetNextUID(void)
{
	return sActorUIDs++;
}
int ActorsGetFreeIndex(void)
{
	// Find an empty slot in actor list
	// actors.size if no slot found (i.e. add to end)
	CA_FOREACH(const TActor, a, gActors)
	if (!a->isInUse)
	{
		return _ca_index;
	}
	CA_FOREACH_END()
	return (int)gActors.size;
}

static void GoreEmitterInit(Emitter *em, const char *particleClassName);
TActor *ActorAdd(NActorAdd aa)
{
	// Don't add if UID exists
	if (ActorGetByUID(aa.UID) != NULL)
	{
		LOG(LM_ACTOR, LL_DEBUG, "actor uid(%d) already exists; not adding",
			(int)aa.UID);
		return NULL;
	}
	const int id = ActorsGetFreeIndex();
	while (id >= (int)gActors.size)
	{
		TActor a;
		memset(&a, 0, sizeof a);
		CArrayPushBack(&gActors, &a);
	}
	TActor *actor = CArrayGet(&gActors, id);
	memset(actor, 0, sizeof *actor);
	actor->uid = aa.UID;
	LOG(LM_ACTOR, LL_DEBUG, "add actor uid(%d) playerUID(%d)", actor->uid,
		aa.PlayerUID);
	actor->pilotUID = aa.PilotUID;
	actor->vehicleUID = aa.VehicleUID;
	CArrayInit(&actor->ammo, sizeof(int));
	for (int i = 0; i < AmmoGetNumClasses(&gAmmo); i++)
	{
		// Initialise with twice the standard ammo amount
		const int amount =
			AmmoGetById(&gAmmo, i)->Amount * AMMO_STARTING_MULTIPLE;
		CArrayPushBack(&actor->ammo, &amount);
	}
	if (gMission.missionData->WeaponPersist)
	{
		for (int i = 0; i < aa.Ammo_count; i++)
		{
			// Use persisted ammo amount if it is greater
			if ((int)aa.Ammo[i].Amount >
				AmmoGetById(&gAmmo, aa.Ammo[i].Id)->Amount *
					AMMO_STARTING_MULTIPLE)
			{
				CArraySet(&actor->ammo, aa.Ammo[i].Id, &aa.Ammo[i].Amount);
			}
		}
	}
	actor->PlayerUID = aa.PlayerUID;
	actor->charId = aa.CharId;
	const Character *c = ActorGetCharacter(actor);
	if (aa.PlayerUID >= 0)
	{
		// Add all player weapons
		PlayerData *p = PlayerDataGetByUID(aa.PlayerUID);
		for (int i = 0; i < MAX_WEAPONS; i++)
		{
			Weapon gun = WeaponCreate(p->guns[i]);
			actor->guns[i] = gun;
			if (i < MAX_GUNS && ACTOR_GET_GUN(actor)->Gun == NULL)
			{
				actor->gunIndex = i;
			}
			if (i >= MAX_GUNS && ACTOR_GET_GRENADE(actor)->Gun == NULL)
			{
				actor->grenadeIndex = i - MAX_GUNS;
			}
		}
		p->ActorUID = aa.UID;
	}
	else
	{
		// Add sole weapon from character type
		Weapon gun = WeaponCreate(c->Gun);
		actor->guns[0] = gun;
		actor->gunIndex = 0;
	}
	actor->health = aa.Health;
	actor->action = ACTORACTION_MOVING;
	actor->thing.Pos.x = actor->thing.Pos.y = -1;
	actor->thing.kind = KIND_CHARACTER;
	actor->thing.drawFunc = NULL;
	actor->thing.size = svec2i(ACTOR_W, ACTOR_H);
	actor->thing.flags = THING_IMPASSABLE | THING_CAN_BE_SHOT | aa.ThingFlags;
	actor->thing.id = id;
	actor->isInUse = true;

	actor->flags = FLAGS_SLEEPING | c->flags;
	// Flag corrections
	if (actor->flags & FLAGS_AWAKEALWAYS)
	{
		actor->flags &= ~FLAGS_SLEEPING;
	}
	// Rescue objectives always have follower flag on
	if (actor->thing.flags & THING_OBJECTIVE)
	{
		const Objective *o = CArrayGet(
			&gMission.missionData->Objectives,
			ObjectiveFromThing(actor->thing.flags));
		if (o->Type == OBJECTIVE_RESCUE)
		{
			// If they don't have prisoner flag set, automatically rescue
			// them
			if (!(actor->flags & FLAGS_PRISONER) && !gCampaign.IsClient)
			{
				GameEvent e = GameEventNew(GAME_EVENT_RESCUE_CHARACTER);
				e.u.Rescue.UID = aa.UID;
				GameEventsEnqueue(&gGameEvents, e);
				UpdateMissionObjective(
					&gMission, actor->thing.flags, OBJECTIVE_RESCUE, 1);
			}
		}
	}

	actor->direction = aa.Direction;
	actor->DrawRadians = (float)dir2radians[actor->direction];
	actor->anim = AnimationGetActorAnimation(ACTORANIMATION_IDLE);
	actor->slideLock = 0;
	if (c->bot)
	{
		actor->aiContext = AIContextNew();
		ActorSetAIState(actor, AI_STATE_IDLE);
	}

	EmitterInit(
		&actor->barrelSmoke, StrParticleClass(&gParticleClasses, "smoke"),
		svec2_zero(), -0.05f, 0.05f, 3, 3, 0, 0, 10);
	EmitterInit(
		&actor->healEffect, StrParticleClass(&gParticleClasses, "health_plus"),
		svec2_zero(), -0.1f, 0.1f, 0, 0, 0, 0, 0);
	GoreEmitterInit(&actor->blood1, "blood1");
	GoreEmitterInit(&actor->blood2, "blood2");
	GoreEmitterInit(&actor->blood3, "blood3");

	TryMoveActor(actor, NetToVec2(aa.Pos));

	return actor;
}
static void GoreEmitterInit(Emitter *em, const char *particleClassName)
{
	EmitterInit(
		em, StrParticleClass(&gParticleClasses, particleClassName),
		svec2_zero(), 0, GORE_EMITTER_MAX_SPEED, 6, 12, -0.1, 0.1, 0);
}

void ActorDestroy(TActor *a)
{
	CASSERT(a->isInUse, "Destroying in-use actor");
	CArrayTerminate(&a->ammo);
	MapRemoveThing(&gMap, &a->thing);
	// Set PlayerData's ActorUID to -1 to signify actor destruction
	PlayerData *p = PlayerDataGetByUID(a->PlayerUID);
	if (p != NULL)
		p->ActorUID = -1;
	AIContextDestroy(a->aiContext);
	a->isInUse = false;
}

TActor *ActorGetByUID(const int uid)
{
	CA_FOREACH(TActor, a, gActors)
	if (a->uid == uid)
	{
		return a;
	}
	CA_FOREACH_END()
	return NULL;
}

const Character *ActorGetCharacter(const TActor *a)
{
	if (a->PlayerUID >= 0)
	{
		return &PlayerDataGetByUID(a->PlayerUID)->Char;
	}
	return CArrayGet(&gCampaign.Setting.characters.OtherChars, a->charId);
}

struct vec2 ActorGetAverageWeaponMuzzleOffset(const TActor *a)
{
	const Weapon *w = ACTOR_GET_WEAPON(a);
	struct vec2 offset = svec2_zero();
	for (int i = 0; i < WeaponClassNumBarrels(w->Gun); i++)
	{
		offset = svec2_add(offset, ActorGetMuzzleOffset(a, w, i));
	}
	return svec2_scale(offset, 1.0f / (float)WeaponClassNumBarrels(w->Gun));
}
struct vec2 ActorGetMuzzleOffset(
	const TActor *a, const Weapon *w, const int barrel)
{
	const Character *c = ActorGetCharacter(a);
	const CharSprites *cs = c->Class->Sprites;
	return WeaponClassGetBarrelMuzzleOffset(
		w->Gun, cs, barrel, a->direction, w->barrels[barrel].state);
}
int ActorWeaponGetAmmo(
	const TActor *a, const WeaponClass *wc, const int barrel)
{
	const int ammoId = WC_BARREL_ATTR(*wc, AmmoId, barrel);
	if (ammoId == -1)
	{
		return -1;
	}
	return *(int *)CArrayGet(&a->ammo, ammoId);
}
int ActorGetCanFireBarrel(const TActor *a, const Weapon *w)
{
	if (w->Gun == NULL)
	{
		return -1;
	}
	const int barrel = WeaponGetUnlockedBarrel(w);
	if (barrel == -1)
	{
		return -1;
	}
	const bool hasAmmo = ActorWeaponGetAmmo(a, w->Gun, barrel) != 0;
	if (gCampaign.Setting.Ammo && !hasAmmo)
	{
		// TODO: multi guns not firing if the first gun is out of ammo
		return -1;
	}
	return barrel;
}
bool ActorTrySwitchWeapon(const TActor *a, const bool allGuns)
{
	// Find the next weapon to switch to
	// If the player does not have a grenade key set, allow switching to
	// grenades (classic style)
	const int switchCount = allGuns ? MAX_WEAPONS : MAX_GUNS;
	const int startIndex =
		ActorGetNumGuns(a) > 0 ? a->gunIndex : a->grenadeIndex + MAX_GUNS;
	int weaponIndex = startIndex;
	do
	{
		weaponIndex = (weaponIndex + 1) % switchCount;
	} while (a->guns[weaponIndex].Gun == NULL);
	if (weaponIndex == startIndex)
	{
		// No other weapon to switch to
		return false;
	}

	GameEvent e = GameEventNew(GAME_EVENT_ACTOR_SWITCH_GUN);
	e.u.ActorSwitchGun.UID = a->uid;
	e.u.ActorSwitchGun.GunIdx = weaponIndex;
	GameEventsEnqueue(&gGameEvents, e);
	return true;
}
void ActorSwitchGun(const NActorSwitchGun sg)
{
	TActor *a = ActorGetByUID(sg.UID);
	if (a == NULL || !a->isInUse)
		return;
	a->gunIndex = sg.GunIdx;
	const WeaponClass *gun = ACTOR_GET_WEAPON(a)->Gun;
	SoundPlayAt(&gSoundDevice, gun->SwitchSound, a->thing.Pos);
	ActorSetChatter(a, gun->name, CHATTER_SWITCH_GUN);
}

bool ActorIsImmune(const TActor *actor, const special_damage_e damage)
{
	switch (damage)
	{
	case SPECIAL_FLAME:
		return actor->flags & FLAGS_ASBESTOS;
	case SPECIAL_POISON:
	case SPECIAL_PETRIFY: // fallthrough
		return actor->flags & FLAGS_IMMUNITY;
	case SPECIAL_CONFUSE:
		return actor->flags & FLAGS_IMMUNITY;
	default:
		break;
	}
	// Don't bother if health already 0 or less
	if (actor->health <= 0)
	{
		return 1;
	}
	return 0;
}

bool ActorTakesDamage(const TActor *actor, const special_damage_e damage)
{
	switch (damage)
	{
	case SPECIAL_FLAME:
		return !(actor->flags & FLAGS_ASBESTOS);
	default:
		return true;
	}
}

#define MAX_POISONED_COUNT 140

static void ActorTakeSpecialDamage(
	TActor *actor, const special_damage_e damage, const int ticks)
{
	if (ActorIsImmune(actor, damage))
	{
		return;
	}
	switch (damage)
	{
	case SPECIAL_FLAME:
		actor->flamed = ticks;
		break;
	case SPECIAL_POISON:
		actor->poisoned = MAX(actor->poisoned + ticks, MAX_POISONED_COUNT);
		break;
	case SPECIAL_PETRIFY:
		if (!actor->petrified)
		{
			actor->petrified = ticks;
		}
		break;
	case SPECIAL_CONFUSE:
		actor->confused = ticks;
		break;
	default:
		// do nothing
		break;
	}
}

static void ActorTakeHit(
	TActor *actor, const int flags, const int sourceUID,
	const special_damage_e damage, const int specialTicks);
void ActorHit(const NThingDamage d)
{
	TActor *a = ActorGetByUID(d.UID);
	if (!a->isInUse)
		return;

	ActorTakeHit(a, d.Flags, d.SourceActorUID, d.Special, d.SpecialTicks);
	if (d.Power > 0)
	{
		DamageActor(a, d.Power, d.SourceActorUID);

		// Add damage text
		// See if there is one already; if so remove it and add a new one,
		// combining the damage numbers
		int damage = (int)d.Power;
		struct vec2 pos =
			svec2_add(a->Pos, svec2(RAND_FLOAT(-3, 3), RAND_FLOAT(-3, 3)));
		CA_FOREACH(const Particle, p, gParticles)
		if (p->isInUse && p->ActorUID == a->uid)
		{
			damage += a->accumulatedDamage;
			if (svec2_distance(pos, p->Pos) <
				DAMAGE_TEXT_DISTANCE_RESET_THRESHOLD)
			{
				pos = p->Pos;
			}
			GameEvent e = GameEventNew(GAME_EVENT_PARTICLE_REMOVE);
			e.u.ParticleRemoveId = _ca_index;
			GameEventsEnqueue(&gGameEvents, e);
			break;
		}
		CA_FOREACH_END()
		a->accumulatedDamage = damage;

		GameEvent s = GameEventNew(GAME_EVENT_ADD_PARTICLE);
		s.u.AddParticle.Class =
			StrParticleClass(&gParticleClasses, "damage_text");
		s.u.AddParticle.ActorUID = a->uid;
		s.u.AddParticle.Pos = pos;
		s.u.AddParticle.Z = BULLET_Z * Z_FACTOR;
		s.u.AddParticle.DZ = 3;
		sprintf(s.u.AddParticle.Text, "-%d", damage);
		GameEventsEnqueue(&gGameEvents, s);

		ActorAddBloodSplatters(a, d.Power, d.Mass, NetToVec2(d.Vel));

		// Rumble if taking hit
		if (a->PlayerUID >= 0)
		{
			const PlayerData *p = PlayerDataGetByUID(a->PlayerUID);
			if (p->inputDevice == INPUT_DEVICE_JOYSTICK)
			{
				JoyImpact(p->deviceIndex);
			}
		}

		a->grimaceCounter = GRIMACE_HIT_TICKS;
	}
}

static void ActorTakeHit(
	TActor *actor, const int flags, const int sourceUID,
	const special_damage_e damage, const int specialTicks)
{
	// Wake up if this is an AI
	if (!gCampaign.IsClient)
	{
		AIWake(actor, 1);
	}
	const TActor *source = ActorGetByUID(sourceUID);
	const int playerUID = source != NULL ? source->PlayerUID : -1;
	if (ActorIsInvulnerable(
			actor, flags, playerUID, gCampaign.Entry.Mode, damage))
	{
		return;
	}
	ActorTakeSpecialDamage(actor, damage, specialTicks);
}

bool ActorIsInvulnerable(
	const TActor *actor, const int flags, const int playerUID,
	const GameMode mode, const special_damage_e special)
{
	if (actor->flags & FLAGS_INVULNERABLE)
	{
		return true;
	}

	if (!(flags & FLAGS_HURTALWAYS) && !(actor->flags & FLAGS_VICTIM))
	{
		// Same player hits
		if (playerUID >= 0 && playerUID == actor->PlayerUID)
		{
			return true;
		}
		const bool isGood = playerUID >= 0 || (flags & FLAGS_GOOD_GUY);
		const bool isTargetGood =
			actor->PlayerUID >= 0 || (actor->flags & FLAGS_GOOD_GUY);
		// Friendly fire (NPCs)
		if (!IsPVP(mode) && !ConfigGetBool(&gConfig, "Game.FriendlyFire") &&
			isGood && isTargetGood)
		{
			return true;
		}
		// Enemies don't hurt each other
		if (!isGood && !isTargetGood)
		{
			return true;
		}
		if (!ActorTakesDamage(actor, special))
		{
			return true;
		}
	}

	return false;
}

static void ActorAddBloodSplatters(
	TActor *a, const int power, const float mass, const struct vec2 hitVector)
{
	const GoreAmount ga = ConfigGetEnum(&gConfig, "Graphics.Gore");
	if (ga == GORE_NONE)
		return;

	// Emit blood based on power and gore setting
	int bloodPower = power * 2;
	// Randomly cycle through the blood types
	int bloodSize = 1;
	const struct vec2 hitVNorm = svec2_normalize(hitVector);
	const float speedBase = MAX(1.0f, mass) * SHOT_IMPULSE_FACTOR;
	while (bloodPower > 0)
	{
		Emitter *em = NULL;
		switch (bloodSize)
		{
		case 1:
			em = &a->blood1;
			break;
		case 2:
			em = &a->blood2;
			break;
		default:
			em = &a->blood3;
			break;
		}
		bloodSize++;
		if (bloodSize > 3)
		{
			bloodSize = 1;
		}
		const struct vec2 vel =
			svec2_scale(hitVNorm, speedBase * RAND_FLOAT(0.5f, 1));
		AddParticle ap;
		memset(&ap, 0, sizeof ap);
		ap.Pos = a->Pos;
		ap.Angle = NAN;
		ap.Z = 10;
		ap.Vel = vel;
		ap.Mask = ActorGetCharacter(a)->Class->BloodColor;
		EmitterStart(em, &ap);
		switch (ga)
		{
		case GORE_LOW:
			bloodPower /= 8;
			break;
		case GORE_MEDIUM:
			bloodPower /= 2;
			break;
		default:
			bloodPower = bloodPower * 7 / 8;
			break;
		}
	}
}

int ActorGetHealthPercent(const TActor *a)
{
	const int maxHealth = ActorGetCharacter(a)->maxHealth;
	return a->health * 100 / maxHealth;
}

bool ActorIsLowHealth(const TActor *a)
{
	return ActorGetHealthPercent(a) < LOW_HEALTH_PERCENTAGE;
}

bool ActorIsGrimacing(const TActor *a)
{
	const Weapon *gun = ACTOR_GET_WEAPON(a);
	if (gun->Gun)
	{
		for (int i = 0; i < WeaponClassNumBarrels(gun->Gun); i++)
		{
			if (gun->barrels[i].state == GUNSTATE_FIRING ||
				gun->barrels[i].state == GUNSTATE_RECOIL)
			{
				return true;
			}
		}
	}
	return (a->grimaceCounter % GRIMACE_PERIOD) > GRIMACE_PERIOD / 2;
}

bool ActorIsLocalPlayer(const int uid)
{
	const TActor *a = ActorGetByUID(uid);
	// Don't accept updates if actor doesn't exist
	// This can happen in the very first frame, where we haven't yet
	// processed an actor add message
	// Otherwise this shouldn't happen
	if (a == NULL)
		return true;

	return PlayerIsLocal(a->PlayerUID);
}
