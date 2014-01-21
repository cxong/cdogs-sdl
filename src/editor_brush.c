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

#include <cdogs/map.h>
#include <cdogs/mission_convert.h>


const char *BrushTypeStr(BrushType t)
{
	switch (t)
	{
	case BRUSHTYPE_POINT:
		return "Point";
	case BRUSHTYPE_LINE:
		return "Line";
	case BRUSHTYPE_BOX:
		return "Box";
	case BRUSHTYPE_BOX_FILLED:
		return "Box Filled";
	case BRUSHTYPE_ROOM:
		return "Room";
	case BRUSHTYPE_SELECT:
		return "Select";
	default:
		assert(0 && "unknown brush type");
		return "";
	}
}
BrushType StrBrushType(const char *s)
{
	if (strcmp(s, "Point") == 0)
	{
		return BRUSHTYPE_POINT;
	}
	else if (strcmp(s, "Line") == 0)
	{
		return BRUSHTYPE_LINE;
	}
	else if (strcmp(s, "Box") == 0)
	{
		return BRUSHTYPE_BOX;
	}
	else if (strcmp(s, "Box Filled") == 0)
	{
		return BRUSHTYPE_BOX_FILLED;
	}
	else if (strcmp(s, "Room") == 0)
	{
		return BRUSHTYPE_ROOM;
	}
	else if (strcmp(s, "Select") == 0)
	{
		return BRUSHTYPE_SELECT;
	}
	else
	{
		assert(0 && "unknown brush type");
		return BRUSHTYPE_POINT;
	}
}


void EditorBrushInit(EditorBrush *b)
{
	b->Type = BRUSHTYPE_POINT;
	b->MainType = MAP_WALL;
	b->SecondaryType = MAP_FLOOR;
	b->PaintType = b->MainType;
	b->IsActive = 0;
	b->IsPainting = 0;
	b->BrushSize = 1;
	b->LastPos = Vec2iNew(-1, -1);
	b->Pos = Vec2iNew(-1, -1);
	CArrayInit(&b->HighlightedTiles, sizeof(Vec2i));
}
void EditorBrushTerminate(EditorBrush *b)
{
	CArrayTerminate(&b->HighlightedTiles);
}

static void BresenhamLine(
	EditorBrush *b, Vec2i start, Vec2i end,
	void (*func)(EditorBrush *, Vec2i, void *), void *data)
{
	// Bresenham's line algorithm
	Vec2i d = Vec2iNew(abs(end.x - start.x), abs(end.y - start.y));
	Vec2i s = Vec2iNew(start.x < end.x ? 1 : -1, start.y < end.y ? 1 : -1);
	int err = d.x - d.y;
	Vec2i v = start;
	for (;;)
	{
		int e2 = 2 * err;
		if (Vec2iEqual(v, end))
		{
			break;
		}
		func(b, v, data);
		if (e2 > -d.y)
		{
			err -= d.y;
			v.x += s.x;
		}
		if (Vec2iEqual(v, end))
		{
			break;
		}
		if (e2 < d.x)
		{
			err += d.x;
			v.y += s.y;
		}
	}
	func(b, end, data);
}

static void EditorBrushHighlightPoint(EditorBrush *b, Vec2i p, void *data)
{
	Vec2i v;
	UNUSED(data);
	for (v.y = 0; v.y < b->BrushSize; v.y++)
	{
		for (v.x = 0; v.x < b->BrushSize; v.x++)
		{
			Vec2i pos = Vec2iAdd(p, v);
			CArrayPushBack(&b->HighlightedTiles, &pos);
		}
	}
}
void EditorBrushSetHighlightedTiles(EditorBrush *b)
{
	int useSimpleHighlight = 1;
	switch (b->Type)
	{
	case BRUSHTYPE_POINT:
		useSimpleHighlight = 1;
		break;
	case BRUSHTYPE_LINE:
		if (b->IsPainting)
		{
			useSimpleHighlight = 0;
			// highlight a line
			CArrayClear(&b->HighlightedTiles);
			BresenhamLine(
				b, b->LastPos, b->Pos, EditorBrushHighlightPoint, NULL);
		}
		break;
	case BRUSHTYPE_BOX:	// fallthrough
	case BRUSHTYPE_ROOM:
		if (b->IsPainting)
		{
			Vec2i v;
			Vec2i d = Vec2iNew(
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
						EditorBrushHighlightPoint(b, v, NULL);
					}
				}
			}
		}
		break;
	case BRUSHTYPE_BOX_FILLED:
		if (b->IsPainting)
		{
			Vec2i v;
			Vec2i d = Vec2iNew(
				b->Pos.x > b->LastPos.x ? 1 : -1,
				b->Pos.y > b->LastPos.y ? 1 : -1);
			useSimpleHighlight = 0;
			CArrayClear(&b->HighlightedTiles);
			for (v.y = b->LastPos.y; v.y != b->Pos.y + d.y; v.y += d.y)
			{
				for (v.x = b->LastPos.x; v.x != b->Pos.x + d.x; v.x += d.x)
				{
					EditorBrushHighlightPoint(b, v, NULL);
				}
			}
		}
		break;
	case BRUSHTYPE_SELECT:
		if (b->IsPainting)
		{
			if (b->IsMoving)
			{
				Vec2i v;
				Vec2i offset = Vec2iNew(
					b->Pos.x - b->DragPos.x, b->Pos.y - b->DragPos.y);
				useSimpleHighlight = 0;
				CArrayClear(&b->HighlightedTiles);
				for (v.y = 0; v.y < b->SelectionSize.y; v.y++)
				{
					for (v.x = 0; v.x < b->SelectionSize.x; v.x++)
					{
						Vec2i vOffset = Vec2iAdd(
							Vec2iAdd(v, b->SelectionStart), offset);
						EditorBrushHighlightPoint(b, vOffset, NULL);
					}
				}
			}
			else
			{
				// resize the selection
				Vec2i v;
				Vec2i d = Vec2iNew(
					b->Pos.x > b->LastPos.x ? 1 : -1,
					b->Pos.y > b->LastPos.y ? 1 : -1);
				useSimpleHighlight = 0;
				CArrayClear(&b->HighlightedTiles);
				for (v.y = b->LastPos.y; v.y != b->Pos.y + d.y; v.y += d.y)
				{
					for (v.x = b->LastPos.x; v.x != b->Pos.x + d.x; v.x += d.x)
					{
						EditorBrushHighlightPoint(b, v, NULL);
					}
				}
			}
		}
		else
		{
			// If there's a valid selection, draw that instead
			if (b->SelectionSize.x > 0 && b->SelectionSize.y > 0)
			{
				Vec2i v;
				useSimpleHighlight = 0;
				CArrayClear(&b->HighlightedTiles);
				for (v.y = 0; v.y < b->SelectionSize.y; v.y++)
				{
					for (v.x = 0; v.x < b->SelectionSize.x; v.x++)
					{
						Vec2i vOffset = Vec2iAdd(v, b->SelectionStart);
						EditorBrushHighlightPoint(b, vOffset, NULL);
					}
				}
			}
		}
	}
	if (useSimpleHighlight)
	{
		// Simple highlight at brush tip based on brush size
		CArrayClear(&b->HighlightedTiles);
		EditorBrushHighlightPoint(b, b->Pos, NULL);
	}
}

static void EditorBrushPaintTilesAt(EditorBrush *b, Vec2i pos, Mission *m)
{
	Vec2i v;
	for (v.y = 0; v.y < b->BrushSize; v.y++)
	{
		for (v.x = 0; v.x < b->BrushSize; v.x++)
		{
			Vec2i paintPos = Vec2iAdd(pos, v);
			MissionSetTile(m, paintPos, b->PaintType);
		}
	}
}
static void EditorBrushPaintLine(EditorBrush *b, Mission *m)
{
	// Draw tiles between the last point and the current point
	if (b->IsPainting)
	{
		BresenhamLine(b, b->LastPos, b->Pos, EditorBrushPaintTilesAt, m);
	}
	EditorBrushPaintTilesAt(b, b->Pos, m);
	b->IsPainting = 1;
	b->LastPos = b->Pos;
}
int EditorBrushStartPainting(EditorBrush *b, Mission *m, int isMain)
{
	if (!b->IsPainting)
	{
		b->LastPos = b->Pos;
	}
	b->PaintType = isMain ? b->MainType : b->SecondaryType;
	switch (b->Type)
	{
	case BRUSHTYPE_POINT:
		b->IsPainting = 1;
		EditorBrushPaintLine(b, m);
		return 1;
	case BRUSHTYPE_LINE:	// fallthrough
	case BRUSHTYPE_BOX:	// fallthrough
	case BRUSHTYPE_BOX_FILLED:	// fallthrough
	case BRUSHTYPE_ROOM:	// fallthrough
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
	default:
		assert(0 && "unknown brush type");
		break;
	}
	b->IsPainting = 1;
	return 0;
}
static void EditorBrushPaintBox(
	EditorBrush *b, Mission *m,
	unsigned short lineType, unsigned short fillType)
{
	// Draw the fill first, then the line
	// This will create the expected results when brush size is
	// greater than one, without having the box drawing routine made
	// aware of brush sizes
	Vec2i v;
	Vec2i d = Vec2iNew(
		b->Pos.x > b->LastPos.x ? 1 : -1,
		b->Pos.y > b->LastPos.y ? 1 : -1);
	// Draw fill
	if (fillType != MAP_NOTHING)
	{
		b->PaintType = fillType;
		for (v.y = b->LastPos.y; v.y != b->Pos.y + d.y; v.y += d.y)
		{
			for (v.x = b->LastPos.x; v.x != b->Pos.x + d.x; v.x += d.x)
			{
				if (v.x != b->LastPos.x && v.x != b->Pos.x &&
					v.y != b->LastPos.y && v.y != b->Pos.y)
				{
					EditorBrushPaintTilesAt(b, v, m);
				}
			}
		}
	}
	if (lineType != MAP_NOTHING)
	{
		b->PaintType = lineType;
		for (v.y = b->LastPos.y; v.y != b->Pos.y + d.y; v.y += d.y)
		{
			for (v.x = b->LastPos.x; v.x != b->Pos.x + d.x; v.x += d.x)
			{
				if (v.x == b->LastPos.x || v.x == b->Pos.x ||
					v.y == b->LastPos.y || v.y == b->Pos.y)
				{
					EditorBrushPaintTilesAt(b, v, m);
				}
			}
		}
	}
}
int EditorBrushStopPainting(EditorBrush *b, Mission *m)
{
	int hasPainted = 0;
	if (b->IsPainting)
	{
		switch (b->Type)
		{
		case BRUSHTYPE_LINE:
			EditorBrushPaintLine(b, m);
			hasPainted = 1;
			break;
		case BRUSHTYPE_BOX:
			EditorBrushPaintBox(b, m, b->PaintType, MAP_NOTHING);
			hasPainted = 1;
			break;
		case BRUSHTYPE_BOX_FILLED:
			EditorBrushPaintBox(b, m, b->PaintType, b->PaintType);
			hasPainted = 1;
			break;
		case BRUSHTYPE_ROOM:
			EditorBrushPaintBox(b, m, MAP_WALL, MAP_ROOM);
			hasPainted = 1;
			break;
		case BRUSHTYPE_SELECT:
			if (b->IsMoving)
			{
				// Move the tiles from the source to the target
				// Need to copy all the tiles to a temp buffer first in case
				// we are moving to an overlapped position
				CArray movedTiles;
				Vec2i v;
				int i;
				int delta;
				CArrayInit(&movedTiles, sizeof(unsigned short));
				// Copy tiles to temp from selection, setting them to MAP_FLOOR
				// in the process
				for (v.y = 0; v.y < b->SelectionSize.y; v.y++)
				{
					for (v.x = 0; v.x < b->SelectionSize.x; v.x++)
					{
						Vec2i vOffset = Vec2iAdd(v, b->SelectionStart);
						int index = vOffset.y * m->Size.x + vOffset.x;
						unsigned short *tile = CArrayGet(
							&m->u.StaticTiles, index);
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
						Vec2i vOffset = Vec2iAdd(v, b->SelectionStart);
						if (vOffset.x >= 0 && vOffset.x < m->Size.x &&
							vOffset.y >= 0 && vOffset.y < m->Size.y)
						{
							int index = vOffset.y * m->Size.x + vOffset.x;
							unsigned short *tileFrom =
								CArrayGet(&movedTiles, i);
							unsigned short *tileTo = CArrayGet(
								&m->u.StaticTiles, index);
							*tileTo = *tileFrom;
							hasPainted = 1;
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
					b->SelectionSize = Vec2iZero();
				}

				b->IsMoving = 0;
			}
			else
			{
				// Record the selection size
				b->SelectionStart.x = MIN(b->LastPos.x, b->Pos.x);
				b->SelectionStart.y = MIN(b->LastPos.y, b->Pos.y);
				b->SelectionSize.x = abs(b->LastPos.x - b->Pos.x) + 1;
				b->SelectionSize.y = abs(b->LastPos.y - b->Pos.y) + 1;
				// Disallow 1x1 selection sizes
				if (b->SelectionSize.x <= 1 && b->SelectionSize.y <= 1)
				{
					b->SelectionSize = Vec2iZero();
				}
			}
		}
	}
	b->IsPainting = 0;
	CArrayClear(&b->HighlightedTiles);
	return hasPainted;
}
