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

-------------------------------------------------------------------------------

 blit.c - screen blitter 
 
 Author: $Author$
 Rev:    $Revision$
 URL:    $HeadURL$
 ID:     $Id$
 
*/

#include "SDL.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "blit.h"
#include "grafx.h"
#include "events.h"
#include "pics.h" /* for gPalette */
#include "utils.h" /* for debug() */

unsigned char *r_screen;
extern SDL_Surface *screen;


int clipleft = 0, cliptop = 0, clipright = 0, clipbottom = 0;

//this function is referenced by 4 macros, that do all the args

void Blit(int x, int y, void *pic, void *table, int mode) {
	int height = PicHeight(pic);
	int width = PicWidth(pic);
	int yoff, xoff;
	unsigned char *current = pic;
	unsigned char *target;
	unsigned char *xlate = (unsigned char *) table;

	int i;

	current += 4;

	for (i = 0; i < height; i++) {
		int j;
	
		yoff = i + y;
		if (yoff > clipbottom)
			break;
		if (yoff < cliptop){
			current += width;
			continue;
		}
		yoff *= SCREEN_WIDTH;
		for (j = 0; j < width; j++) {
			xoff = j + x;
			if (xoff < clipleft){
				current++;
				continue;
			}
			if (xoff > clipright){
				current += width - j;
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

void SetClip(int left, int top, int right, int bottom)
{
	clipright = right;
	clipleft = left;
	cliptop = top;
	clipbottom = bottom;
	return;
}

void SetDstScreen(void *the_screen)
{
	r_screen = the_screen;
	return;
}

void *GetDstScreen(void)
{
	return r_screen;
}

#define PixelIndex(x, y, w, h)		(y * w + x)

#ifdef _MSC_VER
#define inline __inline
#endif

static inline
void Scale8(char unsigned *d, const unsigned char *s, const int w, const int h, const int sf)
{
	int sx;
	int sy;
	int f = sf;
	
	int dx, dy, dw, dh;
	char p;
	
	if (f > 4) f = 4;	/* max 4x for the moment */
	 
	dw = w * f;
	dh = h * f;
	
	for (sy = 0; sy < h; sy++) {
		dy = f * sy;
		for (sx = 0; sx < w; sx++) {
			p = s[PixelIndex(sx, sy, w, h)];
			dx = f * sx;

			switch (f) {
				case 4:
					/* right side */
					d[PixelIndex((dx + 3),	(dy + 1),	dw, dh)] = p;
					d[PixelIndex((dx + 3),	(dy + 2),	dw, dh)] = p;
					d[PixelIndex((dx + 3),	dy,			dw, dh)] = p;

					/* bottom row */
					d[PixelIndex(dx,		(dy + 3),	dw, dh)] = p;
					d[PixelIndex((dx + 1),	(dy + 3),	dw, dh)] = p;
					d[PixelIndex((dx + 2),	(dy + 3),	dw, dh)] = p;

					/* bottom right */
					d[PixelIndex((dx + 3),	(dy + 3),	dw, dh)] = p;

				case 3:
					/* right side */
					d[PixelIndex((dx + 2),	(dy + 1),	dw, dh)] = p;
					d[PixelIndex((dx + 2),	dy,			dw, dh)] = p;

					/* bottom row */
					d[PixelIndex(dx,		(dy + 2),	dw, dh)] = p;
					d[PixelIndex((dx + 1),	(dy + 2),	dw, dh)] = p;

					/* bottom right */
					d[PixelIndex((dx + 2),	(dy + 2),	dw, dh)] = p;


				case 2:
					d[PixelIndex((dx + 1),	dy,			dw, dh)] = p;
					d[PixelIndex((dx + 1),	(dy + 1),	dw, dh)] = p;
					d[PixelIndex(dx,		(dy + 1),	dw, dh)] = p;

				default:
					d[PixelIndex(dx,		dy,			dw, dh)] = p;
			}
		}
	}
}

void CopyToScreen(void)
{
	unsigned char *pScreen = screen->pixels;	
	int scr_w, scr_h, scr_size, scalef;
	
	scr_w = Screen_GetWidth();
	scr_h = Screen_GetHeight();
	scr_size = Screen_GetMemSize();
	scalef = Gfx_GetHint(HINT_SCALEFACTOR);

	/* this really needs to go someplace nicer,
	 * as it's a bit of a hack, being here. */
	if (IsEventPending(EVENT_QUIT)) {
		debug("QUIT EVENT!\n");
		exit(EXIT_SUCCESS);
	} else if (IsEventPending(EVENT_ACTIVE)) {
		/* Set the palette, just in case we had a change of focus
		 * and we don't things to go trippy for the player */
		debug("ACTIVE EVENT!\n");
		SetPalette(gPalette);
	}
	
	if (SDL_LockSurface(screen) == -1) {
		printf("Couldn't lock surface; not drawing\n");
		return;
	}
	
	if (scalef == 1)
		memcpy(pScreen, r_screen, scr_size);	/* 1 -> 1 */
	else {
		Scale8(pScreen, r_screen, scr_w, scr_h, scalef);
	}
	
	SDL_UnlockSurface(screen);
	SDL_Flip(screen);
	return;
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

void SetPalette(void *pal)
{
	color *palette = (color *) pal;
	SDL_Color newpal[256];
	int i;
	
	for (i = 0; i < 256; i++) {
		newpal[i].r = palette[i].red	* GAMMA_R;
		newpal[i].g = palette[i].green	* GAMMA_G;
		newpal[i].b = palette[i].blue	* GAMMA_B;

		newpal[i].unused = 0;
	}
	SDL_SetPalette(screen, SDL_PHYSPAL, newpal, 0, 256);
	return;
}
