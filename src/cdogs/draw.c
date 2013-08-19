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


void FixBuffer(DrawBuffer *buffer, int isShadow)
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
				MapMarkAsVisited(
					Vec2iNew(x + buffer->xStart, y + buffer->yStart));
				tile->flags &= ~MAPTILE_OUT_OF_SIGHT;
				tile->isVisited = 1;
			}
		}
		tile += X_TILES - buffer->width;
	}
}

void SetLineOfSight(
	DrawBuffer *buffer, int x, int y, int dx, int dy, int shadowFlag)
{
	TTile *dTile = &buffer->tiles[0][0] + (y+dy)*X_TILES + (x+dx);
	if (dTile->flags & (MAPTILE_NO_SEE | shadowFlag))
	{
		TTile *tile = &buffer->tiles[0][0] + y*X_TILES + x;
		tile->flags |= shadowFlag;
	}
}

void LineOfSight(int xc, int yc, DrawBuffer *buffer, int shadowFlag)
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
		Vec2i c = Vec2iNew(xc, yc);
		int distanceSquared = gConfig.Game.SightRange * gConfig.Game.SightRange;
		for (y = 0; y < Y_TILES; y++)
		{
			for (x = 0; x < buffer->width; x++)
			{
				if (DistanceSquared(c, Vec2iNew(x, y)) >= distanceSquared)
				{
					TTile *tile = &buffer->tiles[0][0] + y*X_TILES + x;
					tile->flags |= shadowFlag;
				}
			}
		}
	}
}


// Three types of tile drawing, based on line of sight:
// Unvisited: black
// Out of sight: dark, or if fog disabled, black
// In sight: full color
static color_t GetTileLOSMask(TTile *tile)
{
	if (!tile->isVisited)
	{
		return colorBlack;
	}
	if (tile->flags & MAPTILE_OUT_OF_SIGHT)
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

void DrawWallColumn(int y, Vec2i pos, TTile *tile)
{
	while (y >= 0 && (tile->flags & MAPTILE_IS_WALL))
	{
		BlitMasked(
			&gGraphicsDevice,
			gPics[tile->pic],
			pos,
			GetTileLOSMask(tile),
			0);
		pos.y -= TILE_HEIGHT;
		tile -= X_TILES;
		y--;
	}
}


void AddItemToDisplayList(TTileItem * t, TTileItem ** list);

void DrawBufferDraw(DrawBuffer *b, int xOffset)
{
	int x, y;
	Vec2i pos;
	TTile *tile;

	// First draw the floor tiles (which do not obstruct anything)
	tile = &b->tiles[0][0];
	for (y = 0, pos.y = b->dy; y < Y_TILES; y++, pos.y += TILE_HEIGHT)
	{
		for (x = 0, pos.x = b->dx + xOffset;
			 x < b->width;
			 x++, tile++, pos.x += TILE_WIDTH)
		{
			if (tile->pic >= 0 &&
				!(tile->flags & (MAPTILE_IS_WALL | MAPTILE_OFFSET_PIC)))
			{
				BlitMasked(
					&gGraphicsDevice,
					gPics[tile->pic],
					pos,
					GetTileLOSMask(tile),
					0);
			}
		}
		tile += X_TILES - b->width;
	}
	
	// Then draw debris (wrecks)
	tile = &b->tiles[0][0];
	for (y = 0; y < Y_TILES; y++)
	{
		TTileItem *displayList = NULL;
		TTileItem *t;
		for (x = 0; x < b->width; x++, tile++)
		{
			for (t = tile->things; t; t = t->next)
			{
				if (t->flags & TILEITEM_IS_WRECK)
				{
					AddItemToDisplayList(t, &displayList);
				}
			}
		}
		for (t = displayList; t; t = t->nextToDisplay)
		{
			(*(t->drawFunc))(t->x - b->xTop + xOffset, t->y - b->yTop, t->data);
		}
		tile += X_TILES - b->width;
	}

	// Now draw walls and (non-wreck) things in proper order
	tile = &b->tiles[0][0];
	pos.y = b->dy + cWallOffset.dy;
	for (y = 0; y < Y_TILES; y++, pos.y += TILE_HEIGHT)
	{
		TTileItem *displayList = NULL;
		TTileItem *t;
		pos.x = b->dx + cWallOffset.dx + xOffset;
		for (x = 0; x < b->width; x++, tile++, pos.x += TILE_WIDTH)
		{
			if (tile->flags & MAPTILE_IS_WALL)
			{
				if (!(tile->flags & MAPTILE_DELAY_DRAW))
				{
					DrawWallColumn(y, pos, tile);
				}
			}
			else if ((tile->flags & MAPTILE_OFFSET_PIC))
			{
				// Drawing doors
				const TOffsetPic *p = &(cGeneralPics[tile->pic]);
				BlitMasked(
					&gGraphicsDevice,
					gPics[p->picIndex],
					Vec2iNew(pos.x + p->dx, pos.y + p->dy),
					GetTileLOSMask(tile),
					0);
			}
			for (t = tile->things; t; t = t->next)
			{
				if (!(t->flags & TILEITEM_IS_WRECK))
				{
					AddItemToDisplayList(t, &displayList);
				}
			}
		}
		for (t = displayList; t; t = t->nextToDisplay)
		{
			(*(t->drawFunc))(t->x - b->xTop + xOffset, t->y - b->yTop, t->data);
		}
		tile += X_TILES - b->width;
	}
}

void AddItemToDisplayList(TTileItem * t, TTileItem ** list)
{
	TTileItem *l;
	TTileItem *lPrev;
	t->nextToDisplay = NULL;
	for (lPrev = NULL, l = *list; l; lPrev = l, l = l->nextToDisplay)
	{
		// draw in Y order
		if (l->y >= t->y)
		{
			break;
		}
	}
	t->nextToDisplay = l;
	if (lPrev)
	{
		lPrev->nextToDisplay = t;
	}
	else
	{
		*list = t;
	}
}
