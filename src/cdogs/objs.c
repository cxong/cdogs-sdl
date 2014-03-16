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

TMobileObject *gMobObjList = NULL;
int gMobileObjId;
static TObject *objList = NULL;


// Draw functions

Pic *GetObjectPic(void *data)
{
	const TObject * obj = data;
	const TOffsetPic *ofpic = obj->pic;
	if (!ofpic)
	{
		return NULL;
	}

	// Default old pic
	Pic *pic = PicManagerGetFromOld(&gPicManager, ofpic->picIndex);

	// Try to get new pic if available
	if (obj->picName && obj->picName[0] != '\0')
	{
		Pic *newPic = PicManagerGetPic(&gPicManager, obj->picName);
		if (newPic)
		{
			pic = newPic;
		}
	}
	pic->offset = Vec2iNew(ofpic->dx, ofpic->dy);
	return pic;
}


static void TrackKills(int player, TActor *victim)
{
	if (player >= 0)
	{
		if (victim->pData ||
			(victim->flags & (FLAGS_GOOD_GUY | FLAGS_PENALTY)))
		{
			gPlayerDatas[player].friendlies++;
		}
		else
		{
			gPlayerDatas[player].kills++;
		}
	}
}

// The damage function!

int DamageCharacter(
	Vec2i hitVector,
	int power,
	int flags,
	int player,
	TTileItem *target,
	special_damage_e damage,
	campaign_mode_e mode,
	int isHitSoundEnabled)
{
	TActor *actor = (TActor *)target->data;
	int isInvulnerable;

	// Don't let players hurt themselves
	if (!(flags & FLAGS_HURTALWAYS) && player >= 0 && actor->pData &&
		&gPlayerDatas[player] == actor->pData)
	{
		return 0;
	}

	if (ActorIsImmune(actor, damage))
	{
		return 1;
	}
	isInvulnerable = ActorIsInvulnerable(actor, flags, player, mode);
	ActorTakeHit(
		actor,
		hitVector,
		power,
		damage,
		isHitSoundEnabled,
		isInvulnerable,
		Vec2iNew(target->x, target->y));
	if (isInvulnerable)
	{
		return 1;
	}
	InjureActor(actor, power);
	if (actor->health <= 0)
	{
		TrackKills(player, actor);
	}
	// If a good guy hurt a non-good guy
	if ((player >= 0 || (flags & FLAGS_GOOD_GUY)) &&
		!(actor->pData || (actor->flags & FLAGS_GOOD_GUY)))
	{
		if (player >= 0 && power != 0)
		{
			// Calculate score based on if they hit a penalty character
			GameEvent e;
			e.Type = GAME_EVENT_SCORE;
			e.u.Score.PlayerIndex = player;
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

	return 1;
}

static void DamageObject(
	int power,
	int flags,
	int player,
	TTileItem *target,
	special_damage_e damage,
	int isHitSoundEnabled)
{
	TObject *object = (TObject *)target->data;
	// Don't bother if object already destroyed
	if (object->structure <= 0)
	{
		return;
	}

	object->structure -= power;
	if (isHitSoundEnabled && power > 0)
	{
		SoundPlayAt(
			&gSoundDevice,
			SoundGetHit(damage, 0),
			Vec2iNew(target->x, target->y));
	}

	// Destroying objects and all the wonderful things that happen
	if (object->structure <= 0)
	{
		object->structure = 0;
		if (CheckMissionObjective(
				&gMission, object->tileItem.flags, OBJECTIVE_DESTROY))
		{
			if (player >= 0)
			{
				GameEvent e;
				e.Type = GAME_EVENT_SCORE;
				e.u.Score.PlayerIndex = player;
				e.u.Score.Score = OBJECT_SCORE;
				GameEventsEnqueue(&gGameEvents, e);
			}
		}
		if (object->flags & OBJFLAG_QUAKE)
		{
			GameEvent shake;
			shake.Type = GAME_EVENT_SCREEN_SHAKE;
			shake.u.ShakeAmount = SHAKE_BIG_AMOUNT;
			GameEventsEnqueue(&gGameEvents, shake);
		}
		Vec2i fullPos = Vec2iReal2Full(
			Vec2iNew(object->tileItem.x, object->tileItem.y));
		if (object->flags & OBJFLAG_EXPLOSIVE)
		{
			AddExplosion(fullPos, flags, player);
		}
		else if (object->flags & OBJFLAG_FLAMMABLE)
		{
			AddFireExplosion(fullPos, flags, player);
		}
		else if (object->flags & OBJFLAG_POISONOUS)
		{
			AddGasExplosion(fullPos, flags, SPECIAL_POISON, player);
		}
		else if (object->flags & OBJFLAG_CONFUSING)
		{
			AddGasExplosion(fullPos, flags, SPECIAL_CONFUSE, player);
		}
		else
		{
			// A wreck left after the destruction of this object
			TMobileObject *obj = AddFireBall(0, -1);
			obj->count = 10;
			obj->power = 0;
			obj->x = fullPos.x;
			obj->y = fullPos.y;
			SoundPlayAt(
				&gSoundDevice,
				SND_BANG,
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
			RemoveObject(object);
		}
	}
}

int DamageSomething(
	Vec2i hitVector,
	int power,
	int flags,
	int player,
	TTileItem *target,
	special_damage_e damage,
	int isHitSoundEnabled)
{
	if (!target)
	{
		return 0;
	}

	switch (target->kind)
	{
	case KIND_CHARACTER:
		return DamageCharacter(
			hitVector,
			power,
			flags,
			player,
			target,
			damage,
			gCampaign.Entry.mode,
			gConfig.Sound.Hits && isHitSoundEnabled);

	case KIND_OBJECT:
		DamageObject(
			power, flags, player, target, damage,
			gConfig.Sound.Hits && isHitSoundEnabled);
		break;

	case KIND_PIC:
	case KIND_MOBILEOBJECT:
		break;
	}

	return 1;
}


void UpdateMobileObjects(TMobileObject **mobObjList, int ticks)
{
	TMobileObject *obj = *mobObjList;
	while (obj)
	{
		if ((*(obj->updateFunc))(obj, ticks) == 0)
		{
			obj->range = 0;
			GameEvent e;
			e.Type = GAME_EVENT_MOBILE_OBJECT_REMOVE;
			e.u.MobileObjectRemoveId = obj->id;
			GameEventsEnqueue(&gGameEvents, e);
		}
		obj = obj->next;
	}
}

void MobileObjectRemove(TMobileObject **mobObjList, int id)
{
	while (*mobObjList)
	{
		if ((*mobObjList)->id == id)
		{
			CASSERT(
				(*mobObjList)->range == 0,
				"unexpected removal of non-zero range mobobj");
			TMobileObject *obj = *mobObjList;
			*mobObjList = obj->next;
			MapRemoveTileItem(&gMap, &obj->tileItem);
			CFREE(obj);
			return;
		}
		else
		{
			mobObjList = &((*mobObjList)->next);
		}
	}
	CASSERT(false, "failed to remove mobile object");
}


void KillAllMobileObjects(TMobileObject **mobObjList)
{
	while (*mobObjList)
	{
		TMobileObject *o = *mobObjList;
		*mobObjList = (*mobObjList)->next;
		MapRemoveTileItem(&gMap, &o->tileItem);
		CFREE(o);
	}
}

static void InternalAddObject(
	int x, int y, int w, int h,
	const TOffsetPic * pic, const TOffsetPic * wreckedPic,
	const char *picName,
	int structure, int idx, int objFlags, int tileFlags)
{
	TObject *o;
	CCALLOC(o, sizeof(TObject));
	o->pic = pic;
	o->wreckedPic = wreckedPic;
	o->picName = picName;
	o->objectIndex = idx;
	o->structure = structure;
	o->flags = objFlags;
	o->tileItem.flags = tileFlags;
	o->tileItem.kind = KIND_OBJECT;
	o->tileItem.data = o;
	o->tileItem.getPicFunc = GetObjectPic;
	o->tileItem.getActorPicsFunc = NULL;
	o->tileItem.w = w;
	o->tileItem.h = h;
	o->tileItem.actor = NULL;
	MapMoveTileItem(&gMap, &o->tileItem, Vec2iFull2Real(Vec2iNew(x, y)));
	o->next = objList;
	objList = o;
}

void AddObject(
	int x, int y, Vec2i size, const TOffsetPic * pic, int idx, int tileFlags)
{
	InternalAddObject(
		x, y, size.x, size.y, pic, NULL, NULL, 0, idx, 0, tileFlags);
}

void AddDestructibleObject(
	Vec2i pos, int w, int h,
	const TOffsetPic * pic, const TOffsetPic * wreckedPic,
	const char *picName,
	int structure, int objFlags, int tileFlags)
{
	Vec2i fullPos = Vec2iReal2Full(pos);
	InternalAddObject(
		fullPos.x, fullPos.y, w, h, pic, wreckedPic, picName, structure, 0,
		objFlags, tileFlags);
}

void RemoveObject(TObject * obj)
{
	TObject **h = &objList;

	while (*h && *h != obj)
		h = &((*h)->next);
	if (*h) {
		*h = obj->next;
		MapRemoveTileItem(&gMap, &obj->tileItem);
		CFREE(obj);
	}
}

void KillAllObjects(void)
{
	TObject *o;

	while (objList) {
		o = objList;
		objList = objList->next;
		MapRemoveTileItem(&gMap, &o->tileItem);
		CFREE(o);
	}
}


void MobileObjectUpdate(TMobileObject *obj, int ticks)
{
	obj->count += ticks;
	obj->soundLock = MAX(0, obj->soundLock - ticks);
}
static int UpdateMobileObject(TMobileObject *obj, int ticks)
{
	MobileObjectUpdate(obj, ticks);
	if (obj->count > obj->range)
		return 0;
	return 1;
}
static void BogusDraw(Vec2i pos, TileItemDrawFuncData *data)
{
	UNUSED(pos);
	UNUSED(data);
}
TMobileObject *AddMobileObject(TMobileObject **mobObjList, int player)
{
	TMobileObject *obj;
	CCALLOC(obj, sizeof(TMobileObject));

	obj->id = gMobileObjId++;
	obj->player = player;
	obj->tileItem.kind = KIND_MOBILEOBJECT;
	obj->tileItem.data = obj;
	obj->special = SPECIAL_NONE;
	obj->soundLock = 0;
	obj->next = *mobObjList;
	obj->tileItem.getPicFunc = NULL;
	obj->tileItem.getActorPicsFunc = NULL;
	obj->tileItem.drawFunc = (TileItemDrawFunc)BogusDraw;
	obj->tileItem.drawData.Obj = obj;
	obj->updateFunc = UpdateMobileObject;
	*mobObjList = obj;
	return obj;
}

static void DrawFireball(Vec2i pos, TileItemDrawFuncData *data)
{
	const TMobileObject *obj = data->Obj;
	if (obj->count < obj->state)
	{
		return;
	}
	const TOffsetPic *pic = &cFireBallPics[(obj->count - obj->state) / 4];
	if (obj->z > 0)
	{
		pos.y -= obj->z / 4;
	}
	BlitOld(
		pos.x + pic->dx, pos.y + pic->dy,
		PicManagerGetOldPic(&gPicManager, pic->picIndex),
		NULL,
		BLIT_TRANSPARENT);
}
TMobileObject *AddFireBall(int flags, int player)
{
	TMobileObject *obj = AddMobileObject(&gMobObjList, player);
	obj->updateFunc = UpdateExplosion;
	obj->tileItem.drawFunc = (TileItemDrawFunc)DrawFireball;
	obj->tileItem.w = 7;
	obj->tileItem.h = 5;
	obj->kind = MOBOBJ_FIREBALL;
	obj->range = FIREBALL_MAX * 4 - 1;
	obj->flags = flags;
	obj->power = FIREBALL_POWER;
	return obj;
}

int UpdateExplosion(TMobileObject *obj, int ticks)
{
	MobileObjectUpdate(obj, ticks);
	if (obj->count < 0)
	{
		return 1;
	}
	if (obj->count > obj->range)
	{
		return 0;
	}

	Vec2i pos =
		Vec2iScale(Vec2iAdd(Vec2iNew(obj->x, obj->y), obj->vel), ticks);
	obj->z += obj->dz * ticks;
	obj->dz = MAX(0, obj->dz - ticks);

	HitItem(obj, pos, SPECIAL_EXPLOSION);

	if (!ShootWall(pos.x >> 8, pos.y >> 8))
	{
		obj->x = pos.x;
		obj->y = pos.y;
		MapMoveTileItem(
			&gMap, &obj->tileItem, Vec2iFull2Real(pos));
		return 1;
	}
	return 0;
}

int HitItem(TMobileObject *obj, Vec2i pos, special_damage_e special)
{
	TTileItem *item;
	int hasHit;
	Vec2i realPos = Vec2iFull2Real(pos);

	// Don't hit if no damage dealt
	// This covers non-damaging debris explosions
	if (obj->power <= 0 && (special == SPECIAL_NONE || special == SPECIAL_EXPLOSION))
	{
		return 0;
	}

	item = GetItemOnTileInCollision(
		&obj->tileItem, realPos, TILEITEM_CAN_BE_SHOT, COLLISIONTEAM_NONE,
		gCampaign.Entry.mode == CAMPAIGN_MODE_DOGFIGHT);
	hasHit = DamageSomething(
		obj->vel, obj->power, obj->flags, obj->player,
		item,
		special,
		obj->soundLock <= 0);
	if (hasHit && obj->soundLock <= 0)
	{
		obj->soundLock += SOUND_LOCK_MOBILE_OBJECT;
	}
	return hasHit;
}
