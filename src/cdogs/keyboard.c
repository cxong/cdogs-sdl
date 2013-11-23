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
#include "keyboard.h"

#include <string.h>

#include "utils.h"


#define KEYBOARD_REPEAT_DELAY 500
#define KEYBOARD_REPEAT_TICKS 100

void KeyInit(keyboard_t *keyboard)
{
	memset(keyboard, 0, sizeof *keyboard);
	keyboard->modState = KMOD_NONE;
	keyboard->ticks = 0;
	keyboard->repeatedTicks = 0;
	keyboard->isFirstRepeat = 1;
}

void KeyPrePoll(keyboard_t *keyboard)
{
	memcpy(
		keyboard->previousKeys,
		keyboard->currentKeys,
		sizeof keyboard->previousKeys);
	keyboard->modState = SDL_GetModState();
}

void KeyOnKeyDown(keyboard_t *keyboard, SDL_keysym s)
{
	keyboard->currentKeys[s.sym].isPressed = 1;
	if (s.unicode >= (Uint16)' ' && s.unicode <= (Uint16)'~')
	{
		keyboard->currentKeys[s.sym].unicode = s.unicode;
	}
	else
	{
		keyboard->currentKeys[s.sym].unicode = 0;
	}
	memmove(
		keyboard->pressedKeysBuffer + 1, keyboard->pressedKeysBuffer,
		sizeof *keyboard->pressedKeysBuffer * (8 - 1));
	keyboard->pressedKeysBuffer[0] = s.sym;
}
void KeyOnKeyUp(keyboard_t *keyboard, SDL_keysym s)
{
	keyboard->currentKeys[s.sym].isPressed = 0;
}

void KeyPostPoll(keyboard_t *keyboard, Uint32 ticks)
{
	int isRepeating = 0;
	int areSameKeysPressed = 1;
	int i;
	for (i = 0; i < 512; i++)
	{
		if (keyboard->previousKeys[i].isPressed ^
			keyboard->currentKeys[i].isPressed)
		{
			areSameKeysPressed = 0;
			break;
		}
	}
	// If same keys have been pressed, remember how long they have been pressed
	if (areSameKeysPressed)
	{
		Uint32 ticksElapsed = ticks - keyboard->ticks;
		keyboard->repeatedTicks += ticksElapsed;
	}
	else
	{
		keyboard->repeatedTicks = 0;
		keyboard->isFirstRepeat = 1;
	}
	// If more time has elapsed, forget about previous keys for repeating
	if (keyboard->repeatedTicks > KEYBOARD_REPEAT_DELAY &&
		keyboard->isFirstRepeat)
	{
		keyboard->repeatedTicks -= KEYBOARD_REPEAT_DELAY;
		isRepeating = 1;
	}
	else if (keyboard->repeatedTicks > KEYBOARD_REPEAT_TICKS &&
		!keyboard->isFirstRepeat)
	{
		keyboard->repeatedTicks -= KEYBOARD_REPEAT_TICKS;
		isRepeating = 1;
	}
	if (isRepeating)
	{
		memcpy(
			keyboard->pressedKeys,
			keyboard->currentKeys,
			sizeof keyboard->pressedKeys);
		keyboard->isFirstRepeat = 0;
	}
	else
	{
		for (i = 0; i < 512; i++)
		{
			keyboard->pressedKeys[i].isPressed =
				keyboard->currentKeys[i].isPressed &&
				!keyboard->previousKeys[i].isPressed;
		}
	}
	keyboard->ticks = ticks;
}

int KeyIsDown(keyboard_t *keyboard, int key)
{
	return keyboard->currentKeys[key].isPressed;
}

int KeyIsPressed(keyboard_t *keyboard, int key)
{
	return keyboard->pressedKeys[key].isPressed;
}

int KeyIsReleased(keyboard_t *keyboard, int key)
{
	return !KeyIsDown(keyboard, key) && keyboard->previousKeys[key].isPressed;
}

int KeyGetPressed(keyboard_t *keyboard)
{
	int i;
	for (i = 0; i < 512; i++)
	{
		if (KeyIsPressed(keyboard, i))
		{
			return i;
		}
	}
	return 0;
}

int KeyGetTyped(keyboard_t *keyboard)
{
	int i;
	for (i = 0; i < 128; i++)
	{
		Uint16 unicode = keyboard->currentKeys[i].unicode;
		if (KeyIsPressed(keyboard, i) &&
			unicode >= (Uint16)' ' &&
			unicode <= (Uint16)'~')
		{
			return unicode;
		}
	}
	return 0;
}
