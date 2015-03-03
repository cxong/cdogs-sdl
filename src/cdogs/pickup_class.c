/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (c) 2015, Cong Xu
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
#include "map.h"
#include "powerup.h"


PickupClasses gPickupClasses;


PickupType StrPickupType(const char *s)
{
	S2T(PICKUP_JEWEL, "Score");
	S2T(PICKUP_HEALTH, "Health");
	S2T(PICKUP_AMMO, "Ammo");
	S2T(PICKUP_KEYCARD, "Key");
	S2T(PICKUP_GUN, "Gun");
	return PICKUP_NONE;
}

PickupClass *StrPickupClass(const char *s)
{
	if (s == NULL || strlen(s) == 0)
	{
		return NULL;
	}
	for (int i = 0; i < (int)gPickupClasses.CustomClasses.size; i++)
	{
		PickupClass *c = CArrayGet(&gPickupClasses.CustomClasses, i);
		if (strcmp(s, c->Name) == 0)
		{
			return c;
		}
	}
	for (int i = 0; i < (int)gPickupClasses.Classes.size; i++)
	{
		PickupClass *c = CArrayGet(&gPickupClasses.Classes, i);
		if (strcmp(s, c->Name) == 0)
		{
			return c;
		}
	}
	CASSERT(false, "cannot parse bullet name");
	return NULL;
}
PickupClass *IntPickupClass(const int i)
{
	static const char *pickupItems[] = {
		"folder",
		"disk1",
		"disk2",
		"disk3",
		"blueprint",
		"cd",
		"bag",
		"holo",
		"bottle",
		"radio",
		"circuit",
		"paper"
	};
#define PICKUPS_COUNT (sizeof(pickupItems)/sizeof(const char *))
	if (i < 0 || i >= (int)PICKUPS_COUNT)
	{
		return NULL;
	}
	return StrPickupClass(pickupItems[i]);
}
PickupClass *KeyPickupClass(const int style, const int i)
{
	// Define the key styles and colours
#define K(_s, _c) "key_" _s "_" _c
#define KS(_s) { K(_s, "yellow"), K(_s, "green"), K(_s, "blue"), K(_s, "red") }
	static const char *keys[][4] = {
		KS("office"), KS("dungeon"), KS("plain"), KS("cube")
	};
	// TODO: support more styles and colours
	if (style < 0 || style >= KEYSTYLE_COUNT || i < 0 || i >= KEY_COUNT)
	{
		return NULL;
	}
	return StrPickupClass(keys[style][i]);
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
	for (int i = 0; i < (int)gPickupClasses.CustomClasses.size; i++)
	{
		Ammo *a = CArrayGet(&gPickupClasses.CustomClasses, i);
		if (strcmp(s, a->Name) == 0)
		{
			return i + (int)gPickupClasses.Classes.size;
		}
	}
	for (int i = 0; i < (int)gPickupClasses.Classes.size; i++)
	{
		Ammo *a = CArrayGet(&gPickupClasses.Classes, i);
		if (strcmp(s, a->Name) == 0)
		{
			return i;
		}
	}
	CASSERT(false, "cannot parse pickup class name");
	return 0;
}

#define VERSION 1

void PickupClassesInit(
	PickupClasses *classes, const char *filename,
	const AmmoClasses *ammo, const GunClasses *guns)
{
	CArrayInit(&classes->Classes, sizeof(PickupClass));
	CArrayInit(&classes->CustomClasses, sizeof(PickupClass));

	FILE *f = fopen(filename, "r");
	json_t *root = NULL;
	if (f == NULL)
	{
		printf("Error: cannot load pickups file %s\n", filename);
		goto bail;
	}
	enum json_error e = json_stream_parse(f, &root);
	if (e != JSON_OK)
	{
		printf("Error parsing pickups file %s\n", filename);
		goto bail;
	}
	PickupClassesLoadJSON(&classes->Classes, root);
	PickupClassesLoadAmmo(&classes->Classes, &ammo->Ammo);
	PickupClassesLoadGuns(&classes->Classes, &guns->Guns);

bail:
	if (f != NULL)
	{
		fclose(f);
	}
	json_free_value(&root);
}
static void LoadPickupclass(PickupClass *c, json_t *node);
void PickupClassesLoadJSON(CArray *classes, json_t *root)
{
	int version;
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
		LoadPickupclass(&c, child);
		CArrayPushBack(classes, &c);
	}
}
static void LoadPickupclass(PickupClass *c, json_t *node)
{
	memset(c, 0, sizeof *c);
	char *tmp;

	c->Name = GetString(node, "Name");
	LoadPic(&c->Pic, node, "Pic", "OldPic");
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
	case PICKUP_AMMO:
		tmp = GetString(node, "Ammo");
		c->u.Ammo.Id = StrAmmoId(tmp);
		CFREE(tmp);
		LoadInt(&c->u.Ammo.Amount, node, "AmmoAmount");
		break;
	case PICKUP_KEYCARD:
		JSON_UTILS_LOAD_ENUM(c->u.Keys, node, "Key", StrKeycard);
		break;
	case PICKUP_GUN:
		CASSERT(false, "unimplemented");
		break;
	default:
		CASSERT(false, "Unknown pickup type");
		break;
	}
}

void PickupClassesLoadAmmo(CArray *classes, const CArray *ammoClasses)
{
	for (int i = 0; i < (int)ammoClasses->size; i++)
	{
		const Ammo *a = CArrayGet(ammoClasses, i);
		PickupClass c;
		char buf[256];
		sprintf(buf, "ammo_%s", a->Name);
		CSTRDUP(c.Name, buf);
		c.Pic = a->Pic;
		c.Type = PICKUP_AMMO;
		c.u.Ammo.Id = StrAmmoId(a->Name);
		c.u.Ammo.Amount = a->Amount;
		CArrayPushBack(classes, &c);
	}
}

void PickupClassesLoadGuns(CArray *classes, const CArray *gunClasses)
{
	for (int i = 0; i < (int)gunClasses->size; i++)
	{
		const GunDescription *g = CArrayGet(gunClasses, i);
		PickupClass c;
		char buf[256];
		sprintf(buf, "gun_%s", g->name);
		CSTRDUP(c.Name, buf);
		c.Pic = g->Icon;
		c.Type = PICKUP_GUN;
		c.u.GunId = GunDescriptionId(g);
		CArrayPushBack(classes, &c);
	}
}

void PickupClassesClear(CArray *classes)
{
	for (int i = 0; i < (int)classes->size; i++)
	{
		PickupClass *c = CArrayGet(classes, i);
		CFREE(c->Name);
	}
	CArrayClear(classes);
}
void PickupClassesTerminate(PickupClasses *classes)
{
	PickupClassesClear(&classes->Classes);
	CArrayTerminate(&classes->Classes);
	PickupClassesClear(&classes->CustomClasses);
	CArrayTerminate(&classes->CustomClasses);
}

int PickupClassesGetScoreCount(const PickupClasses *classes)
{
	int count = 0;
	for (int i = 0; i < (int)classes->Classes.size; i++)
	{
		const PickupClass *c = CArrayGet(&classes->Classes, i);
		if (c->Type == PICKUP_JEWEL)
		{
			count++;
		}
	}
	for (int i = 0; i < (int)classes->CustomClasses.size; i++)
	{
		const PickupClass *c = CArrayGet(&classes->CustomClasses, i);
		if (c->Type == PICKUP_JEWEL)
		{
			count++;
		}
	}
	return count;
}
