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
	return c->Setting.title;
}
static char *CampaignGetAuthor(UIObject *o, CampaignOptions *c)
{
	UNUSED(o);
	return c->Setting.author;
}
static char *CampaignGetDescription(UIObject *o, CampaignOptions *c)
{
	UNUSED(o);
	return c->Setting.description;
}
static void CheckMission(
	UIObject *o, GraphicsDevice *g, struct Mission **missionPtr)
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
static char *MissionGetTitle(UIObject *o, struct Mission **missionPtr)
{
	UNUSED(o);
	if (!*missionPtr)
	{
		o->IsVisible = 0;
		return NULL;
	}
	o->IsVisible = 1;
	return (*missionPtr)->title;
}
static char *MissionGetDescription(UIObject *o, struct Mission **missionPtr)
{
	UNUSED(o);
	if (!*missionPtr)
	{
		o->IsVisible = 0;
		return NULL;
	}
	o->IsVisible = 1;
	return (*missionPtr)->description;
}
static char *MissionGetSong(UIObject *o, struct Mission **missionPtr)
{
	UNUSED(o);
	if (!*missionPtr)
	{
		o->IsVisible = 0;
		return NULL;
	}
	o->IsVisible = 1;
	return (*missionPtr)->song;
}
static char *MissionGetWidthStr(UIObject *o, struct Mission **missionPtr)
{
	static char s[128];
	UNUSED(o);
	if (!*missionPtr) return NULL;
	sprintf(s, "Width: %d", (*missionPtr)->mapWidth);
	return s;
}
static char *MissionGetHeightStr(UIObject *o, struct Mission **missionPtr)
{
	static char s[128];
	UNUSED(o);
	if (!*missionPtr) return NULL;
	sprintf(s, "Height: %d", (*missionPtr)->mapHeight);
	return s;
}
static char *MissionGetWallCountStr(UIObject *o, struct Mission **missionPtr)
{
	static char s[128];
	UNUSED(o);
	if (!*missionPtr) return NULL;
	sprintf(s, "Walls: %d", (*missionPtr)->wallCount);
	return s;
}
static char *MissionGetWallLengthStr(UIObject *o, struct Mission **missionPtr)
{
	static char s[128];
	UNUSED(o);
	if (!*missionPtr) return NULL;
	sprintf(s, "Len: %d", (*missionPtr)->wallLength);
	return s;
}
static char *MissionGetRoomCountStr(UIObject *o, struct Mission **missionPtr)
{
	static char s[128];
	UNUSED(o);
	if (!*missionPtr) return NULL;
	sprintf(s, "Rooms: %d", (*missionPtr)->roomCount);
	return s;
}
static char *MissionGetSquareCountStr(UIObject *o, struct Mission **missionPtr)
{
	static char s[128];
	UNUSED(o);
	if (!*missionPtr) return NULL;
	sprintf(s, "Sqr: %d", (*missionPtr)->squareCount);
	return s;
}
static char *MissionGetDensityStr(UIObject *o, struct Mission **missionPtr)
{
	static char s[128];
	UNUSED(o);
	if (!*missionPtr) return NULL;
	sprintf(s, "Dens: %d", (*missionPtr)->baddieDensity);
	return s;
}
static void MissionDrawWallStyle(
	UIObject *o, GraphicsDevice *g, struct Mission **missionPtr)
{
	int index;
	int count = WALL_STYLE_COUNT;
	if (!*missionPtr) return; 
	index = (*missionPtr)->wallStyle;
	DrawStyleArea(
		o->Pos,
		"Wall",
		g,
		PicManagerGetOldPic(&gPicManager, cWallPics[index % count][WALL_SINGLE]),
		index, count,
		UIObjectIsHighlighted(o));
}
static void MissionDrawFloorStyle(
	UIObject *o, GraphicsDevice *g, struct Mission **missionPtr)
{
	int index;
	int count = FLOOR_STYLE_COUNT;
	if (!*missionPtr)
	{
		o->IsVisible = 0;
		return;
	}
	o->IsVisible = 1;
	index = (*missionPtr)->floorStyle;
	DrawStyleArea(
		o->Pos,
		"Floor",
		g,
		PicManagerGetOldPic(&gPicManager, cFloorPics[index % count][FLOOR_NORMAL]),
		index, count,
		UIObjectIsHighlighted(o));
}
static void MissionDrawRoomStyle(
	UIObject *o, GraphicsDevice *g, struct Mission **missionPtr)
{
	int index;
	int count = ROOMFLOOR_COUNT;
	if (!*missionPtr)
	{
		o->IsVisible = 0;
		return;
	}
	o->IsVisible = 1;
	index = (*missionPtr)->roomStyle;
	DrawStyleArea(
		o->Pos,
		"Rooms",
		g,
		PicManagerGetOldPic(&gPicManager, cRoomPics[index % count][ROOMFLOOR_NORMAL]),
		index, count,
		UIObjectIsHighlighted(o));
}
static void MissionDrawDoorStyle(
	UIObject *o, GraphicsDevice *g, struct Mission **missionPtr)
{
	int index;
	int count = GetEditorInfo().doorCount;
	if (!*missionPtr)
	{
		o->IsVisible = 0;
		return;
	}
	o->IsVisible = 1;
	index = (*missionPtr)->doorStyle;
	DrawStyleArea(
		o->Pos,
		"Doors",
		g,
		PicManagerGetOldPic(&gPicManager, cGeneralPics[gMission.doorPics[0].horzPic].picIndex),
		index, count,
		UIObjectIsHighlighted(o));
}
static void MissionDrawKeyStyle(
	UIObject *o, GraphicsDevice *g, struct Mission **missionPtr)
{
	int index;
	int count = GetEditorInfo().keyCount;
	if (!*missionPtr)
	{
		o->IsVisible = 0;
		return;
	}
	o->IsVisible = 1;
	index = (*missionPtr)->keyStyle;
	DrawStyleArea(
		o->Pos,
		"Keys",
		g,
		PicManagerGetOldPic(&gPicManager, cGeneralPics[gMission.keyPics[0]].picIndex),
		index, count,
		UIObjectIsHighlighted(o));
}
static void MissionDrawExitStyle(
	UIObject *o, GraphicsDevice *g, struct Mission **missionPtr)
{
	int index;
	int count = GetEditorInfo().exitCount;
	if (!*missionPtr)
	{
		o->IsVisible = 0;
		return;
	}
	o->IsVisible = 1;
	index = (*missionPtr)->exitStyle;
	DrawStyleArea(
		o->Pos,
		"Exit",
		g,
		PicManagerGetOldPic(&gPicManager, gMission.exitPic),
		index, count,
		UIObjectIsHighlighted(o));
}
static char *MissionGetWallColorStr(UIObject *o, struct Mission **missionPtr)
{
	static char s[128];
	UNUSED(o);
	if (!*missionPtr) return NULL;
	sprintf(s, "Walls: %s", RangeName((*missionPtr)->wallRange));
	return s;
}
static char *MissionGetFloorColorStr(UIObject *o, struct Mission **missionPtr)
{
	static char s[128];
	UNUSED(o);
	if (!*missionPtr) return NULL;
	sprintf(s, "Floor: %s", RangeName((*missionPtr)->floorRange));
	return s;
}
static char *MissionGeRoomColorStr(UIObject *o, struct Mission **missionPtr)
{
	static char s[128];
	UNUSED(o);
	if (!*missionPtr) return NULL;
	sprintf(s, "Rooms: %s", RangeName((*missionPtr)->roomRange));
	return s;
}
static char *MissionGeExtraColorStr(UIObject *o, struct Mission **missionPtr)
{
	static char s[128];
	UNUSED(o);
	if (!*missionPtr) return NULL;
	sprintf(s, "Extra: %s", RangeName((*missionPtr)->altRange));
	return s;
}
static char *MissionGetCharacterCountStr(UIObject *o, struct Mission **missionPtr)
{
	static char s[128];
	UNUSED(o);
	if (!*missionPtr) return NULL;
	sprintf(s, "Characters (%d/%d)", (*missionPtr)->baddieCount, BADDIE_MAX);
	return s;
}
static char *MissionGetSpecialCountStr(UIObject *o, struct Mission **missionPtr)
{
	static char s[128];
	UNUSED(o);
	if (!*missionPtr) return NULL;
	sprintf(
		s, "Mission objective characters (%d/%d)",
		(*missionPtr)->specialCount, SPECIAL_MAX);
	return s;
}
static char *MissionGetObjectiveDescription(
	UIObject *o, struct Mission **missionPtr)
{
	static char s[128];
	int i;
	if (!*missionPtr)
	{
		o->IsVisible = 0;
		return NULL;
	}
	i = o->Id - YC_OBJECTIVES;
	if ((*missionPtr)->objectiveCount <= i)
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
	return (*missionPtr)->objectives[i].description;
}
static char *GetWeaponCountStr(UIObject *o, void *v)
{
	static char s[128];
	UNUSED(o);
	UNUSED(v);
	sprintf(s, "Available weapons (%d/%d)", gMission.weaponCount, WEAPON_MAX);
	return s;
}
static char *GetObjectCountStr(UIObject *o, void *v)
{
	static char s[128];
	UNUSED(o);
	UNUSED(v);
	sprintf(s, "Map items (%d/%d)", gMission.objectCount, ITEMS_MAX);
	return s;
}
typedef struct
{
	struct Mission **missionPtr;
	int index;
} MissionIndexData;
static void MissionDrawEnemy(
	UIObject *o, GraphicsDevice *g, MissionIndexData *data)
{
	UNUSED(g);
	if (!*data->missionPtr) return;
	if (data->index >= (*data->missionPtr)->baddieCount) return;
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
	if (data->index >= (*data->missionPtr)->specialCount) return;
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
	if (data->index >= (*data->missionPtr)->itemCount) return;
	DisplayMapItem(
		g,
		Vec2iAdd(o->Pos, Vec2iScaleDiv(o->Size, 2)),
		gMission.mapObjects[data->index],
		(*data->missionPtr)->itemDensity[data->index],
		UIObjectIsHighlighted(o));
}
static void MissionDrawWeaponStatus(
	UIObject *o, GraphicsDevice *g, MissionIndexData *data)
{
	if (!*data->missionPtr) return;
	DisplayFlag(
		g,
		o->Pos,
		gGunDescriptions[data->index].name,
		(*data->missionPtr)->weaponSelection & (1 << data->index),
		UIObjectIsHighlighted(o));
}
static char *MissionGetObjectiveStr(UIObject *o, MissionIndexData *data)
{
	UNUSED(o);
	if (!*data->missionPtr) return NULL;
	if ((*data->missionPtr)->objectiveCount <= data->index) return NULL;
	switch ((*data->missionPtr)->objectives[data->index].type)
	{
	case OBJECTIVE_KILL:
		return "Kill";
	case OBJECTIVE_RESCUE:
		return "Rescue";
	case OBJECTIVE_COLLECT:
		return "Collect";
	case OBJECTIVE_DESTROY:
		return "Destroy";
	case OBJECTIVE_INVESTIGATE:
		return "Explore";
	default:
		assert(0 && "Unknown objective type");
		return "???";
	}
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
	UNUSED(g);
	if (!*data->missionPtr) return;
	if ((*data->missionPtr)->objectiveCount <= data->index) return;
	// TODO: only one kill and rescue objective allowed
	switch ((*data->missionPtr)->objectives[data->index].type)
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
		pic = cGeneralPics[gMission.objectives[data->index].pickupItem];
		break;
	case OBJECTIVE_DESTROY:
		pic = cGeneralPics[gMission.objectives[data->index].blowupObject->pic];
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
static char *MissionGetObjectiveRequired(UIObject *o, MissionIndexData *data)
{
	static char s[128];
	UNUSED(o);
	if (!*data->missionPtr) return NULL;
	if ((*data->missionPtr)->objectiveCount <= data->index) return NULL;
	sprintf(s, "%d", (*data->missionPtr)->objectives[data->index].required);
	return s;
}
static char *MissionGetObjectiveTotal(UIObject *o, MissionIndexData *data)
{
	static char s[128];
	UNUSED(o);
	if (!*data->missionPtr) return NULL;
	if ((*data->missionPtr)->objectiveCount <= data->index) return NULL;
	sprintf(
		s, "out of %d", (*data->missionPtr)->objectives[data->index].count);
	return s;
}
static char *MissionGetObjectiveFlags(UIObject *o, MissionIndexData *data)
{
	int flags;
	static char s[128];
	UNUSED(o);
	if (!*data->missionPtr) return NULL;
	if ((*data->missionPtr)->objectiveCount <= data->index) return NULL;
	flags = (*data->missionPtr)->objectives[data->index].flags;
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


static void MissionChangeWidth(struct Mission **missionPtr, int d)
{
	(*missionPtr)->mapWidth = CLAMP((*missionPtr)->mapWidth + d, 16, XMAX);
}
static void MissionChangeHeight(struct Mission **missionPtr, int d)
{
	(*missionPtr)->mapHeight = CLAMP((*missionPtr)->mapHeight + d, 16, XMAX);
}
static void MissionChangeWallCount(struct Mission **missionPtr, int d)
{
	(*missionPtr)->wallCount = CLAMP((*missionPtr)->wallCount + d, 0, 200);
}
static void MissionChangeWallLength(struct Mission **missionPtr, int d)
{
	(*missionPtr)->wallLength = CLAMP((*missionPtr)->wallLength + d, 1, 100);
}
static void MissionChangeRoomCount(struct Mission **missionPtr, int d)
{
	(*missionPtr)->roomCount = CLAMP((*missionPtr)->roomCount + d, 0, 100);
}
static void MissionChangeSquareCount(struct Mission **missionPtr, int d)
{
	(*missionPtr)->squareCount = CLAMP((*missionPtr)->squareCount + d, 0, 100);
}
static void MissionChangeDensity(struct Mission **missionPtr, int d)
{
	(*missionPtr)->baddieDensity = CLAMP((*missionPtr)->baddieDensity + d, 0, 100);
}
static void MissionChangeWallStyle(struct Mission **missionPtr, int d)
{
	(*missionPtr)->wallStyle = CLAMP_OPPOSITE(
		(*missionPtr)->wallStyle + d, 0, WALL_STYLE_COUNT - 1);
}
static void MissionChangeFloorStyle(struct Mission **missionPtr, int d)
{
	(*missionPtr)->floorStyle = CLAMP_OPPOSITE(
		(*missionPtr)->floorStyle + d, 0, FLOOR_STYLE_COUNT - 1);
}
static void MissionChangeRoomStyle(struct Mission **missionPtr, int d)
{
	(*missionPtr)->roomStyle = CLAMP_OPPOSITE(
		(*missionPtr)->roomStyle + d, 0, ROOMFLOOR_COUNT - 1);
}
static void MissionChangeDoorStyle(struct Mission **missionPtr, int d)
{
	(*missionPtr)->doorStyle = CLAMP_OPPOSITE(
		(*missionPtr)->doorStyle + d, 0, GetEditorInfo().doorCount - 1);
}
static void MissionChangeKeyStyle(struct Mission **missionPtr, int d)
{
	(*missionPtr)->keyStyle = CLAMP_OPPOSITE(
		(*missionPtr)->keyStyle + d, 0, GetEditorInfo().keyCount - 1);
}
static void MissionChangeExitStyle(struct Mission **missionPtr, int d)
{
	(*missionPtr)->exitStyle = CLAMP_OPPOSITE(
		(*missionPtr)->exitStyle + d, 0, GetEditorInfo().exitCount - 1);
}
static void MissionChangeWallColor(struct Mission **missionPtr, int d)
{
	(*missionPtr)->wallRange = CLAMP_OPPOSITE(
		(*missionPtr)->wallRange + d, 0, GetEditorInfo().rangeCount - 1);
}
static void MissionChangeFloorColor(struct Mission **missionPtr, int d)
{
	(*missionPtr)->floorRange = CLAMP_OPPOSITE(
		(*missionPtr)->floorRange + d, 0, GetEditorInfo().rangeCount - 1);
}
static void MissionChangeRoomColor(struct Mission **missionPtr, int d)
{
	(*missionPtr)->roomRange = CLAMP_OPPOSITE(
		(*missionPtr)->roomRange + d, 0, GetEditorInfo().rangeCount - 1);
}
static void MissionChangeExtraColor(struct Mission **missionPtr, int d)
{
	(*missionPtr)->altRange = CLAMP_OPPOSITE(
		(*missionPtr)->altRange + d, 0, GetEditorInfo().rangeCount - 1);
}
static void MissionChangeEnemy(MissionIndexData *data, int d)
{
	(*data->missionPtr)->baddies[data->index] = CLAMP_OPPOSITE(
		(*data->missionPtr)->baddies[data->index] + d,
		0,
		gCampaign.Setting.characters.otherCount - 1);
	gCampaign.Setting.characters.baddies[data->index] =
		&gCampaign.Setting.characters.others[(*data->missionPtr)->baddies[data->index]];
}
static void MissionChangeSpecialChar(MissionIndexData *data, int d)
{
	(*data->missionPtr)->specials[data->index] = CLAMP_OPPOSITE(
		(*data->missionPtr)->specials[data->index] + d,
		0,
		gCampaign.Setting.characters.otherCount - 1);
	gCampaign.Setting.characters.specials[data->index] =
		&gCampaign.Setting.characters.others[(*data->missionPtr)->specials[data->index]];
}
static void MissionChangeWeapon(MissionIndexData *data, int d)
{
	UNUSED(d);
	(*data->missionPtr)->weaponSelection ^= (1 << data->index);
}
static void MissionChangeMapItem(MissionIndexData *data, int d)
{
	if (gEventHandlers.keyboard.modState & KMOD_SHIFT)
	{
		(*data->missionPtr)->itemDensity[data->index] =
			CLAMP((*data->missionPtr)->itemDensity[data->index] + 5 * d,
			0,
			512);
	}
	else
	{
		(*data->missionPtr)->items[data->index] = CLAMP_OPPOSITE(
			(*data->missionPtr)->items[data->index] + d,
			0,
			GetEditorInfo().itemCount - 1);
	}
}
static void MissionChangeObjectiveIndex(MissionIndexData *data, int d);
static void MissionChangeObjectiveType(MissionIndexData *data, int d)
{
	struct MissionObjective *objective =
		&(*data->missionPtr)->objectives[data->index];
	objective->type = CLAMP_OPPOSITE(
		objective->type + d, 0, OBJECTIVE_INVESTIGATE);
	// Initialise the index of the objective
	MissionChangeObjectiveIndex(data, 0);
}
static void MissionChangeObjectiveIndex(MissionIndexData *data, int d)
{
	struct MissionObjective *objective =
		&(*data->missionPtr)->objectives[data->index];
	int limit;
	switch (objective->type)
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
		limit = gCampaign.Setting.characters.otherCount - 1;
		break;
	default:
		assert(0 && "Unknown objective type");
		return;
	}
	objective->index = CLAMP_OPPOSITE(objective->index + d, 0, limit);
}
static void MissionChangeObjectiveRequired(MissionIndexData *data, int d)
{
	struct MissionObjective *objective =
		&(*data->missionPtr)->objectives[data->index];
	objective->required = CLAMP_OPPOSITE(
		objective->required + d, 0, MIN(100, objective->count));
}
static void MissionChangeObjectiveTotal(MissionIndexData *data, int d)
{
	struct MissionObjective *objective =
		&(*data->missionPtr)->objectives[data->index];
	objective->count = CLAMP_OPPOSITE(
		objective->count + d, objective->required, 100);
}
static void MissionChangeObjectiveFlags(MissionIndexData *data, int d)
{
	struct MissionObjective *objective =
		&(*data->missionPtr)->objectives[data->index];
	// Max is combination of all flags, i.e. largest flag doubled less one
	objective->flags = CLAMP_OPPOSITE(
		objective->flags + d, 0, OBJECTIVE_NOACCESS * 2 - 1);
}


static UIObject *CreateCampaignObjs(void);
static UIObject *CreateMissionObjs(struct Mission **missionPtr);
static UIObject *CreateClassicMapObjs(Vec2i pos, struct Mission **missionPtr);
static UIObject *CreateWeaponObjs(struct Mission **missionPtr);
static UIObject *CreateMapItemObjs(struct Mission **missionPtr);
static UIObject *CreateCharacterObjs(struct Mission **missionPtr);
static UIObject *CreateSpecialCharacterObjs(struct Mission **missionPtr);
static UIObject *CreateObjectiveObjs(
	Vec2i pos, struct Mission **missionPtr, int index);

UIObject *CreateMainObjs(struct Mission **missionPtr)
{
	int th = CDogsTextHeight();
	UIObject *cc;
	UIObject *c;
	UIObject *o;
	UIObject *o2;
	UIObject *oc;
	int i;
	int x;
	int y;
	Vec2i objectivesPos;
	cc = UIObjectCreate(UITYPE_NONE, 0, Vec2iZero(), Vec2iZero());

	// Titles

	y = 5;

	o = UIObjectCreate(
		UITYPE_TEXTBOX, YC_CAMPAIGNTITLE, Vec2iNew(25, y), Vec2iNew(240, th));
	o->u.Textbox.TextLinkFunc = CampaignGetTitle;
	o->Data = &gCampaign;
	CSTRDUP(o->u.Textbox.Hint, "(Campaign title)");
	o->Flags = UI_SELECT_ONLY_FIRST;
	UIObjectAddChild(o, CreateCampaignObjs());
	UIObjectAddChild(cc, o);

	o = UIObjectCreate(
		UITYPE_NONE, YC_MISSIONINDEX, Vec2iNew(270, y), Vec2iNew(49, th));
	UIObjectAddChild(cc, o);

	y = 2 * th;

	// Mission-only controls
	// Only visible if the current mission is valid
	c = UIObjectCreate(UITYPE_CUSTOM, 0, Vec2iZero(), Vec2iZero());
	c->u.CustomDrawFunc = CheckMission;
	c->Data = missionPtr;
	UIObjectAddChild(cc, c);

	o = UIObjectCreate(
		UITYPE_TEXTBOX, YC_MISSIONTITLE, Vec2iNew(25, y), Vec2iNew(175, th));
	o->Id2 = XC_MISSIONTITLE;
	o->u.Textbox.TextLinkFunc = MissionGetTitle;
	o->Data = missionPtr;
	CSTRDUP(o->u.Textbox.Hint, "(Mission title)");
	UIObjectAddChild(o, CreateMissionObjs(missionPtr));
	UIObjectAddChild(c, o);

	// mission properties
	// size, walls, rooms etc.

	y = 10 + 2 * th;

	o = UIObjectCreate(
		UITYPE_LABEL, YC_MISSIONPROPS, Vec2iZero(), Vec2iNew(35, th));

	x = 20;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_WIDTH;
	o2->u.LabelFunc = MissionGetWidthStr;
	o2->Data = missionPtr;
	o2->ChangeFunc = MissionChangeWidth;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);
	x += 40;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_HEIGHT;
	o2->u.LabelFunc = MissionGetHeightStr;
	o2->Data = missionPtr;
	o2->ChangeFunc = MissionChangeHeight;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);

	// Properties for classic C-Dogs maps
	x = 20;
	y += th;
	UIObjectAddChild(c, CreateClassicMapObjs(Vec2iNew(x, y), missionPtr));

	// Mission looks
	// wall/floor styles etc.

	y += th;

	UIObjectDestroy(o);
	o = UIObjectCreate(
		UITYPE_CUSTOM, YC_MISSIONLOOKS, Vec2iZero(), Vec2iNew(25, 25 + th));

	x = 20;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_WALL;
	o2->u.CustomDrawFunc = MissionDrawWallStyle;
	o2->Data = missionPtr;
	o2->ChangeFunc = MissionChangeWallStyle;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);
	x += 30;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_FLOOR;
	o2->u.CustomDrawFunc = MissionDrawFloorStyle;
	o2->Data = missionPtr;
	o2->ChangeFunc = MissionChangeFloorStyle;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);
	x += 30;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_ROOM;
	o2->u.CustomDrawFunc = MissionDrawRoomStyle;
	o2->Data = missionPtr;
	o2->ChangeFunc = MissionChangeRoomStyle;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);
	x += 30;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_DOORS;
	o2->u.CustomDrawFunc = MissionDrawDoorStyle;
	o2->Data = missionPtr;
	o2->ChangeFunc = MissionChangeDoorStyle;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);
	x += 30;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_KEYS;
	o2->u.CustomDrawFunc = MissionDrawKeyStyle;
	o2->Data = missionPtr;
	o2->ChangeFunc = MissionChangeKeyStyle;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);
	x += 30;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_EXIT;
	o2->u.CustomDrawFunc = MissionDrawExitStyle;
	o2->Data = missionPtr;
	o2->ChangeFunc = MissionChangeExitStyle;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);

	// colours

	x = 200;

	UIObjectDestroy(o);
	o = UIObjectCreate(
		UITYPE_LABEL, YC_MISSIONLOOKS, Vec2iZero(), Vec2iNew(100, th));

	o2 = UIObjectCopy(o);
	o2->Id2 = XC_COLOR1;
	o2->u.LabelFunc = MissionGetWallColorStr;
	o2->Data = missionPtr;
	o2->ChangeFunc = MissionChangeWallColor;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);
	y += th;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_COLOR2;
	o2->u.LabelFunc = MissionGetFloorColorStr;
	o2->Data = missionPtr;
	o2->ChangeFunc = MissionChangeFloorColor;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);
	y += th;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_COLOR3;
	o2->u.LabelFunc = MissionGeRoomColorStr;
	o2->Data = missionPtr;
	o2->ChangeFunc = MissionChangeRoomColor;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);
	y += th;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_COLOR4;
	o2->u.LabelFunc = MissionGeExtraColorStr;
	o2->Data = missionPtr;
	o2->ChangeFunc = MissionChangeExtraColor;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);

	// mission data

	x = 20;
	y += th;

	UIObjectDestroy(o);
	o = UIObjectCreate(UITYPE_LABEL, 0, Vec2iZero(), Vec2iNew(189, th));
	o->Flags = UI_SELECT_ONLY;

	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = CStr;
	o2->Data = "Mission description";
	o2->Id = YC_MISSIONDESC;
	o2->Pos = Vec2iNew(x, y);
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

	y += th;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = MissionGetCharacterCountStr;
	o2->Data = missionPtr;
	o2->Id = YC_CHARACTERS;
	o2->Pos = Vec2iNew(x, y);
	CSTRDUP(o2->Tooltip, "Use Insert, Delete and PageUp/PageDown");
	UIObjectAddChild(o2, CreateCharacterObjs(missionPtr));
	UIObjectAddChild(c, o2);
	y += th;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = MissionGetSpecialCountStr;
	o2->Data = missionPtr;
	o2->Id = YC_SPECIALS;
	o2->Pos = Vec2iNew(x, y);
	CSTRDUP(o2->Tooltip, "Use Insert, Delete and PageUp/PageDown");
	UIObjectAddChild(o2, CreateSpecialCharacterObjs(missionPtr));
	UIObjectAddChild(c, o2);
	y += th;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = GetWeaponCountStr;
	o2->Data = NULL;
	o2->Id = YC_WEAPONS;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(o2, CreateWeaponObjs(missionPtr));
	UIObjectAddChild(c, o2);
	y += th;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = GetObjectCountStr;
	o2->Data = NULL;
	o2->Id = YC_ITEMS;
	o2->Pos = Vec2iNew(x, y);
	CSTRDUP(o2->Tooltip,
		"Use Insert, Delete and PageUp/PageDown\n"
		"Shift+click to change amounts");
	UIObjectAddChild(o2, CreateMapItemObjs(missionPtr));
	UIObjectAddChild(c, o2);

	// objectives
	y += 2;
	objectivesPos = Vec2iNew(x, y + 8 * th);
	UIObjectDestroy(o);
	o = UIObjectCreate(UITYPE_TEXTBOX, 0, Vec2iZero(), Vec2iNew(189, th));
	o->Flags = UI_SELECT_ONLY;

	for (i = 0; i < OBJECTIVE_MAX; i++)
	{
		y += th;
		o2 = UIObjectCopy(o);
		o2->Id = YC_OBJECTIVES + i;
		o2->Type = UITYPE_TEXTBOX;
		o2->u.Textbox.TextLinkFunc = MissionGetObjectiveDescription;
		o2->Data = missionPtr;
		CSTRDUP(o2->u.Textbox.Hint, "(Objective description)");
		o2->Pos = Vec2iNew(x, y);
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
static UIObject *CreateMissionObjs(struct Mission **missionPtr)
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
static UIObject *CreateClassicMapObjs(Vec2i pos, struct Mission **missionPtr)
{
	int th = CDogsTextHeight();
	UIObject *c = UIObjectCreate(UITYPE_NONE, 0, Vec2iZero(), Vec2iZero());
	UIObject *o = UIObjectCreate(
		UITYPE_LABEL, YC_MISSIONPROPS, Vec2iZero(), Vec2iNew(35, th));

	UIObject *o2 = UIObjectCopy(o);
	o2->Id2 = XC_WALLCOUNT;
	o2->u.LabelFunc = MissionGetWallCountStr;
	o2->Data = missionPtr;
	o2->ChangeFunc = MissionChangeWallCount;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.x += 40;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_WALLLENGTH;
	o2->u.LabelFunc = MissionGetWallLengthStr;
	o2->Data = missionPtr;
	o2->ChangeFunc = MissionChangeWallLength;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.x += 40;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_ROOMCOUNT;
	o2->u.LabelFunc = MissionGetRoomCountStr;
	o2->Data = missionPtr;
	o2->ChangeFunc = MissionChangeRoomCount;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.x += 40;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_SQRCOUNT;
	o2->u.LabelFunc = MissionGetSquareCountStr;
	o2->Data = missionPtr;
	o2->ChangeFunc = MissionChangeSquareCount;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.x += 40;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_DENSITY;
	o2->u.LabelFunc = MissionGetDensityStr;
	o2->Data = missionPtr;
	o2->ChangeFunc = MissionChangeDensity;
	o2->Pos = pos;
	CSTRDUP(o2->Tooltip, "Number of non-objective characters");
	UIObjectAddChild(c, o2);

	return c;
}
static UIObject *CreateWeaponObjs(struct Mission **missionPtr)
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
	for (i = 0; i < WEAPON_MAX; i++)
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
static UIObject *CreateMapItemObjs(struct Mission **missionPtr)
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
	for (i = 0; i < ITEMS_MAX; i++)
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
	Vec2i pos, struct Mission **missionPtr, int index)
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
static UIObject *CreateCharacterObjs(struct Mission **missionPtr)
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
static UIObject *CreateSpecialCharacterObjs(struct Mission **missionPtr)
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
