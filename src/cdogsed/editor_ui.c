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
#include "editor_ui.h"

#include <assert.h>

#include <cdogs/door.h>
#include <cdogs/draw/draw.h>
#include <cdogs/draw/draw_actor.h>
#include <cdogs/draw/drawtools.h>
#include <cdogs/events.h>
#include <cdogs/files.h>
#include <cdogs/font.h>
#include <cdogs/mission.h>
#include <cdogs/mission_convert.h>
#include <cdogs/pic_manager.h>

#include "editor_ui_cave.h"
#include "editor_ui_color.h"
#include "editor_ui_common.h"
#include "editor_ui_objectives.h"
#include "editor_ui_static.h"
#include "editor_ui_weapons.h"


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
static const char *CampaignGetMissionIndexStr(UIObject *o, void *data)
{
	static char s[128];
	UNUSED(o);
	CampaignOptions *co = data;
	Mission *mission = CampaignGetCurrentMission(co);
	if (mission == NULL)
	{
		if (co->Setting.Missions.size == 0)
		{
			return NULL;
		}
		sprintf(s, "End/%d", (int)co->Setting.Missions.size);
	}
	else
	{
		sprintf(
			s, "Mission %d/%d",
			co->MissionIndex + 1, (int)co->Setting.Missions.size);
	}
	return s;
}
static void CheckMission(UIObject *o, void *data)
{
	CampaignOptions *co = data;
	if (!CampaignGetCurrentMission(co))
	{
		o->IsVisible = false;
		// Need to unhighlight to prevent children being drawn
		UIObjectUnhighlight(o, false);
		return;
	}
	o->IsVisible = true;
}
MISSION_CHECK_TYPE_FUNC(MAPTYPE_CLASSIC)
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
	UNUSED(g);
	CampaignOptions *co = data;
	if (!CampaignGetCurrentMission(co)) return;
	DisplayFlag(
		Vec2iAdd(pos, o->Pos), "Edge rooms",
		CampaignGetCurrentMission(co)->u.Classic.Rooms.Edge,
		UIObjectIsHighlighted(o));
}
static void MissionDrawRoomsOverlap(
	UIObject *o, GraphicsDevice *g, Vec2i pos, void *data)
{
	UNUSED(o);
	UNUSED(g);
	UNUSED(pos);
	CampaignOptions *co = data;
	if (!CampaignGetCurrentMission(co)) return;
	DisplayFlag(
		Vec2iAdd(pos, o->Pos), "Room overlap",
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
	UNUSED(g);
	CampaignOptions *co = data;
	if (!CampaignGetCurrentMission(co)) return;
	DisplayFlag(
		Vec2iAdd(pos, o->Pos), "Doors",
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
	CampaignOptions *co = data;
	const Mission *m = CampaignGetCurrentMission(co);
	if (!m) return;
	const int idx = PicManagerGetWallStyleIndex(&gPicManager, m->WallStyle);
	DrawStyleArea(
		Vec2iAdd(pos, o->Pos),
		"Wall",
		&PicManagerGetMaskedStylePic(
			&gPicManager, "wall", m->WallStyle, "o",
			m->WallMask, m->AltMask)->pic,
		idx, (int)gPicManager.wallStyleNames.size,
		UIObjectIsHighlighted(o));
}
static void MissionDrawFloorStyle(
	UIObject *o, GraphicsDevice *g, Vec2i pos, void *data)
{
	UNUSED(g);
	CampaignOptions *co = data;
	const Mission *m = CampaignGetCurrentMission(co);
	if (!m) return;
	const int idx = PicManagerGetTileStyleIndex(&gPicManager, m->FloorStyle);
	DrawStyleArea(
		Vec2iAdd(pos, o->Pos),
		"Floor",
		&PicManagerGetMaskedStylePic(
			&gPicManager, "tile", m->FloorStyle, "normal",
			m->FloorMask, m->AltMask)->pic,
		idx, (int)gPicManager.tileStyleNames.size,
		UIObjectIsHighlighted(o));
}
static void MissionDrawRoomStyle(
	UIObject *o, GraphicsDevice *g, Vec2i pos, void *data)
{
	UNUSED(g);
	CampaignOptions *co = data;
	const Mission *m = CampaignGetCurrentMission(co);
	if (!m) return;
	const int idx = PicManagerGetTileStyleIndex(&gPicManager, m->RoomStyle);
	DrawStyleArea(
		Vec2iAdd(pos, o->Pos),
		"Rooms",
		&PicManagerGetMaskedStylePic(
			&gPicManager, "tile", m->RoomStyle, "normal",
			m->RoomMask, m->AltMask)->pic,
		idx, (int)gPicManager.tileStyleNames.size,
		UIObjectIsHighlighted(o));
}
static void MissionDrawDoorStyle(
	UIObject *o, GraphicsDevice *g, Vec2i pos, void *data)
{
	UNUSED(g);
	CampaignOptions *co = data;
	const Mission *m = CampaignGetCurrentMission(co);
	if (!m) return;
	const int idx = PicManagerGetDoorStyleIndex(&gPicManager, m->DoorStyle);
	DrawStyleArea(
		Vec2iAdd(pos, o->Pos),
		"Doors",
		&GetDoorPic(&gPicManager, m->DoorStyle, "normal", true)->pic,
		idx, (int)gPicManager.doorStyleNames.size,
		UIObjectIsHighlighted(o));
}
static void MissionDrawKeyStyle(
	UIObject *o, GraphicsDevice *g, Vec2i pos, void *data)
{
	UNUSED(g);
	CampaignOptions *co = data;
	Mission *m = CampaignGetCurrentMission(co);
	if (m == NULL)
	{
		return;
	}
	const int idx = PicManagerGetKeyStyleIndex(&gPicManager, m->KeyStyle);
	DrawStyleArea(
		Vec2iAdd(pos, o->Pos),
		"Keys",
		KeyPickupClass(m->KeyStyle, 0)->Pic,
		idx, (int)gPicManager.keyStyleNames.size,
		UIObjectIsHighlighted(o));
}
static void MissionDrawExitStyle(
	UIObject *o, GraphicsDevice *g, Vec2i pos, void *data)
{
	UNUSED(g);
	CampaignOptions *co = data;
	Mission *m = CampaignGetCurrentMission(co);
	if (m == NULL)
	{
		return;
	}
	const int idx = PicManagerGetExitStyleIndex(&gPicManager, m->ExitStyle);
	DrawStyleArea(
		Vec2iAdd(pos, o->Pos),
		"Exit",
		&PicManagerGetExitPic(&gPicManager, m->ExitStyle, false)->pic,
		idx, (int)gPicManager.exitStyleNames.size,
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
	CA_FOREACH(const GunDescription, g, gGunDescriptions.Guns)
		if (g->IsRealGun)
		{
			totalWeapons++;
		}
	CA_FOREACH_END()
	CA_FOREACH(const GunDescription, g, gGunDescriptions.CustomGuns)
		if (g->IsRealGun)
		{
			totalWeapons++;
		}
	CA_FOREACH_END()
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
	DrawCharacterSimple(
		CArrayGet(&store->OtherChars, charIndex),
		Vec2iAdd(Vec2iAdd(pos, o->Pos), Vec2iScaleDiv(o->Size, 2)),
		DIRECTION_DOWN, UIObjectIsHighlighted(o), true);
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
	DrawCharacterSimple(
		CArrayGet(&store->OtherChars, charIndex),
		Vec2iAdd(Vec2iAdd(pos, o->Pos), Vec2iScaleDiv(o->Size, 2)),
		DIRECTION_DOWN, UIObjectIsHighlighted(o), true);
}
static void MissionDrawMapItem(
	UIObject *o, GraphicsDevice *g, Vec2i pos, void *vData)
{
	UNUSED(g);
	MissionIndexData *data = vData;
	if (!CampaignGetCurrentMission(data->co)) return;
	const Mission *m = CampaignGetCurrentMission(data->co);
	if (data->index >= (int)m->MapObjectDensities.size) return;
	const MapObjectDensity *mod =
		CArrayGet(&m->MapObjectDensities, data->index);
	DisplayMapItemWithDensity(
		Vec2iAdd(Vec2iAdd(pos, o->Pos), Vec2iScaleDiv(o->Size, 2)),
		mod,
		UIObjectIsHighlighted(o));
}
static void DrawStyleArea(
	Vec2i pos,
	const char *name,
	const Pic *pic,
	int idx, int count,
	int isHighlighted)
{
	FontStrMask(name, pos, isHighlighted ? colorRed : colorWhite);
	pos.y += FontH();
	Blit(&gGraphicsDevice, pic, pos);
	// Display style index and count, right aligned
	char buf[16];
	sprintf(buf, "%d/%d", idx + 1, count);
	FontStrMask(
		buf, Vec2iNew(pos.x + 28 - FontStrW(buf), pos.y + 17), colorGray);
}


static void CampaignChangeMission(void *data, int d)
{
	CampaignOptions *co = data;
	co->MissionIndex =
		CLAMP(co->MissionIndex + d, 0, (int)co->Setting.Missions.size);
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
typedef struct
{
	CampaignOptions *C;
	MapType Type;
} MissionChangeTypeData;
static void MissionChangeType(void *data, int d)
{
	UNUSED(d);
	MissionChangeTypeData *mct = data;
	if (mct->Type == gMission.missionData->Type)
	{
		return;
	}
	Map map;
	MissionOptionsTerminate(&gMission);
	CampaignAndMissionSetup(mct->C, &gMission);
	memset(&map, 0, sizeof map);
	MapLoad(&map, &gMission, mct->C);
	MapLoadDynamic(&map, &gMission, &mct->C->Setting.characters);
	MissionConvertToType(gMission.missionData, &map, mct->Type);
}
static void MissionChangeWallStyle(void *data, int d)
{
	CampaignOptions *co = data;
	Mission *m = CampaignGetCurrentMission(co);
	const int idx = CLAMP_OPPOSITE(
		PicManagerGetWallStyleIndex(&gPicManager, m->WallStyle) + d,
		0,
		(int)gPicManager.wallStyleNames.size - 1);
	strcpy(
		m->WallStyle, *(char **)CArrayGet(&gPicManager.wallStyleNames, idx));
}
static void MissionChangeFloorStyle(void *data, int d)
{
	CampaignOptions *co = data;
	Mission *m = CampaignGetCurrentMission(co);
	const int idx = CLAMP_OPPOSITE(
		PicManagerGetTileStyleIndex(&gPicManager, m->FloorStyle) + d,
		0,
		(int)gPicManager.tileStyleNames.size - 1);
	strcpy(
		m->FloorStyle, *(char **)CArrayGet(&gPicManager.tileStyleNames, idx));
}
static void MissionChangeRoomStyle(void *data, int d)
{
	CampaignOptions *co = data;
	Mission *m = CampaignGetCurrentMission(co);
	const int idx = CLAMP_OPPOSITE(
		PicManagerGetTileStyleIndex(&gPicManager, m->RoomStyle) + d,
		0,
		(int)gPicManager.tileStyleNames.size - 1);
	strcpy(
		m->RoomStyle, *(char **)CArrayGet(&gPicManager.tileStyleNames, idx));
}
static void MissionChangeDoorStyle(void *data, int d)
{
	CampaignOptions *co = data;
	Mission *m = CampaignGetCurrentMission(co);
	const int idx = CLAMP_OPPOSITE(
		PicManagerGetDoorStyleIndex(&gPicManager, m->DoorStyle) + d,
		0,
		(int)gPicManager.doorStyleNames.size - 1);
	strcpy(
		m->DoorStyle, *(char **)CArrayGet(&gPicManager.doorStyleNames, idx));
}
static void MissionChangeKeyStyle(void *data, int d)
{
	CampaignOptions *co = data;
	Mission *m = CampaignGetCurrentMission(co);
	const int idx = CLAMP_OPPOSITE(
		PicManagerGetKeyStyleIndex(&gPicManager, m->KeyStyle) + d,
		0,
		(int)gPicManager.keyStyleNames.size - 1);
	strcpy(m->KeyStyle, *(char **)CArrayGet(&gPicManager.keyStyleNames, idx));
}
static void MissionChangeExitStyle(void *data, int d)
{
	CampaignOptions *co = data;
	Mission *m = CampaignGetCurrentMission(co);
	const int idx = CLAMP_OPPOSITE(
		PicManagerGetExitStyleIndex(&gPicManager, m->ExitStyle) + d,
		0,
		(int)gPicManager.exitStyleNames.size - 1);
	strcpy(
		m->ExitStyle, *(char **)CArrayGet(&gPicManager.exitStyleNames, idx));
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
static void DeactivateBrush(UIObject *o, void *data)
{
	UNUSED(o);
	EditorBrush *b = data;
	b->IsActive = false;
}


static UIObject *CreateCampaignObjs(CampaignOptions *co);
static UIObject *CreateMissionObjs(CampaignOptions *co);
static UIObject *CreateClassicMapObjs(Vec2i pos, CampaignOptions *co);
static UIObject *CreateMapItemObjs(CampaignOptions *co, int dy);
static UIObject *CreateCharacterObjs(CampaignOptions *co, int dy);
static UIObject *CreateSpecialCharacterObjs(CampaignOptions *co, int dy);

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

	UIObject *o;

	// Collapse button
	cData->collapsePic = PicManagerGetPic(&gPicManager, "editor/collapse");
	cData->expandPic = PicManagerGetPic(&gPicManager, "editor/expand");
	o = UIObjectCreate(
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
	o->IsBackground = true;
	o->u.CustomDrawFunc = DrawBackground;
	o->OnFocusFunc = DeactivateBrush;
	o->Data = brush;
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
	// Only draw background over completely transparent pixels
	const color_t c = { 32, 32, 64, 196 };
	const Uint32 p = COLOR2PIXEL(c);
	Vec2i v;
	for (v.y = 0; v.y < o->Size.y; v.y++)
	{
		for (v.x = 0; v.x < o->Size.x; v.x++)
		{
			const Vec2i pos1 = Vec2iAdd(pos, v);
			if (pos1.x < g->clipping.left || pos1.x > g->clipping.right ||
				pos1.y < g->clipping.top || pos1.y > g->clipping.bottom)
			{
				continue;
			}
			const int idx = PixelIndex(
				pos1.x, pos1.y, g->cachedConfig.Res.x, g->cachedConfig.Res.y);
			const color_t existing = PIXEL2COLOR(g->buf[idx]);
			if (existing.a != 0)
			{
				continue;
			}
			g->buf[idx] = p;
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
	Vec2i pos = Vec2iZero();
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

	// Mission insert/delete/index, current mission
	// Layout from right to left
	o = UIObjectCreate(
		UITYPE_LABEL, 0,
		Vec2iNew(270, pos.y), Vec2iNew(FontStrW("Mission 99/99"), th));
	o->u.LabelFunc = CampaignGetMissionIndexStr;
	o->ChangeFunc = CampaignChangeMission;
	o->ReloadData = true;
	o->Data = &gCampaign;
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

	// Drop-down menu for map type
	pos.x += 40;
	o2 = UIObjectCopy(o);
	o2->Size.x = 50;
	o2->u.LabelFunc = MissionGetTypeStr;
	o2->Data = co;
	o2->Pos = pos;
	CSTRDUP(o2->Tooltip,
		"WARNING: changing map type will\nlose your previous map settings");
	UIObject *oMapType =
		UIObjectCreate(UITYPE_CONTEXT_MENU, 0, Vec2iZero(), Vec2iZero());
	for (int i = 0; i < (int)MAPTYPE_COUNT; i++)
	{
		UIObject *oMapTypeChild = UIObjectCopy(o);
		oMapTypeChild->Pos.y = i * th;
		UIObjectSetDynamicLabel(oMapTypeChild, MapTypeStr((MapType)i));
		oMapTypeChild->IsDynamicData = true;
		CMALLOC(oMapTypeChild->Data, sizeof(MissionChangeTypeData));
		((MissionChangeTypeData *)oMapTypeChild->Data)->C = co;
		((MissionChangeTypeData *)oMapTypeChild->Data)->Type = (MapType)i;
		oMapTypeChild->ChangeFunc = MissionChangeType;
		UIObjectAddChild(oMapType, oMapTypeChild);
	}
	UIObjectAddChild(o2, oMapType);
	// HACK: add another container UI object so that the BFS draw order doesn't
	// draw the context menu below the type-specific objs
	UIObject *hackContainer =
		UIObjectCreate(UITYPE_NONE, 0, Vec2iZero(), Vec2iZero());
	UIObjectAddChild(hackContainer, o2);
	UIObjectAddChild(c, hackContainer);
	pos.x = 20;
	pos.y += th;
	UIObjectAddChild(c, CreateClassicMapObjs(pos, co));
	UIObjectAddChild(c, CreateStaticMapObjs(pos, co, brush));
	UIObjectAddChild(c, CreateCaveMapObjs(pos, co));

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

	UIObjectDestroy(o);

	// objectives
	pos.y += 2 + th;
	CreateObjectivesObjs(co, c, pos);

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
	o->u.Textbox.MaxLen = sizeof ((Mission *)0)->Song - 1;
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
	const int x = pos.x;
	o->ChangesData = true;
	// Check whether the map type matches, and set visibility
	c->CheckVisible = MissionCheckTypeFunc;
	c->Data = co;

	UIObject *o2 = CreateCampaignSeedObj(pos, co);
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
	o2->ChangesData = true;
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

typedef struct
{
	CampaignOptions *C;
	int Idx;
	MapObject *M;
} MapItemIndexData;
static void MissionChangeMapItemDensity(void *vData, int d);
static bool MapItemObjFunc(UIObject *o, MapObject *mo, void *vData);
static UIObject *CreateMapItemObjs(CampaignOptions *co, int dy)
{
	UIObject *c = UIObjectCreate(UITYPE_NONE, 0, Vec2iZero(), Vec2iZero());
	c->Flags = UI_ENABLED_WHEN_PARENT_HIGHLIGHTED_ONLY;

	UIObject *o = UIObjectCreate(
		UITYPE_CUSTOM, 0, Vec2iZero(), Vec2iNew(20, 40));
	o->u.CustomDrawFunc = MissionDrawMapItem;
	o->ChangeFuncAlt = MissionChangeMapItemDensity;
	o->Flags = UI_LEAVE_YC;
	o->ChangesData = 1;
	for (int i = 0; i < 32; i++)	// TODO: no limit to objects
	{
		const int x = 10 + i * 20;
		// Drop-down menu for objective type
		UIObject *o2 = UIObjectCopy(o);
		o2->Id2 = i;
		CMALLOC(o2->Data, sizeof(MissionIndexData));
		o2->IsDynamicData = 1;
		((MissionIndexData *)o2->Data)->co = co;
		((MissionIndexData *)o2->Data)->index = i;
		o2->Pos = Vec2iNew(x, Y_ABS - dy);
		CSTRDUP(
			o2->Tooltip,
			"Click: change map object; Shift+Click: change density");
		UIObjectAddChild(o2, CreateAddMapItemObjs(
			Vec2iNew(o2->Size.x, o2->Size.y / 2),
			MapItemObjFunc, o2->Data, sizeof(MapItemIndexData), false));
		UIObjectAddChild(c, o2);
	}

	UIObjectDestroy(o);
	return c;
}
static void MissionChangeMapItemDensity(void *vData, int d)
{
	MissionIndexData *data = vData;
	Mission *m = CampaignGetCurrentMission(data->co);
	if (data->index >= (int)m->MapObjectDensities.size)
	{
		return;
	}
	MapObjectDensity *mod = CArrayGet(&m->MapObjectDensities, data->index);
	mod->Density = CLAMP(mod->Density + 5 * d, 0, 512);
}
static void MissionSetMapItem(void *vData, int d);
static void DrawMapItem(
	UIObject *o, GraphicsDevice *g, Vec2i pos, void *vData);
static bool MapItemObjFunc(UIObject *o, MapObject *mo, void *vData)
{
	o->ChangeFunc = MissionSetMapItem;
	o->u.CustomDrawFunc = DrawMapItem;
	MissionIndexData *data = vData;
	((MapItemIndexData *)o->Data)->C = data->co;
	((MapItemIndexData *)o->Data)->Idx = data->index;
	((MapItemIndexData *)o->Data)->M = mo;
	o->Tooltip = MakePlacementFlagTooltip(mo);
	return true;
}
static void MissionSetMapItem(void *vData, int d)
{
	UNUSED(d);
	MapItemIndexData *data = vData;
	Mission *m = CampaignGetCurrentMission(data->C);
	if (data->Idx >= (int)m->MapObjectDensities.size)
	{
		return;
	}
	MapObjectDensity *mod = CArrayGet(&m->MapObjectDensities, data->Idx);
	mod->M = data->M;
}
static void DrawMapItem(
	UIObject *o, GraphicsDevice *g, Vec2i pos, void *vData)
{
	const MapItemIndexData *data = vData;
	DisplayMapItem(
		Vec2iAdd(Vec2iAdd(pos, o->Pos), Vec2iScaleDiv(o->Size, 2)), data->M);
	if (data->M->Type == MAP_OBJECT_TYPE_PICKUP_SPAWNER)
	{
		// Also draw the pickup object spawned by this spawner
		const Pic *pic = data->M->u.PickupClass->Pic;
		pos = Vec2iMinus(pos, Vec2iScaleDiv(pic->size, 2));
		Blit(
			g, pic,
			Vec2iAdd(Vec2iAdd(pos, o->Pos), Vec2iScaleDiv(o->Size, 2)));
	}
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
