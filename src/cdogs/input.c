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
#include "input.h"

#include <assert.h>
#include <stdlib.h>

#include "config.h"
#include "joystick.h"
#include "keyboard.h"
#include "defs.h"
#include "sounds.h"
#include "gamedata.h"

#define MOUSE_MOVE_DEAD_ZONE 12

InputDevices gInputDevices;


void InputChangeDevice(
	InputDevices *devices, input_device_e *d, input_device_e *dOther)
{
	int numJoys = devices->joysticks.numJoys;
	input_device_e newDevice;
	int isFirst = 1;
	int available[INPUT_DEVICE_COUNT];
	available[INPUT_DEVICE_KEYBOARD] = 1;
	available[INPUT_DEVICE_MOUSE] = *dOther != INPUT_DEVICE_MOUSE;
	available[INPUT_DEVICE_JOYSTICK_1] =
		numJoys >= 1 && *dOther != INPUT_DEVICE_JOYSTICK_1;
	available[INPUT_DEVICE_JOYSTICK_2] =
		numJoys >= 2 && *dOther != INPUT_DEVICE_JOYSTICK_2;
	for (newDevice = *d; isFirst || newDevice != *d;)
	{
		if (!isFirst && available[newDevice])
		{
			break;
		}
		isFirst = 0;
		newDevice++;
		if (newDevice == INPUT_DEVICE_COUNT)
		{
			newDevice = INPUT_DEVICE_KEYBOARD;
		}
	}
	*d = newDevice;
	debug(D_NORMAL, "change control to: %s\n", InputDeviceStr(*d));
}

static int SwapButtons(int cmd)
{
	int c = (cmd & ~(CMD_BUTTON1 | CMD_BUTTON2));
	if (cmd & CMD_BUTTON1)
		c |= CMD_BUTTON2;
	if (cmd & CMD_BUTTON2)
		c |= CMD_BUTTON1;
	return c;
}

int GetKeyboardCmd(
	keyboard_t *keyboard, input_keys_t *keys,
	int (*keyFunc)(keyboard_t *, int))
{
	int cmd = 0;
	
	if (keyFunc(keyboard, keys->left))			cmd |= CMD_LEFT;
	else if (keyFunc(keyboard, keys->right))	cmd |= CMD_RIGHT;
	
	if (keyFunc(keyboard, keys->up))			cmd |= CMD_UP;
	else if (keyFunc(keyboard, keys->down))		cmd |= CMD_DOWN;
	
	if (keyFunc(keyboard, keys->button1))		cmd |= CMD_BUTTON1;

	if (keyFunc(keyboard, keys->button2))		cmd |= CMD_BUTTON2;

	return cmd;
}

int GetMouseCmd(
	Mouse *mouse, int (*mouseFunc)(Mouse *, int), int useMouseMove, Vec2i pos)
{
	int cmd = 0;
	
	if (useMouseMove)
	{
		int dx = abs(mouse->currentPos.x - pos.x);
		int dy = abs(mouse->currentPos.y - pos.y);
		if (dx > MOUSE_MOVE_DEAD_ZONE || dy > MOUSE_MOVE_DEAD_ZONE)
		{
			if (2 * dx > dy)
			{
				if (pos.x < mouse->currentPos.x)			cmd |= CMD_RIGHT;
				else if (pos.x > mouse->currentPos.x)		cmd |= CMD_LEFT;
			}
			if (2 * dy > dx)
			{
				if (pos.y < mouse->currentPos.y)			cmd |= CMD_DOWN;
				else if (pos.y > mouse->currentPos.y)		cmd |= CMD_UP;
			}
		}
	}
	else
	{
		if (mouseFunc(mouse, SDL_BUTTON_WHEELUP))			cmd |= CMD_UP;
		else if (mouseFunc(mouse, SDL_BUTTON_WHEELDOWN))	cmd |= CMD_DOWN;
	}

	if (mouseFunc(mouse, SDL_BUTTON_LEFT))					cmd |= CMD_BUTTON1;
	if (mouseFunc(mouse, SDL_BUTTON_RIGHT))					cmd |= CMD_BUTTON2;
	if (mouseFunc(mouse, SDL_BUTTON_MIDDLE))				cmd |= CMD_BUTTON3;
	
	return cmd;
}

int GetJoystickCmd(
	joystick_t *joystick, int (*joyFunc)(joystick_t *, int), int swapButtons)
{
	int cmd = 0;

	if (joyFunc(joystick, CMD_LEFT))		cmd |= CMD_LEFT;
	else if (joyFunc(joystick, CMD_RIGHT))	cmd |= CMD_RIGHT;

	if (joyFunc(joystick, CMD_UP))			cmd |= CMD_UP;
	else if (joyFunc(joystick, CMD_DOWN))	cmd |= CMD_DOWN;
	
	if (joyFunc(joystick, CMD_BUTTON1))		cmd |= CMD_BUTTON1;

	if (joyFunc(joystick, CMD_BUTTON2))		cmd |= CMD_BUTTON2;
	
	if (joyFunc(joystick, CMD_BUTTON3))		cmd |= CMD_BUTTON3;

	if (joyFunc(joystick, CMD_BUTTON4))		cmd |= CMD_BUTTON4;
	
	if (swapButtons)
	{
		cmd = SwapButtons(cmd);
	}
	
	return cmd;
}

int GetOnePlayerCmd(
	KeyConfig *config,
	int (*keyFunc)(keyboard_t *, int),
	int (*mouseFunc)(Mouse *, int),
	int (*joyFunc)(joystick_t *, int))
{
	int cmd = 0;
	if (config->Device == INPUT_DEVICE_KEYBOARD)
	{
		cmd = GetKeyboardCmd(&gInputDevices.keyboard, &config->Keys, keyFunc);
	}
	else if (config->Device == INPUT_DEVICE_MOUSE)
	{
		cmd = GetMouseCmd(&gInputDevices.mouse, mouseFunc, 0, Vec2iZero());
	}
	else
	{
		int swapButtons = 0;
		joystick_t *joystick = &gInputDevices.joysticks.joys[0];

		if (config->Device == INPUT_DEVICE_JOYSTICK_1)
		{
			joystick = &gInputDevices.joysticks.joys[0];
			swapButtons = gConfig.Input.SwapButtonsJoystick1;
		}
		else if (config->Device == INPUT_DEVICE_JOYSTICK_2)
		{
			joystick = &gInputDevices.joysticks.joys[1];
			swapButtons = gConfig.Input.SwapButtonsJoystick2;
		}

		cmd = GetJoystickCmd(joystick, joyFunc, swapButtons);
	}
	return cmd;
}


void GetPlayerCmd(int *cmd1, int *cmd2)
{
	int (*keyFunc)(keyboard_t *, int) = KeyIsPressed;
	int (*mouseFunc)(Mouse *, int) = MouseIsPressed;
	int (*joyFunc)(joystick_t *, int) = JoyIsPressed;

	if (cmd1 != NULL)
	{
		*cmd1 = GetOnePlayerCmd(
			&gConfig.Input.PlayerKeys[0], keyFunc, mouseFunc, joyFunc);
	}
	if (cmd2 != NULL)
	{
		*cmd2 = GetOnePlayerCmd(
			&gConfig.Input.PlayerKeys[1], keyFunc, mouseFunc, joyFunc);
	}
}

int InputGetGameCmd(
	InputDevices *devices, InputConfig *config, int player, Vec2i playerPos)
{
	int cmd = 0;
	int swapButtons = 0;
	joystick_t *joystick = &devices->joysticks.joys[0];
	
	switch (config->PlayerKeys[player].Device)
	{
		case INPUT_DEVICE_KEYBOARD:
			cmd = GetKeyboardCmd(
				&devices->keyboard,
				&config->PlayerKeys[player].Keys,
				KeyIsDown);
			break;
		case INPUT_DEVICE_MOUSE:
			cmd = GetMouseCmd(&devices->mouse, MouseIsDown, 1, playerPos);
			break;
		case INPUT_DEVICE_JOYSTICK_1:
			joystick = &devices->joysticks.joys[0];
			swapButtons = config->SwapButtonsJoystick1;
			cmd = GetJoystickCmd(joystick, JoyIsDown, swapButtons);
			break;
		case INPUT_DEVICE_JOYSTICK_2:
			joystick = &devices->joysticks.joys[1];
			swapButtons = config->SwapButtonsJoystick2;
			cmd = GetJoystickCmd(joystick, JoyIsDown, swapButtons);
			break;
		default:
			assert(0 && "unknown input device");
			break;
	}
	
	return cmd;
}

int GetMenuCmd(void)
{
	int cmd = 0;
	if (KeyIsPressed(&gInputDevices.keyboard, SDLK_ESCAPE))
	{
		return CMD_ESC;
	}

	GetPlayerCmd(&cmd, NULL);
	if (!cmd)
	{
		if (KeyIsPressed(&gInputDevices.keyboard, SDLK_LEFT))
		{
			cmd |= CMD_LEFT;
		}
		else if (KeyIsPressed(&gInputDevices.keyboard, SDLK_RIGHT))
		{
			cmd |= CMD_RIGHT;
		}
		if (KeyIsPressed(&gInputDevices.keyboard, SDLK_UP))
		{
			cmd |= CMD_UP;
		}
		else if (KeyIsPressed(&gInputDevices.keyboard, SDLK_DOWN))
		{
			cmd |= CMD_DOWN;
		}
		if (KeyIsPressed(&gInputDevices.keyboard, SDLK_RETURN))
		{
			cmd |= CMD_BUTTON1;
		}
		if (KeyIsPressed(&gInputDevices.keyboard, SDLK_BACKSPACE))
		{
			cmd |= CMD_BUTTON2;
		}
	}

	return cmd;
}

void InputInit(InputDevices *devices, PicPaletted *mouseCursor)
{
	KeyInit(&devices->keyboard);
	JoyInit(&devices->joysticks);
	MouseInit(&devices->mouse, mouseCursor);
}

int InputGetKey(input_keys_t *keys, key_code_e keyCode)
{
	switch (keyCode)
	{
	case KEY_CODE_LEFT:
		return keys->left;
		break;
	case KEY_CODE_RIGHT:
		return keys->right;
		break;
	case KEY_CODE_UP:
		return keys->up;
		break;
	case KEY_CODE_DOWN:
		return keys->down;
		break;
	case KEY_CODE_BUTTON1:
		return keys->button1;
		break;
	case KEY_CODE_BUTTON2:
		return keys->button2;
		break;
	default:
		printf("Error unhandled key code %d\n", keyCode);
		assert(0);
		return 0;
	}
}

void InputSetKey(input_keys_t *keys, int key, key_code_e keyCode)
{
	switch (keyCode)
	{
	case KEY_CODE_LEFT:
		keys->left = key;
		break;
	case KEY_CODE_RIGHT:
		keys->right = key;
		break;
	case KEY_CODE_UP:
		keys->up = key;
		break;
	case KEY_CODE_DOWN:
		keys->down = key;
		break;
	case KEY_CODE_BUTTON1:
		keys->button1 = key;
		break;
	case KEY_CODE_BUTTON2:
		keys->button2 = key;
		break;
	default:
		assert(0);
		break;
	}
}

void InputPoll(InputDevices *devices, Uint32 ticks)
{
	SDL_Event e;
	KeyPrePoll(&devices->keyboard);
	JoyPoll(&devices->joysticks);
	MousePrePoll(&devices->mouse);
	while (SDL_PollEvent(&e))
	{
		switch(e.type)
		{
		case SDL_KEYDOWN:
			KeyOnKeyDown(&devices->keyboard, e.key.keysym);
			break;
		case SDL_KEYUP:
			KeyOnKeyUp(&devices->keyboard, e.key.keysym);
			break;
		case SDL_MOUSEBUTTONDOWN:
			MouseOnButtonDown(&devices->mouse, e.button.button);
			break;
		case SDL_MOUSEBUTTONUP:
			MouseOnButtonUp(&devices->mouse, e.button.button);
			break;
		default:
			break;
		}
	}
	KeyPostPoll(&devices->keyboard, ticks);
	MousePostPoll(&devices->mouse, ticks);
}

void InputTerminate(InputDevices *devices)
{
	JoyTerminate(&devices->joysticks);
}

const char *InputDeviceName(int d)
{
	switch (d)
	{
	case INPUT_DEVICE_KEYBOARD:
		return "Keyboard";
	case INPUT_DEVICE_MOUSE:
		return "Mouse";
	case INPUT_DEVICE_JOYSTICK_1:
		return SDL_JoystickName(0);
	case INPUT_DEVICE_JOYSTICK_2:
		return SDL_JoystickName(1);
	default:
		return "";
	}
}
