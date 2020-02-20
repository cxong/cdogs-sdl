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
#include "tile_brush.h"

#include "editor_ui_common.h"
#include "nk_window.h"


#define WIDTH 400
#define HEIGHT 300
#define ROW_HEIGHT 25


typedef struct
{
	struct nk_context *ctx;
	const PicManager *pm;
	Mission *m;
	int *brushIdx;
	CArray texIdsTileClasses;	// of GLuint
	int tileIdx;
} TileBrushData;


static void ResetTexIds(TileBrushData *data);
static void Draw(SDL_Window *win, struct nk_context *ctx, void *data);
void TileBrush(
	const PicManager *pm, EventHandlers *handlers, CampaignOptions *co,
	int *brushIdx)
{
	NKWindowConfig cfg;
	memset(&cfg, 0, sizeof cfg);
	cfg.Title = "Tile Brush";
	cfg.Size = svec2i(WIDTH, HEIGHT);
	color_t bg = { 41, 26, 26, 255 };
	cfg.BG = bg;
	cfg.Handlers = handlers;
	cfg.Draw = Draw;

	NKWindowInit(&cfg);

	TileBrushData data;
	data.pm = pm;
	data.m = CampaignGetCurrentMission(co);
	CASSERT(data.m->Type == MAPTYPE_STATIC, "unexpected map type");
	data.brushIdx = brushIdx;
	CArrayInit(&data.texIdsTileClasses, sizeof(GLuint));
	ResetTexIds(&data);
	cfg.DrawData = &data;
	data.ctx = cfg.ctx;

	NKWindow(cfg);

	glDeleteTextures(
		(GLsizei)data.texIdsTileClasses.size,
		(const GLuint *)data.texIdsTileClasses.data);
	CArrayTerminate(&data.texIdsTileClasses);
}
static int DrawTileType(any_t data, any_t key);
static void Draw(SDL_Window *win, struct nk_context *ctx, void *data)
{
	UNUSED(win);
	TileBrushData *tbData = data;
	if (nk_begin(ctx, "", nk_rect(0, 0, WIDTH, HEIGHT), 0))
	{
		nk_layout_row_dynamic(ctx, ROW_HEIGHT, 5);
		if (nk_button_label(ctx, "Add"))
		{
			TileClass tc;
			TileClassInit(
				&tc, &gPicManager, &gTileFloor, "tile", "normal",
				colorGray, colorGray);
			if (MissionStaticAddTileClass(&tbData->m->u.Static, &tc) != NULL)
			{
				ResetTexIds(tbData);
				*tbData->brushIdx = (int)tbData->texIdsTileClasses.size - 1;
			}
			TileClassTerminate(&tc);
		}
		if (nk_button_label(ctx, "Duplicate"))
		{
			const TileClass *selectedTC = MissionStaticIdTileClass(
				&tbData->m->u.Static, *tbData->brushIdx);
			if (MissionStaticAddTileClass(
				&tbData->m->u.Static, selectedTC) != NULL)
			{
				ResetTexIds(tbData);
				*tbData->brushIdx = (int)tbData->texIdsTileClasses.size - 1;
			}
		}
		if (hashmap_length(tbData->m->u.Static.TileClasses) > 0 &&
			nk_button_label(ctx, "Remove"))
		{
			if (MissionStaticRemoveTileClass(
				&tbData->m->u.Static, *tbData->brushIdx))
			{
				ResetTexIds(tbData);
				*tbData->brushIdx = MIN(
					*tbData->brushIdx,
					(int)tbData->texIdsTileClasses.size - 1);
			}
		}

		nk_layout_row_dynamic(ctx, 40 * PIC_SCALE, WIDTH / 120);
		tbData->tileIdx = 0;
		if (hashmap_iterate_keys(
			tbData->m->u.Static.TileClasses, DrawTileType, tbData) != MAP_OK)
		{
			CASSERT(false, "failed to draw static tile classes");
		}
	}
	nk_end(ctx);
}
static void DrawTileClass(
	struct nk_context *ctx, const PicManager *pm, const TileClass *tc,
	const struct vec2i pos, const GLuint texid);
static int DrawTileType(any_t data, any_t key)
{
	TileBrushData *tbData = data;
	TileClass *tc;
	const int error = hashmap_get(
		tbData->m->u.Static.TileClasses, (const char *)key, (any_t *)&tc);
	if (error != MAP_OK)
	{
		CASSERT(false, "cannot find tile class");
		return error;
	}
	// TODO: use keys instead of index
	char name[256];
	TileClassGetBrushName(name, tc);
	const int selected = *tbData->brushIdx == tbData->tileIdx;
	if (nk_select_label(tbData->ctx, name,
		NK_TEXT_ALIGN_BOTTOM | NK_TEXT_ALIGN_CENTERED, selected))
	{
		*tbData->brushIdx = tbData->tileIdx;
	}
	const GLuint *texid =
		CArrayGet(&tbData->texIdsTileClasses, tbData->tileIdx);
	DrawTileClass(tbData->ctx, tbData->pm, tc, svec2i(-40, 5), *texid);
	tbData->tileIdx++;
	return MAP_OK;
}
static void DrawTileClass(
	struct nk_context *ctx, const PicManager *pm, const TileClass *tc,
	const struct vec2i pos, const GLuint texid)
{
	const Pic *pic = TileClassGetPic(pm, tc);
	DrawPic(ctx, pic, texid, pos, PIC_SCALE);
}

static void ResetTexIds(TileBrushData *data)
{
	if (data->texIdsTileClasses.size > 0)
	{
		glDeleteTextures(
			(GLsizei)data->texIdsTileClasses.size,
			(const GLuint *)data->texIdsTileClasses.data);
	}
	const int nTileClasses = hashmap_length(data->m->u.Static.TileClasses);
	CArrayResize(&data->texIdsTileClasses, nTileClasses, NULL);
	glGenTextures(
		nTileClasses, (GLuint *)data->texIdsTileClasses.data);
}
