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

#include <SDL_stdinc.h>

#include "grafx.h"
#include "pic.h"
#include "pic_file.h"
#include "vector.h"

#define BLIT_TRANSPARENT 1
#define BLIT_BACKGROUND 2

void BlitOld(int x, int y, PicPaletted *pic, const void *table, int mode);
void BlitBackground(
	GraphicsDevice *device,
	const Pic *pic, Vec2i pos, const HSV *tint, const bool isTransparent);
void Blit(GraphicsDevice *device, const Pic *pic, Vec2i pos);
void BlitMasked(
	GraphicsDevice *device,
	const Pic *pic,
	Vec2i pos,
	color_t mask,
	int isTransparent);
void BlitBlend(
	GraphicsDevice *g, const Pic *pic, Vec2i pos, const color_t blend);
void BlitPicHighlight(
	GraphicsDevice *g, const Pic *pic, const Vec2i pos, const color_t color);
/* DrawPic - simply draws a rectangular picture to screen. I do not
 * remember if this is the one that ignores zero source-pixels or not, but
 * that much should be obvious.
 */
#define DrawPic(x, y, pic) (BlitOld(x, y, pic, NULL, 0))
/* 
 * DrawTPic - I think the T here stands for transparent, ie ignore zero
 * source pixels when copying data.
 */
#define DrawTPic(x, y, pic) (BlitOld(x, y, pic, NULL, BLIT_TRANSPARENT))
/*
 * DrawTTPic - I think this stands for translated transparent. What this
 * does is that for each source pixel that would be copied it will first
 * translate the value by looking it up in the provided table. This means
 * that you can provide a 256 byte table to change any or all colors of
 * the source image. This feature is used heavily in the game.
 */
#define DrawTTPic(x, y, pic, table) (BlitOld(x, y, pic, table, BLIT_TRANSPARENT))
/* 
 * DrawBTPic - I think the B stands for background here. If I remember
 * correctly this one uses the sourc eimage only as a mask. If a pixel in
 * the image is non-zero, look at the value at the destination and
 * translate that value through the table and put it back. This is used to
 * do the "invisible" guys as well as the gas clouds.
 */
#define DrawBTPic(g, pic, pos, tint) BlitBackground(g, pic, pos, tint, true)

void BlitFlip(GraphicsDevice *g);

#define BLIT_BRIGHTNESS_MIN (-10)
#define BLIT_BRIGHTNESS_MAX 10
