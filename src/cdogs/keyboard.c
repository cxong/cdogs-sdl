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
#include "keyboard.h"

#include <string.h>

#include "log.h"
#include "utils.h"


const char *KeycodeStr(int k)
{
	switch (k)
	{
		T2S(KEY_CODE_LEFT, "left");
		T2S(KEY_CODE_RIGHT, "right");
		T2S(KEY_CODE_UP, "up");
		T2S(KEY_CODE_DOWN, "down");
		T2S(KEY_CODE_BUTTON1, "button1");
		T2S(KEY_CODE_BUTTON2, "button2");
		T2S(KEY_CODE_MAP, "map");
	default:
		return "";
	}
}

#define KEYBOARD_REPEAT_DELAY 500
#define KEYBOARD_REPEAT_TICKS 100
#define DIAGONAL_RELEASE_DELAY 10000

void KeyInit(keyboard_t *keyboard)
{
	memset(keyboard, 0, sizeof *keyboard);
	keyboard->modState = KMOD_NONE;
	keyboard->ticks = 0;
	keyboard->repeatedTicks = 0;
	keyboard->isFirstRepeat = 1;
	for (int i = 0; i < MAX_KEYBOARD_CONFIGS; i++)
	{
		char buf[256];
		sprintf(buf, "Input.PlayerCodes%d", i);
		keyboard->PlayerKeys[i] = KeyLoadPlayerKeys(ConfigGet(&gConfig, buf));
	}
}
InputKeys KeyLoadPlayerKeys(Config *c)
{
	InputKeys k;
	k.left = (SDL_Scancode)ConfigGetInt(c, "left");
	k.right = (SDL_Scancode)ConfigGetInt(c, "right");
	k.up = (SDL_Scancode)ConfigGetInt(c, "up");
	k.down = (SDL_Scancode)ConfigGetInt(c, "down");
	k.button1 = (SDL_Scancode)ConfigGetInt(c, "button1");
	k.button2 = (SDL_Scancode)ConfigGetInt(c, "button2");
	k.map = (SDL_Scancode)ConfigGetInt(c, "map");
	return k;
}

void KeyPrePoll(keyboard_t *keyboard)
{
	memcpy(
		keyboard->previousKeys,
		keyboard->currentKeys,
		sizeof keyboard->previousKeys);
	keyboard->modState = SDL_GetModState();
	keyboard->Typed[0] = '\0';
}

void KeyOnKeyDown(keyboard_t *keyboard, const SDL_Keysym s)
{
	keyboard->currentKeys[s.scancode].isPressed = true;
	if (s.sym >= (SDL_Keycode)' ' && s.sym <= (SDL_Keycode)'z')
	{
		keyboard->currentKeys[s.scancode].keycode = s.sym;
	}
	else
	{
		keyboard->currentKeys[s.scancode].keycode = 0;
	}
}
void KeyOnKeyUp(keyboard_t *keyboard, const SDL_Keysym s)
{
	keyboard->currentKeys[s.scancode].isPressed = 0;
}

/*void DiagonalHold(keyboard_t *keyboard)
{
    if (keyboard->currentKeys[keyboard->PlayerKeys->up].isPressed == true && keyboard->currentKeys[keyboard->PlayerKeys->left].isPressed == true) 
    {
        keyboard->upDiagonalTicks = (int)SDL_GetTicks() + DIAGONAL_RELEASE_DELAY;
        keyboard->leftDiagonalTicks = (int)SDL_GetTicks() + DIAGONAL_RELEASE_DELAY;
    } 
    int difTicks = keyboard->upDiagonalTicks - (int)SDL_GetTicks();
    if (difTicks > 0) 
    {
        //keyboard->currentKeys[SDL_SCANCODE_UP].isPressed = true;
        keyboard->currentKeys[keyboard->PlayerKeys->up].isPressed = true;
    }
    
    if (keyboard->upDiagonalTicks > 0 && keyboard->currentKeys[SDL_SCANCODE_DOWN].isPressed == true)
    {
        keyboard->upDiagonalTicks = 0;
    }
    if (difTicks <= 0)
    {
        //keyboard->currentKeys[SDL_SCANCODE_UP].isPressed = false;
        //difTicks = 0;
        keyboard->currentKeys[keyboard->PlayerKeys->up].isPressed = false;
        
    }
    //fprintf(stderr, "%d %d\n", difTicks, keyboard->currentKeys[SDL_SCANCODE_UP].isPressed);
}*/

void DiagonalHold(keyboard_t *keyboard)
{
    int currentTicks = (int)SDL_GetTicks(); //Everything uses this to determine whether to keep holding a diagonal by comparing how long it has been since it set that diagonal's ticks vs current time
    
    if ((keyboard->currentKeys[keyboard->PlayerKeys->up].isPressed == true && keyboard->currentKeys[keyboard->PlayerKeys->left].isPressed == true) && ((keyboard->upLeftDiagonal == UNPRESSED) || (keyboard->upLeftDiagonal == 0)))
    {
        keyboard->upLeftDiagonal = PRESSED;
        keyboard->upDiagonalTicks = (-1);
        keyboard->leftDiagonalTicks = (-1);
    } //A basic "make sure everything is ready to be set to SUSTAIN barring anything else changing"
    
    if ((keyboard->previousKeys[keyboard->PlayerKeys->up].isPressed == true && keyboard->previousKeys[keyboard->PlayerKeys->left].isPressed == true) 
        && (keyboard->currentKeys[keyboard->PlayerKeys->up].isPressed == false && keyboard->currentKeys[keyboard->PlayerKeys->left].isPressed == false)
        && (keyboard->upLeftDiagonal != SUSTAIN))
        {
            keyboard->upLeftDiagonal = SUSTAIN;
            keyboard->upDiagonalTicks = (int)SDL_GetTicks() + DIAGONAL_RELEASE_DELAY;
            keyboard->leftDiagonalTicks = (int)SDL_GetTicks() + DIAGONAL_RELEASE_DELAY;
        } //Sets to SUSTAIN, makes sure that it only does so when the buttons have actually released
        
    if ((keyboard->upLeftDiagonal == SUSTAIN) && ((keyboard->upDiagonalTicks - keyboard->leftDiagonalTicks < 30) && (keyboard->upDiagonalTicks - keyboard->leftDiagonalTicks > 0)))
    {
        keyboard->leftDiagonalTicks = keyboard->upDiagonalTicks;
    }
    
    if ((keyboard->upLeftDiagonal == SUSTAIN) && ((keyboard->leftDiagonalTicks - keyboard->upDiagonalTicks < 30) && (keyboard->leftDiagonalTicks - keyboard->upDiagonalTicks > 0)))
    {
        keyboard->upDiagonalTicks = keyboard->leftDiagonalTicks;
    } //Allows things within a range to exit pressed/sustain status at the same time, otherwise the problem is just kicked down the road
    
    if ((keyboard->upDiagonalTicks > currentTicks) && (keyboard->leftDiagonalTicks > currentTicks) && (keyboard->upLeftDiagonal == SUSTAIN))
    {
        keyboard->currentKeys[keyboard->PlayerKeys->up].isPressed = true; 
        keyboard->currentKeys[keyboard->PlayerKeys->left].isPressed = true;
    } //Keeps buttons pressed for x milliseconds to help determine if a diagonal was intended
    
    if ((keyboard->upLeftDiagonal == SUSTAIN) && ((keyboard->upDiagonalTicks <= currentTicks) && (keyboard->leftDiagonalTicks <= currentTicks)))
    {
        keyboard->upLeftDiagonal = UNPRESSED;
        keyboard->currentKeys[keyboard->PlayerKeys->up].isPressed = false;
        keyboard->currentKeys[keyboard->PlayerKeys->left].isPressed = false;
    } //Ends a SUSTAIN, returns keyboard to neutral state
    
    if ((keyboard->currentKeys[keyboard->PlayerKeys->up].isPressed == true && keyboard->currentKeys[keyboard->PlayerKeys->left].isPressed == true) && ((keyboard->upLeftDiagonal == UNPRESSED) || (keyboard->upLeftDiagonal == 0))
    && (downLeftDiagonal == SSUSTAIN || upRightDiagonal == SUSTAIN || downRightDiagonal == SUSTAIN))
    {
        downLeftDiagonal = UNPRESSED;
        upRightDiagonal = UNPRESSED;
        downRightDiagonal = UNPRESSED;
        keyboard->upDiagonalTicks = (-1);
        keyboard->leftDiagonalTicks = (-1);
        keyboard->rightDiagonalTicks = (-1);
        keyboard->downDiagonalTicks = (-1);
    } //A check to make sure that sustain status is never in place for multiple directions, probably a shorter less redundant way to do this but story of this function's hideous life

    //fprintf(stderr, "%d\n", keyboard->upLeftDiagonal);
    //fprintf(stderr, "%d\n", keyboard->upDiagonalTicks);
}

void KeyPostPoll(keyboard_t *keyboard, Uint32 ticks)
{
	int isRepeating = 0;
	int areSameKeysPressed = 1;
	for (int i = 0; i < SDL_NUM_SCANCODES; i++)
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
		keyboard->isFirstRepeat = false;
		// Ignore the keys that tend to stay pressed/unpressed
		// i.e. lock keys
		keyboard->pressedKeys[SDL_SCANCODE_NUMLOCKCLEAR].isPressed = false;
		keyboard->pressedKeys[SDL_SCANCODE_CAPSLOCK].isPressed = false;
		keyboard->pressedKeys[SDL_SCANCODE_SCROLLLOCK].isPressed = false;
	}
	else
	{
		for (int i = 0; i < SDL_NUM_SCANCODES; i++)
		{
			keyboard->pressedKeys[i].isPressed =
				keyboard->currentKeys[i].isPressed &&
				!keyboard->previousKeys[i].isPressed;
		}
	}
	keyboard->ticks = ticks;
    
    DiagonalHold(keyboard);
    
    //fprintf(stderr, "%d\n", keyboard->currentKeys[SDL_SCANCODE_UP].isPressed);
}

bool KeyIsDown(const keyboard_t *k, const int key)
{
	return k->currentKeys[key].isPressed;
}

bool KeyIsPressed(const keyboard_t *k, const int key)
{
	return k->pressedKeys[key].isPressed;
}

bool KeyIsReleased(const keyboard_t *k, const int key)
{
	return !KeyIsDown(k, key) && k->previousKeys[key].isPressed;
}

SDL_Scancode KeyGetPressed(const keyboard_t *k)
{
	for (SDL_Scancode i = 0; i < SDL_NUM_SCANCODES; i++)
	{
		if (KeyIsPressed(k, i))
		{
			return i;
		}
	}
	return 0;
}

SDL_Scancode KeyGet(const InputKeys *keys, const key_code_e keyCode)
{
	switch (keyCode)
	{
	case KEY_CODE_LEFT:
		return keys->left;
	case KEY_CODE_RIGHT:
		return keys->right;
	case KEY_CODE_UP:
		return keys->up;
	case KEY_CODE_DOWN:
		return keys->down;
	case KEY_CODE_BUTTON1:
		return keys->button1;
	case KEY_CODE_BUTTON2:
		return keys->button2;
	case KEY_CODE_MAP:
		return keys->map;
	default:
		LOG(LM_MAIN, LL_ERROR, "Unhandled key code %d\n", (int)keyCode);
		return SDL_SCANCODE_UNKNOWN;
	}
}
