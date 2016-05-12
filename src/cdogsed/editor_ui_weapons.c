/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (c) 2013-2016, Cong Xu
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
#include "editor_ui_weapons.h"

#include <cdogs/font.h>

#include "editor_ui_common.h"


typedef enum
{
	SELECT_ALL,
	SELECT_NONE,
	SELECT_INVERSE
} SelectMode;
typedef struct
{
	CampaignOptions *co;
	SelectMode Mode;
} MissionAllGunsData;
static bool HasWeapon(const Mission *m, const GunDescription *g);
static void AddAllWeapons(Mission *m);
static void MissionSelectWeapons(void *vData, int d)
{
	UNUSED(d);
	MissionAllGunsData *data = vData;
	Mission *currentMission = CampaignGetCurrentMission(data->co);
	if (currentMission == NULL) return;
	switch (data->Mode)
	{
	case SELECT_ALL:
		AddAllWeapons(currentMission);
		break;
	case SELECT_NONE:
		CArrayClear(&currentMission->Weapons);
		break;
	case SELECT_INVERSE:
		{
			// Add all the weapons not currently available, then delete the first
			// entries in the weapons array corresponding to the previously
			// available weapons
			const int availableWeapons = (int)currentMission->Weapons.size;
			AddAllWeapons(currentMission);
			for (int i = 0; i < availableWeapons; i++)
			{
				CArrayDelete(&currentMission->Weapons, 0);
			}
		}
		break;
	default:
		CASSERT(false, "unknown case");
		break;
	}
}
static void AddAllWeapons(Mission *m)
{
	CA_FOREACH(const GunDescription, g, gGunDescriptions.Guns)
		if (g->IsRealGun && !HasWeapon(m, g))
		{
			CArrayPushBack(&m->Weapons, &g);
		}
	CA_FOREACH_END()
		CA_FOREACH(const GunDescription, g, gGunDescriptions.CustomGuns)
		if (g->IsRealGun && !HasWeapon(m, g))
		{
			CArrayPushBack(&m->Weapons, &g);
		}
	CA_FOREACH_END()
}

typedef struct
{
	CampaignOptions *co;
	const GunDescription *Gun;
} MissionGunData;
static void MissionDrawWeaponStatus(
	UIObject *o, GraphicsDevice *g, Vec2i pos, void *vData)
{
	UNUSED(g);
	const MissionGunData *data = vData;
	const Mission *currentMission = CampaignGetCurrentMission(data->co);
	if (currentMission == NULL) return;
	const bool hasWeapon = HasWeapon(currentMission, data->Gun);
	DisplayFlag(
		Vec2iAdd(pos, o->Pos),
		data->Gun->name,
		hasWeapon,
		UIObjectIsHighlighted(o));
}
static void MissionChangeWeapon(void *vData, int d)
{
	UNUSED(d);
	MissionGunData *data = vData;
	bool hasWeapon = false;
	int weaponIndex = -1;
	Mission *currentMission = CampaignGetCurrentMission(data->co);
	CA_FOREACH(const GunDescription *, desc, currentMission->Weapons)
		if (data->Gun == *desc)
		{
			hasWeapon = true;
			weaponIndex = _ca_index;
			break;
		}
	CA_FOREACH_END()
	if (hasWeapon)
	{
		CArrayDelete(&currentMission->Weapons, weaponIndex);
	}
	else
	{
		CArrayPushBack(&currentMission->Weapons, &data->Gun);
	}
}
// Check if the current mission has this gun available already
static bool HasWeapon(const Mission *m, const GunDescription *g)
{
	CA_FOREACH(const GunDescription *, desc, m->Weapons)
		if (g == *desc)
		{
			return true;
		}
	CA_FOREACH_END()
	return false;
}

#define HEADER_COLUMN_WIDTH 75
#define COLUMN_WIDTH 90

static void CreateWeaponSpecialToggleObj(
	CampaignOptions *co, UIObject *c, const Vec2i pos,
	const SelectMode mode, const char *label);
static void CreateWeaponToggleObjs(
	CampaignOptions *co, UIObject *c, const UIObject *o,
	int *idx, const int rows, CArray *guns);
UIObject *CreateWeaponObjs(CampaignOptions *co)
{
	const int th = FontH();
	UIObject *c = UIObjectCreate(
		UITYPE_CONTEXT_MENU, 0, Vec2iZero(), Vec2iZero());
	c->Flags = UI_ENABLED_WHEN_PARENT_HIGHLIGHTED_ONLY;

	// Add special toggle controls
	CreateWeaponSpecialToggleObj(co, c, Vec2iZero(), SELECT_ALL, "Select All");
	CreateWeaponSpecialToggleObj(
		co, c, Vec2iNew(HEADER_COLUMN_WIDTH, 0), SELECT_NONE, "Select None");
	CreateWeaponSpecialToggleObj(
		co, c, Vec2iNew(HEADER_COLUMN_WIDTH * 2, 0), SELECT_INVERSE, "Invert");

	// Create a dummy label that can be clicked to close the context menu
	CreateCloseLabel(c, Vec2iNew(HEADER_COLUMN_WIDTH * 3, 0));

	// Add a toggle entry for each gun
	UIObject *o = UIObjectCreate(
		UITYPE_CUSTOM, 0, Vec2iZero(), Vec2iNew(80, th));
	o->u.CustomDrawFunc = MissionDrawWeaponStatus;
	o->ChangeFunc = MissionChangeWeapon;
	o->ChangeDisablesContext = false;
	o->Flags = UI_LEAVE_YC;
	o->ChangesData = true;
	const int rows = 10;
	int idx = 0;
	CreateWeaponToggleObjs(co, c, o, &idx, rows, &gGunDescriptions.Guns);
	CreateWeaponToggleObjs(co, c, o, &idx, rows, &gGunDescriptions.CustomGuns);

	UIObjectDestroy(o);
	return c;
}
static void CreateWeaponSpecialToggleObj(
	CampaignOptions *co, UIObject *c, const Vec2i pos,
	const SelectMode mode, const char *label)
{
	const int th = FontH();
	UIObject *o = UIObjectCreate(
		UITYPE_LABEL, 0, Vec2iZero(), Vec2iNew(HEADER_COLUMN_WIDTH - 10, th));
	o->Label = label;
	o->ChangeFunc = MissionSelectWeapons;
	o->ChangeDisablesContext = false;
	o->ChangesData = true;
	CMALLOC(o->Data, sizeof(MissionAllGunsData));
	o->IsDynamicData = true;
	((MissionAllGunsData *)o->Data)->co = co;
	((MissionAllGunsData *)o->Data)->Mode = mode;
	o->Pos = pos;
	UIObjectAddChild(c, o);
}
static void CreateWeaponToggleObjs(
	CampaignOptions *co, UIObject *c, const UIObject *o,
	int *idx, const int rows, CArray *guns)
{
	const int th = FontH();
	CA_FOREACH(const GunDescription, g, *guns)
		if (!g->IsRealGun)
		{
			continue;
		}
		UIObject *o2 = UIObjectCopy(o);
		CMALLOC(o2->Data, sizeof(MissionGunData));
		o2->IsDynamicData = true;
		((MissionGunData *)o2->Data)->co = co;
		((MissionGunData *)o2->Data)->Gun = g;
		o2->Pos = Vec2iNew(
			*idx / rows * COLUMN_WIDTH, (*idx % rows) * th + th * 2);
		UIObjectAddChild(c, o2);
		(*idx)++;
	CA_FOREACH_END()
}
