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

#include <cdogs/map.h>
#include <cdogs/mission_convert.h>


void EditorBrushInit(EditorBrush *b)
{
	b->MainType = MAP_WALL;
	b->SecondaryType = MAP_FLOOR;
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

void EditorBrushSetHighlightedTiles(EditorBrush *b)
{
	Vec2i v;
	CArrayClear(&b->HighlightedTiles);
	for (v.y = 0; v.y < b->BrushSize; v.y++)
	{
		for (v.x = 0; v.x < b->BrushSize; v.x++)
		{
			Vec2i pos = Vec2iAdd(b->Pos, v);
			CArrayPushBack(&b->HighlightedTiles, &pos);
		}
	}
}

static void EditorBrushPaintTilesAt(
	EditorBrush *b, Mission *m, Vec2i pos, unsigned short tileType)
{
	Vec2i v;
	for (v.y = 0; v.y < b->BrushSize; v.y++)
	{
		for (v.x = 0; v.x < b->BrushSize; v.x++)
		{
			Vec2i paintPos = Vec2iAdd(pos, v);
			MissionSetTile(m, paintPos, tileType);
		}
	}
}
void EditorBrushPaintTiles(EditorBrush *b, Mission *m, int isMain)
{
	unsigned short tileType = isMain ? b->MainType : b->SecondaryType;
	// Draw tiles between the last point and the current point
	if (b->IsPainting)
	{
		// Bresenham's line algorithm
		Vec2i d = Vec2iNew(
			abs(b->Pos.x - b->LastPos.x), abs(b->Pos.y - b->LastPos.y));
		Vec2i s = Vec2iNew(
			b->LastPos.x < b->Pos.x ? 1 : -1,
			b->LastPos.y < b->Pos.y ? 1 : -1);
		int err = d.x - d.y;
		Vec2i v = b->LastPos;
		for (;;)
		{
			int e2 = 2 * err;
			if (Vec2iEqual(v, b->Pos))
			{
				break;
			}
			EditorBrushPaintTilesAt(b, m, v, tileType);
			if (e2 > -d.y)
			{
				err -= d.y;
				v.x += s.x;
			}
			if (Vec2iEqual(v, b->Pos))
			{
				break;
			}
			if (e2 < d.x)
			{
				err += d.x;
				v.y += s.y;
			}
		}
	}
	EditorBrushPaintTilesAt(b, m, b->Pos, tileType);
	b->IsPainting = 1;
	b->LastPos = b->Pos;
}
