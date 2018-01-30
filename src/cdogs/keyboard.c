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
#define DIAGONAL_RELEASE_DELAY 20

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

void DiagonalHold(keyboard_t *keyboard, int currentPlayer)
{
    //To change the level of delay, set DIAGONAL_RELEASE_DELAY up top to another number, if more than two players are ever allowed to map to a keyboard, expansion should be automatic when MAX_KEYBOARD_CONFIGS changes
    // ------
    int currentTicks = (int)SDL_GetTicks(); //Used to determine whether to keep holding a diagonal by comparing how long it has been since it set that diagonal's ticks vs current time
    //diagonal is held to give some leeway between releasing one half of it and the other so that it's easier to stop facing a diagonal
    
    const Uint8* realKeyboardState = SDL_GetKeyboardState(NULL); //This is the physical state of the keyboard untouched by any program manipulations, never manipulated, just compared against, eliminates a lot of bugs
    
    int cd = keyboard->diagonalState[currentPlayer].currentDiagonal;
    // cd isn't always initialized to something useful/useable but shouldn't be called before it has a useable value
    
    // -----
    KeyPress currentPlayerKeys[4] = {keyboard->currentKeys[keyboard->PlayerKeys[currentPlayer].up], keyboard->currentKeys[keyboard->PlayerKeys[currentPlayer].right], 
    keyboard->currentKeys[keyboard->PlayerKeys[currentPlayer].down], keyboard->currentKeys[keyboard->PlayerKeys[currentPlayer].left]}; 
    //Needed an array of keyboard directions arranged in a circle to be able to turn this into a simple loop
    
    int realCurrentPlayerKeys[4] = {realKeyboardState[keyboard->PlayerKeys[currentPlayer].up], realKeyboardState[keyboard->PlayerKeys[currentPlayer].right],
    realKeyboardState[keyboard->PlayerKeys[currentPlayer].down], realKeyboardState[keyboard->PlayerKeys[currentPlayer].left]};
    //Needed a similar array for the actual state of the keyboard so 'i' would always match between currentKeys and realKeyboard
    
    // ------
    //If an opposing direction is pressed to what is currently sustained, immediately end sustain, an attempt to make sure false positives are nipped in the bud
        if ((keyboard->diagonalState[currentPlayer].diagonalStatus == DIAGONAL_STATUS_SUSTAIN) && (currentPlayerKeys[(cd + 2) % 4].isPressed || (currentPlayerKeys[(cd + 3) % 4].isPressed)))
            {
                keyboard->diagonalState[currentPlayer].diagonalStatus = DIAGONAL_STATUS_UNPRESSED;
                keyboard->diagonalState[currentPlayer].diagonalTicks = (-1);
                currentPlayerKeys[cd].isPressed = realCurrentPlayerKeys[cd];
                currentPlayerKeys[(cd + 1) % 4].isPressed = realCurrentPlayerKeys[(cd + 1) % 4];
            } 

   
    // ------
    //Sets sustain if both buttons in a diagonal have been pressed but one is no longer pressed 
        if ((keyboard->diagonalState[currentPlayer].diagonalStatus == DIAGONAL_STATUS_PRESSED) && ((currentPlayerKeys[cd].isPressed || currentPlayerKeys[(cd + 1) % 4].isPressed)))
            {
                keyboard->diagonalState[currentPlayer].diagonalStatus = DIAGONAL_STATUS_SUSTAIN;
                keyboard->diagonalState[currentPlayer].diagonalTicks = currentTicks + DIAGONAL_RELEASE_DELAY;
            } 
    
    // -----
    //Sets the stage to change to sustain, hopefully avoids false positives by creating an intermediary state between not pressed and a diagonal being sustained
    for (int i = 0; i < 4; ++i) 
    {
        if ((keyboard->diagonalState[currentPlayer].diagonalStatus == DIAGONAL_STATUS_UNPRESSED) && (currentPlayerKeys[i].isPressed && currentPlayerKeys[(i + 1) % 4].isPressed)) 
            {
                keyboard->diagonalState[currentPlayer].currentDiagonal = i;
                keyboard->diagonalState[currentPlayer].diagonalStatus = DIAGONAL_STATUS_PRESSED;
                keyboard->diagonalState[currentPlayer].diagonalTicks = (-1);
            }
    }
    
    // -----
    //Keeps buttons pressed for DIAGONAL_RELEASE_DELAY (currently 20) milliseconds to help determine if a diagonal was intended
        if ((keyboard->diagonalState[currentPlayer].diagonalStatus == DIAGONAL_STATUS_SUSTAIN)  && (keyboard->diagonalState[currentPlayer].diagonalTicks > currentTicks))
        {
            currentPlayerKeys[cd].isPressed = true; 
            currentPlayerKeys[(cd + 1) % 4].isPressed = true;
        } 
    
    // -----
    //Ends a SUSTAIN, returns keyboard to neutral state, assigns both to real value to avoid causing a bug that makes one of the two cardinal directions used to immediately stop functioning
    //the first time after being pressed after a sustain
        if (((keyboard->diagonalState[currentPlayer].diagonalStatus == DIAGONAL_STATUS_SUSTAIN) && ((keyboard->diagonalState[currentPlayer].diagonalTicks <= currentTicks) 
        || ((!realCurrentPlayerKeys[cd]) && (!realCurrentPlayerKeys[(cd + 1) % 4])))))
            {
                keyboard->diagonalState[currentPlayer].diagonalStatus = DIAGONAL_STATUS_UNPRESSED;
                currentPlayerKeys[cd].isPressed = realCurrentPlayerKeys[cd];
                currentPlayerKeys[(cd + 1) % 4].isPressed = realCurrentPlayerKeys[(cd + 1) % 4];
            } 
    
    // -----
    keyboard->currentKeys[keyboard->PlayerKeys[currentPlayer].up].isPressed = currentPlayerKeys[0].isPressed;
    keyboard->currentKeys[keyboard->PlayerKeys[currentPlayer].right].isPressed = currentPlayerKeys[1].isPressed;
    keyboard->currentKeys[keyboard->PlayerKeys[currentPlayer].down].isPressed = currentPlayerKeys[2].isPressed;
    keyboard->currentKeys[keyboard->PlayerKeys[currentPlayer].left].isPressed = currentPlayerKeys[3].isPressed;
    //Propogates all changes to the keyboard state, when these are removed the function ceases to do anything.
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
    
    for (int currentPlayer = 0; currentPlayer < MAX_KEYBOARD_CONFIGS; ++currentPlayer)
        {
            //cycles through keys set to diagonals on each player to make them a bit stickier so diagonals are easier to land on
            DiagonalHold(keyboard, currentPlayer);
        }
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
