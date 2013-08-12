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
#ifndef __KEYBOARD
#define __KEYBOARD

#include <SDL_events.h>
#include <SDL_keysym.h>

typedef struct
{
	int isPressed;
	Uint16 unicode;
} KeyPress;

typedef struct
{
	KeyPress previousKeys[512];
	KeyPress currentKeys[512];
	KeyPress pressedKeys[512];
	SDLMod modState;
	Uint32 ticks;
	Uint32 repeatedTicks;
	int isFirstRepeat;
} keyboard_t;

void KeyInit(keyboard_t *keyboard);
void KeyPrePoll(keyboard_t *keyboard);
void KeyOnKeyDown(keyboard_t *keyboard, SDL_keysym s);
void KeyOnKeyUp(keyboard_t *keyboard, SDL_keysym s);
void KeyPostPoll(keyboard_t *keyboard, Uint32 ticks);
int KeyIsDown(keyboard_t *keyboard, int key);
int KeyIsPressed(keyboard_t *keyboard, int key);
int KeyIsReleased(keyboard_t *keyboard, int key);
int KeyGetPressed(keyboard_t *keyboard);
int KeyGetTyped(keyboard_t *keyboard);

#endif
