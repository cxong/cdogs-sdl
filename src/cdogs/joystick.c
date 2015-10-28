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
#include "joystick.h"

#include <string.h>
#include <stdio.h>

#include "defs.h"
#include "events.h"
#include "log.h"


void JoyInit(CArray *joys)
{
	CArrayInit(joys, sizeof(Joystick));

	// Detect all current controllers
	const int n = SDL_NumJoysticks();
	LOG(LM_INPUT, LL_DEBUG, "%d controllers found", n);
	if (n == 0)
	{
		return;
	}
	Joystick j;
	memset(&j, 0, sizeof j);
	for (int i = 0; i < n; i++)
	{
		if (!SDL_IsGameController(i))
		{
			continue;
		}

		j.gc = SDL_GameControllerOpen(i);
		if (j.gc == NULL)
		{
			LOG(LM_INPUT, LL_ERROR, "Failed to open game controller: %s",
				SDL_GetError());
			continue;
		}
		j.j = SDL_GameControllerGetJoystick(j.gc);
		if (j.j == NULL)
		{
			LOG(LM_INPUT, LL_ERROR, "Failed to open joystick: %s",
				SDL_GetError());
			continue;
		}
		j.id = SDL_JoystickInstanceID(j.j);
		if (j.id == -1)
		{
			LOG(LM_INPUT, LL_ERROR, "Failed to get joystick instance ID: %s",
				SDL_GetError());
			continue;
		}
		CArrayPushBack(joys, &j);
	}
}

void JoyTerminate(CArray *joys)
{
	CA_FOREACH(Joystick, j, *joys)
		SDL_GameControllerClose(j->gc);
	CA_FOREACH_END()
	CArrayTerminate(joys);
}

static Joystick *GetJoystick(const SDL_JoystickID id)
{
	CA_FOREACH(Joystick, j, gEventHandlers.joysticks)
		if (j->id == id) return j;
	CA_FOREACH_END()
	CASSERT(false, "Cannot find joystick");
	return NULL;
}

bool JoyIsDown(const SDL_JoystickID id, const int cmd)
{
	return !!(GetJoystick(id)->currentCmd & cmd);
}

bool JoyIsPressed(const SDL_JoystickID id, const int cmd)
{
	return JoyIsDown(id, cmd) && !(GetJoystick(id)->previousCmd & cmd);
}

int JoyGetPressed(const SDL_JoystickID id)
{
	const Joystick *j = GetJoystick(id);
	// Pressed is current and not previous, so take bitwise complement
	return j->currentCmd & ~j->previousCmd;
}

void JoyPrePoll(CArray *joys)
{
	CA_FOREACH(Joystick, j, *joys)
		j->pressedCmd = 0;
		j->previousCmd = j->currentCmd;
	CA_FOREACH_END()
}

void JoyAdded(const Sint32 which)
{
	LOG(LM_INPUT, LL_DEBUG, "Added joystick index %d", which);
	// TODO: implement
}
void JoyRemoved(const Sint32 which)
{
	LOG(LM_INPUT, LL_DEBUG, "Removed joystick index %d", which);
	// TODO: implement
}
int ControllerButtonToCmd(const Uint8 button);
void JoyOnButtonDown(const SDL_ControllerButtonEvent e)
{
	LOG(LM_INPUT, LL_DEBUG, "Joystick %d button down %d", e.which, e.button);
	Joystick *j = GetJoystick(e.which);
	j->currentCmd |= ControllerButtonToCmd(e.button);
}
void JoyOnButtonUp(const SDL_ControllerButtonEvent e)
{
	LOG(LM_INPUT, LL_DEBUG, "Joystick %d button up %d", e.which, e.button);
	Joystick *j = GetJoystick(e.which);
	const int cmd = ControllerButtonToCmd(e.button);
	if ((j->currentCmd & cmd) && !(j->previousCmd & cmd))
	{
		j->pressedCmd |= cmd;
	}
	j->currentCmd &= ~cmd;
}
int ControllerButtonToCmd(const Uint8 button)
{
	switch (button)
	{
	case SDL_CONTROLLER_BUTTON_A: return CMD_BUTTON1;
	case SDL_CONTROLLER_BUTTON_B: return CMD_BUTTON2;
	case SDL_CONTROLLER_BUTTON_BACK: return CMD_MAP;
	case SDL_CONTROLLER_BUTTON_START: return CMD_ESC;
	case SDL_CONTROLLER_BUTTON_DPAD_UP: return CMD_UP;
	case SDL_CONTROLLER_BUTTON_DPAD_DOWN: return CMD_DOWN;
	case SDL_CONTROLLER_BUTTON_DPAD_LEFT: return CMD_LEFT;
	case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: return CMD_RIGHT;
	default: return 0;
	}
	// TODO: check button mappings
}
void JoyOnAxis(const SDL_ControllerAxisEvent e)
{
	LOG(LM_INPUT, LL_DEBUG, "Joystick %d received axis %d, %d",
		e.which, e.axis, e.value);
	// TODO: implement
}

const char *JoyName(const SDL_JoystickID id)
{
	return SDL_GameControllerName(GetJoystick(id)->gc);
}

const char *JoyButtonNameColor(
	const SDL_JoystickID id, const int cmd, color_t *color)
{
	// TODO: implement
	UNUSED(id);
	UNUSED(cmd);
	*color = colorGray;
	return "FIZZ";
	/*
	switch (gEventHandlers.joysticks.joys[deviceIndex].Type)
	{
	case JOY_XBOX_360:
		switch (cmd)
		{
		case CMD_LEFT: return "left";
		case CMD_RIGHT: return "right";
		case CMD_UP: return "up";
		case CMD_DOWN: return "down";
		case CMD_BUTTON1: *color = colorGreen; return "A";
		case CMD_BUTTON2: *color = colorRed; return "B";
		case CMD_MAP: return "Back";
		case CMD_ESC: return "Start";
		default: CASSERT(false, "unknown button"); return NULL;
		}
	default:
		switch (cmd)
		{
		case CMD_LEFT: return "left";
		case CMD_RIGHT: return "right";
		case CMD_UP: return "up";
		case CMD_DOWN: return "down";
		case CMD_BUTTON1: return "button 1";
		case CMD_BUTTON2: return "button 2";
		case CMD_MAP: return "button 3";
		case CMD_ESC: return "button 4";
		default: CASSERT(false, "unknown button"); return NULL;
		}
	}*/
}
