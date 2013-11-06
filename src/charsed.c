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
#include "charsed.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <SDL.h>

#include <cdogs/actors.h>
#include <cdogs/config.h>
#include <cdogs/defs.h>
#include <cdogs/draw.h>
#include <cdogs/drawtools.h>
#include <cdogs/grafx.h>
#include <cdogs/keyboard.h>
#include <cdogs/mission.h>
#include <cdogs/palette.h>
#include <cdogs/pic_manager.h>
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


static MouseRect localClicks[] =
{
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


static int PosToCharacterIndex(Vec2i pos, int *idx)
{
	if (pos.y < 10 + 5 * TH + 5)
	{
		return 0;
	}

	pos.y -= 10 + 5 * TH + 5;

	pos.x /= 20;
	pos.y /= 30;
	*idx = 16 * pos.y + pos.x;
	return 1;
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

static void DrawTooltips(
	GraphicsDevice *device, Vec2i pos, int yc, int xc, int mouseYc, int mouseXc)
{
	UNUSED(yc);
	UNUSED(xc);
	switch (mouseYc)
	{
		case YC_ATTRIBUTES:
			switch (mouseXc)
			{
				case XC_TRACK:
					DrawTooltip(device, pos,
						"Looking towards the player\n"
						"Useless for friendly characters");
					break;
				case XC_DELAY:
					DrawTooltip(device, pos,
						"Frames before making another decision");
					break;
			}
			break;
		case YC_FLAGS:
			switch (mouseXc)
			{
				case XC_IMMUNITY:
					DrawTooltip(device, pos, "Immune to poison");
					break;
				case XC_RUNS_AWAY:
					DrawTooltip(device, pos, "Runs away from player");
					break;
				case XC_SNEAKY:
					DrawTooltip(device, pos, "Shoots back when player shoots");
					break;
				case XC_SLEEPING:
					DrawTooltip(device, pos, "Doesn't move unless seen");
					break;
			}
			break;
		case YC_FLAGS2:
			switch (mouseXc)
			{
				case XC_PRISONER:
					DrawTooltip(device, pos, "Doesn't move until touched");
					break;
				case XC_PENALTY:
					DrawTooltip(device, pos, "Large score penalty when shot");
					break;
				case XC_VICTIM:
					DrawTooltip(device, pos, "Takes damage from everyone");
					break;
			}
		default:
			break;
	}
}

static void Display(CampaignSettingNew *setting, int idx, int xc, int yc)
{
	int x, y = 10;
	char s[50];
	const Character *b;
	int i;
	int tag;

	for (i = 0; i < GraphicsGetScreenSize(&gGraphicsDevice.cachedConfig); i++)
	{
		gGraphicsDevice.buf[i] = LookupPalette(74);
	}

	sprintf(s, "%d", setting->characters.otherCount);
	CDogsTextStringAt(10, 190, s);

	if (idx >= 0 && idx < setting->characters.otherCount)
	{
		b = &setting->characters.others[idx];
		DisplayCDogsText(30, y, "Face", yc == YC_APPEARANCE && xc == XC_FACE);
		DisplayCDogsText(60, y, "Skin", yc == YC_APPEARANCE && xc == XC_SKIN);
		DisplayCDogsText(90, y, "Hair", yc == YC_APPEARANCE && xc == XC_HAIR);
		DisplayCDogsText(120, y, "Body", yc == YC_APPEARANCE && xc == XC_BODY);
		DisplayCDogsText(150, y, "Arms", yc == YC_APPEARANCE && xc == XC_ARMS);
		DisplayCDogsText(180, y, "Legs", yc == YC_APPEARANCE && xc == XC_LEGS);
		y += CDogsTextHeight();

		sprintf(s, "Speed: %d%%", (100 * b->speed) / 256);
		DisplayCDogsText(20, y, s, yc == YC_ATTRIBUTES && xc == XC_SPEED);
		sprintf(s, "Hp: %d", b->maxHealth);
		DisplayCDogsText(70, y, s, yc == YC_ATTRIBUTES && xc == XC_HEALTH);
		sprintf(s, "Move: %d%%", b->bot.probabilityToMove);
		DisplayCDogsText(120, y, s, yc == YC_ATTRIBUTES && xc == XC_MOVE);
		sprintf(s, "Track: %d%%", b->bot.probabilityToTrack);
		DisplayCDogsText(170, y, s, yc == YC_ATTRIBUTES && xc == XC_TRACK);
		sprintf(s, "Shoot: %d%%", b->bot.probabilityToShoot);
		DisplayCDogsText(220, y, s, yc == YC_ATTRIBUTES && xc == XC_SHOOT);
		sprintf(s, "Delay: %d", b->bot.actionDelay);
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
		for (i = 0; i < setting->characters.otherCount; i++)
		{
			DisplayCharacter(x, y + 20, i, idx == i, 0);
			x += 20;
			if (x > gGraphicsDevice.cachedConfig.ResolutionWidth)
			{
				x = 10;
				y += 30;
			}
		}
	}

	if (MouseTryGetRectTag(&gInputDevices.mouse, &tag))
	{
		int mouseYc = tag & 0xFF;
		int mouseXc = (tag & 0xFF00) >> 8;
		Vec2i tooltipPos = Vec2iAdd(
			gInputDevices.mouse.currentPos, Vec2iNew(10, 10));
		DrawTooltips(&gGraphicsDevice, tooltipPos, yc, xc, mouseYc, mouseXc);
	}
	MouseDraw(&gInputDevices.mouse);

	BlitFlip(&gGraphicsDevice, &gConfig.Graphics);
}

static void Change(
	CharacterStore *store,
	int idx,
	int yc, int xc,
	int d)
{
	Character *b;

	if (idx < 0 || idx >= store->otherCount)
	{
		return;
	}

	b = &store->others[idx];
	switch (yc)
	{
	case YC_APPEARANCE:
		switch (xc) {
		case XC_FACE:
			b->looks.face = CLAMP_OPPOSITE(b->looks.face + d, 0, FACE_COUNT - 1);
			break;

		case XC_SKIN:
			b->looks.skin = CLAMP_OPPOSITE(b->looks.skin + d, 0, SHADE_COUNT - 1);
			break;

		case XC_HAIR:
			b->looks.hair = CLAMP_OPPOSITE(b->looks.hair + d, 0, SHADE_COUNT - 1);
			break;

		case XC_BODY:
			b->looks.body = CLAMP_OPPOSITE(b->looks.body + d, 0, SHADE_COUNT - 1);
			break;

		case XC_ARMS:
			b->looks.arm = CLAMP_OPPOSITE(b->looks.arm + d, 0, SHADE_COUNT - 1);
			break;

		case XC_LEGS:
			b->looks.leg = CLAMP_OPPOSITE(b->looks.leg + d, 0, SHADE_COUNT - 1);
			break;
		}
		break;

	case YC_ATTRIBUTES:
		switch (xc) {
		case XC_SPEED:
			b->speed = CLAMP(b->speed + d * 64, 128, 512);
			break;

		case XC_HEALTH:
			b->maxHealth = CLAMP(b->maxHealth + d * 10, 10, 500);
			break;

		case XC_MOVE:
			b->bot.probabilityToMove =
				CLAMP(b->bot.probabilityToMove + d * 5, 0, 100);
			break;

		case XC_TRACK:
			b->bot.probabilityToTrack =
				CLAMP(b->bot.probabilityToTrack + d * 5, 0, 100);
			break;

		case XC_SHOOT:
			b->bot.probabilityToShoot =
				CLAMP(b->bot.probabilityToShoot + d * 5, 0, 100);
			break;

		case XC_DELAY:
			b->bot.actionDelay = CLAMP(b->bot.actionDelay + d, 0, 50);
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
		b->gun = (gun_e)CLAMP_OPPOSITE((int)b->gun + d, 0, GUN_COUNT - 1);
		break;
	}
}


static void InsertCharacter(CharacterStore *store, int idx, Character *data)
{
	Character *c = CharacterStoreInsertOther(store, idx);
	if (data)
	{
		memcpy(&c, data, sizeof *c);
	}
	else
	{
		// set up character template
		c->looks.armedBody = BODY_ARMED;
		c->looks.unarmedBody = BODY_UNARMED;
		c->looks.face = FACE_OGRE;
		c->looks.skin = SHADE_GREEN;
		c->looks.arm = SHADE_DKGRAY;
		c->looks.body = SHADE_DKGRAY;
		c->looks.leg = SHADE_DKGRAY;
		c->looks.hair = SHADE_BLACK;
		c->speed = 256;
		c->gun = GUN_MG;
		c->maxHealth = 40;
		c->flags = FLAGS_IMMUNITY;
		memset(c->table, 0, sizeof c->table);
		c->bot.probabilityToMove = 50;
		c->bot.probabilityToTrack = 25;
		c->bot.probabilityToShoot = 2;
		c->bot.actionDelay = 15;
		CharacterSetLooks(c, &c->looks);
	}
}

static void DeleteCharacter(CharacterStore *store, int *idx)
{
	CharacterStoreDeleteOther(store, *idx);
	if (*idx > 0 && *idx >= store->otherCount - 1)
	{
		(*idx)--;
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

void RestoreBkg(int x, int y, unsigned int *bkg)
{
	PicPaletted *bgPic = PicManagerGetOldPic(&gPicManager, 145);
	int w = bgPic->w;
	int h = bgPic->h;
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

static void HandleInput(
	int c, int *xc, int *yc,
	int *idx, CharacterStore *store, Character *scrap, int *done)
{
	if (gInputDevices.keyboard.modState & (KMOD_ALT | KMOD_CTRL))
	{
		switch (c)
		{
		case 'x':
			*scrap = store->others[*idx];
			DeleteCharacter(store, idx);
			fileChanged = 1;
			break;

		case 'c':
			*scrap = store->others[*idx];
			break;

		case 'v':
			InsertCharacter(store, *idx, scrap);
			fileChanged = 1;
			break;

		case 'n':
			InsertCharacter(store, store->otherCount, NULL);
			*idx = store->otherCount - 1;
			fileChanged = 1;
			break;
		}
	}
	else
	{
		switch (c)
		{
		case SDLK_HOME:
			if (*idx > 0)
			{
				(*idx)--;
			}
			break;

		case SDLK_END:
			if (*idx < store->otherCount - 1)
			{
				(*idx)++;
			}
			break;

		case SDLK_INSERT:
			InsertCharacter(store, *idx, NULL);
			fileChanged = 1;
			break;

		case SDLK_DELETE:
			DeleteCharacter(store, idx);
			fileChanged = 1;
			break;

		case SDLK_UP:
			(*yc)--;
			AdjustYC(yc);
			AdjustXC(*yc, xc);
			break;

		case SDLK_DOWN:
			(*yc)++;
			AdjustYC(yc);
			AdjustXC(*yc, xc);
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
			Change(store, *idx, *yc, *xc, 1);
			fileChanged = 1;
			break;

		case SDLK_PAGEDOWN:
			Change(store, *idx, *yc, *xc, -1);
			fileChanged = 1;
			break;

		case SDLK_ESCAPE:
			*done = 1;
			break;
		}
	}
}

void EditCharacters(CampaignSettingNew *setting)
{
	int done = 0;
	int idx = 0;
	int xc = 0, yc = 0;
	int xcOld, ycOld;
	Character scrap;

	memset(&scrap, 0, sizeof(scrap));
	MouseSetRects(&gInputDevices.mouse, localClicks, NULL);

	while (!done)
	{
		int tag;
		int c, m;
		InputPoll(&gInputDevices, SDL_GetTicks());
		c = KeyGetPressed(&gInputDevices.keyboard);
		m = MouseGetPressed(&gInputDevices.mouse);
		if (m)
		{
			xcOld = xc;
			ycOld = yc;
			// Only change selection on left/right click
			if ((m == SDL_BUTTON_LEFT || m == SDL_BUTTON_RIGHT) &&
				PosToCharacterIndex(gInputDevices.mouse.currentPos, &tag))
			{
				if (tag >= 0 && tag < setting->characters.otherCount)
				{
					idx = tag;
				}
			}
			else if (MouseTryGetRectTag(&gInputDevices.mouse, &tag))
			{
				int isSameSelection;
				xcOld = xc;
				ycOld = yc;
				if (m == SDL_BUTTON_LEFT || m == SDL_BUTTON_RIGHT)
				{
					xc = (tag >> 8);
					yc = (tag & 0xFF);
					AdjustYC(&yc);
					AdjustXC(yc, &xc);
				}
				isSameSelection = xc == xcOld && yc == ycOld;
				if (m == SDL_BUTTON_LEFT ||
					(m == SDL_BUTTON_WHEELUP && isSameSelection))
				{
					c = SDLK_PAGEUP;
				}
				else if (m == SDL_BUTTON_RIGHT ||
					(m == SDL_BUTTON_WHEELDOWN && isSameSelection))
				{
					c = SDLK_PAGEDOWN;
				}
			}
		}

		HandleInput(c, &xc, &yc, &idx, &setting->characters, &scrap, &done);
		Display(setting, idx, xc, yc);
		SDL_Delay(10);
	}
}

void DrawTooltip(GraphicsDevice *device, Vec2i pos, const char *s)
{
	Vec2i bgSize = TextGetSize(s);
	color_t bgColor = { 64, 64, 64, 196 };
	DrawRectangle(device, pos, bgSize, bgColor, 0);
	DrawTextString(s, device, pos);
}
