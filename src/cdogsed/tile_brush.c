/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.
	Copyright (c) 2019-2021 Cong Xu
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
#define OPS_HEIGHT 35

static const char *IndexBulletClassName(const int i)
{
	CASSERT(
		i >= 0 && i < (int)gBulletClasses.Classes.size +
						  (int)gBulletClasses.CustomClasses.size,
		"Bullet index out of bounds");
	const BulletClass *b;
	if (i < (int)gBulletClasses.Classes.size)
	{
		b = CArrayGet(&gBulletClasses.Classes, i);
	}
	else
	{
		b = CArrayGet(&gBulletClasses.CustomClasses, i - gBulletClasses.Classes.size);
	}
	return b->Name;
}
static int BulletClassIndex(const BulletClass *b)
{
	if (b == NULL)
	{
		return -1;
	}
	int i = 0;
	CA_FOREACH(const BulletClass, b2, gBulletClasses.Classes)
	if (b == b2)
	{
		return i;
	}
	i++;
	CA_FOREACH_END()
	CA_FOREACH(const BulletClass, b2, gBulletClasses.CustomClasses)
	if (b == b2)
	{
		return i;
	}
	i++;
	CA_FOREACH_END()
	CASSERT(false, "cannot find bullet");
	return -1;
}

typedef struct
{
	struct nk_context *ctx;
	PicManager *pm;
	Mission *m;
	int *brushIdx;
	CArray texIdsTileClasses; // of GLuint
	char *tcTypes;
	char *floorStyles;
	char *wallStyles;
	char *doorStyles;
	char *bullets;
	CArray texIdsFloorStyles;
	CArray texIdsWallStyles;
	CArray texIdsDoorStyles;
	EditorResult result;
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
	map_t c, const TileClass *base, PicManager *pm, const char *style,
	const color_t mask, const color_t maskAlt);
static bool Draw(SDL_Window *win, struct nk_context *ctx, void *data);
EditorResult TileBrush(
	PicManager *pm, EventHandlers *handlers, Campaign *co,
	int *brushIdx)
{
	NKWindowConfig cfg;
	memset(&cfg, 0, sizeof cfg);
	cfg.Title = "Tile Brush";
	cfg.Size = svec2i(WIDTH, HEIGHT);
	color_t bg = {41, 26, 26, 255};
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
	data.floorStyles =
		GetClassNames(pm->tileStyleNames.size, IndexFloorStyleName);
	data.wallStyles =
		GetClassNames(pm->wallStyleNames.size, IndexWallStyleName);
	data.doorStyles =
		GetClassNames(pm->doorStyleNames.size, IndexDoorStyleName);
	// TODO: tile previews show wrong icons; possibly due to clashes between
	// SDL2 textures and OGL textures. Load SDL2 textures first and in a second
	// run, load OGL textures
	data.bullets = GetClassNames(BulletClassesCount(&gBulletClasses), IndexBulletClassName);
	TexArrayInit(&data.texIdsFloorStyles, pm->tileStyleNames.size);
	CA_FOREACH(const GLuint, texid, data.texIdsFloorStyles)
	const char *style = *(char **)CArrayGet(&pm->tileStyleNames, _ca_index);
	const TileClass *styleClass = GetOrAddTileClass(
		gMap.TileClasses, &gTileFloor, pm, style, colorBattleshipGrey, colorOfficeGreen);
	LoadTexFromPic(*texid, styleClass->Pic);
	CA_FOREACH_END()
	TexArrayInit(&data.texIdsWallStyles, pm->wallStyleNames.size);
	CA_FOREACH(const GLuint, texid, data.texIdsWallStyles)
	const char *style = *(char **)CArrayGet(&pm->wallStyleNames, _ca_index);
	const TileClass *styleClass = GetOrAddTileClass(
		gMap.TileClasses, &gTileWall, pm, style, colorGravel, colorOfficeGreen);
	LoadTexFromPic(*texid, styleClass->Pic);
	CA_FOREACH_END()
	TexArrayInit(&data.texIdsDoorStyles, pm->doorStyleNames.size);
	CA_FOREACH(const GLuint, texid, data.texIdsDoorStyles)
	const char *style = *(char **)CArrayGet(&pm->doorStyleNames, _ca_index);
	const TileClass *styleClass =
		GetOrAddTileClass(gMap.TileClasses, &gTileDoor, pm, style, colorWhite, colorOfficeGreen);
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
	CFREE(data.bullets);
	TexArrayTerminate(&data.texIdsFloorStyles);
	TexArrayTerminate(&data.texIdsWallStyles);
	TexArrayTerminate(&data.texIdsDoorStyles);
	return data.result;
}

static void DrawTileOpsRow(
	struct nk_context *ctx, TileBrushData *tbData, TileClass *selectedTC,
	bool *result);
static void DrawTilePropsSidebar(
	struct nk_context *ctx, TileBrushData *tbData, TileClass *selectedTC);
static void DrawTileType(
	TileBrushData *tbData, TileClass *tc, const int tileId,
	const int tilesDrawn);
static bool Draw(SDL_Window *win, struct nk_context *ctx, void *data)
{
	UNUSED(win);
	bool result = true;
	TileBrushData *tbData = data;
	TileClass *selectedTC =
		MissionStaticIdTileClass(&tbData->m->u.Static, *tbData->brushIdx);

	if (nk_begin(
			ctx, "Properties", nk_rect(0, 0, SIDE_WIDTH, HEIGHT),
			NK_WINDOW_BORDER | NK_WINDOW_TITLE))
	{
		if (selectedTC != NULL)
		{
			DrawTilePropsSidebar(ctx, tbData, selectedTC);
		}
		nk_end(ctx);
	}

	if (nk_begin(
			ctx, "Ops", nk_rect(SIDE_WIDTH, 0, MAIN_WIDTH, OPS_HEIGHT),
			NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR))
	{
		DrawTileOpsRow(ctx, tbData, selectedTC, &result);
		nk_end(ctx);
	}

	if (nk_begin(
		 ctx, "Tiles", nk_rect(SIDE_WIDTH, OPS_HEIGHT, MAIN_WIDTH, HEIGHT - OPS_HEIGHT),
		 NK_WINDOW_BORDER))
	{
		nk_layout_row_dynamic(ctx, 40 * PIC_SCALE, MAIN_WIDTH / 120);
		int tilesDrawn = 0;
		for (int i = 0;
			 tilesDrawn < hashmap_length(tbData->m->u.Static.TileClasses); i++)
		{
			TileClass *tc = MissionStaticIdTileClass(&tbData->m->u.Static, i);
			if (tc != NULL)
			{
				DrawTileType(tbData, tc, i, tilesDrawn);
				tilesDrawn++;
			}
		}
		nk_end(ctx);
	}

	return result;
}
static void DrawTileOpsRow(
	struct nk_context *ctx, TileBrushData *tbData, TileClass *selectedTC,
	bool *result)
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
			&tc, &gPicManager, &gTileFloor, "tile", "normal", colorGray,
			colorGray);
		if (MissionStaticAddTileClass(&tbData->m->u.Static, &tc) != NULL)
		{
			ResetTexIds(tbData);
		}
		TileClassTerminate(&tc);
		tbData->result |= EDITOR_RESULT_CHANGED;
	}
	if (nk_button_label(ctx, "Duplicate"))
	{
		if (MissionStaticAddTileClass(&tbData->m->u.Static, selectedTC) !=
			NULL)
		{
			ResetTexIds(tbData);
		}
		tbData->result |= EDITOR_RESULT_CHANGED;
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
						 &tbData->m->u.Static, *tbData->brushIdx) == NULL;
					 (*tbData->brushIdx)++)
					;
			}
		}
		tbData->result |= EDITOR_RESULT_CHANGED_AND_RELOAD;
	}
}
static void DrawTileTypeSelect(
	struct nk_context *ctx, TileBrushData *tbData, TileClass *selectedTC);
static void DrawTileStyleSelect(
	struct nk_context *ctx, TileBrushData *tbData, TileClass *selectedTC);
static void DrawTileBulletSelect(
	struct nk_context *ctx, TileBrushData *tbData, TileClass *selectedTC);
static void DrawTilePropsSidebar(
	struct nk_context *ctx, TileBrushData *tbData, TileClass *selectedTC)
{
	nk_layout_row_dynamic(ctx, ROW_HEIGHT, 1);

	DrawTileTypeSelect(ctx, tbData, selectedTC);

	if (selectedTC->Type != TILE_CLASS_NOTHING)
	{
		DrawTileStyleSelect(ctx, tbData, selectedTC);
		if (selectedTC->Type == TILE_CLASS_FLOOR)
		{
			bool hasDamage = StrBulletClass(selectedTC->DamageBullet) != NULL;
			if (DrawCheckbox(
				 ctx, "Damaging", "Whether actors take damage on this tile",
				 &hasDamage))
			{
				CFREE(selectedTC->DamageBullet);
				if (hasDamage)
				{
					CSTRDUP(selectedTC->DamageBullet, IndexBulletClassName(0));
				}
				else
				{
					selectedTC->DamageBullet = NULL;
				}
				tbData->result |= EDITOR_RESULT_CHANGED_AND_RELOAD;
			}
			if (hasDamage)
			{
				DrawTileBulletSelect(ctx, tbData, selectedTC);
			}
		}
		// Changing colour requires regenerating tile mask pics
		if (ColorPicker(ctx, ROW_HEIGHT, "Primary Color", &selectedTC->Mask))
		{
			tbData->result |= EDITOR_RESULT_CHANGED_AND_RELOAD;
		}
		if (ColorPicker(ctx, ROW_HEIGHT, "Alt Color", &selectedTC->MaskAlt))
		{
			tbData->result |= EDITOR_RESULT_CHANGED_AND_RELOAD;
		}

		// Changing tile props can affect map object placement
		if (DrawCheckbox(
				ctx, "Can Walk", "Whether actors can walk through this tile",
				&selectedTC->canWalk))
		{
			tbData->result |= EDITOR_RESULT_CHANGED_AND_RELOAD;
		}
		if (DrawCheckbox(
				ctx, "Opaque", "Whether this tile cannot be seen through",
				&selectedTC->isOpaque))
		{
			tbData->result |= EDITOR_RESULT_CHANGED_AND_RELOAD;
		}
		if (DrawCheckbox(
				ctx, "Shootable", "Whether bullets will hit this tile",
				&selectedTC->shootable))
		{
			tbData->result |= EDITOR_RESULT_CHANGED_AND_RELOAD;
		}
		if (DrawCheckbox(
				ctx, "IsRoom",
				"Affects random placement of indoor/outdoor map objects",
				&selectedTC->IsRoom))
		{
			tbData->result |= EDITOR_RESULT_CHANGED_AND_RELOAD;
		}
	}
}
static void DrawTileTypeSelect(
	struct nk_context *ctx, TileBrushData *tbData, TileClass *selectedTC)
{
	nk_label(ctx, "Type:", NK_TEXT_LEFT);
	const TileClassType newType = nk_combo_separator(
		ctx, tbData->tcTypes, '\0', selectedTC->Type, TILE_CLASS_COUNT,
		ROW_HEIGHT, nk_vec2(nk_widget_width(ctx), 8 * ROW_HEIGHT));
	if (newType != selectedTC->Type)
	{
		TileClassTerminate(selectedTC);
		const TileClass *base = &gTileFloor;
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
		tbData->result |= EDITOR_RESULT_CHANGED_AND_RELOAD;
	}
}
static void DrawTileStyleSelect(
	struct nk_context *ctx, TileBrushData *tbData, TileClass *selectedTC)
{
	nk_label(ctx, "Style:", NK_TEXT_LEFT);
	const GLuint *styleTexIds = NULL;
	const char *styles = "";
	int styleSelected = -1;
	int styleCount = 0;
	switch (selectedTC->Type)
	{
	case TILE_CLASS_FLOOR:
		styles = tbData->floorStyles;
		styleSelected = FloorStyleIndex(selectedTC->Style);
		styleTexIds = tbData->texIdsFloorStyles.data;
		styleCount = (int)tbData->texIdsFloorStyles.size;
		break;
	case TILE_CLASS_WALL:
		styles = tbData->wallStyles;
		styleSelected = WallStyleIndex(selectedTC->Style);
		styleTexIds = tbData->texIdsWallStyles.data;
		styleCount = (int)tbData->texIdsWallStyles.size;
		break;
	case TILE_CLASS_DOOR:
		styles = tbData->doorStyles;
		styleSelected = DoorStyleIndex(selectedTC->Style);
		styleTexIds = tbData->texIdsDoorStyles.data;
		styleCount = (int)tbData->texIdsDoorStyles.size;
		break;
	default:
		break;
	}
	const int newStyle = nk_combo_separator_image(
		ctx, styleTexIds, styles, '\0', styleSelected, styleCount, ROW_HEIGHT,
		nk_vec2(nk_widget_width(ctx), 8 * ROW_HEIGHT));
	if (newStyle != styleSelected)
	{
		CFREE(selectedTC->Style);
		CSTRDUP(
			selectedTC->Style, IndexTileStyleName(selectedTC->Type, newStyle));
		TileClassReloadPic(selectedTC, tbData->pm);
		tbData->result |= EDITOR_RESULT_CHANGED_AND_RELOAD;
	}
}

static void DrawTileBulletSelect(
	struct nk_context *ctx, TileBrushData *tbData, TileClass *selectedTC)
{
	struct nk_rect bounds = nk_widget_bounds(ctx);
	nk_label(ctx, "Damage Bullet:", NK_TEXT_LEFT);
	const int selectedIndex = BulletClassIndex(StrBulletClass(selectedTC->DamageBullet));
	const int newBullet = nk_combo_separator(
		ctx, tbData->bullets, '\0', selectedIndex, BulletClassesCount(&gBulletClasses),
		ROW_HEIGHT, nk_vec2(nk_widget_width(ctx), 8 * ROW_HEIGHT));
	if (newBullet != selectedIndex)
	{
		CFREE(selectedTC->DamageBullet);
		CSTRDUP(
			selectedTC->DamageBullet, IndexBulletClassName(newBullet));
		tbData->result |= EDITOR_RESULT_CHANGED_AND_RELOAD;
	}
	if (nk_input_is_mouse_hovering_rect(&ctx->input, bounds))
	{
		nk_tooltip(ctx, "Bullet type to hit player with when stepped on");
	}
}

static void DrawTileClass(
	struct nk_context *ctx, PicManager *pm, TileClass *tc,
	const struct vec2i pos, const GLuint texid);
static void DrawTileType(
	TileBrushData *tbData, TileClass *tc, const int tileId,
	const int tilesDrawn)
{
	char name[256];
	TileClassGetBrushName(name, tc);
	const int selected = *tbData->brushIdx == tileId;
	if (nk_select_label(
			tbData->ctx, name, NK_TEXT_ALIGN_BOTTOM | NK_TEXT_ALIGN_CENTERED,
			selected))
	{
		*tbData->brushIdx = tileId;
	}
	const GLuint *texid = CArrayGet(&tbData->texIdsTileClasses, tilesDrawn);
	DrawTileClass(tbData->ctx, tbData->pm, tc, svec2i(-40, 5), *texid);
}
static void DrawTileClass(
	struct nk_context *ctx, PicManager *pm, TileClass *tc,
	const struct vec2i pos, const GLuint texid)
{
	const Pic *pic = TileClassGetPic(pm, tc);
	if (pic == NULL)
	{
		// Attempt to reload pic (we may have changed its colour for example)
		TileClassReloadPic(tc, pm);
		pic = TileClassGetPic(pm, tc);
	}
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
	glGenTextures(nTileClasses, (GLuint *)data->texIdsTileClasses.data);
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
	map_t c, const TileClass *base, PicManager *pm, const char *style,
	const color_t mask, const color_t maskAlt)
{
	const TileClass *styleClass = TileClassesGetMaskedTile(
		c, base, style, TileClassBaseStyleType(base->Type), mask, maskAlt);
	if (styleClass == &gTileNothing)
	{
		styleClass = TileClassesAdd(
			c, pm, base, style, TileClassBaseStyleType(base->Type),
			mask, maskAlt);
	}
	return styleClass;
}
