/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.

    Copyright (c) 2013-2014, Cong Xu
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

#ifdef __MINGW32__
// MinGW time bug
// http://stackoverflow.com/questions/21015113/difftime-returns-incorrect-value-in-mingw
#define _USE_32BIT_TIME_T 1
#endif
#include <time.h>

#include <cdogs/c_array.h>
#include <cdogs/color.h>


/*
credits_displayer_t is a module that loads a number of credit_t entries from a
CREDITS file, and displays each for a fixed amount of time.
This is used to show the credits (name + description) in the game's main menu.
The credit entries are in a file with alternating lines of name and
description, as so:

<name1>
<description1>
<name2>
<description2>

Empty lines are ignored
For more details see /doc/CREDITS
*/

typedef struct
{
	char *name;
	char *message;
} credit_t;

typedef struct
{
	CArray credits;	// of credit_t
	time_t lastUpdateTime;
	int creditsIndex;
	color_t nameColor;
	color_t textColor;
} credits_displayer_t;

void LoadCredits(
	credits_displayer_t *displayer, color_t nameColor, color_t textColor);
void UnloadCredits(credits_displayer_t *displayer);
void ShowCredits(credits_displayer_t *displayer);
