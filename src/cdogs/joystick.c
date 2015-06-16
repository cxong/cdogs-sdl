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


void JoyInit(joysticks_t *joys)
{
	memset(joys->joys, 0, sizeof(joys->joys));
	JoyReset(joys);
}

void GJoyReset(void *joys)
{
	JoyReset(joys);
}
void JoyReset(joysticks_t *joys)
{
	int i;

	JoyTerminate(joys);

	joys->numJoys = SDL_NumJoysticks();
	LOG(LM_INPUT, LL_DEBUG, "%d joysticks found", joys->numJoys);
	if (joys->numJoys == 0)
	{
		return;
	}
	for (i = 0; i < joys->numJoys; i++)
	{
		joystick_t *joy = &joys->joys[i];
		joy->j = SDL_JoystickOpen(i);
		if (joy->j == NULL)
		{
			printf("Failed to open joystick.\n");
			continue;
		}

		// Find joystick-specific fields
		const char *name = SDL_JoystickName(i);
		if (strstr(name, " 360 Controller") != NULL)
		{
			joy->Type = JOY_XBOX_360;
		#ifdef __APPLE__
			// TODO: OS X Yosemite driver doesn't detect hats
			// Find out if this is driver/SDL problem
			joy->Button1 = 11;	// A
			joy->Button2 = 12;	// B
			joy->ButtonMap = 5;	// back
			joy->ButtonEsc = 4;	// start
		#else
			joy->Button1 = 0;	// A
			joy->Button2 = 1;	// B
			joy->ButtonMap = 4;	// back
			joy->ButtonEsc = 5;	// start
		#endif
		}
		else
		{
			joy->Type = JOY_UNKNOWN;
			joy->Button1 = 0;
			joy->Button2 = 1;
			joy->ButtonMap = 2;
			joy->ButtonEsc = 3;
		}

		joy->numButtons = SDL_JoystickNumButtons(joy->j);
		joy->numAxes = SDL_JoystickNumAxes(joy->j);
		joy->numHats = SDL_JoystickNumHats(joy->j);

		printf("Opened Joystick %d\n", i);
		printf(" -> %s\n", JoyName(i));
		printf(" -> Axes: %d Buttons: %d Hats: %d\n",
			joy->numAxes, joy->numButtons, joy->numHats);
	}
	JoyPoll(joys);
}

void JoyTerminate(joysticks_t *joys)
{
	int i;
	for (i = 0; i < MAX_JOYSTICKS; i++)
	{
		if (joys->joys[i].j != NULL)
		{
			printf("Closing joystick.\n");
			SDL_JoystickClose(joys->joys[i].j);
			joys->joys[i].j = NULL;
		}
	}
}

#define JOY_AXIS_THRESHOLD	16384

static void GetAxis(joystick_t *joy);
void JoyPollOne(joystick_t *joy)
{
	joy->previousButtonsField = joy->currentButtonsField;
	joy->currentButtonsField = 0;

	GetAxis(joy);

	// Get hat state, convert to direction
	for (int i = 0; i < joy->numHats; i++)
	{
		Uint8 hat = SDL_JoystickGetHat(joy->j, i);
		switch (hat)
		{
		case SDL_HAT_UP:
			joy->currentButtonsField |= CMD_UP;
			break;
		case SDL_HAT_RIGHT:
			joy->currentButtonsField |= CMD_RIGHT;
			break;
		case SDL_HAT_DOWN:
			joy->currentButtonsField |= CMD_DOWN;
			break;
		case SDL_HAT_LEFT:
			joy->currentButtonsField |= CMD_LEFT;
			break;
		case SDL_HAT_RIGHTUP:
			joy->currentButtonsField |= CMD_RIGHT;
			joy->currentButtonsField |= CMD_UP;
			break;
		case SDL_HAT_RIGHTDOWN:
			joy->currentButtonsField |= CMD_RIGHT;
			joy->currentButtonsField |= CMD_DOWN;
			break;
		case SDL_HAT_LEFTUP:
			joy->currentButtonsField |= CMD_LEFT;
			joy->currentButtonsField |= CMD_UP;
			break;
		case SDL_HAT_LEFTDOWN:
			joy->currentButtonsField |= CMD_LEFT;
			joy->currentButtonsField |= CMD_DOWN;
			break;
		case SDL_HAT_CENTERED:
		default:
			break;
		}
	}

	// Get buttons
#define GET_BUTTON(_button, _cmd)\
	if (SDL_JoystickGetButton(joy->j, _button))\
	{\
		joy->currentButtonsField |= _cmd;\
	}
	GET_BUTTON(joy->Button1, CMD_BUTTON1);
	GET_BUTTON(joy->Button2, CMD_BUTTON2);
	GET_BUTTON(joy->ButtonMap, CMD_MAP);
	GET_BUTTON(joy->ButtonEsc, CMD_ESC);

	for (int i = 0; i < joy->numAxes; i++)
	{
		int x = SDL_JoystickGetAxis(joy->j, i);
		if (x < -JOY_AXIS_THRESHOLD || x > JOY_AXIS_THRESHOLD)
		{
			debug(D_NORMAL, "axis %d value %d\n", i, x);
		}
	}

	// Special controls
	switch (joy->Type)
	{
	case JOY_XBOX_360:
		// Right trigger fire
		{
			const int z = SDL_JoystickGetAxis(joy->j, 2);
			if (z < -JOY_AXIS_THRESHOLD)
			{
				joy->currentButtonsField |= CMD_BUTTON1;
			}
		}
		break;
	default:
		// do nothing
		break;
	}
}
static void GetAxis(joystick_t *joy)
{
	// Get axes values, convert to direction
	const int x = SDL_JoystickGetAxis(joy->j, 0);
	const int y = SDL_JoystickGetAxis(joy->j, 1);
	if (x < -JOY_AXIS_THRESHOLD)
	{
		joy->currentButtonsField |= CMD_LEFT;
	}
	else if (x > JOY_AXIS_THRESHOLD)
	{
		joy->currentButtonsField |= CMD_RIGHT;
	}
	if (y < -JOY_AXIS_THRESHOLD)
	{
		joy->currentButtonsField |= CMD_UP;
	}
	else if (y > JOY_AXIS_THRESHOLD)
	{
		joy->currentButtonsField |= CMD_DOWN;
	}
}

void JoyPoll(joysticks_t *joys)
{
	int i;
	if (joys->numJoys == 0)
	{
		return;
	}
	SDL_JoystickUpdate();
	for (i = 0; i < joys->numJoys; i++)
	{
		if (joys->joys[i].j != NULL)
		{
			JoyPollOne(&joys->joys[i]);
		}
	}
}

int JoyIsDown(joystick_t *joystick, int button)
{
	return !!(joystick->currentButtonsField & button);
}

int JoyIsPressed(joystick_t *joystick, int button)
{
	return JoyIsDown(joystick, button) && !(joystick->previousButtonsField & button);
}

int JoyGetPressed(joystick_t *joystick)
{
	int cmd = 0;
	int i;
	for (i = 0; i < joystick->numButtons; i++)
	{
		int mask = 1 << i;
		if (JoyIsPressed(joystick, mask))
		{
			cmd |= mask;
		}
	}
	return cmd;
}

const char *JoyName(const int deviceIndex)
{
	switch (gEventHandlers.joysticks.joys[deviceIndex].Type)
	{
	case JOY_XBOX_360:
		return "Xbox 360 controller";
	default:
		return SDL_JoystickName(deviceIndex);
	}
}

const char *JoyButtonNameColor(
	const int deviceIndex, const int cmd, color_t *color)
{
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
	}
}
