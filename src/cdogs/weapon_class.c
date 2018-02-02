/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (c) 2013-2018 Cong Xu
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
#include "weapon_class.h"

#include "ammo.h"
#include "game_events.h"
#include "json_utils.h"
#include "log.h"
#include "net_util.h"
#include "utils.h"


WeaponClasses gWeaponClasses;

// Initialise all the static weapon data
#define VERSION 2
void WeaponClassesInitialize(WeaponClasses *wcs)
{
	memset(wcs, 0, sizeof *wcs);
	CArrayInit(&wcs->Guns, sizeof(WeaponClass));
	CArrayInit(&wcs->CustomGuns, sizeof(WeaponClass));
}
static void LoadGunDescription(
	WeaponClass *wc, json_t *node, const WeaponClass *defaultGun,
	const int version);
static void GunDescriptionTerminate(WeaponClass *wc);
void WeaponClassesLoadJSON(WeaponClasses *wcs, CArray *classes, json_t *root)
{
	LOG(LM_MAP, LL_DEBUG, "loading weapons");
	int version;
	LoadInt(&version, root, "Version");
	if (version > VERSION || version <= 0)
	{
		CASSERT(false, "cannot read guns file version");
		return;
	}

	WeaponClass *defaultDesc = &wcs->Default;
	// Only load default gun from main game data
	if (classes == &wcs->Guns)
	{
		json_t *defaultNode = json_find_first_label(root, "DefaultGun");
		if (defaultNode != NULL)
		{
			LoadGunDescription(defaultDesc, defaultNode->child, NULL, version);
		}
		else
		{
			memset(defaultDesc, 0, sizeof *defaultDesc);
		}
		CASSERT(wcs->Guns.size == 0, "guns not empty");
		for (int i = 0; i < GUN_COUNT; i++)
		{
			WeaponClass gd;
			if (defaultNode != NULL)
			{
				LoadGunDescription(&gd, defaultNode->child, NULL, version);
			}
			else
			{
				memset(&gd, 0, sizeof gd);
			}
			CArrayPushBack(&wcs->Guns, &gd);
		}
	}
	json_t *gunsNode = json_find_first_label(root, "Guns")->child;
	for (json_t *child = gunsNode->child; child; child = child->next)
	{
		WeaponClass gd;
		LoadGunDescription(&gd, child, defaultDesc, version);
		int idx = -1;
		// Only allow index for non-custom guns
		if (classes == &wcs->Guns)
		{
			LoadInt(&idx, child, "Index");
		}
		if (idx >= 0 && idx < GUN_COUNT)
		{
			WeaponClass *gExisting = CArrayGet(&wcs->Guns, idx);
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
			WeaponClass gd;
			LoadGunDescription(&gd, child, defaultDesc, version);
			gd.IsRealGun = false;
			CArrayPushBack(classes, &gd);
		}
	}
}
static void LoadGunDescription(
	WeaponClass *wc, json_t *node, const WeaponClass *defaultGun,
	const int version)
{
	memset(wc, 0, sizeof *wc);
	wc->AmmoId = -1;
	if (defaultGun)
	{
		memcpy(wc, defaultGun, sizeof *wc);
		if (defaultGun->name)
		{
			CSTRDUP(wc->name, defaultGun->name);
		}
		if (defaultGun->Description)
		{
			CSTRDUP(wc->Description, defaultGun->Description);
		}
		wc->MuzzleHeight /= Z_FACTOR;
	}
	char *tmp;

	tmp = NULL;
	LoadStr(&tmp, node, "Pic");
	if (tmp != NULL)
	{
		char buf[CDOGS_PATH_MAX];
		sprintf(buf, "chars/guns/%s", tmp);
		wc->Pic = PicManagerGetSprites(&gPicManager, buf);
		CFREE(tmp);
	}

	LoadBool(&wc->IsGrenade, node, "IsGrenade");

	const Pic *icon = NULL;
	LoadPic(&icon, node, "Icon");
	if (icon != NULL)
	{
		wc->Icon = icon;
	}

	tmp = NULL;
	LoadStr(&tmp, node, "Name");
	if (tmp != NULL)
	{
		CFREE(wc->name);
		wc->name = tmp;
	}

	tmp = NULL;
	LoadStr(&tmp, node, "Description");
	if (tmp != NULL)
	{
		CFREE(wc->Description);
		wc->Description = tmp;
	}

	tmp = NULL;
	LoadStr(&tmp, node, "Bullet");
	if (tmp != NULL)
	{
		wc->Bullet = StrBulletClass(tmp);
		CFREE(tmp);
	}

	tmp = NULL;
	LoadStr(&tmp, node, "Ammo");
	if (tmp != NULL)
	{
		wc->AmmoId = StrAmmoId(tmp);
		CFREE(tmp);
	}

	LoadInt(&wc->Cost, node, "Cost");

	LoadInt(&wc->Lock, node, "Lock");

	LoadInt(&wc->ReloadLead, node, "ReloadLead");

	LoadSoundFromNode(&wc->Sound, node, "Sound");
	LoadSoundFromNode(&wc->ReloadSound, node, "ReloadSound");
	LoadSoundFromNode(&wc->SwitchSound, node, "SwitchSound");

	LoadInt(&wc->SoundLockLength, node, "SoundLockLength");

	LoadFloat(&wc->Recoil, node, "Recoil");

	LoadInt(&wc->Spread.Count, node, "SpreadCount");
	LoadFloat(&wc->Spread.Width, node, "SpreadWidth");
	LoadFloat(&wc->AngleOffset, node, "AngleOffset");

	LoadInt(&wc->MuzzleHeight, node, "MuzzleHeight");
	wc->MuzzleHeight *= Z_FACTOR;
	if (json_find_first_label(node, "Elevation"))
	{
		LoadInt(&wc->ElevationLow, node, "Elevation");
		wc->ElevationHigh = wc->ElevationLow;
	}
	LoadInt(&wc->ElevationLow, node, "ElevationLow");
	LoadInt(&wc->ElevationHigh, node, "ElevationHigh");
	wc->ElevationLow = MIN(wc->ElevationLow, wc->ElevationHigh);
	wc->ElevationHigh = MAX(wc->ElevationLow, wc->ElevationHigh);
	tmp = NULL;
	LoadStr(&tmp, node, "MuzzleFlashParticle");
	if (tmp != NULL)
	{
		wc->MuzzleFlash = StrParticleClass(&gParticleClasses, tmp);
		CFREE(tmp);
	}

	tmp = NULL;
	LoadStr(&tmp, node, "Brass");
	if (tmp != NULL)
	{
		wc->Brass = StrParticleClass(&gParticleClasses, tmp);
		CFREE(tmp);
	}

	LoadBool(&wc->CanShoot, node, "CanShoot");

	LoadBool(&wc->CanDrop, node, "CanDrop");

	LoadInt(&wc->ShakeAmount, node, "ShakeAmount");

	wc->IsRealGun = true;

	if (version < 2)
	{
		if (!wc->CanShoot)
		{
			wc->Lock = 0;
		}
	}

	LOG(LM_MAP, LL_DEBUG,
		"loaded gun name(%s) bullet(%s) ammo(%d) cost(%d) lock(%d)...",
		wc->name, wc->Bullet != NULL ? wc->Bullet->Name : "", wc->AmmoId,
		wc->Cost, wc->Lock);
	LOG(LM_MAP, LL_DEBUG,
		"...reloadLead(%d) soundLockLength(%d) recoil(%f)...",
		wc->ReloadLead, wc->SoundLockLength, wc->Recoil);
	LOG(LM_MAP, LL_DEBUG,
		"...spread(%frad x%d) angleOffset(%f) muzzleHeight(%d)...",
		wc->Spread.Width, wc->Spread.Count, wc->AngleOffset, wc->MuzzleHeight);
	LOG(LM_MAP, LL_DEBUG,
		"...elevation(%d-%d) muzzleFlash(%s) brass(%s) canShoot(%s)...",
		wc->ElevationLow, wc->ElevationHigh,
		wc->MuzzleFlash != NULL ? wc->MuzzleFlash->Name : "",
		wc->Brass != NULL ? wc->Brass->Name : "",
		wc->CanShoot ? "true" : "false");
	LOG(LM_MAP, LL_DEBUG,
		"...canDrop(%s) shakeAmount(%d)",
		wc->CanDrop ? "true" : "false", wc->ShakeAmount);
}
void WeaponClassesTerminate(WeaponClasses *wcs)
{
	WeaponClassesClear(&wcs->Guns);
	CArrayTerminate(&wcs->Guns);
	WeaponClassesClear(&wcs->CustomGuns);
	CArrayTerminate(&wcs->CustomGuns);
	GunDescriptionTerminate(&wcs->Default);
}
void WeaponClassesClear(CArray *classes)
{
	CA_FOREACH(WeaponClass, g, *classes)
		GunDescriptionTerminate(g);
	CA_FOREACH_END()
	CArrayClear(classes);
}
static void GunDescriptionTerminate(WeaponClass *wc)
{
	CFREE(wc->name);
	CFREE(wc->Description);
	memset(wc, 0, sizeof *wc);
}

// TODO: use map structure?
const WeaponClass *StrWeaponClass(const char *s)
{
	CA_FOREACH(const WeaponClass, gd, gWeaponClasses.CustomGuns)
		if (strcmp(s, gd->name) == 0)
		{
			return gd;
		}
	CA_FOREACH_END()
	CA_FOREACH(const WeaponClass, gd, gWeaponClasses.Guns)
		if (strcmp(s, gd->name) == 0)
		{
			return gd;
		}
	CA_FOREACH_END()
	fprintf(stderr, "Cannot parse gun name: %s\n", s);
	return NULL;
}
WeaponClass *IdWeaponClass(const int i)
{
	CASSERT(
		i >= 0 &&
		i < (int)gWeaponClasses.Guns.size +
		(int)gWeaponClasses.CustomGuns.size,
		"Gun index out of bounds");
	if (i < (int)gWeaponClasses.Guns.size)
	{
		return CArrayGet(&gWeaponClasses.Guns, i);
	}
	return CArrayGet(
		&gWeaponClasses.CustomGuns, i - gWeaponClasses.Guns.size);
}
int WeaponClassId(const WeaponClass *wc)
{
	int idx = 0;
	for (int i = 0; i < (int)gWeaponClasses.Guns.size; i++, idx++)
	{
		const WeaponClass *wc2 = CArrayGet(&gWeaponClasses.Guns, i);
		if (wc2 == wc)
		{
			return idx;
		}
	}
	for (int i = 0; i < (int)gWeaponClasses.CustomGuns.size; i++, idx++)
	{
		const WeaponClass *wc2 = CArrayGet(&gWeaponClasses.CustomGuns, i);
		if (wc2 == wc)
		{
			return idx;
		}
	}
	CASSERT(false, "cannot find gun");
	return -1;
}
WeaponClass *IndexWeaponClassReal(const int i)
{
	int j = 0;
	CA_FOREACH(WeaponClass, wc, gWeaponClasses.Guns)
		if (!wc->IsRealGun)
		{
			continue;
		}
		if (j == i)
		{
			return wc;
		}
		j++;
	CA_FOREACH_END()
	CA_FOREACH(WeaponClass, wc, gWeaponClasses.CustomGuns)
		if (!wc->IsRealGun)
		{
			continue;
		}
		if (j == i)
		{
			return wc;
		}
		j++;
	CA_FOREACH_END()
	CASSERT(false, "cannot find gun");
	return NULL;
}

void WeaponClassFire(
	const WeaponClass *wc, const struct vec2 pos, const int z,
	const double radians,
	const int flags, const int playerUID, const int uid,
	const bool playSound, const bool isGun)
{
	GameEvent e = GameEventNew(GAME_EVENT_GUN_FIRE);
	e.u.GunFire.UID = uid;
	e.u.GunFire.PlayerUID = playerUID;
	strcpy(e.u.GunFire.Gun, wc->name);
	e.u.GunFire.MuzzlePos = Vec2ToNet(pos);
	e.u.GunFire.Z = z;
	e.u.GunFire.Angle = (float)radians;
	e.u.GunFire.Sound = playSound;
	e.u.GunFire.Flags = flags;
	e.u.GunFire.IsGun = isGun;
	GameEventsEnqueue(&gGameEvents, e);
}

void WeaponClassAddBrass(
	const WeaponClass *wc, const direction_e d, const struct vec2 pos)
{
	// Check configuration
	if (!ConfigGetBool(&gConfig, "Graphics.Brass"))
	{
		return;
	}
	CASSERT(wc->Brass, "Cannot create brass for no-brass weapon");
	GameEvent e = GameEventNew(GAME_EVENT_ADD_PARTICLE);
	e.u.AddParticle.Class = wc->Brass;
	const float radians = dir2radians[d];
	const struct vec2 ejectionPortOffset = svec2_scale(
		Vec2FromRadiansScaled(radians), 7);
	e.u.AddParticle.Pos = svec2_subtract(pos, ejectionPortOffset);
	e.u.AddParticle.Z = wc->MuzzleHeight;
	e.u.AddParticle.Vel = svec2_scale(
		Vec2FromRadians(radians + MPI_2), 0.333333f);
	e.u.AddParticle.Vel.x += RAND_FLOAT(-0.25f, 0.25f);
	e.u.AddParticle.Vel.y += RAND_FLOAT(-0.25f, 0.25f);
	e.u.AddParticle.Angle = RAND_DOUBLE(0, M_PI * 2);
	e.u.AddParticle.DZ = (rand() % 6) + 6;
	e.u.AddParticle.Spin = RAND_DOUBLE(-0.1, 0.1);
	GameEventsEnqueue(&gGameEvents, e);
}

static struct vec2 GetMuzzleOffset(const direction_e d);
struct vec2 WeaponClassGetMuzzleOffset(
	const WeaponClass *desc, const CharSprites *cs, const direction_e dir)
{
	if (!WeaponClassHasMuzzle(desc))
	{
		return svec2_zero();
	}
	CASSERT(desc->Pic != NULL, "Gun has no pic");
	const struct vec2 gunOffset = cs->Offsets.Dir[BODY_PART_GUN][dir];
	return svec2_add(gunOffset, GetMuzzleOffset(dir));
}
static struct vec2 GetMuzzleOffset(const direction_e d)
{
	// TODO: gun-specific muzzle offsets
	#define BARREL_LENGTH 10
	return svec2_scale(Vec2FromRadians(dir2radians[d]), BARREL_LENGTH);
}

bool WeaponClassHasMuzzle(const WeaponClass *wc)
{
	return wc->Pic != NULL && wc->CanShoot;
}
bool WeaponClassIsHighDPS(const WeaponClass *wc)
{
	if (wc->Bullet == NULL)
	{
		return false;
	}
	// TODO: generalised way of determining explosive bullets
	return
		wc->Bullet->Falling.DropGuns.size > 0 ||
		wc->Bullet->OutOfRangeGuns.size > 0 ||
		wc->Bullet->HitGuns.size > 0;
}
float WeaponClassGetRange(const WeaponClass *wc)
{
	const BulletClass *b = wc->Bullet;
	if (b == NULL)
	{
		return 0;
	}
	const float speed = (b->SpeedLow + b->SpeedHigh) / 2;
	const int range = (b->RangeLow + b->RangeHigh) / 2;
	float effectiveRange = speed * range;
	if (b->Falling.GravityFactor != 0 && b->Falling.DestroyOnDrop)
	{
		// Halve effective range
		// TODO: this assumes a certain bouncing range
		effectiveRange *= 0.5f;
	}
	return effectiveRange;
}
bool WeaponClassIsLongRange(const WeaponClass *wc)
{
	return WeaponClassGetRange(wc) > 130;
}
bool WeaponClassIsShortRange(const WeaponClass *wc)
{
	return WeaponClassGetRange(wc) < 100;
}
const Pic *WeaponClassGetIcon(const WeaponClass *wc)
{
	return wc->Icon;
}

void BulletAndWeaponInitialize(
	BulletClasses *b, WeaponClasses *wcs, const char *bpath, const char *gpath)
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

	WeaponClassesInitialize(wcs);
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
	WeaponClassesLoadJSON(wcs, &wcs->Guns, groot);

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
