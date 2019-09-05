/*
 Copyright (c) 2017-2019 Cong Xu
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

#include <SDL.h>

#include "c_array.h"
#include "vector.h"


typedef struct
{
	SDL_Window *window;
	SDL_Renderer *renderer;
	CArray texturesBkg;	// of SDL_Texture *
	CArray textures;	// of SDL_Texture *
	color_t bkgMask;
} WindowContext;

bool WindowContextCreate(
	WindowContext *wc, const Rect2i windowDim, const int windowFlags,
	const char *title, SDL_Surface *icon,
	const struct vec2i rendererLogicalSize);
bool WindowContextInitTextures(
	WindowContext *wc, const struct vec2i rendererLogicalSize);
void WindowContextDestroy(WindowContext *wc);
void WindowContextDestroyTextures(WindowContext *wc);

void WindowsAdjustPosition(WindowContext *wc1, WindowContext *wc2);

SDL_Texture *WindowContextCreateTexture(
	WindowContext *wc, const SDL_TextureAccess texAccess,
	const struct vec2i res, const SDL_BlendMode blend, const Uint8 alpha,
	const bool isBkg);

// Render things before game-specific stuff
void WindowContextPreRender(WindowContext *wc);
// Render things after game-specific stuff
void WindowContextPostRender(WindowContext *wc);
