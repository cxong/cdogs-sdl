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

    Copyright (c) 2013, Cong Xu
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
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <SDL.h>

#include "config.h"
#include "grafx.h"
#include "events.h"
#include "pics.h" /* for gPalette */
#include "utils.h" /* for debug() */


int clipleft = 0, cliptop = 0, clipright = 0, clipbottom = 0;

//this function is referenced by 4 macros, that do all the args

void Blit(int x, int y, Pic *pic, void *table, int mode)
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
		if (yoff > clipbottom)
			break;
		if (yoff < cliptop)
		{
			current += pic->w;
			continue;
		}
		yoff *= gGraphicsDevice.cachedConfig.ResolutionWidth;
		for (j = 0; j < pic->w; j++)
		{
			xoff = j + x;
			if (xoff < clipleft){
				current++;
				continue;
			}
			if (xoff > clipright)
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

void BlitBackground(int x, int y, Pic *pic, HSV *tint, int mode)
{
	int yoff, xoff;
	unsigned char *current = pic->data;

	int i;

	assert(mode & BLIT_BACKGROUND);

	for (i = 0; i < pic->h; i++)
	{
		int j;

		yoff = i + y;
		if (yoff > clipbottom)
			break;
		if (yoff < cliptop)
		{
			current += pic->w;
			continue;
		}
		yoff *= gGraphicsDevice.cachedConfig.ResolutionWidth;
		for (j = 0; j < pic->w; j++)
		{
			xoff = j + x;
			if (xoff < clipleft){
				current++;
				continue;
			}
			if (xoff > clipright)
			{
				current += pic->w - j;
				break;
			}
			if ((mode & BLIT_TRANSPARENT && *current) || !(mode & BLIT_TRANSPARENT))
			{
				Uint32 *target = gGraphicsDevice.buf + yoff + xoff;
				if (tint != NULL)
				{
					color_t targetColor, blendedColor;
					SDL_GetRGB(
						*target,
						gGraphicsDevice.screen->format,
						&targetColor.r, &targetColor.g, &targetColor.b);
					blendedColor = ColorTint(targetColor, *tint);
					*target = SDL_MapRGB(
						gGraphicsDevice.screen->format,
						blendedColor.r, blendedColor.g, blendedColor.b);
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

void BlitRectangle(
	Uint32 *screen,
	int left, int top,
	int width, int height,
	unsigned char color,
	int flags)
{
	int y;
	Uint32 rgbColor = LookupPalette(color);
	if (width < 3 || height < 3)
	{
		flags &= ~BLIT_FLAG_ROUNDED;
	}
	for (y = top; y < top + height; y++)
	{
		int isFirstOrLastLine = y == top || y == top + height - 1;
		if (isFirstOrLastLine && (flags & BLIT_FLAG_ROUNDED))
		{
			memset(
				screen + left + 1 + y*gGraphicsDevice.cachedConfig.ResolutionWidth,
				rgbColor,
				(width - 2) * sizeof *screen);
		}
		else if (!isFirstOrLastLine && (flags & BLIT_FLAG_LINE))
		{
			*(screen + left + y*gGraphicsDevice.cachedConfig.ResolutionWidth) =
				rgbColor;
			*(screen + left + width - 1 + y*gGraphicsDevice.cachedConfig.ResolutionWidth) =
				rgbColor;
		}
		else
		{
			memset(
				screen + left + y*gGraphicsDevice.cachedConfig.ResolutionWidth,
				rgbColor,
				width * sizeof *screen);
		}
	}
}

//  *
// ***
//  *
void BlitCross(Uint32 *screen, int x, int y, unsigned char color)
{
	Uint32 rgbColor = LookupPalette(color);
	screen += x;
	screen += y * gGraphicsDevice.cachedConfig.ResolutionWidth;
	*screen = rgbColor;
	*(screen - 1) = rgbColor;
	*(screen + 1) = rgbColor;
	*(screen - gGraphicsDevice.cachedConfig.ResolutionWidth) = rgbColor;
	*(screen + gGraphicsDevice.cachedConfig.ResolutionWidth) = rgbColor;
}

void CDogsSetClip(int left, int top, int right, int bottom)
{
	clipright = right;
	clipleft = left;
	cliptop = top;
	clipbottom = bottom;
	return;
}

static TPalette gCurrentPalette;
#define GAMMA 3
Uint32 LookupPalette(unsigned char index)
{
	color_t color = gCurrentPalette[index];
	color.r = (Uint8)CLAMP(color.r * GAMMA, 0, 255);
	color.g = (Uint8)CLAMP(color.g * GAMMA, 0, 255);
	color.b = (Uint8)CLAMP(color.b * GAMMA, 0, 255);
	return SDL_MapRGB(gGraphicsDevice.screen->format, color.r, color.g, color.b);
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

void CopyToScreen(void)
{
	Uint32 *pScreen = (Uint32 *)gGraphicsDevice.screen->pixels;
	int scr_w, scr_h, scr_size, scalef;

	scr_w = gGraphicsDevice.cachedConfig.ResolutionWidth;
	scr_h = gGraphicsDevice.cachedConfig.ResolutionHeight;
	scr_size = scr_w * scr_h;
	scalef = gConfig.Graphics.ScaleFactor;

	/* this really needs to go someplace nicer,
	 * as it's a bit of a hack, being here. */
	if (IsEventPending(EVENT_QUIT)) {
		debug(D_NORMAL, "QUIT EVENT!\n");
		exit(EXIT_SUCCESS);
	} else if (IsEventPending(EVENT_ACTIVE)) {
		/* Set the palette, just in case we had a change of focus
		 * and we don't things to go trippy for the player */
		debug(D_NORMAL, "ACTIVE EVENT!\n");
		CDogsSetPalette(gPalette);
	}

	if (SDL_LockSurface(gGraphicsDevice.screen) == -1)
	{
		printf("Couldn't lock surface; not drawing\n");
		return;
	}

	if (scalef == 1)
	{
		memcpy(pScreen, gGraphicsDevice.buf, sizeof *pScreen * scr_w * scr_h);
	}
	else if (gConfig.Graphics.ScaleMode == SCALE_MODE_BILINEAR)
	{
		Bilinear(pScreen, gGraphicsDevice.buf, scr_w, scr_h, scalef);
	}
	else
	{
		Scale8(pScreen, gGraphicsDevice.buf, scr_w, scr_h, scalef);
	}

	SDL_UnlockSurface(gGraphicsDevice.screen);
	SDL_Flip(gGraphicsDevice.screen);
}

void CDogsSetPalette(TPalette palette)
{
	memcpy(gCurrentPalette, palette, sizeof gCurrentPalette);
}

int BlitGetBrightness(void)
{
	return gConfig.Graphics.Brightness;
}
void BlitSetBrightness(int brightness)
{
	int i;
	double f;
	if (brightness < BLIT_BRIGHTNESS_MIN || brightness > BLIT_BRIGHTNESS_MAX)
	{
		return;
	}

	f = pow(1.07177346254, brightness);	// 10th root of 2; i.e. n^10 = 2
	for (i = 0; i < 255; i++)
	{
		gPalette[i].r = (unsigned char)CLAMP(f * origPalette[i].r, 0, 255);
		gPalette[i].g = (unsigned char)CLAMP(f * origPalette[i].g, 0, 255);
		gPalette[i].b = (unsigned char)CLAMP(f * origPalette[i].b, 0, 255);
	}
	CDogsSetPalette(gPalette);
}
