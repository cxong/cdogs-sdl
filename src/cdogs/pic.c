/*
	Copyright (c) 2013-2016, 2018-2019 Cong Xu
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
#include "pic.h"

#include <stdlib.h>
#include <string.h>

#include "c_hashmap/hashmap.h"
#include "defs.h"
#include "grafx.h"
#include "log.h"
#include "texture.h"
#include "utils.h"

map_t textureDebugger = NULL;


color_t PixelToColor(
	const SDL_PixelFormat *f, const Uint8 aShift, const Uint32 pixel)
{
	color_t c;
	SDL_GetRGBA(pixel, f, &c.r, &c.g, &c.b, &c.a);
	// Manually apply the alpha as SDL seems to always set it to 0
	c.a = (Uint8)((pixel & ~(f->Rmask | f->Gmask | f->Bmask)) >> aShift);
	return c;
}
Uint32 ColorToPixel(
	const SDL_PixelFormat *f, const Uint8 aShift, const color_t color)
{
	const Uint32 pixel = SDL_MapRGBA(f, color.r, color.g, color.b, color.a);
	// Manually apply the alpha as SDL seems to always set it to 0
	return (pixel & (f->Rmask | f->Gmask | f->Bmask)) |
		((Uint32)color.a << aShift);
}


void PicLoad(
	Pic *p, const struct vec2i size, const struct vec2i offset, const SDL_Surface *image)
{
	memset(p, 0, sizeof *p);
	p->size = size;
	p->offset = svec2i_zero();
	CMALLOC(p->Data, size.x * size.y * sizeof *((Pic *)0)->Data);
	if (p->Data == NULL)
	{
		return;
	}
	// Manually copy the pixels and replace the alpha component,
	// since our gfx device format has no alpha
	int srcI = offset.y*image->w + offset.x;
	for (int i = 0; i < size.x * size.y; i++, srcI++)
	{
		const Uint32 pixel = ((Uint32 *)image->pixels)[srcI];
		color_t c;
		SDL_GetRGBA(pixel, image->format, &c.r, &c.g, &c.b, &c.a);
		// If completely transparent, replace rgb with black (0) too
		// This is because transparency blitting checks entire pixel
		if (c.a == 0)
		{
			p->Data[i] = 0;
		}
		else
		{
			p->Data[i] = COLOR2PIXEL(c);
		}
		if ((i + 1) % size.x == 0)
		{
			srcI += image->w - size.x;
		}
	}

	if (!PicTryMakeTex(p))
	{
		goto bail;
	}
	return;

bail:
	PicFree(p);
}
bool PicTryMakeTex(Pic *p)
{
	CASSERT(!PicIsNone(p), "cannot make tex of none pic");
	if (textureDebugger == NULL)
	{
		textureDebugger = hashmap_new();
	}
	if (p->Tex != NULL)
	{
		LOG(LM_GFX, LL_TRACE, "destroying texture %p data(%p)", p->Tex, p->Data);
		SDL_DestroyTexture(p->Tex);
		if (LL_TRACE >= LogModuleGetLevel(LM_GFX))
		{
			char key[32];
			sprintf(key, "%p", p->Tex);
			if (hashmap_get(textureDebugger, key, NULL) == MAP_OK)
			{
				if (hashmap_remove(textureDebugger, key) != MAP_OK)
				{
					LOG(LM_GFX, LL_TRACE, "Error: cannot remove tex from debugger");
				}
				else
				{
					LOG(LM_GFX, LL_TRACE, "Texture count: %d",
						hashmap_length(textureDebugger));
				}
			}
			else
			{
				LOG(LM_GFX, LL_TRACE, "Error: destroying unknown texture");
			}
		}
	}
	p->Tex = TextureCreate(
		gGraphicsDevice.gameWindow.renderer, SDL_TEXTUREACCESS_STATIC,
		p->size, SDL_BLENDMODE_NONE, 255);
	if (p->Tex == NULL)
	{
		LOG(LM_GFX, LL_ERROR, "cannot create texture: %s", SDL_GetError());
		return false;
	}
	if (SDL_UpdateTexture(
		p->Tex, NULL, p->Data, p->size.x * sizeof(Uint32)) != 0)
	{
		LOG(LM_GFX, LL_ERROR, "cannot update texture: %s", SDL_GetError());
		return false;
	}
	if (SDL_SetTextureBlendMode(p->Tex, SDL_BLENDMODE_BLEND) != 0)
	{
		LOG(LM_GFX, LL_ERROR, "cannot set texture blend mode: %s",
			SDL_GetError());
		return false;
	}
	LOG(LM_GFX, LL_TRACE, "made texture %p data(%p) count(%d)",
		p->Tex, p->Data, hashmap_length(textureDebugger));
	if (LL_TRACE >= LogModuleGetLevel(LM_GFX))
	{
		char key[32];
		sprintf(key, "%p", p->Tex);
		if (hashmap_get(textureDebugger, key, NULL) != MAP_MISSING)
		{
			LOG(LM_GFX, LL_TRACE, "Error: repeated texture loc");
		}
		if (hashmap_put(textureDebugger, key, (any_t)0) != MAP_OK)
		{
			LOG(LM_GFX, LL_TRACE, "Error: cannot add texture to debugger");
		}
	}
	return true;
}

// Note: does not copy the texture
Pic PicCopy(const Pic *src)
{
	Pic p = *src;
	const size_t size = p.size.x * p.size.y * sizeof *p.Data;
	CMALLOC(p.Data, size);
	memcpy(p.Data, src->Data, size);
	p.Tex = NULL;
	return p;
}

void PicFree(Pic *pic)
{
	if (pic->Tex != NULL)
	{
		LOG(LM_GFX, LL_TRACE, "freeing texture %p data(%p)", pic->Tex, pic->Data);
		SDL_DestroyTexture(pic->Tex);
		if (LL_TRACE >= LogModuleGetLevel(LM_GFX))
		{
			char key[32];
			sprintf(key, "%p", pic->Tex);
			if (hashmap_get(textureDebugger, key, NULL) == MAP_OK)
			{
				if (hashmap_remove(textureDebugger, key) != MAP_OK)
				{
					LOG(LM_GFX, LL_TRACE, "Error: cannot remove tex from debugger");
				}
				else
				{
					LOG(LM_GFX, LL_TRACE, "Texture count: %d",
						hashmap_length(textureDebugger));
				}
			}
			else
			{
				LOG(LM_GFX, LL_TRACE, "Error: destroying unknown texture");
			}
		}
	}
	pic->size = svec2i_zero();
	CFREE(pic->Data);
	pic->Data = NULL;
}

bool PicIsNone(const Pic *pic)
{
	return pic->size.x == 0 || pic->size.y == 0 || pic->Data == NULL;
}

void PicTrim(Pic *pic, const bool xTrim, const bool yTrim)
{
	// Scan all pixels looking for the min/max of x and y
	struct vec2i min = pic->size;
	struct vec2i max = svec2i_zero();
	for (struct vec2i pos = svec2i_zero(); pos.y < pic->size.y; pos.y++)
	{
		for (pos.x = 0; pos.x < pic->size.x; pos.x++)
		{
			const Uint32 pixel = *(pic->Data + pos.x + pos.y * pic->size.x);
			if (pixel > 0)
			{
				min.x = MIN(min.x, pos.x);
				min.y = MIN(min.y, pos.y);
				max.x = MAX(max.x, pos.x);
				max.y = MAX(max.y, pos.y);
			}
		}
	}
	// If no opaque pixels found, don't trim
	struct vec2i newSize = pic->size;
	struct vec2i offset = svec2i_zero();
	if (min.x < max.x && min.y < max.y)
	{
		if (xTrim)
		{
			newSize.x = max.x - min.x + 1;
			offset.x = min.x;
		}
		if (yTrim)
		{
			newSize.y = max.y - min.y + 1;
			offset.y = min.y;
		}
	}
	PicShrink(pic, newSize, offset);
}
void PicShrink(Pic *pic, const struct vec2i size, const struct vec2i offset)
{
	// Trim by copying pixels
	Uint32 *newData;
	CMALLOC(newData, size.x * size.y * sizeof *newData);
	if (newData == NULL)
	{
		return;
	}
	for (struct vec2i pos = svec2i_zero(); pos.y < size.y; pos.y++)
	{
		for (pos.x = 0; pos.x < size.x; pos.x++)
		{
			Uint32 *target = newData + pos.x + pos.y * size.x;
			const int srcIdx =
				pos.x + offset.x + (pos.y + offset.y) * pic->size.x;
			*target = *(pic->Data + srcIdx);
		}
	}
	// Replace the old data
	CFREE(pic->Data);
	pic->Data = newData;
	pic->size = size;
	pic->offset = svec2i_zero();
	PicTryMakeTex(pic);
}

bool PicPxIsEdge(const Pic *pic, const struct vec2i pos, const bool isPixel)
{
	const bool isTopOrBottomEdge = pos.y == -1 || pos.y == pic->size.y;
	const bool isLeftOrRightEdge = pos.x == -1 || pos.x == pic->size.x;
	const bool isLeft =
		pos.x > 0 && !isTopOrBottomEdge &&
		PIXEL2COLOR(*(pic->Data + pos.x - 1 + pos.y * pic->size.x)).a;
	const bool isRight =
		pos.x < pic->size.x - 1 && !isTopOrBottomEdge &&
		PIXEL2COLOR(*(pic->Data + pos.x + 1 + pos.y * pic->size.x)).a;
	const bool isAbove =
		pos.y > 0 && !isLeftOrRightEdge &&
		PIXEL2COLOR(*(pic->Data + pos.x + (pos.y - 1) * pic->size.x)).a;
	const bool isBelow =
		pos.y < pic->size.y - 1 && !isLeftOrRightEdge &&
		PIXEL2COLOR(*(pic->Data + pos.x + (pos.y + 1) * pic->size.x)).a;
	if (isPixel)
	{
		return !(isLeft && isRight && isAbove && isBelow);
	}
	else
	{
		return isLeft || isRight || isAbove || isBelow;
	}
}
color_t PicGetRandomColor(const Pic *p)
{
	// Get a random non-transparent pixel from the pic
	for (;;)
	{
		const uint32_t px = p->Data[rand() % (p->size.x * p->size.y)];
		const color_t c = PIXEL2COLOR(px);
		if (c.a > 0)
		{
			return c;
		}
	}
}

void PicRender(
	const Pic *p, SDL_Renderer *r, const struct vec2i pos, const color_t mask,
	const double radians, const struct vec2 scale, const SDL_RendererFlip flip,
	const Rect2i srcRect)
{
	Rect2i src = Rect2iNew(
		svec2i_max(srcRect.Pos, svec2i_zero()), svec2i_zero()
	);
	src.Size = svec2i_is_zero(srcRect.Size) ? p->size :
		svec2i_min(svec2i_subtract(srcRect.Size, src.Pos), p->size);
	Rect2i dest = Rect2iNew(pos, src.Size);
	// Apply scale to render dest
	if (!svec2_is_equal(scale, svec2_one()))
	{
		dest.Pos.x -= (mint_t)MROUND((scale.x - 1) * src.Size.x / 2);
		dest.Pos.y -= (mint_t)MROUND((scale.y - 1) * src.Size.y / 2);
		dest.Size.x = (mint_t)MROUND(src.Size.x * scale.x);
		dest.Size.y = (mint_t)MROUND(src.Size.y * scale.y);
	}
	const double angle = ToDegrees(radians);
	TextureRender(p->Tex, r, src, dest, mask, angle, flip);
}
