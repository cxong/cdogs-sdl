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

    Copyright (c) 2013-2017, Cong Xu
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
#include "draw/drawtools.h"
#include "events.h"
#include "game_events.h"
#include "log.h"
#include "pic_manager.h"
#include "sounds.h"
#include "defs.h"
#include "objs.h"
#include "pickup.h"
#include "gamedata.h"
#include "triggers.h"
#include "hiscores.h"
#include "mission.h"
#include "game.h"
#include "utils.h"

#define FOOTSTEP_DISTANCE_PLUS 380
#define REPEL_STRENGTH 14
#define SLIDE_LOCK 50
#define SLIDE_X (TILE_WIDTH / 3)
#define SLIDE_Y (TILE_HEIGHT / 3)
#define VEL_DECAY_X (TILE_WIDTH * 2)
#define VEL_DECAY_Y (TILE_WIDTH * 2)	// Note: deliberately tile width
#define SOUND_LOCK_WEAPON_CLICK 20
#define DROP_GUN_CHANCE 0.2
#define DRAW_RADIAN_SPEED (PI/16)
#define BLEED_PERCENTAGE 25	// start bleeding if below this % of health


CArray gPlayerIds;


CArray gActors;
static unsigned int sActorUIDs = 0;


void ActorSetState(TActor *actor, const ActorAnimation state)
{
	actor->anim = AnimationGetActorAnimation(state);
}

static void CheckPickups(TActor *actor);
void UpdateActorState(TActor * actor, int ticks)
{
	Weapon *gun = ActorGetGun(actor);
	WeaponUpdate(gun, ticks);
	ActorFireUpdate(gun, actor, ticks);

	// If we're ready to pick up, always check the pickups
	if (actor->PickupAll && !gCampaign.IsClient)
	{
		CheckPickups(actor);
	}

	if (actor->health > 0)
	{
		if (actor->lastHealth != actor->health)
		{
			actor->lastHealth > actor->health ? -- actor->lastHealth:
				++ actor->lastHealth;
		}
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

	TileItemUpdate(&actor->tileItem, ticks);

	actor->stateCounter = MAX(0, actor->stateCounter - ticks);
	if (actor->stateCounter > 0)
	{
		return;
	}

	if (actor->health <= 0) {
		actor->dead++;
		actor->MoveVel = Vec2iZero();
		actor->stateCounter = 4;
		actor->tileItem.flags = 0;
		return;
	}

	// Draw rotation interpolation
	const float targetRadians = (float)dir2radians[actor->direction];
	if (actor->DrawRadians - targetRadians > PI)
	{
		actor->DrawRadians -= 2 * (float)PI;
	}
	if (actor->DrawRadians - targetRadians < -PI)
	{
		actor->DrawRadians += 2 * (float)PI;
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
		(AnimationGetFrame(&actor->anim) == 2 ||
		AnimationGetFrame(&actor->anim) == 6) &&
		actor->anim.newFrame)
	{
		SoundPlayAtPlusDistance(
			&gSoundDevice, StrSound("footsteps"),
			Vec2iNew(actor->tileItem.x, actor->tileItem.y),
			FOOTSTEP_DISTANCE_PLUS);
	}

	// Animation
	AnimationUpdate(&actor->anim, ticks);

	// Chatting
	actor->ChatterCounter = MAX(0, actor->ChatterCounter - ticks);
	if (actor->ChatterCounter == 0)
	{
		// Stop chatting
		strcpy(actor->Chatter, "");
	}
}

static Vec2i GetConstrainedFullPos(
	const Map *map, const Vec2i fromFull, const Vec2i toFull,
	const Vec2i size);
static void OnMove(TActor *a);
bool TryMoveActor(TActor *actor, Vec2i pos)
{
	CASSERT(!Vec2iEqual(actor->Pos, pos), "trying to move to same position");

	actor->hasCollided = true;
	actor->CanPickupSpecial = false;

	const Vec2i oldPos = actor->Pos;
	pos = GetConstrainedFullPos(&gMap, actor->Pos, pos, actor->tileItem.size);
	if (Vec2iEqual(oldPos, pos))
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
			TILEITEM_IMPASSABLE, CalcCollisionTeam(true, actor),
			IsPVP(gCampaign.Entry.Mode)
		};
		TTileItem *target = OverlapGetFirstItem(
			&actor->tileItem, pos, actor->tileItem.size, params);
		if (target)
		{
			Weapon *gun = ActorGetGun(actor);
			const TObject *object = target->kind == KIND_OBJECT ?
				CArrayGet(&gObjs, target->id) : NULL;
			if (!gun->Gun->CanShoot && actor->health > 0 &&
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

			const Vec2i yPos = Vec2iNew(actor->Pos.x, pos.y);
			if (OverlapGetFirstItem(
				&actor->tileItem, yPos, actor->tileItem.size, params))
			{
				pos.y = actor->Pos.y;
			}
			const Vec2i xPos = Vec2iNew(pos.x, actor->Pos.y);
			if (OverlapGetFirstItem(
				&actor->tileItem, xPos, actor->tileItem.size, params))
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
				IsCollisionWithWall(Vec2iFull2Real(pos), actor->tileItem.size))
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
// Note: must use full coordinates to do collisions, despite collisions using
// real coordinates, because fractional movement will be blocked otherwise
// since real coordinates are the same.
static Vec2i GetConstrainedFullPos(
	const Map *map, const Vec2i fromFull, const Vec2i toFull,
	const Vec2i size)
{
	// Check collision with wall
	if (!IsCollisionWithWall(Vec2iFull2Real(toFull), size))
	{
		// Not in collision; just return where we wanted to go
		return toFull;
	}
	
	CASSERT(size.x >= size.y, "tall collision not supported");
	const Vec2i dv = Vec2iMinus(toFull, fromFull);

	// If moving diagonally, use rectangular bounds and
	// try to move in only x or y directions
	if (dv.x != 0 && dv.y != 0)
	{
		// X-only movement
		const Vec2i xVec = Vec2iNew(toFull.x, fromFull.y);
		if (!IsCollisionWithWall(Vec2iFull2Real(xVec), size))
		{
			return xVec;
		}
		// Y-only movement
		const Vec2i yVec = Vec2iNew(fromFull.x, toFull.y);
		if (!IsCollisionWithWall(Vec2iFull2Real(yVec), size))
		{
			return yVec;
		}
		// If we're still stuck, we're possibly stuck on a corner which is not
		// in collision with a diamond but is colliding with the box.
		// If so try x- or y-only movement, but with the benefit of diamond
		// slipping.
		const Vec2i xPos = GetConstrainedFullPos(map, fromFull, xVec, size);
		if (!Vec2iEqual(xPos, fromFull))
		{
			return xPos;
		}
		const Vec2i yPos = GetConstrainedFullPos(map, fromFull, yVec, size);
		if (!Vec2iEqual(yPos, fromFull))
		{
			return yPos;
		}
	}

	// Now check diagonal movement, if we were moving in an x- or y-
	// only direction
	// Note: we're moving at extra speed because dx/dy are only magnitude 1;
	// if we divide then we get 0 which ruins the logic
	if (dv.x == 0)
	{
		// Moving up or down; try moving to the left or right diagonally
		// Scale X movement because our diamond is wider than tall, so we
		// may need to scale the diamond wider.
		const int xScale =
			size.x > size.y ? (int)ceil((double)size.x / size.y) : 1;
		const Vec2i diag1Vec =
			Vec2iAdd(fromFull, Vec2iNew(-dv.y * xScale, dv.y));
		if (!IsCollisionDiamond(map, Vec2iFull2Real(diag1Vec), size))
		{
			return diag1Vec;
		}
		const Vec2i diag2Vec =
			Vec2iAdd(fromFull, Vec2iNew(dv.y * xScale, dv.y));
		if (!IsCollisionDiamond(map, Vec2iFull2Real(diag2Vec), size))
		{
			return diag2Vec;
		}
	}
	else if (dv.y == 0)
	{
		// Moving left or right; try moving up or down diagonally
		const Vec2i diag1Vec =
			Vec2iAdd(fromFull, Vec2iNew(dv.x, -dv.x));
		if (!IsCollisionDiamond(map, Vec2iFull2Real(diag1Vec), size))
		{
			return diag1Vec;
		}
		const Vec2i diag2Vec =
			Vec2iAdd(fromFull, Vec2iNew(dv.x, dv.x));
		if (!IsCollisionDiamond(map, Vec2iFull2Real(diag2Vec), size))
		{
			return diag2Vec;
		}
	}

	// All alternative movements are in collision; don't move
	return fromFull;
}

void ActorMove(const NActorMove am)
{
	TActor *a = ActorGetByUID(am.UID);
	if (a == NULL || !a->isInUse) return;
	a->Pos = Net2Vec2i(am.Pos);
	a->MoveVel = Net2Vec2i(am.MoveVel);
	OnMove(a);
}
static void CheckTrigger(const Vec2i tilePos);
static void CheckRescue(const TActor *a);
static void OnMove(TActor *a)
{
	const Vec2i realPos = Vec2iFull2Real(a->Pos);
	MapTryMoveTileItem(&gMap, &a->tileItem, realPos);
	if (MapIsTileInExit(&gMap, &a->tileItem))
	{
		a->action = ACTORACTION_EXITING;
	}
	else
	{
		a->action = ACTORACTION_MOVING;
	}

	if (!gCampaign.IsClient)
	{
		CheckTrigger(Vec2iToTile(realPos));

		CheckPickups(a);

		CheckRescue(a);
	}
}
static void CheckTrigger(const Vec2i tilePos)
{
	const Tile *t = MapGetTile(&gMap, tilePos);
	CA_FOREACH(Trigger *, tp, t->triggers)
		if (TriggerCanActivate(*tp, gMission.KeyFlags))
		{
			GameEvent e = GameEventNew(GAME_EVENT_TRIGGER);
			e.u.TriggerEvent.ID = (*tp)->id;
			e.u.TriggerEvent.Tile = Vec2i2Net(tilePos);
			GameEventsEnqueue(&gGameEvents, e);
		}
	CA_FOREACH_END()
}
// Check if the player can pickup any item
static bool CheckPickupFunc(
	TTileItem *ti, void *data, const Vec2i colA, const Vec2i colB,
	const Vec2i normal);
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
	OverlapTileItems(
		&actor->tileItem, actor->Pos, actor->tileItem.size,
		params, CheckPickupFunc, actor, NULL, NULL, NULL);
}
static bool CheckPickupFunc(
	TTileItem *ti, void *data, const Vec2i colA, const Vec2i colB,
	const Vec2i normal)
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
		TILEITEM_IMPASSABLE, CalcCollisionTeam(true, a),
		IsPVP(gCampaign.Entry.Mode)
	};
	const TTileItem *target = OverlapGetFirstItem(
		&a->tileItem, a->Pos,
		Vec2iAdd(a->tileItem.size, Vec2iNew(RESCUE_CHECK_PAD, RESCUE_CHECK_PAD)),
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
				&gMission, other->tileItem.flags, OBJECTIVE_RESCUE, 1);
		}
	}
}

void ActorHeal(TActor *actor, int health)
{
	actor->lastHealth = actor->health;
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
		const Vec2i pos = Vec2iNew(actor->tileItem.x, actor->tileItem.y);
		SoundPlayAt(
			&gSoundDevice,
			StrSound(ActorGetCharacter(actor)->Class->Sounds.Aargh), pos);
		if (actor->PlayerUID >= 0)
		{
			SoundPlayAt(
				&gSoundDevice,
				StrSound("hahaha"),
				pos);
		}
		if (actor->tileItem.flags & TILEITEM_OBJECTIVE)
		{
			UpdateMissionObjective(
				&gMission, actor->tileItem.flags, OBJECTIVE_KILL, 1);
			// If we've killed someone we have rescued, deduct from the
			// rescue objective
			if (!(actor->flags & FLAGS_PRISONER))
			{
				UpdateMissionObjective(
					&gMission, actor->tileItem.flags, OBJECTIVE_RESCUE, -1);
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
	CA_FOREACH(const Weapon, w, actor->guns)
		if (w->Gun->AmmoId == ammoId)
		{
			return true;
		}
	CA_FOREACH_END()
	return false;
}

static bool ActorHasGun(const TActor *a, const GunDescription *gun);
void ActorReplaceGun(const NActorReplaceGun rg)
{
	TActor *a = ActorGetByUID(rg.UID);
	if (a == NULL || !a->isInUse) return;
	const GunDescription *gun = StrGunDescription(rg.Gun);
	CASSERT(gun != NULL, "cannot find gun");
	// If player already has gun, don't do anything
	if (ActorHasGun(a, gun))
	{
		return;
	}
	LOG(LM_ACTOR, LL_DEBUG, "actor uid(%d) replacing gun(%s) idx(%d) size(%d)",
		(int)rg.UID, rg.Gun, rg.GunIdx, (int)a->guns.size);
	Weapon w = WeaponCreate(gun);
	if (a->guns.size <= rg.GunIdx)
	{
		CASSERT(rg.GunIdx < a->guns.size + 1, "gun idx would leave gap");
		CArrayPushBack(&a->guns, &w);
		// Switch immediately to picked up gun
		a->gunIndex = (int)a->guns.size - 1;
	}
	else
	{
		memcpy(CArrayGet(&a->guns, rg.GunIdx), &w, a->guns.elemSize);
	}

	SoundPlayAt(&gSoundDevice, gun->SwitchSound, Vec2iFull2Real(a->Pos));
}
static bool ActorHasGun(const TActor *a, const GunDescription *gun)
{
	CA_FOREACH(const Weapon, w, a->guns)
		if (w->Gun == gun)
		{
			return true;
		}
	CA_FOREACH_END()
	return false;
}

// Set AI state and possibly say something based on the state
void ActorSetAIState(TActor *actor, const AIState s)
{
	if (AIContextSetState(actor->aiContext, s) &&
		AIContextShowChatter(
		actor->aiContext, ConfigGetEnum(&gConfig, "Interface.AIChatter")))
	{
		// Say something for a while
		strcpy(actor->Chatter, AIStateGetChatterText(actor->aiContext->State));
		actor->ChatterCounter = 2;
	}
}

void Shoot(TActor *actor)
{
	Weapon *gun = ActorGetGun(actor);
	if (!ActorCanFire(actor))
	{
		if (!WeaponIsLocked(gun) && ConfigGetBool(&gConfig, "Game.Ammo"))
		{
			CASSERT(ActorGunGetAmmo(actor, gun) == 0, "should be out of ammo");
			// Play a clicking sound if this gun is out of ammo
			if (gun->clickLock <= 0)
			{
				SoundPlayAt(
					&gSoundDevice,
					StrSound("click"), Vec2iFull2Real(actor->Pos));
				gun->clickLock = SOUND_LOCK_WEAPON_CLICK;
			}
		}
		return;
	}
	ActorFire(gun, actor);
	if (actor->PlayerUID >= 0)
	{
		if (ConfigGetBool(&gConfig, "Game.Ammo") && gun->Gun->AmmoId >= 0)
		{
			GameEvent e = GameEventNew(GAME_EVENT_ACTOR_USE_AMMO);
			e.u.UseAmmo.UID = actor->uid;
			e.u.UseAmmo.PlayerUID = actor->PlayerUID;
			e.u.UseAmmo.AmmoId = gun->Gun->AmmoId;
			e.u.UseAmmo.Amount = 1;
			GameEventsEnqueue(&gGameEvents, e);
		}
		else if (gun->Gun->Cost != 0)
		{
			// Classic C-Dogs score consumption
			GameEvent e = GameEventNew(GAME_EVENT_SCORE);
			e.u.Score.PlayerUID = actor->PlayerUID;
			e.u.Score.Score = -gun->Gun->Cost;
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

int ActorTryShoot(TActor *actor, int cmd)
{
	int willShoot = !actor->petrified && (cmd & CMD_BUTTON1);
	if (willShoot)
	{
		Shoot(actor);
	}
	else if (ActorGetGun(actor)->state != GUNSTATE_READY)
	{
		GameEvent e = GameEventNew(GAME_EVENT_GUN_STATE);
		e.u.GunState.ActorUID = actor->uid;
		e.u.GunState.State = GUNSTATE_READY;
		GameEventsEnqueue(&gGameEvents, e);
	}
	return willShoot;
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
		int hasChangedDirection, hasShot, hasMoved;
		hasChangedDirection = ActorTryChangeDirection(actor, cmd, actor->lastCmd);
		hasShot = ActorTryShoot(actor, cmd);
		hasMoved = ActorTryMove(actor, cmd, hasShot, ticks);
		if (!hasChangedDirection && !hasShot && !hasMoved)
		{
			// Idle if player hasn't done anything
			if (actor->anim.Type != ACTORANIMATION_IDLE)
			{
				GameEvent e = GameEventNew(GAME_EVENT_ACTOR_STATE);
				e.u.ActorState.UID = actor->uid;
				e.u.ActorState.State = (int32_t)ACTORANIMATION_IDLE;
				GameEventsEnqueue(&gGameEvents, e);
			}
		}
	}

	actor->lastCmd = cmd;
	if (cmd & CMD_BUTTON2)
	{
		if (CMD_HAS_DIRECTION(cmd))
		{
			actor->specialCmdDir = true;
		}
		else
		{
			// Special: pick up things that can only be picked up on demand
			if (!actor->PickupAll)
			{
				GameEvent e = GameEventNew(GAME_EVENT_ACTOR_PICKUP_ALL);
				e.u.ActorPickupAll.UID = actor->uid;
				e.u.ActorPickupAll.PickupAll = true;
				GameEventsEnqueue(&gGameEvents, e);
			}
			actor->PickupAll = true;
		}
	}
	else
	{
		actor->specialCmdDir = false;
		if (actor->PickupAll)
		{
			GameEvent e = GameEventNew(GAME_EVENT_ACTOR_PICKUP_ALL);
			e.u.ActorPickupAll.UID = actor->uid;
			e.u.ActorPickupAll.PickupAll = false;
			GameEventsEnqueue(&gGameEvents, e);
		}
		actor->PickupAll = false;
	}
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
	actor->MoveVel = Vec2iZero();
	if (willMove)
	{
		const int moveAmount = ActorGetCharacter(actor)->speed * ticks;
		if (cmd & CMD_LEFT)
		{
			actor->MoveVel.x -= moveAmount;
		}
		else if (cmd & CMD_RIGHT)
		{
			actor->MoveVel.x += moveAmount;
		}
		if (cmd & CMD_UP)
		{
			actor->MoveVel.y -= moveAmount;
		}
		else if (cmd & CMD_DOWN)
		{
			actor->MoveVel.y += moveAmount;
		}

		if (actor->anim.Type != ACTORANIMATION_WALKING)
		{
			GameEvent e = GameEventNew(GAME_EVENT_ACTOR_STATE);
			e.u.ActorState.UID = actor->uid;
			e.u.ActorState.State = (int32_t)ACTORANIMATION_WALKING;
			GameEventsEnqueue(&gGameEvents, e);
		}
	}
	else
	{
		if (actor->anim.Type != ACTORANIMATION_IDLE)
		{
			GameEvent e = GameEventNew(GAME_EVENT_ACTOR_STATE);
			e.u.ActorState.UID = actor->uid;
			e.u.ActorState.State = (int32_t)ACTORANIMATION_IDLE;
			GameEventsEnqueue(&gGameEvents, e);
		}
	}

	// If we have changed our move commands, send the move event
	if (cmd != actor->lastCmd || actor->hasCollided)
	{
		GameEvent e = GameEventNew(GAME_EVENT_ACTOR_MOVE);
		e.u.ActorMove.UID = actor->uid;
		e.u.ActorMove.Pos = Vec2i2Net(actor->Pos);
		e.u.ActorMove.MoveVel = Vec2i2Net(actor->MoveVel);
		GameEventsEnqueue(&gGameEvents, e);
	}

	return willMove;
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
	Vec2i vel = Vec2iZero();
	if (cmd & CMD_LEFT)			vel.x = -SLIDE_X * 256;
	else if (cmd & CMD_RIGHT)	vel.x = SLIDE_X * 256;
	if (cmd & CMD_UP)			vel.y = -SLIDE_Y * 256;
	else if (cmd & CMD_DOWN)	vel.y = SLIDE_Y * 256;
	e.u.ActorSlide.Vel = Vec2i2Net(vel);
	GameEventsEnqueue(&gGameEvents, e);
	
	actor->slideLock = SLIDE_LOCK;
}

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
				TILEITEM_IMPASSABLE, COLLISIONTEAM_NONE,
				IsPVP(gCampaign.Entry.Mode)
			};
			const TTileItem *collidingItem = OverlapGetFirstItem(
				&actor->tileItem, actor->Pos, actor->tileItem.size, params);
			if (collidingItem && collidingItem->kind == KIND_CHARACTER)
			{
				TActor *collidingActor = CArrayGet(
					&gActors, collidingItem->id);
				if (CalcCollisionTeam(1, collidingActor) ==
					CalcCollisionTeam(1, actor))
				{
					Vec2i v = Vec2iMinus(actor->Pos, collidingActor->Pos);
					if (Vec2iIsZero(v))
					{
						v = Vec2iNew(1, 0);
					}
					v = Vec2iScale(Vec2iNorm(v), REPEL_STRENGTH);
					GameEvent e = GameEventNew(GAME_EVENT_ACTOR_IMPULSE);
					e.u.ActorImpulse.UID = actor->uid;
					e.u.ActorImpulse.Vel = Vec2i2Net(v);
					e.u.ActorImpulse.Pos = Vec2i2Net(actor->Pos);
					GameEventsEnqueue(&gGameEvents, e);
					e.u.ActorImpulse.UID = collidingActor->uid;
					e.u.ActorImpulse.Vel = Vec2i2Net(Vec2iScale(v, -1));
					e.u.ActorImpulse.Pos = Vec2i2Net(collidingActor->Pos);
					GameEventsEnqueue(&gGameEvents, e);
				}
			}
		}
		// If low on health, bleed
		const int maxHealth = ActorGetCharacter(actor)->maxHealth;
		const int healthPct = actor->health * 100 / maxHealth;
		if (healthPct < BLEED_PERCENTAGE)
		{
			actor->bleedCounter--;
			if (actor->bleedCounter <= 0)
			{
				ActorAddBloodSplatters(actor, 1, Vec2iZero());
				actor->bleedCounter += healthPct;
			}
		}
	CA_FOREACH_END()
}
static void CheckManualPickups(TActor *a);
static void ActorUpdatePosition(TActor *actor, int ticks)
{
	Vec2i newPos = Vec2iAdd(actor->Pos, actor->MoveVel);
	if (!Vec2iIsZero(actor->tileItem.VelFull))
	{
		newPos = Vec2iAdd(newPos, Vec2iScale(actor->tileItem.VelFull, ticks));

		for (int i = 0; i < ticks; i++)
		{
			if (actor->tileItem.VelFull.x > 0)
			{
				actor->tileItem.VelFull.x =
					MAX(0, actor->tileItem.VelFull.x - VEL_DECAY_X);
			}
			else
			{
				actor->tileItem.VelFull.x =
					MIN(0, actor->tileItem.VelFull.x + VEL_DECAY_X);
			}
			if (actor->tileItem.VelFull.y > 0)
			{
				actor->tileItem.VelFull.y =
					MAX(0, actor->tileItem.VelFull.y - VEL_DECAY_Y);
			}
			else
			{
				actor->tileItem.VelFull.y =
					MIN(0, actor->tileItem.VelFull.y + VEL_DECAY_Y);
			}
		}
	}

	if (!Vec2iEqual(actor->Pos, newPos))
	{
		TryMoveActor(actor, newPos);
	}
	// Check if we're standing over any manual pickups
	CheckManualPickups(actor);
}
// Check if the actor is over any manual pickups
static bool CheckManualPickupFunc(
	TTileItem *ti, void *data, const Vec2i colA, const Vec2i colB,
	const Vec2i normal);
static void CheckManualPickups(TActor *a)
{
	// NPCs can't pickup
	if (a->PlayerUID < 0) return;
	const CollisionParams params =
	{
		0, CalcCollisionTeam(true, a), IsPVP(gCampaign.Entry.Mode)
	};
	OverlapTileItems(
		&a->tileItem, a->Pos,
		a->tileItem.size, params, CheckManualPickupFunc, a, NULL, NULL, NULL);
}
static bool CheckManualPickupFunc(
	TTileItem *ti, void *data, const Vec2i colA, const Vec2i colB,
	const Vec2i normal)
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
		char buf[256];
		strcpy(buf, "");
		InputGetButtonName(
			pData->inputDevice, pData->deviceIndex, CMD_BUTTON2, buf);
		sprintf(a->Chatter, "%s to pick up\n%s",
			buf, IdGunDescription(p->class->u.GunId)->name);
		a->ChatterCounter = 2;
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

	// Add a blood pool
	GameEvent e = GameEventNew(GAME_EVENT_MAP_OBJECT_ADD);
	e.u.MapObjectAdd.UID = ObjsGetNextUID();
	const MapObject *mo = RandomBloodMapObject(&gMapObjects);
	strcpy(e.u.MapObjectAdd.MapObjectClass, mo->Name);
	e.u.MapObjectAdd.Pos = Vec2i2Net(Vec2iFull2Real(actor->Pos));
	e.u.MapObjectAdd.TileItemFlags = MapObjectGetFlags(mo);
	e.u.MapObjectAdd.Health = mo->Health;
	GameEventsEnqueue(&gGameEvents, e);

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
	if (!gCampaign.IsClient)
	{
		CA_FOREACH(const Weapon, w, actor->guns)
			// Check if the actor's gun has ammo at all
			if (w->Gun->AmmoId < 0)
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
			e.u.AddPickup.TileItemFlags = 0;
			// Add a little random offset so the pickups aren't all together
			const Vec2i offset = Vec2iNew(
				RAND_INT(-TILE_WIDTH, TILE_WIDTH) / 2,
				RAND_INT(-TILE_HEIGHT, TILE_HEIGHT) / 2);
			e.u.AddPickup.Pos = Vec2i2Net(Vec2iAdd(Vec2iFull2Real(actor->Pos), offset));
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
		const int gunIndex = RAND_INT(0, (int)actor->guns.size - 1);
		const Weapon *w = CArrayGet(&actor->guns, gunIndex);
		if (!w->Gun->CanDrop)
		{
			return;
		}
		GameEvent e = GameEventNew(GAME_EVENT_ADD_PICKUP);
		e.u.AddPickup.UID = PickupsGetNextUID();
		sprintf(e.u.AddPickup.PickupClass, "gun_%s", w->Gun->name);
		e.u.AddPickup.IsRandomSpawned = false;
		e.u.AddPickup.SpawnerUID = -1;
		e.u.AddPickup.TileItemFlags = 0;
		e.u.AddPickup.Pos = Vec2i2Net(Vec2iFull2Real(actor->Pos));
		GameEventsEnqueue(&gGameEvents, e);
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
	CArrayInit(&actor->guns, sizeof(Weapon));
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
		for (int i = 0; i < p->weaponCount; i++)
		{
			Weapon gun = WeaponCreate(p->weapons[i]);
			CArrayPushBack(&actor->guns, &gun);
		}
		p->ActorUID = aa.UID;
	}
	else
	{
		// Add sole weapon from character type
		Weapon gun = WeaponCreate(c->Gun);
		CArrayPushBack(&actor->guns, &gun);
	}
	actor->gunIndex = 0;
	actor->health = aa.Health;
	actor->lastHealth = actor->health;
	actor->action = ACTORACTION_MOVING;
	actor->tileItem.x = actor->tileItem.y = -1;
	actor->tileItem.kind = KIND_CHARACTER;
	actor->tileItem.getPicFunc = NULL;
	actor->tileItem.drawFunc = NULL;
	actor->tileItem.size = Vec2iNew(ACTOR_W, ACTOR_H);
	actor->tileItem.flags =
		TILEITEM_IMPASSABLE | TILEITEM_CAN_BE_SHOT | aa.TileItemFlags;
	actor->tileItem.id = id;
	actor->isInUse = true;

	actor->flags = FLAGS_SLEEPING | c->flags;
	// Flag corrections
	if (actor->flags & FLAGS_AWAKEALWAYS)
	{
		actor->flags &= ~FLAGS_SLEEPING;
	}
	// Rescue objectives always have follower flag on
	if (actor->tileItem.flags & TILEITEM_OBJECTIVE)
	{
		const Objective *o = CArrayGet(
			&gMission.missionData->Objectives,
			ObjectiveFromTileItem(actor->tileItem.flags));
		if (o->Type == OBJECTIVE_RESCUE)
		{
			// If they don't have prisoner flag set, automatically rescue them
			if (!(actor->flags & FLAGS_PRISONER) && !gCampaign.IsClient)
			{
				GameEvent e = GameEventNew(GAME_EVENT_RESCUE_CHARACTER);
				e.u.Rescue.UID = aa.UID;
				GameEventsEnqueue(&gGameEvents, e);
				UpdateMissionObjective(
					&gMission, actor->tileItem.flags, OBJECTIVE_RESCUE, 1);
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

	GoreEmitterInit(&actor->blood1, "blood1");
	GoreEmitterInit(&actor->blood2, "blood2");
	GoreEmitterInit(&actor->blood3, "blood3");

	TryMoveActor(actor, Net2Vec2i(aa.FullPos));

	// Spawn sound for player actors
	if (aa.PlayerUID >= 0)
	{
		SoundPlayAt(
			&gSoundDevice, StrSound("spawn"), Vec2iFull2Real(actor->Pos));
	}
	return actor;
}
static void GoreEmitterInit(Emitter *em, const char *particleClassName)
{
	EmitterInit(
		em, StrParticleClass(&gParticleClasses, particleClassName),
		Vec2iZero(), 0, 64, 6, 12, -0.1, 0.1);
}

void ActorDestroy(TActor *a)
{
	CASSERT(a->isInUse, "Destroying in-use actor");
	CArrayTerminate(&a->guns);
	CArrayTerminate(&a->ammo);
	MapRemoveTileItem(&gMap, &a->tileItem);
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

Weapon *ActorGetGun(const TActor *a)
{
	return CArrayGet(&a->guns, a->gunIndex);
}
Vec2i ActorGetGunMuzzleOffset(const TActor *a)
{
	const GunDescription *gun = ActorGetGun(a)->Gun;
	const CharSprites *cs = ActorGetCharacter(a)->Class->Sprites;
	return GunGetMuzzleOffset(gun, cs, a->direction);
}
int ActorGunGetAmmo(const TActor *a, const Weapon *w)
{
	if (w->Gun->AmmoId == -1)
	{
		return -1;
	}
	return *(int *)CArrayGet(&a->ammo, w->Gun->AmmoId);
}
bool ActorCanFire(const TActor *a)
{
	const Weapon *w = ActorGetGun(a);
	const bool hasAmmo = ActorGunGetAmmo(a, w) != 0;
	return
		!WeaponIsLocked(w) &&
		(!ConfigGetBool(&gConfig, "Game.Ammo") || hasAmmo);
}
bool ActorCanSwitchGun(const TActor *a)
{
	return a->guns.size > 1;
}
void ActorSwitchGun(const NActorSwitchGun sg)
{
	TActor *a = ActorGetByUID(sg.UID);
	if (a == NULL || !a->isInUse) return;
	CASSERT(sg.GunIdx < a->guns.size, "can't switch to unavailable gun");
	a->gunIndex = sg.GunIdx;
	SoundPlayAt(
		&gSoundDevice,
		ActorGetGun(a)->Gun->SwitchSound,
		Vec2iNew(a->tileItem.x, a->tileItem.y));
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

void ActorTakeHit(TActor *actor, const special_damage_e damage)
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

void ActorAddBloodSplatters(TActor *a, const int power, const Vec2i hitVector)
{
	const GoreAmount ga = ConfigGetEnum(&gConfig, "Graphics.Gore");
	if (ga == GORE_NONE) return;

	// Emit blood based on power and gore setting
	int bloodPower = power * 2;
	// Randomly cycle through the blood types
	int bloodSize = 1;
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
		const Vec2i vel = Vec2iScaleDiv(
			Vec2iScale(hitVector, (rand() % 8 + 8) * power),
			15 * SHOT_IMPULSE_DIVISOR);
		EmitterStart(em, a->Pos, 10, vel);
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
