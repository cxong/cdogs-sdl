/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.
	Copyright (c) 2020 Cong Xu
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
#include "exit_brush.h"

#include "editor_ui_common.h"
#include "nk_window.h"

#define WIDTH 500
#define HEIGHT 200
#define MAIN_WIDTH 300
#define SIDE_WIDTH (WIDTH - MAIN_WIDTH)
#define ROW_HEIGHT 25

typedef struct
{
	struct nk_context *ctx;
	Campaign *co;
	Mission *m;
	int *exitIdx;
	EditorResult result;
} ExitBrushData;

static bool Draw(SDL_Window *win, struct nk_context *ctx, void *data);
EditorResult ExitBrush(EventHandlers *handlers, Campaign *co, int *exitIdx)
{
	NKWindowConfig cfg;
	memset(&cfg, 0, sizeof cfg);
	cfg.Title = "Exit Brush";
	cfg.Size = svec2i(WIDTH, HEIGHT);
	color_t bg = {41, 26, 26, 255};
	cfg.BG = bg;
	cfg.Handlers = handlers;
	cfg.Draw = Draw;

	NKWindowInit(&cfg);

	ExitBrushData data;
	memset(&data, 0, sizeof data);
	data.co = co;
	data.m = CampaignGetCurrentMission(co);
	CASSERT(data.m->Type == MAPTYPE_STATIC, "unexpected map type");
	data.exitIdx = exitIdx;
	cfg.DrawData = &data;
	data.ctx = cfg.ctx;

	NKWindow(cfg);

	return data.result;
}

static void DrawOpsRow(
	struct nk_context *ctx, ExitBrushData *eData, bool *result);
static void DrawPropsSidebar(
	struct nk_context *ctx, ExitBrushData *eData, Exit *selectedExit);
static void DrawExit(ExitBrushData *eData, Exit *e, const int idx);
static bool Draw(SDL_Window *win, struct nk_context *ctx, void *data)
{
	UNUSED(win);
	bool result = true;
	ExitBrushData *eData = data;
	Exit *selectedExit = CArrayGet(&eData->m->u.Static.Exits, *eData->exitIdx);

	if (nk_begin(
			ctx, "Properties", nk_rect(0, 0, SIDE_WIDTH, HEIGHT),
			NK_WINDOW_BORDER | NK_WINDOW_TITLE) &&
		selectedExit != NULL)
	{
		DrawPropsSidebar(ctx, eData, selectedExit);
	}
	nk_end(ctx);

	if (nk_begin(
			ctx, "", nk_rect(SIDE_WIDTH, 0, MAIN_WIDTH, HEIGHT),
			NK_WINDOW_BORDER))
	{
		DrawOpsRow(ctx, eData, &result);

		nk_layout_row_dynamic(ctx, 15 * PIC_SCALE, 1);
		CA_FOREACH(Exit, e, eData->m->u.Static.Exits)
		DrawExit(eData, e, _ca_index);
		CA_FOREACH_END()
	}
	nk_end(ctx);
	return result;
}
static void DrawOpsRow(
	struct nk_context *ctx, ExitBrushData *eData, bool *result)
{
	nk_layout_row_dynamic(ctx, ROW_HEIGHT, 3);
	if (nk_button_label(ctx, "Done"))
	{
		*result = false;
	}
	if (nk_button_label(ctx, "Add"))
	{
		Exit e;
		memset(&e, 0, sizeof e);
		e.Mission = eData->co->MissionIndex + 1;
		*eData->exitIdx = (int)eData->m->u.Static.Exits.size;
		CArrayPushBack(&eData->m->u.Static.Exits, &e);
		eData->result |= EDITOR_RESULT_CHANGED;
	}
	if (eData->m->u.Static.Exits.size > 0 && nk_button_label(ctx, "Remove"))
	{
		CArrayDelete(&eData->m->u.Static.Exits, *eData->exitIdx);
		*eData->exitIdx =
			MIN(*eData->exitIdx, (int)eData->m->u.Static.Exits.size - 1);
		eData->result |= EDITOR_RESULT_CHANGED_AND_RELOAD;
	}
}
static void DrawPropsSidebar(
	struct nk_context *ctx, ExitBrushData *eData, Exit *selectedExit)
{
	nk_layout_row_dynamic(ctx, ROW_HEIGHT, 1);

	char buf[256];

	// Location
	struct nk_rect bounds = nk_widget_bounds(ctx);
	sprintf(
		buf, "Location: %d, %d", selectedExit->R.Pos.x, selectedExit->R.Pos.y);
	nk_label(ctx, buf, NK_TEXT_LEFT);
	if (nk_input_is_mouse_hovering_rect(&ctx->input, bounds))
	{
		nk_tooltip(ctx, "Click and drag location in main editor");
	}

	// Size
	sprintf(
		buf, "Size: %d x %d", selectedExit->R.Size.x, selectedExit->R.Size.y);
	nk_label(ctx, buf, NK_TEXT_LEFT);

	// Mission
	bounds = nk_widget_bounds(ctx);
	const int mission = selectedExit->Mission;
	nk_property_int(
		ctx, "Mission", 0, &selectedExit->Mission,
		(int)eData->co->Setting.Missions.size - 1, 1, 1.f);
	if (nk_input_is_mouse_hovering_rect(&ctx->input, bounds))
	{
		sprintf(buf, "Current mission: %d", eData->co->MissionIndex);
		nk_tooltip(ctx, buf);
	}
	if (mission != selectedExit->Mission)
	{
		eData->result |= EDITOR_RESULT_CHANGED;
	}

	// Hidden
	if (DrawCheckbox(
			ctx, "Hidden", "Whether this exit shows up in compass and automap",
			&selectedExit->Hidden))
	{
		eData->result |= EDITOR_RESULT_CHANGED;
	}
}

static void DrawExit(ExitBrushData *eData, Exit *e, const int idx)
{
	char buf[256];
	sprintf(
		buf, "%d: mission %d (%d,%d %dx%d)", idx, e->Mission, e->R.Pos.x,
		e->R.Pos.y, e->R.Size.x, e->R.Size.y);
	const int selected = *eData->exitIdx == idx;
	if (nk_select_label(
			eData->ctx, buf, NK_TEXT_ALIGN_BOTTOM | NK_TEXT_ALIGN_CENTERED,
			selected))
	{
		*eData->exitIdx = idx;
	}
}
