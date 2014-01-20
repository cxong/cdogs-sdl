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
	case BRUSHTYPE_BOX:
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
	b->IsPainting = 1;
	b->PaintType = isMain ? b->MainType : b->SecondaryType;
	switch (b->Type)
	{
	case BRUSHTYPE_POINT:
		EditorBrushPaintLine(b, m);
		return 1;
	case BRUSHTYPE_LINE:	// fallthrough
	case BRUSHTYPE_BOX:	// fallthrough
	case BRUSHTYPE_BOX_FILLED:
		// don't paint until the end
		break;
	default:
		assert(0 && "unknown brush type");
		break;
	}
	return 0;
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
			{
				Vec2i v;
				Vec2i d = Vec2iNew(
					b->Pos.x > b->LastPos.x ? 1 : -1,
					b->Pos.y > b->LastPos.y ? 1 : -1);
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
				hasPainted = 1;
			}
			break;
		case BRUSHTYPE_BOX_FILLED:
			{
				Vec2i v;
				Vec2i d = Vec2iNew(
					b->Pos.x > b->LastPos.x ? 1 : -1,
					b->Pos.y > b->LastPos.y ? 1 : -1);
				for (v.y = b->LastPos.y; v.y != b->Pos.y + d.y; v.y += d.y)
				{
					for (v.x = b->LastPos.x; v.x != b->Pos.x + d.x; v.x += d.x)
					{
						EditorBrushPaintTilesAt(b, v, m);
					}
				}
				hasPainted = 1;
			}
			break;
		}
	}
	b->IsPainting = 0;
	CArrayClear(&b->HighlightedTiles);
	return hasPainted;
}
