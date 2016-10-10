/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.

	Copyright (c) 2013-2016, Cong Xu
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
#include "menu_utils.h"

#include <assert.h>

#include <cdogs/draw/draw.h>
#include <cdogs/draw/draw_actor.h>
#include <cdogs/font.h>


// Display a character and the player name above it, with the character
// centered around the target position
void DisplayCharacterAndName(
	Vec2i pos, Character *c, const direction_e d,
	const char *name, const color_t color)
{
	// Move the point down a bit since the default character draw point is at
	// its feet
	pos.y += 8;
	Vec2i namePos = Vec2iAdd(pos, Vec2iNew(-FontStrW(name) / 2, -30));
	DrawCharacterSimple(c, pos, d, false, false);
	FontStrMask(name, namePos, color);
}

void MenuDisplayPlayer(
	const menu_t *menu, GraphicsDevice *g,
	const Vec2i pos, const Vec2i size, const void *data)
{
	UNUSED(g);
	const MenuDisplayPlayerData *d = data;
	Vec2i playerPos;
	char s[22];
	UNUSED(menu);
	Vec2i dPos = pos;
	dPos.x -= size.x;	// move to left half of screen
	playerPos = Vec2iNew(
		dPos.x + size.x * 3 / 4 - 12 / 2, CENTER_Y(dPos, size, 0));

	PlayerData *pData = PlayerDataGetByUID(d->PlayerUID);
	if (d->currentMenu && strcmp((*d->currentMenu)->name, "Name") == 0)
	{
		sprintf(s, "%c%s%c", '>', pData->name, '<');
	}
	else
	{
		strcpy(s, pData->name);
	}

	DisplayCharacterAndName(playerPos, &pData->Char, d->Dir, s, colorWhite);
}

void MenuDisplayPlayerControls(
	const menu_t *menu, GraphicsDevice *g,
	const Vec2i pos, const Vec2i size, const void *data)
{
	UNUSED(g);
	char s[256];
	const int *playerUID = data;
	const int y = pos.y + size.y - FontH();

	UNUSED(menu);

	const PlayerData *pData = PlayerDataGetByUID(*playerUID);
	char directionNames[256];
	InputGetDirectionNames(
		directionNames, pData->inputDevice, pData->deviceIndex);
	switch (pData->inputDevice)
	{
	case INPUT_DEVICE_KEYBOARD:
		{
			char button1[256], button2[256];
			InputGetButtonName(
				pData->inputDevice, pData->deviceIndex, CMD_BUTTON1, button1);
			InputGetButtonName(
				pData->inputDevice, pData->deviceIndex, CMD_BUTTON2, button2);
			sprintf(s, "(%s,\n%s and %s)", directionNames, button1, button2);
			FontStr(s, Vec2iNew(pos.x - FontStrW(s) / 2, y - FontH()));
		}
		break;
	case INPUT_DEVICE_MOUSE:
		sprintf(s, "(%s to scroll,\nleft and right click)", directionNames);
		FontStr(s, Vec2iNew(pos.x - FontStrW(s) / 2, y - FontH()));
		break;
	case INPUT_DEVICE_JOYSTICK:
		{
			sprintf(s, "(%s,",
				InputDeviceName(pData->inputDevice, pData->deviceIndex));
			Vec2i textPos = Vec2iNew(pos.x - FontStrW(s) / 2, y - FontH());
			FontStr(s, textPos);
			textPos.y += FontH();
			color_t c = colorWhite;
			InputGetButtonNameColor(
				pData->inputDevice, pData->deviceIndex, CMD_BUTTON1, s, &c);
			textPos = FontStrMask(s, textPos, c);
			textPos = FontStr(" and ", textPos);
			c = colorWhite;
			InputGetButtonNameColor(
				pData->inputDevice, pData->deviceIndex, CMD_BUTTON2, s, &c);
			textPos = FontStrMask(s, textPos, c);
			FontStr(")", textPos);
		}
		break;
	case INPUT_DEVICE_AI:
		sprintf(s, "(%s)",
			InputDeviceName(pData->inputDevice, pData->deviceIndex));
		FontStr(s, Vec2iNew(pos.x - FontStrW(s) / 2, y));
		break;
	case INPUT_DEVICE_UNSET:
		strcpy(s, "(no device; plug in a controller)");
		FontStr(s, Vec2iNew(pos.x - FontStrW(s) / 2, y));
		break;
	default:
		CASSERT(false, "unknown device");
		break;
	}
}
