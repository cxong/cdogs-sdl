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
#include "objs.h"

#include <assert.h>

#include "bullet_class.h"
#include "damage.h"
#include "gamedata.h"
#include "log.h"
#include "net_util.h"
#include "pickup.h"

CArray gObjs;
CArray gMobObjs;
static unsigned int sObjUIDs = 0;
static unsigned int sMobObjUIDs = 0;

// Draw functions

static void MapObjectDraw(
	GraphicsDevice *g, const int id, const struct vec2i pos)
{
	const TObject *obj = CArrayGet(&gObjs, id);
	CASSERT(obj->isInUse, "Cannot draw non-existent map object");
	CPicDrawContext c = CPicDrawContextNew();
	c.Offset = obj->Class->Offset;
	CPicDraw(g, &obj->thing.CPic, pos, &c);
}

#define NUM_SPALL_PARTICLES 3
#define SPALL_IMPULSE_FACTOR 1.0f
void DamageObject(const NThingDamage d)
{
	TObject *o = ObjGetByUID(d.UID);
	// Don't bother if object already destroyed
	if (o->Health <= 0)
	{
		return;
	}

	// Create damage spall
	Emitter em;
	EmitterInit(&em, NULL, svec2_zero(), -0.5f, 0.5f, 1, 4, 0, 0, 0);
	AddParticle ap;
	memset(&ap, 0, sizeof ap);
	ap.Pos = o->thing.Pos;
	ap.Angle = NAN;
	ap.Z = 10;
	ap.Vel =
		svec2_scale(svec2_normalize(NetToVec2(d.Vel)), SPALL_IMPULSE_FACTOR);
	// Generate spall
	for (int i = 0; i < MIN(o->Health, d.Power); i++)
	{
		char buf[256];
		sprintf(buf, "spall%d", rand() % NUM_SPALL_PARTICLES + 1);
		ap.Class = StrParticleClass(&gParticleClasses, buf);
		// Choose random colour from object
		ap.Mask = PicGetRandomColor(CPicGetPic(&o->Class->Pic, 0));
		EmitterStart(&em, &ap);
	}

	o->Health -= d.Power;

	// Destroying objects and all the wonderful things that happen
	if (o->Health <= 0)
	{
		if (!gCampaign.IsClient)
		{
			GameEvent e = GameEventNew(GAME_EVENT_MAP_OBJECT_REMOVE);
			e.u.MapObjectRemove.UID = o->uid;
			e.u.MapObjectRemove.ActorUID = d.SourceActorUID;
			e.u.MapObjectRemove.Flags = d.Flags;
			GameEventsEnqueue(&gGameEvents, e);
		}

		// Exploding spall
		EmitterInit(&em, NULL, svec2_zero(), -1.0f, 1.0f, 1, 16, 0, 0, 0);
		ap.Vel = svec2_scale(ap.Vel, 0.5f);
		for (int i = 0; i < 20; i++)
		{
			char buf[256];
			sprintf(buf, "spall%d", rand() % NUM_SPALL_PARTICLES + 1);
			ap.Class = StrParticleClass(&gParticleClasses, buf);
			// Choose random colour from object
			ap.Mask = PicGetRandomColor(CPicGetPic(&o->Class->Pic, 0));
			EmitterStart(&em, &ap);
		}
	}
}

static void AddPickupAtObject(const TObject *o, const PickupType type)
{
	GameEvent e = GameEventNew(GAME_EVENT_ADD_PICKUP);
	switch (type)
	{
	case PICKUP_HEALTH:
		if (!ConfigGetBool(&gConfig, "Game.HealthPickups"))
		{
			return;
		}
		strcpy(e.u.AddPickup.PickupClass, "health");
		break;
	case PICKUP_AMMO:
		if (!gCampaign.Setting.Ammo)
		{
			return;
		}
		// Pick a random ammo type and spawn it
		{
			const int ammoId = rand() % AmmoGetNumClasses(&gAmmo);
			const Ammo *a = AmmoGetById(&gAmmo, ammoId);
			sprintf(e.u.AddPickup.PickupClass, "ammo_%s", a->Name);
		}
		break;
	case PICKUP_GUN:
		// Pick a random mission gun type and spawn it
		{
			const int gunId = (int)(rand() % gMission.Weapons.size);
			const WeaponClass **wc = CArrayGet(&gMission.Weapons, gunId);
			sprintf(e.u.AddPickup.PickupClass, "gun_%s", (*wc)->name);
		}
		break;
	default:
		CASSERT(false, "unexpected pickup type");
		break;
	}
	e.u.AddPickup.Pos = Vec2ToNet(o->thing.Pos);
	e.u.AddPickup.IsRandomSpawned = true;
	GameEventsEnqueue(&gGameEvents, e);
}

static void PlaceWreck(const char *wreckClass, const Thing *ti);
void ObjRemove(const NMapObjectRemove mor)
{
	TObject *o = ObjGetByUID(mor.UID);
	o->Health = 0;

	if (!gCampaign.IsClient)
	{
		// Update objective
		UpdateMissionObjective(
			&gMission, o->thing.flags, OBJECTIVE_DESTROY, 1);
		const TActor *a = ActorGetByUID(mor.ActorUID);
		const int playerUID = a != NULL ? a->PlayerUID : -1;
		// Extra score if objective
		if ((o->thing.flags & THING_OBJECTIVE) && playerUID >= 0)
		{
			GameEvent e = GameEventNew(GAME_EVENT_SCORE);
			e.u.Score.PlayerUID = playerUID;
			e.u.Score.Score = OBJECT_SCORE;
			GameEventsEnqueue(&gGameEvents, e);
		}

		// Weapons that go off when this object is destroyed
		CA_FOREACH(const WeaponClass *, wc, o->Class->DestroyGuns)
		WeaponClassFire(
			*wc, o->thing.Pos, 0, 0, mor.Flags, mor.ActorUID, true, false);
		CA_FOREACH_END()

		// Random chance to add pickups in single player modes
		if (!IsPVP(gCampaign.Entry.Mode))
		{
			CA_FOREACH(
				const MapObjectDestroySpawn, mods, o->Class->DestroySpawn)
			const double chance = (double)rand() / RAND_MAX;
			if (chance < mods->SpawnChance)
			{
				AddPickupAtObject(o, mods->Type);
			}
			CA_FOREACH_END()
		}

		// A wreck left after the destruction of this object
		// TODO: doesn't need to be network event
		GameEvent e = GameEventNew(GAME_EVENT_ADD_BULLET);
		e.u.AddBullet.UID = MobObjsObjsGetNextUID();
		strcpy(e.u.AddBullet.BulletClass, "fireball_wreck");
		e.u.AddBullet.MuzzlePos = Vec2ToNet(o->thing.Pos);
		GameEventsEnqueue(&gGameEvents, e);
	}

	GameEvent es = GameEventNew(GAME_EVENT_SOUND_AT);
	strcpy(es.u.SoundAt.Sound, "bang");
	es.u.SoundAt.Pos = Vec2ToNet(o->thing.Pos);
	GameEventsEnqueue(&gGameEvents, es);

	// If wreck is available spawn it in the exact same position
	PlaceWreck(o->Class->Wreck, &o->thing);

	ObjDestroy(o);

	// Update pathfinding cache since this object could have blocked a path
	// before
	PathCacheClear(&gPathCache);
}
static void PlaceWreck(const char *wreckClass, const Thing *ti)
{
	if (wreckClass == NULL)
	{
		return;
	}
	GameEvent e = GameEventNew(GAME_EVENT_MAP_OBJECT_ADD);
	e.u.MapObjectAdd.UID = ObjsGetNextUID();
	const MapObject *mo = StrMapObject(wreckClass);
	CASSERT(mo != NULL, "cannot find wreck");
	if (mo == NULL)
	{
		LOG(LM_MAIN, LL_ERROR, "wreck (%s) not found", wreckClass);
		return;
	}
	strcpy(e.u.MapObjectAdd.MapObjectClass, mo->Name);
	e.u.MapObjectAdd.Pos = Vec2ToNet(ti->Pos);
	e.u.MapObjectAdd.ThingFlags = MapObjectGetFlags(mo);
	e.u.MapObjectAdd.Health = mo->Health;
	GameEventsEnqueue(&gGameEvents, e);
}

bool CanHit(const int flags, const int uid, const Thing *target)
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
	const ThingKind targetKind,
	const int targetUID, const special_damage_e special,
	const bool allowFriendlyHitSound)
{
	switch (targetKind)
	{
	case KIND_CHARACTER: {
		const TActor *a = ActorGetByUID(targetUID);
		return allowFriendlyHitSound || !ActorIsImmune(a, special);
	}
	case KIND_OBJECT:
		return true;
	default:
		CASSERT(false, "cannot damage tile item kind");
		break;
	}
	return false;
}

static void DoDamageThing(
	const ThingKind targetKind, const int targetUID, const TActor *source,
	const int flags, const special_damage_e special, const bool canDamage,
	const int power, const float mass, const struct vec2 hitVector);
static void DoDamageCharacter(
	const TActor *actor, const TActor *source, const struct vec2 hitVector,
	const int power, const float mass, const int flags,
	const special_damage_e special);
void Damage(
	const struct vec2 hitVector, const int power, const float mass,
	const int flags, const TActor *source, const ThingKind targetKind,
	const int targetUID, const special_damage_e special)
{
	switch (targetKind)
	{
	case KIND_CHARACTER: {
		const TActor *actor = ActorGetByUID(targetUID);
		DoDamageCharacter(
			actor, source, hitVector, power, mass, flags, special);
	}
	break;
	case KIND_OBJECT:
		DoDamageThing(
			targetKind, targetUID, source, flags, special, true, power, mass,
			hitVector);
		break;
	default:
		CASSERT(false, "cannot damage tile item kind");
		break;
	}
}
static void DoDamageThing(
	const ThingKind targetKind, const int targetUID, const TActor *source,
	const int flags, const special_damage_e special, const bool canDamage,
	const int power, const float mass, const struct vec2 hitVector)
{
	GameEvent e = GameEventNew(GAME_EVENT_THING_DAMAGE);
	e.u.ThingDamage.UID = targetUID;
	e.u.ThingDamage.Kind = targetKind;
	e.u.ThingDamage.SourceActorUID = source ? source->uid : -1;
	e.u.ThingDamage.Flags = flags;
	e.u.ThingDamage.Special = special;
	e.u.ThingDamage.Power = canDamage ? power : 0;
	e.u.ThingDamage.Mass = mass;
	e.u.ThingDamage.Vel = Vec2ToNet(hitVector);
	GameEventsEnqueue(&gGameEvents, e);
}
static void DoDamageCharacter(
	const TActor *actor, const TActor *source, const struct vec2 hitVector,
	const int power, const float mass, const int flags,
	const special_damage_e special)
{
	// Create events: hit, damage, score
	CASSERT(actor->isInUse, "Cannot damage nonexistent player");
	CASSERT(
		CanHitCharacter(flags, source ? source->uid : -1, actor),
		"damaging undamageable actor");

	// Shot pushback, based on mass and velocity
	const float impulseFactor = mass * SHOT_IMPULSE_FACTOR;
	const struct vec2 vel = svec2_scale(hitVector, impulseFactor);
	if (!svec2_is_zero(vel))
	{
		GameEvent ei = GameEventNew(GAME_EVENT_ACTOR_IMPULSE);
		ei.u.ActorImpulse.UID = actor->uid;
		ei.u.ActorImpulse.Vel = Vec2ToNet(vel);
		ei.u.ActorImpulse.Pos = Vec2ToNet(actor->Pos);
		GameEventsEnqueue(&gGameEvents, ei);
	}

	const bool canDamage = CanDamageCharacter(flags, source, actor, special);

	DoDamageThing(
		KIND_CHARACTER, actor->uid, source, flags, special, canDamage, power,
		mass, hitVector);

	if (canDamage)
	{
		// Don't score for friendly or player hits
		const bool isFriendly =
			(actor->flags & FLAGS_GOOD_GUY) ||
			(!IsPVP(gCampaign.Entry.Mode) && actor->PlayerUID >= 0);
		if (source && source->PlayerUID >= 0 && power != 0 && !isFriendly)
		{
			// Calculate score based on
			// if they hit a penalty character
			GameEvent e = GameEventNew(GAME_EVENT_SCORE);
			e.u.Score.PlayerUID = source->PlayerUID;
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
	CA_FOREACH(TMobileObject, obj, gMobObjs)
	if (!obj->isInUse)
	{
		continue;
	}
	if (!BulletUpdate(obj, ticks) && !gCampaign.IsClient)
	{
		GameEvent e = GameEventNew(GAME_EVENT_REMOVE_BULLET);
		e.u.RemoveBullet.UID = obj->UID;
		GameEventsEnqueue(&gGameEvents, e);
		continue;
	}
	CA_FOREACH_END()
}

void ObjsInit(void)
{
	CArrayInit(&gObjs, sizeof(TObject));
	CArrayReserve(&gObjs, 1024);
	sObjUIDs = 0;
}
void ObjsTerminate(void)
{
	CA_FOREACH(TObject, o, gObjs)
	if (o->isInUse)
	{
		ObjDestroy(o);
	}
	CA_FOREACH_END()
	CArrayTerminate(&gObjs);
}
int ObjsGetNextUID(void)
{
	return sObjUIDs++;
}

void ObjAdd(const NMapObjectAdd amo)
{
	// Don't add if UID exists
	if (ObjGetByUID(amo.UID) != NULL)
	{
		LOG(LM_MAIN, LL_DEBUG, "object uid(%d) already exists; not adding",
			(int)amo.UID);
		return;
	}
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
	ThingInit(&o->thing, i, KIND_OBJECT, o->Class->Size, amo.ThingFlags);
	o->Health = amo.Health;
	o->thing.CPic = o->Class->Pic;
	o->thing.CPic.Mask = Net2Color(amo.Mask);
	if (ColorEquals(o->thing.CPic.Mask, colorTransparent))
	{
		o->thing.CPic.Mask = o->Class->Pic.Mask;
	}
	o->thing.CPicFunc = MapObjectDraw;
	MapTryMoveThing(&gMap, &o->thing, NetToVec2(amo.Pos));
	EmitterInit(
		&o->damageSmoke, StrParticleClass(&gParticleClasses, "smoke_big"),
		svec2_zero(), -0.05f, 0.05f, 3, 3, 0, 0, 20);
	o->isInUse = true;
	LOG(LM_MAIN, LL_DEBUG,
		"added object uid(%d) class(%s) health(%d) pos(%d, %d)", (int)amo.UID,
		amo.MapObjectClass, amo.Health, amo.Pos.x, amo.Pos.y);

	// Update pathfinding cache since this object could block a path
	PathCacheClear(&gPathCache);
}

void ObjDestroy(TObject *o)
{
	CASSERT(o->isInUse, "Destroying in-use object");
	MapRemoveThing(&gMap, &o->thing);
	o->isInUse = false;
}

bool ObjIsDangerous(const TObject *o)
{
	// TODO: something more sophisticated? Check if weapon is dangerous
	return o->Class->DestroyGuns.size > 0;
}

void UpdateObjects(const int ticks)
{
	CA_FOREACH(TObject, obj, gObjs)
	if (!obj->isInUse)
	{
		continue;
	}
	ThingUpdate(&obj->thing, ticks);
	switch (obj->Class->Type)
	{
	case MAP_OBJECT_TYPE_NORMAL:
		// Emit smoke when damaged
		if (obj->Class->DamageSmoke.HealthThreshold >= 0 &&
			obj->Health <=
				obj->Class->Health * obj->Class->DamageSmoke.HealthThreshold)
		{
			AddParticle ap;
			memset(&ap, 0, sizeof ap);
			ap.Pos = svec2_add(
				obj->thing.Pos,
				svec2(
					RAND_FLOAT(-obj->thing.size.x / 4, obj->thing.size.x / 4),
					RAND_FLOAT(
						-obj->thing.size.y / 4, obj->thing.size.y / 4)));
			ap.Mask = colorWhite;
			EmitterUpdate(&obj->damageSmoke, &ap, ticks);
		}
		break;
	case MAP_OBJECT_TYPE_PICKUP_SPAWNER:
		if (gCampaign.IsClient)
			break;
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
			strcpy(e.u.AddPickup.PickupClass, obj->Class->u.PickupClass->Name);
			e.u.AddPickup.SpawnerUID = obj->uid;
			e.u.AddPickup.Pos = Vec2ToNet(obj->thing.Pos);
			GameEventsEnqueue(&gGameEvents, e);
		}
		break;
	default:
		// Do nothing
		break;
	}
	CA_FOREACH_END()
}

TObject *ObjGetByUID(const int uid)
{
	CA_FOREACH(TObject, o, gObjs)
	if (o->uid == uid)
	{
		return o;
	}
	CA_FOREACH_END()
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
	CA_FOREACH(TMobileObject, m, gMobObjs)
	if (m->isInUse)
	{
		BulletDestroy(m);
	}
	CA_FOREACH_END()
	CArrayTerminate(&gMobObjs);
}
int MobObjsObjsGetNextUID(void)
{
	return sMobObjUIDs++;
}
TMobileObject *MobObjGetByUID(const int uid)
{
	CA_FOREACH(TMobileObject, o, gMobObjs)
	if (o->UID == uid)
	{
		return o;
	}
	CA_FOREACH_END()
	return NULL;
}
