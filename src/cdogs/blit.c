/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.
	Copyright (c) 2013-2020 Cong Xu
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
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <SDL.h>

#include "config.h"
#include "log.h"

color_t *CharColorGetByType(CharColors *c, const CharColorType t)
{
	switch (t)
	{
	case CHAR_COLOR_SKIN:
		return &c->Skin;
	case CHAR_COLOR_ARMS:
		return &c->Arms;
	case CHAR_COLOR_BODY:
		return &c->Body;
	case CHAR_COLOR_LEGS:
		return &c->Legs;
	case CHAR_COLOR_HAIR:
		return &c->Hair;
	case CHAR_COLOR_FEET:
		return &c->Feet;
	default:
		CASSERT(false, "Unexpected colour");
		return &c->Skin;
	}
}

// Character colors are embedded in pixels in two ways:
// - From source images, as color keys
//   - E.g. pure red represents skin. This either means
//     - G and B channel are near-zero and R channel is not, or
//     - G and B channel are very close but not max, and R channel is at max
//   - This is for ease of authoring
// - In-game, where colors are greyscale but alpha value is set to a special
//   value
//   - E.g. 254 alpha represents skin
// When character sprites are masked with character colors, it examines pixels
// with special alpha values and masks with the associated character color.
// Otherwise a plain mask is performed.

uint8_t CharColorTypeAlpha(const CharColorType t)
{
	static uint8_t alphas[] = {254, 253, 252, 251, 250, 249};
	if (t < CHAR_COLOR_COUNT)
	{
		return alphas[t];
	}
	return 255;
}
#define CHAR_COLOR_THRESHOLD 5
CharColorType CharColorTypeFromColor(const color_t c)
{
	if (abs((int)c.r - c.g) < CHAR_COLOR_THRESHOLD &&
		abs((int)c.g - c.b) < CHAR_COLOR_THRESHOLD &&
		abs((int)c.r - c.b) < CHAR_COLOR_THRESHOLD)
	{
		// don't convert greyscale colours
		return CHAR_COLOR_COUNT;
	}
	if ((c.g < 5 && c.b < 5) || (abs((int)c.g - c.b) < 5 && c.r > 250))
	{
		// Skin (R)
		return CHAR_COLOR_SKIN;
	}
	else if ((c.r < 5 && c.b < 5) || (abs((int)c.r - c.b) < 5 && c.g > 250))
	{
		// Hair (G)
		return CHAR_COLOR_HAIR;
	}
	else if ((c.r < 5 && c.g < 5) || (abs((int)c.r - c.g) < 5 && c.b > 250))
	{
		// Arms (B)
		return CHAR_COLOR_ARMS;
	}
	else if (c.b < 5 || (c.r > 250 && c.g > 250))
	{
		// Body (RG)
		return CHAR_COLOR_BODY;
	}
	else if (c.r < 5 || (c.g > 250 && c.b > 250))
	{
		// Legs (GB)
		return CHAR_COLOR_LEGS;
	}
	else if (c.g < 5 || (c.r > 250 && c.b > 250))
	{
		// Feet (RB)
		return CHAR_COLOR_FEET;
	}

	return CHAR_COLOR_COUNT;
}
CharColors CharColorsFromOneColor(const color_t color)
{
	CharColors c = {color, color, color, color, color, color};
	return c;
}
color_t CharColorsGetChannelMask(const CharColors *c, const uint8_t alpha)
{
	switch (alpha)
	{
	case 255:
		return colorWhite;
	case 254:
		return c->Skin;
	case 253:
		return c->Arms;
	case 252:
		return c->Body;
	case 251:
		return c->Legs;
	case 250:
		return c->Hair;
	case 249:
		return c->Feet;
	default:
		return colorWhite;
	}
}
void CharColorsGetMaskedName(char *buf, const char *base, const CharColors *c)
{
	char bufSkin[COLOR_STR_BUF], bufArms[COLOR_STR_BUF],
		bufBody[COLOR_STR_BUF], bufLegs[COLOR_STR_BUF], bufHair[COLOR_STR_BUF],
		bufFeet[COLOR_STR_BUF];
	ColorStr(bufSkin, c->Skin);
	ColorStr(bufArms, c->Arms);
	ColorStr(bufBody, c->Body);
	ColorStr(bufLegs, c->Legs);
	ColorStr(bufHair, c->Hair);
	ColorStr(bufFeet, c->Feet);
	sprintf(
		buf, "%s/%s/%s/%s/%s/%s/%s", base, bufSkin, bufArms, bufBody, bufLegs,
		bufHair, bufFeet);
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
