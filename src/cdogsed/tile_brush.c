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


#define WIDTH 600
#define HEIGHT 500
#define MAIN_WIDTH 400
#define SIDE_WIDTH (WIDTH - MAIN_WIDTH)
#define ROW_HEIGHT 25


typedef struct
{
	struct nk_context *ctx;
	PicManager *pm;
	Mission *m;
	int *brushIdx;
	CArray texIdsTileClasses;	// of GLuint
	int tileIdx;
	char *tcTypes;
	char *floorStyles;
	char *wallStyles;
	char *doorStyles;
	CArray texIdsFloorStyles;
	CArray texIdsWallStyles;
	CArray texIdsDoorStyles;
} TileBrushData;


static void ResetTexIds(TileBrushData *data);
static const char *IndexFloorStyleName(const int idx);
static const char *IndexWallStyleName(const int idx);
static const char *IndexDoorStyleName(const int idx);
static const char *IndexTileStyleName(const TileClassType type, const int idx);
static int FloorStyleIndex(const char *style);
static int WallStyleIndex(const char *style);
static int DoorStyleIndex(const char *style);
static const TileClass *GetOrAddTileClass(
	const TileClass *base, PicManager *pm, const char *style,
	const color_t mask, const color_t maskAlt);
static bool Draw(SDL_Window *win, struct nk_context *ctx, void *data);
void TileBrush(
	PicManager *pm, EventHandlers *handlers, CampaignOptions *co,
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
	memset(&data, 0, sizeof data);
	data.pm = pm;
	data.m = CampaignGetCurrentMission(co);
	CASSERT(data.m->Type == MAPTYPE_STATIC, "unexpected map type");
	data.brushIdx = brushIdx;
	CArrayInit(&data.texIdsTileClasses, sizeof(GLuint));
	ResetTexIds(&data);
	cfg.DrawData = &data;
	data.ctx = cfg.ctx;
	data.tcTypes = GetClassNames(
		TILE_CLASS_COUNT, (const char *(*)(const int))TileClassTypeStr);
	data.floorStyles = GetClassNames(
		pm->tileStyleNames.size, IndexFloorStyleName);
	data.wallStyles = GetClassNames(
		pm->wallStyleNames.size, IndexWallStyleName);
	data.doorStyles = GetClassNames(
		pm->doorStyleNames.size, IndexDoorStyleName);
	TexArrayInit(&data.texIdsFloorStyles, pm->tileStyleNames.size);
	CA_FOREACH(const GLuint, texid, data.texIdsFloorStyles)
		const char *style = *(char **)CArrayGet(&pm->tileStyleNames, _ca_index);
		const TileClass *styleClass = GetOrAddTileClass(
			&gTileFloor, pm, style, colorBattleshipGrey, colorOfficeGreen);
		LoadTexFromPic(*texid, styleClass->Pic);
	CA_FOREACH_END()
	TexArrayInit(&data.texIdsWallStyles, pm->wallStyleNames.size);
	CA_FOREACH(const GLuint, texid, data.texIdsWallStyles)
		const char *style = *(char **)CArrayGet(&pm->wallStyleNames, _ca_index);
		const TileClass *styleClass = GetOrAddTileClass(
			&gTileWall, pm, style, colorGravel, colorOfficeGreen);
		LoadTexFromPic(*texid, styleClass->Pic);
	CA_FOREACH_END()
	TexArrayInit(&data.texIdsDoorStyles, pm->doorStyleNames.size);
	CA_FOREACH(const GLuint, texid, data.texIdsDoorStyles)
		const char *style = *(char **)CArrayGet(&pm->doorStyleNames, _ca_index);
		const TileClass *styleClass = GetOrAddTileClass(
			&gTileDoor, pm, style, colorWhite, colorOfficeGreen);
		LoadTexFromPic(*texid, styleClass->Pic);
	CA_FOREACH_END()

	NKWindow(cfg);

	glDeleteTextures(
		(GLsizei)data.texIdsTileClasses.size,
		(const GLuint *)data.texIdsTileClasses.data);
	CArrayTerminate(&data.texIdsTileClasses);
	CFREE(data.tcTypes);
	CFREE(data.floorStyles);
	CFREE(data.wallStyles);
	CFREE(data.doorStyles);
	TexArrayTerminate(&data.texIdsFloorStyles);
	TexArrayTerminate(&data.texIdsWallStyles);
	TexArrayTerminate(&data.texIdsDoorStyles);
}

static void DrawTileOpsRow(
	struct nk_context *ctx, TileBrushData *tbData,
	TileClass *selectedTC, bool *result);
static void DrawTilePropsSidebar(
	struct nk_context *ctx, TileBrushData *tbData, TileClass *selectedTC);
static int DrawTileType(TileBrushData *tbData, TileClass *tc, const int tileId);
static bool Draw(SDL_Window *win, struct nk_context *ctx, void *data)
{
	UNUSED(win);
	bool result = true;
	TileBrushData *tbData = data;
	TileClass* selectedTC = MissionStaticIdTileClass(
		&tbData->m->u.Static, *tbData->brushIdx);
	if (nk_begin(ctx, "", nk_rect(0, 0, MAIN_WIDTH, HEIGHT), NK_WINDOW_BORDER))
	{
		DrawTileOpsRow(ctx, tbData, selectedTC, &result);

		nk_layout_row_dynamic(ctx, 40 * PIC_SCALE, MAIN_WIDTH / 120);
		tbData->tileIdx = 0;
		int tilesDrawn = 0;
		for (int i = 0;
			tilesDrawn < hashmap_length(tbData->m->u.Static.TileClasses);
			i++)
		{
			TileClass *tc = MissionStaticIdTileClass(&tbData->m->u.Static, i);
			if (tc != NULL)
			{
				DrawTileType(tbData, tc, i);
				tilesDrawn++;
			}
		}
	}
	nk_end(ctx);

	if (nk_begin(
		ctx, "Properties", nk_rect(MAIN_WIDTH, 0, SIDE_WIDTH, HEIGHT),
		NK_WINDOW_BORDER | NK_WINDOW_TITLE) &&
		selectedTC != NULL)
	{
		DrawTilePropsSidebar(ctx, tbData, selectedTC);
	}
	nk_end(ctx);
	return result;
}
static void DrawTileOpsRow(
	struct nk_context *ctx, TileBrushData *tbData,
	TileClass *selectedTC, bool *result)
{
	nk_layout_row_dynamic(ctx, ROW_HEIGHT, 4);
	if (nk_button_label(ctx, "Done"))
	{
		*result = false;
	}
	if (nk_button_label(ctx, "Add"))
	{
		TileClass tc;
		TileClassInit(
			&tc, &gPicManager, &gTileFloor, "tile", "normal",
			colorGray, colorGray);
		if (MissionStaticAddTileClass(&tbData->m->u.Static, &tc) != NULL)
		{
			ResetTexIds(tbData);
		}
		TileClassTerminate(&tc);
	}
	if (nk_button_label(ctx, "Duplicate"))
	{
		if (MissionStaticAddTileClass(
			&tbData->m->u.Static, selectedTC) != NULL)
		{
			ResetTexIds(tbData);
		}
	}
	if (hashmap_length(tbData->m->u.Static.TileClasses) > 0 &&
		nk_button_label(ctx, "Remove"))
	{
		if (MissionStaticRemoveTileClass(
			&tbData->m->u.Static, *tbData->brushIdx))
		{
			ResetTexIds(tbData);
			// Set selected tile index to an existing tile
			if (hashmap_length(tbData->m->u.Static.TileClasses) > 0)
			{
				for (*tbData->brushIdx = 0;
					MissionStaticIdTileClass(
						&tbData->m->u.Static,
						*tbData->brushIdx
					) == NULL;
					(*tbData->brushIdx)++);
			}
		}
	}
}
static void DrawTileTypeSelect(
	struct nk_context *ctx, TileBrushData *tbData, TileClass *selectedTC);
static void DrawTileStyleSelect(
	struct nk_context *ctx, TileBrushData *tbData, TileClass *selectedTC);
static void DrawTilePropsSidebar(
	struct nk_context *ctx, TileBrushData *tbData, TileClass *selectedTC)
{
	nk_layout_row_dynamic(ctx, ROW_HEIGHT, 1);
	
	DrawTileTypeSelect(ctx, tbData, selectedTC);

	if (selectedTC->Type != TILE_CLASS_NOTHING)
	{
		DrawTileStyleSelect(ctx, tbData, selectedTC);
		ColorPicker(ctx, ROW_HEIGHT, "Primary Color", &selectedTC->Mask);
		ColorPicker(ctx, ROW_HEIGHT, "Alt Color", &selectedTC->MaskAlt);
	}
}
static void DrawTileTypeSelect(
	struct nk_context *ctx, TileBrushData *tbData, TileClass *selectedTC)
{
	nk_label(ctx, "Type:", NK_TEXT_LEFT);
	const TileClassType newType = nk_combo_separator(
		ctx, tbData->tcTypes, '\0', selectedTC->Type,
		TILE_CLASS_COUNT, ROW_HEIGHT,
		nk_vec2(nk_widget_width(ctx), 8 * ROW_HEIGHT)
	);
	if (newType != selectedTC->Type)
	{
		TileClassTerminate(selectedTC);
		const TileClass* base = &gTileFloor;
		switch (newType)
		{
		case TILE_CLASS_FLOOR:
			break;
		case TILE_CLASS_WALL:
			base = &gTileWall;
			break;
		case TILE_CLASS_DOOR:
			base = &gTileDoor;
			break;
		case TILE_CLASS_NOTHING:
			base = &gTileNothing;
			break;
		default:
			CASSERT(false, "unknown tile class");
			break;
		}
		TileClassInitDefault(
			selectedTC, tbData->pm, base, NULL, &selectedTC->Mask);
	}
}
static void DrawTileStyleSelect(
	struct nk_context *ctx, TileBrushData *tbData, TileClass *selectedTC)
{
	nk_label(ctx, "Style:", NK_TEXT_LEFT);
	const GLuint* styleTexIds = NULL;
	const char* styles = "";
	int styleSelected = -1;
	int styleCount = 0;
	switch (selectedTC->Type)
	{
	case TILE_CLASS_FLOOR:
		styles = tbData->floorStyles;
		styleSelected = FloorStyleIndex(selectedTC->Style);
		styleTexIds = tbData->texIdsFloorStyles.data;
		styleCount = tbData->texIdsFloorStyles.size;
		break;
	case TILE_CLASS_WALL:
		styles = tbData->wallStyles;
		styleSelected = WallStyleIndex(selectedTC->Style);
		styleTexIds = tbData->texIdsWallStyles.data;
		styleCount = tbData->texIdsWallStyles.size;
		break;
	case TILE_CLASS_DOOR:
		styles = tbData->doorStyles;
		styleSelected = DoorStyleIndex(selectedTC->Style);
		styleTexIds = tbData->texIdsDoorStyles.data;
		styleCount = tbData->texIdsDoorStyles.size;
		break;
	default:
		break;
	}
	const int newStyle = nk_combo_separator_image(
		ctx, styleTexIds, styles, '\0', styleSelected,
		styleCount, ROW_HEIGHT,
		nk_vec2(nk_widget_width(ctx), 8 * ROW_HEIGHT)
	);
	if (newStyle != styleSelected)
	{
		CFREE(selectedTC->Style);
		CSTRDUP(
			selectedTC->Style, IndexTileStyleName(selectedTC->Type, newStyle));
		TileClassReloadPic(selectedTC, tbData->pm);
	}
}
static void DrawTileClass(
	struct nk_context *ctx, const PicManager *pm, const TileClass *tc,
	const struct vec2i pos, const GLuint texid);
static int DrawTileType(TileBrushData *tbData, TileClass *tc, const int tileId)
{
	char name[256];
	TileClassGetBrushName(name, tc);
	const int selected = *tbData->brushIdx == tileId;
	if (nk_select_label(tbData->ctx, name,
		NK_TEXT_ALIGN_BOTTOM | NK_TEXT_ALIGN_CENTERED, selected))
	{
		*tbData->brushIdx = tileId;
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

static const char *IndexFloorStyleName(const int idx)
{
	return *(const char **)CArrayGet(&gPicManager.tileStyleNames, idx);
}
static const char *IndexWallStyleName(const int idx)
{
	return *(const char **)CArrayGet(&gPicManager.wallStyleNames, idx);
}
static const char *IndexDoorStyleName(const int idx)
{
	return *(const char **)CArrayGet(&gPicManager.doorStyleNames, idx);
}
static const char *IndexTileStyleName(const TileClassType type, const int idx)
{
	switch (type)
	{
	case TILE_CLASS_FLOOR:
		return IndexFloorStyleName(idx);
	case TILE_CLASS_WALL:
		return IndexWallStyleName(idx);
	case TILE_CLASS_DOOR:
		return IndexDoorStyleName(idx);
	default:
		return NULL;
	}
}
static int TileStyleIndex(const CArray *styles, const char *style)
{
	if (style == NULL)
	{
		return -1;
	}
	CA_FOREACH(const char *, styleName, *styles)
		if (strcmp(*styleName, style) == 0)
		{
			return _ca_index;
		}
	CA_FOREACH_END()
	return -1;
}
static int FloorStyleIndex(const char *style)
{
	return TileStyleIndex(&gPicManager.tileStyleNames, style);
}
static int WallStyleIndex(const char *style)
{
	return TileStyleIndex(&gPicManager.wallStyleNames, style);
}
static int DoorStyleIndex(const char *style)
{
	return TileStyleIndex(&gPicManager.doorStyleNames, style);
}

static const TileClass *GetOrAddTileClass(
	const TileClass *base, PicManager *pm, const char *style,
	const color_t mask, const color_t maskAlt)
{
	const TileClass *styleClass = TileClassesGetMaskedTile(
		base, style, TileClassBaseStyleType(base->Type), mask, maskAlt);
	if (styleClass == &gTileNothing)
	{
		styleClass = TileClassesAdd(
			&gTileClasses, pm, base, style, TileClassBaseStyleType(base->Type),
			mask, maskAlt);
	}
	return styleClass;
}
