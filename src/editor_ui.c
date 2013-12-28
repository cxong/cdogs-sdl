/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (C) 1995 Ronny Wester
    Copyright (C) 2003 Jeremy Chin
    Copyright (C) 2003 Lucas Martin-King

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    This file incorporates work covered by the following copyright and
    permission notice:

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

#include <cdogs/text.h>


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
static char *MissionGetDescription(struct Mission **missionPtr)
{
	if (!*missionPtr)
	{
		return NULL;
	}
	return (*missionPtr)->description;
}

UICollection CreateMainObjs(struct Mission **missionPtr)
{
	int th = CDogsTextHeight();
	UICollection c;
	UIObject *o;
	UIObject *o2;
	UIObject *oc;
	int i;
	int x;
	int y;
	CArrayInit(&c.Objs, sizeof o);

	// Titles

	y = 5;

	o = UIObjectCreate(YC_CAMPAIGNTITLE, Vec2iNew(25, y), Vec2iNew(240, th));
	o->Type = UITYPE_TEXTBOX;
	o->u.Textbox.TextLinkFunc = CampaignGetTitle;
	o->u.Textbox.TextLinkData = &gCampaign;
	CSTRDUP(o->u.Textbox.Hint, "(Campaign title)");
	o->Flags = UI_SELECT_ONLY_FIRST;
	CArrayPushBack(&c.Objs, &o);

	o = UIObjectCreate(YC_MISSIONINDEX, Vec2iNew(270, y), Vec2iNew(49, th));
	CArrayPushBack(&c.Objs, &o);

	y = 2 * th;

	o = UIObjectCreate(YC_MISSIONTITLE, Vec2iNew(25, y), Vec2iNew(175, th));
	CArrayPushBack(&c.Objs, &o);

	// mission properties
	// size, walls, rooms etc.

	y = 10 + 2 * th;

	o = UIObjectCreate(YC_MISSIONPROPS, Vec2iZero(), Vec2iNew(35, th));

	x = 20;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_WIDTH;
	o2->Pos = Vec2iNew(x, y);
	CArrayPushBack(&c.Objs, &o2);
	x += 40;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_HEIGHT;
	o2->Pos = Vec2iNew(x, y);
	CArrayPushBack(&c.Objs, &o2);
	x += 40;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_WALLCOUNT;
	o2->Pos = Vec2iNew(x, y);
	CArrayPushBack(&c.Objs, &o2);
	x += 40;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_WALLLENGTH;
	o2->Pos = Vec2iNew(x, y);
	CArrayPushBack(&c.Objs, &o2);
	x += 40;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_ROOMCOUNT;
	o2->Pos = Vec2iNew(x, y);
	CArrayPushBack(&c.Objs, &o2);
	x += 40;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_SQRCOUNT;
	o2->Pos = Vec2iNew(x, y);
	CArrayPushBack(&c.Objs, &o2);
	x += 40;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_DENSITY;
	o2->Pos = Vec2iNew(x, y);
	CSTRDUP(o2->Tooltip, "Number of non-objective characters");
	CArrayPushBack(&c.Objs, &o2);

	// Mission looks
	// wall/floor styles etc.

	y += th;

	UIObjectDestroy(o);
	o = UIObjectCreate(YC_MISSIONLOOKS, Vec2iZero(), Vec2iNew(25, 25 + th));

	x = 20;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_WALL;
	o2->Pos = Vec2iNew(x, y);
	CArrayPushBack(&c.Objs, &o2);
	x += 30;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_FLOOR;
	o2->Pos = Vec2iNew(x, y);
	CArrayPushBack(&c.Objs, &o2);
	x += 30;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_ROOM;
	o2->Pos = Vec2iNew(x, y);
	CArrayPushBack(&c.Objs, &o2);
	x += 30;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_DOORS;
	o2->Pos = Vec2iNew(x, y);
	CArrayPushBack(&c.Objs, &o2);
	x += 30;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_KEYS;
	o2->Pos = Vec2iNew(x, y);
	CArrayPushBack(&c.Objs, &o2);
	x += 30;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_EXIT;
	o2->Pos = Vec2iNew(x, y);
	CArrayPushBack(&c.Objs, &o2);

	// colours

	x = 200;

	UIObjectDestroy(o);
	o = UIObjectCreate(YC_MISSIONLOOKS, Vec2iZero(), Vec2iNew(100, th));

	o2 = UIObjectCopy(o);
	o2->Id2 = XC_COLOR1;
	o2->Pos = Vec2iNew(x, y);
	CArrayPushBack(&c.Objs, &o2);
	y += th;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_COLOR2;
	o2->Pos = Vec2iNew(x, y);
	CArrayPushBack(&c.Objs, &o2);
	y += th;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_COLOR3;
	o2->Pos = Vec2iNew(x, y);
	CArrayPushBack(&c.Objs, &o2);
	y += th;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_COLOR4;
	o2->Pos = Vec2iNew(x, y);
	CArrayPushBack(&c.Objs, &o2);

	// mission data

	x = 10;
	y += th;

	UIObjectDestroy(o);
	o = UIObjectCreate(0, Vec2iZero(), Vec2iNew(189, th));
	o->Flags = UI_SELECT_ONLY;

	o2 = UIObjectCopy(o);
	o2->Id = YC_MISSIONDESC;
	o2->Pos = Vec2iNew(x, y);
	oc = UIObjectCreate(0, Vec2iNew(25, 150), Vec2iNew(295, 5 * th));
	oc->Type = UITYPE_TEXTBOX;
	oc->u.Textbox.TextLinkFunc = MissionGetDescription;
	oc->u.Textbox.TextLinkData = missionPtr;
	CSTRDUP(oc->u.Textbox.Hint, "(Mission description)");
	CArrayInit(&o2->Children.Objs, sizeof o);
	CArrayPushBack(&o2->Children.Objs, &oc);
	CArrayPushBack(&c.Objs, &o2);

	y += th;
	o2 = UIObjectCopy(o);
	o2->Id = YC_CHARACTERS;
	o2->Pos = Vec2iNew(x, y);
	CArrayPushBack(&c.Objs, &o2);
	y += th;
	o2 = UIObjectCopy(o);
	o2->Id = YC_SPECIALS;
	o2->Pos = Vec2iNew(x, y);
	CArrayPushBack(&c.Objs, &o2);
	y += th;
	o2 = UIObjectCopy(o);
	o2->Id = YC_WEAPONS;
	o2->Pos = Vec2iNew(x, y);
	CArrayPushBack(&c.Objs, &o2);
	y += th;
	o2 = UIObjectCopy(o);
	o2->Id = YC_ITEMS;
	o2->Pos = Vec2iNew(x, y);
	CSTRDUP(o2->Tooltip, "Shift+click to change amounts");
	CArrayPushBack(&c.Objs, &o2);

	// objectives
	y += 2;

	for (i = 0; i < OBJECTIVE_MAX; i++)
	{
		y += th;
		o2 = UIObjectCopy(o);
		o2->Id = YC_OBJECTIVES + i;
		o2->Pos = Vec2iNew(x, y);
		CSTRDUP(o2->Tooltip, "insert/delete: add/remove objective");
		CArrayPushBack(&c.Objs, &o2);
	}

	UIObjectDestroy(o);
	return c;
}
UICollection CreateCampaignObjs(void)
{
	int th = CDogsTextHeight();
	UICollection c;
	UIObject *o;
	UIObject *o2;
	int x;
	int y;
	CArrayInit(&c.Objs, sizeof o);

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
	CArrayPushBack(&c.Objs, &o2);

	y += th;
	o2 = UIObjectCopy(o);
	o2->Type = UITYPE_TEXTBOX;
	o2->u.Textbox.TextLinkFunc = CampaignGetDescription;
	o2->u.Textbox.TextLinkData = &gCampaign;
	CSTRDUP(o2->u.Textbox.Hint, "(Campaign description)");
	o2->Id2 = XC_CAMPAIGNDESC;
	o2->Pos = Vec2iNew(x, y);
	o2->Size = Vec2iNew(295, 5 * th);
	CArrayPushBack(&c.Objs, &o2);

	UIObjectDestroy(o);
	return c;
}
UICollection CreateMissionObjs(void)
{
	int th = CDogsTextHeight();
	UICollection c;
	UIObject *o;
	CArrayInit(&c.Objs, sizeof o);

	o = UIObjectCreate(YC_MISSIONTITLE, Vec2iNew(0, 150), Vec2iNew(319, th));
	o->Id2 = XC_MUSICFILE;
	o->Flags = UI_SELECT_ONLY;
	CArrayPushBack(&c.Objs, &o);

	return c;
}
UICollection CreateWeaponObjs(void)
{
	int th = CDogsTextHeight();
	UICollection c;
	UIObject *o;
	UIObject *o2;
	int i;
	CArrayInit(&c.Objs, sizeof o);

	o = UIObjectCreate(0, Vec2iZero(), Vec2iNew(80, th));
	o->Flags = UI_LEAVE_YC;
	for (i = 0; i < WEAPON_MAX; i++)
	{
		int x = 10 + i / 4 * 90;
		int y = 150 + (i % 4) * th;
		o2 = UIObjectCopy(o);
		o2->Id2 = i;
		o2->Pos = Vec2iNew(x, y);
		CArrayPushBack(&c.Objs, &o2);
	}

	UIObjectDestroy(o);
	return c;
}
UICollection CreateMapItemObjs(void)
{
	UICollection c;
	UIObject *o;
	UIObject *o2;
	int i;
	CArrayInit(&c.Objs, sizeof o);

	o = UIObjectCreate(0, Vec2iZero(), Vec2iNew(19, 40));
	o->Flags = UI_LEAVE_YC;
	for (i = 0; i < ITEMS_MAX; i++)
	{
		int x = 10 + i * 20;
		o2 = UIObjectCopy(o);
		o2->Id2 = i;
		o2->Pos = Vec2iNew(x, 150);
		CArrayPushBack(&c.Objs, &o2);
	}

	UIObjectDestroy(o);
	return c;
}
UICollection CreateObjectiveObjs(void)
{
	UICollection c;
	UIObject *o;
	UIObject *o2;
	int x;
	int y;
	CArrayInit(&c.Objs, sizeof o);

	o = UIObjectCreate(0, Vec2iZero(), Vec2iZero());
	o->Flags = UI_LEAVE_YC;
	y = 150;

	x = 20;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_TYPE;
	o2->Pos = Vec2iNew(x, y);
	o2->Size = Vec2iNew(35, 40);
	CArrayPushBack(&c.Objs, &o2);
	x += 40;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_INDEX;
	o2->Pos = Vec2iNew(x, y);
	o2->Size = Vec2iNew(30, 40);
	CArrayPushBack(&c.Objs, &o2);
	x += 30;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_REQUIRED;
	o2->Pos = Vec2iNew(x, y);
	o2->Size = Vec2iNew(20, 40);
	CSTRDUP(o2->Tooltip, "0: optional objective");
	CArrayPushBack(&c.Objs, &o2);
	x += 20;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_TOTAL;
	o2->Pos = Vec2iNew(x, y);
	o2->Size = Vec2iNew(35, 40);
	CArrayPushBack(&c.Objs, &o2);
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
	CArrayPushBack(&c.Objs, &o2);

	UIObjectDestroy(o);
	return c;
}
UICollection CreateCharacterObjs(void)
{
	UICollection c;
	UIObject *o;
	UIObject *o2;
	int i;
	CArrayInit(&c.Objs, sizeof o);

	o = UIObjectCreate(0, Vec2iZero(), Vec2iNew(19, 40));
	o->Flags = UI_LEAVE_YC | UI_SELECT_ONLY_FIRST;
	for (i = 0; i < 15; i++)
	{
		int x = 10 + i * 20;
		o2 = UIObjectCopy(o);
		o2->Id2 = i;
		o2->Pos = Vec2iNew(x, 150);
		CArrayPushBack(&c.Objs, &o2);
	}

	UIObjectDestroy(o);
	return c;
}

UICollection CreateCharEditorObjs(void)
{
	int th = CDogsTextHeight();
	UICollection c;
	UIObject *o;
	UIObject *o2;
	int x;
	int y;
	CArrayInit(&c.Objs, sizeof o);

	// Appearance

	y = 10;
	o = UIObjectCreate(YC_APPEARANCE, Vec2iZero(), Vec2iNew(40, th));

	x = 30;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_FACE;
	o2->Pos = Vec2iNew(x, y);
	CArrayPushBack(&c.Objs, &o2);
	x += 30;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_SKIN;
	o2->Pos = Vec2iNew(x, y);
	CArrayPushBack(&c.Objs, &o2);
	x += 30;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_HAIR;
	o2->Pos = Vec2iNew(x, y);
	CArrayPushBack(&c.Objs, &o2);
	x += 30;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_BODY;
	o2->Pos = Vec2iNew(x, y);
	CArrayPushBack(&c.Objs, &o2);
	x += 30;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_ARMS;
	o2->Pos = Vec2iNew(x, y);
	CArrayPushBack(&c.Objs, &o2);
	x += 30;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_LEGS;
	o2->Pos = Vec2iNew(x, y);
	CArrayPushBack(&c.Objs, &o2);

	// Character attributes

	y += th;
	UIObjectDestroy(o);
	o = UIObjectCreate(YC_ATTRIBUTES, Vec2iZero(), Vec2iNew(40, th));

	x = 20;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_SPEED;
	o2->Pos = Vec2iNew(x, y);
	CArrayPushBack(&c.Objs, &o2);
	x += 50;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_HEALTH;
	o2->Pos = Vec2iNew(x, y);
	CArrayPushBack(&c.Objs, &o2);
	x += 50;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_MOVE;
	o2->Pos = Vec2iNew(x, y);
	CArrayPushBack(&c.Objs, &o2);
	x += 50;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_TRACK;
	o2->Pos = Vec2iNew(x, y);
	CSTRDUP(o2->Tooltip,
		"Looking towards the player\n"
		"Useless for friendly characters");
	CArrayPushBack(&c.Objs, &o2);
	x += 50;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_SHOOT;
	o2->Pos = Vec2iNew(x, y);
	CArrayPushBack(&c.Objs, &o2);
	x += 50;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_DELAY;
	o2->Pos = Vec2iNew(x, y);
	CSTRDUP(o2->Tooltip, "Frames before making another decision");
	CArrayPushBack(&c.Objs, &o2);

	// Character flags

	y += th;
	UIObjectDestroy(o);
	o = UIObjectCreate(YC_FLAGS, Vec2iZero(), Vec2iNew(40, th));

	x = 5;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_ASBESTOS;
	o2->Pos = Vec2iNew(x, y);
	CArrayPushBack(&c.Objs, &o2);
	x += 45;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_IMMUNITY;
	o2->Pos = Vec2iNew(x, y);
	CSTRDUP(o2->Tooltip, "Immune to poison");
	CArrayPushBack(&c.Objs, &o2);
	x += 45;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_SEETHROUGH;
	o2->Pos = Vec2iNew(x, y);
	CArrayPushBack(&c.Objs, &o2);
	x += 45;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_RUNS_AWAY;
	o2->Pos = Vec2iNew(x, y);
	CSTRDUP(o2->Tooltip, "Runs away from player");
	CArrayPushBack(&c.Objs, &o2);
	x += 45;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_SNEAKY;
	o2->Pos = Vec2iNew(x, y);
	CSTRDUP(o2->Tooltip, "Shoots back when player shoots");
	CArrayPushBack(&c.Objs, &o2);
	x += 45;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_GOOD_GUY;
	o2->Pos = Vec2iNew(x, y);
	CArrayPushBack(&c.Objs, &o2);
	x += 45;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_SLEEPING;
	o2->Pos = Vec2iNew(x, y);
	CSTRDUP(o2->Tooltip, "Doesn't move unless seen");
	CArrayPushBack(&c.Objs, &o2);

	y += th;
	x = 5;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_PRISONER;
	o2->Pos = Vec2iNew(x, y);
	CSTRDUP(o2->Tooltip, "Doesn't move until touched");
	CArrayPushBack(&c.Objs, &o2);
	x += 45;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_INVULNERABLE;
	o2->Pos = Vec2iNew(x, y);
	CArrayPushBack(&c.Objs, &o2);
	x += 45;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_FOLLOWER;
	o2->Pos = Vec2iNew(x, y);
	CArrayPushBack(&c.Objs, &o2);
	x += 45;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_PENALTY;
	o2->Pos = Vec2iNew(x, y);
	CSTRDUP(o2->Tooltip, "Large score penalty when shot");
	CArrayPushBack(&c.Objs, &o2);
	x += 45;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_VICTIM;
	o2->Pos = Vec2iNew(x, y);
	CSTRDUP(o2->Tooltip, "Takes damage from everyone");
	CArrayPushBack(&c.Objs, &o2);
	x += 45;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_AWAKE;
	o2->Pos = Vec2iNew(x, y);
	CArrayPushBack(&c.Objs, &o2);

	// Weapon

	y += th;
	UIObjectDestroy(o);
	o = UIObjectCreate(YC_WEAPON, Vec2iNew(50, y), Vec2iNew(210, th));
	CArrayPushBack(&c.Objs, &o);

	return c;
}
