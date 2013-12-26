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
	UIObject o;
	UIObject o2;
	int x;
	int y;
	CArrayInit(&c.Objs, sizeof o);

	// Titles

	y = 5;

	memset(&o, 0, sizeof o);
	o.Type = UITYPE_TEXTBOX;
	o.u.Textbox.TextLinkFunc = CampaignGetTitle;
	o.u.Textbox.TextLinkData = &gCampaign;
	CSTRDUP(o.u.Textbox.Hint, "(Campaign title)");
	o.Id = YC_CAMPAIGNTITLE;
	o.Flags = UI_SELECT_ONLY_FIRST;
	o.Pos = Vec2iNew(25, y);
	o.Size = Vec2iNew(240, th);
	CArrayPushBack(&c.Objs, &o);

	memset(&o, 0, sizeof o);
	o.Id = YC_MISSIONINDEX;
	o.Pos = Vec2iNew(270, y);
	o.Size = Vec2iNew(49, th);
	CArrayPushBack(&c.Objs, &o);

	y = 2 * th;

	memset(&o, 0, sizeof o);
	o.Id = YC_MISSIONTITLE;
	o.Flags = UI_SELECT_ONLY;
	o.Pos = Vec2iNew(25, y);
	o.Size = Vec2iNew(175, th);
	CArrayPushBack(&c.Objs, &o);

	// mission properties
	// size, walls, rooms etc.

	y = 10 + 2 * th;
	
	memset(&o, 0, sizeof o);
	o.Id = YC_MISSIONPROPS;
	o.Size = Vec2iNew(35, th);

	x = 20;
	o.Id2 = XC_WIDTH;
	o.Pos = Vec2iNew(x, y);
	CArrayPushBack(&c.Objs, &o);
	x += 40;
	o.Id2 = XC_HEIGHT;
	o.Pos = Vec2iNew(x, y);
	CArrayPushBack(&c.Objs, &o);
	x += 40;
	o.Id2 = XC_WALLCOUNT;
	o.Pos = Vec2iNew(x, y);
	CArrayPushBack(&c.Objs, &o);
	x += 40;
	o.Id2 = XC_WALLLENGTH;
	o.Pos = Vec2iNew(x, y);
	CArrayPushBack(&c.Objs, &o);
	x += 40;
	o.Id2 = XC_ROOMCOUNT;
	o.Pos = Vec2iNew(x, y);
	CArrayPushBack(&c.Objs, &o);
	x += 40;
	o.Id2 = XC_SQRCOUNT;
	o.Pos = Vec2iNew(x, y);
	CArrayPushBack(&c.Objs, &o);
	x += 40;
	o.Id2 = XC_DENSITY;
	o.Pos = Vec2iNew(x, y);
	CSTRDUP(o.Tooltip, "Number of non-objective characters");
	CArrayPushBack(&c.Objs, &o);

	// Mission looks
	// wall/floor styles etc.

	y += th;

	memset(&o, 0, sizeof o);
	o.Id = YC_MISSIONLOOKS;
	o.Size = Vec2iNew(25, 25 + th);

	x = 20;
	o.Id2 = XC_WALL;
	o.Pos = Vec2iNew(x, y);
	CArrayPushBack(&c.Objs, &o);
	x += 30;
	o.Id2 = XC_FLOOR;
	o.Pos = Vec2iNew(x, y);
	CArrayPushBack(&c.Objs, &o);
	x += 30;
	o.Id2 = XC_ROOM;
	o.Pos = Vec2iNew(x, y);
	CArrayPushBack(&c.Objs, &o);
	x += 30;
	o.Id2 = XC_DOORS;
	o.Pos = Vec2iNew(x, y);
	CArrayPushBack(&c.Objs, &o);
	x += 30;
	o.Id2 = XC_KEYS;
	o.Pos = Vec2iNew(x, y);
	CArrayPushBack(&c.Objs, &o);
	x += 30;
	o.Id2 = XC_EXIT;
	o.Pos = Vec2iNew(x, y);
	CArrayPushBack(&c.Objs, &o);

	// colours

	x = 200;

	memset(&o, 0, sizeof o);
	o.Id = YC_MISSIONLOOKS;
	o.Size = Vec2iNew(100, th);

	o.Id2 = XC_COLOR1;
	o.Pos = Vec2iNew(x, y);
	CArrayPushBack(&c.Objs, &o);
	y += th;
	o.Id2 = XC_COLOR2;
	o.Pos = Vec2iNew(x, y);
	CArrayPushBack(&c.Objs, &o);
	y += th;
	o.Id2 = XC_COLOR3;
	o.Pos = Vec2iNew(x, y);
	CArrayPushBack(&c.Objs, &o);
	y += th;
	o.Id2 = XC_COLOR4;
	o.Pos = Vec2iNew(x, y);
	CArrayPushBack(&c.Objs, &o);

	// mission data

	x = 10;
	y += th;

	memset(&o, 0, sizeof o);
	o.Flags = UI_SELECT_ONLY;
	o.Size = Vec2iNew(189, th);

	o.Id = YC_MISSIONDESC;
	o.Pos = Vec2iNew(x, y);
	memset(&o2, 0, sizeof o2);
	o2.Type = UITYPE_TEXTBOX;
	o2.u.Textbox.TextLinkFunc = MissionGetDescription;
	o2.u.Textbox.TextLinkData = missionPtr;
	CSTRDUP(o2.u.Textbox.Hint, "(Mission description)");
	o2.Flags = UI_IGNORE;
	o2.Pos = Vec2iNew(25, 150);
	o2.Size = Vec2iNew(295, 5 * th);
	CArrayInit(&o.Children.Objs, sizeof o);
	CArrayPushBack(&o.Children.Objs, &o2);
	CArrayPushBack(&c.Objs, &o);

	memset(&o.Children.Objs, 0, sizeof o.Children.Objs);
	y += th;
	o.Id = YC_CHARACTERS;
	o.Pos = Vec2iNew(x, y);
	CArrayPushBack(&c.Objs, &o);
	y += th;
	o.Id = YC_SPECIALS;
	o.Pos = Vec2iNew(x, y);
	CArrayPushBack(&c.Objs, &o);
	y += th;
	o.Id = YC_WEAPONS;
	o.Pos = Vec2iNew(x, y);
	CArrayPushBack(&c.Objs, &o);
	y += th;
	o.Id = YC_ITEMS;
	o.Pos = Vec2iNew(x, y);
	CSTRDUP(o.Tooltip, "Shift+click to change amounts");
	CArrayPushBack(&c.Objs, &o);
	o.Tooltip = NULL;

	// objectives
	y += 2;

	y += th;
	o.Id = YC_OBJECTIVES;
	o.Pos = Vec2iNew(x, y);
	CSTRDUP(o.Tooltip, "insert/delete: add/remove objective");
	CArrayPushBack(&c.Objs, &o);
	y += th;
	o.Id = YC_OBJECTIVES + 1;
	o.Pos = Vec2iNew(x, y);
	CSTRDUP(o.Tooltip, "insert/delete: add/remove objective");
	CArrayPushBack(&c.Objs, &o);
	y += th;
	o.Id = YC_OBJECTIVES + 2;
	o.Pos = Vec2iNew(x, y);
	CSTRDUP(o.Tooltip, "insert/delete: add/remove objective");
	CArrayPushBack(&c.Objs, &o);
	y += th;
	o.Id = YC_OBJECTIVES + 3;
	o.Pos = Vec2iNew(x, y);
	CSTRDUP(o.Tooltip, "insert/delete: add/remove objective");
	CArrayPushBack(&c.Objs, &o);
	y += th;
	o.Id = YC_OBJECTIVES + 4;
	o.Pos = Vec2iNew(x, y);
	CSTRDUP(o.Tooltip, "insert/delete: add/remove objective");
	CArrayPushBack(&c.Objs, &o);

	return c;
}
UICollection CreateCampaignObjs(void)
{
	int th = CDogsTextHeight();
	UICollection c;
	UIObject o;
	int x;
	int y;
	CArrayInit(&c.Objs, sizeof o);

	x = 25;
	y = 150;

	memset(&o, 0, sizeof o);
	o.Id = YC_CAMPAIGNTITLE;
	o.Flags = UI_SELECT_ONLY;

	o.Type = UITYPE_TEXTBOX;
	o.u.Textbox.TextLinkFunc = CampaignGetAuthor;
	o.u.Textbox.TextLinkData = &gCampaign;
	CSTRDUP(o.u.Textbox.Hint, "(Campaign author)");
	o.Id2 = XC_AUTHOR;
	o.Pos = Vec2iNew(x, y);
	o.Size = Vec2iNew(295, th);
	CArrayPushBack(&c.Objs, &o);

	y += th;
	o.Type = UITYPE_TEXTBOX;
	o.u.Textbox.TextLinkFunc = CampaignGetDescription;
	o.u.Textbox.TextLinkData = &gCampaign;
	CSTRDUP(o.u.Textbox.Hint, "(Campaign description)");
	o.Id2 = XC_CAMPAIGNDESC;
	o.Pos = Vec2iNew(x, y);
	o.Size = Vec2iNew(295, 5 * th);
	CArrayPushBack(&c.Objs, &o);

	return c;
}
UICollection CreateMissionObjs(void)
{
	int th = CDogsTextHeight();
	UICollection c;
	UIObject o;
	CArrayInit(&c.Objs, sizeof o);

	memset(&o, 0, sizeof o);
	o.Id = YC_MISSIONTITLE;
	o.Id2 = XC_MUSICFILE;
	o.Flags = UI_SELECT_ONLY;
	o.Pos = Vec2iNew(0, 150);
	o.Size = Vec2iNew(319, th);
	CArrayPushBack(&c.Objs, &o);

	return c;
}
UICollection CreateWeaponObjs(void)
{
	int th = CDogsTextHeight();
	UICollection c;
	UIObject o;
	int i;
	CArrayInit(&c.Objs, sizeof o);

	memset(&o, 0, sizeof o);
	o.Id = 0;
	o.Flags = UI_LEAVE_YC;
	o.Size = Vec2iNew(80, th);
	for (i = 0; i < WEAPON_MAX; i++)
	{
		int x = 10 + i / 4 * 90;
		int y = 150 + (i % 4) * th;
		o.Id2 = i;
		o.Pos = Vec2iNew(x, y);
		CArrayPushBack(&c.Objs, &o);
	}

	return c;
}
UICollection CreateMapItemObjs(void)
{
	UICollection c;
	UIObject o;
	int i;
	CArrayInit(&c.Objs, sizeof o);

	memset(&o, 0, sizeof o);
	o.Id = 0;
	o.Flags = UI_LEAVE_YC;
	o.Size = Vec2iNew(19, 40);
	for (i = 0; i < ITEMS_MAX; i++)
	{
		int x = 10 + i * 20;
		o.Id2 = i;
		o.Pos = Vec2iNew(x, 150);
		CArrayPushBack(&c.Objs, &o);
	}

	return c;
}
UICollection CreateObjectiveObjs(void)
{
	UICollection c;
	UIObject o;
	int x;
	int y;
	CArrayInit(&c.Objs, sizeof o);

	memset(&o, 0, sizeof o);
	o.Id = 0;
	o.Flags = UI_LEAVE_YC;
	y = 150;

	x = 20;
	o.Id2 = XC_TYPE;
	o.Pos = Vec2iNew(x, y);
	o.Size = Vec2iNew(35, 40);
	CArrayPushBack(&c.Objs, &o);
	x += 40;
	o.Id2 = XC_INDEX;
	o.Pos = Vec2iNew(x, y);
	o.Size = Vec2iNew(30, 40);
	CArrayPushBack(&c.Objs, &o);
	x += 30;
	o.Id2 = XC_REQUIRED;
	o.Pos = Vec2iNew(x, y);
	o.Size = Vec2iNew(20, 40);
	CSTRDUP(o.Tooltip, "0: optional objective");
	CArrayPushBack(&c.Objs, &o);
	o.Tooltip = NULL;
	x += 20;
	o.Id2 = XC_TOTAL;
	o.Pos = Vec2iNew(x, y);
	o.Size = Vec2iNew(35, 40);
	CArrayPushBack(&c.Objs, &o);
	x += 40;
	o.Id2 = XC_FLAGS;
	o.Pos = Vec2iNew(x, y);
	o.Size = Vec2iNew(100, 40);
	CSTRDUP(o.Tooltip,
		"hidden: not shown on map\n"
		"pos.known: always shown on map\n"
		"access: in locked room\n"
		"no-count: don't show completed count");
	CArrayPushBack(&c.Objs, &o);

	return c;
}
UICollection CreateCharacterObjs(void)
{
	UICollection c;
	UIObject o;
	int i;
	CArrayInit(&c.Objs, sizeof o);

	memset(&o, 0, sizeof o);
	o.Id = 0;
	o.Flags = UI_LEAVE_YC | UI_SELECT_ONLY_FIRST;
	o.Size = Vec2iNew(19, 40);
	for (i = 0; i < 15; i++)
	{
		int x = 10 + i * 20;
		o.Id2 = i;
		o.Pos = Vec2iNew(x, 150);
		CArrayPushBack(&c.Objs, &o);
	}

	return c;
}

UICollection CreateCharEditorObjs(void)
{
	int th = CDogsTextHeight();
	UICollection c;
	UIObject o;
	int x;
	int y;
	CArrayInit(&c.Objs, sizeof o);

	// Appearance

	y = 10;
	memset(&o, 0, sizeof o);
	o.Id = YC_APPEARANCE;
	o.Size = Vec2iNew(40, th);

	x = 30;
	o.Id2 = XC_FACE;
	o.Pos = Vec2iNew(x, y);
	CArrayPushBack(&c.Objs, &o);
	x += 30;
	o.Id2 = XC_SKIN;
	o.Pos = Vec2iNew(x, y);
	CArrayPushBack(&c.Objs, &o);
	x += 30;
	o.Id2 = XC_HAIR;
	o.Pos = Vec2iNew(x, y);
	CArrayPushBack(&c.Objs, &o);
	x += 30;
	o.Id2 = XC_BODY;
	o.Pos = Vec2iNew(x, y);
	CArrayPushBack(&c.Objs, &o);
	x += 30;
	o.Id2 = XC_ARMS;
	o.Pos = Vec2iNew(x, y);
	CArrayPushBack(&c.Objs, &o);
	x += 30;
	o.Id2 = XC_LEGS;
	o.Pos = Vec2iNew(x, y);
	CArrayPushBack(&c.Objs, &o);

	// Character attributes

	y += th;
	memset(&o, 0, sizeof o);
	o.Id = YC_ATTRIBUTES;
	o.Size = Vec2iNew(40, th);

	x = 20;
	o.Id2 = XC_SPEED;
	o.Pos = Vec2iNew(x, y);
	CArrayPushBack(&c.Objs, &o);
	x += 50;
	o.Id2 = XC_HEALTH;
	o.Pos = Vec2iNew(x, y);
	CArrayPushBack(&c.Objs, &o);
	x += 50;
	o.Id2 = XC_MOVE;
	o.Pos = Vec2iNew(x, y);
	CArrayPushBack(&c.Objs, &o);
	x += 50;
	o.Id2 = XC_TRACK;
	o.Pos = Vec2iNew(x, y);
	CSTRDUP(o.Tooltip,
		"Looking towards the player\n"
		"Useless for friendly characters");
	CArrayPushBack(&c.Objs, &o);
	o.Tooltip = NULL;
	x += 50;
	o.Id2 = XC_SHOOT;
	o.Pos = Vec2iNew(x, y);
	CArrayPushBack(&c.Objs, &o);
	x += 50;
	o.Id2 = XC_DELAY;
	o.Pos = Vec2iNew(x, y);
	CSTRDUP(o.Tooltip, "Frames before making another decision");
	CArrayPushBack(&c.Objs, &o);

	// Character flags

	y += th;
	memset(&o, 0, sizeof o);
	o.Id = YC_FLAGS;
	o.Size = Vec2iNew(40, th);

	x = 5;
	o.Id2 = XC_ASBESTOS;
	o.Pos = Vec2iNew(x, y);
	CArrayPushBack(&c.Objs, &o);
	x += 45;
	o.Id2 = XC_IMMUNITY;
	o.Pos = Vec2iNew(x, y);
	CSTRDUP(o.Tooltip, "Immune to poison");
	CArrayPushBack(&c.Objs, &o);
	o.Tooltip = NULL;
	x += 45;
	o.Id2 = XC_SEETHROUGH;
	o.Pos = Vec2iNew(x, y);
	CArrayPushBack(&c.Objs, &o);
	x += 45;
	o.Id2 = XC_RUNS_AWAY;
	o.Pos = Vec2iNew(x, y);
	CSTRDUP(o.Tooltip, "Runs away from player");
	CArrayPushBack(&c.Objs, &o);
	o.Tooltip = NULL;
	x += 45;
	o.Id2 = XC_SNEAKY;
	o.Pos = Vec2iNew(x, y);
	CSTRDUP(o.Tooltip, "Shoots back when player shoots");
	CArrayPushBack(&c.Objs, &o);
	o.Tooltip = NULL;
	x += 45;
	o.Id2 = XC_GOOD_GUY;
	o.Pos = Vec2iNew(x, y);
	CArrayPushBack(&c.Objs, &o);
	x += 45;
	o.Id2 = XC_SLEEPING;
	o.Pos = Vec2iNew(x, y);
	CSTRDUP(o.Tooltip, "Doesn't move unless seen");
	CArrayPushBack(&c.Objs, &o);
	o.Tooltip = NULL;

	y += th;
	x = 5;
	o.Id2 = XC_PRISONER;
	o.Pos = Vec2iNew(x, y);
	CSTRDUP(o.Tooltip, "Doesn't move until touched");
	CArrayPushBack(&c.Objs, &o);
	o.Tooltip = NULL;
	x += 45;
	o.Id2 = XC_INVULNERABLE;
	o.Pos = Vec2iNew(x, y);
	CArrayPushBack(&c.Objs, &o);
	x += 45;
	o.Id2 = XC_FOLLOWER;
	o.Pos = Vec2iNew(x, y);
	CArrayPushBack(&c.Objs, &o);
	x += 45;
	o.Id2 = XC_PENALTY;
	o.Pos = Vec2iNew(x, y);
	CSTRDUP(o.Tooltip, "Large score penalty when shot");
	CArrayPushBack(&c.Objs, &o);
	o.Tooltip = NULL;
	x += 45;
	o.Id2 = XC_VICTIM;
	o.Pos = Vec2iNew(x, y);
	CSTRDUP(o.Tooltip, "Takes damage from everyone");
	CArrayPushBack(&c.Objs, &o);
	o.Tooltip = NULL;
	x += 45;
	o.Id2 = XC_AWAKE;
	o.Pos = Vec2iNew(x, y);
	CArrayPushBack(&c.Objs, &o);

	// Weapon

	y += th;
	memset(&o, 0, sizeof o);
	o.Id = YC_WEAPON;
	o.Pos = Vec2iNew(50, y);
	o.Size = Vec2iNew(210, th);
	CArrayPushBack(&c.Objs, &o);

	return c;
}
