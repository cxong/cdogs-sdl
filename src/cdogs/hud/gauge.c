/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.

    Copyright (c) 2019 Cong Xu
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
#include "gauge.h"

#include "draw/nine_slice.h"


// Draw a gauge with a 9-slice background and inner level
//  --------------
// (XXXXXXXX|     )
//  --------------
void HUDDrawGauge(
	GraphicsDevice *g, const PicManager *pm, struct vec2i pos,
	const int width, const int innerWidth,
	const color_t barColor, const color_t backColor)
{
	const int height = 10; // TODO: arbitrary height

	const Pic *backPic = PicManagerGetPic(pm, "hud/gauge_back");
	Draw9Slice(
		g, backPic, Rect2iNew(pos, svec2i(width, height)), 0, 3, 0, 4, false,
		backColor, SDL_FLIP_NONE);

	if (innerWidth > 0)
	{
		HUDDrawGaugeInner(g, pm, pos, innerWidth, barColor);
	}
}

void HUDDrawGaugeInner(
	GraphicsDevice *g, const PicManager *pm, struct vec2i pos,
	const int width, const color_t barColor)
{
	const int height = 10; // TODO: arbitrary height

	const Pic *innerPic = PicManagerGetPic(pm, "hud/gauge_inner");
	Draw9Slice(
		g, innerPic, Rect2iNew(pos, svec2i(width, height)), 0, 3, 0, 4, false,
		barColor, SDL_FLIP_NONE);
}
