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
#include "screen_shake.h"
#include "blit.h"
#include "pic_manager.h"
#include "defs.h"
#include "actors.h"
#include "gamedata.h"
#include "mission.h"
#include "game.h"
#include "utils.h"

#define SOUND_LOCK_MOBILE_OBJECT 12

CArray gObjs;
CArray gMobObjs;


// Draw functions

const Pic *GetObjectPic(const int id, Vec2i *offset)
{
	const TObject *obj = CArrayGet(&gObjs, id);

	Pic *pic = NULL;
	// Try to get new pic if available
	if (obj->picName && obj->picName[0] != '\0')
	{
		pic = PicManagerGetPic(&gPicManager, obj->picName);
	}
	// Use new pic offset if old one unavailable
	const TOffsetPic *ofpic = obj->pic;
	if (!ofpic)
	{
		// If new one also unavailable, bail
		if (pic == NULL)
		{
			return NULL;
		}
		*offset = Vec2iScaleDiv(pic->size, -2);
	}
	else if (pic == NULL)
	{
		// Default old pic
		pic = PicManagerGetFromOld(&gPicManager, ofpic->picIndex);
		*offset = pic->offset;
	}
	if (ofpic != NULL)
	{
		*offset = Vec2iNew(ofpic->dx, ofpic->dy);
	}
	return pic;
}


static void DamageObject(
	const int power, const int flags, const int player, const int uid,
	TTileItem *target)
{
	TObject *object = CArrayGet(&gObjs, target->id);
	// Don't bother if object already destroyed
	if (object->structure <= 0)
	{
		return;
	}

	object->structure -= power;
	const Vec2i pos = Vec2iNew(target->x, target->y);

	// Destroying objects and all the wonderful things that happen
	if (object->structure <= 0)
	{
		object->structure = 0;
		UpdateMissionObjective(
			&gMission, object->tileItem.flags, OBJECTIVE_DESTROY,
			player, pos);
		if (object->flags & OBJFLAG_QUAKE)
		{
			GameEvent shake;
			shake.Type = GAME_EVENT_SCREEN_SHAKE;
			shake.u.ShakeAmount = SHAKE_BIG_AMOUNT;
			GameEventsEnqueue(&gGameEvents, shake);
		}
		const Vec2i fullPos = Vec2iReal2Full(
			Vec2iNew(object->tileItem.x, object->tileItem.y));
		if (object->flags & OBJFLAG_EXPLOSIVE)
		{
			GunAddBullets(
				StrGunDescription("explosion1"), fullPos, 0, 0,
				flags, player, uid, true);
			GunAddBullets(
				StrGunDescription("explosion2"), fullPos, 0, 0,
				flags, player, uid, true);
			GunAddBullets(
				StrGunDescription("explosion3"), fullPos, 0, 0,
				flags, player, uid, true);
		}
		else if (object->flags & OBJFLAG_FLAMMABLE)
		{
			GunAddBullets(
				StrGunDescription("fire_explosion"), fullPos, 0, 0,
				flags, player, uid, true);
		}
		else if (object->flags & OBJFLAG_POISONOUS)
		{
			GunAddBullets(
				StrGunDescription("gas_poison_explosion"), fullPos, 0, 0,
				flags, player, uid, true);
		}
		else if (object->flags & OBJFLAG_CONFUSING)
		{
			GunAddBullets(
				StrGunDescription("gas_confuse_explosion"), fullPos, 0, 0,
				flags, player, uid, true);
		}
		else
		{
			// A wreck left after the destruction of this object
			GameEvent e;
			memset(&e, 0, sizeof e);
			e.Type = GAME_EVENT_ADD_BULLET;
			e.u.AddBullet.BulletClass = StrBulletClass("fireball_wreck");
			e.u.AddBullet.MuzzlePos = fullPos;
			e.u.AddBullet.MuzzleHeight = 0;
			e.u.AddBullet.Angle = 0;
			e.u.AddBullet.Elevation = 0;
			e.u.AddBullet.Flags = 0;
			e.u.AddBullet.PlayerIndex = -1;
			e.u.AddBullet.UID = -1;
			GameEventsEnqueue(&gGameEvents, e);
			SoundPlayAt(
				&gSoundDevice,
				gSoundDevice.wreckSound,
				Vec2iNew(object->tileItem.x, object->tileItem.y));
		}
		if (object->wreckedPic)
		{
			object->tileItem.flags = TILEITEM_IS_WRECK;
			object->pic = object->wreckedPic;
			object->picName = "";
		}
		else
		{
			ObjDestroy(object->tileItem.id);
		}
	}
}

static bool DoDamageCharacter(
	const Vec2i pos,
	const Vec2i hitVector,
	const int power,
	const int flags,
	const int player,
	const int uid,
	const TTileItem *target,
	const special_damage_e special,
	const HitSounds *hitSounds,
	const bool allowFriendlyHitSound);
bool DamageSomething(
	const Vec2i hitVector,
	const int power,
	const int flags,
	const int player,
	const int uid,
	TTileItem *target,
	const special_damage_e special,
	const HitSounds *hitSounds,
	const bool allowFriendlyHitSound)
{
	if (!target)
	{
		return 0;
	}

	const Vec2i pos = Vec2iNew(target->x, target->y);
	switch (target->kind)
	{
	case KIND_CHARACTER:
		return DoDamageCharacter(
			pos, hitVector,
			power, flags, player, uid,
			target, special, hitSounds, allowFriendlyHitSound);

	case KIND_OBJECT:
		DamageObject(power, flags, player, uid, target);
		if (gConfig.Sound.Hits && hitSounds != NULL && power > 0)
		{
			GameEvent es;
			es.Type = GAME_EVENT_SOUND_AT;
			es.u.SoundAt.Sound = hitSounds->Object;
			es.u.SoundAt.Pos = pos;
			GameEventsEnqueue(&gGameEvents, es);
		}
		break;

	case KIND_PARTICLE:
	case KIND_MOBILEOBJECT:
		break;
	}

	return 1;
}
static bool DoDamageCharacter(
	const Vec2i pos,
	const Vec2i hitVector,
	const int power,
	const int flags,
	const int player,
	const int uid,
	const TTileItem *target,
	const special_damage_e special,
	const HitSounds *hitSounds,
	const bool allowFriendlyHitSound)
{
	// Create events: hit, damage, score
	TActor *actor = CArrayGet(&gActors, target->id);
	CASSERT(actor->isInUse, "Cannot damage nonexistent player");
	bool canHit = CanHitCharacter(flags, uid, actor);
	if (canHit)
	{
		GameEvent e;
		e.Type = GAME_EVENT_HIT_CHARACTER;
		e.u.HitCharacter.TargetId = actor->tileItem.id;
		e.u.HitCharacter.Special = special;
		GameEventsEnqueue(&gGameEvents, e);
		if (gConfig.Sound.Hits && hitSounds != NULL &&
			!ActorIsImmune(actor, special) &&
			(allowFriendlyHitSound || !ActorIsInvulnerable(
			actor, flags, player, gCampaign.Entry.Mode)))
		{
			GameEvent es;
			es.Type = GAME_EVENT_SOUND_AT;
			es.u.SoundAt.Sound = hitSounds->Flesh;
			es.u.SoundAt.Pos = pos;
			GameEventsEnqueue(&gGameEvents, es);
		}
		if (gConfig.Game.ShotsPushback)
		{
			GameEvent ei;
			ei.Type = GAME_EVENT_ACTOR_IMPULSE;
			ei.u.ActorImpulse.Id = actor->tileItem.id;
			ei.u.ActorImpulse.Vel =
				Vec2iScaleDiv(Vec2iScale(hitVector, power), 25);
			GameEventsEnqueue(&gGameEvents, ei);
		}
		if (CanDamageCharacter(flags, player, uid, actor, special))
		{
			GameEvent e1;
			e1.Type = GAME_EVENT_DAMAGE_CHARACTER;
			e1.u.DamageCharacter.Power = power;
			e1.u.DamageCharacter.PlayerIndex = player;
			e1.u.DamageCharacter.TargetId = actor->tileItem.id;
			e1.u.DamageCharacter.TargetPlayerIndex = -1;
			if (actor->pData)
			{
				e1.u.DamageCharacter.TargetPlayerIndex =
					actor->pData->playerIndex;
			}
			GameEventsEnqueue(&gGameEvents, e1);

			if (gConfig.Game.Gore != GORE_NONE)
			{
				GameEvent eb;
				memset(&eb, 0, sizeof eb);
				eb.Type = GAME_EVENT_ADD_PARTICLE;
				eb.u.AddParticle.FullPos = Vec2iReal2Full(pos);
				eb.u.AddParticle.Z = 10 * Z_FACTOR;
				int bloodPower = power * 2;
				int bloodSize = 1;
				while (bloodPower > 0)
				{
					switch (bloodSize)
					{
					case 1:
						eb.u.AddParticle.Class =
							StrParticleClass(&gParticleClasses, "blood1");
						break;
					case 2:
						eb.u.AddParticle.Class =
							StrParticleClass(&gParticleClasses, "blood2");
						break;
					default:
						eb.u.AddParticle.Class =
							StrParticleClass(&gParticleClasses, "blood3");
						break;
					}
					bloodSize++;
					if (bloodSize > 3)
					{
						bloodSize = 1;
					}
					eb.u.AddParticle.Vel =
						Vec2iScaleDiv(Vec2iScale(hitVector, rand() % 8 + 8), 20);
					eb.u.AddParticle.Vel.x += (rand() % 128) - 64;
					eb.u.AddParticle.Vel.y += (rand() % 128) - 64;
					eb.u.AddParticle.Angle = RAND_DOUBLE(0, PI * 2);
					eb.u.AddParticle.DZ = (rand() % 6) + 6;
					eb.u.AddParticle.Spin = RAND_DOUBLE(-0.1, 0.1);
					GameEventsEnqueue(&gGameEvents, eb);
					switch (gConfig.Game.Gore)
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

			if (player >= 0 && power != 0)
			{
				// Calculate score based on
				// if they hit a penalty character
				GameEvent e2;
				e2.Type = GAME_EVENT_SCORE;
				e2.u.Score.PlayerIndex = player;
				if (actor->flags & FLAGS_PENALTY)
				{
					e2.u.Score.Score = PENALTY_MULTIPLIER * power;
				}
				else
				{
					e2.u.Score.Score = power;
				}
				GameEventsEnqueue(&gGameEvents, e2);
			}
		}
	}
	return canHit;
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
		if ((*(obj->updateFunc))(obj, ticks) == 0)
		{
			GameEvent e;
			e.Type = GAME_EVENT_MOBILE_OBJECT_REMOVE;
			e.u.MobileObjectRemoveId = i;
			GameEventsEnqueue(&gGameEvents, e);
		}
		else
		{
			CPicUpdate(&obj->tileItem.CPic, ticks);
		}
	}
}


void ObjsInit(void)
{
	CArrayInit(&gObjs, sizeof(TObject));
	CArrayReserve(&gObjs, 1024);
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
void AddObjectOld(
	int x, int y, Vec2i size,
	const TOffsetPic * pic, PickupType type, int tileFlags)
{
	TObject *o = CArrayGet(&gObjs, ObjAdd(
		Vec2iNew(x, y), size, NULL, type, tileFlags));
	o->pic = pic;
	o->wreckedPic = NULL;
	o->structure = 0;
	o->flags = 0;
	MapTryMoveTileItem(&gMap, &o->tileItem, Vec2iFull2Real(Vec2iNew(x, y)));
}
int ObjAdd(
	Vec2i pos, Vec2i size,
	const char *picName, PickupType type, int tileFlags)
{
	// Find an empty slot in actor list
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
	o->pic = NULL;
	o->wreckedPic = NULL;
	o->picName = picName;
	o->Type = type;
	o->structure = 0;
	o->flags = 0;
	o->tileItem.x = o->tileItem.y = -1;
	o->tileItem.flags = tileFlags;
	o->tileItem.kind = KIND_OBJECT;
	o->tileItem.getPicFunc = GetObjectPic;
	o->tileItem.getActorPicsFunc = NULL;
	o->tileItem.w = size.x;
	o->tileItem.h = size.y;
	o->tileItem.id = i;
	MapTryMoveTileItem(&gMap, &o->tileItem, Vec2iFull2Real(pos));
	o->isInUse = true;
	return i;
}
void ObjAddDestructible(
	Vec2i pos, Vec2i size,
	const TOffsetPic *pic, const TOffsetPic *wreckedPic,
	const char *picName,
	int structure, int objFlags, int tileFlags)
{
	Vec2i fullPos = Vec2iReal2Full(pos);
	TObject *o = CArrayGet(&gObjs, ObjAdd(
		fullPos, size, picName, OBJ_NONE, tileFlags));
	o->pic = pic;
	o->wreckedPic = wreckedPic;
	o->structure = structure;
	o->flags = objFlags;
}
void ObjDestroy(int id)
{
	TObject *o = CArrayGet(&gObjs, id);
	CASSERT(o->isInUse, "Destroying in-use object");
	MapRemoveTileItem(&gMap, &o->tileItem);
	o->isInUse = false;
}


void MobileObjectUpdate(TMobileObject *obj, int ticks)
{
	obj->count += ticks;
	obj->soundLock = MAX(0, obj->soundLock - ticks);
}
bool UpdateMobileObject(TMobileObject *obj, const int ticks)
{
	MobileObjectUpdate(obj, ticks);
	return obj->count <= obj->range;
}
static void BogusDraw(Vec2i pos, TileItemDrawFuncData *data)
{
	UNUSED(pos);
	UNUSED(data);
}

void MobObjsInit(void)
{
	CArrayInit(&gMobObjs, sizeof(TMobileObject));
	CArrayReserve(&gMobObjs, 1024);
}
void MobObjsTerminate(void)
{
	for (int i = 0; i < (int)gMobObjs.size; i++)
	{
		TMobileObject *m = CArrayGet(&gMobObjs, i);
		if (m->isInUse)
		{
			MobObjDestroy(i);
		}
	}
	CArrayTerminate(&gMobObjs);
}
int MobObjAdd(const Vec2i fullpos, const int player, const int uid)
{
	// Find an empty slot in mobobj list
	TMobileObject *obj = NULL;
	int i;
	for (i = 0; i < (int)gMobObjs.size; i++)
	{
		TMobileObject *m = CArrayGet(&gMobObjs, i);
		if (!m->isInUse)
		{
			obj = m;
			break;
		}
	}
	if (obj == NULL)
	{
		TMobileObject m;
		memset(&m, 0, sizeof m);
		CArrayPushBack(&gMobObjs, &m);
		i = (int)gMobObjs.size - 1;
		obj = CArrayGet(&gMobObjs, i);
	}
	memset(obj, 0, sizeof *obj);
	obj->x = fullpos.x;
	obj->y = fullpos.y;
	obj->player = player;
	obj->uid = uid;
	obj->tileItem.kind = KIND_MOBILEOBJECT;
	obj->tileItem.id = i;
	obj->soundLock = 0;
	obj->isInUse = true;
	obj->tileItem.x = obj->tileItem.y = -1;
	obj->tileItem.getPicFunc = NULL;
	obj->tileItem.getActorPicsFunc = NULL;
	obj->tileItem.drawFunc = (TileItemDrawFunc)BogusDraw;
	obj->tileItem.drawData.MobObjId = i;
	obj->updateFunc = UpdateMobileObject;
	MapTryMoveTileItem(&gMap, &obj->tileItem, Vec2iFull2Real(fullpos));
	return i;
}
void MobObjDestroy(int id)
{
	TMobileObject *m = CArrayGet(&gMobObjs, id);
	CASSERT(m->isInUse, "Destroying not-in-use mobobj");
	MapRemoveTileItem(&gMap, &m->tileItem);
	m->isInUse = false;
}

typedef struct
{
	bool HasHit;
	bool MultipleHits;
	bool HasFirstCollision;
	TMobileObject *Obj;
} HitItemData;
static void HitItemFunc(TTileItem *ti, void *data);
bool HitItem(TMobileObject *obj, const Vec2i pos, const bool multipleHits)
{
	// Don't hit if no damage dealt
	// This covers non-damaging debris explosions
	if (obj->bulletClass->Power <= 0 &&
		obj->bulletClass->Special == SPECIAL_NONE)
	{
		return 0;
	}

	// Get all items that collide
	HitItemData data;
	data.HasHit = false;
	data.MultipleHits = multipleHits;
	data.HasFirstCollision = false;
	data.Obj = obj;
	CollideAllItems(
		&obj->tileItem, Vec2iFull2Real(pos),
		TILEITEM_CAN_BE_SHOT, COLLISIONTEAM_NONE,
		gCampaign.Entry.Mode == CAMPAIGN_MODE_DOGFIGHT,
		HitItemFunc, &data);
	return data.HasHit;
}
static void HitItemFunc(TTileItem *ti, void *data)
{
	HitItemData *hData = data;
	if (hData->HasFirstCollision && !hData->MultipleHits)
	{
		return;
	}
	hData->HasFirstCollision = true;
	hData->HasHit = DamageSomething(
		hData->Obj->vel, hData->Obj->bulletClass->Power,
		hData->Obj->flags, hData->Obj->player, hData->Obj->uid,
		ti,
		hData->Obj->bulletClass->Special,
		hData->Obj->soundLock <= 0 ? &hData->Obj->bulletClass->HitSound : NULL,
		true);
	if (hData->HasHit && hData->Obj->soundLock <= 0)
	{
		hData->Obj->soundLock += SOUND_LOCK_MOBILE_OBJECT;
	}
}
