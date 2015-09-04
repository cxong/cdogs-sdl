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

    Copyright (c) 2013-2015, Cong Xu
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
#include "objs.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "bullet_class.h"
#include "collision.h"
#include "config.h"
#include "damage.h"
#include "game_events.h"
#include "map.h"
#include "net_util.h"
#include "screen_shake.h"
#include "blit.h"
#include "pic_manager.h"
#include "pickup.h"
#include "defs.h"
#include "actors.h"
#include "gamedata.h"
#include "mission.h"
#include "game.h"
#include "utils.h"

CArray gObjs;
CArray gMobObjs;
static unsigned int sObjUIDs = 0;
static unsigned int sMobObjUIDs = 0;


// Draw functions

static const Pic *GetObjectPic(const int id, Vec2i *offset)
{
	const TObject *obj = CArrayGet(&gObjs, id);
	return MapObjectGetPic(obj->Class, offset, obj->Health == 0);
}


static void DestroyObject(
	TObject *o, const int flags, const int playerUID, const int uid);
void DamageObject(const NMapObjectDamage mod)
{
	TObject *o = ObjGetByUID(mod.UID);
	// Don't bother if object already destroyed
	if (o->Health <= 0)
	{
		return;
	}

	o->Health -= mod.Power;

	// Destroying objects and all the wonderful things that happen
	if (o->Health <= 0)
	{
		DestroyObject(o, mod.Flags, mod.PlayerUID, mod.UID);
	}
}
static void DestroyObject(
	TObject *o, const int flags, const int playerUID, const int uid)
{
	o->Health = 0;

	const Vec2i realPos = Vec2iNew(o->tileItem.x, o->tileItem.y);

	if (!gCampaign.IsClient)
	{
		// Update objective
		UpdateMissionObjective(&gMission, o->tileItem.flags, OBJECTIVE_DESTROY);
		// Extra score if objective
		if ((o->tileItem.flags & TILEITEM_OBJECTIVE) && playerUID >= 0)
		{
			GameEvent e = GameEventNew(GAME_EVENT_SCORE);
			e.u.Score.PlayerUID = playerUID;
			e.u.Score.Score = OBJECT_SCORE;
			GameEventsEnqueue(&gGameEvents, e);
		}

		// Weapons that go off when this object is destroyed
		const Vec2i fullPos = Vec2iReal2Full(realPos);
		for (int i = 0; i < (int)o->Class->DestroyGuns.size; i++)
		{
			const GunDescription **g = CArrayGet(&o->Class->DestroyGuns, i);
			GunFire(*g, fullPos, 0, 0, flags, playerUID, uid, true, false);
		}

		// A wreck left after the destruction of this object
		// TODO: doesn't need to be network event
		GameEvent e = GameEventNew(GAME_EVENT_ADD_BULLET);
		e.u.AddBullet.UID = MobObjsObjsGetNextUID();
		strcpy(e.u.AddBullet.BulletClass, "fireball_wreck");
		e.u.AddBullet.MuzzlePos = Vec2i2Net(fullPos);
		e.u.AddBullet.MuzzleHeight = 0;
		e.u.AddBullet.Angle = 0;
		e.u.AddBullet.Elevation = 0;
		e.u.AddBullet.Flags = 0;
		e.u.AddBullet.PlayerUID = -1;
		e.u.AddBullet.ActorUID = -1;
		GameEventsEnqueue(&gGameEvents, e);
	}

	SoundPlayAt(&gSoundDevice, gSoundDevice.wreckSound, realPos);

	// Turn the object into a wreck, if available
	if (o->Class->Wreck.Pic)
	{
		o->tileItem.flags = TILEITEM_IS_WRECK;
	}
	else
	{
		ObjDestroy(o->tileItem.id);
	}

	// Update pathfinding cache since this object could have blocked a path
	// before
	PathCacheClear(&gPathCache);
}

bool CanHit(const int flags, const int uid, const TTileItem *target)
{
	switch (target->kind)
	{
	case KIND_CHARACTER:
		return CanHitCharacter(flags, uid, CArrayGet(&gActors, target->id));
	case KIND_OBJECT:
		return true;
	default:
		CASSERT(false, "cannot damage tile item kind");
		break;
	}
	return false;
}
bool HasHitSound(
	const int power, const int flags, const int playerUID,
	const TileItemKind targetKind, const int targetUID,
	const special_damage_e special, const bool allowFriendlyHitSound)
{
	switch (targetKind)
	{
	case KIND_CHARACTER:
		{
			const TActor *a = ActorGetByUID(targetUID);
			return
				!ActorIsImmune(a, special) &&
				(allowFriendlyHitSound || !ActorIsInvulnerable(
					a, flags, playerUID, gCampaign.Entry.Mode));
		}
	case KIND_OBJECT:
		return power > 0;
	default:
		CASSERT(false, "cannot damage tile item kind");
		break;
	}
	return false;
}

static void DoDamageCharacter(
	const Vec2i hitVector,
	const int power,
	const int flags,
	const int playerUID,
	const int uid,
	TActor *actor,
	const special_damage_e special);
void Damage(
	const Vec2i hitVector,
	const int power,
	const int flags,
	const int playerUID,
	const int uid,
	const TileItemKind targetKind, const int targetUID,
	const special_damage_e special)
{
	switch (targetKind)
	{
	case KIND_CHARACTER:
		DoDamageCharacter(
			hitVector,
			power, flags, playerUID, uid,
			ActorGetByUID(targetUID), special);
		break;
	case KIND_OBJECT:
		{
			GameEvent e = GameEventNew(GAME_EVENT_MAP_OBJECT_DAMAGE);
			e.u.MapObjectDamage.UID = targetUID;
			e.u.MapObjectDamage.Power = power;
			e.u.MapObjectDamage.ActorUID = uid;
			e.u.MapObjectDamage.PlayerUID = playerUID;
			e.u.MapObjectDamage.Flags = flags;
			GameEventsEnqueue(&gGameEvents, e);
		}
		break;
	default:
		CASSERT(false, "cannot damage tile item kind");
		break;
	}
}
static void DoDamageCharacter(
	const Vec2i hitVector,
	const int power,
	const int flags,
	const int playerUID,
	const int uid,
	TActor *actor,
	const special_damage_e special)
{
	// Create events: hit, damage, score
	CASSERT(actor->isInUse, "Cannot damage nonexistent player");
	CASSERT(CanHitCharacter(flags, uid, actor), "damaging undamageable actor");

	if (ConfigGetBool(&gConfig, "Game.ShotsPushback"))
	{
		GameEvent ei = GameEventNew(GAME_EVENT_ACTOR_IMPULSE);
		ei.u.ActorImpulse.UID = actor->uid;
		ei.u.ActorImpulse.Vel = Vec2i2Net(Vec2iScaleDiv(
			Vec2iScale(hitVector, power), SHOT_IMPULSE_DIVISOR));
		ei.u.ActorImpulse.Pos = Vec2i2Net(actor->Pos);
		GameEventsEnqueue(&gGameEvents, ei);
	}

	const bool canDamage =
		CanDamageCharacter(flags, playerUID, uid, actor, special);

	GameEvent e = GameEventNew(GAME_EVENT_ACTOR_HIT);
	e.u.ActorHit.UID = actor->uid;
	e.u.ActorHit.PlayerUID = actor->PlayerUID;
	e.u.ActorHit.HitterPlayerUID = playerUID;
	e.u.ActorHit.Special = special;
	e.u.ActorHit.Power = canDamage ? power : 0;
	e.u.ActorHit.Vel = Vec2i2Net(hitVector);
	GameEventsEnqueue(&gGameEvents, e);

	if (canDamage)
	{
		// Don't score for friendly or player hits
		const bool isFriendly =
			(actor->flags & FLAGS_GOOD_GUY) ||
			(!IsPVP(gCampaign.Entry.Mode) && actor->PlayerUID >= 0);
		if (playerUID >= 0 && power != 0 && !isFriendly)
		{
			// Calculate score based on
			// if they hit a penalty character
			e = GameEventNew(GAME_EVENT_SCORE);
			e.u.Score.PlayerUID = playerUID;
			if (actor->flags & FLAGS_PENALTY)
			{
				e.u.Score.Score = PENALTY_MULTIPLIER * power;
			}
			else
			{
				e.u.Score.Score = power;
			}
			GameEventsEnqueue(&gGameEvents, e);
		}
	}
}


void UpdateMobileObjects(int ticks)
{
	for (int i = 0; i < (int)gMobObjs.size; i++)
	{
		TMobileObject *obj = CArrayGet(&gMobObjs, i);
		if (!obj->isInUse)
		{
			continue;
		}
		if (!obj->updateFunc(obj, ticks) && !gCampaign.IsClient)
		{
			GameEvent e = GameEventNew(GAME_EVENT_REMOVE_BULLET);
			e.u.RemoveBullet.UID = obj->UID;
			GameEventsEnqueue(&gGameEvents, e);
			continue;
		}
		CPicUpdate(&obj->tileItem.CPic, ticks);
	}
}


void ObjsInit(void)
{
	CArrayInit(&gObjs, sizeof(TObject));
	CArrayReserve(&gObjs, 1024);
	sObjUIDs = 0;
}
void ObjsTerminate(void)
{
	for (int i = 0; i < (int)gObjs.size; i++)
	{
		TObject *o = CArrayGet(&gObjs, i);
		if (o->isInUse)
		{
			ObjDestroy(i);
		}
	}
	CArrayTerminate(&gObjs);
}
int ObjsGetNextUID(void)
{
	return sObjUIDs++;
}

void ObjAdd(const NMapObjectAdd amo)
{
	// Find an empty slot in object list
	TObject *o = NULL;
	int i;
	for (i = 0; i < (int)gObjs.size; i++)
	{
		TObject *obj = CArrayGet(&gObjs, i);
		if (!obj->isInUse)
		{
			o = obj;
			break;
		}
	}
	if (o == NULL)
	{
		TObject obj;
		memset(&obj, 0, sizeof obj);
		CArrayPushBack(&gObjs, &obj);
		i = (int)gObjs.size - 1;
		o = CArrayGet(&gObjs, i);
	}
	memset(o, 0, sizeof *o);
	o->uid = amo.UID;
	o->Class = StrMapObject(amo.MapObjectClass);
	o->Health = amo.Health;
	o->tileItem.x = o->tileItem.y = -1;
	o->tileItem.flags = amo.TileItemFlags;
	o->tileItem.kind = KIND_OBJECT;
	o->tileItem.getPicFunc = GetObjectPic;
	o->tileItem.getActorPicsFunc = NULL;
	o->tileItem.size = o->Class->Size;
	o->tileItem.id = i;
	MapTryMoveTileItem(&gMap, &o->tileItem, Net2Vec2i(amo.Pos));
	o->isInUse = true;
}
void ObjDestroy(int id)
{
	TObject *o = CArrayGet(&gObjs, id);
	CASSERT(o->isInUse, "Destroying in-use object");
	MapRemoveTileItem(&gMap, &o->tileItem);
	o->isInUse = false;
}

bool ObjIsDangerous(const TObject *o)
{
	// TODO: something more sophisticated? Check if weapon is dangerous
	return o->Class->DestroyGuns.size > 0;
}

void UpdateObjects(const int ticks)
{
	for (int i = 0; i < (int)gObjs.size; i++)
	{
		TObject *obj = CArrayGet(&gObjs, i);
		if (!obj->isInUse)
		{
			continue;
		}
		switch (obj->Class->Type)
		{
		case MAP_OBJECT_TYPE_PICKUP_SPAWNER:
			if (gCampaign.IsClient) break;
			// If counter -1, it is inactive i.e. already spawned pickup
			if (obj->counter == -1)
			{
				break;
			}
			obj->counter -= ticks;
			if (obj->counter <= 0)
			{
				// Deactivate spawner by setting counter to -1
				// Spawner reactivated only when ammo taken
				obj->counter = -1;
				GameEvent e = GameEventNew(GAME_EVENT_ADD_PICKUP);
				e.u.AddPickup.UID = PickupsGetNextUID();
				strcpy(
					e.u.AddPickup.PickupClass,
					obj->Class->u.PickupClass->Name);
				e.u.AddPickup.IsRandomSpawned = false;
				e.u.AddPickup.SpawnerUID = obj->uid;
				e.u.AddPickup.TileItemFlags = 0;
				e.u.AddPickup.Pos =
					Vec2i2Net(Vec2iNew(obj->tileItem.x, obj->tileItem.y));
				GameEventsEnqueue(&gGameEvents, e);
			}
			break;
		default:
			// Do nothing
			break;
		}
	}
}

TObject *ObjGetByUID(const int uid)
{
	for (int i = 0; i < (int)gObjs.size; i++)
	{
		TObject *o = CArrayGet(&gObjs, i);
		if (o->uid == uid)
		{
			return o;
		}
	}
	CASSERT(false, "Cannot find object by UID");
	return NULL;
}


void MobObjsInit(void)
{
	CArrayInit(&gMobObjs, sizeof(TMobileObject));
	CArrayReserve(&gMobObjs, 1024);
	sMobObjUIDs = 0;
}
void MobObjsTerminate(void)
{
	for (int i = 0; i < (int)gMobObjs.size; i++)
	{
		TMobileObject *m = CArrayGet(&gMobObjs, i);
		if (m->isInUse)
		{
			MobObjDestroy(m);
		}
	}
	CArrayTerminate(&gMobObjs);
}
int MobObjsObjsGetNextUID(void)
{
	return sMobObjUIDs++;
}
TMobileObject *MobObjGetByUID(const int uid)
{
	for (int i = 0; i < (int)gMobObjs.size; i++)
	{
		TMobileObject *o = CArrayGet(&gMobObjs, i);
		if (o->UID == uid)
		{
			return o;
		}
	}
	return NULL;
}
void MobObjDestroy(TMobileObject *m)
{
	CASSERT(m->isInUse, "Destroying not-in-use mobobj");
	MapRemoveTileItem(&gMap, &m->tileItem);
	m->isInUse = false;
}
