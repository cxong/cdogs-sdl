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

    Copyright (c) 2013-2016, Cong Xu
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

#include <SDL_events.h>
#include <SDL_image.h>
#include <SDL_mouse.h>

#include "blit.h"
#include "config.h"
#include "defs.h"
#include "draw/drawtools.h"
#include "grafx_bg.h"
#include "log.h"
#include "palette.h"
#include "files.h"
#include "utils.h"


GraphicsDevice gGraphicsDevice;

static void Gfx_ModeSet(const Vec2i *mode)
{
	ConfigGet(&gConfig, "Graphics.ResolutionWidth")->u.Int.Value = mode->x;
	ConfigGet(&gConfig, "Graphics.ResolutionHeight")->u.Int.Value = mode->y;
}

void Gfx_ModePrev(void)
{
	GraphicsDevice *device = &gGraphicsDevice;
	device->modeIndex--;
	if (device->modeIndex < 0)
	{
		device->modeIndex = (int)device->validModes.size - 1;
	}
	Gfx_ModeSet(CArrayGet(&device->validModes, device->modeIndex));
}

void Gfx_ModeNext(void)
{
	GraphicsDevice *device = &gGraphicsDevice;
	device->modeIndex++;
	if (device->modeIndex >= (int)device->validModes.size)
	{
		device->modeIndex = 0;
	}
	Gfx_ModeSet(CArrayGet(&device->validModes, device->modeIndex));
}

static int FindValidMode(GraphicsDevice *device, const int w, const int h)
{
	CA_FOREACH(const Vec2i, mode, device->validModes)
		if (mode->x == w && mode->y == h)
		{
			return _ca_index;
		}
	CA_FOREACH_END()
	return -1;
}

static void AddGraphicsMode(GraphicsDevice *device, const int w, const int h)
{
	// Don't add if mode already exists
	if (FindValidMode(device, w, h) != -1)
	{
		return;
	}

	const int size = w * h;
	int i;
	for (i = 0; i < (int)device->validModes.size; i++)
	{
		// Ordered by actual resolution ascending and scale descending
		const Vec2i *mode = CArrayGet(&device->validModes, i);
		const int actualResolution = mode->x * mode->y;
		if (actualResolution >= size)
		{
			break;
		}
	}
	Vec2i mode = Vec2iNew(w, h);
	CArrayInsert(&device->validModes, i, &mode);
}

void GraphicsInit(GraphicsDevice *device, Config *c)
{
	memset(device, 0, sizeof *device);
	CArrayInit(&device->validModes, sizeof(Vec2i));
	// Add default modes
	AddGraphicsMode(device, 320, 240);
	AddGraphicsMode(device, 400, 300);
	AddGraphicsMode(device, 640, 480);
	GraphicsConfigSetFromConfig(&device->cachedConfig, c);
}

static void AddSupportedGraphicsModes(GraphicsDevice *device)
{
	// TODO: multiple window support?
	const int numDisplayModes = SDL_GetNumDisplayModes(0);
	if (numDisplayModes < 1)
	{
		LOG(LM_GFX, LL_ERROR, "no valid display modes: %s", SDL_GetError());
		return;
	}
	for (int i = 0; i < numDisplayModes; i++)
	{
		SDL_DisplayMode mode;
		if (SDL_GetDisplayMode(0, i, &mode) != 0)
		{
			LOG(LM_GFX, LL_ERROR, "cannot get display mode: %s",
				SDL_GetError());
			continue;
		}
		if (mode.w < 320 || mode.h < 240)
		{
			break;
		}
		AddGraphicsMode(device, mode.w, mode.h);
	}
}

// Initialises the video subsystem.
// To prevent needless screen flickering, config is compared with cache
// to see if anything changed. If not, don't recreate the screen.
static SDL_Texture *CreateTexture(
	SDL_Renderer *renderer, const SDL_TextureAccess access, const Vec2i res,
	const SDL_BlendMode blend, const Uint8 alpha);
void GraphicsInitialize(GraphicsDevice *g)
{
	if (g->IsInitialized && !g->cachedConfig.RestartFlags)
	{
		return;
	}

	if (!g->IsWindowInitialized)
	{
		char buf[CDOGS_PATH_MAX];
		GetDataFilePath(buf, "graphics/cdogs_icon.bmp");
		g->icon = IMG_Load(buf);
		AddSupportedGraphicsModes(g);
		g->IsWindowInitialized = true;
	}

	g->IsInitialized = false;

	const int w = g->cachedConfig.Res.x;
	const int h = g->cachedConfig.Res.y;

	const bool initRenderer =
		!!(g->cachedConfig.RestartFlags & RESTART_RESOLUTION);
	const bool initTextures =
		!!(g->cachedConfig.RestartFlags &
		(RESTART_RESOLUTION | RESTART_SCALE_MODE));
	const bool initBrightness =
		!!(g->cachedConfig.RestartFlags &
		(RESTART_RESOLUTION | RESTART_SCALE_MODE | RESTART_BRIGHTNESS));

	if (initRenderer)
	{
		Uint32 sdlFlags = SDL_WINDOW_RESIZABLE;
		if (g->cachedConfig.Fullscreen)
		{
			sdlFlags |= SDL_WINDOW_FULLSCREEN;
		}

		LOG(LM_GFX, LL_INFO, "graphics mode(%dx%d %dx)",
			w, h, g->cachedConfig.ScaleFactor);
		// Get the previous window's size and recreate it
		Vec2i windowSize = Vec2iNew(
			w * g->cachedConfig.ScaleFactor, h * g->cachedConfig.ScaleFactor);
		if (g->window)
		{
			SDL_GetWindowSize(g->window, &windowSize.x, &windowSize.y);
		}
		LOG(LM_GFX, LL_DEBUG, "destroying previous renderer");
		SDL_DestroyTexture(g->screen);
		SDL_DestroyTexture(g->bkg);
		SDL_DestroyTexture(g->brightnessOverlay);
		SDL_DestroyRenderer(g->renderer);
		SDL_FreeFormat(g->Format);
		SDL_DestroyWindow(g->window);
		LOG(LM_GFX, LL_DEBUG, "creating window %dx%d flags(%X)",
			windowSize.x, windowSize.y, sdlFlags);
		if (SDL_CreateWindowAndRenderer(
				windowSize.x, windowSize.y, sdlFlags,
				&g->window, &g->renderer) == -1 ||
			g->window == NULL || g->renderer == NULL)
		{
			LOG(LM_GFX, LL_ERROR, "cannot create window or renderer: %s",
				SDL_GetError());
			return;
		}
		char title[32];
		sprintf(title, "C-Dogs SDL %s%s",
			g->cachedConfig.IsEditor ? "Editor " : "",
			CDOGS_SDL_VERSION);
		LOG(LM_GFX, LL_DEBUG, "setting title(%s) and icon", title);
		SDL_SetWindowTitle(g->window, title);
		SDL_SetWindowIcon(g->window, g->icon);
		g->Format = SDL_AllocFormat(SDL_PIXELFORMAT_ARGB8888);

		if (SDL_RenderSetLogicalSize(g->renderer, w, h) != 0)
		{
			LOG(LM_GFX, LL_ERROR, "cannot set renderer logical size: %s",
				SDL_GetError());
			return;
		}

		GraphicsSetBlitClip(
			g, 0, 0, g->cachedConfig.Res.x - 1, g->cachedConfig.Res.y - 1);
	}

	if (initTextures)
	{
		if (!initRenderer)
		{
			SDL_DestroyTexture(g->screen);
			SDL_DestroyTexture(g->bkg);
			SDL_DestroyTexture(g->brightnessOverlay);
		}

		// Set render scale mode
		const char *renderScaleQuality = "nearest";
		switch ((ScaleMode)ConfigGetEnum(&gConfig, "Graphics.ScaleMode"))
		{
		case SCALE_MODE_NN:
			renderScaleQuality = "nearest";
			break;
		case SCALE_MODE_BILINEAR:
			renderScaleQuality = "linear";
			break;
		default:
			CASSERT(false, "unknown scale mode");
			break;
		}
		LOG(LM_GFX, LL_DEBUG, "setting scale quality %s", renderScaleQuality);
		if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, renderScaleQuality))
		{
			LOG(LM_GFX, LL_WARN, "cannot set render quality hint: %s",
				SDL_GetError());
		}

		g->screen = CreateTexture(
			g->renderer, SDL_TEXTUREACCESS_STREAMING, Vec2iNew(w, h),
			SDL_BLENDMODE_BLEND, 255);
		if (g->screen == NULL)
		{
			return;
		}

		CFREE(g->buf);
		CCALLOC(g->buf, GraphicsGetMemSize(&g->cachedConfig));
		g->bkg = CreateTexture(
			g->renderer, SDL_TEXTUREACCESS_STATIC, Vec2iNew(w, h),
			SDL_BLENDMODE_NONE, 255);
		if (g->bkg == NULL)
		{
			return;
		}
	}

	if (initBrightness)
	{
		if (!initRenderer && !initTextures)
		{
			SDL_DestroyTexture(g->brightnessOverlay);
		}

		const int brightness = ConfigGetInt(&gConfig, "Graphics.Brightness");
		// Alpha is approximately 50% max
		const Uint8 alpha = (Uint8)(brightness > 0 ? brightness : -brightness) * 13;
		g->brightnessOverlay = CreateTexture(
			g->renderer, SDL_TEXTUREACCESS_STATIC, Vec2iNew(w, h),
			SDL_BLENDMODE_BLEND, alpha);
		if (g->brightnessOverlay == NULL)
		{
			return;
		}
		const color_t overlayColour = brightness > 0 ? colorWhite : colorBlack;
		DrawRectangle(g, Vec2iZero(), g->cachedConfig.Res, overlayColour, 0);
		SDL_UpdateTexture(
			g->brightnessOverlay, NULL, g->buf,
			g->cachedConfig.Res.x * sizeof(Uint32));
		memset(g->buf, 0, GraphicsGetMemSize(&g->cachedConfig));
		g->cachedConfig.Brightness = brightness;
	}

	g->IsInitialized = true;
	g->cachedConfig.Res.x = w;
	g->cachedConfig.Res.y = h;
	g->cachedConfig.RestartFlags = 0;
}
static SDL_Texture *CreateTexture(
	SDL_Renderer *renderer, const SDL_TextureAccess access, const Vec2i res,
	const SDL_BlendMode blend, const Uint8 alpha)
{
	SDL_Texture *t = SDL_CreateTexture(
		renderer, SDL_PIXELFORMAT_ARGB8888, access, res.x, res.y);
	if (t == NULL)
	{
		LOG(LM_GFX, LL_ERROR, "cannot create texture: %s", SDL_GetError());
		return NULL;
	}
	if (SDL_SetTextureBlendMode(t, blend) != 0)
	{
		LOG(LM_GFX, LL_ERROR, "cannot set blend mode: %s", SDL_GetError());
		return NULL;
	}
	if (SDL_SetTextureAlphaMod(t, alpha) != 0)
	{
		LOG(LM_GFX, LL_ERROR, "cannot set texture alpha: %s", SDL_GetError());
		return NULL;
	}
	return t;
}

void GraphicsTerminate(GraphicsDevice *g)
{
	debug(D_NORMAL, "Shutting down video...\n");
	CArrayTerminate(&g->validModes);
	SDL_FreeSurface(g->icon);
	SDL_DestroyTexture(g->screen);
	SDL_DestroyTexture(g->bkg);
	SDL_DestroyTexture(g->brightnessOverlay);
	SDL_DestroyRenderer(g->renderer);
	SDL_FreeFormat(g->Format);
	SDL_DestroyWindow(g->window);
	SDL_VideoQuit();
	CFREE(g->buf);
}

int GraphicsGetScreenSize(GraphicsConfig *config)
{
	return config->Res.x * config->Res.y;
}

int GraphicsGetMemSize(GraphicsConfig *config)
{
	return GraphicsGetScreenSize(config) * sizeof(Uint32);
}

void GraphicsConfigSet(
	GraphicsConfig *c,
	const Vec2i res, const bool fullscreen,
	const int scaleFactor, const ScaleMode scaleMode, const int brightness)
{
	if (!Vec2iEqual(res, c->Res))
	{
		c->Res = res;
		c->RestartFlags |= RESTART_RESOLUTION;
	}
#define SET(_lhs, _rhs, _flag) \
	if ((_lhs) != (_rhs)) \
	{ \
		(_lhs) = (_rhs); \
		c->RestartFlags |= (_flag); \
	}
	SET(c->Fullscreen, fullscreen, RESTART_RESOLUTION);
	SET(c->ScaleFactor, scaleFactor, RESTART_RESOLUTION);
	SET(c->ScaleMode, scaleMode, RESTART_SCALE_MODE);
	SET(c->Brightness, brightness, RESTART_BRIGHTNESS);
}

void GraphicsConfigSetFromConfig(GraphicsConfig *gc, Config *c)
{
	GraphicsConfigSet(
		gc,
		Vec2iNew(
			ConfigGetInt(c, "Graphics.ResolutionWidth"),
			ConfigGetInt(c, "Graphics.ResolutionHeight")),
		ConfigGetBool(c, "Graphics.Fullscreen"),
		ConfigGetInt(c, "Graphics.ScaleFactor"),
		(ScaleMode)ConfigGetEnum(c, "Graphics.ScaleMode"),
		ConfigGetInt(c, "Graphics.Brightness"));
}

char *GrafxGetModeStr(void)
{
	static char buf[16];
	sprintf(buf, "%dx%d",
		ConfigGetInt(&gConfig, "Graphics.ResolutionWidth"),
		ConfigGetInt(&gConfig, "Graphics.ResolutionHeight"));
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
		device->cachedConfig.Res.x - 1,
		device->cachedConfig.Res.y - 1);
}
