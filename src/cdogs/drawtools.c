/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (C) 1995 Ronny Wester
    Copyright (C) 2003 Jeremy Chin 
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

#include <stdio.h>

#include "config.h"
#include "drawtools.h"
#include "utils.h"
#include "blit.h"
#include "grafx.h"


void	Draw_Point (const int x, const int y, const unsigned char c)
{
	Uint32 *screen = gGraphicsDevice.buf;
	screen[PixelIndex(
		x,
		y,
		gGraphicsDevice.cachedConfig.ResolutionWidth,
		gGraphicsDevice.cachedConfig.ResolutionHeight)] = LookupPalette(c);
}

static
void
Draw_StraightLine (const int x1, const int y1, const int x2, const int y2, const unsigned char c)
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

static void Draw_DiagonalLine(
	const int x1, const int x2, const unsigned char c)
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

/*static void Draw_OtherLine(
	const int x1, const int y1, const int x2, const int y2, const unsigned char c)
{
	printf("### Other lines not yet implemented! ###\n");
}*/

void	Draw_Line  (const int x1, const int y1, const int x2, const int y2, const unsigned char c)
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

void DrawPointMask(GraphicsDevice *device, Vector2i pos, color_t mask)
{
	Uint32 *screen = device->buf;
	int idx = PixelIndex(
		pos.x, pos.y,
		device->cachedConfig.ResolutionWidth,
		device->cachedConfig.ResolutionHeight);
	color_t c = PixelToColor(device, screen[idx]);
	c.r = (uint8_t)((int)c.r * mask.r / 255);
	c.g = (uint8_t)((int)c.g * mask.g / 255);
	c.b = (uint8_t)((int)c.b * mask.b / 255);
	screen[idx] = PixelFromColor(device, c);
}

void DrawPointTint(GraphicsDevice *device, Vector2i pos, HSV tint)
{
	Uint32 *screen = device->buf;
	int idx = PixelIndex(
		pos.x, pos.y,
		device->cachedConfig.ResolutionWidth,
		device->cachedConfig.ResolutionHeight);
	color_t c = PixelToColor(device, screen[idx]);
	c = ColorTint(c, tint);
	screen[idx] = PixelFromColor(device, c);
}
