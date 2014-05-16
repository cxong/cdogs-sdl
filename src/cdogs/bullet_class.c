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
#include "bullet_class.h"

#include <math.h>

#include "ai_utils.h"
#include "collision.h"
#include "drawtools.h"
#include "game_events.h"
#include "objs.h"
#include "screen_shake.h"

BulletClass gBulletClasses[BULLET_COUNT];


// Draw functions

static void DrawBullet(Vec2i pos, TileItemDrawFuncData *data)
{
	const TMobileObject *obj = CArrayGet(&gMobObjs, data->MobObjId);
	CASSERT(obj->isInUse, "Cannot draw non-existent bullet");
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
	const TMobileObject *obj = CArrayGet(&gMobObjs, data->MobObjId);
	CASSERT(obj->isInUse, "Cannot draw non-existent mobobj");
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

Pic *GetFlame(int id)
{
	const TMobileObject *obj = CArrayGet(&gMobObjs, id);
	const TOffsetPic *pic = &cFlamePics[obj->state & 3];
	Pic *p = PicManagerGetFromOld(&gPicManager, pic->picIndex);
	p->offset.x = pic->dx;
	p->offset.y = pic->dy - obj->z;
	return p;
}

static void DrawBeam(Vec2i pos, TileItemDrawFuncData *data)
{
	const TMobileObject *obj = CArrayGet(&gMobObjs, data->MobObjId);
	CASSERT(obj->isInUse, "Cannot draw non-existent mobobj");
	const TOffsetPic *pic = &cBeamPics[data->u.Beam][obj->state];
	pos = Vec2iAdd(pos, Vec2iNew(pic->dx, pic->dy - obj->z));
	Blit(
		&gGraphicsDevice,
		PicManagerGetFromOld(&gPicManager, pic->picIndex),
		pos);
}

static void DrawGrenade(Vec2i pos, TileItemDrawFuncData *data)
{
	const TMobileObject *obj = CArrayGet(&gMobObjs, data->MobObjId);
	CASSERT(obj->isInUse, "Cannot draw non-existent mobobj");
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

void DrawGasCloud(Vec2i pos, TileItemDrawFuncData *data)
{
	const TMobileObject *obj = CArrayGet(&gMobObjs, data->MobObjId);
	CASSERT(obj->isInUse, "Cannot draw non-existent mobobj");
	const TOffsetPic *pic = &cFireBallPics[8 + (obj->state & 3)];
	DrawBTPic(
		pos.x + pic->dx, pos.y + pic->dy - obj->z,
		PicManagerGetOldPic(&gPicManager, pic->picIndex),
		&data->u.Tint);
}


void AddExplosion(Vec2i pos, int flags, int player)
{
	int i;
	flags |= FLAGS_HURTALWAYS;
	for (i = 0; i < 8; i++)
	{
		const Vec2i vel = GetFullVectorsForRadians(i * 0.25 * PI);
		const Vec2i fireballPos = Vec2iAdd(pos, Vec2iScale(vel, 2));
		GameEventAddFireball(fireballPos, flags, player, vel, 0, 0);
	}
	for (i = 0; i < 8; i++)
	{
		const Vec2i vel =
			Vec2iScaleDiv(Vec2iScale(
			GetFullVectorsForRadians((i * 0.25 + 0.125) * PI), 3), 4);
		const Vec2i fireballPos = Vec2iAdd(pos, vel);
		GameEventAddFireball(fireballPos, flags, player, vel, 8, -8);
	}
	for (i = 0; i < 8; i++)
	{
		const Vec2i vel =
			Vec2iScaleDiv(GetFullVectorsForRadians(i * 0.25 * PI), 2);
		GameEventAddFireball(pos, flags, player, vel, 11, -16);
	}
	GameEvent sound;
	sound.Type = GAME_EVENT_SOUND_AT;
	sound.u.SoundAt.Sound = SND_EXPLOSION;
	sound.u.SoundAt.Pos = Vec2iFull2Real(pos);
	GameEventsEnqueue(&gGameEvents, sound);
	GameEvent shake;
	shake.Type = GAME_EVENT_SCREEN_SHAKE;
	shake.u.ShakeAmount = SHAKE_SMALL_AMOUNT;
	GameEventsEnqueue(&gGameEvents, shake);
}


static void AddFrag(
	const Vec2i fullPos, int flags, const int playerIndex)
{
	flags |= FLAGS_HURTALWAYS;
	GameEvent e;
	e.Type = GAME_EVENT_ADD_BULLET;
	e.u.AddBullet.Bullet = BULLET_FRAG;
	e.u.AddBullet.MuzzlePos = fullPos;
	e.u.AddBullet.MuzzleHeight = 0;
	e.u.AddBullet.Direction = DIRECTION_UP;	// TODO: accurate direction
	e.u.AddBullet.Flags = flags;
	e.u.AddBullet.PlayerIndex = playerIndex;
	for (int i = 0; i < 16; i++)
	{
		e.u.AddBullet.Angle = i / 16.0 * 2 * PI;
		GameEventsEnqueue(&gGameEvents, e);
	}
	GameEvent sound;
	sound.Type = GAME_EVENT_SOUND_AT;
	sound.u.SoundAt.Sound = SND_BANG;
	sound.u.SoundAt.Pos = Vec2iFull2Real(fullPos);
	GameEventsEnqueue(&gGameEvents, sound);
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

int UpdateMolotovFlame(struct MobileObject *obj, int ticks)
{
	MobileObjectUpdate(obj, ticks);
	if (obj->count > obj->range)
	{
		return 0;
	}

	if ((obj->count & 3) == 0)
		obj->state = rand();

	for (int i = 0; i < ticks; i++)
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
	Vec2i pos = UpdateAndGetCloudPosition(obj, ticks);

	HitItem(obj, pos, SPECIAL_FLAME);

	if (!ShootWall(pos.x >> 8, pos.y >> 8))
	{
		obj->x = pos.x;
		obj->y = pos.y;
		MapMoveTileItem(&gMap, &obj->tileItem, Vec2iFull2Real(pos));
		return 1;
	} else
		return 1;
}

void AddFireExplosion(Vec2i pos, int flags, int player)
{
	flags |= FLAGS_HURTALWAYS;
	for (int i = 0; i < 16; i++)
	{
		GameEventAddMolotovFlame(pos, flags, player);
	}
	GameEvent sound;
	sound.Type = GAME_EVENT_SOUND_AT;
	sound.u.SoundAt.Sound = SND_BANG;
	sound.u.SoundAt.Pos = Vec2iFull2Real(pos);
	GameEventsEnqueue(&gGameEvents, sound);
}
void AddGasExplosion(
	Vec2i pos, int flags, special_damage_e special, int player)
{
	flags |= FLAGS_HURTALWAYS;
	for (int i = 0; i < 8; i++)
	{
		GameEventAddGasCloud(pos, flags, player, special);
	}
	GameEvent sound;
	sound.Type = GAME_EVENT_SOUND_AT;
	sound.u.SoundAt.Sound = SND_BANG;
	sound.u.SoundAt.Pos = Vec2iFull2Real(pos);
	GameEventsEnqueue(&gGameEvents, sound);
}

int UpdateGasCloud(struct MobileObject *obj, int ticks)
{
	MobileObjectUpdate(obj, ticks);
	if (obj->count > obj->range)
	{
		return 0;
	}

	if ((obj->count & 3) == 0)
	{
		obj->state = rand();
	}

	Vec2i pos = UpdateAndGetCloudPosition(obj, ticks);

	HitItem(obj, pos, obj->special);

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
	Vec2i vel = Vec2iFull2Real(Vec2iScale(
		GetFullVectorsForRadians(radians), speed));
	pos = Vec2iAdd(pos, Vec2iScale(vel, 6));
	TMobileObject *obj = CArrayGet(&gMobObjs, MobObjAdd(pos, player));
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
	obj->vel = vel;
	obj->z = z;
}

int UpdateGrenade(TMobileObject *obj, int ticks)
{
	MobileObjectUpdate(obj, ticks);
	if (obj->count > obj->range)
	{
		const Vec2i fullPos = Vec2iNew(obj->x, obj->y);
		switch (obj->kind)
		{
		case MOBOBJ_GRENADE:
			AddExplosion(fullPos, obj->flags, obj->player);
			break;

		case MOBOBJ_FRAGGRENADE:
			AddFrag(fullPos, obj->flags, obj->player);
			break;

		case MOBOBJ_MOLOTOV:
			AddFireExplosion(fullPos, obj->flags, obj->player);
			break;

		case MOBOBJ_GASBOMB:
			AddGasExplosion(fullPos, obj->flags, SPECIAL_POISON, obj->player);
			break;

		case MOBOBJ_GASBOMB2:
			AddGasExplosion(fullPos, obj->flags, SPECIAL_CONFUSE, obj->player);
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
	Vec2i pos = Vec2iNew(obj->x, obj->y);
	if (obj->count > obj->range)
	{
		AddFireExplosion(pos, obj->flags, obj->player);
		return false;
	}

	int x = obj->x + obj->vel.x * ticks;
	int y = obj->y + obj->vel.y * ticks;

	obj->z += obj->dz * ticks;
	if (obj->z <= 0)
	{
		AddFireExplosion(pos, obj->flags, obj->player);
		return false;
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
		AddFireExplosion(pos, obj->flags, obj->player);
		return false;
	}
	MapMoveTileItem(&gMap, &obj->tileItem, Vec2iFull2Real(pos));
	return 1;
}

int InternalUpdateBullet(TMobileObject *obj, int special, int ticks)
{
	MobileObjectUpdate(obj, ticks);
	if (obj->count > obj->range)
	{
		return 0;
	}

	Vec2i pos =
		Vec2iScale(Vec2iAdd(Vec2iNew(obj->x, obj->y), obj->vel), ticks);

	if (HitItem(obj, pos, special))
	{
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
	if (!ShootWall(pos.x >> 8, pos.y >> 8))
	{
		obj->x = pos.x;
		obj->y = pos.y;
		MapMoveTileItem(&gMap, &obj->tileItem, Vec2iFull2Real(pos));
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
	return InternalUpdateBullet(obj, obj->special, ticks);
}

int UpdateFlame(TMobileObject *obj, int ticks)
{
	MobileObjectUpdate(obj, ticks);
	if (obj->count > obj->range)
	{
		return 0;
	}

	if ((obj->count & 3) == 0)
	{
		obj->state = rand();
	}

	Vec2i pos =
		Vec2iScale(Vec2iAdd(Vec2iNew(obj->x, obj->y), obj->vel), ticks);

	if (HitItem(obj, pos, SPECIAL_FLAME))
	{
		obj->count = obj->range;
		obj->x = pos.x;
		obj->y = pos.y;
		MapMoveTileItem(
			&gMap, &obj->tileItem, Vec2iFull2Real(pos));
		return 1;
	}

	if (!ShootWall(pos.x >> 8, pos.y >> 8))
	{
		obj->x = pos.x;
		obj->y = pos.y;
		MapMoveTileItem(
			&gMap, &obj->tileItem, Vec2iFull2Real(pos));
		return 1;
	}
	else
		return 0;
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
		AddExplosion(Vec2iNew(obj->x, obj->y), obj->flags, obj->player);
		return false;
	}
	return true;
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
		b->Special = SPECIAL_NONE;
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


	b = &gBulletClasses[BULLET_GAS];
	b->UpdateFunc = UpdateGasCloud;
	b->DrawFunc = (TileItemDrawFunc)DrawGasCloud;
	b->Speed = 384;
	b->Range = 35;
	b->Power = 0;
	b->Special = SPECIAL_POISON;

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
	b->UpdateFunc = UpdateBullet;
	b->DrawFunc = (TileItemDrawFunc)DrawBullet;
	b->DrawData.u.Bullet.Ofspic = OFSPIC_MOLOTOV;
	b->DrawData.u.Bullet.UseMask = false;
	b->DrawData.u.Bullet.Tint = tintDarker;
	b->Speed = 768;
	b->Range = 45;
	b->Power = 0;
	b->Size = 5;
	b->Special = SPECIAL_PETRIFY;

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
	TMobileObject *obj, int z, BulletType type, int flags)
{
	BulletClass *b = &gBulletClasses[type];
	obj->bulletClass = *b;
	obj->updateFunc = b->UpdateFunc;
	obj->tileItem.getPicFunc = b->GetPicFunc;
	obj->tileItem.getActorPicsFunc = NULL;
	obj->tileItem.drawFunc = b->DrawFunc;
	obj->tileItem.drawData.u = b->DrawData.u;
	obj->kind = MOBOBJ_BULLET;
	obj->z = z;
	obj->flags = flags;
	obj->vel = Vec2iFull2Real(Vec2iScale(obj->vel, b->Speed));
	obj->range = b->Range;
	obj->power = b->Power;
	obj->special = b->Special;
	obj->tileItem.w = b->Size;
	obj->tileItem.h = b->Size;
}

void AddGrenade(
	Vec2i pos, int z, double radians, BulletType type, int flags, int player)
{
	TMobileObject *obj = CArrayGet(&gMobObjs, MobObjAdd(pos, player));
	obj->vel = GetFullVectorsForRadians(radians);
	obj->dz = 24;
	SetBulletProps(obj, z, type, flags);
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

void AddBullet(
	Vec2i pos, int z, double radians, BulletType type, int flags, int player)
{
	TMobileObject *obj = CArrayGet(&gMobObjs, MobObjAdd(pos, player));
	obj->vel = GetFullVectorsForRadians(radians);
	SetBulletProps(obj, z, type, flags);
}

void AddBulletDirectional(
	Vec2i pos, int z, direction_e dir, BulletType type, int flags, int player)
{
	TMobileObject *obj = CArrayGet(&gMobObjs, MobObjAdd(pos, player));
	obj->vel = GetFullVectorsForRadians(dir2radians[dir]);
	obj->state = dir;
	SetBulletProps(obj, z, type, flags);
}

void AddBulletBig(
	Vec2i pos, int z, double radians, BulletType type, int flags, int player)
{
	Vec2i vel = GetVectorsForRadians(radians);
	pos = Vec2iAdd(pos, Vec2iNew(vel.x * 4, vel.y * 7));
	TMobileObject *obj = CArrayGet(&gMobObjs, MobObjAdd(pos, player));
	obj->vel = GetFullVectorsForRadians(radians);
	SetBulletProps(obj, z, type, flags);
}

void BulletAdd(
	const BulletType bullet,
	const Vec2i muzzlePos, const int muzzleHeight,
	const double angle, const direction_e d,
	const int flags, const int playerIndex)
{
	switch (bullet)
	{
	case BULLET_MG:			// fallthrough
	case BULLET_SHOTGUN:	// fallthrough
	case BULLET_BROWN:		// fallthrough
	case BULLET_PROXMINE:	// fallthrough
	case BULLET_DYNAMITE:	// fallthrough
	case BULLET_RAPID:		// fallthrough
	case BULLET_HEATSEEKER:	// fallthrough
	case BULLET_FRAG:
		AddBullet(muzzlePos, muzzleHeight, angle, bullet, flags, playerIndex);
		break;

	case BULLET_GRENADE:		// fallthrough
	case BULLET_SHRAPNELBOMB:	// fallthrough
	case BULLET_MOLOTOV:		// fallthrough
	case BULLET_GASBOMB:		// fallthrough
	case BULLET_CONFUSEBOMB:
		AddGrenade(muzzlePos, muzzleHeight, angle, bullet, flags, playerIndex);
		break;

	case BULLET_FLAME:	// fallthrough
	case BULLET_PETRIFIER:
		AddBulletBig(muzzlePos, muzzleHeight, angle, bullet, flags, playerIndex);
		break;

	case BULLET_LASER:	// fallthrough
	case BULLET_SNIPER:
		AddBulletDirectional(
			muzzlePos, muzzleHeight, d, bullet, flags, playerIndex);
		break;

	case BULLET_GAS:
		AddGasCloud(
			muzzlePos, muzzleHeight, angle, 384, 35,
			flags, SPECIAL_POISON, playerIndex);
		break;

	default:
		// unknown bullet?
		CASSERT(false, "Unknown bullet");
		break;
	}
}