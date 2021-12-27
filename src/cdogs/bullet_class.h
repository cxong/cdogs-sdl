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

    Copyright (c) 2013-2015, 2018-2019, 2021 Cong Xu
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

#include <json/json.h>

#include "proto/msg.pb.h"

#include "particle.h"
#include "sounds.h"
#include "tile.h"

typedef struct
{
	bool Hit;
	char *Sound;
} HitProps;

struct MobileObject;
typedef struct
{
	char *Name;
	CPic CPic;
	struct
	{
		const ParticleClass *P;
		float Width;
		int TicksPerEmit;
	} Trail;
	struct vec2i ShadowSize;
	int Delay;	// number of frames before moving
	float SpeedLow;
	float SpeedHigh;
	bool SpeedScale;	// whether to scale X/Y speed based on perspective
	float Friction;	// Amount to subtract from velocity per tick
	// -1 is infinite range
	int RangeLow;
	int RangeHigh;
	int Power;
	float Mass;
	struct vec2i Size;
	struct
	{
		special_damage_e Effect;
		int Ticks;
	} Special;
	bool HurtAlways;
	bool Persists;	// remains even after hitting walls/items
	const ParticleClass *Spark;
	const ParticleClass *OutOfRangeSpark;
	const ParticleClass *WallMark;
	bool WallBounces;
	struct
	{
		HitProps Object;
		HitProps Flesh;
		HitProps Wall;
		// TODO: haven't defined what hit wall false means
	} Hit;
	struct
	{
		float GravityFactor;	// 0 for non-falling bullets
		bool FallsDown;	// if false, DZ is never negative
		bool DestroyOnDrop;
		bool Bounces;
		CArray DropGuns;	// of const WeaponClass *
	} Falling;
	int SeekFactor;	// -1 to disable; higher = less seeking
	bool Erratic;

	// Special weapons to fire if certain events occur
	CArray OutOfRangeGuns;	// of const WeaponClass *
	CArray HitGuns;	// of const WeaponClass *
	CArray ProximityGuns;	// of const WeaponClass *

	// Temporary JSON object for two-step bullet loading
	json_t *node;
} BulletClass;
typedef struct
{
	CArray Classes;	// of BulletClass
	BulletClass Default;
	CArray CustomClasses;	// of BulletClass
	json_t *root;
} BulletClasses;
extern BulletClasses gBulletClasses;

int BulletClassesCount(const BulletClasses *classes);

BulletClass *StrBulletClass(const char *s);

void BulletInitialize(BulletClasses *bullets);
void BulletLoadJSON(
	BulletClasses *bullets, CArray *classes, json_t *bulletNode);
// 2-step initialisation since bullet and weapon reference each other
void BulletLoadWeapons(BulletClasses *bullets);
void BulletClassesClear(CArray *classes);
void BulletTerminate(BulletClasses *bullets);

void BulletAdd(const NAddBullet add);
void BulletDestroy(struct MobileObject *obj);

bool BulletUpdate(struct MobileObject *obj, const int ticks);
void BulletBounce(const NBulletBounce bb);

// Type of material that the bullet hit
typedef enum
{
	HIT_NONE,
	HIT_WALL,
	HIT_OBJECT,
	HIT_FLESH
} HitType;
void PlayHitSound(const BulletClass *b, const HitType t, const struct vec2 pos);
