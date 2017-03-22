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

    Copyright (c) 2013-2017, Cong Xu
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
#include "log.h"
#include "net_util.h"
#include "objs.h"
#include "sounds.h"

GunClasses gGunDescriptions;

// Initialise all the static weapon data
#define VERSION 1
void WeaponInitialize(GunClasses *g)
{
	memset(g, 0, sizeof *g);
	CArrayInit(&g->Guns, sizeof(GunDescription));
	CArrayInit(&g->CustomGuns, sizeof(GunDescription));
}
static void LoadGunDescription(
	GunDescription *g, json_t *node, const GunDescription *defaultGun);
static void GunDescriptionTerminate(GunDescription *g);
void WeaponLoadJSON(GunClasses *g, CArray *classes, json_t *root)
{
	LOG(LM_MAP, LL_DEBUG, "loading weapons");
	int version;
	LoadInt(&version, root, "Version");
	if (version > VERSION || version <= 0)
	{
		CASSERT(false, "cannot read guns file version");
		return;
	}

	GunDescription *defaultDesc = &g->Default;
	// Only load default gun from main game data
	if (classes == &g->Guns)
	{
		json_t *defaultNode = json_find_first_label(root, "DefaultGun");
		if (defaultNode != NULL)
		{
			LoadGunDescription(defaultDesc, defaultNode->child, NULL);
		}
		else
		{
			memset(defaultDesc, 0, sizeof *defaultDesc);
		}
		CASSERT(g->Guns.size == 0, "guns not empty");
		for (int i = 0; i < GUN_COUNT; i++)
		{
			GunDescription gd;
			if (defaultNode != NULL)
			{
				LoadGunDescription(&gd, defaultNode->child, NULL);
			}
			else
			{
				memset(&gd, 0, sizeof gd);
			}
			CArrayPushBack(&g->Guns, &gd);
		}
	}
	json_t *gunsNode = json_find_first_label(root, "Guns")->child;
	for (json_t *child = gunsNode->child; child; child = child->next)
	{
		GunDescription gd;
		LoadGunDescription(&gd, child, defaultDesc);
		int idx = -1;
		// Only allow index for non-custom guns
		if (classes == &g->Guns)
		{
			LoadInt(&idx, child, "Index");
		}
		if (idx >= 0 && idx < GUN_COUNT)
		{
			GunDescription *gExisting = CArrayGet(&g->Guns, idx);
			GunDescriptionTerminate(gExisting);
			memcpy(gExisting, &gd, sizeof gd);
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

	tmp = NULL;
	LoadStr(&tmp, node, "Pic");
	if (tmp != NULL)
	{
		char buf[CDOGS_PATH_MAX];
		sprintf(buf, "chars/guns/%s", tmp);
		g->Pic = PicManagerGetSprites(&gPicManager, buf);
		CFREE(tmp);
	}

	const Pic *icon = NULL;
	LoadPic(&icon, node, "Icon");
	if (icon != NULL)
	{
		g->Icon = icon;
	}

	tmp = NULL;
	LoadStr(&tmp, node, "Name");
	if (tmp != NULL)
	{
		CFREE(g->name);
		g->name = tmp;
	}

	tmp = NULL;
	LoadStr(&tmp, node, "Description");
	if (tmp != NULL)
	{
		CFREE(g->Description);
		g->Description = tmp;
	}

	tmp = NULL;
	LoadStr(&tmp, node, "Bullet");
	if (tmp != NULL)
	{
		g->Bullet = StrBulletClass(tmp);
		CFREE(tmp);
	}

	tmp = NULL;
	LoadStr(&tmp, node, "Ammo");
	if (tmp != NULL)
	{
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
	tmp = NULL;
	LoadStr(&tmp, node, "MuzzleFlashParticle");
	if (tmp != NULL)
	{
		g->MuzzleFlash = StrParticleClass(&gParticleClasses, tmp);
		CFREE(tmp);
	}

	tmp = NULL;
	LoadStr(&tmp, node, "Brass");
	if (tmp != NULL)
	{
		g->Brass = StrParticleClass(&gParticleClasses, tmp);
		CFREE(tmp);
	}

	LoadBool(&g->CanShoot, node, "CanShoot");

	LoadBool(&g->CanDrop, node, "CanDrop");

	LoadInt(&g->ShakeAmount, node, "ShakeAmount");

	g->IsRealGun = true;

	LOG(LM_MAP, LL_DEBUG,
		"loaded gun name(%s) bullet(%s) ammo(%d) cost(%d) lock(%d)...",
		g->name, g->Bullet != NULL ? g->Bullet->Name : "", g->AmmoId, g->Cost,
		g->Lock);
	LOG(LM_MAP, LL_DEBUG,
		"...reloadLead(%d) soundLockLength(%d) recoil(%f)...",
		g->ReloadLead, g->SoundLockLength, g->Recoil);
	LOG(LM_MAP, LL_DEBUG,
		"...spread(%frad x%d) angleOffset(%f) muzzleHeight(%d)...",
		g->Spread.Width, g->Spread.Count, g->AngleOffset, g->MuzzleHeight);
	LOG(LM_MAP, LL_DEBUG,
		"...elevation(%d-%d) muzzleFlash(%s) brass(%s) canShoot(%s)...",
		g->ElevationLow, g->ElevationHigh,
		g->MuzzleFlash != NULL ? g->MuzzleFlash->Name : "",
		g->Brass != NULL ? g->Brass->Name : "",
		g->CanShoot ? "true" : "false");
	LOG(LM_MAP, LL_DEBUG,
		"...canDrop(%s) shakeAmount(%d)",
		g->CanDrop ? "true" : "false", g->ShakeAmount);
}
void WeaponTerminate(GunClasses *g)
{
	WeaponClassesClear(&g->Guns);
	CArrayTerminate(&g->Guns);
	WeaponClassesClear(&g->CustomGuns);
	CArrayTerminate(&g->CustomGuns);
	GunDescriptionTerminate(&g->Default);
}
void WeaponClassesClear(CArray *classes)
{
	CA_FOREACH(GunDescription, g, *classes)
		GunDescriptionTerminate(g);
	CA_FOREACH_END()
	CArrayClear(classes);
}
static void GunDescriptionTerminate(GunDescription *g)
{
	CFREE(g->name);
	CFREE(g->Description);
	memset(g, 0, sizeof *g);
}

int GunGetNumClasses(const GunClasses *g)
{
	return (int)g->Guns.size + (int)g->CustomGuns.size;
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
	CA_FOREACH(const GunDescription, gd, gGunDescriptions.CustomGuns)
		if (strcmp(s, gd->name) == 0)
		{
			return gd;
		}
	CA_FOREACH_END()
	CA_FOREACH(const GunDescription, gd, gGunDescriptions.Guns)
		if (strcmp(s, gd->name) == 0)
		{
			return gd;
		}
	CA_FOREACH_END()
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
GunDescription *IndexGunDescriptionReal(const int i)
{
	int j = 0;
	CA_FOREACH(GunDescription, g, gGunDescriptions.Guns)
		if (!g->IsRealGun)
		{
			continue;
		}
		if (j == i)
		{
			return g;
		}
		j++;
	CA_FOREACH_END()
	CA_FOREACH(GunDescription, g, gGunDescriptions.CustomGuns)
		if (!g->IsRealGun)
		{
			continue;
		}
		if (j == i)
		{
			return g;
		}
		j++;
	CA_FOREACH_END()
	CASSERT(false, "cannot find gun");
	return NULL;
}

void WeaponUpdate(Weapon *w, const int ticks)
{
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
		if (w->stateCounter == 0)
		{
			switch (w->state)
			{
			case GUNSTATE_FIRING:
				WeaponSetState(w, GUNSTATE_RECOIL);
				break;
			case GUNSTATE_RECOIL:
				WeaponSetState(w, GUNSTATE_FIRING);
				break;
			default:
				// do nothing
				break;
			}
		}
	}
}

bool WeaponIsLocked(const Weapon *w)
{
	return w->lock > 0;
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
	// Check configuration
	if (!ConfigGetBool(&gConfig, "Graphics.Brass"))
	{
		return;
	}
	CASSERT(g->Brass, "Cannot create brass for no-brass weapon");
	GameEvent e = GameEventNew(GAME_EVENT_ADD_PARTICLE);
	e.u.AddParticle.Class = g->Brass;
	double x, y;
	const double radians = dir2radians[d];
	GetVectorsForRadians(radians, &x, &y);
	const Vec2i ejectionPortOffset = Vec2iReal2Full(Vec2iScale(Vec2iNew(
		(int)round(x), (int)round(y)), 7));
	e.u.AddParticle.FullPos = Vec2iMinus(pos, ejectionPortOffset);
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

static Vec2i GetMuzzleOffset(const direction_e d);
Vec2i GunGetMuzzleOffset(
	const GunDescription *desc, const CharSprites *cs, const direction_e dir)
{
	if (!GunHasMuzzle(desc))
	{
		return Vec2iZero();
	}
	CASSERT(desc->Pic != NULL, "Gun has no pic");
	const Vec2i gunOffset = cs->Offsets.Dir[BODY_PART_GUN][dir];
	const Vec2i position = Vec2iAdd(gunOffset, GetMuzzleOffset(dir));
	return Vec2iReal2Full(position);
}
static Vec2i GetMuzzleOffset(const direction_e d)
{
	// TODO: gun-specific muzzle offsets
	#define BARREL_LENGTH 10
	Vec2i v = Vec2iFromPolar(BARREL_LENGTH, dir2radians[d]);
	v.y = v.y * 3 / 4;
	return v;
}

void WeaponSetState(Weapon *w, const gunstate_e state)
{
	w->state = state;
	switch (state)
	{
	case GUNSTATE_FIRING:
		w->stateCounter = 4;
		break;
	case GUNSTATE_RECOIL:
		// This is to make sure the gun stays recoiled as long as the gun is
		// "locked", i.e. cannot fire
		w->stateCounter = MAX(1, w->lock - 3);
		break;
	default:
		w->stateCounter = -1;
		break;
	}
}

bool GunHasMuzzle(const GunDescription *g)
{
	return g->Pic != NULL && g->CanShoot;
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
	char buf[CDOGS_PATH_MAX];
	GetDataFilePath(buf, bpath);
	bf = fopen(buf, "r");
	if (bf == NULL)
	{
		LOG(LM_MAP, LL_ERROR, "Error: cannot load bullets file %s", buf);
		goto bail;
	}
	e = json_stream_parse(bf, &broot);
	if (e != JSON_OK)
	{
		LOG(LM_MAP, LL_ERROR, "Error parsing bullets file %s [error %d]",
			buf, (int)e);
		goto bail;
	}
	BulletLoadJSON(b, &b->Classes, broot);

	WeaponInitialize(g);
	GetDataFilePath(buf, gpath);
	gf = fopen(buf, "r");
	if (gf == NULL)
	{
		LOG(LM_MAP, LL_ERROR, "Error: cannot load guns file %s", buf);
		goto bail;
	}
	e = json_stream_parse(gf, &groot);
	if (e != JSON_OK)
	{
		LOG(LM_MAP, LL_ERROR, "Error parsing guns file %s [error %d]",
			buf, (int)e);
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
