/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (c) 2014, Cong Xu
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

#include <cdogs/algorithms.h>
#include <cdogs/map.h>
#include <cdogs/map_build.h>
#include <cdogs/mission_convert.h>


void EditorBrushInit(EditorBrush *b)
{
	memset(b, 0, sizeof *b);
	b->Type = BRUSHTYPE_POINT;
	b->MainType = MAP_WALL;
	b->SecondaryType = MAP_FLOOR;
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
	SDL_FreeSurface(b->GuideImageSurface);
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
	case BRUSHTYPE_POINT:	// fallthrough
	case BRUSHTYPE_ROOM_PAINTER:
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
	case BRUSHTYPE_BOX:	// fallthrough
	case BRUSHTYPE_ROOM:	// fallthrough
	case BRUSHTYPE_SET_EXIT:
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
				struct vec2i offset = svec2i(
					b->Pos.x - b->DragPos.x, b->Pos.y - b->DragPos.y);
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
						struct vec2i vOffset = svec2i_add(v, b->SelectionStart);
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

static void SetTile(Mission *m, struct vec2i pos, const uint16_t tile)
{
	if (MissionTrySetTile(m, pos, tile))
	{
		bool isRoom = false;
		const TileClass *t = MapBuildGetTileFromType(tile, &isRoom);
		MapBuildTile(&gMap, m, pos, t, isRoom);
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
	for (v.y = 0; v.y < b->BrushSize; v.y++)
	{
		for (v.x = 0; v.x < b->BrushSize; v.x++)
		{
			SetTile(m, svec2i_add(pos, v), b->PaintType);
		}
	}
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
// Paint all the edge tiles as a wall, unless they are room tiles already;
// then paint the interior as room tiles
static void EditorBrushPaintRoom(EditorBrush *b, Mission *m)
{
	struct vec2i v;
	for (v.y = 0; v.y < b->BrushSize; v.y++)
	{
		for (v.x = 0; v.x < b->BrushSize; v.x++)
		{
			uint16_t tile = MAP_ROOM;
			if (v.x == 0 || v.x == b->BrushSize - 1 ||
				v.y == 0 || v.y == b->BrushSize - 1)
			{
				tile = MAP_WALL;
			}
			const struct vec2i pos = svec2i_add(b->Pos, v);
			const TileClass *tcExisting = MapGetTile(&gMap, v)->Class;
			if (tcExisting->IsDoor || tcExisting->IsWall || tcExisting->IsFloor)
			{
				SetTile(m, pos, tile);
			}
		}
	}
	b->IsPainting = true;
	b->LastPos = b->Pos;
}
typedef struct
{
	Mission *m;
	uint16_t fromType;
	uint16_t toType;
} PaintFloodFillData;
static void MissionFillTile(void *data, struct vec2i v);
static bool MissionIsTileSame(void *data, struct vec2i v);
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
	case BRUSHTYPE_LINE:	// fallthrough
	case BRUSHTYPE_BOX:	// fallthrough
	case BRUSHTYPE_BOX_FILLED:	// fallthrough
	case BRUSHTYPE_ROOM:	// fallthrough
	case BRUSHTYPE_SET_EXIT:
		// don't paint until the end
		break;
	case BRUSHTYPE_ROOM_PAINTER:
		EditorBrushPaintRoom(b, m);
		return EDITOR_RESULT_CHANGED;
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
		if ((MissionGetTile(m, b->Pos) & MAP_MASKACCESS) != b->PaintType &&
			b->PaintType != MAP_DOOR)
		{
			FloodFillData data;
			data.Fill = MissionFillTile;
			data.IsSame = MissionIsTileSame;
			PaintFloodFillData pData;
			pData.m = m;
			pData.fromType = MissionGetTile(m, b->Pos) & MAP_MASKACCESS;
			pData.toType = b->PaintType;
			data.data = &pData;
			if (CFloodFill(b->Pos, &data))
			{
				return EDITOR_RESULT_CHANGED;
			}
		}
		return EDITOR_RESULT_NONE;
	case BRUSHTYPE_SET_PLAYER_START:
		if (MissionGetTile(m, b->Pos) == MAP_ROOM ||
			MissionGetTile(m, b->Pos) == MAP_FLOOR)
		{
			m->u.Static.Start = b->Pos;
			return EDITOR_RESULT_CHANGED;
		}
		break;
	case BRUSHTYPE_ADD_ITEM:
		if (isMain)
		{
			if (MissionStaticTryAddItem(m, b->u.MapObject, b->Pos))
			{
				return EDITOR_RESULT_CHANGED_AND_RELOAD;
			}
		}
		else
		{
			if (MissionStaticTryRemoveItemAt(m, b->Pos))
			{
				return EDITOR_RESULT_CHANGED_AND_RELOAD;
			}
		}
		break;
	case BRUSHTYPE_ADD_CHARACTER:
		if (isMain)
		{
			if (MissionStaticTryAddCharacter(m, b->u.ItemIndex, b->Pos))
			{
				return EDITOR_RESULT_CHANGED_AND_RELOAD;
			}
		}
		else
		{
			if (MissionStaticTryRemoveCharacterAt(m, b->Pos))
			{
				return EDITOR_RESULT_CHANGED_AND_RELOAD;
			}
		}
		break;
	case BRUSHTYPE_ADD_OBJECTIVE:
		if (isMain)
		{
			if (MissionStaticTryAddObjective(
				m, b->u.ItemIndex, b->Index2, b->Pos))
			{
				return EDITOR_RESULT_CHANGED_AND_RELOAD;
			}
		}
		else
		{
			if (MissionStaticTryRemoveObjectiveAt(m, b->Pos))
			{
				return EDITOR_RESULT_CHANGED_AND_RELOAD;
			}
		}
		break;
	case BRUSHTYPE_ADD_KEY:
		if (isMain)
		{
			if (MissionStaticTryAddKey(m, b->u.ItemIndex, b->Pos))
			{
				return EDITOR_RESULT_CHANGED_AND_RELOAD;
			}
		}
		else
		{
			if (MissionStaticTryRemoveKeyAt(m, b->Pos))
			{
				return EDITOR_RESULT_CHANGED_AND_RELOAD;
			}
		}
		break;
	case BRUSHTYPE_SET_KEY:
		if (isMain || b->u.ItemIndex > 0)
		{
			if (MissionStaticTrySetKey(m, b->u.ItemIndex, b->Pos))
			{
				return EDITOR_RESULT_CHANGED_AND_RELOAD;
			}
		}
		else
		{
			if (MissionStaticTryUnsetKeyAt(m, b->Pos))
			{
				return EDITOR_RESULT_CHANGED_AND_RELOAD;
			}
		}
		break;
	default:
		assert(0 && "unknown brush type");
		break;
	}
	b->IsPainting = 1;
	return EDITOR_RESULT_NONE;
}
static void MissionFillTile(void *data, struct vec2i v)
{
	PaintFloodFillData *pData = data;
	SetTile(pData->m, v, pData->toType);
}
static bool MissionIsTileSame(void *data, struct vec2i v)
{
	PaintFloodFillData *pData = data;
	return (MissionGetTile(pData->m, v) & MAP_MASKACCESS) == pData->fromType;
}
static void EditorBrushPaintBox(
	EditorBrush *b, Mission *m, uint16_t lineType, uint16_t fillType)
{
	// Draw the fill first, then the line
	// This will create the expected results when brush size is
	// greater than one, without having the box drawing routine made
	// aware of brush sizes
	struct vec2i v;
	struct vec2i d = svec2i(
		b->Pos.x > b->LastPos.x ? 1 : -1,
		b->Pos.y > b->LastPos.y ? 1 : -1);
	EditorBrushPaintTilesAtData paintData;
	paintData.brush = b;
	paintData.mission = m;
	// Draw fill
	if (fillType != MAP_UNSET)
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
	if (lineType != MAP_UNSET)
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
			EditorBrushPaintBox(b, m, b->PaintType, MAP_UNSET);
			result = EDITOR_RESULT_CHANGED;
			break;
		case BRUSHTYPE_BOX_FILLED:
			EditorBrushPaintBox(b, m, b->PaintType, b->PaintType);
			result = EDITOR_RESULT_CHANGED;
			break;
		case BRUSHTYPE_ROOM:
			EditorBrushPaintBox(b, m, MAP_WALL, MAP_ROOM);
			result = EDITOR_RESULT_CHANGED;
			break;
		case BRUSHTYPE_ROOM_PAINTER:
			// Reload map to update tiles
			result = EDITOR_RESULT_RELOAD;
			break;
		case BRUSHTYPE_SELECT:
			if (b->IsMoving)
			{
				// Move the tiles from the source to the target
				// Need to copy all the tiles to a temp buffer first in case
				// we are moving to an overlapped position
				CArray movedTiles;
				struct vec2i v;
				int i;
				int delta;
				CArrayInit(&movedTiles, sizeof(uint16_t));
				// Copy tiles to temp from selection, setting them to MAP_FLOOR
				// in the process
				for (v.y = 0; v.y < b->SelectionSize.y; v.y++)
				{
					for (v.x = 0; v.x < b->SelectionSize.x; v.x++)
					{
						struct vec2i vOffset = svec2i_add(v, b->SelectionStart);
						int idx = vOffset.y * m->Size.x + vOffset.x;
						uint16_t *tile = CArrayGet(&m->u.Static.Tiles, idx);
						CArrayPushBack(&movedTiles, tile);
						*tile = MAP_FLOOR;
					}
				}
				// Move the selection to the new position
				b->SelectionStart.x += b->Pos.x - b->DragPos.x;
				b->SelectionStart.y += b->Pos.y - b->DragPos.y;
				// Copy tiles to the new area, for parts of the new area that
				// are valid
				i = 0;
				for (v.y = 0; v.y < b->SelectionSize.y; v.y++)
				{
					for (v.x = 0; v.x < b->SelectionSize.x; v.x++)
					{
						struct vec2i vOffset = svec2i_add(v, b->SelectionStart);
						if (vOffset.x >= 0 && vOffset.x < m->Size.x &&
							vOffset.y >= 0 && vOffset.y < m->Size.y)
						{
							int idx = vOffset.y * m->Size.x + vOffset.x;
							uint16_t *tileFrom = CArrayGet(&movedTiles, i);
							uint16_t *tileTo = CArrayGet(
								&m->u.Static.Tiles, idx);
							*tileTo = *tileFrom;
							result = EDITOR_RESULT_CHANGED_AND_RELOAD;
						}
						i++;
					}
				}
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
		case BRUSHTYPE_SET_EXIT:
			{
				struct vec2i exitStart = svec2i_min(b->LastPos, b->Pos);
				struct vec2i exitEnd = svec2i_max(b->LastPos, b->Pos);
				// Clamp within map boundaries
				exitStart = svec2i_clamp(
					exitStart, svec2i_zero(), svec2i_subtract(m->Size, svec2i_one()));
				exitEnd = svec2i_clamp(
					exitEnd, svec2i_zero(), svec2i_subtract(m->Size, svec2i_one()));
				// Check that size is big enough
				struct vec2i size =
					svec2i_add(svec2i_subtract(exitEnd, exitStart), svec2i_one());
				if (size.x >= 3 && size.y >= 3)
				{
					// Check that exit area has changed
					if (!svec2i_is_equal(exitStart, m->u.Static.Exit.Start) ||
						!svec2i_is_equal(exitEnd, m->u.Static.Exit.End))
					{
						m->u.Static.Exit.Start = exitStart;
						m->u.Static.Exit.End = exitEnd;
						result = EDITOR_RESULT_CHANGED_AND_RELOAD;
					}
				}
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
