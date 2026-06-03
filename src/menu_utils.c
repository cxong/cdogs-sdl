/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.

	Copyright (c) 2013-2016, 2026 Cong Xu
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
#include <cdogs/draw/drawtools.h>
#include <cdogs/font.h>

// Display a character and the player name above it, with the character
// centered around the target position
void DisplayCharacterAndName(
	struct vec2i pos, const Character *c, const direction_e d,
	const char *name, const color_t color, const WeaponClass *gun)
{
	// Move the point down a bit since the default character draw point is at
	// its feet
	pos.y += 8;
	struct vec2i namePos = svec2i_add(pos, svec2i(-FontStrW(name) / 2, -30));
	DrawCharacterSimple(c, pos, d, false, false, gun);
	FontStrMask(name, namePos, color);
}

void MenuDisplayPlayer(
	const menu_t *menu, GraphicsDevice *g, const struct vec2i pos,
	const struct vec2i size, const void *data)
{
	UNUSED(g);
	const MenuDisplayPlayerData *d = data;
	struct vec2i playerPos;
	char s[22];
	UNUSED(menu);
	struct vec2i dPos = pos;
	dPos.x -= size.x; // move to left half of screen
	playerPos =
		svec2i(dPos.x + size.x * 3 / 4 - 12 / 2, CENTER_Y(dPos, size, 0) - 12);

	PlayerData *pData = PlayerDataGetByUID(d->PlayerUID);
	if (d->currentMenu && strcmp((*d->currentMenu)->name, "Name") == 0)
	{
		sprintf(s, "%c%s%c", '>', pData->name, '<');
	}
	else
	{
		strcpy(s, pData->name);
	}
	const WeaponClass *gun = NULL;
	if (d->GunIdx >= 0 && d->GunIdx < MAX_WEAPONS)
	{
		gun = pData->guns[d->GunIdx];
	}

	DisplayCharacterAndName(
		playerPos, &pData->Char, d->Dir, s, colorWhite, gun);
}

void MenuDisplayPlayerControls(
	const menu_t *menu, GraphicsDevice *g, const struct vec2i pos,
	const struct vec2i size, const void *data)
{
	UNUSED(g);
	char s[256];
	const int *playerUID = data;
	const int keyH = 13;
	const int y = pos.y + size.y - keyH;

	UNUSED(menu);

	const PlayerData *pData = PlayerDataGetByUID(*playerUID);
	switch (pData->inputDevice)
	{
	case INPUT_DEVICE_KEYBOARD: {
		const int keyBgW = 13;
		// Try to roughly center the buttons
		struct vec2i keyPos = svec2i(pos.x - (keyBgW * 5 + 3) / 2, y);
		keyPos = DrawDirectionButtons(
			pData->inputDevice, pData->deviceIndex, keyPos);
		keyPos.x += 3; // add some spacing between directions and buttons
		keyPos = DrawButton(
			pData->inputDevice, pData->deviceIndex, CMD_BUTTON1, keyPos);
		keyPos = DrawButton(
			pData->inputDevice, pData->deviceIndex, CMD_BUTTON2, keyPos);
	}
	break;
	case INPUT_DEVICE_JOYSTICK: {
		const int keyBgW = 13;
		struct vec2i buttonPos = svec2i(pos.x - (14 + keyBgW * 2) / 2, y);
		buttonPos = DrawDirectionButtons(
			pData->inputDevice, pData->deviceIndex, buttonPos);
		buttonPos.x += 3; // add some spacing between directions and buttons
		buttonPos = DrawButton(
			pData->inputDevice, pData->deviceIndex, CMD_BUTTON1, buttonPos);
		buttonPos = DrawButton(
			pData->inputDevice, pData->deviceIndex, CMD_BUTTON2, buttonPos);
	}
	break;
	case INPUT_DEVICE_AI:
		sprintf(
			s, "(%s)",
			InputDeviceName(pData->inputDevice, pData->deviceIndex));
		FontStr(s, svec2i(pos.x - FontStrW(s) / 2, y));
		break;
	case INPUT_DEVICE_UNSET:
		strcpy(s, "(no device; plug in a controller)");
		FontStr(s, svec2i(pos.x - FontStrW(s) / 2, y));
		break;
	default:
		CASSERT(false, "unknown device");
		break;
	}
}
