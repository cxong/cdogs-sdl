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


BulletType StrBulletType(const char *s)
{
	if (s == NULL || strlen(s) == 0)
	{
		return BULLET_NONE;
	}
	for (int i = 0; i < BULLET_COUNT; i++)
	{
		if (gBulletClasses[i].Name &&
			strcmp(gBulletClasses[i].Name, s) == 0)
		{
			return gBulletClasses[i].Type;
		}
	}
	CASSERT(false, "cannot find bullet type");
	return BULLET_NONE;
}

// Draw functions

static void DrawBullet(Vec2i pos, TileItemDrawFuncData *data)
{
	const TMobileObject *obj = CArrayGet(&gMobObjs, data->MobObjId);
	CASSERT(obj->isInUse, "Cannot draw non-existent bullet");
	if (data->u.Bullet.Pic)
	{
		pos = Vec2iMinus(pos, Vec2iScaleDiv(data->u.Bullet.Pic->size, 2));
		BlitMasked(
			&gGraphicsDevice,
			data->u.Bullet.Pic,
			pos, data->u.Bullet.Mask, true);
	}
	else
	{
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
				&gGraphicsDevice,
				PicManagerGetFromOld(&gPicManager, pic->picIndex),
				pos,
				&data->u.Bullet.Tint);
		}
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

static const Pic *GetFlame(int id, Vec2i *offset)
{
	TMobileObject *obj = CArrayGet(&gMobObjs, id);
	CASSERT(obj->isInUse, "Cannot draw non-existent mobobj");
	if ((obj->count & 3) == 0)
	{
		obj->state.frame = rand();
	}
	const TOffsetPic *pic = &cFlamePics[obj->state.frame & 3];
	offset->x = pic->dx;
	offset->y = pic->dy - obj->z;
	return PicManagerGetFromOld(&gPicManager, pic->picIndex);
}

static const Pic *GetBeam(int id, Vec2i *offset)
{
	const TMobileObject *obj = CArrayGet(&gMobObjs, id);
	CASSERT(obj->isInUse, "Cannot draw non-existent mobobj");
	// Calculate direction based on velocity
	const direction_e dir = RadiansToDirection(Vec2iToRadians(obj->vel));
	const Pic *p;
	if (obj->bulletClass->Beam.Sprites)
	{
		p = CArrayGet(&obj->bulletClass->Beam.Sprites->pics, dir);
		offset->x = -p->size.x / 2;
		offset->y = -p->size.y / 2 - obj->z;
	}
	else
	{
		const TOffsetPic *pic = &cBeamPics[obj->bulletClass->Beam.Beam][dir];
		p = PicManagerGetFromOld(&gPicManager, pic->picIndex);
		offset->x = pic->dx;
		offset->y = pic->dy - obj->z;
	}
	return p;
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
	const TOffsetPic *pic = &cFireBallPics[8 + (obj->state.frame & 3)];
	DrawBTPic(
		&gGraphicsDevice,
		PicManagerGetFromOld(&gPicManager, pic->picIndex),
		Vec2iNew(pos.x + pic->dx, pos.y + pic->dy - obj->z),
		&data->u.Tint);
}


void AddExplosion(Vec2i pos, int flags, int player)
{
	int i;
	flags |= FLAGS_HURTALWAYS;
	for (i = 0; i < 8; i++)
	{
		GameEventAddFireball(
			&gBulletClasses[BULLET_FIREBALL1],
			pos, flags, player, 0, 0, i * 0.25 * PI);
	}
	for (i = 0; i < 8; i++)
	{
		GameEventAddFireball(
			&gBulletClasses[BULLET_FIREBALL2],
			pos, flags, player, 8, -8, (i * 0.25 + 0.125) * PI);
	}
	for (i = 0; i < 8; i++)
	{
		GameEventAddFireball(
			&gBulletClasses[BULLET_FIREBALL3],
			pos, flags, player, 11, -16, i * 0.25 * PI);
	}
	GameEvent sound;
	sound.Type = GAME_EVENT_SOUND_AT;
	sound.u.SoundAt.Sound = StrSound("explosion");
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
	sound.u.SoundAt.Sound = StrSound("bang");
	sound.u.SoundAt.Pos = Vec2iFull2Real(fullPos);
	GameEventsEnqueue(&gGameEvents, sound);
}


static void SetBulletProps(
	TMobileObject *obj, int z, BulletType type, int flags)
{
	const BulletClass *b = &gBulletClasses[type];
	obj->bulletClass = b;
	obj->updateFunc = b->UpdateFunc;
	obj->tileItem.getPicFunc = b->GetPicFunc;
	obj->tileItem.getActorPicsFunc = NULL;
	obj->tileItem.drawFunc = b->DrawFunc;
	obj->tileItem.drawData.u = b->DrawData.u;
	obj->kind = MOBOBJ_BULLET;
	obj->z = z;
	obj->range = RAND_INT(b->RangeLow, b->RangeHigh);
	obj->flags = flags;
	obj->vel = Vec2iFull2Real(Vec2iScale(
		obj->vel, RAND_INT(b->SpeedLow, b->SpeedHigh)));
	if (b->SpeedScale)
	{
		obj->vel.y = obj->vel.y * TILE_HEIGHT / TILE_WIDTH;
	}
	obj->tileItem.w = b->Size.x;
	obj->tileItem.h = b->Size.y;
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
	sound.u.SoundAt.Sound = StrSound("bang");
	sound.u.SoundAt.Pos = Vec2iFull2Real(pos);
	GameEventsEnqueue(&gGameEvents, sound);
}
void AddGasExplosion(
	Vec2i pos, int flags, const BulletClass *class, int player)
{
	flags |= FLAGS_HURTALWAYS;
	for (int i = 0; i < 8; i++)
	{
		GameEventAddGasCloud(class, pos, flags, player);
	}
	GameEvent sound;
	sound.Type = GAME_EVENT_SOUND_AT;
	sound.u.SoundAt.Sound = StrSound("bang");
	sound.u.SoundAt.Pos = Vec2iFull2Real(pos);
	GameEventsEnqueue(&gGameEvents, sound);
}

void AddGasCloud(
	Vec2i pos, int z, double radians, int flags, int player)
{
	const BulletClass *b = &gBulletClasses[BULLET_GAS];
	Vec2i vel = Vec2iFull2Real(Vec2iScale(
		GetFullVectorsForRadians(radians),
		RAND_INT(b->SpeedLow, b->SpeedHigh)));
	if (b->SpeedScale)
	{
		vel.y = vel.y * TILE_HEIGHT / TILE_WIDTH;
	}
	pos = Vec2iAdd(pos, Vec2iScale(vel, 6));
	TMobileObject *obj = CArrayGet(&gMobObjs, MobObjAdd(pos, player));
	SetBulletProps(obj, z, BULLET_GAS, flags);
	obj->tileItem.drawData.u.Tint =
		b->Special == SPECIAL_CONFUSE ? tintPurple : tintPoison;
	obj->kind = MOBOBJ_FIREBALL;
	obj->vel = vel;
}
static void GrenadeExplode(const TMobileObject *obj)
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
		AddGasExplosion(
			fullPos, obj->flags,
			&gBulletClasses[BULLET_GAS_CLOUD_POISON], obj->player);
		break;

	case MOBOBJ_GASBOMB2:
		AddGasExplosion(
			fullPos, obj->flags,
			&gBulletClasses[BULLET_GAS_CLOUD_CONFUSE], obj->player);
		break;

	default:
		CASSERT(false, "Unknown grenade kind");
		break;
	}
}
static void Explode(const TMobileObject *obj)
{
	const Vec2i fullPos = Vec2iNew(obj->x, obj->y);
	AddExplosion(fullPos, obj->flags, obj->player);
}

static Vec2i SeekTowards(
	const Vec2i pos, const Vec2i vel, const double speedMin,
	const Vec2i targetPos, const int seekFactor)
{
	// Compensate for bullet's velocity
	const Vec2i targetVel = Vec2iMinus(Vec2iMinus(targetPos, pos), vel);
	// Don't seek if the coordinates are too big
	if (abs(targetVel.x) > 10000 || abs(targetVel.y) > 10000 ||
		Vec2iEqual(targetVel, Vec2iZero()))
	{
		return vel;
	}
	const double targetMag = sqrt(
		targetVel.x*targetVel.x + targetVel.y*targetVel.y);
	const double magnitude = MAX(speedMin,
		Vec2iEqual(vel, Vec2iZero()) ? speedMin : sqrt(vel.x*vel.x + vel.y*vel.y));
	const double combinedX =
		vel.x / magnitude * seekFactor + targetVel.x / targetMag;
	const double combinedY =
		vel.y / magnitude * seekFactor + targetVel.y / targetMag;
	return Vec2iNew(
		(int)round(combinedX * magnitude / (seekFactor + 1)),
		(int)round(combinedY * magnitude / (seekFactor + 1)));
}


bool UpdateBullet(TMobileObject *obj, const int ticks)
{
	MobileObjectUpdate(obj, ticks);
	if (obj->count < 0)
	{
		return true;
	}
	if (obj->range >= 0 && obj->count > obj->range)
	{
		if (obj->bulletClass->OutOfRangeFunc)
		{
			obj->bulletClass->OutOfRangeFunc(obj);
		}
		return false;
	}

	if (obj->bulletClass->RandomAnimation && !(obj->count & 3))
	{
		obj->state.frame = rand();
	}

	const Vec2i objPos = Vec2iNew(obj->x, obj->y);

	if (obj->bulletClass->SeekFactor > 0)
	{
		// Find the closest target to this bullet and steer towards it
		TActor *target = AIGetClosestEnemy(
			objPos, obj->flags, obj->player >= 0);
		if (target && !target->dead)
		{
			for (int i = 0; i < ticks; i++)
			{
				obj->vel = SeekTowards(
					objPos, obj->vel,
					obj->bulletClass->SpeedLow, target->Pos,
					obj->bulletClass->SeekFactor);
			}
		}
	}

	const Vec2i pos = Vec2iScale(Vec2iAdd(objPos, obj->vel), ticks);
	const bool hitItem = HitItem(obj, pos);
	const Vec2i realPos = Vec2iFull2Real(pos);

	// Falling (grenades)
	if (obj->bulletClass->Falling.Enabled)
	{
		switch (obj->bulletClass->Falling.Type)
		{
		case FALLING_TYPE_BOUNCE:
			{
				bool hasDropped = false;
				for (int i = 0; i < ticks; i++)
				{
					obj->z += obj->dz;
					if (obj->z <= 0)
					{
						if (!hasDropped && obj->bulletClass->Falling.DropFunc)
						{
							obj->bulletClass->Falling.DropFunc(obj);
						}
						hasDropped = true;
						if (obj->bulletClass->Falling.DestroyOnDrop)
						{
							return false;
						}
						GameEvent e;
						e.Type = GAME_EVENT_SOUND_AT;
						e.u.SoundAt.Sound = obj->bulletClass->WallHitSound;
						e.u.SoundAt.Pos = realPos;
						GameEventsEnqueue(&gGameEvents, e);
						obj->z = 0;
						obj->dz = -obj->dz / 2;
					}
					else
					{
						obj->dz--;
					}
				}
			}
			break;
		case FALLING_TYPE_DZ:
			obj->z += obj->dz * ticks;
			obj->dz = MAX(0, obj->dz - ticks);
			break;
		case FALLING_TYPE_Z:
			obj->z += obj->dz / 2;
			if (obj->z <= 0)
			{
				obj->z = 0;
			}
			else
			{
				obj->dz--;
			}
			break;
		default:
			CASSERT(false, "Unknown falling type");
			break;
		}
	}
	
	// Friction
	for (int i = 0; i < ticks; i++)
	{
		if (obj->vel.x > 0)
		{
			obj->vel.x -= obj->bulletClass->Friction.x;
		}
		else if (obj->vel.x < 0)
		{
			obj->vel.x += obj->bulletClass->Friction.x;
		}

		if (obj->vel.y > 0)
		{
			obj->vel.y -= obj->bulletClass->Friction.y;
		}
		else if (obj->vel.y < 0)
		{
			obj->vel.y += obj->bulletClass->Friction.y;
		}
	}

	const bool hitWall = ShootWall(realPos.x, realPos.y);
	if (hitWall)
	{
		GameEvent e;
		e.Type = GAME_EVENT_SOUND_AT;
		e.u.SoundAt.Sound = obj->bulletClass->WallHitSound;
		e.u.SoundAt.Pos = realPos;
		GameEventsEnqueue(&gGameEvents, e);
	}
	if ((hitWall && !obj->bulletClass->WallBounces) ||
		(hitItem && obj->bulletClass->HitsObjects))
	{
		if (obj->bulletClass->HitFunc)
		{
			obj->bulletClass->HitFunc(obj);
		}
		if (obj->bulletClass->Spark != NULL)
		{
			GameEvent e;
			e.Type = GAME_EVENT_ADD_PARTICLE;
			e.u.AddParticle.Class = obj->bulletClass->Spark;
			e.u.AddParticle.FullPos = pos;
			e.u.AddParticle.Z = obj->z;
			GameEventsEnqueue(&gGameEvents, e);
		}
		if (hitWall || !obj->bulletClass->Persists)
		{
			return false;
		}
	}
	if (hitWall)
	{
		// Bouncing
		Vec2i objRealPos = Vec2iFull2Real(objPos);
		if (!ShootWall(objRealPos.x, realPos.y))
		{
			obj->y = pos.y;
			obj->vel.x = -obj->vel.x;
		}
		else if (!ShootWall(realPos.x, objRealPos.y))
		{
			obj->x = pos.x;
			obj->vel.y = -obj->vel.y;
		}
		else
		{
			obj->vel.x = -obj->vel.x;
			obj->vel.y = -obj->vel.y;
			// Keep bouncing the bullet back if it's inside a wall
			objRealPos = Vec2iFull2Real(Vec2iNew(obj->x, obj->y));
			while (ShootWall(objRealPos.x, objRealPos.y) &&
				objRealPos.x >= 0 &&
				objRealPos.x < gMap.Size.x * TILE_WIDTH &&
				objRealPos.y >= 0 &&
				objRealPos.y < gMap.Size.y * TILE_HEIGHT)
			{
				obj->x += obj->vel.x;
				obj->y += obj->vel.y;
				objRealPos = Vec2iFull2Real(Vec2iNew(obj->x, obj->y));
			}
		}
	}
	else
	{
		obj->x = pos.x;
		obj->y = pos.y;
	}
	MapMoveTileItem(&gMap, &obj->tileItem, realPos);

	if (obj->bulletClass->Erratic)
	{
		for (int i = 0; i < ticks; i++)
		{
			obj->vel.x += ((rand() % 3) - 1) * 128;
			obj->vel.y += ((rand() % 3) - 1) * 128;
		}
	}

	// Proximity function, destroy
	// Only check proximity every now and then
	if (obj->bulletClass->ProximityFunc && !(obj->count & 3))
	{
		// Detonate the mine if there are characters in the tiles around it
		const Vec2i tv =
			Vec2iToTile(Vec2iFull2Real(Vec2iNew(obj->x, obj->y)));
		Vec2i dv;
		for (dv.y = -1; dv.y <= 1; dv.y++)
		{
			for (dv.x = -1; dv.x <= 1; dv.x++)
			{
				const Vec2i dtv = Vec2iAdd(tv, dv);
				if (!MapIsTileIn(&gMap, dtv))
				{
					continue;
				}
				if (TileHasCharacter(MapGetTile(&gMap, dtv)))
				{
					obj->bulletClass->ProximityFunc(obj);
					return false;
				}
			}
		}
	}

	return true;
}

static void AddActiveMine(const TMobileObject *obj)
{
	GameEvent e;
	e.Type = GAME_EVENT_ADD_BULLET;
	e.u.AddBullet.Bullet = BULLET_ACTIVEMINE;
	e.u.AddBullet.MuzzlePos = Vec2iNew(obj->x, obj->y);
	e.u.AddBullet.MuzzleHeight = obj->z;
	e.u.AddBullet.Angle = 0;
	e.u.AddBullet.Direction = DIRECTION_UP;
	e.u.AddBullet.Flags = obj->flags;
	e.u.AddBullet.PlayerIndex = obj->player;
	GameEventsEnqueue(&gGameEvents, e);
	GameEvent sound;
	sound.Type = GAME_EVENT_SOUND_AT;
	sound.u.SoundAt.Sound = StrSound("mine_arm");
	sound.u.SoundAt.Pos = Vec2iFull2Real(Vec2iNew(obj->x, obj->y));
	GameEventsEnqueue(&gGameEvents, sound);
}
static void AddTriggeredMine(const TMobileObject *obj)
{
	GameEvent e;
	e.Type = GAME_EVENT_ADD_BULLET;
	e.u.AddBullet.Bullet = BULLET_TRIGGEREDMINE;
	e.u.AddBullet.MuzzlePos = Vec2iNew(obj->x, obj->y);
	e.u.AddBullet.MuzzleHeight = obj->z;
	e.u.AddBullet.Angle = 0;
	e.u.AddBullet.Direction = DIRECTION_UP;
	e.u.AddBullet.Flags = obj->flags;
	e.u.AddBullet.PlayerIndex = obj->player;
	GameEventsEnqueue(&gGameEvents, e);
	GameEvent sound;
	sound.Type = GAME_EVENT_SOUND_AT;
	sound.u.SoundAt.Sound = StrSound("mine_trigger");
	sound.u.SoundAt.Pos = Vec2iFull2Real(Vec2iNew(obj->x, obj->y));
	GameEventsEnqueue(&gGameEvents, sound);
}


void BulletInitialize(void)
{
	// Defaults
	BulletClass *b;
	for (int i = 0; i < BULLET_COUNT; i++)
	{
		b = &gBulletClasses[i];
		b->Type = (BulletType)i;
		b->UpdateFunc = UpdateBullet;
		b->GetPicFunc = NULL;
		b->DrawFunc = NULL;
		memset(&b->DrawData, 0, sizeof b->DrawData);
		b->Beam.Sprites = NULL;
		b->SpeedScale = false;
		b->Friction = Vec2iZero();
		b->Size = Vec2iZero();
		b->Special = SPECIAL_NONE;
		b->Persists = false;
		b->Spark = ParticleClassGet(&gParticleClasses, "spark");
		b->WallHitSound = StrSound("ricochet");
		b->WallBounces = false;
		b->HitsObjects = true;
		b->Falling.Enabled = false;
		b->Falling.Type = FALLING_TYPE_BOUNCE;
		b->Falling.DestroyOnDrop = false;
		b->Falling.DropFunc = NULL;
		b->OutOfRangeFunc = NULL;
		b->HitFunc = NULL;
		b->RandomAnimation = false;
		b->SeekFactor = -1;
		b->Erratic = false;
	}

	b = &gBulletClasses[BULLET_MG];
	b->Name = "mg";
	b->DrawFunc = (TileItemDrawFunc)DrawBullet;
	b->DrawData.u.Bullet.Ofspic = OFSPIC_BULLET;
	b->DrawData.u.Bullet.UseMask = true;
	b->DrawData.u.Bullet.Mask = colorWhite;
	b->SpeedLow = b->SpeedHigh = 768;
	b->RangeLow = b->RangeHigh = 60;
	b->Power = 10;

	b = &gBulletClasses[BULLET_SHOTGUN];
	b->Name = "shotgun";
	b->DrawFunc = (TileItemDrawFunc)DrawBullet;
	b->DrawData.u.Bullet.Ofspic = OFSPIC_BULLET;
	b->DrawData.u.Bullet.UseMask = true;
	b->DrawData.u.Bullet.Mask = colorWhite;
	b->SpeedLow = b->SpeedHigh = 640;
	b->RangeLow = b->RangeHigh = 50;
	b->Power = 15;

	b = &gBulletClasses[BULLET_FLAME];
	b->Name = "flame";
	b->GetPicFunc = GetFlame;
	b->SpeedLow = b->SpeedHigh = 384;
	b->RangeLow = b->RangeHigh = 30;
	b->Power = 12;
	b->Size = Vec2iNew(5, 5);
	b->Special = SPECIAL_FLAME;
	b->Spark = NULL;
	b->WallHitSound = StrSound("hit_fire");
	b->WallBounces = true;
	b->RandomAnimation = true;

	b = &gBulletClasses[BULLET_LASER];
	b->Name = "laser";
	b->GetPicFunc = GetBeam;
	b->Beam.Beam = BEAM_PIC_BEAM;
	b->SpeedLow = b->SpeedHigh = 1024;
	b->RangeLow = b->RangeHigh = 90;
	b->Power = 20;
	b->Size = Vec2iNew(2, 2);

	b = &gBulletClasses[BULLET_SNIPER];
	b->Name = "sniper";
	b->GetPicFunc = GetBeam;
	b->Beam.Beam = BEAM_PIC_BRIGHT;
	b->SpeedLow = b->SpeedHigh = 1024;
	b->RangeLow = b->RangeHigh = 90;
	b->Power = 50;

	b = &gBulletClasses[BULLET_FRAG];
	b->Name = "frag";
	b->DrawFunc = (TileItemDrawFunc)DrawBullet;
	b->DrawData.u.Bullet.Ofspic = OFSPIC_BULLET;
	b->DrawData.u.Bullet.UseMask = true;
	b->DrawData.u.Bullet.Mask = colorWhite;
	b->SpeedLow = b->SpeedHigh = 640;
	b->RangeLow = b->RangeHigh = 50;
	b->Power = 40;


	// Grenades

	b = &gBulletClasses[BULLET_GRENADE];
	b->Name = "grenade";
	b->DrawFunc = (TileItemDrawFunc)DrawGrenade;
	b->DrawData.u.GrenadeColor = colorWhite;
	b->SpeedLow = b->SpeedHigh = 384;
	b->RangeLow = b->RangeHigh = 100;
	b->Power = 0;
	b->Spark = NULL;
	b->WallHitSound = StrSound("bounce");
	b->WallBounces = true;
	b->HitsObjects = false;
	b->Falling.Enabled = true;
	b->OutOfRangeFunc = GrenadeExplode;

	b = &gBulletClasses[BULLET_SHRAPNELBOMB];
	b->Name = "shrapnelbomb";
	b->DrawFunc = (TileItemDrawFunc)DrawGrenade;
	b->DrawData.u.GrenadeColor = colorGray;
	b->SpeedLow = b->SpeedHigh = 384;
	b->RangeLow = b->RangeHigh = 100;
	b->Power = 0;
	b->Spark = NULL;
	b->WallHitSound = StrSound("bounce");
	b->WallBounces = true;
	b->HitsObjects = false;
	b->Falling.Enabled = true;
	b->OutOfRangeFunc = GrenadeExplode;

	b = &gBulletClasses[BULLET_MOLOTOV];
	b->Name = "molotov";
	b->DrawFunc = (TileItemDrawFunc)DrawMolotov;
	b->DrawData.u.GrenadeColor = colorWhite;
	b->SpeedLow = b->SpeedHigh = 384;
	b->RangeLow = b->RangeHigh = 100;
	b->Power = 0;
	b->Spark = NULL;
	b->WallHitSound = NULL;
	b->HitsObjects = false;
	b->Falling.Enabled = true;
	b->Falling.DestroyOnDrop = true;
	b->Falling.DropFunc = GrenadeExplode;
	b->OutOfRangeFunc = GrenadeExplode;
	b->HitFunc = GrenadeExplode;

	b = &gBulletClasses[BULLET_GASBOMB];
	b->Name = "gasbomb";
	b->DrawFunc = (TileItemDrawFunc)DrawGrenade;
	b->DrawData.u.GrenadeColor = colorGreen;
	b->SpeedLow = b->SpeedHigh = 384;
	b->RangeLow = b->RangeHigh = 100;
	b->Power = 0;
	b->Spark = NULL;
	b->WallHitSound = StrSound("bounce");
	b->WallBounces = true;
	b->HitsObjects = false;
	b->Falling.Enabled = true;
	b->OutOfRangeFunc = GrenadeExplode;

	b = &gBulletClasses[BULLET_CONFUSEBOMB];
	b->Name = "confusebomb";
	b->DrawFunc = (TileItemDrawFunc)DrawGrenade;
	b->DrawData.u.GrenadeColor = colorPurple;
	b->SpeedLow = b->SpeedHigh = 384;
	b->RangeLow = b->RangeHigh = 100;
	b->Power = 0;
	b->Spark = NULL;
	b->WallHitSound = StrSound("bounce");
	b->WallBounces = true;
	b->HitsObjects = false;
	b->Falling.Enabled = true;
	b->OutOfRangeFunc = GrenadeExplode;

	b = &gBulletClasses[BULLET_GAS];
	b->Name = "gas";
	b->DrawFunc = (TileItemDrawFunc)DrawGasCloud;
	b->SpeedLow = b->SpeedHigh = 384;
	b->Friction = Vec2iNew(4, 3);
	b->RangeLow = b->RangeHigh = 35;
	b->Power = 0;
	b->Size = Vec2iNew(10, 10);
	b->Special = SPECIAL_POISON;
	b->Persists = true;
	b->Spark = NULL;
	b->WallHitSound = StrSound("hit_gas");
	b->WallBounces = true;
	b->RandomAnimation = true;

	b = &gBulletClasses[BULLET_RAPID];
	b->Name = "pulse";
	b->GetPicFunc = GetBeam;
	b->Beam.Sprites = PicManagerGetSprites(&gPicManager, "pulse");
	b->SpeedLow = b->SpeedHigh = 1280;
	b->RangeLow = b->RangeHigh = 25;
	b->Power = 6;

	b = &gBulletClasses[BULLET_HEATSEEKER];
	b->Name = "heatseeker";
	b->GetPicFunc = GetBeam;
	b->Beam.Sprites = PicManagerGetSprites(&gPicManager, "rockets");
	b->SpeedLow = b->SpeedHigh = 512;
	b->RangeLow = b->RangeHigh = 60;
	b->Power = 20;
	b->Size = Vec2iNew(3, 3);
	b->SeekFactor = 20;

	b = &gBulletClasses[BULLET_BROWN];
	b->Name = "rapid";
	b->GetPicFunc = GetBeam;
	b->Beam.Sprites = PicManagerGetSprites(&gPicManager, "rapid");
	b->SpeedLow = b->SpeedHigh = 768;
	b->RangeLow = b->RangeHigh = 45;
	b->Power = 15;
	b->Erratic = true;

	b = &gBulletClasses[BULLET_PETRIFIER];
	b->Name = "petrifier";
	b->DrawFunc = (TileItemDrawFunc)DrawBullet;
	b->DrawData.u.Bullet.Ofspic = OFSPIC_MOLOTOV;
	b->DrawData.u.Bullet.UseMask = false;
	b->DrawData.u.Bullet.Tint = tintDarker;
	b->SpeedLow = b->SpeedHigh = 768;
	b->RangeLow = b->RangeHigh = 45;
	b->Power = 0;
	b->Size = Vec2iNew(5, 5);
	b->Special = SPECIAL_PETRIFY;

	b = &gBulletClasses[BULLET_PROXMINE];
	b->Name = "proxmine";
	b->DrawFunc = (TileItemDrawFunc)DrawBullet;
	b->DrawData.u.Bullet.Pic = PicManagerGetPic(&gPicManager, "mine_inactive");
	b->DrawData.u.Bullet.Ofspic = OFSPIC_MINE;
	b->DrawData.u.Bullet.UseMask = true;
	b->DrawData.u.Bullet.Mask = colorWhite;
	b->SpeedLow = b->SpeedHigh = 0;
	b->RangeLow = b->RangeHigh = 140;
	b->Power = 0;
	b->Persists = true;
	b->HitsObjects = false;
	b->OutOfRangeFunc = AddActiveMine;

	b = &gBulletClasses[BULLET_DYNAMITE];
	b->Name = "dynamite";
	b->DrawFunc = (TileItemDrawFunc)DrawBullet;
	b->DrawData.u.Bullet.Ofspic = OFSPIC_DYNAMITE;
	b->DrawData.u.Bullet.UseMask = true;
	b->DrawData.u.Bullet.Mask = colorWhite;
	b->SpeedLow = b->SpeedHigh = 0;
	b->RangeLow = b->RangeHigh = 210;
	b->Power = 0;
	b->Persists = true;
	b->Spark = NULL;
	b->HitsObjects = false;
	b->OutOfRangeFunc = Explode;


	b = &gBulletClasses[BULLET_FIREBALL_WRECK];
	b->Name = "fireball_wreck";
	b->DrawFunc = (TileItemDrawFunc)DrawFireball;
	b->SpeedLow = b->SpeedHigh = 0;
	b->RangeLow = b->RangeHigh = FIREBALL_MAX * 4 - 1;
	b->Power = 0;
	b->Size = Vec2iNew(7, 5);
	b->Special = SPECIAL_EXPLOSION;
	b->Persists = true;
	b->Spark = NULL;

	b = &gBulletClasses[BULLET_FIREBALL1];
	b->Name = "fireball1";
	b->DrawFunc = (TileItemDrawFunc)DrawFireball;
	b->SpeedLow = b->SpeedHigh = 256;
	b->RangeLow = b->RangeHigh = FIREBALL_MAX * 4 - 1;
	b->Power = FIREBALL_POWER;
	b->Size = Vec2iNew(7, 5);
	b->Special = SPECIAL_EXPLOSION;
	b->Persists = true;
	b->Spark = NULL;
	b->WallHitSound = NULL;
	b->Falling.Enabled = true;
	b->Falling.Type = FALLING_TYPE_DZ;

	b = &gBulletClasses[BULLET_FIREBALL2];
	b->Name = "fireball2";
	b->DrawFunc = (TileItemDrawFunc)DrawFireball;
	b->SpeedLow = b->SpeedHigh = 192;
	b->RangeLow = b->RangeHigh = FIREBALL_MAX * 4 - 1;
	b->Power = FIREBALL_POWER;
	b->Size = Vec2iNew(7, 5);
	b->Special = SPECIAL_EXPLOSION;
	b->Persists = true;
	b->Spark = NULL;
	b->WallHitSound = NULL;
	b->Falling.Enabled = true;
	b->Falling.Type = FALLING_TYPE_DZ;

	b = &gBulletClasses[BULLET_FIREBALL3];
	b->Name = "fireball3";
	b->DrawFunc = (TileItemDrawFunc)DrawFireball;
	b->SpeedLow = b->SpeedHigh = 128;
	b->RangeLow = b->RangeHigh = FIREBALL_MAX * 4 - 1;
	b->Power = FIREBALL_POWER;
	b->Size = Vec2iNew(7, 5);
	b->Special = SPECIAL_EXPLOSION;
	b->Persists = true;
	b->Spark = NULL;
	b->WallHitSound = NULL;
	b->Falling.Enabled = true;
	b->Falling.Type = FALLING_TYPE_DZ;

	b = &gBulletClasses[BULLET_MOLOTOV_FLAME];
	b->Name = "molotov_flame";
	b->GetPicFunc = GetFlame;
	b->SpeedLow = -256;
	b->SpeedHigh = 16 * 31 - 256;
	b->SpeedScale = true;
	b->Friction = Vec2iNew(4, 3);
	b->RangeLow = FLAME_RANGE * 4;
	b->RangeHigh = (FLAME_RANGE + 8 - 1) * 4;
	b->Power = 2;
	b->Size = Vec2iNew(5, 5);
	b->Special = SPECIAL_FLAME;
	b->Persists = true;
	b->Spark = NULL;
	b->WallHitSound = StrSound("hit_fire");
	b->WallBounces = true;
	b->Falling.Enabled = true;
	b->Falling.Type = FALLING_TYPE_Z;
	b->RandomAnimation = true;

	b = &gBulletClasses[BULLET_GAS_CLOUD_POISON];
	b->Name = "gas_cloud_poison";
	b->DrawFunc = DrawGasCloud;
	b->SpeedLow = 0;
	b->SpeedHigh = 255;
	b->Friction = Vec2iNew(4, 3);
	b->RangeLow = 48 * 4 - 1;
	b->RangeHigh = (48 - 8 - 1) * 4 - 1;
	b->Power = 0;
	b->Size = Vec2iNew(10, 10);
	b->Special = SPECIAL_POISON;
	b->Persists = true;
	b->Spark = NULL;
	b->WallHitSound = StrSound("hit_gas");
	b->WallBounces = true;
	b->RandomAnimation = true;

	b = &gBulletClasses[BULLET_GAS_CLOUD_CONFUSE];
	b->Name = "gas_cloud_confuse";
	b->DrawFunc = DrawGasCloud;
	b->SpeedLow = 0;
	b->SpeedHigh = 255;
	b->Friction = Vec2iNew(4, 3);
	b->RangeLow = 48 * 4 - 1;
	b->RangeHigh = (48 - 8 - 1) * 4 - 1;
	b->Power = 0;
	b->Size = Vec2iNew(10, 10);
	b->Special = SPECIAL_CONFUSE;
	b->Persists = true;
	b->Spark = NULL;
	b->WallHitSound = StrSound("hit_gas");
	b->WallBounces = true;
	b->RandomAnimation = true;


	b = &gBulletClasses[BULLET_ACTIVEMINE];
	b->Name = "activemine";
	b->DrawFunc = (TileItemDrawFunc)DrawBullet;
	b->DrawData.u.Bullet.Pic = PicManagerGetPic(&gPicManager, "mine_active");
	b->DrawData.u.Bullet.Ofspic = OFSPIC_MINE;
	b->DrawData.u.Bullet.UseMask = true;
	b->DrawData.u.Bullet.Mask = colorWhite;
	b->SpeedLow = b->SpeedHigh = 0;
	b->RangeLow = b->RangeHigh = -1;
	b->Power = 0;
	b->Persists = true;
	b->HitsObjects = false;
	b->ProximityFunc = AddTriggeredMine;

	b = &gBulletClasses[BULLET_TRIGGEREDMINE];
	b->Name = "triggeredmine";
	b->DrawFunc = (TileItemDrawFunc)DrawBullet;
	b->DrawData.u.Bullet.Pic = PicManagerGetPic(&gPicManager, "mine_active");
	b->DrawData.u.Bullet.Ofspic = OFSPIC_MINE;
	b->DrawData.u.Bullet.UseMask = true;
	b->DrawData.u.Bullet.Mask = colorWhite;
	b->SpeedLow = b->SpeedHigh = 0;
	b->RangeLow = b->RangeHigh = 5;
	b->Power = 0;
	b->Persists = true;
	b->HitsObjects = false;
	b->OutOfRangeFunc = Explode;
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
	if (!Vec2iEqual(gBulletClasses[type].Size, Vec2iZero()))
	{
		const Vec2i vel = GetVectorsForRadians(radians);
		pos = Vec2iAdd(pos, Vec2iNew(vel.x * 4, vel.y * 7));
	}
	TMobileObject *obj = CArrayGet(&gMobObjs, MobObjAdd(pos, player));
	obj->vel = GetFullVectorsForRadians(radians);
	SetBulletProps(obj, z, type, flags);
}

void AddBulletDirectional(
	Vec2i pos, int z, direction_e dir, BulletType type, int flags, int player)
{
	TMobileObject *obj = CArrayGet(&gMobObjs, MobObjAdd(pos, player));
	obj->vel = GetFullVectorsForRadians(dir2radians[dir]);
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
	case BULLET_FLAME:		// fallthrough
	case BULLET_BROWN:		// fallthrough
	case BULLET_PETRIFIER:	// fallthrough
	case BULLET_PROXMINE:	// fallthrough
	case BULLET_DYNAMITE:	// fallthrough
	case BULLET_RAPID:		// fallthrough
	case BULLET_HEATSEEKER:	// fallthrough
	case BULLET_FRAG:		// fallthrough
	case BULLET_ACTIVEMINE:	// fallthrough
	case BULLET_TRIGGEREDMINE:
		AddBullet(muzzlePos, muzzleHeight, angle, bullet, flags, playerIndex);
		break;

	case BULLET_GRENADE:		// fallthrough
	case BULLET_SHRAPNELBOMB:	// fallthrough
	case BULLET_MOLOTOV:		// fallthrough
	case BULLET_GASBOMB:		// fallthrough
	case BULLET_CONFUSEBOMB:
		AddGrenade(muzzlePos, muzzleHeight, angle, bullet, flags, playerIndex);
		break;

	case BULLET_LASER:	// fallthrough
	case BULLET_SNIPER:
		AddBulletDirectional(
			muzzlePos, muzzleHeight, d, bullet, flags, playerIndex);
		break;

	case BULLET_GAS:
		AddGasCloud(muzzlePos, muzzleHeight, angle, flags, playerIndex);
		break;

	default:
		// unknown bullet?
		CASSERT(false, "Unknown bullet");
		break;
	}
}