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
#include "tile_brush.h"

#include "nk_window.h"


static void Draw(SDL_Window *win, struct nk_context *ctx, void *data);
void TileBrush(EventHandlers *handlers)
{
	NKWindowConfig cfg;
	memset(&cfg, 0, sizeof cfg);
	cfg.Title = "Tile Brush";
	cfg.Size = svec2i(400, 300);
	cfg.MinSize = cfg.Size;
	cfg.WindowFlags = 0;
	color_t bg = { 41, 26, 26, 255 };
	cfg.BG = bg;
	cfg.Handlers = handlers;
	cfg.Draw = Draw;
	cfg.DrawData = NULL;

	NKWindowInit(&cfg);
	NKWindow(cfg);
}
static void Draw(SDL_Window *win, struct nk_context *ctx, void *data)
{
	UNUSED(win);
	UNUSED(data);
	if (nk_begin(ctx, "Tile Brush",
		nk_rect(10, 10, 400 - 10, 300 - 10),
		NK_WINDOW_BORDER | NK_WINDOW_TITLE))
	{
	}
	nk_end(ctx);
}
