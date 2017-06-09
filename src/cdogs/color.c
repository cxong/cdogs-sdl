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
#include "color.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"

color_t colorWhite = { 255, 255, 255, 255 };
color_t colorRed = { 255, 0, 0, 255 };
color_t colorGreen = { 0, 255, 0, 255 };
color_t colorBlue = { 0, 0, 255, 255 };
color_t colorPoison = { 64, 192, 64, 255 };
color_t colorBlack = { 0, 0, 0, 255 };
color_t colorDarker = { 192, 192, 192, 255 };
color_t colorPurple = { 192, 0, 192, 255 };
color_t colorGray = { 128, 128, 128, 255 };
color_t colorYellow = { 255, 255, 128, 255 };
color_t colorMagenta = { 255, 0, 255, 255 };
color_t colorCyan = { 0, 255, 255, 255 };
color_t colorFog = { 96, 96, 96, 255 };

color_t colorMaroon = { 0x84, 0, 0, 255 };
color_t colorLonestar = { 0x70, 0, 0, 255 };
color_t colorRusticRed = { 0x48, 0, 0, 255 };
color_t colorOfficeGreen = { 0, 0x84, 0, 255 };
color_t colorPakistanGreen = { 0, 0x70, 0, 255 };
color_t colorDarkFern = { 0, 0x48, 0, 255 };
color_t colorNavyBlue = { 0, 0, 0x84, 255 };
color_t colorArapawa = { 0, 0, 0x70, 255 };
color_t colorStratos = { 0, 0, 0x48, 255 };
color_t colorPatriarch = { 0x84, 0, 0x84, 255 };
color_t colorPompadour = { 0x70, 0, 0x70, 255 };
color_t colorLoulou = { 0x48, 0, 0x48, 255 };
color_t colorBattleshipGrey = { 0x84, 0x84, 0x84, 255 };
color_t colorDoveGray = { 0x70, 0x70, 0x70, 255 };
color_t colorGravel = { 0x48, 0x48, 0x48, 255 };
color_t colorComet = { 0x5C, 0x5C, 0x84, 255 };
color_t colorFiord = { 0x48, 0x48, 0x70, 255 };
color_t colorTuna = { 0x34, 0x34, 0x48, 255 };
color_t colorHacienda = { 0x94, 0x80, 0x2C, 255 };
color_t colorKumera = { 0x84, 0x70, 0x24, 255 };
color_t colorHimalaya = { 0x74, 0x60, 0x1C, 255 };
color_t colorChocolate = { 0x84, 0x44, 0, 255 };
color_t colorNutmeg = { 0x70, 0x38, 0, 255 };
color_t colorBracken = { 0x48, 0x24, 0, 255 };
color_t colorTeal = { 0, 0x84, 0x84, 255 };
color_t colorSkobeloff = { 0, 0x70, 0x70, 255 };
color_t colorDeepJungleGreen = { 0, 0x48, 0x48, 255 };

color_t colorSkin = { 0xF4, 0x94, 0x4C, 255 };
color_t colorDarkSkin = { 0x93, 0x5D, 0x37, 255 };
color_t colorAsianSkin = { 0xFF, 0xCC, 0x99, 255 };

color_t colorLightBlue = { 80, 80, 160, 255 };

color_t ColorMult(color_t c, color_t m)
{
	c.r = (uint8_t)((int)c.r * m.r / 255);
	c.g = (uint8_t)((int)c.g * m.g / 255);
	c.b = (uint8_t)((int)c.b * m.b / 255);
	return c;
}
color_t ColorAlphaBlend(color_t a, color_t b)
{
	a.r = (uint8_t)(((int)a.r*(255 - b.a) + (int)b.r*b.a)/255);
	a.g = (uint8_t)(((int)a.g*(255 - b.a) + (int)b.g*b.a)/255);
	a.b = (uint8_t)(((int)a.b*(255 - b.a) + (int)b.b*b.a)/255);
	a.a = 255;
	return a;
}

HSV tintNone = { -1.0, 1.0, 1.0 };
HSV tintRed = { 0.0, 1.0, 1.0 };
HSV tintYellow = { 60.0, 1.0, 1.0 };
HSV tintGreen = { 120.0, 1.0, 1.0 };
HSV tintCyan = { 180.0, 1.0, 1.0 };
HSV tintPoison = { 120.0, 0.33, 2.0 };
HSV tintGray = { -1.0, 0.0, 1.0 };
HSV tintPurple = { 300, 1.0, 1.0 };
HSV tintDarker = { -1.0, 1.0, 0.75 };

color_t ColorTint(color_t c, HSV hsv)
{
	// Adapted from answer by David H
	// http://stackoverflow.com/a/6930407/2038264
	// License: http://creativecommons.org/licenses/by-sa/3.0/
	// Author profile: http://stackoverflow.com/users/1633251/david-h
	color_t out;
	int vAvg = ((int)c.r + c.g + c.b) / 3;
	uint8_t vComponent = (uint8_t)CLAMP(hsv.v * vAvg, 0, 255);

	if (hsv.s <= 0.0)
	{
		// No saturation; just gray
		out.r = vComponent;
		out.g = vComponent;
		out.b = vComponent;
	}
	else if (hsv.h >= 0)
	{
		// set hue to h; use regular HSV to RGB conversion
		double ff;
		uint8_t p, q, t;
		long i;
		double hh = hsv.h;
		if (hh >= 360.0)
		{
			hh = 0.0;
		}
		hh /= 60.0;
		i = (long)hh;
		ff = hh - i;
		p = (uint8_t)CLAMP(vComponent * (1.0 - hsv.s), 0, 255);
		q = (uint8_t)CLAMP(vComponent * (1.0 - (hsv.s * ff)), 0, 255);
		t = (uint8_t)CLAMP(vComponent * (1.0 - (hsv.s * (1.0 - ff))), 0, 255);

		switch(i)
		{
		case 0:
			out.r = vComponent;
			out.g = t;
			out.b = p;
			break;
		case 1:
			out.r = q;
			out.g = vComponent;
			out.b = p;
			break;
		case 2:
			out.r = p;
			out.g = vComponent;
			out.b = t;
			break;

		case 3:
			out.r = p;
			out.g = q;
			out.b = vComponent;
			break;
		case 4:
			out.r = t;
			out.g = p;
			out.b = vComponent;
			break;
		case 5:
		default:
			out.r = vComponent;
			out.g = p;
			out.b = q;
			break;
		}
	}
	else
	{
		// Just set saturation and value
		// Use weighted average to shift components towards grey for saturation
		out.r = (uint8_t)CLAMP(hsv.v * (vAvg*(1.0-hsv.s) + hsv.s*c.r), 0, 255);
		out.g = (uint8_t)CLAMP(hsv.v * (vAvg*(1.0-hsv.s) + hsv.s*c.g), 0, 255);
		out.b = (uint8_t)CLAMP(hsv.v * (vAvg*(1.0-hsv.s) + hsv.s*c.b), 0, 255);
	}
	out.a = c.a;
	return out;
}

bool ColorEquals(const color_t a, const color_t b)
{
	return a.r == b.r && a.g == b.g && a.b == b.b;
}
bool HSVEquals(const HSV a, const HSV b)
{
	const double epsilon = 0.000001;
	return fabs(a.h - b.h) < epsilon && fabs(a.s - b.s) < epsilon &&
		fabs(a.v - b.v) < epsilon;
}

color_t StrColor(const char *s)
{
	if (s == NULL || strlen(s) != 6)
	{
		return colorBlack;
	}
	int hex = (int)strtol(s, NULL, 16);
	color_t c;
	c.a = 255;
	c.r = (hex >> 16) & 255;
	c.g = (hex >> 8) & 255;
	c.b = hex & 255;
	return c;
}
void ColorStr(char *s, const color_t c)
{
	sprintf(s, "%02x%02x%02x", c.r, c.g, c.b);
}
