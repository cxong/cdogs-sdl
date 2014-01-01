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
static void MissionDrawEnemy(
	UIObject *o, GraphicsDevice *g, struct Mission **missionPtr)
{
	UNUSED(g);
	if (!*missionPtr) return;
	if (o->Id2 >= (*missionPtr)->baddieCount) return;
	DisplayCharacter(
		Vec2iAdd(o->Pos, Vec2iScaleDiv(o->Size, 2)),
		gCampaign.Setting.characters.baddies[o->Id2],
		UIObjectIsHighlighted(o), 1);
}
static void MissionDrawSpecialChar(
	UIObject *o, GraphicsDevice *g, struct Mission **missionPtr)
{
	UNUSED(g);
	if (!*missionPtr) return;
	if (o->Id2 >= (*missionPtr)->specialCount) return;
	DisplayCharacter(
		Vec2iAdd(o->Pos, Vec2iScaleDiv(o->Size, 2)),
		gCampaign.Setting.characters.specials[o->Id2],
		UIObjectIsHighlighted(o), 1);
}
static void DisplayMapItem(
	GraphicsDevice *g,
	Vec2i pos, TMapObject *mo, int density, int isHighlighted);
static void MissionDrawMapItem(
	UIObject *o, GraphicsDevice *g, struct Mission **missionPtr)
{
	if (!*missionPtr) return;
	if (o->Id2 >= (*missionPtr)->itemCount) return;
	DisplayMapItem(
		g,
		Vec2iAdd(o->Pos, Vec2iScaleDiv(o->Size, 2)),
		gMission.mapObjects[o->Id2],
		(*missionPtr)->itemDensity[o->Id2],
		UIObjectIsHighlighted(o));
}
static void MissionDrawWeaponStatus(
	UIObject *o, GraphicsDevice *g, struct Mission **missionPtr)
{
	if (!*missionPtr) return;
	DisplayFlag(
		g,
		o->Pos,
		gGunDescriptions[o->Id2].name,
		(*missionPtr)->weaponSelection & (1 << o->Id2),
		UIObjectIsHighlighted(o));
}
typedef struct
{
	struct Mission **missionPtr;
	int index;
} ObjectiveObjData;
static char *MissionGetObjectiveStr(UIObject *o, ObjectiveObjData *data)
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
	UIObject *o, GraphicsDevice *g, ObjectiveObjData *data)
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
static char *MissionGetObjectiveRequired(UIObject *o, ObjectiveObjData *data)
{
	static char s[128];
	UNUSED(o);
	if (!*data->missionPtr) return NULL;
	if ((*data->missionPtr)->objectiveCount <= data->index) return NULL;
	sprintf(s, "%d", (*data->missionPtr)->objectives[data->index].required);
	return s;
}
static char *MissionGetObjectiveTotal(UIObject *o, ObjectiveObjData *data)
{
	static char s[128];
	UNUSED(o);
	if (!*data->missionPtr) return NULL;
	if ((*data->missionPtr)->objectiveCount <= data->index) return NULL;
	sprintf(
		s, "out of %d", (*data->missionPtr)->objectives[data->index].count);
	return s;
}
static char *MissionGetObjectiveFlags(UIObject *o, ObjectiveObjData *data)
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


UIObject *CreateObjectiveObjs(struct Mission **missionPtr, int index);

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
	cc = UIObjectCreate(UITYPE_NONE, 0, Vec2iZero(), Vec2iZero());

	// Titles

	y = 5;

	o = UIObjectCreate(
		UITYPE_TEXTBOX, YC_CAMPAIGNTITLE, Vec2iNew(25, y), Vec2iNew(240, th));
	o->u.Textbox.TextLinkFunc = CampaignGetTitle;
	o->u.Textbox.TextLinkData = &gCampaign;
	CSTRDUP(o->u.Textbox.Hint, "(Campaign title)");
	o->Flags = UI_SELECT_ONLY_FIRST;
	UIObjectAddChild(cc, o);

	o = UIObjectCreate(
		UITYPE_NONE, YC_MISSIONINDEX, Vec2iNew(270, y), Vec2iNew(49, th));
	UIObjectAddChild(cc, o);

	y = 2 * th;

	// Mission-only controls
	// Only visible if the current mission is valid
	c = UIObjectCreate(UITYPE_CUSTOM, 0, Vec2iZero(), Vec2iZero());
	c->u.CustomDraw.DrawFunc = CheckMission;
	c->u.CustomDraw.DrawData = missionPtr;
	UIObjectAddChild(cc, c);

	o = UIObjectCreate(
		UITYPE_TEXTBOX, YC_MISSIONTITLE, Vec2iNew(25, y), Vec2iNew(175, th));
	o->Id2 = XC_MISSIONTITLE;
	o->u.Textbox.TextLinkFunc = MissionGetTitle;
	o->u.Textbox.TextLinkData = missionPtr;
	CSTRDUP(o->u.Textbox.Hint, "(Mission title)");
	UIObjectAddChild(c, o);

	// mission properties
	// size, walls, rooms etc.

	y = 10 + 2 * th;

	o = UIObjectCreate(
		UITYPE_LABEL, YC_MISSIONPROPS, Vec2iZero(), Vec2iNew(35, th));

	x = 20;
	o2 = UIObjectCopy(o);
	o2->u.Label.TextLinkFunc = MissionGetWidthStr;
	o2->u.Label.TextLinkData = missionPtr;
	o2->Id2 = XC_WIDTH;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);
	x += 40;
	o2 = UIObjectCopy(o);
	o2->u.Label.TextLinkFunc = MissionGetHeightStr;
	o2->u.Label.TextLinkData = missionPtr;
	o2->Id2 = XC_HEIGHT;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);
	x += 40;
	o2 = UIObjectCopy(o);
	o2->u.Label.TextLinkFunc = MissionGetWallCountStr;
	o2->u.Label.TextLinkData = missionPtr;
	o2->Id2 = XC_WALLCOUNT;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);
	x += 40;
	o2 = UIObjectCopy(o);
	o2->u.Label.TextLinkFunc = MissionGetWallLengthStr;
	o2->u.Label.TextLinkData = missionPtr;
	o2->Id2 = XC_WALLLENGTH;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);
	x += 40;
	o2 = UIObjectCopy(o);
	o2->u.Label.TextLinkFunc = MissionGetRoomCountStr;
	o2->u.Label.TextLinkData = missionPtr;
	o2->Id2 = XC_ROOMCOUNT;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);
	x += 40;
	o2 = UIObjectCopy(o);
	o2->u.Label.TextLinkFunc = MissionGetSquareCountStr;
	o2->u.Label.TextLinkData = missionPtr;
	o2->Id2 = XC_SQRCOUNT;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);
	x += 40;
	o2 = UIObjectCopy(o);
	o2->u.Label.TextLinkFunc = MissionGetDensityStr;
	o2->u.Label.TextLinkData = missionPtr;
	o2->Id2 = XC_DENSITY;
	o2->Pos = Vec2iNew(x, y);
	CSTRDUP(o2->Tooltip, "Number of non-objective characters");
	UIObjectAddChild(c, o2);

	// Mission looks
	// wall/floor styles etc.

	y += th;

	UIObjectDestroy(o);
	o = UIObjectCreate(
		UITYPE_CUSTOM, YC_MISSIONLOOKS, Vec2iZero(), Vec2iNew(25, 25 + th));

	x = 20;
	o2 = UIObjectCopy(o);
	o2->u.CustomDraw.DrawFunc = MissionDrawWallStyle;
	o2->u.CustomDraw.DrawData = missionPtr;
	o2->Id2 = XC_WALL;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);
	x += 30;
	o2 = UIObjectCopy(o);
	o2->u.CustomDraw.DrawFunc = MissionDrawFloorStyle;
	o2->u.CustomDraw.DrawData = missionPtr;
	o2->Id2 = XC_FLOOR;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);
	x += 30;
	o2 = UIObjectCopy(o);
	o2->u.CustomDraw.DrawFunc = MissionDrawRoomStyle;
	o2->u.CustomDraw.DrawData = missionPtr;
	o2->Id2 = XC_ROOM;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);
	x += 30;
	o2 = UIObjectCopy(o);
	o2->u.CustomDraw.DrawFunc = MissionDrawDoorStyle;
	o2->u.CustomDraw.DrawData = missionPtr;
	o2->Id2 = XC_DOORS;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);
	x += 30;
	o2 = UIObjectCopy(o);
	o2->u.CustomDraw.DrawFunc = MissionDrawKeyStyle;
	o2->u.CustomDraw.DrawData = missionPtr;
	o2->Id2 = XC_KEYS;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);
	x += 30;
	o2 = UIObjectCopy(o);
	o2->u.CustomDraw.DrawFunc = MissionDrawExitStyle;
	o2->u.CustomDraw.DrawData = missionPtr;
	o2->Id2 = XC_EXIT;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);

	// colours

	x = 200;

	UIObjectDestroy(o);
	o = UIObjectCreate(
		UITYPE_LABEL, YC_MISSIONLOOKS, Vec2iZero(), Vec2iNew(100, th));

	o2 = UIObjectCopy(o);
	o2->u.Label.TextLinkFunc = MissionGetWallColorStr;
	o2->u.Label.TextLinkData = missionPtr;
	o2->Id2 = XC_COLOR1;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);
	y += th;
	o2 = UIObjectCopy(o);
	o2->u.Label.TextLinkFunc = MissionGetFloorColorStr;
	o2->u.Label.TextLinkData = missionPtr;
	o2->Id2 = XC_COLOR2;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);
	y += th;
	o2 = UIObjectCopy(o);
	o2->u.Label.TextLinkFunc = MissionGeRoomColorStr;
	o2->u.Label.TextLinkData = missionPtr;
	o2->Id2 = XC_COLOR3;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);
	y += th;
	o2 = UIObjectCopy(o);
	o2->u.Label.TextLinkFunc = MissionGeExtraColorStr;
	o2->u.Label.TextLinkData = missionPtr;
	o2->Id2 = XC_COLOR4;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);

	// mission data

	x = 20;
	y += th;

	UIObjectDestroy(o);
	o = UIObjectCreate(UITYPE_LABEL, 0, Vec2iZero(), Vec2iNew(189, th));
	o->Flags = UI_SELECT_ONLY;

	o2 = UIObjectCopy(o);
	o2->u.Label.TextLinkFunc = CStr;
	o2->u.Label.TextLinkData = "Mission description";
	o2->Id = YC_MISSIONDESC;
	o2->Pos = Vec2iNew(x, y);
	o2->Size = TextGetSize(o2->u.Label.TextLinkData);
	oc = UIObjectCreate(
		UITYPE_TEXTBOX, YC_MISSIONDESC,
		Vec2iNew(25, 150), Vec2iNew(295, 5 * th));
	oc->Flags = UI_ENABLED_WHEN_PARENT_HIGHLIGHTED_ONLY;
	oc->u.Textbox.TextLinkFunc = MissionGetDescription;
	oc->u.Textbox.TextLinkData = missionPtr;
	CSTRDUP(oc->u.Textbox.Hint, "(Mission description)");
	UIObjectAddChild(o2, oc);
	UIObjectAddChild(c, o2);

	y += th;
	o2 = UIObjectCopy(o);
	o2->u.Label.TextLinkFunc = MissionGetCharacterCountStr;
	o2->u.Label.TextLinkData = missionPtr;
	o2->Id = YC_CHARACTERS;
	o2->Pos = Vec2iNew(x, y);
	CSTRDUP(o2->Tooltip, "Use Insert, Delete and PageUp/PageDown");
	UIObjectAddChild(c, o2);
	y += th;
	o2 = UIObjectCopy(o);
	o2->u.Label.TextLinkFunc = MissionGetSpecialCountStr;
	o2->u.Label.TextLinkData = missionPtr;
	o2->Id = YC_SPECIALS;
	o2->Pos = Vec2iNew(x, y);
	CSTRDUP(o2->Tooltip, "Use Insert, Delete and PageUp/PageDown");
	UIObjectAddChild(c, o2);
	y += th;
	o2 = UIObjectCopy(o);
	o2->u.Label.TextLinkFunc = GetWeaponCountStr;
	o2->u.Label.TextLinkData = NULL;
	o2->Id = YC_WEAPONS;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);
	y += th;
	o2 = UIObjectCopy(o);
	o2->u.Label.TextLinkFunc = GetObjectCountStr;
	o2->u.Label.TextLinkData = NULL;
	o2->Id = YC_ITEMS;
	o2->Pos = Vec2iNew(x, y);
	CSTRDUP(o2->Tooltip,
		"Use Insert, Delete and PageUp/PageDown\n"
		"Shift+click to change amounts");
	UIObjectAddChild(c, o2);

	// objectives
	y += 2;
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
		o2->u.Textbox.TextLinkData = missionPtr;
		CSTRDUP(o2->u.Textbox.Hint, "(Objective description)");
		o2->Pos = Vec2iNew(x, y);
		CSTRDUP(o2->Tooltip, "insert/delete: add/remove objective");
		UIObjectAddChild(o2, CreateObjectiveObjs(missionPtr, i));
		UIObjectAddChild(c, o2);
	}

	UIObjectDestroy(o);
	return cc;
}
UIObject *CreateCampaignObjs(void)
{
	int th = CDogsTextHeight();
	UIObject *c;
	UIObject *o;
	UIObject *o2;
	int x;
	int y;
	c = UIObjectCreate(UITYPE_NONE, 0, Vec2iZero(), Vec2iZero());

	x = 25;
	y = 150;

	o = UIObjectCreate(
		UITYPE_TEXTBOX, YC_CAMPAIGNTITLE, Vec2iZero(), Vec2iZero());
	o->Flags = UI_SELECT_ONLY;

	o2 = UIObjectCopy(o);
	o2->u.Textbox.TextLinkFunc = CampaignGetAuthor;
	o2->u.Textbox.TextLinkData = &gCampaign;
	CSTRDUP(o2->u.Textbox.Hint, "(Campaign author)");
	o2->Id2 = XC_AUTHOR;
	o2->Pos = Vec2iNew(x, y);
	o2->Size = Vec2iNew(295, th);
	UIObjectAddChild(c, o2);

	y += th;
	o2 = UIObjectCopy(o);
	o2->u.Textbox.TextLinkFunc = CampaignGetDescription;
	o2->u.Textbox.TextLinkData = &gCampaign;
	CSTRDUP(o2->u.Textbox.Hint, "(Campaign description)");
	o2->Id2 = XC_CAMPAIGNDESC;
	o2->Pos = Vec2iNew(x, y);
	o2->Size = Vec2iNew(295, 5 * th);
	UIObjectAddChild(c, o2);

	UIObjectDestroy(o);
	return c;
}
UIObject *CreateMissionObjs(struct Mission **missionPtr)
{
	int th = CDogsTextHeight();
	UIObject *c;
	UIObject *o;
	c = UIObjectCreate(UITYPE_NONE, 0, Vec2iZero(), Vec2iZero());

	o = UIObjectCreate(
		UITYPE_TEXTBOX, YC_MISSIONTITLE, Vec2iNew(20, 150), Vec2iNew(319, th));
	o->u.Textbox.TextLinkFunc = MissionGetSong;
	o->u.Textbox.TextLinkData = missionPtr;
	CSTRDUP(o->u.Textbox.Hint, "(Mission song)");
	o->Id2 = XC_MUSICFILE;
	o->Flags = UI_SELECT_ONLY;
	UIObjectAddChild(c, o);

	return c;
}
UIObject *CreateWeaponObjs(struct Mission **missionPtr)
{
	int th = CDogsTextHeight();
	UIObject *c;
	UIObject *o;
	UIObject *o2;
	int i;
	c = UIObjectCreate(UITYPE_NONE, 0, Vec2iZero(), Vec2iZero());

	o = UIObjectCreate(UITYPE_CUSTOM, 0, Vec2iZero(), Vec2iNew(80, th));
	o->u.CustomDraw.DrawFunc = MissionDrawWeaponStatus;
	o->u.CustomDraw.DrawData = missionPtr;
	o->Flags = UI_LEAVE_YC;
	for (i = 0; i < WEAPON_MAX; i++)
	{
		int x = 10 + i / 4 * 90;
		int y = 150 + (i % 4) * th;
		o2 = UIObjectCopy(o);
		o2->Id2 = i;
		o2->Pos = Vec2iNew(x, y);
		UIObjectAddChild(c, o2);
	}

	UIObjectDestroy(o);
	return c;
}
UIObject *CreateMapItemObjs(struct Mission **missionPtr)
{
	UIObject *c;
	UIObject *o;
	UIObject *o2;
	int i;
	c = UIObjectCreate(UITYPE_NONE, 0, Vec2iZero(), Vec2iZero());

	o = UIObjectCreate(UITYPE_CUSTOM, 0, Vec2iZero(), Vec2iNew(20, 40));
	o->u.CustomDraw.DrawFunc = MissionDrawMapItem;
	o->u.CustomDraw.DrawData = missionPtr;
	o->Flags = UI_LEAVE_YC;
	for (i = 0; i < ITEMS_MAX; i++)
	{
		int x = 10 + i * 20;
		o2 = UIObjectCopy(o);
		o2->Id2 = i;
		o2->Pos = Vec2iNew(x, 150);
		UIObjectAddChild(c, o2);
	}

	UIObjectDestroy(o);
	return c;
}
UIObject *CreateObjectiveObjs(struct Mission **missionPtr, int index)
{
	int th = CDogsTextHeight();
	UIObject *c;
	UIObject *o;
	UIObject *o2;
	int x;
	int y;
	c = UIObjectCreate(UITYPE_NONE, 0, Vec2iZero(), Vec2iZero());
	c->Flags = UI_ENABLED_WHEN_PARENT_HIGHLIGHTED_ONLY;

	o = UIObjectCreate(UITYPE_NONE, 0, Vec2iZero(), Vec2iZero());
	o->Flags = UI_LEAVE_YC;
	y = 150;

	x = 20;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_TYPE;
	o2->Type = UITYPE_LABEL;
	o2->u.Label.TextLinkFunc = MissionGetObjectiveStr;
	CMALLOC(o2->u.Label.TextLinkData, sizeof(ObjectiveObjData));
	o2->u.Label.IsDynamicData = 1;
	((ObjectiveObjData *)o2->u.Label.TextLinkData)->missionPtr = missionPtr;
	((ObjectiveObjData *)o2->u.Label.TextLinkData)->index = index;
	o2->Pos = Vec2iNew(x, y);
	o2->Size = Vec2iNew(35, th);
	UIObjectAddChild(c, o2);
	x += 40;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_INDEX;
	o2->Type = UITYPE_CUSTOM;
	o2->u.CustomDraw.DrawFunc = MissionDrawObjective;
	CMALLOC(o2->u.CustomDraw.DrawData, sizeof(ObjectiveObjData));
	o2->u.CustomDraw.IsDynamicData = 1;
	((ObjectiveObjData *)o2->u.CustomDraw.DrawData)->missionPtr = missionPtr;
	((ObjectiveObjData *)o2->u.CustomDraw.DrawData)->index = index;
	o2->Pos = Vec2iNew(x, y);
	o2->Size = Vec2iNew(30, th);
	UIObjectAddChild(c, o2);
	x += 30;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_REQUIRED;
	o2->Type = UITYPE_LABEL;
	o2->u.Label.TextLinkFunc = MissionGetObjectiveRequired;
	CMALLOC(o2->u.Label.TextLinkData, sizeof(ObjectiveObjData));
	o2->u.Label.IsDynamicData = 1;
	((ObjectiveObjData *)o2->u.Label.TextLinkData)->missionPtr = missionPtr;
	((ObjectiveObjData *)o2->u.Label.TextLinkData)->index = index;
	o2->Pos = Vec2iNew(x, y);
	o2->Size = Vec2iNew(20, th);
	CSTRDUP(o2->Tooltip, "0: optional objective");
	UIObjectAddChild(c, o2);
	x += 20;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_TOTAL;
	o2->Type = UITYPE_LABEL;
	o2->u.Label.TextLinkFunc = MissionGetObjectiveTotal;
	CMALLOC(o2->u.Label.TextLinkData, sizeof(ObjectiveObjData));
	o2->u.Label.IsDynamicData = 1;
	((ObjectiveObjData *)o2->u.Label.TextLinkData)->missionPtr = missionPtr;
	((ObjectiveObjData *)o2->u.Label.TextLinkData)->index = index;
	o2->Pos = Vec2iNew(x, y);
	o2->Size = Vec2iNew(35, th);
	UIObjectAddChild(c, o2);
	x += 40;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_FLAGS;
	o2->Type = UITYPE_LABEL;
	o2->u.Label.TextLinkFunc = MissionGetObjectiveFlags;
	CMALLOC(o2->u.Label.TextLinkData, sizeof(ObjectiveObjData));
	o2->u.Label.IsDynamicData = 1;
	((ObjectiveObjData *)o2->u.Label.TextLinkData)->missionPtr = missionPtr;
	((ObjectiveObjData *)o2->u.Label.TextLinkData)->index = index;
	o2->Pos = Vec2iNew(x, y);
	o2->Size = Vec2iNew(100, th);
	CSTRDUP(o2->Tooltip,
		"hidden: not shown on map\n"
		"pos.known: always shown on map\n"
		"access: in locked room\n"
		"no-count: don't show completed count");
	UIObjectAddChild(c, o2);

	UIObjectDestroy(o);
	return c;
}
UIObject *CreateCharacterObjs(struct Mission **missionPtr)
{
	UIObject *c;
	UIObject *o;
	UIObject *o2;
	int i;
	c = UIObjectCreate(UITYPE_NONE, 0, Vec2iZero(), Vec2iZero());

	o = UIObjectCreate(UITYPE_CUSTOM, 0, Vec2iZero(), Vec2iNew(20, 40));
	o->u.CustomDraw.DrawFunc = MissionDrawEnemy;
	o->u.CustomDraw.DrawData = missionPtr;
	o->Flags = UI_LEAVE_YC | UI_SELECT_ONLY_FIRST;
	for (i = 0; i < 15; i++)
	{
		int x = 10 + i * 20;
		o2 = UIObjectCopy(o);
		o2->Id2 = i;
		o2->Pos = Vec2iNew(x, 150);
		UIObjectAddChild(c, o2);
	}

	UIObjectDestroy(o);
	return c;
}
UIObject *CreateSpecialCharacterObjs(struct Mission **missionPtr)
{
	UIObject *c;
	UIObject *o;
	UIObject *o2;
	int i;
	c = UIObjectCreate(UITYPE_NONE, 0, Vec2iZero(), Vec2iZero());

	o = UIObjectCreate(UITYPE_CUSTOM, 0, Vec2iZero(), Vec2iNew(20, 40));
	o->u.CustomDraw.DrawFunc = MissionDrawSpecialChar;
	o->u.CustomDraw.DrawData = missionPtr;
	o->Flags = UI_LEAVE_YC | UI_SELECT_ONLY_FIRST;
	for (i = 0; i < 15; i++)
	{
		int x = 10 + i * 20;
		o2 = UIObjectCopy(o);
		o2->Id2 = i;
		o2->Pos = Vec2iNew(x, 150);
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
