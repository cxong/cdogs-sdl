/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.

    Copyright (c) 2013-2015, Cong Xu
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
#pragma once

#include <SDL_events.h>
#include <SDL_keyboard.h>
#include <SDL_keycode.h>
#include <SDL_timer.h>

#include "config.h"

typedef struct
{
	bool isPressed;
	SDL_Keycode keycode;
} KeyPress;

typedef struct
{
	SDL_Scancode left;
	SDL_Scancode right;
	SDL_Scancode up;
	SDL_Scancode down;
	SDL_Scancode button1;
	SDL_Scancode button2;
	SDL_Scancode map;
} InputKeys;
#define MAX_KEYBOARD_CONFIGS 2

typedef enum
{
	KEY_CODE_LEFT,
	KEY_CODE_RIGHT,
	KEY_CODE_UP,
	KEY_CODE_DOWN,
	KEY_CODE_BUTTON1,
	KEY_CODE_BUTTON2,

	KEY_CODE_MAP
} key_code_e;
const char *KeycodeStr(int k);

typedef enum
{
       DIAGONAL_STATUS_UNPRESSED,
       DIAGONAL_STATUS_PRESSED,
       DIAGONAL_STATUS_SUSTAIN
} diagonal_status;

typedef struct
{
        int32_t diagonalTicks[MAX_KEYBOARD_CONFIGS][4];
        diagonal_status diagonalStatus[MAX_KEYBOARD_CONFIGS][4];
} diagonals_hold_status;

typedef struct
{
	KeyPress previousKeys[SDL_NUM_SCANCODES];
	KeyPress currentKeys[SDL_NUM_SCANCODES];
	KeyPress pressedKeys[SDL_NUM_SCANCODES];
	SDL_Keymod modState;
	char Typed[32];
	Uint32 ticks;
	Uint32 repeatedTicks;
	bool isFirstRepeat;
	InputKeys PlayerKeys[MAX_KEYBOARD_CONFIGS];
        diagonals_hold_status diagonalState;
        //diagonalTicks[0] = upRightDiagonalTicks;
        //diagonalTicks[1] = downRightDiagonalTicks;
        //diagonalTicks[2] = downLeftDiagonalTicks};
        //diagonalTicks[3] = upLeftDiagonalTicks;
        //diagonal_status diagonalStatus[MAX_KEYBOARD_CONFIGS][4];
        //diagonal_status upRightDiagonal;
        //diagonal_status downRightDiagonal;
        //diagonal_status downLeftDiagonal; 
        //diagonal_status upLeftDiagonal; Keep arrays straight, status and ticks have to match for the loops to work.
} keyboard_t;

void KeyInit(keyboard_t *keyboard);
InputKeys KeyLoadPlayerKeys(Config *c);
void KeyPrePoll(keyboard_t *keyboard);
void KeyOnKeyDown(keyboard_t *keyboard, const SDL_Keysym s);
void KeyOnKeyUp(keyboard_t *keyboard, const SDL_Keysym s);
void KeyPostPoll(keyboard_t *keyboard, Uint32 ticks);
bool KeyIsDown(const keyboard_t *k, const int key);
bool KeyIsPressed(const keyboard_t *k, const int key);
bool KeyIsReleased(const keyboard_t *k, const int key);
SDL_Scancode KeyGetPressed(const keyboard_t *k);

SDL_Scancode KeyGet(const InputKeys *keys, const key_code_e keyCode);
