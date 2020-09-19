/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.
	Copyright (c) 2015-2016, 2018, 2020 Cong Xu
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
#include "pickup.h"

#include "ammo.h"
#include "game_events.h"
#include "json_utils.h"
#include "log.h"
#include "mission.h"
#include "powerup.h"

PickupClasses gPickupClasses;

PickupType StrPickupType(const char *s)
{
	S2T(PICKUP_JEWEL, "Score");
	S2T(PICKUP_HEALTH, "Health");
	S2T(PICKUP_AMMO, "Ammo");
	S2T(PICKUP_KEYCARD, "Key");
	S2T(PICKUP_GUN, "Gun");
	S2T(PICKUP_SHOW_MAP, "ShowMap");
	return PICKUP_NONE;
}
const char *PickupTypeStr(const PickupType pt)
{
	switch (pt)
	{
		T2S(PICKUP_JEWEL, "Score");
		T2S(PICKUP_HEALTH, "Health");
		T2S(PICKUP_AMMO, "Ammo");
		T2S(PICKUP_KEYCARD, "Key");
		T2S(PICKUP_GUN, "Gun");
		T2S(PICKUP_SHOW_MAP, "ShowMap");
	default:
		return "";
	}
}

PickupClass *StrPickupClass(const char *s)
{
	if (s == NULL || strlen(s) == 0)
	{
		return NULL;
	}
	CA_FOREACH(PickupClass, c, gPickupClasses.CustomClasses)
	if (strcmp(s, c->Name) == 0)
	{
		return c;
	}
	CA_FOREACH_END()
	CA_FOREACH(PickupClass, c, gPickupClasses.Classes)
	if (strcmp(s, c->Name) == 0)
	{
		return c;
	}
	CA_FOREACH_END()
	CA_FOREACH(PickupClass, c, gPickupClasses.KeyClasses)
	if (strcmp(s, c->Name) == 0)
	{
		return c;
	}
	CA_FOREACH_END()
	CASSERT(false, "cannot parse pickup class");
	return NULL;
}
PickupClass *IntPickupClass(const int i)
{
	static const char *pickupItems[] = {
		"folder", "disk1", "disk2",	 "disk3", "blueprint", "cd",
		"sack",	  "holo",  "bottle", "radio", "pci_card",  "paper"};
#define PICKUPS_COUNT (sizeof(pickupItems) / sizeof(const char *))
	if (i < 0 || i >= (int)PICKUPS_COUNT)
	{
		return NULL;
	}
	return StrPickupClass(pickupItems[i]);
}
const char *IntExitStyle(const int i)
{
	static const char *exitStyles[] = {"hazard", "plate"};
	return exitStyles[abs(i) % 2];
}
#define KEYSTYLE_COUNT 4
const char *IntKeyStyle(const int style)
{
	static const char *keyStyles[] = {"office", "dungeon", "plain", "cube"};
	return keyStyles[abs(style) % KEYSTYLE_COUNT];
}
PickupClass *IntKeyPickupClass(const int style, const int i)
{
	return KeyPickupClass(IntKeyStyle(style), i);
}
// Define the key colours
static const char *keyColors[] = {"yellow", "green", "blue", "red"};
// TODO: support more colours
PickupClass *KeyPickupClass(const char *style, const int i)
{
	static char buf[256];
	sprintf(buf, "keys/%s/%s", style, keyColors[abs(i) % KEY_COUNT]);
	CA_FOREACH(PickupClass, c, gPickupClasses.KeyClasses)
	if (strcmp(buf, c->Name) == 0)
	{
		return c;
	}
	CA_FOREACH_END()
	CASSERT(false, "cannot parse key class");
	return NULL;
}
PickupClass *PickupClassGetById(PickupClasses *classes, const int id)
{
	if (id < (int)classes->Classes.size)
	{
		return CArrayGet(&classes->Classes, id);
	}
	else
	{
		return CArrayGet(
			&classes->CustomClasses, id - (int)classes->Classes.size);
	}
}
int StrPickupClassId(const char *s)
{
	if (s == NULL || strlen(s) == 0)
	{
		return 0;
	}
	CA_FOREACH(const PickupClass, c, gPickupClasses.CustomClasses)
	if (strcmp(s, c->Name) == 0)
	{
		return _ca_index + (int)gPickupClasses.Classes.size;
	}
	CA_FOREACH_END()
	CA_FOREACH(const PickupClass, c, gPickupClasses.Classes)
	if (strcmp(s, c->Name) == 0)
	{
		return _ca_index;
	}
	CA_FOREACH_END()
	CASSERT(false, "cannot parse pickup class name");
	return 0;
}

#define VERSION 2

void PickupClassesInit(
	PickupClasses *classes, const char *filename, const AmmoClasses *ammo,
	const WeaponClasses *guns)
{
	CArrayInit(&classes->Classes, sizeof(PickupClass));
	CArrayInit(&classes->CustomClasses, sizeof(PickupClass));
	CArrayInit(&classes->KeyClasses, sizeof(PickupClass));

	char buf[CDOGS_PATH_MAX];
	GetDataFilePath(buf, filename);
	FILE *f = fopen(buf, "r");
	json_t *root = NULL;
	if (f == NULL)
	{
		LOG(LM_MAIN, LL_ERROR, "Error: cannot load pickups file %s", buf);
		goto bail;
	}
	enum json_error e = json_stream_parse(f, &root);
	if (e != JSON_OK)
	{
		LOG(LM_MAIN, LL_ERROR, "Error parsing pickups file %s", buf);
		goto bail;
	}
	PickupClassesLoadJSON(&classes->Classes, root);
	PickupClassesLoadAmmo(&classes->Classes, &ammo->Ammo);
	PickupClassesLoadGuns(&classes->Classes, &guns->Guns);
	PickupClassesLoadKeys(&classes->KeyClasses);

bail:
	if (f != NULL)
	{
		fclose(f);
	}
	json_free_value(&root);
}
static bool TryLoadPickupclass(
	PickupClass *c, json_t *node, const int version);
void PickupClassesLoadJSON(CArray *classes, json_t *root)
{
	int version = -1;
	LoadInt(&version, root, "Version");
	if (version > VERSION || version <= 0)
	{
		CASSERT(false, "cannot read pickups file version");
		return;
	}

	json_t *pickupsNode = json_find_first_label(root, "Pickups")->child;
	for (json_t *child = pickupsNode->child; child; child = child->next)
	{
		PickupClass c;
		if (TryLoadPickupclass(&c, child, version))
		{
			CArrayPushBack(classes, &c);
		}
	}
}
static bool TryLoadPickupclass(PickupClass *c, json_t *node, const int version)
{
	memset(c, 0, sizeof *c);
	char *tmp;

	JSON_UTILS_LOAD_ENUM(c->Type, node, "Type", StrPickupType);
	switch (c->Type)
	{
	case PICKUP_JEWEL:
		// Set default score
		c->u.Score = PICKUP_SCORE;
		LoadInt(&c->u.Score, node, "Score");
		break;
	case PICKUP_HEALTH:
		// Set default heal amount
		c->u.Health = HEALTH_PICKUP_HEAL_AMOUNT;
		LoadInt(&c->u.Health, node, "Health");
		break;
	case PICKUP_AMMO: {
		tmp = GetString(node, "Ammo");
		c->u.Ammo.Id = StrAmmoId(tmp);
		CFREE(tmp);
		const Ammo *ammo = AmmoGetById(&gAmmo, c->u.Ammo.Id);
		// Set default ammo amount
		int amount = ammo->Amount;
		LoadInt(&amount, node, "Amount");
		c->u.Ammo.Amount = amount;
		break;
	}
	case PICKUP_KEYCARD:
		// Do nothing; keys now loaded directly from graphics files
		return false;
	case PICKUP_GUN:
		CASSERT(false, "unimplemented");
		break;
	case PICKUP_SHOW_MAP:
		// do nothing
		break;
	default:
		CASSERT(false, "Unknown pickup type");
		break;
	}
	c->Name = GetString(node, "Name");
	json_t *picNode = json_find_first_label(node, "Pic")->child;
	if (version < 2)
	{
		CPicLoadNormal(&c->Pic, picNode);
	}
	else
	{
		CPicLoadJSON(&c->Pic, picNode);
	}
	return true;
}

// TODO: move ammo pickups to pickups file; remove "ammo_"
void PickupClassesLoadAmmo(CArray *classes, const CArray *ammoClasses)
{
	CA_FOREACH(const Ammo, a, *ammoClasses)
	PickupClass c;
	char buf[256];
	sprintf(buf, "ammo_%s", a->Name);
	CSTRDUP(c.Name, buf);
	CPicCopyPic(&c.Pic, &a->Pic);
	c.Type = PICKUP_AMMO;
	c.u.Ammo.Id = StrAmmoId(a->Name);
	c.u.Ammo.Amount = a->Amount;
	CArrayPushBack(classes, &c);
	CA_FOREACH_END()
}

void PickupClassesLoadGuns(CArray *classes, const CArray *gunClasses)
{
	CA_FOREACH(const WeaponClass, wc, *gunClasses)
	PickupClass c;
	char buf[256];
	sprintf(buf, "gun_%s", wc->name);
	CSTRDUP(c.Name, buf);
	CPicInitNormal(&c.Pic, wc->Icon);
	c.Type = PICKUP_GUN;
	c.u.GunId = WeaponClassId(wc);
	CArrayPushBack(classes, &c);
	CA_FOREACH_END()
}

void PickupClassesLoadKeys(CArray *classes)
{
	CA_FOREACH(const char *, keyStyleName, gPicManager.keyStyleNames)
	for (int i = 0; i < KEY_COUNT; i++)
	{
		PickupClass c;
		memset(&c, 0, sizeof c);
		char buf[CDOGS_FILENAME_MAX];
		sprintf(buf, "keys/%s/%s", *keyStyleName, keyColors[i]);
		CSTRDUP(c.Name, buf);
		CPicInitNormalFromName(&c.Pic, c.Name);
		c.Type = PICKUP_KEYCARD;
		c.u.Keys = StrKeycard(keyColors[i]);
		CArrayPushBack(classes, &c);
	}
	CA_FOREACH_END()
}

void PickupClassesClear(CArray *classes)
{
	CA_FOREACH(PickupClass, c, *classes)
	CFREE(c->Name);
	CA_FOREACH_END()
	CArrayClear(classes);
}
void PickupClassesTerminate(PickupClasses *classes)
{
	PickupClassesClear(&classes->Classes);
	CArrayTerminate(&classes->Classes);
	PickupClassesClear(&classes->CustomClasses);
	CArrayTerminate(&classes->CustomClasses);
	PickupClassesClear(&classes->KeyClasses);
	CArrayTerminate(&classes->KeyClasses);
}

int PickupClassesCount(const PickupClasses *classes)
{
	return (int)classes->Classes.size + (int)classes->CustomClasses.size;
}

int PickupClassesGetScoreIdx(const PickupClass *p)
{
	if (p == NULL)
	{
		return 0;
	}
	int idx = -1;
	CA_FOREACH(const PickupClass, c, gPickupClasses.Classes)
	if (c->Type == PICKUP_JEWEL)
	{
		idx++;
		if (c == p)
		{
			return idx;
		}
	}
	CA_FOREACH_END()
	CA_FOREACH(PickupClass, c, gPickupClasses.CustomClasses)
	if (c->Type == PICKUP_JEWEL)
	{
		idx++;
		if (c == p)
		{
			return idx;
		}
	}
	CA_FOREACH_END()
	return 0;
}
int PickupClassesGetScoreCount(const PickupClasses *classes)
{
	int count = 0;
	CA_FOREACH(const PickupClass, c, classes->Classes)
	if (c->Type == PICKUP_JEWEL)
	{
		count++;
	}
	CA_FOREACH_END()
	CA_FOREACH(const PickupClass, c, classes->CustomClasses)
	if (c->Type == PICKUP_JEWEL)
	{
		count++;
	}
	CA_FOREACH_END()
	return count;
}
PickupClass *IntScorePickupClass(const int i)
{
	int idx = -1;
	CA_FOREACH(PickupClass, c, gPickupClasses.Classes)
	if (c->Type == PICKUP_JEWEL)
	{
		idx++;
		if (idx == i)
		{
			return c;
		}
	}
	CA_FOREACH_END()
	CA_FOREACH(PickupClass, c, gPickupClasses.CustomClasses)
	if (c->Type == PICKUP_JEWEL)
	{
		idx++;
		if (idx == i)
		{
			return c;
		}
	}
	CA_FOREACH_END()
	return NULL;
}
