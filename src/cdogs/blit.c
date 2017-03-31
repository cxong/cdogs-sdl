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
#include "blit.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <SDL.h>

#include "config.h"
#include "log.h"


color_t *CharColorGetByType(CharColors *c, const CharColorType t)
{
	switch (t)
	{
	case CHAR_COLOR_SKIN: return &c->Skin;
	case CHAR_COLOR_ARMS: return &c->Arms;
	case CHAR_COLOR_BODY: return &c->Body;
	case CHAR_COLOR_LEGS: return &c->Legs;
	case CHAR_COLOR_HAIR: return &c->Hair;
	default:
		CASSERT(false, "Unexpected colour");
		return &c->Skin;
	}
}


void BlitPicHighlight(
	GraphicsDevice *g, const Pic *pic, const Vec2i pos, const color_t color)
{
	// Draw highlight around the picture
	int i;
	for (i = -1; i < pic->size.y + 1; i++)
	{
		int j;
		int yoff = i + pos.y + pic->offset.y;
		if (yoff > g->clipping.bottom)
		{
			break;
		}
		if (yoff < g->clipping.top)
		{
			continue;
		}
		yoff *= g->cachedConfig.Res.x;
		for (j = -1; j < pic->size.x + 1; j++)
		{
			int xoff = j + pos.x + pic->offset.x;
			if (xoff < g->clipping.left)
			{
				continue;
			}
			if (xoff > g->clipping.right)
			{
				break;
			}
			// Draw highlight if current pixel is empty,
			// and is next to a picture edge
			bool isTopOrBottomEdge = i == -1 || i == pic->size.y;
			bool isLeftOrRightEdge = j == -1 || j == pic->size.x;
			bool isPixelEmpty =
				isTopOrBottomEdge || isLeftOrRightEdge ||
				!PIXEL2COLOR(*(pic->Data + j + i * pic->size.x)).a;
			if (isPixelEmpty &&
				PicPxIsEdge(pic, Vec2iNew(j, i), !isPixelEmpty))
			{
				Uint32 *target = g->buf + yoff + xoff;
				const color_t targetColor = PIXEL2COLOR(*target);
				const color_t blendedColor = ColorAlphaBlend(
					targetColor, color);
				*target = COLOR2PIXEL(blendedColor);
			}
		}
	}
}

void BlitBackground(
	GraphicsDevice *device,
	const Pic *pic, Vec2i pos, const HSV *tint, const bool isTransparent)
{
	Uint32 *current = pic->Data;
	pos = Vec2iAdd(pos, pic->offset);
	for (int i = 0; i < pic->size.y; i++)
	{
		int yoff = i + pos.y;
		if (yoff > device->clipping.bottom)
		{
			break;
		}
		if (yoff < device->clipping.top)
		{
			current += pic->size.x;
			continue;
		}
		yoff *= device->cachedConfig.Res.x;
		for (int j = 0; j < pic->size.x; j++)
		{
			int xoff = j + pos.x;
			if (xoff < device->clipping.left)
			{
				current++;
				continue;
			}
			if (xoff > device->clipping.right)
			{
				current += pic->size.x - j;
				break;
			}
			if ((isTransparent && *current) ||  !isTransparent)
			{
				Uint32 *target = gGraphicsDevice.buf + yoff + xoff;
				if (tint != NULL)
				{
					const color_t targetColor = PIXEL2COLOR(*target);
					const color_t blendedColor = ColorTint(targetColor, *tint);
					*target = COLOR2PIXEL(blendedColor);
				}
				else
				{
					*target = *current;
				}
			}
			current++;
		}
	}
}

void Blit(GraphicsDevice *device, const Pic *pic, Vec2i pos)
{
	Uint32 *current = pic->Data;
	pos = Vec2iAdd(pos, pic->offset);
	for (int i = 0; i < pic->size.y; i++)
	{
		int yoff = i + pos.y;
		if (yoff > device->clipping.bottom)
		{
			break;
		}
		if (yoff < device->clipping.top)
		{
			current += pic->size.x;
			continue;
		}
		yoff *= device->cachedConfig.Res.x;
		for (int j = 0; j < pic->size.x; j++)
		{
			Uint32 *target;
			int xoff = j + pos.x;
			if (xoff < device->clipping.left)
			{
				current++;
				continue;
			}
			if (xoff > device->clipping.right)
			{
				current += pic->size.x - j;
				break;
			}
			if ((*current & device->Format->Amask) == 0)
			{
				current++;
				continue;
			}
			target = device->buf + yoff + xoff;
			*target = *current;
			current++;
		}
	}
}

Uint32 PixelMult(const Uint32 p, const Uint32 m)
{
	return
		((p & 0xFF) * (m & 0xFF) / 0xFF) |
		((((p & 0xFF00) >> 8) * ((m & 0xFF00) >> 8) / 0xFF) << 8) |
		((((p & 0xFF0000) >> 16) * ((m & 0xFF0000) >> 16) / 0xFF) << 16) |
		((((p & 0xFF000000) >> 24) * ((m & 0xFF000000) >> 24) / 0xFF) << 24);
}
void BlitMasked(
	GraphicsDevice *device,
	const Pic *pic,
	Vec2i pos,
	color_t mask,
	int isTransparent)
{
	Uint32 *current = pic->Data;
	if (current == NULL)
	{
		CASSERT(false, "unexpected NULL pic data");
		return;
	}
	const Uint32 maskPixel = COLOR2PIXEL(mask);
	int i;
	pos = Vec2iAdd(pos, pic->offset);
	for (i = 0; i < pic->size.y; i++)
	{
		int yoff = i + pos.y;
		if (yoff > device->clipping.bottom)
		{
			break;
		}
		if (yoff < device->clipping.top)
		{
			current += pic->size.x;
			continue;
		}
		yoff *= device->cachedConfig.Res.x;
		for (int j = 0; j < pic->size.x; j++)
		{
			Uint32 *target;
			int xoff = j + pos.x;
			if (xoff < device->clipping.left)
			{
				current++;
				continue;
			}
			if (xoff > device->clipping.right)
			{
				current += pic->size.x - j;
				break;
			}
			if (isTransparent &&
				((*current & device->Format->Amask) >> device->Format->Ashift) < 3)
			{
				current++;
				continue;
			}
			target = device->buf + yoff + xoff;
			*target = PixelMult(*current, maskPixel);
			*target |= device->Format->Amask;
			current++;
		}
	}
}
void BlitCharMultichannel(
	GraphicsDevice *device,
	const Pic *pic,
	const Vec2i pos,
	const CharColors *masks)
{
	Uint32 *current = pic->Data;
	Vec2i v = Vec2iAdd(pos, pic->offset);
	for (int i = 0; i < pic->size.y; i++)
	{
		int yoff = i + v.y;
		if (yoff > device->clipping.bottom)
		{
			break;
		}
		if (yoff < device->clipping.top)
		{
			current += pic->size.x;
			continue;
		}
		yoff *= device->cachedConfig.Res.x;
		for (int j = 0; j < pic->size.x; j++)
		{
			Uint32 *target;
			const int xoff = j + v.x;
			if (xoff < device->clipping.left)
			{
				current++;
				continue;
			}
			if (xoff > device->clipping.right)
			{
				current += pic->size.x - j;
				break;
			}
			if (*current == 0)
			{
				current++;
				continue;
			}
			target = device->buf + yoff + xoff;
			const color_t color = PIXEL2COLOR(*current);
			*target = PixelMult(
				*current,
				COLOR2PIXEL(CharColorsGetChannelMask(masks, color.a)));
			current++;
		}
	}
}
color_t CharColorsGetChannelMask(
	const CharColors *c, const uint8_t alpha)
{
	switch (alpha)
	{
	case 255: return colorWhite;
	case 254: return c->Skin;
	case 253: return c->Arms;
	case 252: return c->Body;
	case 251: return c->Legs;
	case 250: return c->Hair;
	default:
		CASSERT(false, "Unknown alpha channel");
		return colorWhite;
	}
}
void BlitBlend(
	GraphicsDevice *g, const Pic *pic, Vec2i pos, const color_t blend)
{
	Uint32 *current = pic->Data;
	pos = Vec2iAdd(pos, pic->offset);
	for (int i = 0; i < pic->size.y; i++)
	{
		int yoff = i + pos.y;
		if (yoff > g->clipping.bottom)
		{
			break;
		}
		if (yoff < g->clipping.top)
		{
			current += pic->size.x;
			continue;
		}
		yoff *= g->cachedConfig.Res.x;
		for (int j = 0; j < pic->size.x; j++)
		{
			int xoff = j + pos.x;
			if (xoff < g->clipping.left)
			{
				current++;
				continue;
			}
			if (xoff > g->clipping.right)
			{
				current += pic->size.x - j;
				break;
			}
			if (*current == 0)
			{
				current++;
				continue;
			}
			Uint32 *target = g->buf + yoff + xoff;
			const color_t currentColor = PIXEL2COLOR(*current);
			color_t blendedColor = ColorMult(
				currentColor, blend);
			blendedColor.a = blend.a;
			const color_t targetColor = PIXEL2COLOR(*target);
			blendedColor = ColorAlphaBlend(targetColor, blendedColor);
			*target = COLOR2PIXEL(blendedColor);
			current++;
		}
	}
}

static void RenderTexture(SDL_Renderer *r, SDL_Texture *t);
void BlitFlip(GraphicsDevice *g)
{
	SDL_UpdateTexture(
		g->screen, NULL, g->buf, g->cachedConfig.Res.x * sizeof(Uint32));
	if (SDL_RenderClear(g->renderer) != 0)
	{
		LOG(LM_MAIN, LL_ERROR, "Failed to clear renderer: %s\n",
			SDL_GetError());
		return;
	}
	RenderTexture(g->renderer, g->bkg);
	RenderTexture(g->renderer, g->screen);
	// Apply brightness as an overlay texture
	RenderTexture(g->renderer, g->brightnessOverlay);

	SDL_RenderPresent(g->renderer);
}
static void RenderTexture(SDL_Renderer *r, SDL_Texture *t)
{
	if (SDL_RenderCopy(r, t, NULL, NULL) != 0)
	{
		LOG(LM_MAIN, LL_ERROR, "Failed to render texture: %s", SDL_GetError());
	}
}
