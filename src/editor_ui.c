/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (c) 2013, Cong Xu
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

#include <cdogs/mission.h>
#include <cdogs/text.h>


static char *CStr(char *s)
{
	return s;
}
static char *CampaignGetTitle(CampaignOptions *c)
{
	return c->Setting.title;
}
static char *CampaignGetAuthor(CampaignOptions *c)
{
	return c->Setting.author;
}
static char *CampaignGetDescription(CampaignOptions *c)
{
	return c->Setting.description;
}
static char *MissionGetTitle(struct Mission **missionPtr)
{
	if (!*missionPtr) return NULL;
	return (*missionPtr)->title;
}
static char *MissionGetDescription(struct Mission **missionPtr)
{
	if (!*missionPtr) return NULL;
	return (*missionPtr)->description;
}
static char *MissionGetSong(struct Mission **missionPtr)
{
	if (!*missionPtr) return NULL;
	return (*missionPtr)->song;
}
static char *MissionGetWidthStr(struct Mission **missionPtr)
{
	static char s[128];
	if (!*missionPtr) return NULL;
	sprintf(s, "Width: %d", (*missionPtr)->mapWidth);
	return s;
}
static char *MissionGetHeightStr(struct Mission **missionPtr)
{
	static char s[128];
	if (!*missionPtr) return NULL;
	sprintf(s, "Height: %d", (*missionPtr)->mapHeight);
	return s;
}
static char *MissionGetWallCountStr(struct Mission **missionPtr)
{
	static char s[128];
	if (!*missionPtr) return NULL;
	sprintf(s, "Walls: %d", (*missionPtr)->wallCount);
	return s;
}
static char *MissionGetWallLengthStr(struct Mission **missionPtr)
{
	static char s[128];
	if (!*missionPtr) return NULL;
	sprintf(s, "Len: %d", (*missionPtr)->wallLength);
	return s;
}
static char *MissionGetRoomCountStr(struct Mission **missionPtr)
{
	static char s[128];
	if (!*missionPtr) return NULL;
	sprintf(s, "Rooms: %d", (*missionPtr)->roomCount);
	return s;
}
static char *MissionGetSquareCountStr(struct Mission **missionPtr)
{
	static char s[128];
	if (!*missionPtr) return NULL;
	sprintf(s, "Sqr: %d", (*missionPtr)->squareCount);
	return s;
}
static char *MissionGetDensityStr(struct Mission **missionPtr)
{
	static char s[128];
	if (!*missionPtr) return NULL;
	sprintf(s, "Dens: %d", (*missionPtr)->baddieDensity);
	return s;
}
static char *MissionGetWallColorStr(struct Mission **missionPtr)
{
	static char s[128];
	if (!*missionPtr) return NULL;
	sprintf(s, "Walls: %s", RangeName((*missionPtr)->wallRange));
	return s;
}
static char *MissionGetFloorColorStr(struct Mission **missionPtr)
{
	static char s[128];
	if (!*missionPtr) return NULL;
	sprintf(s, "Floor: %s", RangeName((*missionPtr)->floorRange));
	return s;
}
static char *MissionGeRoomColorStr(struct Mission **missionPtr)
{
	static char s[128];
	if (!*missionPtr) return NULL;
	sprintf(s, "Rooms: %s", RangeName((*missionPtr)->roomRange));
	return s;
}
static char *MissionGeExtraColorStr(struct Mission **missionPtr)
{
	static char s[128];
	if (!*missionPtr) return NULL;
	sprintf(s, "Extra: %s", RangeName((*missionPtr)->altRange));
	return s;
}
static char *MissionGetCharacterCountStr(struct Mission **missionPtr)
{
	static char s[128];
	if (!*missionPtr) return NULL;
	sprintf(s, "Characters (%d/%d)", (*missionPtr)->baddieCount, BADDIE_MAX);
	return s;
}
static char *MissionGetSpecialCountStr(struct Mission **missionPtr)
{
	static char s[128];
	if (!*missionPtr) return NULL;
	sprintf(
		s, "Mission objective characters (%d/%d)",
		(*missionPtr)->specialCount, SPECIAL_MAX);
	return s;
}
static char *GetWeaponCountStr(void *v)
{
	static char s[128];
	UNUSED(v);
	sprintf(s, "Available weapons (%d/%d)", gMission.weaponCount, WEAPON_MAX);
	return s;
}
static char *GetObjectCountStr(void *v)
{
	static char s[128];
	UNUSED(v);
	sprintf(s, "Map items (%d/%d)", gMission.objectCount, ITEMS_MAX);
	return s;
}

UIObject *CreateMainObjs(struct Mission **missionPtr)
{
	int th = CDogsTextHeight();
	UIObject *c;
	UIObject *o;
	UIObject *o2;
	UIObject *oc;
	int i;
	int x;
	int y;
	c = UIObjectCreate(0, Vec2iZero(), Vec2iZero());

	// Titles

	y = 5;

	o = UIObjectCreate(YC_CAMPAIGNTITLE, Vec2iNew(25, y), Vec2iNew(240, th));
	o->Type = UITYPE_TEXTBOX;
	o->u.Textbox.TextLinkFunc = CampaignGetTitle;
	o->u.Textbox.TextLinkData = &gCampaign;
	CSTRDUP(o->u.Textbox.Hint, "(Campaign title)");
	o->Flags = UI_SELECT_ONLY_FIRST;
	UIObjectAddChild(c, o);

	o = UIObjectCreate(YC_MISSIONINDEX, Vec2iNew(270, y), Vec2iNew(49, th));
	UIObjectAddChild(c, o);

	y = 2 * th;

	o = UIObjectCreate(YC_MISSIONTITLE, Vec2iNew(25, y), Vec2iNew(175, th));
	o->Id2 = XC_MISSIONTITLE;
	o->Type = UITYPE_TEXTBOX;
	o->u.Textbox.TextLinkFunc = MissionGetTitle;
	o->u.Textbox.TextLinkData = missionPtr;
	CSTRDUP(o->u.Textbox.Hint, "(Mission title)");
	UIObjectAddChild(c, o);

	// mission properties
	// size, walls, rooms etc.

	y = 10 + 2 * th;

	o = UIObjectCreate(YC_MISSIONPROPS, Vec2iZero(), Vec2iNew(35, th));

	x = 20;
	o2 = UIObjectCopy(o);
	o2->Type = UITYPE_LABEL;
	o2->u.Label.TextLinkFunc = MissionGetWidthStr;
	o2->u.Label.TextLinkData = missionPtr;
	o2->Id2 = XC_WIDTH;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);
	x += 40;
	o2 = UIObjectCopy(o);
	o2->Type = UITYPE_LABEL;
	o2->u.Label.TextLinkFunc = MissionGetHeightStr;
	o2->u.Label.TextLinkData = missionPtr;
	o2->Id2 = XC_HEIGHT;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);
	x += 40;
	o2 = UIObjectCopy(o);
	o2->Type = UITYPE_LABEL;
	o2->u.Label.TextLinkFunc = MissionGetWallCountStr;
	o2->u.Label.TextLinkData = missionPtr;
	o2->Id2 = XC_WALLCOUNT;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);
	x += 40;
	o2 = UIObjectCopy(o);
	o2->Type = UITYPE_LABEL;
	o2->u.Label.TextLinkFunc = MissionGetWallLengthStr;
	o2->u.Label.TextLinkData = missionPtr;
	o2->Id2 = XC_WALLLENGTH;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);
	x += 40;
	o2 = UIObjectCopy(o);
	o2->Type = UITYPE_LABEL;
	o2->u.Label.TextLinkFunc = MissionGetRoomCountStr;
	o2->u.Label.TextLinkData = missionPtr;
	o2->Id2 = XC_ROOMCOUNT;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);
	x += 40;
	o2 = UIObjectCopy(o);
	o2->Type = UITYPE_LABEL;
	o2->u.Label.TextLinkFunc = MissionGetSquareCountStr;
	o2->u.Label.TextLinkData = missionPtr;
	o2->Id2 = XC_SQRCOUNT;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);
	x += 40;
	o2 = UIObjectCopy(o);
	o2->Type = UITYPE_LABEL;
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
	o = UIObjectCreate(YC_MISSIONLOOKS, Vec2iZero(), Vec2iNew(25, 25 + th));

	x = 20;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_WALL;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);
	x += 30;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_FLOOR;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);
	x += 30;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_ROOM;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);
	x += 30;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_DOORS;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);
	x += 30;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_KEYS;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);
	x += 30;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_EXIT;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);

	// colours

	x = 200;

	UIObjectDestroy(o);
	o = UIObjectCreate(YC_MISSIONLOOKS, Vec2iZero(), Vec2iNew(100, th));

	o2 = UIObjectCopy(o);
	o2->Type = UITYPE_LABEL;
	o2->u.Label.TextLinkFunc = MissionGetWallColorStr;
	o2->u.Label.TextLinkData = missionPtr;
	o2->Id2 = XC_COLOR1;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);
	y += th;
	o2 = UIObjectCopy(o);
	o2->Type = UITYPE_LABEL;
	o2->u.Label.TextLinkFunc = MissionGetFloorColorStr;
	o2->u.Label.TextLinkData = missionPtr;
	o2->Id2 = XC_COLOR2;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);
	y += th;
	o2 = UIObjectCopy(o);
	o2->Type = UITYPE_LABEL;
	o2->u.Label.TextLinkFunc = MissionGeRoomColorStr;
	o2->u.Label.TextLinkData = missionPtr;
	o2->Id2 = XC_COLOR3;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);
	y += th;
	o2 = UIObjectCopy(o);
	o2->Type = UITYPE_LABEL;
	o2->u.Label.TextLinkFunc = MissionGeExtraColorStr;
	o2->u.Label.TextLinkData = missionPtr;
	o2->Id2 = XC_COLOR4;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);

	// mission data

	x = 20;
	y += th;

	UIObjectDestroy(o);
	o = UIObjectCreate(0, Vec2iZero(), Vec2iNew(189, th));
	o->Flags = UI_SELECT_ONLY;

	o2 = UIObjectCopy(o);
	o2->Type = UITYPE_LABEL;
	o2->u.Label.TextLinkFunc = CStr;
	o2->u.Label.TextLinkData = "Mission description";
	o2->Id = YC_MISSIONDESC;
	o2->Pos = Vec2iNew(x, y);
	o2->Size = TextGetSize(o2->u.Label.TextLinkData);
	oc = UIObjectCreate(YC_MISSIONDESC, Vec2iNew(25, 150), Vec2iNew(295, 5 * th));
	oc->Type = UITYPE_TEXTBOX;
	oc->Flags = UI_ENABLED_WHEN_PARENT_HIGHLIGHTED_ONLY;
	oc->u.Textbox.TextLinkFunc = MissionGetDescription;
	oc->u.Textbox.TextLinkData = missionPtr;
	CSTRDUP(oc->u.Textbox.Hint, "(Mission description)");
	UIObjectAddChild(o2, oc);
	UIObjectAddChild(c, o2);

	y += th;
	o2 = UIObjectCopy(o);
	o2->Type = UITYPE_LABEL;
	o2->u.Label.TextLinkFunc = MissionGetCharacterCountStr;
	o2->u.Label.TextLinkData = missionPtr;
	o2->Id = YC_CHARACTERS;
	o2->Pos = Vec2iNew(x, y);
	CSTRDUP(o2->Tooltip, "Use Insert, Delete and PageUp/PageDown");
	UIObjectAddChild(c, o2);
	y += th;
	o2 = UIObjectCopy(o);
	o2->Type = UITYPE_LABEL;
	o2->u.Label.TextLinkFunc = MissionGetSpecialCountStr;
	o2->u.Label.TextLinkData = missionPtr;
	o2->Id = YC_SPECIALS;
	o2->Pos = Vec2iNew(x, y);
	CSTRDUP(o2->Tooltip, "Use Insert, Delete and PageUp/PageDown");
	UIObjectAddChild(c, o2);
	y += th;
	o2 = UIObjectCopy(o);
	o2->Type = UITYPE_LABEL;
	o2->u.Label.TextLinkFunc = GetWeaponCountStr;
	o2->u.Label.TextLinkData = NULL;
	o2->Id = YC_WEAPONS;
	o2->Pos = Vec2iNew(x, y);
	UIObjectAddChild(c, o2);
	y += th;
	o2 = UIObjectCopy(o);
	o2->Type = UITYPE_LABEL;
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

	for (i = 0; i < OBJECTIVE_MAX; i++)
	{
		y += th;
		o2 = UIObjectCopy(o);
		o2->Id = YC_OBJECTIVES + i;
		o2->Pos = Vec2iNew(x, y);
		CSTRDUP(o2->Tooltip, "insert/delete: add/remove objective");
		UIObjectAddChild(c, o2);
	}

	UIObjectDestroy(o);
	return c;
}
UIObject *CreateCampaignObjs(void)
{
	int th = CDogsTextHeight();
	UIObject *c;
	UIObject *o;
	UIObject *o2;
	int x;
	int y;
	c = UIObjectCreate(0, Vec2iZero(), Vec2iZero());

	x = 25;
	y = 150;

	o = UIObjectCreate(YC_CAMPAIGNTITLE, Vec2iZero(), Vec2iZero());
	o->Flags = UI_SELECT_ONLY;

	o2 = UIObjectCopy(o);
	o2->Type = UITYPE_TEXTBOX;
	o2->u.Textbox.TextLinkFunc = CampaignGetAuthor;
	o2->u.Textbox.TextLinkData = &gCampaign;
	CSTRDUP(o2->u.Textbox.Hint, "(Campaign author)");
	o2->Id2 = XC_AUTHOR;
	o2->Pos = Vec2iNew(x, y);
	o2->Size = Vec2iNew(295, th);
	UIObjectAddChild(c, o2);

	y += th;
	o2 = UIObjectCopy(o);
	o2->Type = UITYPE_TEXTBOX;
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
	c = UIObjectCreate(0, Vec2iZero(), Vec2iZero());

	o = UIObjectCreate(YC_MISSIONTITLE, Vec2iNew(20, 150), Vec2iNew(319, th));
	o->Type = UITYPE_TEXTBOX;
	o->u.Textbox.TextLinkFunc = MissionGetSong;
	o->u.Textbox.TextLinkData = missionPtr;
	CSTRDUP(o->u.Textbox.Hint, "(Mission song)");
	o->Id2 = XC_MUSICFILE;
	o->Flags = UI_SELECT_ONLY;
	UIObjectAddChild(c, o);

	return c;
}
UIObject *CreateWeaponObjs(void)
{
	int th = CDogsTextHeight();
	UIObject *c;
	UIObject *o;
	UIObject *o2;
	int i;
	c = UIObjectCreate(0, Vec2iZero(), Vec2iZero());

	o = UIObjectCreate(0, Vec2iZero(), Vec2iNew(80, th));
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
UIObject *CreateMapItemObjs(void)
{
	UIObject *c;
	UIObject *o;
	UIObject *o2;
	int i;
	c = UIObjectCreate(0, Vec2iZero(), Vec2iZero());

	o = UIObjectCreate(0, Vec2iZero(), Vec2iNew(19, 40));
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
UIObject *CreateObjectiveObjs(void)
{
	UIObject *c;
	UIObject *o;
	UIObject *o2;
	int x;
	int y;
	c = UIObjectCreate(0, Vec2iZero(), Vec2iZero());

	o = UIObjectCreate(0, Vec2iZero(), Vec2iZero());
	o->Flags = UI_LEAVE_YC;
	y = 150;

	x = 20;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_TYPE;
	o2->Pos = Vec2iNew(x, y);
	o2->Size = Vec2iNew(35, 40);
	UIObjectAddChild(c, o2);
	x += 40;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_INDEX;
	o2->Pos = Vec2iNew(x, y);
	o2->Size = Vec2iNew(30, 40);
	UIObjectAddChild(c, o2);
	x += 30;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_REQUIRED;
	o2->Pos = Vec2iNew(x, y);
	o2->Size = Vec2iNew(20, 40);
	CSTRDUP(o2->Tooltip, "0: optional objective");
	UIObjectAddChild(c, o2);
	x += 20;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_TOTAL;
	o2->Pos = Vec2iNew(x, y);
	o2->Size = Vec2iNew(35, 40);
	UIObjectAddChild(c, o2);
	x += 40;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_FLAGS;
	o2->Pos = Vec2iNew(x, y);
	o2->Size = Vec2iNew(100, 40);
	CSTRDUP(o2->Tooltip,
		"hidden: not shown on map\n"
		"pos.known: always shown on map\n"
		"access: in locked room\n"
		"no-count: don't show completed count");
	UIObjectAddChild(c, o2);

	UIObjectDestroy(o);
	return c;
}
UIObject *CreateCharacterObjs(void)
{
	UIObject *c;
	UIObject *o;
	UIObject *o2;
	int i;
	c = UIObjectCreate(0, Vec2iZero(), Vec2iZero());

	o = UIObjectCreate(0, Vec2iZero(), Vec2iNew(20, 40));
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
	c = UIObjectCreate(0, Vec2iZero(), Vec2iZero());

	// Appearance

	y = 10;
	o = UIObjectCreate(YC_APPEARANCE, Vec2iZero(), Vec2iNew(40, th));

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
	o = UIObjectCreate(YC_ATTRIBUTES, Vec2iZero(), Vec2iNew(40, th));

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
	o = UIObjectCreate(YC_FLAGS, Vec2iZero(), Vec2iNew(40, th));

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
	o = UIObjectCreate(YC_WEAPON, Vec2iNew(50, y), Vec2iNew(210, th));
	UIObjectAddChild(c, o);

	return c;
}
