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

#include <stdio.h>

#include "algorithms.h"
#include "config.h"
#include "draw/drawtools.h"
#include "palette.h"
#include "utils.h"
#include "blit.h"
#include "grafx.h"


void Draw_Point(const int x, const int y, color_t c)
{
	Uint32 *screen = gGraphicsDevice.buf;
	int idx = PixelIndex(
		x,
		y,
		gGraphicsDevice.cachedConfig.Res.x,
		gGraphicsDevice.cachedConfig.Res.y);
	if (x < gGraphicsDevice.clipping.left ||
		x > gGraphicsDevice.clipping.right ||
		y < gGraphicsDevice.clipping.top ||
		y > gGraphicsDevice.clipping.bottom)
	{
		return;
	}
	if (c.a == 255)
	{
		screen[idx] = COLOR2PIXEL(c);
	}
	else
	{
		const color_t existing = PIXEL2COLOR(screen[idx]);
		screen[idx] = COLOR2PIXEL(ColorAlphaBlend(existing, c));
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
			Draw_Point(x1, i, c);
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
			Draw_Point(i, y1, c);
		}
	}
	return;
}

#define ABS(x)	( x > 0 ? x : (-x) )

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
		Draw_Point(i, i, c);
	}
	
	return;
}

static void DrawPointFunc(void *data, const Vec2i pos);
void DrawLine(const Vec2i from, const Vec2i to, color_t c)
{
	AlgoLineDrawData data;
	data.Draw = DrawPointFunc;
	data.data = &c;
	BresenhamLineDraw(from, to, &data);
}
static void DrawPointFunc(void *data, const Vec2i pos)
{
	const color_t *c = data;
	Draw_Point(pos.x, pos.y, *c);
}

void Draw_Line(
	const int x1, const int y1, const int x2, const int y2, color_t c)
{
	//debug("(%d, %d) -> (%d, %d)\n", x1, y1, x2, y2);
	
	if (x1 == x2 || y1 == y2) 
		Draw_StraightLine(x1, y1, x2, y2, c);
	else if (ABS((x2 - x1)) == ABS((y1 - y2)))
		Draw_DiagonalLine(x1, x2, c);
	/*else
		Draw_OtherLine(x1, y1, x2, y2, c);*/
	
	return;
}

void DrawPointMask(GraphicsDevice *g, Vec2i pos, color_t mask)
{
	if (pos.x < g->clipping.left || pos.x > g->clipping.right ||
		pos.y < g->clipping.top || pos.y > g->clipping.bottom)
	{
		return;
	}
	const int idx = PixelIndex(
		pos.x, pos.y, g->cachedConfig.Res.x, g->cachedConfig.Res.y);
	Uint32 *screen = g->buf;
	color_t c = PIXEL2COLOR(screen[idx]);
	c = ColorMult(c, mask);
	screen[idx] = COLOR2PIXEL(c);
}

void DrawPointTint(GraphicsDevice *device, Vec2i pos, HSV tint)
{
	Uint32 *screen = device->buf;
	int idx = PixelIndex(
		pos.x, pos.y,
		device->cachedConfig.Res.x,
		device->cachedConfig.Res.y);
	color_t c;
	if (pos.x < device->clipping.left || pos.x > device->clipping.right ||
		pos.y < device->clipping.top || pos.y > device->clipping.bottom)
	{
		return;
	}
	c = PIXEL2COLOR(screen[idx]);
	c = ColorTint(c, tint);
	screen[idx] = COLOR2PIXEL(c);
}

void DrawRectangle(
	GraphicsDevice *device, Vec2i pos, Vec2i size, color_t color, int flags)
{
	int y;
	if (size.x < 3 || size.y < 3)
	{
		flags &= ~DRAW_FLAG_ROUNDED;
	}
	for (y = MAX(pos.y, device->clipping.top);
		y < MIN(pos.y + size.y, device->clipping.bottom + 1);
		y++)
	{
		int isFirstOrLastLine = y == pos.y || y == pos.y + size.y - 1;
		if (isFirstOrLastLine && (flags & DRAW_FLAG_ROUNDED))
		{
			int x;
			for (x = MAX(pos.x + 1, device->clipping.left);
				x < MIN(pos.x + size.x - 1, device->clipping.right + 1);
				x++)
			{
				Draw_Point(x, y, color);
			}
		}
		else if (!isFirstOrLastLine && (flags & DRAW_FLAG_LINE))
		{
			Draw_Point(pos.x, y, color);
			Draw_Point(pos.x + size.x - 1, y, color);
		}
		else
		{
			int x;
			for (x = MAX(pos.x, device->clipping.left);
				x < MIN(pos.x + size.x, device->clipping.right + 1);
				x++)
			{
				Draw_Point(x, y, color);
			}
		}
	}
}

void DrawCross(GraphicsDevice *device, int x, int y, color_t color)
{
	Uint32 *screen = device->buf;
	const Uint32 pixel = COLOR2PIXEL(color);
	screen += x;
	screen += y * gGraphicsDevice.cachedConfig.Res.x;
	*screen = pixel;
	*(screen - 1) = pixel;
	*(screen + 1) = pixel;
	*(screen - gGraphicsDevice.cachedConfig.Res.x) = pixel;
	*(screen + gGraphicsDevice.cachedConfig.Res.x) = pixel;
}

void DrawShadow(GraphicsDevice *device, Vec2i pos, Vec2i size)
{
	if (!ConfigGetBool(&gConfig, "Graphics.Shadows"))
	{
		return;
	}
	Vec2i drawPos;
	for (drawPos.y = pos.y - size.y; drawPos.y < pos.y + size.y; drawPos.y++)
	{
		if (drawPos.y >= device->clipping.bottom)
		{
			break;
		}
		if (drawPos.y < device->clipping.top)
		{
			continue;
		}
		for (drawPos.x = pos.x - size.x; drawPos.x < pos.x + size.x; drawPos.x++)
		{
			// Calculate value tint based on distance from center
			Vec2i scaledPos;
			int distance2;
			if (drawPos.x >= device->clipping.right)
			{
				break;
			}
			if (drawPos.x < device->clipping.left)
			{
				continue;
			}
			scaledPos.x = drawPos.x;
			scaledPos.y = (drawPos.y - pos.y) * size.x / size.y + pos.y;
			distance2 = DistanceSquared(scaledPos, pos);
			// Maximum distance is x, so scale distance squared by x squared
			const HSV tint =
			{
				-1.0, 1.0, CLAMP(distance2 * 1.0 / (size.x*size.x), 0.0, 1.0)
			};
			DrawPointTint(device, drawPos, tint);
		}
	}
}
