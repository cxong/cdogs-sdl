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

#include <string.h>
#include <stdlib.h>

#include "config.h"
#include "pics.h"
#include "draw.h"
#include "blit.h"



void AddItemToDisplayList(TTileItem * t, TTileItem ** list)
{
	while (*list && (*list)->y < t->y)
		list = &(*list)->nextToDisplay;
	t->nextToDisplay = *list;
	*list = t;
}

void
SetBuffer(int x_origin, int y_origin, struct Buffer *buffer, int width)
{
	int x, y;
	TTile *bufTile;

	buffer->width = width;

	buffer->xTop = x_origin - TILE_WIDTH * width / 2;
	//buffer->yTop = y_origin - 100;
	buffer->yTop = y_origin - TILE_HEIGHT * Y_TILES / 2;

	buffer->xStart = buffer->xTop / TILE_WIDTH;
	buffer->yStart = buffer->yTop / TILE_HEIGHT;
	if (buffer->xTop < 0)
		buffer->xStart--;
	if (buffer->yTop < 0)
		buffer->yStart--;

	buffer->dx = buffer->xStart * TILE_WIDTH - buffer->xTop;
	buffer->dy = buffer->yStart * TILE_HEIGHT - buffer->yTop;

	bufTile = &buffer->tiles[0][0];
	for (y = buffer->yStart; y < buffer->yStart + Y_TILES; y++) {
		for (x = buffer->xStart;
		     x < buffer->xStart + buffer->width; x++, bufTile++)
			if (x >= 0 && x < XMAX && y >= 0 && y < YMAX)
				*bufTile = Map(x, y);
		bufTile += X_TILES - buffer->width;
	}
}

void FixBuffer(struct Buffer *buffer, int isShadow)
{
	int x, y;
	TTile *tile, *tileBelow;

	tile = &buffer->tiles[0][0];
	tileBelow = &buffer->tiles[0][0] + X_TILES;
	for (y = 0; y < Y_TILES - 1; y++)
	{
		for (x = 0; x < buffer->width; x++, tile++, tileBelow++)
		{
			if (!(tile->flags & (MAPTILE_IS_WALL | MAPTILE_OFFSET_PIC)) &&
				(tileBelow->flags & MAPTILE_IS_WALL))
			{
				tile->pic = -1;
			}
			else if ((tile->flags & MAPTILE_IS_WALL) &&
				(tileBelow->flags & MAPTILE_IS_WALL))
			{
				tile->flags |= MAPTILE_DELAY_DRAW;
			}
		}
		tile += X_TILES - buffer->width;
		tileBelow += X_TILES - buffer->width;
	}

	tile = &buffer->tiles[0][0];
	for (y = 0; y < Y_TILES; y++) {
		for (x = 0; x < buffer->width; x++, tile++) {
			if ((tile->flags & isShadow) == isShadow) {
				tile->things = NULL;
				tile->flags |= MAPTILE_OUT_OF_SIGHT;
			}
			else
			{
				Vector2i pos;
				pos.x = x + buffer->xStart;
				pos.y = y + buffer->yStart;
				MapMarkAsVisited(pos);
				tile->flags &= ~MAPTILE_OUT_OF_SIGHT;
				tile->flags |= MAPTILE_VISITED;
			}
		}
		tile += X_TILES - buffer->width;
	}
}

void SetLineOfSight(
	struct Buffer *buffer, int x, int y, int dx, int dy, int shadowFlag)
{
	TTile *dTile = &buffer->tiles[0][0] + (y+dy)*X_TILES + (x+dx);
	if (dTile->flags & (MAPTILE_NO_SEE | shadowFlag))
	{
		TTile *tile = &buffer->tiles[0][0] + y*X_TILES + x;
		tile->flags |= shadowFlag;
	}
}

void LineOfSight(int xc, int yc, struct Buffer *buffer, int shadowFlag)
{
	int x, y;

	xc = xc / TILE_WIDTH - buffer->xStart;
	yc = yc / TILE_HEIGHT - buffer->yStart;

	for (x = xc - 2; x >= 0; x--)
	{
		SetLineOfSight(buffer, x, yc, 1, 0, shadowFlag);
	}
	for (x = xc + 2; x < buffer->width; x++)
	{
		SetLineOfSight(buffer, x, yc, -1, 0, shadowFlag);
	}
	for (y = yc - 1; y >= 0; y--)
	{
		SetLineOfSight(buffer, xc, y, 0, 1, shadowFlag);
		for (x = xc - 1; x >= 0; x--)
		{
			SetLineOfSight(buffer, x, y, 1, 1, shadowFlag);
		}
		for (x = xc + 1; x < buffer->width; x++)
		{
			SetLineOfSight(buffer, x, y, -1, 1, shadowFlag);
		}
	}
	for (y = yc + 1; y < Y_TILES; y++)
	{
		SetLineOfSight(buffer, xc, y, 0, -1, shadowFlag);
		for (x = xc - 1; x >= 0; x--)
		{
			SetLineOfSight(buffer, x, y, 1, -1, shadowFlag);
		}
		for (x = xc + 1; x < buffer->width; x++)
		{
			SetLineOfSight(buffer, x, y, -1, -1, shadowFlag);
		}
	}
	
	if (gConfig.Game.SightRange > 0)
	{
		Vector2i c;
		int distanceSquared = gConfig.Game.SightRange * gConfig.Game.SightRange;
		c.x = xc;
		c.y = yc;
		for (y = 0; y < Y_TILES; y++)
		{
			for (x = 0; x < buffer->width; x++)
			{
				Vector2i v;
				v.x = x;
				v.y = y;
				if (DistanceSquared(c, v) >= distanceSquared)
				{
					TTile *tile = &buffer->tiles[0][0] + y*X_TILES + x;
					tile->flags |= shadowFlag;
				}
			}
		}
	}
}


// Two types of tile drawing, based on line of sight:
// Out of sight: dark
// In sight: full color
static color_t GetTileLOSMask(int flags)
{
	if (!(flags & MAPTILE_VISITED))
	{
		return colorBlack;
	}
	if (flags & MAPTILE_OUT_OF_SIGHT)
	{
		if (gConfig.Game.Fog)
		{
			color_t mask = { 96, 96, 96 };
			return mask;
		}
		else
		{
			return colorBlack;
		}
	}
	return colorWhite;
}

void DrawWallColumn(int y, int xc, int yc, TTile * tile)
{
	while (y >= 0 && (tile->flags & MAPTILE_IS_WALL))
	{
		Vector2i pos;
		pos.x = xc;
		pos.y = yc;
		BlitWithMask(
			&gGraphicsDevice,
			gPics[tile->pic],
			pos,
			GetTileLOSMask(tile->flags));
		yc -= TILE_HEIGHT;
		tile -= X_TILES;
		y--;
	}
}

void DrawBuffer(struct Buffer *b, int xOffset)
{
	int x, y, xc, yc;
	TTile *tile;
	TTileItem *t, *displayList;

	// First draw the floor tiles (which do not obstruct anything)
	tile = &b->tiles[0][0];
	for (y = 0, yc = b->dy; y < Y_TILES; y++, yc += TILE_HEIGHT)
	{
		for (x = 0, xc = b->dx + xOffset;
			 x < b->width;
			 x++, tile++, xc += TILE_WIDTH)
		{
			if (tile->pic >= 0 &&
				!(tile->flags & (MAPTILE_IS_WALL | MAPTILE_OFFSET_PIC)))
			{
				Vector2i pos;
				pos.x = xc;
				pos.y = yc;
				BlitWithMask(
					&gGraphicsDevice,
					gPics[tile->pic],
					pos,
					GetTileLOSMask(tile->flags));
			}
		}
		tile += X_TILES - b->width;
	}

	// Now draw walls and things in proper order
	tile = &b->tiles[0][0];
	yc = b->dy + cWallOffset.dy;
	for (y = 0; y < Y_TILES; y++, yc += TILE_HEIGHT) {
		displayList = NULL;
		xc = b->dx + cWallOffset.dx + xOffset;
		for (x = 0; x < b->width; x++, tile++, xc += TILE_WIDTH)
		{
			if (tile->flags & MAPTILE_IS_WALL)
			{
				if (!(tile->flags & MAPTILE_DELAY_DRAW))
				{
					DrawWallColumn(y, xc, yc, tile);
				}
			}
			else if ((tile->flags & MAPTILE_OFFSET_PIC))
			{
				const TOffsetPic *p;
				p = &(cGeneralPics[tile->pic]);
				DrawPic (xc + p->dx,
				     yc + p->dy,
				     gPics[p->picIndex],
				     gCompiledPics[p->picIndex]);
			}
			t = tile->things;
			while (t) {
				AddItemToDisplayList(t, &displayList);
				t = t->next;
			}
		}

		t = displayList;
		while (t) {
			(*(t->drawFunc)) (t->x - b->xTop +
					  xOffset, t->y - b->yTop,
					  t->data);
			t = t->nextToDisplay;
		}
		tile += X_TILES - b->width;
	}
}
