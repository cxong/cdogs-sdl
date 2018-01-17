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
#include "texture.h"

#include "log.h"


SDL_Texture *TextureCreate(
	SDL_Renderer *renderer, const SDL_TextureAccess access, const struct vec2i res,
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

void TextureRender(SDL_Texture *t, SDL_Renderer *r)
{
	if (SDL_RenderCopy(r, t, NULL, NULL) != 0)
	{
		LOG(LM_MAIN, LL_ERROR, "Failed to render texture: %s", SDL_GetError());
	}
}
