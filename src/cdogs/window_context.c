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
#include "window_context.h"

#include "log.h"
#include "texture.h"


bool WindowContextCreate(
	WindowContext *wc, const Rect2i windowDim, const int windowFlags,
	const char *title, SDL_Surface *icon,
	const struct vec2i rendererLogicalSize)
{
	LOG(LM_GFX, LL_DEBUG, "creating window (%X, %X) %dx%d flags(%X)",
		windowDim.Pos.x, windowDim.Pos.y, windowDim.Size.x, windowDim.Size.y,
		windowFlags);
	wc->bkgMask = colorWhite;
	wc->window = SDL_CreateWindow(
		title, windowDim.Pos.x, windowDim.Pos.y,
		windowDim.Size.x, windowDim.Size.y, windowFlags);
	if (wc->window == NULL)
	{
		LOG(LM_GFX, LL_ERROR, "cannot create window: %s", SDL_GetError());
		return false;
	}
	wc->renderer = SDL_CreateRenderer(
		wc->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
	if (wc->renderer == NULL)
	{
		LOG(LM_GFX, LL_ERROR, "cannot create renderer: %s", SDL_GetError());
		return false;
	}
	LOG(LM_GFX, LL_DEBUG, "setting icon");
	SDL_SetWindowIcon(wc->window, icon);

	if (!WindowContextInitTextures(wc, rendererLogicalSize))
	{
		return false;
	}
	return true;
}
bool WindowContextInitTextures(
	WindowContext *wc, const struct vec2i rendererLogicalSize)
{
	CArrayInit(&wc->texturesBkg, sizeof(SDL_Texture *));
	CArrayInit(&wc->textures, sizeof(SDL_Texture *));

	if (SDL_RenderSetLogicalSize(
		wc->renderer, rendererLogicalSize.x, rendererLogicalSize.y) != 0)
	{
		LOG(LM_GFX, LL_ERROR, "cannot set renderer logical size: %s",
			SDL_GetError());
		return false;
	}
	return true;
}
void WindowContextDestroy(WindowContext *wc)
{
	WindowContextDestroyTextures(wc);
	SDL_DestroyRenderer(wc->renderer);
	SDL_DestroyWindow(wc->window);
}
void WindowContextDestroyTextures(WindowContext *wc)
{
	CA_FOREACH(SDL_Texture *, t, wc->texturesBkg)
		SDL_DestroyTexture(*t);
	CA_FOREACH_END()
	CArrayTerminate(&wc->texturesBkg);
	CA_FOREACH(SDL_Texture *, t, wc->textures)
		SDL_DestroyTexture(*t);
	CA_FOREACH_END()
	CArrayTerminate(&wc->textures);
}

void WindowsAdjustPosition(WindowContext *wc1, WindowContext *wc2)
{
	// Adjust windows so that they are side-by-side
	struct vec2i pos;
	SDL_GetWindowPosition(wc1->window, &pos.x, &pos.y);
	struct vec2i size;
	SDL_GetWindowSize(wc1->window, &size.x, &size.y);

	SDL_SetWindowPosition(wc1->window, pos.x - size.x / 2, pos.y);
	SDL_SetWindowPosition(wc2->window, pos.x + size.x / 2, pos.y);
}

SDL_Texture *WindowContextCreateTexture(
	WindowContext *wc, const SDL_TextureAccess texAccess,
	const struct vec2i res, const SDL_BlendMode blend, const Uint8 alpha,
	const bool isBkg)
{
	SDL_Texture *t = TextureCreate(wc->renderer, texAccess, res, blend, alpha);
	CArrayPushBack(isBkg ? &wc->texturesBkg : &wc->textures, &t);
	return t;
}

void WindowContextPreRender(WindowContext *wc)
{
	if (SDL_SetRenderDrawColor(wc->renderer, 0, 0, 0, 255) != 0)
	{
		LOG(LM_GFX, LL_ERROR, "Failed to set draw color: %s", SDL_GetError());
	}
	if (SDL_RenderClear(wc->renderer) != 0)
	{
		LOG(LM_MAIN, LL_ERROR, "Failed to clear renderer: %s", SDL_GetError());
		return;
	}
	CA_FOREACH(SDL_Texture *, t, wc->texturesBkg)
		TextureRender(
			*t, wc->renderer, Rect2iZero(), Rect2iZero(), wc->bkgMask, 0,
			SDL_FLIP_NONE);
	CA_FOREACH_END()
}

void WindowContextPostRender(WindowContext *wc)
{
	CA_FOREACH(SDL_Texture *, t, wc->textures)
		TextureRender(
			*t, wc->renderer, Rect2iZero(), Rect2iZero(), colorWhite, 0,
			SDL_FLIP_NONE);
	CA_FOREACH_END()

	SDL_RenderPresent(wc->renderer);
}
