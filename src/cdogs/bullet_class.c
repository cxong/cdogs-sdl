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

    Copyright (c) 2013-2017, Cong Xu
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
#include "collision/collision.h"
#include "draw/drawtools.h"
#include "game_events.h"
#include "json_utils.h"
#include "log.h"
#include "net_util.h"
#include "objs.h"
#include "screen_shake.h"

BulletClasses gBulletClasses;

#define SPECIAL_LOCK 12


// TODO: use map structure?
BulletClass *StrBulletClass(const char *s)
{
	if (s == NULL || strlen(s) == 0)
	{
		return NULL;
	}
	CA_FOREACH(BulletClass, b, gBulletClasses.CustomClasses)
		if (strcmp(s, b->Name) == 0)
		{
			return b;
		}
	CA_FOREACH_END()
	CA_FOREACH(BulletClass, b, gBulletClasses.Classes)
		if (strcmp(s, b->Name) == 0)
		{
			return b;
		}
	CA_FOREACH_END()
	CASSERT(false, "cannot parse bullet name");
	return NULL;
}

// Draw functions

static CPicDrawContext GetBulletDrawContext(const int id)
{
	const TMobileObject *obj = CArrayGet(&gMobObjs, id);
	CASSERT(obj->isInUse, "Cannot draw non-existent mobobj");
	// Calculate direction based on velocity
	const direction_e dir =
		RadiansToDirection(Vec2iToRadians(obj->tileItem.VelFull));
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


static Vec2i SeekTowards(
	const Vec2i pos, const Vec2i vel, const double speedMin,
	const Vec2i targetPos, const int seekFactor)
{
	// Compensate for bullet's velocity
	const Vec2i targetVel = Vec2iMinus(Vec2iMinus(targetPos, pos), vel);
	// Don't seek if the coordinates are too big
	if (abs(targetVel.x) > 10000 || abs(targetVel.y) > 10000 ||
		Vec2iIsZero(targetVel))
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
typedef struct
{
	HitType Type;
	Vec2i Pos;
	Vec2i Normal;
} HitResult;
static HitResult HitItem(
	TMobileObject *obj, const Vec2i pos, const bool multipleHits);
bool UpdateBullet(struct MobileObject *obj, const int ticks)
{
	TileItemUpdate(&obj->tileItem, ticks);
	obj->count += ticks;
	obj->specialLock = MAX(0, obj->specialLock - ticks);
	if (obj->count < obj->bulletClass->Delay)
	{
		return true;
	}

	if (obj->range >= 0 && obj->count > obj->range)
	{
		if (!gCampaign.IsClient)
		{
			FireGuns(obj, &obj->bulletClass->OutOfRangeGuns);
		}
		if (obj->bulletClass->OutOfRangeSpark != NULL)
		{
			GameEvent s = GameEventNew(GAME_EVENT_ADD_PARTICLE);
			s.u.AddParticle.Class = obj->bulletClass->OutOfRangeSpark;
			s.u.AddParticle.FullPos = Vec2iNew(obj->x, obj->y);
			s.u.AddParticle.Z = obj->z;
			GameEventsEnqueue(&gGameEvents, s);
		}
		return false;
	}

	const Vec2i posStart = Vec2iNew(obj->x, obj->y);

	if (obj->bulletClass->SeekFactor > 0)
	{
		// Find the closest target to this bullet and steer towards it
		const TActor *owner = ActorGetByUID(obj->ActorUID);
		if (owner == NULL)
		{
			return false;
		}
		const TActor *target = AIGetClosestEnemy(posStart, owner, obj->flags);
		if (target && !target->dead)
		{
			for (int i = 0; i < ticks; i++)
			{
				obj->tileItem.VelFull = SeekTowards(
					posStart, obj->tileItem.VelFull,
					obj->bulletClass->SpeedLow, target->Pos,
					obj->bulletClass->SeekFactor);
			}
		}
	}

	HitResult hit = { HIT_NONE, Vec2iZero(), Vec2iZero() };
	if (!gCampaign.IsClient)
	{
		hit = HitItem(obj, posStart, obj->bulletClass->Persists);
	}
	Vec2i pos =
		Vec2iScale(Vec2iAdd(posStart, obj->tileItem.VelFull), ticks);

	if (hit.Type != HIT_NONE)
	{
		GameEvent b = GameEventNew(GAME_EVENT_BULLET_BOUNCE);
		b.u.BulletBounce.UID = obj->UID;
		b.u.BulletBounce.HitType = (int)hit.Type;
		bool alive = true;
		if ((hit.Type == HIT_WALL && !obj->bulletClass->WallBounces) ||
			((hit.Type == HIT_OBJECT || hit.Type == HIT_FLESH) &&
				obj->bulletClass->HitsObjects))
		{
			b.u.BulletBounce.Spark = true;
			CASSERT(!gCampaign.IsClient, "Cannot process bounces as client");
			FireGuns(obj, &obj->bulletClass->HitGuns);
			if (hit.Type == HIT_WALL || !obj->bulletClass->Persists)
			{
				alive = false;
			}
		}
		const Vec2i hitPos = hit.Type != HIT_NONE ? hit.Pos : pos;
		b.u.BulletBounce.BouncePos = Vec2i2Net(hitPos);
		b.u.BulletBounce.Pos = Vec2i2Net(pos);
		b.u.BulletBounce.Vel = Vec2i2Net(obj->tileItem.VelFull);
		if (hit.Type == HIT_WALL && !Vec2iIsZero(obj->tileItem.VelFull))
		{
			// Bouncing
			GetWallBouncePosVelFull(
				posStart, obj->tileItem.VelFull, hit.Pos, hit.Normal,
				&pos, &obj->tileItem.VelFull);
			b.u.BulletBounce.Pos = Vec2i2Net(pos);
			b.u.BulletBounce.Vel = Vec2i2Net(obj->tileItem.VelFull);
		}
		GameEventsEnqueue(&gGameEvents, b);
		if (!alive)
		{
			return false;
		}
	}

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
					if (!gCampaign.IsClient)
					{
						FireGuns(obj, &obj->bulletClass->Falling.DropGuns);
					}
				}
				hasDropped = true;
				if (obj->bulletClass->Falling.DestroyOnDrop)
				{
					return false;
				}
				SoundPlayAt(
					&gSoundDevice,
					StrSound(obj->bulletClass->HitSound.Wall), realPos);
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
	const bool isDiagonal =
		obj->tileItem.VelFull.x != 0 && obj->tileItem.VelFull.y != 0;
	const int frictionComponent = isDiagonal ?
		(int)round(obj->bulletClass->Friction / sqrt(2)) :
		obj->bulletClass->Friction;
	for (int i = 0; i < ticks; i++)
	{
		if (obj->tileItem.VelFull.x > 0)
		{
			obj->tileItem.VelFull.x -= frictionComponent;
		}
		else if (obj->tileItem.VelFull.x < 0)
		{
			obj->tileItem.VelFull.x += frictionComponent;
		}

		if (obj->tileItem.VelFull.y > 0)
		{
			obj->tileItem.VelFull.y -= frictionComponent;
		}
		else if (obj->tileItem.VelFull.y < 0)
		{
			obj->tileItem.VelFull.y += frictionComponent;
		}
	}
	if (!MapTryMoveTileItem(&gMap, &obj->tileItem, realPos))
	{
		obj->count = obj->range;
		return false;
	}
	obj->x = pos.x;
	obj->y = pos.y;

	if (obj->bulletClass->Erratic)
	{
		for (int i = 0; i < ticks; i++)
		{
			obj->tileItem.VelFull.x += ((rand() % 3) - 1) * 128;
			obj->tileItem.VelFull.y += ((rand() % 3) - 1) * 128;
		}
	}

	// Proximity function, destroy
	// Only check proximity every now and then
	if (obj->bulletClass->ProximityGuns.size > 0 && !(obj->count & 3))
	{
		if (!gCampaign.IsClient)
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
	}

	return true;
}
static void FireGuns(const TMobileObject *obj, const CArray *guns)
{
	const Vec2i fullPos = Vec2iNew(obj->x, obj->y);
	const double angle = Vec2iToRadians(obj->tileItem.VelFull);
	for (int i = 0; i < (int)guns->size; i++)
	{
		const GunDescription **g = CArrayGet(guns, i);
		GunFire(
			*g, fullPos, obj->z, angle, obj->flags, obj->PlayerUID, obj->ActorUID,
			true, false);
	}
}
typedef struct
{
	HitType HitType;
	bool MultipleHits;
	TMobileObject *Obj;
	union
	{
		TTileItem *Target;
		Vec2i TilePos;
	} u;
	Vec2i ColPos;
	Vec2i ColNormal;
	int ColPosDistRealSquared;
} HitItemData;
static bool HitItemFunc(
	TTileItem *ti, void *data, const Vec2i colA, const Vec2i colB,
	const Vec2i normal);
static bool CheckWall(const Vec2i tilePos);
static bool HitWallFunc(
	const Vec2i tilePos, void *data, const Vec2i col, const Vec2i normal);
static void OnHit(HitItemData *data, TTileItem *target);
static HitResult HitItem(
	TMobileObject *obj, const Vec2i pos, const bool multipleHits)
{
	// Get all items that collide
	HitItemData data;
	data.HitType = HIT_NONE;
	data.MultipleHits = multipleHits;
	data.Obj = obj;
	data.ColPos = pos;
	data.ColNormal = Vec2iZero();
	data.ColPosDistRealSquared = -1;
	const CollisionParams params =
	{
		TILEITEM_CAN_BE_SHOT, COLLISIONTEAM_NONE, IsPVP(gCampaign.Entry.Mode)
	};
	OverlapTileItems(
		&obj->tileItem, pos,
		obj->tileItem.size, params, HitItemFunc, &data,
		CheckWall, HitWallFunc, &data);
	if (!multipleHits && data.ColPosDistRealSquared >= 0)
	{
		if (data.HitType == HIT_OBJECT || data.HitType == HIT_FLESH)
		{
			OnHit(&data, data.u.Target);
		}
	}
	HitResult hit = { data.HitType, data.ColPos, data.ColNormal };
	return hit;
}
static HitType GetHitType(
	const TTileItem *ti, const TMobileObject *bullet, int *targetUID);
static void SetClosestCollision(
	HitItemData *data, const Vec2i col, const Vec2i normal, HitType ht,
	TTileItem *target, const Vec2i tilePos);
static bool HitItemFunc(
	TTileItem *ti, void *data, const Vec2i colA, const Vec2i colB,
	const Vec2i normal)
{
	UNUSED(colB);
	HitItemData *hData = data;

	// Check bullet-to-other collisions
	if (!CanHit(hData->Obj->flags, hData->Obj->ActorUID, ti))
	{
		goto bail;
	}

	// If we can hit multiple targets, just process those hits immediately
	// Otherwise, find the closest target and only process the hit for that one
	// at the end.
	if (hData->MultipleHits)
	{
		OnHit(hData, ti);
	}
	else
	{
		SetClosestCollision(
			hData, colA, normal, GetHitType(ti, hData->Obj, NULL),
			ti, Vec2iZero());
	}

bail:
	return true;
}
static HitType GetHitType(
	const TTileItem *ti, const TMobileObject *bullet, int *targetUID)
{
	int tUID = -1;
	HitType ht = HIT_NONE;
	switch (ti->kind)
	{
	case KIND_CHARACTER:
		ht = HIT_FLESH;
		tUID = ((const TActor *)CArrayGet(&gActors, ti->id))->uid;
		break;
	case KIND_OBJECT:
		ht = HIT_OBJECT;
		tUID = ((const TObject *)CArrayGet(&gObjs, ti->id))->uid;
		break;
	default:
		CASSERT(false, "cannot damage target kind");
		break;
	}
	if (bullet->tileItem.SoundLock > 0 ||
		!HasHitSound(
			bullet->flags, bullet->PlayerUID, ti->kind, tUID,
			bullet->bulletClass->Special, true))
	{
		ht = HIT_NONE;
	}
	if (targetUID != NULL)
	{
		*targetUID = tUID;
	}
	return ht;
}
static bool CheckWall(const Vec2i tilePos)
{
	const Tile *t = MapGetTile(&gMap, tilePos);
	return t == NULL || t->flags & MAPTILE_NO_SHOOT;
}
static bool HitWallFunc(
	const Vec2i tilePos, void *data, const Vec2i col, const Vec2i normal)
{
	HitItemData *hData = data;

	SetClosestCollision(hData, col, normal, HIT_WALL, NULL, tilePos);

	return true;
}
static void SetClosestCollision(
	HitItemData *data, const Vec2i col, const Vec2i normal, HitType ht,
	TTileItem *target, const Vec2i tilePos)
{
	// Choose the best collision point (i.e. closest to origin)
	const int d2 = DistanceSquared(
		Vec2iFull2Real(col),
		Vec2iFull2Real(Vec2iNew(data->Obj->x, data->Obj->y)));
	if (data->ColPosDistRealSquared < 0 || d2 < data->ColPosDistRealSquared)
	{
		data->ColPos = col;
		data->ColPosDistRealSquared = d2;
		data->ColNormal = normal;
		data->HitType = ht;
		if (ht == HIT_WALL)
		{
			data->u.TilePos = tilePos;
		}
		else
		{
			data->u.Target = target;
		}
	}
}
static void OnHit(HitItemData *data, TTileItem *target)
{
	int targetUID = -1;
	data->HitType = GetHitType(target, data->Obj, &targetUID);
	Damage(
		data->Obj->tileItem.VelFull,
		data->Obj->bulletClass->Power, data->Obj->bulletClass->Mass,
		data->Obj->flags, data->Obj->PlayerUID, data->Obj->ActorUID,
		target->kind, targetUID,
		data->Obj->bulletClass->Special);
	if (data->Obj->tileItem.SoundLock <= 0)
	{
		data->Obj->tileItem.SoundLock += SOUND_LOCK_TILE_OBJECT;
	}
	if (target->SoundLock <= 0)
	{
		target->SoundLock += SOUND_LOCK_TILE_OBJECT;
	}
	if (data->Obj->specialLock <= 0)
	{
		data->Obj->specialLock += SPECIAL_LOCK;
	}
}


#define VERSION 3
static void LoadBullet(
	BulletClass *b, json_t *node, const BulletClass *defaultBullet,
	const int version);
void BulletInitialize(BulletClasses *bullets)
{
	memset(bullets, 0, sizeof *bullets);
	CArrayInit(&bullets->Classes, sizeof(BulletClass));
	CArrayInit(&bullets->CustomClasses, sizeof(BulletClass));
}
static void BulletClassFree(BulletClass *b);
void BulletLoadJSON(
	BulletClasses *bullets, CArray *classes, json_t *bulletNode)
{
	LOG(LM_MAP, LL_DEBUG, "loading bullets");
	int version;
	LoadInt(&version, bulletNode, "Version");
	if (version > VERSION || version <= 0)
	{
		CASSERT(false, "cannot read bullets file version");
		return;
	}

	// Defaults
	json_t *defaultNode = json_find_first_label(bulletNode, "DefaultBullet");
	if (defaultNode != NULL)
	{
		BulletClassFree(&bullets->Default);
		LoadBullet(&bullets->Default, defaultNode->child, NULL, version);
	}

	json_t *bulletsNode = json_find_first_label(bulletNode, "Bullets")->child;
	for (json_t *child = bulletsNode->child; child; child = child->next)
	{
		BulletClass b;
		LoadBullet(&b, child, &bullets->Default, version);
		CArrayPushBack(classes, &b);
	}

	bullets->root = bulletNode;
}
static void LoadHitsound(
	char **hitsound, json_t *node, const char *name, const int version);
static void LoadBullet(
	BulletClass *b, json_t *node, const BulletClass *defaultBullet,
	const int version)
{
	memset(b, 0, sizeof *b);
	if (defaultBullet != NULL)
	{
		memcpy(b, defaultBullet, sizeof *b);
		if (defaultBullet->Name != NULL)
		{
			CSTRDUP(b->Name, defaultBullet->Name);
		}
		if (defaultBullet->HitSound.Object != NULL)
		{
			CSTRDUP(b->HitSound.Object, defaultBullet->HitSound.Object);
		}
		if (defaultBullet->HitSound.Flesh != NULL)
		{
			CSTRDUP(b->HitSound.Flesh, defaultBullet->HitSound.Flesh);
		}
		if (defaultBullet->HitSound.Wall != NULL)
		{
			CSTRDUP(b->HitSound.Wall, defaultBullet->HitSound.Wall);
		}
		// TODO: enable default bullet guns?
		memset(&b->Falling.DropGuns, 0, sizeof b->Falling.DropGuns);
		memset(&b->OutOfRangeGuns, 0, sizeof b->OutOfRangeGuns);
		memset(&b->HitGuns, 0, sizeof b->HitGuns);
		memset(&b->ProximityGuns, 0, sizeof b->ProximityGuns);
	}

	char *tmp;

	tmp = NULL;
	LoadStr(&tmp, node, "Name");
	if (tmp != NULL)
	{
		CFREE(b->Name);
		b->Name = tmp;
	}
	if (json_find_first_label(node, "Pic"))
	{
		CPicLoadJSON(&b->CPic, json_find_first_label(node, "Pic")->child);
	}
	LoadVec2i(&b->ShadowSize, node, "ShadowSize");
	LoadInt(&b->Delay, node, "Delay");
	if (json_find_first_label(node, "Speed"))
	{
		LoadInt(&b->SpeedLow, node, "Speed");
		b->SpeedHigh = b->SpeedLow;
	}
	LoadInt(&b->SpeedLow, node, "SpeedLow");
	LoadInt(&b->SpeedHigh, node, "SpeedHigh");
	b->SpeedLow = MIN(b->SpeedLow, b->SpeedHigh);
	b->SpeedHigh = MAX(b->SpeedLow, b->SpeedHigh);
	LoadBool(&b->SpeedScale, node, "SpeedScale");
	LoadInt(&b->Friction, node, "Friction");
	if (json_find_first_label(node, "Range"))
	{
		LoadInt(&b->RangeLow, node, "Range");
		b->RangeHigh = b->RangeLow;
	}
	LoadInt(&b->RangeLow, node, "RangeLow");
	LoadInt(&b->RangeHigh, node, "RangeHigh");
	b->RangeLow = MIN(b->RangeLow, b->RangeHigh);
	b->RangeHigh = MAX(b->RangeLow, b->RangeHigh);
	LoadInt(&b->Power, node, "Power");

	if (version < 2)
	{
		// Old version default mass = power
		b->Mass = b->Power;
	}
	else
	{
		LoadDouble(&b->Mass, node, "Mass");
	}

	LoadVec2i(&b->Size, node, "Size");
	tmp = NULL;
	LoadStr(&tmp, node, "Special");
	if (tmp != NULL)
	{
		b->Special = StrSpecialDamage(tmp);
		CFREE(tmp);
	}
	LoadBool(&b->HurtAlways, node, "HurtAlways");
	LoadBool(&b->Persists, node, "Persists");
	tmp = NULL;
	LoadStr(&tmp, node, "Spark");
	if (tmp != NULL)
	{
		b->Spark = StrParticleClass(&gParticleClasses, tmp);
		CFREE(tmp);
	}
	tmp = NULL;
	LoadStr(&tmp, node, "OutOfRangeSpark");
	if (tmp != NULL)
	{
		b->OutOfRangeSpark = StrParticleClass(&gParticleClasses, tmp);
		CFREE(tmp);
	}
	if (json_find_first_label(node, "HitSounds"))
	{
		json_t *hitSounds = json_find_first_label(node, "HitSounds")->child;
		LoadHitsound(&b->HitSound.Object, hitSounds, "Object", version);
		LoadHitsound(&b->HitSound.Flesh, hitSounds, "Flesh", version);
		LoadHitsound(&b->HitSound.Wall, hitSounds, "Wall", version);
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

	LOG(LM_MAP, LL_DEBUG,
		"loaded bullet name(%s) shadowSize(%d, %d) delay(%d) speed(%d-%d)...",
		b->Name != NULL ? b->Name : "", b->ShadowSize.x, b->ShadowSize.y,
		b->Delay, b->SpeedLow, b->SpeedHigh);
	LOG(LM_MAP, LL_DEBUG,
		"...speedScale(%s) friction(%d) range(%d-%d) power(%d)...",
		b->SpeedScale ? "true" : "false", b->Friction,
		b->RangeLow, b->RangeHigh, b->Power);
	LOG(LM_MAP, LL_DEBUG,
		"...size(%d, %d) hurtAlways(%s) persists(%s) spark(%s, %s)...",
		b->Size.x, b->Size.y, b->HurtAlways ? "true" : "false",
		b->Persists ? "true" : "false",
		b->Spark != NULL ? b->Spark->Name : "",
		b->OutOfRangeSpark != NULL ? b->OutOfRangeSpark->Name : "");
	LOG(LM_MAP, LL_DEBUG,
		"...hitSounds(object(%s), flesh(%s), wall(%s)) wallBounces(%s)...",
		b->HitSound.Object != NULL ? b->HitSound.Object : "",
		b->HitSound.Flesh != NULL ? b->HitSound.Flesh : "",
		b->HitSound.Wall != NULL ? b->HitSound.Wall : "",
		b->WallBounces ? "true" : "false");
	LOG(LM_MAP, LL_DEBUG,
		"...hitsObjects(%s) gravity(%d) fallsDown(%s) destroyOnDrop(%s)...",
		b->HitsObjects ? "true" : "false", b->Falling.GravityFactor,
		b->Falling.FallsDown ? "true" : "false",
		b->Falling.DestroyOnDrop ? "true" : "false");
	LOG(LM_MAP, LL_DEBUG,
		"...dropGuns(%d) seekFactor(%d) erratic(%s)...",
		(int)b->Falling.DropGuns.size, b->SeekFactor,
		b->Erratic ? "true" : "false");
	LOG(LM_MAP, LL_DEBUG,
		"...outOfRangeGuns(%d) hitGuns(%d) proximityGuns(%d)",
		(int)b->OutOfRangeGuns.size,
		(int)b->HitGuns.size,
		(int)b->ProximityGuns.size);
}
static void LoadHitsound(
	char **hitsound, json_t *node, const char *name, const int version)
{
	CFREE(*hitsound);
	*hitsound = NULL;
	LoadStr(hitsound, node, name);
	if (version < 3)
	{
		// Moved hit_XXX sounds to hits folder
		if (*hitsound != NULL)
		{
			char buf[CDOGS_FILENAME_MAX];
			strcpy(buf, "hits/");
			if (strncmp(*hitsound, "hit_", strlen("hit_")) == 0)
			{
				strcat(buf, *hitsound + strlen("hit_"));
				CFREE(*hitsound);
				CSTRDUP(*hitsound, buf);
			}
			else if (strncmp(*hitsound, "knife_", strlen("knife_")) == 0)
			{
				strcpy(buf, "hits/knife_");
				strcat(buf, *hitsound + strlen("knife_"));
				CFREE(*hitsound);
				CSTRDUP(*hitsound, buf);
			}
		}
	}
}
static void BulletClassesLoadWeapons(CArray *classes);
void BulletLoadWeapons(BulletClasses *bullets)
{
	BulletClassesLoadWeapons(&bullets->Classes);
	BulletClassesLoadWeapons(&bullets->CustomClasses);
	json_free_value(&bullets->root);
}
static void BulletClassesLoadWeapons(CArray *classes)
{
	for (int i = 0; i < (int)classes->size; i++)
	{
		BulletClass *b = CArrayGet(classes, i);
		if (b->node == NULL)
		{
			continue;
		}

		if (json_find_first_label(b->node, "Falling"))
		{
			LoadBulletGuns(
				&b->Falling.DropGuns,
				json_find_first_label(b->node, "Falling")->child,
				"DropGuns");
		}
		LoadBulletGuns(&b->OutOfRangeGuns, b->node, "OutOfRangeGuns");
		LoadBulletGuns(&b->HitGuns, b->node, "HitGuns");
		LoadBulletGuns(&b->ProximityGuns, b->node, "ProximityGuns");

		b->node = NULL;
	}
}
void BulletTerminate(BulletClasses *bullets)
{
	BulletClassFree(&bullets->Default);
	BulletClassesClear(&bullets->Classes);
	CArrayTerminate(&bullets->Classes);
	BulletClassesClear(&bullets->CustomClasses);
	CArrayTerminate(&bullets->CustomClasses);
}
void BulletClassesClear(CArray *classes)
{
	for (int i = 0; i < (int)classes->size; i++)
	{
		BulletClassFree(CArrayGet(classes, i));
	}
	CArrayClear(classes);
}
static void BulletClassFree(BulletClass *b)
{
	CFREE(b->Name);
	CFREE(b->HitSound.Object);
	CFREE(b->HitSound.Flesh);
	CFREE(b->HitSound.Wall);
	CArrayTerminate(&b->OutOfRangeGuns);
	CArrayTerminate(&b->HitGuns);
	CArrayTerminate(&b->Falling.DropGuns);
	CArrayTerminate(&b->ProximityGuns);
}

void BulletAdd(const NAddBullet add)
{
	const Vec2i pos = Net2Vec2i(add.MuzzlePos);

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
	obj->UID = add.UID;
	obj->bulletClass = StrBulletClass(add.BulletClass);
	obj->x = pos.x;
	obj->y = pos.y;
	obj->z = add.MuzzleHeight;
	obj->dz = add.Elevation;

	obj->tileItem.VelFull = Vec2iFull2Real(Vec2iScale(
		GetFullVectorsForRadians(add.Angle),
		RAND_INT(obj->bulletClass->SpeedLow, obj->bulletClass->SpeedHigh)));
	if (obj->bulletClass->SpeedScale)
	{
		obj->tileItem.VelFull.y =
			obj->tileItem.VelFull.y * TILE_WIDTH / TILE_HEIGHT;
	}

	obj->PlayerUID = add.PlayerUID;
	obj->ActorUID = add.ActorUID;
	obj->range = RAND_INT(
		obj->bulletClass->RangeLow, obj->bulletClass->RangeHigh);

	obj->flags = add.Flags;
	if (obj->bulletClass->HurtAlways)
	{
		obj->flags |= FLAGS_HURTALWAYS;
	}

	obj->tileItem.kind = KIND_MOBILEOBJECT;
	obj->tileItem.id = i;
	obj->isInUse = true;
	obj->tileItem.x = obj->tileItem.y = -1;
	obj->tileItem.getPicFunc = NULL;
	obj->tileItem.drawFunc = NULL;
	obj->tileItem.drawData.MobObjId = i;
	obj->tileItem.CPic = obj->bulletClass->CPic;
	obj->tileItem.CPicFunc = GetBulletDrawContext;
	obj->tileItem.size = obj->bulletClass->Size;
	obj->tileItem.ShadowSize = obj->bulletClass->ShadowSize;
	obj->updateFunc = UpdateBullet;
	MapTryMoveTileItem(&gMap, &obj->tileItem, Vec2iFull2Real(pos));
}

void PlayHitSound(const HitSounds *h, const HitType t, const Vec2i realPos)
{
	switch (t)
	{
	case HIT_NONE:
		// Do nothing
		break;
	case HIT_WALL:
		SoundPlayAt(&gSoundDevice, StrSound(h->Wall), realPos);
		break;
	case HIT_OBJECT:
		SoundPlayAt(&gSoundDevice, StrSound(h->Object), realPos);
		break;
	case HIT_FLESH:
		SoundPlayAt(&gSoundDevice, StrSound(h->Flesh), realPos);
		break;
	default:
		CASSERT(false, "unknown hit type")
			break;
	}
}
