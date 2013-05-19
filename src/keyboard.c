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
#include "keyboard.h"

#include <string.h>


keyboard_t gKeyboard;

void KeyInit(keyboard_t *keyboard)
{
	memset(keyboard->current_keys, 0, sizeof(keyboard->current_keys));
	memset(keyboard->previous_keys, 0, sizeof(keyboard->previous_keys));
}

void KeyPoll(keyboard_t *keyboard)
{
	memcpy(keyboard->previous_keys, keyboard->current_keys, sizeof(keyboard->previous_keys));
	while (SDL_PollEvent(&keyboard->keyevent))
	{
		switch(keyboard->keyevent.type)
		{
		case SDL_KEYDOWN:
			keyboard->current_keys[keyboard->keyevent.key.keysym.sym] = 1;
			break;
		case SDL_KEYUP:
			keyboard->current_keys[keyboard->keyevent.key.keysym.sym] = 0;
		default:
			break;
		}
	}
}

int KeyIsDown(keyboard_t *keyboard, int key)
{
	return keyboard->current_keys[key];
}

int KeyIsPressed(keyboard_t *keyboard, int key)
{
	return KeyIsDown(keyboard, key) && !keyboard->previous_keys[key];
}

int KeyIsReleased(keyboard_t *keyboard, int key)
{
	return !KeyIsDown(keyboard, key) && keyboard->previous_keys[key];
}

int KeyGetPressed(keyboard_t *keyboard)
{
	int i;
	for (i = 0; i < 512; i++)
	{
		if (KeyIsPressed(keyboard, i))
		{
			return i;
		}
	}
	return 0;
}
