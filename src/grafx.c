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
*/
#include "grafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>

#include <SDL.h>
#include <SDL_endian.h>

#include "defs.h"
#include "blit.h"
#include "pics.h" /* for gPalette */
#include "sprcomp.h"
#include "files.h"
#include "utils.h"

GFX_Mode gfx_modelist[] = {
	{ 320, 240 },
	{ 400, 300 },
	{ 640, 480 },
	{ 800, 600 }, /* things go strange above this... */
	{ 0, 0 },
};
#define MODE_MAX 3


#define Wrap(var, min, max)			\
	{					\
		if (var > max) var = min;	\
		if (var < min) var = max;	\
	}

static int mode_idx = 1;

GFX_Mode * Gfx_ModePrev(void)
{
	mode_idx--;
	Wrap(mode_idx, 0, MODE_MAX)

	return &gfx_modelist[mode_idx];
}

GFX_Mode * Gfx_ModeNext(void)
{
	mode_idx++;
	Wrap(mode_idx, 0, MODE_MAX)

	return &gfx_modelist[mode_idx];
}

static int ValidMode(unsigned int w, unsigned int h)
{
	int i;

	for (i = 0; ; i++) {
		unsigned int m_w = gfx_modelist[i].w;
		unsigned int m_h = gfx_modelist[i].h;

		if (m_w == 0)
		{
			break;
		}

		if (m_w == w && m_h == h) {
			mode_idx = i;
			return 1;
		}
	}

	return 0;
}

/* These are the default hints as used by the graphics subsystem */
int hints[HINT_END] = {
	0,		// HINT_FULLSCREEN
	1,		// HINT_WINDOW
	1,		// HINT_SCALEFACTOR
	320,		// HINT_WIDTH
	240,		// HINT_HEIGHT
	0		// HINT_FORCEMODE
};

void Gfx_SetHint(const GFX_Hint h, const int val)
{
	if (h >= HINT_END)
		return;
	hints[h] = val;
}

#define Hint(h)	hints[h]

int Gfx_GetHint(const GFX_Hint h)
{
	if (h >= HINT_END)
		return 0;
	return Hint(h);
}

int GrafxIsFullscreen(void)
{
	return Gfx_GetHint(HINT_FULLSCREEN);
}

SDL_Surface *gScreen = NULL;
/* probably not the best thing, but we need the performance */
int screen_w;
int screen_h;

/* Initialises the video subsystem.

   Note: dynamic resolution change is not supported. */
int InitVideo(void)
{
	char title[32];
	SDL_Surface *new_screen = NULL;
	int sdl_flags = 0;
	unsigned int w, h = 0;
	unsigned int rw, rh;

	sdl_flags |= SDL_HWPALETTE;
	sdl_flags |= SDL_SWSURFACE;

	if (Hint(HINT_FULLSCREEN)) sdl_flags |= SDL_FULLSCREEN;

	if (gScreen == NULL)
	{
		rw = w = Hint(HINT_WIDTH);
		rh = h = Hint(HINT_HEIGHT);
	} else {
		/* We do this because the game dies horribly if you try to
		dynamically change the _virtual_ resolution */
		rw = w = screen_w;
		rh = h = screen_h;
	}

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

	printf("Window dimensions:\t%dx%d\n", rw, rh);
	new_screen = SDL_SetVideoMode(rw, rh, 8, sdl_flags);

	if (new_screen == NULL) {
		printf("ERROR: InitVideo: %s\n", SDL_GetError() );
		return -1;
	}

	if (gScreen == NULL)
	{ /* only do this the first time */
		debug(D_NORMAL, "setting caption and icon...\n");
		sprintf(title, "C-Dogs %s [Port %s]", CDOGS_VERSION, CDOGS_SDL_VERSION);
		SDL_WM_SetCaption(title, NULL);
		SDL_WM_SetIcon(SDL_LoadBMP(GetDataFilePath("cdogs_icon.bmp")), NULL);
		SDL_ShowCursor(SDL_DISABLE);
	} else {
		debug(D_NORMAL, "Changed video mode...\n");
	}

	if (gScreen == NULL)
	{
		screen_w = Hint(HINT_WIDTH);
		screen_h = Hint(HINT_HEIGHT);
	}

	gScreen = new_screen;

	CDogsSetClip(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);
	debug(D_NORMAL, "Internal dimensions:\t%dx%d\n", SCREEN_WIDTH, SCREEN_HEIGHT);

	CDogsSetPalette(gPalette);

	return 0;
}

void ShutDownVideo(void)
{
	debug(D_NORMAL, "Shutting down video...\n");
	SDL_VideoQuit();
}

void GrafxToggleFullscreen(void)
{
	Gfx_HintToggle(HINT_FULLSCREEN);
	InitVideo();
}

void GrafxTryResolution(GFX_Mode *m)
{
	if (m != NULL)
	{
		debug(D_NORMAL, "new mode? %d x %d\n", m->w, m->h);
		Gfx_SetHint(HINT_WIDTH, m->w);
		Gfx_SetHint(HINT_HEIGHT, m->h);
	}
}

void GrafxTryPrevResolution(void)
{
	GrafxTryResolution(Gfx_ModePrev());
}
void GrafxTryNextResolution(void)
{
	GrafxTryResolution(Gfx_ModeNext());
}

int GrafxGetScale(void)
{
	return Gfx_GetHint(HINT_SCALEFACTOR);
}
void GrafxSetScale(int scale)
{
	if (scale >= 1 && scale <= 4)
	{
		Gfx_SetHint(HINT_SCALEFACTOR, (const int)scale);
		InitVideo();
	}
}
char *GrafxGetResolutionStr(void)
{
	static char buf[16];
	sprintf(buf, "%dx%d", Gfx_GetHint(HINT_WIDTH), Gfx_GetHint(HINT_HEIGHT));
	return buf;
}

typedef struct _Pic {
	short int w;
	short int h;
	char *data;
} Pic;

int ReadPics(
	const char *filename, void **pics, int maxPics,
	color_t * palette)
{
	FILE *f;
	int eof = 0;
	unsigned short int size;
	int i = 0;

	f = fopen(filename, "rb");
	if (f != NULL) {
		size_t elementsRead;
	#define CHECK_FREAD(count)\
		if (elementsRead != count) {\
			debug(D_NORMAL, "Error ReadPics\n");\
			fclose(f);\
			return 0;\
		}
		if (palette) {
			elementsRead = fread(palette, sizeof(TPalette), 1, f);
			CHECK_FREAD(1)
		} else
			fseek(f, sizeof(TPalette), SEEK_CUR);

		while (!eof && i < maxPics) {
			elementsRead = fread(&size, sizeof(size), 1, f);
			CHECK_FREAD(1)
			swap16(&size);
			if (size) {
				Pic *p = sys_mem_alloc(size);

				f_read16(f, &p->w, 2);
				f_read16(f, &p->h, 2);

				f_read(f, &p->data, size - 4);

				pics[i] = p;

				if (ferror(f) || feof(f))
					eof = 1;
			} else {
				pics[i] = NULL;
			}
			i++;
		}
		fclose(f);
	#undef CHECK_FREAD
	}
	return i;
}

int AppendPics(const char *filename, void **pics, int startIndex,
	       int maxPics)
{
	FILE *f;
	int eof = 0;
	unsigned short int size;
	int i = startIndex;

	f = fopen(filename, "rb");
	if (f != NULL) {
		fseek(f, sizeof(TPalette), SEEK_CUR);

		while (!eof && i < maxPics) {
			size_t elementsRead;
		#define CHECK_FREAD(count)\
			if (elementsRead != count) {\
				debug(D_NORMAL, "Error AppendPics\n");\
				fclose(f);\
				return 0;\
			}
			elementsRead = fread(&size, sizeof(size), 1, f);
			CHECK_FREAD(1)
			swap16(&size);
			if (size) {
				Pic *p = sys_mem_alloc(size);

				f_read16(f, &p->w, 2);
				f_read16(f, &p->h, 2);
				f_read(f, &p->data, size - 4);

				pics[i] = p;

				if (ferror(f) || feof(f))
					eof = 1;
			} else {
				pics[i] = NULL;
			}
			i++;
		#undef CHECK_FREAD
		}
		fclose(f);
	}

	return i - startIndex;
}

void SetColorZero(unsigned char r, unsigned char g, unsigned char b)
{
	SDL_Color col;
	col.r = r;
	col.g = g;
	col.b = b;
	SDL_SetPalette(gScreen, SDL_PHYSPAL, &col, 0, 1);
	return;
}
