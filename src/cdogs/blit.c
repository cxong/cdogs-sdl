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

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <SDL.h>

#include "config.h"
#include "grafx.h"
#include "events.h"
#include "pics.h" /* for gPalette */
#include "utils.h" /* for debug() */

unsigned char *r_screen;


int clipleft = 0, cliptop = 0, clipright = 0, clipbottom = 0;

//this function is referenced by 4 macros, that do all the args

void Blit(int x, int y, Pic *pic, void *table, int mode)
{
	int yoff, xoff;
	unsigned char *current = pic->data;
	unsigned char *target;
	unsigned char *xlate = (unsigned char *) table;

	int i;

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
			target = r_screen + yoff + xoff;
			if ((mode & BLIT_TRANSPARENT && *current) || !(mode & BLIT_TRANSPARENT)){
				if (table){
					if (mode & BLIT_BACKGROUND)
						*target = xlate[*target];
					else
						*target = xlate[*current];
				}
				else
					*target = *current;
			}
			current++;
		}
	}
	return;
}

void CDogsSetClip(int left, int top, int right, int bottom)
{
	clipright = right;
	clipleft = left;
	cliptop = top;
	clipbottom = bottom;
	return;
}

void SetDstScreen(unsigned char *screen)
{
	r_screen = screen;
	return;
}

unsigned char *GetDstScreen(void)
{
	return r_screen;
}

#define PixelIndex(x, y, w)		(y * w + x)

static INLINE
void Scale8(char unsigned *d, const unsigned char *s, const int w, const int h,
	    const int sf)
{
	int sx;
	int sy;
	int f = sf;

	int dx, dy, dw;
	char p;

	if (f > 4) f = 4;	/* max 4x for the moment */

	dw = w * f;

	for (sy = 0; sy < h; sy++) {
		dy = f * sy;
		for (sx = 0; sx < w; sx++) {
			p = s[PixelIndex(sx, sy, w)];
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

void CopyToScreen(void)
{
	unsigned char *pScreen = gGraphicsDevice.screen->pixels;
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
		memcpy(pScreen, r_screen, scr_size);	/* 1 -> 1 */
	else {
		Scale8(pScreen, r_screen, scr_w, scr_h, scalef);
	}

	SDL_UnlockSurface(gGraphicsDevice.screen);
	SDL_Flip(gGraphicsDevice.screen);
}

void AltScrCopy(void)
{
	CopyToScreen();
	return;
}

#define GAMMA	3

#define GAMMA_R	GAMMA
#define GAMMA_G	GAMMA
#define GAMMA_B	GAMMA

void CDogsSetPalette(void *pal)
{
	color_t *palette = (color_t *)pal;
	SDL_Color newpal[256];
	int i;

	for (i = 0; i < 256; i++) {
		newpal[i].r = palette[i].red	* GAMMA_R;
		newpal[i].g = palette[i].green	* GAMMA_G;
		newpal[i].b = palette[i].blue	* GAMMA_B;

		newpal[i].unused = 0;
	}
	SDL_SetPalette(gGraphicsDevice.screen, SDL_PHYSPAL, newpal, 0, 256);
	return;
}

int BlitGetBrightness(void)
{
	return gConfig.Graphics.Brightness;
}
void BlitSetBrightness(int brightness)
{
	if (brightness >= -10 && brightness <= 10)
	{
		int i;
		double f;
		f = 1.0 + brightness / 33.3;
		for (i = 0; i < 255; i++)
		{
			gPalette[i].red = (unsigned char)CLAMP(f * origPalette[i].red, 0, 254);
			gPalette[i].green = (unsigned char)CLAMP(f * origPalette[i].green, 0, 254);
			gPalette[i].blue = (unsigned char)CLAMP(f * origPalette[i].blue, 0, 254);
		}
		CDogsSetPalette(gPalette);
	}
}
