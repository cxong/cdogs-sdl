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
#include "events.h"

#include <assert.h>
#include <stdlib.h>

#include <SDL_timer.h>

#include "config.h"
#include "gamedata.h"
#include "log.h"
#include "music.h"
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
	KeyInit(&handlers->keyboard);
	JoyReset(&handlers->joysticks);
	MouseInit(
		&handlers->mouse, mouseCursor, mouseTrail, handlers->mouse.hideMouse);
}

void EventPoll(EventHandlers *handlers, Uint32 ticks)
{
	SDL_Event e;
	handlers->HasResolutionChanged = false;
	handlers->HasLostFocus = false;
	KeyPrePoll(&handlers->keyboard);
	MousePrePoll(&handlers->mouse);
	JoyPrePoll(&handlers->joysticks);
	SDL_free(handlers->DropFile);
	handlers->DropFile = NULL;
	// Don't process mouse events if focus just regained this cycle
	// This is to prevent bogus click events outside the window, e.g. in the
	// title bar
	bool regainedFocus = false;
	while (SDL_PollEvent(&e))
	{
		switch (e.type)
		{
		case SDL_KEYDOWN:
			if (e.key.repeat)
			{
				break;
			}
			KeyOnKeyDown(&handlers->keyboard, e.key.keysym);
			break;
		case SDL_KEYUP:
			KeyOnKeyUp(&handlers->keyboard, e.key.keysym);
			break;
		case SDL_TEXTINPUT:
			strcpy(handlers->keyboard.Typed, e.text.text);
			break;

		case SDL_CONTROLLERDEVICEADDED:
			{
				const SDL_JoystickID jid = JoyAdded(e.cdevice.which);
				if (jid == -1)
				{
					break;
				}
				// If there are players with unset devices,
				// set this controller to them
				CA_FOREACH(PlayerData, p, gPlayerDatas)
					if (p->inputDevice == INPUT_DEVICE_UNSET)
					{
						PlayerTrySetInputDevice(p, INPUT_DEVICE_JOYSTICK, jid);
						LOG(LM_INPUT, LL_INFO,
							"Joystick %d assigned to player %d", jid, p->UID);
						break;
					}
				CA_FOREACH_END()
			}
			break;

		case SDL_CONTROLLERDEVICEREMOVED:
			JoyRemoved(e.cdevice.which);
			// If there was a player using this joystick,
			// set their input device to nothing
			CA_FOREACH(PlayerData, p, gPlayerDatas)
				if (p->inputDevice == INPUT_DEVICE_JOYSTICK &&
					p->deviceIndex == e.cdevice.which)
				{
					PlayerTrySetInputDevice(p, INPUT_DEVICE_UNSET, 0);
					LOG(LM_INPUT, LL_WARN, "Joystick for player %d removed",
						p->UID);
					break;
				}
			CA_FOREACH_END()
			break;

		case SDL_CONTROLLERBUTTONDOWN:
			JoyOnButtonDown(e.cbutton);
			break;

		case SDL_CONTROLLERBUTTONUP:
			JoyOnButtonUp(e.cbutton);
			break;

		case SDL_CONTROLLERAXISMOTION:
			JoyOnAxis(e.caxis);
			break;

		case SDL_MOUSEBUTTONDOWN:
			if (regainedFocus) break;
			MouseOnButtonDown(&handlers->mouse, e.button.button);
			break;
		case SDL_MOUSEBUTTONUP:
			if (regainedFocus) break;
			MouseOnButtonUp(&handlers->mouse, e.button.button);
			break;
		case SDL_MOUSEWHEEL:
			if (regainedFocus) break;
			MouseOnWheel(&handlers->mouse, e.wheel.x, e.wheel.y);
			break;
		case SDL_WINDOWEVENT:
			switch (e.window.event)
			{
			case SDL_WINDOWEVENT_FOCUS_GAINED:
				regainedFocus = true;
				MusicSetPlaying(&gSoundDevice, true);
				break;
			case SDL_WINDOWEVENT_FOCUS_LOST:
				if (!gCampaign.IsClient && !ConfigGetBool(&gConfig, "StartServer"))
				{
					MusicSetPlaying(&gSoundDevice, false);
					handlers->HasLostFocus = true;
				}
				// Reset input handlers
				EventReset(
					handlers, handlers->mouse.cursor, handlers->mouse.trail);
				break;
			case SDL_WINDOWEVENT_SIZE_CHANGED:
				handlers->HasResolutionChanged = true;
				if (gGraphicsDevice.cachedConfig.IsEditor)
				{
					const int scale = ConfigGetInt(&gConfig, "Graphics.ScaleFactor");
					GraphicsConfigSet(
						&gGraphicsDevice.cachedConfig,
						Vec2iScaleDiv(
							Vec2iNew(e.window.data1, e.window.data2), scale),
						false,
						scale,
						gGraphicsDevice.cachedConfig.ScaleMode,
						gGraphicsDevice.cachedConfig.Brightness);
					GraphicsInitialize(&gGraphicsDevice);
				}
				break;
			default:
				// do nothing
				break;
			}
			break;
		case SDL_QUIT:
			handlers->HasQuit = true;
			break;
		case SDL_DROPFILE:
			handlers->DropFile = e.drop.file;
			break;
		default:
			break;
		}
	}
	KeyPostPoll(&handlers->keyboard, ticks);
	MousePostPoll(&handlers->mouse, ticks);
}

int GetKeyboardCmd(
	keyboard_t *keyboard, const int kbIndex, const bool isPressed)
{
	int cmd = 0;
	bool (*keyFunc)(const keyboard_t *, const int) =
		isPressed ? KeyIsPressed : KeyIsDown;
	const InputKeys *keys = &keyboard->PlayerKeys[kbIndex];

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
	bool (*mouseFunc)(const Mouse *, const int) =
		isPressed ? MouseIsPressed : MouseIsDown;

	if (useMouseMove)
	{
		cmd |= MouseGetMove(mouse, pos);
	}
	else
	{
		if (MouseWheel(mouse).y > 0)			cmd |= CMD_UP;
		else if (MouseWheel(mouse).y < 0)		cmd |= CMD_DOWN;
	}

	if (mouseFunc(mouse, SDL_BUTTON_LEFT))		cmd |= CMD_BUTTON1;
	if (mouseFunc(mouse, SDL_BUTTON_RIGHT))		cmd |= CMD_BUTTON2;
	if (mouseFunc(mouse, SDL_BUTTON_MIDDLE))	cmd |= CMD_MAP;

	return cmd;
}

static int GetJoystickCmd(const SDL_JoystickID id, bool isPressed)
{
	int cmd = 0;
	bool (*joyFunc)(const SDL_JoystickID, const int) =
		isPressed ? JoyIsPressed : JoyIsDown;

	if (joyFunc(id, CMD_LEFT))			cmd |= CMD_LEFT;
	else if (joyFunc(id, CMD_RIGHT))	cmd |= CMD_RIGHT;

	if (joyFunc(id, CMD_UP))			cmd |= CMD_UP;
	else if (joyFunc(id, CMD_DOWN))		cmd |= CMD_DOWN;

	if (joyFunc(id, CMD_BUTTON1))		cmd |= CMD_BUTTON1;

	if (joyFunc(id, CMD_BUTTON2))		cmd |= CMD_BUTTON2;

	if (joyFunc(id, CMD_MAP))			cmd |= CMD_MAP;

	if (joyFunc(id, CMD_ESC))			cmd |= CMD_ESC;

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
		cmd = GetJoystickCmd(playerData->deviceIndex, false);
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
		cmd = GetJoystickCmd(deviceIndex, isPressed);
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
	bool firstJoyPressedEsc = false;
	SDL_JoystickID firstJoyId = 0;
	if (handlers->joysticks.size > 0)
	{
		const Joystick *firstJoy = CArrayGet(&handlers->joysticks, 0);
		firstJoyId = firstJoy->id;
		firstJoyPressedEsc = JoyIsPressed(firstJoyId, CMD_ESC);
	}
	if (KeyIsPressed(kb, SDL_SCANCODE_ESCAPE) || firstJoyPressedEsc)
	{
		return CMD_ESC;
	}

	// Check first player keyboard
	int cmd = GetOnePlayerCmd(handlers, true, INPUT_DEVICE_KEYBOARD, 0);
	if (!cmd)
	{
		// Check keyboard
		if (KeyIsPressed(kb, SDL_SCANCODE_LEFT))		cmd |= CMD_LEFT;
		else if (KeyIsPressed(kb, SDL_SCANCODE_RIGHT))	cmd |= CMD_RIGHT;

		if (KeyIsPressed(kb, SDL_SCANCODE_UP))			cmd |= CMD_UP;
		else if (KeyIsPressed(kb, SDL_SCANCODE_DOWN))	cmd |= CMD_DOWN;

		if (KeyIsPressed(kb, SDL_SCANCODE_RETURN))		cmd |= CMD_BUTTON1;

		if (KeyIsPressed(kb, SDL_SCANCODE_BACKSPACE))	cmd |= CMD_BUTTON2;
	}
	if (!cmd && handlers->joysticks.size > 0)
	{
		// Check joystick 1
		cmd = GetOnePlayerCmd(
			handlers, true, INPUT_DEVICE_JOYSTICK, firstJoyId);
	}
	if (!cmd)
	{
		// Check mouse
		cmd = GetOnePlayerCmd(handlers, true, INPUT_DEVICE_MOUSE, 0);
	}

	return cmd;
}


void InputGetButtonNameColor(
	const input_device_e d, const int dIndex, const int cmd,
	char *buf, color_t *color)
{
	switch (d)
	{
	case INPUT_DEVICE_KEYBOARD:
	#ifdef __GCWZERO__
		if (color != NULL)
		{
			*color = colorBlue;
		}
		switch (cmd)
		{
			case CMD_LEFT: strcpy(buf, "left"); return;
			case CMD_RIGHT: strcpy(buf, "right"); return;
			case CMD_UP: strcpy(buf, "up"); return;
			case CMD_DOWN:strcpy(buf, "down"); return;
			case CMD_BUTTON1: strcpy(buf, "A"); return;
			case CMD_BUTTON2: strcpy(buf, "B"); return;
			case CMD_MAP: *color = colorGray; strcpy(buf, "L"); return;
			case CMD_ESC: strcpy(buf, "SELECT"); return;
			default: CASSERT(false, "unknown button"); return;
		}
	#else
		{
			const InputKeys *keys =
				&gEventHandlers.keyboard.PlayerKeys[dIndex];
			switch (cmd)
			{
			case CMD_LEFT: strcpy(buf, SDL_GetScancodeName(keys->left)); return;
			case CMD_RIGHT: strcpy(buf, SDL_GetScancodeName(keys->right)); return;
			case CMD_UP: strcpy(buf, SDL_GetScancodeName(keys->up)); return;
			case CMD_DOWN: strcpy(buf, SDL_GetScancodeName(keys->down)); return;
			case CMD_BUTTON1: strcpy(buf, SDL_GetScancodeName(keys->button1)); return;
			case CMD_BUTTON2: strcpy(buf, SDL_GetScancodeName(keys->button2)); return;
			case CMD_MAP: strcpy(buf, SDL_GetScancodeName(keys->map)); return;
			case CMD_ESC: strcpy(buf, SDL_GetScancodeName(SDL_SCANCODE_ESCAPE)); return;
			default: CASSERT(false, "unknown button"); return;
			}
		}
	#endif
		break;
	case INPUT_DEVICE_MOUSE:
		switch (cmd)
		{
		case CMD_LEFT: strcpy(buf, "left"); return;
		case CMD_RIGHT: strcpy(buf, "right"); return;
		case CMD_UP: strcpy(buf, "up"); return;
		case CMD_DOWN: strcpy(buf, "down"); return;
		case CMD_BUTTON1: strcpy(buf, "left click"); return;
		case CMD_BUTTON2: strcpy(buf, "right click"); return;
		case CMD_MAP: strcpy(buf, "middle click"); return;
		case CMD_ESC: strcpy(buf, ""); return;
		default: CASSERT(false, "unknown button"); return;
		}
		break;
	case INPUT_DEVICE_JOYSTICK:
		JoyButtonNameColor(dIndex, cmd, buf, color);
		return;
	case INPUT_DEVICE_AI:
		return;
	default:
		CASSERT(false, "unknown input device");
		return;
	}
}
void InputGetDirectionNames(
	char *buf, const input_device_e d, const int dIndex)
{
	strcpy(buf, "");
	switch (d)
	{
	case INPUT_DEVICE_KEYBOARD:
		{
			char left[256], right[256], up[256], down[256];
			InputGetButtonName(d, dIndex, CMD_LEFT, left);
			InputGetButtonName(d, dIndex, CMD_RIGHT, right),
			InputGetButtonName(d, dIndex, CMD_UP, up),
			InputGetButtonName(d, dIndex, CMD_DOWN, down);
			sprintf(buf, "%s, %s, %s, %s", left, right, up, down);
		}
		break;
	case INPUT_DEVICE_MOUSE:
		strcpy(buf, "mouse wheel");
		break;
	case INPUT_DEVICE_JOYSTICK:
		strcpy(buf, "directions");
		break;
	case INPUT_DEVICE_AI:
		break;
	case INPUT_DEVICE_UNSET:
		break;
	default:
		CASSERT(false, "unknown device");
		break;
	}
}

SDL_Scancode GetKey(EventHandlers *handlers)
{
	SDL_Scancode k = SDL_SCANCODE_UNKNOWN;
	do
	{
		EventPoll(handlers, SDL_GetTicks());
		k = KeyGetPressed(&handlers->keyboard);
		SDL_Delay(10);
	} while (k == SDL_SCANCODE_UNKNOWN);
	return k;
}

SDL_Scancode EventWaitKeyOrText(EventHandlers *handlers)
{
	SDL_Scancode k = SDL_SCANCODE_UNKNOWN;
	do
	{
		EventPoll(handlers, SDL_GetTicks());
		k = KeyGetPressed(&handlers->keyboard);
		SDL_Delay(10);
	} while (k == SDL_SCANCODE_UNKNOWN && handlers->keyboard.Typed[0] == '\0');
	return k;
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
	if (KeyIsPressed(&handlers->keyboard, SDL_SCANCODE_ESCAPE) ||
		handlers->HasQuit)
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
