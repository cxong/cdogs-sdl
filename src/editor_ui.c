/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (c) 2013-2014, Cong Xu
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

#include <cdogs/draw.h>
#include <cdogs/events.h>
#include <cdogs/mission.h>
#include <cdogs/mission_convert.h>
#include <cdogs/pic_manager.h>
#include <cdogs/text.h>


static void DrawStyleArea(
	Vec2i pos,
	const char *name,
	GraphicsDevice *device,
	PicPaletted *pic,
	int index, int count,
	int isHighlighted);

static char *CampaignGetTitle(UIObject *o, CampaignOptions *c)
{
	UNUSED(o);
	return c->Setting.Title;
}
static char *CampaignGetAuthor(UIObject *o, CampaignOptions *c)
{
	UNUSED(o);
	return c->Setting.Author;
}
static char *CampaignGetDescription(UIObject *o, CampaignOptions *c)
{
	UNUSED(o);
	return c->Setting.Description;
}
static char *CampaignGetSeedStr(UIObject *o, CampaignOptions *c)
{
	static char s[128];
	UNUSED(o);
	sprintf(s, "Seed: %u", c->seed);
	return s;
}
static void CheckMission(
	UIObject *o, GraphicsDevice *g, Vec2i pos, CampaignOptions *co)
{
	UNUSED(g);
	UNUSED(pos);
	if (!CampaignGetCurrentMission(co))
	{
		o->IsVisible = 0;
		// Need to unhighlight to prevent children being drawn
		UIObjectUnhighlight(o);
		return;
	}
	o->IsVisible = 1;
}
static void MissionCheckTypeClassic(
	UIObject *o, GraphicsDevice *g, Vec2i pos, CampaignOptions *co)
{
	Mission *m = CampaignGetCurrentMission(co);
	UNUSED(g);
	UNUSED(pos);
	if (!m || m->Type != MAPTYPE_CLASSIC)
	{
		o->IsVisible = 0;
		// Need to unhighlight to prevent children being drawn
		UIObjectUnhighlight(o);
		return;
	}
	o->IsVisible = 1;
}
static void MissionCheckTypeStatic(
	UIObject *o, GraphicsDevice *g, Vec2i pos, CampaignOptions *co)
{
	Mission *m = CampaignGetCurrentMission(co);
	UNUSED(g);
	UNUSED(pos);
	if (!m || m->Type != MAPTYPE_STATIC)
	{
		o->IsVisible = 0;
		// Need to unhighlight to prevent children being drawn
		UIObjectUnhighlight(o);
		return;
	}
	o->IsVisible = 1;
}
static char *MissionGetTitle(UIObject *o, CampaignOptions *co)
{
	UNUSED(o);
	if (!CampaignGetCurrentMission(co))
	{
		o->IsVisible = 0;
		return NULL;
	}
	o->IsVisible = 1;
	return CampaignGetCurrentMission(co)->Title;
}
static char *MissionGetDescription(UIObject *o, CampaignOptions *co)
{
	UNUSED(o);
	if (!CampaignGetCurrentMission(co))
	{
		o->IsVisible = 0;
		return NULL;
	}
	o->IsVisible = 1;
	return CampaignGetCurrentMission(co)->Description;
}
static char *MissionGetSong(UIObject *o, CampaignOptions *co)
{
	UNUSED(o);
	if (!CampaignGetCurrentMission(co))
	{
		o->IsVisible = 0;
		return NULL;
	}
	o->IsVisible = 1;
	return CampaignGetCurrentMission(co)->Song;
}
static char *MissionGetWidthStr(UIObject *o, CampaignOptions *co)
{
	static char s[128];
	UNUSED(o);
	if (!CampaignGetCurrentMission(co)) return NULL;
	sprintf(s, "Width: %d", CampaignGetCurrentMission(co)->Size.x);
	return s;
}
static char *MissionGetHeightStr(UIObject *o, CampaignOptions *co)
{
	static char s[128];
	UNUSED(o);
	if (!CampaignGetCurrentMission(co)) return NULL;
	sprintf(s, "Height: %d", CampaignGetCurrentMission(co)->Size.y);
	return s;
}
static char *MissionGetWallCountStr(UIObject *o, CampaignOptions *co)
{
	static char s[128];
	UNUSED(o);
	if (!CampaignGetCurrentMission(co)) return NULL;
	sprintf(s, "Walls: %d", CampaignGetCurrentMission(co)->u.Classic.Walls);
	return s;
}
static char *MissionGetWallLengthStr(UIObject *o, CampaignOptions *co)
{
	static char s[128];
	UNUSED(o);
	if (!CampaignGetCurrentMission(co)) return NULL;
	sprintf(s, "Len: %d", CampaignGetCurrentMission(co)->u.Classic.WallLength);
	return s;
}
static char *MissionGetCorridorWidthStr(UIObject *o, CampaignOptions *co)
{
	static char s[128];
	UNUSED(o);
	if (!CampaignGetCurrentMission(co)) return NULL;
	sprintf(s, "CorridorWidth: %d", CampaignGetCurrentMission(co)->u.Classic.CorridorWidth);
	return s;
}
static char *MissionGetRoomCountStr(UIObject *o, CampaignOptions *co)
{
	static char s[128];
	UNUSED(o);
	if (!CampaignGetCurrentMission(co)) return NULL;
	sprintf(s, "Rooms: %d", CampaignGetCurrentMission(co)->u.Classic.Rooms);
	return s;
}
static char *MissionGetRoomMinStr(UIObject *o, CampaignOptions *co)
{
	static char s[128];
	UNUSED(o);
	if (!CampaignGetCurrentMission(co)) return NULL;
	sprintf(s, "RoomMin: %d", CampaignGetCurrentMission(co)->u.Classic.Rooms.Min);
	return s;
}
static char *MissionGetRoomMaxStr(UIObject *o, CampaignOptions *co)
{
	static char s[128];
	UNUSED(o);
	if (!CampaignGetCurrentMission(co)) return NULL;
	sprintf(s, "RoomMax: %d", CampaignGetCurrentMission(co)->u.Classic.Rooms.Max);
	return s;
}
static void MissionDrawEdgeRooms(
	UIObject *o, GraphicsDevice *g, Vec2i pos, CampaignOptions *co)
{
	static char s[128];
	UNUSED(o);
	if (!CampaignGetCurrentMission(co)) return;
	DisplayFlag(
		g, Vec2iAdd(pos, o->Pos), "Edge rooms",
		CampaignGetCurrentMission(co)->u.Classic.Rooms.Edge,
		UIObjectIsHighlighted(o));
}
static void MissionDrawRoomsOverlap(
	UIObject *o, GraphicsDevice *g, Vec2i pos, CampaignOptions *co)
{
	static char s[128];
	UNUSED(o);
	UNUSED(pos);
	if (!CampaignGetCurrentMission(co)) return;
	DisplayFlag(
		g, Vec2iAdd(pos, o->Pos), "Room overlap",
		CampaignGetCurrentMission(co)->u.Classic.Rooms.Overlap,
		UIObjectIsHighlighted(o));
}
static char *MissionGetRoomWallCountStr(UIObject *o, CampaignOptions *co)
{
	static char s[128];
	UNUSED(o);
	if (!CampaignGetCurrentMission(co)) return NULL;
	sprintf(s, "RoomWalls: %d", CampaignGetCurrentMission(co)->u.Classic.Rooms.Walls);
	return s;
}
static char *MissionGetRoomWallLenStr(UIObject *o, CampaignOptions *co)
{
	static char s[128];
	UNUSED(o);
	if (!CampaignGetCurrentMission(co)) return NULL;
	sprintf(s, "RoomWallLen: %d", CampaignGetCurrentMission(co)->u.Classic.Rooms.WallLength);
	return s;
}
static char *MissionGetRoomWallPadStr(UIObject *o, CampaignOptions *co)
{
	static char s[128];
	UNUSED(o);
	if (!CampaignGetCurrentMission(co)) return NULL;
	sprintf(s, "RoomWallPad: %d", CampaignGetCurrentMission(co)->u.Classic.Rooms.WallPad);
	return s;
}
static char *MissionGetSquareCountStr(UIObject *o, CampaignOptions *co)
{
	static char s[128];
	UNUSED(o);
	if (!CampaignGetCurrentMission(co)) return NULL;
	sprintf(s, "Sqr: %d", CampaignGetCurrentMission(co)->u.Classic.Squares);
	return s;
}
static void MissionDrawDoorEnabled(
	UIObject *o, GraphicsDevice *g, Vec2i pos, CampaignOptions *co)
{
	static char s[128];
	UNUSED(o);
	if (!CampaignGetCurrentMission(co)) return;
	DisplayFlag(
		g, Vec2iAdd(pos, o->Pos), "Doors",
		CampaignGetCurrentMission(co)->u.Classic.Doors.Enabled,
		UIObjectIsHighlighted(o));
}
static char *MissionGetDoorMinStr(UIObject *o, CampaignOptions *co)
{
	static char s[128];
	UNUSED(o);
	if (!CampaignGetCurrentMission(co)) return NULL;
	sprintf(s, "DoorMin: %d", CampaignGetCurrentMission(co)->u.Classic.Doors.Min);
	return s;
}
static char *MissionGetDoorMaxStr(UIObject *o, CampaignOptions *co)
{
	static char s[128];
	UNUSED(o);
	if (!CampaignGetCurrentMission(co)) return NULL;
	sprintf(s, "DoorMax: %d", CampaignGetCurrentMission(co)->u.Classic.Doors.Max);
	return s;
}
static char *MissionGetPillarCountStr(UIObject *o, CampaignOptions *co)
{
	static char s[128];
	UNUSED(o);
	if (!CampaignGetCurrentMission(co)) return NULL;
	sprintf(s, "Pillars: %d", CampaignGetCurrentMission(co)->u.Classic.Pillars.Count);
	return s;
}
static char *MissionGetPillarMinStr(UIObject *o, CampaignOptions *co)
{
	static char s[128];
	UNUSED(o);
	if (!CampaignGetCurrentMission(co)) return NULL;
	sprintf(s, "PillarMin: %d", CampaignGetCurrentMission(co)->u.Classic.Pillars.Min);
	return s;
}
static char *MissionGetPillarMaxStr(UIObject *o, CampaignOptions *co)
{
	static char s[128];
	UNUSED(o);
	if (!CampaignGetCurrentMission(co)) return NULL;
	sprintf(s, "PillarMax: %d", CampaignGetCurrentMission(co)->u.Classic.Pillars.Max);
	return s;
}
static char *MissionGetDensityStr(UIObject *o, CampaignOptions *co)
{
	static char s[128];
	UNUSED(o);
	if (!CampaignGetCurrentMission(co)) return NULL;
	sprintf(s, "Dens: %d", CampaignGetCurrentMission(co)->EnemyDensity);
	return s;
}
static char *MissionGetTypeStr(UIObject *o, CampaignOptions *co)
{
	static char s[128];
	UNUSED(o);
	if (!CampaignGetCurrentMission(co)) return NULL;
	sprintf(s, "Type: %s", MapTypeStr(CampaignGetCurrentMission(co)->Type));
	return s;
}
static void MissionDrawWallStyle(
	UIObject *o, GraphicsDevice *g, Vec2i pos, CampaignOptions *co)
{
	int index;
	int count = WALL_STYLE_COUNT;
	if (!CampaignGetCurrentMission(co)) return;
	index = CampaignGetCurrentMission(co)->WallStyle;
	DrawStyleArea(
		Vec2iAdd(pos, o->Pos),
		"Wall",
		g,
		PicManagerGetOldPic(&gPicManager, cWallPics[index % count][WALL_SINGLE]),
		index, count,
		UIObjectIsHighlighted(o));
}
static void MissionDrawFloorStyle(
	UIObject *o, GraphicsDevice *g, Vec2i pos, CampaignOptions *co)
{
	int index;
	int count = FLOOR_STYLE_COUNT;
	if (!CampaignGetCurrentMission(co))
	{
		o->IsVisible = 0;
		return;
	}
	o->IsVisible = 1;
	index = CampaignGetCurrentMission(co)->FloorStyle;
	DrawStyleArea(
		Vec2iAdd(pos, o->Pos),
		"Floor",
		g,
		PicManagerGetOldPic(&gPicManager, cFloorPics[index % count][FLOOR_NORMAL]),
		index, count,
		UIObjectIsHighlighted(o));
}
static void MissionDrawRoomStyle(
	UIObject *o, GraphicsDevice *g, Vec2i pos, CampaignOptions *co)
{
	int index;
	int count = ROOMFLOOR_COUNT;
	if (!CampaignGetCurrentMission(co))
	{
		o->IsVisible = 0;
		return;
	}
	o->IsVisible = 1;
	index = CampaignGetCurrentMission(co)->RoomStyle;
	DrawStyleArea(
		Vec2iAdd(pos, o->Pos),
		"Rooms",
		g,
		PicManagerGetOldPic(&gPicManager, cRoomPics[index % count][ROOMFLOOR_NORMAL]),
		index, count,
		UIObjectIsHighlighted(o));
}
static void MissionDrawDoorStyle(
	UIObject *o, GraphicsDevice *g, Vec2i pos, CampaignOptions *co)
{
	int index;
	int count = GetEditorInfo().doorCount;
	if (!CampaignGetCurrentMission(co))
	{
		o->IsVisible = 0;
		return;
	}
	o->IsVisible = 1;
	index = CampaignGetCurrentMission(co)->DoorStyle;
	DrawStyleArea(
		Vec2iAdd(pos, o->Pos),
		"Doors",
		g,
		PicManagerGetOldPic(&gPicManager, cGeneralPics[gMission.doorPics[0].horzPic].picIndex),
		index, count,
		UIObjectIsHighlighted(o));
}
static void MissionDrawKeyStyle(
	UIObject *o, GraphicsDevice *g, Vec2i pos, CampaignOptions *co)
{
	int index;
	int count = GetEditorInfo().keyCount;
	if (!CampaignGetCurrentMission(co))
	{
		o->IsVisible = 0;
		return;
	}
	o->IsVisible = 1;
	index = CampaignGetCurrentMission(co)->KeyStyle;
	DrawStyleArea(
		Vec2iAdd(pos, o->Pos),
		"Keys",
		g,
		PicManagerGetOldPic(&gPicManager, cGeneralPics[gMission.keyPics[0]].picIndex),
		index, count,
		UIObjectIsHighlighted(o));
}
static void MissionDrawExitStyle(
	UIObject *o, GraphicsDevice *g, Vec2i pos, CampaignOptions *co)
{
	int index;
	int count = GetEditorInfo().exitCount;
	if (!CampaignGetCurrentMission(co))
	{
		o->IsVisible = 0;
		return;
	}
	o->IsVisible = 1;
	index = CampaignGetCurrentMission(co)->ExitStyle;
	DrawStyleArea(
		Vec2iAdd(pos, o->Pos),
		"Exit",
		g,
		PicManagerGetOldPic(&gPicManager, gMission.exitPic),
		index, count,
		UIObjectIsHighlighted(o));
}
static char *MissionGetWallColorStr(UIObject *o, CampaignOptions *co)
{
	static char s[128];
	UNUSED(o);
	if (!CampaignGetCurrentMission(co)) return NULL;
	sprintf(s, "Walls: %s", RangeName(CampaignGetCurrentMission(co)->WallColor));
	return s;
}
static char *MissionGetFloorColorStr(UIObject *o, CampaignOptions *co)
{
	static char s[128];
	UNUSED(o);
	if (!CampaignGetCurrentMission(co)) return NULL;
	sprintf(s, "Floor: %s", RangeName(CampaignGetCurrentMission(co)->FloorColor));
	return s;
}
static char *MissionGeRoomColorStr(UIObject *o, CampaignOptions *co)
{
	static char s[128];
	UNUSED(o);
	if (!CampaignGetCurrentMission(co)) return NULL;
	sprintf(s, "Rooms: %s", RangeName(CampaignGetCurrentMission(co)->RoomColor));
	return s;
}
static char *MissionGeExtraColorStr(UIObject *o, CampaignOptions *co)
{
	static char s[128];
	UNUSED(o);
	if (!CampaignGetCurrentMission(co)) return NULL;
	sprintf(s, "Extra: %s", RangeName(CampaignGetCurrentMission(co)->AltColor));
	return s;
}
static char *MissionGetCharacterCountStr(UIObject *o, CampaignOptions *co)
{
	static char s[128];
	UNUSED(o);
	if (!CampaignGetCurrentMission(co)) return NULL;
	sprintf(s, "Characters (%d)", CampaignGetCurrentMission(co)->Enemies.size);
	return s;
}
static char *MissionGetSpecialCountStr(UIObject *o, CampaignOptions *co)
{
	static char s[128];
	UNUSED(o);
	if (!CampaignGetCurrentMission(co)) return NULL;
	sprintf(
		s, "Mission objective characters (%d)",
		CampaignGetCurrentMission(co)->SpecialChars.size);
	return s;
}
static char *MissionGetObjectiveDescription(
	UIObject *o, CampaignOptions *co)
{
	static char s[128];
	int i;
	if (!CampaignGetCurrentMission(co))
	{
		o->IsVisible = 0;
		return NULL;
	}
	i = o->Id - YC_OBJECTIVES;
	if ((int)CampaignGetCurrentMission(co)->Objectives.size <= i)
	{
		if (i == 0)
		{
			// first objective and mission has no objectives
			o->IsVisible = 1;
			o->u.Textbox.IsEditable = 0;
			return "-- mission objectives --";
		}
		o->IsVisible = 0;
		return NULL;
	}
	o->IsVisible = 1;
	o->u.Textbox.IsEditable = 1;
	return ((MissionObjective *)CArrayGet(&CampaignGetCurrentMission(co)->Objectives, i))->Description;
}
static char *GetWeaponCountStr(UIObject *o, void *v)
{
	static char s[128];
	UNUSED(o);
	UNUSED(v);
	sprintf(
		s, "Available weapons (%d/%d)",
		GetNumWeapons(gMission.missionData->Weapons), GUN_COUNT);
	return s;
}
static char *GetObjectCountStr(UIObject *o, void *v)
{
	static char s[128];
	UNUSED(o);
	UNUSED(v);
	sprintf(s, "Map items (%d)", gMission.MapObjects.size);
	return s;
}
typedef struct
{
	CampaignOptions *co;
	int index;
} MissionIndexData;
static void MissionDrawEnemy(
	UIObject *o, GraphicsDevice *g, Vec2i pos, MissionIndexData *data)
{
	UNUSED(g);
	if (!CampaignGetCurrentMission(data->co)) return;
	if (data->index >= (int)CampaignGetCurrentMission(data->co)->Enemies.size)
	{
		return;
	}
	DisplayCharacter(
		Vec2iAdd(Vec2iAdd(pos, o->Pos), Vec2iScaleDiv(o->Size, 2)),
		data->co->Setting.characters.baddies[data->index],
		UIObjectIsHighlighted(o), 1);
}
static void MissionDrawSpecialChar(
	UIObject *o, GraphicsDevice *g, Vec2i pos, MissionIndexData *data)
{
	UNUSED(g);
	if (!CampaignGetCurrentMission(data->co)) return;
	if (data->index >=
		(int)CampaignGetCurrentMission(data->co)->SpecialChars.size)
	{
		return;
	}
	DisplayCharacter(
		Vec2iAdd(Vec2iAdd(pos, o->Pos), Vec2iScaleDiv(o->Size, 2)),
		data->co->Setting.characters.specials[data->index],
		UIObjectIsHighlighted(o), 1);
}
static void DisplayMapItemWithDensity(
	GraphicsDevice *g,
	Vec2i pos, MapObject *mo, int density, int isHighlighted);
static void MissionDrawMapItem(
	UIObject *o, GraphicsDevice *g, Vec2i pos, MissionIndexData *data)
{
	if (!CampaignGetCurrentMission(data->co)) return;
	if (data->index >= (int)CampaignGetCurrentMission(data->co)->Items.size) return;
	DisplayMapItemWithDensity(
		g,
		Vec2iAdd(Vec2iAdd(pos, o->Pos), Vec2iScaleDiv(o->Size, 2)),
		CArrayGet(&gMission.MapObjects, data->index),
		*(int *)CArrayGet(&CampaignGetCurrentMission(data->co)->ItemDensities, data->index),
		UIObjectIsHighlighted(o));
}
static void MissionDrawWeaponStatus(
	UIObject *o, GraphicsDevice *g, Vec2i pos, MissionIndexData *data)
{
	int hasWeapon;
	if (!CampaignGetCurrentMission(data->co)) return;
	hasWeapon = CampaignGetCurrentMission(data->co)->Weapons[data->index];
	DisplayFlag(
		g,
		Vec2iAdd(pos, o->Pos),
		gGunDescriptions[data->index].name,
		hasWeapon,
		UIObjectIsHighlighted(o));
}
static const char *MissionGetObjectiveStr(UIObject *o, MissionIndexData *data)
{
	UNUSED(o);
	if (!CampaignGetCurrentMission(data->co)) return NULL;
	if ((int)CampaignGetCurrentMission(data->co)->Objectives.size <= data->index) return NULL;
	return ObjectiveTypeStr(((MissionObjective *)CArrayGet(
		&CampaignGetCurrentMission(data->co)->Objectives, data->index))->Type);
}
static void GetCharacterHeadPic(
	Character *c, TOffsetPic *pic, TranslationTable **t);
static void MissionDrawObjective(
	UIObject *o, GraphicsDevice *g, Vec2i pos, MissionIndexData *data)
{
	CharacterStore *store = &data->co->Setting.characters;
	Character *c;
	TOffsetPic pic;
	TranslationTable *table = NULL;
	struct Objective *obj;
	UNUSED(g);
	if (!CampaignGetCurrentMission(data->co)) return;
	if ((int)CampaignGetCurrentMission(data->co)->Objectives.size <= data->index) return;
	// TODO: only one kill and rescue objective allowed
	obj = CArrayGet(&gMission.Objectives, data->index);
	switch (((MissionObjective *)CArrayGet(
		&CampaignGetCurrentMission(data->co)->Objectives, data->index))->Type)
	{
	case OBJECTIVE_KILL:
		if (store->specialCount == 0)
		{
			c = &store->players[0];
		}
		else
		{
			c = CharacterStoreGetSpecial(store, 0);
		}
		GetCharacterHeadPic(c, &pic, &table);
		break;
	case OBJECTIVE_RESCUE:
		if (store->prisonerCount == 0)
		{
			c = &store->players[0];
		}
		else
		{
			c = CharacterStoreGetPrisoner(store, 0);
		}
		GetCharacterHeadPic(c, &pic, &table);
		break;
	case OBJECTIVE_COLLECT:
		pic = cGeneralPics[obj->pickupItem];
		break;
	case OBJECTIVE_DESTROY:
		pic = cGeneralPics[obj->blowupObject->pic];
		break;
	case OBJECTIVE_INVESTIGATE:
		pic.dx = pic.dy = 0;
		pic.picIndex = -1;
		break;
	default:
		assert(0 && "Unknown objective type");
		return;
	}
	if (pic.picIndex >= 0)
	{
		DrawTTPic(
			pos.x + o->Pos.x + o->Size.x / 2 + pic.dx,
			pos.y + o->Pos.y + o->Size.y / 2 + pic.dy,
			PicManagerGetOldPic(&gPicManager, pic.picIndex), table);
	}
}
static MissionObjective *GetMissionObjective(Mission *m, int index)
{
	return CArrayGet(&m->Objectives, index);
}
static char *MissionGetObjectiveRequired(UIObject *o, MissionIndexData *data)
{
	static char s[128];
	UNUSED(o);
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
static char *MissionGetObjectiveTotal(UIObject *o, MissionIndexData *data)
{
	static char s[128];
	UNUSED(o);
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
static char *MissionGetObjectiveFlags(UIObject *o, MissionIndexData *data)
{
	int flags;
	static char s[128];
	UNUSED(o);
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
static char *BrushGetTypeStr(EditorBrush *brush, int isMain)
{
	static char s[128];
	char *brushStr = "";
	switch (isMain ? brush->MainType : brush->SecondaryType)
	{
	case MAP_FLOOR:
		brushStr = "Floor";
		break;
	case MAP_WALL:
		brushStr = "Wall";
		break;
	case MAP_DOOR:
		brushStr = "Door";
		break;
	case MAP_ROOM:
		brushStr = "Room";
		break;
	default:
		assert(0 && "invalid brush type");
		return "";
	}
	sprintf(s, "Brush %d: %s", isMain ? 1 : 2, brushStr);
	return s;
}
static char *BrushGetMainTypeStr(UIObject *o, EditorBrush *brush)
{
	UNUSED(o);
	return BrushGetTypeStr(brush, 1);
}
static char *BrushGetSecondaryTypeStr(UIObject *o, EditorBrush *brush)
{
	UNUSED(o);
	return BrushGetTypeStr(brush, 0);
}
static char *BrushGetSizeStr(UIObject *o, EditorBrush *brush)
{
	static char s[128];
	UNUSED(o);
	sprintf(s, "Brush Size: %d", brush->BrushSize);
	return s;
}
static char *BrushGetBrushTypeStr(UIObject *o, EditorBrush *brush)
{
	static char s[128];
	UNUSED(o);
	sprintf(s, "Brush Type: %s", BrushTypeStr(brush->Type));
	return s;
}
typedef struct
{
	EditorBrush *Brush;
	int ItemIndex;
} IndexedEditorBrush;
static void DisplayMapItem(Vec2i pos, MapObject *mo);
static void DrawMapItem(
	UIObject *o, GraphicsDevice *g, Vec2i pos, IndexedEditorBrush *data)
{
	UNUSED(g);
	MapObject *mo = MapObjectGet(data->ItemIndex);
	DisplayMapItem(
		Vec2iAdd(Vec2iAdd(pos, o->Pos), Vec2iScaleDiv(o->Size, 2)), mo);
}
typedef struct
{
	IndexedEditorBrush Brush;
	CharacterStore *Store;
} EditorBrushAndCampaign;
static void DrawCharacter(
	UIObject *o, GraphicsDevice *g, Vec2i pos, EditorBrushAndCampaign *data)
{
	UNUSED(g);
	Character *c = CArrayGet(&data->Store->OtherChars, data->Brush.ItemIndex);
	DisplayCharacter(
		Vec2iAdd(Vec2iAdd(pos, o->Pos), Vec2iScaleDiv(o->Size, 2)),
		c, 0, 0);
}

static void DrawStyleArea(
	Vec2i pos,
	const char *name,
	GraphicsDevice *g,
	PicPaletted *pic,
	int index, int count,
	int isHighlighted)
{
	char buf[16];
	DrawTextStringMasked(name, g, pos, isHighlighted ? colorRed : colorWhite);
	pos.y += CDogsTextHeight();
	DrawTPic(pos.x, pos.y, pic);
	// Display style index and count, right aligned
	sprintf(buf, "%d/%d", index + 1, count);
	DrawTextStringMasked(
		buf,
		g,
		Vec2iNew(pos.x + 28 - TextGetStringWidth(buf), pos.y + 17),
		colorGray);
}
static void DisplayMapItemWithDensity(
	GraphicsDevice *g,
	Vec2i pos, MapObject *mo, int density, int isHighlighted)
{
	DisplayMapItem(pos, mo);
	if (isHighlighted)
	{
		DrawTextCharMasked(
			'\020', g, Vec2iAdd(pos, Vec2iNew(-8, -4)), colorWhite);
	}
	char s[10];
	sprintf(s, "%d", density);
	DrawTextString(s, g, Vec2iAdd(pos, Vec2iNew(-8, 5)));
}
static void DisplayMapItem(Vec2i pos, MapObject *mo)
{
	const TOffsetPic *pic = &cGeneralPics[mo->pic];
	DrawTPic(
		pos.x + pic->dx, pos.y + pic->dy,
		PicManagerGetOldPic(&gPicManager, pic->picIndex));
}
static void GetCharacterHeadPic(
	Character *c, TOffsetPic *pic, TranslationTable **t)
{
	int i = c->looks.face;
	*t = &c->table;
	pic->picIndex = cHeadPic[i][DIRECTION_DOWN][STATE_IDLE];
	pic->dx = cHeadOffset[i][DIRECTION_DOWN].dx;
	pic->dy = cHeadOffset[i][DIRECTION_DOWN].dy;
}

void DisplayFlag(
	GraphicsDevice *g, Vec2i pos, const char *s, int isOn, int isHighlighted)
{
	color_t labelMask = isHighlighted ? colorRed : colorWhite;
	pos = DrawTextStringMasked(s, g, pos, labelMask);
	pos = DrawTextCharMasked(':', g, pos, labelMask);
	if (isOn)
	{
		DrawTextStringMasked("On", g, pos, colorPurple);
	}
	else
	{
		DrawTextStringMasked("Off", g, pos, colorWhite);
	}
}


static void CampaignChangeSeed(CampaignOptions *c, int d)
{
	if (gEventHandlers.keyboard.modState & KMOD_SHIFT)
	{
		d *= 10;
	}
	if (d < 0 && c->seed < (unsigned)-d)
	{
		c->seed = 0;
	}
	else
	{
		c->seed += d;
	}
}
static void MissionChangeWidth(CampaignOptions *co, int d)
{
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
static void MissionChangeHeight(CampaignOptions *co, int d)
{
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
static void MissionChangeWallCount(CampaignOptions *co, int d)
{
	CampaignGetCurrentMission(co)->u.Classic.Walls =
		CLAMP(CampaignGetCurrentMission(co)->u.Classic.Walls + d, 0, 200);
}
static void MissionChangeWallLength(CampaignOptions *co, int d)
{
	CampaignGetCurrentMission(co)->u.Classic.WallLength =
		CLAMP(CampaignGetCurrentMission(co)->u.Classic.WallLength + d, 1, 100);
}
static void MissionChangeCorridorWidth(CampaignOptions *co, int d)
{
	CampaignGetCurrentMission(co)->u.Classic.CorridorWidth =
		CLAMP(CampaignGetCurrentMission(co)->u.Classic.CorridorWidth + d, 1, 5);
}
static void MissionChangeRoomCount(CampaignOptions *co, int d)
{
	CampaignGetCurrentMission(co)->u.Classic.Rooms.Count =
		CLAMP(CampaignGetCurrentMission(co)->u.Classic.Rooms.Count + d, 0, 100);
}
static void MissionChangeRoomMin(CampaignOptions *co, int d)
{
	CampaignGetCurrentMission(co)->u.Classic.Rooms.Min =
		CLAMP(CampaignGetCurrentMission(co)->u.Classic.Rooms.Min + d, 5, 50);
	CampaignGetCurrentMission(co)->u.Classic.Rooms.Max = MAX(
		CampaignGetCurrentMission(co)->u.Classic.Rooms.Min,
		CampaignGetCurrentMission(co)->u.Classic.Rooms.Max);
}
static void MissionChangeRoomMax(CampaignOptions *co, int d)
{
	CampaignGetCurrentMission(co)->u.Classic.Rooms.Max =
		CLAMP(CampaignGetCurrentMission(co)->u.Classic.Rooms.Max + d, 5, 50);
	CampaignGetCurrentMission(co)->u.Classic.Rooms.Min = MIN(
		CampaignGetCurrentMission(co)->u.Classic.Rooms.Min,
		CampaignGetCurrentMission(co)->u.Classic.Rooms.Max);
}
static void MissionChangeEdgeRooms(CampaignOptions *co, int d)
{
	UNUSED(d);
	CampaignGetCurrentMission(co)->u.Classic.Rooms.Edge =
		!CampaignGetCurrentMission(co)->u.Classic.Rooms.Edge;
}
static void MissionChangeRoomsOverlap(CampaignOptions *co, int d)
{
	UNUSED(d);
	CampaignGetCurrentMission(co)->u.Classic.Rooms.Overlap =
		!CampaignGetCurrentMission(co)->u.Classic.Rooms.Overlap;
}
static void MissionChangeRoomWallCount(CampaignOptions *co, int d)
{
	CampaignGetCurrentMission(co)->u.Classic.Rooms.Walls =
		CLAMP(CampaignGetCurrentMission(co)->u.Classic.Rooms.Walls + d, 0, 50);
}
static void MissionChangeRoomWallLen(CampaignOptions *co, int d)
{
	CampaignGetCurrentMission(co)->u.Classic.Rooms.WallLength =
		CLAMP(CampaignGetCurrentMission(co)->u.Classic.Rooms.WallLength + d, 1, 50);
}
static void MissionChangeRoomWallPad(CampaignOptions *co, int d)
{
	CampaignGetCurrentMission(co)->u.Classic.Rooms.WallPad =
		CLAMP(CampaignGetCurrentMission(co)->u.Classic.Rooms.WallPad + d, 2, 10);
}
static void MissionChangeSquareCount(CampaignOptions *co, int d)
{
	CampaignGetCurrentMission(co)->u.Classic.Squares =
		CLAMP(CampaignGetCurrentMission(co)->u.Classic.Squares + d, 0, 100);
}
static void MissionChangeDoorEnabled(CampaignOptions *co, int d)
{
	UNUSED(d);
	CampaignGetCurrentMission(co)->u.Classic.Doors.Enabled =
		!CampaignGetCurrentMission(co)->u.Classic.Doors.Enabled;
}
static void MissionChangeDoorMin(CampaignOptions *co, int d)
{
	CampaignGetCurrentMission(co)->u.Classic.Doors.Min =
		CLAMP(CampaignGetCurrentMission(co)->u.Classic.Doors.Min + d, 1, 6);
	CampaignGetCurrentMission(co)->u.Classic.Doors.Max = MAX(
		CampaignGetCurrentMission(co)->u.Classic.Doors.Min,
		CampaignGetCurrentMission(co)->u.Classic.Doors.Max);
}
static void MissionChangeDoorMax(CampaignOptions *co, int d)
{
	CampaignGetCurrentMission(co)->u.Classic.Doors.Max =
		CLAMP(CampaignGetCurrentMission(co)->u.Classic.Doors.Max + d, 1, 6);
	CampaignGetCurrentMission(co)->u.Classic.Doors.Min = MIN(
		CampaignGetCurrentMission(co)->u.Classic.Doors.Min,
		CampaignGetCurrentMission(co)->u.Classic.Doors.Max);
}
static void MissionChangePillarCount(CampaignOptions *co, int d)
{
	CampaignGetCurrentMission(co)->u.Classic.Pillars.Count =
		CLAMP(CampaignGetCurrentMission(co)->u.Classic.Pillars.Count + d, 0, 50);
}
static void MissionChangePillarMin(CampaignOptions *co, int d)
{
	CampaignGetCurrentMission(co)->u.Classic.Pillars.Min =
		CLAMP(CampaignGetCurrentMission(co)->u.Classic.Pillars.Min + d, 1, 50);
	CampaignGetCurrentMission(co)->u.Classic.Pillars.Max = MAX(
		CampaignGetCurrentMission(co)->u.Classic.Pillars.Min,
		CampaignGetCurrentMission(co)->u.Classic.Pillars.Max);
}
static void MissionChangePillarMax(CampaignOptions *co, int d)
{
	CampaignGetCurrentMission(co)->u.Classic.Pillars.Max =
		CLAMP(CampaignGetCurrentMission(co)->u.Classic.Pillars.Max + d, 1, 50);
	CampaignGetCurrentMission(co)->u.Classic.Pillars.Min = MIN(
		CampaignGetCurrentMission(co)->u.Classic.Pillars.Min,
		CampaignGetCurrentMission(co)->u.Classic.Pillars.Max);
}
static void MissionChangeDensity(CampaignOptions *co, int d)
{
	CampaignGetCurrentMission(co)->EnemyDensity = CLAMP(CampaignGetCurrentMission(co)->EnemyDensity + d, 0, 100);
}
static void MissionChangeType(CampaignOptions *co, int d)
{
	MapType type = CLAMP_OPPOSITE(
		CampaignGetCurrentMission(co)->Type + d,
		MAPTYPE_CLASSIC,
		MAPTYPE_STATIC);
	Map map;
	MissionOptionsTerminate(&gMission);
	CampaignAndMissionSetup(1, co, &gMission);
	memset(&map, 0, sizeof map);
	MapLoad(&map, &gMission);
	MissionConvertToType(gMission.missionData, &map, type);
}
static void MissionChangeWallStyle(CampaignOptions *co, int d)
{
	CampaignGetCurrentMission(co)->WallStyle = CLAMP_OPPOSITE(
		CampaignGetCurrentMission(co)->WallStyle + d, 0, WALL_STYLE_COUNT - 1);
}
static void MissionChangeFloorStyle(CampaignOptions *co, int d)
{
	CampaignGetCurrentMission(co)->FloorStyle = CLAMP_OPPOSITE(
		CampaignGetCurrentMission(co)->FloorStyle + d, 0, FLOOR_STYLE_COUNT - 1);
}
static void MissionChangeRoomStyle(CampaignOptions *co, int d)
{
	CampaignGetCurrentMission(co)->RoomStyle = CLAMP_OPPOSITE(
		CampaignGetCurrentMission(co)->RoomStyle + d, 0, ROOMFLOOR_COUNT - 1);
}
static void MissionChangeDoorStyle(CampaignOptions *co, int d)
{
	CampaignGetCurrentMission(co)->DoorStyle = CLAMP_OPPOSITE(
		CampaignGetCurrentMission(co)->DoorStyle + d, 0, GetEditorInfo().doorCount - 1);
}
static void MissionChangeKeyStyle(CampaignOptions *co, int d)
{
	CampaignGetCurrentMission(co)->KeyStyle = CLAMP_OPPOSITE(
		CampaignGetCurrentMission(co)->KeyStyle + d, 0, GetEditorInfo().keyCount - 1);
}
static void MissionChangeExitStyle(CampaignOptions *co, int d)
{
	CampaignGetCurrentMission(co)->ExitStyle = CLAMP_OPPOSITE(
		CampaignGetCurrentMission(co)->ExitStyle + d, 0, GetEditorInfo().exitCount - 1);
}
static void MissionChangeWallColor(CampaignOptions *co, int d)
{
	CampaignGetCurrentMission(co)->WallColor = CLAMP_OPPOSITE(
		CampaignGetCurrentMission(co)->WallColor + d, 0, GetEditorInfo().rangeCount - 1);
}
static void MissionChangeFloorColor(CampaignOptions *co, int d)
{
	CampaignGetCurrentMission(co)->FloorColor = CLAMP_OPPOSITE(
		CampaignGetCurrentMission(co)->FloorColor + d, 0, GetEditorInfo().rangeCount - 1);
}
static void MissionChangeRoomColor(CampaignOptions *co, int d)
{
	CampaignGetCurrentMission(co)->RoomColor = CLAMP_OPPOSITE(
		CampaignGetCurrentMission(co)->RoomColor + d, 0, GetEditorInfo().rangeCount - 1);
}
static void MissionChangeExtraColor(CampaignOptions *co, int d)
{
	CampaignGetCurrentMission(co)->AltColor = CLAMP_OPPOSITE(
		CampaignGetCurrentMission(co)->AltColor + d, 0, GetEditorInfo().rangeCount - 1);
}
static void MissionChangeEnemy(MissionIndexData *data, int d)
{
	int enemy = *(int *)CArrayGet(
		&CampaignGetCurrentMission(data->co)->Enemies, data->index);
	enemy = CLAMP_OPPOSITE(
		enemy + d, 0, (int)data->co->Setting.characters.OtherChars.size - 1);
	*(int *)CArrayGet(
		&CampaignGetCurrentMission(data->co)->Enemies, data->index) = enemy;
	data->co->Setting.characters.baddies[data->index] =
		CArrayGet(&data->co->Setting.characters.OtherChars, enemy);
}
static void MissionChangeSpecialChar(MissionIndexData *data, int d)
{
	int c = *(int *)CArrayGet(
		&CampaignGetCurrentMission(data->co)->SpecialChars, data->index);
	c = CLAMP_OPPOSITE(
		c + d, 0, (int)data->co->Setting.characters.OtherChars.size - 1);
	*(int *)CArrayGet(
		&CampaignGetCurrentMission(data->co)->SpecialChars, data->index) = c;
	data->co->Setting.characters.specials[data->index] =
		CArrayGet(&data->co->Setting.characters.OtherChars, c);
}
static void MissionChangeWeapon(MissionIndexData *data, int d)
{
	int hasWeapon;
	UNUSED(d);
	hasWeapon = CampaignGetCurrentMission(data->co)->Weapons[data->index];
	CampaignGetCurrentMission(data->co)->Weapons[data->index] = !hasWeapon;
}
static void MissionChangeMapItem(MissionIndexData *data, int d)
{
	if (gEventHandlers.keyboard.modState & KMOD_SHIFT)
	{
		int density = *(int *)CArrayGet(
			&CampaignGetCurrentMission(data->co)->ItemDensities, data->index);
		density = CLAMP(density + 5 * d, 0, 512);
		*(int *)CArrayGet(&CampaignGetCurrentMission(data->co)->ItemDensities, data->index) = density;
	}
	else
	{
		int i = *(int *)CArrayGet(
			&CampaignGetCurrentMission(data->co)->Items, data->index);
		i = CLAMP_OPPOSITE(i + d, 0, MapObjectGetCount() - 1);
		*(int *)CArrayGet(&CampaignGetCurrentMission(data->co)->Items, data->index) = i;
	}
}
static void MissionChangeObjectiveIndex(MissionIndexData *data, int d);
static void MissionChangeObjectiveType(MissionIndexData *data, int d)
{
	MissionObjective *mobj = GetMissionObjective(
		CampaignGetCurrentMission(data->co), data->index);
	mobj->Type = CLAMP_OPPOSITE(mobj->Type + d, 0, OBJECTIVE_INVESTIGATE);
	// Initialise the index of the objective
	MissionChangeObjectiveIndex(data, 0);
}
static void MissionChangeObjectiveIndex(MissionIndexData *data, int d)
{
	MissionObjective *mobj = GetMissionObjective(
		CampaignGetCurrentMission(data->co), data->index);
	int limit;
	switch (mobj->Type)
	{
	case OBJECTIVE_COLLECT:
		limit = GetEditorInfo().pickupCount - 1;
		break;
	case OBJECTIVE_DESTROY:
		limit = MapObjectGetCount() - 1;
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
static void MissionChangeObjectiveRequired(MissionIndexData *data, int d)
{
	MissionObjective *mobj = GetMissionObjective(
		CampaignGetCurrentMission(data->co), data->index);
	mobj->Required = CLAMP_OPPOSITE(
		mobj->Required + d, 0, MIN(100, mobj->Count));
}
static void MissionChangeObjectiveTotal(MissionIndexData *data, int d)
{
	MissionObjective *mobj = GetMissionObjective(
		CampaignGetCurrentMission(data->co), data->index);
	mobj->Count = CLAMP_OPPOSITE(mobj->Count + d, mobj->Required, 100);
}
static void MissionChangeObjectiveFlags(MissionIndexData *data, int d)
{
	MissionObjective *mobj = GetMissionObjective(
		CampaignGetCurrentMission(data->co), data->index);
	// Max is combination of all flags, i.e. largest flag doubled less one
	mobj->Flags = CLAMP_OPPOSITE(
		mobj->Flags + d, 0, OBJECTIVE_NOACCESS * 2 - 1);
}
static void BrushChangeType(EditorBrush *b, int d, int isMain)
{
	unsigned short brushType = isMain ? b->MainType : b->SecondaryType;
	if (brushType == 0 && d < 0)
	{
		brushType = MAP_ROOM;
	}
	else if (brushType == MAP_ROOM && d > 0)
	{
		brushType = MAP_FLOOR;
	}
	else
	{
		brushType = (unsigned short)(brushType + d);
	}
	if (isMain)
	{
		b->MainType = brushType;
	}
	else
	{
		b->SecondaryType = brushType;
	}
}
static void BrushChangeMainType(EditorBrush *b, int d)
{
	BrushChangeType(b, d, 1);
}
static void BrushChangeSecondaryType(EditorBrush *b, int d)
{
	BrushChangeType(b, d, 0);
}
static void BrushChangeSize(EditorBrush *b, int d)
{
	b->BrushSize = CLAMP(b->BrushSize + d, 1, 5);
}
static int BrushIsBrushTypePoint(EditorBrush *b)
{
	return b->Type == BRUSHTYPE_POINT;
}
static int BrushIsBrushTypeLine(EditorBrush *b)
{
	return b->Type == BRUSHTYPE_LINE;
}
static int BrushIsBrushTypeBox(EditorBrush *b)
{
	return b->Type == BRUSHTYPE_BOX;
}
static int BrushIsBrushTypeBoxFilled(EditorBrush *b)
{
	return b->Type == BRUSHTYPE_BOX_FILLED;
}
static int BrushIsBrushTypeRoom(EditorBrush *b)
{
	return b->Type == BRUSHTYPE_ROOM;
}
static int BrushIsBrushTypeSelect(EditorBrush *b)
{
	return b->Type == BRUSHTYPE_SELECT;
}
static int BrushIsBrushTypeAddItem(EditorBrush *b)
{
	return
		b->Type == BRUSHTYPE_SET_PLAYER_START ||
		b->Type == BRUSHTYPE_ADD_ITEM ||
		b->Type == BRUSHTYPE_ADD_CHARACTER;
}
static void BrushSetBrushTypePoint(EditorBrush *b, int d)
{
	UNUSED(d);
	b->Type = BRUSHTYPE_POINT;
}
static void BrushSetBrushTypeLine(EditorBrush *b, int d)
{
	UNUSED(d);
	b->Type = BRUSHTYPE_LINE;
}
static void BrushSetBrushTypeBox(EditorBrush *b, int d)
{
	UNUSED(d);
	b->Type = BRUSHTYPE_BOX;
}
static void BrushSetBrushTypeBoxFilled(EditorBrush *b, int d)
{
	UNUSED(d);
	b->Type = BRUSHTYPE_BOX_FILLED;
}
static void BrushSetBrushTypeRoom(EditorBrush *b, int d)
{
	UNUSED(d);
	b->Type = BRUSHTYPE_ROOM;
}
static void BrushSetBrushTypeSelect(EditorBrush *b, int d)
{
	UNUSED(d);
	b->Type = BRUSHTYPE_SELECT;
}
static void BrushSetBrushTypeSetPlayerStart(EditorBrush *b, int d)
{
	UNUSED(d);
	b->Type = BRUSHTYPE_SET_PLAYER_START;
}
static void BrushSetBrushTypeAddMapItem(IndexedEditorBrush *b, int d)
{
	UNUSED(d);
	b->Brush->Type = BRUSHTYPE_ADD_ITEM;
	b->Brush->ItemIndex = b->ItemIndex;
}
static void BrushSetBrushTypeAddCharacter(IndexedEditorBrush *b, int d)
{
	UNUSED(d);
	b->Brush->Type = BRUSHTYPE_ADD_CHARACTER;
	b->Brush->ItemIndex = b->ItemIndex;
}
static void ActivateBrush(UIObject *o, EditorBrush *b)
{
	UNUSED(o);
	b->IsActive = 1;
}
static void DeactivateBrush(EditorBrush *b)
{
	b->IsActive = 0;
}


static UIObject *CreateCampaignObjs(CampaignOptions *co);
static UIObject *CreateMissionObjs(CampaignOptions *co);
static UIObject *CreateClassicMapObjs(Vec2i pos, CampaignOptions *co);
static UIObject *CreateStaticMapObjs(
	Vec2i pos, CampaignOptions *co, EditorBrush *brush);
static UIObject *CreateWeaponObjs(CampaignOptions *co);
static UIObject *CreateMapItemObjs(CampaignOptions *co);
static UIObject *CreateCharacterObjs(CampaignOptions *co);
static UIObject *CreateSpecialCharacterObjs(CampaignOptions *co);
static UIObject *CreateObjectiveObjs(
	Vec2i pos, CampaignOptions *co, int index);

UIObject *CreateMainObjs(CampaignOptions *co, EditorBrush *brush)
{
	int th = CDogsTextHeight();
	UIObject *cc;
	UIObject *c;
	UIObject *o;
	UIObject *o2;
	UIObject *oc;
	int i;
	Vec2i pos;
	Vec2i objectivesPos;
	cc = UIObjectCreate(UITYPE_NONE, 0, Vec2iZero(), Vec2iZero());

	// Titles

	pos.y = 5;

	o = UIObjectCreate(
		UITYPE_TEXTBOX, YC_CAMPAIGNTITLE,
		Vec2iNew(25, pos.y), Vec2iNew(240, th));
	o->u.Textbox.TextLinkFunc = CampaignGetTitle;
	o->Data = co;
	CSTRDUP(o->u.Textbox.Hint, "(Campaign title)");
	o->Flags = UI_SELECT_ONLY_FIRST;
	UIObjectAddChild(o, CreateCampaignObjs(co));
	UIObjectAddChild(cc, o);

	o = UIObjectCreate(
		UITYPE_NONE, YC_MISSIONINDEX, Vec2iNew(270, pos.y), Vec2iNew(49, th));
	UIObjectAddChild(cc, o);

	pos.y = 2 * th;

	// Mission-only controls
	// Only visible if the current mission is valid
	c = UIObjectCreate(UITYPE_CUSTOM, 0, Vec2iZero(), Vec2iZero());
	c->u.CustomDrawFunc = CheckMission;
	c->Data = co;
	UIObjectAddChild(cc, c);

	o = UIObjectCreate(
		UITYPE_TEXTBOX, YC_MISSIONTITLE,
		Vec2iNew(25, pos.y), Vec2iNew(175, th));
	o->Id2 = XC_MISSIONTITLE;
	o->u.Textbox.TextLinkFunc = MissionGetTitle;
	o->Data = co;
	CSTRDUP(o->u.Textbox.Hint, "(Mission title)");
	UIObjectAddChild(o, CreateMissionObjs(co));
	UIObjectAddChild(c, o);

	// mission properties
	// size, walls, rooms etc.

	pos.y = 10 + 2 * th;

	o = UIObjectCreate(
		UITYPE_LABEL, 0, Vec2iZero(), Vec2iNew(35, th));
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

	pos.y += th * 6;

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
	UIObjectAddChild(c, o2);
	pos.x += 30;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_ROOM;
	o2->u.CustomDrawFunc = MissionDrawRoomStyle;
	o2->Data = co;
	o2->ChangeFunc = MissionChangeRoomStyle;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.x += 30;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_DOORS;
	o2->u.CustomDrawFunc = MissionDrawDoorStyle;
	o2->Data = co;
	o2->ChangeFunc = MissionChangeDoorStyle;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.x += 30;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_KEYS;
	o2->u.CustomDrawFunc = MissionDrawKeyStyle;
	o2->Data = co;
	o2->ChangeFunc = MissionChangeKeyStyle;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.x += 30;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_EXIT;
	o2->u.CustomDrawFunc = MissionDrawExitStyle;
	o2->Data = co;
	o2->ChangeFunc = MissionChangeExitStyle;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);

	// colours

	pos.x = 200;

	UIObjectDestroy(o);
	o = UIObjectCreate(
		UITYPE_LABEL, YC_MISSIONLOOKS, Vec2iZero(), Vec2iNew(100, th));
	o->ChangesData = 1;

	o2 = UIObjectCopy(o);
	o2->Id2 = XC_COLOR1;
	o2->u.LabelFunc = MissionGetWallColorStr;
	o2->Data = co;
	o2->ChangeFunc = MissionChangeWallColor;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.y += th;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_COLOR2;
	o2->u.LabelFunc = MissionGetFloorColorStr;
	o2->Data = co;
	o2->ChangeFunc = MissionChangeFloorColor;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.y += th;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_COLOR3;
	o2->u.LabelFunc = MissionGeRoomColorStr;
	o2->Data = co;
	o2->ChangeFunc = MissionChangeRoomColor;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.y += th;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_COLOR4;
	o2->u.LabelFunc = MissionGeExtraColorStr;
	o2->Data = co;
	o2->ChangeFunc = MissionChangeExtraColor;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);

	// mission data

	pos.x = 20;
	pos.y += th;

	UIObjectDestroy(o);
	o = UIObjectCreate(UITYPE_LABEL, 0, Vec2iZero(), Vec2iNew(189, th));
	o->Flags = UI_SELECT_ONLY;

	o2 = UIObjectCopy(o);
	o2->Label = "Mission description";
	o2->Id = YC_MISSIONDESC;
	o2->Pos = pos;
	o2->Size = TextGetSize(o2->Label);
	oc = UIObjectCreate(
		UITYPE_TEXTBOX, YC_MISSIONDESC,
		Vec2iNew(25, 170), Vec2iNew(295, 5 * th));
	oc->Flags = UI_ENABLED_WHEN_PARENT_HIGHLIGHTED_ONLY;
	oc->u.Textbox.TextLinkFunc = MissionGetDescription;
	oc->Data = co;
	CSTRDUP(oc->u.Textbox.Hint, "(Mission description)");
	UIObjectAddChild(o2, oc);
	UIObjectAddChild(c, o2);

	pos.y += th;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = MissionGetCharacterCountStr;
	o2->Data = co;
	o2->Id = YC_CHARACTERS;
	o2->Pos = pos;
	CSTRDUP(o2->Tooltip, "Use Insert, Delete and PageUp/PageDown");
	UIObjectAddChild(o2, CreateCharacterObjs(co));
	UIObjectAddChild(c, o2);
	pos.y += th;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = MissionGetSpecialCountStr;
	o2->Data = co;
	o2->Id = YC_SPECIALS;
	o2->Pos = pos;
	CSTRDUP(o2->Tooltip, "Use Insert, Delete and PageUp/PageDown");
	UIObjectAddChild(o2, CreateSpecialCharacterObjs(co));
	UIObjectAddChild(c, o2);
	pos.y += th;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = GetWeaponCountStr;
	o2->Data = NULL;
	o2->Id = YC_WEAPONS;
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
		"Use Insert, Delete and PageUp/PageDown\n"
		"Shift+click to change amounts");
	UIObjectAddChild(o2, CreateMapItemObjs(co));
	UIObjectAddChild(c, o2);

	// objectives
	pos.y += 2;
	objectivesPos = Vec2iNew(0, 8 * th);
	UIObjectDestroy(o);
	o = UIObjectCreate(UITYPE_TEXTBOX, 0, Vec2iZero(), Vec2iNew(189, th));
	o->Flags = UI_SELECT_ONLY;

	for (i = 0; i < OBJECTIVE_MAX_OLD; i++)
	{
		pos.y += th;
		o2 = UIObjectCopy(o);
		o2->Id = YC_OBJECTIVES + i;
		o2->Type = UITYPE_TEXTBOX;
		o2->u.Textbox.TextLinkFunc = MissionGetObjectiveDescription;
		o2->Data = co;
		CSTRDUP(o2->u.Textbox.Hint, "(Objective description)");
		o2->Pos = pos;
		CSTRDUP(o2->Tooltip, "insert/delete: add/remove objective");
		UIObjectAddChild(o2, CreateObjectiveObjs(objectivesPos, co, i));
		UIObjectAddChild(c, o2);
	}

	UIObjectDestroy(o);
	return cc;
}
static UIObject *CreateCampaignObjs(CampaignOptions *co)
{
	int th = CDogsTextHeight();
	UIObject *c;
	UIObject *o;
	UIObject *o2;
	int x;
	int y;
	c = UIObjectCreate(UITYPE_NONE, 0, Vec2iZero(), Vec2iZero());
	c->Flags = UI_ENABLED_WHEN_PARENT_HIGHLIGHTED_ONLY;

	x = 25;
	y = 170;

	o = UIObjectCreate(
		UITYPE_TEXTBOX, YC_CAMPAIGNTITLE, Vec2iZero(), Vec2iZero());
	o->Flags = UI_SELECT_ONLY;

	o2 = UIObjectCopy(o);
	o2->u.Textbox.TextLinkFunc = CampaignGetAuthor;
	o2->Data = co;
	CSTRDUP(o2->u.Textbox.Hint, "(Campaign author)");
	o2->Id2 = XC_AUTHOR;
	o2->Pos = Vec2iNew(x, y);
	o2->Size = Vec2iNew(295, th);
	UIObjectAddChild(c, o2);

	y += th;
	o2 = UIObjectCopy(o);
	o2->u.Textbox.TextLinkFunc = CampaignGetDescription;
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
	int th = CDogsTextHeight();
	UIObject *c;
	UIObject *o;
	c = UIObjectCreate(UITYPE_NONE, 0, Vec2iZero(), Vec2iZero());
	c->Flags = UI_ENABLED_WHEN_PARENT_HIGHLIGHTED_ONLY;

	o = UIObjectCreate(
		UITYPE_TEXTBOX, YC_MISSIONTITLE, Vec2iNew(20, 170), Vec2iNew(319, th));
	o->u.Textbox.TextLinkFunc = MissionGetSong;
	o->Data = co;
	CSTRDUP(o->u.Textbox.Hint, "(Mission song)");
	o->Id2 = XC_MUSICFILE;
	o->Flags = UI_SELECT_ONLY;
	UIObjectAddChild(c, o);

	return c;
}
static UIObject *CreateClassicMapObjs(Vec2i pos, CampaignOptions *co)
{
	int th = CDogsTextHeight();
	UIObject *c = UIObjectCreate(UITYPE_CUSTOM, 0, Vec2iZero(), Vec2iZero());
	UIObject *o = UIObjectCreate(
		UITYPE_LABEL, 0, Vec2iZero(), Vec2iNew(50, th));
	int x = pos.x;
	UIObject *o2;
	o->ChangesData = 1;
	// Use a custom UIObject to check whether the map type matches,
	// and set visibility
	c->u.CustomDrawFunc = MissionCheckTypeClassic;
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
static UIObject *CreateAddItemObjs(
	Vec2i pos, EditorBrush *brush, CharacterStore *store);
static UIObject *CreateStaticMapObjs(
	Vec2i pos, CampaignOptions *co, EditorBrush *brush)
{
	int x = pos.x;
	int th = CDogsTextHeight();
	UIObject *c = UIObjectCreate(UITYPE_CUSTOM, 0, Vec2iZero(), Vec2iZero());
	UIObject *o2;
	// Use a custom UIObject to check whether the map type matches,
	// and set visibility
	c->u.CustomDrawFunc = MissionCheckTypeStatic;
	c->Data = co;

	UIObject *o = UIObjectCreate(UITYPE_BUTTON, 0, Vec2iZero(), Vec2iZero());
	o->Data = brush;
	o->OnFocusFunc = ActivateBrush;
	o->OnUnfocusFunc = DeactivateBrush;
	o2 = UIObjectCopy(o);
	UIButtonSetPic(o2, PicManagerGetPic(&gPicManager, "pencil"));
	o2->u.Button.IsDownFunc = BrushIsBrushTypePoint;
	o2->ChangeFunc = BrushSetBrushTypePoint;
	CSTRDUP(o2->Tooltip, "Point");
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.x += o2->Size.x;
	o2 = UIObjectCopy(o);
	UIButtonSetPic(o2, PicManagerGetPic(&gPicManager, "line"));
	o2->u.Button.IsDownFunc = BrushIsBrushTypeLine;
	o2->ChangeFunc = BrushSetBrushTypeLine;
	CSTRDUP(o2->Tooltip, "Line");
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.x += o2->Size.x;
	o2 = UIObjectCopy(o);
	UIButtonSetPic(o2, PicManagerGetPic(&gPicManager, "box"));
	o2->u.Button.IsDownFunc = BrushIsBrushTypeBox;
	o2->ChangeFunc = BrushSetBrushTypeBox;
	CSTRDUP(o2->Tooltip, "Box");
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.x += o2->Size.x;
	o2 = UIObjectCopy(o);
	UIButtonSetPic(o2, PicManagerGetPic(&gPicManager, "box_filled"));
	o2->u.Button.IsDownFunc = BrushIsBrushTypeBoxFilled;
	o2->ChangeFunc = BrushSetBrushTypeBoxFilled;
	CSTRDUP(o2->Tooltip, "Box filled");
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.x += o2->Size.x;
	o2 = UIObjectCopy(o);
	UIButtonSetPic(o2, PicManagerGetPic(&gPicManager, "room"));
	o2->u.Button.IsDownFunc = BrushIsBrushTypeRoom;
	o2->ChangeFunc = BrushSetBrushTypeRoom;
	CSTRDUP(o2->Tooltip, "Room");
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.x += o2->Size.x;
	o2 = UIObjectCopy(o);
	UIButtonSetPic(o2, PicManagerGetPic(&gPicManager, "select"));
	o2->u.Button.IsDownFunc = BrushIsBrushTypeSelect;
	o2->ChangeFunc = BrushSetBrushTypeSelect;
	CSTRDUP(o2->Tooltip, "Select and move");
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.x += o2->Size.x;
	o2 = UIObjectCopy(o);
	UIButtonSetPic(o2, PicManagerGetPic(&gPicManager, "add"));
	o2->u.Button.IsDownFunc = BrushIsBrushTypeAddItem;
	CSTRDUP(o2->Tooltip, "Add item");
	o2->Pos = pos;
	UIObjectAddChild(o2,
		CreateAddItemObjs(o2->Size, brush, &co->Setting.characters));
	UIObjectAddChild(c, o2);

	UIObjectDestroy(o);
	o = UIObjectCreate(
		UITYPE_LABEL, 0, Vec2iZero(), Vec2iNew(60, th));
	pos.x = x;
	pos.y += o2->Size.y;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = BrushGetMainTypeStr;
	o2->Data = brush;
	o2->ChangeFunc = BrushChangeMainType;
	o2->OnFocusFunc = ActivateBrush;
	o2->OnUnfocusFunc = DeactivateBrush;
	CSTRDUP(o2->Tooltip, "Left click to paint the map with this tile type");
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.x += o2->Size.x;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = BrushGetSecondaryTypeStr;
	o2->Data = brush;
	o2->ChangeFunc = BrushChangeSecondaryType;
	o2->OnFocusFunc = ActivateBrush;
	o2->OnUnfocusFunc = DeactivateBrush;
	CSTRDUP(o2->Tooltip, "Right click to paint the map with this tile type");
	o2->Pos = pos;
	UIObjectAddChild(c, o2);

	pos.x = x;
	pos.y += th;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = BrushGetSizeStr;
	o2->Data = brush;
	o2->ChangeFunc = BrushChangeSize;
	o2->OnFocusFunc = ActivateBrush;
	o2->OnUnfocusFunc = DeactivateBrush;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);

	UIObjectDestroy(o);
	return c;
}
static UIObject *CreateWeaponObjs(CampaignOptions *co)
{
	int th = CDogsTextHeight();
	UIObject *c;
	UIObject *o;
	UIObject *o2;
	int i;
	c = UIObjectCreate(UITYPE_NONE, 0, Vec2iZero(), Vec2iZero());
	c->Flags = UI_ENABLED_WHEN_PARENT_HIGHLIGHTED_ONLY;

	o = UIObjectCreate(UITYPE_CUSTOM, 0, Vec2iZero(), Vec2iNew(80, th));
	o->u.CustomDrawFunc = MissionDrawWeaponStatus;
	o->ChangeFunc = MissionChangeWeapon;
	o->Flags = UI_LEAVE_YC;
	o->ChangesData = 1;
	for (i = 0; i < GUN_COUNT; i++)
	{
		int x = 10 + i / 4 * 90;
		int y = 170 + (i % 4) * th;
		o2 = UIObjectCopy(o);
		o2->Id2 = i;
		CMALLOC(o2->Data, sizeof(MissionIndexData));
		o2->IsDynamicData = 1;
		((MissionIndexData *)o2->Data)->co = co;
		((MissionIndexData *)o2->Data)->index = i;
		o2->Pos = Vec2iNew(x, y);
		UIObjectAddChild(c, o2);
	}

	UIObjectDestroy(o);
	return c;
}
static UIObject *CreateMapItemObjs(CampaignOptions *co)
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
		o2->Pos = Vec2iNew(x, 170);
		UIObjectAddChild(c, o2);
	}

	UIObjectDestroy(o);
	return c;
}
static UIObject *CreateObjectiveObjs(Vec2i pos, CampaignOptions *co, int index)
{
	int th = CDogsTextHeight();
	UIObject *c;
	UIObject *o;
	UIObject *o2;
	c = UIObjectCreate(UITYPE_NONE, 0, Vec2iZero(), Vec2iZero());
	c->Flags = UI_ENABLED_WHEN_PARENT_HIGHLIGHTED_ONLY;

	o = UIObjectCreate(UITYPE_NONE, 0, Vec2iZero(), Vec2iZero());
	o->Flags = UI_LEAVE_YC;
	o->ChangesData = 1;

	o2 = UIObjectCopy(o);
	o2->Id2 = XC_TYPE;
	o2->Type = UITYPE_LABEL;
	o2->u.LabelFunc = MissionGetObjectiveStr;
	o2->ChangeFunc = MissionChangeObjectiveType;
	CMALLOC(o2->Data, sizeof(MissionIndexData));
	o2->IsDynamicData = 1;
	((MissionIndexData *)o2->Data)->co = co;
	((MissionIndexData *)o2->Data)->index = index;
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
	((MissionIndexData *)o2->Data)->index = index;
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
	((MissionIndexData *)o2->Data)->index = index;
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
	((MissionIndexData *)o2->Data)->index = index;
	o2->Pos = pos;
	o2->Size = Vec2iNew(35, th);
	UIObjectAddChild(c, o2);
	pos.x += 40;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_FLAGS;
	o2->Type = UITYPE_LABEL;
	o2->u.LabelFunc = MissionGetObjectiveFlags;
	o2->ChangeFunc = MissionChangeObjectiveFlags;
	CMALLOC(o2->Data, sizeof(MissionIndexData));
	o2->IsDynamicData = 1;
	((MissionIndexData *)o2->Data)->co = co;
	((MissionIndexData *)o2->Data)->index = index;
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
static UIObject *CreateCharacterObjs(CampaignOptions *co)
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
		o2->Pos = Vec2iNew(x, 170);
		UIObjectAddChild(c, o2);
	}

	UIObjectDestroy(o);
	return c;
}
static UIObject *CreateSpecialCharacterObjs(CampaignOptions *co)
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
		o2->Pos = Vec2iNew(x, 170);
		UIObjectAddChild(c, o2);
	}

	UIObjectDestroy(o);
	return c;
}

UIObject *CreateCharEditorObjs(void)
{
	int th = CDogsTextHeight();
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

static UIObject *CreateAddMapItemObjs(Vec2i pos, EditorBrush *brush);
static UIObject *CreateAddCharacterObjs(
	Vec2i pos, EditorBrush *brush, CharacterStore *store);
static UIObject *CreateAddItemObjs(
	Vec2i pos, EditorBrush *brush, CharacterStore *store)
{
	int th = CDogsTextHeight();
	UIObject *o2;
	UIObject *c = UIObjectCreate(UITYPE_CONTEXT_MENU, 0, pos, Vec2iZero());

	UIObject *o = UIObjectCreate(
		UITYPE_LABEL, 0, Vec2iZero(), Vec2iNew(50, th));
	o->Data = brush;

	pos = Vec2iZero();
	o2 = UIObjectCopy(o);
	o2->Label = "Player start";
	o2->ChangeFunc = BrushSetBrushTypeSetPlayerStart;
	o2->Pos = pos;
	CSTRDUP(o2->Tooltip, "Location where players start");
	UIObjectAddChild(c, o2);
	pos.y += th;
	o2 = UIObjectCopy(o);
	o2->Label = "Map item";
	o2->Pos = pos;
	UIObjectAddChild(o2, CreateAddMapItemObjs(o2->Size, brush));
	UIObjectAddChild(c, o2);
	pos.y += th;
	o2 = UIObjectCopy(o);
	o2->Label = "Character";
	o2->Pos = pos;
	UIObjectAddChild(o2, CreateAddCharacterObjs(o2->Size, brush, store));
	UIObjectAddChild(c, o2);

	UIObjectDestroy(o);
	return c;
}
static UIObject *CreateAddMapItemObjs(Vec2i pos, EditorBrush *brush)
{
	UIObject *o2;
	UIObject *c = UIObjectCreate(UITYPE_CONTEXT_MENU, 0, pos, Vec2iZero());

	UIObject *o = UIObjectCreate(
		UITYPE_CUSTOM, 0,
		Vec2iZero(), Vec2iNew(TILE_WIDTH + 4, TILE_HEIGHT * 2 + 4));
	o->ChangeFunc = BrushSetBrushTypeAddMapItem;
	o->u.CustomDrawFunc = DrawMapItem;
	pos = Vec2iZero();
	int width = 8;
	for (int i = 0; i < MapObjectGetCount(); i++)
	{
		o2 = UIObjectCopy(o);
		o2->IsDynamicData = 1;
		CMALLOC(o2->Data, sizeof(IndexedEditorBrush));
		((IndexedEditorBrush *)o2->Data)->Brush = brush;
		((IndexedEditorBrush *)o2->Data)->ItemIndex = i;
		o2->Pos = pos;
		UIObjectAddChild(c, o2);
		pos.x += o->Size.x;
		if (((i + 1) % width) == 0)
		{
			pos.x = 0;
			pos.y += o->Size.y;
		}
	}

	UIObjectDestroy(o);
	return c;
}
static void CreateAddCharacterSubObjs(
	UIObject *c, EditorBrushAndCampaign *data);
static UIObject *CreateAddCharacterObjs(
	Vec2i pos, EditorBrush *brush, CharacterStore *store)
{
	UIObject *c = UIObjectCreate(UITYPE_CONTEXT_MENU, 0, pos, Vec2iZero());
	// Need to update UI objects dynamically as new characters can be
	// added and removed
	c->OnFocusFunc = CreateAddCharacterSubObjs;
	c->IsDynamicData = 1;
	CMALLOC(c->Data, sizeof(EditorBrushAndCampaign));
	((EditorBrushAndCampaign *)c->Data)->Brush.Brush = brush;
	((EditorBrushAndCampaign *)c->Data)->Store = store;

	return c;
}
static void CreateAddCharacterSubObjs(
	UIObject *c, EditorBrushAndCampaign *data)
{
	if (c->Children.size == data->Store->OtherChars.size)
	{
		return;
	}
	// Recreate the child UI objects
	UIObject **objs = c->Children.data;
	for (int i = 0; i < (int)c->Children.size; i++, objs++)
	{
		UIObjectDestroy(*objs);
	}
	CArrayTerminate(&c->Children);
	CArrayInit(&c->Children, sizeof c);

	UIObject *o = UIObjectCreate(
		UITYPE_CUSTOM, 0,
		Vec2iZero(), Vec2iNew(TILE_WIDTH + 4, TILE_HEIGHT * 2 + 4));
	o->ChangeFunc = BrushSetBrushTypeAddCharacter;
	o->u.CustomDrawFunc = DrawCharacter;
	Vec2i pos = Vec2iZero();
	int width = 8;
	for (int i = 0; i < (int)data->Store->OtherChars.size; i++)
	{
		UIObject *o2 = UIObjectCopy(o);
		o2->IsDynamicData = 1;
		CMALLOC(o2->Data, sizeof(EditorBrushAndCampaign));
		((EditorBrushAndCampaign *)o2->Data)->Brush.Brush = data->Brush.Brush;
		((EditorBrushAndCampaign *)o2->Data)->Store = data->Store;
		((EditorBrushAndCampaign *)o2->Data)->Brush.ItemIndex = i;
		o2->Pos = pos;
		UIObjectAddChild(c, o2);
		pos.x += o->Size.x;
		if (((i + 1) % width) == 0)
		{
			pos.x = 0;
			pos.y += o->Size.y;
		}
	}
	UIObjectDestroy(o);
}
