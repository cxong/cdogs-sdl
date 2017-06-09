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
#pragma once

#include "grafx.h"

void Draw_Point(const int x, const int y, color_t c);
void Draw_Line(
	const int x1, const int y1, const int x2, const int y2, color_t c);
void DrawLine(const Vec2i from, const Vec2i to, color_t c);

#define PixelIndex(x, y, w, h)		(y * w + x)

#define Draw_Box(x1, y1, x2, y2, c)		\
		Draw_Line(x1, y1, x2, y1, c);	\
		Draw_Line(x2, y1, x2, y2, c);	\
		Draw_Line(x1, y2, x2, y2, c);	\
		Draw_Line(x1, y1, x1, y2, c);

#define Draw_Rect(x, y, w, h, c)	Draw_Box(x,y,((x + (w - 1))),((y + (h - 1))),c)

void DrawPointMask(GraphicsDevice *device, Vec2i pos, color_t mask);
void DrawPointTint(GraphicsDevice *device, Vec2i pos, HSV tint);

typedef enum
{
	DRAW_FLAG_LINE = 1,
	DRAW_FLAG_ROUNDED = 2
} DrawFlags;
void DrawRectangle(
	GraphicsDevice *device, Vec2i pos, Vec2i size, color_t color, int flags);

//  *
// ***
//  *
void DrawCross(GraphicsDevice *device, int x, int y, color_t color);

void DrawShadow(GraphicsDevice *device, Vec2i pos, Vec2i size);
