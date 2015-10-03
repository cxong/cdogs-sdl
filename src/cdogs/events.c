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
#include "events.h"

#include <assert.h>
#include <stdlib.h>

#include <SDL_timer.h>

#include "config.h"
#include "gamedata.h"
#include "pic_manager.h"


EventHandlers gEventHandlers;

void EventInit(
	EventHandlers *handlers, Pic *mouseCursor, Pic *mouseTrail,
	const bool hideMouse)
{
	memset(handlers, 0, sizeof *handlers);
	KeyInit(&handlers->keyboard);
	JoyInit(&handlers->joysticks);
	MouseInit(&handlers->mouse, mouseCursor, mouseTrail, hideMouse);
}
void EventTerminate(EventHandlers *handlers)
{
	JoyTerminate(&handlers->joysticks);
}
void EventReset(EventHandlers *handlers, Pic *mouseCursor, Pic *mouseTrail)
{
	handlers->HasResolutionChanged = 0;
	KeyInit(&handlers->keyboard);
	JoyInit(&handlers->joysticks);
	MouseInit(
		&handlers->mouse, mouseCursor, mouseTrail, handlers->mouse.hideMouse);
}

void EventPoll(EventHandlers *handlers, Uint32 ticks)
{
	SDL_Event e;
	handlers->HasResolutionChanged = 0;
	KeyPrePoll(&handlers->keyboard);
	JoyPoll(&handlers->joysticks);
	MousePrePoll(&handlers->mouse);
	while (SDL_PollEvent(&e))
	{
		switch (e.type)
		{
		case SDL_KEYDOWN:
			KeyOnKeyDown(&handlers->keyboard, e.key.keysym);
			break;
		case SDL_KEYUP:
			KeyOnKeyUp(&handlers->keyboard, e.key.keysym);
			break;
		case SDL_MOUSEBUTTONDOWN:
			MouseOnButtonDown(&handlers->mouse, e.button.button);
			break;
		case SDL_MOUSEBUTTONUP:
			MouseOnButtonUp(&handlers->mouse, e.button.button);
			break;
		case SDL_VIDEORESIZE:
			{
				const int scale = ConfigGetInt(&gConfig, "Graphics.ScaleFactor");
				GraphicsConfigSet(
					&gGraphicsDevice.cachedConfig,
					Vec2iNew(e.resize.w / scale, e.resize.h / scale),
					false,
					scale);
				GraphicsInitialize(&gGraphicsDevice, false);
				handlers->HasResolutionChanged = 1;
			}
			break;
		case SDL_QUIT:
			handlers->HasQuit = true;
			break;
		default:
			break;
		}
	}
	KeyPostPoll(&handlers->keyboard, ticks);
	MousePostPoll(&handlers->mouse, ticks);
}

static int GetKeyboardCmd(
	keyboard_t *keyboard, const int kbIndex, const bool isPressed)
{
	int cmd = 0;
	int (*keyFunc)(keyboard_t *, int) = isPressed ? KeyIsPressed : KeyIsDown;
	const input_keys_t *keys = &keyboard->PlayerKeys[kbIndex];

	if (keyFunc(keyboard, keys->left))			cmd |= CMD_LEFT;
	else if (keyFunc(keyboard, keys->right))	cmd |= CMD_RIGHT;

	if (keyFunc(keyboard, keys->up))			cmd |= CMD_UP;
	else if (keyFunc(keyboard, keys->down))		cmd |= CMD_DOWN;

	if (keyFunc(keyboard, keys->button1))		cmd |= CMD_BUTTON1;

	if (keyFunc(keyboard, keys->button2))		cmd |= CMD_BUTTON2;

	return cmd;
}
static int GetMouseCmd(
	Mouse *mouse, bool isPressed, int useMouseMove, Vec2i pos)
{
	int cmd = 0;
	int (*mouseFunc)(Mouse *, int) = isPressed ? MouseIsPressed : MouseIsDown;

	if (useMouseMove)
	{
		cmd |= MouseGetMove(mouse, pos);
	}
	else
	{
		if (mouseFunc(mouse, SDL_BUTTON_WHEELUP))			cmd |= CMD_UP;
		else if (mouseFunc(mouse, SDL_BUTTON_WHEELDOWN))	cmd |= CMD_DOWN;
	}

	if (mouseFunc(mouse, SDL_BUTTON_LEFT))					cmd |= CMD_BUTTON1;
	if (mouseFunc(mouse, SDL_BUTTON_RIGHT))					cmd |= CMD_BUTTON2;
	if (mouseFunc(mouse, SDL_BUTTON_MIDDLE))				cmd |= CMD_MAP;

	return cmd;
}

static int GetJoystickCmd(joystick_t *joystick, bool isPressed)
{
	int cmd = 0;
	int (*joyFunc)(joystick_t *, int) = isPressed ? JoyIsPressed : JoyIsDown;

	if (joyFunc(joystick, CMD_LEFT))		cmd |= CMD_LEFT;
	else if (joyFunc(joystick, CMD_RIGHT))	cmd |= CMD_RIGHT;

	if (joyFunc(joystick, CMD_UP))			cmd |= CMD_UP;
	else if (joyFunc(joystick, CMD_DOWN))	cmd |= CMD_DOWN;

	if (joyFunc(joystick, CMD_BUTTON1))		cmd |= CMD_BUTTON1;

	if (joyFunc(joystick, CMD_BUTTON2))		cmd |= CMD_BUTTON2;

	if (joyFunc(joystick, CMD_MAP))			cmd |= CMD_MAP;

	if (joyFunc(joystick, CMD_ESC))			cmd |= CMD_ESC;

	return cmd;
}

int GetGameCmd(
	EventHandlers *handlers,
	const PlayerData *playerData, const Vec2i playerPos)
{
	int cmd = 0;

	switch (playerData->inputDevice)
	{
	case INPUT_DEVICE_KEYBOARD:
		cmd = GetKeyboardCmd(
			&handlers->keyboard, playerData->deviceIndex, false);
		break;
	case INPUT_DEVICE_MOUSE:
		cmd = GetMouseCmd(&handlers->mouse, false, 1, playerPos);
		break;
	case INPUT_DEVICE_JOYSTICK:
		{
			joystick_t *joystick =
				&handlers->joysticks.joys[playerData->deviceIndex];
			cmd = GetJoystickCmd(joystick, false);
		}
		break;
	default:
		// do nothing
		break;
	}

	return cmd;
}

int GetOnePlayerCmd(
	EventHandlers *handlers, const bool isPressed,
	const input_device_e device, const int deviceIndex)
{
	int cmd = 0;
	switch (device)
	{
	case INPUT_DEVICE_KEYBOARD:
		cmd = GetKeyboardCmd(&handlers->keyboard, deviceIndex, isPressed);
		break;
	case INPUT_DEVICE_MOUSE:
		cmd = GetMouseCmd(&handlers->mouse, isPressed, 0, Vec2iZero());
		break;
	case INPUT_DEVICE_JOYSTICK:
		{
			joystick_t *joystick = &handlers->joysticks.joys[deviceIndex];
			cmd = GetJoystickCmd(joystick, isPressed);
		}
		break;
	default:
		// Do nothing
		break;
	}
	return cmd;
}

void GetPlayerCmds(EventHandlers *handlers, int (*cmds)[MAX_LOCAL_PLAYERS])
{
	int idx = 0;
	for (int i = 0; i < (int)gPlayerDatas.size; i++, idx++)
	{
		const PlayerData *p = CArrayGet(&gPlayerDatas, i);
		if (!p->IsLocal)
		{
			idx--;
			continue;
		}
		if (p->inputDevice != INPUT_DEVICE_UNSET)
		{
			(*cmds)[idx] = GetOnePlayerCmd(
				handlers, true, p->inputDevice, p->deviceIndex);
		}
	}
}

int GetMenuCmd(EventHandlers *handlers)
{
	keyboard_t *kb = &handlers->keyboard;
	if (KeyIsPressed(kb, SDLK_ESCAPE) ||
		JoyIsPressed(&handlers->joysticks.joys[0], CMD_ESC))
	{
		return CMD_ESC;
	}

	// Check first player keyboard
	int cmd = GetOnePlayerCmd(handlers, true, INPUT_DEVICE_KEYBOARD, 0);
	if (!cmd)
	{
		// Check keyboard
		if (KeyIsPressed(kb, SDLK_LEFT))		cmd |= CMD_LEFT;
		else if (KeyIsPressed(kb, SDLK_RIGHT))	cmd |= CMD_RIGHT;

		if (KeyIsPressed(kb, SDLK_UP))			cmd |= CMD_UP;
		else if (KeyIsPressed(kb, SDLK_DOWN))	cmd |= CMD_DOWN;

		if (KeyIsPressed(kb, SDLK_RETURN))		cmd |= CMD_BUTTON1;

		if (KeyIsPressed(kb, SDLK_BACKSPACE))	cmd |= CMD_BUTTON2;
	}
	if (!cmd && handlers->joysticks.numJoys > 0)
	{
		// Check joystick 1
		cmd = GetOnePlayerCmd(handlers, true, INPUT_DEVICE_JOYSTICK, 0);
	}
	if (!cmd)
	{
		// Check mouse
		cmd = GetOnePlayerCmd(handlers, true, INPUT_DEVICE_MOUSE, 0);
	}

	return cmd;
}


const char *InputGetButtonName(
	const input_device_e d, const int dIndex, const int cmd)
{
	color_t c;
	return InputGetButtonNameColor(d, dIndex, cmd, &c);
}
const char *InputGetButtonNameColor(
	const input_device_e d, const int dIndex, const int cmd, color_t *color)
{
	switch (d)
	{
	case INPUT_DEVICE_KEYBOARD:
	#ifdef __GCWZERO__
		*color = colorBlue;
		switch (cmd)
		{
			case CMD_LEFT: return "left";
			case CMD_RIGHT: return "right";
			case CMD_UP: return "up";
			case CMD_DOWN: return "down";
			case CMD_BUTTON1: return "A";
			case CMD_BUTTON2: return "B";
			case CMD_MAP: *color = colorGray; return "L";
			case CMD_ESC: return "SELECT";
			default: CASSERT(false, "unknown button"); return NULL;
		}
	#else
		{
			const input_keys_t *keys =
				&gEventHandlers.keyboard.PlayerKeys[dIndex];
			switch (cmd)
			{
			case CMD_LEFT: return SDL_GetKeyName(keys->left);
			case CMD_RIGHT: return SDL_GetKeyName(keys->right);
			case CMD_UP: return SDL_GetKeyName(keys->up);
			case CMD_DOWN: return SDL_GetKeyName(keys->down);
			case CMD_BUTTON1: return SDL_GetKeyName(keys->button1);
			case CMD_BUTTON2: return SDL_GetKeyName(keys->button2);
			case CMD_MAP: return SDL_GetKeyName(keys->map);
			case CMD_ESC: return SDL_GetKeyName(SDLK_ESCAPE);
			default: CASSERT(false, "unknown button"); return NULL;
			}
		}
	#endif
		break;
	case INPUT_DEVICE_MOUSE:
		switch (cmd)
		{
		case CMD_LEFT: return "left";
		case CMD_RIGHT: return "right";
		case CMD_UP: return "up";
		case CMD_DOWN: return "down";
		case CMD_BUTTON1: return "left click";
		case CMD_BUTTON2: return "right click";
		case CMD_MAP: return "middle click";
		case CMD_ESC: return "";
		default: CASSERT(false, "unknown button"); return NULL;
		}
		break;
	case INPUT_DEVICE_JOYSTICK:
		// TODO: button names and special colours for popular gamepads
		return JoyButtonNameColor(dIndex, cmd, color);
	case INPUT_DEVICE_AI:
		return NULL;
	default:
		CASSERT(false, "unknown input device");
		return NULL;
	}
}
void InputGetDirectionNames(
	char *buf, const input_device_e d, const int dIndex)
{
	strcpy(buf, "");
	switch (d)
	{
	case INPUT_DEVICE_KEYBOARD:
		sprintf(buf, "%s, %s, %s, %s",
			InputGetButtonName(d, dIndex, CMD_LEFT),
			InputGetButtonName(d, dIndex, CMD_RIGHT),
			InputGetButtonName(d, dIndex, CMD_UP),
			InputGetButtonName(d, dIndex, CMD_DOWN));
		break;
	case INPUT_DEVICE_MOUSE:
		strcpy(buf, "mouse wheel");
		break;
	case INPUT_DEVICE_JOYSTICK:
		strcpy(buf, "directions");
		break;
	case INPUT_DEVICE_AI:
		break;
	default:
		CASSERT(false, "unknown device");
		break;
	}
}

int GetKey(EventHandlers *handlers)
{
	int key_pressed = 0;
	do
	{
		EventPoll(handlers, SDL_GetTicks());
		key_pressed = KeyGetPressed(&handlers->keyboard);
	} while (!key_pressed);
	return key_pressed;
}

static GameLoopResult WaitResult(
	GameLoopWaitForAnyKeyOrButtonData *data, const bool result);
GameLoopResult GameLoopWaitForAnyKeyOrButtonFunc(void *data)
{
	GameLoopWaitForAnyKeyOrButtonData *wData = data;
	int cmds[MAX_LOCAL_PLAYERS];
	memset(cmds, 0, sizeof cmds);
	GetPlayerCmds(&gEventHandlers, &cmds);
	for (int i = 0; i < MAX_LOCAL_PLAYERS; i++)
	{
		if (cmds[i] & (CMD_BUTTON1 | CMD_BUTTON2))
		{
			// Interpret anything other than CMD_BUTTON1 as cancel
			return WaitResult(wData, cmds[i] & CMD_BUTTON1);
		}
	}

	// Check menu commands
	const int menuCmd = GetMenuCmd(&gEventHandlers);
	if (menuCmd & (CMD_BUTTON1 | CMD_BUTTON2))
	{
		// Interpret anything other than CMD_BUTTON1 as cancel
		return WaitResult(wData, menuCmd & CMD_BUTTON1);
	}

	// Check if anyone pressed escape
	if (EventIsEscape(&gEventHandlers, cmds, menuCmd))
	{
		return WaitResult(wData, false);
	}

	return UPDATE_RESULT_OK;
}
static GameLoopResult WaitResult(
	GameLoopWaitForAnyKeyOrButtonData *data, const bool result)
{
	if (data)
	{
		data->IsOK = result;
	}
	return UPDATE_RESULT_EXIT;
}

bool EventIsEscape(
	EventHandlers *handlers,
	const int cmds[MAX_LOCAL_PLAYERS], const int menuCmd)
{
	for (int i = 0; i < MAX_LOCAL_PLAYERS; i++)
	{
		if (cmds[i] & CMD_ESC)
		{
			return true;
		}
	}

	// Check keyboard escape
	if (KeyIsPressed(&handlers->keyboard, SDLK_ESCAPE) || handlers->HasQuit)
	{
		return true;
	}

	// Check menu commands
	if (menuCmd & CMD_ESC)
	{
		return true;
	}

	return false;
}
