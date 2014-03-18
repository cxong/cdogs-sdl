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
#ifndef __OBJSH
#define __OBJSH

#include "actors.h"
#include "bullet_class.h"
#include "map.h"
#include "pics.h"
#include "vector.h"


// Mobile objects
#define MOBOBJ_BULLET       0
#define MOBOBJ_GRENADE      1
#define MOBOBJ_FIREBALL     2
#define MOBOBJ_SPARK        3
#define MOBOBJ_FRAGGRENADE  4
#define MOBOBJ_MOLOTOV      5
#define MOBOBJ_GASBOMB      6
#define MOBOBJ_GASBOMB2     7

#define LASER_RANGE       90
#define LASER_SPEED     1024
#define LASER_POWER       20

#define MG_RANGE          60
#define MG_SPEED         768
#define MG_POWER          10

#define SHOTGUN_RANGE     50
#define SHOTGUN_SPEED    640
#define SHOTGUN_POWER     15

#define FLAME_RANGE       30
#define FLAME_SPEED      384
#define FLAME_POWER       12

#define SNIPER_RANGE      90
#define SNIPER_SPEED    1024
#define SNIPER_POWER      50

#define FIREBALL_POWER     5

#define GRENADE_SPEED    384

// Bullet "height"
#define BULLET_Z   10

// Indices for "pick up" objects
typedef enum
{
	OBJ_NONE,
	OBJ_JEWEL,
	OBJ_HEALTH,
	OBJ_KEYCARD_YELLOW,
	OBJ_KEYCARD_GREEN,
	OBJ_KEYCARD_BLUE,
	OBJ_KEYCARD_RED
} PickupType;


#define OBJFLAG_EXPLOSIVE   1
#define OBJFLAG_FLAMMABLE   2
#define OBJFLAG_POISONOUS   4
#define OBJFLAG_CONFUSING   8
#define OBJFLAG_PETRIFYING  16
#define OBJFLAG_QUAKE       32

#define OBJFLAG_DANGEROUS   31


struct Object {
	const TOffsetPic *pic;
	const TOffsetPic *wreckedPic;
	const char *picName;
	PickupType Type;
	int structure;
	int flags;
	TTileItem tileItem;
	struct Object *next;
};
typedef struct Object TObject;

typedef struct MobileObject
{
	int id;
	int player;	// -1 if not owned by any player
	BulletClass bulletClass;
	int kind;
	int x, y, z;
	Vec2i vel;
	int dz;
	int count;
	int state;
	int range;
	int power;
	special_damage_e special;
	int flags;
	int soundLock;
	TTileItem tileItem;
	BulletUpdateFunc updateFunc;
	struct MobileObject *next;
} TMobileObject;
typedef int (*MobObjUpdateFunc)(TMobileObject *, int);
extern TMobileObject *gMobObjList;
extern int gMobileObjId;


int DamageSomething(
	Vec2i hitVector,
	int power,
	int flags,
	int player,
	TTileItem *target,
	special_damage_e damage,
	bool hasHitSound);

void AddObjectOld(
	int x, int y, Vec2i size,
	const TOffsetPic * pic, PickupType type, int tileFlags);
void AddObject(
	Vec2i pos, Vec2i size, const char *picName, PickupType type, int tileFlags);
void AddDestructibleObject(
	Vec2i pos, int w, int h,
	const TOffsetPic * pic, const TOffsetPic * wreckedPic,
	const char *picName,
	int structure, int objFlags, int tileFlags);
void RemoveObject(TObject * obj);
void KillAllObjects(void);

void UpdateMobileObjects(TMobileObject **mobObjList, int ticks);
void MobileObjectRemove(TMobileObject **mobObjList, int id);
TMobileObject *AddMobileObject(TMobileObject **mobObjList, int player);
TMobileObject *AddFireBall(int flags, int player);
void MobileObjectUpdate(TMobileObject *obj, int ticks);
int UpdateExplosion(TMobileObject *obj, int ticks);
int HitItem(TMobileObject *obj, Vec2i pos, special_damage_e special);
void KillAllMobileObjects(TMobileObject **mobObjList);

#endif
