/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.
	Copyright (c) 2013-2022 Cong Xu
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

static const char *GunTypeStr(const GunType t)
{
	switch (t)
	{
		T2S(GUNTYPE_NORMAL, "Normal");
		T2S(GUNTYPE_MELEE, "Melee");
		T2S(GUNTYPE_GRENADE, "Grenade");
		T2S(GUNTYPE_MULTI, "Multi");
	default:
		return "";
	}
}

// Initialise all the static weapon data
#define VERSION 3
void WeaponClassesInitialize(WeaponClasses *wcs)
{
	memset(wcs, 0, sizeof *wcs);
	CArrayInit(&wcs->Guns, sizeof(WeaponClass));
	CArrayInit(&wcs->CustomGuns, sizeof(WeaponClass));
}
static void LoadWeaponClass(WeaponClass *wc, json_t *node, const int version);
static void WeaponClassTerminate(WeaponClass *wc);
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

	if (classes == &wcs->Guns)
	{
		CASSERT(wcs->Guns.size == 0, "guns not empty");
		for (int i = 0; i < GUN_COUNT; i++)
		{
			WeaponClass gd;
			memset(&gd, 0, sizeof gd);
			CArrayPushBack(&wcs->Guns, &gd);
		}
	}
	json_t *gunsNode = json_find_first_label(root, "Guns")->child;
	for (json_t *child = gunsNode->child; child; child = child->next)
	{
		WeaponClass gd;
		LoadWeaponClass(&gd, child, version);
		int idx = -1;
		// Only allow index for non-custom guns
		if (classes == &wcs->Guns)
		{
			LoadInt(&idx, child, "Index");
		}
		if (idx >= 0 && idx < GUN_COUNT)
		{
			WeaponClass *gExisting = CArrayGet(&wcs->Guns, idx);
			WeaponClassTerminate(gExisting);
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
		for (json_t *child = pseudoGunsNode->child->child; child;
			 child = child->next)
		{
			WeaponClass gd;
			LoadWeaponClass(&gd, child, version);
			gd.IsRealGun = false;
			CArrayPushBack(classes, &gd);
		}
	}
}
static void LoadWeaponClass(WeaponClass *wc, json_t *node, const int version)
{
	memset(wc, 0, sizeof *wc);

	const json_t *gunsNode = json_find_first_label(node, "Guns");
	if (gunsNode)
	{
		wc->Type = GUNTYPE_MULTI;
	}
	else
	{
		bool canShoot = true;
		LoadBool(&canShoot, node, "CanShoot");
		bool isGrenade = false;
		LoadBool(&isGrenade, node, "IsGrenade");
		if (isGrenade)
		{
			wc->Type = GUNTYPE_GRENADE;
		}
		else if (!canShoot)
		{
			wc->Type = GUNTYPE_MELEE;
		}
	}

	char *tmp;

	if (wc->Type != GUNTYPE_MULTI)
	{
		// Initialise default gun values
		CSTRDUP(wc->u.Normal.Sprites, "chars/guns/blaster");
		wc->u.Normal.Grips = 1;
		wc->Icon = PicManagerGetPic(&gPicManager, "peashooter");
		wc->u.Normal.Sound = StrSound("laserpew");
		wc->SwitchSound = StrSound("switch");
		wc->u.Normal.Spread.Count = 1;
		wc->u.Normal.MuzzleHeight = 10 * Z_FACTOR;
		wc->u.Normal.MuzzleFlash =
			StrParticleClass(&gParticleClasses, "muzzle_flash_default");
		wc->u.Normal.AmmoId = -1;
		wc->CanDrop = true;
	}

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

	LoadInt(&wc->Lock, node, "Lock");

	LoadSoundFromNode(&wc->SwitchSound, node, "SwitchSound");

	LoadBool(&wc->CanDrop, node, "CanDrop");

	LoadStr(&wc->DropGun, node, "DropGun");

	if (wc->Type == GUNTYPE_MULTI)
	{
		int i = 0;
		for (const json_t *gunNode = gunsNode->child->child; gunNode;
			 gunNode = gunNode->next, i++)
		{
			wc->u.Guns[i] = json_unescape(gunNode->text);
		}
	}
	else
	{
		tmp = NULL;
		LoadStr(&tmp, node, "Pic");
		if (tmp != NULL)
		{
			CFREE(wc->u.Normal.Sprites);
			wc->u.Normal.Sprites = NULL;
			if (strlen(tmp) > 0)
			{
				char buf[CDOGS_PATH_MAX];
				sprintf(buf, "chars/guns/%s", tmp);
				CSTRDUP(wc->u.Normal.Sprites, buf);
			}
			CFREE(tmp);
		}

		LoadInt(&wc->u.Normal.Grips, node, "Grips");

		CArrayInit(&wc->u.Normal.Bullets, sizeof(const BulletClass *));
		tmp = NULL;
		LoadStr(&tmp, node, "Bullet");
		if (tmp != NULL)
		{
			const BulletClass *bullet = StrBulletClass(tmp);
			CArrayPushBack(&wc->u.Normal.Bullets, &bullet);
			CFREE(tmp);
		}
		const json_t *bulletsNode = json_find_first_label(node, "Bullets");
		if (bulletsNode)
		{
			for (const json_t *bulletNode = bulletsNode->child->child;
				 bulletNode; bulletNode = bulletNode->next)
			{
				const BulletClass *bullet = StrBulletClass(bulletNode->text);
				CArrayPushBack(&wc->u.Normal.Bullets, &bullet);
			}
		}

		tmp = NULL;
		LoadStr(&tmp, node, "Ammo");
		if (tmp != NULL)
		{
			wc->u.Normal.AmmoId = StrAmmoId(tmp);
			if (wc->Type == GUNTYPE_GRENADE)
			{
				// Grenade weapons also allow the ammo pickups to act as gun
				// pickups
				Ammo *ammo = AmmoGetById(&gAmmo, wc->u.Normal.AmmoId);
				CFREE(ammo->DefaultGun);
				CSTRDUP(ammo->DefaultGun, wc->name);
				// Replace icon with that of the ammo
				wc->Icon = CPicGetPic(&ammo->Pic, 0);
			}
			CFREE(tmp);
		}

		LoadInt(&wc->u.Normal.Cost, node, "Cost");

		LoadInt(&wc->u.Normal.ReloadLead, node, "ReloadLead");

		LoadSoundFromNode(&wc->u.Normal.Sound, node, "Sound");
		LoadSoundFromNode(&wc->u.Normal.ReloadSound, node, "ReloadSound");

		wc->u.Normal.SoundLockLength = wc->Lock;
		LoadInt(&wc->u.Normal.SoundLockLength, node, "SoundLockLength");

		LoadFloat(&wc->u.Normal.Recoil, node, "Recoil");

		LoadInt(&wc->u.Normal.Spread.Count, node, "SpreadCount");
		LoadFloat(&wc->u.Normal.Spread.Width, node, "SpreadWidth");
		LoadFloat(&wc->u.Normal.AngleOffset, node, "AngleOffset");

		int muzzleHeight = 0;
		LoadInt(&muzzleHeight, node, "MuzzleHeight");
		if (muzzleHeight)
		{
			wc->u.Normal.MuzzleHeight = muzzleHeight * Z_FACTOR;
		}
		if (json_find_first_label(node, "Elevation"))
		{
			LoadInt(&wc->u.Normal.ElevationLow, node, "Elevation");
			wc->u.Normal.ElevationHigh = wc->u.Normal.ElevationLow;
		}
		LoadInt(&wc->u.Normal.ElevationLow, node, "ElevationLow");
		LoadInt(&wc->u.Normal.ElevationHigh, node, "ElevationHigh");
		wc->u.Normal.ElevationLow =
			MIN(wc->u.Normal.ElevationLow, wc->u.Normal.ElevationHigh);
		wc->u.Normal.ElevationHigh =
			MAX(wc->u.Normal.ElevationLow, wc->u.Normal.ElevationHigh);
		tmp = NULL;
		LoadStr(&tmp, node, "MuzzleFlashParticle");
		if (tmp != NULL)
		{
			wc->u.Normal.MuzzleFlash =
				StrParticleClass(&gParticleClasses, tmp);
			CFREE(tmp);
		}

		tmp = NULL;
		LoadStr(&tmp, node, "Brass");
		if (tmp != NULL)
		{
			wc->u.Normal.Brass = StrParticleClass(&gParticleClasses, tmp);
			CFREE(tmp);
		}

		if (version < 3)
		{
			LoadInt(&wc->u.Normal.Shake.Amount, node, "ShakeAmount");
		}
		else if (json_find_first_label(node, "Shake"))
		{
			json_t *shake = json_find_first_label(node, "Shake")->child;
			LoadInt(&wc->u.Normal.Shake.Amount, shake, "Amount");
			LoadBool(
				&wc->u.Normal.Shake.CameraSubjectOnly, shake,
				"CameraSubjectOnly");
		}

		LoadBool(&wc->u.Normal.Auto, node, "Auto");
	}

	wc->IsRealGun = true;

	if (version < 2)
	{
		CASSERT(wc->Type != GUNTYPE_MULTI, "unexpected gun type");
		if (wc->Type == GUNTYPE_MELEE)
		{
			wc->Lock = 0;
		}
	}

	LoadInt(&wc->Price, node, "Price");

	LOG(LM_MAP, LL_DEBUG, "loaded %s name(%s) lock(%d)...",
		GunTypeStr(wc->Type), wc->name, wc->Lock);
	LOG(LM_MAP, LL_DEBUG, "...canDrop(%s)", wc->CanDrop ? "true" : "false");
	if (wc->Type == GUNTYPE_MULTI)
	{
		LOG(LM_MAP, LL_DEBUG, "...guns{%s, %s}",
			wc->u.Guns[0] ? wc->u.Guns[0] : "",
			wc->u.Guns[1] ? wc->u.Guns[1] : "");
	}
	else
	{
		LOG(LM_MAP, LL_DEBUG, "bullets(");
		CA_FOREACH(const BulletClass *, bc, wc->u.Normal.Bullets)
		if (_ca_index > 0)
		{
			LOG(LM_MAP, LL_DEBUG, ", ");
		}
		LOG(LM_MAP, LL_DEBUG, "%s", (*bc)->Name);
		CA_FOREACH_END()
		LOG(LM_MAP, LL_DEBUG, ") ammo(%d) cost(%d)...", wc->u.Normal.AmmoId,
			wc->u.Normal.Cost);
		LOG(LM_MAP, LL_DEBUG,
			"...reloadLead(%d) soundLockLength(%d) recoil(%f)...",
			wc->u.Normal.ReloadLead, wc->u.Normal.SoundLockLength,
			wc->u.Normal.Recoil);
		LOG(LM_MAP, LL_DEBUG,
			"...spread(%frad x%d) angleOffset(%f) muzzleHeight(%d)...",
			wc->u.Normal.Spread.Width, wc->u.Normal.Spread.Count,
			wc->u.Normal.AngleOffset, wc->u.Normal.MuzzleHeight);
		LOG(LM_MAP, LL_DEBUG,
			"...elevation(%d-%d) muzzleFlash(%s) brass(%s)...",
			wc->u.Normal.ElevationLow, wc->u.Normal.ElevationHigh,
			wc->u.Normal.MuzzleFlash != NULL ? wc->u.Normal.MuzzleFlash->Name
											 : "",
			wc->u.Normal.Brass != NULL ? wc->u.Normal.Brass->Name : "");
		LOG(LM_MAP, LL_DEBUG, "...shake{amount(%d), cameraSubjectOnly(%s)}",
			wc->u.Normal.Shake.Amount,
			wc->u.Normal.Shake.CameraSubjectOnly ? "true" : "false");
		LOG(LM_MAP, LL_DEBUG, "...auto(%s)",
			wc->u.Normal.Auto ? "true" : "false");
	}

	LOG(LM_MAP, LL_DEBUG, "price(%d)", wc->Price);
}
void WeaponClassesTerminate(WeaponClasses *wcs)
{
	WeaponClassesClear(&wcs->Guns);
	CArrayTerminate(&wcs->Guns);
	WeaponClassesClear(&wcs->CustomGuns);
	CArrayTerminate(&wcs->CustomGuns);
}
void WeaponClassesClear(CArray *classes)
{
	CA_FOREACH(WeaponClass, g, *classes)
	WeaponClassTerminate(g);
	CA_FOREACH_END()
	CArrayClear(classes);
}
static void WeaponClassTerminate(WeaponClass *wc)
{
	CFREE(wc->name);
	CFREE(wc->Description);
	CFREE(wc->DropGun);
	if (wc->Type == GUNTYPE_MULTI)
	{
		for (int i = 0; i < MAX_BARRELS; i++)
		{
			CFREE(wc->u.Guns[i]);
		}
	}
	else
	{
		CFREE(wc->u.Normal.Sprites);
		CArrayTerminate(&wc->u.Normal.Bullets);
	}
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
	return NULL;
}
WeaponClass *IdWeaponClass(const int i)
{
	CASSERT(
		i >= 0 && i < (int)gWeaponClasses.Guns.size +
						  (int)gWeaponClasses.CustomGuns.size,
		"Gun index out of bounds");
	if (i < (int)gWeaponClasses.Guns.size)
	{
		return CArrayGet(&gWeaponClasses.Guns, i);
	}
	return CArrayGet(&gWeaponClasses.CustomGuns, i - gWeaponClasses.Guns.size);
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

void WeaponClassFire(
	const WeaponClass *wc, const struct vec2 pos, const float z,
	const double radians, const int flags, const int actorUID,
	const bool playSound, const bool isGun)
{
	CASSERT(wc->Type != GUNTYPE_MULTI, "unexpected gun type");
	GameEvent e = GameEventNew(GAME_EVENT_GUN_FIRE);
	e.u.GunFire.ActorUID = actorUID;
	strcpy(e.u.GunFire.Gun, wc->name);
	e.u.GunFire.MuzzlePos = Vec2ToNet(pos);
	// TODO: GunFire Z to float
	e.u.GunFire.Z = (int)z;
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
	CASSERT(wc->Type == GUNTYPE_NORMAL, "gun type can't have brass");
	CASSERT(
		wc->u.Normal.Brass != NULL, "Cannot create brass for no-brass weapon");
	GameEvent e = GameEventNew(GAME_EVENT_ADD_PARTICLE);
	e.u.AddParticle.Class = wc->u.Normal.Brass;
	const float radians = dir2radians[d];
	const struct vec2 ejectionPortOffset =
		svec2_scale(Vec2FromRadiansScaled(radians), 7);
	e.u.AddParticle.Pos = svec2_subtract(pos, ejectionPortOffset);
	e.u.AddParticle.Z = (float)wc->u.Normal.MuzzleHeight;
	e.u.AddParticle.Vel =
		svec2_scale(Vec2FromRadians(radians + MPI_2), 0.333333f);
	e.u.AddParticle.Vel.x += RAND_FLOAT(-0.25f, 0.25f);
	e.u.AddParticle.Vel.y += RAND_FLOAT(-0.25f, 0.25f);
	e.u.AddParticle.Angle = RAND_DOUBLE(0, MPI * 2);
	e.u.AddParticle.DZ = (float)((rand() % 6) + 6);
	e.u.AddParticle.Spin = RAND_DOUBLE(-0.1, 0.1);
	GameEventsEnqueue(&gGameEvents, e);
}

static struct vec2 GetMuzzleOffset(
	const direction_e d, const gunstate_e state);
struct vec2 WeaponClassGetBarrelMuzzleOffset(
	const WeaponClass *wc, const CharSprites *cs, const int barrel,
	direction_e dir, const gunstate_e state)
{
	if (wc->Type == GUNTYPE_MULTI)
	{
		wc = StrWeaponClass(wc->u.Guns[barrel]);
		CASSERT(wc != NULL, "cannot find gun");
	}
	else
	{
		CASSERT(barrel == 0, "unexpected barrel index");
	}
	if (!WeaponClassHasMuzzle(wc))
	{
		return svec2_zero();
	}
	CASSERT(wc->u.Normal.Sprites != NULL, "Gun has no pic");
	CASSERT(barrel < 2, "up to two barrels supported");
	// For the other barrel, mirror dir and offset along X axis
	if (barrel == 1)
	{
		dir = DirectionMirrorX(dir);
	}
	const struct vec2 gunOffset =
		cs->Offsets.Dir[barrel == 0 ? BODY_PART_GUN_R : BODY_PART_GUN_L][dir];
	const struct vec2 offset =
		svec2_add(gunOffset, GetMuzzleOffset(dir, state));
	if (barrel == 1)
	{
		return svec2(-offset.x, offset.y);
	}
	return offset;
}
static struct vec2 GetMuzzleOffset(const direction_e d, const gunstate_e state)
{
// TODO: gun-specific muzzle offsets
#define BARREL_LENGTH 10
#define BARREL_LENGTH_READY 7
	// Barrel slightly shortened when not firing
	const double barrelLength =
		state == GUNSTATE_FIRING ? BARREL_LENGTH : BARREL_LENGTH_READY;
	return svec2_scale(Vec2FromRadians(dir2radians[d]), (float)barrelLength);
}
float WeaponClassGetMuzzleHeight(
	const WeaponClass *wc, const gunstate_e state, const int barrel)
{
	// Muzzle slightly higher when not firing
	// TODO: convert MuzzleHeight to float
	const int muzzleHeight = WC_BARREL_ATTR(*wc, MuzzleHeight, barrel);
	return (
		float)(muzzleHeight + (state == GUNSTATE_FIRING ? 0 : 4 * Z_FACTOR));
}

bool WeaponClassHasMuzzle(const WeaponClass *wc)
{
	return wc->Type == GUNTYPE_NORMAL && wc->u.Normal.Sprites != NULL;
}
bool WeaponClassIsHighDPS(const WeaponClass *wc)
{
	if (wc->Type == GUNTYPE_MULTI)
	{
		return WeaponClassIsHighDPS(StrWeaponClass(wc->u.Guns[0])) ||
			   WeaponClassIsHighDPS(StrWeaponClass(wc->u.Guns[1]));
	}
	CA_FOREACH(const BulletClass *, bc, wc->u.Normal.Bullets)
	// TODO: generalised way of determining explosive bullets
	if ((*bc)->Falling.DropGuns.size > 0 || (*bc)->OutOfRangeGuns.size > 0 ||
		(*bc)->HitGuns.size > 0)
	{
		return true;
	}
	CA_FOREACH_END()
	return false;
}
float WeaponClassGetRange(const WeaponClass *wc)
{
	if (wc->Type == GUNTYPE_MULTI)
	{
		return (WeaponClassGetRange(StrWeaponClass(wc->u.Guns[0])) +
				WeaponClassGetRange(StrWeaponClass(wc->u.Guns[1]))) /
			   2.0f;
	}
	float maxRange = 0;
	CA_FOREACH(const BulletClass *, bc, wc->u.Normal.Bullets)
	const float speed = ((*bc)->SpeedLow + (*bc)->SpeedHigh) / 2;
	const int range = ((*bc)->RangeLow + (*bc)->RangeHigh) / 2;
	float effectiveRange = speed * range;
	if ((*bc)->Falling.GravityFactor != 0 && (*bc)->Falling.DestroyOnDrop)
	{
		// Halve effective range
		// TODO: this assumes a certain bouncing range
		effectiveRange *= 0.5f;
	}
	maxRange = MAX(maxRange, effectiveRange);
	CA_FOREACH_END()
	return maxRange;
}
bool WeaponClassIsLongRange(const WeaponClass *wc)
{
	return WeaponClassGetRange(wc) > 130;
}
bool WeaponClassIsShortRange(const WeaponClass *wc)
{
	return WeaponClassGetRange(wc) < 100;
}
bool WeaponClassCanShoot(const WeaponClass *wc)
{
	if (wc->Type == GUNTYPE_MULTI)
	{
		return WeaponClassCanShoot(WeaponClassGetBarrel(wc, 0)) ||
			   WeaponClassCanShoot(WeaponClassGetBarrel(wc, 1));
	}
	return wc->Type != GUNTYPE_MELEE;
}
int WeaponClassNumBarrels(const WeaponClass *wc)
{
	return wc->Type == GUNTYPE_MULTI ? 2 : 1;
}
const WeaponClass *WeaponClassGetBarrel(
	const WeaponClass *wc, const int barrel)
{
	if (wc->Type == GUNTYPE_MULTI)
	{
		return StrWeaponClass(wc->u.Guns[barrel]);
	}
	return wc;
}
const BulletClass *WeaponClassGetBullet(
	const WeaponClass *wc, const int barrel)
{
	if (barrel == -1)
	{
		return NULL;
	}
	wc = WeaponClassGetBarrel(wc, barrel);
	CA_FOREACH(const BulletClass *, bc, wc->u.Normal.Bullets)
	if (*bc)
		return *bc;
	CA_FOREACH_END()
	return NULL;
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
		LOG(LM_MAP, LL_ERROR, "Error parsing bullets file %s [error %d]", buf,
			(int)e);
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
		LOG(LM_MAP, LL_ERROR, "Error parsing guns file %s [error %d]", buf,
			(int)e);
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
