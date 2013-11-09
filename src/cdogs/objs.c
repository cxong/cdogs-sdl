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

    Copyright (c) 2013, Cong Xu
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

#include <string.h>
#include <stdlib.h>

#include "collision.h"
#include "config.h"
#include "drawtools.h"
#include "game_events.h"
#include "map.h"
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


TMobileObject *gMobObjList = NULL;
static TObject *objList = NULL;

void Fire(int x, int y, int flags);
void Gas(int x, int y, int flags, int special);
int HitItem(TMobileObject * obj, int x, int y, special_damage_e special);


// Draw functions

void DrawObject(int x, int y, const TObject * obj)
{
	const TOffsetPic *pic = obj->pic;

	if (pic)
	{
		DrawTPic(
			x + pic->dx,
			y + pic->dy,
			PicManagerGetOldPic(&gPicManager, pic->picIndex));
	}
}

void DrawBullet(int x, int y, const TMobileObject * obj)
{
	const TOffsetPic *pic;

	pic = &cGeneralPics[OFSPIC_BULLET];
	DrawTPic(
		x + pic->dx,
		y + pic->dy - obj->z,
		PicManagerGetOldPic(&gPicManager, pic->picIndex));
}

void DrawBrownBullet(int x, int y, const TMobileObject * obj)
{
	const TOffsetPic *pic;

	pic = &cGeneralPics[OFSPIC_SNIPERBULLET];
	DrawTPic(
		x + pic->dx,
		y + pic->dy - obj->z,
		PicManagerGetOldPic(&gPicManager, pic->picIndex));
}

void DrawPetrifierBullet(int x, int y, const TMobileObject * obj)
{
	const TOffsetPic *pic = &cGeneralPics[OFSPIC_MOLOTOV];
	DrawBTPic(
		x + pic->dx, y + pic->dy - obj->z,
		PicManagerGetOldPic(&gPicManager, pic->picIndex),
		&tintDarker);
}

void DrawSeeker(int x, int y, const TMobileObject * obj)
{
	const TOffsetPic *pic;

	pic = &cGeneralPics[OFSPIC_SNIPERBULLET];
	DrawTTPic(
		x + pic->dx,
		y + pic->dy - obj->z,
		PicManagerGetOldPic(&gPicManager, pic->picIndex),
		tableFlamed);
}

void DrawMine(int x, int y, const TMobileObject * obj)
{
	const TOffsetPic *pic;

	pic = &cGeneralPics[OFSPIC_MINE];
	DrawTPic(
		x + pic->dx,
		y + pic->dy - obj->z,
		PicManagerGetOldPic(&gPicManager, pic->picIndex));
}

void DrawDynamite(int x, int y, const TMobileObject * obj)
{
	const TOffsetPic *pic;

	pic = &cGeneralPics[OFSPIC_DYNAMITE];
	DrawTPic(
		x + pic->dx,
		y + pic->dy - obj->z,
		PicManagerGetOldPic(&gPicManager, pic->picIndex));
}

static void DrawGrenadeShadow(GraphicsDevice *device, Vec2i pos)
{
	DrawShadow(device, pos, Vec2iNew(4, 3));
}

void DrawMolotov(int x, int y, const TMobileObject * obj)
{
	const TOffsetPic *pic;

	pic = &cGeneralPics[OFSPIC_MOLOTOV];
	if (obj->z > 0)
	{
		DrawGrenadeShadow(&gGraphicsDevice, Vec2iNew(x, y));
		y -= obj->z / 16;
	}
	DrawTPic(
		x + pic->dx,
		y + pic->dy,
		PicManagerGetOldPic(&gPicManager, pic->picIndex));
}

void DrawFlame(int x, int y, const TMobileObject * obj)
{
	const TOffsetPic *pic;

	pic = &cFlamePics[obj->state & 3];
	DrawTPic(
		x + pic->dx,
		y + pic->dy - obj->z,
		PicManagerGetOldPic(&gPicManager, pic->picIndex));
}

void DrawLaserBolt(int x, int y, const TMobileObject * obj)
{
	const TOffsetPic *pic;

	pic = &cBeamPics[obj->state];
	DrawTPic(
		x + pic->dx,
		y + pic->dy - obj->z,
		PicManagerGetOldPic(&gPicManager, pic->picIndex));
}

void DrawBrightBolt(int x, int y, const TMobileObject * obj)
{
	const TOffsetPic *pic;

	pic = &cBrightBeamPics[obj->state];
	DrawTPic(
		x + pic->dx,
		y + pic->dy - obj->z,
		PicManagerGetOldPic(&gPicManager, pic->picIndex));
}

void DrawSpark(int x, int y, const TMobileObject * obj)
{
	const TOffsetPic *pic;

	pic = &cGeneralPics[OFSPIC_SPARK];
	DrawTPic(
		x + pic->dx,
		y + pic->dy - obj->z,
		PicManagerGetOldPic(&gPicManager, pic->picIndex));
}

void DrawGrenade(int x, int y, const TMobileObject * obj)
{
	const TOffsetPic *pic;

	pic = &cGrenadePics[(obj->count / 2) & 3];
	if (obj->z > 0)
	{
		DrawGrenadeShadow(&gGraphicsDevice, Vec2iNew(x, y));
		y -= obj->z / 16;
	}
	DrawTPic(
		x + pic->dx,
		y + pic->dy,
		PicManagerGetOldPic(&gPicManager, pic->picIndex));
}

void DrawGasCloud(int x, int y, const TMobileObject * obj)
{
	const TOffsetPic *pic;

	pic = &cFireBallPics[8 + (obj->state & 3)];
	DrawBTPic(
		x + pic->dx, y + pic->dy,
		PicManagerGetOldPic(&gPicManager, pic->picIndex),
		obj->z ? &tintPurple : &tintPoison);
}

void DrawFireball(int x, int y, const TMobileObject * obj)
{
	const TOffsetPic *pic;

	if (obj->count < obj->state)
		return;
	pic = &cFireBallPics[(obj->count - obj->state) / 4];
	if (obj->z > 0)
	{
		y -= obj->z / 4;
	}
	DrawTPic(
		x + pic->dx,
		y + pic->dy,
		PicManagerGetOldPic(&gPicManager, pic->picIndex));
}

void BogusDraw(int x, int y, void *data)
{
	UNUSED(x);
	UNUSED(y);
	UNUSED(data);
}


// Prototype this as it is used by DamageSomething()
TMobileObject *AddFireBall(int flags);


static void TrackKills(TActor * victim, int flags)
{
	if (flags & FLAGS_PLAYER1)
	{
		if (victim->flags & (FLAGS_PLAYERS | FLAGS_GOOD_GUY | FLAGS_PENALTY))
		{
			gPlayerDatas[0].friendlies++;
		}
		else
		{
			gPlayerDatas[0].kills++;
		}
	}
	else if (flags & FLAGS_PLAYER2)
	{
		if (victim->flags & (FLAGS_PLAYERS | FLAGS_GOOD_GUY | FLAGS_PENALTY))
		{
			gPlayerDatas[1].friendlies++;
		}
		else
		{
			gPlayerDatas[1].kills++;
		}
	}
}

// The damage function!

int DamageCharacter(
	Vec2i hitVector,
	int power,
	int flags,
	TTileItem *target,
	special_damage_e damage,
	campaign_mode_e mode,
	int isHitSoundEnabled)
{
	TActor *actor = (TActor *)target->data;
	int isInvulnerable;

	if (!(flags & FLAGS_HURTALWAYS) &&
		(flags & FLAGS_PLAYERS) &&
		(flags & FLAGS_PLAYERS) == (actor->flags & FLAGS_PLAYERS))
	{
		return 0;
	}

	if (ActorIsImmune(actor, damage))
	{
		return 1;
	}
	isInvulnerable = ActorIsInvulnerable(actor, flags, mode);
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
		TrackKills(actor, flags);
	}
	// If a good guy hurt a non-good guy
	if ((flags & (FLAGS_PLAYERS | FLAGS_GOOD_GUY)) &&
		!(actor->flags & (FLAGS_PLAYERS | FLAGS_GOOD_GUY)))
	{
		// Calculate score
		if (actor->flags & FLAGS_PENALTY)
		{
			Score(flags, -3 * power);
		}
		else
		{
			Score(flags, power);
		}
	}

	return 1;
}

void DamageObject(
	int power,
	int flags,
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
		if (CheckMissionObjective(object->tileItem.flags))
		{
			Score(flags, 50);
		}
		if (object->flags & OBJFLAG_QUAKE)
		{
			GameEvent shake;
			shake.Type = GAME_EVENT_SCREEN_SHAKE;
			shake.u.ShakeAmount = 70;
			GameEventsEnqueue(&gGameEvents, shake);
		}
		if (object->flags & OBJFLAG_EXPLOSIVE)
		{
			AddExplosion(object->tileItem.x << 8, object->tileItem.y << 8, flags);
		}
		else if (object->flags & OBJFLAG_FLAMMABLE)
		{
			Fire(object->tileItem.x << 8, object->tileItem.y << 8, flags);
		}
		else if (object->flags & OBJFLAG_POISONOUS)
		{
			Gas(object->tileItem.x << 8,
				object->tileItem.y << 8,
				flags,
				SPECIAL_POISON);
		}
		else if (object->flags & OBJFLAG_CONFUSING)
		{
			Gas(object->tileItem.x << 8,
				object->tileItem.y << 8,
				flags,
				SPECIAL_CONFUSE);
		}
		else
		{
			TMobileObject *obj = AddFireBall(0);
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
			target,
			damage,
			gCampaign.Entry.mode,
			gConfig.Sound.Hits && isHitSoundEnabled);

	case KIND_OBJECT:
		DamageObject(power, flags, target, damage, gConfig.Sound.Hits && isHitSoundEnabled);
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

void Frag(int x, int y, int flags)
{
	int i;

	flags |= FLAGS_HURTALWAYS;
	for (i = 0; i < 16; i++)
	{
		AddBullet(x, y, i * 16, SHOTGUN_SPEED, SHOTGUN_RANGE, 40, flags);
	}
	SoundPlayAt(&gSoundDevice, SND_BANG, Vec2iNew(x >> 8, y >> 8));
}

Vec2i UpdateAndGetCloudPosition(TMobileObject *obj, int ticks)
{
	Vec2i pos;
	int i;

	pos.x = obj->x + obj->dx * ticks;
	pos.y = obj->y + obj->dy * ticks;

	for (i = 0; i < ticks; i++)
	{
		if (obj->dx > 0)
		{
			obj->dx -= 4;
		}
		else if (obj->dx < 0)
		{
			obj->dx += 4;
		}

		if (obj->dy > 0)
		{
			obj->dy -= 3;
		}
		else if (obj->dy < 0)
		{
			obj->dy += 3;
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

	if (!HitWall(pos.x >> 8, pos.y >> 8))
	{
		obj->x = pos.x;
		obj->y = pos.y;
		MoveTileItem(&obj->tileItem, pos.x >> 8, pos.y >> 8);
		return 1;
	} else
		return 1;
}

TMobileObject *AddMobileObject(TMobileObject **mobObjList)
{
	TMobileObject *obj;
	CCALLOC(obj, sizeof(TMobileObject));

	obj->tileItem.kind = KIND_MOBILEOBJECT;
	obj->tileItem.data = obj;
	obj->soundLock = 0;
	obj->next = *mobObjList;
	obj->tileItem.drawFunc = (TileItemDrawFunc)BogusDraw;
	obj->updateFunc = UpdateMobileObject;
	*mobObjList = obj;
	return obj;
}

TMobileObject *AddMolotovFlame(int x, int y, int flags)
{
	TMobileObject *obj = AddMobileObject(&gMobObjList);
	obj->updateFunc = UpdateMolotovFlame;
	obj->tileItem.drawFunc = (TileItemDrawFunc)DrawFlame;
	obj->tileItem.w = 5;
	obj->tileItem.h = 5;
	obj->kind = MOBOBJ_FIREBALL;
	obj->range = (FLAME_RANGE + rand() % 8) * 4;
	obj->flags = flags;
	obj->power = 2;
	obj->x = x;
	obj->y = y;
	obj->dx = 16 * (rand() % 32) - 256;
	obj->dy = 12 * (rand() % 32) - 192;
	obj->dz = 4 + rand() % 4;
	return obj;
}

void Fire(int x, int y, int flags)
{
	int i;

	flags |= FLAGS_HURTALWAYS;
	for (i = 0; i < 16; i++)
	{
		AddMolotovFlame(x, y, flags);
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

	HitItem(obj, pos.x, pos.y, obj->z ? SPECIAL_CONFUSE : SPECIAL_POISON);

	if (!HitWall(pos.x >> 8, pos.y >> 8))
	{
		obj->x = pos.x;
		obj->y = pos.y;
		MoveTileItem(&obj->tileItem, pos.x >> 8, pos.y >> 8);
		return 1;
	} else
		return 1;
}

void AddGasCloud(int x, int y, int angle, int speed, int range, int flags,
		 int special)
{
	TMobileObject *obj = AddMobileObject(&gMobObjList);
	obj->updateFunc = UpdateGasCloud;
	obj->tileItem.drawFunc = (TileItemDrawFunc)DrawGasCloud;
	obj->tileItem.w = 10;
	obj->tileItem.h = 10;
	obj->kind = MOBOBJ_FIREBALL;
	obj->range = range;
	obj->flags = flags;
	obj->power = 0;
	obj->z = (special == SPECIAL_CONFUSE);
	GetVectorsForAngle(angle, &obj->dx, &obj->dy);
	obj->dx = (speed * obj->dx) / 256;
	obj->dy = (speed * obj->dy) / 256;
	obj->x = x + 6 * obj->dx;
	obj->y = y + 6 * obj->dy;
}

void Gas(int x, int y, int flags, int special)
{
	int i;

	flags |= FLAGS_HURTALWAYS;
	for (i = 0; i < 8; i++)
	{
		AddGasCloud(
			x, y,
			rand() & 255,
			(256 + rand()) & 255,
			(48 - (rand() % 8)) * 4 - 1,
			flags,
			special);
	}
	SoundPlayAt(&gSoundDevice, SND_BANG, Vec2iNew(x >> 8, y >> 8));
}

int UpdateGrenade(TMobileObject *obj, int ticks)
{
	int x, y;
	int i;

	MobileObjectUpdate(obj, ticks);
	if (obj->count > obj->range)
	{
		switch (obj->kind)
		{
		case MOBOBJ_GRENADE:
			AddExplosion(obj->x, obj->y, obj->flags);
			break;

		case MOBOBJ_FRAGGRENADE:
			Frag(obj->x, obj->y, obj->flags);
			break;

		case MOBOBJ_MOLOTOV:
			Fire(obj->x, obj->y, obj->flags);
			break;

		case MOBOBJ_GASBOMB:
			Gas(obj->x, obj->y, obj->flags, SPECIAL_POISON);
			break;

		case MOBOBJ_GASBOMB2:
			Gas(obj->x, obj->y, obj->flags, SPECIAL_CONFUSE);
			break;
		}
		return 0;
	}

	x = obj->x + obj->dx * ticks;
	y = obj->y + obj->dy * ticks;

	for (i = 0; i < ticks; i++)
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

	if (!HitWall(x >> 8, y >> 8)) {
		obj->x = x;
		obj->y = y;
	} else if (!HitWall(obj->x >> 8, y >> 8)) {
		obj->y = y;
		obj->dx = -obj->dx;
	} else if (!HitWall(x >> 8, obj->y >> 8)) {
		obj->x = x;
		obj->dy = -obj->dy;
	} else {
		obj->dx = -obj->dx;
		obj->dy = -obj->dy;
		return 1;
	}
	MoveTileItem(&obj->tileItem, obj->x >> 8, obj->y >> 8);
	return 1;
}

int UpdateMolotov(TMobileObject *obj, int ticks)
{
	int x, y;

	MobileObjectUpdate(obj, ticks);
	if (obj->count > obj->range)
	{
		Fire(obj->x, obj->y, obj->flags);
		return 0;
	}

	x = obj->x + obj->dx * ticks;
	y = obj->y + obj->dy * ticks;

	obj->z += obj->dz * ticks;
	if (obj->z <= 0)
	{
		Fire(obj->x, obj->y, obj->flags);
		return 0;
	}
	else
	{
		obj->dz -= ticks;
	}

	if (!HitWall(x >> 8, y >> 8)) {
		obj->x = x;
		obj->y = y;
	} else {
		Fire(obj->x, obj->y, obj->flags);
		return 0;
	}
	MoveTileItem(&obj->tileItem, obj->x >> 8, obj->y >> 8);
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
		&obj->tileItem, realPos, TILEITEM_CAN_BE_SHOT, COLLISIONTEAM_NONE);
	hasHit = DamageSomething(
		Vec2iNew(obj->dx, obj->dy),
		obj->power,
		obj->flags,
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
	int x, y;

	MobileObjectUpdate(obj, ticks);
	if (obj->count > obj->range)
		return 0;

	x = obj->x + obj->dx * ticks;
	y = obj->y + obj->dy * ticks;

	if (HitItem(obj, x, y, special)) {
		obj->count = 0;
		obj->range = 0;
		obj->tileItem.drawFunc = (TileItemDrawFunc)DrawSpark;
		obj->updateFunc = UpdateSpark;
		return 1;
	}
	if (!HitWall(x >> 8, y >> 8)) {
		obj->x = x;
		obj->y = y;
		MoveTileItem(&obj->tileItem, x >> 8, y >> 8);
		return 1;
	} else {
		obj->count = 0;
		obj->range = 0;
		obj->tileItem.drawFunc = (TileItemDrawFunc)DrawSpark;
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
	return InternalUpdateBullet(obj, 0, ticks);
}

int UpdateBrownBullet(TMobileObject *obj, int ticks)
{
	if (InternalUpdateBullet(obj, 0, ticks))
	{
		int i;
		for (i = 0; i < ticks; i++)
		{
			obj->dx += ((rand() % 3) - 1) * 128;
			obj->dy += ((rand() % 3) - 1) * 128;
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
		AddExplosion(obj->x, obj->y, obj->flags);
		return 0;
	}
	return 1;
}

int UpdateActiveMine(TMobileObject *obj, int ticks)
{
	int tx, ty, dx, dy;

	MobileObjectUpdate(obj, ticks);
	if (obj->count & 3)
	{
		return 1;
	}

	tx = (obj->x >> 8) / TILE_WIDTH;
	ty = (obj->y >> 8) / TILE_HEIGHT;

	if (tx == 0 || ty == 0 || tx >= XMAX - 1 || ty >= YMAX - 1)
	{
		return 0;
	}

	for (dy = -1; dy <= 1; dy++)
	{
		for (dx = -1; dx <= 1; dx++)
		{
			TTileItem *item = Map(tx + dx, ty + dy).things;
			while (item)
			{
				if (item->kind == KIND_CHARACTER)
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
				item = item->next;
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
	int x, y;

	MobileObjectUpdate(obj, ticks);
	if (obj->count > obj->range)
		return 0;

	if ((obj->count & 3) == 0)
		obj->state = rand();

	x = obj->x + obj->dx * ticks;
	y = obj->y + obj->dy * ticks;

	if (HitItem(obj, x, y, SPECIAL_FLAME)) {
		obj->count = obj->range;
		obj->x = x;
		obj->y = y;
		MoveTileItem(&obj->tileItem, x >> 8, y >> 8);
		return 1;
	}

	if (!HitWall(x >> 8, y >> 8)) {
		obj->x = x;
		obj->y = y;
		MoveTileItem(&obj->tileItem, x >> 8, y >> 8);
		return 1;
	} else
		return 0;
}

int UpdateExplosion(TMobileObject *obj, int ticks)
{
	int x, y;

	MobileObjectUpdate(obj, ticks);
	if (obj->count < 0)
		return 1;
	if (obj->count > obj->range)
		return 0;

	x = obj->x + obj->dx * ticks;
	y = obj->y + obj->dy * ticks;
	obj->z += obj->dz * ticks;
	obj->dz = MAX(0, obj->dz - ticks);

	HitItem(obj, x, y, SPECIAL_EXPLOSION);

	if (!HitWall(x >> 8, y >> 8)) {
		obj->x = x;
		obj->y = y;
		MoveTileItem(&obj->tileItem, x >> 8, y >> 8);
		return 1;
	}
	return 0;
}

void UpdateMobileObjects(TMobileObject **mobObjList, int ticks)
{
	TMobileObject *obj = *mobObjList;
	int do_remove = 0;

	while (obj)
	{
		if ((*(obj->updateFunc))(obj, ticks) == 0)
		{
			obj->range = 0;
			do_remove = 1;
		}
		obj = obj->next;
	}
	if (do_remove)
	{
		while (*mobObjList)
		{
			if ((*mobObjList)->range == 0)
			{
				obj = *mobObjList;
				*mobObjList = obj->next;
				RemoveTileItem(&obj->tileItem);
				CFREE(obj);
			}
			else
			{
				mobObjList = &((*mobObjList)->next);
			}
		}
	}
}

void AddGrenade(int x, int y, int angle, int flags, int kind)
{
	TMobileObject *obj = AddMobileObject(&gMobObjList);
	if (kind == MOBOBJ_MOLOTOV)
	{
		obj->updateFunc = UpdateMolotov;
		obj->tileItem.drawFunc = (TileItemDrawFunc)DrawMolotov;
	} else {
		obj->updateFunc = UpdateGrenade;
		obj->tileItem.drawFunc = (TileItemDrawFunc)DrawGrenade;
	}
	obj->kind = kind;
	obj->x = x;
	obj->y = y;
	obj->z = 0;
	GetVectorsForAngle(angle, &obj->dx, &obj->dy);
	obj->dx = (GRENADE_SPEED * obj->dx) / 256;
	obj->dy = (GRENADE_SPEED * obj->dy) / 256;
	obj->dz = 24;
	obj->range = 100;
	obj->flags = flags;
}

void AddBullet(int x, int y, int angle, int speed, int range, int power,
	       int flags)
{
	TMobileObject *obj = AddMobileObject(&gMobObjList);
	obj->updateFunc = UpdateBullet;
	obj->tileItem.drawFunc = (TileItemDrawFunc)DrawBullet;
	obj->kind = MOBOBJ_BULLET;
	obj->z = BULLET_Z;
	GetVectorsForAngle(angle, &obj->dx, &obj->dy);
	obj->dx = (speed * obj->dx) / 256;
	obj->dy = (speed * obj->dy) / 256;
	obj->x = x;
	obj->y = y;
	obj->range = range;
	obj->power = power;
	obj->flags = flags;
}

void AddRapidBullet(int x, int y, int angle, int speed, int range,
		    int power, int flags)
{
	TMobileObject *obj = AddMobileObject(&gMobObjList);
	obj->updateFunc = UpdateBullet;
	obj->tileItem.drawFunc = (TileItemDrawFunc)DrawBrownBullet;
	obj->kind = MOBOBJ_BULLET;
	obj->z = BULLET_Z;
	GetVectorsForAngle(angle, &obj->dx, &obj->dy);
	obj->dx = (speed * obj->dx) / 256;
	obj->dy = (speed * obj->dy) / 256;
	obj->x = x;
	obj->y = y;
	obj->range = range;
	obj->power = power;
	obj->flags = flags;
}

void AddSniperBullet(int x, int y, int direction, int flags)
{
	TMobileObject *obj = AddMobileObject(&gMobObjList);
	obj->updateFunc = UpdateBullet;
	obj->tileItem.drawFunc =  (TileItemDrawFunc)DrawBrightBolt;
	obj->kind = MOBOBJ_BULLET;
	obj->z = BULLET_Z;
	GetVectorsForAngle(dir2angle[direction], &obj->dx, &obj->dy);
	obj->dx = (SNIPER_SPEED * obj->dx) / 256;
	obj->dy = (SNIPER_SPEED * obj->dy) / 256;
	obj->x = x;
	obj->y = y;
	obj->state = direction;
	obj->range = SNIPER_RANGE;
	obj->power = SNIPER_POWER;
	obj->flags = flags;
}

void AddBrownBullet(int x, int y, int angle, int speed, int range,
		    int power, int flags)
{
	TMobileObject *obj = AddMobileObject(&gMobObjList);
	obj->updateFunc = UpdateBrownBullet;
	obj->tileItem.drawFunc = (TileItemDrawFunc)DrawBrownBullet;
	obj->kind = MOBOBJ_BULLET;
	obj->z = BULLET_Z;
	GetVectorsForAngle(angle, &obj->dx, &obj->dy);
	obj->dx = (speed * obj->dx) / 256;
	obj->dy = (speed * obj->dy) / 256;
	obj->x = x;
	obj->y = y;
	obj->range = range;
	obj->power = power;
	obj->flags = flags;
}

void AddFlame(int x, int y, int angle, int flags)
{
	TMobileObject *obj = AddMobileObject(&gMobObjList);
	obj->updateFunc = UpdateFlame;
	obj->tileItem.drawFunc = (TileItemDrawFunc)DrawFlame;
	obj->tileItem.w = 5;
	obj->tileItem.h = 5;
	obj->kind = MOBOBJ_BULLET;
	obj->z = BULLET_Z;
	GetVectorsForAngle(angle, &obj->dx, &obj->dy);
	obj->x = x + 4 * obj->dx;
	obj->y = y + 7 * obj->dy;
	obj->dx = (FLAME_SPEED * obj->dx) / 256;
	obj->dy = (FLAME_SPEED * obj->dy) / 256;
	obj->range = FLAME_RANGE;
	obj->power = FLAME_POWER;
	obj->flags = flags;
}

void AddLaserBolt(int x, int y, int direction, int flags)
{
	TMobileObject *obj = AddMobileObject(&gMobObjList);
	obj->updateFunc = UpdateBullet;
	obj->tileItem.drawFunc = (TileItemDrawFunc)DrawLaserBolt;
	obj->tileItem.w = 2;
	obj->tileItem.h = 2;
	obj->kind = MOBOBJ_BULLET;
	obj->z = BULLET_Z;
	GetVectorsForAngle(dir2angle[direction], &obj->dx, &obj->dy);
	obj->dx = (LASER_SPEED * obj->dx) / 256;
	obj->dy = (LASER_SPEED * obj->dy) / 256;
	obj->x = x;
	obj->y = y;
	obj->range = LASER_RANGE;
	obj->state = direction;
	obj->flags = flags;
	obj->power = LASER_POWER;
}

void AddPetrifierBullet(int x, int y, int angle, int speed, int range,
			int flags)
{
	TMobileObject *obj = AddMobileObject(&gMobObjList);
	obj->updateFunc = UpdatePetrifierBullet;
	obj->tileItem.drawFunc = (TileItemDrawFunc)DrawPetrifierBullet;
	obj->tileItem.w = 5;
	obj->tileItem.h = 5;
	obj->kind = MOBOBJ_BULLET;
	obj->z = BULLET_Z;
	GetVectorsForAngle(angle, &obj->dx, &obj->dy);
	obj->dx = (speed * obj->dx) / 256;
	obj->dy = (speed * obj->dy) / 256;
	obj->x = x + 4 * obj->dx;
	obj->y = y + 7 * obj->dy;
	obj->range = range;
	obj->flags = flags;
	obj->power = 0;
}

void AddHeatseeker(int x, int y, int angle, int speed, int range,
		   int power, int flags)
{
	TMobileObject *obj = AddMobileObject(&gMobObjList);
	obj->updateFunc = UpdateSeeker;
	obj->tileItem.drawFunc = (TileItemDrawFunc)DrawSeeker;
	obj->tileItem.w = 3;
	obj->tileItem.h = 3;
	obj->kind = MOBOBJ_BULLET;
	obj->z = BULLET_Z;
	GetVectorsForAngle(angle, &obj->dx, &obj->dy);
	obj->dx = (speed * obj->dx) / 256;
	obj->dy = (speed * obj->dy) / 256;
	obj->dz = speed;
	obj->x = x;
	obj->y = y;
	obj->range = range;
	obj->flags = flags;
	obj->power = power;
}

void AddProximityMine(int x, int y, int flags)
{
	TMobileObject *obj = AddMobileObject(&gMobObjList);
	obj->updateFunc = UpdateDroppedMine;
	obj->tileItem.drawFunc = (TileItemDrawFunc)DrawMine;
	obj->kind = MOBOBJ_BULLET;
	obj->x = x;
	obj->y = y;
	obj->range = 140;
	obj->flags = flags;
	MoveTileItem(&obj->tileItem, obj->x >> 8, obj->y >> 8);
}

void AddDynamite(int x, int y, int flags)
{
	TMobileObject *obj = AddMobileObject(&gMobObjList);
	obj->updateFunc = UpdateTriggeredMine;
	obj->tileItem.drawFunc = (TileItemDrawFunc)DrawDynamite;
	obj->kind = MOBOBJ_BULLET;
	obj->x = x;
	obj->y = y;
	obj->range = 210;
	obj->flags = flags;
	MoveTileItem(&obj->tileItem, obj->x >> 8, obj->y >> 8);
}

TMobileObject *AddFireBall(int flags)
{
	TMobileObject *obj = AddMobileObject(&gMobObjList);
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

void AddExplosion(int x, int y, int flags)
{
	TMobileObject *obj;
	int i;

	GameEvent shake;
	shake.Type = GAME_EVENT_SCREEN_SHAKE;
	shake.u.ShakeAmount = 15;
	GameEventsEnqueue(&gGameEvents, shake);

	flags |= FLAGS_HURTALWAYS;
	for (i = 0; i < 8; i++) {
		obj = AddFireBall(flags);
		GetVectorsForAngle(i * 32, &obj->dx, &obj->dy);
		obj->x = x + 2 * obj->dx;
		obj->y = y + 2 * obj->dy;
		obj->dz = 0;
	}
	for (i = 0; i < 8; i++) {
		obj = AddFireBall(flags);
		GetVectorsForAngle(i * 32 + 16, &obj->dx, &obj->dy);
		obj->x = x + obj->dx;
		obj->y = y + obj->dy;
		obj->dx *= 3;
		obj->dy *= 3;
		obj->dx /= 4;
		obj->dy /= 4;
		obj->dz = 8;
		obj->count = -8;
	}
	for (i = 0; i < 8; i++) {
		obj = AddFireBall(flags);
		obj->x = x;
		obj->y = y;
		obj->z = 0;
		GetVectorsForAngle(i * 32, &obj->dx, &obj->dy);
		obj->dx /= 2;
		obj->dy /= 2;
		obj->dz = 11;
		obj->count = -16;
	}

	SoundPlayAt(&gSoundDevice, SND_EXPLOSION, Vec2iNew(x >> 8, y >> 8));
}

void KillAllMobileObjects(TMobileObject **mobObjList)
{
	while (*mobObjList)
	{
		TMobileObject *o = *mobObjList;
		*mobObjList = (*mobObjList)->next;
		RemoveTileItem(&o->tileItem);
		CFREE(o);
	}
}

void InternalAddObject(
	int x, int y, int w, int h,
	const TOffsetPic * pic, const TOffsetPic * wreckedPic,
	int structure, int idx, int objFlags, int tileFlags)
{
	TObject *o;
	CCALLOC(o, sizeof(TObject));
	o->pic = pic;
	o->wreckedPic = wreckedPic;
	o->objectIndex = idx;
	o->structure = structure;
	o->flags = objFlags;
	o->tileItem.flags = tileFlags;
	o->tileItem.kind = KIND_OBJECT;
	o->tileItem.data = o;
	o->tileItem.drawFunc = (TileItemDrawFunc)DrawObject;
	o->tileItem.w = w;
	o->tileItem.h = h;
	o->tileItem.actor = NULL;
	MoveTileItem(&o->tileItem, x >> 8, y >> 8);
	o->next = objList;
	objList = o;
}

void AddObject(
	int x, int y, int w, int h, const TOffsetPic * pic, int idx, int tileFlags)
{
	InternalAddObject(x, y, w, h, pic, NULL, 0, idx, 0, tileFlags);
}

void AddDestructibleObject(int x, int y, int w, int h,
			   const TOffsetPic * pic,
			   const TOffsetPic * wreckedPic,
			   int structure, int objFlags, int tileFlags)
{
	InternalAddObject(x, y, w, h, pic, wreckedPic, structure, 0,
			  objFlags, tileFlags);
}

void RemoveObject(TObject * obj)
{
	TObject **h = &objList;

	while (*h && *h != obj)
		h = &((*h)->next);
	if (*h) {
		*h = obj->next;
		RemoveTileItem(&obj->tileItem);
		CFREE(obj);
	}
}

void KillAllObjects(void)
{
	TObject *o;

	while (objList) {
		o = objList;
		objList = objList->next;
		RemoveTileItem(&o->tileItem);
		CFREE(o);
	}
}
