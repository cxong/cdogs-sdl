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
#include "color.h"

#include <stdlib.h>

#include "utils.h"

color_t colorRed = { 255, 0, 0 };
color_t colorGreen = { 0, 255, 0 };
color_t colorPoison = { 64, 255, 64 };
color_t colorBlack = { 0, 0, 0 };
color_t colorDarker = { 192, 192, 192 };
color_t colorPurple = { 128, 0, 128 };

color_t ColorMult(color_t a, color_t b)
{
	a.red = (int)a.red * b.red / 255;
	a.green = (int)a.green * b.green / 255;
	a.blue = (int)a.blue * b.blue / 255;
	return a;
}

color_t ColorRandomTint(void)
{
	// Generate three random numbers between 0-1, and divide each by 1/3rd the sum.
	// The resulting three scaled numbers should add up to 1.
	color_t c;
	double r_scalar = rand() * 1.0 / RAND_MAX;
	double g_scalar = rand() * 1.0 / RAND_MAX;
	double b_scalar = rand() * 1.0 / RAND_MAX;
	double scale_factor = r_scalar + g_scalar + b_scalar;
	r_scalar /= scale_factor;
	g_scalar /= scale_factor;
	b_scalar /= scale_factor;
	c.red = (uint8_t)CLAMP(r_scalar * 256, 0, 255);
	c.green = (uint8_t)CLAMP(g_scalar * 256, 0, 255);
	c.blue = (uint8_t)CLAMP(b_scalar * 256, 0, 255);
	return c;
}
