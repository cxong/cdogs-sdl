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
#include "blit.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <hqx.h>
#include <SDL.h>

#include "config.h"
#include "grafx.h"
#include "palette.h"
#include "utils.h" /* for debug() */


color_t PixelToColor(GraphicsDevice *device, Uint32 pixel)
{
	SDL_PixelFormat *f = device->screen->format;
	color_t c;
	SDL_GetRGB(pixel, f, &c.r, &c.g, &c.b);
	// Manually apply the alpha as SDL seems to always set it to 0
	c.a = (Uint8)((pixel & ~(f->Rmask | f->Gmask | f->Bmask)) >> device->Ashift);
	return c;
}
Uint32 PixelFromColor(GraphicsDevice *device, color_t color)
{
	SDL_PixelFormat *f = device->screen->format;
	Uint32 pixel = SDL_MapRGBA(f, color.r, color.g, color.b, color.a);
	// Manually apply the alpha as SDL seems to always set it to 0
	return (pixel & (f->Rmask | f->Gmask | f->Bmask)) | (color.a << device->Ashift);
}

void BlitOld(int x, int y, PicPaletted *pic, void *table, int mode)
{
	int yoff, xoff;
	unsigned char *current = pic->data;
	unsigned char *xlate = (unsigned char *)table;

	int i;

	assert(!(mode & BLIT_BACKGROUND));

	for (i = 0; i < pic->h; i++)
	{
		int j;

		yoff = i + y;
		if (yoff > gGraphicsDevice.clipping.bottom)
		{
			break;
		}
		if (yoff < gGraphicsDevice.clipping.top)
		{
			current += pic->w;
			continue;
		}
		yoff *= gGraphicsDevice.cachedConfig.ResolutionWidth;
		for (j = 0; j < pic->w; j++)
		{
			xoff = j + x;
			if (xoff < gGraphicsDevice.clipping.left)
			{
				current++;
				continue;
			}
			if (xoff > gGraphicsDevice.clipping.right)
			{
				current += pic->w - j;
				break;
			}
			if ((mode & BLIT_TRANSPARENT && *current) || !(mode & BLIT_TRANSPARENT))
			{
				Uint32 *target = gGraphicsDevice.buf + yoff + xoff;
				if (table != NULL)
				{
					*target = LookupPalette(xlate[*current]);
				}
				else
				{
					*target = LookupPalette(*current);
				}
			}
			current++;
		}
	}
}

void BlitPicHighlight(GraphicsDevice *g, Pic *pic, Vec2i pos, color_t color)
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
		yoff *= g->cachedConfig.ResolutionWidth;
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
				!PixelToColor(g, *(pic->Data + j + i * pic->size.x)).a;
			if (isPixelEmpty)
			{
				bool isLeft =
					j > 0 && !isTopOrBottomEdge &&
					PixelToColor(g, *(pic->Data + j - 1 + i * pic->size.x)).a;
				bool isRight =
					j < pic->size.x - 1 && !isTopOrBottomEdge &&
					PixelToColor(g, *(pic->Data + j + 1 + i * pic->size.x)).a;
				bool isAbove =
					i > 0 && !isLeftOrRightEdge &&
					PixelToColor(g, *(pic->Data + j + (i - 1) * pic->size.x)).a;
				bool isBelow =
					i < pic->size.y - 1 && !isLeftOrRightEdge &&
					PixelToColor(g, *(pic->Data + j + (i + 1) * pic->size.x)).a;
				if (isLeft || isRight || isAbove || isBelow)
				{
					Uint32 *target = g->buf + yoff + xoff;
					color_t targetColor = PixelToColor(g, *target);
					color_t blendedColor = ColorAlphaBlend(
						targetColor, color);
					*target = PixelFromColor(g, blendedColor);
				}
			}
		}
	}
}

void BlitBackground(int x, int y, PicPaletted *pic, HSV *tint, int mode)
{
	int yoff, xoff;
	unsigned char *current = pic->data;

	int i;

	assert(mode & BLIT_BACKGROUND);

	for (i = 0; i < pic->h; i++)
	{
		int j;

		yoff = i + y;
		if (yoff > gGraphicsDevice.clipping.bottom)
		{
			break;
		}
		if (yoff < gGraphicsDevice.clipping.top)
		{
			current += pic->w;
			continue;
		}
		yoff *= gGraphicsDevice.cachedConfig.ResolutionWidth;
		for (j = 0; j < pic->w; j++)
		{
			xoff = j + x;
			if (xoff < gGraphicsDevice.clipping.left)
			{
				current++;
				continue;
			}
			if (xoff > gGraphicsDevice.clipping.right)
			{
				current += pic->w - j;
				break;
			}
			if ((mode & BLIT_TRANSPARENT && *current) || !(mode & BLIT_TRANSPARENT))
			{
				Uint32 *target = gGraphicsDevice.buf + yoff + xoff;
				if (tint != NULL)
				{
					color_t targetColor =
						PixelToColor(&gGraphicsDevice, *target);
					color_t blendedColor = ColorTint(targetColor, *tint);
					*target = PixelFromColor(&gGraphicsDevice, blendedColor);
				}
				else
				{
					*target = LookupPalette(*current);
				}
			}
			current++;
		}
	}
}

void Blit(GraphicsDevice *device, Pic *pic, Vec2i pos)
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
		yoff *= device->cachedConfig.ResolutionWidth;
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
			if ((*current & device->Amask) == 0)
			{
				/*
				// hack to blit transparent background as hot pink
				target = device->buf + yoff + xoff;
				*target = PixelFromColor(device, colorMagenta);
				*/
				current++;
				continue;
			}
			target = device->buf + yoff + xoff;
			*target = *current;
			current++;
		}
	}
	/*
	// hack to blit boundary
	for (int i = -1; i < pic->size.y + 1; i++)
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
		yoff *= device->cachedConfig.ResolutionWidth;
		for (int j = -1; j < pic->size.x + 1; j++)
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
			if (i != -1 && i != pic->size.y && j != -1 && j != pic->size.x)
			{
				current++;
				continue;
			}
			target = device->buf + yoff + xoff;
			*target = PixelFromColor(device, colorWhite);
			current++;
		}
	}
	*/
}

Uint32 PixelMult(Uint32 p, Uint32 m)
{
	return
		((p & 0xFF) * (m & 0xFF) / 0xFF) |
		((((p & 0xFF00) >> 8) * ((m & 0xFF00) >> 8) / 0xFF) << 8) |
		((((p & 0xFF0000) >> 16) * ((m & 0xFF0000) >> 16) / 0xFF) << 16) |
		((((p & 0xFF000000) >> 24) * ((m & 0xFF000000) >> 24) / 0xFF) << 24);
}
void BlitMasked(
	GraphicsDevice *device,
	Pic *pic,
	Vec2i pos,
	color_t mask,
	int isTransparent)
{
	Uint32 *current = pic->Data;
	Uint32 maskPixel = PixelFromColor(device, mask);
	int i;
	pos = Vec2iAdd(pos, pic->offset);
	for (i = 0; i < pic->size.y; i++)
	{
		int j;
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
		yoff *= device->cachedConfig.ResolutionWidth;
		for (j = 0; j < pic->size.x; j++)
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
			if (isTransparent && *current == 0)
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
void BlitBlend(GraphicsDevice *g, Pic *pic, Vec2i pos, color_t blend)
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
		yoff *= g->cachedConfig.ResolutionWidth;
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
			color_t currentColor = PixelToColor(g, *current);
			color_t blendedColor = ColorMult(
				currentColor, blend);
			blendedColor.a = blend.a;
			color_t targetColor = PixelToColor(g, *target);
			blendedColor = ColorAlphaBlend(targetColor, blendedColor);
			*target = PixelFromColor(g, blendedColor);
			current++;
		}
	}
}

#define PixelIndex(x, y, w)		(y * w + x)

static INLINE
void Scale8(Uint32 *d, const Uint32 *s, const int w, const int h, const int sf)
{
	int sx;
	int sy;
	int f = sf;

	int dx, dy, dw;

	if (f > 4)
	{
		f = 4;	/* max 4x for the moment */
	}

	dw = w * f;

	for (sy = 0; sy < h; sy++) {
		dy = f * sy;
		for (sx = 0; sx < w; sx++)
		{
			Uint32 p = s[PixelIndex(sx, sy, w)];
			dx = f * sx;

			switch (f) {
				case 4:
					/* right side */
					d[PixelIndex((dx + 3),	(dy + 1),	dw)] = p;
					d[PixelIndex((dx + 3),	(dy + 2),	dw)] = p;
					d[PixelIndex((dx + 3),	dy,		dw)] = p;

					/* bottom row */
					d[PixelIndex(dx,	(dy + 3),	dw)] = p;
					d[PixelIndex((dx + 1),	(dy + 3),	dw)] = p;
					d[PixelIndex((dx + 2),	(dy + 3),	dw)] = p;

					/* bottom right */
					d[PixelIndex((dx + 3),	(dy + 3),	dw)] = p;

				case 3:
					/* right side */
					d[PixelIndex((dx + 2),	(dy + 1),	dw)] = p;
					d[PixelIndex((dx + 2),	dy,		dw)] = p;

					/* bottom row */
					d[PixelIndex(dx,	(dy + 2),	dw)] = p;
					d[PixelIndex((dx + 1),	(dy + 2),	dw)] = p;

					/* bottom right */
					d[PixelIndex((dx + 2),	(dy + 2),	dw)] = p;

				case 2:
					d[PixelIndex((dx + 1),	dy,		dw)] = p;
					d[PixelIndex((dx + 1),	(dy + 1),	dw)] = p;
					d[PixelIndex(dx,	(dy + 1),	dw)] = p;

				default:
					d[PixelIndex(dx,	dy,		dw)] = p;
			}
		}
	}
}

static Uint32 PixAvg(Uint32 p1, Uint32 p2)
{
	union
	{
		Uint8 rgba[4];
		Uint32 out;
	} u1, u2;
	int i;
	u1.out = p1;
	u2.out = p2;
	for (i = 0; i < 4; i++)
	{
		u1.rgba[i] = (Uint8)CLAMP(((int)u1.rgba[i] + u2.rgba[i]) / 2, 0, 255);
	}
	return u1.out;
}
static Uint32 Pix3rds(Uint32 p1, Uint32 p2)
{
	union
	{
		Uint8 rgba[4];
		Uint32 out;
	} u1, u2;
	int i;
	u1.out = p1;
	u2.out = p2;
	for (i = 0; i < 4; i++)
	{
		u1.rgba[i] = (Uint8)CLAMP(((int)u1.rgba[i] + u2.rgba[i]*2) / 3, 0, 255);
	}
	return u1.out;
}
static void Bilinear(
	Uint32 *dest, const Uint32 *src,
	const int w, const int h,
	const int scaleFactor)
{
	int sx, sy;
	int dw = scaleFactor * w;
	for (sy = 0; sy < h; sy++)
	{
		int dy = scaleFactor * sy;
		for (sx = 0; sx < w; sx++)
		{
			Uint32 p = src[PixelIndex(sx, sy, w)];
			int dx = scaleFactor * sx;
			switch (scaleFactor)
			{
			#define BLIT(x, y, pix) dest[PixelIndex((x), (y), dw)] = pix;
			case 4:
				{
					// 0 1 2 3|g
					// 4 5 6 7|h
					// 8 9 a b|i
					// c d e f|j
					// k-l-m-n+o
					Uint32 pg = src[PixelIndex(MIN(sx+1, w-1), sy, w)];
					Uint32 p2 = PixAvg(p, pg);
					Uint32 p1 = PixAvg(p, p2);
					Uint32 p3 = PixAvg(p2, pg);
					Uint32 pk = src[PixelIndex(sx, MIN(sy+1, h-1), w)];
					Uint32 p8 = PixAvg(p, pk);
					Uint32 p4 = PixAvg(p, p8);
					Uint32 pc = PixAvg(p8, pk);
					Uint32 po = src[PixelIndex(MIN(sx+1, w-1), MIN(sy+1, h-1), w)];
					Uint32 pi = PixAvg(pg, po);
					Uint32 pa = PixAvg(p8, pi);
					Uint32 p9 = PixAvg(p8, pa);
					Uint32 pb = PixAvg(pa, pi);
					Uint32 p6 = PixAvg(p2, pa);
					Uint32 p5 = PixAvg(p4, p6);
					Uint32 pm = PixAvg(pk, po);
					Uint32 pe = PixAvg(pa, pm);
					Uint32 ph = PixAvg(pg, pi);
					Uint32 p7 = PixAvg(p6, ph);
					Uint32 pj = PixAvg(pi, po);
					Uint32 pd = PixAvg(pc, pe);
					Uint32 pf = PixAvg(pe, pj);
					BLIT(dx, dy, p);
					BLIT(dx+1, dy, p1);
					BLIT(dx+2, dy, p2);
					BLIT(dx+3, dy, p3);
					BLIT(dx, dy+1, p4);
					BLIT(dx+1, dy+1, p5);
					BLIT(dx+2, dy+1, p6);
					BLIT(dx+3, dy+1, p7);
					BLIT(dx, dy+2, p8);
					BLIT(dx+1, dy+2, p9);
					BLIT(dx+2, dy+2, pa);
					BLIT(dx+3, dy+2, pb);
					BLIT(dx, dy+3, pc);
					BLIT(dx+1, dy+3, pd);
					BLIT(dx+2, dy+3, pe);
					BLIT(dx+3, dy+3, pf);
				}
				break;
			case 3:
				{
					// 0 1 2|9
					// 3 4 5|a
					// 6 7 8|b
					// c-d-e+f
					Uint32 p9 = src[PixelIndex(MIN(sx+1, w-1), sy, w)];
					Uint32 p1 = Pix3rds(p9, p);
					Uint32 p2 = Pix3rds(p, p9);
					Uint32 pc = src[PixelIndex(sx, MIN(sy+1, h-1), w)];
					Uint32 p3 = Pix3rds(pc, p);
					Uint32 p6 = Pix3rds(p, pc);
					Uint32 pf = src[PixelIndex(MIN(sx+1, w-1), MIN(sy+1, h-1), w)];
					Uint32 pa = Pix3rds(pf, p9);
					Uint32 pb = Pix3rds(p9, pf);
					Uint32 p4 = Pix3rds(pa, p3);
					Uint32 p5 = Pix3rds(p3, pa);
					Uint32 p7 = Pix3rds(pb, p6);
					Uint32 p8 = Pix3rds(p6, pb);
					BLIT(dx, dy, p);
					BLIT(dx+1, dy, p1);
					BLIT(dx+2, dy, p2);
					BLIT(dx, dy+1, p3);
					BLIT(dx+1, dy+1, p4);
					BLIT(dx+2, dy+1, p5);
					BLIT(dx, dy+2, p6);
					BLIT(dx+1, dy+2, p7);
					BLIT(dx+2, dy+2, p8);
				}
				break;
			case 2:
				{
					// 0 1|4
					// 2 3|5
					// 6-7+8
					Uint32 p4 = src[PixelIndex(MIN(sx+1, w-1), sy, w)];
					Uint32 p1 = PixAvg(p, p4);
					Uint32 p6 = src[PixelIndex(sx, MIN(sy+1, h-1), w)];
					Uint32 p2 = PixAvg(p, p6);
					Uint32 p8 = src[PixelIndex(MIN(sx+1, w-1), MIN(sy+1, h-1), w)];
					Uint32 p5 = PixAvg(p4, p8);
					Uint32 p3 = PixAvg(p2, p5);
					BLIT(dx, dy, p);
					BLIT(dx+1, dy, p1);
					BLIT(dx, dy+1, p2);
					BLIT(dx+1, dy+1, p3);
				}
				break;
			default:
				BLIT(dx, dy, p);
				break;
			#undef BLIT
			}
		}
	}
}

static void ApplyBrightness(Uint32 *screen, Vec2i screenSize, int brightness)
{
	double f = pow(1.07177346254, brightness);	// 10th root of 2; i.e. n^10 = 2
	int m = (int)(0xFF * f);
	int y;
	for (y = 0; y < screenSize.y; y++)
	{
		int x;
		for (x = 0; x < screenSize.x; x++)
		{
			// Semi-optimised pixel multiplcation routine
			// Multiply each 8-bit component with the gamma mask
			// Detect overflows by checking if there are bits above 255
			// If so, turning into boolean (!!) and negation will create
			// a -1 (i.e. FFFFFFFF) mask which fills all the bits with 1
			// i.e. saturated multiply
			int idx = x + y * screenSize.x;
			Uint32 p = screen[idx];
			Uint32 pp;
			screen[idx] = 0;
			pp = ((p & 0xFF) * m / 0xFF);
			screen[idx] |= (pp | -!!(pp >> 8)) & 0xFF;
			pp = (((p >> 8) & 0xFF) * m / 0xFF);
			screen[idx] |= ((pp | -!!(pp >> 8)) & 0xFF) << 8;
			pp = (((p >> 16) & 0xFF) * m / 0xFF);
			screen[idx] |= ((pp | -!!(pp >> 8)) & 0xFF) << 16;
			pp = (((p >> 24) & 0xFF) * m / 0xFF);
			screen[idx] |= ((pp | -!!(pp >> 8)) & 0xFF) << 24;
		}
	}
}

void BlitFlip(GraphicsDevice *device, GraphicsConfig *config)
{
	Uint32 *pScreen = (Uint32 *)device->screen->pixels;
	Vec2i screenSize = Vec2iNew(
		device->cachedConfig.ResolutionWidth,
		device->cachedConfig.ResolutionHeight);
	int scr_size = screenSize.x * screenSize.y;
	int scalef = config->ScaleFactor;

	ApplyBrightness(device->buf, screenSize, config->Brightness);

	if (SDL_LockSurface(device->screen) == -1)
	{
		printf("Couldn't lock surface; not drawing\n");
		return;
	}

	if (scalef == 1)
	{
		memcpy(pScreen, device->buf, sizeof *pScreen * scr_size);
	}
	else if (config->ScaleMode == SCALE_MODE_BILINEAR)
	{
		Bilinear(pScreen, device->buf, screenSize.x, screenSize.y, scalef);
	}
	else if (config->ScaleMode == SCALE_MODE_HQX)
	{
		switch (scalef)
		{
		case 2:
			hq2x_32(device->buf, pScreen, screenSize.x, screenSize.y);
			break;
		case 3:
			hq3x_32(device->buf, pScreen, screenSize.x, screenSize.y);
			break;
		case 4:
			hq4x_32(device->buf, pScreen, screenSize.x, screenSize.y);
			break;
		default:
			assert(0);
			break;
		}
	}
	else
	{
		Scale8(pScreen, device->buf, screenSize.x, screenSize.y, scalef);
	}

	SDL_UnlockSurface(device->screen);
	SDL_Flip(device->screen);
}
