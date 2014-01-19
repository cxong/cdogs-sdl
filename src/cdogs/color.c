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
#include "color.h"

#include <stdlib.h>

#include "utils.h"

color_t colorWhite = { 255, 255, 255, 255 };
color_t colorRed = { 255, 0, 0, 255 };
color_t colorGreen = { 0, 255, 0, 255 };
color_t colorPoison = { 64, 192, 64, 255 };
color_t colorBlack = { 0, 0, 0, 255 };
color_t colorDarker = { 192, 192, 192, 255 };
color_t colorPurple = { 192, 0, 192, 255 };
color_t colorGray = { 128, 128, 128, 255 };
color_t colorYellow = { 255, 255, 128, 255 };
color_t colorCyan = { 0, 255, 255, 255 };

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
HSV tintGreen = { 120.0, 1.0, 1.0 };
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

int ColorEquals(color_t a, color_t b)
{
	return a.r == b.r && a.g == b.g && a.b == b.b;
}
