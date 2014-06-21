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
#ifndef __BULLET_CLASS
#define __BULLET_CLASS

#include "particle.h"
#include "sounds.h"
#include "tile.h"

struct MobileObject;
typedef bool (*BulletUpdateFunc)(struct MobileObject *, int);
typedef enum
{
	FALLING_TYPE_BOUNCE,
	FALLING_TYPE_DZ,
	FALLING_TYPE_Z
} BulletClassFallingType;
typedef struct
{
	const NamedSprites *Sprites;
	BeamPic Beam;
} BeamSprites;
typedef struct
{
	char *Name;
	TileItemGetPicFunc GetPicFunc;
	TileItemDrawFunc DrawFunc;
	TileItemDrawFuncData DrawData;
	BeamSprites Beam;
	HSV Tint;
	int Delay;	// number of frames before moving
	int SpeedLow;
	int SpeedHigh;
	bool SpeedScale;	// whether to scale X/Y speed based on perspective
	Vec2i Friction;	// Amount to subtract from velocity per tick
	// -1 is infinite range
	int RangeLow;
	int RangeHigh;
	int Power;
	Vec2i Size;
	special_damage_e Special;
	bool HurtAlways;
	bool Persists;	// remains even after hitting walls/items
	const ParticleClass *Spark;
	Mix_Chunk *WallHitSound;
	bool WallBounces;
	bool HitsObjects;
	struct
	{
		int GravityFactor;	// 0 for non-falling bullets
		BulletClassFallingType Type;
		bool DestroyOnDrop;
		CArray DropGuns;	// of const GunDescription *
	} Falling;
	bool RandomAnimation;
	int SeekFactor;	// -1 to disable; higher = less seeking
	bool Erratic;

	// Special weapons to fire if certain events occur
	CArray OutOfRangeGuns;	// of const GunDescription *
	CArray HitGuns;	// of const GunDescription *
	CArray ProximityGuns;	// of const GunDescription *
} BulletClass;
extern CArray gBulletClasses;	// of BulletClass

BulletClass *StrBulletClass(const char *s);

void BulletInitialize(CArray *bullets);
// 2-step initialisation since bullet and weapon reference each other
void BulletInitialize2(CArray *bullets);
void BulletTerminate(CArray *bullets);

typedef struct
{
	const BulletClass *Class;
	Vec2i FullPos;
	int Flags;
	int PlayerIndex;
	double Angle;
	int DZ;
} AddFireball;
void FireballAdd(const AddFireball e);

typedef struct
{
	const BulletClass *BulletClass;
	Vec2i MuzzlePos;
	int MuzzleHeight;
	double Angle;
	int Elevation;
	int Flags;
	int PlayerIndex;
} AddBullet;
void BulletAdd(const AddBullet add);

bool UpdateBullet(struct MobileObject *obj, const int ticks);

#endif
