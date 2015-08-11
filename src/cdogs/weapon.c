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
#include "weapon.h"

#include <assert.h>
#include <math.h>

#include <json/json.h>

#include "ammo.h"
#include "config.h"
#include "game_events.h"
#include "json_utils.h"
#include "net_util.h"
#include "objs.h"
#include "sounds.h"

GunClasses gGunDescriptions;

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
#define VERSION 1
void WeaponInitialize(GunClasses *g)
{
	memset(g, 0, sizeof *g);
	CArrayInit(&g->Guns, sizeof(const GunDescription));
	CArrayInit(&g->CustomGuns, sizeof(const GunDescription));
}
static void LoadGunDescription(
	GunDescription *g, json_t *node, const GunDescription *defaultGun);
void WeaponLoadJSON(GunClasses *g, CArray *classes, json_t *root)
{
	int version;
	LoadInt(&version, root, "Version");
	if (version > VERSION || version <= 0)
	{
		CASSERT(false, "cannot read guns file version");
		return;
	}

	GunDescription *defaultDesc = &g->Default;
	json_t *defaultNode = json_find_first_label(root, "DefaultGun");
	if (defaultNode != NULL)
	{
		LoadGunDescription(defaultDesc, defaultNode->child, NULL);
		for (int i = 0; i < GUN_COUNT; i++)
		{
			CArrayPushBack(&g->Guns, defaultDesc);
		}
	}
	json_t *gunsNode = json_find_first_label(root, "Guns")->child;
	for (json_t *child = gunsNode->child; child; child = child->next)
	{
		GunDescription gd;
		LoadGunDescription(&gd, child, defaultDesc);
		int idx = -1;
		LoadInt(&idx, child, "Index");
		CASSERT(
			!(idx >= 0 && idx < GUN_COUNT && classes != &g->Guns),
			"Cannot load gun with index as custom gun");
		if (idx >= 0 && idx < GUN_COUNT && classes == &g->Guns)
		{
			memcpy(CArrayGet(&g->Guns, idx), &gd, sizeof gd);
		}
		else
		{
			CArrayPushBack(classes, &gd);
		}
	}
	json_t *pseudoGunsNode = json_find_first_label(root, "PseudoGuns");
	if (pseudoGunsNode != NULL)
	{
		for (json_t *child = pseudoGunsNode->child->child;
			child;
			child = child->next)
		{
			GunDescription gd;
			LoadGunDescription(&gd, child, defaultDesc);
			gd.IsRealGun = false;
			CArrayPushBack(classes, &gd);
		}
	}
}
static void LoadGunDescription(
	GunDescription *g, json_t *node, const GunDescription *defaultGun)
{
	memset(g, 0, sizeof *g);
	g->AmmoId = -1;
	if (defaultGun)
	{
		memcpy(g, defaultGun, sizeof *g);
		if (defaultGun->name)
		{
			CSTRDUP(g->name, defaultGun->name);
		}
		if (defaultGun->Description)
		{
			CSTRDUP(g->Description, defaultGun->Description);
		}
		g->MuzzleHeight /= Z_FACTOR;
	}
	char *tmp;

	if (json_find_first_label(node, "Pic"))
	{
		tmp = GetString(node, "Pic");
		if (strcmp(tmp, "blaster") == 0)
		{
			g->pic = GUNPIC_BLASTER;
		}
		else if (strcmp(tmp, "knife") == 0)
		{
			g->pic = GUNPIC_KNIFE;
		}
		else
		{
			g->pic = -1;
		}
		CFREE(tmp);
	}

	if (json_find_first_label(node, "Icon"))
	{
		tmp = GetString(node, "Icon");
		g->Icon = PicManagerGet(&gPicManager, tmp, -1);
		CFREE(tmp);
	}

	g->name = GetString(node, "Name");

	LoadStr(&g->Description, node, "Description");

	if (json_find_first_label(node, "Bullet"))
	{
		tmp = GetString(node, "Bullet");
		g->Bullet = StrBulletClass(tmp);
		CFREE(tmp);
	}

	if (json_find_first_label(node, "Ammo"))
	{
		tmp = GetString(node, "Ammo");
		g->AmmoId = StrAmmoId(tmp);
		CFREE(tmp);
	}

	LoadInt(&g->Cost, node, "Cost");

	LoadInt(&g->Lock, node, "Lock");

	LoadInt(&g->ReloadLead, node, "ReloadLead");

	LoadSoundFromNode(&g->Sound, node, "Sound");
	LoadSoundFromNode(&g->ReloadSound, node, "ReloadSound");
	LoadSoundFromNode(&g->SwitchSound, node, "SwitchSound");

	LoadInt(&g->SoundLockLength, node, "SoundLockLength");

	LoadDouble(&g->Recoil, node, "Recoil");

	LoadInt(&g->Spread.Count, node, "SpreadCount");
	LoadDouble(&g->Spread.Width, node, "SpreadWidth");
	LoadDouble(&g->AngleOffset, node, "AngleOffset");

	LoadInt(&g->MuzzleHeight, node, "MuzzleHeight");
	g->MuzzleHeight *= Z_FACTOR;
	if (json_find_first_label(node, "Elevation"))
	{
		LoadInt(&g->ElevationLow, node, "Elevation");
		g->ElevationHigh = g->ElevationLow;
	}
	LoadInt(&g->ElevationLow, node, "ElevationLow");
	LoadInt(&g->ElevationHigh, node, "ElevationHigh");
	g->ElevationLow = MIN(g->ElevationLow, g->ElevationHigh);
	g->ElevationHigh = MAX(g->ElevationLow, g->ElevationHigh);
	if (json_find_first_label(node, "MuzzleFlashParticle"))
	{
		tmp = GetString(node, "MuzzleFlashParticle");
		g->MuzzleFlash = StrParticleClass(&gParticleClasses, tmp);
		CFREE(tmp);
	}

	if (json_find_first_label(node, "Brass"))
	{
		tmp = GetString(node, "Brass");
		g->Brass = StrParticleClass(&gParticleClasses, tmp);
		CFREE(tmp);
	}

	LoadBool(&g->CanShoot, node, "CanShoot");

	LoadInt(&g->ShakeAmount, node, "ShakeAmount");

	g->IsRealGun = true;
}
void WeaponTerminate(GunClasses *g)
{
	WeaponClassesClear(&g->Guns);
	CArrayTerminate(&g->Guns);
	WeaponClassesClear(&g->CustomGuns);
	CArrayTerminate(&g->CustomGuns);
}
void WeaponClassesClear(CArray *classes)
{
	for (int i = 0; i < (int)classes->size; i++)
	{
		GunDescription *gd = CArrayGet(classes, i);
		CFREE(gd->name);
		CFREE(gd->Description);
	}
	CArrayClear(classes);
}

Weapon WeaponCreate(const GunDescription *gun)
{
	Weapon w;
	memset(&w, 0, sizeof w);
	w.Gun = gun;
	w.state = GUNSTATE_READY;
	w.lock = 0;
	w.soundLock = 0;
	w.stateCounter = -1;
	return w;
}

// TODO: use map structure?
const GunDescription *StrGunDescription(const char *s)
{
	for (int i = 0; i < (int)gGunDescriptions.Guns.size; i++)
	{
		const GunDescription *gd = CArrayGet(&gGunDescriptions.Guns, i);
		if (strcmp(s, gd->name) == 0)
		{
			return gd;
		}
	}
	for (int i = 0; i < (int)gGunDescriptions.CustomGuns.size; i++)
	{
		const GunDescription *gd =
			CArrayGet(&gGunDescriptions.CustomGuns, i);
		if (strcmp(s, gd->name) == 0)
		{
			return gd;
		}
	}
	fprintf(stderr, "Cannot parse gun name: %s\n", s);
	return NULL;
}
GunDescription *IdGunDescription(const int i)
{
	CASSERT(
		i >= 0 &&
		i < (int)gGunDescriptions.Guns.size +
		(int)gGunDescriptions.CustomGuns.size,
		"Gun index out of bounds");
	if (i < (int)gGunDescriptions.Guns.size)
	{
		return CArrayGet(&gGunDescriptions.Guns, i);
	}
	return CArrayGet(
		&gGunDescriptions.CustomGuns, i - gGunDescriptions.Guns.size);
}
int GunDescriptionId(const GunDescription *g)
{
	int idx = 0;
	for (int i = 0; i < (int)gGunDescriptions.Guns.size; i++, idx++)
	{
		const GunDescription *c = CArrayGet(&gGunDescriptions.Guns, i);
		if (c == g)
		{
			return idx;
		}
	}
	for (int i = 0; i < (int)gGunDescriptions.CustomGuns.size; i++, idx++)
	{
		const GunDescription *c = CArrayGet(&gGunDescriptions.CustomGuns, i);
		if (c == g)
		{
			return idx;
		}
	}
	CASSERT(false, "cannot find gun");
	return -1;
}

void WeaponUpdate(
	Weapon *w, const int ticks, const Vec2i fullPos, const direction_e d,
	const int playerUID)
{
	// Reload sound
	if (ConfigGetBool(&gConfig, "Sound.Reloads") &&
		w->lock > w->Gun->ReloadLead &&
		w->lock - ticks <= w->Gun->ReloadLead &&
		w->lock > 0 &&
		w->Gun->ReloadSound != NULL)
	{
		GameEvent e = GameEventNew(GAME_EVENT_GUN_RELOAD);
		e.u.GunReload.PlayerUID = playerUID;
		strcpy(e.u.GunReload.Gun, w->Gun->name);
		e.u.GunReload.FullPos = Vec2i2Net(fullPos);
		e.u.GunReload.Direction = (int)d;
		GameEventsEnqueue(&gGameEvents, e);
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
	w->clickLock -= ticks;
	if (w->clickLock < 0)
	{
		w->clickLock = 0;
	}
	if (w->stateCounter >= 0)
	{
		w->stateCounter = MAX(0, w->stateCounter - ticks);
		if (w->stateCounter == 0 && w->state == GUNSTATE_FIRING)
		{
			WeaponSetState(w, GUNSTATE_RECOIL);
		}
	}
}

bool WeaponIsLocked(const Weapon *w)
{
	return w->lock > 0;
}

void WeaponFire(
	Weapon *w, const direction_e d, const Vec2i pos,
	const int flags, const int playerUID, const int uid)
{
	if (w->state != GUNSTATE_FIRING && w->state != GUNSTATE_RECOIL)
	{
		GameEvent e = GameEventNew(GAME_EVENT_GUN_STATE);
		e.u.GunState.ActorUID = uid;
		e.u.GunState.State = GUNSTATE_FIRING;
		GameEventsEnqueue(&gGameEvents, e);
	}
	if (!w->Gun->CanShoot)
	{
		return;
	}

	const double radians = dir2radians[d];
	const Vec2i muzzleOffset = GunGetMuzzleOffset(w->Gun, d);
	const Vec2i muzzlePosition = Vec2iAdd(pos, muzzleOffset);
	const bool playSound = w->soundLock <= 0;
	GunFire(
		w->Gun, muzzlePosition, w->Gun->MuzzleHeight, radians,
		flags, playerUID, uid, playSound, true);
	if (playSound)
	{
		w->soundLock = w->Gun->SoundLockLength;
	}

	w->lock = w->Gun->Lock;
}

void GunFire(
	const GunDescription *g, const Vec2i fullPos, const int z,
	const double radians,
	const int flags, const int playerUID, const int uid,
	const bool playSound, const bool isGun)
{
	GameEvent e = GameEventNew(GAME_EVENT_GUN_FIRE);
	e.u.GunFire.UID = uid;
	e.u.GunFire.PlayerUID = playerUID;
	strcpy(e.u.GunFire.Gun, g->name);
	e.u.GunFire.MuzzleFullPos = Vec2i2Net(fullPos);
	e.u.GunFire.Z = z;
	e.u.GunFire.Angle = (float)radians;
	e.u.GunFire.Sound = playSound;
	e.u.GunFire.Flags = flags;
	e.u.GunFire.IsGun = isGun;
	GameEventsEnqueue(&gGameEvents, e);
}

void GunAddBrass(
	const GunDescription *g, const direction_e d, const Vec2i pos)
{
	CASSERT(g->Brass, "Cannot create brass for no-brass weapon");
	GameEvent e = GameEventNew(GAME_EVENT_ADD_PARTICLE);
	e.u.AddParticle.Class = g->Brass;
	double x, y;
	const double radians = dir2radians[d];
	GetVectorsForRadians(radians, &x, &y);
	const Vec2i ejectionPortOffset = Vec2iReal2Full(Vec2iScale(Vec2iNew(
		(int)round(x), (int)round(y)), 7));
	const Vec2i muzzleOffset = GunGetMuzzleOffset(g, d);
	const Vec2i muzzlePosition = Vec2iAdd(pos, muzzleOffset);
	e.u.AddParticle.FullPos = Vec2iMinus(muzzlePosition, ejectionPortOffset);
	e.u.AddParticle.Z = g->MuzzleHeight;
	e.u.AddParticle.Vel = Vec2iScaleDiv(
		GetFullVectorsForRadians(radians + PI / 2), 3);
	e.u.AddParticle.Vel.x += (rand() % 128) - 64;
	e.u.AddParticle.Vel.y += (rand() % 128) - 64;
	e.u.AddParticle.Angle = RAND_DOUBLE(0, PI * 2);
	e.u.AddParticle.DZ = (rand() % 6) + 6;
	e.u.AddParticle.Spin = RAND_DOUBLE(-0.1, 0.1);
	GameEventsEnqueue(&gGameEvents, e);
}

Vec2i GunGetMuzzleOffset(const GunDescription *desc, const direction_e dir)
{
	if (!GunHasMuzzle(desc))
	{
		return Vec2iZero();
	}
	gunpic_e g = desc->pic;
	CASSERT(g >= 0, "Gun has no pic");
	int body = (int)g < 0 ? BODY_UNARMED : BODY_ARMED;
	Vec2i position = Vec2iNew(
		cGunHandOffset[body][dir].dx +
		cGunPics[g][dir][GUNSTATE_FIRING].dx +
		cMuzzleOffset[g][dir].dx,
		cGunHandOffset[body][dir].dy +
		cGunPics[g][dir][GUNSTATE_FIRING].dy +
		cMuzzleOffset[g][dir].dy + BULLET_Z);
	return Vec2iReal2Full(position);
}

void WeaponSetState(Weapon *w, const gunstate_e state)
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

bool GunHasMuzzle(const GunDescription *g)
{
	return g->pic == GUNPIC_BLASTER;
}
bool IsHighDPS(const GunDescription *g)
{
	if (g->Bullet == NULL)
	{
		return false;
	}
	// TODO: generalised way of determining explosive bullets
	return
		g->Bullet->Falling.DropGuns.size > 0 ||
		g->Bullet->OutOfRangeGuns.size > 0 ||
		g->Bullet->HitGuns.size > 0;
}
int GunGetRange(const GunDescription *g)
{
	const BulletClass *b = g->Bullet;
	if (b == NULL)
	{
		return 0;
	}
	const int speed = (b->SpeedLow + b->SpeedHigh) / 2;
	const int range = (b->RangeLow + b->RangeHigh) / 2;
	int effectiveRange = speed * range;
	if (b->Falling.GravityFactor != 0 && b->Falling.DestroyOnDrop)
	{
		// Halve effective range
		// TODO: this assumes a certain bouncing range
		effectiveRange /= 2;
	}
	return effectiveRange / 256;
}
bool IsLongRange(const GunDescription *g)
{
	return GunGetRange(g) > 130;
}
bool IsShortRange(const GunDescription *g)
{
	return GunGetRange(g) < 100;
}

void BulletAndWeaponInitialize(
	BulletClasses *b, GunClasses *g, const char *bpath, const char *gpath)
{
	BulletInitialize(b);

	FILE *bf = NULL;
	FILE *gf = NULL;
	json_t *broot = NULL;
	json_t *groot = NULL;
	enum json_error e;

	// 2-pass bullet loading will free root for us
	bool freeBRoot = true;
	bf = fopen(bpath, "r");
	if (bf == NULL)
	{
		printf("Error: cannot load bullets file %s\n", bpath);
		goto bail;
	}
	e = json_stream_parse(bf, &broot);
	if (e != JSON_OK)
	{
		printf("Error parsing bullets file %s [error %d]\n", bpath, (int)e);
		goto bail;
	}
	BulletLoadJSON(b, &b->Classes, broot);

	WeaponInitialize(g);
	gf = fopen(gpath, "r");
	if (gf == NULL)
	{
		printf("Error: cannot load guns file %s\n", gpath);
		goto bail;
	}
	e = json_stream_parse(gf, &groot);
	if (e != JSON_OK)
	{
		printf("Error parsing guns file %s [error %d]\n", gpath, (int)e);
		goto bail;
	}
	WeaponLoadJSON(g, &g->Guns, groot);

	BulletLoadWeapons(b);
	freeBRoot = false;

bail:
	if (bf)
	{
		fclose(bf);
	}
	if (gf)
	{
		fclose(gf);
	}
	if (freeBRoot)
	{
		json_free_value(&broot);
	}
	json_free_value(&groot);
}
