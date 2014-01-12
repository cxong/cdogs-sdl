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
#include <cdogs/pic_manager.h>
#include <cdogs/text.h>


static void DrawStyleArea(
	Vec2i pos,
	const char *name,
	GraphicsDevice *device,
	PicPaletted *pic,
	int index, int count,
	int isHighlighted);

static char *CStr(UIObject *o, char *s)
{
	UNUSED(o);
	return s;
}
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
	UIObject *o, GraphicsDevice *g, Mission **missionPtr)
{
	UNUSED(g);
	if (!*missionPtr)
	{
		o->IsVisible = 0;
		// Need to unhighlight to prevent children being drawn
		UIObjectUnhighlight(o);
		return;
	}
	o->IsVisible = 1;
}
static char *MissionGetTitle(UIObject *o, Mission **missionPtr)
{
	UNUSED(o);
	if (!*missionPtr)
	{
		o->IsVisible = 0;
		return NULL;
	}
	o->IsVisible = 1;
	return (*missionPtr)->Title;
}
static char *MissionGetDescription(UIObject *o, Mission **missionPtr)
{
	UNUSED(o);
	if (!*missionPtr)
	{
		o->IsVisible = 0;
		return NULL;
	}
	o->IsVisible = 1;
	return (*missionPtr)->Description;
}
static char *MissionGetSong(UIObject *o, Mission **missionPtr)
{
	UNUSED(o);
	if (!*missionPtr)
	{
		o->IsVisible = 0;
		return NULL;
	}
	o->IsVisible = 1;
	return (*missionPtr)->Song;
}
static char *MissionGetWidthStr(UIObject *o, Mission **missionPtr)
{
	static char s[128];
	UNUSED(o);
	if (!*missionPtr) return NULL;
	sprintf(s, "Width: %d", (*missionPtr)->Size.x);
	return s;
}
static char *MissionGetHeightStr(UIObject *o, Mission **missionPtr)
{
	static char s[128];
	UNUSED(o);
	if (!*missionPtr) return NULL;
	sprintf(s, "Height: %d", (*missionPtr)->Size.y);
	return s;
}
static char *MissionGetWallCountStr(UIObject *o, Mission **missionPtr)
{
	static char s[128];
	UNUSED(o);
	if (!*missionPtr) return NULL;
	sprintf(s, "Walls: %d", (*missionPtr)->u.Classic.Walls);
	return s;
}
static char *MissionGetWallLengthStr(UIObject *o, Mission **missionPtr)
{
	static char s[128];
	UNUSED(o);
	if (!*missionPtr) return NULL;
	sprintf(s, "Len: %d", (*missionPtr)->u.Classic.WallLength);
	return s;
}
static char *MissionGetCorridorWidthStr(UIObject *o, Mission **missionPtr)
{
	static char s[128];
	UNUSED(o);
	if (!*missionPtr) return NULL;
	sprintf(s, "CorridorWidth: %d", (*missionPtr)->u.Classic.CorridorWidth);
	return s;
}
static char *MissionGetRoomCountStr(UIObject *o, Mission **missionPtr)
{
	static char s[128];
	UNUSED(o);
	if (!*missionPtr) return NULL;
	sprintf(s, "Rooms: %d", (*missionPtr)->u.Classic.Rooms);
	return s;
}
static char *MissionGetRoomMinStr(UIObject *o, Mission **missionPtr)
{
	static char s[128];
	UNUSED(o);
	if (!*missionPtr) return NULL;
	sprintf(s, "RoomMin: %d", (*missionPtr)->u.Classic.Rooms.Min);
	return s;
}
static char *MissionGetRoomMaxStr(UIObject *o, Mission **missionPtr)
{
	static char s[128];
	UNUSED(o);
	if (!*missionPtr) return NULL;
	sprintf(s, "RoomMax: %d", (*missionPtr)->u.Classic.Rooms.Max);
	return s;
}
static void MissionDrawEdgeRooms(
	UIObject *o, GraphicsDevice *g, Mission **missionPtr)
{
	static char s[128];
	UNUSED(o);
	if (!*missionPtr) return;
	DisplayFlag(
		g, o->Pos, "Edge rooms", (*missionPtr)->u.Classic.Rooms.Edge,
		UIObjectIsHighlighted(o));
}
static void MissionDrawRoomsOverlap(
	UIObject *o, GraphicsDevice *g, Mission **missionPtr)
{
	static char s[128];
	UNUSED(o);
	if (!*missionPtr) return;
	DisplayFlag(
		g, o->Pos, "Room overlap", (*missionPtr)->u.Classic.Rooms.Overlap,
		UIObjectIsHighlighted(o));
}
static char *MissionGetRoomWallCountStr(UIObject *o, Mission **missionPtr)
{
	static char s[128];
	UNUSED(o);
	if (!*missionPtr) return NULL;
	sprintf(s, "RoomWalls: %d", (*missionPtr)->u.Classic.Rooms.Walls);
	return s;
}
static char *MissionGetRoomWallLenStr(UIObject *o, Mission **missionPtr)
{
	static char s[128];
	UNUSED(o);
	if (!*missionPtr) return NULL;
	sprintf(s, "RoomWallLen: %d", (*missionPtr)->u.Classic.Rooms.WallLength);
	return s;
}
static char *MissionGetRoomWallPadStr(UIObject *o, Mission **missionPtr)
{
	static char s[128];
	UNUSED(o);
	if (!*missionPtr) return NULL;
	sprintf(s, "RoomWallPad: %d", (*missionPtr)->u.Classic.Rooms.WallPad);
	return s;
}
static char *MissionGetSquareCountStr(UIObject *o, Mission **missionPtr)
{
	static char s[128];
	UNUSED(o);
	if (!*missionPtr) return NULL;
	sprintf(s, "Sqr: %d", (*missionPtr)->u.Classic.Squares);
	return s;
}
static void MissionDrawDoorEnabled(
	UIObject *o, GraphicsDevice *g, Mission **missionPtr)
{
	static char s[128];
	UNUSED(o);
	if (!*missionPtr) return;
	DisplayFlag(
		g, o->Pos, "Doors", (*missionPtr)->u.Classic.Doors.Enabled,
		UIObjectIsHighlighted(o));
}
static char *MissionGetDoorMinStr(UIObject *o, Mission **missionPtr)
{
	static char s[128];
	UNUSED(o);
	if (!*missionPtr) return NULL;
	sprintf(s, "DoorMin: %d", (*missionPtr)->u.Classic.Doors.Min);
	return s;
}
static char *MissionGetDoorMaxStr(UIObject *o, Mission **missionPtr)
{
	static char s[128];
	UNUSED(o);
	if (!*missionPtr) return NULL;
	sprintf(s, "DoorMax: %d", (*missionPtr)->u.Classic.Doors.Max);
	return s;
}
static char *MissionGetPillarCountStr(UIObject *o, Mission **missionPtr)
{
	static char s[128];
	UNUSED(o);
	if (!*missionPtr) return NULL;
	sprintf(s, "Pillars: %d", (*missionPtr)->u.Classic.Pillars.Count);
	return s;
}
static char *MissionGetPillarMinStr(UIObject *o, Mission **missionPtr)
{
	static char s[128];
	UNUSED(o);
	if (!*missionPtr) return NULL;
	sprintf(s, "PillarMin: %d", (*missionPtr)->u.Classic.Pillars.Min);
	return s;
}
static char *MissionGetPillarMaxStr(UIObject *o, Mission **missionPtr)
{
	static char s[128];
	UNUSED(o);
	if (!*missionPtr) return NULL;
	sprintf(s, "PillarMax: %d", (*missionPtr)->u.Classic.Pillars.Max);
	return s;
}
static char *MissionGetDensityStr(UIObject *o, Mission **missionPtr)
{
	static char s[128];
	UNUSED(o);
	if (!*missionPtr) return NULL;
	sprintf(s, "Dens: %d", (*missionPtr)->EnemyDensity);
	return s;
}
static void MissionDrawWallStyle(
	UIObject *o, GraphicsDevice *g, Mission **missionPtr)
{
	int index;
	int count = WALL_STYLE_COUNT;
	if (!*missionPtr) return; 
	index = (*missionPtr)->WallStyle;
	DrawStyleArea(
		o->Pos,
		"Wall",
		g,
		PicManagerGetOldPic(&gPicManager, cWallPics[index % count][WALL_SINGLE]),
		index, count,
		UIObjectIsHighlighted(o));
}
static void MissionDrawFloorStyle(
	UIObject *o, GraphicsDevice *g, Mission **missionPtr)
{
	int index;
	int count = FLOOR_STYLE_COUNT;
	if (!*missionPtr)
	{
		o->IsVisible = 0;
		return;
	}
	o->IsVisible = 1;
	index = (*missionPtr)->FloorStyle;
	DrawStyleArea(
		o->Pos,
		"Floor",
		g,
		PicManagerGetOldPic(&gPicManager, cFloorPics[index % count][FLOOR_NORMAL]),
		index, count,
		UIObjectIsHighlighted(o));
}
static void MissionDrawRoomStyle(
	UIObject *o, GraphicsDevice *g, Mission **missionPtr)
{
	int index;
	int count = ROOMFLOOR_COUNT;
	if (!*missionPtr)
	{
		o->IsVisible = 0;
		return;
	}
	o->IsVisible = 1;
	index = (*missionPtr)->RoomStyle;
	DrawStyleArea(
		o->Pos,
		"Rooms",
		g,
		PicManagerGetOldPic(&gPicManager, cRoomPics[index % count][ROOMFLOOR_NORMAL]),
		index, count,
		UIObjectIsHighlighted(o));
}
static void MissionDrawDoorStyle(
	UIObject *o, GraphicsDevice *g, Mission **missionPtr)
{
	int index;
	int count = GetEditorInfo().doorCount;
	if (!*missionPtr)
	{
		o->IsVisible = 0;
		return;
	}
	o->IsVisible = 1;
	index = (*missionPtr)->DoorStyle;
	DrawStyleArea(
		o->Pos,
		"Doors",
		g,
		PicManagerGetOldPic(&gPicManager, cGeneralPics[gMission.doorPics[0].horzPic].picIndex),
		index, count,
		UIObjectIsHighlighted(o));
}
static void MissionDrawKeyStyle(
	UIObject *o, GraphicsDevice *g, Mission **missionPtr)
{
	int index;
	int count = GetEditorInfo().keyCount;
	if (!*missionPtr)
	{
		o->IsVisible = 0;
		return;
	}
	o->IsVisible = 1;
	index = (*missionPtr)->KeyStyle;
	DrawStyleArea(
		o->Pos,
		"Keys",
		g,
		PicManagerGetOldPic(&gPicManager, cGeneralPics[gMission.keyPics[0]].picIndex),
		index, count,
		UIObjectIsHighlighted(o));
}
static void MissionDrawExitStyle(
	UIObject *o, GraphicsDevice *g, Mission **missionPtr)
{
	int index;
	int count = GetEditorInfo().exitCount;
	if (!*missionPtr)
	{
		o->IsVisible = 0;
		return;
	}
	o->IsVisible = 1;
	index = (*missionPtr)->ExitStyle;
	DrawStyleArea(
		o->Pos,
		"Exit",
		g,
		PicManagerGetOldPic(&gPicManager, gMission.exitPic),
		index, count,
		UIObjectIsHighlighted(o));
}
static char *MissionGetWallColorStr(UIObject *o, Mission **missionPtr)
{
	static char s[128];
	UNUSED(o);
	if (!*missionPtr) return NULL;
	sprintf(s, "Walls: %s", RangeName((*missionPtr)->WallColor));
	return s;
}
static char *MissionGetFloorColorStr(UIObject *o, Mission **missionPtr)
{
	static char s[128];
	UNUSED(o);
	if (!*missionPtr) return NULL;
	sprintf(s, "Floor: %s", RangeName((*missionPtr)->FloorColor));
	return s;
}
static char *MissionGeRoomColorStr(UIObject *o, Mission **missionPtr)
{
	static char s[128];
	UNUSED(o);
	if (!*missionPtr) return NULL;
	sprintf(s, "Rooms: %s", RangeName((*missionPtr)->RoomColor));
	return s;
}
static char *MissionGeExtraColorStr(UIObject *o, Mission **missionPtr)
{
	static char s[128];
	UNUSED(o);
	if (!*missionPtr) return NULL;
	sprintf(s, "Extra: %s", RangeName((*missionPtr)->AltColor));
	return s;
}
static char *MissionGetCharacterCountStr(UIObject *o, Mission **missionPtr)
{
	static char s[128];
	UNUSED(o);
	if (!*missionPtr) return NULL;
	sprintf(s, "Characters (%d)", (*missionPtr)->Enemies.size);
	return s;
}
static char *MissionGetSpecialCountStr(UIObject *o, Mission **missionPtr)
{
	static char s[128];
	UNUSED(o);
	if (!*missionPtr) return NULL;
	sprintf(
		s, "Mission objective characters (%d)",
		(*missionPtr)->SpecialChars.size);
	return s;
}
static char *MissionGetObjectiveDescription(
	UIObject *o, Mission **missionPtr)
{
	static char s[128];
	int i;
	if (!*missionPtr)
	{
		o->IsVisible = 0;
		return NULL;
	}
	i = o->Id - YC_OBJECTIVES;
	if ((int)(*missionPtr)->Objectives.size <= i)
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
	return ((MissionObjective *)CArrayGet(&(*missionPtr)->Objectives, i))->Description;
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
	Mission **missionPtr;
	int index;
} MissionIndexData;
static void MissionDrawEnemy(
	UIObject *o, GraphicsDevice *g, MissionIndexData *data)
{
	UNUSED(g);
	if (!*data->missionPtr) return;
	if (data->index >= (int)(*data->missionPtr)->Enemies.size) return;
	DisplayCharacter(
		Vec2iAdd(o->Pos, Vec2iScaleDiv(o->Size, 2)),
		gCampaign.Setting.characters.baddies[data->index],
		UIObjectIsHighlighted(o), 1);
}
static void MissionDrawSpecialChar(
	UIObject *o, GraphicsDevice *g, MissionIndexData *data)
{
	UNUSED(g);
	if (!*data->missionPtr) return;
	if (data->index >= (int)(*data->missionPtr)->SpecialChars.size) return;
	DisplayCharacter(
		Vec2iAdd(o->Pos, Vec2iScaleDiv(o->Size, 2)),
		gCampaign.Setting.characters.specials[data->index],
		UIObjectIsHighlighted(o), 1);
}
static void DisplayMapItem(
	GraphicsDevice *g,
	Vec2i pos, TMapObject *mo, int density, int isHighlighted);
static void MissionDrawMapItem(
	UIObject *o, GraphicsDevice *g, MissionIndexData *data)
{
	if (!*data->missionPtr) return;
	if (data->index >= (int)(*data->missionPtr)->Items.size) return;
	DisplayMapItem(
		g,
		Vec2iAdd(o->Pos, Vec2iScaleDiv(o->Size, 2)),
		CArrayGet(&gMission.MapObjects, data->index),
		*(int *)CArrayGet(&(*data->missionPtr)->ItemDensities, data->index),
		UIObjectIsHighlighted(o));
}
static void MissionDrawWeaponStatus(
	UIObject *o, GraphicsDevice *g, MissionIndexData *data)
{
	int hasWeapon;
	if (!*data->missionPtr) return;
	hasWeapon = (*data->missionPtr)->Weapons[data->index];
	DisplayFlag(
		g,
		o->Pos,
		gGunDescriptions[data->index].name,
		hasWeapon,
		UIObjectIsHighlighted(o));
}
static const char *MissionGetObjectiveStr(UIObject *o, MissionIndexData *data)
{
	UNUSED(o);
	if (!*data->missionPtr) return NULL;
	if ((int)(*data->missionPtr)->Objectives.size <= data->index) return NULL;
	return ObjectiveTypeStr(((MissionObjective *)CArrayGet(
		&(*data->missionPtr)->Objectives, data->index))->Type);
}
static void GetCharacterHeadPic(
	Character *c, TOffsetPic *pic, TranslationTable **t);
static void MissionDrawObjective(
	UIObject *o, GraphicsDevice *g, MissionIndexData *data)
{
	CharacterStore *store = &gCampaign.Setting.characters;
	Character *c;
	TOffsetPic pic;
	TranslationTable *table = NULL;
	struct Objective *obj;
	UNUSED(g);
	if (!*data->missionPtr) return;
	if ((int)(*data->missionPtr)->Objectives.size <= data->index) return;
	// TODO: only one kill and rescue objective allowed
	obj = CArrayGet(&gMission.Objectives, data->index);
	switch (((MissionObjective *)CArrayGet(
		&(*data->missionPtr)->Objectives, data->index))->Type)
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
			o->Pos.x + o->Size.x / 2 + pic.dx,
			o->Pos.y + o->Size.y / 2 + pic.dy,
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
	if (!*data->missionPtr) return NULL;
	if ((int)(*data->missionPtr)->Objectives.size <= data->index) return NULL;
	sprintf(s, "%d",
		GetMissionObjective(*data->missionPtr, data->index)->Required);
	return s;
}
static char *MissionGetObjectiveTotal(UIObject *o, MissionIndexData *data)
{
	static char s[128];
	UNUSED(o);
	if (!*data->missionPtr) return NULL;
	if ((int)(*data->missionPtr)->Objectives.size <= data->index) return NULL;
	sprintf(s, "out of %d",
		GetMissionObjective(*data->missionPtr, data->index)->Count);
	return s;
}
static char *MissionGetObjectiveFlags(UIObject *o, MissionIndexData *data)
{
	int flags;
	static char s[128];
	UNUSED(o);
	if (!*data->missionPtr) return NULL;
	if ((int)(*data->missionPtr)->Objectives.size <= data->index) return NULL;
	flags = GetMissionObjective(*data->missionPtr, data->index)->Flags;
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
static void DisplayMapItem(
	GraphicsDevice *g,
	Vec2i pos, TMapObject *mo, int density, int isHighlighted)
{
	char s[10];

	const TOffsetPic *pic = &cGeneralPics[mo->pic];
	DrawTPic(
		pos.x + pic->dx, pos.y + pic->dy,
		PicManagerGetOldPic(&gPicManager, pic->picIndex));

	if (isHighlighted)
	{
		DrawTextCharMasked(
			'\020', g, Vec2iAdd(pos, Vec2iNew(-8, -4)), colorWhite);
	}
	sprintf(s, "%d", density);
	DrawTextString(s, g, Vec2iAdd(pos, Vec2iNew(-8, 5)));
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
static void MissionChangeWidth(Mission **missionPtr, int d)
{
	(*missionPtr)->Size.x = CLAMP((*missionPtr)->Size.x + d, 16, XMAX);
}
static void MissionChangeHeight(Mission **missionPtr, int d)
{
	(*missionPtr)->Size.y = CLAMP((*missionPtr)->Size.y + d, 16, XMAX);
}
static void MissionChangeWallCount(Mission **missionPtr, int d)
{
	(*missionPtr)->u.Classic.Walls =
		CLAMP((*missionPtr)->u.Classic.Walls + d, 0, 200);
}
static void MissionChangeWallLength(Mission **missionPtr, int d)
{
	(*missionPtr)->u.Classic.WallLength =
		CLAMP((*missionPtr)->u.Classic.WallLength + d, 1, 100);
}
static void MissionChangeCorridorWidth(Mission **missionPtr, int d)
{
	(*missionPtr)->u.Classic.CorridorWidth =
		CLAMP((*missionPtr)->u.Classic.CorridorWidth + d, 1, 5);
}
static void MissionChangeRoomCount(Mission **missionPtr, int d)
{
	(*missionPtr)->u.Classic.Rooms.Count =
		CLAMP((*missionPtr)->u.Classic.Rooms.Count + d, 0, 100);
}
static void MissionChangeRoomMin(Mission **missionPtr, int d)
{
	(*missionPtr)->u.Classic.Rooms.Min =
		CLAMP((*missionPtr)->u.Classic.Rooms.Min + d, 5, 50);
	(*missionPtr)->u.Classic.Rooms.Max = MAX(
		(*missionPtr)->u.Classic.Rooms.Min,
		(*missionPtr)->u.Classic.Rooms.Max);
}
static void MissionChangeRoomMax(Mission **missionPtr, int d)
{
	(*missionPtr)->u.Classic.Rooms.Max =
		CLAMP((*missionPtr)->u.Classic.Rooms.Max + d, 5, 50);
	(*missionPtr)->u.Classic.Rooms.Min = MIN(
		(*missionPtr)->u.Classic.Rooms.Min,
		(*missionPtr)->u.Classic.Rooms.Max);
}
static void MissionChangeEdgeRooms(Mission **missionPtr, int d)
{
	UNUSED(d);
	(*missionPtr)->u.Classic.Rooms.Edge =
		!(*missionPtr)->u.Classic.Rooms.Edge;
}
static void MissionChangeRoomsOverlap(Mission **missionPtr, int d)
{
	UNUSED(d);
	(*missionPtr)->u.Classic.Rooms.Overlap =
		!(*missionPtr)->u.Classic.Rooms.Overlap;
}
static void MissionChangeRoomWallCount(Mission **missionPtr, int d)
{
	(*missionPtr)->u.Classic.Rooms.Walls =
		CLAMP((*missionPtr)->u.Classic.Rooms.Walls + d, 0, 50);
}
static void MissionChangeRoomWallLen(Mission **missionPtr, int d)
{
	(*missionPtr)->u.Classic.Rooms.WallLength =
		CLAMP((*missionPtr)->u.Classic.Rooms.WallLength + d, 1, 50);
}
static void MissionChangeRoomWallPad(Mission **missionPtr, int d)
{
	(*missionPtr)->u.Classic.Rooms.WallPad =
		CLAMP((*missionPtr)->u.Classic.Rooms.WallPad + d, 2, 10);
}
static void MissionChangeSquareCount(Mission **missionPtr, int d)
{
	(*missionPtr)->u.Classic.Squares =
		CLAMP((*missionPtr)->u.Classic.Squares + d, 0, 100);
}
static void MissionChangeDoorEnabled(Mission **missionPtr, int d)
{
	UNUSED(d);
	(*missionPtr)->u.Classic.Doors.Enabled =
		!(*missionPtr)->u.Classic.Doors.Enabled;
}
static void MissionChangeDoorMin(Mission **missionPtr, int d)
{
	(*missionPtr)->u.Classic.Doors.Min =
		CLAMP((*missionPtr)->u.Classic.Doors.Min + d, 1, 6);
	(*missionPtr)->u.Classic.Doors.Max = MAX(
		(*missionPtr)->u.Classic.Doors.Min,
		(*missionPtr)->u.Classic.Doors.Max);
}
static void MissionChangeDoorMax(Mission **missionPtr, int d)
{
	(*missionPtr)->u.Classic.Doors.Max =
		CLAMP((*missionPtr)->u.Classic.Doors.Max + d, 1, 6);
	(*missionPtr)->u.Classic.Doors.Min = MIN(
		(*missionPtr)->u.Classic.Doors.Min,
		(*missionPtr)->u.Classic.Doors.Max);
}
static void MissionChangePillarCount(Mission **missionPtr, int d)
{
	(*missionPtr)->u.Classic.Pillars.Count =
		CLAMP((*missionPtr)->u.Classic.Pillars.Count + d, 0, 50);
}
static void MissionChangePillarMin(Mission **missionPtr, int d)
{
	(*missionPtr)->u.Classic.Pillars.Min =
		CLAMP((*missionPtr)->u.Classic.Pillars.Min + d, 1, 50);
	(*missionPtr)->u.Classic.Pillars.Max = MAX(
		(*missionPtr)->u.Classic.Pillars.Min,
		(*missionPtr)->u.Classic.Pillars.Max);
}
static void MissionChangePillarMax(Mission **missionPtr, int d)
{
	(*missionPtr)->u.Classic.Pillars.Max =
		CLAMP((*missionPtr)->u.Classic.Pillars.Max + d, 1, 50);
	(*missionPtr)->u.Classic.Pillars.Min = MIN(
		(*missionPtr)->u.Classic.Pillars.Min,
		(*missionPtr)->u.Classic.Pillars.Max);
}
static void MissionChangeDensity(Mission **missionPtr, int d)
{
	(*missionPtr)->EnemyDensity = CLAMP((*missionPtr)->EnemyDensity + d, 0, 100);
}
static void MissionChangeWallStyle(Mission **missionPtr, int d)
{
	(*missionPtr)->WallStyle = CLAMP_OPPOSITE(
		(*missionPtr)->WallStyle + d, 0, WALL_STYLE_COUNT - 1);
}
static void MissionChangeFloorStyle(Mission **missionPtr, int d)
{
	(*missionPtr)->FloorStyle = CLAMP_OPPOSITE(
		(*missionPtr)->FloorStyle + d, 0, FLOOR_STYLE_COUNT - 1);
}
static void MissionChangeRoomStyle(Mission **missionPtr, int d)
{
	(*missionPtr)->RoomStyle = CLAMP_OPPOSITE(
		(*missionPtr)->RoomStyle + d, 0, ROOMFLOOR_COUNT - 1);
}
static void MissionChangeDoorStyle(Mission **missionPtr, int d)
{
	(*missionPtr)->DoorStyle = CLAMP_OPPOSITE(
		(*missionPtr)->DoorStyle + d, 0, GetEditorInfo().doorCount - 1);
}
static void MissionChangeKeyStyle(Mission **missionPtr, int d)
{
	(*missionPtr)->KeyStyle = CLAMP_OPPOSITE(
		(*missionPtr)->KeyStyle + d, 0, GetEditorInfo().keyCount - 1);
}
static void MissionChangeExitStyle(Mission **missionPtr, int d)
{
	(*missionPtr)->ExitStyle = CLAMP_OPPOSITE(
		(*missionPtr)->ExitStyle + d, 0, GetEditorInfo().exitCount - 1);
}
static void MissionChangeWallColor(Mission **missionPtr, int d)
{
	(*missionPtr)->WallColor = CLAMP_OPPOSITE(
		(*missionPtr)->WallColor + d, 0, GetEditorInfo().rangeCount - 1);
}
static void MissionChangeFloorColor(Mission **missionPtr, int d)
{
	(*missionPtr)->FloorColor = CLAMP_OPPOSITE(
		(*missionPtr)->FloorColor + d, 0, GetEditorInfo().rangeCount - 1);
}
static void MissionChangeRoomColor(Mission **missionPtr, int d)
{
	(*missionPtr)->RoomColor = CLAMP_OPPOSITE(
		(*missionPtr)->RoomColor + d, 0, GetEditorInfo().rangeCount - 1);
}
static void MissionChangeExtraColor(Mission **missionPtr, int d)
{
	(*missionPtr)->AltColor = CLAMP_OPPOSITE(
		(*missionPtr)->AltColor + d, 0, GetEditorInfo().rangeCount - 1);
}
static void MissionChangeEnemy(MissionIndexData *data, int d)
{
	int enemy = *(int *)CArrayGet(&(*data->missionPtr)->Enemies, data->index);
	enemy = CLAMP_OPPOSITE(
		enemy + d, 0, (int)gCampaign.Setting.characters.OtherChars.size - 1);
	*(int *)CArrayGet(&(*data->missionPtr)->Enemies, data->index) = enemy;
	gCampaign.Setting.characters.baddies[data->index] =
		CArrayGet(&gCampaign.Setting.characters.OtherChars, enemy);
}
static void MissionChangeSpecialChar(MissionIndexData *data, int d)
{
	int c = *(int *)CArrayGet(&(*data->missionPtr)->SpecialChars, data->index);
	c = CLAMP_OPPOSITE(
		c + d, 0, (int)gCampaign.Setting.characters.OtherChars.size - 1);
	*(int *)CArrayGet(&(*data->missionPtr)->SpecialChars, data->index) = c;
	gCampaign.Setting.characters.specials[data->index] =
		CArrayGet(&gCampaign.Setting.characters.OtherChars, c);
}
static void MissionChangeWeapon(MissionIndexData *data, int d)
{
	int hasWeapon;
	UNUSED(d);
	hasWeapon = (*data->missionPtr)->Weapons[data->index];
	(*data->missionPtr)->Weapons[data->index] = !hasWeapon;
}
static void MissionChangeMapItem(MissionIndexData *data, int d)
{
	if (gEventHandlers.keyboard.modState & KMOD_SHIFT)
	{
		int density = *(int *)CArrayGet(
			&(*data->missionPtr)->ItemDensities, data->index);
		density = CLAMP(density + 5 * d, 0, 512);
		*(int *)CArrayGet(&(*data->missionPtr)->ItemDensities, data->index) = density;
	}
	else
	{
		int i = *(int *)CArrayGet(
			&(*data->missionPtr)->Items, data->index);
		i = CLAMP_OPPOSITE(i + d, 0, GetEditorInfo().itemCount - 1);
		*(int *)CArrayGet(&(*data->missionPtr)->Items, data->index) = i;
	}
}
static void MissionChangeObjectiveIndex(MissionIndexData *data, int d);
static void MissionChangeObjectiveType(MissionIndexData *data, int d)
{
	MissionObjective *mobj = GetMissionObjective(*data->missionPtr, data->index);
	mobj->Type = CLAMP_OPPOSITE(mobj->Type + d, 0, OBJECTIVE_INVESTIGATE);
	// Initialise the index of the objective
	MissionChangeObjectiveIndex(data, 0);
}
static void MissionChangeObjectiveIndex(MissionIndexData *data, int d)
{
	MissionObjective *mobj = GetMissionObjective(*data->missionPtr, data->index);
	int limit;
	switch (mobj->Type)
	{
	case OBJECTIVE_COLLECT:
		limit = GetEditorInfo().pickupCount - 1;
		break;
	case OBJECTIVE_DESTROY:
		limit = GetEditorInfo().itemCount - 1;
		break;
	case OBJECTIVE_KILL:
	case OBJECTIVE_INVESTIGATE:
		limit = 0;
		break;
	case OBJECTIVE_RESCUE:
		limit = gCampaign.Setting.characters.OtherChars.size - 1;
		break;
	default:
		assert(0 && "Unknown objective type");
		return;
	}
	mobj->Index = CLAMP_OPPOSITE(mobj->Index + d, 0, limit);
}
static void MissionChangeObjectiveRequired(MissionIndexData *data, int d)
{
	MissionObjective *mobj = GetMissionObjective(*data->missionPtr, data->index);
	mobj->Required = CLAMP_OPPOSITE(
		mobj->Required + d, 0, MIN(100, mobj->Count));
}
static void MissionChangeObjectiveTotal(MissionIndexData *data, int d)
{
	MissionObjective *mobj = GetMissionObjective(*data->missionPtr, data->index);
	mobj->Count = CLAMP_OPPOSITE(mobj->Count + d, mobj->Required, 100);
}
static void MissionChangeObjectiveFlags(MissionIndexData *data, int d)
{
	MissionObjective *mobj = GetMissionObjective(*data->missionPtr, data->index);
	// Max is combination of all flags, i.e. largest flag doubled less one
	mobj->Flags = CLAMP_OPPOSITE(
		mobj->Flags + d, 0, OBJECTIVE_NOACCESS * 2 - 1);
}


static UIObject *CreateCampaignObjs(void);
static UIObject *CreateMissionObjs(Mission **missionPtr);
static UIObject *CreateClassicMapObjs(Vec2i pos, Mission **missionPtr);
static UIObject *CreateWeaponObjs(Mission **missionPtr);
static UIObject *CreateMapItemObjs(Mission **missionPtr);
static UIObject *CreateCharacterObjs(Mission **missionPtr);
static UIObject *CreateSpecialCharacterObjs(Mission **missionPtr);
static UIObject *CreateObjectiveObjs(
	Vec2i pos, Mission **missionPtr, int index);

UIObject *CreateMainObjs(Mission **missionPtr)
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
	o->Data = &gCampaign;
	CSTRDUP(o->u.Textbox.Hint, "(Campaign title)");
	o->Flags = UI_SELECT_ONLY_FIRST;
	UIObjectAddChild(o, CreateCampaignObjs());
	UIObjectAddChild(cc, o);

	o = UIObjectCreate(
		UITYPE_NONE, YC_MISSIONINDEX, Vec2iNew(270, pos.y), Vec2iNew(49, th));
	UIObjectAddChild(cc, o);

	pos.y = 2 * th;

	// Mission-only controls
	// Only visible if the current mission is valid
	c = UIObjectCreate(UITYPE_CUSTOM, 0, Vec2iZero(), Vec2iZero());
	c->u.CustomDrawFunc = CheckMission;
	c->Data = missionPtr;
	UIObjectAddChild(cc, c);

	o = UIObjectCreate(
		UITYPE_TEXTBOX, YC_MISSIONTITLE,
		Vec2iNew(25, pos.y), Vec2iNew(175, th));
	o->Id2 = XC_MISSIONTITLE;
	o->u.Textbox.TextLinkFunc = MissionGetTitle;
	o->Data = missionPtr;
	CSTRDUP(o->u.Textbox.Hint, "(Mission title)");
	UIObjectAddChild(o, CreateMissionObjs(missionPtr));
	UIObjectAddChild(c, o);

	// mission properties
	// size, walls, rooms etc.

	pos.y = 10 + 2 * th;

	o = UIObjectCreate(
		UITYPE_LABEL, 0, Vec2iZero(), Vec2iNew(35, th));

	pos.x = 20;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = MissionGetWidthStr;
	o2->Data = missionPtr;
	o2->ChangeFunc = MissionChangeWidth;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.x += 40;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = MissionGetHeightStr;
	o2->Data = missionPtr;
	o2->ChangeFunc = MissionChangeHeight;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.x += 40;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = MissionGetDensityStr;
	o2->Data = missionPtr;
	o2->ChangeFunc = MissionChangeDensity;
	o2->Pos = pos;
	CSTRDUP(o2->Tooltip, "Number of non-objective characters");
	UIObjectAddChild(c, o2);

	pos.x += 40;
	o2 = UIObjectCreate(UITYPE_TAB, 0, pos, Vec2iNew(50, th));
	// Properties for classic C-Dogs maps
	pos.x = 20;
	pos.y += th;
	UITabAddChild(o2, CreateClassicMapObjs(pos, missionPtr), "Type: Classic+");
	UIObjectAddChild(c, o2);

	// Mission looks
	// wall/floor styles etc.

	pos.y += th * 6;

	UIObjectDestroy(o);
	o = UIObjectCreate(
		UITYPE_CUSTOM, YC_MISSIONLOOKS, Vec2iZero(), Vec2iNew(25, 25 + th));

	pos.x = 20;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_WALL;
	o2->u.CustomDrawFunc = MissionDrawWallStyle;
	o2->Data = missionPtr;
	o2->ChangeFunc = MissionChangeWallStyle;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.x += 30;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_FLOOR;
	o2->u.CustomDrawFunc = MissionDrawFloorStyle;
	o2->Data = missionPtr;
	o2->ChangeFunc = MissionChangeFloorStyle;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.x += 30;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_ROOM;
	o2->u.CustomDrawFunc = MissionDrawRoomStyle;
	o2->Data = missionPtr;
	o2->ChangeFunc = MissionChangeRoomStyle;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.x += 30;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_DOORS;
	o2->u.CustomDrawFunc = MissionDrawDoorStyle;
	o2->Data = missionPtr;
	o2->ChangeFunc = MissionChangeDoorStyle;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.x += 30;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_KEYS;
	o2->u.CustomDrawFunc = MissionDrawKeyStyle;
	o2->Data = missionPtr;
	o2->ChangeFunc = MissionChangeKeyStyle;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.x += 30;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_EXIT;
	o2->u.CustomDrawFunc = MissionDrawExitStyle;
	o2->Data = missionPtr;
	o2->ChangeFunc = MissionChangeExitStyle;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);

	// colours

	pos.x = 200;

	UIObjectDestroy(o);
	o = UIObjectCreate(
		UITYPE_LABEL, YC_MISSIONLOOKS, Vec2iZero(), Vec2iNew(100, th));

	o2 = UIObjectCopy(o);
	o2->Id2 = XC_COLOR1;
	o2->u.LabelFunc = MissionGetWallColorStr;
	o2->Data = missionPtr;
	o2->ChangeFunc = MissionChangeWallColor;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.y += th;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_COLOR2;
	o2->u.LabelFunc = MissionGetFloorColorStr;
	o2->Data = missionPtr;
	o2->ChangeFunc = MissionChangeFloorColor;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.y += th;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_COLOR3;
	o2->u.LabelFunc = MissionGeRoomColorStr;
	o2->Data = missionPtr;
	o2->ChangeFunc = MissionChangeRoomColor;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.y += th;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_COLOR4;
	o2->u.LabelFunc = MissionGeExtraColorStr;
	o2->Data = missionPtr;
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
	o2->u.LabelFunc = CStr;
	o2->Data = "Mission description";
	o2->Id = YC_MISSIONDESC;
	o2->Pos = pos;
	o2->Size = TextGetSize(o2->Data);
	oc = UIObjectCreate(
		UITYPE_TEXTBOX, YC_MISSIONDESC,
		Vec2iNew(25, 170), Vec2iNew(295, 5 * th));
	oc->Flags = UI_ENABLED_WHEN_PARENT_HIGHLIGHTED_ONLY;
	oc->u.Textbox.TextLinkFunc = MissionGetDescription;
	oc->Data = missionPtr;
	CSTRDUP(oc->u.Textbox.Hint, "(Mission description)");
	UIObjectAddChild(o2, oc);
	UIObjectAddChild(c, o2);

	pos.y += th;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = MissionGetCharacterCountStr;
	o2->Data = missionPtr;
	o2->Id = YC_CHARACTERS;
	o2->Pos = pos;
	CSTRDUP(o2->Tooltip, "Use Insert, Delete and PageUp/PageDown");
	UIObjectAddChild(o2, CreateCharacterObjs(missionPtr));
	UIObjectAddChild(c, o2);
	pos.y += th;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = MissionGetSpecialCountStr;
	o2->Data = missionPtr;
	o2->Id = YC_SPECIALS;
	o2->Pos = pos;
	CSTRDUP(o2->Tooltip, "Use Insert, Delete and PageUp/PageDown");
	UIObjectAddChild(o2, CreateSpecialCharacterObjs(missionPtr));
	UIObjectAddChild(c, o2);
	pos.y += th;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = GetWeaponCountStr;
	o2->Data = NULL;
	o2->Id = YC_WEAPONS;
	o2->Pos = pos;
	UIObjectAddChild(o2, CreateWeaponObjs(missionPtr));
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
	UIObjectAddChild(o2, CreateMapItemObjs(missionPtr));
	UIObjectAddChild(c, o2);

	// objectives
	pos.y += 2;
	objectivesPos = Vec2iNew(pos.x, pos.y + 8 * th);
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
		o2->Data = missionPtr;
		CSTRDUP(o2->u.Textbox.Hint, "(Objective description)");
		o2->Pos = pos;
		CSTRDUP(o2->Tooltip, "insert/delete: add/remove objective");
		UIObjectAddChild(o2, CreateObjectiveObjs(objectivesPos, missionPtr, i));
		UIObjectAddChild(c, o2);
	}

	UIObjectDestroy(o);
	return cc;
}
static UIObject *CreateCampaignObjs(void)
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
	o2->Data = &gCampaign;
	CSTRDUP(o2->u.Textbox.Hint, "(Campaign author)");
	o2->Id2 = XC_AUTHOR;
	o2->Pos = Vec2iNew(x, y);
	o2->Size = Vec2iNew(295, th);
	UIObjectAddChild(c, o2);

	y += th;
	o2 = UIObjectCopy(o);
	o2->u.Textbox.TextLinkFunc = CampaignGetDescription;
	o2->Data = &gCampaign;
	CSTRDUP(o2->u.Textbox.Hint, "(Campaign description)");
	o2->Id2 = XC_CAMPAIGNDESC;
	o2->Pos = Vec2iNew(x, y);
	o2->Size = Vec2iNew(295, 5 * th);
	UIObjectAddChild(c, o2);

	UIObjectDestroy(o);
	return c;
}
static UIObject *CreateMissionObjs(Mission **missionPtr)
{
	int th = CDogsTextHeight();
	UIObject *c;
	UIObject *o;
	c = UIObjectCreate(UITYPE_NONE, 0, Vec2iZero(), Vec2iZero());
	c->Flags = UI_ENABLED_WHEN_PARENT_HIGHLIGHTED_ONLY;

	o = UIObjectCreate(
		UITYPE_TEXTBOX, YC_MISSIONTITLE, Vec2iNew(20, 170), Vec2iNew(319, th));
	o->u.Textbox.TextLinkFunc = MissionGetSong;
	o->Data = missionPtr;
	CSTRDUP(o->u.Textbox.Hint, "(Mission song)");
	o->Id2 = XC_MUSICFILE;
	o->Flags = UI_SELECT_ONLY;
	UIObjectAddChild(c, o);

	return c;
}
static UIObject *CreateClassicMapObjs(Vec2i pos, Mission **missionPtr)
{
	int th = CDogsTextHeight();
	UIObject *c = UIObjectCreate(UITYPE_NONE, 0, Vec2iZero(), Vec2iZero());
	UIObject *o = UIObjectCreate(
		UITYPE_LABEL, 0, Vec2iZero(), Vec2iNew(50, th));
	int x = pos.x;

	UIObject *o2 = UIObjectCopy(o);
	o2->u.LabelFunc = CampaignGetSeedStr;
	o2->Data = &gCampaign;
	o2->ChangeFunc = CampaignChangeSeed;
	CSTRDUP(o2->Tooltip, "Preview with different random seed");
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.x += o2->Size.x;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = MissionGetWallCountStr;
	o2->Data = missionPtr;
	o2->ChangeFunc = MissionChangeWallCount;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.x += o2->Size.x;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = MissionGetWallLengthStr;
	o2->Data = missionPtr;
	o2->ChangeFunc = MissionChangeWallLength;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.x += o2->Size.x;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = MissionGetCorridorWidthStr;
	o2->Data = missionPtr;
	o2->ChangeFunc = MissionChangeCorridorWidth;
	o2->Pos = pos;
	o2->Size.x = 60;
	UIObjectAddChild(c, o2);
	
	pos.x = x;
	pos.y += th;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = MissionGetRoomCountStr;
	o2->Data = missionPtr;
	o2->ChangeFunc = MissionChangeRoomCount;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.x += o2->Size.x;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = MissionGetRoomMinStr;
	o2->Data = missionPtr;
	o2->ChangeFunc = MissionChangeRoomMin;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.x += o2->Size.x;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = MissionGetRoomMaxStr;
	o2->Data = missionPtr;
	o2->ChangeFunc = MissionChangeRoomMax;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.x += o2->Size.x;
	o2 = UIObjectCreate(UITYPE_CUSTOM, 0, pos, Vec2iNew(60, th));
	o2->u.CustomDrawFunc = MissionDrawEdgeRooms;
	o2->Data = missionPtr;
	o2->ChangeFunc = MissionChangeEdgeRooms;
	UIObjectAddChild(c, o2);
	pos.x += o2->Size.x;
	o2 = UIObjectCreate(UITYPE_CUSTOM, 0, pos, Vec2iNew(60, th));
	o2->u.CustomDrawFunc = MissionDrawRoomsOverlap;
	o2->Data = missionPtr;
	o2->ChangeFunc = MissionChangeRoomsOverlap;
	UIObjectAddChild(c, o2);

	pos.x = x;
	pos.y += th;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = MissionGetRoomWallCountStr;
	o2->Data = missionPtr;
	o2->ChangeFunc = MissionChangeRoomWallCount;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.x += o2->Size.x;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = MissionGetRoomWallLenStr;
	o2->Data = missionPtr;
	o2->ChangeFunc = MissionChangeRoomWallLen;
	o2->Pos = pos;
	o2->Size.x = 60;
	UIObjectAddChild(c, o2);
	pos.x += o2->Size.x;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = MissionGetRoomWallPadStr;
	o2->Data = missionPtr;
	o2->ChangeFunc = MissionChangeRoomWallPad;
	o2->Pos = pos;
	o2->Size.x = 60;
	UIObjectAddChild(c, o2);

	pos.x = x;
	pos.y += th;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = MissionGetSquareCountStr;
	o2->Data = missionPtr;
	o2->ChangeFunc = MissionChangeSquareCount;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);

	pos.x = x;
	pos.y += th;
	o2 = UIObjectCreate(UITYPE_CUSTOM, 0, pos, o->Size);
	o2->u.CustomDrawFunc = MissionDrawDoorEnabled;
	o2->Data = missionPtr;
	o2->ChangeFunc = MissionChangeDoorEnabled;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.x += o2->Size.x;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = MissionGetDoorMinStr;
	o2->Data = missionPtr;
	o2->ChangeFunc = MissionChangeDoorMin;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.x += o2->Size.x;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = MissionGetDoorMaxStr;
	o2->Data = missionPtr;
	o2->ChangeFunc = MissionChangeDoorMax;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);

	pos.x = x;
	pos.y += th;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = MissionGetPillarCountStr;
	o2->Data = missionPtr;
	o2->ChangeFunc = MissionChangePillarCount;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.x += o2->Size.x;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = MissionGetPillarMinStr;
	o2->Data = missionPtr;
	o2->ChangeFunc = MissionChangePillarMin;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.x += o2->Size.x;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = MissionGetPillarMaxStr;
	o2->Data = missionPtr;
	o2->ChangeFunc = MissionChangePillarMax;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);

	return c;
}
static UIObject *CreateWeaponObjs(Mission **missionPtr)
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
	for (i = 0; i < GUN_COUNT; i++)
	{
		int x = 10 + i / 4 * 90;
		int y = 170 + (i % 4) * th;
		o2 = UIObjectCopy(o);
		o2->Id2 = i;
		CMALLOC(o2->Data, sizeof(MissionIndexData));
		o2->IsDynamicData = 1;
		((MissionIndexData *)o2->Data)->missionPtr = missionPtr;
		((MissionIndexData *)o2->Data)->index = i;
		o2->Pos = Vec2iNew(x, y);
		UIObjectAddChild(c, o2);
	}

	UIObjectDestroy(o);
	return c;
}
static UIObject *CreateMapItemObjs(Mission **missionPtr)
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
	for (i = 0; i < 32; i++)	// TODO: no limit to objects
	{
		int x = 10 + i * 20;
		o2 = UIObjectCopy(o);
		o2->Id2 = i;
		CMALLOC(o2->Data, sizeof(MissionIndexData));
		o2->IsDynamicData = 1;
		((MissionIndexData *)o2->Data)->missionPtr = missionPtr;
		((MissionIndexData *)o2->Data)->index = i;
		o2->Pos = Vec2iNew(x, 170);
		UIObjectAddChild(c, o2);
	}

	UIObjectDestroy(o);
	return c;
}
static UIObject *CreateObjectiveObjs(
	Vec2i pos, Mission **missionPtr, int index)
{
	int th = CDogsTextHeight();
	UIObject *c;
	UIObject *o;
	UIObject *o2;
	c = UIObjectCreate(UITYPE_NONE, 0, Vec2iZero(), Vec2iZero());
	c->Flags = UI_ENABLED_WHEN_PARENT_HIGHLIGHTED_ONLY;

	o = UIObjectCreate(UITYPE_NONE, 0, Vec2iZero(), Vec2iZero());
	o->Flags = UI_LEAVE_YC;

	o2 = UIObjectCopy(o);
	o2->Id2 = XC_TYPE;
	o2->Type = UITYPE_LABEL;
	o2->u.LabelFunc = MissionGetObjectiveStr;
	o2->ChangeFunc = MissionChangeObjectiveType;
	CMALLOC(o2->Data, sizeof(MissionIndexData));
	o2->IsDynamicData = 1;
	((MissionIndexData *)o2->Data)->missionPtr = missionPtr;
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
	((MissionIndexData *)o2->Data)->missionPtr = missionPtr;
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
	((MissionIndexData *)o2->Data)->missionPtr = missionPtr;
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
	((MissionIndexData *)o2->Data)->missionPtr = missionPtr;
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
	((MissionIndexData *)o2->Data)->missionPtr = missionPtr;
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
static UIObject *CreateCharacterObjs(Mission **missionPtr)
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
	for (i = 0; i < 15; i++)
	{
		int x = 10 + i * 20;
		o2 = UIObjectCopy(o);
		o2->Id2 = i;
		o2->IsDynamicData = 1;
		CMALLOC(o2->Data, sizeof(MissionIndexData));
		((MissionIndexData *)o2->Data)->missionPtr = missionPtr;
		((MissionIndexData *)o2->Data)->index = i;
		o2->Pos = Vec2iNew(x, 170);
		UIObjectAddChild(c, o2);
	}

	UIObjectDestroy(o);
	return c;
}
static UIObject *CreateSpecialCharacterObjs(Mission **missionPtr)
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
	for (i = 0; i < 15; i++)
	{
		int x = 10 + i * 20;
		o2 = UIObjectCopy(o);
		o2->Id2 = i;
		o2->IsDynamicData = 1;
		CMALLOC(o2->Data, sizeof(MissionIndexData));
		((MissionIndexData *)o2->Data)->missionPtr = missionPtr;
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
