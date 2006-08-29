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

 grafx.c - graphics related functions 
 
 Author: $Author$
 Rev:    $Revision$
 URL:    $HeadURL$
 ID:     $Id$
 
*/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include "SDL.h"
#include "SDL_endian.h"

#include "defs.h"
#include "grafx.h"
#include "blit.h"
#include "sprcomp.h"
#include "files.h"
#include "utils.h"

typedef struct {
	unsigned int w, h;
} GFX_Mode;

GFX_Mode modelist[] = {
	{ 320, 200 },
	{ 320, 240 },
	{ 400, 300 },
	{ 640, 480 },
	{ 800, 600 }, /* things go strange above this... */
	{ 0, 0 },
};

int ValidMode(int w, int h)
{
	int i;
	
	for (i = 0; ; i++) {
		unsigned int m_w = modelist[i].w;
		unsigned int m_h = modelist[i].h;
	
		if (m_w == 0) return 0;
		if (m_w == w && m_h == h) return 1;
	}
	
	return 0;
}

int hints[HINT_END] = {
	0,		// HINT_FULLSCREEN
	1,		// HINT_WINDOW
	1,		// HINT_SCALEFACTOR
	320,	// HINT_WIDTH
	240		// HINT_HEIGHT
};

void Gfx_SetHint(const GFX_Hint h, const int val)
{
	if (h < 0 || h >= HINT_END) return;
	hints[h] = val;
}

//int Gfx_GetHint(const GFX_Hint h)
#define Hint(h)	hints[h]

int Gfx_GetHint(const GFX_Hint h)
{
	if (h < 0 || h >= HINT_END) return 0;
	return Hint(h);
}

SDL_Surface *screen = NULL;
/* probably not the best thing, but we need the performance */
int screen_w;
int screen_h; 

int InitVideo(int mode)
{	
	char title[32];
	SDL_Surface *new_screen = NULL;
	int sdl_flags = 0;
	int w, h = 0;
	int rw, rh;

	sdl_flags |= SDL_HWPALETTE;
	sdl_flags |= SDL_SWSURFACE;

	if (Hint(HINT_FULLSCREEN)) sdl_flags |= SDL_FULLSCREEN;

	rw = w = Hint(HINT_WIDTH);
	rh = h = Hint(HINT_HEIGHT);

	if (Hint(HINT_SCALEFACTOR) > 1) {
		rw *= Hint(HINT_SCALEFACTOR);
		rh *= Hint(HINT_SCALEFACTOR);
	}

	if (!Hint(HINT_FORCEMODE)) {
		if (!ValidMode(w, h)) {
			printf("!!! Invalid Video Mode %dx%d\n", w, h);
			return -1;
		}
	} else {
		printf("\n");
		printf("  BIG FAT WARNING: If this blows up in your face,\n");
		printf("  and mutilates your cat, please don't cry.\n");
		printf("\n");
	}

	printf("Window dimentions:\t%dx%d\n", rw, rh);
	new_screen = SDL_SetVideoMode(rw, rh, 8, sdl_flags);

	if (new_screen == NULL) {
		printf("ERROR: InitVideo: %s\n", SDL_GetError() );
		return -1;
	}	

	if (screen == NULL) { /* only do this the first time */
		debug("setting caption and icon...\n");
		sprintf(title, "C-Dogs %s [Port %s]", CDOGS_VERSION, CDOGS_SDL_VERSION);
		SDL_WM_SetCaption(title, NULL);
		SDL_WM_SetIcon(SDL_LoadBMP(GetDataFilePath("cdogs_icon.bmp")), NULL);
	} else {
		printf("Changed video mode...\n");
	}
	
	screen = new_screen;
	
	screen_w = Hint(HINT_WIDTH);
	screen_h = Hint(HINT_HEIGHT);
	
	SetClip(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);
	printf("Internal dimentions:\t%dx%d\n", SCREEN_WIDTH, SCREEN_HEIGHT);
			
	return 0;
}

int ReadPics(const char *filename, void **pics, int maxPics,
	     color * palette)
{
	int f;
	int eof = 0;
	unsigned short int size;
	int i = 0;

	f = fopen(filename, "rb");
	if (f != NULL) {
		if (palette)
			fread(palette, sizeof(TPalette), 1, f);
		else
			fseek(f, sizeof(TPalette), SEEK_CUR);
			
		while (!eof && i < maxPics) {
			fread(&size, sizeof(size), 1, f);
			swap16(&size);
			if (size) {
				pics[i] = sys_mem_alloc(size);
				
				fread(pics[i] + 0, 1, 2, f);
				swap16(pics[i] + 0);
				fread(pics[i] + 2, 1, 2, f);
				swap16(pics[i] + 2);

				fread(pics[i] + 4, 1, size - 4, f);
				
				if (ferror(f) || feof(f))
					eof = 1;
			} else
				pics[i] = NULL;
			i++;
		}
		fclose(f);
	}
	return i;
}

int AppendPics(const char *filename, void **pics, int startIndex,
	       int maxPics)
{
	int f;
	int eof = 0;
	unsigned short int size;
	int i = startIndex;
	
	f = fopen(filename, "rb");
	if (f != NULL) {
		fseek(f, sizeof(TPalette), SEEK_CUR);
			
		while (!eof && i < maxPics) {
			fread(&size, sizeof(size), 1, f);
			swap16(&size);
			if (size) {
				pics[i] = sys_mem_alloc(size);
				
				fread(pics[i] + 0, 1, 2, f);
				swap16(pics[i] + 0);
				fread(pics[i] + 2, 1, 2, f);
				swap16(pics[i] + 2);

				fread(pics[i] + 4, 1, size - 4, f);
				
				if (ferror(f) || feof(f))
					eof = 1;
			} else
				pics[i] = NULL;
			i++;
		}
		fclose(f);
	}
	
	return i - startIndex;
}

int CompilePics(int picCount, void **pics, void **compiledPics)
{
	int i, size, total = 0;
	int skipped = 0;

	for (i = 0; i < picCount; i++) {
		if (pics[i]) {
			size = compileSprite(pics[i], NULL);
			total += size;
			if (size) {
				compiledPics[i] = sys_mem_alloc(size);
				compileSprite(pics[i], compiledPics[i]);
			} else {
				compiledPics[i] = NULL;
				skipped++;
			}
		} else
			compiledPics[i] = NULL;
	}
	if (skipped)
		printf("%d solid pics not compiled\n", skipped);
	return total;
}

int RLEncodePics(int picCount, void **pics, void **rlePics)
{
	int i, size, total = 0;
	int skipped = 0;

	for (i = 0; i < picCount; i++) {
		if (pics[i]) {
			size = RLEncodeSprite(pics[i], NULL);
			total += size;
			if (size) {
				rlePics[i] = sys_mem_alloc(size);
				RLEncodeSprite(pics[i], rlePics[i]);
			} else {
				rlePics[i] = NULL;
				skipped++;
			}
		} else
			rlePics[i] = NULL;
	}
	if (skipped)
		printf("%d solid pics not RLE'd\n", skipped);
	return total;
}

void vsync(void)
{
	return;
}

inline int PicWidth(void *pic)
{
	if (!pic)
		return 0;
	return ((short *) pic)[0];
}

inline int PicHeight(void *pic)
{
	if (!pic)
		return 0;
	return ((short *) pic)[1];
}

void SetColorZero(int r, int g, int b)
{
	SDL_Color col;
	col.r = r;
	col.g = g;
	col.b = b;
	SDL_SetPalette(screen, SDL_PHYSPAL, &col, 0, 1);
	return;
}
