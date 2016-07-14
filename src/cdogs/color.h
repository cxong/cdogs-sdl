/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.

    Copyright (c) 2013-2016, Cong Xu
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

#include "sys_specifics.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
} color_t;
extern color_t colorWhite;
extern color_t colorRed;
extern color_t colorGreen;
extern color_t colorBlue;
extern color_t colorPoison;
extern color_t colorBlack;
extern color_t colorDarker;
extern color_t colorPurple;
extern color_t colorGray;
extern color_t colorYellow;
extern color_t colorMagenta;
extern color_t colorCyan;
extern color_t colorFog;

extern color_t colorMaroon;
extern color_t colorLonestar;
extern color_t colorRusticRed;
extern color_t colorOfficeGreen;
extern color_t colorPakistanGreen;
extern color_t colorDarkFern;
extern color_t colorNavyBlue;
extern color_t colorArapawa;
extern color_t colorStratos;
extern color_t colorPatriarch;
extern color_t colorPompadour;
extern color_t colorLoulou;
extern color_t colorBattleshipGrey;
extern color_t colorDoveGray;
extern color_t colorGravel;
extern color_t colorComet;
extern color_t colorFiord;
extern color_t colorTuna;
extern color_t colorHacienda;
extern color_t colorKumera;
extern color_t colorHimalaya;
extern color_t colorChocolate;
extern color_t colorNutmeg;
extern color_t colorBracken;
extern color_t colorTeal;
extern color_t colorSkobeloff;
extern color_t colorDeepJungleGreen;

extern color_t colorSkin;
extern color_t colorDarkSkin;
extern color_t colorAsianSkin;

extern color_t colorLightBlue;

color_t ColorMult(color_t c, color_t m);
color_t ColorAlphaBlend(color_t a, color_t b);

typedef struct
{
	double h, s, v;
} HSV;
extern HSV tintNone;
extern HSV tintRed;
extern HSV tintYellow;
extern HSV tintGreen;
extern HSV tintCyan;
extern HSV tintPoison;
extern HSV tintGray;
extern HSV tintPurple;
extern HSV tintDarker;
// Multiply by HSV components; used for tinting
// h: hue, if >= 0 then the hue is forced to be this value
// s: saturation, where all RGB components are shifted towards the average value
// v: scale factor on the final components
color_t ColorTint(color_t c, HSV hsv);

bool ColorEquals(const color_t a, const color_t b);
bool HSVEquals(const HSV a, const HSV b);

// Convert hex string to color
color_t StrColor(const char *s);
// Convert colour to hex string
void ColorStr(char *s, const color_t c);
