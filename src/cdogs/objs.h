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

    Copyright (c) 2013-2016, 2018-2019, 2021 Cong Xu
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
#pragma once

#include "actors.h"
#include "bullet_class.h"
#include "map.h"
#include "pics.h"
#include "vector.h"


// Bullet "height"
#define BULLET_Z   10

#define AMMO_SPAWNER_RESPAWN_TICKS (FPS_FRAMELIMIT * 30)

#define SHOT_IMPULSE_FACTOR 0.04f


typedef struct
{
	int uid;
	const MapObject *Class;
	int Health;
	int counter;
	Thing thing;
	Emitter damageSmoke;
	bool isInUse;
} TObject;

typedef struct MobileObject
{
	int UID;
	int ActorUID;	// unique ID of actor that owns this object
					// (prevent self collision)
	const BulletClass *bulletClass;
	float z;
	float dz;
	int count;
	int range;
	int flags;
	Thing thing;
	Emitter trail;
	bool isInUse;
} TMobileObject;
typedef int (*MobObjUpdateFunc)(TMobileObject *, int);
extern CArray gMobObjs;	// of TMobileObject
extern CArray gObjs;	// of TObject


bool CanHit(const BulletClass *b, const int flags, const int uid, const Thing *target);
bool HasHitSound(
	const ThingKind targetKind, const int targetUID,
	const special_damage_e special, const bool allowFriendlyHitSound);
void Damage(
	const struct vec2 hitVector,
	const BulletClass *bullet,
	const int flags,
	const TActor *source,
	const ThingKind targetKind, const int targetUID);

void ObjsInit(void);
void ObjsTerminate(void);
int ObjsGetNextUID(void);
void ObjAdd(const NMapObjectAdd amo);
void ObjRemove(const NMapObjectRemove mor);
void ObjDestroy(TObject *o);

// Check if this object is dangerous; i.e. on destruction will explode
bool ObjIsDangerous(const TObject *o);

void UpdateObjects(const int ticks);

TObject *ObjGetByUID(const int uid);

void DamageObject(const NThingDamage d);

void BulletToDamageEvent(const BulletClass *b, GameEvent *e);

void UpdateMobileObjects(int ticks);
void MobObjsInit(void);
void MobObjsTerminate(void);
int MobObjsObjsGetNextUID(void);
TMobileObject *MobObjGetByUID(const int uid);
