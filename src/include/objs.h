/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (C) 1995 Ronny Webster
    Copyright (C) 2003 Jeremy Chin 
    Copyright (C) 2003 Lucas Martin-King 

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

-------------------------------------------------------------------------------

 objs.h - <description here>

*/

#ifndef __OBJSH
#define __OBJSH

#include "map.h"
#include "pics.h"


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
#define OBJ_JEWEL           0
#define OBJ_KEYCARD_RED     1
#define OBJ_KEYCARD_BLUE    2
#define OBJ_KEYCARD_GREEN   3
#define OBJ_KEYCARD_YELLOW  4
#define OBJ_PUZZLE_1        5
#define OBJ_PUZZLE_2        6
#define OBJ_PUZZLE_3        7
#define OBJ_PUZZLE_4        8
#define OBJ_PUZZLE_5        9
#define OBJ_PUZZLE_6        10
#define OBJ_PUZZLE_7        11
#define OBJ_PUZZLE_8        12
#define OBJ_PUZZLE_9        13
#define OBJ_PUZZLE_10       14


#define OBJFLAG_EXPLOSIVE   1
#define OBJFLAG_FLAMMABLE   2
#define OBJFLAG_POISONOUS   4
#define OBJFLAG_CONFUSING   8
#define OBJFLAG_PETRIFYING  16
#define OBJFLAG_QUAKE       32

#define OBJFLAG_DANGEROUS   31

#define SPECIAL_FLAME       1
#define SPECIAL_POISON      2
#define SPECIAL_PETRIFY     3
#define SPECIAL_CONFUSE     4


struct Object {
	TOffsetPic *pic;
	TOffsetPic *wreckedPic;
	int objectIndex;
	int structure;
	int flags;
	TTileItem tileItem;
	struct Object *next;
};
typedef struct Object TObject;


struct MobileObject {
	int kind;
	int x, y, z;
	int dx, dy, dz;
	int count;
	int state;
	int range;
	int power;
	int flags;
	TTileItem tileItem;
	int (*updateFunc) (struct MobileObject *);
	struct MobileObject *next;
};
typedef struct MobileObject TMobileObject;
typedef int (*MobObjUpdateFunc) (struct MobileObject *);


int DamageSomething(int dx, int dy, int power, int flags,
		    TTileItem * target, int special);

void AddObject(int x, int y, int w, int h,
	       TOffsetPic * pic, int index, int tileFlags);
void AddDestructibleObject(int x, int y, int w, int h,
			   TOffsetPic * pic, TOffsetPic * wreckedPic,
			   int structure, int objFlags, int tileFlags);
void RemoveObject(TObject * obj);
void KillAllObjects(void);

void UpdateMobileObjects(void);
void AddGrenade(int x, int y, int angle, int flags, int kind);
void AddBullet(int x, int y, int angle, int speed, int range, int power,
	       int flags);
void AddRapidBullet(int x, int y, int angle, int speed, int range,
		    int power, int flags);
void AddSniperBullet(int x, int y, int direction, int flags);
void AddBrownBullet(int x, int y, int angle, int speed, int range,
		    int power, int flags);
void AddFlame(int x, int y, int angle, int flags);
void AddLaserBolt(int x, int y, int direction, int flags);
void AddExplosion(int x, int y, int flags);
void AddGasCloud(int x, int y, int angle, int speed, int range, int flags,
		 int special);
void AddPetrifierBullet(int x, int y, int angle, int speed, int range,
			int flags);
void AddHeatseeker(int x, int y, int angle, int speed, int range,
		   int power, int flags);
void AddProximityMine(int x, int y, int flags);
void AddDynamite(int x, int y, int flags);
void KillAllMobileObjects(void);

#endif
