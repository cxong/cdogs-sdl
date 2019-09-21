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

    Copyright (c) 2013-2019 Cong Xu
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
#include "ai_coop.h"
#include "ai_utils.h"
#include "ammo.h"
#include "character.h"
#include "collision/collision.h"
#include "config.h"
#include "damage.h"
#include "draw/drawtools.h"
#include "events.h"
#include "game_events.h"
#include "log.h"
#include "pic_manager.h"
#include "sounds.h"
#include "thing.h"
#include "defs.h"
#include "pickup.h"
#include "gamedata.h"
#include "triggers.h"
#include "mission.h"
#include "game.h"
#include "utils.h"

#define FOOTSTEP_DISTANCE_PLUS 250
#define FOOTSTEP_MAX_ANIM_SPEED 2
#define REPEL_STRENGTH 0.06f
#define SLIDE_LOCK 50
#define SLIDE_X (TILE_WIDTH / 3)
#define SLIDE_Y (TILE_HEIGHT / 3)
#define VEL_DECAY_X (TILE_WIDTH * 2 / 256.0f)
#define VEL_DECAY_Y (TILE_WIDTH * 2 / 256.0f)	// Note: deliberately tile width
#define SOUND_LOCK_WEAPON_CLICK 20
#define DROP_GUN_CHANCE 0.2
#define DRAW_RADIAN_SPEED (MPI/16)
// Percent of health considered low; bleed and flash HUD if low
#define LOW_HEALTH_PERCENTAGE 25
#define GORE_EMITTER_MAX_SPEED 0.25f
#define CHATTER_SWITCH_GUN 45 // TODO: based on clock time instead of game ticks


CArray gPlayerIds;


CArray gActors;
static unsigned int sActorUIDs = 0;


void ActorSetState(TActor *actor, const ActorAnimation state)
{
	actor->anim = AnimationGetActorAnimation(state);
}

static void ActorUpdateWeapon(TActor *a, Weapon *w, const int ticks);
static void CheckPickups(TActor *actor);
void UpdateActorState(TActor * actor, int ticks)
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

	if (actor->health <= 0) {
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
		actor->DrawRadians += (float)MIN(DRAW_RADIAN_SPEED*ticks, -dr);
	}
	else if (dr > 0)
	{
		actor->DrawRadians -= (float)MIN(DRAW_RADIAN_SPEED*ticks, dr);
	}

	// Footstep sounds
	// Step on 2 and 6
	// TODO: custom animation and footstep frames
	if (ConfigGetBool(&gConfig, "Sound.Footsteps") &&
		actor->anim.Type == ACTORANIMATION_WALKING &&
		(AnimationGetFrame(&actor->anim) == 2 ||
		AnimationGetFrame(&actor->anim) == 6) &&
		actor->anim.newFrame)
	{
		SoundPlayAtPlusDistance(
			&gSoundDevice, StrSound("footsteps"), actor->thing.Pos,
			FOOTSTEP_DISTANCE_PLUS);
	}

	// Animation
	float animTicks = 1;
	if (actor->anim.Type == ACTORANIMATION_WALKING)
	{
		// Update walk animation based on actor speed
		animTicks = MIN(
			svec2_length(svec2_add(actor->MoveVel, actor->thing.Vel)),
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
}
static void ActorUpdateWeapon(TActor *a, Weapon *w, const int ticks)
{
	if (w->Gun == NULL)
	{
		return;
	}
	WeaponUpdate(w, ticks);
	ActorFireUpdate(w, a, ticks);
	if (WeaponClassHasMuzzle(w->Gun) && WeaponIsOverheating(w))
	{
		AddParticle ap;
		memset(&ap, 0, sizeof ap);
		ap.Pos = svec2_add(a->Pos, ActorGetMuzzleOffset(a, w));
		ap.Z = WeaponClassGetMuzzleHeight(w->Gun, w->state) / Z_FACTOR;
		ap.Mask = colorWhite;
		EmitterUpdate(&a->barrelSmoke, &ap, ticks);
	}
}

static struct vec2 GetConstrainedPos(
	const Map *map, const struct vec2 from, const struct vec2 to,
	const struct vec2i size);
static void OnMove(TActor *a);
bool TryMoveActor(TActor *actor, struct vec2 pos)
{
	CASSERT(!svec2_is_nearly_equal(actor->Pos, pos, EPSILON_POS),
		"trying to move to same position");

	actor->hasCollided = true;
	actor->CanPickupSpecial = false;

	const struct vec2 oldPos = actor->Pos;
	pos = GetConstrainedPos(&gMap, actor->Pos, pos, actor->thing.size);
	if (svec2_is_nearly_equal(oldPos, pos, EPSILON_POS))
	{
		return false;
	}

	// Check for object collisions
	// Only do this if we are the owner of the actor, since this may lead to
	// melee damage
	if ((!gCampaign.IsClient && actor->PlayerUID < 0) ||
		ActorIsLocalPlayer(actor->uid))
	{
		const CollisionParams params =
		{
			THING_IMPASSABLE, CalcCollisionTeam(true, actor),
			IsPVP(gCampaign.Entry.Mode)
		};
		Thing *target = OverlapGetFirstItem(
			&actor->thing, pos, actor->thing.size, params);
		if (target)
		{
			Weapon *gun = ACTOR_GET_WEAPON(actor);
			const TObject *object = target->kind == KIND_OBJECT ?
				CArrayGet(&gObjs, target->id) : NULL;
			if (ActorCanFireWeapon(actor, gun) && !gun->Gun->CanShoot &&
				actor->health > 0 &&
				(!object || !ObjIsDangerous(object)))
			{
				if (CanHit(actor->flags, actor->uid, target))
				{
					// Tell the server that we want to melee something
					GameEvent e = GameEventNew(GAME_EVENT_ACTOR_MELEE);
					e.u.Melee.UID = actor->uid;
					strcpy(e.u.Melee.BulletClass, gun->Gun->Bullet->Name);
					e.u.Melee.TargetKind = target->kind;
					switch (target->kind)
					{
					case KIND_CHARACTER:
						e.u.Melee.TargetUID =
							((const TActor *)CArrayGet(&gActors, target->id))->uid;
						e.u.Melee.HitType = HIT_FLESH;
						break;
					case KIND_OBJECT:
						e.u.Melee.TargetUID =
							((const TObject *)CArrayGet(&gObjs, target->id))->uid;
						e.u.Melee.HitType = HIT_OBJECT;
						break;
					default:
						CASSERT(false, "cannot damage target kind");
						break;
					}
					gun->lock = gun->Gun->Lock;
					if (gun->soundLock <= 0)
					{
						gun->soundLock += gun->Gun->SoundLockLength;
					}
					else
					{
						e.u.Melee.HitType = (int)HIT_NONE;
					}
					GameEventsEnqueue(&gGameEvents, e);
				}
				return false;
			}

			const struct vec2 yPos = svec2(actor->Pos.x, pos.y);
			if (OverlapGetFirstItem(
				&actor->thing, yPos, actor->thing.size, params))
			{
				pos.y = actor->Pos.y;
			}
			const struct vec2 xPos = svec2(pos.x, actor->Pos.y);
			if (OverlapGetFirstItem(
				&actor->thing, xPos, actor->thing.size, params))
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
	if (!nearly_equal(dv.x, 0.0f, EPSILON_POS) && !nearly_equal(dv.y, 0.0f, EPSILON_POS))
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
		const struct vec2 diag1Vec =
			svec2_add(from, svec2(dv.x, -dv.x));
		if (!IsCollisionDiamond(map, diag1Vec, size))
		{
			return diag1Vec;
		}
		const struct vec2 diag2Vec =
			svec2_add(from, svec2(dv.x, dv.x));
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
	if (a == NULL || !a->isInUse) return;
	a->Pos = NetToVec2(am.Pos);
	a->MoveVel = NetToVec2(am.MoveVel);
	OnMove(a);
}
static void CheckTrigger(const struct vec2i tilePos, const bool showLocked);
static void CheckRescue(const TActor *a);
static void OnMove(TActor *a)
{
	MapTryMoveThing(&gMap, &a->thing, a->Pos);
	if (MapIsTileInExit(&gMap, &a->thing))
	{
		a->action = ACTORACTION_EXITING;
	}
	else
	{
		a->action = ACTORACTION_MOVING;
	}

	if (!gCampaign.IsClient)
	{
		CheckTrigger(Vec2ToTile(a->Pos), ActorIsLocalPlayer(a->uid));

		CheckPickups(a);

		CheckRescue(a);
	}
}
static void CheckTrigger(const struct vec2i tilePos, const bool showLocked)
{
	const Tile *t = MapGetTile(&gMap, tilePos);
	CA_FOREACH(Trigger *, tp, t->triggers)
		if (!TriggerTryActivate(*tp, gMission.KeyFlags, tilePos) &&
			(*tp)->isActive &&
			TriggerCannotActivate(*tp) &&
			showLocked)
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
static void CheckPickups(TActor *actor)
{
	// NPCs can't pickup
	if (actor->PlayerUID < 0)
	{
		return;
	}
	const CollisionParams params =
	{
		0, CalcCollisionTeam(true, actor), IsPVP(gCampaign.Entry.Mode)
	};
	OverlapThings(
		&actor->thing, actor->Pos, actor->thing.size,
		params, CheckPickupFunc, actor, NULL, NULL, NULL);
}
static bool CheckPickupFunc(
	Thing *ti, void *data, const struct vec2 colA, const struct vec2 colB,
	const struct vec2 normal)
{
	UNUSED(colA);
	UNUSED(colB);
	UNUSED(normal);
	// Always return true, as we can pickup multiple items in one go
	if (ti->kind != KIND_PICKUP) return true;
	TActor *a = data;
	PickupPickup(a, CArrayGet(&gPickups, ti->id), a->PickupAll);
	return true;
}
static void CheckRescue(const TActor *a)
{
	// NPCs can't rescue
	if (a->PlayerUID < 0) return;

	// Check an area slightly bigger than the actor's size for rescue
	// objectives
#define RESCUE_CHECK_PAD 2
	const CollisionParams params =
	{
		THING_IMPASSABLE, CalcCollisionTeam(true, a),
		IsPVP(gCampaign.Entry.Mode)
	};
	const Thing *target = OverlapGetFirstItem(
		&a->thing, a->Pos,
		svec2i_add(a->thing.size, svec2i(RESCUE_CHECK_PAD, RESCUE_CHECK_PAD)),
		params);
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
}

void InjureActor(TActor * actor, int injury)
{
	const int lastHealth = actor->health;
	actor->health -= injury;
	if (lastHealth > 0 && actor->health <= 0)
	{
		actor->stateCounter = 0;
		SoundPlayAt(
			&gSoundDevice,
			StrSound(ActorGetCharacter(actor)->Class->Sounds.Aargh),
			actor->thing.Pos);
		if (actor->PlayerUID >= 0)
		{
			SoundPlayAt(
				&gSoundDevice,
				StrSound("hahaha"),
				actor->thing.Pos);
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
		if (actor->guns[i].Gun != NULL && actor->guns[i].Gun->AmmoId == ammoId)
		{
			return true;
		}
	}
	return false;
}

void ActorReplaceGun(const NActorReplaceGun rg)
{
	TActor *a = ActorGetByUID(rg.UID);
	if (a == NULL || !a->isInUse) return;
	const WeaponClass *wc = StrWeaponClass(rg.Gun);
	CASSERT(wc != NULL, "cannot find gun");
	// If player already has gun, don't do anything
	if (ActorHasGun(a, wc))
	{
		return;
	}
	LOG(LM_ACTOR, LL_DEBUG, "actor uid(%d) replacing gun(%s) idx(%d)",
		(int)rg.UID, rg.Gun, rg.GunIdx);
	Weapon w = WeaponCreate(wc);
	memcpy(&a->guns[rg.GunIdx], &w, sizeof w);
	// Switch immediately to picked up gun
	const PlayerData *p = PlayerDataGetByUID(a->PlayerUID);
	if (wc->IsGrenade && PlayerHasGrenadeButton(p))
	{
		a->grenadeIndex = rg.GunIdx - MAX_GUNS;
	}
	else
	{
		a->gunIndex = rg.GunIdx;
	}

	SoundPlayAt(&gSoundDevice, wc->SwitchSound, a->Pos);
}

bool ActorHasGun(const TActor *a, const WeaponClass *wc)
{
	for (int i = 0; i < MAX_WEAPONS; i++)
	{
		if (a->guns[i].Gun == wc)
		{
			return true;
		}
	}
	return false;
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
	if (AIContextSetState(actor->aiContext, s))
	{
		ActorSetChatter(
			actor,
			AIStateGetChatterText(actor->aiContext->State),
			AIContextShowChatter(
				actor->aiContext,
				ConfigGetEnum(&gConfig, "Interface.AIChatter"))
		);
	}
}

static void FireWeapon(TActor *a, Weapon *w)
{
	if (w->Gun == NULL)
	{
		return;
	}
	if (!ActorCanFireWeapon(a, w))
	{
		if (!WeaponIsLocked(w) && ConfigGetBool(&gConfig, "Game.Ammo"))
		{
			CASSERT(ActorWeaponGetAmmo(a, w->Gun) == 0, "should be out of ammo");
			// Play a clicking sound if this weapon is out of ammo
			if (w->clickLock <= 0)
			{
				SoundPlayAt(&gSoundDevice, StrSound("click"), a->Pos);
				w->clickLock = SOUND_LOCK_WEAPON_CLICK;
			}
		}
		return;
	}
	ActorFire(w, a);
	if (a->PlayerUID >= 0)
	{
		if (ConfigGetBool(&gConfig, "Game.Ammo") && w->Gun->AmmoId >= 0)
		{
			GameEvent e = GameEventNew(GAME_EVENT_ACTOR_USE_AMMO);
			e.u.UseAmmo.UID = a->uid;
			e.u.UseAmmo.PlayerUID = a->PlayerUID;
			e.u.UseAmmo.AmmoId = w->Gun->AmmoId;
			e.u.UseAmmo.Amount = 1;
			GameEventsEnqueue(&gGameEvents, e);
		}
		else if (w->Gun->Cost != 0)
		{
			// Classic C-Dogs score consumption
			GameEvent e = GameEventNew(GAME_EVENT_SCORE);
			e.u.Score.PlayerUID = a->PlayerUID;
			e.u.Score.Score = -w->Gun->Cost;
			GameEventsEnqueue(&gGameEvents, e);
		}
	}
}

static bool ActorTryChangeDirection(
	TActor *actor, const int cmd, const int prevCmd)
{
	const bool willChangeDirecton =
		!actor->petrified &&
		CMD_HAS_DIRECTION(cmd) &&
		(!(cmd & CMD_BUTTON2) || ConfigGetEnum(&gConfig, "Game.SwitchMoveStyle") != SWITCHMOVE_STRAFE) &&
		(!(prevCmd & CMD_BUTTON1) || ConfigGetEnum(&gConfig, "Game.FireMoveStyle") != FIREMOVE_STRAFE);
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
	else if (ACTOR_GET_GUN(actor)->state != GUNSTATE_READY)
	{
		GameEvent e = GameEventNew(GAME_EVENT_GUN_STATE);
		e.u.GunState.ActorUID = actor->uid;
		e.u.GunState.State = GUNSTATE_READY;
		GameEventsEnqueue(&gGameEvents, e);
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
void CommandActor(TActor * actor, int cmd, int ticks)
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
		if (actor->anim.Type != anim)
		{
			GameEvent e = GameEventNew(GAME_EVENT_ACTOR_STATE);
			e.u.ActorState.UID = actor->uid;
			e.u.ActorState.State = (int32_t)anim;
			GameEventsEnqueue(&gGameEvents, e);
		}
	}

	actor->specialCmdDir = CMD_HAS_DIRECTION(cmd);
	if ((cmd & CMD_BUTTON2) && !actor->specialCmdDir)
	{
		// Special: pick up things that can only be picked up on demand
		if (!actor->PickupAll && !(actor->lastCmd & CMD_BUTTON2))
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
		(ConfigGetEnum(&gConfig, "Game.SwitchMoveStyle") == SWITCHMOVE_STRAFE &&
		(cmd & CMD_BUTTON2));
	const bool willMove =
		!actor->petrified && CMD_HAS_DIRECTION(cmd) && canMoveWhenShooting;
	actor->MoveVel = svec2_zero();
	if (willMove)
	{
		const float moveAmount = ActorGetCharacter(actor)->speed * ticks;
		struct vec2 moveVel = svec2_zero();
		if (cmd & CMD_LEFT) moveVel.x--;
		else if (cmd & CMD_RIGHT) moveVel.x++;
		if (cmd & CMD_UP) moveVel.y--;
		else if (cmd & CMD_DOWN) moveVel.y++;
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
	if (cmd & CMD_LEFT)			vel.x = -SLIDE_X;
	else if (cmd & CMD_RIGHT)	vel.x = SLIDE_X;
	if (cmd & CMD_UP)			vel.y = -SLIDE_Y;
	else if (cmd & CMD_DOWN)	vel.y = SLIDE_Y;
	e.u.ActorSlide.Vel = Vec2ToNet(vel);
	GameEventsEnqueue(&gGameEvents, e);
	
	actor->slideLock = SLIDE_LOCK;
}

static void ActorAddBloodSplatters(
	TActor *a, const int power, const float mass, const struct vec2 hitVector);

static void ActorUpdatePosition(TActor *actor, int ticks);
static void ActorDie(TActor *actor);
void UpdateAllActors(int ticks)
{
	CA_FOREACH(TActor, actor, gActors)
		if (!actor->isInUse)
		{
			continue;
		}
		ActorUpdatePosition(actor, ticks);
		UpdateActorState(actor, ticks);
		if (actor->dead > DEATH_MAX)
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
			const CollisionParams params =
			{
				THING_IMPASSABLE, COLLISIONTEAM_NONE,
				IsPVP(gCampaign.Entry.Mode)
			};
			const Thing *collidingItem = OverlapGetFirstItem(
				&actor->thing, actor->Pos, actor->thing.size, params);
			if (collidingItem && collidingItem->kind == KIND_CHARACTER)
			{
				TActor *collidingActor = CArrayGet(
					&gActors, collidingItem->id);
				if (CalcCollisionTeam(1, collidingActor) ==
					CalcCollisionTeam(1, actor))
				{
					struct vec2 v = svec2_subtract(
						actor->Pos, collidingActor->Pos);
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
	struct vec2 newPos = svec2_add(actor->Pos, actor->MoveVel);
	if (!svec2_is_zero(actor->thing.Vel))
	{
		newPos = svec2_add(
			newPos, svec2_scale(actor->thing.Vel, (float)ticks));

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
static void CheckManualPickups(TActor *a)
{
	// NPCs can't pickup
	if (a->PlayerUID < 0) return;
	const CollisionParams params =
	{
		0, CalcCollisionTeam(true, a), IsPVP(gCampaign.Entry.Mode)
	};
	OverlapThings(
		&a->thing, a->Pos,
		a->thing.size, params, CheckManualPickupFunc, a, NULL, NULL, NULL);
}
static bool CheckManualPickupFunc(
	Thing *ti, void *data, const struct vec2 colA, const struct vec2 colB,
	const struct vec2 normal)
{
	UNUSED(colA);
	UNUSED(colB);
	UNUSED(normal);
	TActor *a = data;
	if (ti->kind != KIND_PICKUP) return true;
	const Pickup *p = CArrayGet(&gPickups, ti->id);
	if (!PickupIsManual(p)) return true;
	// "Say" that the weapon must be picked up using a command
	const PlayerData *pData = PlayerDataGetByUID(a->PlayerUID);
	if (pData->IsLocal && IsPlayerHuman(pData))
	{
		char buttonName[256];
		strcpy(buttonName, "");
		InputGetButtonName(
			pData->inputDevice, pData->deviceIndex, CMD_BUTTON2, buttonName);
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
static void ActorAddAmmoPickup(const TActor *actor);
static void ActorAddGunPickup(const TActor *actor);
static void ActorDie(TActor *actor)
{
	// Add an ammo pickup of the actor's gun
	if (ConfigGetBool(&gConfig, "Game.Ammo"))
	{
		ActorAddAmmoPickup(actor);
	}

	// Random chance to add gun pickup
	if ((float)rand() / RAND_MAX < DROP_GUN_CHANCE)
	{
		ActorAddGunPickup(actor);
	}

	if (ConfigGetEnum(&gConfig, "Graphics.Gore") != GORE_NONE)
	{
		// Add blood pool
		AddRandomBloodPool(
			actor->Pos, ActorGetCharacter(actor)->Class->BloodColor);
	}

	GameEvent e = GameEventNew(GAME_EVENT_ACTOR_DIE);
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
	if (!gCampaign.IsClient)
	{
		for (int i = 0; i < MAX_WEAPONS; i++)
		{
			const Weapon *w = &actor->guns[i];
			// Check if the actor's gun has ammo at all
			if (w->Gun == NULL || w->Gun->AmmoId < 0)
			{
				continue;
			}

			// Don't spawn ammo if no players use it
			if (PlayersNumUseAmmo(w->Gun->AmmoId) == 0)
			{
				continue;
			}

			GameEvent e = GameEventNew(GAME_EVENT_ADD_PICKUP);
			e.u.AddPickup.UID = PickupsGetNextUID();
			const Ammo *a = AmmoGetById(&gAmmo, w->Gun->AmmoId);
			sprintf(e.u.AddPickup.PickupClass, "ammo_%s", a->Name);
			e.u.AddPickup.IsRandomSpawned = false;
			e.u.AddPickup.SpawnerUID = -1;
			e.u.AddPickup.ThingFlags = 0;
			// Add a little random offset so the pickups aren't all together
			const struct vec2 offset = svec2(
				(float)RAND_INT(-TILE_WIDTH, TILE_WIDTH) / 2,
				(float)RAND_INT(-TILE_HEIGHT, TILE_HEIGHT) / 2);
			e.u.AddPickup.Pos = Vec2ToNet(svec2_add(actor->Pos, offset));
			GameEventsEnqueue(&gGameEvents, e);
		CA_FOREACH_END()
	}

}
static void ActorAddGunPickup(const TActor *actor)
{
	if (IsUnarmedBot(actor))
	{
		return;
	}

	// Select a gun at random to drop
	if (!gCampaign.IsClient)
	{
		const Weapon *w;
		for (;;)
		{
			const int gunIndex = RAND_INT(0, MAX_WEAPONS - 1);
			w = &actor->guns[gunIndex];
			if (w->Gun != NULL)
			{
				break;
			}
		}
		PickupAddGun(w->Gun, actor->Pos);
	}
}
static bool IsUnarmedBot(const TActor *actor)
{
	// Note: if the actor is AI with no shooting time,
	// then it's an unarmed actor
	const Character *c = ActorGetCharacter(actor);
	return c->bot != NULL && c->bot->probabilityToShoot == 0;
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
		if (!a->isInUse) continue;
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
		LOG(LM_ACTOR, LL_DEBUG,
			"actor uid(%d) already exists; not adding", (int)aa.UID);
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
	LOG(LM_ACTOR, LL_DEBUG,
		"add actor uid(%d) playerUID(%d)", actor->uid, aa.PlayerUID);
	CArrayInit(&actor->ammo, sizeof(int));
	for (int i = 0; i < AmmoGetNumClasses(&gAmmo); i++)
	{
		// Initialise with twice the standard ammo amount
		// TODO: special game modes, keeping track of ammo, ammo persistence
		const int amount =
			AmmoGetById(&gAmmo, i)->Amount * AMMO_STARTING_MULTIPLE;
		CArrayPushBack(&actor->ammo, &amount);
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
	actor->thing.flags =
		THING_IMPASSABLE | THING_CAN_BE_SHOT | aa.ThingFlags;
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
			// If they don't have prisoner flag set, automatically rescue them
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
		&actor->barrelSmoke,
		StrParticleClass(&gParticleClasses, "smoke"),
		svec2_zero(), -0.05f, 0.05f, 3, 3, 0, 0, 10);
	GoreEmitterInit(&actor->blood1, "blood1");
	GoreEmitterInit(&actor->blood2, "blood2");
	GoreEmitterInit(&actor->blood3, "blood3");

	TryMoveActor(actor, NetToVec2(aa.Pos));

	// Spawn sound for player actors
	if (aa.PlayerUID >= 0)
	{
		SoundPlayAt(&gSoundDevice, StrSound("spawn"), actor->Pos);
	}
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
	if (p != NULL) p->ActorUID = -1;
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

struct vec2 ActorGetWeaponMuzzleOffset(const TActor *a)
{
	return ActorGetMuzzleOffset(a, ACTOR_GET_WEAPON(a));
}
struct vec2 ActorGetMuzzleOffset(const TActor *a, const Weapon *w)
{
	const Character *c = ActorGetCharacter(a);
	const CharSprites *cs = c->Class->Sprites;
	return WeaponClassGetMuzzleOffset(w->Gun, cs, a->direction, w->state);
}
int ActorWeaponGetAmmo(const TActor *a, const WeaponClass *wc)
{
	if (wc->AmmoId == -1)
	{
		return -1;
	}
	return *(int *)CArrayGet(&a->ammo, wc->AmmoId);
}
bool ActorCanFireWeapon(const TActor *a, const Weapon *w)
{
	if (w->Gun == NULL)
	{
		return false;
	}
	const bool hasAmmo = ActorWeaponGetAmmo(a, w->Gun) != 0;
	return
		!WeaponIsLocked(w) &&
		(!ConfigGetBool(&gConfig, "Game.Ammo") || hasAmmo);
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
	if (a == NULL || !a->isInUse) return;
	a->gunIndex = sg.GunIdx;
	const WeaponClass *gun = ACTOR_GET_WEAPON(a)->Gun;
	SoundPlayAt(&gSoundDevice, gun->SwitchSound, a->thing.Pos);
	ActorSetChatter(a, gun->name, CHATTER_SWITCH_GUN);
}

bool ActorIsImmune(const TActor *actor, const special_damage_e damage)
{
	// Fire immunity
	if (damage == SPECIAL_FLAME && (actor->flags & FLAGS_ASBESTOS))
	{
		return 1;
	}
	// Poison immunity
	if (damage == SPECIAL_POISON && (actor->flags & FLAGS_IMMUNITY))
	{
		return 1;
	}
	// Confuse immunity
	if (damage == SPECIAL_CONFUSE && (actor->flags & FLAGS_IMMUNITY))
	{
		return 1;
	}
	// Don't bother if health already 0 or less
	if (actor->health <= 0)
	{
		return 1;
	}
	return 0;
}


// Special damage durations
#define FLAMED_COUNT        10
#define POISONED_COUNT       8
#define MAX_POISONED_COUNT 140
#define PETRIFIED_COUNT     95
#define CONFUSED_COUNT     700

void ActorTakeSpecialDamage(TActor *actor, special_damage_e damage)
{
	switch (damage)
	{
	case SPECIAL_FLAME:
		actor->flamed = FLAMED_COUNT;
		break;
	case SPECIAL_POISON:
		if (actor->poisoned < MAX_POISONED_COUNT)
		{
			actor->poisoned += POISONED_COUNT;
		}
		break;
	case SPECIAL_PETRIFY:
		if (!actor->petrified)
		{
			actor->petrified = PETRIFIED_COUNT;
		}
		break;
	case SPECIAL_CONFUSE:
		actor->confused = CONFUSED_COUNT;
		break;
	default:
		// do nothing
		break;
	}
}

static void ActorTakeHit(TActor *actor, const special_damage_e damage);
void ActorHit(const NThingDamage d)
{
	TActor *a = ActorGetByUID(d.UID);
	if (!a->isInUse) return;
	ActorTakeHit(a, d.Special);
	if (d.Power > 0)
	{
		DamageActor(a, d.Power, d.SourceActorUID);

		// Add damage text
		// See if there is one already; if so remove it and add a new one,
		// combining the damage numbers
		int damage = (int)d.Power;
		struct vec2 pos = svec2_add(
			a->Pos, svec2(RAND_FLOAT(-3, 3), RAND_FLOAT(-3, 3)));
		CA_FOREACH(const Particle, p, gParticles)
			if (p->isInUse && p->ActorUID == a->uid)
			{
				damage += a->accumulatedDamage;
				pos = p->Pos;
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
	}
}

static void ActorTakeHit(TActor *actor, const special_damage_e damage)
{
	// Wake up if this is an AI
	if (!gCampaign.IsClient && actor->aiContext)
	{
		actor->flags &= ~FLAGS_SLEEPING;
		ActorSetAIState(actor, AI_STATE_NONE);
	}
	// Check immune again
	// This can happen if multiple damage events overkill this actor,
	// need to ignore the overkill scores
	if (ActorIsImmune(actor, damage))
	{
		return;
	}
	ActorTakeSpecialDamage(actor, damage);
}

bool ActorIsInvulnerable(
	const TActor *actor, const int flags, const int playerUID,
	const GameMode mode)
{
	if (actor->flags & FLAGS_INVULNERABLE)
	{
		return 1;
	}

	if (!(flags & FLAGS_HURTALWAYS) && !(actor->flags & FLAGS_VICTIM))
	{
		// Same player hits
		if (playerUID >= 0 && playerUID == actor->PlayerUID)
		{
			return 1;
		}
		const bool isGood = playerUID >= 0 || (flags & FLAGS_GOOD_GUY);
		const bool isTargetGood =
			actor->PlayerUID >= 0 || (actor->flags & FLAGS_GOOD_GUY);
		// Friendly fire (NPCs)
		if (!IsPVP(mode) &&
			!ConfigGetBool(&gConfig, "Game.FriendlyFire") &&
			isGood && isTargetGood)
		{
			return 1;
		}
		// Enemies don't hurt each other
		if (!isGood && !isTargetGood)
		{
			return 1;
		}
	}

	return 0;
}

static void ActorAddBloodSplatters(
	TActor *a, const int power, const float mass, const struct vec2 hitVector)
{
	const GoreAmount ga = ConfigGetEnum(&gConfig, "Graphics.Gore");
	if (ga == GORE_NONE) return;

	// Emit blood based on power and gore setting
	int bloodPower = power * 2;
	// Randomly cycle through the blood types
	int bloodSize = 1;
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
			svec2_scale(hitVector, speedBase * RAND_FLOAT(0.5f, 1));
		AddParticle ap;
		memset(&ap, 0, sizeof ap);
		ap.Pos = a->Pos;
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

bool ActorIsLocalPlayer(const int uid)
{
	const TActor *a = ActorGetByUID(uid);
	// Don't accept updates if actor doesn't exist
	// This can happen in the very first frame, where we haven't yet
	// processed an actor add message
	// Otherwise this shouldn't happen
	if (a == NULL) return true;

	return PlayerIsLocal(a->PlayerUID);
}
