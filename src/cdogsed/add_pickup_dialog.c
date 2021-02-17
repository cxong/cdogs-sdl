/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.
	Copyright (c) 2020-2021 Cong Xu
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
#include "add_pickup_dialog.h"

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
	const PickupClass **pc;
	CArray texIdsPickupClasses; // of GLuint
} AddPickupData;

static bool Draw(SDL_Window *win, struct nk_context *ctx, void *data);
EditorResult AddPickupDialog(
	PicManager *pm, EventHandlers *handlers, const PickupClass **pc)
{
	NKWindowConfig cfg;
	memset(&cfg, 0, sizeof cfg);
	cfg.Title = "Add Pickup";
	cfg.Size = svec2i(WIDTH, HEIGHT);
	color_t bg = {41, 26, 26, 255};
	cfg.BG = bg;
	cfg.Handlers = handlers;
	cfg.Draw = Draw;
	cfg.WindowFlags = SDL_WINDOW_RESIZABLE;

	NKWindowInit(&cfg);

	AddPickupData data;
	memset(&data, 0, sizeof data);
	data.pm = pm;
	data.pc = pc;
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

static void DrawPropsSidebar(struct nk_context *ctx, const PickupClass *pc);
static void DrawOpsRow(struct nk_context *ctx, bool *result);
static void DrawPickup(
	AddPickupData *aData, const PickupClass *pc, const int idx);
static bool Draw(SDL_Window *win, struct nk_context *ctx, void *data)
{
	bool result = true;
	AddPickupData *aData = data;
	struct vec2i windowSize;
	SDL_GetWindowSize(win, &windowSize.x, &windowSize.y);
	const float mainWidth = (float)(windowSize.x - SIDE_WIDTH);

	if (nk_begin(
			ctx, "", nk_rect(0, 0, mainWidth, (float)windowSize.y),
			NK_WINDOW_BORDER))
	{
		DrawOpsRow(ctx, &result);

		nk_layout_row_dynamic(ctx, 30 * PIC_SCALE, (int)(mainWidth / 90));
		for (int i = 0; i < PickupClassesCount(&gPickupClasses); i++)
		{
			const PickupClass *pc = PickupClassGetById(&gPickupClasses, i);
			DrawPickup(aData, pc, i);
		}
		nk_end(ctx);
	}

	if (nk_begin(
			ctx, "Properties",
			nk_rect(mainWidth, 0, SIDE_WIDTH, (float)windowSize.y),
			NK_WINDOW_BORDER | NK_WINDOW_TITLE))
	{
		if (*aData->pc != NULL)
		{
			DrawPropsSidebar(ctx, *aData->pc);
		}
		nk_end(ctx);
	}

	return result;
}
static void DrawPropsSidebar(struct nk_context *ctx, const PickupClass *pc)
{
	nk_layout_row_dynamic(ctx, ROW_HEIGHT, 1);

	char buf[256];

	nk_label(ctx, pc->Name, NK_TEXT_LEFT);

	sprintf(buf, "Type: %s", PickupTypeStr(pc->Type));
	nk_label(ctx, buf, NK_TEXT_LEFT);

	switch (pc->Type)
	{
	case PICKUP_JEWEL:
		sprintf(buf, "Score: %d", pc->u.Score);
		nk_label(ctx, buf, NK_TEXT_LEFT);
		break;
	case PICKUP_HEALTH:
		sprintf(buf, "Health: %d", pc->u.Health);
		nk_label(ctx, buf, NK_TEXT_LEFT);
		break;
	case PICKUP_AMMO: {
		const Ammo *a = AmmoGetById(&gAmmo, pc->u.Ammo.Id);
		sprintf(buf, "Ammo: %s", a->Name);
		nk_label(ctx, buf, NK_TEXT_LEFT);
		sprintf(buf, "Amount: %d", (int)pc->u.Ammo.Amount);
		nk_label(ctx, buf, NK_TEXT_LEFT);
	}
	break;
	case PICKUP_KEYCARD:
		// TODO: draw colour square of key
		break;
	case PICKUP_GUN: {
		const WeaponClass *wc = IdWeaponClass(pc->u.GunId);
		sprintf(buf, "Weapon: %s", wc->name);
		nk_label(ctx, buf, NK_TEXT_LEFT);
	}
	case PICKUP_SHOW_MAP:
		break;
		break;
	default:
		CASSERT(false, "Unknown pickup type");
		break;
	}
}
static void DrawOpsRow(struct nk_context *ctx, bool *result)
{
	nk_layout_row_dynamic(ctx, ROW_HEIGHT, 1);
	if (nk_button_label(ctx, "Done"))
	{
		*result = false;
	}
}

static void DrawPickup(
	AddPickupData *aData, const PickupClass *pc, const int idx)
{
	const bool selected = *aData->pc == pc;
	if (nk_select_label(
			aData->ctx, pc->Name,
			NK_TEXT_ALIGN_BOTTOM | NK_TEXT_ALIGN_CENTERED, selected))
	{
		*aData->pc = pc;
	}
	const GLuint *texid = CArrayGet(&aData->texIdsPickupClasses, idx);
	const Pic *pic = CPicGetPic(&pc->Pic, 0);
	DrawPic(aData->ctx, pic, *texid, svec2i(-40, 5), PIC_SCALE);
}
