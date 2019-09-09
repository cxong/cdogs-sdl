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

    Copyright (c) 2013-2018 Cong Xu
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

void Blit(GraphicsDevice *device, const Pic *pic, struct vec2i pos)
{
	Uint32 *current = pic->Data;
	pos = svec2i_add(pos, pic->offset);
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
	struct vec2i pos,
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
	pos = svec2i_add(pos, pic->offset);
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
			current++;
		}
	}
}
CharColors CharColorsFromOneColor(const color_t color)
{
	CharColors c = { color, color, color, color, color };
	return c;
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
void CharColorsGetMaskedName(char *buf, const char *base, const CharColors *c)
{
	char bufSkin[16], bufArms[16], bufBody[16], bufLegs[16], bufHair[16];
	ColorStr(bufSkin, c->Skin);
	ColorStr(bufArms, c->Arms);
	ColorStr(bufBody, c->Body);
	ColorStr(bufLegs, c->Legs);
	ColorStr(bufHair, c->Hair);
	sprintf(buf, "%s/%s/%s/%s/%s/%s", base,
		bufSkin, bufArms, bufBody, bufLegs, bufHair);
}

void BlitClearBuf(GraphicsDevice *g)
{
	memset(g->buf, 0, GraphicsGetMemSize(&g->cachedConfig));
}
void BlitUpdateFromBuf(GraphicsDevice *g, SDL_Texture *t)
{
	if (SDL_UpdateTexture(
		t, NULL, g->buf, g->cachedConfig.Res.x * sizeof(Uint32)) != 0)
	{
		LOG(LM_GFX, LL_ERROR, "cannot update texture: %s", SDL_GetError());
	}
}
