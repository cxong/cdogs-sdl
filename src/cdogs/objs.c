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
#include <math.h>
#include <string.h>
#include <stdlib.h>

#include "ai_utils.h"
#include "collision.h"
#include "config.h"
#include "drawtools.h"
#include "game_events.h"
#include "map.h"
#include "screen_shake.h"
#include "blit.h"
#include "pic_manager.h"
#include "defs.h"
#include "sounds.h"
#include "actors.h"
#include "gamedata.h"
#include "mission.h"
#include "game.h"
#include "utils.h"

#define SOUND_LOCK_MOBILE_OBJECT 12

BulletClass gBulletClasses[BULLET_COUNT];

TMobileObject *gMobObjList = NULL;
int gMobileObjId;
static TObject *objList = NULL;

static void Fire(int x, int y, int flags, int player);
static void Gas(int x, int y, int flags, special_damage_e special, int player);
int HitItem(TMobileObject * obj, int x, int y, special_damage_e special);


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

static void DrawBullet(Vec2i pos, TileItemDrawFuncData *data)
{
	const TMobileObject *obj = data->Obj;
	const TOffsetPic *pic = &cGeneralPics[data->u.Bullet.Ofspic];
	pos = Vec2iAdd(pos, Vec2iNew(pic->dx, pic->dy - obj->z));
	if (data->u.Bullet.UseMask)
	{
		BlitMasked(
			&gGraphicsDevice,
			PicManagerGetFromOld(&gPicManager, pic->picIndex),
			pos, data->u.Bullet.Mask, 1);
	}
	else
	{
		DrawBTPic(
			pos.x, pos.y,
			PicManagerGetOldPic(&gPicManager, pic->picIndex),
			&data->u.Bullet.Tint);
	}
}

static void DrawGrenadeShadow(GraphicsDevice *device, Vec2i pos)
{
	DrawShadow(device, pos, Vec2iNew(4, 3));
}

static void DrawMolotov(Vec2i pos, TileItemDrawFuncData *data)
{
	const TMobileObject *obj = data->Obj;
	const TOffsetPic *pic = &cGeneralPics[OFSPIC_MOLOTOV];
	if (obj->z > 0)
	{
		DrawGrenadeShadow(&gGraphicsDevice, pos);
		pos.y -= obj->z / 16;
	}
	DrawTPic(
		pos.x + pic->dx, pos.y + pic->dy,
		PicManagerGetOldPic(&gPicManager, pic->picIndex));
}

Pic *GetFlame(void *data)
{
	const TMobileObject *obj = data;
	const TOffsetPic *pic = &cFlamePics[obj->state & 3];
	Pic *p = PicManagerGetFromOld(&gPicManager, pic->picIndex);
	p->offset.x = pic->dx;
	p->offset.y = pic->dy - obj->z;
	return p;
}

static void DrawBeam(Vec2i pos, TileItemDrawFuncData *data)
{
	const TMobileObject *obj = data->Obj;
	const TOffsetPic *pic = &cBeamPics[data->u.Beam][obj->state];
	pos = Vec2iAdd(pos, Vec2iNew(pic->dx, pic->dy - obj->z));
	BlitMasked(
		&gGraphicsDevice,
		PicManagerGetFromOld(&gPicManager, pic->picIndex),
		pos, data->u.Bullet.Mask, 1);
}

static void DrawGrenade(Vec2i pos, TileItemDrawFuncData *data)
{
	const TMobileObject *obj = data->Obj;
	const TOffsetPic *pic = &cGrenadePics[(obj->count / 2) & 3];
	if (obj->z > 0)
	{
		DrawGrenadeShadow(&gGraphicsDevice, pos);
		pos.y -= obj->z / 16;
	}
	BlitMasked(
		&gGraphicsDevice,
		PicManagerGetFromOld(&gPicManager, pic->picIndex),
		Vec2iAdd(pos, Vec2iNew(pic->dx, pic->dy)), data->u.GrenadeColor, 1);
}

static void DrawGasCloud(Vec2i pos, TileItemDrawFuncData *data)
{
	const TMobileObject *obj = data->Obj;
	const TOffsetPic *pic = &cFireBallPics[8 + (obj->state & 3)];
	DrawBTPic(
		pos.x + pic->dx, pos.y + pic->dy - obj->z,
		PicManagerGetOldPic(&gPicManager, pic->picIndex),
		&data->u.Tint);
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

void BogusDraw(Vec2i pos, TileItemDrawFuncData *data)
{
	UNUSED(pos);
	UNUSED(data);
}


// Prototype this as it is used by DamageSomething()
static TMobileObject *AddFireBall(int flags, int player);


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

static void AddExplosion(int x, int y, int flags, int player)
{
	TMobileObject *obj;
	int i;

	GameEvent shake;
	shake.Type = GAME_EVENT_SCREEN_SHAKE;
	shake.u.ShakeAmount = SHAKE_SMALL_AMOUNT;
	GameEventsEnqueue(&gGameEvents, shake);

	flags |= FLAGS_HURTALWAYS;
	for (i = 0; i < 8; i++)
	{
		obj = AddFireBall(flags, player);
		obj->vel = GetFullVectorsForRadians(i * 0.25 * PI);
		obj->x = x + 2 * obj->vel.x;
		obj->y = y + 2 * obj->vel.y;
		obj->dz = 0;
	}
	for (i = 0; i < 8; i++)
	{
		obj = AddFireBall(flags, player);
		obj->vel = GetFullVectorsForRadians((i * 0.25 + 0.125) * PI);
		obj->x = x + obj->vel.x;
		obj->y = y + obj->vel.y;
		obj->vel = Vec2iScaleDiv(Vec2iScale(obj->vel, 3), 4);
		obj->dz = 8;
		obj->count = -8;
	}
	for (i = 0; i < 8; i++)
	{
		obj = AddFireBall(flags, player);
		obj->x = x;
		obj->y = y;
		obj->z = 0;
		obj->vel = GetFullVectorsForRadians(i * 0.25 * PI);
		obj->vel = Vec2iScaleDiv(obj->vel, 2);
		obj->dz = 11;
		obj->count = -16;
	}

	SoundPlayAt(&gSoundDevice, SND_EXPLOSION, Vec2iNew(x >> 8, y >> 8));
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
		if (object->flags & OBJFLAG_EXPLOSIVE)
		{
			AddExplosion(
				object->tileItem.x << 8, object->tileItem.y << 8,
				flags, player);
		}
		else if (object->flags & OBJFLAG_FLAMMABLE)
		{
			Fire(
				object->tileItem.x << 8, object->tileItem.y << 8,
				flags, player);
		}
		else if (object->flags & OBJFLAG_POISONOUS)
		{
			Gas(object->tileItem.x << 8,
				object->tileItem.y << 8,
				flags,
				SPECIAL_POISON,
				player);
		}
		else if (object->flags & OBJFLAG_CONFUSING)
		{
			Gas(object->tileItem.x << 8,
				object->tileItem.y << 8,
				flags,
				SPECIAL_CONFUSE,
				player);
		}
		else
		{
			// A wreck left after the destruction of this object
			TMobileObject *obj = AddFireBall(0, -1);
			obj->count = 10;
			obj->power = 0;
			obj->x = object->tileItem.x << 8;
			obj->y = object->tileItem.y << 8;
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


// Update functions

// Base update function
void MobileObjectUpdate(TMobileObject *obj, int ticks)
{
	obj->count += ticks;
	obj->soundLock = MAX(0, obj->soundLock - ticks);
}

int UpdateMobileObject(TMobileObject *obj, int ticks)
{
	MobileObjectUpdate(obj, ticks);
	if (obj->count > obj->range)
		return 0;
	return 1;
}

static void Frag(int x, int y, int flags, int player)
{
	flags |= FLAGS_HURTALWAYS;
	for (int i = 0; i < 16; i++)
	{
		AddBullet(
			Vec2iNew(x, y), i / 16.0 * 2 * PI, BULLET_FRAG, flags, player);
	}
	SoundPlayAt(&gSoundDevice, SND_BANG, Vec2iNew(x >> 8, y >> 8));
}

Vec2i UpdateAndGetCloudPosition(TMobileObject *obj, int ticks)
{
	Vec2i pos = Vec2iScale(obj->vel, ticks);
	pos.x += obj->x;
	pos.y += obj->y;
	for (int i = 0; i < ticks; i++)
	{
		if (obj->vel.x > 0)
		{
			obj->vel.x -= 4;
		}
		else if (obj->vel.x < 0)
		{
			obj->vel.x += 4;
		}

		if (obj->vel.y > 0)
		{
			obj->vel.y -= 3;
		}
		else if (obj->vel.y < 0)
		{
			obj->vel.y += 3;
		}
	}

	return pos;
}

int UpdateMolotovFlame(TMobileObject *obj, int ticks)
{
	Vec2i pos;
	int i;

	MobileObjectUpdate(obj, ticks);
	if (obj->count > obj->range)
		return 0;

	if ((obj->count & 3) == 0)
		obj->state = rand();

	for (i = 0; i < ticks; i++)
	{
		obj->z += obj->dz / 2;
		if (obj->z <= 0)
		{
			obj->z = 0;
		}
		else
		{
			obj->dz--;
		}
	}
	pos = UpdateAndGetCloudPosition(obj, ticks);

	HitItem(obj, pos.x, pos.y, SPECIAL_FLAME);

	if (!ShootWall(pos.x >> 8, pos.y >> 8))
	{
		obj->x = pos.x;
		obj->y = pos.y;
		MapMoveTileItem(&gMap, &obj->tileItem, Vec2iFull2Real(pos));
		return 1;
	} else
		return 1;
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

TMobileObject *AddMolotovFlame(int x, int y, int flags, int player)
{
	TMobileObject *obj = AddMobileObject(&gMobObjList, player);
	obj->updateFunc = UpdateMolotovFlame;
	obj->tileItem.getPicFunc = GetFlame;
	obj->tileItem.w = 5;
	obj->tileItem.h = 5;
	obj->kind = MOBOBJ_FIREBALL;
	obj->range = (FLAME_RANGE + rand() % 8) * 4;
	obj->flags = flags;
	obj->power = 2;
	obj->x = x;
	obj->y = y;
	obj->vel.x = 16 * (rand() % 32) - 256;
	obj->vel.y = 12 * (rand() % 32) - 192;
	obj->dz = 4 + rand() % 4;
	return obj;
}

static void Fire(int x, int y, int flags, int player)
{
	int i;

	flags |= FLAGS_HURTALWAYS;
	for (i = 0; i < 16; i++)
	{
		AddMolotovFlame(x, y, flags, player);
	}
	SoundPlayAt(&gSoundDevice, SND_BANG, Vec2iNew(x >> 8, y >> 8));
}

int UpdateGasCloud(TMobileObject *obj, int ticks)
{
	Vec2i pos;

	MobileObjectUpdate(obj, ticks);
	if (obj->count > obj->range)
		return 0;

	if ((obj->count & 3) == 0)
		obj->state = rand();

	pos = UpdateAndGetCloudPosition(obj, ticks);

	HitItem(obj, pos.x, pos.y, obj->special);

	if (!ShootWall(pos.x >> 8, pos.y >> 8))
	{
		obj->x = pos.x;
		obj->y = pos.y;
		MapMoveTileItem(&gMap, &obj->tileItem, Vec2iFull2Real(pos));
		return 1;
	} else
		return 1;
}

void AddGasCloud(
	Vec2i pos, int z, double radians, int speed, int range,
	int flags, special_damage_e special, int player)
{
	TMobileObject *obj = AddMobileObject(&gMobObjList, player);
	obj->updateFunc = UpdateGasCloud;
	obj->tileItem.drawFunc = (TileItemDrawFunc)DrawGasCloud;
	obj->tileItem.drawData.u.Tint =
		special == SPECIAL_CONFUSE ? tintPurple : tintPoison;
	obj->tileItem.w = 10;
	obj->tileItem.h = 10;
	obj->kind = MOBOBJ_FIREBALL;
	obj->range = range;
	obj->flags = flags;
	obj->power = 0;
	obj->special = special;
	obj->vel = GetFullVectorsForRadians(radians);
	obj->vel = Vec2iScaleDiv(Vec2iScale(obj->vel, speed), 256);
	obj->x = pos.x + 6 * obj->vel.x;
	obj->y = pos.y + 6 * obj->vel.y;
	obj->z = z;
}

static void Gas(int x, int y, int flags, special_damage_e special, int player)
{
	int i;

	flags |= FLAGS_HURTALWAYS;
	for (i = 0; i < 8; i++)
	{
		AddGasCloud(
			Vec2iNew(x, y), 0,
			(double)rand() / RAND_MAX * 2 * PI,
			(256 + rand()) & 255,
			(48 - (rand() % 8)) * 4 - 1,
			flags,
			special,
			player);
	}
	SoundPlayAt(&gSoundDevice, SND_BANG, Vec2iNew(x >> 8, y >> 8));
}

int UpdateGrenade(TMobileObject *obj, int ticks)
{
	MobileObjectUpdate(obj, ticks);
	if (obj->count > obj->range)
	{
		switch (obj->kind)
		{
		case MOBOBJ_GRENADE:
			AddExplosion(obj->x, obj->y, obj->flags, obj->player);
			break;

		case MOBOBJ_FRAGGRENADE:
			Frag(obj->x, obj->y, obj->flags, obj->player);
			break;

		case MOBOBJ_MOLOTOV:
			Fire(obj->x, obj->y, obj->flags, obj->player);
			break;

		case MOBOBJ_GASBOMB:
			Gas(obj->x, obj->y, obj->flags, SPECIAL_POISON, obj->player);
			break;

		case MOBOBJ_GASBOMB2:
			Gas(obj->x, obj->y, obj->flags, SPECIAL_CONFUSE, obj->player);
			break;
		}
		return 0;
	}

	int x = obj->x + obj->vel.x * ticks;
	int y = obj->y + obj->vel.y * ticks;

	for (int i = 0; i < ticks; i++)
	{
		obj->z += obj->dz;
		if (obj->z <= 0)
		{
			obj->z = 0;
			obj->dz = -obj->dz / 2;
		}
		else
		{
			obj->dz--;
		}
	}

	if (!ShootWall(x >> 8, y >> 8))
	{
		obj->x = x;
		obj->y = y;
	}
	else if (!ShootWall(obj->x >> 8, y >> 8))
	{
		obj->y = y;
		obj->vel.x = -obj->vel.x;
	}
	else if (!ShootWall(x >> 8, obj->y >> 8))
	{
		obj->x = x;
		obj->vel.y = -obj->vel.y;
	}
	else
	{
		obj->vel.x = -obj->vel.x;
		obj->vel.y = -obj->vel.y;
		return 1;
	}
	MapMoveTileItem(
		&gMap, &obj->tileItem, Vec2iFull2Real(Vec2iNew(obj->x, obj->y)));
	return 1;
}

int UpdateMolotov(TMobileObject *obj, int ticks)
{
	MobileObjectUpdate(obj, ticks);
	if (obj->count > obj->range)
	{
		Fire(obj->x, obj->y, obj->flags, obj->player);
		return 0;
	}

	int x = obj->x + obj->vel.x * ticks;
	int y = obj->y + obj->vel.y * ticks;

	obj->z += obj->dz * ticks;
	if (obj->z <= 0)
	{
		Fire(obj->x, obj->y, obj->flags, obj->player);
		return 0;
	}
	else
	{
		obj->dz -= ticks;
	}

	if (!ShootWall(x >> 8, y >> 8))
	{
		obj->x = x;
		obj->y = y;
	}
	else
	{
		Fire(obj->x, obj->y, obj->flags, obj->player);
		return 0;
	}
	MapMoveTileItem(
		&gMap, &obj->tileItem, Vec2iFull2Real(Vec2iNew(obj->x, obj->y)));
	return 1;
}

int UpdateSpark(TMobileObject *obj, int ticks)
{
	MobileObjectUpdate(obj, ticks);
	if (obj->count > obj->range)
		return 0;
	return 1;
}

int HitItem(TMobileObject * obj, int x, int y, special_damage_e special)
{
	TTileItem *item;
	int hasHit;
	Vec2i realPos = Vec2iScaleDiv(Vec2iNew(x, y), 256);

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

int InternalUpdateBullet(TMobileObject *obj, int special, int ticks)
{
	MobileObjectUpdate(obj, ticks);
	if (obj->count > obj->range)
		return 0;

	int x = obj->x + obj->vel.x * ticks;
	int y = obj->y + obj->vel.y * ticks;

	if (HitItem(obj, x, y, special)) {
		obj->count = 0;
		obj->range = 0;
		obj->tileItem.getPicFunc = NULL;
		obj->tileItem.getActorPicsFunc = NULL;
		obj->tileItem.drawFunc = (TileItemDrawFunc)DrawBullet;
		obj->tileItem.drawData.u.Bullet.Ofspic = OFSPIC_SPARK;
		obj->tileItem.drawData.u.Bullet.Mask = colorWhite;
		obj->updateFunc = UpdateSpark;
		return 1;
	}
	if (!ShootWall(x >> 8, y >> 8))
	{
		obj->x = x;
		obj->y = y;
		MapMoveTileItem(
			&gMap, &obj->tileItem, Vec2iFull2Real(Vec2iNew(x, y)));
		return 1;
	} else {
		obj->count = 0;
		obj->range = 0;
		obj->tileItem.getPicFunc = NULL;
		obj->tileItem.getActorPicsFunc = NULL;
		obj->tileItem.drawFunc = (TileItemDrawFunc)DrawBullet;
		obj->tileItem.drawData.u.Bullet.Ofspic = OFSPIC_SPARK;
		obj->tileItem.drawData.u.Bullet.Mask = colorWhite;
		obj->updateFunc = UpdateSpark;
		return 1;
	}
}

int UpdateBullet(TMobileObject *obj, int ticks)
{
	return InternalUpdateBullet(obj, 0, ticks);
}

int UpdatePetrifierBullet(TMobileObject *obj, int ticks)
{
	return InternalUpdateBullet(obj, SPECIAL_PETRIFY, ticks);
}

int UpdateSeeker(TMobileObject * obj, int ticks)
{
	TActor *target;
	if (!InternalUpdateBullet(obj, 0, ticks))
	{
		return 0;
	}
	// Find the closest target to this bullet and steer towards it
	// Compensate for the bullet's velocity
	target = AIGetClosestEnemy(
		Vec2iNew(obj->x, obj->y), obj->flags, obj->player >= 0);
	if (target)
	{
		double magnitude;
		int seekSpeed = 50;
		Vec2i impulse = Vec2iNew(
			target->Pos.x - obj->x - obj->vel.x * 2,
			target->Pos.y - obj->y - obj->vel.y * 2);
		// Don't seek if the coordinates are too big
		if (abs(impulse.x) < 10000 && abs(impulse.y) < 10000 &&
			(impulse.x != 0 || impulse.y != 0))
		{
			magnitude = sqrt(impulse.x*impulse.x + impulse.y*impulse.y);
			impulse.x = (int)floor(impulse.x * seekSpeed / magnitude);
			impulse.y = (int)floor(impulse.y * seekSpeed / magnitude);
		}
		else
		{
			impulse = Vec2iZero();
		}
		obj->vel = Vec2iAdd(obj->vel, Vec2iScale(impulse, ticks));
	}
	return 1;
}

int UpdateBrownBullet(TMobileObject *obj, int ticks)
{
	if (InternalUpdateBullet(obj, 0, ticks))
	{
		int i;
		for (i = 0; i < ticks; i++)
		{
			obj->vel.x += ((rand() % 3) - 1) * 128;
			obj->vel.y += ((rand() % 3) - 1) * 128;
		}
		return 1;
	}
	return 0;
}

int UpdateTriggeredMine(TMobileObject *obj, int ticks)
{
	MobileObjectUpdate(obj, ticks);
	if (obj->count >= obj->range)
	{
		AddExplosion(obj->x, obj->y, obj->flags, obj->player);
		return 0;
	}
	return 1;
}

int UpdateActiveMine(TMobileObject *obj, int ticks)
{
	Vec2i tv = Vec2iToTile(Vec2iFull2Real(Vec2iNew(obj->x, obj->y)));
	Vec2i dv;

	MobileObjectUpdate(obj, ticks);

	// Check if the mine is still arming
	if (obj->count & 3)
	{
		return 1;
	}

	if (!MapIsTileIn(&gMap, tv))
	{
		return 0;
	}

	// Detonate the mine if there are characters in the tiles around it
	for (dv.y = -1; dv.y <= 1; dv.y++)
	{
		for (dv.x = -1; dv.x <= 1; dv.x++)
		{
			if (TileHasCharacter(MapGetTile(&gMap, Vec2iAdd(tv, dv))))
			{
				obj->updateFunc = UpdateTriggeredMine;
				obj->count = 0;
				obj->range = 5;
				SoundPlayAt(
					&gSoundDevice,
					SND_HAHAHA,
					Vec2iNew(obj->tileItem.x, obj->tileItem.y));
				return 1;
			}
		}
	}
	return 1;
}

int UpdateDroppedMine(TMobileObject *obj, int ticks)
{
	MobileObjectUpdate(obj, ticks);
	if (obj->count >= obj->range)
		obj->updateFunc = UpdateActiveMine;
	return 1;
}

int UpdateFlame(TMobileObject *obj, int ticks)
{
	MobileObjectUpdate(obj, ticks);
	if (obj->count > obj->range)
		return 0;

	if ((obj->count & 3) == 0)
		obj->state = rand();

	int x = obj->x + obj->vel.x * ticks;
	int y = obj->y + obj->vel.y * ticks;

	if (HitItem(obj, x, y, SPECIAL_FLAME)) {
		obj->count = obj->range;
		obj->x = x;
		obj->y = y;
		MapMoveTileItem(
			&gMap, &obj->tileItem, Vec2iFull2Real(Vec2iNew(x, y)));
		return 1;
	}

	if (!ShootWall(x >> 8, y >> 8))
	{
		obj->x = x;
		obj->y = y;
		MapMoveTileItem(
			&gMap, &obj->tileItem, Vec2iFull2Real(Vec2iNew(x, y)));
		return 1;
	} else
		return 0;
}

int UpdateExplosion(TMobileObject *obj, int ticks)
{
	MobileObjectUpdate(obj, ticks);
	if (obj->count < 0)
		return 1;
	if (obj->count > obj->range)
		return 0;

	int x = obj->x + obj->vel.x * ticks;
	int y = obj->y + obj->vel.y * ticks;
	obj->z += obj->dz * ticks;
	obj->dz = MAX(0, obj->dz - ticks);

	HitItem(obj, x, y, SPECIAL_EXPLOSION);

	if (!ShootWall(x >> 8, y >> 8))
	{
		obj->x = x;
		obj->y = y;
		MapMoveTileItem(
			&gMap, &obj->tileItem, Vec2iFull2Real(Vec2iNew(x, y)));
		return 1;
	}
	return 0;
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


void BulletInitialize(void)
{
	// Defaults
	int i;
	BulletClass *b;
	for (i = 0; i < BULLET_COUNT; i++)
	{
		b = &gBulletClasses[i];
		b->UpdateFunc = UpdateBullet;
		b->GetPicFunc = NULL;
		b->DrawFunc = NULL;
		b->Size = 0;
	}

	b = &gBulletClasses[BULLET_MG];
	b->DrawFunc = (TileItemDrawFunc)DrawBullet;
	b->DrawData.u.Bullet.Ofspic = OFSPIC_BULLET;
	b->DrawData.u.Bullet.UseMask = true;
	b->DrawData.u.Bullet.Mask = colorWhite;
	b->Speed = 768;
	b->Range = 60;
	b->Power = 10;

	b = &gBulletClasses[BULLET_SHOTGUN];
	b->DrawFunc = (TileItemDrawFunc)DrawBullet;
	b->DrawData.u.Bullet.Ofspic = OFSPIC_BULLET;
	b->DrawData.u.Bullet.UseMask = true;
	b->DrawData.u.Bullet.Mask = colorWhite;
	b->Speed = 640;
	b->Range = 50;
	b->Power = 15;

	b = &gBulletClasses[BULLET_FLAME];
	b->UpdateFunc = UpdateFlame;
	b->GetPicFunc = GetFlame;
	b->Speed = 384;
	b->Range = 30;
	b->Power = 12;
	b->Size = 5;

	b = &gBulletClasses[BULLET_LASER];
	b->DrawFunc = (TileItemDrawFunc)DrawBeam;
	b->DrawData.u.Beam = BEAM_PIC_BEAM;
	b->Speed = 1024;
	b->Range = 90;
	b->Power = 20;
	b->Size = 2;

	b = &gBulletClasses[BULLET_SNIPER];
	b->DrawFunc = (TileItemDrawFunc)DrawBeam;
	b->DrawData.u.Beam = BEAM_PIC_BRIGHT;
	b->Speed = 1024;
	b->Range = 90;
	b->Power = 50;

	b = &gBulletClasses[BULLET_FRAG];
	b->DrawFunc = (TileItemDrawFunc)DrawBullet;
	b->DrawData.u.Bullet.Ofspic = OFSPIC_BULLET;
	b->DrawData.u.Bullet.UseMask = true;
	b->DrawData.u.Bullet.Mask = colorWhite;
	b->Speed = 640;
	b->Range = 50;
	b->Power = 40;


	// Grenades

	b = &gBulletClasses[BULLET_GRENADE];
	b->UpdateFunc = UpdateGrenade;
	b->DrawFunc = (TileItemDrawFunc)DrawGrenade;
	b->DrawData.u.GrenadeColor = colorWhite;
	b->Speed = 384;
	b->Range = 100;
	b->Power = 0;

	b = &gBulletClasses[BULLET_SHRAPNELBOMB];
	b->UpdateFunc = UpdateGrenade;
	b->DrawFunc = (TileItemDrawFunc)DrawGrenade;
	b->DrawData.u.GrenadeColor = colorGray;
	b->Speed = 384;
	b->Range = 100;
	b->Power = 0;

	b = &gBulletClasses[BULLET_MOLOTOV];
	b->UpdateFunc = UpdateGrenade;
	b->DrawFunc = (TileItemDrawFunc)DrawGrenade;
	b->DrawData.u.GrenadeColor = colorWhite;
	b->Speed = 384;
	b->Range = 100;
	b->Power = 0;

	b = &gBulletClasses[BULLET_GASBOMB];
	b->UpdateFunc = UpdateGrenade;
	b->DrawFunc = (TileItemDrawFunc)DrawGrenade;
	b->DrawData.u.GrenadeColor = colorGreen;
	b->Speed = 384;
	b->Range = 100;
	b->Power = 0;

	b = &gBulletClasses[BULLET_CONFUSEBOMB];
	b->UpdateFunc = UpdateGrenade;
	b->DrawFunc = (TileItemDrawFunc)DrawGrenade;
	b->DrawData.u.GrenadeColor = colorPurple;
	b->Speed = 384;
	b->Range = 100;
	b->Power = 0;


	b = &gBulletClasses[BULLET_RAPID];
	b->DrawFunc = (TileItemDrawFunc)DrawBullet;
	b->DrawData.u.Bullet.Ofspic = OFSPIC_SNIPERBULLET;
	b->DrawData.u.Bullet.UseMask = true;
	b->DrawData.u.Bullet.Mask = colorWhite;
	b->Speed = 1280;
	b->Range = 25;
	b->Power = 6;

	b = &gBulletClasses[BULLET_HEATSEEKER];
	b->UpdateFunc = UpdateSeeker;
	b->DrawFunc = (TileItemDrawFunc)DrawBullet;
	b->DrawData.u.Bullet.Ofspic = OFSPIC_SNIPERBULLET;
	b->DrawData.u.Bullet.UseMask = true;
	b->DrawData.u.Bullet.Mask = colorRed;
	b->Speed = 512;
	b->Range = 60;
	b->Power = 20;
	b->Size = 3;

	b = &gBulletClasses[BULLET_BROWN];
	b->UpdateFunc = UpdateBrownBullet;
	b->DrawFunc = (TileItemDrawFunc)DrawBullet;
	b->DrawData.u.Bullet.Ofspic = OFSPIC_SNIPERBULLET;
	b->DrawData.u.Bullet.UseMask = true;
	b->DrawData.u.Bullet.Mask = colorWhite;
	b->Speed = 768;
	b->Range = 45;
	b->Power = 15;

	b = &gBulletClasses[BULLET_PETRIFIER];
	b->UpdateFunc = UpdatePetrifierBullet;
	b->DrawFunc = (TileItemDrawFunc)DrawBullet;
	b->DrawData.u.Bullet.Ofspic = OFSPIC_MOLOTOV;
	b->DrawData.u.Bullet.UseMask = false;
	b->DrawData.u.Bullet.Tint = tintDarker;
	b->Speed = 768;
	b->Range = 45;
	b->Power = 0;
	b->Size = 5;

	b = &gBulletClasses[BULLET_PROXMINE];
	b->UpdateFunc = UpdateDroppedMine;
	b->DrawFunc = (TileItemDrawFunc)DrawBullet;
	b->DrawData.u.Bullet.Ofspic = OFSPIC_MINE;
	b->DrawData.u.Bullet.UseMask = true;
	b->DrawData.u.Bullet.Mask = colorWhite;
	b->Speed = 0;
	b->Range = 140;
	b->Power = 0;

	b = &gBulletClasses[BULLET_DYNAMITE];
	b->UpdateFunc = UpdateTriggeredMine;
	b->DrawFunc = (TileItemDrawFunc)DrawBullet;
	b->DrawData.u.Bullet.Ofspic = OFSPIC_DYNAMITE;
	b->DrawData.u.Bullet.UseMask = true;
	b->DrawData.u.Bullet.Mask = colorWhite;
	b->Speed = 0;
	b->Range = 210;
	b->Power = 0;
}


static void SetBulletProps(
	TMobileObject *obj, Vec2i pos, BulletType type, int flags)
{
	BulletClass *b = &gBulletClasses[type];
	obj->bulletClass = *b;
	obj->updateFunc = b->UpdateFunc;
	obj->tileItem.getPicFunc = b->GetPicFunc;
	obj->tileItem.getActorPicsFunc = NULL;
	obj->tileItem.drawFunc = b->DrawFunc;
	obj->tileItem.drawData.u = b->DrawData.u;
	obj->kind = MOBOBJ_BULLET;
	obj->z = BULLET_Z;
	obj->flags = flags;
	obj->x = pos.x;
	obj->y = pos.y;
	obj->vel = Vec2iScaleDiv(Vec2iScale(obj->vel, b->Speed), 256);
	obj->range = b->Range;
	obj->power = b->Power;
	obj->tileItem.w = b->Size;
	obj->tileItem.h = b->Size;
}

void AddGrenade(Vec2i pos, double radians, BulletType type, int flags, int player)
{
	TMobileObject *obj = AddMobileObject(&gMobObjList, player);
	obj->vel = GetFullVectorsForRadians(radians);
	obj->dz = 24;
	SetBulletProps(obj, pos, type, flags);
	switch (type)
	{
	case BULLET_GRENADE:
		obj->kind = MOBOBJ_GRENADE;
		break;
	case BULLET_SHRAPNELBOMB:
		obj->kind = MOBOBJ_FRAGGRENADE;
		break;
	case BULLET_MOLOTOV:
		obj->kind = MOBOBJ_MOLOTOV;
		obj->updateFunc = UpdateMolotov;
		obj->tileItem.getPicFunc = NULL;
		obj->tileItem.getActorPicsFunc = NULL;
		obj->tileItem.drawFunc = (TileItemDrawFunc)DrawMolotov;
		break;
	case BULLET_GASBOMB:
		obj->kind = MOBOBJ_GASBOMB;
		break;
	case BULLET_CONFUSEBOMB:
		obj->kind = MOBOBJ_GASBOMB2;
		break;
	default:
		assert(0 && "invalid grenade type");
		break;
	}
	obj->z = 0;
}

void AddBullet(Vec2i pos, double radians, BulletType type, int flags, int player)
{
	TMobileObject *obj = AddMobileObject(&gMobObjList, player);
	obj->vel = GetFullVectorsForRadians(radians);
	SetBulletProps(obj, pos, type, flags);
}

void AddBulletDirectional(
	Vec2i pos, direction_e dir, BulletType type, int flags, int player)
{
	TMobileObject *obj = AddMobileObject(&gMobObjList, player);
	obj->vel = GetFullVectorsForRadians(dir2radians[dir]);
	obj->state = dir;
	SetBulletProps(obj, pos, type, flags);
}

void AddBulletBig(
	Vec2i pos, double radians, BulletType type, int flags, int player)
{
	TMobileObject *obj = AddMobileObject(&gMobObjList, player);
	obj->vel = GetFullVectorsForRadians(radians);
	SetBulletProps(obj, pos, type, flags);
	obj->x = obj->x + 4 * obj->vel.x;
	obj->y = obj->y + 7 * obj->vel.y;
}

void AddBulletGround(
	Vec2i pos, double radians, BulletType type, int flags, int player)
{
	TMobileObject *obj = AddMobileObject(&gMobObjList, player);
	obj->vel = GetFullVectorsForRadians(radians);
	SetBulletProps(obj, pos, type, flags);
	obj->z = 0;
	MapMoveTileItem(
		&gMap, &obj->tileItem, Vec2iFull2Real(Vec2iNew(obj->x, obj->y)));
}

static TMobileObject *AddFireBall(int flags, int player)
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
