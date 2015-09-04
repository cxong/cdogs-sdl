/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
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
#include "editor_ui.h"

#include <assert.h>

#include <cdogs/door.h>
#include <cdogs/draw.h>
#include <cdogs/drawtools.h>
#include <cdogs/events.h>
#include <cdogs/files.h>
#include <cdogs/font.h>
#include <cdogs/mission.h>
#include <cdogs/mission_convert.h>
#include <cdogs/pic_manager.h>

#include "editor_ui_color.h"
#include "editor_ui_common.h"
#include "editor_ui_static.h"


#define Y_ABS 200

static void DrawStyleArea(
	Vec2i pos,
	const char *name,
	const Pic *pic,
	int idx, int count,
	int isHighlighted);

static char *CampaignGetTitle(UIObject *o, void *data)
{
	UNUSED(o);
	CampaignOptions *co = data;
	return co->Setting.Title;
}
static char **CampaignGetTitleSrc(void *data)
{
	CampaignOptions *co = data;
	return &co->Setting.Title;
}
static char *CampaignGetAuthor(UIObject *o, void *data)
{
	UNUSED(o);
	CampaignOptions *co = data;
	return co->Setting.Author;
}
static char **CampaignGetAuthorSrc(void *data)
{
	CampaignOptions *co = data;
	return &co->Setting.Author;
}
static char *CampaignGetDescription(UIObject *o, void *data)
{
	UNUSED(o);
	CampaignOptions *co = data;
	return co->Setting.Description;
}
static char **CampaignGetDescriptionSrc(void *data)
{
	CampaignOptions *co = data;
	return &co->Setting.Description;
}
static const char *CampaignGetSeedStr(UIObject *o, void *data)
{
	static char s[128];
	UNUSED(o);
	CampaignOptions *co = data;
	if (!CampaignGetCurrentMission(co)) return NULL;
	sprintf(s, "Seed: %u", co->seed);
	return s;
}
static void CheckMission(UIObject *o, void *data)
{
	CampaignOptions *co = data;
	if (!CampaignGetCurrentMission(co))
	{
		o->IsVisible = false;
		// Need to unhighlight to prevent children being drawn
		UIObjectUnhighlight(o);
		return;
	}
	o->IsVisible = true;
}
static void MissionCheckTypeClassic(UIObject *o, void *data)
{
	CampaignOptions *co = data;
	Mission *m = CampaignGetCurrentMission(co);
	if (!m || m->Type != MAPTYPE_CLASSIC)
	{
		o->IsVisible = false;
		// Need to unhighlight to prevent children being drawn
		UIObjectUnhighlight(o);
		return;
	}
	o->IsVisible = true;
}
static char *MissionGetTitle(UIObject *o, void *data)
{
	UNUSED(o);
	CampaignOptions *co = data;
	if (!CampaignGetCurrentMission(co))
	{
		return NULL;
	}
	return CampaignGetCurrentMission(co)->Title;
}
static void MissionCheckVisible(UIObject *o, void *data)
{
	CampaignOptions *co = data;
	o->IsVisible = CampaignGetCurrentMission(co) != NULL;
}
static char **MissionGetTitleSrc(void *data)
{
	CampaignOptions *co = data;
	if (!CampaignGetCurrentMission(co))
	{
		return NULL;
	}
	return &CampaignGetCurrentMission(co)->Title;
}
static char *MissionGetDescription(UIObject *o, void *data)
{
	UNUSED(o);
	CampaignOptions *co = data;
	if (!CampaignGetCurrentMission(co))
	{
		return NULL;
	}
	return CampaignGetCurrentMission(co)->Description;
}
static char **MissionGetDescriptionSrc(void *data)
{
	CampaignOptions *co = data;
	if (!CampaignGetCurrentMission(co))
	{
		return NULL;
	}
	return &CampaignGetCurrentMission(co)->Description;
}
static char *MissionGetSong(UIObject *o, void *data)
{
	UNUSED(o);
	CampaignOptions *co = data;
	if (!CampaignGetCurrentMission(co))
	{
		return NULL;
	}
	return CampaignGetCurrentMission(co)->Song;
}
static const char *MissionGetWidthStr(UIObject *o, void *data)
{
	static char s[128];
	UNUSED(o);
	CampaignOptions *co = data;
	if (!CampaignGetCurrentMission(co)) return NULL;
	sprintf(s, "Width: %d", CampaignGetCurrentMission(co)->Size.x);
	return s;
}
static const char *MissionGetHeightStr(UIObject *o, void *data)
{
	static char s[128];
	UNUSED(o);
	CampaignOptions *co = data;
	if (!CampaignGetCurrentMission(co)) return NULL;
	sprintf(s, "Height: %d", CampaignGetCurrentMission(co)->Size.y);
	return s;
}
static const char *MissionGetWallCountStr(UIObject *o, void *data)
{
	static char s[128];
	UNUSED(o);
	CampaignOptions *co = data;
	if (!CampaignGetCurrentMission(co)) return NULL;
	sprintf(s, "Walls: %d", CampaignGetCurrentMission(co)->u.Classic.Walls);
	return s;
}
static const char *MissionGetWallLengthStr(UIObject *o, void *data)
{
	static char s[128];
	UNUSED(o);
	CampaignOptions *co = data;
	if (!CampaignGetCurrentMission(co)) return NULL;
	sprintf(s, "Len: %d", CampaignGetCurrentMission(co)->u.Classic.WallLength);
	return s;
}
static const char *MissionGetCorridorWidthStr(UIObject *o, void *data)
{
	static char s[128];
	UNUSED(o);
	CampaignOptions *co = data;
	if (!CampaignGetCurrentMission(co)) return NULL;
	sprintf(s, "CorridorWidth: %d", CampaignGetCurrentMission(co)->u.Classic.CorridorWidth);
	return s;
}
static const char *MissionGetRoomCountStr(UIObject *o, void *data)
{
	static char s[128];
	UNUSED(o);
	CampaignOptions *co = data;
	if (!CampaignGetCurrentMission(co)) return NULL;
	sprintf(
		s, "Rooms: %d", CampaignGetCurrentMission(co)->u.Classic.Rooms.Count);
	return s;
}
static const char *MissionGetRoomMinStr(UIObject *o, void *data)
{
	static char s[128];
	UNUSED(o);
	CampaignOptions *co = data;
	if (!CampaignGetCurrentMission(co)) return NULL;
	sprintf(s, "RoomMin: %d", CampaignGetCurrentMission(co)->u.Classic.Rooms.Min);
	return s;
}
static const char *MissionGetRoomMaxStr(UIObject *o, void *data)
{
	static char s[128];
	UNUSED(o);
	CampaignOptions *co = data;
	if (!CampaignGetCurrentMission(co)) return NULL;
	sprintf(s, "RoomMax: %d", CampaignGetCurrentMission(co)->u.Classic.Rooms.Max);
	return s;
}
static void MissionDrawEdgeRooms(
	UIObject *o, GraphicsDevice *g, Vec2i pos, void *data)
{
	UNUSED(o);
	CampaignOptions *co = data;
	if (!CampaignGetCurrentMission(co)) return;
	DisplayFlag(
		g, Vec2iAdd(pos, o->Pos), "Edge rooms",
		CampaignGetCurrentMission(co)->u.Classic.Rooms.Edge,
		UIObjectIsHighlighted(o));
}
static void MissionDrawRoomsOverlap(
	UIObject *o, GraphicsDevice *g, Vec2i pos, void *data)
{
	UNUSED(o);
	UNUSED(pos);
	CampaignOptions *co = data;
	if (!CampaignGetCurrentMission(co)) return;
	DisplayFlag(
		g, Vec2iAdd(pos, o->Pos), "Room overlap",
		CampaignGetCurrentMission(co)->u.Classic.Rooms.Overlap,
		UIObjectIsHighlighted(o));
}
static const char *MissionGetRoomWallCountStr(UIObject *o, void *data)
{
	static char s[128];
	UNUSED(o);
	CampaignOptions *co = data;
	if (!CampaignGetCurrentMission(co)) return NULL;
	sprintf(s, "RoomWalls: %d", CampaignGetCurrentMission(co)->u.Classic.Rooms.Walls);
	return s;
}
static const char *MissionGetRoomWallLenStr(UIObject *o, void *data)
{
	static char s[128];
	UNUSED(o);
	CampaignOptions *co = data;
	if (!CampaignGetCurrentMission(co)) return NULL;
	sprintf(s, "RoomWallLen: %d", CampaignGetCurrentMission(co)->u.Classic.Rooms.WallLength);
	return s;
}
static const char *MissionGetRoomWallPadStr(UIObject *o, void *data)
{
	static char s[128];
	UNUSED(o);
	CampaignOptions *co = data;
	if (!CampaignGetCurrentMission(co)) return NULL;
	sprintf(s, "RoomWallPad: %d", CampaignGetCurrentMission(co)->u.Classic.Rooms.WallPad);
	return s;
}
static const char *MissionGetSquareCountStr(UIObject *o, void *data)
{
	static char s[128];
	UNUSED(o);
	CampaignOptions *co = data;
	if (!CampaignGetCurrentMission(co)) return NULL;
	sprintf(s, "Sqr: %d", CampaignGetCurrentMission(co)->u.Classic.Squares);
	return s;
}
static void MissionDrawDoorEnabled(
	UIObject *o, GraphicsDevice *g, Vec2i pos, void *data)
{
	UNUSED(o);
	CampaignOptions *co = data;
	if (!CampaignGetCurrentMission(co)) return;
	DisplayFlag(
		g, Vec2iAdd(pos, o->Pos), "Doors",
		CampaignGetCurrentMission(co)->u.Classic.Doors.Enabled,
		UIObjectIsHighlighted(o));
}
static const char *MissionGetDoorMinStr(UIObject *o, void *data)
{
	static char s[128];
	UNUSED(o);
	CampaignOptions *co = data;
	if (!CampaignGetCurrentMission(co)) return NULL;
	sprintf(s, "DoorMin: %d", CampaignGetCurrentMission(co)->u.Classic.Doors.Min);
	return s;
}
static const char *MissionGetDoorMaxStr(UIObject *o, void *data)
{
	static char s[128];
	UNUSED(o);
	CampaignOptions *co = data;
	if (!CampaignGetCurrentMission(co)) return NULL;
	sprintf(s, "DoorMax: %d", CampaignGetCurrentMission(co)->u.Classic.Doors.Max);
	return s;
}
static const char *MissionGetPillarCountStr(UIObject *o, void *data)
{
	static char s[128];
	UNUSED(o);
	CampaignOptions *co = data;
	if (!CampaignGetCurrentMission(co)) return NULL;
	sprintf(s, "Pillars: %d", CampaignGetCurrentMission(co)->u.Classic.Pillars.Count);
	return s;
}
static const char *MissionGetPillarMinStr(UIObject *o, void *data)
{
	static char s[128];
	UNUSED(o);
	CampaignOptions *co = data;
	if (!CampaignGetCurrentMission(co)) return NULL;
	sprintf(s, "PillarMin: %d", CampaignGetCurrentMission(co)->u.Classic.Pillars.Min);
	return s;
}
static const char *MissionGetPillarMaxStr(UIObject *o, void *data)
{
	static char s[128];
	UNUSED(o);
	CampaignOptions *co = data;
	if (!CampaignGetCurrentMission(co)) return NULL;
	sprintf(s, "PillarMax: %d", CampaignGetCurrentMission(co)->u.Classic.Pillars.Max);
	return s;
}
static const char *MissionGetDensityStr(UIObject *o, void *data)
{
	static char s[128];
	UNUSED(o);
	CampaignOptions *co = data;
	if (!CampaignGetCurrentMission(co)) return NULL;
	sprintf(s, "Dens: %d", CampaignGetCurrentMission(co)->EnemyDensity);
	return s;
}
static const char *MissionGetTypeStr(UIObject *o, void *data)
{
	static char s[128];
	UNUSED(o);
	CampaignOptions *co = data;
	if (!CampaignGetCurrentMission(co)) return NULL;
	sprintf(s, "Type: %s", MapTypeStr(CampaignGetCurrentMission(co)->Type));
	return s;
}
static void MissionDrawWallStyle(
	UIObject *o, GraphicsDevice *g, Vec2i pos, void *data)
{
	UNUSED(g);
	int count = WALL_STYLE_COUNT;
	CampaignOptions *co = data;
	const Mission *m = CampaignGetCurrentMission(co);
	if (!m) return;
	int idx = m->WallStyle;
	DrawStyleArea(
		Vec2iAdd(pos, o->Pos),
		"Wall",
		&PicManagerGetMaskedStylePic(
			&gPicManager, "wall", idx % count, WALL_SINGLE,
			m->WallMask, m->AltMask)->pic,
		idx, count,
		UIObjectIsHighlighted(o));
}
static void MissionDrawFloorStyle(
	UIObject *o, GraphicsDevice *g, Vec2i pos, void *data)
{
	UNUSED(g);
	int count = FLOOR_STYLE_COUNT;
	CampaignOptions *co = data;
	const Mission *m = CampaignGetCurrentMission(co);
	if (!m) return;
	const int idx = m->FloorStyle;
	DrawStyleArea(
		Vec2iAdd(pos, o->Pos),
		"Floor",
		&PicManagerGetMaskedStylePic(
			&gPicManager, "floor", idx % count, FLOOR_NORMAL,
			m->FloorMask, m->AltMask)->pic,
		idx, count,
		UIObjectIsHighlighted(o));
}
static void MissionDrawRoomStyle(
	UIObject *o, GraphicsDevice *g, Vec2i pos, void *data)
{
	UNUSED(g);
	CampaignOptions *co = data;
	const Mission *m = CampaignGetCurrentMission(co);
	if (!m) return;
	const int idx = m->RoomStyle;
	const int count = ROOM_STYLE_COUNT;
	DrawStyleArea(
		Vec2iAdd(pos, o->Pos),
		"Rooms",
		&PicManagerGetMaskedStylePic(
			&gPicManager, "room", idx % count, ROOMFLOOR_NORMAL,
			m->RoomMask, m->AltMask)->pic,
		idx, count,
		UIObjectIsHighlighted(o));
}
static void MissionDrawDoorStyle(
	UIObject *o, GraphicsDevice *g, Vec2i pos, void *data)
{
	UNUSED(g);
	CampaignOptions *co = data;
	if (!CampaignGetCurrentMission(co))
	{
		return;
	}
	const char *doorStyle = CampaignGetCurrentMission(co)->DoorStyle;
	DrawStyleArea(
		Vec2iAdd(pos, o->Pos),
		"Doors", &GetDoorPic(&gPicManager, doorStyle, "normal", true)->pic,
		PicManagerGetDoorStyleIndex(&gPicManager, doorStyle),
		(int)gPicManager.doorStyleNames.size,
		UIObjectIsHighlighted(o));
}
static void MissionDrawKeyStyle(
	UIObject *o, GraphicsDevice *g, Vec2i pos, void *data)
{
	UNUSED(g);
	int count = GetEditorInfo().keyCount;
	CampaignOptions *co = data;
	if (!CampaignGetCurrentMission(co))
	{
		return;
	}
	int idx = CampaignGetCurrentMission(co)->KeyStyle;
	DrawStyleArea(
		Vec2iAdd(pos, o->Pos),
		"Keys",
		KeyPickupClass(gMission.keyStyle, 0)->Pic,
		idx, count,
		UIObjectIsHighlighted(o));
}
static void MissionDrawExitStyle(
	UIObject *o, GraphicsDevice *g, Vec2i pos, void *data)
{
	UNUSED(g);
	int count = GetEditorInfo().exitCount;
	CampaignOptions *co = data;
	if (!CampaignGetCurrentMission(co))
	{
		return;
	}
	int idx = CampaignGetCurrentMission(co)->ExitStyle;
	DrawStyleArea(
		Vec2iAdd(pos, o->Pos),
		"Exit",
		&gMission.exitPic->pic,
		idx, count,
		UIObjectIsHighlighted(o));
}
static const char *MissionGetCharacterCountStr(UIObject *o, void *data)
{
	static char s[128];
	UNUSED(o);
	CampaignOptions *co = data;
	if (!CampaignGetCurrentMission(co)) return NULL;
	sprintf(
		s, "Characters (%d)", (int)CampaignGetCurrentMission(co)->Enemies.size);
	return s;
}
static const char *MissionGetSpecialCountStr(UIObject *o, void *data)
{
	static char s[128];
	UNUSED(o);
	CampaignOptions *co = data;
	if (!CampaignGetCurrentMission(co)) return NULL;
	sprintf(
		s, "Mission objective characters (%d)",
		(int)CampaignGetCurrentMission(co)->SpecialChars.size);
	return s;
}
typedef struct
{
	CampaignOptions *Campaign;
	int MissionObjectiveIndex;
} MissionObjectiveData;
static char *MissionGetObjectiveDescription(UIObject *o, void *data)
{
	MissionObjectiveData *mData = data;
	Mission *m = CampaignGetCurrentMission(mData->Campaign);
	if (!m)
	{
		return NULL;
	}
	int i = mData->MissionObjectiveIndex;
	if ((int)m->Objectives.size <= i)
	{
		if (i == 0)
		{
			// first objective and mission has no objectives
			o->u.Textbox.IsEditable = 0;
			return "-- mission objectives --";
		}
		return NULL;
	}
	o->u.Textbox.IsEditable = 1;
	return ((MissionObjective *)CArrayGet(&m->Objectives, i))->Description;
}
static void MissionCheckObjectiveDescription(UIObject *o, void *data)
{
	MissionObjectiveData *mData = data;
	Mission *m = CampaignGetCurrentMission(mData->Campaign);
	if (!m)
	{
		o->IsVisible = false;
		return;
	}
	int i = mData->MissionObjectiveIndex;
	if ((int)m->Objectives.size <= i)
	{
		if (i == 0)
		{
			// first objective and mission has no objectives
			o->IsVisible = true;
			return;
		}
		o->IsVisible = false;
		return;
	}
	o->IsVisible = true;
}
static char **MissionGetObjectiveDescriptionSrc(void *data)
{
	MissionObjectiveData *mData = data;
	Mission *m = CampaignGetCurrentMission(mData->Campaign);
	if (!m)
	{
		return NULL;
	}
	int i = mData->MissionObjectiveIndex;
	if ((int)m->Objectives.size <= i)
	{
		return NULL;
	}
	return &((MissionObjective *)CArrayGet(&m->Objectives, i))->Description;
}
static const char *GetWeaponCountStr(UIObject *o, void *v)
{
	static char s[128];
	UNUSED(o);
	UNUSED(v);
	Mission *m = CampaignGetCurrentMission(&gCampaign);
	if (!m)
	{
		return NULL;
	}
	int totalWeapons = 0;
	for (int i = 0; i < (int)gGunDescriptions.Guns.size; i++)
	{
		const GunDescription *g = CArrayGet(&gGunDescriptions.Guns, i);
		if (g->IsRealGun)
		{
			totalWeapons++;
		}
	}
	for (int i = 0; i < (int)gGunDescriptions.CustomGuns.size; i++)
	{
		const GunDescription *g = CArrayGet(&gGunDescriptions.CustomGuns, i);
		if (g->IsRealGun)
		{
			totalWeapons++;
		}
	}
	sprintf(
		s, "Available weapons (%d/%d)",
		(int)gMission.missionData->Weapons.size, totalWeapons);
	return s;
}
static const char *GetObjectCountStr(UIObject *o, void *v)
{
	static char s[128];
	UNUSED(o);
	UNUSED(v);
	Mission *m = CampaignGetCurrentMission(&gCampaign);
	if (!m)
	{
		return NULL;
	}
	sprintf(
		s, "Map items (%d)",
		(int)gMission.missionData->MapObjectDensities.size);
	return s;
}
typedef struct
{
	CampaignOptions *co;
	int index;
} MissionIndexData;
static void MissionDrawEnemy(
	UIObject *o, GraphicsDevice *g, Vec2i pos, void *vData)
{
	UNUSED(g);
	MissionIndexData *data = vData;
	if (!CampaignGetCurrentMission(data->co)) return;
	if (data->index >= (int)CampaignGetCurrentMission(data->co)->Enemies.size)
	{
		return;
	}
	const CharacterStore *store = &data->co->Setting.characters;
	const int charIndex = *(int *)CArrayGet(&store->baddieIds, data->index);
	DisplayCharacter(
		Vec2iAdd(Vec2iAdd(pos, o->Pos), Vec2iScaleDiv(o->Size, 2)),
		CArrayGet(&store->OtherChars, charIndex),
		UIObjectIsHighlighted(o), 1);
}
static void MissionDrawSpecialChar(
	UIObject *o, GraphicsDevice *g, Vec2i pos, void *vData)
{
	UNUSED(g);
	MissionIndexData *data = vData;
	if (!CampaignGetCurrentMission(data->co)) return;
	if (data->index >=
		(int)CampaignGetCurrentMission(data->co)->SpecialChars.size)
	{
		return;
	}
	const CharacterStore *store = &data->co->Setting.characters;
	const int charIndex = CharacterStoreGetSpecialId(store, data->index);
	DisplayCharacter(
		Vec2iAdd(Vec2iAdd(pos, o->Pos), Vec2iScaleDiv(o->Size, 2)),
		CArrayGet(&store->OtherChars, charIndex),
		UIObjectIsHighlighted(o), 1);
}
static void DisplayMapItemWithDensity(
	GraphicsDevice *g,
	const Vec2i pos, const MapObjectDensity *mod, const bool isHighlighted);
static void MissionDrawMapItem(
	UIObject *o, GraphicsDevice *g, Vec2i pos, void *vData)
{
	MissionIndexData *data = vData;
	if (!CampaignGetCurrentMission(data->co)) return;
	const Mission *m = CampaignGetCurrentMission(data->co);
	if (data->index >= (int)m->MapObjectDensities.size) return;
	const MapObjectDensity *mod =
		CArrayGet(&m->MapObjectDensities, data->index);
	DisplayMapItemWithDensity(
		g,
		Vec2iAdd(Vec2iAdd(pos, o->Pos), Vec2iScaleDiv(o->Size, 2)),
		mod,
		UIObjectIsHighlighted(o));
}
typedef struct
{
	CampaignOptions *co;
	const GunDescription *Gun;
} MissionGunData;
static void MissionDrawWeaponStatus(
	UIObject *o, GraphicsDevice *g, Vec2i pos, void *vData)
{
	const MissionGunData *data = vData;
	const Mission *currentMission = CampaignGetCurrentMission(data->co);
	if (currentMission == NULL) return;
	bool hasWeapon = false;
	for (int i = 0; i < (int)currentMission->Weapons.size; i++)
	{
		const GunDescription **desc = CArrayGet(&currentMission->Weapons, i);
		if (data->Gun == *desc)
		{
			hasWeapon = true;
			break;
		}
	}
	DisplayFlag(
		g,
		Vec2iAdd(pos, o->Pos),
		data->Gun->name,
		hasWeapon,
		UIObjectIsHighlighted(o));
}
static const char *MissionGetObjectiveStr(UIObject *o, void *vData)
{
	UNUSED(o);
	MissionIndexData *data = vData;
	if (!CampaignGetCurrentMission(data->co)) return NULL;
	if ((int)CampaignGetCurrentMission(data->co)->Objectives.size <= data->index) return NULL;
	return ObjectiveTypeStr(((MissionObjective *)CArrayGet(
		&CampaignGetCurrentMission(data->co)->Objectives, data->index))->Type);
}
static void GetCharacterHeadPic(
	Character *c, TOffsetPic *pic, TranslationTable **t);
static void MissionDrawObjective(
	UIObject *o, GraphicsDevice *g, Vec2i pos, void *vData)
{
	MissionIndexData *data = vData;
	CharacterStore *store = &data->co->Setting.characters;
	TOffsetPic pic;
	pic.dx = pic.dy = 0;
	pic.picIndex = -1;
	TranslationTable *table = NULL;
	UNUSED(g);
	if (!CampaignGetCurrentMission(data->co)) return;
	if ((int)CampaignGetCurrentMission(data->co)->Objectives.size <= data->index) return;
	// TODO: only one kill and rescue objective allowed
	const ObjectiveDef *obj = CArrayGet(&gMission.Objectives, data->index);
	const Pic *newPic = NULL;
	switch (((MissionObjective *)CArrayGet(
		&CampaignGetCurrentMission(data->co)->Objectives, data->index))->Type)
	{
	case OBJECTIVE_KILL:
		if (store->specialIds.size > 0)
		{
			Character *cd = CArrayGet(
				&store->OtherChars, CharacterStoreGetSpecialId(store, 0));
			GetCharacterHeadPic(cd, &pic, &table);
		}
		break;
	case OBJECTIVE_RESCUE:
		if (store->prisonerIds.size > 0)
		{
			Character *cd = CArrayGet(
				&store->OtherChars, CharacterStoreGetPrisonerId(store, 0));
			GetCharacterHeadPic(cd, &pic, &table);
		}
		break;
	case OBJECTIVE_COLLECT:
		newPic = obj->pickupClass->Pic;
		break;
	case OBJECTIVE_DESTROY:
		newPic = obj->blowupObject->Normal.Pic;
		break;
	case OBJECTIVE_INVESTIGATE:
		// no picture
		break;
	default:
		assert(0 && "Unknown objective type");
		return;
	}
	const Vec2i drawPos =
		Vec2iAdd(Vec2iAdd(pos, o->Pos), Vec2iScaleDiv(o->Size, 2));
	if (pic.picIndex >= 0)
	{
		DrawTTPic(
			drawPos.x + pic.dx, drawPos.y + pic.dy,
			PicManagerGetOldPic(&gPicManager, pic.picIndex), table);
	}
	else if (newPic != NULL)
	{
		Blit(g, newPic, Vec2iMinus(drawPos, Vec2iScaleDiv(newPic->size, 2)));
	}
}
static MissionObjective *GetMissionObjective(const Mission *m, const int idx)
{
	return CArrayGet(&m->Objectives, idx);
}
static const char *MissionGetObjectiveRequired(UIObject *o, void *vData)
{
	static char s[128];
	UNUSED(o);
	MissionIndexData *data = vData;
	if (!CampaignGetCurrentMission(data->co)) return NULL;
	if ((int)CampaignGetCurrentMission(data->co)->Objectives.size <=
		data->index)
	{
		return NULL;
	}
	sprintf(s, "%d", GetMissionObjective(
		CampaignGetCurrentMission(data->co), data->index)->Required);
	return s;
}
static const char *MissionGetObjectiveTotal(UIObject *o, void *vData)
{
	static char s[128];
	UNUSED(o);
	MissionIndexData *data = vData;
	if (!CampaignGetCurrentMission(data->co)) return NULL;
	if ((int)CampaignGetCurrentMission(data->co)->Objectives.size <=
		data->index)
	{
		return NULL;
	}
	sprintf(s, "out of %d", GetMissionObjective(
		CampaignGetCurrentMission(data->co), data->index)->Count);
	return s;
}
static const char *MissionGetObjectiveFlags(UIObject *o, void *vData)
{
	int flags;
	static char s[128];
	UNUSED(o);
	MissionIndexData *data = vData;
	if (!CampaignGetCurrentMission(data->co)) return NULL;
	if ((int)CampaignGetCurrentMission(data->co)->Objectives.size <=
		data->index)
	{
		return NULL;
	}
	flags = GetMissionObjective(
		CampaignGetCurrentMission(data->co), data->index)->Flags;
	if (!flags)
	{
		return "(normal)";
	}
	sprintf(s, "%s %s %s %s %s",
		(flags & OBJECTIVE_HIDDEN) ? "hidden" : "",
		(flags & OBJECTIVE_POSKNOWN) ? "pos.known" : "",
		(flags & OBJECTIVE_HIACCESS) ? "access" : "",
		(flags & OBJECTIVE_UNKNOWNCOUNT) ? "no-count" : "",
		(flags & OBJECTIVE_NOACCESS) ? "no-access" : "");
	return s;
}

static void DrawStyleArea(
	Vec2i pos,
	const char *name,
	const Pic *pic,
	int idx, int count,
	int isHighlighted)
{
	char buf[16];
	FontStrMask(name, pos, isHighlighted ? colorRed : colorWhite);
	pos.y += FontH();
	Blit(&gGraphicsDevice, pic, pos);
	// Display style index and count, right aligned
	sprintf(buf, "%d/%d", idx + 1, count);
	FontStrMask(
		buf, Vec2iNew(pos.x + 28 - FontStrW(buf), pos.y + 17), colorGray);
}
static void DisplayMapItemWithDensity(
	GraphicsDevice *g,
	const Vec2i pos, const MapObjectDensity *mod, const bool isHighlighted)
{
	UNUSED(g);
	DisplayMapItem(pos, mod->M);
	if (isHighlighted)
	{
		FontCh('>', Vec2iAdd(pos, Vec2iNew(-8, -4)));
	}
	char s[10];
	sprintf(s, "%d", mod->Density);
	FontStr(s, Vec2iAdd(pos, Vec2iNew(-8, 5)));
}
static void GetCharacterHeadPic(
	Character *c, TOffsetPic *pic, TranslationTable **t)
{
	const int i = c->looks.Face;
	*t = &c->table;
	pic->picIndex = cHeadPic[i][DIRECTION_DOWN][STATE_IDLE];
	pic->dx = cHeadOffset[i][DIRECTION_DOWN].dx;
	pic->dy = cHeadOffset[i][DIRECTION_DOWN].dy;
}

void DisplayFlag(
	GraphicsDevice *g, Vec2i pos, const char *s, int isOn, int isHighlighted)
{
	UNUSED(g);
	color_t labelMask = isHighlighted ? colorRed : colorWhite;
	pos = FontStrMask(s, pos, labelMask);
	pos = FontChMask(':', pos, labelMask);
	FontStrMask(isOn ? "On" : "Off", pos, isOn ? colorPurple : colorWhite);
}


static void CampaignChangeSeed(void *data, int d)
{
	if (gEventHandlers.keyboard.modState & KMOD_SHIFT)
	{
		d *= 10;
	}
	CampaignOptions *co = data;
	if (d < 0 && co->seed < (unsigned)-d)
	{
		co->seed = 0;
	}
	else
	{
		co->seed += d;
	}
}
static void MissionInsertNew(void *data, int d)
{
	UNUSED(d);
	CampaignOptions *co = data;
	InsertMission(co, NULL, co->MissionIndex);
}
static void MissionDelete(void *data, int d)
{
	UNUSED(d);
	CampaignOptions *co = data;
	// TODO: this still means the file is changed if user chooses no
	if (co->Setting.Missions.size > 0 &&
		ConfirmScreen("", "Delete mission? (Y/N)"))
	{
		DeleteMission(co);
	}
}
static void MissionChangeWidth(void *data, int d)
{
	CampaignOptions *co = data;
	Mission *m = CampaignGetCurrentMission(co);
	int old = m->Size.x;
	if (gEventHandlers.keyboard.modState & KMOD_SHIFT)
	{
		d *= 10;
	}
	m->Size.x = CLAMP(m->Size.x + d, 16, 256);
	if (m->Type == MAPTYPE_STATIC)
	{
		MissionStaticLayout(m, Vec2iNew(old, m->Size.y));
	}
}
static void MissionChangeHeight(void *data, int d)
{
	CampaignOptions *co = data;
	Mission *m = CampaignGetCurrentMission(co);
	int old = m->Size.y;
	if (gEventHandlers.keyboard.modState & KMOD_SHIFT)
	{
		d *= 10;
	}
	m->Size.y = CLAMP(m->Size.y + d, 16, 256);
	if (m->Type == MAPTYPE_STATIC)
	{
		MissionStaticLayout(m, Vec2iNew(m->Size.x, old));
	}
}
static void MissionChangeWallCount(void *data, int d)
{
	CampaignOptions *co = data;
	CampaignGetCurrentMission(co)->u.Classic.Walls =
		CLAMP(CampaignGetCurrentMission(co)->u.Classic.Walls + d, 0, 200);
}
static void MissionChangeWallLength(void *data, int d)
{
	CampaignOptions *co = data;
	CampaignGetCurrentMission(co)->u.Classic.WallLength =
		CLAMP(CampaignGetCurrentMission(co)->u.Classic.WallLength + d, 1, 100);
}
static void MissionChangeCorridorWidth(void *data, int d)
{
	CampaignOptions *co = data;
	CampaignGetCurrentMission(co)->u.Classic.CorridorWidth =
		CLAMP(CampaignGetCurrentMission(co)->u.Classic.CorridorWidth + d, 1, 5);
}
static void MissionChangeRoomCount(void *data, int d)
{
	CampaignOptions *co = data;
	CampaignGetCurrentMission(co)->u.Classic.Rooms.Count =
		CLAMP(CampaignGetCurrentMission(co)->u.Classic.Rooms.Count + d, 0, 100);
}
static void MissionChangeRoomMin(void *data, int d)
{
	CampaignOptions *co = data;
	CampaignGetCurrentMission(co)->u.Classic.Rooms.Min =
		CLAMP(CampaignGetCurrentMission(co)->u.Classic.Rooms.Min + d, 5, 50);
	CampaignGetCurrentMission(co)->u.Classic.Rooms.Max = MAX(
		CampaignGetCurrentMission(co)->u.Classic.Rooms.Min,
		CampaignGetCurrentMission(co)->u.Classic.Rooms.Max);
}
static void MissionChangeRoomMax(void *data, int d)
{
	CampaignOptions *co = data;
	CampaignGetCurrentMission(co)->u.Classic.Rooms.Max =
		CLAMP(CampaignGetCurrentMission(co)->u.Classic.Rooms.Max + d, 5, 50);
	CampaignGetCurrentMission(co)->u.Classic.Rooms.Min = MIN(
		CampaignGetCurrentMission(co)->u.Classic.Rooms.Min,
		CampaignGetCurrentMission(co)->u.Classic.Rooms.Max);
}
static void MissionChangeEdgeRooms(void *data, int d)
{
	UNUSED(d);
	CampaignOptions *co = data;
	CampaignGetCurrentMission(co)->u.Classic.Rooms.Edge =
		!CampaignGetCurrentMission(co)->u.Classic.Rooms.Edge;
}
static void MissionChangeRoomsOverlap(void *data, int d)
{
	UNUSED(d);
	CampaignOptions *co = data;
	CampaignGetCurrentMission(co)->u.Classic.Rooms.Overlap =
		!CampaignGetCurrentMission(co)->u.Classic.Rooms.Overlap;
}
static void MissionChangeRoomWallCount(void *data, int d)
{
	CampaignOptions *co = data;
	CampaignGetCurrentMission(co)->u.Classic.Rooms.Walls =
		CLAMP(CampaignGetCurrentMission(co)->u.Classic.Rooms.Walls + d, 0, 50);
}
static void MissionChangeRoomWallLen(void *data, int d)
{
	CampaignOptions *co = data;
	CampaignGetCurrentMission(co)->u.Classic.Rooms.WallLength =
		CLAMP(CampaignGetCurrentMission(co)->u.Classic.Rooms.WallLength + d, 1, 50);
}
static void MissionChangeRoomWallPad(void *data, int d)
{
	CampaignOptions *co = data;
	CampaignGetCurrentMission(co)->u.Classic.Rooms.WallPad =
		CLAMP(CampaignGetCurrentMission(co)->u.Classic.Rooms.WallPad + d, 1, 10);
}
static void MissionChangeSquareCount(void *data, int d)
{
	CampaignOptions *co = data;
	CampaignGetCurrentMission(co)->u.Classic.Squares =
		CLAMP(CampaignGetCurrentMission(co)->u.Classic.Squares + d, 0, 100);
}
static void MissionChangeDoorEnabled(void *data, int d)
{
	UNUSED(d);
	CampaignOptions *co = data;
	CampaignGetCurrentMission(co)->u.Classic.Doors.Enabled =
		!CampaignGetCurrentMission(co)->u.Classic.Doors.Enabled;
}
static void MissionChangeDoorMin(void *data, int d)
{
	CampaignOptions *co = data;
	CampaignGetCurrentMission(co)->u.Classic.Doors.Min =
		CLAMP(CampaignGetCurrentMission(co)->u.Classic.Doors.Min + d, 1, 6);
	CampaignGetCurrentMission(co)->u.Classic.Doors.Max = MAX(
		CampaignGetCurrentMission(co)->u.Classic.Doors.Min,
		CampaignGetCurrentMission(co)->u.Classic.Doors.Max);
}
static void MissionChangeDoorMax(void *data, int d)
{
	CampaignOptions *co = data;
	CampaignGetCurrentMission(co)->u.Classic.Doors.Max =
		CLAMP(CampaignGetCurrentMission(co)->u.Classic.Doors.Max + d, 1, 6);
	CampaignGetCurrentMission(co)->u.Classic.Doors.Min = MIN(
		CampaignGetCurrentMission(co)->u.Classic.Doors.Min,
		CampaignGetCurrentMission(co)->u.Classic.Doors.Max);
}
static void MissionChangePillarCount(void *data, int d)
{
	CampaignOptions *co = data;
	CampaignGetCurrentMission(co)->u.Classic.Pillars.Count =
		CLAMP(CampaignGetCurrentMission(co)->u.Classic.Pillars.Count + d, 0, 50);
}
static void MissionChangePillarMin(void *data, int d)
{
	CampaignOptions *co = data;
	CampaignGetCurrentMission(co)->u.Classic.Pillars.Min =
		CLAMP(CampaignGetCurrentMission(co)->u.Classic.Pillars.Min + d, 1, 50);
	CampaignGetCurrentMission(co)->u.Classic.Pillars.Max = MAX(
		CampaignGetCurrentMission(co)->u.Classic.Pillars.Min,
		CampaignGetCurrentMission(co)->u.Classic.Pillars.Max);
}
static void MissionChangePillarMax(void *data, int d)
{
	CampaignOptions *co = data;
	CampaignGetCurrentMission(co)->u.Classic.Pillars.Max =
		CLAMP(CampaignGetCurrentMission(co)->u.Classic.Pillars.Max + d, 1, 50);
	CampaignGetCurrentMission(co)->u.Classic.Pillars.Min = MIN(
		CampaignGetCurrentMission(co)->u.Classic.Pillars.Min,
		CampaignGetCurrentMission(co)->u.Classic.Pillars.Max);
}
static void MissionChangeDensity(void *data, int d)
{
	CampaignOptions *co = data;
	CampaignGetCurrentMission(co)->EnemyDensity = CLAMP(CampaignGetCurrentMission(co)->EnemyDensity + d, 0, 100);
}
static void MissionChangeType(void *data, int d)
{
	CampaignOptions *co = data;
	MapType type = CLAMP_OPPOSITE(
		(int)CampaignGetCurrentMission(co)->Type + d,
		MAPTYPE_CLASSIC,
		MAPTYPE_STATIC);
	Map map;
	MissionOptionsTerminate(&gMission);
	CampaignAndMissionSetup(1, co, &gMission);
	memset(&map, 0, sizeof map);
	MapLoad(&map, &gMission, co);
	MapLoadDynamic(&map, &gMission, &co->Setting.characters);
	MissionConvertToType(gMission.missionData, &map, type);
}
static void MissionChangeWallStyle(void *data, int d)
{
	CampaignOptions *co = data;
	CampaignGetCurrentMission(co)->WallStyle = CLAMP_OPPOSITE(
		CampaignGetCurrentMission(co)->WallStyle + d, 0, WALL_STYLE_COUNT - 1);
}
static void MissionChangeFloorStyle(void *data, int d)
{
	CampaignOptions *co = data;
	CampaignGetCurrentMission(co)->FloorStyle = CLAMP_OPPOSITE(
		CampaignGetCurrentMission(co)->FloorStyle + d, 0, FLOOR_STYLE_COUNT - 1);
}
static void MissionChangeRoomStyle(void *data, int d)
{
	CampaignOptions *co = data;
	CampaignGetCurrentMission(co)->RoomStyle = CLAMP_OPPOSITE(
		CampaignGetCurrentMission(co)->RoomStyle + d, 0, ROOM_STYLE_COUNT - 1);
}
static void MissionChangeDoorStyle(void *data, int d)
{
	CampaignOptions *co = data;
	const char *doorStyle = CampaignGetCurrentMission(co)->DoorStyle;
	const int newIdx = CLAMP_OPPOSITE(
		PicManagerGetDoorStyleIndex(&gPicManager, doorStyle) + d,
		0, (int)gPicManager.doorStyleNames.size - 1);
	const char **newDoorStyle =
		CArrayGet(&gPicManager.doorStyleNames, newIdx);
	strcpy(CampaignGetCurrentMission(co)->DoorStyle, *newDoorStyle);
}
static void MissionChangeKeyStyle(void *data, int d)
{
	CampaignOptions *co = data;
	CampaignGetCurrentMission(co)->KeyStyle = CLAMP_OPPOSITE(
		CampaignGetCurrentMission(co)->KeyStyle + d, 0, GetEditorInfo().keyCount - 1);
}
static void MissionChangeExitStyle(void *data, int d)
{
	CampaignOptions *co = data;
	CampaignGetCurrentMission(co)->ExitStyle = CLAMP_OPPOSITE(
		CampaignGetCurrentMission(co)->ExitStyle + d, 0, GetEditorInfo().exitCount - 1);
}
static void MissionChangeEnemy(void *vData, int d)
{
	MissionIndexData *data = vData;
	int enemy = *(int *)CArrayGet(
		&CampaignGetCurrentMission(data->co)->Enemies, data->index);
	enemy = CLAMP_OPPOSITE(
		enemy + d, 0, (int)data->co->Setting.characters.OtherChars.size - 1);
	*(int *)CArrayGet(
		&CampaignGetCurrentMission(data->co)->Enemies, data->index) = enemy;
	*(int *)CArrayGet(
		&data->co->Setting.characters.baddieIds, data->index) = enemy;
}
static void MissionChangeSpecialChar(void *vData, int d)
{
	MissionIndexData *data = vData;
	int c = *(int *)CArrayGet(
		&CampaignGetCurrentMission(data->co)->SpecialChars, data->index);
	c = CLAMP_OPPOSITE(
		c + d, 0, (int)data->co->Setting.characters.OtherChars.size - 1);
	*(int *)CArrayGet(
		&CampaignGetCurrentMission(data->co)->SpecialChars, data->index) = c;
	*(int *)CArrayGet(
		&data->co->Setting.characters.specialIds, data->index) = c;
}
static void MissionChangeWeapon(void *vData, int d)
{
	UNUSED(d);
	MissionGunData *data = vData;
	bool hasWeapon = false;
	int weaponIndex = -1;
	Mission *currentMission = CampaignGetCurrentMission(data->co);
	for (int i = 0; i < (int)currentMission->Weapons.size; i++)
	{
		const GunDescription **desc = CArrayGet(&currentMission->Weapons, i);
		if (data->Gun == *desc)
		{
			hasWeapon = true;
			weaponIndex = i;
			break;
		}
	}
	if (hasWeapon)
	{
		CArrayDelete(&currentMission->Weapons, weaponIndex);
	}
	else
	{
		CArrayPushBack(&currentMission->Weapons, &data->Gun);
	}
}
static void MissionChangeMapItem(void *vData, int d)
{
	MissionIndexData *data = vData;
	MapObjectDensity *mod = CArrayGet(
		&CampaignGetCurrentMission(data->co)->MapObjectDensities, data->index);
	if (gEventHandlers.keyboard.modState & KMOD_SHIFT)
	{
		mod->Density = CLAMP(mod->Density + 5 * d, 0, 512);
	}
	else
	{
		const int i = CLAMP_OPPOSITE(
			MapObjectIndex(mod->M) + d, 0, MapObjectsCount(&gMapObjects) - 1);
		mod->M = IndexMapObject(i);
	}
}
static void MissionChangeObjectiveIndex(void *vData, int d);
static void MissionChangeObjectiveType(void *vData, int d)
{
	MissionIndexData *data = vData;
	MissionObjective *mobj = GetMissionObjective(
		CampaignGetCurrentMission(data->co), data->index);
	mobj->Type = CLAMP_OPPOSITE((int)mobj->Type + d, 0, OBJECTIVE_INVESTIGATE);
	// Initialise the index of the objective
	MissionChangeObjectiveIndex(data, 0);
}
static void MissionChangeObjectiveIndex(void *vData, int d)
{
	MissionIndexData *data = vData;
	MissionObjective *mobj = GetMissionObjective(
		CampaignGetCurrentMission(data->co), data->index);
	int limit;
	switch (mobj->Type)
	{
	case OBJECTIVE_COLLECT:
		limit = PickupClassesGetScoreCount(&gPickupClasses) - 1;
		break;
	case OBJECTIVE_DESTROY:
		limit = (int)gMapObjects.Destructibles.size - 1;
		break;
	case OBJECTIVE_KILL:
	case OBJECTIVE_INVESTIGATE:
		limit = 0;
		break;
	case OBJECTIVE_RESCUE:
		limit = data->co->Setting.characters.OtherChars.size - 1;
		break;
	default:
		assert(0 && "Unknown objective type");
		return;
	}
	mobj->Index = CLAMP_OPPOSITE(mobj->Index + d, 0, limit);
}
static void MissionChangeObjectiveRequired(void *vData, int d)
{
	MissionIndexData *data = vData;
	MissionObjective *mobj = GetMissionObjective(
		CampaignGetCurrentMission(data->co), data->index);
	mobj->Required = CLAMP_OPPOSITE(
		mobj->Required + d, 0, MIN(100, mobj->Count));
}
static void MissionChangeObjectiveTotal(void *vData, int d)
{
	MissionIndexData *data = vData;
	const Mission *m = CampaignGetCurrentMission(data->co);
	MissionObjective *mobj = GetMissionObjective(m, data->index);
	mobj->Count = CLAMP_OPPOSITE(mobj->Count + d, mobj->Required, 100);
	// Don't let the total reduce to less than static ones we've placed
	if (m->Type == MAPTYPE_STATIC)
	{
		for (int i = 0; i < (int)m->u.Static.Objectives.size; i++)
		{
			const ObjectivePositions *op =
				CArrayGet(&m->u.Static.Objectives, i);
			if (op->Index == data->index)
			{
				mobj->Count = MAX(mobj->Count, (int)op->Positions.size);
				break;
			}
		}
	}
}
static void MissionChangeObjectiveFlags(void *vData, int d)
{
	MissionIndexData *data = vData;
	MissionObjective *mobj = GetMissionObjective(
		CampaignGetCurrentMission(data->co), data->index);
	// Max is combination of all flags, i.e. largest flag doubled less one
	mobj->Flags = CLAMP_OPPOSITE(
		mobj->Flags + d, 0, OBJECTIVE_NOACCESS * 2 - 1);
}


static UIObject *CreateCampaignObjs(CampaignOptions *co);
static UIObject *CreateMissionObjs(CampaignOptions *co);
static UIObject *CreateClassicMapObjs(Vec2i pos, CampaignOptions *co);
static UIObject *CreateWeaponObjs(CampaignOptions *co);
static UIObject *CreateMapItemObjs(CampaignOptions *co, int dy);
static UIObject *CreateCharacterObjs(CampaignOptions *co, int dy);
static UIObject *CreateSpecialCharacterObjs(CampaignOptions *co, int dy);
static UIObject *CreateObjectiveObjs(
	Vec2i pos, CampaignOptions *co, int idx);

typedef struct
{
	bool IsCollapsed;
	Vec2i Size;
	Pic *collapsePic;
	Pic *expandPic;
	UIObject *collapseButton;
	UIObject *child;
	UIObject *background;
} CollapsedData;
static void ToggleCollapse(void *data, int d);
static void DrawBackground(
	UIObject *o, GraphicsDevice *g, Vec2i pos, void *data);
static UIObject *CreateEditorObjs(CampaignOptions *co, EditorBrush *brush);
UIObject *CreateMainObjs(CampaignOptions *co, EditorBrush *brush, Vec2i size)
{
	UIObject *cc = UIObjectCreate(UITYPE_NONE, 0, Vec2iZero(), Vec2iZero());
	CollapsedData *cData;
	CMALLOC(cData, sizeof(CollapsedData));
	cData->IsCollapsed = false;
	cData->Size = size;
	cc->Data = cData;
	cc->IsDynamicData = true;

	// Collapse button
	cData->collapsePic = PicManagerGetPic(&gPicManager, "editor/collapse");
	cData->expandPic = PicManagerGetPic(&gPicManager, "editor/expand");
	UIObject *o = UIObjectCreate(
		UITYPE_BUTTON, 0,
		Vec2iMinus(size, cData->collapsePic->size), Vec2iZero());
	UIButtonSetPic(o, cData->collapsePic);
	o->DoNotHighlight = true;
	o->ChangeFunc = ToggleCollapse;
	o->Data = cData;
	CSTRDUP(o->Tooltip, "Collapse/expand");
	UIObjectAddChild(cc, o);
	cData->collapseButton = o;

	// The rest of the UI
	o = CreateEditorObjs(co, brush);
	UIObjectAddChild(cc, o);
	cData->child = o;

	// Background
	o = UIObjectCreate(UITYPE_CUSTOM, 0, Vec2iZero(), size);
	o->DoNotHighlight = true;
	o->u.CustomDrawFunc = DrawBackground;
	o->Data = cData;
	UIObjectAddChild(cc, o);
	cData->background = o;

	return cc;
}
static void ToggleCollapse(void *data, int d)
{
	UNUSED(d);
	CollapsedData *cData = data;
	cData->IsCollapsed = !cData->IsCollapsed;
	if (cData->IsCollapsed)
	{
		// Change button pic, move button
		cData->collapseButton->u.Button.Pic = cData->expandPic;
		cData->collapseButton->Pos = Vec2iZero();
	}
	else
	{
		// Change button pic, move button
		cData->collapseButton->u.Button.Pic = cData->collapsePic;
		cData->collapseButton->Pos = Vec2iMinus(
			cData->Size, cData->collapsePic->size);
	}
	cData->child->IsVisible = !cData->IsCollapsed;
	cData->background->IsVisible = !cData->IsCollapsed;
}
static void DrawBackground(
	UIObject *o, GraphicsDevice *g, Vec2i pos, void *data)
{
	UNUSED(data);
	Vec2i v;
	for (v.y = 0; v.y < o->Size.y; v.y++)
	{
		for (v.x = 0; v.x < o->Size.x; v.x++)
		{
			DrawPointTint(g, Vec2iAdd(v, pos), tintDarker);
		}
	}
}
static UIObject *CreateEditorObjs(CampaignOptions *co, EditorBrush *brush)
{
	const int th = FontH();
	UIObject *c;
	UIObject *o;
	UIObject *o2;
	UIObject *oc;
	int i;
	Vec2i pos;
	Vec2i objectivesPos;
	UIObject *cc = UIObjectCreate(UITYPE_NONE, 0, Vec2iZero(), Vec2iZero());

	// Titles

	pos.y = 5;

	o = UIObjectCreate(
		UITYPE_TEXTBOX, YC_CAMPAIGNTITLE,
		Vec2iNew(25, pos.y), Vec2iNew(140, th));
	o->u.Textbox.TextLinkFunc = CampaignGetTitle;
	o->u.Textbox.TextSourceFunc = CampaignGetTitleSrc;
	o->Data = co;
	CSTRDUP(o->u.Textbox.Hint, "(Campaign title)");
	o->Flags = UI_SELECT_ONLY_FIRST;
	UIObjectAddChild(o, CreateCampaignObjs(co));
	UIObjectAddChild(cc, o);

	// Mission insert/delete/index
	// Layout from right to left
	o = UIObjectCreate(
		UITYPE_NONE, YC_MISSIONINDEX, Vec2iNew(270, pos.y), Vec2iNew(49, th));
	UIObjectAddChild(cc, o);
	o = UIObjectCreate(
		UITYPE_LABEL, 0, Vec2iNew(o->Pos.x, pos.y), Vec2iNew(0, th));
	o->Label = "Insert mission";
	o->Size.x = FontStrW(o->Label);
	o->Pos.x -= o->Size.x + 10;
	o->ChangeFunc = MissionInsertNew;
	o->Data = &gCampaign;
	o->ChangesData = true;
	UIObjectAddChild(cc, o);
	o = UIObjectCreate(
		UITYPE_LABEL, 0, Vec2iNew(o->Pos.x, pos.y), Vec2iNew(0, th));
	o->Label = "Delete mission";
	o->Size.x = FontStrW(o->Label);
	o->Pos.x -= o->Size.x + 10;
	o->ChangeFunc = MissionDelete;
	o->Data = &gCampaign;
	o->ChangesData = true;
	UIObjectAddChild(cc, o);

	pos.y = 2 * th;

	// Mission-only controls
	// Only visible if the current mission is valid
	c = UIObjectCreate(UITYPE_NONE, 0, Vec2iZero(), Vec2iZero());
	c->CheckVisible = CheckMission;
	c->Data = co;
	UIObjectAddChild(cc, c);

	o = UIObjectCreate(
		UITYPE_TEXTBOX, YC_MISSIONTITLE,
		Vec2iNew(25, pos.y), Vec2iNew(175, th));
	o->Id2 = XC_MISSIONTITLE;
	o->u.Textbox.TextLinkFunc = MissionGetTitle;
	o->u.Textbox.TextSourceFunc = MissionGetTitleSrc;
	o->Data = co;
	CSTRDUP(o->u.Textbox.Hint, "(Mission title)");
	o->CheckVisible = MissionCheckVisible;
	UIObjectAddChild(o, CreateMissionObjs(co));
	UIObjectAddChild(c, o);

	// mission properties
	// size, walls, rooms etc.

	pos.y = 10 + 2 * th;

	o = UIObjectCreate(
		UITYPE_LABEL, 0, Vec2iZero(), Vec2iNew(40, th));
	o->ChangesData = 1;

	pos.x = 20;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = MissionGetWidthStr;
	o2->Data = co;
	o2->ChangeFunc = MissionChangeWidth;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.x += 40;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = MissionGetHeightStr;
	o2->Data = co;
	o2->ChangeFunc = MissionChangeHeight;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.x += 40;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = MissionGetDensityStr;
	o2->Data = co;
	o2->ChangeFunc = MissionChangeDensity;
	o2->Pos = pos;
	CSTRDUP(o2->Tooltip, "Number of non-objective characters");
	UIObjectAddChild(c, o2);

	pos.x += 40;
	o2 = UIObjectCopy(o);
	o2->Size.x = 50;
	o2->u.LabelFunc = MissionGetTypeStr;
	o2->Data = co;
	o2->ChangeFunc = MissionChangeType;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.x = 20;
	pos.y += th;
	UIObjectAddChild(c, CreateClassicMapObjs(pos, co));
	UIObjectAddChild(c, CreateStaticMapObjs(pos, co, brush));

	// Mission looks
	// wall/floor styles etc.

	pos.y += th * 6 + 2;

	UIObjectDestroy(o);
	o = UIObjectCreate(
		UITYPE_CUSTOM, YC_MISSIONLOOKS, Vec2iZero(), Vec2iNew(25, 25 + th));
	o->ChangesData = 1;

	pos.x = 20;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_WALL;
	o2->u.CustomDrawFunc = MissionDrawWallStyle;
	o2->Data = co;
	o2->ChangeFunc = MissionChangeWallStyle;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.x += 30;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_FLOOR;
	o2->u.CustomDrawFunc = MissionDrawFloorStyle;
	o2->Data = co;
	o2->ChangeFunc = MissionChangeFloorStyle;
	o2->Pos = pos;
	o2->CheckVisible = MissionCheckVisible;
	UIObjectAddChild(c, o2);
	pos.x += 30;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_ROOM;
	o2->u.CustomDrawFunc = MissionDrawRoomStyle;
	o2->Data = co;
	o2->ChangeFunc = MissionChangeRoomStyle;
	o2->Pos = pos;
	o2->CheckVisible = MissionCheckVisible;
	UIObjectAddChild(c, o2);
	pos.x += 30;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_DOORS;
	o2->u.CustomDrawFunc = MissionDrawDoorStyle;
	o2->Data = co;
	o2->ChangeFunc = MissionChangeDoorStyle;
	o2->Pos = pos;
	o2->CheckVisible = MissionCheckVisible;
	UIObjectAddChild(c, o2);
	pos.x += 30;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_KEYS;
	o2->u.CustomDrawFunc = MissionDrawKeyStyle;
	o2->Data = co;
	o2->ChangeFunc = MissionChangeKeyStyle;
	o2->Pos = pos;
	o2->CheckVisible = MissionCheckVisible;
	UIObjectAddChild(c, o2);
	pos.x += 30;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_EXIT;
	o2->u.CustomDrawFunc = MissionDrawExitStyle;
	o2->Data = co;
	o2->ChangeFunc = MissionChangeExitStyle;
	o2->Pos = pos;
	o2->CheckVisible = MissionCheckVisible;
	UIObjectAddChild(c, o2);

	// colours

	pos.x = 200;
	pos = CreateColorObjs(co, c, pos);

	// mission data

	pos.x = 20;
	pos.y += th + 5;

	UIObjectDestroy(o);
	o = UIObjectCreate(UITYPE_LABEL, 0, Vec2iZero(), Vec2iNew(189, th));
	o->Flags = UI_SELECT_ONLY;

	o2 = UIObjectCopy(o);
	o2->Label = "Mission description";
	o2->Id = YC_MISSIONDESC;
	o2->Pos = pos;
	o2->Size = FontStrSize(o2->Label);
	oc = UIObjectCreate(
		UITYPE_TEXTBOX, YC_MISSIONDESC,
		Vec2iNew(0, Y_ABS - pos.y), Vec2iNew(295, 5 * th));
	oc->Flags = UI_ENABLED_WHEN_PARENT_HIGHLIGHTED_ONLY;
	oc->u.Textbox.TextLinkFunc = MissionGetDescription;
	oc->u.Textbox.TextSourceFunc = MissionGetDescriptionSrc;
	oc->Data = co;
	CSTRDUP(oc->u.Textbox.Hint, "(Mission description)");
	oc->CheckVisible = MissionCheckVisible;
	UIObjectAddChild(o2, oc);
	UIObjectAddChild(c, o2);

	pos.y += th;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = MissionGetCharacterCountStr;
	o2->Data = co;
	o2->Id = YC_CHARACTERS;
	o2->Pos = pos;
	CSTRDUP(
		o2->Tooltip, "Use Insert/CTRL+i, Delete/CTRL+d and PageUp/PageDown");
	UIObjectAddChild(o2, CreateCharacterObjs(co, pos.y));
	UIObjectAddChild(c, o2);
	pos.y += th;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = MissionGetSpecialCountStr;
	o2->Data = co;
	o2->Id = YC_SPECIALS;
	o2->Pos = pos;
	CSTRDUP(
		o2->Tooltip, "Use Insert/CTRL+i, Delete/CTRL+d and PageUp/PageDown");
	UIObjectAddChild(o2, CreateSpecialCharacterObjs(co, pos.y));
	UIObjectAddChild(c, o2);
	pos.y += th;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = GetWeaponCountStr;
	o2->Data = NULL;
	o2->Pos = pos;
	UIObjectAddChild(o2, CreateWeaponObjs(co));
	UIObjectAddChild(c, o2);
	pos.y += th;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = GetObjectCountStr;
	o2->Data = NULL;
	o2->Id = YC_ITEMS;
	o2->Pos = pos;
	CSTRDUP(o2->Tooltip,
		"Use Insert/CTRL+i, Delete/CTRL+d and PageUp/PageDown\n"
		"Shift+click to change amounts");
	UIObjectAddChild(o2, CreateMapItemObjs(co, pos.y));
	UIObjectAddChild(c, o2);

	// objectives
	pos.y += 2;
	objectivesPos = Vec2iNew(0, 7 * th);
	UIObjectDestroy(o);
	o = UIObjectCreate(UITYPE_TEXTBOX, 0, Vec2iZero(), Vec2iNew(300, th));
	o->Flags = UI_SELECT_ONLY;

	for (i = 0; i < OBJECTIVE_MAX_OLD; i++)
	{
		pos.y += th;
		o2 = UIObjectCopy(o);
		o2->Id = YC_OBJECTIVES + i;
		o2->Type = UITYPE_TEXTBOX;
		o2->u.Textbox.TextLinkFunc = MissionGetObjectiveDescription;
		o2->u.Textbox.TextSourceFunc = MissionGetObjectiveDescriptionSrc;
		o2->IsDynamicData = 1;
		CMALLOC(o2->Data, sizeof(MissionObjectiveData));
		((MissionObjectiveData *)o2->Data)->Campaign = co;
		((MissionObjectiveData *)o2->Data)->MissionObjectiveIndex = i;
		CSTRDUP(o2->u.Textbox.Hint, "(Objective description)");
		o2->Pos = pos;
		CSTRDUP(o2->Tooltip, "insert/delete: add/remove objective");
		o2->CheckVisible = MissionCheckObjectiveDescription;
		UIObjectAddChild(o2, CreateObjectiveObjs(objectivesPos, co, i));
		UIObjectAddChild(c, o2);
	}

	UIObjectDestroy(o);
	return cc;
}
static UIObject *CreateCampaignObjs(CampaignOptions *co)
{
	const int th = FontH();
	UIObject *c;
	UIObject *o;
	UIObject *o2;
	int x;
	int y;
	c = UIObjectCreate(UITYPE_NONE, 0, Vec2iZero(), Vec2iZero());
	c->Flags = UI_ENABLED_WHEN_PARENT_HIGHLIGHTED_ONLY;

	x = 0;
	y = Y_ABS;

	o = UIObjectCreate(
		UITYPE_TEXTBOX, YC_CAMPAIGNTITLE, Vec2iZero(), Vec2iZero());
	o->Flags = UI_SELECT_ONLY;

	o2 = UIObjectCopy(o);
	o2->u.Textbox.TextLinkFunc = CampaignGetAuthor;
	o2->u.Textbox.TextSourceFunc = CampaignGetAuthorSrc;
	o2->Data = co;
	CSTRDUP(o2->u.Textbox.Hint, "(Campaign author)");
	o2->Id2 = XC_AUTHOR;
	o2->Pos = Vec2iNew(x, y);
	o2->Size = Vec2iNew(295, th);
	UIObjectAddChild(c, o2);

	y += th;
	o2 = UIObjectCopy(o);
	o2->u.Textbox.TextLinkFunc = CampaignGetDescription;
	o2->u.Textbox.TextSourceFunc = CampaignGetDescriptionSrc;
	o2->Data = co;
	CSTRDUP(o2->u.Textbox.Hint, "(Campaign description)");
	o2->Id2 = XC_CAMPAIGNDESC;
	o2->Pos = Vec2iNew(x, y);
	o2->Size = Vec2iNew(295, 5 * th);
	UIObjectAddChild(c, o2);

	UIObjectDestroy(o);
	return c;
}
static UIObject *CreateMissionObjs(CampaignOptions *co)
{
	const int th = FontH();
	UIObject *c;
	UIObject *o;
	c = UIObjectCreate(UITYPE_NONE, 0, Vec2iZero(), Vec2iZero());
	c->Flags = UI_ENABLED_WHEN_PARENT_HIGHLIGHTED_ONLY;

	o = UIObjectCreate(
		UITYPE_TEXTBOX, YC_MISSIONTITLE, Vec2iNew(0, Y_ABS), Vec2iNew(319, th));
	o->u.Textbox.TextLinkFunc = MissionGetSong;
	o->Data = co;
	CSTRDUP(o->u.Textbox.Hint, "(Mission song)");
	o->Id2 = XC_MUSICFILE;
	o->Flags = UI_SELECT_ONLY;
	o->CheckVisible = MissionCheckVisible;
	UIObjectAddChild(c, o);

	return c;
}
static UIObject *CreateClassicMapObjs(Vec2i pos, CampaignOptions *co)
{
	const int th = FontH();
	UIObject *c = UIObjectCreate(UITYPE_NONE, 0, Vec2iZero(), Vec2iZero());
	UIObject *o = UIObjectCreate(
		UITYPE_LABEL, 0, Vec2iZero(), Vec2iNew(50, th));
	int x = pos.x;
	UIObject *o2;
	o->ChangesData = 1;
	// Check whether the map type matches, and set visibility
	c->CheckVisible = MissionCheckTypeClassic;
	c->Data = co;

	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = CampaignGetSeedStr;
	o2->Data = co;
	o2->ChangeFunc = CampaignChangeSeed;
	CSTRDUP(o2->Tooltip, "Preview with different random seed");
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.x += o2->Size.x;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = MissionGetWallCountStr;
	o2->Data = co;
	o2->ChangeFunc = MissionChangeWallCount;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.x += o2->Size.x;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = MissionGetWallLengthStr;
	o2->Data = co;
	o2->ChangeFunc = MissionChangeWallLength;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.x += o2->Size.x;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = MissionGetCorridorWidthStr;
	o2->Data = co;
	o2->ChangeFunc = MissionChangeCorridorWidth;
	o2->Pos = pos;
	o2->Size.x = 60;
	UIObjectAddChild(c, o2);
	
	pos.x = x;
	pos.y += th;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = MissionGetRoomCountStr;
	o2->Data = co;
	o2->ChangeFunc = MissionChangeRoomCount;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.x += o2->Size.x;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = MissionGetRoomMinStr;
	o2->Data = co;
	o2->ChangeFunc = MissionChangeRoomMin;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.x += o2->Size.x;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = MissionGetRoomMaxStr;
	o2->Data = co;
	o2->ChangeFunc = MissionChangeRoomMax;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.x += o2->Size.x;
	o2 = UIObjectCreate(UITYPE_CUSTOM, 0, pos, Vec2iNew(60, th));
	o2->u.CustomDrawFunc = MissionDrawEdgeRooms;
	o2->Data = co;
	o2->ChangeFunc = MissionChangeEdgeRooms;
	UIObjectAddChild(c, o2);
	pos.x += o2->Size.x;
	o2 = UIObjectCreate(UITYPE_CUSTOM, 0, pos, Vec2iNew(60, th));
	o2->u.CustomDrawFunc = MissionDrawRoomsOverlap;
	o2->Data = co;
	o2->ChangeFunc = MissionChangeRoomsOverlap;
	UIObjectAddChild(c, o2);

	pos.x = x;
	pos.y += th;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = MissionGetRoomWallCountStr;
	o2->Data = co;
	o2->ChangeFunc = MissionChangeRoomWallCount;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.x += o2->Size.x;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = MissionGetRoomWallLenStr;
	o2->Data = co;
	o2->ChangeFunc = MissionChangeRoomWallLen;
	o2->Pos = pos;
	o2->Size.x = 60;
	UIObjectAddChild(c, o2);
	pos.x += o2->Size.x;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = MissionGetRoomWallPadStr;
	o2->Data = co;
	o2->ChangeFunc = MissionChangeRoomWallPad;
	o2->Pos = pos;
	o2->Size.x = 60;
	UIObjectAddChild(c, o2);

	pos.x = x;
	pos.y += th;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = MissionGetSquareCountStr;
	o2->Data = co;
	o2->ChangeFunc = MissionChangeSquareCount;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);

	pos.x = x;
	pos.y += th;
	o2 = UIObjectCreate(UITYPE_CUSTOM, 0, pos, o->Size);
	o2->u.CustomDrawFunc = MissionDrawDoorEnabled;
	o2->Data = co;
	o2->ChangeFunc = MissionChangeDoorEnabled;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.x += o2->Size.x;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = MissionGetDoorMinStr;
	o2->Data = co;
	o2->ChangeFunc = MissionChangeDoorMin;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.x += o2->Size.x;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = MissionGetDoorMaxStr;
	o2->Data = co;
	o2->ChangeFunc = MissionChangeDoorMax;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);

	pos.x = x;
	pos.y += th;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = MissionGetPillarCountStr;
	o2->Data = co;
	o2->ChangeFunc = MissionChangePillarCount;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.x += o2->Size.x;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = MissionGetPillarMinStr;
	o2->Data = co;
	o2->ChangeFunc = MissionChangePillarMin;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.x += o2->Size.x;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = MissionGetPillarMaxStr;
	o2->Data = co;
	o2->ChangeFunc = MissionChangePillarMax;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);

	UIObjectDestroy(o);
	return c;
}
static void CreateWeaponToggleObjs(
	CampaignOptions *co, UIObject *c, const UIObject *o,
	int *idx, const int rows, CArray *guns);
static UIObject *CreateWeaponObjs(CampaignOptions *co)
{
	const int th = FontH();
	UIObject *c = UIObjectCreate(
		UITYPE_CONTEXT_MENU, 0, Vec2iZero(), Vec2iZero());
	c->Flags = UI_ENABLED_WHEN_PARENT_HIGHLIGHTED_ONLY;

	UIObject *o = UIObjectCreate(
		UITYPE_CUSTOM, 0, Vec2iZero(), Vec2iNew(80, th));
	o->u.CustomDrawFunc = MissionDrawWeaponStatus;
	o->ChangeFunc = MissionChangeWeapon;
	o->Flags = UI_LEAVE_YC;
	o->ChangesData = true;
	const int rows = 10;
	int idx = 0;
	CreateWeaponToggleObjs(co, c, o, &idx, rows, &gGunDescriptions.Guns);
	CreateWeaponToggleObjs(co, c, o, &idx, rows, &gGunDescriptions.CustomGuns);

	UIObjectDestroy(o);
	return c;
}
static void CreateWeaponToggleObjs(
	CampaignOptions *co, UIObject *c, const UIObject *o,
	int *idx, const int rows, CArray *guns)
{
	const int th = FontH();
	for (int i = 0; i < (int)guns->size; i++)
	{
		const GunDescription *g = CArrayGet(guns, i);
		if (!g->IsRealGun)
		{
			continue;
		}
		UIObject *o2 = UIObjectCopy(o);
		CMALLOC(o2->Data, sizeof(MissionGunData));
		o2->IsDynamicData = true;
		((MissionGunData *)o2->Data)->co = co;
		((MissionGunData *)o2->Data)->Gun = g;
		o2->Pos = Vec2iNew(*idx / rows * 90, (*idx % rows) * th);
		UIObjectAddChild(c, o2);
		(*idx)++;
	}
}
static UIObject *CreateMapItemObjs(CampaignOptions *co, int dy)
{
	UIObject *c;
	UIObject *o;
	UIObject *o2;
	int i;
	c = UIObjectCreate(UITYPE_NONE, 0, Vec2iZero(), Vec2iZero());
	c->Flags = UI_ENABLED_WHEN_PARENT_HIGHLIGHTED_ONLY;

	o = UIObjectCreate(UITYPE_CUSTOM, 0, Vec2iZero(), Vec2iNew(20, 40));
	o->u.CustomDrawFunc = MissionDrawMapItem;
	o->ChangeFunc = MissionChangeMapItem;
	o->Flags = UI_LEAVE_YC;
	o->ChangesData = 1;
	for (i = 0; i < 32; i++)	// TODO: no limit to objects
	{
		int x = 10 + i * 20;
		o2 = UIObjectCopy(o);
		o2->Id2 = i;
		CMALLOC(o2->Data, sizeof(MissionIndexData));
		o2->IsDynamicData = 1;
		((MissionIndexData *)o2->Data)->co = co;
		((MissionIndexData *)o2->Data)->index = i;
		o2->Pos = Vec2iNew(x, Y_ABS - dy);
		UIObjectAddChild(c, o2);
	}

	UIObjectDestroy(o);
	return c;
}
static UIObject *CreateObjectiveObjs(Vec2i pos, CampaignOptions *co, int idx)
{
	const int th = FontH();
	UIObject *c;
	UIObject *o;
	UIObject *o2;
	c = UIObjectCreate(UITYPE_NONE, 0, Vec2iZero(), Vec2iZero());
	c->Flags = UI_ENABLED_WHEN_PARENT_HIGHLIGHTED_ONLY;

	o = UIObjectCreate(UITYPE_NONE, 0, Vec2iZero(), Vec2iZero());
	o->Flags = UI_LEAVE_YC;
	o->ChangesData = 1;

	pos.y -= idx * th;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_TYPE;
	o2->Type = UITYPE_LABEL;
	o2->u.LabelFunc = MissionGetObjectiveStr;
	o2->ChangeFunc = MissionChangeObjectiveType;
	CMALLOC(o2->Data, sizeof(MissionIndexData));
	o2->IsDynamicData = 1;
	((MissionIndexData *)o2->Data)->co = co;
	((MissionIndexData *)o2->Data)->index = idx;
	o2->Pos = pos;
	o2->Size = Vec2iNew(35, th);
	UIObjectAddChild(c, o2);
	pos.x += 40;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_INDEX;
	o2->Type = UITYPE_CUSTOM;
	o2->u.CustomDrawFunc = MissionDrawObjective;
	o2->ChangeFunc = MissionChangeObjectiveIndex;
	CMALLOC(o2->Data, sizeof(MissionIndexData));
	o2->IsDynamicData = 1;
	((MissionIndexData *)o2->Data)->co = co;
	((MissionIndexData *)o2->Data)->index = idx;
	o2->Pos = pos;
	o2->Size = Vec2iNew(30, th);
	UIObjectAddChild(c, o2);
	pos.x += 30;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_REQUIRED;
	o2->Type = UITYPE_LABEL;
	o2->u.LabelFunc = MissionGetObjectiveRequired;
	o2->ChangeFunc = MissionChangeObjectiveRequired;
	CMALLOC(o2->Data, sizeof(MissionIndexData));
	o2->IsDynamicData = 1;
	((MissionIndexData *)o2->Data)->co = co;
	((MissionIndexData *)o2->Data)->index = idx;
	o2->Pos = pos;
	o2->Size = Vec2iNew(20, th);
	CSTRDUP(o2->Tooltip, "0: optional objective");
	UIObjectAddChild(c, o2);
	pos.x += 20;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_TOTAL;
	o2->Type = UITYPE_LABEL;
	o2->u.LabelFunc = MissionGetObjectiveTotal;
	o2->ChangeFunc = MissionChangeObjectiveTotal;
	CMALLOC(o2->Data, sizeof(MissionIndexData));
	o2->IsDynamicData = 1;
	((MissionIndexData *)o2->Data)->co = co;
	((MissionIndexData *)o2->Data)->index = idx;
	o2->Pos = pos;
	o2->Size = Vec2iNew(40, th);
	UIObjectAddChild(c, o2);
	pos.x += 45;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_FLAGS;
	o2->Type = UITYPE_LABEL;
	o2->u.LabelFunc = MissionGetObjectiveFlags;
	o2->ChangeFunc = MissionChangeObjectiveFlags;
	CMALLOC(o2->Data, sizeof(MissionIndexData));
	o2->IsDynamicData = 1;
	((MissionIndexData *)o2->Data)->co = co;
	((MissionIndexData *)o2->Data)->index = idx;
	o2->Pos = pos;
	o2->Size = Vec2iNew(100, th);
	CSTRDUP(o2->Tooltip,
		"hidden: not shown on map\n"
		"pos.known: always shown on map\n"
		"access: in locked room\n"
		"no-count: don't show completed count\n"
		"no-access: not in locked rooms");
	UIObjectAddChild(c, o2);

	UIObjectDestroy(o);
	return c;
}
static UIObject *CreateCharacterObjs(CampaignOptions *co, int dy)
{
	UIObject *c;
	UIObject *o;
	UIObject *o2;
	int i;
	c = UIObjectCreate(UITYPE_NONE, 0, Vec2iZero(), Vec2iZero());
	c->Flags = UI_ENABLED_WHEN_PARENT_HIGHLIGHTED_ONLY;

	o = UIObjectCreate(UITYPE_CUSTOM, 0, Vec2iZero(), Vec2iNew(20, 40));
	o->u.CustomDrawFunc = MissionDrawEnemy;
	o->ChangeFunc = MissionChangeEnemy;
	o->Flags = UI_LEAVE_YC | UI_SELECT_ONLY_FIRST;
	o->ChangesData = 1;
	for (i = 0; i < 15; i++)
	{
		int x = 10 + i * 20;
		o2 = UIObjectCopy(o);
		o2->Id2 = i;
		o2->IsDynamicData = 1;
		CMALLOC(o2->Data, sizeof(MissionIndexData));
		((MissionIndexData *)o2->Data)->co = co;
		((MissionIndexData *)o2->Data)->index = i;
		o2->Pos = Vec2iNew(x, Y_ABS - dy);
		UIObjectAddChild(c, o2);
	}

	UIObjectDestroy(o);
	return c;
}
static UIObject *CreateSpecialCharacterObjs(CampaignOptions *co, int dy)
{
	UIObject *c;
	UIObject *o;
	UIObject *o2;
	int i;
	c = UIObjectCreate(UITYPE_NONE, 0, Vec2iZero(), Vec2iZero());
	c->Flags = UI_ENABLED_WHEN_PARENT_HIGHLIGHTED_ONLY;

	o = UIObjectCreate(UITYPE_CUSTOM, 0, Vec2iZero(), Vec2iNew(20, 40));
	o->u.CustomDrawFunc = MissionDrawSpecialChar;
	o->ChangeFunc = MissionChangeSpecialChar;
	o->Flags = UI_LEAVE_YC | UI_SELECT_ONLY_FIRST;
	o->ChangesData = 1;
	for (i = 0; i < 15; i++)
	{
		int x = 10 + i * 20;
		o2 = UIObjectCopy(o);
		o2->Id2 = i;
		o2->IsDynamicData = 1;
		CMALLOC(o2->Data, sizeof(MissionIndexData));
		((MissionIndexData *)o2->Data)->co = co;
		((MissionIndexData *)o2->Data)->index = i;
		o2->Pos = Vec2iNew(x, Y_ABS - dy);
		UIObjectAddChild(c, o2);
	}

	UIObjectDestroy(o);
	return c;
}

UIObject *CreateCharEditorObjs(void)
{
	const int th = FontH();
	UIObject *c;
	UIObject *o;
	UIObject *o2;
	int x;
	int y;
	c = UIObjectCreate(UITYPE_NONE, 0, Vec2iZero(), Vec2iZero());

	// Appearance

	y = 10;
	o = UIObjectCreate(
		UITYPE_NONE, YC_APPEARANCE, Vec2iZero(), Vec2iNew(40, th));

	x = 30;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_FACE;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);
	x += 30;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_SKIN;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);
	x += 30;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_HAIR;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);
	x += 30;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_BODY;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);
	x += 30;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_ARMS;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);
	x += 30;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_LEGS;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);

	// Character attributes

	y += th;
	UIObjectDestroy(o);
	o = UIObjectCreate(
		UITYPE_NONE, YC_ATTRIBUTES, Vec2iZero(), Vec2iNew(40, th));

	x = 20;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_SPEED;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);
	x += 50;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_HEALTH;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);
	x += 50;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_MOVE;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);
	x += 50;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_TRACK;
	o2->Pos = Vec2iNew(x, y);
	CSTRDUP(o2->Tooltip,
		"Looking towards the player\n"
		"Useless for friendly characters");
	UIObjectAddChild(c, o2);
	x += 50;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_SHOOT;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);
	x += 50;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_DELAY;
	o2->Pos = Vec2iNew(x, y);
	CSTRDUP(o2->Tooltip, "Frames before making another decision");
	UIObjectAddChild(c, o2);

	// Character flags

	y += th;
	UIObjectDestroy(o);
	o = UIObjectCreate(UITYPE_NONE, YC_FLAGS, Vec2iZero(), Vec2iNew(40, th));

	x = 5;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_ASBESTOS;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);
	x += 45;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_IMMUNITY;
	o2->Pos = Vec2iNew(x, y);
	CSTRDUP(o2->Tooltip, "Immune to poison");
	UIObjectAddChild(c, o2);
	x += 45;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_SEETHROUGH;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);
	x += 45;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_RUNS_AWAY;
	o2->Pos = Vec2iNew(x, y);
	CSTRDUP(o2->Tooltip, "Runs away from player");
	UIObjectAddChild(c, o2);
	x += 45;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_SNEAKY;
	o2->Pos = Vec2iNew(x, y);
	CSTRDUP(o2->Tooltip, "Shoots back when player shoots");
	UIObjectAddChild(c, o2);
	x += 45;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_GOOD_GUY;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);
	x += 45;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_SLEEPING;
	o2->Pos = Vec2iNew(x, y);
	CSTRDUP(o2->Tooltip, "Doesn't move unless seen");
	UIObjectAddChild(c, o2);

	y += th;
	x = 5;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_PRISONER;
	o2->Pos = Vec2iNew(x, y);
	CSTRDUP(o2->Tooltip, "Doesn't move until touched");
	UIObjectAddChild(c, o2);
	x += 45;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_INVULNERABLE;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);
	x += 45;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_FOLLOWER;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);
	x += 45;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_PENALTY;
	o2->Pos = Vec2iNew(x, y);
	CSTRDUP(o2->Tooltip, "Large score penalty when shot");
	UIObjectAddChild(c, o2);
	x += 45;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_VICTIM;
	o2->Pos = Vec2iNew(x, y);
	CSTRDUP(o2->Tooltip, "Takes damage from everyone");
	UIObjectAddChild(c, o2);
	x += 45;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_AWAKE;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);

	// Weapon

	y += th;
	UIObjectDestroy(o);
	o = UIObjectCreate(
		UITYPE_NONE, YC_WEAPON, Vec2iNew(50, y), Vec2iNew(210, th));
	UIObjectAddChild(c, o);

	return c;
}
