/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (c) 2013-2019 Cong Xu
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
	char bufSkin[COLOR_STR_BUF], bufArms[COLOR_STR_BUF],
		bufBody[COLOR_STR_BUF], bufLegs[COLOR_STR_BUF], bufHair[COLOR_STR_BUF];
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
void BlitFillBuf(GraphicsDevice *g, const color_t c)
{
	const Uint32 pixel = COLOR2PIXEL(c);
	RECT_FOREACH(Rect2iNew(svec2i_zero(), g->cachedConfig.Res))
		g->buf[_i] = pixel;
	RECT_FOREACH_END()
}
void BlitUpdateFromBuf(GraphicsDevice *g, SDL_Texture *t)
{
	if (SDL_UpdateTexture(
		t, NULL, g->buf, g->cachedConfig.Res.x * sizeof(Uint32)) != 0)
	{
		LOG(LM_GFX, LL_ERROR, "cannot update texture: %s", SDL_GetError());
	}
}
