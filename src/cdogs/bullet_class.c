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

    Copyright (c) 2013-2015, Cong Xu
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
#include "net_util.h"
#include "objs.h"
#include "screen_shake.h"

BulletClasses gBulletClasses;

#define SOUND_LOCK_MOBILE_OBJECT 12
#define SPECIAL_LOCK 12


// TODO: use map structure?
BulletClass *StrBulletClass(const char *s)
{
	if (s == NULL || strlen(s) == 0)
	{
		return NULL;
	}
	for (int i = 0; i < (int)gBulletClasses.CustomClasses.size; i++)
	{
		BulletClass *b = CArrayGet(&gBulletClasses.CustomClasses, i);
		if (strcmp(s, b->Name) == 0)
		{
			return b;
		}
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
static HitType HitItem(
	TMobileObject *obj, const Vec2i pos, const bool multipleHits);
bool UpdateBullet(TMobileObject *obj, const int ticks)
{
	obj->count += ticks;
	obj->soundLock = MAX(0, obj->soundLock - ticks);
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
		return false;
	}

	const Vec2i objPos = Vec2iNew(obj->x, obj->y);

	if (obj->bulletClass->SeekFactor > 0)
	{
		// Find the closest target to this bullet and steer towards it
		const TActor *owner = ActorGetByUID(obj->ActorUID);
		if (owner == NULL)
		{
			return false;
		}
		const TActor *target = AIGetClosestEnemy(objPos, owner, obj->flags);
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
	HitType hitItem = HIT_NONE;
	if (!gCampaign.IsClient)
	{
		hitItem = HitItem(obj, pos, obj->bulletClass->Persists);
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
	const bool isDiagonal = obj->vel.x != 0 && obj->vel.y != 0;
	int frictionComponent = isDiagonal ?
		(int)round(obj->bulletClass->Friction / sqrt(2)) :
		obj->bulletClass->Friction;
	for (int i = 0; i < ticks; i++)
	{
		if (obj->vel.x > 0)
		{
			obj->vel.x -= frictionComponent;
		}
		else if (obj->vel.x < 0)
		{
			obj->vel.x += frictionComponent;
		}

		if (obj->vel.y > 0)
		{
			obj->vel.y -= frictionComponent;
		}
		else if (obj->vel.y < 0)
		{
			obj->vel.y += frictionComponent;
		}
	}

	bool hitWall = false;
	if (!gCampaign.IsClient && hitItem == HIT_NONE)
	{
		hitWall =
			MapIsRealPosIn(&gMap, realPos) && ShootWall(realPos.x, realPos.y);
	}
	if (hitWall || hitItem != HIT_NONE)
	{
		GameEvent b = GameEventNew(GAME_EVENT_BULLET_BOUNCE);
		b.u.BulletBounce.UID = obj->UID;
		if (hitWall && !Vec2iIsZero(obj->vel))
		{
			b.u.BulletBounce.HitType = (int)HIT_WALL;
		}
		else
		{
			b.u.BulletBounce.HitType = (int)hitItem;
		}
		bool alive = true;
		if ((hitWall && !obj->bulletClass->WallBounces) ||
			((hitItem != HIT_NONE) && obj->bulletClass->HitsObjects))
		{
			b.u.BulletBounce.Spark = true;
			CASSERT(!gCampaign.IsClient, "Cannot process bounces as client");
			FireGuns(obj, &obj->bulletClass->HitGuns);
			if (hitWall || !obj->bulletClass->Persists)
			{
				alive = false;
			}
		}
		b.u.BulletBounce.BouncePos = Vec2i2Net(pos);
		b.u.BulletBounce.BounceVel = Vec2i2Net(obj->vel);
		if (hitWall && !Vec2iIsZero(obj->vel))
		{
			// Bouncing
			Vec2i bounceVel = obj->vel;
			pos = GetWallBounceFullPos(objPos, pos, &bounceVel);
			b.u.BulletBounce.BouncePos = Vec2i2Net(pos);
			b.u.BulletBounce.BounceVel = Vec2i2Net(bounceVel);
			obj->vel = bounceVel;
		}
		GameEventsEnqueue(&gGameEvents, b);
		if (!alive)
		{
			return false;
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
			obj->vel.x += ((rand() % 3) - 1) * 128;
			obj->vel.y += ((rand() % 3) - 1) * 128;
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
	const double angle = Vec2iToRadians(obj->vel);
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
} HitItemData;
static bool HitItemFunc(TTileItem *ti, void *data);
static HitType HitItem(
	TMobileObject *obj, const Vec2i pos, const bool multipleHits)
{
	// Don't hit if no damage dealt
	// This covers non-damaging debris explosions
	if (obj->bulletClass->Power <= 0 &&
		(obj->specialLock > 0 || obj->bulletClass->Special == SPECIAL_NONE))
	{
		return HIT_NONE;
	}

	// Get all items that collide
	HitItemData data;
	data.HitType = HIT_NONE;
	data.MultipleHits = multipleHits;
	data.Obj = obj;
	CollideTileItems(
		&obj->tileItem, Vec2iFull2Real(pos),
		TILEITEM_CAN_BE_SHOT, COLLISIONTEAM_NONE,
		IsPVP(gCampaign.Entry.Mode),
		HitItemFunc, &data);
	return data.HitType;
}
static bool HitItemFunc(TTileItem *ti, void *data)
{
	HitItemData *hData = data;
	if (!CanHit(hData->Obj->flags, hData->Obj->ActorUID, ti))
	{
		goto bail;
	}
	int targetUID = -1;
	switch (ti->kind)
	{
	case KIND_CHARACTER:
		hData->HitType = HIT_FLESH;
		targetUID = ((const TActor *)CArrayGet(&gActors, ti->id))->uid;
		break;
	case KIND_OBJECT:
		hData->HitType = HIT_OBJECT;
		targetUID = ((const TObject *)CArrayGet(&gObjs, ti->id))->uid;
		break;
	default:
		CASSERT(false, "cannot damage target kind");
		break;
	}
	if (hData->Obj->soundLock > 0 ||
		!HasHitSound(
		hData->Obj->bulletClass->Power, hData->Obj->flags, hData->Obj->PlayerUID,
		ti->kind, targetUID, hData->Obj->bulletClass->Special, true))
	{
		hData->HitType = HIT_NONE;
	}
	Damage(
		hData->Obj->vel, hData->Obj->bulletClass->Power,
		hData->Obj->flags, hData->Obj->PlayerUID, hData->Obj->ActorUID,
		ti->kind, targetUID,
		hData->Obj->bulletClass->Special);
	if (hData->Obj->soundLock <= 0)
	{
		hData->Obj->soundLock += SOUND_LOCK_MOBILE_OBJECT;
	}
	if (hData->Obj->specialLock <= 0)
	{
		hData->Obj->specialLock += SPECIAL_LOCK;
	}

bail:
	// Whether to produce multiple hits from the same TMobileObject
	return hData->MultipleHits;
}



#define VERSION 1
static void LoadBullet(
	BulletClass *b, json_t *node, const BulletClass *defaultBullet);
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
		LoadBullet(&bullets->Default, defaultNode->child, NULL);
	}

	json_t *bulletsNode = json_find_first_label(bulletNode, "Bullets")->child;
	for (json_t *child = bulletsNode->child; child; child = child->next)
	{
		BulletClass b;
		LoadBullet(&b, child, &bullets->Default);
		CArrayPushBack(classes, &b);
	}

	bullets->root = bulletNode;
}
static void LoadBullet(
	BulletClass *b, json_t *node, const BulletClass *defaultBullet)
{
	memset(b, 0, sizeof *b);
	if (defaultBullet != NULL)
	{
		memcpy(b, defaultBullet, sizeof *b);
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

	LoadStr(&b->Name, node, "Name");
	if (json_find_first_label(node, "Pic"))
	{
		json_t *pic = json_find_first_label(node, "Pic")->child;
		tmp = GetString(pic, "Type");
		b->CPic.Type = StrPicType(tmp);
		CFREE(tmp);
		bool picLoaded = false;
		switch (b->CPic.Type)
		{
		case PICTYPE_NORMAL:
			tmp = GetString(pic, "Pic");
			b->CPic.u.Pic = PicManagerGetPic(&gPicManager, tmp);
			CFREE(tmp);
			picLoaded = b->CPic.u.Pic != NULL;
			break;
		case PICTYPE_DIRECTIONAL:
			tmp = GetString(pic, "Sprites");
			b->CPic.u.Sprites =
				&PicManagerGetSprites(&gPicManager, tmp)->pics;
			CFREE(tmp);
			picLoaded = b->CPic.u.Sprites != NULL;
			break;
		case PICTYPE_ANIMATED:	// fallthrough
		case PICTYPE_ANIMATED_RANDOM:
			tmp = GetString(pic, "Sprites");
			b->CPic.u.Animated.Sprites =
				&PicManagerGetSprites(&gPicManager, tmp)->pics;
			CFREE(tmp);
			LoadInt(&b->CPic.u.Animated.Count, pic, "Count");
			LoadInt(&b->CPic.u.Animated.TicksPerFrame, pic, "TicksPerFrame");
			// Set safe default ticks per frame 1;
			// if 0 then this leads to infinite loop when animating
			b->CPic.u.Animated.TicksPerFrame = MAX(
				b->CPic.u.Animated.TicksPerFrame, 1);
			picLoaded = b->CPic.u.Animated.Sprites != NULL;
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
		}
		if ((json_find_first_label(pic, "OldPic") &&
			ConfigGetBool(&gConfig, "Graphics.OriginalPics")) ||
			!picLoaded)
		{
			int oldPic = PIC_UZIBULLET;
			LoadInt(&oldPic, pic, "OldPic");
			b->CPic.Type = PICTYPE_NORMAL;
			b->CPic.u.Pic = PicManagerGetFromOld(&gPicManager, oldPic);
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
	LoadInt(&b->Friction, node, "Friction");
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
		b->Spark = StrParticleClass(&gParticleClasses, tmp);
		CFREE(tmp);
	}
	if (json_find_first_label(node, "HitSounds"))
	{
		json_t *hitSounds = json_find_first_label(node, "HitSounds")->child;
		CFREE(b->HitSound.Object);
		b->HitSound.Object = NULL;
		LoadStr(&b->HitSound.Object, hitSounds, "Object");
		CFREE(b->HitSound.Flesh);
		b->HitSound.Flesh = NULL;
		LoadStr(&b->HitSound.Flesh, hitSounds, "Flesh");
		CFREE(b->HitSound.Wall);
		b->HitSound.Wall = NULL;
		LoadStr(&b->HitSound.Wall, hitSounds, "Wall");
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

	obj->vel = Vec2iFull2Real(Vec2iScale(
		GetFullVectorsForRadians(add.Angle),
		RAND_INT(obj->bulletClass->SpeedLow, obj->bulletClass->SpeedHigh)));
	if (obj->bulletClass->SpeedScale)
	{
		obj->vel.y = obj->vel.y * TILE_WIDTH / TILE_HEIGHT;
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
	obj->tileItem.getActorPicsFunc = NULL;
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
