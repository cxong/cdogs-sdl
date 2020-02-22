/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.
	Copyright (c) 2019-2020 Cong Xu
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

#include <SDL_opengl.h>
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_BUTTON_TRIGGER_ON_RELEASE
#ifdef _MSC_VER
// Guard against compile time constant in nk_memset
#pragma warning(push)
#pragma warning(disable: 4127)
#endif
#include <nuklear/nuklear.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <cdogs/animation.h>
#include <cdogs/character.h>
#include <cdogs/color.h>
#include <cdogs/defs.h>
#include <cdogs/events.h>
#include <cdogs/vector.h>

#define PIC_SCALE 2


typedef struct
{
	SDL_Window *win;
	struct nk_context *ctx;
	SDL_GLContext glContext;

	const char *Title;
	struct vec2i Size;
	struct vec2i MinSize;
	int WindowFlags;
	color_t BG;
	SDL_Surface *Icon;
	EventHandlers *Handlers;
	bool (*Draw)(SDL_Window *, struct nk_context *, void *);
	void *DrawData;
} NKWindowConfig;
// Note: need to init before initialising textures
void NKWindowInit(NKWindowConfig *cfg);
void NKWindow(NKWindowConfig cfg);

int nk_combo_separator_image(struct nk_context *ctx,
	const GLuint *img_ids, const char *items_separated_by_separator,
	int separator, int selected, int count, int item_height,
	struct nk_vec2 size);

// Util functions
void LoadTexFromPic(const GLuint texid, const Pic *pic);
void DrawPic(
	struct nk_context *ctx, const Pic *pic, const GLuint texid,
	const struct vec2i pos, const float scale);
