/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.
	Copyright (c) 2013-2017, 2020-2021 Cong Xu
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

#include "joystick.h"
#include "keyboard.h"
#include "mouse.h"
#include "player.h"

typedef struct
{
	keyboard_t keyboard;
	CArray joysticks;
	Mouse mouse;
	// Filename received via drag and drop
	char *DropFile;

	bool HasResolutionChanged;
	bool HasQuit;
	bool HasLostFocus;
    int DemoQuitTimer;
} EventHandlers;

extern EventHandlers gEventHandlers;

void EventInit(EventHandlers *handlers);
void EventTerminate(EventHandlers *handlers);
void EventReset(EventHandlers *handlers);

void EventPoll(
	EventHandlers *handlers, const Uint32 ticks, int (*onEvent)(SDL_Event *));

int GetOnePlayerCmd(
	EventHandlers *handlers, const bool isPressed, const input_device_e device,
	const int deviceIndex);
int GetGameCmd(EventHandlers *handlers, const PlayerData *playerData);
int GetKeyboardCmd(
	keyboard_t *keyboard, const int kbIndex, const bool isPressed);
SDL_Scancode GetKey(EventHandlers *handlers);
// Wait until there is a key press or text input
SDL_Scancode EventWaitKeyOrText(EventHandlers *handlers);
typedef enum
{
	EVENT_WAIT_CONTINUE,
	EVENT_WAIT_OK,
	EVENT_WAIT_CANCEL
} EventWaitResult;
EventWaitResult EventWaitForAnyKeyOrButton(void);
void GetPlayerCmds(EventHandlers *handlers, int (*cmds)[MAX_LOCAL_PLAYERS]);
int GetMenuCmd(EventHandlers *handlers, const bool useMouse);
void InputGetButtonNameColor(
	const input_device_e d, const int dIndex, const int cmd, char *buf,
	color_t *color);
#define InputGetButtonName(_d, _dIndex, _cmd, _buf)                           \
	InputGetButtonNameColor(_d, _dIndex, _cmd, _buf, NULL)
// Return a string that shows the direction controls for an input device
void InputGetDirectionNames(
	char *buf, const input_device_e d, const int dIndex);
bool InputHasGrenadeButton(const input_device_e d, const int dIndex);

bool EventIsEscape(
	EventHandlers *handlers, const int cmds[MAX_LOCAL_PLAYERS],
	const int menuCmd);
