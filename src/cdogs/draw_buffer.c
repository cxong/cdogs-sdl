/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (C) 1995 Ronny Wester
    Copyright (C) 2003 Jeremy Chin 
    Copyright (C) 2003-2007 Lucas Martin-King 

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    This file incorporates work covered by the following copyright and
    permission notice:

    Copyright (c) 2013-2014, Cong Xu
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
#include "draw_buffer.h"

#include <assert.h>

#include "algorithms.h"


void DrawBufferInit(DrawBuffer *b, Vec2i size, GraphicsDevice *g)
{
	debug(D_MAX, "Initialising draw buffer %dx%d\n", size.x, size.y);
	b->OrigSize = size;
	CMALLOC(b->tiles, size.x * sizeof *b->tiles);
	CMALLOC(b->tiles[0], size.x * size.y * sizeof *b->tiles[0]);
	for (int i = 1; i < size.x; i++)
	{
		b->tiles[i] = b->tiles[0] + i * size.y;
	}
	b->g = g;
	CArrayInit(&b->displaylist, sizeof(const TTileItem *));
	CArrayReserve(&b->displaylist, 32);
	debug(D_MAX, "Initialised draw buffer %dx%d\n", size.x, size.y);
}
void DrawBufferTerminate(DrawBuffer *b)
{
	CFREE(b->tiles[0]);
	CFREE(b->tiles);
	CArrayTerminate(&b->displaylist);
}

void DrawBufferSetFromMap(
	DrawBuffer *buffer, Map *map, Vec2i origin, int width)
{
	int x, y;
	Tile *bufTile;

	buffer->Size = Vec2iNew(width, buffer->OrigSize.y);

	buffer->xTop = origin.x - TILE_WIDTH * width / 2;
	//buffer->yTop = y_origin - 100;
	buffer->yTop = origin.y - TILE_HEIGHT * buffer->OrigSize.y / 2;

	buffer->xStart = buffer->xTop / TILE_WIDTH;
	buffer->yStart = buffer->yTop / TILE_HEIGHT;
	if (buffer->xTop < 0)
	{
		buffer->xStart--;
	}
	if (buffer->yTop < 0)
	{
		buffer->yStart--;
	}

	buffer->dx = buffer->xStart * TILE_WIDTH - buffer->xTop;
	buffer->dy = buffer->yStart * TILE_HEIGHT - buffer->yTop;

	bufTile = &buffer->tiles[0][0];
	for (y = buffer->yStart; y < buffer->yStart + buffer->Size.y; y++)
	{
		for (x = buffer->xStart;
			x < buffer->xStart + buffer->Size.x;
			x++, bufTile++)
		{
			if (x >= 0 && x < map->Size.x && y >= 0 && y < map->Size.y)
			{
				*bufTile = *MapGetTile(map, Vec2iNew(x, y));
			}
			else
			{
				*bufTile = TileNone();
			}
		}
		bufTile += buffer->OrigSize.x - buffer->Size.x;
	}
}

static Tile *GetTile(DrawBuffer *buffer, Vec2i pos)
{
	if (pos.x < 0 || pos.x >= buffer->Size.x ||
		pos.y < 0 || pos.y >= buffer->Size.y)
	{
		return NULL;
	}
	return &buffer->tiles[0][0] + pos.y * buffer->OrigSize.x + pos.x;
}

typedef struct
{
	DrawBuffer *b;
	Vec2i center;
	int sightRange2;
} LOSData;
static bool IsNextTileBlockedAndSetVisibility(void *data, Vec2i pos)
{
	LOSData *lData = data;
	// Check sight range
	if (lData->sightRange2 > 0 &&
		DistanceSquared(lData->center, pos) >= lData->sightRange2)
	{
		return true;
	}
	// Check buffer range
	Tile *tile = GetTile(lData->b, pos);
	if (!tile)
	{
		return true;
	}
	tile->flags |= MAPTILE_IS_VISIBLE;
	// Check if this tile is an obstruction
	return tile->flags & MAPTILE_NO_SEE;
}

static bool IsTileVisibleNonObstruction(DrawBuffer *buffer, Vec2i pos)
{
	Tile *tile = GetTile(buffer, pos);
	if (!tile)
	{
		return false;
	}
	return !(tile->flags & MAPTILE_NO_SEE) &&
		(tile->flags & MAPTILE_IS_VISIBLE);
}

static void SetObstructionVisible(DrawBuffer *buffer, Vec2i pos, Tile *tile)
{
	Vec2i d;
	for (d.x = -1; d.x < 2; d.x++)
	{
		for (d.y = -1; d.y < 2; d.y++)
		{
			if (IsTileVisibleNonObstruction(buffer, Vec2iAdd(pos, d)))
			{
				tile->flags |= MAPTILE_IS_VISIBLE;
				return;
			}
		}
	}
}

// Perform LOS by casting rays from the centre to the edges, terminating
// whenever an obstruction or out-of-range is reached.
void DrawBufferLOS(DrawBuffer *buffer, Vec2i center)
{
	int sightRange = gConfig.Game.SightRange;	// Note: can be zero
	LOSData data;
	data.b = buffer;
	data.center.x = center.x / TILE_WIDTH - buffer->xStart;
	data.center.y = center.y / TILE_HEIGHT - buffer->yStart;
	data.sightRange2 = sightRange * sightRange;

	// First mark center tile and all adjacent tiles as visible
	// +-+-+-+
	// |V|V|V|
	// +-+-+-+
	// |V|C|V|
	// +-+-+-+
	// |V|V|V|  (C=center, V=visible)
	// +-+-+-+
	Vec2i end;
	for (end.x = data.center.x - 1; end.x < data.center.x + 2; end.x++)
	{
		for (end.y = data.center.y - 1; end.y < data.center.y + 2; end.y++)
		{
			Tile *tile = GetTile(buffer, end);
			if (tile)
			{
				tile->flags |= MAPTILE_IS_VISIBLE;
			}
		}
	}

	// Work out the perimeter of the LOS casts
	Vec2i origin = Vec2iZero();
	if (sightRange > 0)
	{
		// Limit the perimeter to the sight range
		origin.x = MAX(origin.x, data.center.x - sightRange);
		origin.y = MAX(origin.y, data.center.y - sightRange);
	}
	Vec2i perimSize = Vec2iScale(Vec2iMinus(data.center, origin), 2);

	// Start from the top-left cell, and proceed clockwise around
	end = origin;
	HasClearLineData lineData;
	lineData.IsBlocked = IsNextTileBlockedAndSetVisibility;
	lineData.data = &data;
	// Top edge
	for (; end.x < origin.x + perimSize.x; end.x++)
	{
		HasClearLineXiaolinWu(data.center, end, &lineData);
	}
	// right edge
	for (; end.y < origin.y + perimSize.y; end.y++)
	{
		HasClearLineXiaolinWu(data.center, end, &lineData);
	}
	// bottom edge
	for (; end.x > origin.x; end.x--)
	{
		HasClearLineXiaolinWu(data.center, end, &lineData);
	}
	// left edge
	for (; end.y > origin.y; end.y--)
	{
		HasClearLineXiaolinWu(data.center, end, &lineData);
	}

	// Second pass: make any non-visible obstructions that are adjacent to
	// visible non-obstructions visible too
	// This is to ensure runs of walls stay visible
	for (end.y = origin.y; end.y < origin.y + perimSize.y; end.y++)
	{
		for (end.x = origin.x; end.x < origin.x + perimSize.x; end.x++)
		{
			Tile *tile = GetTile(buffer, end);
			if (!tile || !(tile->flags & MAPTILE_NO_SEE))
			{
				continue;
			}
			// Check sight range
			if (data.sightRange2 > 0 &&
				DistanceSquared(data.center, end) >= data.sightRange2)
			{
				continue;
			}
			SetObstructionVisible(buffer, end, tile);
		}
	}
}

static int CompareY(const void *v1, const void *v2);
void DrawBufferSortDisplayList(DrawBuffer *buffer)
{
	qsort(
		buffer->displaylist.data,
		buffer->displaylist.size,
		buffer->displaylist.elemSize,
		CompareY);
}
static int CompareY(const void *v1, const void *v2)
{
	const TTileItem * const *t1 = v1;
	const TTileItem * const *t2 = v2;
	if ((*t1)->y < (*t2)->y)
	{
		return -1;
	}
	else if ((*t1)->y >(*t2)->y)
	{
		return 1;
	}
	return 0;
}
