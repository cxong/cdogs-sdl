/*
    Copyright (c) 2015, Cong Xu

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
#ifndef _SDL_joystickbuttonnames_h
#define _SDL_joystickbuttonnames_h

#include <SDL_gamecontroller.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \return -1 on error; 0 on success
 */
int SDLJBN_Init(void);

void SDLJBN_Quit(void);

/**
 *  Load a set of mappings from a file
 * 
 * \return number of mappings added, -1 on error
 */
int SDLJBN_AddMappingsFromFile(const char *file);

/**
 *  Load a set of mappings from a SDL_RWops object
 * 
 * \return number of mappings added, -1 on error
 */
int SDLJBN_AddMappingsFromRW(SDL_RWops *rw, int freerw);

/**
 * Get the button name and color for an SDL_GameControllerButton
 * Arguments can be set to NULL if they are not required
 * Use SDLJBN_GetError to get the error reason
 *
 * \return -1 on error; 0 on success
 */
int SDLJBN_GetButtonNameAndColor(SDL_Joystick *joystick,
                                 SDL_GameControllerButton button,
                                 char *s, Uint8 *r, Uint8 *g, Uint8 *b);
/**
 * Get the axis name and color for an SDL_GameControllerAxis
 * Arguments can be set to NULL if they are not required
 * Use SDLJBN_GetError to get the error reason
 *
 * \return -1 on error; 0 on success
 */
int SDLJBN_GetAxisNameAndColor(SDL_Joystick *joystick,
                               SDL_GameControllerAxis axis,
                               char *s, Uint8 *r, Uint8 *g, Uint8 *b);

/* Convenience macros */
#define SDLJBN_GetButtonName(joystick, button, s)\
	SDLJBN_GetButtonNameAndColor(joystick, button, s, NULL, NULL, NULL);
#define SDLJBN_GetButtonColor(joystick, button, r, g, b)\
	SDLJBN_GetButtonNameAndColor(joystick, button, NULL, r, g, b);
#define SDLJBN_GetAxisName(joystick, axis, s)\
	SDLJBN_GetAxisNameAndColor(joystick, axis, s, NULL, NULL, NULL);
#define SDLJBN_GetAxisColor(joystick, axis, r, g, b)\
	SDLJBN_GetAxisNameAndColor(joystick, axis, NULL, r, g, b);

const char *SDLJBN_GetError(void);

#ifdef __cplusplus
}
#endif

#endif
