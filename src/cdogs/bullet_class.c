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
#include "json_utils.h"
#include "objs.h"
#include "screen_shake.h"

BulletClasses gBulletClasses;


// TODO: use map structure?
BulletClass *StrBulletClass(const char *s)
{
	if (s == NULL || strlen(s) == 0)
	{
		return NULL;
	}
	for (int i = 0; i < (int)gBulletClasses.Classes.size; i++)
	{
		BulletClass *b = CArrayGet(&gBulletClasses.Classes, i);
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
	obj->tileItem.CPicFunc = GetBulletDrawContext;
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
		bool hasDropped = obj->z <= 0;
		for (int i = 0; i < ticks; i++)
		{
			obj->z += obj->dz;
			if (obj->z <= 0)
			{
				obj->z = 0;
				if (obj->bulletClass->Falling.Bounces)
				{
					obj->dz = -obj->dz / 2;
				}
				else
				{
					obj->dz = 0;
				}
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
				e.u.SoundAt.Sound = obj->bulletClass->HitSound.Wall;
				e.u.SoundAt.Pos = realPos;
				GameEventsEnqueue(&gGameEvents, e);
			}
			else
			{
				obj->dz -= obj->bulletClass->Falling.GravityFactor;
			}
			if (!obj->bulletClass->Falling.FallsDown)
			{
				obj->dz = MAX(0, obj->dz);
			}
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
		e.u.SoundAt.Sound = obj->bulletClass->HitSound.Wall;
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


#define VERSION 1
static void LoadBullet(
	BulletClass *b, json_t *node, const BulletClass *defaultBullet);
void BulletInitialize(BulletClasses *bullets, const char *filename)
{
	CArrayInit(&bullets->Classes, sizeof(BulletClass));
	FILE *f = fopen(filename, "r");
	json_t *root = NULL;
	if (f == NULL)
	{
		printf("Error: cannot load bullets file %s\n", filename);
		goto bail;
	}
	const enum json_error e = json_stream_parse(f, &root);
	if (e != JSON_OK)
	{
		printf("Error parsing bullets file %s\n", filename);
		goto bail;
	}
	int version;
	LoadInt(&version, root, "Version");
	if (version > VERSION || version <= 0)
	{
		CASSERT(false, "cannot read bullets file version");
		goto bail;
	}

	// Defaults
	BulletClass defaultB;
	LoadBullet(
		&defaultB, json_find_first_label(root, "DefaultBullet")->child, NULL);
	json_t *bulletsNode = json_find_first_label(root, "Bullets")->child;
	for (json_t *child = bulletsNode->child; child; child = child->next)
	{
		BulletClass b;
		LoadBullet(&b, child, &defaultB);
		CArrayPushBack(&bullets->Classes, &b);
	}

bail:
	bullets->root = root;
	if (f)
	{
		fclose(f);
	}
}
static void LoadBullet(
	BulletClass *b, json_t *node, const BulletClass *defaultBullet)
{
	if (defaultBullet != NULL)
	{
		memcpy(b, defaultBullet, sizeof *b);
	}
	else
	{
		memset(b, 0, sizeof *b);
	}
	char *tmp;

	if (json_find_first_label(node, "Name"))
	{
		b->Name = GetString(node, "Name");
	}
	if (json_find_first_label(node, "Pic"))
	{
		json_t *pic = json_find_first_label(node, "Pic")->child;
		tmp = GetString(pic, "Type");
		b->CPic.Type = StrPicType(tmp);
		CFREE(tmp);
		switch (b->CPic.Type)
		{
		case PICTYPE_NORMAL:
			tmp = GetString(pic, "Pic");
			b->CPic.u.Pic = PicManagerGetPic(&gPicManager, tmp);
			CFREE(tmp);
			break;
		case PICTYPE_DIRECTIONAL:
			tmp = GetString(pic, "Sprites");
			b->CPic.u.Sprites =
				&PicManagerGetSprites(&gPicManager, tmp)->pics;
			CFREE(tmp);
			break;
		case PICTYPE_ANIMATED:	// fallthrough
		case PICTYPE_ANIMATED_RANDOM:
			tmp = GetString(pic, "Sprites");
			b->CPic.u.Animated.Sprites =
				&PicManagerGetSprites(&gPicManager, tmp)->pics;
			CFREE(tmp);
			LoadInt(&b->CPic.u.Animated.Count, pic, "Count");
			LoadInt(&b->CPic.u.Animated.TicksPerFrame, pic, "TicksPerFrame");
			break;
		default:
			CASSERT(false, "unknown pic type");
			break;
		}
		b->CPic.UseMask = true;
		b->CPic.u1.Mask = colorWhite;
		if (json_find_first_label(pic, "Mask"))
		{
			tmp = GetString(pic, "Mask");
			b->CPic.u1.Mask = StrColor(tmp);
			CFREE(tmp);
		}
		else if (json_find_first_label(pic, "Tint"))
		{
			b->CPic.UseMask = false;
			json_t *tint = json_find_first_label(pic, "Tint")->child->child;
			b->CPic.u1.Tint.h = atof(tint->text);
			tint = tint->next;
			b->CPic.u1.Tint.s = atof(tint->text);
			tint = tint->next;
			b->CPic.u1.Tint.v = atof(tint->text);
			tint = tint->next;
		}
	}
	LoadVec2i(&b->ShadowSize, node, "ShadowSize");
	LoadInt(&b->Delay, node, "Delay");
	if (json_find_first_label(node, "Speed"))
	{
		LoadInt(&b->SpeedLow, node, "Speed");
		b->SpeedHigh = b->SpeedLow;
	}
	if (json_find_first_label(node, "SpeedLow"))
	{
		LoadInt(&b->SpeedLow, node, "SpeedLow");
	}
	if (json_find_first_label(node, "SpeedHigh"))
	{
		LoadInt(&b->SpeedHigh, node, "SpeedHigh");
	}
	b->SpeedLow = MIN(b->SpeedLow, b->SpeedHigh);
	b->SpeedHigh = MAX(b->SpeedLow, b->SpeedHigh);
	LoadBool(&b->SpeedScale, node, "SpeedScale");
	LoadVec2i(&b->Friction, node, "Friction");
	if (json_find_first_label(node, "Range"))
	{
		LoadInt(&b->RangeLow, node, "Range");
		b->RangeHigh = b->RangeLow;
	}
	if (json_find_first_label(node, "RangeLow"))
	{
		LoadInt(&b->RangeLow, node, "RangeLow");
	}
	if (json_find_first_label(node, "RangeHigh"))
	{
		LoadInt(&b->RangeHigh, node, "RangeHigh");
	}
	b->RangeLow = MIN(b->RangeLow, b->RangeHigh);
	b->RangeHigh = MAX(b->RangeLow, b->RangeHigh);
	LoadInt(&b->Power, node, "Power");
	LoadVec2i(&b->Size, node, "Size");
	if (json_find_first_label(node, "Special"))
	{
		tmp = GetString(node, "Special");
		b->Special = StrSpecialDamage(tmp);
		CFREE(tmp);
	}
	LoadBool(&b->HurtAlways, node, "HurtAlways");
	LoadBool(&b->Persists, node, "Persists");
	if (json_find_first_label(node, "Spark"))
	{
		tmp = GetString(node, "Spark");
		b->Spark = ParticleClassGet(&gParticleClasses, tmp);
		CFREE(tmp);
	}
	if (json_find_first_label(node, "HitSounds"))
	{
		json_t *hitSounds = json_find_first_label(node, "HitSounds")->child;
		if (json_find_first_label(hitSounds, "Object"))
		{
			tmp = GetString(hitSounds, "Object");
			b->HitSound.Object = StrSound(tmp);
			CFREE(tmp);
		}
		if (json_find_first_label(hitSounds, "Flesh"))
		{
			tmp = GetString(hitSounds, "Flesh");
			b->HitSound.Flesh = StrSound(tmp);
			CFREE(tmp);
		}
		if (json_find_first_label(hitSounds, "Wall"))
		{
			tmp = GetString(hitSounds, "Wall");
			b->HitSound.Wall = StrSound(tmp);
			CFREE(tmp);
		}
	}
	LoadBool(&b->WallBounces, node, "WallBounces");
	LoadBool(&b->HitsObjects, node, "HitsObjects");
	if (json_find_first_label(node, "Falling"))
	{
		json_t *falling = json_find_first_label(node, "Falling")->child;
		LoadInt(&b->Falling.GravityFactor, falling, "GravityFactor");
		LoadBool(&b->Falling.FallsDown, falling, "FallsDown");
		LoadBool(&b->Falling.DestroyOnDrop, falling, "DestroyOnDrop");
		LoadBool(&b->Falling.Bounces, falling, "Bounces");
	}
	LoadInt(&b->SeekFactor, node, "SeekFactor");
	LoadBool(&b->Erratic, node, "Erratic");

	b->node = node;
}
static void LoadBulletGuns(CArray *guns, json_t *node);
void BulletInitialize2(BulletClasses *bullets)
{
	for (int i = 0; i < (int)bullets->Classes.size; i++)
	{
		BulletClass *b = CArrayGet(&bullets->Classes, i);
		if (json_find_first_label(b->node, "Falling"))
		{
			json_t *falling = json_find_first_label(b->node, "Falling")->child;
			LoadBulletGuns(
				&b->Falling.DropGuns,
				json_find_first_label(falling, "DropGuns"));
		}
		LoadBulletGuns(
			&b->OutOfRangeGuns,
			json_find_first_label(b->node, "OutOfRangeGuns"));
		LoadBulletGuns(
			&b->HitGuns,
			json_find_first_label(b->node, "HitGuns"));
		LoadBulletGuns(
			&b->ProximityGuns,
			json_find_first_label(b->node, "ProximityGuns"));
	}
	json_free_value(&bullets->root);
}
static void LoadBulletGuns(CArray *guns, json_t *node)
{
	if (node == NULL || node->child == NULL)
	{
		return;
	}
	CArrayInit(guns, sizeof(const GunDescription *));
	for (json_t *gun = node->child->child; gun; gun = gun->next)
	{
		const GunDescription *g = StrGunDescription(gun->text);
		CArrayPushBack(guns, &g);
	}
}
void BulletTerminate(BulletClasses *bullets)
{
	for (int i = 0; i < (int)bullets->Classes.size; i++)
	{
		BulletClass *b = CArrayGet(&bullets->Classes, i);
		CFREE(b->Name);
		CArrayTerminate(&b->OutOfRangeGuns);
		CArrayTerminate(&b->HitGuns);
		CArrayTerminate(&b->Falling.DropGuns);
		CArrayTerminate(&b->ProximityGuns);
	}
	CArrayTerminate(&bullets->Classes);
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