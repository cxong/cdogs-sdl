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
#pragma once

#include <stdbool.h>

#include <SDL.h>

#include "c_array.h"
#include "color.h"
#include "config.h"
#include "vector.h"
#include "sys_specifics.h"

#define RESTART_RESOLUTION 1
#define RESTART_SCALE_MODE 2
#define RESTART_BRIGHTNESS 4
#define RESTART_ALL -1
typedef struct
{
	Vec2i Res;
	bool Fullscreen;
	int ScaleFactor;
	ScaleMode ScaleMode;
	int Brightness;
	bool IsEditor;

	int RestartFlags;
} GraphicsConfig;

typedef struct
{
	int left;
	int top;
	int right;
	int bottom;
} BlitClipping;

typedef struct
{
	int IsInitialized;
	int IsWindowInitialized;
	SDL_Surface *icon;
	SDL_Texture *screen;
	SDL_Renderer *renderer;
	SDL_Window *window;
	SDL_PixelFormat *Format;
	GraphicsConfig cachedConfig;
	CArray validModes;	// of Vec2i, w x h
	int modeIndex;
	BlitClipping clipping;
	Uint32 *buf;
	SDL_Texture *bkg;
	SDL_Texture *brightnessOverlay;
} GraphicsDevice;

extern GraphicsDevice gGraphicsDevice;

void GraphicsInit(GraphicsDevice *device, Config *c);
void GraphicsInitialize(GraphicsDevice *g);
void GraphicsTerminate(GraphicsDevice *g);
int GraphicsGetScreenSize(GraphicsConfig *config);
int GraphicsGetMemSize(GraphicsConfig *config);
void GraphicsConfigSet(
	GraphicsConfig *c,
	const Vec2i res, const bool fullscreen,
	const int scaleFactor, const ScaleMode scaleMode, const int brightness);
void GraphicsConfigSetFromConfig(GraphicsConfig *gc, Config *c);

void Gfx_ModePrev(void);
void Gfx_ModeNext(void);

char *GrafxGetModeStr(void);

void GraphicsSetBlitClip(
	GraphicsDevice *device, int left, int top, int right, int bottom);
void GraphicsResetBlitClip(GraphicsDevice *device);

#define CenterX(w)		((gGraphicsDevice.cachedConfig.Res.x - w) / 2)
#define CenterY(h)		((gGraphicsDevice.cachedConfig.Res.y - h) / 2)

#define CenterOf(a, b, w)	((a + (((b - a) - w) / 2)))

#define CenterOfRight(w)	CenterOf((gGraphicsDevice.cachedConfig.Res.x / 2), (gGraphicsDevice.cachedConfig.Res.x), w)
#define CenterOfLeft(w)		CenterOf(0, (gGraphicsDevice.cachedConfig.Res.x / 2), w)
