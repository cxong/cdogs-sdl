/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
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
#include "char_editor.h"

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_opengl.h>
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_SDL_GL2_IMPLEMENTATION
#include <nuklear/nuklear.h>
#include <nuklear/nuklear_sdl_gl2.h>
#include <cdogs/actors.h>
#include <cdogs/log.h>

#define MAX_VERTEX_MEMORY 512 * 1024
#define MAX_ELEMENT_MEMORY 128 * 1024


static bool HandleEvents(struct nk_context *ctx, EventHandlers *handlers);
void CharEditor(
	CampaignSetting *setting, EventHandlers *handlers, int *fileChanged)
{
	UNUSED(setting);
	UNUSED(fileChanged);

	// TODO: reset SDL; SDL_Renderer messes with nuklear

	SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "1");
	SDL_Init(SDL_INIT_VIDEO);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

	SDL_Window *win = SDL_CreateWindow("Character Editor",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		600, 600,
		SDL_WINDOW_OPENGL|SDL_WINDOW_SHOWN|SDL_WINDOW_RESIZABLE);

	SDL_GLContext glContext = SDL_GL_CreateContext(win);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	struct nk_context *ctx = nk_sdl_init(win);

	// Initialise fonts
	struct nk_font_atlas *atlas;
	nk_sdl_font_stash_begin(&atlas);
	nk_sdl_font_stash_end();

	// Properties
	int selected = 0;
	const int len = 3;
	int speed = 256;
	unsigned int flags = 0;

	struct nk_color background = nk_rgb(42, 25, 25);
	Uint32 ticksNow = SDL_GetTicks();
	Uint32 ticksElapsed = 0;
	for (;;)
	{
		Uint32 ticksThen = ticksNow;
		ticksNow = SDL_GetTicks();
		ticksElapsed += ticksNow - ticksThen;
		if (ticksElapsed < 1000 / FPS_FRAMELIMIT * 2)
		{
			SDL_Delay(1);
			continue;
		}

		if (!HandleEvents(ctx, handlers))
		{
			goto bail;
		}

		if (nk_begin(ctx, "Character", nk_rect(10, 10, 250, 500),
			NK_WINDOW_BORDER|NK_WINDOW_TITLE))
		{
			nk_layout_row_dynamic(ctx, 20, 1);
			nk_label(ctx, "Class", NK_TEXT_LEFT);
			nk_layout_row_dynamic(ctx, 25, 1);
			nk_combobox_string(
				ctx, "WarBaby\0Ice\0Ogre", &selected, len, 25,
				nk_vec2(nk_widget_width(ctx), len * 25));

			nk_layout_row_dynamic(ctx, 20, 1);
			nk_label(ctx, "Skin", NK_TEXT_LEFT);
			nk_layout_row_dynamic(ctx, 25, 1);
			if (nk_combo_begin_color(ctx, background, nk_vec2(nk_widget_width(ctx),400))) {
				nk_layout_row_dynamic(ctx, 120, 1);
				background = nk_color_picker(ctx, background, NK_RGBA);
				nk_layout_row_dynamic(ctx, 25, 1);
				background.r = (nk_byte)nk_propertyi(ctx, "#R:", 0, background.r, 255, 1,1);
				background.g = (nk_byte)nk_propertyi(ctx, "#G:", 0, background.g, 255, 1,1);
				background.b = (nk_byte)nk_propertyi(ctx, "#B:", 0, background.b, 255, 1,1);
				background.a = (nk_byte)nk_propertyi(ctx, "#A:", 0, background.a, 255, 1,1);
				nk_combo_end(ctx);
			}
			// TODO: Arms, Body, Legs, Hair

			// Speed (256 = 100%)
			nk_layout_row_dynamic(ctx, 25, 1);
			nk_property_int(ctx, "Speed (%):", 0, &speed, 400, 10, 1);

			nk_layout_row_dynamic(ctx, 20, 1);
			nk_label(ctx, "Gun", NK_TEXT_LEFT);
			nk_layout_row_dynamic(ctx, 25, 1);
			nk_combobox_string(
				ctx, "Machine Gun\0Shotgun\0Powergun", &selected, len, 25,
				nk_vec2(nk_widget_width(ctx), len * 25));

			nk_layout_row_dynamic(ctx, 25, 1);
			nk_property_int(ctx, "Max Health:", 10, &speed, 1000, 10, 1);

			struct nk_rect bounds = nk_widget_bounds(ctx);
			nk_checkbox_flags_label(ctx, "Asbestos", &flags, FLAGS_ASBESTOS);
			if (nk_input_is_mouse_hovering_rect(&ctx->input, bounds))
			{
				nk_tooltip(ctx, "This is a tooltip");
			}
			nk_checkbox_flags_label(ctx, "Immunity", &flags, FLAGS_IMMUNITY);
			nk_checkbox_flags_label(
				ctx, "See-through", &flags, FLAGS_SEETHROUGH);
			// TODO: run-away, sneaky, good guy, asleep, prisoner, invulnerable,
			// follower, penalty, victim, awake
			
			enum {EASY, HARD};
			static int op = EASY;

			nk_layout_row_static(ctx, 30, 80, 1);
			if (nk_button_label(ctx, "button"))
			{
				fprintf(stdout, "button pressed\n");
			}
			nk_layout_row_dynamic(ctx, 30, 2);
			if (nk_option_label(ctx, "easy", op == EASY)) op = EASY;
			if (nk_option_label(ctx, "hard", op == HARD)) op = HARD;

			/*nk_layout_row_dynamic(ctx, 20, 1);
			nk_label(ctx, "Image:", NK_TEXT_LEFT);
			nk_layout_row_static(
				ctx,
				tex_h, tex_w,
				1);
			nk_image(ctx, tex);*/
		}
		nk_end(ctx);

		if (nk_begin(ctx, "AI", nk_rect(270, 10, 250, 180),
			NK_WINDOW_BORDER|NK_WINDOW_TITLE))
		{
			nk_layout_row_dynamic(ctx, 25, 1);
			nk_property_int(ctx, "Move (%):", 0, &speed, 100, 5, 1);
			nk_layout_row_dynamic(ctx, 25, 1);
			nk_property_int(ctx, "Track (%):", 0, &speed, 100, 5, 1);
			nk_layout_row_dynamic(ctx, 25, 1);
			nk_property_int(ctx, "Shoot (%):", 0, &speed, 100, 5, 1);
			nk_layout_row_dynamic(ctx, 25, 1);
			nk_property_int(ctx, "Action delay:", 0, &speed, 50, 5, 1);
		}
		nk_end(ctx);

		/* Draw */
		{float bg[4];
		nk_color_fv(bg, background);
		int winWidth, winHeight;
		SDL_GetWindowSize(win, &winWidth, &winHeight);
		glViewport(0, 0, winWidth, winHeight);
		glClear(GL_COLOR_BUFFER_BIT);
		glClearColor(bg[0], bg[1], bg[2], bg[3]);

		// Blit the texture to screen
		//draw_tex(tex.handle.id, -0.5f, -0.5f, 1.f, 1.f);
		nk_sdl_render(NK_ANTI_ALIASING_ON, MAX_VERTEX_MEMORY, MAX_ELEMENT_MEMORY);

		// Display
		SDL_GL_SwapWindow(win);}
		ticksElapsed -= 1000 / (FPS_FRAMELIMIT * 2);
	}

bail:
	nk_sdl_shutdown();
	//glDeleteTextures(1, (const GLuint *)&tex.handle.id);
	SDL_GL_DeleteContext(glContext);
	SDL_DestroyWindow(win);
}

static bool HandleEvents(struct nk_context *ctx, EventHandlers *handlers)
{
	SDL_Event e;
	nk_input_begin(ctx);
	bool run = true;
	while (SDL_PollEvent(&e))
	{
		switch (e.type)
		{
			case SDL_KEYDOWN:
				if (e.key.repeat)
				{
					break;
				}
				KeyOnKeyDown(&handlers->keyboard, e.key.keysym);
				break;
			case SDL_KEYUP:
				KeyOnKeyUp(&handlers->keyboard, e.key.keysym);
				break;
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
	nk_input_end(ctx);
	return run;
}
