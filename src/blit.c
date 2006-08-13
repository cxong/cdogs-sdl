/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (C) 1995 Ronny Wester
    Copyright (C) 2003 Jeremy Chin 
    Copyright (C) 2003 Lucas Martin-King 

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

*/

#include "SDL.h"
#include <string.h>
#include <math.h>
#include "blit.h"
#include "grafx.h"
#include "events.h"

char *r_screen;
extern SDL_Surface *screen;

int clipleft = 0, cliptop = 0, clipright = 319, clipbottom = 199;

//this function is referenced by 4 macros, that do all the args

void Blit(int x, int y, void *pic, void *table, int mode) {

	int height = PicHeight(pic);
	int width = PicWidth(pic);
	int yoff, xoff;
	unsigned char *current = pic;
	current += 4;
	unsigned char *target;
	unsigned char *xlate = (char *) table;

	int i;

	for (i = 0; i < height; i++) {
		int j;
	
		yoff = i + y;
		if (yoff > clipbottom)
			break;
		if (yoff < cliptop){
			current += width;
			continue;
		}
		yoff *= 320;
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

void CopyToScreen(void)
{
	char *pScreen = screen->pixels;
	int width, height;
	float scalex, scaley;
	int x, y;
	int yoff, yoff2;
	
	if (IsEventPending(EVENT_QUIT)) {
		printf("QUIT EVENT!\n");
		exit(0);
	}
	
	if (SDL_LockSurface(screen) == -1) {
		printf("Couldn't lock surface; not drawing\n");
		return;
	}
	if (screen->w == 320 && screen->h == 200)
		memcpy(pScreen, r_screen, 64000);	//320*200
	else {
		int i;
	
		width = screen->w;
		height = screen->h;
		scalex = 320.0f/width;
		scaley = 200.0f/height;
		for (i = 0; i < height; i++) {
			int j;
			y = i * scaley;
			yoff = width*i;
			yoff2 = y*320;
			for (j = 0; j < width; j++) {
				x = j * scalex;
				pScreen[yoff + j] = r_screen[x + yoff2];
			}
		}
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
