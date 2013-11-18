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
#ifndef __INPUT
#define __INPUT

#include <string.h>

#include "character.h"
#include "joystick.h"
#include "keyboard.h"
#include "mouse.h"
#include "sys_specifics.h"
#include "utils.h"

typedef struct
{
	int left;
	int right;
	int up;
	int down;
	int button1;
	int button2;
	int map;
} input_keys_t;

typedef struct
{
	input_device_e Device;
	input_keys_t Keys;
} KeyConfig;

#define MAX_KEYBOARD_CONFIGS 2
typedef struct
{
	KeyConfig PlayerKeys[MAX_KEYBOARD_CONFIGS];
} InputConfig;

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

typedef struct
{
	keyboard_t keyboard;
	joysticks_t joysticks;
	Mouse mouse;
} InputDevices;

extern InputDevices gInputDevices;

void InputChangeDevice(
	InputDevices *devices, input_device_e *d, input_device_e *dOther);

void GetPlayerCmds(int (*cmds)[MAX_PLAYERS]);
int GetMenuCmd(void);

void InputInit(InputDevices *devices, PicPaletted *mouseCursor);
int InputGetKey(input_keys_t *keys, key_code_e keyCode);
int InputGetGameCmd(
	InputDevices *devices, InputConfig *config, int player, Vec2i playerPos);
void InputSetKey(input_keys_t *keys, int key, key_code_e keyCode);
void InputPoll(InputDevices *devices, Uint32 ticks);
void InputTerminate(InputDevices *devices);

const char *InputDeviceName(int d);

#endif
