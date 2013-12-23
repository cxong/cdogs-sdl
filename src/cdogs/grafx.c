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

#include <hqx.h>
#include <SDL_events.h>
#include <SDL_mouse.h>

#include "actors.h"
#include "ai.h"
#include "blit.h"
#include "config.h"
#include "defs.h"
#include "draw.h"
#include "drawtools.h"
#include "mission.h"
#include "objs.h"
#include "palette.h"
#include "files.h"
#include "triggers.h"
#include "utils.h"


GraphicsDevice gGraphicsDevice;

static void Gfx_ModeSet(GraphicsConfig *config, GraphicsMode *mode)
{
	config->ResolutionWidth = mode->Width;
	config->ResolutionHeight = mode->Height;
	config->ScaleFactor = mode->ScaleFactor;
}

void Gfx_ModePrev(void)
{
	GraphicsDevice *device = &gGraphicsDevice;
	GraphicsConfig *config = &gConfig.Graphics;
	device->modeIndex--;
	if (device->modeIndex < 0)
	{
		device->modeIndex = device->numValidModes - 1;
	}
	Gfx_ModeSet(config, &device->validModes[device->modeIndex]);
}

void Gfx_ModeNext(void)
{
	GraphicsDevice *device = &gGraphicsDevice;
	GraphicsConfig *config = &gConfig.Graphics;
	device->modeIndex++;
	if (device->modeIndex >= device->numValidModes)
	{
		device->modeIndex = 0;
	}
	Gfx_ModeSet(config, &device->validModes[device->modeIndex]);
}

static int FindValidMode(GraphicsDevice *device, int width, int height, int scaleFactor)
{
	int i;
	for (i = 0; i < device->numValidModes; i++)
	{
		GraphicsMode *mode = &device->validModes[i];
		if (mode->Width == width &&
			mode->Height == height &&
			mode->ScaleFactor == scaleFactor)
		{
			return i;
		}
	}
	return -1;
}

int IsRestartRequiredForConfig(GraphicsDevice *device, GraphicsConfig *config)
{
	return
		!device->IsInitialized ||
		device->cachedConfig.Fullscreen != config->Fullscreen ||
		device->cachedConfig.ResolutionWidth != config->ResolutionWidth ||
		device->cachedConfig.ResolutionHeight != config->ResolutionHeight ||
		device->cachedConfig.ScaleFactor != config->ScaleFactor;
}

static void AddGraphicsMode(
	GraphicsDevice *device, int width, int height, int scaleFactor)
{
	int i = 0;
	int actualResolutionToAdd = width * height * scaleFactor * scaleFactor;
	GraphicsMode *mode = &device->validModes[i];

	// Don't add if mode already exists
	if (FindValidMode(device, width, height, scaleFactor) != -1)
	{
		return;
	}

	for (; i < device->numValidModes; i++)
	{
		// Ordered by actual resolution ascending and scale descending
		int actualResolution;
		mode = &device->validModes[i];
		actualResolution =
			mode->Width * mode->Height * mode->ScaleFactor * mode->ScaleFactor;
		if (actualResolution > actualResolutionToAdd ||
			(actualResolution == actualResolutionToAdd &&
			mode->ScaleFactor < scaleFactor))
		{
			break;
		}
	}
	device->numValidModes++;
	CREALLOC(device->validModes, device->numValidModes * sizeof *device->validModes);
	// If inserting, move later modes one place further
	if (i < device->numValidModes - 1)
	{
		memmove(
			device->validModes + i + 1,
			device->validModes + i,
			(device->numValidModes - 1 - i) * sizeof *device->validModes);
	}
	mode = &device->validModes[i];
	mode->Width = width;
	mode->Height = height;
	mode->ScaleFactor = scaleFactor;
}

void GraphicsInit(GraphicsDevice *device)
{
	device->IsInitialized = 0;
	device->IsWindowInitialized = 0;
	device->screen = NULL;
	memset(&device->cachedConfig, 0, sizeof device->cachedConfig);
	device->validModes = NULL;
	device->clipping.left = 0;
	device->clipping.top = 0;
	device->clipping.right = 0;
	device->clipping.bottom = 0;
	device->numValidModes = 0;
	device->modeIndex = 0;
	// Add default modes
	AddGraphicsMode(device, 320, 240, 1);
	AddGraphicsMode(device, 400, 300, 1);
	AddGraphicsMode(device, 640, 480, 1);
	AddGraphicsMode(device, 320, 240, 2);
	device->buf = NULL;
	device->bkg = NULL;
	hqxInit();
}

void AddSupportedModesForBPP(GraphicsDevice *device, int bpp)
{
	SDL_Rect** modes;
	SDL_PixelFormat fmt;
	int i;
	memset(&fmt, 0, sizeof fmt);
	fmt.BitsPerPixel = (Uint8)bpp;

	modes = SDL_ListModes(&fmt, SDL_FULLSCREEN);
	if (modes == (SDL_Rect**)0)
	{
		return;
	}
	if (modes == (SDL_Rect**)-1)
	{
		return;
	}

	for (i = 0; modes[i]; i++)
	{
		int validScaleFactors[] = { 1, 2, 3, 4 };
		int j;
		for (j = 0; j < 4; j++)
		{
			int scaleFactor = validScaleFactors[j];
			int w, h;
			if (modes[i]->w % scaleFactor || modes[i]->h % scaleFactor)
			{
				continue;
			}
			if (modes[i]->w % 4)
			{
				// TODO: why does width have to be divisible by 4? 1366x768 doesn't work
				continue;
			}
			w = modes[i]->w / scaleFactor;
			h = modes[i]->h / scaleFactor;
			if (w < 320 || h < 240)
			{
				break;
			}
			AddGraphicsMode(device, w, h, scaleFactor);
		}
	}
}

void AddSupportedGraphicsModes(GraphicsDevice *device)
{
	AddSupportedModesForBPP(device, 16);
	AddSupportedModesForBPP(device, 24);
	AddSupportedModesForBPP(device, 32);
}

static void MakeRandomBackground(
	GraphicsDevice *device, GraphicsConfig *config)
{
	HSV tint;
	SetupQuickPlayCampaign(&gCampaign.Setting, &gConfig.QuickPlay);
	gCampaign.seed = rand();
	tint.h = rand() * 360.0 / RAND_MAX;
	tint.s = rand() * 1.0 / RAND_MAX;
	tint.v = 0.5;
	GrafxMakeBackground(device, config, tint, 0);
	gCampaign.seed = gConfig.Game.RandomSeed;
}

// Initialises the video subsystem.
// To prevent needless screen flickering, config is compared with cache
// to see if anything changed. If not, don't recreate the screen.
void GraphicsInitialize(
	GraphicsDevice *device, GraphicsConfig *config, TPalette palette,
	int force)
{
	int sdl_flags = 0;
	unsigned int w, h = 0;
	unsigned int rw, rh;

	if (!IsRestartRequiredForConfig(device, config))
	{
		return;
	}

	if (!device->IsWindowInitialized)
	{
		/* only do this the first time */
		char title[32];
		debug(D_NORMAL, "setting caption and icon...\n");
		sprintf(title, "C-Dogs SDL %s", CDOGS_SDL_VERSION);
		SDL_WM_SetCaption(title, NULL);
		SDL_WM_SetIcon(SDL_LoadBMP(GetDataFilePath("cdogs_icon.bmp")), NULL);
		SDL_ShowCursor(SDL_DISABLE);
		AddSupportedGraphicsModes(device);
	}

	device->IsInitialized = 0;

	sdl_flags |= SDL_SWSURFACE;
	sdl_flags |= config->IsEditor ? SDL_RESIZABLE : 0;
	if (config->Fullscreen)
	{
		sdl_flags |= SDL_FULLSCREEN;
	}

	rw = w = config->ResolutionWidth;
	rh = h = config->ResolutionHeight;

	if (config->ScaleFactor > 1)
	{
		rw *= config->ScaleFactor;
		rh *= config->ScaleFactor;
	}

	if (!force && !config->IsEditor)
	{
		device->modeIndex = FindValidMode(device, w, h, config->ScaleFactor);
		if (device->modeIndex == -1)
		{
			device->modeIndex = 0;
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

	printf("Graphics mode:\t%dx%d %dx (actual %dx%d)\n",
		w, h, config->ScaleFactor, rw, rh);
	SDL_FreeSurface(device->screen);
	device->screen = SDL_SetVideoMode(rw, rh, 32, sdl_flags);
	if (device->screen == NULL)
	{
		printf("ERROR: InitVideo: %s\n", SDL_GetError());
		return;
	}

	CFREE(device->buf);
	CCALLOC(device->buf, GraphicsGetMemSize(config));
	CFREE(device->bkg);
	CCALLOC(device->bkg, GraphicsGetMemSize(config));

	debug(D_NORMAL, "Changed video mode...\n");

	GraphicsSetBlitClip(
		device,
		0, 0, config->ResolutionWidth - 1,config->ResolutionHeight - 1);
	debug(D_NORMAL, "Internal dimensions:\t%dx%d\n",
		config->ResolutionWidth, config->ResolutionHeight);

	device->IsInitialized = 1;
	device->IsWindowInitialized = 1;
	device->cachedConfig = *config;
	device->cachedConfig.ResolutionWidth = w;
	device->cachedConfig.ResolutionHeight = h;
	CDogsSetPalette(palette);
	// Need to make background here since dimensions use cached config
	if (!config->IsEditor)
	{
		MakeRandomBackground(device, config);
	}
}

void GraphicsTerminate(GraphicsDevice *device)
{
	debug(D_NORMAL, "Shutting down video...\n");
	SDL_FreeSurface(device->screen);
	SDL_VideoQuit();
	CFREE(device->buf);
	CFREE(device->bkg);
}

int GraphicsGetScreenSize(GraphicsConfig *config)
{
	return config->ResolutionWidth * config->ResolutionHeight;
}

int GraphicsGetMemSize(GraphicsConfig *config)
{
	return GraphicsGetScreenSize(config) * sizeof(Uint32);
}

void GrafxMakeBackground(
	GraphicsDevice *device, GraphicsConfig *config, HSV tint, int missionIdx)
{
	DrawBuffer buffer;
	Vec2i v;

	DrawBufferInit(&buffer, Vec2iNew(X_TILES, Y_TILES));
	SetupMission(missionIdx, 1, &gCampaign);
	SetupMap();
	InitializeBadGuys();
	CreateEnemies();
	MapMarkAllAsVisited();
	DrawBufferSetFromMap(
		&buffer, gMap,
		Vec2iNew(XMAX * TILE_WIDTH / 2, YMAX * TILE_HEIGHT / 2),
		X_TILES,
		Vec2iNew(X_TILES, Y_TILES));
	DrawBufferDraw(&buffer, Vec2iZero());
	DrawBufferTerminate(&buffer);
	KillAllActors();
	KillAllObjects();
	FreeTriggersAndWatches();

	for (v.y = 0; v.y < config->ResolutionHeight; v.y++)
	{
		for (v.x = 0; v.x < config->ResolutionWidth; v.x++)
		{
			DrawPointTint(device, v, tint);
		}
	}
	memcpy(device->bkg, device->buf, GraphicsGetMemSize(config));
	memset(device->buf, 0, GraphicsGetMemSize(config));
}

void GraphicsBlitBkg(GraphicsDevice *device)
{
	memcpy(device->buf, device->bkg, GraphicsGetMemSize(&device->cachedConfig));
}

char *GrafxGetModeStr(void)
{
	static char buf[16];
	sprintf(buf, "%dx%d %dx",
		gConfig.Graphics.ResolutionWidth,
		gConfig.Graphics.ResolutionHeight,
		gConfig.Graphics.ScaleFactor);
	return buf;
}

void GraphicsSetBlitClip(
	GraphicsDevice *device, int left, int top, int right, int bottom)
{
	device->clipping.left = left;
	device->clipping.top = top;
	device->clipping.right = right;
	device->clipping.bottom = bottom;
}

void GraphicsResetBlitClip(GraphicsDevice *device)
{
	GraphicsSetBlitClip(
		device,
		0, 0,
		device->cachedConfig.ResolutionWidth - 1,
		device->cachedConfig.ResolutionHeight - 1);
}
