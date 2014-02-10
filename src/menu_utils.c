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
#include "menu_utils.h"

#include <assert.h>

#include <cdogs/draw.h>
#include <cdogs/text.h>


// Display a character and the player name above it, with the character
// centered around the target position
void DisplayCharacterAndName(
	GraphicsDevice *g, Vec2i pos, Character *c, char *name)
{
	Vec2i namePos;
	// Move the point down a bit since the default character draw point is at
	// its feet
	pos.y += 8;
	namePos = Vec2iAdd(pos, Vec2iNew(-TextGetStringWidth(name) / 2, -30));
	DrawCharacterSimple(
		c, pos,
		DIRECTION_DOWN, STATE_IDLE, -1, GUNSTATE_READY, &c->table);
	TextString(&gTextManager, name, g, namePos);
}

void MenuDisplayPlayer(
	menu_t *menu, GraphicsDevice *g, Vec2i pos, Vec2i size, void *data)
{
	MenuDisplayPlayerData *d = data;
	Vec2i playerPos;
	char s[22];
	UNUSED(menu);
	pos.x -= size.x;	// move to left half of screen
	playerPos = Vec2iNew(
		pos.x + size.x * 3 / 4 - 12 / 2, CENTER_Y(pos, size, 0));

	if (d->currentMenu && strcmp((*d->currentMenu)->name, "Name") == 0)
	{
		sprintf(s, "%c%s%c", '\020', d->pData->name, '\021');
	}
	else
	{
		strcpy(s, d->pData->name);
	}

	DisplayCharacterAndName(g, playerPos, d->c, s);
}

void MenuDisplayPlayerControls(
	menu_t *menu, GraphicsDevice *g, Vec2i pos, Vec2i size, void *data)
{
	char s[256];
	MenuDisplayPlayerControlsData *d = data;
	Vec2i textPos = Vec2iNew(0, pos.y + size.y - CDogsTextHeight());
	int textWidth = 0;

	UNUSED(menu);

	switch (d->pData->inputDevice)
	{
	case INPUT_DEVICE_KEYBOARD:
		{
			input_keys_t *keys =
				&d->inputConfig->PlayerKeys[d->pData->deviceIndex].Keys;
			sprintf(s, "(%s, %s, %s, %s, %s and %s)",
				SDL_GetKeyName(keys->left),
				SDL_GetKeyName(keys->right),
				SDL_GetKeyName(keys->up),
				SDL_GetKeyName(keys->down),
				SDL_GetKeyName(keys->button1),
				SDL_GetKeyName(keys->button2));
			textWidth = TextGetStringWidth(s);
		}
		break;
	case INPUT_DEVICE_MOUSE:
		sprintf(s, "(mouse wheel to scroll, left and right click)");
		break;
	case INPUT_DEVICE_JOYSTICK:
		sprintf(s, "(%s)",
			InputDeviceName(d->pData->inputDevice, d->pData->deviceIndex));
		break;
	case INPUT_DEVICE_AI:
		sprintf(s, "(Computer)");
		break;
	default:
		assert(0 && "unknown device");
		break;
	}

	// If the text is too long, split the text with a newline
	textWidth = TextGetStringWidth(s);
	if (textWidth < 125)
	{
		textPos.x = pos.x - textWidth / 2;
		TextString(&gTextManager, s, g, textPos);
	}
	else
	{
		// find the first whitespace before half of the string, split there,
		// and print two lines
		char *secondLine;
		for (secondLine = &s[strlen(s) / 2]; secondLine > s; secondLine--)
		{
			if (isspace(*secondLine))
			{
				*secondLine = '\0';
				secondLine++;
				break;
			}
		}
		textWidth = TextGetStringWidth(s);
		textPos.x = pos.x - textWidth / 2;
		textPos.y -= CDogsTextHeight();
		TextString(&gTextManager, s, g, textPos);
		textWidth = TextGetStringWidth(secondLine);
		textPos.x = pos.x - textWidth / 2;
		textPos.y += CDogsTextHeight();
		TextString(&gTextManager, secondLine, g, textPos);
	}
}
