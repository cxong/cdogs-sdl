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


keyboard_t gKeyboard;

#define KEYBOARD_REPEAT_TICKS 100

void KeyInit(keyboard_t *keyboard)
{
	memset(keyboard->currentKeys, 0, sizeof keyboard->currentKeys);
	memset(keyboard->previousKeys, 0, sizeof keyboard->previousKeys);
	memset(keyboard->pressedKeys, 0, sizeof keyboard->pressedKeys);
	keyboard->modState = KMOD_NONE;
	keyboard->ticks = 0;
	keyboard->repeatedTicks;
}

void KeyPoll(keyboard_t *keyboard, Uint32 ticks)
{
	int areSameKeysDown = memcmp(
		keyboard->previousKeys,
		keyboard->currentKeys,
		sizeof keyboard->previousKeys) == 0;
	memcpy(
		keyboard->previousKeys,
		keyboard->currentKeys,
		sizeof keyboard->previousKeys);
	while (SDL_PollEvent(&keyboard->keyevent))
	{
		switch(keyboard->keyevent.type)
		{
		case SDL_KEYDOWN:
			keyboard->currentKeys[keyboard->keyevent.key.keysym.sym] = 1;
			break;
		case SDL_KEYUP:
			keyboard->currentKeys[keyboard->keyevent.key.keysym.sym] = 0;
		default:
			break;
		}
	}
	keyboard->modState = SDL_GetModState();

	// If same keys have been pressed, remember how long they have been pressed
	if (areSameKeysDown)
	{
		Uint32 ticksElapsed = ticks - keyboard->ticks;
		keyboard->repeatedTicks += ticksElapsed;
	}
	else
	{
		keyboard->repeatedTicks = 0;
	}
	// If more time has elapsed, forget about previous keys for repeating
	if (keyboard->repeatedTicks > KEYBOARD_REPEAT_TICKS)
	{
		keyboard->repeatedTicks -= KEYBOARD_REPEAT_TICKS;
		memcpy(
			keyboard->pressedKeys,
			keyboard->currentKeys,
			sizeof keyboard->pressedKeys);
	}
	else
	{
		int i;
		for (i = 0; i < 512; i++)
		{
			keyboard->pressedKeys[i] =
				keyboard->currentKeys[i] && !keyboard->previousKeys[i];
		}
	}
	keyboard->ticks = ticks;
}

int KeyIsDown(keyboard_t *keyboard, int key)
{
	return keyboard->currentKeys[key];
}

int KeyIsPressed(keyboard_t *keyboard, int key)
{
	return keyboard->pressedKeys[key];
}

int KeyIsReleased(keyboard_t *keyboard, int key)
{
	return !KeyIsDown(keyboard, key) && keyboard->previousKeys[key];
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
