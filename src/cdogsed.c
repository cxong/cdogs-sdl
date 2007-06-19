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

-------------------------------------------------------------------------------

 cdogsed.c - map/mission editor 

*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
// #include <conio.h>
#include <ctype.h>
// #include <bios.h>
#include "SDL.h"

#include "objs.h"
#include "actors.h"
#include "grafx.h"
#include "pics.h"
#include "text.h"
#include "gamedata.h"
#include "mission.h"
#include "files.h"
#include "triggers.h"
#include "automap.h"
#include "charsed.h"
#include "events.h"
#include "utils.h"


#define YC_CAMPAIGNTITLE    0
#define YC_MISSIONINDEX     1
#define YC_MISSIONTITLE     2
#define YC_MISSIONPROPS     3
#define YC_MISSIONLOOKS     4
#define YC_MISSIONDESC      5
#define YC_CHARACTERS       6
#define YC_SPECIALS         7
#define YC_WEAPONS          8
#define YC_ITEMS            9
#define YC_OBJECTIVES       10

#define XC_CAMPAIGNTITLE    0
#define XC_AUTHOR           1
#define XC_CAMPAIGNDESC     2

#define XC_MISSIONTITLE     0
#define XC_MUSICFILE        1

#define XC_WIDTH            0
#define XC_HEIGHT           1
#define XC_WALLCOUNT        2
#define XC_WALLLENGTH       3
#define XC_ROOMCOUNT        4
#define XC_SQRCOUNT         5
#define XC_DENSITY          6

#define XC_WALL             0
#define XC_FLOOR            1
#define XC_ROOM             2
#define XC_DOORS            3
#define XC_KEYS             4
#define XC_EXIT             5
#define XC_COLOR1           6
#define XC_COLOR2           7
#define XC_COLOR3           8
#define XC_COLOR4           9

#define XC_TYPE             0
#define XC_INDEX            1
#define XC_REQUIRED         2
#define XC_TOTAL            3
#define XC_FLAGS            4

#define XC_MAXWEAPONS      10


#define TH  8
#define SELECT_ONLY        (1 << 16)
#define LEAVE_YC           (1 << 17)
#define LEAVE_XC           (1 << 18)
#define SELECT_ONLY_FIRST  (1 << 19)


// Mouse click areas:

static struct MouseRect localClicks[] = {
	{25, 5, 265, 5 + TH - 1, YC_CAMPAIGNTITLE + SELECT_ONLY},
	{270, 5, 319, 5 + TH - 1, YC_MISSIONINDEX + SELECT_ONLY},
	{25, 8 + TH, 200, 8 + 2 * TH - 1, YC_MISSIONTITLE + SELECT_ONLY},

	{20, 10 + 2 * TH, 55, 10 + 3 * TH - 1,
	 YC_MISSIONPROPS + (XC_WIDTH << 8)},
	{60, 10 + 2 * TH, 95, 10 + 3 * TH - 1,
	 YC_MISSIONPROPS + (XC_HEIGHT << 8)},
	{100, 10 + 2 * TH, 135, 10 + 3 * TH - 1,
	 YC_MISSIONPROPS + (XC_WALLCOUNT << 8)},
	{140, 10 + 2 * TH, 175, 10 + 3 * TH - 1,
	 YC_MISSIONPROPS + (XC_WALLLENGTH << 8)},
	{180, 10 + 2 * TH, 215, 10 + 3 * TH - 1,
	 YC_MISSIONPROPS + (XC_ROOMCOUNT << 8)},
	{220, 10 + 2 * TH, 255, 10 + 3 * TH - 1,
	 YC_MISSIONPROPS + (XC_SQRCOUNT << 8)},
	{260, 10 + 2 * TH, 295, 10 + 3 * TH - 1,
	 YC_MISSIONPROPS + (XC_DENSITY << 8)},

	{20, 10 + 3 * TH, 45, 35 + 4 * TH - 1,
	 YC_MISSIONLOOKS + (XC_WALL << 8)},
	{50, 10 + 3 * TH, 75, 35 + 4 * TH - 1,
	 YC_MISSIONLOOKS + (XC_FLOOR << 8)},
	{80, 10 + 3 * TH, 105, 35 + 4 * TH - 1,
	 YC_MISSIONLOOKS + (XC_ROOM << 8)},
	{110, 10 + 3 * TH, 135, 35 + 4 * TH - 1,
	 YC_MISSIONLOOKS + (XC_DOORS << 8)},
	{140, 10 + 3 * TH, 165, 35 + 4 * TH - 1,
	 YC_MISSIONLOOKS + (XC_KEYS << 8)},
	{170, 10 + 3 * TH, 195, 35 + 4 * TH - 1,
	 YC_MISSIONLOOKS + (XC_EXIT << 8)},

	{200, 10 + 3 * TH, 300, 10 + 4 * TH - 1,
	 YC_MISSIONLOOKS + (XC_COLOR1 << 8)},
	{200, 10 + 4 * TH, 300, 10 + 5 * TH - 1,
	 YC_MISSIONLOOKS + (XC_COLOR2 << 8)},
	{200, 10 + 5 * TH, 300, 10 + 6 * TH - 1,
	 YC_MISSIONLOOKS + (XC_COLOR3 << 8)},
	{200, 10 + 6 * TH, 300, 10 + 7 * TH - 1,
	 YC_MISSIONLOOKS + (XC_COLOR4 << 8)},

	{10, 35 + 4 * TH, 199, 35 + 5 * TH - 1,
	 YC_MISSIONDESC + SELECT_ONLY},
	{10, 35 + 5 * TH, 199, 35 + 6 * TH - 1,
	 YC_CHARACTERS + SELECT_ONLY},
	{10, 35 + 6 * TH, 199, 35 + 7 * TH - 1, YC_SPECIALS + SELECT_ONLY},
	{10, 35 + 7 * TH, 199, 35 + 8 * TH - 1, YC_WEAPONS + SELECT_ONLY},
	{10, 35 + 8 * TH, 199, 35 + 9 * TH - 1, YC_ITEMS + SELECT_ONLY},
	{10, 37 + 9 * TH, 199, 37 + 10 * TH - 1,
	 YC_OBJECTIVES + SELECT_ONLY},
	{10, 37 + 10 * TH, 199, 37 + 11 * TH - 1,
	 YC_OBJECTIVES + 1 + SELECT_ONLY},
	{10, 37 + 11 * TH, 199, 37 + 12 * TH - 1,
	 YC_OBJECTIVES + 2 + SELECT_ONLY},
	{10, 37 + 12 * TH, 199, 37 + 13 * TH - 1,
	 YC_OBJECTIVES + 3 + SELECT_ONLY},
	{10, 37 + 13 * TH, 199, 37 + 14 * TH - 1,
	 YC_OBJECTIVES + 4 + SELECT_ONLY},

	{0, 0, 0, 0, 0}
};

static struct MouseRect localCampaignClicks[] = {
	{0, 150, 319, 150 + TH - 1,
	 YC_CAMPAIGNTITLE + (XC_AUTHOR << 8) + SELECT_ONLY},
	{0, 150 + TH, 319, 190,
	 YC_CAMPAIGNTITLE + (XC_CAMPAIGNDESC << 8) + SELECT_ONLY},

	{0, 0, 0, 0, 0}
};

static struct MouseRect localMissionClicks[] = {
	{0, 150, 319, 160,
	 YC_MISSIONTITLE + (XC_MUSICFILE << 8) + SELECT_ONLY},

	{0, 0, 0, 0, 0}
};

static struct MouseRect localWeaponClicks[] = {
	{10, 150, 90, 150 + TH - 1, LEAVE_YC + (0 << 8)},
	{10, 150 + TH, 90, 150 + 2 * TH - 1, LEAVE_YC + (1 << 8)},
	{10, 150 + 2 * TH, 90, 150 + 3 * TH - 1, LEAVE_YC + (2 << 8)},
	{10, 150 + 3 * TH, 90, 150 + 4 * TH - 1, LEAVE_YC + (3 << 8)},

	{100, 150, 180, 150 + TH - 1, LEAVE_YC + (4 << 8)},
	{100, 150 + TH, 180, 150 + 2 * TH - 1, LEAVE_YC + (5 << 8)},
	{100, 150 + 2 * TH, 180, 150 + 3 * TH - 1, LEAVE_YC + (6 << 8)},
	{100, 150 + 3 * TH, 180, 150 + 4 * TH - 1, LEAVE_YC + (7 << 8)},

	{190, 150, 270, 150 + TH - 1, LEAVE_YC + (8 << 8)},
	{190, 150 + TH, 270, 150 + 2 * TH - 1, LEAVE_YC + (9 << 8)},
	{190, 150 + 2 * TH, 270, 150 + 3 * TH - 1, LEAVE_YC + (10 << 8)},

	{0, 0, 0, 0, 0}
};

static struct MouseRect localMapItemClicks[] = {
	{0, 150, 19, 190, LEAVE_YC + (0 << 8)},
	{20, 150, 39, 190, LEAVE_YC + (1 << 8)},
	{40, 150, 59, 190, LEAVE_YC + (2 << 8)},
	{60, 150, 79, 190, LEAVE_YC + (3 << 8)},
	{80, 150, 99, 190, LEAVE_YC + (4 << 8)},
	{100, 150, 119, 190, LEAVE_YC + (5 << 8)},
	{120, 150, 139, 190, LEAVE_YC + (6 << 8)},
	{140, 150, 159, 190, LEAVE_YC + (7 << 8)},
	{160, 150, 179, 190, LEAVE_YC + (8 << 8)},
	{180, 150, 199, 190, LEAVE_YC + (9 << 8)},
	{200, 150, 219, 190, LEAVE_YC + (10 << 8)},
	{220, 150, 239, 190, LEAVE_YC + (11 << 8)},
	{240, 150, 259, 190, LEAVE_YC + (12 << 8)},
	{260, 150, 279, 190, LEAVE_YC + (13 << 8)},
	{280, 150, 299, 190, LEAVE_YC + (14 << 8)},
	{300, 150, 319, 190, LEAVE_YC + (15 << 8)},

	{0, 0, 0, 0, 0}
};

static struct MouseRect localObjectiveClicks[] = {
	{20, 150, 55, 190, LEAVE_YC + (XC_TYPE << 8)},
	{60, 150, 85, 190, LEAVE_YC + (XC_INDEX << 8)},
	{90, 150, 105, 190, LEAVE_YC + (XC_REQUIRED << 8)},
	{110, 150, 145, 190, LEAVE_YC + (XC_TOTAL << 8)},
	{150, 150, 195, 190, LEAVE_YC + (XC_FLAGS << 8)},

	{0, 0, 0, 0, 0}
};

static struct MouseRect localCharacterClicks[] = {
	{10, 150, 29, 190, LEAVE_YC + (0 << 8) + SELECT_ONLY_FIRST},
	{30, 150, 49, 190, LEAVE_YC + (1 << 8) + SELECT_ONLY_FIRST},
	{50, 150, 69, 190, LEAVE_YC + (2 << 8) + SELECT_ONLY_FIRST},
	{70, 150, 89, 190, LEAVE_YC + (3 << 8) + SELECT_ONLY_FIRST},
	{90, 150, 109, 190, LEAVE_YC + (4 << 8) + SELECT_ONLY_FIRST},
	{110, 150, 129, 190, LEAVE_YC + (5 << 8) + SELECT_ONLY_FIRST},
	{130, 150, 149, 190, LEAVE_YC + (6 << 8) + SELECT_ONLY_FIRST},
	{150, 150, 169, 190, LEAVE_YC + (7 << 8) + SELECT_ONLY_FIRST},
	{170, 150, 189, 190, LEAVE_YC + (8 << 8) + SELECT_ONLY_FIRST},
	{190, 150, 209, 190, LEAVE_YC + (9 << 8) + SELECT_ONLY_FIRST},
	{210, 150, 229, 190, LEAVE_YC + (10 << 8) + SELECT_ONLY_FIRST},
	{230, 150, 249, 190, LEAVE_YC + (11 << 8) + SELECT_ONLY_FIRST},
	{250, 150, 269, 190, LEAVE_YC + (12 << 8) + SELECT_ONLY_FIRST},
	{270, 150, 289, 190, LEAVE_YC + (13 << 8) + SELECT_ONLY_FIRST},
	{290, 150, 309, 190, LEAVE_YC + (14 << 8) + SELECT_ONLY_FIRST},

	{0, 0, 0, 0, 0}
};



// Globals

TCampaignSetting campaign;
struct Mission *currentMission;
static char lastFile[128];



// Code...

void DisplayText(int x, int y, const char *text, int hilite, int editable)
{
	TextGoto(x, y);
	if (editable) {
		if (hilite)
			TextCharWithTable('\020', tableFlamed);
		else
			TextChar('\020');
	}

	if (hilite && !editable)
		TextStringWithTable(text, tableFlamed);
	else
		TextString(text);

	if (editable) {
		if (hilite)
			TextCharWithTable('\021', tableFlamed);
		else
			TextChar('\021');
	}
}

void DrawObjectiveInfo(int index, int y, int xc)
{
	TOffsetPic pic;
	TranslationTable *table = NULL;
	int i;
	const char *typeText;
	char s[50];

	switch (currentMission->objectives[index].type) {
	case OBJECTIVE_KILL:
		typeText = "Kill";
		i = characterDesc[currentMission->baddieCount +
				  CHARACTER_OTHERS].facePic;
		table =
		    characterDesc[currentMission->baddieCount +
				  CHARACTER_OTHERS].table;
		pic.picIndex = cHeadPic[i][DIRECTION_DOWN][STATE_IDLE];
		pic.dx = cHeadOffset[i][DIRECTION_DOWN].dx;
		pic.dy = cHeadOffset[i][DIRECTION_DOWN].dy;
		break;
	case OBJECTIVE_RESCUE:
		typeText = "Rescue";
		i = characterDesc[CHARACTER_PRISONER].facePic;
		table = characterDesc[CHARACTER_PRISONER].table;
		pic.picIndex = cHeadPic[i][DIRECTION_DOWN][STATE_IDLE];
		pic.dx = cHeadOffset[i][DIRECTION_DOWN].dx;
		pic.dy = cHeadOffset[i][DIRECTION_DOWN].dy;
		break;
	case OBJECTIVE_COLLECT:
		typeText = "Collect";
		i = gMission.objectives[index].pickupItem;
		pic = cGeneralPics[i];
		break;
	case OBJECTIVE_DESTROY:
		typeText = "Destroy";
		i = gMission.objectives[index].blowupObject->pic;
		pic = cGeneralPics[i];
		break;
	case OBJECTIVE_INVESTIGATE:
		typeText = "Explore";
		break;
	default:
		typeText = "???";
		i = gMission.objectives[i].pickupItem;
		pic = cGeneralPics[i];
	}

	DisplayText(20, y, typeText, xc == XC_TYPE, 0);

	if (currentMission->objectives[index].type !=
	    OBJECTIVE_INVESTIGATE) {
		if (table)
			DrawTTPic(60 + pic.dx, y + 8 + pic.dy,
				  gPics[pic.picIndex], table, NULL);
		else
			DrawTPic(60 + pic.dx, y + 8 + pic.dy,
				 gPics[pic.picIndex], NULL);
	}

	sprintf(s, "%d", currentMission->objectives[index].required);
	DisplayText(90, y, s, xc == XC_REQUIRED, 0);
	sprintf(s, "out of %d", currentMission->objectives[index].count);
	DisplayText(110, y, s, xc == XC_TOTAL, 0);

	sprintf(s, "%s %s %s %s %s",
		(currentMission->objectives[index].
		 flags & OBJECTIVE_HIDDEN) != 0 ? "hidden" : "",
		(currentMission->objectives[index].
		 flags & OBJECTIVE_POSKNOWN) != 0 ? "pos.known" : "",
		(currentMission->objectives[index].
		 flags & OBJECTIVE_HIACCESS) != 0 ? "access" : "",
		(currentMission->objectives[index].
		 flags & OBJECTIVE_UNKNOWNCOUNT) != 0 ? "no-count" : "",
		(currentMission->objectives[index].
		 flags & OBJECTIVE_NOACCESS) != 0 ? "no-access" : "");
	DisplayText(150, y, s, xc == XC_FLAGS, 0);

	SetSecondaryMouseRects(localObjectiveClicks);
}

int MissionDescription(int y, const char *description, int hilite)
{
	int w_ws, w_word, x, lines;
	const char *ws, *word, *p, *s;

	TextGoto(20 - TextCharWidth('\020'), y);
	if (hilite)
		TextCharWithTable('\020', tableFlamed);
	else
		TextChar('\020');

	x = 20;
	lines = 1;
	TextGoto(x, y);

	s = ws = word = description;
	while (*s) {
		// Find word
		ws = s;
		while (*s == ' ' || *s == '\n')
			s++;
		word = s;
		while (*s != 0 && *s != ' ' && *s != '\n')
			s++;

		for (w_ws = 0, p = ws; p < word; p++)
			w_ws += TextCharWidth(*p);
		for (w_word = 0; p < s; p++)
			w_word += TextCharWidth(*p);

		if (x + w_ws + w_word > 300 && w_ws + w_word < 280) {
			y += TextHeight();
			x = 20;
			lines++;
			ws = word;
			w_ws = 0;
		}

		x += w_ws;
		TextGoto(x, y);

		for (p = word; p < s; p++)
			TextChar(*p);

		x += w_word;
	}

	if (hilite)
		TextCharWithTable('\021', tableFlamed);
	else
		TextChar('\021');

	return lines;
}

void DisplayCharacter(int x, int y, int character, int hilite)
{
	struct CharacterDescription *cd;
	TOffsetPic body, head;

	cd = &characterDesc[character];

	body.dx = cBodyOffset[cd->unarmedBodyPic][DIRECTION_DOWN].dx;
	body.dy = cBodyOffset[cd->unarmedBodyPic][DIRECTION_DOWN].dy;
	body.picIndex =
	    cBodyPic[cd->unarmedBodyPic][DIRECTION_DOWN][STATE_IDLE];

	head.dx =
	    cNeckOffset[cd->unarmedBodyPic][DIRECTION_DOWN].dx +
	    cHeadOffset[cd->facePic][DIRECTION_DOWN].dx;
	head.dy =
	    cNeckOffset[cd->unarmedBodyPic][DIRECTION_DOWN].dy +
	    cHeadOffset[cd->facePic][DIRECTION_DOWN].dy;
	head.picIndex = cHeadPic[cd->facePic][DIRECTION_DOWN][STATE_IDLE];

	DrawTTPic(x + body.dx, y + body.dy, gPics[body.picIndex],
		  cd->table, NULL);
	DrawTTPic(x + head.dx, y + head.dy, gPics[head.picIndex],
		  cd->table, NULL);

	if (hilite) {
		TextGoto(x - 8, y - 16);
		TextChar('\020');
		TextGoto(x - 8, y + 8);
		TextString(gunDesc[cd->defaultGun].gunName);
	}
}

static void ShowWeaponStatus(int x, int y, int weapon, int xc)
{
	DisplayFlag(x, y, gunDesc[weapon].gunName,
		    (currentMission->weaponSelection & (1 << weapon)) != 0,
		    xc == weapon);
}

void ListWeapons(int y, int xc)
{
	ShowWeaponStatus(10, y, 0, xc);
	ShowWeaponStatus(10, y + TH, 1, xc);
	ShowWeaponStatus(10, y + 2 * TH, 2, xc);
	ShowWeaponStatus(10, y + 3 * TH, 3, xc);

	ShowWeaponStatus(100, y, 4, xc);
	ShowWeaponStatus(100, y + TH, 5, xc);
	ShowWeaponStatus(100, y + 2 * TH, 6, xc);
	ShowWeaponStatus(100, y + 3 * TH, 7, xc);

	ShowWeaponStatus(190, y, 8, xc);
	ShowWeaponStatus(190, y + TH, 9, xc);
	ShowWeaponStatus(190, y + 2 * TH, 10, xc);

	SetSecondaryMouseRects(localWeaponClicks);
}

void DisplayMapItem(int x, int y, TMapObject * mo, int density, int hilite)
{
	char s[10];

	TOffsetPic *pic = &cGeneralPics[mo->pic];
	DrawTPic(x + pic->dx, y + pic->dy, gPics[pic->picIndex], NULL);

	if (hilite) {
		TextGoto(x - 8, y - 4);
		TextChar('\020');
	}
	sprintf(s, "%d", density);
	TextGoto(x - 8, y + 5);
	TextString(s);

	SetSecondaryMouseRects(localMapItemClicks);
}

void Display(int index, int xc, int yc, int key)
{
	char s[128];
	int y = 5;
	int i;

	SetSecondaryMouseRects(NULL);
	memset(GetDstScreen(), 58, Screen_GetMemSize());

	sprintf(s, "Key: 0x%x", key);
	TextStringAt(270, 190, s);

	DisplayText(25, y, campaign.title, yc == YC_CAMPAIGNTITLE
		    && xc == XC_CAMPAIGNTITLE, 1);

	if (fileChanged)
		DrawTPic(10, y, gPics[221], NULL);

	if (currentMission) {
		sprintf(s, "Mission %d/%d", index + 1,
			campaign.missionCount);
		DisplayText(270, y, s, yc == YC_MISSIONINDEX, 0);

		y += TextHeight() + 3;
		DisplayText(25, y, currentMission->title,
			    yc == YC_MISSIONTITLE
			    && xc == XC_MISSIONTITLE, 1);

		y += TextHeight() + 2;

		sprintf(s, "Width: %d", currentMission->mapWidth);
		DisplayText(20, y, s, yc == YC_MISSIONPROPS
			    && xc == XC_WIDTH, 0);

		sprintf(s, "Height: %d", currentMission->mapHeight);
		DisplayText(60, y, s, yc == YC_MISSIONPROPS
			    && xc == XC_HEIGHT, 0);

		sprintf(s, "Walls: %d", currentMission->wallCount);
		DisplayText(100, y, s, yc == YC_MISSIONPROPS
			    && xc == XC_WALLCOUNT, 0);

		sprintf(s, "Len: %d", currentMission->wallLength);
		DisplayText(140, y, s, yc == YC_MISSIONPROPS
			    && xc == XC_WALLLENGTH, 0);

		sprintf(s, "Rooms: %d", currentMission->roomCount);
		DisplayText(180, y, s, yc == YC_MISSIONPROPS
			    && xc == XC_ROOMCOUNT, 0);

		sprintf(s, "Sqr: %d", currentMission->squareCount);
		DisplayText(220, y, s, yc == YC_MISSIONPROPS
			    && xc == XC_SQRCOUNT, 0);

		sprintf(s, "Dens: %d", currentMission->baddieDensity);
		DisplayText(260, y, s, yc == YC_MISSIONPROPS
			    && xc == XC_DENSITY, 0);

		y += TextHeight();

		DisplayText(20, y, "Wall", yc == YC_MISSIONLOOKS
			    && xc == XC_WALL, 0);
		DisplayText(50, y, "Floor", yc == YC_MISSIONLOOKS
			    && xc == XC_FLOOR, 0);
		DisplayText(80, y, "Rooms", yc == YC_MISSIONLOOKS
			    && xc == XC_ROOM, 0);
		DisplayText(110, y, "Doors", yc == YC_MISSIONLOOKS
			    && xc == XC_DOORS, 0);
		DisplayText(140, y, "Keys", yc == YC_MISSIONLOOKS
			    && xc == XC_KEYS, 0);
		DisplayText(170, y, "Exit", yc == YC_MISSIONLOOKS
			    && xc == XC_EXIT, 0);

		sprintf(s, "Walls: %s",
			RangeName(currentMission->wallRange));
		DisplayText(200, y, s, yc == YC_MISSIONLOOKS
			    && xc == XC_COLOR1, 0);
		sprintf(s, "Floor: %s",
			RangeName(currentMission->floorRange));
		DisplayText(200, y + TH, s, yc == YC_MISSIONLOOKS
			    && xc == XC_COLOR2, 0);
		sprintf(s, "Rooms: %s",
			RangeName(currentMission->roomRange));
		DisplayText(200, y + 2 * TH, s, yc == YC_MISSIONLOOKS
			    && xc == XC_COLOR3, 0);
		sprintf(s, "Extra: %s",
			RangeName(currentMission->altRange));
		DisplayText(200, y + 3 * TH, s, yc == YC_MISSIONLOOKS
			    && xc == XC_COLOR4, 0);

		DrawPic(20, y + TH,
			gPics[cWallPics
			      [currentMission->wallStyle %
			       WALL_COUNT][WALL_SINGLE]], NULL);
		DrawPic(50, y + TH,
			gPics[cFloorPics
			      [currentMission->floorStyle %
			       FLOOR_COUNT][FLOOR_NORMAL]], NULL);
		DrawPic(80, y + TH,
			gPics[cRoomPics
			      [currentMission->roomStyle %
			       ROOMFLOOR_COUNT][ROOMFLOOR_NORMAL]], NULL);
		DrawPic(110, y + TH,
			gPics[cGeneralPics[gMission.doorPics[0].horzPic].
			      picIndex], NULL);
		DrawTPic(140, y + TH,
			 gPics[cGeneralPics[gMission.keyPics[0]].picIndex],
			 NULL);
		DrawPic(170, y + TH, gPics[gMission.exitPic], NULL);

		y += TH + 25;

		DisplayText(20, y, "Mission description",
			    yc == YC_MISSIONDESC, 0);
		y += TextHeight();

		sprintf(s, "Characters (%d/%d)",
			currentMission->baddieCount, BADDIE_MAX);
		DisplayText(20, y, s, yc == YC_CHARACTERS, 0);
		y += TextHeight();

		sprintf(s, "Mission objective characters (%d/%d)",
			currentMission->specialCount, SPECIAL_MAX);
		DisplayText(20, y, s, yc == YC_SPECIALS, 0);
		y += TextHeight();

		sprintf(s, "Available weapons (%d/%d)",
			gMission.weaponCount, WEAPON_MAX);
		DisplayText(20, y, s, yc == YC_WEAPONS, 0);
		y += TextHeight();

		sprintf(s, "Map items (%d/%d)", gMission.objectCount,
			ITEMS_MAX);
		DisplayText(20, y, s, yc == YC_ITEMS, 0);
		y += TextHeight() + 2;

		if (currentMission->objectiveCount) {
			for (i = 0; i < currentMission->objectiveCount;
			     i++) {
				DisplayText(20, y,
					    currentMission->objectives[i].
					    description,
					    yc - YC_OBJECTIVES == i, 1);
				y += TextHeight();
			}
		} else
			DisplayText(20, y, "-- mission objectives --",
				    yc == YC_OBJECTIVES, 0);
	} else if (campaign.missionCount) {
		sprintf(s, "End/%d", campaign.missionCount);
		DisplayText(270, y, s, yc == YC_MISSIONINDEX, 0);
	}

	y = 170;

	switch (yc) {
	case YC_CAMPAIGNTITLE:
		DisplayText(20, 150, campaign.author,
			    yc == YC_CAMPAIGNTITLE && xc == XC_AUTHOR, 1);
		MissionDescription(150 + TH, campaign.description,
				   yc == YC_CAMPAIGNTITLE
				   && xc == XC_CAMPAIGNDESC);
		SetSecondaryMouseRects(localCampaignClicks);
		break;

	case YC_MISSIONTITLE:
		DisplayText(20, 150, currentMission->song,
			    yc == YC_MISSIONTITLE
			    && xc == XC_MUSICFILE, 1);
		SetSecondaryMouseRects(localMissionClicks);
		break;

	case YC_MISSIONDESC:
		MissionDescription(150, currentMission->description,
				   yc == YC_MISSIONDESC);
		break;

	case YC_CHARACTERS:
		TextStringAt(5, 190,
			     "Use Insert, Delete and PageUp/PageDown");
		if (!currentMission)
			break;
		for (i = 0; i < currentMission->baddieCount; i++)
			DisplayCharacter(20 + 20 * i, y,
					 CHARACTER_OTHERS + i, xc == i);
		SetSecondaryMouseRects(localCharacterClicks);
		break;

	case YC_SPECIALS:
		TextStringAt(5, 190,
			     "Use Insert, Delete and PageUp/PageDown");
		if (!currentMission)
			break;
		for (i = 0; i < currentMission->specialCount; i++)
			DisplayCharacter(20 + 20 * i, y,
					 CHARACTER_OTHERS +
					 currentMission->baddieCount + i,
					 xc == i);
		SetSecondaryMouseRects(localCharacterClicks);
		break;

	case YC_ITEMS:
		TextStringAt(5, 190,
			     "Use Insert, Delete and PageUp/PageDown");
		if (!currentMission)
			break;
		for (i = 0; i < currentMission->itemCount; i++)
			DisplayMapItem(10 + 20 * i, y,
				       gMission.mapObjects[i],
				       currentMission->itemDensity[i],
				       xc == i);
		break;

	case YC_WEAPONS:
		if (!currentMission)
			break;
		ListWeapons(150, xc);
		break;

	default:
		if (currentMission &&
		    yc >= YC_OBJECTIVES
		    && yc - YC_OBJECTIVES <
		    currentMission->objectiveCount) {
			TextStringAt(5, 190,
				     "Use Insert, Delete and PageUp/PageDown");
			DrawObjectiveInfo(yc - YC_OBJECTIVES, y, xc);
		}
		break;
	}

	vsync();
	CopyToScreen();
}

static int Change(int yc, int xc, int d, int *mission)
{
	struct EditorInfo edInfo;
	int limit;

	if (yc == YC_MISSIONINDEX) {
		*mission += d;
		AdjustInt(mission, 0, campaign.missionCount, 0);
		return 0;
	}

	if (!currentMission)
		return 0;

	GetEditorInfo(&edInfo);

	switch (yc) {
	case YC_MISSIONPROPS:
		switch (xc) {
		case XC_WIDTH:
			currentMission->mapWidth += d;
			AdjustInt(&currentMission->mapWidth, 16, 64, 0);
			break;
		case XC_HEIGHT:
			currentMission->mapHeight += d;
			AdjustInt(&currentMission->mapHeight, 16, 64, 0);
			break;
		case XC_WALLCOUNT:
			currentMission->wallCount += d;
			AdjustInt(&currentMission->wallCount, 0, 200, 0);
			break;
		case XC_WALLLENGTH:
			currentMission->wallLength += d;
			AdjustInt(&currentMission->wallLength, 1, 100, 0);
			break;
		case XC_ROOMCOUNT:
			currentMission->roomCount += d;
			AdjustInt(&currentMission->roomCount, 0, 100, 0);
			break;
		case XC_SQRCOUNT:
			currentMission->squareCount += d;
			AdjustInt(&currentMission->squareCount, 0, 100, 0);
			break;
		case XC_DENSITY:
			currentMission->baddieDensity += d;
			AdjustInt(&currentMission->baddieDensity, 0, 100,
				  0);
			break;
		}
		break;

	case YC_MISSIONLOOKS:
		switch (xc) {
		case XC_WALL:
			currentMission->wallStyle += d;
			AdjustInt(&currentMission->wallStyle, 0,
				  WALL_COUNT - 1, 1);
			break;
		case XC_FLOOR:
			currentMission->floorStyle += d;
			AdjustInt(&currentMission->floorStyle, 0,
				  FLOOR_COUNT - 1, 1);
			break;
		case XC_ROOM:
			currentMission->roomStyle += d;
			AdjustInt(&currentMission->roomStyle, 0,
				  ROOMFLOOR_COUNT - 1, 1);
			break;
		case XC_DOORS:
			currentMission->doorStyle += d;
			AdjustInt(&currentMission->doorStyle, 0,
				  edInfo.doorCount - 1, 1);
			break;
		case XC_KEYS:
			currentMission->keyStyle += d;
			AdjustInt(&currentMission->keyStyle, 0,
				  edInfo.keyCount - 1, 1);
			break;
		case XC_EXIT:
			currentMission->exitStyle += d;
			AdjustInt(&currentMission->exitStyle, 0,
				  edInfo.exitCount - 1, 1);
			break;
		case XC_COLOR1:
			currentMission->wallRange += d;
			AdjustInt(&currentMission->wallRange, 0,
				  edInfo.rangeCount - 1, 1);
			break;
		case XC_COLOR2:
			currentMission->floorRange += d;
			AdjustInt(&currentMission->floorRange, 0,
				  edInfo.rangeCount - 1, 1);
			break;
		case XC_COLOR3:
			currentMission->roomRange += d;
			AdjustInt(&currentMission->roomRange, 0,
				  edInfo.rangeCount - 1, 1);
			break;
		case XC_COLOR4:
			currentMission->altRange += d;
			AdjustInt(&currentMission->altRange, 0,
				  edInfo.rangeCount - 1, 1);
			break;
		}
		break;

	case YC_CHARACTERS:
		currentMission->baddies[xc] += d;
		AdjustInt(&currentMission->baddies[xc], 0,
			  campaign.characterCount - 1, 1);
		break;

	case YC_SPECIALS:
		currentMission->specials[xc] += d;
		AdjustInt(&currentMission->specials[xc], 0,
			  campaign.characterCount - 1, 1);
		break;

	case YC_WEAPONS:
		currentMission->weaponSelection ^= (1 << xc);
		break;

	case YC_ITEMS:
		if (KeyDown(SDLK_LSHIFT) || KeyDown(SDLK_RSHIFT))	// Either shift key down?
		{
			currentMission->itemDensity[xc] += 5 * d;
			AdjustInt(&currentMission->itemDensity[xc], 0, 512,
				  0);
		} else {
			currentMission->items[xc] += d;
			AdjustInt(&currentMission->items[xc], 0,
				  edInfo.itemCount - 1, 1);
		}
		break;

	default:
		if (yc >= YC_OBJECTIVES) {
			switch (xc) {
			case XC_TYPE:
				currentMission->objectives[yc -
							   YC_OBJECTIVES].
				    type += d;
				AdjustInt(&currentMission->
					  objectives[yc -
						     YC_OBJECTIVES].type,
					  0, OBJECTIVE_INVESTIGATE, 1);
				d = 0;

			case XC_INDEX:
				switch (currentMission->
					objectives[yc -
						   YC_OBJECTIVES].type) {
				case OBJECTIVE_COLLECT:
					limit = edInfo.pickupCount - 1;
					break;
				case OBJECTIVE_DESTROY:
					limit = edInfo.itemCount - 1;
					break;
				case OBJECTIVE_KILL:
				case OBJECTIVE_INVESTIGATE:
					limit = 0;
					break;
				case OBJECTIVE_RESCUE:
					limit =
					    campaign.characterCount - 1;
					break;
				}
				currentMission->objectives[yc -
							   YC_OBJECTIVES].
				    index += d;
				AdjustInt(&currentMission->
					  objectives[yc -
						     YC_OBJECTIVES].index,
					  0, limit, 1);
				break;

			case XC_REQUIRED:
				currentMission->objectives[yc -
							   YC_OBJECTIVES].
				    required += d;
				AdjustInt(&currentMission->
					  objectives[yc -
						     YC_OBJECTIVES].
					  required, 0, 100, 1);
				break;

			case XC_TOTAL:
				currentMission->objectives[yc -
							   YC_OBJECTIVES].
				    count += d;
				AdjustInt(&currentMission->
					  objectives[yc -
						     YC_OBJECTIVES].count,
					  0, 100, 1);
				break;

			case XC_FLAGS:
				currentMission->objectives[yc -
							   YC_OBJECTIVES].
				    flags += d;
				AdjustInt(&currentMission->
					  objectives[yc -
						     YC_OBJECTIVES].flags,
					  0, 15, 1);
				break;
			}
		}
		break;
	}
	return 1;
}

void InsertMission(int index, struct Mission *mission)
{
	int i;

	if (campaign.missionCount == MAX_MISSIONS)
		return;

	for (i = campaign.missionCount; i > index; i--)
		campaign.missions[i] = campaign.missions[i - 1];
	if (mission)
		campaign.missions[index] = *mission;
	else {
		memset(&campaign.missions[index], 0,
		       sizeof(struct Mission));
		strcpy(campaign.missions[index].title, "Mission title");
		strcpy(campaign.missions[index].description,
		       "Briefing text");
		campaign.missions[index].mapWidth = 48;
		campaign.missions[index].mapHeight = 48;
	}
	campaign.missionCount++;
}

void DeleteMission(int *index)
{
	int i;

	if (*index >= campaign.missionCount)
		return;

	for (i = *index; i < campaign.missionCount - 1; i++)
		campaign.missions[i] = campaign.missions[i + 1];
	campaign.missionCount--;
	if (campaign.missionCount > 0 && *index >= campaign.missionCount)
		*index = campaign.missionCount - 1;
}

void AddObjective(void)
{
	currentMission->objectiveCount++;
	AdjustInt(&currentMission->objectiveCount, 0, OBJECTIVE_MAX, 0);
}

void DeleteObjective(int index)
{
	int i;

	currentMission->objectiveCount--;
	for (i = index; i < currentMission->objectiveCount; i++)
		currentMission->objectives[i] =
		    currentMission->objectives[i + 1];
	AdjustInt(&currentMission->objectiveCount, 0, OBJECTIVE_MAX, 0);
}

void DeleteCharacter(int index)
{
	int i;

	currentMission->baddieCount--;
	for (i = index; i < currentMission->baddieCount; i++)
		currentMission->baddies[i] =
		    currentMission->baddies[i + 1];
	AdjustInt(&currentMission->baddieCount, 0, BADDIE_MAX, 0);
}

void DeleteSpecial(int index)
{
	int i;

	currentMission->specialCount--;
	for (i = index; i < currentMission->specialCount; i++)
		currentMission->specials[i] =
		    currentMission->specials[i + 1];
	AdjustInt(&currentMission->specialCount, 0, SPECIAL_MAX, 0);
}

void DeleteItem(int index)
{
	int i;

	currentMission->itemCount--;
	for (i = index; i < currentMission->itemCount; i++)
		currentMission->items[i] = currentMission->items[i + 1];
	AdjustInt(&currentMission->itemCount, 0, ITEMS_MAX, 0);
}

static void Append(char *s, int maxlen, int c)
{
	int l = strlen(s);

	if (l < maxlen) {
		s[l + 1] = 0;
		s[l] = c;
	}
}

static void Backspace(char *s)
{
	if (s[0])
		s[strlen(s) - 1] = 0;
}

static void AddChar(int xc, int yc, int c)
{
	if (yc == YC_CAMPAIGNTITLE) {
		switch (xc) {
		case XC_CAMPAIGNTITLE:
			Append(campaign.title, sizeof(campaign.title) - 1,
			       c);
			break;
		case XC_AUTHOR:
			Append(campaign.author,
			       sizeof(campaign.author) - 1, c);
			break;
		case XC_CAMPAIGNDESC:
			Append(campaign.description,
			       sizeof(campaign.description) - 1, c);
			break;
		}
	}

	if (!currentMission)
		return;

	switch (yc) {
	case YC_MISSIONTITLE:
		if (xc == XC_MUSICFILE)
			Append(currentMission->song,
			       sizeof(currentMission->song) - 1, c);
		else
			Append(currentMission->title,
			       sizeof(currentMission->title) - 1, c);
		break;

	case YC_MISSIONDESC:
		Append(currentMission->description,
		       sizeof(currentMission->description) - 1, c);
		break;

	default:
		if (yc - YC_OBJECTIVES < currentMission->objectiveCount)
			Append(currentMission->
			       objectives[yc - YC_OBJECTIVES].description,
			       sizeof(currentMission->objectives[0].
				      description) - 1, c);
		break;
	}
}

static void DelChar(int xc, int yc)
{
	if (yc == YC_CAMPAIGNTITLE) {
		switch (xc) {
		case XC_CAMPAIGNTITLE:
			Backspace(campaign.title);
			break;
		case XC_AUTHOR:
			Backspace(campaign.author);
			break;
		case XC_CAMPAIGNDESC:
			Backspace(campaign.description);
			break;
		}
	}

	if (!currentMission)
		return;

	switch (yc) {
	case YC_MISSIONTITLE:
		if (xc == XC_MUSICFILE)
			Backspace(currentMission->song);
		else
			Backspace(currentMission->title);
		break;

	case YC_MISSIONDESC:
		Backspace(currentMission->description);
		break;

	default:
		if (yc - YC_OBJECTIVES < currentMission->objectiveCount)
			Backspace(currentMission->
				  objectives[yc -
					     YC_OBJECTIVES].description);
		break;
	}
}

static void AdjustYC(int *yc)
{
	if (currentMission) {
		if (currentMission->objectiveCount)
			AdjustInt(yc, 0,
				  YC_OBJECTIVES +
				  currentMission->objectiveCount - 1, 1);
		else
			AdjustInt(yc, 0, YC_OBJECTIVES, 1);
	} else
		AdjustInt(yc, 0, YC_MISSIONINDEX, 1);
}

static void AdjustXC(int yc, int *xc)
{
	switch (yc) {
	case YC_CAMPAIGNTITLE:
		AdjustInt(xc, 0, XC_CAMPAIGNDESC, 1);
		break;

	case YC_MISSIONTITLE:
		AdjustInt(xc, 0, XC_MUSICFILE, 1);
		break;

	case YC_MISSIONPROPS:
		AdjustInt(xc, 0, XC_DENSITY, 1);
		break;

	case YC_MISSIONLOOKS:
		AdjustInt(xc, 0, XC_COLOR4, 1);
		break;

	case YC_CHARACTERS:
		if (currentMission && currentMission->baddieCount)
			AdjustInt(xc, 0, currentMission->baddieCount - 1,
				  1);
		break;

	case YC_SPECIALS:
		if (currentMission && currentMission->specialCount)
			AdjustInt(xc, 0, currentMission->specialCount - 1,
				  1);
		break;

	case YC_ITEMS:
		if (currentMission && currentMission->itemCount)
			AdjustInt(xc, 0, currentMission->itemCount - 1, 1);
		break;

	case YC_WEAPONS:
		AdjustInt(xc, 0, XC_MAXWEAPONS, 1);
		break;

	default:
		if (yc >= YC_OBJECTIVES)
			AdjustInt(xc, 0, XC_FLAGS, 1);
		break;
	}
}

static void Setup(int index, int buildTables)
{
	if (index < campaign.missionCount) {
		currentMission = &campaign.missions[index];
		SetupMission(index, buildTables);
	} else
		currentMission = NULL;
}

static void Save(int asCode)
{
	char filename[128];
//      char drive[_MAX_DRIVE];
	char dir[96];
	char name[32];
//      char ext[_MAX_EXT];
	char c;
	int i;

	strcpy(filename, lastFile);
	while (1) {
		memset(GetDstScreen(), 58, Screen_GetMemSize());
		TextStringAt(125, 50, "Save as:");
		TextGoto(125, 50 + TextHeight());
		TextChar('\020');
		TextString(filename);
		TextChar('\021');
		//vsync();
		CopyToScreen();

		c = GetKey();
		switch (c) {
		case ENTER:
			if (!filename[0])
				break;
			if (asCode) {
				SaveCampaignAsC(filename, name, &campaign);
			} else {
				SaveCampaign(filename, &campaign);
			}
			fileChanged = 0;
			return;

		case ESCAPE:
			return;

		case BACKSPACE:
			if (filename[0])
				filename[strlen(filename) - 1] = 0;
			break;

		default:
			if (strlen(filename) == sizeof(filename) - 1)
				break;
			c = toupper(c);
			if ((c >= 'A' && c <= 'Z') ||
			    c == '-' || c == '_' || c == '\\') {
				i = strlen(filename);
				filename[i + 1] = 0;
				filename[i] = c;
			}
		}
	}
}

static int ConfirmQuit(void)
{
	int c;

	memset(GetDstScreen(), 58, Screen_GetMemSize());
	TextStringAt(80, 50, "Campaign has been modified, but not saved");
	TextStringAt(110, 50 + TH, "Quit anyway? (Y/N)");
	vsync();
	CopyToScreen();

	c = GetKey();
	return (c == 'Y' || c == 'y');
}

static void EditCampaign(void)
{
	int done = 0;
	int c = 0;
	int mission = 0;
	int xc = 0, yc = 0;
	int xcOld, ycOld;
	struct Mission scrap;
	struct EditorInfo edInfo;
	int x, y, buttons, tag;

	GetEditorInfo(&edInfo);

	memset(&scrap, 0, sizeof(scrap));
	SetMouseRects(localClicks);

	gCampaign.setting = &campaign;
	gCampaign.seed = 0;
	Setup(mission, 1);

	SDL_EnableKeyRepeat(0, 0);
	while (!done) {
		Display(mission, xc, yc, c);

		do {
			GetEvent(&c, &x, &y, &buttons);
			if (buttons) {
				if (GetMouseRectTag(x, y, &tag)) {
					xcOld = xc;
					ycOld = yc;
					if ((tag & LEAVE_YC) == 0) {
						yc = (tag & 0xFF);
						AdjustYC(&yc);
					}
					if ((tag & LEAVE_XC) == 0) {
						xc = ((tag >> 8) & 0xFF);
						AdjustXC(yc, &xc);
					}
					if ((tag & SELECT_ONLY) != 0)
						c = DUMMY;
					else if ((tag & SELECT_ONLY_FIRST)
						 == 0 || (xc == xcOld
							  && yc == ycOld))
						c = (buttons ==
						     1 ? PAGEUP :
						     PAGEDOWN);
					else
						c = DUMMY;
				}
			}
		}
		while (!c);

		switch (c) {
		case HOME:
			if (mission > 0)
				mission--;
			Setup(mission, 0);
			break;

		case END:
			if (mission < campaign.missionCount)
				mission++;
			Setup(mission, 0);
			break;

		case INSERT:
			switch (yc) {
			case YC_CHARACTERS:
				currentMission->baddieCount++;
				AdjustInt(&currentMission->baddieCount, 0,
					  BADDIE_MAX, 0);
				xc = currentMission->baddieCount - 1;
				break;

			case YC_SPECIALS:
				currentMission->specialCount++;
				AdjustInt(&currentMission->specialCount, 0,
					  SPECIAL_MAX, 0);
				xc = currentMission->specialCount - 1;
				break;

			case YC_ITEMS:
				currentMission->itemCount++;
				AdjustInt(&currentMission->itemCount, 0,
					  ITEMS_MAX, 0);
				xc = currentMission->itemCount - 1;
				break;

			default:
				if (yc >= YC_OBJECTIVES)
					AddObjective();
				else
					InsertMission(mission, NULL);
				break;
			}
			fileChanged = 1;
			Setup(mission, 0);
			break;

		case ALT_X:
			scrap = campaign.missions[mission];

		case DELETE:
			switch (yc) {
			case YC_CHARACTERS:
				DeleteCharacter(xc);
				break;

			case YC_SPECIALS:
				DeleteSpecial(xc);
				break;

			case YC_ITEMS:
				DeleteItem(xc);
				break;

			default:
				if (yc >= YC_OBJECTIVES)
					DeleteObjective(yc -
							YC_OBJECTIVES);
				else
					DeleteMission(&mission);
				AdjustYC(&yc);
				break;
			}
			fileChanged = 1;
			Setup(mission, 0);
			break;

		case ALT_C:
			scrap = campaign.missions[mission];
			break;

		case ALT_V:
			InsertMission(mission, &scrap);
			fileChanged = 1;
			Setup(mission, 0);
			break;

		case ALT_N:
			InsertMission(campaign.missionCount, NULL);
			mission = campaign.missionCount - 1;
			fileChanged = 1;
			Setup(mission, 0);
			break;

		case ARROW_UP:
			yc--;
			AdjustYC(&yc);
			AdjustXC(yc, &xc);
			break;

		case ARROW_DOWN:
			yc++;
			AdjustYC(&yc);
			AdjustXC(yc, &xc);
			break;

		case ARROW_LEFT:
			xc--;
			AdjustXC(yc, &xc);
			break;

		case ARROW_RIGHT:
			xc++;
			AdjustXC(yc, &xc);
			break;

		case PAGEUP:
			if (Change(yc, xc, 1, &mission))
				fileChanged = 1;
			Setup(mission, 0);
			break;

		case PAGEDOWN:
			if (Change(yc, xc, -1, &mission))
				fileChanged = 1;
			Setup(mission, 0);
			break;

		case ESCAPE:
		case ALT_Q:
			if (!fileChanged || ConfirmQuit())
				done = 1;
			break;

		case BACKSPACE:
			DelChar(xc, yc);
			fileChanged = 1;
			break;

		case ALT_S:
			Save(0);
			break;

		case ALT_H:
			Save(1);
			break;

		case ALT_M:
			Setup(mission, 0);
			SetupMap();
			DisplayAutoMap(1);
			GetKey();
			KillAllObjects();
			FreeTriggersAndWatches();
			break;

		case ALT_E:
			EditCharacters(&campaign);
			Setup(mission, 0);
			SetMouseRects(localClicks);
			break;

		default:
			if (c >= ' ' && c <= 'z') {
				fileChanged = 1;
				AddChar(xc, yc, c);
			}
			break;
		}
		SDL_Delay(50);
	}
}


struct Mission missions[MAX_MISSIONS];
TBadGuy characters[MAX_CHARACTERS];

void *myScreen;

int main(int argc, char *argv[])
{
	int i;
	int loaded = 0;

	memset(&campaign, 0, sizeof(campaign));
	strcpy(campaign.title, "Campaign title");
	memset(&missions, 0, sizeof(missions));
	campaign.missions = missions;
	memset(&characters, 0, sizeof(characters));
	campaign.characters = characters;
	currentMission = NULL;

	printf("C-Dogs Editor v0.8\n");
	printf("Copyright Ronny Wester 1996\n");

	for (i = 1; i < argc; i++) {
		if (strlen(argv[i]) > 1
		    && (*(argv[i]) == '-' || *(argv[i]) == '/')) {
			// check options here...
		} else if (!loaded) {
			memset(lastFile, 0, sizeof(lastFile));
			strncpy(lastFile, argv[i], sizeof(lastFile) - 1);
			if (strchr(lastFile, '.') == NULL &&
			    sizeof(lastFile) - strlen(lastFile) > 3) {
				strcat(lastFile, ".CPN");
			}
			if (LoadCampaign
			    (lastFile, &campaign, MAX_MISSIONS,
			     MAX_CHARACTERS) == CAMPAIGN_OK)
				loaded = 1;
		}
	}

	printf("Data directory:\t\t%s\n",	GetDataFilePath(""));
	printf("Config directory:\t%s\n\n",	GetConfigFilePath(""));

	if (!ReadPics(GetDataFilePath("graphics/cdogs.px"), gPics, PIC_COUNT1, gPalette)) {
		printf("Unable to read CDOGS.PX\n");
		exit(0);
	}
	if (!AppendPics(GetDataFilePath("graphics/cdogs2.px"), gPics, PIC_COUNT1, PIC_MAX)) {
		printf("Unable to read CDOGS2.PX\n");
		exit(0);
	}
	gPalette[0].red = gPalette[0].green = gPalette[0].blue = 0;
	memcpy(origPalette, gPalette, sizeof(origPalette));
	InitializeTranslationTables();
	BuildTranslationTables();
	TextInit(GetDataFilePath("graphics/font.px"), -2, YES, YES);

	//InitVideo(YES);
	
	Gfx_SetHint(HINT_WIDTH, 320);
	Gfx_SetHint(HINT_HEIGHT, 240);
	Gfx_SetHint(HINT_SCALEFACTOR, 2);
	
	if (InitVideo() == -1) {
		printf("Video didn't init!\n");
		exit(EXIT_FAILURE);
	}

	SetPalette(gPalette);

	myScreen = sys_mem_alloc(Screen_GetMemSize());
	memset(myScreen, 0, Screen_GetMemSize());
	SetDstScreen(myScreen);

	InitMouse();
	EditCampaign();

	free(myScreen);

	//TextMode();
	exit(EXIT_SUCCESS);
}
