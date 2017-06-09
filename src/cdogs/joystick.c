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
#include "SDL_JoystickButtonNames/SDL_joystickbuttonnames.h"


void JoyInit(CArray *joys)
{
	char buf[CDOGS_PATH_MAX];
	GetDataFilePath(buf, "data/gamecontrollerdb.txt");
	if (SDL_GameControllerAddMappingsFromFile(buf) == -1)
	{
		LOG(LM_INPUT, LL_ERROR, "cannot load controller mappings file: %s",
			SDL_GetError());
	}
	GetDataFilePath(buf, "data/gamecontrollerbuttondb.txt");
	if (SDLJBN_AddMappingsFromFile(buf) == -1)
	{
		LOG(LM_INPUT, LL_ERROR, "cannot load button mappings file: %s",
			SDLJBN_GetError());
	}

	CArrayInit(joys, sizeof(Joystick));

	// Detect all current controllers
	const int n = SDL_NumJoysticks();
	LOG(LM_INPUT, LL_INFO, "%d controllers found", n);
	if (n == 0)
	{
		return;
	}
	for (int i = 0; i < n; i++)
	{
		JoyAdded(i);
	}
}

void JoyReset(CArray *joys)
{
	CA_FOREACH(Joystick, j, *joys)
		j->currentCmd = j->pressedCmd = j->previousCmd = 0;
	CA_FOREACH_END()
}

static void JoyTerminateOne(Joystick *j)
{
	SDL_GameControllerClose(j->gc);
	SDL_HapticClose(j->haptic);
}

void JoyTerminate(CArray *joys)
{
	CA_FOREACH(Joystick, j, *joys)
		JoyTerminateOne(j);
	CA_FOREACH_END()
	CArrayTerminate(joys);
}

static Joystick *GetJoystick(const SDL_JoystickID id)
{
	CA_FOREACH(Joystick, j, gEventHandlers.joysticks)
		if (j->id == id) return j;
	CA_FOREACH_END()
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

SDL_JoystickID JoyAdded(const Sint32 which)
{
	if (!SDL_IsGameController(which))
	{
		return -1;
	}

	Joystick j;
	memset(&j, 0, sizeof j);
	j.hapticEffectId = -1;
	j.gc = SDL_GameControllerOpen(which);
	if (j.gc == NULL)
	{
		LOG(LM_INPUT, LL_ERROR, "Failed to open game controller: %s",
			SDL_GetError());
		return -1;
	}
	j.j = SDL_GameControllerGetJoystick(j.gc);
	if (j.j == NULL)
	{
		LOG(LM_INPUT, LL_ERROR, "Failed to open joystick: %s",
			SDL_GetError());
		return -1;
	}
	j.id = SDL_JoystickInstanceID(j.j);
	if (j.id == -1)
	{
		LOG(LM_INPUT, LL_ERROR, "Failed to get joystick instance ID: %s",
			SDL_GetError());
		return -1;
	}
	const int isHaptic = SDL_JoystickIsHaptic(j.j);
	if (isHaptic > 0)
	{
		j.haptic = SDL_HapticOpenFromJoystick(j.j);
		if (j.haptic == NULL)
		{
			LOG(LM_INPUT, LL_ERROR, "Failed to open haptic: %s",
				SDL_GetError());
		}
		else
		{
			if (SDL_HapticRumbleInit(j.haptic) != 0)
			{
				LOG(LM_INPUT, LL_ERROR, "Failed to init rumble: %s",
					SDL_GetError());
			}
			const int hapticQuery = SDL_HapticQuery(j.haptic);
			LOG(LM_INPUT, LL_INFO, "Haptic support: %x", hapticQuery);
			if (hapticQuery & SDL_HAPTIC_CONSTANT)
			{
				SDL_HapticEffect he;
				memset(&he, 0, sizeof he);
				he.type = SDL_HAPTIC_CONSTANT;
				he.constant.length = 100;
				he.constant.level = 20000; // 20000/32767 strength
				j.hapticEffectId = SDL_HapticNewEffect(j.haptic, &he);
				if (j.hapticEffectId == -1)
				{
					LOG(LM_INPUT, LL_ERROR,
						"Failed to create haptic effect: %s", SDL_GetError());
				}
			}
		}
	}
	else if (isHaptic < 0)
	{
		LOG(LM_INPUT, LL_ERROR, "Failed to query haptic: %s", SDL_GetError());
	}
	CArrayPushBack(&gEventHandlers.joysticks, &j);
	LOG(LM_INPUT, LL_INFO, "Added joystick index %d id %d", which, j.id);
	return j.id;
}
void JoyRemoved(const Sint32 which)
{
	LOG(LM_INPUT, LL_INFO, "Removed joystick id %d", which);
	CA_FOREACH(Joystick, j, gEventHandlers.joysticks)
		if (j->id == which)
		{
			JoyTerminateOne(j);
			CArrayDelete(&gEventHandlers.joysticks, _ca_index);
			return;
		}
	CA_FOREACH_END()
	CASSERT(false, "Cannot find joystick");
}

static void JoyOnCmd(Joystick *j, const int cmd, const bool isDown);

int ControllerButtonToCmd(const Uint8 button);
void JoyOnButtonDown(const SDL_ControllerButtonEvent e)
{
	LOG(LM_INPUT, LL_DEBUG, "Joystick %d button down %d", e.which, e.button);
	JoyOnCmd(GetJoystick(e.which), ControllerButtonToCmd(e.button), true);
}
void JoyOnButtonUp(const SDL_ControllerButtonEvent e)
{
	LOG(LM_INPUT, LL_DEBUG, "Joystick %d button up %d", e.which, e.button);
	JoyOnCmd(GetJoystick(e.which), ControllerButtonToCmd(e.button), false);
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
}
#define DEADZONE 16384
void JoyOnAxis(const SDL_ControllerAxisEvent e)
{
	LOG(LM_INPUT, LL_DEBUG, "Joystick %d received axis %d, %d",
		e.which, e.axis, e.value);
	Joystick *j = GetJoystick(e.which);
	switch (e.axis)
	{
	case SDL_CONTROLLER_AXIS_LEFTX:
		JoyOnCmd(j, CMD_LEFT, e.value < -DEADZONE);
		JoyOnCmd(j, CMD_RIGHT, e.value > DEADZONE);
		break;
	case SDL_CONTROLLER_AXIS_LEFTY:
		JoyOnCmd(j, CMD_UP, e.value < -DEADZONE);
		JoyOnCmd(j, CMD_DOWN, e.value > DEADZONE);
		break;
	case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
		// Right trigger fire
		JoyOnCmd(j, CMD_BUTTON1, e.value > DEADZONE);
		break;
	default:
		// Ignore axis
		break;
	}
}

static void JoyOnCmd(Joystick *j, const int cmd, const bool isDown)
{
	if (isDown)
	{
		j->currentCmd |= cmd;
	}
	else
	{
		if ((j->currentCmd & cmd) && !(j->previousCmd & cmd))
		{
			j->pressedCmd |= cmd;
		}
		j->currentCmd &= ~cmd;
	}
}

void JoyRumble(
	const SDL_JoystickID id, const float strength, const Uint32 length)
{
	Joystick *j = GetJoystick(id);
	if (j->haptic == NULL) return;
	if (SDL_HapticRumblePlay(j->haptic, strength, length) < 0)
	{
		LOG(LM_INPUT, LL_ERROR, "Failed to rumble: %s", SDL_GetError());
	}
}
void JoyImpact(const SDL_JoystickID id)
{
	Joystick *j = GetJoystick(id);
	if (j->hapticEffectId != -1 &&
		SDL_HapticRunEffect(j->haptic, j->hapticEffectId, 1) != 0)
	{
		LOG(LM_INPUT, LL_ERROR, "Failed to run haptic effect: %s",
			SDL_GetError());
	}
}

const char *JoyName(const SDL_JoystickID id)
{
	const Joystick *j = GetJoystick(id);
	const char *controllerName = SDL_GameControllerName(j->gc);
	const char *joystickName = SDL_JoystickName(j->j);
	// Try to use the more specific name
	if (joystickName != NULL &&
		strcmp(controllerName, "Generic DirectInput Controller") == 0)
	{
		return joystickName;
	}
	return controllerName;
}

static int CmdToControllerButton(const int cmd);
void JoyButtonNameColor(
	const SDL_JoystickID id, const int cmd, char *buf, color_t *color)
{
	Joystick *j = GetJoystick(id);
	const int button = CmdToControllerButton(cmd);
	int res;
	if (color != NULL)
	{
		res = SDLJBN_GetButtonNameAndColor(
			j->j, button, buf, &color->r, &color->g, &color->b);
	}
	else
	{
		res = SDLJBN_GetButtonName(j->j, button, buf);
	}
	if (res < 0)
	{
		LOG(LM_INPUT, LL_WARN, "Could not get button name/colour: %s",
			SDLJBN_GetError());
		strcpy(buf, "?");
	}
}
static int CmdToControllerButton(const int cmd)
{
	switch (cmd)
	{
	case CMD_BUTTON1: return SDL_CONTROLLER_BUTTON_A;
	case CMD_BUTTON2: return SDL_CONTROLLER_BUTTON_B;
	case CMD_MAP: return SDL_CONTROLLER_BUTTON_BACK;
	case CMD_ESC: return SDL_CONTROLLER_BUTTON_START;
	case CMD_UP: return SDL_CONTROLLER_BUTTON_DPAD_UP;
	case CMD_DOWN: return SDL_CONTROLLER_BUTTON_DPAD_DOWN;
	case CMD_LEFT: return SDL_CONTROLLER_BUTTON_DPAD_LEFT;
	case CMD_RIGHT: return SDL_CONTROLLER_BUTTON_DPAD_RIGHT;
	default: return SDL_CONTROLLER_BUTTON_INVALID;
	}
}
