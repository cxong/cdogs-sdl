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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include <SDL.h>

#include <cdogs/actors.h>
#include <cdogs/automap.h>
#include <cdogs/config.h>
#include <cdogs/events.h>
#include <cdogs/files.h>
#include <cdogs/grafx.h>
#include <cdogs/keyboard.h>
#include <cdogs/mission.h>
#include <cdogs/objs.h>
#include <cdogs/pics.h>
#include <cdogs/text.h>
#include <cdogs/triggers.h>
#include <cdogs/utils.h>

#include "charsed.h"


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

static MouseRect localClicks[] =
{
	{25, 5, 265, 5 + TH - 1, YC_CAMPAIGNTITLE + SELECT_ONLY_FIRST},
	{270, 5, 319, 5 + TH - 1, YC_MISSIONINDEX},
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

static MouseRect localCampaignClicks[] =
{
	{0, 150, 319, 150 + TH - 1,
	 YC_CAMPAIGNTITLE + (XC_AUTHOR << 8) + SELECT_ONLY},
	{0, 150 + TH, 319, 190,
	 YC_CAMPAIGNTITLE + (XC_CAMPAIGNDESC << 8) + SELECT_ONLY},

	{0, 0, 0, 0, 0}
};

static MouseRect localMissionClicks[] =
{
	{0, 150, 319, 160,
	 YC_MISSIONTITLE + (XC_MUSICFILE << 8) + SELECT_ONLY},

	{0, 0, 0, 0, 0}
};

static MouseRect localWeaponClicks[] =
{
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

static MouseRect localMapItemClicks[] =
{
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

static MouseRect localObjectiveClicks[] =
{
	{20, 150, 55, 190, LEAVE_YC + (XC_TYPE << 8)},
	{60, 150, 85, 190, LEAVE_YC + (XC_INDEX << 8)},
	{90, 150, 105, 190, LEAVE_YC + (XC_REQUIRED << 8)},
	{110, 150, 145, 190, LEAVE_YC + (XC_TOTAL << 8)},
	{150, 150, 195, 190, LEAVE_YC + (XC_FLAGS << 8)},

	{0, 0, 0, 0, 0}
};

static MouseRect localCharacterClicks[] =
{
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

struct Mission *currentMission;
static char lastFile[CDOGS_FILENAME_MAX];



// Code...

void DisplayCDogsText(int x, int y, const char *text, int hilite, int editable)
{
	CDogsTextGoto(x, y);
	if (editable) {
		if (hilite)
			CDogsTextCharWithTable('\020', &tableFlamed);
		else
			CDogsTextChar('\020');
	}

	if (hilite && !editable)
		CDogsTextStringWithTable(text, &tableFlamed);
	else
		CDogsTextString(text);

	if (editable) {
		if (hilite)
			CDogsTextCharWithTable('\021', &tableFlamed);
		else
			CDogsTextChar('\021');
	}
}

void DrawObjectiveInfo(int idx, int y, int xc)
{
	TOffsetPic pic;
	TranslationTable *table = NULL;
	int i;
	const char *typeCDogsText;
	char s[50];

	switch (currentMission->objectives[idx].type)
	{
	case OBJECTIVE_KILL:
		typeCDogsText = "Kill";
		i = gCharacterDesc[currentMission->baddieCount +
				  CHARACTER_OTHERS].facePic;
		table =
		    &gCharacterDesc[currentMission->baddieCount +
				  CHARACTER_OTHERS].table;
		pic.picIndex = cHeadPic[i][DIRECTION_DOWN][STATE_IDLE];
		pic.dx = cHeadOffset[i][DIRECTION_DOWN].dx;
		pic.dy = cHeadOffset[i][DIRECTION_DOWN].dy;
		break;
	case OBJECTIVE_RESCUE:
		typeCDogsText = "Rescue";
		i = gCharacterDesc[CHARACTER_PRISONER].facePic;
		table = &gCharacterDesc[CHARACTER_PRISONER].table;
		pic.picIndex = cHeadPic[i][DIRECTION_DOWN][STATE_IDLE];
		pic.dx = cHeadOffset[i][DIRECTION_DOWN].dx;
		pic.dy = cHeadOffset[i][DIRECTION_DOWN].dy;
		break;
	case OBJECTIVE_COLLECT:
		typeCDogsText = "Collect";
		i = gMission.objectives[idx].pickupItem;
		pic = cGeneralPics[i];
		break;
	case OBJECTIVE_DESTROY:
		typeCDogsText = "Destroy";
		i = gMission.objectives[idx].blowupObject->pic;
		pic = cGeneralPics[i];
		break;
	case OBJECTIVE_INVESTIGATE:
		typeCDogsText = "Explore";
		pic.dx = pic.dy = 0;
		pic.picIndex = -1;
		break;
	default:
		typeCDogsText = "???";
		i = gMission.objectives[idx].pickupItem;
		pic = cGeneralPics[i];
		break;
	}

	DisplayCDogsText(20, y, typeCDogsText, xc == XC_TYPE, 0);

	if (pic.picIndex >= 0)
	{
		DrawTTPic(60 + pic.dx, y + 8 + pic.dy, gPics[pic.picIndex], table);
	}

	sprintf(s, "%d", currentMission->objectives[idx].required);
	DisplayCDogsText(90, y, s, xc == XC_REQUIRED, 0);
	sprintf(s, "out of %d", currentMission->objectives[idx].count);
	DisplayCDogsText(110, y, s, xc == XC_TOTAL, 0);

	sprintf(s, "%s %s %s %s %s",
		(currentMission->objectives[idx].flags & OBJECTIVE_HIDDEN) != 0 ? "hidden" : "",
		(currentMission->objectives[idx].flags & OBJECTIVE_POSKNOWN) != 0 ? "pos.known" : "",
		(currentMission->objectives[idx].flags & OBJECTIVE_HIACCESS) != 0 ? "access" : "",
		(currentMission->objectives[idx].flags & OBJECTIVE_UNKNOWNCOUNT) != 0 ? "no-count" : "",
		(currentMission->objectives[idx].flags & OBJECTIVE_NOACCESS) != 0 ? "no-access" : "");
	DisplayCDogsText(150, y, s, xc == XC_FLAGS, 0);

	MouseSetSecondaryRects(&gInputDevices.mouse, localObjectiveClicks);
}

static int MissionDescription(
	int y, const char *description, const char *hint, int hilite)
{
	int lines;
	const char *p, *s;
	int isEmptyText = strlen(description) == 0;
	color_t bracketMask = hilite ? colorRed : colorWhite;
	color_t textMask = isEmptyText ? colorGray : colorWhite;
	Vec2i pos;
	if (isEmptyText)
	{
		description = hint;
	}

	pos = Vec2iNew(20 - CDogsTextCharWidth('\020'), y);
	pos = DrawTextCharMasked('\020', &gGraphicsDevice, pos, bracketMask);

	pos.x = 20;
	lines = 1;

	s = description;
	while (*s) {
		// Find word
		// First find spaces before a word, then find the word
		// e.g.: "       abc123  def"
		//        ^      ^     ^
		//        |      |     |
		//     wordSpace word end
		const char *wordSpace = s;
		const char *word;
		int wordSpaceWidth;
		int wordWidth;
		while (*s == ' ' || *s == '\n')
			s++;
		word = s;
		while (*s != 0 && *s != ' ' && *s != '\n')
			s++;

		wordSpaceWidth = TextGetSubstringWidth(wordSpace, word - wordSpace);
		wordWidth = TextGetSubstringWidth(word, s - word);

		if (pos.x + wordSpaceWidth + wordWidth > 300 &&
			wordSpaceWidth + wordWidth < 280)
		{
			pos.y += CDogsTextHeight();
			pos.x = 20;
			lines++;
			wordSpace = word;
			wordSpaceWidth = 0;
		}

		pos.x += wordSpaceWidth;

		for (p = word; p < s; p++)
		{
			pos = DrawTextCharMasked(*p, &gGraphicsDevice, pos, textMask);
		}
	}

	pos = DrawTextCharMasked('\021', &gGraphicsDevice, pos, bracketMask);

	return lines;
}

void DisplayCharacter(int x, int y, int character, int hilite)
{
	struct CharacterDescription *cd;
	TOffsetPic body, head;

	cd = &gCharacterDesc[character];

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

	DrawTTPic(x + body.dx, y + body.dy, gPics[body.picIndex], cd->table);
	DrawTTPic(x + head.dx, y + head.dy, gPics[head.picIndex], cd->table);

	if (hilite) {
		CDogsTextGoto(x - 8, y - 16);
		CDogsTextChar('\020');
		CDogsTextGoto(x - 8, y + 8);
		CDogsTextString(gGunDescriptions[cd->defaultGun].gunName);
	}
}

static void ShowWeaponStatus(int x, int y, int weapon, int xc)
{
	DisplayFlag(
		x,
		y,
		gGunDescriptions[weapon].gunName,
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

	MouseSetSecondaryRects(&gInputDevices.mouse, localWeaponClicks);
}

static void DrawEditableTextWithEmptyHint(
	Vec2i pos, char *text, char *hint, int isHighlighted)
{
	int isEmptyText = strlen(text) == 0;
	color_t bracketMask = isHighlighted ? colorRed : colorWhite;
	color_t textMask = isEmptyText ? colorGray : colorWhite;
	if (isEmptyText)
	{
		text = hint;
	}
	pos = DrawTextCharMasked('\020', &gGraphicsDevice, pos, bracketMask);
	pos = DrawTextStringMasked(text, &gGraphicsDevice, pos, textMask);
	pos = DrawTextCharMasked('\021', &gGraphicsDevice, pos, bracketMask);
}

void DisplayMapItem(int x, int y, TMapObject * mo, int density, int hilite)
{
	char s[10];

	const TOffsetPic *pic = &cGeneralPics[mo->pic];
	DrawTPic(x + pic->dx, y + pic->dy, gPics[pic->picIndex]);

	if (hilite) {
		CDogsTextGoto(x - 8, y - 4);
		CDogsTextChar('\020');
	}
	sprintf(s, "%d", density);
	CDogsTextGoto(x - 8, y + 5);
	CDogsTextString(s);

	MouseSetSecondaryRects(&gInputDevices.mouse, localMapItemClicks);
}

static void DrawStyleArea(
	Vec2i pos,
	const char *name,
	GraphicsDevice *device,
	Pic *pic,
	int index, int count,
	int isHighlighted)
{
	char buf[16];
	DisplayCDogsText(pos.x, pos.y, name, isHighlighted, 0);
	DrawPic(
		pos.x, pos.y + TH,
		pic);
	// Display style index and count, right aligned
	sprintf(buf, "%d/%d", index + 1, count);
	DrawTextStringMasked(
		buf,
		device,
		Vec2iNew(pos.x + 28 - TextGetStringWidth(buf), pos.y + 25),
		colorGray);
}

static void DisplayMission(int idx, int xc, int yc, int y)
{
	char s[128];
	struct EditorInfo ei;
	sprintf(s, "Mission %d/%d", idx + 1, gCampaign.Setting.missionCount);
	DisplayCDogsText(270, y, s, yc == YC_MISSIONINDEX, 0);

	y += CDogsTextHeight() + 3;
	DrawEditableTextWithEmptyHint(
		Vec2iNew(25, y),
		currentMission->title, "(Mission title)",
		yc == YC_MISSIONTITLE && xc == XC_MISSIONTITLE);

	y += CDogsTextHeight() + 2;

	sprintf(s, "Width: %d", currentMission->mapWidth);
	DisplayCDogsText(20, y, s, yc == YC_MISSIONPROPS && xc == XC_WIDTH, 0);

	sprintf(s, "Height: %d", currentMission->mapHeight);
	DisplayCDogsText(60, y, s, yc == YC_MISSIONPROPS && xc == XC_HEIGHT, 0);

	sprintf(s, "Walls: %d", currentMission->wallCount);
	DisplayCDogsText(100, y, s, yc == YC_MISSIONPROPS && xc == XC_WALLCOUNT, 0);

	sprintf(s, "Len: %d", currentMission->wallLength);
	DisplayCDogsText(140, y, s, yc == YC_MISSIONPROPS && xc == XC_WALLLENGTH, 0);

	sprintf(s, "Rooms: %d", currentMission->roomCount);
	DisplayCDogsText(180, y, s, yc == YC_MISSIONPROPS && xc == XC_ROOMCOUNT, 0);

	sprintf(s, "Sqr: %d", currentMission->squareCount);
	DisplayCDogsText(220, y, s, yc == YC_MISSIONPROPS && xc == XC_SQRCOUNT, 0);

	sprintf(s, "Dens: %d", currentMission->baddieDensity);
	DisplayCDogsText(260, y, s, yc == YC_MISSIONPROPS && xc == XC_DENSITY, 0);

	y += CDogsTextHeight();

	DrawStyleArea(
		Vec2iNew(20, y),
		"Wall",
		&gGraphicsDevice,
		gPics[cWallPics[currentMission->wallStyle % WALL_STYLE_COUNT][WALL_SINGLE]],
		currentMission->wallStyle, WALL_STYLE_COUNT,
		yc == YC_MISSIONLOOKS && xc == XC_WALL);
	DrawStyleArea(
		Vec2iNew(50, y),
		"Floor",
		&gGraphicsDevice,
		gPics[cFloorPics[currentMission->floorStyle % FLOOR_STYLE_COUNT][FLOOR_NORMAL]],
		currentMission->floorStyle, FLOOR_STYLE_COUNT,
		yc == YC_MISSIONLOOKS && xc == XC_FLOOR);
	DrawStyleArea(
		Vec2iNew(80, y),
		"Rooms",
		&gGraphicsDevice,
		gPics[cRoomPics[currentMission->roomStyle % ROOMFLOOR_COUNT][ROOMFLOOR_NORMAL]],
		currentMission->roomStyle, ROOMFLOOR_COUNT,
		yc == YC_MISSIONLOOKS && xc == XC_ROOM);
	GetEditorInfo(&ei);
	DrawStyleArea(
		Vec2iNew(110, y),
		"Doors",
		&gGraphicsDevice,
		gPics[cGeneralPics[gMission.doorPics[0].horzPic].picIndex],
		currentMission->doorStyle, ei.doorCount,
		yc == YC_MISSIONLOOKS && xc == XC_DOORS);
	DrawStyleArea(
		Vec2iNew(140, y),
		"Keys",
		&gGraphicsDevice,
		gPics[cGeneralPics[gMission.keyPics[0]].picIndex],
		currentMission->keyStyle, ei.keyCount,
		yc == YC_MISSIONLOOKS && xc == XC_KEYS);
	DrawStyleArea(
		Vec2iNew(170, y),
		"Exit",
		&gGraphicsDevice,
		gPics[gMission.exitPic],
		currentMission->exitStyle, ei.exitCount,
		yc == YC_MISSIONLOOKS && xc == XC_EXIT);

	sprintf(s, "Walls: %s", RangeName(currentMission->wallRange));
	DisplayCDogsText(
		200, y, s, yc == YC_MISSIONLOOKS && xc == XC_COLOR1, 0);
	sprintf(s, "Floor: %s", RangeName(currentMission->floorRange));
	DisplayCDogsText(
		200, y + TH, s, yc == YC_MISSIONLOOKS && xc == XC_COLOR2, 0);
	sprintf(s, "Rooms: %s", RangeName(currentMission->roomRange));
	DisplayCDogsText(
		200, y + 2 * TH, s, yc == YC_MISSIONLOOKS && xc == XC_COLOR3, 0);
	sprintf(s, "Extra: %s", RangeName(currentMission->altRange));
	DisplayCDogsText(
		200, y + 3 * TH, s, yc == YC_MISSIONLOOKS && xc == XC_COLOR4, 0);

	y += TH + 25;

	DisplayCDogsText(20, y, "Mission description", yc == YC_MISSIONDESC, 0);
	y += CDogsTextHeight();

	sprintf(s, "Characters (%d/%d)", currentMission->baddieCount, BADDIE_MAX);
	DisplayCDogsText(20, y, s, yc == YC_CHARACTERS, 0);
	y += CDogsTextHeight();

	sprintf(
		s,
		"Mission objective characters (%d/%d)",
		currentMission->specialCount,
		SPECIAL_MAX);
	DisplayCDogsText(20, y, s, yc == YC_SPECIALS, 0);
	y += CDogsTextHeight();

	sprintf(s, "Available weapons (%d/%d)", gMission.weaponCount, WEAPON_MAX);
	DisplayCDogsText(20, y, s, yc == YC_WEAPONS, 0);
	y += CDogsTextHeight();

	sprintf(s, "Map items (%d/%d)", gMission.objectCount, ITEMS_MAX);
	DisplayCDogsText(20, y, s, yc == YC_ITEMS, 0);
	y += CDogsTextHeight() + 2;

	if (currentMission->objectiveCount)
	{
		int i;
		for (i = 0; i < currentMission->objectiveCount; i++)
		{
			DrawEditableTextWithEmptyHint(
				Vec2iNew(20, y),
				currentMission->objectives[i].description,
				"(Objective description)",
				yc - YC_OBJECTIVES == i);
			y += CDogsTextHeight();
		}
	}
	else
	{
		DisplayCDogsText(
			20, y, "-- mission objectives --", yc == YC_OBJECTIVES, 0);
	}
}

void Display(int idx, int xc, int yc, int key)
{
	char s[128];
	int y = 5;
	int i;

	MouseSetSecondaryRects(&gInputDevices.mouse, NULL);
	if (currentMission)
	{
		GraphicsBlitBkg(&gGraphicsDevice);
	}
	else
	{
		for (i = 0; i < GraphicsGetScreenSize(&gGraphicsDevice.cachedConfig); i++)
		{
			gGraphicsDevice.buf[i] = LookupPalette(58);
		}
	}

	sprintf(s, "Key: 0x%x", key);
	CDogsTextStringAt(270, 190, s);

	DrawEditableTextWithEmptyHint(
		Vec2iNew(25, y),
		gCampaign.Setting.title, "(Campaign title)",
		yc == YC_CAMPAIGNTITLE && xc == XC_CAMPAIGNTITLE);

	if (fileChanged)
	{
		DrawTPic(10, y, gPics[221]);
	}

	DrawTextString("Press F1 for help", &gGraphicsDevice, Vec2iNew(20, 200));

	if (currentMission)
	{
		DisplayMission(idx, xc, yc, y);
	}
	else if (gCampaign.Setting.missionCount)
	{
		sprintf(s, "End/%d", gCampaign.Setting.missionCount);
		DisplayCDogsText(270, y, s, yc == YC_MISSIONINDEX, 0);
	}

	y = 170;

	switch (yc) {
	case YC_CAMPAIGNTITLE:
		DrawEditableTextWithEmptyHint(
			Vec2iNew(25, 150),
			gCampaign.Setting.author, "(Campaign author)",
			yc == YC_CAMPAIGNTITLE && xc == XC_AUTHOR);
		MissionDescription(
			150 + TH, gCampaign.Setting.description, "(Campaign description)",
			yc == YC_CAMPAIGNTITLE && xc == XC_CAMPAIGNDESC);
		MouseSetSecondaryRects(&gInputDevices.mouse, localCampaignClicks);
		break;

	case YC_MISSIONTITLE:
		DrawEditableTextWithEmptyHint(
			Vec2iNew(20, 150),
			currentMission->song, "(Mission song)",
			yc == YC_MISSIONTITLE && xc == XC_MUSICFILE);
		MouseSetSecondaryRects(&gInputDevices.mouse, localMissionClicks);
		break;

	case YC_MISSIONDESC:
		MissionDescription(
			150,
			currentMission->description, "(Mission description)",
			yc == YC_MISSIONDESC);
		break;

	case YC_CHARACTERS:
		CDogsTextStringAt(5, 190, "Use Insert, Delete and PageUp/PageDown");
		if (!currentMission)
		{
			break;
		}
		for (i = 0; i < currentMission->baddieCount; i++)
		{
			DisplayCharacter(20 + 20 * i, y, CHARACTER_OTHERS + i, xc == i);
		}
		MouseSetSecondaryRects(&gInputDevices.mouse, localCharacterClicks);
		break;

	case YC_SPECIALS:
		CDogsTextStringAt(5, 190, "Use Insert, Delete and PageUp/PageDown");
		if (!currentMission)
		{
			break;
		}
		for (i = 0; i < currentMission->specialCount; i++)
		{
			DisplayCharacter(
				20 + 20 * i, y,
				CHARACTER_OTHERS + currentMission->baddieCount + i,
				xc == i);
		}
		MouseSetSecondaryRects(&gInputDevices.mouse, localCharacterClicks);
		break;

	case YC_ITEMS:
		CDogsTextStringAt(5, 190,
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
			CDogsTextStringAt(5, 190,
				     "Use Insert, Delete and PageUp/PageDown");
			DrawObjectiveInfo(yc - YC_OBJECTIVES, y, xc);
		}
		break;
	}

	MouseDraw(&gInputDevices.mouse);
	BlitFlip(&gGraphicsDevice, &gConfig.Graphics);
}

static int Change(int yc, int xc, int d, int *mission)
{
	struct EditorInfo edInfo;
	int limit;
	int isChanged = 0;

	if (yc == YC_MISSIONINDEX) {
		*mission += d;
		*mission = CLAMP(*mission, 0, gCampaign.Setting.missionCount);
		return 0;
	}

	if (!currentMission)
		return 0;

	GetEditorInfo(&edInfo);

	switch (yc) {
	case YC_MISSIONPROPS:
		switch (xc) {
		case XC_WIDTH:
			currentMission->mapWidth = CLAMP(currentMission->mapWidth + d, 16, 64);
			break;
		case XC_HEIGHT:
			currentMission->mapHeight = CLAMP(currentMission->mapHeight + d, 16, 64);
			break;
		case XC_WALLCOUNT:
			currentMission->wallCount = CLAMP(currentMission->wallCount + d, 0, 200);
			break;
		case XC_WALLLENGTH:
			currentMission->wallLength = CLAMP(currentMission->wallLength + d, 1, 100);
			break;
		case XC_ROOMCOUNT:
			currentMission->roomCount = CLAMP(currentMission->roomCount + d, 0, 100);
			break;
		case XC_SQRCOUNT:
			currentMission->squareCount = CLAMP(currentMission->squareCount + d, 0, 100);
			break;
		case XC_DENSITY:
			currentMission->baddieDensity = CLAMP(currentMission->baddieDensity + d, 0, 100);
			break;
		}
		isChanged = 1;
		break;

	case YC_MISSIONLOOKS:
		switch (xc) {
		case XC_WALL:
			currentMission->wallStyle = CLAMP_OPPOSITE(
				(int)(currentMission->wallStyle + d), 0, WALL_STYLE_COUNT - 1);
			break;
		case XC_FLOOR:
			currentMission->floorStyle = CLAMP_OPPOSITE(
				(int)(currentMission->floorStyle + d),
				0,
				FLOOR_STYLE_COUNT - 1);
			break;
		case XC_ROOM:
			currentMission->roomStyle = CLAMP_OPPOSITE(
				(int)(currentMission->roomStyle + d), 0, ROOMFLOOR_COUNT - 1);
			break;
		case XC_DOORS:
			currentMission->doorStyle =
				CLAMP_OPPOSITE(currentMission->doorStyle + d, 0, edInfo.doorCount - 1);
			break;
		case XC_KEYS:
			currentMission->keyStyle = CLAMP_OPPOSITE(
				currentMission->keyStyle + d, 0, edInfo.keyCount - 1);
			break;
		case XC_EXIT:
			currentMission->exitStyle = CLAMP_OPPOSITE(
				currentMission->exitStyle + d, 0, edInfo.exitCount - 1);
			break;
		case XC_COLOR1:
			currentMission->wallRange = CLAMP_OPPOSITE(
				currentMission->wallRange + d, 0, edInfo.rangeCount - 1);
			break;
		case XC_COLOR2:
			currentMission->floorRange = CLAMP_OPPOSITE(
				currentMission->floorRange + d, 0, edInfo.rangeCount - 1);
			break;
		case XC_COLOR3:
			currentMission->roomRange = CLAMP_OPPOSITE(
				currentMission->roomRange + d, 0, edInfo.rangeCount - 1);
			break;
		case XC_COLOR4:
			currentMission->altRange = CLAMP_OPPOSITE(
				currentMission->altRange + d, 0, edInfo.rangeCount - 1);
			break;
		}
		isChanged = 1;
		break;

	case YC_CHARACTERS:
		currentMission->baddies[xc] = CLAMP_OPPOSITE(
			currentMission->baddies[xc] + d,
			0,
			gCampaign.Setting.characterCount - 1);
		isChanged = 1;
		break;

	case YC_SPECIALS:
		currentMission->specials[xc] = CLAMP_OPPOSITE(
			currentMission->specials[xc] + d,
			0,
			gCampaign.Setting.characterCount - 1);
		isChanged = 1;
		break;

	case YC_WEAPONS:
		currentMission->weaponSelection ^= (1 << xc);
		isChanged = 1;
		break;

	case YC_ITEMS:
		if (gInputDevices.keyboard.modState & KMOD_SHIFT)
		{
			currentMission->itemDensity[xc] =
				CLAMP(currentMission->itemDensity[xc] +  5 * d, 0, 512);
		}
		else
		{
			currentMission->items[xc] = CLAMP_OPPOSITE(
				currentMission->items[xc] + d, 0, edInfo.itemCount - 1);
		}
		isChanged = 1;
		break;

	default:
		if (yc >= YC_OBJECTIVES)
		{
			struct MissionObjective *objective =
				&currentMission->objectives[yc - YC_OBJECTIVES];
			switch (xc)
			{
			case XC_TYPE:
				objective->type = CLAMP_OPPOSITE(
						objective->type + d, 0, OBJECTIVE_INVESTIGATE);
				d = 0;
				// fallthrough

			case XC_INDEX:
				switch (objective->type)
				{
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
					limit = gCampaign.Setting.characterCount - 1;
					break;
				default:
					// should never get here
					return 0;
				}
				objective->index =
					CLAMP_OPPOSITE(objective->index + d, 0, limit);
				isChanged = 1;
				break;

			case XC_REQUIRED:
				objective->required = CLAMP_OPPOSITE(
					objective->required + d, 0, MIN(100, objective->count));
				isChanged = 1;
				break;

			case XC_TOTAL:
				objective->count = CLAMP_OPPOSITE(
					objective->count + d, objective->required, 100);
				isChanged = 1;
				break;

			case XC_FLAGS:
				objective->flags = CLAMP_OPPOSITE(objective->flags + d, 0, 15);
				isChanged = 1;
				break;
			}
		}
		break;
	}
	return isChanged;
}

void InsertMission(int idx, struct Mission *mission)
{
	int i;
	CREALLOC(
		gCampaign.Setting.missions,
		sizeof *gCampaign.Setting.missions * (gCampaign.Setting.missionCount + 1));
	for (i = gCampaign.Setting.missionCount; i > idx; i--)
	{
		gCampaign.Setting.missions[i] = gCampaign.Setting.missions[i - 1];
	}
	if (mission)
	{
		gCampaign.Setting.missions[idx] = *mission;
	}
	else
	{
		memset(&gCampaign.Setting.missions[idx], 0, sizeof(struct Mission));
		strcpy(gCampaign.Setting.missions[idx].title, "");
		strcpy(gCampaign.Setting.missions[idx].description, "");
		gCampaign.Setting.missions[idx].mapWidth = 48;
		gCampaign.Setting.missions[idx].mapHeight = 48;
	}
	gCampaign.Setting.missionCount++;
}

void DeleteMission(int *idx)
{
	int i;

	if (*idx >= gCampaign.Setting.missionCount)
	{
		return;
	}

	for (i = *idx; i < gCampaign.Setting.missionCount - 1; i++)
	{
		gCampaign.Setting.missions[i] = gCampaign.Setting.missions[i + 1];
	}
	gCampaign.Setting.missionCount--;
	if (gCampaign.Setting.missionCount > 0 && *idx >= gCampaign.Setting.missionCount)
	{
		*idx = gCampaign.Setting.missionCount - 1;
	}
}

void AddObjective(void)
{
	currentMission->objectiveCount =
		CLAMP(currentMission->objectiveCount + 1, 0, OBJECTIVE_MAX);
}

void DeleteObjective(int idx)
{
	int i;

	currentMission->objectiveCount =
		CLAMP(currentMission->objectiveCount - 1, 0, OBJECTIVE_MAX);
	for (i = idx; i < currentMission->objectiveCount; i++)
	{
		currentMission->objectives[i] = currentMission->objectives[i + 1];
	}
}

void DeleteCharacter(int idx)
{
	int i;

	currentMission->baddieCount =
		CLAMP(currentMission->baddieCount - 1, 0, BADDIE_MAX);
	for (i = idx; i < currentMission->baddieCount; i++)
	{
		currentMission->baddies[i] = currentMission->baddies[i + 1];
	}
}

void DeleteSpecial(int idx)
{
	int i;

	currentMission->specialCount =
		CLAMP(currentMission->specialCount - 1, 0, SPECIAL_MAX);
	for (i = idx; i < currentMission->specialCount; i++)
	{
		currentMission->specials[i] = currentMission->specials[i + 1];
	}
}

void DeleteItem(int idx)
{
	int i;

	currentMission->itemCount =
		CLAMP(currentMission->itemCount - 1, 0, ITEMS_MAX);
	for (i = idx; i < currentMission->itemCount; i++)
	{
		currentMission->items[i] = currentMission->items[i + 1];
	}
}

static void Append(char *s, int maxlen, char c)
{
	size_t l = strlen(s);

	if ((int)l < maxlen)
	{
		s[l + 1] = 0;
		s[l] = c;
	}
}

static void Backspace(char *s)
{
	if (s[0])
		s[strlen(s) - 1] = 0;
}

static void AddChar(int xc, int yc, char c)
{
	if (yc == YC_CAMPAIGNTITLE) {
		switch (xc) {
		case XC_CAMPAIGNTITLE:
			Append(
				gCampaign.Setting.title,
				sizeof(gCampaign.Setting.title) - 1,
				c);
			break;
		case XC_AUTHOR:
			Append(
				gCampaign.Setting.author,
				sizeof(gCampaign.Setting.author) - 1,
				c);
			break;
		case XC_CAMPAIGNDESC:
			Append(
				gCampaign.Setting.description,
				sizeof(gCampaign.Setting.description) - 1,
				c);
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
			Backspace(gCampaign.Setting.title);
			break;
		case XC_AUTHOR:
			Backspace(gCampaign.Setting.author);
			break;
		case XC_CAMPAIGNDESC:
			Backspace(gCampaign.Setting.description);
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
	if (currentMission != NULL)
	{
		if (currentMission->objectiveCount)
		{
			*yc = CLAMP_OPPOSITE(
				*yc, 0, YC_OBJECTIVES + currentMission->objectiveCount - 1);
		}
		else
		{
			*yc = CLAMP_OPPOSITE(*yc, 0, YC_OBJECTIVES);
		}
	}
	else
	{
		*yc = CLAMP_OPPOSITE(*yc, 0, YC_MISSIONINDEX);
	}
}

static void AdjustXC(int yc, int *xc)
{
	switch (yc)
	{
	case YC_CAMPAIGNTITLE:
		*xc = CLAMP_OPPOSITE(*xc, 0, XC_CAMPAIGNDESC);
		break;

	case YC_MISSIONTITLE:
		*xc = CLAMP_OPPOSITE(*xc, 0, XC_MUSICFILE);
		break;

	case YC_MISSIONPROPS:
		*xc = CLAMP_OPPOSITE(*xc, 0, XC_DENSITY);
		break;

	case YC_MISSIONLOOKS:
		*xc = CLAMP_OPPOSITE(*xc, 0, XC_COLOR4);
		break;

	case YC_CHARACTERS:
		if (currentMission && currentMission->baddieCount > 0)
		{
			*xc = CLAMP_OPPOSITE(*xc, 0, currentMission->baddieCount - 1);
		}
		break;

	case YC_SPECIALS:
		if (currentMission && currentMission->specialCount > 0)
		{
			*xc = CLAMP_OPPOSITE(*xc, 0, currentMission->specialCount - 1);
		}
		break;

	case YC_ITEMS:
		if (currentMission && currentMission->itemCount > 0)
		{
			*xc = CLAMP_OPPOSITE(*xc, 0, currentMission->itemCount - 1);
		}
		break;

	case YC_WEAPONS:
		*xc = CLAMP_OPPOSITE(*xc, 0, XC_MAXWEAPONS);
		break;

	default:
		if (yc >= YC_OBJECTIVES)
		{
			*xc = CLAMP_OPPOSITE(*xc, 0, XC_FLAGS);
		}
		break;
	}
}

static void MoveSelection(int isForward, int *y, int *x)
{
	if (isForward)
	{
		(*y)++;
	}
	else
	{
		(*y)--;
	}
	AdjustYC(y);
	AdjustXC(*y, x);
}

static void Setup(int idx, int buildTables)
{
	if (idx >= gCampaign.Setting.missionCount)
	{
		currentMission = NULL;
		return;
	}
	currentMission = &gCampaign.Setting.missions[idx];
	SetupMission(idx, buildTables, &gCampaign);
	GrafxMakeBackground(&gGraphicsDevice, &gConfig.Graphics, tintDarker, idx);
}

static void Save(int asCode)
{
	char filename[CDOGS_FILENAME_MAX];
	char name[32];
	int c;

	strcpy(filename, lastFile);
	for (;;)
	{
		int i;
		for (i = 0; i < GraphicsGetScreenSize(&gGraphicsDevice.cachedConfig); i++)
		{
			gGraphicsDevice.buf[i] = LookupPalette(58);
		}
		CDogsTextStringAt(125, 50, "Save as:");
		CDogsTextGoto(125, 50 + CDogsTextHeight());
		CDogsTextChar('\020');
		CDogsTextString(filename);
		CDogsTextChar('\021');
		BlitFlip(&gGraphicsDevice, &gConfig.Graphics);

		c = GetKey(&gInputDevices);
		switch (c)
		{
		case SDLK_RETURN:
		case SDLK_KP_ENTER:
			if (!filename[0])
				break;
			if (asCode)
			{
				SaveCampaignAsC(filename, name, &gCampaign.Setting);
			}
			else
			{
				SaveCampaign(filename, &gCampaign.Setting);
			}
			fileChanged = 0;
			strcpy(lastFile, filename);
			return;

		case SDLK_ESCAPE:
			return;

		case SDLK_BACKSPACE:
			if (filename[0])
				filename[strlen(filename) - 1] = 0;
			break;

		default:
			if (strlen(filename) == sizeof(filename) - 1)
			{
				break;
			}
			c = KeyGetTyped(&gInputDevices.keyboard);
			if (c && c != '*' &&
				(strlen(filename) > 1 || c != '-') && c != '/' &&
				c != ':' && c != '<' && c != '>' && c != '?' &&
				c != '\\' && c != '|')
			{
				size_t i = strlen(filename);
				filename[i + 1] = 0;
				filename[i] = (char)c;
			}
		}
		SDL_Delay(10);
	}
}

static int ConfirmQuit(void)
{
	int c;
	int i;
	for (i = 0; i < GraphicsGetScreenSize(&gGraphicsDevice.cachedConfig); i++)
	{
		gGraphicsDevice.buf[i] = LookupPalette(58);
	}
	CDogsTextStringAt(80, 50, "Campaign has been modified, but not saved");
	CDogsTextStringAt(110, 50 + TH, "Quit anyway? (Y/N)");
	BlitFlip(&gGraphicsDevice, &gConfig.Graphics);

	c = GetKey(&gInputDevices);
	return (c == 'Y' || c == 'y');
}

static void HelpScreen(void)
{
	Vec2i pos = Vec2iNew(20, 20);
	const char *helpText =
		"Help\n"
		"====\n"
		"Use mouse to select controls; keyboard to type text\n"
		"\n"
		"Common commands\n"
		"===============\n"
		"left/right click, page up/down: Increase/decrease value\n"
		"shift + left/right click:       Increase/decrease number of items\n"
		"insert:                         Add new item\n"
		"delete:                         Delete selected item\n"
		"\n"
		"Other commands\n"
		"==============\n"
		"Escape:                         Back or quit\n"
		"Ctrl+E:                         Go to character editor\n"
		"Ctrl+N:                         New mission or character\n"
		"Ctrl+S:                         Save file\n"
		"Ctrl+X, C, V:                   Cut/copy/paste\n"
		"Ctrl+M:                         Preview automap\n"
		"F1:                             This screen\n";
	const char *textP = helpText;
	int i;
	for (i = 0; i < GraphicsGetScreenSize(&gGraphicsDevice.cachedConfig); i++)
	{
		gGraphicsDevice.buf[i] = LookupPalette(58);
	}
	// Display help text line-by-line
	for (;;)
	{
		char buf[256];
		const char *eol = strchr(textP, '\n');
		if (eol == NULL)
		{
			break;
		}
		strncpy(buf, textP, eol - textP);
		buf[eol - textP] = '\0';
		DrawTextString(buf, &gGraphicsDevice, pos);
		pos.y += CDogsTextHeight();
		textP = eol + 1;
	}
	BlitFlip(&gGraphicsDevice, &gConfig.Graphics);
	GetKey(&gInputDevices);
}

static void Delete(int xc, int yc, int *mission)
{
	switch (yc)
	{
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
		{
			DeleteObjective(yc - YC_OBJECTIVES);
		}
		else
		{
			DeleteMission(mission);
		}
		AdjustYC(&yc);
		break;
	}
	fileChanged = 1;
	Setup(*mission, 0);
}

static void HandleInput(
	int c, int m,
	int *xc, int *yc, int *xcOld, int *ycOld,
	int *mission, struct Mission *scrap,
	int *done)
{
	int mouseTag;
	if (m && MouseTryGetRectTag(&gInputDevices.mouse, &mouseTag))
	{
		*xcOld = *xc;
		*ycOld = *yc;
		// Only change selection on left/right click
		if (m == SDL_BUTTON_LEFT || m == SDL_BUTTON_RIGHT)
		{
			if ((mouseTag & LEAVE_YC) == 0)
			{
				*yc = (mouseTag & 0xFF);
				AdjustYC(yc);
			}
			if ((mouseTag & LEAVE_XC) == 0)
			{
				*xc = ((mouseTag >> 8) & 0xFF);
				AdjustXC(*yc, xc);
			}
		}
		if (!(mouseTag & SELECT_ONLY) &&
			(!(mouseTag & SELECT_ONLY_FIRST) || (*xc == *xcOld && *yc == *ycOld)))
		{
			if (m == SDL_BUTTON_LEFT || m == SDL_BUTTON_WHEELUP)
			{
				c = SDLK_PAGEUP;
			}
			else if (m == SDL_BUTTON_RIGHT || m == SDL_BUTTON_WHEELDOWN)
			{
				c = SDLK_PAGEDOWN;
			}
		}
	}
	if (gInputDevices.keyboard.modState & (KMOD_ALT | KMOD_CTRL))
	{
		switch (c)
		{
		case 'x':
			*scrap = gCampaign.Setting.missions[*mission];
			Delete(*xc, *yc, mission);
			break;

		case 'c':
			*scrap = gCampaign.Setting.missions[*mission];
			break;

		case 'v':
			InsertMission(*mission, scrap);
			fileChanged = 1;
			Setup(*mission, 0);
			break;

		case 'q':
			if (!fileChanged || ConfirmQuit())
			{
				*done = 1;
			}
			break;

		case 'n':
			InsertMission(gCampaign.Setting.missionCount, NULL);
			*mission = gCampaign.Setting.missionCount - 1;
			fileChanged = 1;
			Setup(*mission, 0);
			break;

		case 's':
			Save(0);
			break;

		case 'h':
			Save(1);
			break;

		case 'm':
			Setup(*mission, 0);
			SetupMap();
			DisplayAutoMap(1);
			GetKey(&gInputDevices);
			KillAllObjects();
			FreeTriggersAndWatches();
			break;

		case 'e':
			EditCharacters(&gCampaign.Setting);
			Setup(*mission, 0);
			MouseSetRects(&gInputDevices.mouse, localClicks, NULL);
			break;
		}
	}
	else
	{
		switch (c)
		{
		case SDLK_F1:
			HelpScreen();
			break;

		case SDLK_HOME:
			if (*mission > 0)
			{
				(*mission)--;
			}
			Setup(*mission, 0);
			break;

		case SDLK_END:
			if (*mission < gCampaign.Setting.missionCount)
			{
				(*mission)++;
			}
			Setup(*mission, 0);
			break;

		case SDLK_INSERT:
			switch (*yc)
			{
			case YC_CHARACTERS:
				currentMission->baddieCount =
					CLAMP(currentMission->baddieCount + 1, 0, BADDIE_MAX);
				*xc = currentMission->baddieCount - 1;
				break;

			case YC_SPECIALS:
				currentMission->specialCount =
					CLAMP(currentMission->specialCount + 1, 0, SPECIAL_MAX);
				*xc = currentMission->specialCount - 1;
				break;

			case YC_ITEMS:
				currentMission->itemCount =
					CLAMP(currentMission->itemCount + 1, 0, ITEMS_MAX);
				*xc = currentMission->itemCount - 1;
				break;

			default:
				if (*yc >= YC_OBJECTIVES)
				{
					AddObjective();
				}
				else
				{
					InsertMission(*mission, NULL);
				}
				break;
			}
			fileChanged = 1;
			Setup(*mission, 0);
			break;

		case SDLK_DELETE:
			Delete(*xc, *yc, mission);
			break;

		case SDLK_UP:
			MoveSelection(0, yc, xc);
			break;

		case SDLK_DOWN:
			MoveSelection(1, yc, xc);
			break;

		case SDLK_TAB:
			MoveSelection(
				!(gInputDevices.keyboard.modState & KMOD_SHIFT), yc, xc);
			break;

		case SDLK_LEFT:
			(*xc)--;
			AdjustXC(*yc, xc);
			break;

		case SDLK_RIGHT:
			(*xc)++;
			AdjustXC(*yc, xc);
			break;

		case SDLK_PAGEUP:
			if (Change(*yc, *xc, 1, mission))
			{
				fileChanged = 1;
			}
			Setup(*mission, 0);
			break;

		case SDLK_PAGEDOWN:
			if (Change(*yc, *xc, -1, mission))
			{
				fileChanged = 1;
			}
			Setup(*mission, 0);
			break;

		case SDLK_ESCAPE:
			if (!fileChanged || ConfirmQuit())
			{
				*done = 1;
			}
			break;

		case SDLK_BACKSPACE:
			DelChar(*xc, *yc);
			fileChanged = 1;
			break;

		default:
			c = KeyGetTyped(&gInputDevices.keyboard);
			if (c)
			{
				fileChanged = 1;
				AddChar(*xc, *yc, (char)c);
			}
			break;
		}
	}
}

static void EditCampaign(void)
{
	int done = 0;
	int mission = 0;
	int xc = 0, yc = 0;
	int xcOld, ycOld;
	struct Mission scrap;
	struct EditorInfo edInfo;

	GetEditorInfo(&edInfo);

	memset(&scrap, 0, sizeof(scrap));
	MouseSetRects(&gInputDevices.mouse, localClicks, NULL);

	gCampaign.seed = 0;
	Setup(mission, 1);

	SDL_EnableKeyRepeat(0, 0);
	while (!done)
	{
		int c, m;
		InputPoll(&gInputDevices, SDL_GetTicks());
		c = KeyGetPressed(&gInputDevices.keyboard);
		m = MouseGetPressed(&gInputDevices.mouse);

		HandleInput(c, m, &xc, &yc, &xcOld, &ycOld, &mission, &scrap, &done);
		Display(mission, xc, yc, c);
		SDL_Delay(10);
	}
}


int main(int argc, char *argv[])
{
	int i;
	int loaded = 0;

	printf("C-Dogs Editor v0.8\n");
	printf("Copyright Ronny Wester 1996\n");

	debug(D_NORMAL, "Initialising SDL...\n");
	if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_AUDIO | SDL_INIT_VIDEO) != 0)
	{
		printf("Failed to start SDL!\n");
		return -1;
	}
	SDL_EnableUNICODE(SDL_ENABLE);

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
	gPalette[0].r = gPalette[0].g = gPalette[0].b = 0;
	memcpy(origPalette, gPalette, sizeof(origPalette));
	InitializeTranslationTables();
	BuildTranslationTables();
	CDogsTextInit(GetDataFilePath("graphics/font.px"), -2);

	ConfigLoadDefault(&gConfig);
	ConfigLoad(&gConfig, GetConfigFilePath(CONFIG_FILE));
	GraphicsInit(&gGraphicsDevice);
	GraphicsInitialize(&gGraphicsDevice, &gConfig.Graphics, 0);
	if (!gGraphicsDevice.IsInitialized)
	{
		printf("Video didn't init!\n");
		exit(EXIT_FAILURE);
	}

	CDogsSetPalette(gPalette);
	InputInit(&gInputDevices, gPics[145]);

	for (i = 1; i < argc; i++)
	{
		if (!loaded)
		{
			memset(lastFile, 0, sizeof(lastFile));
			strncpy(lastFile, argv[i], sizeof(lastFile) - 1);
			if (strchr(lastFile, '.') == NULL &&
				sizeof lastFile - strlen(lastFile) > 3)
			{
				strcat(lastFile, ".CPN");
			}
			if (LoadCampaign(lastFile, &gCampaign.Setting) == CAMPAIGN_OK)
			{
				loaded = 1;
			}
		}
	}

	if (!loaded)
	{
		// Reset campaign (graphics init may have created dummy campaigns)
		memset(&gCampaign.Setting, 0, sizeof gCampaign.Setting);
		gCampaign.Setting.missions = NULL;
		gCampaign.Setting.characters = NULL;
	}
	currentMission = NULL;

	EditCampaign();

	GraphicsTerminate(&gGraphicsDevice);
	exit(EXIT_SUCCESS);
}
