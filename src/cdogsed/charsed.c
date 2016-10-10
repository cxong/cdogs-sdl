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

    Copyright (c) 2013-2014, 2016 Cong Xu
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
#include <cdogs/draw/draw.h>
#include <cdogs/draw/draw_actor.h>
#include <cdogs/draw/drawtools.h>
#include <cdogs/events.h>
#include <cdogs/font.h>
#include <cdogs/grafx.h>
#include <cdogs/keyboard.h>
#include <cdogs/mission.h>
#include <cdogs/palette.h>
#include <cdogs/pic_manager.h>
#include <cdogs/utils.h>

#include "editor_ui_chared.h"
#include "editor_ui_common.h"
#include "ui_object.h"


int fileChanged = 0;
extern void *myScreen;


static UIObject *sCharEditorObjs;
static CArray sDrawObjs;	// of UIObjectDrawContext, used to cache BFS order
static UIObject *sLastHighlightedObj = NULL;


static int PosToCharacterIndex(Vec2i pos, int *idx)
{
	if (pos.y < 10 + 5 * FontH() + 5)
	{
		return 0;
	}

	pos.y -= 10 + 5 * FontH() + 5;

	pos.x /= 20;
	pos.y /= 30;
	*idx = (10 + gGraphicsDevice.cachedConfig.Res.x) / 20 * pos.y + pos.x;
	return 1;
}

static void DisplayCDogsText(int x, int y, const char *text, int hilite)
{
	color_t mask = hilite ? colorRed : colorWhite;
	FontStrMask(text, Vec2iNew(x, y), mask);
}

static void Display(CampaignSetting *setting, int idx, int xc, int yc)
{
	char s[50];
	int i;
	UIObject *o;

	for (i = 0; i < GraphicsGetScreenSize(&gGraphicsDevice.cachedConfig); i++)
	{
		gGraphicsDevice.buf[i] = LookupPalette(74);
	}

	sprintf(s, "%d", (int)setting->characters.OtherChars.size);
	FontStr(s, Vec2iNew(10, 190));

	if (idx >= 0 && idx < (int)setting->characters.OtherChars.size)
	{
		int x = 40;
		int y = 10;
		DisplayCDogsText(x, y, "Face", yc == YC_APPEARANCE && xc == XC_FACE);
		y += FontH();

		const Character *b = CArrayGet(&setting->characters.OtherChars, idx);
		sprintf(s, "Speed: %d%%", (100 * b->speed) / 256);
		DisplayCDogsText(20, y, s, yc == YC_ATTRIBUTES && xc == XC_SPEED);
		sprintf(s, "Hp: %d", b->maxHealth);
		DisplayCDogsText(70, y, s, yc == YC_ATTRIBUTES && xc == XC_HEALTH);
		sprintf(s, "Move: %d%%", b->bot->probabilityToMove);
		DisplayCDogsText(120, y, s, yc == YC_ATTRIBUTES && xc == XC_MOVE);
		sprintf(s, "Track: %d%%", b->bot->probabilityToTrack);
		DisplayCDogsText(170, y, s, yc == YC_ATTRIBUTES && xc == XC_TRACK);
		sprintf(s, "Shoot: %d%%", b->bot->probabilityToShoot);
		DisplayCDogsText(220, y, s, yc == YC_ATTRIBUTES && xc == XC_SHOOT);
		sprintf(s, "Delay: %d", b->bot->actionDelay);
		DisplayCDogsText(270, y, s, yc == YC_ATTRIBUTES && xc == XC_DELAY);
		y += FontH();

		DisplayFlag(
			Vec2iNew(5, y), "Asbestos", (b->flags & FLAGS_ASBESTOS) != 0,
			yc == YC_FLAGS && xc == XC_ASBESTOS);
		DisplayFlag(
			Vec2iNew(50, y), "Immunity", (b->flags & FLAGS_IMMUNITY) != 0,
			yc == YC_FLAGS && xc == XC_IMMUNITY);
		DisplayFlag(
			Vec2iNew(95, y), "C-thru", (b->flags & FLAGS_SEETHROUGH) != 0,
			yc == YC_FLAGS && xc == XC_SEETHROUGH);
		DisplayFlag(
			Vec2iNew(140, y), "Run-away", (b->flags & FLAGS_RUNS_AWAY) != 0,
			yc == YC_FLAGS && xc == XC_RUNS_AWAY);
		DisplayFlag(
			Vec2iNew(185, y), "Sneaky", (b->flags & FLAGS_SNEAKY) != 0,
			yc == YC_FLAGS && xc == XC_SNEAKY);
		DisplayFlag(
			Vec2iNew(230, y), "Good guy", (b->flags & FLAGS_GOOD_GUY) != 0,
			yc == YC_FLAGS && xc == XC_GOOD_GUY);
		DisplayFlag(
			Vec2iNew(275, y), "Asleep", (b->flags & FLAGS_SLEEPALWAYS) != 0,
			yc == YC_FLAGS && xc == XC_SLEEPING);
		y += FontH();

		DisplayFlag(
			Vec2iNew(5, y), "Prisoner", (b->flags & FLAGS_PRISONER) != 0,
			yc == YC_FLAGS && xc == XC_PRISONER);
		DisplayFlag(
			Vec2iNew(50, y), "Invuln.", (b->flags & FLAGS_INVULNERABLE) != 0,
			yc == YC_FLAGS && xc == XC_INVULNERABLE);
		DisplayFlag(
			Vec2iNew(95, y), "Follower", (b->flags & FLAGS_FOLLOWER) != 0,
			yc == YC_FLAGS && xc == XC_FOLLOWER);
		DisplayFlag(
			Vec2iNew(140, y), "Penalty", (b->flags & FLAGS_PENALTY) != 0,
			yc == YC_FLAGS && xc == XC_PENALTY);
		DisplayFlag(
			Vec2iNew(185, y), "Victim", (b->flags & FLAGS_VICTIM) != 0,
			yc == YC_FLAGS && xc == XC_VICTIM);
		DisplayFlag(
			Vec2iNew(230, y), "Awake", (b->flags & FLAGS_AWAKEALWAYS) != 0,
			yc == YC_FLAGS && xc == XC_AWAKE);
		y += FontH();

		DisplayCDogsText(50, y, b->Gun->name, yc == YC_WEAPON);
		y += FontH() + 5;

		x = 10;
		CA_FOREACH(Character, c, setting->characters.OtherChars)
			DrawCharacterSimple(
				c, Vec2iNew(x, y + 20), DIRECTION_DOWN,
				idx == _ca_index, false);
			x += 20;
			if (x > gGraphicsDevice.cachedConfig.Res.x)
			{
				x = 10;
				y += 30;
			}
		CA_FOREACH_END()
	}

	UIObjectDraw(
		sCharEditorObjs, &gGraphicsDevice, Vec2iZero(),
		gEventHandlers.mouse.currentPos, &sDrawObjs);

	if (UITryGetObject(sCharEditorObjs, gEventHandlers.mouse.currentPos, &o) &&
		o->Tooltip)
	{
		UITooltipDraw(
			&gGraphicsDevice, gEventHandlers.mouse.currentPos, o->Tooltip);
	}
	MouseDraw(&gEventHandlers.mouse);

	BlitFlip(&gGraphicsDevice);
}

static const CharacterClass *GetNextCharacterClass(
	const CharacterClass *c, const int d);
static const GunDescription *GetNextGun(const GunDescription *g, const int d);
static bool Change(
	UIObject *o, CharacterStore *store,
	int idx,
	int yc, int xc,
	const int d, const bool shift)
{
	if (idx < 0 || idx >= (int)store->OtherChars.size)
	{
		return false;
	}

	Character *b = CArrayGet(&store->OtherChars, idx);
	switch (yc)
	{
	case YC_APPEARANCE:
		switch (xc) {
		case XC_FACE:
			b->Class = GetNextCharacterClass(b->Class, d);
			return true;
		}
		break;

	case YC_ATTRIBUTES:
		switch (xc) {
		case XC_SPEED:
			b->speed = CLAMP(b->speed + d * 64, 0, 512);
			return true;

		case XC_HEALTH:
			b->maxHealth = CLAMP(b->maxHealth + d * 10, 10, 500);
			return true;

		case XC_MOVE:
			b->bot->probabilityToMove =
				CLAMP(b->bot->probabilityToMove + d * 5, 0, 100);
			return true;

		case XC_TRACK:
			b->bot->probabilityToTrack =
				CLAMP(b->bot->probabilityToTrack + d * 5, 0, 100);
			return true;

		case XC_SHOOT:
			b->bot->probabilityToShoot =
				CLAMP(b->bot->probabilityToShoot + d * 5, 0, 100);
			return true;

		case XC_DELAY:
			b->bot->actionDelay = CLAMP(b->bot->actionDelay + d, 0, 50);
			return true;
		}
		break;

	case YC_FLAGS:
		switch (xc) {
		case XC_ASBESTOS:
			b->flags ^= FLAGS_ASBESTOS;
			return true;

		case XC_IMMUNITY:
			b->flags ^= FLAGS_IMMUNITY;
			return true;

		case XC_SEETHROUGH:
			b->flags ^= FLAGS_SEETHROUGH;
			return true;

		case XC_RUNS_AWAY:
			b->flags ^= FLAGS_RUNS_AWAY;
			return true;

		case XC_SNEAKY:
			b->flags ^= FLAGS_SNEAKY;
			return true;

		case XC_GOOD_GUY:
			b->flags ^= FLAGS_GOOD_GUY;
			return true;

		case XC_SLEEPING:
			b->flags ^= FLAGS_SLEEPALWAYS;
			return true;

		case XC_PRISONER:
			b->flags ^= FLAGS_PRISONER;
			return true;

		case XC_INVULNERABLE:
			b->flags ^= FLAGS_INVULNERABLE;
			return true;

		case XC_FOLLOWER:
			b->flags ^= FLAGS_FOLLOWER;
			return true;

		case XC_PENALTY:
			b->flags ^= FLAGS_PENALTY;
			return true;

		case XC_VICTIM:
			b->flags ^= FLAGS_VICTIM;
			return true;

		case XC_AWAKE:
			b->flags ^= FLAGS_AWAKEALWAYS;
			return true;
		}
		break;

	case YC_WEAPON:
		b->Gun = GetNextGun(b->Gun, d);
		return true;
	}

	if (o)
	{
		return UIObjectChange(o, d, shift) & EDITOR_RESULT_CHANGED;
	}
	return false;
}
// Look in both built-in guns and custom guns for the next gun
static const GunDescription *GetNextGun(const GunDescription *g, const int d)
{
	const int totalSize =
		(int)gGunDescriptions.Guns.size +
		(int)gGunDescriptions.CustomGuns.size;
	for (int i = 0; i < (int)totalSize; i++)
	{
		const GunDescription *gd;
		if (i < (int)gGunDescriptions.Guns.size)
		{
			gd = CArrayGet(&gGunDescriptions.Guns, i);
		}
		else
		{
			gd = CArrayGet(&gGunDescriptions.CustomGuns,
				i - (int)gGunDescriptions.Guns.size);
		}
		if (gd == g)
		{
			for (int j = i + d; j != i; j += d)
			{
				if (j >= totalSize)
				{
					j -= totalSize;
				}
				if (j < 0)
				{
					j += totalSize;
				}
				if (j < (int)gGunDescriptions.Guns.size)
				{
					gd = CArrayGet(&gGunDescriptions.Guns, j);
				}
				else
				{
					gd = CArrayGet(&gGunDescriptions.CustomGuns,
						j - (int)gGunDescriptions.Guns.size);
				}
				if (gd->IsRealGun)
				{
					return gd;
				}
			}
			CASSERT(false, "Unknown error; real gun expected");
		}
	}
	return g;
}
// Look in both built-in classes and custom classes for the next class
static const CharacterClass *GetNextCharacterClass(
	const CharacterClass *c, const int d)
{
	const int totalSize =
		(int)gCharacterClasses.Classes.size +
		(int)gCharacterClasses.CustomClasses.size;
	for (int i = 0; i < (int)totalSize; i++)
	{
		const CharacterClass *cc;
		if (i < (int)gCharacterClasses.Classes.size)
		{
			cc = CArrayGet(&gCharacterClasses.Classes, i);
		}
		else
		{
			cc = CArrayGet(&gCharacterClasses.CustomClasses,
				i - (int)gCharacterClasses.Classes.size);
		}
		if (cc == c)
		{
			const int idx = CLAMP_OPPOSITE(i + d, 0, totalSize - 1);
			if (idx < (int)gCharacterClasses.Classes.size)
			{
				cc = CArrayGet(&gCharacterClasses.Classes, idx);
			}
			else
			{
				cc = CArrayGet(&gCharacterClasses.CustomClasses,
					idx - (int)gCharacterClasses.Classes.size);
			}
			return cc;
		}
	}
	return c;
}


static void InsertCharacter(CharacterStore *store, int idx, Character *data)
{
	Character *c = CharacterStoreInsertOther(store, idx);
	if (data && data->bot)
	{
		memcpy(c, data, sizeof *c);
		CMALLOC(c->bot, sizeof *c->bot);
		memcpy(c->bot, data->bot, sizeof *c->bot);
	}
	else
	{
		// set up character template
		c->Class = StrCharacterClass("Ogre");
		c->Colors.Skin = colorGreen;
		const color_t darkGray = {64, 64, 64, 255};
		c->Colors.Arms = darkGray;
		c->Colors.Body = darkGray;
		c->Colors.Legs = darkGray;
		c->Colors.Hair = colorBlack;
		c->speed = 256;
		c->Gun = StrGunDescription("Machine gun");
		c->maxHealth = 40;
		c->flags = FLAGS_IMMUNITY;
		c->bot->probabilityToMove = 50;
		c->bot->probabilityToTrack = 25;
		c->bot->probabilityToShoot = 2;
		c->bot->actionDelay = 15;
	}
}

static void DeleteCharacter(CharacterStore *store, int *idx)
{
	CharacterStoreDeleteOther(store, *idx);
	if (*idx > 0 && *idx >= (int)store->OtherChars.size - 1)
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
	switch (yc)
	{
	case YC_ATTRIBUTES:
		*xc = CLAMP_OPPOSITE(*xc, 0, XC_DELAY);
		break;

	case YC_FLAGS:
		*xc = CLAMP_OPPOSITE(*xc, 0, XC_AWAKE);
		break;
	}
}

static void HandleInput(
	int *xc, int *yc, int *xcOld, int *ycOld,
	int *idx, CharacterStore *store, Character *scrap, int *done)
{
	UIObject *o = NULL;
	SDL_Scancode sc = KeyGetPressed(&gEventHandlers.keyboard);
	const int m = MouseGetPressed(&gEventHandlers.mouse);
	const bool shift = gEventHandlers.keyboard.modState & KMOD_SHIFT;
	if (m)
	{
		int charIdx;
		*xcOld = *xc;
		*ycOld = *yc;
		if (UITryGetObject(
			sCharEditorObjs, gEventHandlers.mouse.currentPos, &o))
		{
			if (!o->DoNotHighlight)
			{
				if (sLastHighlightedObj)
				{
					UIObjectUnhighlight(sLastHighlightedObj, true);
				}
				sLastHighlightedObj = o;
				UIObjectHighlight(o, shift);
			}
			CArrayTerminate(&sDrawObjs);
			bool isSameSelection;
			if (m == SDL_BUTTON_LEFT || m == SDL_BUTTON_RIGHT)
			{
				*yc = o->Id;
				*xc = o->Id2;
				if (o->Id >= 0)
				{
					AdjustYC(yc);
					if (o->Id2 >= 0)
					{
						AdjustXC(*yc, xc);
					}
				}
			}
			isSameSelection = *xc == *xcOld && *yc == *ycOld;
			if (m == SDL_BUTTON_LEFT ||
				(MouseWheel(&gEventHandlers.mouse).y > 0 && isSameSelection))
			{
				sc = SDL_SCANCODE_PAGEUP;
			}
			else if (m == SDL_BUTTON_RIGHT ||
				(MouseWheel(&gEventHandlers.mouse).y < 0 && isSameSelection))
			{
				sc = SDL_SCANCODE_PAGEDOWN;
			}
		}
		else if ((m == SDL_BUTTON_LEFT || m == SDL_BUTTON_RIGHT) &&
			PosToCharacterIndex(gEventHandlers.mouse.currentPos, &charIdx))
		{
			// Only change selection on left/right click
			if (charIdx >= 0 &&
				charIdx < (int)store->OtherChars.size)
			{
				*idx = charIdx;
			}
		}
	}
	if (gEventHandlers.keyboard.modState & (KMOD_ALT | KMOD_CTRL))
	{
		const SDL_Keycode kc = SDL_GetKeyFromScancode(sc);
		switch (kc)
		{
		case 'x':
			if (*idx < (int)store->OtherChars.size)
			{
				*scrap = *(Character *)CArrayGet(&store->OtherChars, *idx);
				DeleteCharacter(store, idx);
				fileChanged = 1;
			}
			break;

		case 'c':
			if (*idx < (int)store->OtherChars.size)
			{
				*scrap = *(Character *)CArrayGet(&store->OtherChars, *idx);
			}
			break;

		case 'v':
			InsertCharacter(store, *idx, scrap);
			fileChanged = 1;
			break;

		case 'n':
			InsertCharacter(store, store->OtherChars.size, NULL);
			*idx = store->OtherChars.size - 1;
			fileChanged = 1;
			break;

		default:
			// do nothing
			break;
		}
	}
	else
	{
		switch (sc)
		{
		case SDL_SCANCODE_HOME:
			if (*idx > 0)
			{
				(*idx)--;
			}
			break;

		case SDL_SCANCODE_END:
			if (*idx < (int)store->OtherChars.size - 1)
			{
				(*idx)++;
			}
			break;

		case SDL_SCANCODE_INSERT:
			InsertCharacter(store, *idx, NULL);
			fileChanged = 1;
			break;

		case SDL_SCANCODE_DELETE:
			if (*idx >= (int)store->OtherChars.size)
			{
				// Nothing to delete, do nothing
				return;
			}
			DeleteCharacter(store, idx);
			fileChanged = 1;
			break;

		case SDL_SCANCODE_UP:
			(*yc)--;
			AdjustYC(yc);
			AdjustXC(*yc, xc);
			break;

		case SDL_SCANCODE_DOWN:
			(*yc)++;
			AdjustYC(yc);
			AdjustXC(*yc, xc);
			break;

		case SDL_SCANCODE_LEFT:
			(*xc)--;
			AdjustXC(*yc, xc);
			break;

		case SDL_SCANCODE_RIGHT:
			(*xc)++;
			AdjustXC(*yc, xc);
			break;

		case SDL_SCANCODE_PAGEUP:
			fileChanged = Change(o, store, *idx, *yc, *xc, 1, shift);
			break;

		case SDL_SCANCODE_PAGEDOWN:
			fileChanged = Change(o, store, *idx, *yc, *xc, -1, shift);
			break;

		case SDL_SCANCODE_ESCAPE:
			*done = 1;
			break;

		case SDL_SCANCODE_BACKSPACE:
			fileChanged |= UIObjectDelChar(sCharEditorObjs);
			break;

		default:
			// do nothing
			break;
		}
		// Get text input, ASCII only
		char *c = gEventHandlers.keyboard.Typed;
		while (c && *c >= ' ' && *c <= '~')
		{
			fileChanged |= UIObjectAddChar(sCharEditorObjs, *c);
			c++;
		}
	}
}

void EditCharacters(CampaignSetting *setting)
{
	int done = 0;
	int idx = 0;
	int xc = 0, yc = 0;
	int xcOld, ycOld;
	Character scrap;

	memset(&scrap, 0, sizeof(scrap));

	// Initialise UI elements
	sCharEditorObjs = CreateCharEditorObjs(&idx, &setting->characters);
	memset(&sDrawObjs, 0, sizeof sDrawObjs);

	while (!done)
	{
		EventPoll(&gEventHandlers, SDL_GetTicks());
		if (gEventHandlers.HasQuit)
		{
			done = true;
			break;
		}

		HandleInput(
			&xc, &yc, &xcOld, &ycOld, &idx, &setting->characters, &scrap,
			&done);
		Display(setting, idx, xc, yc);
		SDL_Delay(10);
	}

	UIObjectDestroy(sCharEditorObjs);
	CArrayTerminate(&sDrawObjs);
	sLastHighlightedObj = NULL;
}
