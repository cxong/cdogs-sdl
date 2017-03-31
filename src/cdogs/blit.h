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

    Copyright (c) 2013-2014, 2016 Cong Xu
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
#include "vector.h"

typedef struct
{
	color_t Skin;
	color_t Arms;
	color_t Body;
	color_t Legs;
	color_t Hair;
} CharColors;
typedef enum
{
	CHAR_COLOR_SKIN,
	CHAR_COLOR_ARMS,
	CHAR_COLOR_BODY,
	CHAR_COLOR_LEGS,
	CHAR_COLOR_HAIR,
	CHAR_COLOR_COUNT
} CharColorType;
color_t *CharColorGetByType(CharColors *c, const CharColorType t);

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
void BlitCharMultichannel(
	GraphicsDevice *device,
	const Pic *pic,
	const Vec2i pos,
	const CharColors *masks);
void BlitBlend(
	GraphicsDevice *g, const Pic *pic, Vec2i pos, const color_t blend);
void BlitPicHighlight(
	GraphicsDevice *g, const Pic *pic, const Vec2i pos, const color_t color);

void BlitFlip(GraphicsDevice *g);

Uint32 PixelMult(const Uint32 p, const Uint32 m);
color_t CharColorsGetChannelMask(const CharColors *c, const uint8_t alpha);

#define BLIT_BRIGHTNESS_MIN (-10)
#define BLIT_BRIGHTNESS_MAX 10
