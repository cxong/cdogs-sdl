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

CArray gBulletClasses;


// TODO: use map structure?
BulletClass *StrBulletClass(const char *s)
{
	if (s == NULL || strlen(s) == 0)
	{
		return NULL;
	}
	for (int i = 0; i < (int)gBulletClasses.size; i++)
	{
		BulletClass *b = CArrayGet(&gBulletClasses, i);
		if (strcmp(s, b->Name) == 0)
		{
			return b;
		}
	}
	CASSERT(false, "cannot parse bullet name");
	return NULL;
}

// Draw functions

static CPicDrawContext GetBulletDrawContext(const int id)
{
	const TMobileObject *obj = CArrayGet(&gMobObjs, id);
	CASSERT(obj->isInUse, "Cannot draw non-existent mobobj");
	// Calculate direction based on velocity
	const direction_e dir = RadiansToDirection(Vec2iToRadians(obj->vel));
	const Pic *pic = CPicGetPic(&obj->tileItem.CPic, dir);
	CPicDrawContext c;
	c.Dir = dir;
	if (pic != NULL)
	{
		c.Offset = Vec2iNew(
			pic->size.x / -2, pic->size.y / -2 - obj->z / Z_FACTOR);
	}
	return c;
}


static void SetBulletProps(
	TMobileObject *obj, const int z, const int dz, const BulletClass *b,
	const int flags)
{
	obj->bulletClass = b;
	obj->updateFunc = UpdateBullet;
	obj->tileItem.getPicFunc = NULL;
	obj->tileItem.getActorPicsFunc = NULL;
	obj->tileItem.drawFunc = NULL;
	obj->tileItem.CPic = b->CPic;
	obj->tileItem.CPicFunc = b->CPicFunc;
	obj->z = z;
	obj->dz = dz;
	obj->range = RAND_INT(b->RangeLow, b->RangeHigh);
	obj->flags = flags;
	if (b->HurtAlways)
	{
		obj->flags |= FLAGS_HURTALWAYS;
	}
	obj->vel = Vec2iFull2Real(Vec2iScale(
		obj->vel, RAND_INT(b->SpeedLow, b->SpeedHigh)));
	if (b->SpeedScale)
	{
		obj->vel.y = obj->vel.y * TILE_HEIGHT / TILE_WIDTH;
	}
	obj->tileItem.w = b->Size.x;
	obj->tileItem.h = b->Size.y;
	obj->tileItem.ShadowSize = b->ShadowSize;
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


static void FireGuns(const TMobileObject *obj, const CArray *guns);
bool UpdateBullet(TMobileObject *obj, const int ticks)
{
	MobileObjectUpdate(obj, ticks);
	if (obj->count < obj->bulletClass->Delay)
	{
		return true;
	}

	if (obj->range >= 0 && obj->count > obj->range)
	{
		FireGuns(obj, &obj->bulletClass->OutOfRangeGuns);
		return false;
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

	Vec2i pos = Vec2iScale(Vec2iAdd(objPos, obj->vel), ticks);
	const bool hitItem = HitItem(obj, pos);
	const Vec2i realPos = Vec2iFull2Real(pos);

	// Falling (grenades)
	if (obj->bulletClass->Falling.GravityFactor != 0)
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
						obj->z = 0;
						obj->dz = -obj->dz / 2;
						if (!hasDropped)
						{
							FireGuns(obj, &obj->bulletClass->Falling.DropGuns);
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
					}
					else
					{
						obj->dz -= obj->bulletClass->Falling.GravityFactor;
					}
				}
			}
			break;
		case FALLING_TYPE_DZ:
			obj->z += obj->dz * ticks;
			obj->dz = MAX(
				0, obj->dz - ticks * obj->bulletClass->Falling.GravityFactor);
			break;
		case FALLING_TYPE_Z:
			obj->z += obj->dz * ticks;
			if (obj->z <= 0)
			{
				obj->z = 0;
			}
			else
			{
				obj->dz -= obj->bulletClass->Falling.GravityFactor;
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

	const bool hitWall =
		MapIsTileIn(&gMap, Vec2iToTile(realPos)) &&
		ShootWall(realPos.x, realPos.y);
	if (hitWall && !Vec2iEqual(obj->vel, Vec2iZero()))
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
		FireGuns(obj, &obj->bulletClass->HitGuns);
		if (obj->bulletClass->Spark != NULL)
		{
			GameEvent e;
			memset(&e, 0, sizeof e);
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
	if (hitWall && !Vec2iEqual(obj->vel, Vec2iZero()))
	{
		// Bouncing
		pos = GetWallBounceFullPos(objPos, pos, &obj->vel);
	}
	obj->x = pos.x;
	obj->y = pos.y;
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
	if (obj->bulletClass->ProximityGuns.size > 0 && !(obj->count & 3))
	{
		// Detonate the mine if there are characters in the tiles around it
		const Vec2i tv =
			Vec2iToTile(Vec2iFull2Real(pos));
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
					FireGuns(obj, &obj->bulletClass->ProximityGuns);
					return false;
				}
			}
		}
	}

	return true;
}
static void FireGuns(const TMobileObject *obj, const CArray *guns)
{
	const Vec2i fullPos = Vec2iNew(obj->x, obj->y);
	const double angle = Vec2iToRadians(obj->vel);
	for (int i = 0; i < (int)guns->size; i++)
	{
		const GunDescription **g = CArrayGet(guns, i);
		GunAddBullets(
			*g, fullPos, obj->z, angle, obj->flags, obj->player, true);
	}
}


void BulletInitialize(CArray *bullets)
{
	CArrayInit(bullets, sizeof(BulletClass));

	// Defaults
	BulletClass defaultB;
	memset(&defaultB, 0, sizeof defaultB);
	defaultB.Friction = Vec2iZero();
	defaultB.Size = Vec2iZero();
	defaultB.Special = SPECIAL_NONE;
	defaultB.Spark = ParticleClassGet(&gParticleClasses, "spark");
	defaultB.WallHitSound = StrSound("ricochet");
	defaultB.HitsObjects = true;
	defaultB.Falling.Type = FALLING_TYPE_BOUNCE;
	defaultB.SeekFactor = -1;

	BulletClass b;

	memcpy(&b, &defaultB, sizeof b);
	CSTRDUP(b.Name, "mg");
	b.CPic.Type = PICTYPE_NORMAL;
	b.CPic.u.Pic = PicManagerGetPic(&gPicManager, "bullet");
	b.CPic.UseMask = true;
	b.CPic.u1.Mask = colorWhite;
	b.CPicFunc = GetBulletDrawContext;
	b.SpeedLow = b.SpeedHigh = 768;
	b.RangeLow = b.RangeHigh = 60;
	b.Power = 10;
	CArrayPushBack(bullets, &b);

	memcpy(&b, &defaultB, sizeof b);
	CSTRDUP(b.Name, "shotgun");
	b.CPic.Type = PICTYPE_NORMAL;
	b.CPic.u.Pic = PicManagerGetPic(&gPicManager, "bullet");
	b.CPic.UseMask = true;
	b.CPic.u1.Mask = colorWhite;
	b.CPicFunc = GetBulletDrawContext;
	b.SpeedLow = b.SpeedHigh = 640;
	b.RangeLow = b.RangeHigh = 50;
	b.Power = 15;
	CArrayPushBack(bullets, &b);

	memcpy(&b, &defaultB, sizeof b);
	CSTRDUP(b.Name, "flame");
	b.CPic.Type = PICTYPE_ANIMATED_RANDOM;
	b.CPic.u.Animated.Sprites =
		&PicManagerGetSprites(&gPicManager, "flame")->pics;
	b.CPic.u.Animated.TicksPerFrame = 4;
	b.CPic.UseMask = true;
	b.CPic.u1.Mask = colorWhite;
	b.CPicFunc = GetBulletDrawContext;
	b.SpeedLow = b.SpeedHigh = 384;
	b.RangeLow = b.RangeHigh = 30;
	b.Power = 12;
	b.Size = Vec2iNew(5, 5);
	b.Special = SPECIAL_FLAME;
	b.Spark = NULL;
	b.WallHitSound = StrSound("hit_fire");
	b.WallBounces = true;
	CArrayPushBack(bullets, &b);

	memcpy(&b, &defaultB, sizeof b);
	CSTRDUP(b.Name, "laser");
	b.CPic.Type = PICTYPE_DIRECTIONAL;
	b.CPic.u.Sprites = &PicManagerGetSprites(&gPicManager, "beam")->pics;
	b.CPic.UseMask = true;
	b.CPic.u1.Mask = colorWhite;
	b.CPicFunc = GetBulletDrawContext;
	b.SpeedLow = b.SpeedHigh = 1024;
	b.RangeLow = b.RangeHigh = 90;
	b.Power = 20;
	b.Size = Vec2iNew(2, 2);
	CArrayPushBack(bullets, &b);

	memcpy(&b, &defaultB, sizeof b);
	CSTRDUP(b.Name, "sniper");
	b.CPic.Type = PICTYPE_DIRECTIONAL;
	b.CPic.u.Sprites =
		&PicManagerGetSprites(&gPicManager, "beam_bright")->pics;
	b.CPic.UseMask = true;
	b.CPic.u1.Mask = colorWhite;
	b.CPicFunc = GetBulletDrawContext;
	b.SpeedLow = b.SpeedHigh = 1024;
	b.RangeLow = b.RangeHigh = 90;
	b.Power = 50;
	CArrayPushBack(bullets, &b);

	memcpy(&b, &defaultB, sizeof b);
	CSTRDUP(b.Name, "frag");
	b.CPic.Type = PICTYPE_NORMAL;
	b.CPic.u.Pic = PicManagerGetPic(&gPicManager, "bullet");
	b.CPic.UseMask = true;
	b.CPic.u1.Mask = colorWhite;
	b.CPicFunc = GetBulletDrawContext;
	b.SpeedLow = b.SpeedHigh = 640;
	b.RangeLow = b.RangeHigh = 50;
	b.Power = 40;
	b.HurtAlways = true;
	CArrayPushBack(bullets, &b);


	// Grenades

	memcpy(&b, &defaultB, sizeof b);
	CSTRDUP(b.Name, "grenade");
	b.CPic.Type = PICTYPE_ANIMATED;
	b.CPic.u.Animated.Sprites =
		&PicManagerGetSprites(&gPicManager, "grenade")->pics;
	b.CPic.u.Animated.TicksPerFrame = 2;
	b.CPic.UseMask = true;
	b.CPic.u1.Mask = colorWhite;
	b.CPicFunc = GetBulletDrawContext;
	b.ShadowSize = Vec2iNew(4, 3);
	b.SpeedLow = b.SpeedHigh = 384;
	b.RangeLow = b.RangeHigh = 100;
	b.Power = 0;
	b.Spark = NULL;
	b.WallHitSound = StrSound("bounce");
	b.WallBounces = true;
	b.HitsObjects = false;
	b.Falling.GravityFactor = 1;
	CArrayPushBack(bullets, &b);

	memcpy(&b, &defaultB, sizeof b);
	CSTRDUP(b.Name, "shrapnelbomb");
	b.CPic.Type = PICTYPE_ANIMATED;
	b.CPic.u.Animated.Sprites =
		&PicManagerGetSprites(&gPicManager, "grenade")->pics;
	b.CPic.u.Animated.TicksPerFrame = 2;
	b.CPic.UseMask = true;
	b.CPic.u1.Mask = colorGray;
	b.CPicFunc = GetBulletDrawContext;
	b.ShadowSize = Vec2iNew(4, 3);
	b.SpeedLow = b.SpeedHigh = 384;
	b.RangeLow = b.RangeHigh = 100;
	b.Power = 0;
	b.Spark = NULL;
	b.WallHitSound = StrSound("bounce");
	b.WallBounces = true;
	b.HitsObjects = false;
	b.Falling.GravityFactor = 1;
	CArrayPushBack(bullets, &b);

	memcpy(&b, &defaultB, sizeof b);
	CSTRDUP(b.Name, "molotov");
	b.CPic.Type = PICTYPE_NORMAL;
	b.CPic.u.Pic = PicManagerGetPic(&gPicManager, "molotov");
	b.CPic.UseMask = true;
	b.CPic.u1.Mask = colorWhite;
	b.CPicFunc = GetBulletDrawContext;
	b.ShadowSize = Vec2iNew(4, 3);
	b.SpeedLow = b.SpeedHigh = 384;
	b.RangeLow = b.RangeHigh = 100;
	b.Power = 0;
	b.Spark = NULL;
	b.WallHitSound = NULL;
	b.HitsObjects = false;
	b.Falling.GravityFactor = 1;
	b.Falling.DestroyOnDrop = true;
	CArrayPushBack(bullets, &b);

	memcpy(&b, &defaultB, sizeof b);
	CSTRDUP(b.Name, "gasbomb");
	b.CPic.Type = PICTYPE_ANIMATED;
	b.CPic.u.Animated.Sprites =
		&PicManagerGetSprites(&gPicManager, "grenade")->pics;
	b.CPic.u.Animated.TicksPerFrame = 2;
	b.CPic.UseMask = true;
	b.CPic.u1.Mask = colorGreen;
	b.CPicFunc = GetBulletDrawContext;
	b.ShadowSize = Vec2iNew(4, 3);
	b.SpeedLow = b.SpeedHigh = 384;
	b.RangeLow = b.RangeHigh = 100;
	b.Power = 0;
	b.Spark = NULL;
	b.WallHitSound = StrSound("bounce");
	b.WallBounces = true;
	b.HitsObjects = false;
	b.Falling.GravityFactor = 1;
	CArrayPushBack(bullets, &b);

	memcpy(&b, &defaultB, sizeof b);
	CSTRDUP(b.Name, "confusebomb");
	b.CPic.Type = PICTYPE_ANIMATED;
	b.CPic.u.Animated.Sprites =
		&PicManagerGetSprites(&gPicManager, "grenade")->pics;
	b.CPic.u.Animated.TicksPerFrame = 2;
	b.CPic.UseMask = true;
	b.CPic.u1.Mask = colorPurple;
	b.CPicFunc = GetBulletDrawContext;
	b.ShadowSize = Vec2iNew(4, 3);
	b.SpeedLow = b.SpeedHigh = 384;
	b.RangeLow = b.RangeHigh = 100;
	b.Power = 0;
	b.Spark = NULL;
	b.WallHitSound = StrSound("bounce");
	b.WallBounces = true;
	b.HitsObjects = false;
	b.Falling.GravityFactor = 1;
	CArrayPushBack(bullets, &b);

	memcpy(&b, &defaultB, sizeof b);
	CSTRDUP(b.Name, "gas");
	b.CPic.Type = PICTYPE_ANIMATED_RANDOM;
	b.CPic.u.Animated.Sprites =
		&PicManagerGetSprites(&gPicManager, "gas_cloud")->pics;
	b.CPic.u.Animated.TicksPerFrame = 4;
	b.CPic.UseMask = false;
	b.CPic.u1.Tint = tintPoison;
	b.CPicFunc = GetBulletDrawContext;
	b.SpeedLow = b.SpeedHigh = 384;
	b.Friction = Vec2iNew(4, 3);
	b.RangeLow = b.RangeHigh = 35;
	b.Power = 0;
	b.Size = Vec2iNew(10, 10);
	b.Special = SPECIAL_POISON;
	b.Persists = true;
	b.Spark = NULL;
	b.WallHitSound = StrSound("hit_gas");
	b.WallBounces = true;
	CArrayPushBack(bullets, &b);

	memcpy(&b, &defaultB, sizeof b);
	CSTRDUP(b.Name, "pulse");
	b.CPic.Type = PICTYPE_DIRECTIONAL;
	b.CPic.u.Sprites = &PicManagerGetSprites(&gPicManager, "pulse")->pics;
	b.CPic.UseMask = true;
	b.CPic.u1.Mask = colorWhite;
	b.CPicFunc = GetBulletDrawContext;
	b.SpeedLow = b.SpeedHigh = 1280;
	b.RangeLow = b.RangeHigh = 25;
	b.Power = 6;
	CArrayPushBack(bullets, &b);

	memcpy(&b, &defaultB, sizeof b);
	CSTRDUP(b.Name, "heatseeker");
	b.CPic.Type = PICTYPE_DIRECTIONAL;
	b.CPic.u.Sprites = &PicManagerGetSprites(&gPicManager, "rocket")->pics;
	b.CPic.UseMask = true;
	b.CPic.u1.Mask = colorWhite;
	b.CPicFunc = GetBulletDrawContext;
	b.SpeedLow = b.SpeedHigh = 512;
	b.RangeLow = b.RangeHigh = 60;
	b.Power = 20;
	b.Size = Vec2iNew(3, 3);
	b.Spark = ParticleClassGet(&gParticleClasses, "boom");
	b.WallHitSound = StrSound("boom");
	b.SeekFactor = 20;
	CArrayPushBack(bullets, &b);

	memcpy(&b, &defaultB, sizeof b);
	CSTRDUP(b.Name, "rapid");
	b.CPic.Type = PICTYPE_DIRECTIONAL;
	b.CPic.u.Sprites = &PicManagerGetSprites(&gPicManager, "rapid")->pics;
	b.CPic.UseMask = true;
	b.CPic.u1.Mask = colorWhite;
	b.CPicFunc = GetBulletDrawContext;
	b.SpeedLow = b.SpeedHigh = 768;
	b.RangeLow = b.RangeHigh = 45;
	b.Power = 15;
	b.Erratic = true;
	CArrayPushBack(bullets, &b);

	memcpy(&b, &defaultB, sizeof b);
	CSTRDUP(b.Name, "petrifier");
	b.CPic.Type = PICTYPE_NORMAL;
	b.CPic.u.Pic = PicManagerGetPic(&gPicManager, "molotov");
	b.CPic.UseMask = false;
	b.CPic.u1.Tint = tintDarker;
	b.CPicFunc = GetBulletDrawContext;
	b.SpeedLow = b.SpeedHigh = 768;
	b.RangeLow = b.RangeHigh = 45;
	b.Power = 0;
	b.Size = Vec2iNew(5, 5);
	b.Special = SPECIAL_PETRIFY;
	CArrayPushBack(bullets, &b);

	memcpy(&b, &defaultB, sizeof b);
	CSTRDUP(b.Name, "proxmine");
	b.CPic.Type = PICTYPE_NORMAL;
	b.CPic.u.Pic = PicManagerGetPic(&gPicManager, "mine_inactive");
	b.CPic.UseMask = true;
	b.CPic.u1.Mask = colorWhite;
	b.CPicFunc = GetBulletDrawContext;
	b.SpeedLow = b.SpeedHigh = 0;
	b.RangeLow = b.RangeHigh = 140;
	b.Power = 0;
	b.Persists = true;
	b.HitsObjects = false;
	CArrayPushBack(bullets, &b);

	memcpy(&b, &defaultB, sizeof b);
	CSTRDUP(b.Name, "dynamite");
	b.CPic.Type = PICTYPE_NORMAL;
	b.CPic.u.Pic = PicManagerGetPic(&gPicManager, "dynamite");
	b.CPic.UseMask = true;
	b.CPic.u1.Mask = colorWhite;
	b.CPicFunc = GetBulletDrawContext;
	b.SpeedLow = b.SpeedHigh = 0;
	b.RangeLow = b.RangeHigh = 210;
	b.Power = 0;
	b.Persists = true;
	b.Spark = NULL;
	b.HitsObjects = false;
	CArrayPushBack(bullets, &b);

	memcpy(&b, &defaultB, sizeof b);
	CSTRDUP(b.Name, "swarmer");
	b.CPic.Type = PICTYPE_DIRECTIONAL;
	b.CPic.u.Sprites = &PicManagerGetSprites(&gPicManager, "swarmer")->pics;
	b.CPic.UseMask = true;
	b.CPic.u1.Mask = colorWhite;
	b.CPicFunc = GetBulletDrawContext;
	b.SpeedLow = b.SpeedHigh = 700;
	b.RangeLow = b.RangeHigh = 70;
	b.Power = 12;
	b.Size = Vec2iNew(3, 3);
	b.Spark = ParticleClassGet(&gParticleClasses, "boom");
	b.WallHitSound = StrSound("boom");
	b.SeekFactor = 30;
	CArrayPushBack(bullets, &b);


	memcpy(&b, &defaultB, sizeof b);
	CSTRDUP(b.Name, "fireball_wreck");
	b.CPic.Type = PICTYPE_ANIMATED;
	b.CPic.u.Animated.Sprites =
		&PicManagerGetSprites(&gPicManager, "fireball")->pics;
	b.CPic.u.Animated.Count = 10;
	b.CPic.u.Animated.TicksPerFrame = 4;
	b.CPic.UseMask = true;
	b.CPic.u1.Mask = colorWhite;
	b.CPicFunc = GetBulletDrawContext;
	b.Delay = -10;
	b.SpeedLow = b.SpeedHigh = 0;
	b.RangeLow = b.RangeHigh = FIREBALL_MAX * 4 - 1 + b.Delay;
	b.Power = 0;
	b.Size = Vec2iNew(7, 5);
	b.Special = SPECIAL_EXPLOSION;
	b.Persists = true;
	b.Spark = NULL;
	CArrayPushBack(bullets, &b);

	memcpy(&b, &defaultB, sizeof b);
	CSTRDUP(b.Name, "fireball1");
	b.CPic.Type = PICTYPE_ANIMATED;
	b.CPic.u.Animated.Sprites =
		&PicManagerGetSprites(&gPicManager, "fireball")->pics;
	b.CPic.u.Animated.TicksPerFrame = 4;
	b.CPic.UseMask = true;
	b.CPic.u1.Mask = colorWhite;
	b.CPicFunc = GetBulletDrawContext;
	b.SpeedLow = b.SpeedHigh = 256;
	b.RangeLow = b.RangeHigh = FIREBALL_MAX * 4 - 1;
	b.Power = FIREBALL_POWER;
	b.Size = Vec2iNew(7, 5);
	b.Special = SPECIAL_EXPLOSION;
	b.HurtAlways = true;
	b.Persists = true;
	b.Spark = NULL;
	b.WallHitSound = NULL;
	b.Falling.GravityFactor = 1;
	b.Falling.Type = FALLING_TYPE_DZ;
	CArrayPushBack(bullets, &b);

	memcpy(&b, &defaultB, sizeof b);
	CSTRDUP(b.Name, "fireball2");
	b.CPic.Type = PICTYPE_ANIMATED;
	b.CPic.u.Animated.Sprites =
		&PicManagerGetSprites(&gPicManager, "fireball")->pics;
	b.CPic.u.Animated.Count = -8;
	b.CPic.u.Animated.TicksPerFrame = 4;
	b.CPic.UseMask = true;
	b.CPic.u1.Mask = colorWhite;
	b.CPicFunc = GetBulletDrawContext;
	b.Delay = 8;
	b.SpeedLow = b.SpeedHigh = 192;
	b.RangeLow = b.RangeHigh = FIREBALL_MAX * 4 - 1 + b.Delay;
	b.Power = FIREBALL_POWER;
	b.Size = Vec2iNew(7, 5);
	b.Special = SPECIAL_EXPLOSION;
	b.HurtAlways = true;
	b.Persists = true;
	b.Spark = NULL;
	b.WallHitSound = NULL;
	b.Falling.GravityFactor = 1;
	b.Falling.Type = FALLING_TYPE_DZ;
	CArrayPushBack(bullets, &b);

	memcpy(&b, &defaultB, sizeof b);
	CSTRDUP(b.Name, "fireball3");
	b.CPic.Type = PICTYPE_ANIMATED;
	b.CPic.u.Animated.Sprites =
		&PicManagerGetSprites(&gPicManager, "fireball")->pics;
	b.CPic.u.Animated.Count = -16;
	b.CPic.u.Animated.TicksPerFrame = 4;
	b.CPic.UseMask = true;
	b.CPic.u1.Mask = colorWhite;
	b.CPicFunc = GetBulletDrawContext;
	b.Delay = 16;
	b.SpeedLow = b.SpeedHigh = 128;
	b.RangeLow = b.RangeHigh = FIREBALL_MAX * 4 - 1 + b.Delay;
	b.Power = FIREBALL_POWER;
	b.Size = Vec2iNew(7, 5);
	b.Special = SPECIAL_EXPLOSION;
	b.HurtAlways = true;
	b.Persists = true;
	b.Spark = NULL;
	b.WallHitSound = NULL;
	b.Falling.GravityFactor = 1;
	b.Falling.Type = FALLING_TYPE_DZ;
	CArrayPushBack(bullets, &b);

	memcpy(&b, &defaultB, sizeof b);
	CSTRDUP(b.Name, "molotov_flame");
	b.CPic.Type = PICTYPE_ANIMATED_RANDOM;
	b.CPic.u.Animated.Sprites =
		&PicManagerGetSprites(&gPicManager, "flame")->pics;
	b.CPic.u.Animated.TicksPerFrame = 4;
	b.CPic.UseMask = true;
	b.CPic.u1.Mask = colorWhite;
	b.CPicFunc = GetBulletDrawContext;
	b.SpeedLow = -256;
	b.SpeedHigh = 16 * 31 - 256;
	b.SpeedScale = true;
	b.Friction = Vec2iNew(4, 3);
	b.RangeLow = FLAME_RANGE * 4;
	b.RangeHigh = (FLAME_RANGE + 8 - 1) * 4;
	b.Power = 2;
	b.Size = Vec2iNew(5, 5);
	b.Special = SPECIAL_FLAME;
	b.HurtAlways = true;
	b.Persists = true;
	b.Spark = NULL;
	b.WallHitSound = NULL;
	b.WallBounces = true;
	b.Falling.GravityFactor = 4;
	b.Falling.Type = FALLING_TYPE_Z;
	CArrayPushBack(bullets, &b);

	memcpy(&b, &defaultB, sizeof b);
	CSTRDUP(b.Name, "gas_cloud_poison");
	b.CPic.Type = PICTYPE_ANIMATED_RANDOM;
	b.CPic.u.Animated.Sprites =
		&PicManagerGetSprites(&gPicManager, "gas_cloud")->pics;
	b.CPic.u.Animated.TicksPerFrame = 4;
	b.CPic.UseMask = false;
	b.CPic.u1.Tint = tintPoison;
	b.CPicFunc = GetBulletDrawContext;
	b.SpeedLow = 0;
	b.SpeedHigh = 255;
	b.Friction = Vec2iNew(4, 3);
	b.RangeLow = 48 * 4 - 1;
	b.RangeHigh = (48 - 8 - 1) * 4 - 1;
	b.Power = 0;
	b.Size = Vec2iNew(10, 10);
	b.Special = SPECIAL_POISON;
	b.HurtAlways = true;
	b.Persists = true;
	b.Spark = NULL;
	b.WallHitSound = StrSound("hit_gas");
	b.WallBounces = true;
	CArrayPushBack(bullets, &b);

	memcpy(&b, &defaultB, sizeof b);
	CSTRDUP(b.Name, "gas_cloud_confuse");
	b.CPic.Type = PICTYPE_ANIMATED_RANDOM;
	b.CPic.u.Animated.Sprites =
		&PicManagerGetSprites(&gPicManager, "gas_cloud")->pics;
	b.CPic.u.Animated.TicksPerFrame = 4;
	b.CPic.UseMask = false;
	b.CPic.u1.Tint = tintPurple;
	b.CPicFunc = GetBulletDrawContext;
	b.SpeedLow = 0;
	b.SpeedHigh = 255;
	b.Friction = Vec2iNew(4, 3);
	b.RangeLow = 48 * 4 - 1;
	b.RangeHigh = (48 - 8 - 1) * 4 - 1;
	b.Power = 0;
	b.Size = Vec2iNew(10, 10);
	b.Special = SPECIAL_CONFUSE;
	b.Persists = true;
	b.Spark = NULL;
	b.WallHitSound = StrSound("hit_gas");
	b.WallBounces = true;
	CArrayPushBack(bullets, &b);


	memcpy(&b, &defaultB, sizeof b);
	CSTRDUP(b.Name, "activemine");
	b.CPic.Type = PICTYPE_NORMAL;
	b.CPic.u.Pic = PicManagerGetPic(&gPicManager, "mine_active");
	b.CPic.UseMask = true;
	b.CPic.u1.Mask = colorWhite;
	b.CPicFunc = GetBulletDrawContext;
	b.SpeedLow = b.SpeedHigh = 0;
	b.RangeLow = b.RangeHigh = -1;
	b.Power = 0;
	b.Persists = true;
	b.HitsObjects = false;
	CArrayPushBack(bullets, &b);

	memcpy(&b, &defaultB, sizeof b);
	CSTRDUP(b.Name, "triggeredmine");
	b.CPic.Type = PICTYPE_NORMAL;
	b.CPic.u.Pic = PicManagerGetPic(&gPicManager, "mine_active");
	b.CPic.UseMask = true;
	b.CPic.u1.Mask = colorWhite;
	b.CPicFunc = GetBulletDrawContext;
	b.SpeedLow = b.SpeedHigh = 0;
	b.RangeLow = b.RangeHigh = 5;
	b.Power = 0;
	b.Persists = true;
	b.HitsObjects = false;
	CArrayPushBack(bullets, &b);
}
void BulletInitialize2(CArray *bullets)
{
	UNUSED(bullets);
	BulletClass *b;
	const GunDescription *g;
	
	b = StrBulletClass("grenade");
	CArrayInit(&b->OutOfRangeGuns, sizeof(const GunDescription *));
	g = StrGunDescription("explosion1");
	CArrayPushBack(&b->OutOfRangeGuns, &g);
	g = StrGunDescription("explosion2");
	CArrayPushBack(&b->OutOfRangeGuns, &g);
	g = StrGunDescription("explosion3");
	CArrayPushBack(&b->OutOfRangeGuns, &g);
	
	b = StrBulletClass("shrapnelbomb");
	CArrayInit(&b->OutOfRangeGuns, sizeof(const GunDescription *));
	g = StrGunDescription("frag_explosion");
	CArrayPushBack(&b->OutOfRangeGuns, &g);

	b = StrBulletClass("molotov");
	g = StrGunDescription("fire_explosion");
	CArrayInit(&b->OutOfRangeGuns, sizeof(const GunDescription *));
	CArrayPushBack(&b->OutOfRangeGuns, &g);
	CArrayInit(&b->HitGuns, sizeof(const GunDescription *));
	CArrayPushBack(&b->HitGuns, &g);
	CArrayInit(&b->Falling.DropGuns, sizeof(const GunDescription *));
	CArrayPushBack(&b->Falling.DropGuns, &g);

	b = StrBulletClass("gasbomb");
	CArrayInit(&b->OutOfRangeGuns, sizeof(const GunDescription *));
	g = StrGunDescription("gas_poison_explosion");
	CArrayPushBack(&b->OutOfRangeGuns, &g);

	b = StrBulletClass("confusebomb");
	CArrayInit(&b->OutOfRangeGuns, sizeof(const GunDescription *));
	g = StrGunDescription("gas_confuse_explosion");
	CArrayPushBack(&b->OutOfRangeGuns, &g);

	b = StrBulletClass("proxmine");
	CArrayInit(&b->OutOfRangeGuns, sizeof(const GunDescription *));
	g = StrGunDescription("activemine");
	CArrayPushBack(&b->OutOfRangeGuns, &g);

	b = StrBulletClass("dynamite");
	CArrayInit(&b->OutOfRangeGuns, sizeof(const GunDescription *));
	g = StrGunDescription("explosion1");
	CArrayPushBack(&b->OutOfRangeGuns, &g);
	g = StrGunDescription("explosion2");
	CArrayPushBack(&b->OutOfRangeGuns, &g);
	g = StrGunDescription("explosion3");
	CArrayPushBack(&b->OutOfRangeGuns, &g);

	b = StrBulletClass("activemine");
	CArrayInit(&b->ProximityGuns, sizeof(const GunDescription *));
	g = StrGunDescription("triggeredmine");
	CArrayPushBack(&b->ProximityGuns, &g);

	b = StrBulletClass("triggeredmine");
	CArrayInit(&b->OutOfRangeGuns, sizeof(const GunDescription *));
	g = StrGunDescription("explosion1");
	CArrayPushBack(&b->OutOfRangeGuns, &g);
	g = StrGunDescription("explosion2");
	CArrayPushBack(&b->OutOfRangeGuns, &g);
	g = StrGunDescription("explosion3");
	CArrayPushBack(&b->OutOfRangeGuns, &g);
}
void BulletTerminate(CArray *bullets)
{
	for (int i = 0; i < (int)bullets->size; i++)
	{
		BulletClass *b = CArrayGet(bullets, i);
		CFREE(b->Name);
		CArrayTerminate(&b->OutOfRangeGuns);
		CArrayTerminate(&b->HitGuns);
		CArrayTerminate(&b->Falling.DropGuns);
		CArrayTerminate(&b->ProximityGuns);
	}
	CArrayTerminate(bullets);
}

void FireballAdd(const AddFireball e)
{
	TMobileObject *obj =
		CArrayGet(&gMobObjs, MobObjAdd(e.FullPos, e.PlayerIndex));
	obj->bulletClass = e.Class;
	obj->vel = Vec2iFull2Real(Vec2iScale(
		GetFullVectorsForRadians(e.Angle),
		RAND_INT(e.Class->SpeedLow, e.Class->SpeedHigh)));
	obj->dz = e.DZ;
	obj->updateFunc = UpdateBullet;
	obj->tileItem.drawFunc = NULL;
	obj->tileItem.getPicFunc = NULL;
	obj->tileItem.CPic = e.Class->CPic;
	obj->tileItem.CPicFunc = e.Class->CPicFunc;
	obj->tileItem.w = e.Class->Size.x;
	obj->tileItem.h = e.Class->Size.y;
	obj->range = RAND_INT(e.Class->RangeLow, e.Class->RangeHigh);
	obj->flags = e.Flags;
}

void BulletAdd(const AddBullet add)
{
	Vec2i pos = add.MuzzlePos;
	if (!Vec2iEqual(add.BulletClass->Size, Vec2iZero()))
	{
		const int maxSize = MAX(
			add.BulletClass->Size.x, add.BulletClass->Size.y);
		double x, y;
		GetVectorsForRadians(add.Angle, &x, &y);
		pos = Vec2iAdd(pos, Vec2iReal2Full(
			Vec2iNew((int)round(x * maxSize), (int)round(y * maxSize))));
	}
	TMobileObject *obj = CArrayGet(&gMobObjs, MobObjAdd(pos, add.PlayerIndex));
	obj->vel = GetFullVectorsForRadians(add.Angle);
	SetBulletProps(
		obj, add.MuzzleHeight, add.Elevation, add.BulletClass, add.Flags);
}