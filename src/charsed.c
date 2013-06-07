/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (C) 1995 Ronny Wester
    Copyright (C) 2003 Jeremy Chin
    Copyright (C) 2003-2007 Lucas Martin-King

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
*/
#include "charsed.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <SDL.h>

#include <cdogs/actors.h>
#include <cdogs/config.h>
#include <cdogs/defs.h>
#include <cdogs/events.h>
#include <cdogs/grafx.h>
#include <cdogs/keyboard.h>
#include <cdogs/mission.h>
#include <cdogs/pics.h>
#include <cdogs/text.h>
#include <cdogs/utils.h>


#define YC_APPEARANCE 0
#define YC_ATTRIBUTES 1
#define YC_FLAGS      2
#define YC_FLAGS2     3
#define YC_WEAPON     4

#define XC_FACE       0
#define XC_SKIN       1
#define XC_HAIR       2
#define XC_BODY       3
#define XC_ARMS       4
#define XC_LEGS       5

#define XC_SPEED      0
#define XC_HEALTH     1
#define XC_MOVE       2
#define XC_TRACK      3
#define XC_SHOOT      4
#define XC_DELAY      5

#define XC_ASBESTOS      0
#define XC_IMMUNITY      1
#define XC_SEETHROUGH    2
#define XC_RUNS_AWAY     3
#define XC_SNEAKY        4
#define XC_GOOD_GUY      5
#define XC_SLEEPING      6

#define XC_PRISONER      0
#define XC_INVULNERABLE  1
#define XC_FOLLOWER      2
#define XC_PENALTY       3
#define XC_VICTIM        4
#define XC_AWAKE         5


#define TH  9

// Mouse click areas:


int fileChanged = 0;
extern void *myScreen;


static struct MouseRect localClicks[] = {
	{30, 10, 55, 10 + TH - 1, YC_APPEARANCE + (XC_FACE << 8)},
	{60, 10, 85, 10 + TH - 1, YC_APPEARANCE + (XC_SKIN << 8)},
	{90, 10, 115, 10 + TH - 1, YC_APPEARANCE + (XC_HAIR << 8)},
	{120, 10, 145, 10 + TH - 1, YC_APPEARANCE + (XC_BODY << 8)},
	{150, 10, 175, 10 + TH - 1, YC_APPEARANCE + (XC_ARMS << 8)},
	{180, 10, 205, 10 + TH - 1, YC_APPEARANCE + (XC_LEGS << 8)},

	{20, 10 + TH, 60, 10 + 2 * TH - 1,
	 YC_ATTRIBUTES + (XC_SPEED << 8)},
	{70, 10 + TH, 110, 10 + 2 * TH - 1,
	 YC_ATTRIBUTES + (XC_HEALTH << 8)},
	{120, 10 + TH, 160, 10 + 2 * TH - 1,
	 YC_ATTRIBUTES + (XC_MOVE << 8)},
	{170, 10 + TH, 210, 10 + 2 * TH - 1,
	 YC_ATTRIBUTES + (XC_TRACK << 8)},
	{220, 10 + TH, 260, 10 + 2 * TH - 1,
	 YC_ATTRIBUTES + (XC_SHOOT << 8)},
	{270, 10 + TH, 310, 10 + 2 * TH - 1,
	 YC_ATTRIBUTES + (XC_DELAY << 8)},

	{5, 10 + 2 * TH, 45, 10 + 3 * TH - 1,
	 YC_FLAGS + (XC_ASBESTOS << 8)},
	{50, 10 + 2 * TH, 90, 10 + 3 * TH - 1,
	 YC_FLAGS + (XC_IMMUNITY << 8)},
	{95, 10 + 2 * TH, 135, 10 + 3 * TH - 1,
	 YC_FLAGS + (XC_SEETHROUGH << 8)},
	{140, 10 + 2 * TH, 180, 10 + 3 * TH - 1,
	 YC_FLAGS + (XC_RUNS_AWAY << 8)},
	{185, 10 + 2 * TH, 225, 10 + 3 * TH - 1,
	 YC_FLAGS + (XC_SNEAKY << 8)},
	{230, 10 + 2 * TH, 270, 10 + 3 * TH - 1,
	 YC_FLAGS + (XC_GOOD_GUY << 8)},
	{275, 10 + 2 * TH, 315, 10 + 3 * TH - 1,
	 YC_FLAGS + (XC_SLEEPING << 8)},

	{5, 10 + 3 * TH, 45, 10 + 4 * TH - 1,
	 YC_FLAGS2 + (XC_PRISONER << 8)},
	{50, 10 + 3 * TH, 90, 10 + 4 * TH - 1,
	 YC_FLAGS2 + (XC_INVULNERABLE << 8)},
	{95, 10 + 3 * TH, 135, 10 + 4 * TH - 1,
	 YC_FLAGS2 + (XC_FOLLOWER << 8)},
	{140, 10 + 3 * TH, 180, 10 + 4 * TH - 1,
	 YC_FLAGS2 + (XC_PENALTY << 8)},
	{185, 10 + 3 * TH, 225, 10 + 4 * TH - 1,
	 YC_FLAGS2 + (XC_VICTIM << 8)},
	{230, 10 + 3 * TH, 270, 10 + 4 * TH - 1,
	 YC_FLAGS2 + (XC_AWAKE << 8)},

	{50, 10 + 4 * TH, 260, 10 + 5 * TH - 1, YC_WEAPON},

	{0, 0, 0, 0, 0}
};


static int XYToCharacterIndex(int x, int y, int *index)
{
	if (y < 10 + 5 * TH + 5)
		return 0;

	y -= 10 + 5 * TH + 5;

	x /= 20;
	y /= 30;
	*index = 16 * y + x;
	return 1;
}

static void DisplayCharacter(int x, int y, const TBadGuy * data,
			     int hilite)
{
	struct CharacterDescription *cd;
	TOffsetPic body, head;

	cd = &characterDesc[0];
	SetupMissionCharacter(0, data);

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
		CDogsTextGoto(x - 8, y - 16);
		CDogsTextChar('\020');
	}
}

static void DisplayCDogsText(int x, int y, const char *text, int hilite)
{
	if (hilite)
		CDogsTextStringWithTableAt(x, y, text, &tableFlamed);
	else
		CDogsTextStringAt(x, y, text);
}

void DisplayFlag(int x, int y, const char *s, int on, int hilite)
{
	CDogsTextGoto(x, y);
	if (hilite) {
		CDogsTextStringWithTable(s, &tableFlamed);
		CDogsTextCharWithTable(':', &tableFlamed);
	} else {
		CDogsTextString(s);
		CDogsTextChar(':');
	}
	if (on)
		CDogsTextStringWithTable("On", &tablePurple);
	else
		CDogsTextString("Off");
}

static void Display(CampaignSetting *setting, int index, int xc, int yc)
{
	int x, y = 10;
	char s[50];
	const TBadGuy *b;
	int i;

	memset(GetDstScreen(), 74, GraphicsGetMemSize(&gGraphicsDevice.cachedConfig));

	sprintf(s, "%d/%d", setting->characterCount, MAX_CHARACTERS);
	CDogsTextStringAt(10, 190, s);

	if (index >= 0 && index < setting->characterCount) {
		b = &setting->characters[index];
		DisplayCDogsText(30, y, "Face", yc == YC_APPEARANCE && xc == XC_FACE);
		DisplayCDogsText(60, y, "Skin", yc == YC_APPEARANCE && xc == XC_SKIN);
		DisplayCDogsText(90, y, "Hair", yc == YC_APPEARANCE && xc == XC_HAIR);
		DisplayCDogsText(120, y, "Body", yc == YC_APPEARANCE && xc == XC_BODY);
		DisplayCDogsText(150, y, "Arms", yc == YC_APPEARANCE && xc == XC_ARMS);
		DisplayCDogsText(180, y, "Legs", yc == YC_APPEARANCE && xc == XC_LEGS);
		y += CDogsTextHeight();

		sprintf(s, "Speed: %d%%", (100 * b->speed) / 256);
		DisplayCDogsText(20, y, s, yc == YC_ATTRIBUTES && xc == XC_SPEED);
		sprintf(s, "Hp: %d", b->health);
		DisplayCDogsText(70, y, s, yc == YC_ATTRIBUTES && xc == XC_HEALTH);
		sprintf(s, "Move: %d%%", b->probabilityToMove);
		DisplayCDogsText(120, y, s, yc == YC_ATTRIBUTES && xc == XC_MOVE);
		sprintf(s, "Track: %d%%", b->probabilityToTrack);
		DisplayCDogsText(170, y, s, yc == YC_ATTRIBUTES && xc == XC_TRACK);
		sprintf(s, "Shoot: %d%%", b->probabilityToShoot);
		DisplayCDogsText(220, y, s, yc == YC_ATTRIBUTES && xc == XC_SHOOT);
		sprintf(s, "Delay: %d", b->actionDelay);
		DisplayCDogsText(270, y, s, yc == YC_ATTRIBUTES && xc == XC_DELAY);
		y += CDogsTextHeight();

		DisplayFlag(5, y, "Asbestos",
			    (b->flags & FLAGS_ASBESTOS) != 0,
			    yc == YC_FLAGS && xc == XC_ASBESTOS);
		DisplayFlag(50, y, "Immunity",
			    (b->flags & FLAGS_IMMUNITY) != 0,
			    yc == YC_FLAGS && xc == XC_IMMUNITY);
		DisplayFlag(95, y, "C-thru",
			    (b->flags & FLAGS_SEETHROUGH) != 0,
			    yc == YC_FLAGS && xc == XC_SEETHROUGH);
		DisplayFlag(140, y, "Run-away",
			    (b->flags & FLAGS_RUNS_AWAY) != 0,
			    yc == YC_FLAGS && xc == XC_RUNS_AWAY);
		DisplayFlag(185, y, "Sneaky",
			    (b->flags & FLAGS_SNEAKY) != 0, yc == YC_FLAGS
			    && xc == XC_SNEAKY);
		DisplayFlag(230, y, "Good guy",
			    (b->flags & FLAGS_GOOD_GUY) != 0,
			    yc == YC_FLAGS && xc == XC_GOOD_GUY);
		DisplayFlag(275, y, "Asleep",
			    (b->flags & FLAGS_SLEEPALWAYS) != 0,
			    yc == YC_FLAGS && xc == XC_SLEEPING);
		y += CDogsTextHeight();

		DisplayFlag(5, y, "Prisoner",
			    (b->flags & FLAGS_PRISONER) != 0,
			    yc == YC_FLAGS2 && xc == XC_PRISONER);
		DisplayFlag(50, y, "Invuln.",
			    (b->flags & FLAGS_INVULNERABLE) != 0,
			    yc == YC_FLAGS2 && xc == XC_INVULNERABLE);
		DisplayFlag(95, y, "Follower",
			    (b->flags & FLAGS_FOLLOWER) != 0,
			    yc == YC_FLAGS2 && xc == XC_FOLLOWER);
		DisplayFlag(140, y, "Penalty",
			    (b->flags & FLAGS_PENALTY) != 0,
			    yc == YC_FLAGS2 && xc == XC_PENALTY);
		DisplayFlag(185, y, "Victim",
			    (b->flags & FLAGS_VICTIM) != 0, yc == YC_FLAGS2
			    && xc == XC_VICTIM);
		DisplayFlag(230, y, "Awake",
			    (b->flags & FLAGS_AWAKEALWAYS) != 0,
			    yc == YC_FLAGS2 && xc == XC_AWAKE);
		y += CDogsTextHeight();

		DisplayCDogsText(50, y, GunGetName(b->gun), yc == YC_WEAPON);
		y += CDogsTextHeight() + 5;

		x = 10;
		for (i = 0; i < setting->characterCount; i++) {
			DisplayCharacter(x, y + 20,
					 &setting->characters[i],
					 index == i);
			x += 20;
			if (x > gGraphicsDevice.cachedConfig.ResolutionWidth)
			{
				x = 10;
				y += 30;
			}
		}
	}

	CopyToScreen();
}

static void Change(
	CampaignSetting *setting,
	int index,
	int yc, int xc,
	int d)
{
	TBadGuy *b;

	if (index < 0 || index >= setting->characterCount)
		return;

	b = &setting->characters[index];
	switch (yc) {
	case YC_APPEARANCE:
		switch (xc) {
		case XC_FACE:
			b->facePic = CLAMP_OPPOSITE(b->facePic + d, 0, FACE_COUNT - 1);
			break;

		case XC_SKIN:
			b->skinColor = CLAMP_OPPOSITE(b->skinColor + d, 0, SHADE_COUNT - 1);
			break;

		case XC_HAIR:
			b->hairColor = CLAMP_OPPOSITE(b->hairColor + d, 0, SHADE_COUNT - 1);
			break;

		case XC_BODY:
			b->bodyColor = CLAMP_OPPOSITE(b->bodyColor + d, 0, SHADE_COUNT - 1);
			break;

		case XC_ARMS:
			b->armColor = CLAMP_OPPOSITE(b->armColor + d, 0, SHADE_COUNT - 1);
			break;

		case XC_LEGS:
			b->legColor = CLAMP_OPPOSITE(b->legColor + d, 0, SHADE_COUNT - 1);
			break;
		}
		break;

	case YC_ATTRIBUTES:
		switch (xc) {
		case XC_SPEED:
			b->speed = CLAMP(b->speed + d * 64, 128, 512);
			break;

		case XC_HEALTH:
			b->health = CLAMP(b->health + d * 10, 10, 500);
			break;

		case XC_MOVE:
			b->probabilityToMove = CLAMP(b->probabilityToMove + d * 5, 0, 100);
			break;

		case XC_TRACK:
			b->probabilityToTrack = CLAMP(b->probabilityToTrack + d * 5, 0, 100);
			break;

		case XC_SHOOT:
			b->probabilityToShoot = CLAMP(b->probabilityToShoot + d * 5, 0, 100);
			break;

		case XC_DELAY:
			b->actionDelay = CLAMP(b->actionDelay + d, 0, 50);
			break;
		}
		break;

	case YC_FLAGS:
		switch (xc) {
		case XC_ASBESTOS:
			b->flags ^= FLAGS_ASBESTOS;
			break;

		case XC_IMMUNITY:
			b->flags ^= FLAGS_IMMUNITY;
			break;

		case XC_SEETHROUGH:
			b->flags ^= FLAGS_SEETHROUGH;
			break;

		case XC_RUNS_AWAY:
			b->flags ^= FLAGS_RUNS_AWAY;
			break;

		case XC_SNEAKY:
			b->flags ^= FLAGS_SNEAKY;
			break;

		case XC_GOOD_GUY:
			b->flags ^= FLAGS_GOOD_GUY;
			break;

		case XC_SLEEPING:
			b->flags ^= FLAGS_SLEEPALWAYS;
			break;
		}
		break;

	case YC_FLAGS2:
		switch (xc) {
		case XC_PRISONER:
			b->flags ^= FLAGS_PRISONER;
			break;

		case XC_INVULNERABLE:
			b->flags ^= FLAGS_INVULNERABLE;
			break;

		case XC_FOLLOWER:
			b->flags ^= FLAGS_FOLLOWER;
			break;

		case XC_PENALTY:
			b->flags ^= FLAGS_PENALTY;
			break;

		case XC_VICTIM:
			b->flags ^= FLAGS_VICTIM;
			break;

		case XC_AWAKE:
			b->flags ^= FLAGS_AWAKEALWAYS;
			break;
		}
		break;

	case YC_WEAPON:
		b->gun = CLAMP_OPPOSITE((int)(b->gun + d), 0, GUN_COUNT - 1);
		break;
	}
}


static TBadGuy characterTemplate = {
	BODY_ARMED, BODY_UNARMED, FACE_OGRE, 256, 50, 25, 2, 15, GUN_MG,
	SHADE_GREEN, SHADE_DKGRAY, SHADE_DKGRAY, SHADE_DKGRAY, SHADE_BLACK,
	40, FLAGS_IMMUNITY
};

static void InsertCharacter(
	CampaignSetting *setting, int index,
	TBadGuy *data)
{
	int i;

	if (setting->characterCount == MAX_CHARACTERS)
		return;

	for (i = setting->characterCount; i > index; i--)
		setting->characters[i] = setting->characters[i - 1];
	if (data)
		setting->characters[index] = *data;
	else
		setting->characters[index] = characterTemplate;
	setting->characterCount++;
}

static void DeleteCharacter(CampaignSetting *setting, int *index)
{
	int i;

	setting->characterCount = CLAMP(setting->characterCount - 1, 0, 1000);
	for (i = *index; i < setting->characterCount; i++)
	{
		setting->characters[i] = setting->characters[i + 1];
	}
	if (*index > 0 && *index >= setting->characterCount - 1)
	{
		(*index)--;
	}
}

static void AdjustYC(int *yc)
{
	*yc = CLAMP_OPPOSITE(*yc, 0, YC_WEAPON);
}

static void AdjustXC(int yc, int *xc)
{
	switch (yc) {
	case YC_APPEARANCE:
		*xc = CLAMP_OPPOSITE(*xc, 0, XC_LEGS);
		break;

	case YC_ATTRIBUTES:
		*xc = CLAMP_OPPOSITE(*xc, 0, XC_DELAY);
		break;

	case YC_FLAGS:
		*xc = CLAMP_OPPOSITE(*xc, 0, XC_SLEEPING);
		break;

	case YC_FLAGS2:
		*xc = CLAMP_OPPOSITE(*xc, 0, XC_AWAKE);
		break;
	}
}

void DrawCursor(int x, int y)
{
	DrawTPic(x, y, gPics[145], NULL);
}

void RestoreBkg(int x, int y, unsigned int *bkg)
{
	int w = ((short *) (gPics[145]))[0];
	int h = ((short *) (gPics[145]))[1];
	int xMax = x + w - 1;
	int yMax = y + h - 1;
	int i;
	int offset;
	unsigned int *dstBase = (unsigned int *) 0xA0000;

	if (xMax > 319)
		xMax = 319;
	if (yMax > 199)
		yMax = 199;

	x = x / 4;
	w = (xMax / 4) - x + 1;

	while (y <= yMax) {
		offset = y * 80 + x;
		i = w;
		while (i--) {
			*(dstBase + offset) = *(bkg + offset);
			offset++;
		}
		y++;
	}
}

void GetEvent(int *key, int *x, int *y, int *buttons)
{
	static int scaling = 1;
	static int wasDown = 0;
	static int isRepeating = 0;

	int xPrev = -1, yPrev = -1;
	//void *old = GetDstScreen();

	Mouse(x, y, buttons);
	if (*buttons != 0 && wasDown)
		SDL_EnableKeyRepeat(isRepeating ? 100 : 500, 250);

	//SetDstScreen(new);
	*buttons = *key = 0;
	do
	{
		if (KeyGetPressed(&gKeyboard))
		{
			*key = GetKey(&gKeyboard);
		}

		Mouse(x, y, buttons);
		*x /= scaling;
		if (*x > 319)
			scaling++;

		if (!(*buttons) && !(*key) && (*x != xPrev || *y != yPrev))
		{
			DrawCursor(*x, *y);
			xPrev = *x;
			yPrev = *y;
		}
	}
	while (!(*buttons) && !(*key));

	isRepeating = wasDown && (buttons != 0);
	wasDown = (*buttons != 0);
	//SetDstScreen(old);
}

void EditCharacters(CampaignSetting *setting)
{
	int done = 0;
	int c = 0;
	int index = 0;
	int xc = 0, yc = 0;
	TBadGuy scrap;
	int x, y, buttons, tag;

	memset(&scrap, 0, sizeof(scrap));
	SetMouseRects(localClicks);

	while (!done) {
		Display(setting, index, xc, yc);

		do {
			GetEvent(&c, &x, &y, &buttons);
			if (buttons) {
				if (XYToCharacterIndex(x, y, &tag)) {
					if (tag >= 0
					    && tag <
					    setting->characterCount)
						index = tag;
					c = DUMMY;
				} else if (GetMouseRectTag(x, y, &tag)) {
					xc = (tag >> 8);
					yc = (tag & 0xFF);
					AdjustYC(&yc);
					AdjustXC(yc, &xc);
					c = (buttons ==
					     1 ? PAGEUP : PAGEDOWN);
				}
			}
		}
		while (!c);

		switch (c) {
		case HOME:
			if (index > 0)
				index--;
			break;

		case END:
			if (index < setting->characterCount - 1)
				index++;
			break;

		case INSERT:
			InsertCharacter(setting, index, NULL);
			fileChanged = 1;
			break;

		case ALT_X:
			scrap = setting->characters[index];

		case DELETE:
			DeleteCharacter(setting, &index);
			fileChanged = 1;
			break;

		case ALT_C:
			scrap = setting->characters[index];
			break;

		case ALT_V:
			InsertCharacter(setting, index, &scrap);
			fileChanged = 1;
			break;

		case ALT_N:
			InsertCharacter(setting, setting->characterCount,
					NULL);
			index = setting->characterCount - 1;
			fileChanged = 1;
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
			Change(setting, index, yc, xc, 1);
			fileChanged = 1;
			break;

		case PAGEDOWN:
			Change(setting, index, yc, xc, -1);
			fileChanged = 1;
			break;

		case ESCAPE:
			done = 1;
			break;
		}
	}
}
