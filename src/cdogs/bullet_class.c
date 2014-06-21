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
		pos.y -= obj->z / Z_FACTOR;
		BlitMasked(
			&gGraphicsDevice,
			data->u.Bullet.Pic,
			pos, data->u.Bullet.Mask, true);
	}
	else
	{
		const TOffsetPic *pic = &cGeneralPics[data->u.Bullet.Ofspic];
		pos = Vec2iAdd(pos, Vec2iNew(pic->dx, pic->dy));
		pos.y -= obj->z / Z_FACTOR;
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
		pos.y -= obj->z / Z_FACTOR;
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
	offset->y = pic->dy - obj->z / Z_FACTOR;
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
		offset->y = -p->size.y / 2;
	}
	else
	{
		const TOffsetPic *pic = &cBeamPics[obj->bulletClass->Beam.Beam][dir];
		p = PicManagerGetFromOld(&gPicManager, pic->picIndex);
		offset->x = pic->dx;
		offset->y = pic->dy;
	}
	offset->y -= obj->z / Z_FACTOR;
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
		pos.y -= obj->z / Z_FACTOR;
	}
	BlitMasked(
		&gGraphicsDevice,
		PicManagerGetFromOld(&gPicManager, pic->picIndex),
		Vec2iAdd(pos, Vec2iNew(pic->dx, pic->dy)), data->u.GrenadeColor, 1);
}

static void DrawGasCloud(const Vec2i pos, const TileItemDrawFuncData *data)
{
	const TMobileObject *obj = CArrayGet(&gMobObjs, data->MobObjId);
	CASSERT(obj->isInUse, "Cannot draw non-existent mobobj");
	const TOffsetPic *pic = &cFireBallPics[8 + (obj->state.frame & 3)];
	DrawBTPic(
		&gGraphicsDevice,
		PicManagerGetFromOld(&gPicManager, pic->picIndex),
		Vec2iNew(pos.x + pic->dx, pos.y + pic->dy - obj->z),
		&obj->bulletClass->Tint);
}

static void DrawFireball(Vec2i pos, TileItemDrawFuncData *data)
{
	const TMobileObject *obj = CArrayGet(&gMobObjs, data->MobObjId);
	const int frame = obj->count - obj->bulletClass->Delay - obj->state.frame;
	if (frame < 0)
	{
		return;
	}
	const TOffsetPic *pic = &cFireBallPics[MIN(FIREBALL_MAX - 1, frame / 4)];
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


static void SetBulletProps(
	TMobileObject *obj, int z, const int dz, BulletType type, int flags)
{
	const BulletClass *b = &gBulletClasses[type];
	obj->bulletClass = b;
	obj->updateFunc = UpdateBullet;
	obj->tileItem.getPicFunc = b->GetPicFunc;
	obj->tileItem.getActorPicsFunc = NULL;
	obj->tileItem.drawFunc = b->DrawFunc;
	obj->tileItem.drawData.u = b->DrawData.u;
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


void BulletInitialize(void)
{
	// Defaults
	BulletClass *b;
	for (int i = 0; i < BULLET_COUNT; i++)
	{
		b = &gBulletClasses[i];
		memset(b, 0, sizeof *b);
		b->Type = (BulletType)i;
		b->GetPicFunc = NULL;
		b->DrawFunc = NULL;
		b->Beam.Sprites = NULL;
		b->Tint = tintNone;
		b->SpeedScale = false;
		b->Friction = Vec2iZero();
		b->Size = Vec2iZero();
		b->Special = SPECIAL_NONE;
		b->HurtAlways = false;
		b->Persists = false;
		b->Spark = ParticleClassGet(&gParticleClasses, "spark");
		b->WallHitSound = StrSound("ricochet");
		b->WallBounces = false;
		b->HitsObjects = true;
		b->Falling.GravityFactor = 0;
		b->Falling.Type = FALLING_TYPE_BOUNCE;
		b->Falling.DestroyOnDrop = false;
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
	b->HurtAlways = true;


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
	b->Falling.GravityFactor = 1;

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
	b->Falling.GravityFactor = 1;

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
	b->Falling.GravityFactor = 1;
	b->Falling.DestroyOnDrop = true;

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
	b->Falling.GravityFactor = 1;

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
	b->Falling.GravityFactor = 1;

	b = &gBulletClasses[BULLET_GAS];
	b->Name = "gas";
	b->DrawFunc = DrawGasCloud;
	b->Tint = tintPoison;
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


	b = &gBulletClasses[BULLET_FIREBALL_WRECK];
	b->Name = "fireball_wreck";
	b->DrawFunc = (TileItemDrawFunc)DrawFireball;
	b->Delay = -10;
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
	b->HurtAlways = true;
	b->Persists = true;
	b->Spark = NULL;
	b->WallHitSound = NULL;
	b->Falling.GravityFactor = 1;
	b->Falling.Type = FALLING_TYPE_DZ;

	b = &gBulletClasses[BULLET_FIREBALL2];
	b->Name = "fireball2";
	b->DrawFunc = (TileItemDrawFunc)DrawFireball;
	b->Delay = 8;
	b->SpeedLow = b->SpeedHigh = 192;
	b->RangeLow = b->RangeHigh = FIREBALL_MAX * 4 - 1 + b->Delay;
	b->Power = FIREBALL_POWER;
	b->Size = Vec2iNew(7, 5);
	b->Special = SPECIAL_EXPLOSION;
	b->HurtAlways = true;
	b->Persists = true;
	b->Spark = NULL;
	b->WallHitSound = NULL;
	b->Falling.GravityFactor = 1;
	b->Falling.Type = FALLING_TYPE_DZ;

	b = &gBulletClasses[BULLET_FIREBALL3];
	b->Name = "fireball3";
	b->DrawFunc = (TileItemDrawFunc)DrawFireball;
	b->Delay = 16;
	b->SpeedLow = b->SpeedHigh = 128;
	b->RangeLow = b->RangeHigh = FIREBALL_MAX * 4 - 1 + b->Delay;
	b->Power = FIREBALL_POWER;
	b->Size = Vec2iNew(7, 5);
	b->Special = SPECIAL_EXPLOSION;
	b->HurtAlways = true;
	b->Persists = true;
	b->Spark = NULL;
	b->WallHitSound = NULL;
	b->Falling.GravityFactor = 1;
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
	b->HurtAlways = true;
	b->Persists = true;
	b->Spark = NULL;
	b->WallHitSound = NULL;
	b->WallBounces = true;
	b->Falling.GravityFactor = 6;
	b->Falling.Type = FALLING_TYPE_Z;
	b->RandomAnimation = true;

	b = &gBulletClasses[BULLET_GAS_CLOUD_POISON];
	b->Name = "gas_cloud_poison";
	b->DrawFunc = DrawGasCloud;
	b->Tint = tintPoison;
	b->SpeedLow = 0;
	b->SpeedHigh = 255;
	b->Friction = Vec2iNew(4, 3);
	b->RangeLow = 48 * 4 - 1;
	b->RangeHigh = (48 - 8 - 1) * 4 - 1;
	b->Power = 0;
	b->Size = Vec2iNew(10, 10);
	b->Special = SPECIAL_POISON;
	b->HurtAlways = true;
	b->Persists = true;
	b->Spark = NULL;
	b->WallHitSound = StrSound("hit_gas");
	b->WallBounces = true;
	b->RandomAnimation = true;

	b = &gBulletClasses[BULLET_GAS_CLOUD_CONFUSE];
	b->Name = "gas_cloud_confuse";
	b->DrawFunc = DrawGasCloud;
	b->Tint = tintPurple;
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
}
void BulletInitialize2(void)
{
	BulletClass *b;
	const GunDescription *g;
	
	b = &gBulletClasses[BULLET_GRENADE];
	CArrayInit(&b->OutOfRangeGuns, sizeof(const GunDescription *));
	g = StrGunDescription("explosion1");
	CArrayPushBack(&b->OutOfRangeGuns, &g);
	g = StrGunDescription("explosion2");
	CArrayPushBack(&b->OutOfRangeGuns, &g);
	g = StrGunDescription("explosion3");
	CArrayPushBack(&b->OutOfRangeGuns, &g);
	
	b = &gBulletClasses[BULLET_SHRAPNELBOMB];
	CArrayInit(&b->OutOfRangeGuns, sizeof(const GunDescription *));
	g = StrGunDescription("frag_explosion");
	CArrayPushBack(&b->OutOfRangeGuns, &g);

	b = &gBulletClasses[BULLET_MOLOTOV];
	g = StrGunDescription("fire_explosion");
	CArrayInit(&b->OutOfRangeGuns, sizeof(const GunDescription *));
	CArrayPushBack(&b->OutOfRangeGuns, &g);
	CArrayInit(&b->HitGuns, sizeof(const GunDescription *));
	CArrayPushBack(&b->HitGuns, &g);
	CArrayInit(&b->Falling.DropGuns, sizeof(const GunDescription *));
	CArrayPushBack(&b->Falling.DropGuns, &g);

	b = &gBulletClasses[BULLET_GASBOMB];
	CArrayInit(&b->OutOfRangeGuns, sizeof(const GunDescription *));
	g = StrGunDescription("gas_poison_explosion");
	CArrayPushBack(&b->OutOfRangeGuns, &g);

	b = &gBulletClasses[BULLET_CONFUSEBOMB];
	CArrayInit(&b->OutOfRangeGuns, sizeof(const GunDescription *));
	g = StrGunDescription("gas_confuse_explosion");
	CArrayPushBack(&b->OutOfRangeGuns, &g);

	b = &gBulletClasses[BULLET_PROXMINE];
	CArrayInit(&b->OutOfRangeGuns, sizeof(const GunDescription *));
	g = StrGunDescription("activemine");
	CArrayPushBack(&b->OutOfRangeGuns, &g);

	b = &gBulletClasses[BULLET_DYNAMITE];
	CArrayInit(&b->OutOfRangeGuns, sizeof(const GunDescription *));
	g = StrGunDescription("explosion1");
	CArrayPushBack(&b->OutOfRangeGuns, &g);
	g = StrGunDescription("explosion2");
	CArrayPushBack(&b->OutOfRangeGuns, &g);
	g = StrGunDescription("explosion3");
	CArrayPushBack(&b->OutOfRangeGuns, &g);

	b = &gBulletClasses[BULLET_ACTIVEMINE];
	CArrayInit(&b->ProximityGuns, sizeof(const GunDescription *));
	g = StrGunDescription("triggeredmine");
	CArrayPushBack(&b->ProximityGuns, &g);

	b = &gBulletClasses[BULLET_TRIGGEREDMINE];
	CArrayInit(&b->OutOfRangeGuns, sizeof(const GunDescription *));
	g = StrGunDescription("explosion1");
	CArrayPushBack(&b->OutOfRangeGuns, &g);
	g = StrGunDescription("explosion2");
	CArrayPushBack(&b->OutOfRangeGuns, &g);
	g = StrGunDescription("explosion3");
	CArrayPushBack(&b->OutOfRangeGuns, &g);
}
void BulletTerminate(void)
{
	for (int i = 0; i < BULLET_COUNT; i++)
	{
		BulletClass *b = &gBulletClasses[i];
		CArrayTerminate(&b->OutOfRangeGuns);
		CArrayTerminate(&b->HitGuns);
		CArrayTerminate(&b->Falling.DropGuns);
		CArrayTerminate(&b->ProximityGuns);
	}
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
	obj->tileItem.drawFunc = e.Class->DrawFunc;
	obj->tileItem.getPicFunc = e.Class->GetPicFunc;
	obj->tileItem.w = e.Class->Size.x;
	obj->tileItem.h = e.Class->Size.y;
	obj->range = RAND_INT(e.Class->RangeLow, e.Class->RangeHigh);
	obj->flags = e.Flags;
}

void BulletAdd(const AddBullet add)
{
	Vec2i pos = add.MuzzlePos;
	const BulletClass *b = &gBulletClasses[add.Bullet];
	if (!Vec2iEqual(b->Size, Vec2iZero()))
	{
		const int maxSize = MAX(b->Size.x, b->Size.y);
		double x, y;
		GetVectorsForRadians(add.Angle, &x, &y);
		pos = Vec2iAdd(pos, Vec2iReal2Full(
			Vec2iNew((int)round(x * maxSize), (int)round(y * maxSize))));
	}
	TMobileObject *obj = CArrayGet(&gMobObjs, MobObjAdd(pos, add.PlayerIndex));
	obj->vel = GetFullVectorsForRadians(add.Angle);
	SetBulletProps(
		obj, add.MuzzleHeight, add.Elevation, add.Bullet, add.Flags);
}