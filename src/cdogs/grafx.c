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

    Copyright (c) 2013-2019 Cong Xu
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
#ifdef __EMSCRIPTEN__
#include <SDL2/SDL_image.h>
#else
#include <SDL_image.h>
#endif
#include <SDL_mouse.h>

#include "blit.h"
#include "config.h"
#include "defs.h"
#include "draw/drawtools.h"
#include "font_utils.h"
#include "grafx_bg.h"
#include "log.h"
#include "palette.h"
#include "files.h"
#include "utils.h"


GraphicsDevice gGraphicsDevice;

void GraphicsInit(GraphicsDevice *device, Config *c)
{
	memset(device, 0, sizeof *device);
	GraphicsConfigSetFromConfig(&device->cachedConfig, c);
	device->cachedConfig.RestartFlags = RESTART_ALL;
}

// Initialises the video subsystem.
// To prevent needless screen flickering, config is compared with cache
// to see if anything changed. If not, don't recreate the screen.
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
		g->IsWindowInitialized = true;
	}

	g->IsInitialized = false;

	const int w = g->cachedConfig.Res.x;
	const int h = g->cachedConfig.Res.y;

	const bool initWindow =
		!!(g->cachedConfig.RestartFlags & RESTART_WINDOW);
	const bool initTextures =
		!!(g->cachedConfig.RestartFlags &
		(RESTART_WINDOW | RESTART_SCALE_MODE));
	const bool initBrightness =
		!!(g->cachedConfig.RestartFlags &
		(RESTART_WINDOW | RESTART_SCALE_MODE | RESTART_BRIGHTNESS));

	if (initWindow)
	{
		LOG(LM_GFX, LL_INFO, "graphics mode(%dx%d %dx%s)",
			w, h, g->cachedConfig.ScaleFactor,
			g->cachedConfig.Fullscreen ? " fullscreen" : "");

		Uint32 windowFlags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
		Rect2i windowDim = Rect2iNew(
			svec2i(SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED),
			svec2i(
				ConfigGetInt(&gConfig, "Graphics.WindowWidth"),
				ConfigGetInt(&gConfig, "Graphics.WindowHeight")
			)
		);
		if (g->cachedConfig.Fullscreen)
		{
			windowFlags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
			windowDim.Pos = svec2i_zero();
		}
		LOG(LM_GFX, LL_DEBUG, "destroying previous renderer");
		WindowContextDestroy(&g->gameWindow);
		WindowContextDestroy(&g->secondWindow);
		SDL_FreeFormat(g->Format);

		char title[32];
		sprintf(title, "C-Dogs SDL %s%s",
			g->cachedConfig.IsEditor ? "Editor " : "",
			CDOGS_SDL_VERSION);
		if (!WindowContextCreate(
				&g->gameWindow, windowDim, windowFlags, title, g->icon,
				svec2i(w, h)))
		{
			return;
		}
		if (g->cachedConfig.SecondWindow)
		{
			if (!WindowContextCreate(
					&g->secondWindow, windowDim, windowFlags, title, g->icon,
					svec2i(w, h)))
			{
				return;
			}
			WindowsAdjustPosition(&g->gameWindow, &g->secondWindow);
		}

		g->Format = SDL_AllocFormat(SDL_PIXELFORMAT_ARGB8888);

		// Need to reload textures due to them tied to the renderer (window)
		PicManagerReloadTextures(&gPicManager);
		FontLoadFromJSON(&gFont, "graphics/font.png", "graphics/font.json");
	}

	if (initTextures)
	{
		if (!initWindow)
		{
			WindowContextDestroyTextures(&g->gameWindow);
			WindowContextDestroyTextures(&g->secondWindow);
			WindowContextInitTextures(&g->gameWindow, svec2i(w, h));
			WindowContextInitTextures(&g->secondWindow, svec2i(w, h));
		}

		GraphicsResetClip(g->gameWindow.renderer);

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

		CFREE(g->buf);
		CCALLOC(g->buf, GraphicsGetMemSize(&g->cachedConfig));
		g->bkgTgt = WindowContextCreateTexture(
			&g->gameWindow, SDL_TEXTUREACCESS_TARGET, svec2i(w, h),
			SDL_BLENDMODE_NONE, 255, true);
		if (g->bkgTgt == NULL)
		{
			return;
		}
		g->bkg = WindowContextCreateTexture(
			&g->gameWindow, SDL_TEXTUREACCESS_STATIC, svec2i(w, h),
			SDL_BLENDMODE_BLEND, 255, true);
		if (g->bkg == NULL)
		{
			return;
		}
		if (g->cachedConfig.SecondWindow)
		{
			g->bkgTgt2 = WindowContextCreateTexture(
				&g->secondWindow, SDL_TEXTUREACCESS_TARGET, svec2i(w, h),
				SDL_BLENDMODE_NONE, 255, true);
			if (g->bkgTgt2 == NULL)
			{
				return;
			}
			g->bkg2 = WindowContextCreateTexture(
				&g->secondWindow, SDL_TEXTUREACCESS_STATIC, svec2i(w, h),
				SDL_BLENDMODE_BLEND, 255, true);
			if (g->bkg2 == NULL)
			{
				return;
			}
		}

		g->screen = WindowContextCreateTexture(
			&g->gameWindow, SDL_TEXTUREACCESS_STREAMING, svec2i(w, h),
			SDL_BLENDMODE_BLEND, 255, false);
		if (g->screen == NULL)
		{
			return;
		}

		g->hud = WindowContextCreateTexture(
			&g->gameWindow, SDL_TEXTUREACCESS_STREAMING, svec2i(w, h),
			SDL_BLENDMODE_BLEND, 255, false);
		if (g->hud == NULL)
		{
			return;
		}
		BlitClearBuf(g);
		BlitUpdateFromBuf(g, g->hud);

		if (g->cachedConfig.SecondWindow)
		{
			g->hud2 = WindowContextCreateTexture(
				&g->secondWindow, SDL_TEXTUREACCESS_STREAMING, svec2i(w, h),
				SDL_BLENDMODE_BLEND, 255, false);
			if (g->hud2 == NULL)
			{
				return;
			}
			BlitClearBuf(g);
			BlitUpdateFromBuf(g, g->hud2);
		}
	}

	if (initBrightness)
	{
		if (!initWindow && !initTextures)
		{
			SDL_DestroyTexture(g->brightnessOverlay);
		}

		const int brightness = ConfigGetInt(&gConfig, "Graphics.Brightness");
		// Alpha is approximately 50% max
		const Uint8 alpha =
			(Uint8)(brightness > 0 ? brightness : -brightness) * 13;
		g->brightnessOverlay = WindowContextCreateTexture(
			&g->gameWindow, SDL_TEXTUREACCESS_STATIC, svec2i(w, h),
			SDL_BLENDMODE_BLEND, alpha, false);
		if (g->brightnessOverlay == NULL)
		{
			return;
		}
		const color_t overlayColour = brightness > 0 ? colorWhite : colorBlack;
		BlitFillBuf(g, overlayColour);
		BlitUpdateFromBuf(g, g->brightnessOverlay);
		g->cachedConfig.Brightness = brightness;
	}

	g->IsInitialized = true;
	g->cachedConfig.Res.x = w;
	g->cachedConfig.Res.y = h;
	g->cachedConfig.RestartFlags = 0;
}

void GraphicsTerminate(GraphicsDevice *g)
{
	WindowContextDestroy(&g->gameWindow);
	WindowContextDestroy(&g->secondWindow);
	SDL_FreeFormat(g->Format);
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
	struct vec2i windowSize, const bool fullscreen,
	const int scaleFactor, const ScaleMode scaleMode, const int brightness,
	const bool secondWindow)
{
#define SET(_lhs, _rhs, _flag) \
	if ((_lhs) != (_rhs)) \
	{ \
		(_lhs) = (_rhs); \
		c->RestartFlags |= (_flag); \
	}
	SET(c->Fullscreen, fullscreen, RESTART_WINDOW);
	if (c->Fullscreen)
	{
		// Set to native resolution
		SDL_DisplayMode dm;
		if (SDL_GetCurrentDisplayMode(0, &dm) != 0)
		{
			LOG(LM_GFX, LL_WARN, "cannot get display mode: %s",
				SDL_GetError());
		}
		else
		{
			windowSize = svec2i(dm.w, dm.h);
			ConfigSetInt(&gConfig, "Graphics.WindowWidth", dm.w);
			ConfigSetInt(&gConfig, "Graphics.WindowHeight", dm.h);
		}
	}
	SET(c->ScaleFactor, scaleFactor, RESTART_SCALE_MODE);
	SET(c->ScaleMode, scaleMode, RESTART_SCALE_MODE);
	SET(c->Brightness, brightness, RESTART_BRIGHTNESS);
	SET(c->SecondWindow, secondWindow, RESTART_WINDOW);
	const struct vec2i res = svec2i_scale_divide(windowSize, scaleFactor);
	if (!svec2i_is_equal(res, c->Res))
	{
		c->Res = res;
		c->RestartFlags |= RESTART_SCALE_MODE;
	}
}

void GraphicsConfigSetFromConfig(GraphicsConfig *gc, Config *c)
{
	GraphicsConfigSet(
		gc,
		svec2i(
			ConfigGetInt(c, "Graphics.WindowWidth"),
			ConfigGetInt(c, "Graphics.WindowHeight")),
		ConfigGetBool(c, "Graphics.Fullscreen"),
		ConfigGetInt(c, "Graphics.ScaleFactor"),
		(ScaleMode)ConfigGetEnum(c, "Graphics.ScaleMode"),
		ConfigGetInt(c, "Graphics.Brightness"),
		ConfigGetBool(c, "Graphics.SecondWindow"));
}

void GraphicsSetClip(SDL_Renderer *renderer, const Rect2i r)
{
	const SDL_Rect rect = { r.Pos.x, r.Pos.y, r.Size.x, r.Size.y };
	if (SDL_RenderSetClipRect(renderer, Rect2iIsZero(r) ? NULL : &rect) != 0)
	{
		LOG(LM_MAIN, LL_ERROR, "Could not set clip rect: %s", SDL_GetError());
	}
}

Rect2i GraphicsGetClip(SDL_Renderer *renderer)
{
	SDL_Rect rect;
	SDL_RenderGetClipRect(renderer, &rect);
	return Rect2iNew(svec2i(rect.x, rect.y), svec2i(rect.w, rect.h));
}

void GraphicsResetClip(SDL_Renderer *renderer)
{
	if (SDL_RenderSetClipRect(renderer, NULL) != 0)
	{
		LOG(LM_MAIN, LL_ERROR, "Could not reset clip rect: %s", SDL_GetError());
	}
}
