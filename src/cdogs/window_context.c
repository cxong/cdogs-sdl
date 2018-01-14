/*
 Copyright (c) 2017 Cong Xu
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
	WindowContext *wc, const struct vec2i windowSize, const int sdlFlags,
	const char *title, SDL_Surface *icon, const struct vec2i rendererLogicalSize)
{
	CArrayInit(&wc->textures, sizeof(SDL_Texture *));

	LOG(LM_GFX, LL_DEBUG, "creating window %dx%d flags(%X)",
		windowSize.x, windowSize.y, sdlFlags);
	if (SDL_CreateWindowAndRenderer(
			windowSize.x, windowSize.y, sdlFlags,
			&wc->window, &wc->renderer) == -1 ||
		wc->window == NULL || wc->renderer == NULL)
	{
		LOG(LM_GFX, LL_ERROR, "cannot create window or renderer: %s",
			SDL_GetError());
		return false;
	}
	LOG(LM_GFX, LL_DEBUG, "setting title(%s) and icon", title);
	SDL_SetWindowTitle(wc->window, title);
	SDL_SetWindowIcon(wc->window, icon);

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
	WindowContext *wc, const SDL_TextureAccess access, const struct vec2i res,
	const SDL_BlendMode blend, const Uint8 alpha)
{
	SDL_Texture *t = TextureCreate(wc->renderer, access, res, blend, alpha);
	CArrayPushBack(&wc->textures, &t);
	return t;
}

void WindowContextRender(WindowContext *wc)
{
	if (SDL_RenderClear(wc->renderer) != 0)
	{
		LOG(LM_MAIN, LL_ERROR, "Failed to clear renderer: %s", SDL_GetError());
		return;
	}
	CA_FOREACH(SDL_Texture *, t, wc->textures)
		TextureRender(*t, wc->renderer);
	CA_FOREACH_END()

	SDL_RenderPresent(wc->renderer);
}
