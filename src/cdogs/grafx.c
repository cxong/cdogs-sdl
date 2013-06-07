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
#include "grafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>

#include <SDL_endian.h>

#include "defs.h"
#include "blit.h"
#include "config.h"
#include "pics.h" /* for gPalette */
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


static int mode_idx = 1;

void Gfx_ModePrev(void)
{
	mode_idx--;
	if (mode_idx < 0)
	{
		mode_idx = MODE_MAX;
	}

	gConfig.Graphics.ResolutionWidth = gfx_modelist[mode_idx].w;
	gConfig.Graphics.ResolutionHeight = gfx_modelist[mode_idx].h;
}

void Gfx_ModeNext(void)
{
	mode_idx++;
	if (mode_idx > MODE_MAX)
	{
		mode_idx = 0;
	}

	gConfig.Graphics.ResolutionWidth = gfx_modelist[mode_idx].w;
	gConfig.Graphics.ResolutionHeight = gfx_modelist[mode_idx].h;
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


GraphicsDevice gGraphicsDevice =
{
	0,
	0,
	NULL
};

int IsRestartRequiredForConfig(GraphicsDevice *device, GraphicsConfig *config)
{
	return
		!device->IsInitialized ||
		device->cachedConfig.Fullscreen != config->Fullscreen ||
		device->cachedConfig.ScaleFactor != config->ScaleFactor;
}

/* Initialises the video subsystem.

   Note: dynamic resolution change is not supported. */
// To prevent needless screen flickering, config is compared with cache
// to see if anything changed. If not, don't recreate the screen.
void GraphicsInitialize(GraphicsDevice *device, GraphicsConfig *config, int force)
{
	int sdl_flags = 0;
	unsigned int w, h = 0;
	unsigned int rw, rh;

	if (!IsRestartRequiredForConfig(device, config))
	{
		return;
	}

	device->IsInitialized = 0;

	sdl_flags |= SDL_HWPALETTE;
	sdl_flags |= SDL_SWSURFACE;

	if (config->Fullscreen)
	{
		sdl_flags |= SDL_FULLSCREEN;
	}

	// Don't allow resolution to change
	if (!device->IsWindowInitialized)
	{
		rw = w = config->ResolutionWidth;
		rh = h = config->ResolutionHeight;
	}
	else
	{
		rw = w = device->cachedConfig.ResolutionWidth;
		rh = h = device->cachedConfig.ResolutionHeight;
	}

	if (config->ScaleFactor > 1)
	{
		rw *= config->ScaleFactor;
		rh *= config->ScaleFactor;
	}

	if (!force)
	{
		if (!ValidMode(w, h))
		{
			printf("!!! Invalid Video Mode %dx%d\n", w, h);
			return;
		}
	}
	else
	{
		printf("\n");
		printf("  BIG FAT WARNING: If this blows up in your face,\n");
		printf("  and mutilates your cat, please don't cry.\n");
		printf("\n");
	}

	printf("Window dimensions:\t%dx%d\n", rw, rh);
	SDL_FreeSurface(device->screen);
	device->screen = SDL_SetVideoMode(rw, rh, 8, sdl_flags);
	if (device->screen == NULL)
	{
		printf("ERROR: InitVideo: %s\n", SDL_GetError());
		return;
	}

	if (!device->IsWindowInitialized)
	{
		/* only do this the first time */
		char title[32];
		debug(D_NORMAL, "setting caption and icon...\n");
		sprintf(title, "C-Dogs %s [Port %s]", CDOGS_VERSION, CDOGS_SDL_VERSION);
		SDL_WM_SetCaption(title, NULL);
		SDL_WM_SetIcon(SDL_LoadBMP(GetDataFilePath("cdogs_icon.bmp")), NULL);
		SDL_ShowCursor(SDL_DISABLE);
	}
	else
	{
		debug(D_NORMAL, "Changed video mode...\n");
	}

	CDogsSetClip(0, 0, config->ResolutionWidth - 1, config->ResolutionHeight - 1);
	debug(D_NORMAL, "Internal dimensions:\t%dx%d\n",
		config->ResolutionWidth, config->ResolutionHeight);

	CDogsSetPalette(gPalette);

	device->IsInitialized = 1;
	device->IsWindowInitialized = 1;
	device->cachedConfig = *config;
}

void GraphicsTerminate(GraphicsDevice *device)
{
	debug(D_NORMAL, "Shutting down video...\n");
	SDL_FreeSurface(device->screen);
	SDL_VideoQuit();
}

char *GrafxGetResolutionStr(void)
{
	static char buf[16];
	sprintf(buf, "%dx%d",
		gConfig.Graphics.ResolutionWidth, gConfig.Graphics.ResolutionHeight);
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
	int is_eof = 0;
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

		while (!is_eof && i < maxPics)
		{
			elementsRead = fread(&size, sizeof(size), 1, f);
			CHECK_FREAD(1)
			swap16(&size);
			if (size > 0)
			{
				Pic *p;
				CMALLOC(p, size);

				f_read16(f, &p->w, 2);
				f_read16(f, &p->h, 2);

				f_read(f, &p->data, size - 4);

				pics[i] = p;

				if (ferror(f) || feof(f))
				{
					is_eof = 1;
				}
			}
			else
			{
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
	int is_eof = 0;
	unsigned short int size;
	int i = startIndex;

	f = fopen(filename, "rb");
	if (f != NULL) {
		fseek(f, sizeof(TPalette), SEEK_CUR);

		while (!is_eof && i < maxPics)
		{
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
			if (size > 0)
			{
				Pic *p;
				CMALLOC(p, size);

				f_read16(f, &p->w, 2);
				f_read16(f, &p->h, 2);
				f_read(f, &p->data, size - 4);

				pics[i] = p;

				if (ferror(f) || feof(f))
				{
					is_eof = 1;
				}
			}
			else
			{
				pics[i] = NULL;
			}
			i++;
		#undef CHECK_FREAD
		}
		fclose(f);
	}

	return i - startIndex;
}

void SetColorZero(
	GraphicsDevice *device, unsigned char r, unsigned char g, unsigned char b)
{
	SDL_Color col;
	col.r = r;
	col.g = g;
	col.b = b;
	SDL_SetPalette(device->screen, SDL_PHYSPAL, &col, 0, 1);
	return;
}
