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
#include "destroy_objective_dialog.h"

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
	const MapObject *mo;		// last selected
	CArray texIdsMapObjects; // of GLuint
} DestroyObjectiveData;

static bool Draw(SDL_Window *win, struct nk_context *ctx, void *data);
EditorResult DestroyObjectiveDialog(
	PicManager *pm, EventHandlers *handlers, Objective *o)
{
	NKWindowConfig cfg;
	memset(&cfg, 0, sizeof cfg);
	cfg.Title = "Select Destroy Objectives";
	cfg.Size = svec2i(WIDTH, HEIGHT);
	color_t bg = {41, 26, 26, 255};
	cfg.BG = bg;
	cfg.Handlers = handlers;
	cfg.Draw = Draw;
	cfg.WindowFlags = SDL_WINDOW_RESIZABLE;

	NKWindowInit(&cfg);

	DestroyObjectiveData data;
	memset(&data, 0, sizeof data);
	data.pm = pm;
	data.o = o;
	CArrayInitFill(
		&data.texIdsMapObjects, sizeof(GLuint),
		MapObjectsCount(&gMapObjects), NULL);
	glGenTextures(
	    MapObjectsCount(&gMapObjects),
		(GLuint *)data.texIdsMapObjects.data);
	cfg.DrawData = &data;
	data.ctx = cfg.ctx;

	NKWindow(cfg);

	glDeleteTextures(
		(GLsizei)data.texIdsMapObjects.size,
		(const GLuint *)data.texIdsMapObjects.data);
	CArrayTerminate(&data.texIdsMapObjects);
	return EDITOR_RESULT_CHANGE_TOOL;
}

static void DrawOpsRow(
	struct nk_context *ctx, DestroyObjectiveData *dData, bool *result);
static void DrawMapObject(
	DestroyObjectiveData *data, const MapObject *mo, const int idx);
static void MapObjectDrawPropsSidebar(
	struct nk_context *ctx, const MapObject *mo, const float rh);
static bool Draw(SDL_Window *win, struct nk_context *ctx, void *data)
{
	bool result = true;
	DestroyObjectiveData *dData = data;
	struct vec2i windowSize;
	SDL_GetWindowSize(win, &windowSize.x, &windowSize.y);
	const float mainWidth = (float)(windowSize.x - SIDE_WIDTH);

	if (nk_begin(
			ctx, "", nk_rect(0, 0, mainWidth, (float)windowSize.y),
			NK_WINDOW_BORDER))
	{
		DrawOpsRow(ctx, dData, & result);

		nk_layout_row_dynamic(ctx, 30 * PIC_SCALE, (int)(mainWidth / 90));
		for (int i = 0; i < MapObjectsCount(&gMapObjects); i++)
		{
			const MapObject *mo = IndexMapObject(i);
			DrawMapObject(dData, mo, i);
		}
		nk_end(ctx);
	}

	if (nk_begin(
			ctx, "Properties",
			nk_rect(mainWidth, 0, SIDE_WIDTH, (float)windowSize.y),
			NK_WINDOW_BORDER | NK_WINDOW_TITLE))
	{
		if (dData->mo != NULL)
		{
			MapObjectDrawPropsSidebar(ctx, dData->mo, ROW_HEIGHT);
		}
		nk_end(ctx);
	}

	return result;
}
static void DrawOpsRow(
	struct nk_context *ctx, DestroyObjectiveData *dData, bool *result)
{
	nk_layout_row_dynamic(ctx, ROW_HEIGHT, 3);
	if (nk_button_label(ctx, "Select All"))
	{
		CArrayClear(&dData->o->u.MapObjects);
		for (int i = 0; i < MapObjectsCount(&gMapObjects); i++)
		{
			const MapObject *mo = IndexMapObject(i);
			CArrayPushBack(&dData->o->u.MapObjects, &mo);
		}
	}
	if (nk_button_label(ctx, "Select None"))
	{
		CArrayClear(&dData->o->u.MapObjects);
	}
	if (nk_button_label(ctx, "Done"))
	{
		*result = false;
	}
}

static void DrawMapObject(
	DestroyObjectiveData *data, const MapObject *mo, const int idx)
{
	bool selected = false;
	CA_FOREACH(const MapObject *, m, data->o->u.MapObjects)
	if (*m == mo)
	{
		selected = true;
		break;
	}
	CA_FOREACH_END()
	if (nk_select_label(
			data->ctx, mo->Name, NK_TEXT_ALIGN_BOTTOM | NK_TEXT_ALIGN_CENTERED,
			selected) != selected)
	{
		data->mo = mo;
		if (selected)
		{
			// deselect
			CA_FOREACH(const MapObject *, m, data->o->u.MapObjects)
			if (*m == mo)
			{
				CArrayDelete(&data->o->u.MapObjects, _ca_index);
				break;
			}
			CA_FOREACH_END()
		}
		else
		{
			// select (add)
			CArrayPushBack(&data->o->u.MapObjects, &mo);
			// Sort and remove duplicates
			qsort(
				data->o->u.MapObjects.data, data->o->u.MapObjects.size,
				data->o->u.MapObjects.elemSize, CompareIntsAsc);
			CArrayUnique(&data->o->u.MapObjects, IntsEqual);
		}
	}
	const GLuint *texid = CArrayGet(&data->texIdsMapObjects, idx);
	const Pic *pic = CPicGetPic(&mo->Pic, 0);
	DrawPic(data->ctx, pic, *texid, svec2i(-40, 5), PIC_SCALE);
}

static void MapObjectDrawPropsSidebar(
	struct nk_context *ctx, const MapObject *mo, const float rh)
{
	nk_layout_row_dynamic(ctx, rh, 1);

	char buf[256];

	nk_label(ctx, mo->Name, NK_TEXT_LEFT);

	sprintf(buf, "Health: %d", mo->Health);
	nk_label(ctx, buf, NK_TEXT_LEFT);

	char pbuf[256];
	MapObjectGetPlacementFlagNames(mo, pbuf, "\n");
	sprintf(buf, "Placement:\n%s", pbuf);
	nk_label(ctx, buf, NK_TEXT_LEFT);
	
	if (mo->DestroyGuns.size > 0)
	{
		char exBuf[256];
		MapObjectGetExplosionGunNames(mo, exBuf, "\n");
		sprintf(buf, "Explodes:\n%s", exBuf);
		nk_label(ctx, buf, NK_TEXT_LEFT);
	}
}
