/*
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
#pragma once

#include <SDL_surface.h>

#include "vector.h"

typedef struct
{
	Vec2i size;
	Vec2i offset;
	Uint32 *Data;
} Pic;

extern Pic picNone;

color_t PixelToColor(
	const SDL_PixelFormat *f, const Uint8 aShift, const Uint32 pixel);
Uint32 ColorToPixel(
	const SDL_PixelFormat *f, const Uint8 aShift, const color_t color);
#define PIXEL2COLOR(_p) \
	PixelToColor(gGraphicsDevice.Format, gGraphicsDevice.Format->Ashift, _p)
#define COLOR2PIXEL(_c) \
	ColorToPixel(gGraphicsDevice.Format, gGraphicsDevice.Format->Ashift, _c)

void PicLoad(
	Pic *p, const Vec2i size, const Vec2i offset, const SDL_Surface *image);
Pic PicCopy(const Pic *src);
void PicFree(Pic *pic);
bool PicIsNone(const Pic *pic);

// Detect unused edges and update size and offset to fit
void PicTrim(Pic *pic, const bool xTrim, const bool yTrim);

bool PicPxIsEdge(const Pic *pic, const Vec2i pos, const bool isPixel);
