/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.
	Copyright (c) 2014, 2019-2021 Cong Xu
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
#include "editor_brush.h"

#include <assert.h>

#include <SDL_image.h>

#include <cdogs/algorithms.h>
#include <cdogs/map.h>
#include <cdogs/map_build.h>
#include <cdogs/mission_convert.h>

void EditorBrushInit(EditorBrush *b)
{
	memset(b, 0, sizeof *b);
	b->Type = BRUSHTYPE_POINT;
	b->MainType = 0;
	b->SecondaryType = 1;
	b->PaintType = b->MainType;
	b->IsActive = 0;
	b->IsPainting = 0;
	b->BrushSize = 1;
	b->LastPos = svec2i(-1, -1);
	b->Pos = svec2i(-1, -1);
	b->GuideImageAlpha = 64;
	CArrayInit(&b->HighlightedTiles, sizeof(struct vec2i));
}
void EditorBrushTerminate(EditorBrush *b)
{
	CArrayTerminate(&b->HighlightedTiles);
	PicFree(&b->GuideImagePic);
}

static void EditorBrushHighlightPoint(void *data, struct vec2i p)
{
	EditorBrush *b = data;
	struct vec2i v;
	for (v.y = 0; v.y < b->BrushSize; v.y++)
	{
		for (v.x = 0; v.x < b->BrushSize; v.x++)
		{
			struct vec2i pos = svec2i_add(p, v);
			CArrayPushBack(&b->HighlightedTiles, &pos);
		}
	}
}
void EditorBrushSetHighlightedTiles(EditorBrush *b)
{
	bool useSimpleHighlight = true;
	switch (b->Type)
	{
	case BRUSHTYPE_POINT:
		useSimpleHighlight = true;
		break;
	case BRUSHTYPE_LINE:
		if (b->IsPainting)
		{
			useSimpleHighlight = 0;
			// highlight a line
			CArrayClear(&b->HighlightedTiles);
			AlgoLineDrawData data;
			data.Draw = EditorBrushHighlightPoint;
			data.data = b;
			BresenhamLineDraw(b->LastPos, b->Pos, &data);
		}
		break;
	case BRUSHTYPE_BOX:
	case BRUSHTYPE_BOX_AND_FILL: // fallthrough
	case BRUSHTYPE_SET_EXIT:	 // fallthrough
		if (b->IsPainting)
		{
			struct vec2i v;
			struct vec2i d = svec2i(
				b->Pos.x > b->LastPos.x ? 1 : -1,
				b->Pos.y > b->LastPos.y ? 1 : -1);
			useSimpleHighlight = 0;
			CArrayClear(&b->HighlightedTiles);
			for (v.y = b->LastPos.y; v.y != b->Pos.y + d.y; v.y += d.y)
			{
				for (v.x = b->LastPos.x; v.x != b->Pos.x + d.x; v.x += d.x)
				{
					if (v.x == b->LastPos.x || v.x == b->Pos.x ||
						v.y == b->LastPos.y || v.y == b->Pos.y)
					{
						EditorBrushHighlightPoint(b, v);
					}
				}
			}
		}
		break;
	case BRUSHTYPE_BOX_FILLED:
		if (b->IsPainting)
		{
			struct vec2i v;
			struct vec2i d = svec2i(
				b->Pos.x > b->LastPos.x ? 1 : -1,
				b->Pos.y > b->LastPos.y ? 1 : -1);
			useSimpleHighlight = 0;
			CArrayClear(&b->HighlightedTiles);
			for (v.y = b->LastPos.y; v.y != b->Pos.y + d.y; v.y += d.y)
			{
				for (v.x = b->LastPos.x; v.x != b->Pos.x + d.x; v.x += d.x)
				{
					EditorBrushHighlightPoint(b, v);
				}
			}
		}
		break;
	case BRUSHTYPE_SELECT:
		if (b->IsPainting)
		{
			if (b->IsMoving)
			{
				struct vec2i v;
				struct vec2i offset =
					svec2i(b->Pos.x - b->DragPos.x, b->Pos.y - b->DragPos.y);
				useSimpleHighlight = 0;
				CArrayClear(&b->HighlightedTiles);
				for (v.y = 0; v.y < b->SelectionSize.y; v.y++)
				{
					for (v.x = 0; v.x < b->SelectionSize.x; v.x++)
					{
						struct vec2i vOffset = svec2i_add(
							svec2i_add(v, b->SelectionStart), offset);
						EditorBrushHighlightPoint(b, vOffset);
					}
				}
			}
			else
			{
				// resize the selection
				struct vec2i v;
				struct vec2i d = svec2i(
					b->Pos.x > b->LastPos.x ? 1 : -1,
					b->Pos.y > b->LastPos.y ? 1 : -1);
				useSimpleHighlight = 0;
				CArrayClear(&b->HighlightedTiles);
				for (v.y = b->LastPos.y; v.y != b->Pos.y + d.y; v.y += d.y)
				{
					for (v.x = b->LastPos.x; v.x != b->Pos.x + d.x; v.x += d.x)
					{
						EditorBrushHighlightPoint(b, v);
					}
				}
			}
		}
		else
		{
			// If there's a valid selection, draw that instead
			if (b->SelectionSize.x > 0 && b->SelectionSize.y > 0)
			{
				struct vec2i v;
				useSimpleHighlight = 0;
				CArrayClear(&b->HighlightedTiles);
				for (v.y = 0; v.y < b->SelectionSize.y; v.y++)
				{
					for (v.x = 0; v.x < b->SelectionSize.x; v.x++)
					{
						struct vec2i vOffset =
							svec2i_add(v, b->SelectionStart);
						EditorBrushHighlightPoint(b, vOffset);
					}
				}
			}
		}
	default:
		// do nothing
		break;
	}
	if (useSimpleHighlight)
	{
		// Simple highlight at brush tip based on brush size
		CArrayClear(&b->HighlightedTiles);
		EditorBrushHighlightPoint(b, b->Pos);
	}
}

static void SetTile(
	MapBuilder *mb, Mission *m, const struct vec2i pos, const int tile)
{
	if (MissionStaticTrySetTile(&m->u.Static, mb->Map->Size, pos, tile))
	{
		const TileClass *tc = MissionStaticGetTileClass(
			&mb->mission->u.Static, mb->Map->Size, pos);
		if (tc != NULL)
		{
			MapBuildTile(mb, pos, tc);
		}
	}
}

typedef struct
{
	EditorBrush *brush;
	Mission *mission;
} EditorBrushPaintTilesAtData;
static void EditorBrushPaintTilesAt(void *data, struct vec2i pos)
{
	struct vec2i v;
	EditorBrushPaintTilesAtData *paintData = data;
	EditorBrush *b = paintData->brush;
	Mission *m = paintData->mission;
	MapBuilder mb;
	MapBuilderInit(&mb, &gMap, m, GAME_MODE_NORMAL, NULL);
	for (v.y = 0; v.y < b->BrushSize; v.y++)
	{
		for (v.x = 0; v.x < b->BrushSize; v.x++)
		{
			SetTile(&mb, m, svec2i_add(pos, v), b->PaintType);
		}
	}
	MapBuilderTerminate(&mb);
}
static void EditorBrushPaintLine(EditorBrush *b, Mission *m)
{
	// Draw tiles between the last point and the current point
	EditorBrushPaintTilesAtData paintData;
	paintData.brush = b;
	paintData.mission = m;
	if (b->IsPainting)
	{
		AlgoLineDrawData data;
		data.Draw = EditorBrushPaintTilesAt;
		data.data = &paintData;
		BresenhamLineDraw(b->LastPos, b->Pos, &data);
	}
	EditorBrushPaintTilesAt(&paintData, b->Pos);
	b->IsPainting = 1;
	b->LastPos = b->Pos;
}
typedef struct
{
	MapBuilder mb;
	Mission *m;
	int fromType;
	int toType;
} PaintFloodFillData;
static void MissionFillTile(void *data, const struct vec2i v);
static bool MissionIsTileSame(void *data, const struct vec2i v);
EditorResult EditorBrushStartPainting(EditorBrush *b, Mission *m, int isMain)
{
	if (!b->IsPainting)
	{
		b->LastPos = b->Pos;
	}
	b->PaintType = isMain ? b->MainType : b->SecondaryType;
	switch (b->Type)
	{
	case BRUSHTYPE_POINT:
		b->IsPainting = true;
		EditorBrushPaintLine(b, m);
		return EDITOR_RESULT_CHANGED;
	case BRUSHTYPE_LINE:		 // fallthrough
	case BRUSHTYPE_BOX:			 // fallthrough
	case BRUSHTYPE_BOX_FILLED:	 // fallthrough
	case BRUSHTYPE_BOX_AND_FILL: // fallthrough
	case BRUSHTYPE_SET_EXIT:
		// don't paint until the end
		break;
	case BRUSHTYPE_SELECT:
		// Perform state changes if we've started painting
		if (!b->IsPainting)
		{
			if (!b->IsMoving)
			{
				// check if the click was inside the selection
				// If so, start moving
				if (b->Pos.x >= b->SelectionStart.x &&
					b->Pos.y >= b->SelectionStart.y &&
					b->Pos.x < b->SelectionStart.x + b->SelectionSize.x &&
					b->Pos.y < b->SelectionStart.y + b->SelectionSize.y)
				{
					b->IsMoving = 1;
					b->DragPos = b->Pos;
				}
			}
		}
		break;
	case BRUSHTYPE_FILL:
		// Use flood fill to change all the tiles of the same type to
		// another type
		// Don't paint if target already same type
		// Special case: don't flood-fill doors
		if (MissionStaticIdTileClass(&m->u.Static, b->PaintType)->Type ==
			TILE_CLASS_DOOR)
		{
			break;
		}
		{
			const int tile =
				MissionStaticGetTile(&m->u.Static, m->Size, b->Pos);
			if (tile == b->PaintType)
			{
				break;
			}
			FloodFillData data;
			data.Fill = MissionFillTile;
			data.IsSame = MissionIsTileSame;
			PaintFloodFillData pData;
			MapBuilderInit(&pData.mb, &gMap, m, GAME_MODE_NORMAL, NULL);
			pData.m = m;
			pData.fromType = tile;
			pData.toType = b->PaintType;
			data.data = &pData;
			const bool flooded = CFloodFill(b->Pos, &data);
			MapBuilderTerminate(&pData.mb);
			if (!flooded)
			{
				break;
			}
			return EDITOR_RESULT_CHANGED;
		}
	case BRUSHTYPE_SET_PLAYER_START:
		if (TileIsClear(MapGetTile(&gMap, b->Pos)))
		{
			m->u.Static.Start = b->Pos;
			return EDITOR_RESULT_CHANGED;
		}
		break;
	case BRUSHTYPE_ADD_ITEM:
		if (isMain)
		{
			const Tile *tile = MapGetTile(&gMap, b->Pos);
			const Tile *tileAbove = MapGetTile(&gMap, svec2i(b->Pos.x, b->Pos.y - 1));

			if (MapObjectIsTileOK(b->u.MapObject, tile, tileAbove) &&
				MissionStaticTryAddItem(&m->u.Static, b->u.MapObject, b->Pos))
			{
				return EDITOR_RESULT_CHANGED_AND_RELOAD;
			}
		}
		else
		{
			if (MissionStaticTryRemoveItemAt(&m->u.Static, b->Pos))
			{
				return EDITOR_RESULT_CHANGED_AND_RELOAD;
			}
		}
		break;
	case BRUSHTYPE_ADD_CHARACTER:
		if (isMain)
		{
			const Tile *tile = MapGetTile(&gMap, b->Pos);
			if (TileIsClear(tile))
			{
				CharacterPlace cp = {b->Pos, DIRECTION_DOWN};
				MissionStaticAddCharacter(&m->u.Static, b->u.ItemIndex, cp);
				return EDITOR_RESULT_CHANGED_AND_RELOAD;
			}
			else if (MissionStaticTryRotateCharacterAt(&m->u.Static, b->Pos))
			{
				return EDITOR_RESULT_CHANGED_AND_RELOAD;
			}
		}
		else
		{
			if (MissionStaticTryRemoveCharacterAt(&m->u.Static, b->Pos))
			{
				return EDITOR_RESULT_CHANGED_AND_RELOAD;
			}
		}
		break;
	case BRUSHTYPE_ADD_OBJECTIVE:
		if (isMain)
		{
			MissionStaticAddObjective(
				m, &m->u.Static, b->u.ItemIndex, b->Index2, b->Pos, false);
			return EDITOR_RESULT_CHANGED_AND_RELOAD;
		}
		else
		{
			if (MissionStaticTryRemoveObjective(m, &m->u.Static, b->Pos))
			{
				return EDITOR_RESULT_CHANGED_AND_RELOAD;
			}
		}
		break;
	case BRUSHTYPE_ADD_KEY:
		if (isMain)
		{
			const Tile *tile = MapGetTile(&gMap, b->Pos);
			if (TileIsClear(tile))
			{
				MissionStaticAddKey(&m->u.Static, b->u.ItemIndex, b->Pos);
				return EDITOR_RESULT_CHANGED_AND_RELOAD;
			}
		}
		else
		{
			if (MissionStaticTryRemoveKeyAt(&m->u.Static, b->Pos))
			{
				return EDITOR_RESULT_CHANGED_AND_RELOAD;
			}
		}
		break;
	case BRUSHTYPE_ADD_PICKUP:
		if (isMain)
		{
			const Tile *tile = MapGetTile(&gMap, b->Pos);
			if (TileCanWalk(tile) && MissionStaticTryAddPickup(
					&m->u.Static, b->u.Pickup, b->Pos))
			{
				return EDITOR_RESULT_CHANGED_AND_RELOAD;
			}
		}
		else
		{
			if (MissionStaticTryRemovePickupAt(&m->u.Static, b->Pos))
			{
				return EDITOR_RESULT_CHANGED_AND_RELOAD;
			}
		}
		break;
	case BRUSHTYPE_SET_KEY:
		if (isMain)
		{
			if (MissionStaticTrySetKey(
					&m->u.Static, b->u.ItemIndex, m->Size, b->Pos))
			{
				return EDITOR_RESULT_CHANGED_AND_RELOAD;
			}
		}
		else
		{
			if (MissionStaticTryUnsetKeyAt(&m->u.Static, m->Size, b->Pos))
			{
				return EDITOR_RESULT_CHANGED_AND_RELOAD;
			}
		}
		break;
	default:
		CASSERT(false, "unknown brush type");
		break;
	}
	b->IsPainting = 1;
	return EDITOR_RESULT_NONE;
}
static void MissionFillTile(void *data, const struct vec2i v)
{
	PaintFloodFillData *pData = data;
	SetTile(&pData->mb, pData->m, v, pData->toType);
}
static bool MissionIsTileSame(void *data, const struct vec2i v)
{
	PaintFloodFillData *pData = data;
	if (!Rect2iIsInside(Rect2iNew(svec2i_zero(), pData->mb.Map->Size), v))
	{
		return false;
	}
	return MissionStaticGetTile(
			   &pData->mb.mission->u.Static, pData->mb.Map->Size, v) ==
		   pData->fromType;
}
static void EditorBrushPaintBox(
	EditorBrush *b, Mission *m, const int lineType, const int fillType)
{
	// Draw the fill first, then the line
	// This will create the expected results when brush size is
	// greater than one, without having the box drawing routine made
	// aware of brush sizes
	struct vec2i v;
	struct vec2i d = svec2i(
		b->Pos.x > b->LastPos.x ? 1 : -1, b->Pos.y > b->LastPos.y ? 1 : -1);
	EditorBrushPaintTilesAtData paintData;
	paintData.brush = b;
	paintData.mission = m;
	// Draw fill
	if (fillType >= 0)
	{
		b->PaintType = fillType;
		for (v.y = b->LastPos.y; v.y != b->Pos.y + d.y; v.y += d.y)
		{
			for (v.x = b->LastPos.x; v.x != b->Pos.x + d.x; v.x += d.x)
			{
				if (v.x != b->LastPos.x && v.x != b->Pos.x &&
					v.y != b->LastPos.y && v.y != b->Pos.y)
				{
					EditorBrushPaintTilesAt(&paintData, v);
				}
			}
		}
	}
	// Draw edge
	if (lineType >= 0)
	{
		b->PaintType = lineType;
		for (v.y = b->LastPos.y; v.y != b->Pos.y + d.y; v.y += d.y)
		{
			for (v.x = b->LastPos.x; v.x != b->Pos.x + d.x; v.x += d.x)
			{
				if (v.x == b->LastPos.x || v.x == b->Pos.x ||
					v.y == b->LastPos.y || v.y == b->Pos.y)
				{
					EditorBrushPaintTilesAt(&paintData, v);
				}
			}
		}
	}
}
EditorResult EditorBrushStopPainting(EditorBrush *b, Mission *m)
{
	EditorResult result = EDITOR_RESULT_NONE;
	if (b->IsPainting)
	{
		switch (b->Type)
		{
		case BRUSHTYPE_LINE:
			EditorBrushPaintLine(b, m);
			result = EDITOR_RESULT_CHANGED;
			break;
		case BRUSHTYPE_BOX:
			EditorBrushPaintBox(b, m, b->PaintType, -1);
			result = EDITOR_RESULT_CHANGED;
			break;
		case BRUSHTYPE_BOX_FILLED:
			EditorBrushPaintBox(b, m, b->PaintType, b->PaintType);
			result = EDITOR_RESULT_CHANGED;
			break;
		case BRUSHTYPE_BOX_AND_FILL:
			EditorBrushPaintBox(b, m, b->MainType, b->SecondaryType);
			result = EDITOR_RESULT_CHANGED;
			break;
		case BRUSHTYPE_SELECT:
			if (b->IsMoving)
			{
				// Move the tiles from the source to the target
				// Need to copy all the tiles to a temp buffer first in case
				// we are moving to an overlapped position
				CArray movedTiles;
				int delta;
				CArrayInit(&movedTiles, m->u.Static.Tiles.elemSize);
				// Copy tiles to temp from selection, clearing them
				// in the process
				RECT_FOREACH(Rect2iNew(b->SelectionStart, b->SelectionSize))
				const int tile =
					MissionStaticGetTile(&m->u.Static, m->Size, _v);
				CArrayPushBack(&movedTiles, &tile);
				MissionStaticClearTile(&m->u.Static, m->Size, _v);
				RECT_FOREACH_END()
				// Move the selection to the new position
				b->SelectionStart.x += b->Pos.x - b->DragPos.x;
				b->SelectionStart.y += b->Pos.y - b->DragPos.y;
				// Copy tiles to the new area, for parts of the new area that
				// are valid
				RECT_FOREACH(Rect2iNew(b->SelectionStart, b->SelectionSize))
				if (_v.x >= 0 && _v.x < m->Size.x && _v.y >= 0 &&
					_v.y < m->Size.y)
				{
					const int tile = *(int *)CArrayGet(&movedTiles, _i);
					MissionStaticTrySetTile(&m->u.Static, m->Size, _v, tile);
					result = EDITOR_RESULT_CHANGED_AND_RELOAD;
				}
				RECT_FOREACH_END()
				CArrayTerminate(&movedTiles);
				// Update the selection to fit within map boundaries
				delta = -b->SelectionStart.x;
				if (delta > 0)
				{
					b->SelectionStart.x += delta;
					b->SelectionSize.x -= delta;
				}
				delta = -b->SelectionStart.y;
				if (delta > 0)
				{
					b->SelectionStart.y += delta;
					b->SelectionSize.y -= delta;
				}
				delta = b->SelectionStart.x + b->SelectionSize.x - m->Size.x;
				if (delta > 0)
				{
					b->SelectionSize.x -= delta;
				}
				delta = b->SelectionStart.y + b->SelectionSize.y - m->Size.y;
				if (delta > 0)
				{
					b->SelectionSize.y -= delta;
				}
				// Check if the selection is still valid; if not, invalidate it
				if (b->SelectionSize.x < 0 || b->SelectionSize.y < 0)
				{
					b->SelectionSize = svec2i_zero();
				}

				b->IsMoving = 0;
			}
			else
			{
				// Record the selection size
				b->SelectionStart = svec2i_min(b->LastPos, b->Pos);
				b->SelectionSize.x = abs(b->LastPos.x - b->Pos.x) + 1;
				b->SelectionSize.y = abs(b->LastPos.y - b->Pos.y) + 1;
				// Disallow 1x1 selection sizes
				if (b->SelectionSize.x <= 1 && b->SelectionSize.y <= 1)
				{
					b->SelectionSize = svec2i_zero();
				}
			}
			break;
		case BRUSHTYPE_SET_EXIT: {
			struct vec2i exitStart = svec2i_min(b->LastPos, b->Pos);
			struct vec2i exitEnd = svec2i_max(b->LastPos, b->Pos);
			// Clamp within map boundaries
			exitStart = svec2i_clamp(
				exitStart, svec2i_zero(),
				svec2i_subtract(m->Size, svec2i_one()));
			exitEnd = svec2i_clamp(
				exitEnd, svec2i_zero(),
				svec2i_subtract(m->Size, svec2i_one()));
			// Check that size is big enough
			const struct vec2i size = svec2i_subtract(exitEnd, exitStart);
			if (size.x < 2 || size.y < 2)
			{
				break;
			}
			// Check that exit area has changed
			Exit *exit = CArrayGet(&m->u.Static.Exits, b->u.ItemIndex);
			if (svec2i_is_equal(exitStart, exit->R.Pos) &&
				svec2i_is_equal(size, exit->R.Size))
			{
				break;
			}
			exit->R.Pos = exitStart;
			exit->R.Size = size;
			result = EDITOR_RESULT_CHANGED_AND_RELOAD;
		}
		break;
		default:
			// do nothing
			break;
		}
	}
	b->IsPainting = 0;
	CArrayClear(&b->HighlightedTiles);
	return result;
}

bool EditorBrushTryLoadGuideImage(EditorBrush *b, const char *filename)
{
	PicFree(&b->GuideImagePic);
	SDL_Surface *s = IMG_Load(filename);
	if (s == NULL)
		return false;
	SDL_Surface *sc = SDL_ConvertSurface(s, gGraphicsDevice.Format, 0);
	SDL_FreeSurface(s);
	PicLoad(&b->GuideImagePic, svec2i(sc->w, sc->h), svec2i_zero(), sc);
	SDL_FreeSurface(sc);
	return true;
}
