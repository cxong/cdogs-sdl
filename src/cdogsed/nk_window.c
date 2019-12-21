/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (c) 2019 Cong Xu
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
#include "nk_window.h"

#include <stdbool.h>

#include <SDL_opengl.h>
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_BUTTON_TRIGGER_ON_RELEASE
#define NK_IMPLEMENTATION
#define NK_SDL_GL2_IMPLEMENTATION
#ifdef _MSC_VER
// Guard against compile time constant in nk_memset
#pragma warning(push)
#pragma warning(disable: 4127)
#endif
#include <nuklear/nuklear.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#include <nuklear/nuklear_sdl_gl2.h>
#include <cdogs/draw/draw_actor.h>
#include <cdogs/sys_config.h>


void NKWindowInit(NKWindowConfig *cfg)
{
	SDL_Init(SDL_INIT_VIDEO);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

	cfg->win = SDL_CreateWindow(
		cfg->Title,
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		cfg->Size.x, cfg->Size.y,
		SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | cfg->WindowFlags);
	SDL_SetWindowMinimumSize(cfg->win, cfg->MinSize.x, cfg->MinSize.y);
	if (cfg->Icon)
	{
		SDL_SetWindowIcon(cfg->win, cfg->Icon);
	}

	cfg->glContext = SDL_GL_CreateContext(cfg->win);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	cfg->ctx = nk_sdl_init(cfg->win);
	cfg->ctx->style.checkbox.hover.data.color = nk_rgb(96, 96, 96);
	cfg->ctx->style.checkbox.normal.data.color = nk_rgb(64, 64, 64);
	cfg->ctx->style.checkbox.cursor_hover.data.color = nk_rgb(255, 255, 255);
	cfg->ctx->style.checkbox.cursor_normal.data.color = nk_rgb(200, 200, 200);

	// Initialise fonts
	struct nk_font_atlas *atlas;
	nk_sdl_font_stash_begin(&atlas);
	nk_sdl_font_stash_end();
}


static bool HandleEvents(EventHandlers *handler);
static void Draw(const NKWindowConfig cfg);
void NKWindow(NKWindowConfig cfg)
{
	CASSERT(cfg.win, "Error: did not initialise window");
	CASSERT(cfg.ctx, "Error: did not initialise NK context");
	Uint32 ticksNow = SDL_GetTicks();
	Uint32 ticksElapsed = 0;
	for (;;)
	{
		Uint32 ticksThen = ticksNow;
		ticksNow = SDL_GetTicks();
		ticksElapsed += ticksNow - ticksThen;
		if (ticksElapsed < 1000 / FPS_FRAMELIMIT)
		{
			SDL_Delay(1);
			continue;
		}

		// Note: drawing contains input processing too
		nk_input_begin(cfg.ctx);
		if (!HandleEvents(cfg.Handlers))
		{
			goto bail;
		}
		Draw(cfg);
		nk_input_end(cfg.ctx);

		ticksElapsed = 0;
	}

bail:
	nk_sdl_shutdown();
	SDL_GL_DeleteContext(cfg.glContext);
	SDL_DestroyWindow(cfg.win);
}
static bool HandleEvents(EventHandlers *handler)
{
	SDL_Event e;
	bool run = true;
	while (SDL_PollEvent(&e))
	{
		switch (e.type)
		{
		case SDL_QUIT:
			run = false;
			break;
		case SDL_WINDOWEVENT:
			switch (e.window.event)
			{
			case SDL_WINDOWEVENT_CLOSE:
				run = false;
				break;
			default:
				break;
			}
			break;
		default:
			break;
		}
		nk_sdl_handle_event(&e);
	}

	EventPoll(handler, 1);
	const SDL_Scancode sc = KeyGetPressed(&handler->keyboard);
	if (sc == SDL_SCANCODE_ESCAPE)
	{
		run = false;
	}

	return run;
}
static void Draw(const NKWindowConfig cfg)
{
	cfg.Draw(cfg.win, cfg.ctx, cfg.DrawData);

	int winWidth, winHeight;
	SDL_GetWindowSize(cfg.win, &winWidth, &winHeight);
	glViewport(0, 0, winWidth, winHeight);
	glClear(GL_COLOR_BUFFER_BIT);
	glClearColor(
		cfg.BG.r / 255.0f, cfg.BG.g / 255.0f, cfg.BG.b / 255.0f,
		cfg.BG.a / 255.0f);

	nk_sdl_render(NK_ANTI_ALIASING_ON);
	SDL_GL_SwapWindow(cfg.win);
}

// Custom controls
int nk_combo_separator_image(struct nk_context *ctx,
	const GLuint *img_ids, const char *items_separated_by_separator,
	int separator, int selected, int count, int item_height,
	struct nk_vec2 size)
{
	int i;
	int max_height;
	struct nk_vec2 item_spacing;
	struct nk_vec2 window_padding;
	const char *current_item;
	const char *iter;
	int length = 0;

	NK_ASSERT(ctx);
	NK_ASSERT(items_separated_by_separator);
	if (!ctx || !items_separated_by_separator)
		return selected;

	/* calculate popup window */
	item_spacing = ctx->style.window.spacing;
	window_padding = nk_panel_get_padding(&ctx->style, ctx->current->layout->type);
	max_height = count * item_height + count * (int)item_spacing.y;
	max_height += (int)item_spacing.y * 2 + (int)window_padding.y * 2;
	size.y = NK_MIN(size.y, (float)max_height);

	/* find selected item */
	current_item = items_separated_by_separator;
	for (i = 0; i < count; ++i) {
		iter = current_item;
		while (*iter && *iter != separator) iter++;
		length = (int)(iter - current_item);
		if (i == selected) break;
		current_item = iter + 1;
	}

	// Get widget bounds for drawing currently selected item image later
	struct nk_rect bounds;
	nk_layout_widget_space(&bounds, ctx, ctx->current, nk_false);
	bounds.x += size.x - 56;
	bounds.y += 2;
	bounds.w = (float)12 * PIC_SCALE;
	bounds.h = (float)12 * PIC_SCALE;

	if (nk_combo_begin_text(ctx, current_item, length, size)) {
		current_item = items_separated_by_separator;
		nk_layout_row_dynamic(ctx, (float)item_height, 1);
		for (i = 0; i < count; ++i) {
			const struct nk_image img = nk_image_id(img_ids[i]);
			// TODO: image size
			BeforeDrawTex(img_ids[i]);
			iter = current_item;
			while (*iter && *iter != separator) iter++;
			length = (int)(iter - current_item);
			if (nk_contextual_item_image_text(ctx, img, current_item, length, NK_TEXT_LEFT))
				selected = i;
			current_item = current_item + length + 1;
		}
		nk_combo_end(ctx);
	}

	// Also draw currently selected image
	const struct nk_image comboImg = nk_image_id(img_ids[selected]);
	BeforeDrawTex(img_ids[selected]);
	nk_draw_image(&ctx->current->buffer, bounds, &comboImg, nk_white);

	return selected;
}
void DrawCharacter(
	struct nk_context *ctx, Character *c, GLuint *texids,
	const struct vec2i pos, const Animation *anim, const direction_e d)
{
	const int frame = AnimationGetFrame(anim);
	ActorPics pics = GetCharacterPics(
		c, d, d, anim->Type, frame,
		c->Gun->Sprites, GUNSTATE_READY, colorTransparent, NULL, NULL, 0);
	for (int i = 0; i < BODY_PART_COUNT; i++)
	{
		const Pic *pic = pics.OrderedPics[i];
		if (pic == NULL)
		{
			continue;
		}
		const struct vec2i drawPos = svec2i_add(pos, pics.OrderedOffsets[i]);
		LoadTexFromPic(texids[i], pic);
		struct nk_image tex = nk_image_id((int)texids[i]);
		BeforeDrawTex(texids[i]);
		struct nk_rect bounds;
		nk_layout_widget_space(&bounds, ctx, ctx->current, nk_true);
		bounds.x += drawPos.x * PIC_SCALE + 32;
		bounds.y += drawPos.y * PIC_SCALE + 32;
		bounds.w = (float)pic->size.x * PIC_SCALE;
		bounds.h = (float)pic->size.y * PIC_SCALE;
		nk_draw_image(&ctx->current->buffer, bounds, &tex, nk_white);
	}
}

static Pic PadEven(const Pic *pic);
void LoadTexFromPic(const GLuint texid, const Pic *pic)
{
	glBindTexture(GL_TEXTURE_2D, texid);
	Pic padded = PadEven(pic);
	glTexImage2D(
		GL_TEXTURE_2D, 0, GL_RGBA, padded.size.x, padded.size.y, 0, GL_BGRA,
		GL_UNSIGNED_BYTE, padded.Data);
	PicFree(&padded);
	glBindTexture(GL_TEXTURE_2D, 0);
}
static Pic PadEven(const Pic *pic)
{
	// OGL needs even dimensions for texture
	Pic p;
	memset(&p, 0, sizeof p);
	p.size = svec2i((pic->size.x + 1) / 2 * 2, (pic->size.y + 1) / 2 * 2);
	CCALLOC(p.Data, p.size.x * p.size.y * sizeof *((Pic *)0)->Data);
	for (int i = 0; i < pic->size.y; i++)
	{
		memcpy(
			p.Data + i * p.size.x, pic->Data + i * pic->size.x,
			pic->size.x * sizeof *p.Data);
	}
	return p;
}

void BeforeDrawTex(const GLuint texid)
{
	glBindTexture(GL_TEXTURE_2D, texid);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}
