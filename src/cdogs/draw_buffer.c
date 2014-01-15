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

    Copyright (c) 2013, Cong Xu
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

void DrawBufferInit(DrawBuffer *b, Vec2i size)
{
	int i;
	CMALLOC(b->tiles, size.x * sizeof *b->tiles);
	CMALLOC(b->tiles[0], size.x * size.y * sizeof *b->tiles[0]);
	for(i = 1; i < size.x; i++)
	{
		b->tiles[i] = b->tiles[0] + i * size.y;
	}
}
void DrawBufferTerminate(DrawBuffer *b)
{
	CFREE(b->tiles[0]);
	CFREE(b->tiles);
}

void DrawBufferSetFromMap(
	DrawBuffer *buffer, Map *map, Vec2i origin,
	int width, Vec2i tilesXY)
{
	int x, y;
	Tile *bufTile;

	buffer->width = width;

	buffer->xTop = origin.x - TILE_WIDTH * width / 2;
	//buffer->yTop = y_origin - 100;
	buffer->yTop = origin.y - TILE_HEIGHT * tilesXY.y / 2;

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
	for (y = buffer->yStart; y < buffer->yStart + tilesXY.y; y++)
	{
		for (x = buffer->xStart;
			x < buffer->xStart + buffer->width;
			x++, bufTile++)
		{
			if (x >= 0 && x < XMAX && y >= 0 && y < YMAX)
			{
				*bufTile = *MapGetTile(map, Vec2iNew(x, y));
			}
			else
			{
				*bufTile = TileNone();
			}
		}
		bufTile += tilesXY.x - buffer->width;
	}
}
