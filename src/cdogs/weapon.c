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

    Copyright (c) 2013, Cong Xu
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
#include "weapon.h"

#include <assert.h>

#include "config.h"
#include "objs.h"
#include "sounds.h"


GunDescription gGunDescriptions[] =
{
	// Gun picture		Name				Lock	ReloadLead	Sound			Reload sound
	{GUNPIC_KNIFE,		"Knife",			0,		-1,			-1,				-1},
	{GUNPIC_BLASTER,	"Machine gun",		6,		-1,			SND_MACHINEGUN,	-1},
	{-1,				"Grenades",			30,		-1,			SND_LAUNCH,		-1},
	{GUNPIC_BLASTER,	"Flamer",			6,		-1,			SND_FLAMER,		-1},
	{GUNPIC_BLASTER,	"Shotgun",			50,		10,			SND_SHOTGUN,	SND_SHOTGUN_R},
	{GUNPIC_BLASTER,	"Powergun",			20,		-1,			SND_POWERGUN,	-1},
	{-1,				"Shrapnel bombs",	30,		-1,			SND_LAUNCH,		-1},
	{-1,				"Molotovs",			30,		-1,			SND_LAUNCH,		-1},
	{GUNPIC_BLASTER,	"Sniper rifle",		100,	15,			SND_LASER,		SND_LASER_R},
	{-1,				"Prox. mine",		100,	15,			SND_HAHAHA,		SND_PACKAGE_R},
	{-1,				"Dynamite",			100,	15,			SND_HAHAHA,		SND_PACKAGE_R},
	{-1,				"Chemo bombs",		30,		-1,			SND_LAUNCH,		-1},
	{GUNPIC_BLASTER,	"Petrify gun",		100,	15,			SND_LASER,		SND_LASER_R},
	{GUNPIC_BLASTER,	"Browny gun",		30,		-1,			SND_POWERGUN,	-1},
	{-1,				"Confusion bombs",	30,		-1,			SND_LAUNCH,		-1},
	{GUNPIC_BLASTER,	"Chemo gun",		6,		-1,			SND_FLAMER,		-1}
};

#define RELOAD_DISTANCE_PLUS 300

const TOffsetPic cGunPics[GUNPIC_COUNT][DIRECTION_COUNT][GUNSTATE_COUNT] = {
	{
	 {{-2, -10, 86}, {-3, -8, 78}, {-3, -7, 78}},
	 {{-2, -10, 87}, {-2, -9, 79}, {-3, -8, 79}},
	 {{0, -12, 88}, {0, -5, 80}, {-1, -5, 80}},
	 {{-2, -9, 90}, {0, -2, 81}, {-1, -3, 81}},
	 {{-2, -9, 90}, {-1, -2, 82}, {-1, -3, 82}},
	 {{-6, -10, 91}, {-7, -4, 83}, {-6, -5, 83}},
	 {{-8, -11, 92}, {-12, -6, 84}, {-11, -6, 84}},
	 {{-6, -14, 93}, {-8, -12, 85}, {-7, -11, 85}}
	 },
	{
	 {{-1, -7, 142}, {-1, -7, 142}, {-1, -7, 142}},
	 {{-1, -7, 142}, {-1, -7, 142}, {-1, -7, 142}},
	 {{-2, -8, 143}, {-2, -8, 143}, {-2, -8, 143}},
	 {{-3, -5, 144}, {-3, -5, 144}, {-3, -5, 144}},
	 {{-3, -5, 144}, {-3, -5, 144}, {-3, -5, 144}},
	 {{-3, -5, 144}, {-3, -5, 144}, {-3, -5, 144}},
	 {{-8, -10, 145}, {-8, -10, 145}, {-8, -10, 145}},
	 {{-8, -10, 145}, {-8, -10, 145}, {-8, -10, 145}}
	 }
};

const OffsetTable cMuzzleOffset[GUNPIC_COUNT] = {
	{
	 {2, 0},
	 {7, 2},
	 {13, 2},
	 {7, 6},
	 {2, 6},
	 {2, 6},
	 {0, 2},
	 {2, 2}
	 }
};


Weapon WeaponCreate(gun_e gun)
{
	Weapon w;
	w.gun = gun;
	w.state = GUNSTATE_READY;
	w.lock = 0;
	w.soundLock = 0;
	return w;
}

gunpic_e GunGetPic(gun_e gun)
{
	return gGunDescriptions[gun].gunPic;
}

const char *GunGetName(gun_e gun)
{
	return gGunDescriptions[gun].gunName;
}

void WeaponUpdate(Weapon *w, int ticks, Vector2i tilePosition)
{
	// Reload sound
	if (gConfig.Sound.Reloads &&
		w->lock > gGunDescriptions[w->gun].ReloadLead &&
		w->lock - ticks <= gGunDescriptions[w->gun].ReloadLead &&
		w->lock > 0 &&
		(int)gGunDescriptions[w->gun].ReloadSound != -1)
	{
		SoundPlayAtPlusDistance(
			gGunDescriptions[w->gun].ReloadSound,
			tilePosition.x, tilePosition.y,
			RELOAD_DISTANCE_PLUS);
	}
	w->lock -= ticks;
	if (w->lock < 0)
	{
		w->lock = 0;
	}
	w->soundLock -= ticks;
	if (w->soundLock < 0)
	{
		w->soundLock = 0;
	}
}

int cGunScores[GUN_COUNT] =
{
	0,
	1,
	20,
	1,
	5,
	2,
	20,
	20,
	5,
	10,
	7,
	7,
	10,
	1,
	10,
	5
};

int GunGetScore(gun_e gun)
{
	return cGunScores[gun];
}

int cGunSoundLocks[GUN_COUNT] =
{
	0,
	0,
	0,
	36,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	36
};

int WeaponCanFire(Weapon *w)
{
	return w->lock <= 0;
}

void MachineGun(Vector2i muzzlePosition, int angle, int flags)
{
	angle += (rand() & 7) - 4;
	if (angle < 0)
		angle += 256;

	AddBullet(
		muzzlePosition.x, muzzlePosition.y, angle,
		MG_SPEED, MG_RANGE, MG_POWER, flags);
}

void LaunchGrenade(Vector2i muzzlePosition, int angle, int flags)
{
	AddGrenade(muzzlePosition.x, muzzlePosition.y, angle, flags, MOBOBJ_GRENADE);
}

void Flamer(Vector2i muzzlePosition, int angle, int flags)
{
	AddFlame(muzzlePosition.x, muzzlePosition.y, angle, flags);
}

void ShotGun(Vector2i muzzlePosition, int angle, int flags)
{
	int i;
	angle -= 16;
	for (i = 0; i <= 32; i += 8, angle += 8)
	{
		AddBullet(
			muzzlePosition.x, muzzlePosition.y,
			angle > 0 ? angle : angle + 256,
			SHOTGUN_SPEED, SHOTGUN_RANGE, SHOTGUN_POWER,
			flags);
	}
}

void PowerGun(Vector2i muzzlePosition, direction_e d, int flags)
{
	AddLaserBolt(muzzlePosition.x, muzzlePosition.y, d, flags);
}

void LaunchFragGrenade(Vector2i muzzlePosition, int angle, int flags)
{
	AddGrenade(muzzlePosition.x, muzzlePosition.y, angle, flags, MOBOBJ_FRAGGRENADE);
}

void LaunchMolotov(Vector2i muzzlePosition, int angle, int flags)
{
	AddGrenade(muzzlePosition.x, muzzlePosition.y, angle, flags, MOBOBJ_MOLOTOV);
}

void SniperGun(Vector2i muzzlePosition, direction_e d, int flags)
{
	AddSniperBullet(muzzlePosition.x, muzzlePosition.y, d, flags);
}

void LaunchGasBomb(Vector2i muzzlePosition, int angle, int flags)
{
	AddGrenade(muzzlePosition.x, muzzlePosition.y, angle, flags, MOBOBJ_GASBOMB);
}

void Petrifier(Vector2i muzzlePosition, int angle, int flags)
{
	AddPetrifierBullet(muzzlePosition.x, muzzlePosition.y, angle, 768, 45, flags);
}

void BrownGun(Vector2i muzzlePosition, int angle, int flags)
{
	AddBrownBullet(muzzlePosition.x, muzzlePosition.y, angle, 768, 45, 15, flags);
}

void ConfuseBomb(Vector2i muzzlePosition, int angle, int flags)
{
	AddGrenade(muzzlePosition.x, muzzlePosition.y, angle, flags, MOBOBJ_GASBOMB2);
}

void GasGun(Vector2i muzzlePosition, int angle, int flags)
{
	AddGasCloud(muzzlePosition.x, muzzlePosition.y, angle, 384, 35, flags, SPECIAL_POISON);
}

void Mine(Vector2i muzzlePosition, int flags)
{
	AddProximityMine(muzzlePosition.x, muzzlePosition.y, flags);
}

void Dynamite(Vector2i muzzlePosition, int flags)
{
	AddDynamite(muzzlePosition.x, muzzlePosition.y, flags);
}

/*
void Heatseeker(TActor * actor)
{
	int angle;
	int d = actor->direction;
	int dx, dy;

	angle = dir2angle[d];
	GetMuzzle(actor, &dx, &dy);
	AddHeatseeker(actor->x + 256 * dx, actor->y + 256 * dy, angle,
		      512, 60, 20, actor->flags);
	actor->gunLock = 30;
	Score(actor->flags, -7);
	SoundPlayAt(SND_LAUNCH, actor->tileItem.x, actor->tileItem.y);
}

void PulseRifle(TActor * actor)
{
	int angle;
	int d = actor->direction;
	int dx, dy;

	angle = dir2angle[d];
	angle += (rand() & 7) - 4;
	if (angle < 0)
		angle += 256;

	GetMuzzle(actor, &dx, &dy);
	AddRapidBullet(actor->x + 256 * dx, actor->y + 256 * dy, angle,
		       1280, 25, 7, actor->flags);
	actor->gunLock = 4;
	Score(actor->flags, -1);
	SoundPlayAt(SND_MINIGUN, actor->tileItem.x, actor->tileItem.y);
}
*/

void WeaponPlaySound(Weapon *w, Vector2i tilePosition)
{
	if (w->soundLock <= 0 && (int)gGunDescriptions[w->gun].Sound != -1)
	{
		SoundPlayAt(gGunDescriptions[w->gun].Sound, tilePosition.x, tilePosition.y);
		w->soundLock = cGunSoundLocks[w->gun];
	}
}

void WeaponFire(
	Weapon *w, direction_e d, Vector2i muzzlePosition, Vector2i tilePosition, int flags)
{
	int angle = dir2angle[d];
	assert(WeaponCanFire(w));
	switch (w->gun)
	{
		case GUN_KNIFE:
			// Do nothing
			break;

		case GUN_MG:
			MachineGun(muzzlePosition, angle, flags);
			break;

		case GUN_GRENADE:
			LaunchGrenade(muzzlePosition, angle, flags);
			break;

		case GUN_FLAMER:
			Flamer(muzzlePosition, angle, flags);
			break;

		case GUN_SHOTGUN:
			ShotGun(muzzlePosition, angle, flags);
			break;

		case GUN_POWERGUN:
			PowerGun(muzzlePosition, d, flags);
			break;

		case GUN_FRAGGRENADE:
			LaunchFragGrenade(muzzlePosition, angle, flags);
			break;

		case GUN_MOLOTOV:
			LaunchMolotov(muzzlePosition, angle, flags);
			break;

		case GUN_SNIPER:
			SniperGun(muzzlePosition, d, flags);
			break;

		case GUN_GASBOMB:
			LaunchGasBomb(muzzlePosition, angle, flags);
			break;

		case GUN_PETRIFY:
			Petrifier(muzzlePosition, angle, flags);
			break;

		case GUN_BROWN:
			BrownGun(muzzlePosition, angle, flags);
			break;

		case GUN_CONFUSEBOMB:
			ConfuseBomb(muzzlePosition, angle, flags);
			break;

		case GUN_GASGUN:
			GasGun(muzzlePosition, angle, flags);
			break;

		case GUN_MINE:
			Mine(muzzlePosition, flags);
			break;

		case GUN_DYNAMITE:
			Dynamite(muzzlePosition, flags);
			break;

		default:
			// unknown gun?
			assert(0);
			break;
	}

	w->lock = gGunDescriptions[w->gun].Lock;
	WeaponPlaySound(w, tilePosition);
}

int GunIsStatic(gun_e gun)
{
	switch (gun)
	{
	case GUN_MINE:
	case GUN_DYNAMITE:
		return 1;
	default:
		return 0;
	}
}
int GunHasMuzzle(gun_e gun)
{
	switch (gun)
	{
	case GUN_MG:
	case GUN_FLAMER:
	case GUN_SHOTGUN:
	case GUN_POWERGUN:
	case GUN_SNIPER:
	case GUN_PETRIFY:
	case GUN_BROWN:
	case GUN_GASGUN:
		return 1;
	default:
		return 0;
	}
}
int IsHighDPS(gun_e gun)
{
	switch (gun)
	{
	case GUN_GRENADE:
	case GUN_FRAGGRENADE:
	case GUN_MOLOTOV:
	case GUN_MINE:
	case GUN_DYNAMITE:
		return 1;
	default:
		return 0;
	}
}
int IsLongRange(gun_e gun)
{
	switch (gun)
	{
	case GUN_MG:
	case GUN_POWERGUN:
	case GUN_SNIPER:
	case GUN_PETRIFY:
	case GUN_BROWN:
		return 1;
	default:
		return 0;
	}
}
int IsShortRange(gun_e gun)
{
	switch (gun)
	{
	case GUN_KNIFE:
	case GUN_FLAMER:
	case GUN_MOLOTOV:
	case GUN_MINE:
	case GUN_DYNAMITE:
	case GUN_GASGUN:
		return 1;
	default:
		return 0;
	}
}
