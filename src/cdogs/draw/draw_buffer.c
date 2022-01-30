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

    Copyright (c) 2013-2014, 2018-2019, 2022 Cong Xu
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
#include "draw/draw_buffer.h"

#include <assert.h>

#include "algorithms.h"
#include "log.h"
#include "los.h"


void DrawBufferInit(DrawBuffer *b, struct vec2i size, GraphicsDevice *g)
{
	b->OrigSize = size;
	CArrayInitFillZero(&b->tiles, sizeof(Tile *), size.x * size.y);
	b->g = g;
	CArrayInit(&b->displaylist, sizeof(const Thing *));
	CArrayReserve(&b->displaylist, 32);
}
void DrawBufferTerminate(DrawBuffer *b)
{
	CArrayTerminate(&b->tiles);
	CArrayTerminate(&b->displaylist);
}

void DrawBufferSetFromMap(
	DrawBuffer *buffer, const Map *map, const struct vec2 origin,
	const int width)
{
	buffer->Size = svec2i(width, buffer->OrigSize.y);

	buffer->xTop = (int)origin.x - TILE_WIDTH * width / 2;
	buffer->yTop = (int)origin.y - TILE_HEIGHT * buffer->OrigSize.y / 2;

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

	Tile **bufTile = CArrayGet(&buffer->tiles, 0);
	struct vec2i pos;
	for (pos.y = buffer->yStart;
		pos.y < buffer->yStart + buffer->Size.y;
		pos.y++)
	{
		for (pos.x = buffer->xStart;
			pos.x < buffer->xStart + buffer->Size.x;
			pos.x++, bufTile++)
		{
			if (MapIsTileIn(map, pos))
			{
				*bufTile = MapGetTile(map, pos);
			}
			else
			{
				*bufTile = NULL;
			}
		}
		bufTile += buffer->OrigSize.x - buffer->Size.x;
	}
}

// Set visibility and draw order for wall/door columns
void DrawBufferFix(DrawBuffer *buffer)
{
	int tileIdx = 0;
	for (int y = 0; y < Y_TILES; y++)
	{
		for (int x = 0; x < buffer->Size.x; x++, tileIdx++)
		{
			Tile **tile = CArrayGet(&buffer->tiles, tileIdx);
			if (*tile == NULL) continue;
			const struct vec2i mapTile =
				svec2i(x + buffer->xStart, y + buffer->yStart);
			(*tile)->outOfSight = !LOSTileIsVisible(&gMap, mapTile);
		}
		tileIdx += X_TILES - buffer->Size.x;
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
	const Thing * const *t1 = v1;
	const Thing * const *t2 = v2;
	if ((*t1)->Pos.y < (*t2)->Pos.y)
	{
		return -1;
	}
	else if ((*t1)->Pos.y >(*t2)->Pos.y)
	{
		return 1;
	}
	return 0;
}

const Tile **DrawBufferGetFirstTile(const DrawBuffer *b)
{
	return CArrayGet(&b->tiles, 0);
}
