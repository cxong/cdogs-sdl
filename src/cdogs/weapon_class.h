/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.
	Copyright (c) 2013-2019, 2021-2022 Cong Xu
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

#include "bullet_class.h"
#include "draw/char_sprites.h"

// WARNING: used for old-style maps, do not touch
typedef enum
{
	GUN_KNIFE,
	GUN_MG,
	GUN_GRENADE,
	GUN_FLAMER,
	GUN_SHOTGUN,
	GUN_POWERGUN,
	GUN_FRAGGRENADE,
	GUN_MOLOTOV,
	GUN_SNIPER,
	GUN_MINE,
	GUN_DYNAMITE,
	GUN_GASBOMB,
	GUN_PETRIFY,
	GUN_BROWN,
	GUN_CONFUSEBOMB,
	GUN_GASGUN,
	GUN_PULSERIFLE,
	GUN_HEATSEEKER,
	GUN_COUNT
} gun_e;

typedef enum
{
	GUNTYPE_NORMAL,
	GUNTYPE_MELEE,
	GUNTYPE_GRENADE,
	GUNTYPE_MULTI
} GunType;

// Gun states
typedef enum
{
	GUNSTATE_READY,
	GUNSTATE_FIRING,
	GUNSTATE_RECOIL,
	GUNSTATE_COUNT
} gunstate_e;

typedef struct
{
	const Pic *Icon;
	GunType Type;
	union {
		struct
		{
			char *Sprites;
			int Grips;
			CArray Bullets; // of const BulletClass *
			int AmmoId;		// -1 if the gun does not consume ammo
			int Cost;		// Cost in score to fire weapon
			int ReloadLead;
			Mix_Chunk *Sound;
			Mix_Chunk *ReloadSound;
			int SoundLockLength;
			float Recoil; // Random recoil for inaccurate weapons, in radians
			struct
			{
				int Count;	 // Number of bullets in spread
				float Width; // Width of individual spread, in radians
			} Spread;
			float AngleOffset;
			int MuzzleHeight;
			int ElevationLow;
			int ElevationHigh;
			const ParticleClass *MuzzleFlash;
			const ParticleClass *Brass;
			struct
			{
				int Amount;				// Amount of screen shake to produce
				bool CameraSubjectOnly; // Only shake if gun held by camera
										// subject
			} Shake;
		} Normal;
		char *Guns[MAX_BARRELS];
	} u;
	char *name;
	char *Description;
	int Lock;
	Mix_Chunk *SwitchSound;
	bool CanDrop;	// whether this gun can be dropped to be picked up
	char *DropGun;	// Gun to drop if an actor with this gun dies
	bool IsRealGun; // whether this gun can be used as is by players
} WeaponClass;
typedef struct
{
	CArray Guns;	   // of WeaponClass
	CArray CustomGuns; // of WeaponClass
} WeaponClasses;

extern WeaponClasses gWeaponClasses;

void WeaponClassesInitialize(WeaponClasses *wcs);
void WeaponClassesLoadJSON(WeaponClasses *wcs, CArray *classes, json_t *root);
void WeaponClassesClear(CArray *classes);
void WeaponClassesTerminate(WeaponClasses *wcs);
const WeaponClass *StrWeaponClass(const char *s);
WeaponClass *IdWeaponClass(const int i);
int WeaponClassId(const WeaponClass *wc);
struct vec2 WeaponClassGetBarrelMuzzleOffset(
	const WeaponClass *wc, const CharSprites *cs, const int barrel,
	direction_e dir, const gunstate_e state);
float WeaponClassGetMuzzleHeight(
	const WeaponClass *wc, const gunstate_e state, const int barrel);

void WeaponClassFire(
	const WeaponClass *wc, const struct vec2 pos, const float z,
	const double radians, const int flags, const int actorUID,
	const bool playSound, const bool isGun);
void WeaponClassAddBrass(
	const WeaponClass *wc, const direction_e d, const struct vec2 pos);

float WeaponClassGetRange(const WeaponClass *wc);
bool WeaponClassHasMuzzle(const WeaponClass *wc);
bool WeaponClassIsHighDPS(const WeaponClass *wc);
bool WeaponClassIsLongRange(const WeaponClass *wc);
bool WeaponClassIsShortRange(const WeaponClass *wc);
bool WeaponClassCanShoot(const WeaponClass *wc);
int WeaponClassNumBarrels(const WeaponClass *wc);
const WeaponClass *WeaponClassGetBarrel(
	const WeaponClass *wc, const int barrel);
const BulletClass *WeaponClassGetBullet(
	const WeaponClass *wc, const int barrel);

#define WC_BARREL_ATTR(_wc, _attr, _barrel)                                   \
	((_wc).Type == GUNTYPE_MULTI                                              \
		 ? StrWeaponClass((_wc).u.Guns[_barrel])->u.Normal._attr              \
		 : (_wc).u.Normal._attr)

// Initialise bullets and weapons in one go
void BulletAndWeaponInitialize(
	BulletClasses *b, WeaponClasses *wcs, const char *bpath,
	const char *gpath);
