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

GunDescription gGunDescriptions[GUN_COUNT];

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

// Initialise all the static weapon data
// TODO: load from data file
void WeaponInitialize(void)
{
	GunDescription *g;
	int i;

	// Load defaults
	for (i = 0; i < GUN_COUNT; i++)
	{
		g = &gGunDescriptions[i];
		g->pic = GUNPIC_BLASTER;
		g->SoundLockLength = 0;
		g->Recoil = 0;
		g->Spread.Count = 1;
		g->Spread.Width = 0;
	}

	g = &gGunDescriptions[GUN_KNIFE];
	g->pic = GUNPIC_KNIFE;
	strcpy(g->name, "Knife");
	g->Cost = 0;
	g->Lock = 0;
	g->ReloadLead = -1;
	g->Sound = -1;
	g->ReloadSound = -1;
	g->SoundLockLength = 10;

	g = &gGunDescriptions[GUN_MG];
	strcpy(g->name, "Machine gun");
	g->Cost = 1;
	g->Lock = 6;
	g->ReloadLead = -1;
	g->Sound = SND_MACHINEGUN;
	g->ReloadSound = -1;
	g->Recoil = 7;

	g = &gGunDescriptions[GUN_GRENADE];
	g->pic = -1;
	strcpy(g->name, "Grenades");
	g->Cost = 20;
	g->Lock = 30;
	g->ReloadLead = -1;
	g->Sound = SND_LAUNCH;
	g->ReloadSound = -1;

	g = &gGunDescriptions[GUN_FLAMER];
	strcpy(g->name, "Flamer");
	g->Cost = 1;
	g->Lock = 6;
	g->ReloadLead = -1;
	g->Sound = SND_FLAMER;
	g->ReloadSound = -1;
	g->SoundLockLength = 36;

	g = &gGunDescriptions[GUN_SHOTGUN];
	strcpy(g->name, "Shotgun");
	g->Cost = 5;
	g->Lock = 50;
	g->ReloadLead = 10;
	g->Sound = SND_SHOTGUN;
	g->ReloadSound = SND_SHOTGUN_R;
	g->Spread.Count = 5;
	g->Spread.Width = 8;

	g = &gGunDescriptions[GUN_POWERGUN];
	strcpy(g->name, "Powergun");
	g->Cost = 2;
	g->Lock = 20;
	g->ReloadLead = -1;
	g->Sound = SND_POWERGUN;
	g->ReloadSound = -1;

	g = &gGunDescriptions[GUN_FRAGGRENADE];
	g->pic = -1;
	strcpy(g->name, "Shrapnel bombs");
	g->Cost = 20;
	g->Lock = 30;
	g->ReloadLead = -1;
	g->Sound = SND_LAUNCH;
	g->ReloadSound = -1;

	g = &gGunDescriptions[GUN_MOLOTOV];
	g->pic = -1;
	strcpy(g->name, "Molotovs");
	g->Cost = 20;
	g->Lock = 30;
	g->ReloadLead = -1;
	g->Sound = SND_LAUNCH;
	g->ReloadSound = -1;

	g = &gGunDescriptions[GUN_SNIPER];
	strcpy(g->name, "Sniper rifle");
	g->Cost = 5;
	g->Lock = 100;
	g->ReloadLead = 15;
	g->Sound = SND_LASER;
	g->ReloadSound = SND_LASER_R;

	g = &gGunDescriptions[GUN_MINE];
	g->pic = -1;
	strcpy(g->name, "Prox. mine");
	g->Cost = 10;
	g->Lock = 100;
	g->ReloadLead = 15;
	g->Sound = SND_HAHAHA;
	g->ReloadSound = SND_PACKAGE_R;

	g = &gGunDescriptions[GUN_DYNAMITE];
	g->pic = -1;
	strcpy(g->name, "Dynamite");
	g->Cost = 7;
	g->Lock = 100;
	g->ReloadLead = 15;
	g->Sound = SND_HAHAHA;
	g->ReloadSound = SND_PACKAGE_R;

	g = &gGunDescriptions[GUN_GASBOMB];
	g->pic = -1;
	strcpy(g->name, "Chemo bombs");
	g->Cost = 7;
	g->Lock = 30;
	g->ReloadLead = -1;
	g->Sound = SND_LAUNCH;
	g->ReloadSound = -1;

	g = &gGunDescriptions[GUN_PETRIFY];
	strcpy(g->name, "Petrify gun");
	g->Cost = 10;
	g->Lock = 100;
	g->ReloadLead = 15;
	g->Sound = SND_LASER;
	g->ReloadSound = SND_LASER_R;

	g = &gGunDescriptions[GUN_BROWN];
	strcpy(g->name, "Browny gun");
	g->Cost = 5;
	g->Lock = 30;
	g->ReloadLead = -1;
	g->Sound = SND_POWERGUN;
	g->ReloadSound = -1;

	g = &gGunDescriptions[GUN_CONFUSEBOMB];
	g->pic = -1;
	strcpy(g->name, "Confusion bombs");
	g->Cost = 10;
	g->Lock = 30;
	g->ReloadLead = -1;
	g->Sound = SND_LAUNCH;
	g->ReloadSound = -1;

	g = &gGunDescriptions[GUN_GASGUN];
	strcpy(g->name, "Chemo gun");
	g->Cost = 1;
	g->Lock = 6;
	g->ReloadLead = -1;
	g->Sound = SND_FLAMER;
	g->ReloadSound = -1;
	g->SoundLockLength = 36;

	g = &gGunDescriptions[GUN_PULSERIFLE];
	strcpy(g->name, "Pulse Rifle");
	g->Cost = 1;
	g->Lock = 4;
	g->ReloadLead = -1;
	g->Sound = SND_MINIGUN;
	g->ReloadSound = -1;
	g->Recoil = 7;
}

Weapon WeaponCreate(gun_e gun)
{
	Weapon w;
	w.gun = gun;
	w.state = GUNSTATE_READY;
	w.lock = 0;
	w.soundLock = 0;
	w.stateCounter = -1;
	return w;
}

gunpic_e GunGetPic(gun_e gun)
{
	return gGunDescriptions[gun].pic;
}

const char *GunGetName(gun_e gun)
{
	return gGunDescriptions[gun].name;
}

Vec2i GunGetMuzzleOffset(gun_e gun, direction_e dir, int isArmed)
{
	Vec2i position;
	gunpic_e g = GunGetPic(gun);
	position.x =
		cGunHandOffset[isArmed][dir].dx +
		cGunPics[g][dir][GUNSTATE_FIRING].dx +
		cMuzzleOffset[g][dir].dx;
	position.y =
		cGunHandOffset[isArmed][dir].dy +
		cGunPics[g][dir][GUNSTATE_FIRING].dy +
		cMuzzleOffset[g][dir].dy + BULLET_Z;
	return Vec2iScale(position, 256);
}

void WeaponSetState(Weapon *w, gunstate_e state);

void WeaponUpdate(Weapon *w, int ticks, Vec2i tilePosition)
{
	// Reload sound
	if (gConfig.Sound.Reloads &&
		w->lock > gGunDescriptions[w->gun].ReloadLead &&
		w->lock - ticks <= gGunDescriptions[w->gun].ReloadLead &&
		w->lock > 0 &&
		(int)gGunDescriptions[w->gun].ReloadSound != -1)
	{
		SoundPlayAtPlusDistance(
			&gSoundDevice,
			gGunDescriptions[w->gun].ReloadSound,
			tilePosition,
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
	if (w->stateCounter >= 0)
	{
		w->stateCounter = MAX(0, w->stateCounter - ticks);
		if (w->stateCounter == 0)
		{
			switch (w->state)
			{
			case GUNSTATE_FIRING:
				WeaponSetState(w, GUNSTATE_RECOIL);
				break;
			case GUNSTATE_RECOIL:
				WeaponSetState(w, GUNSTATE_READY);
				break;
			default:
				assert(0);
				break;
			}
		}
	}
}

int GunGetCost(gun_e gun)
{
	return gGunDescriptions[gun].Cost;
}

int WeaponCanFire(Weapon *w)
{
	return w->lock <= 0;
}

void MachineGun(Vec2i muzzlePosition, int angle, int flags, int player)
{
	AddBullet(muzzlePosition, angle, BULLET_MG, flags, player);
}

void LaunchGrenade(Vec2i muzzlePosition, int angle, int flags, int player)
{
	AddGrenade(muzzlePosition, angle, BULLET_GRENADE, flags, player);
}

void Flamer(Vec2i muzzlePosition, int angle, int flags, int player)
{
	AddBulletBig(muzzlePosition, angle, BULLET_FLAME, flags, player);
}

void ShotGun(Vec2i muzzlePosition, int angle, int flags, int player)
{
	AddBullet(muzzlePosition, angle, BULLET_SHOTGUN, flags, player);
}

void PowerGun(Vec2i muzzlePosition, direction_e d, int flags, int player)
{
	AddBulletDirectional(muzzlePosition, d, BULLET_LASER, flags, player);
}

void LaunchFragGrenade(Vec2i muzzlePosition, int angle, int flags, int player)
{
	AddGrenade(muzzlePosition, angle, BULLET_SHRAPNELBOMB, flags, player);
}

void LaunchMolotov(Vec2i muzzlePosition, int angle, int flags, int player)
{
	AddGrenade(muzzlePosition, angle, BULLET_MOLOTOV, flags, player);
}

void SniperGun(Vec2i muzzlePosition, direction_e d, int flags, int player)
{
	AddBulletDirectional(muzzlePosition, d, BULLET_SNIPER, flags, player);
}

void LaunchGasBomb(Vec2i muzzlePosition, int angle, int flags, int player)
{
	AddGrenade(muzzlePosition, angle, BULLET_GASBOMB, flags, player);
}

void Petrifier(Vec2i muzzlePosition, int angle, int flags, int player)
{
	AddBulletBig(muzzlePosition, angle, BULLET_PETRIFIER, flags, player);
}

void BrownGun(Vec2i muzzlePosition, int angle, int flags, int player)
{
	AddBullet(muzzlePosition, angle, BULLET_BROWN, flags, player);
}

void ConfuseBomb(Vec2i muzzlePosition, int angle, int flags, int player)
{
	AddGrenade(muzzlePosition, angle, BULLET_CONFUSEBOMB, flags, player);
}

void GasGun(Vec2i muzzlePosition, int angle, int flags, int player)
{
	AddGasCloud(
		muzzlePosition.x, muzzlePosition.y, angle, 384, 35,
		flags, SPECIAL_POISON, player);
}

void Mine(Vec2i muzzlePosition, int flags, int player)
{
	AddBulletGround(muzzlePosition, 0, BULLET_PROXMINE, flags, player);
}

void Dynamite(Vec2i muzzlePosition, int flags, int player)
{
	AddBulletGround(muzzlePosition, 0, BULLET_DYNAMITE, flags, player);
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
	SoundPlayAt(&gSoundDevice, SND_LAUNCH, actor->tileItem.x, actor->tileItem.y);
}
*/

void PulseRifle(Vec2i muzzlePosition, int angle, int flags, int player)
{
	AddBullet(muzzlePosition, angle, BULLET_RAPID, flags, player);
}

void WeaponPlaySound(Weapon *w, Vec2i tilePosition)
{
	if (w->soundLock <= 0 && (int)gGunDescriptions[w->gun].Sound != -1)
	{
		SoundPlayAt(
			&gSoundDevice,
			gGunDescriptions[w->gun].Sound,
			tilePosition);
		w->soundLock = gGunDescriptions[w->gun].SoundLockLength;
	}
}

void WeaponFire(
	Weapon *w, direction_e d, Vec2i muzzlePosition, Vec2i tilePosition,
	int flags, int player)
{
	int angle = dir2angle[d];
	int i;
	GunDescription *desc = &gGunDescriptions[w->gun];
	int spreadCount = desc->Spread.Count;
	int spreadStartAngle = 0;
	int spreadWidth = desc->Spread.Width;
	if (spreadCount > 1)
	{
		// Find the starting angle of the spread (clockwise)
		// Keep in mind the fencepost problem, i.e. spread of 3 means a
		// total spread angle of 2x width
		spreadStartAngle = -(spreadCount - 1) * spreadWidth / 2;
	}
	
	assert(WeaponCanFire(w));
	for (i = 0; i < spreadCount; i++)
	{
		int spreadAngle = spreadStartAngle + i * spreadWidth;
		int recoil = 0;
		if (desc->Recoil > 0)
		{
			recoil = (rand() % desc->Recoil) - (desc->Recoil + 1) / 2;
		}
		int finalAngle = angle + spreadAngle + recoil;
		if (finalAngle < 0)
		{
			finalAngle += 256;
		}
		switch (w->gun)
		{
		case GUN_KNIFE:
			// Do nothing
			break;

		case GUN_MG:
			MachineGun(muzzlePosition, finalAngle, flags, player);
			break;

		case GUN_GRENADE:
			LaunchGrenade(muzzlePosition, finalAngle, flags, player);
			break;

		case GUN_FLAMER:
			Flamer(muzzlePosition, finalAngle, flags, player);
			break;

		case GUN_SHOTGUN:
			ShotGun(muzzlePosition, finalAngle, flags, player);
			break;

		case GUN_POWERGUN:
			PowerGun(muzzlePosition, d, flags, player);
			break;

		case GUN_FRAGGRENADE:
			LaunchFragGrenade(muzzlePosition, finalAngle, flags, player);
			break;

		case GUN_MOLOTOV:
			LaunchMolotov(muzzlePosition, finalAngle, flags, player);
			break;

		case GUN_SNIPER:
			SniperGun(muzzlePosition, d, flags, player);
			break;

		case GUN_GASBOMB:
			LaunchGasBomb(muzzlePosition, finalAngle, flags, player);
			break;

		case GUN_PETRIFY:
			Petrifier(muzzlePosition, finalAngle, flags, player);
			break;

		case GUN_BROWN:
			BrownGun(muzzlePosition, finalAngle, flags, player);
			break;

		case GUN_CONFUSEBOMB:
			ConfuseBomb(muzzlePosition, finalAngle, flags, player);
			break;

		case GUN_GASGUN:
			GasGun(muzzlePosition, finalAngle, flags, player);
			break;

		case GUN_MINE:
			Mine(muzzlePosition, flags, player);
			break;

		case GUN_DYNAMITE:
			Dynamite(muzzlePosition, flags, player);
			break;

		case GUN_PULSERIFLE:
			PulseRifle(muzzlePosition, finalAngle, flags, player);
			break;

		default:
			// unknown gun?
			assert(0);
			break;
		}
	}

	w->lock = gGunDescriptions[w->gun].Lock;
	WeaponPlaySound(w, tilePosition);
	if (w->state != GUNSTATE_FIRING && w->state != GUNSTATE_RECOIL)
	{
		WeaponSetState(w, GUNSTATE_FIRING);
	}
}

void WeaponHoldFire(Weapon *w)
{
	WeaponSetState(w, GUNSTATE_READY);
}

void WeaponSetState(Weapon *w, gunstate_e state)
{
	w->state = state;
	switch (state)
	{
	case GUNSTATE_FIRING:
		w->stateCounter = 8;
		break;
	case GUNSTATE_RECOIL:
		// This is to make sure the gun stays recoiled as long as the gun is
		// "locked", i.e. cannot fire
		w->stateCounter = w->lock;
		break;
	default:
		w->stateCounter = -1;
		break;
	}
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
	return gGunDescriptions[gun].pic == GUNPIC_BLASTER;
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
