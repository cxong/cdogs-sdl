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
#include "joystick.h"

#include <string.h>
#include <stdio.h>

#include "defs.h"


joysticks_t gJoysticks;

void JoyInit(joysticks_t *joys)
{
	memset(joys->joys, 0, sizeof(joys->joys));
	JoyReset(joys);
}

void JoyReset(joysticks_t *joys)
{
	int i;

	JoyTerminate(joys);

	printf("Checking for joysticks... ");
	joys->numJoys = SDL_NumJoysticks();
	if (joys->numJoys == 0)
	{
		printf("None found.\n");
		return;
	}
	printf("%d found\n", joys->numJoys);
	for (i = 0; i < joys->numJoys; i++)
	{
		joys->joys[i].j = SDL_JoystickOpen(i);

		if (joys->joys[i].j == NULL)
		{
			printf("Failed to open joystick.\n");
		}

		joys->joys[i].numButtons = SDL_JoystickNumButtons(joys->joys[i].j);
		joys->joys[i].numAxes = SDL_JoystickNumAxes(joys->joys[i].j);
		joys->joys[i].numHats = SDL_JoystickNumHats(joys->joys[i].j);

		printf("Opened Joystick %d\n", i);
		printf(" -> %s\n", SDL_JoystickName(i));
		printf(" -> Axes: %d Buttons: %d Hats: %d\n",
			joys->joys[i].numAxes, joys->joys[i].numButtons, joys->joys[i].numHats);
	}
	JoyPoll(joys);
}

void GJoyReset(void)
{
	JoyReset(&gJoysticks);
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

void JoyPollOne(joystick_t *joy)
{
	int i;
	joy->previousButtonsField = joy->currentButtonsField;
	joy->currentButtonsField = 0;

	// Get axes values, convert to direction
	for (i = 0; i < joy->numAxes; i += 2)
	{
		int x = SDL_JoystickGetAxis(joy->j, 0);
		int y = SDL_JoystickGetAxis(joy->j, 1);
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

	// Get hat state, convert to direction
	for (i = 0; i < joy->numHats; i++)
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
	for (i = 0; i < joy->numButtons; i++)
	{
		if (SDL_JoystickGetButton(joy->j, i))
		{
			joy->currentButtonsField |= CMD_BUTTON1 << i;
		}
	}
}

void JoyPoll(joysticks_t *joys)
{
	int i;
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

int JoyIsAnyPressed(joystick_t *joystick)
{
	return !!joystick->currentButtonsField;
}
