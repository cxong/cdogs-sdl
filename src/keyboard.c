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

-------------------------------------------------------------------------------

 keyboard.c - keyboard stuff... just think what was once here... (DOS interrupt
 handlers... *shudder*
 
 Author: $Author$
 Rev:    $Revision$
 URL:    $HeadURL$
 ID:     $Id$
 
*/

#include <string.h>

#include "SDL.h"
#include "keyboard.h"


void InitKeyboard(void)
{
	return;
}

char KeyDown(int key)
{
	Uint8 *tmp;

	SDL_PumpEvents();

	tmp = SDL_GetKeyState(NULL);

	return (char)tmp[key];
}

int GetKeyDown(void)
{
	int i;
	int nr_keys;
	Uint8 *keystate;
	
	SDL_PumpEvents();

	keystate = SDL_GetKeyState(&nr_keys);

	/* We force these to be ignored, because they are often turned on,
	   they are toggle keys, and we don't actually need to check for them,
	   and it fixes a bug of the game "hanging" waiting for key release,
	   because Windows often has NumLock on by default! */
	keystate[SDLK_NUMLOCK] = 0;
	keystate[SDLK_CAPSLOCK] = 0;
	keystate[SDLK_SCROLLOCK] = 0;

	/* Okay, this isn't particularly smart, as it returns only the first
	   key it finds. */
	for (i = 0; i < nr_keys; i++) {
		if (keystate[i] == 1) {
			return i;
		}
	}

	return 0;
}

int AnyKeyDown(void)
{
	return (GetKeyDown() > 0);
}

void ClearKeys(void)
{
//      memset(&uKeys, 0, sizeof(uKeys));
}
