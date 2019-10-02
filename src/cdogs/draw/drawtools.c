/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (C) 2003-2007 Lucas Martin-King 

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    This file incorporates work covered by the following copyright and
    permission notice:

    Copyright (c) 2013-2014, 2018-2019 Cong Xu
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

#include <stdio.h>

#include "algorithms.h"
#include "config.h"
#include "draw/drawtools.h"
#include "log.h"
#include "palette.h"
#include "pic_manager.h"
#include "texture.h"
#include "utils.h"
#include "blit.h"
#include "grafx.h"


void DrawPoint(const struct vec2i pos, const color_t c)
{
	if (SDL_SetRenderDrawBlendMode(
		gGraphicsDevice.gameWindow.renderer, SDL_BLENDMODE_BLEND) != 0)
	{
		LOG(LM_GFX, LL_ERROR, "Failed to set draw blend mode: %s",
			SDL_GetError());
	}
	if (SDL_SetRenderDrawColor(
		gGraphicsDevice.gameWindow.renderer, c.r, c.g, c.b, c.a) != 0)
	{
		LOG(LM_GFX, LL_ERROR, "Failed to set draw color: %s",
			SDL_GetError());
	}
	if (SDL_RenderDrawPoint(
		gGraphicsDevice.gameWindow.renderer, pos.x, pos.y) != 0)
	{
		LOG(LM_GFX, LL_ERROR, "Failed to render point: %s", SDL_GetError());
	}
}

static
void
Draw_StraightLine(
	const int x1, const int y1, const int x2, const int y2, color_t c)
{
	register int i;
	register int start, end;
	
	if (x1 == x2) {					/* vertical line */
		if (y2 > y1) {
			start = y1;
			end = y2;
		} else {
			start = y2;
			end = y1;
		}
		
		for (i = start; i <= end; i++) {
			DrawPoint(svec2i(x1, i), c);
		}
	} else if (y1 == y2) {				/* horizontal line */
		if (x2 > x1) {
			start = x1;
			end = x2;
		} else {
			start = x2;
			end = x1;
		}
			    
		for (i = start; i <= end; i++) {
			DrawPoint(svec2i(i, y1), c);
		}
	}
	return;
}

static void Draw_DiagonalLine(const int x1, const int x2, color_t c)
{
	register int i;
	register int start, end;
	
	if (x1 < x2) {
		start = x1;
		end = x2;
	} else {
		start = x2;
		end = x1;
	}
	
	for (i = start; i < end; i++) {
		DrawPoint(svec2i(i, i), c);
	}
	
	return;
}

static void DrawPointFunc(void *data, const struct vec2i pos);
void DrawLine(const struct vec2i from, const struct vec2i to, color_t c)
{
	AlgoLineDrawData data;
	data.Draw = DrawPointFunc;
	data.data = &c;
	BresenhamLineDraw(from, to, &data);
}
static void DrawPointFunc(void *data, const struct vec2i pos)
{
	const color_t *c = data;
	DrawPoint(pos, *c);
}

void Draw_Line(
	const int x1, const int y1, const int x2, const int y2, color_t c)
{
	if (x1 == x2 || y1 == y2) 
		Draw_StraightLine(x1, y1, x2, y2, c);
	else if (abs((x2 - x1)) == abs((y1 - y2)))
		Draw_DiagonalLine(x1, x2, c);
	/*else
		Draw_OtherLine(x1, y1, x2, y2, c);*/
	
	return;
}

void DrawRectangle(
	GraphicsDevice *g, const struct vec2i pos, const struct vec2i size,
	const color_t color, const bool filled)
{
	SDL_Rect rect = {
		MAX(pos.x, g->clipping.left),
		MAX(pos.y, g->clipping.top),
		MIN(size.x, g->clipping.right + 1 - pos.x),
		MIN(size.y, g->clipping.bottom + 1 - pos.y)
	};
	if (SDL_SetRenderDrawBlendMode(
		g->gameWindow.renderer, SDL_BLENDMODE_BLEND) != 0)
	{
		LOG(LM_GFX, LL_ERROR, "Failed to set draw blend mode: %s",
			SDL_GetError());
	}
	if (SDL_SetRenderDrawColor(
		g->gameWindow.renderer, color.r, color.g, color.b, color.a) != 0)
	{
		LOG(LM_GFX, LL_ERROR, "Failed to set draw color: %s",
			SDL_GetError());
	}
	const int result =
		filled ?
		SDL_RenderFillRect(g->gameWindow.renderer, &rect) :
		SDL_RenderDrawRect(g->gameWindow.renderer, &rect);
	if (result != 0)
	{
		LOG(LM_GFX, LL_ERROR, "Failed to render rect: %s", SDL_GetError());
	}
}

void DrawCross(GraphicsDevice *g, const struct vec2i pos, const color_t c)
{
	if (SDL_SetRenderDrawBlendMode(
		g->gameWindow.renderer, SDL_BLENDMODE_BLEND) != 0)
	{
		LOG(LM_GFX, LL_ERROR, "Failed to set draw blend mode: %s",
			SDL_GetError());
	}
	if (SDL_SetRenderDrawColor(g->gameWindow.renderer, c.r, c.g, c.b, c.a) != 0)
	{
		LOG(LM_GFX, LL_ERROR, "Failed to set draw color: %s",
			SDL_GetError());
	}
	if (SDL_RenderDrawLine(
		g->gameWindow.renderer, pos.x - 1, pos.y, pos.x + 1, pos.y) != 0)
	{
		LOG(LM_GFX, LL_ERROR, "Failed to render line: %s", SDL_GetError());
	}
	if (SDL_RenderDrawLine(
		g->gameWindow.renderer, pos.x, pos.y - 1, pos.x, pos.y + 1) != 0)
	{
		LOG(LM_GFX, LL_ERROR, "Failed to render line: %s", SDL_GetError());
	}
}

void DrawShadow(
	GraphicsDevice *g, const struct vec2i pos, const struct vec2 scale,
	const color_t mask)
{
	if (!ConfigGetBool(&gConfig, "Graphics.Shadows") ||
		ColorEquals(mask, colorTransparent))
	{
		return;
	}
	const Pic *shadow = PicManagerGetPic(&gPicManager, "shadow");
	const struct vec2 drawScale =
		svec2_divide(svec2_scale(scale, 2), svec2_assign_vec2i(shadow->size));
	const struct vec2i drawPos = svec2i_subtract(pos, svec2i_assign_vec2(scale));
	PicRender(
		shadow, g->gameWindow.renderer, drawPos, mask, 0,
		drawScale, SDL_FLIP_NONE, Rect2iZero());
}
