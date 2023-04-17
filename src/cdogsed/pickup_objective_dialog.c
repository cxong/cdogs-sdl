/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.
	Copyright (c) 2023 Cong Xu
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
#include "pickup_objective_dialog.h"

#include "editor_ui_common.h"
#include "nk_window.h"

#define WIDTH 500
#define HEIGHT 500
#define SIDE_WIDTH 150
#define ROW_HEIGHT 25

typedef struct
{
	struct nk_context *ctx;
	const PicManager *pm;
	Objective *o;
	const PickupClass *pc;		// last selected
	CArray texIdsPickupClasses; // of GLuint
} PickupObjectiveData;

static bool Draw(SDL_Window *win, struct nk_context *ctx, void *data);
EditorResult PickupObjectiveDialog(
	PicManager *pm, EventHandlers *handlers, Objective *o)
{
	NKWindowConfig cfg;
	memset(&cfg, 0, sizeof cfg);
	cfg.Title = "Select Objective Pickups";
	cfg.Size = svec2i(WIDTH, HEIGHT);
	color_t bg = {41, 26, 26, 255};
	cfg.BG = bg;
	cfg.Handlers = handlers;
	cfg.Draw = Draw;
	cfg.WindowFlags = SDL_WINDOW_RESIZABLE;

	NKWindowInit(&cfg);

	PickupObjectiveData data;
	memset(&data, 0, sizeof data);
	data.pm = pm;
	data.o = o;
	CArrayInitFill(
		&data.texIdsPickupClasses, sizeof(GLuint),
		PickupClassesCount(&gPickupClasses), NULL);
	glGenTextures(
		PickupClassesCount(&gPickupClasses),
		(GLuint *)data.texIdsPickupClasses.data);
	cfg.DrawData = &data;
	data.ctx = cfg.ctx;

	NKWindow(cfg);

	glDeleteTextures(
		(GLsizei)data.texIdsPickupClasses.size,
		(const GLuint *)data.texIdsPickupClasses.data);
	CArrayTerminate(&data.texIdsPickupClasses);
	return EDITOR_RESULT_CHANGE_TOOL;
}

static void DrawOpsRow(
	struct nk_context *ctx, PickupObjectiveData *pData, bool *result);
static void DrawPickup(
	PickupObjectiveData *data, const PickupClass *pc, const int idx);
static bool Draw(SDL_Window *win, struct nk_context *ctx, void *data)
{
	bool result = true;
	PickupObjectiveData *pData = data;
	struct vec2i windowSize;
	SDL_GetWindowSize(win, &windowSize.x, &windowSize.y);
	const float mainWidth = (float)(windowSize.x - SIDE_WIDTH);

	if (nk_begin(
			ctx, "", nk_rect(0, 0, mainWidth, (float)windowSize.y),
			NK_WINDOW_BORDER))
	{
		DrawOpsRow(ctx, pData, & result);

		nk_layout_row_dynamic(ctx, 30 * PIC_SCALE, (int)(mainWidth / 90));
		for (int i = 0; i < PickupClassesCount(&gPickupClasses); i++)
		{
			const PickupClass *pc = PickupClassGetById(&gPickupClasses, i);
			DrawPickup(pData, pc, i);
		}
		nk_end(ctx);
	}

	if (nk_begin(
			ctx, "Properties",
			nk_rect(mainWidth, 0, SIDE_WIDTH, (float)windowSize.y),
			NK_WINDOW_BORDER | NK_WINDOW_TITLE))
	{
		if (pData->pc != NULL)
		{
			PickupClassDrawPropsSidebar(ctx, pData->pc, ROW_HEIGHT);
		}
		nk_end(ctx);
	}

	return result;
}
static void DrawOpsRow(
	struct nk_context *ctx, PickupObjectiveData *pData, bool *result)
{
	nk_layout_row_dynamic(ctx, ROW_HEIGHT, 3);
	if (nk_button_label(ctx, "Select All"))
	{
		CArrayClear(&pData->o->u.Pickups);
		for (int i = 0; i < PickupClassesCount(&gPickupClasses); i++)
		{
			const PickupClass *pc = PickupClassGetById(&gPickupClasses, i);
			CArrayPushBack(&pData->o->u.Pickups, &pc);
		}
	}
	if (nk_button_label(ctx, "Select None"))
	{
		CArrayClear(&pData->o->u.Pickups);
	}
	if (nk_button_label(ctx, "Done"))
	{
		*result = false;
	}
}

static void DrawPickup(
	PickupObjectiveData *data, const PickupClass *pc, const int idx)
{
	bool selected = false;
	CA_FOREACH(const PickupClass *, p, data->o->u.Pickups)
	if (*p == pc)
	{
		selected = true;
		break;
	}
	CA_FOREACH_END()
	if (nk_select_label(
			data->ctx, pc->Name, NK_TEXT_ALIGN_BOTTOM | NK_TEXT_ALIGN_CENTERED,
			selected) != selected)
	{
		data->pc = pc;
		if (selected)
		{
			// deselect
			CA_FOREACH(const PickupClass *, p, data->o->u.Pickups)
			if (*p == pc)
			{
				CArrayDelete(&data->o->u.Pickups, _ca_index);
				break;
			}
			CA_FOREACH_END()
		}
		else
		{
			// select (add)
			CArrayPushBack(&data->o->u.Pickups, &pc);
			// Sort and remove duplicates
			qsort(
				data->o->u.Pickups.data, data->o->u.Pickups.size,
				data->o->u.Pickups.elemSize, CompareIntsAsc);
			CArrayUnique(&data->o->u.Pickups, IntsEqual);
		}
	}
	const GLuint *texid = CArrayGet(&data->texIdsPickupClasses, idx);
	const Pic *pic = CPicGetPic(&pc->Pic, 0);
	DrawPic(data->ctx, pic, *texid, svec2i(-40, 5), PIC_SCALE);
}
