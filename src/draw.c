/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (C) 1995 Ronny Wester
    Copyright (C) 2003 Jeremy Chin 
    Copyright (C) 2003 Lucas Martin-King 

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

-------------------------------------------------------------------------------

 draw.c - drawing functions

*/

#include <string.h>
#include <stdlib.h>
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

	memset(buffer, 0, sizeof(struct Buffer));

	buffer->width = width;

	buffer->xTop = x_origin - TILE_WIDTH * width / 2;
	buffer->yTop = y_origin - 100;

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
	tileBelow = &buffer->tiles[1][0];
	for (y = 0; y < Y_TILES - 1; y++) {
		for (x = 0; x < buffer->width; x++, tile++, tileBelow++) {
			if ((tile->flags & (IS_WALL | OFFSET_PIC)) == 0 &&
			    (tileBelow->flags & IS_WALL) != 0)
				tile->pic = -1;
			else if ((tile->flags & IS_WALL) != 0 &&
				 (tileBelow->flags & IS_WALL) != 0)
				tile->flags |= DELAY_DRAW;
		}
		tile += X_TILES - buffer->width;
		tileBelow += X_TILES - buffer->width;
	}

	tile = &buffer->tiles[0][0];
	for (y = 0; y < Y_TILES; y++) {
		for (x = 0; x < buffer->width; x++, tile++) {
			if ((tile->flags & isShadow) == isShadow) {
				tile->things = NULL;
				if ((tile->
				     flags & (IS_WALL | DELAY_DRAW)) ==
				    IS_WALL)
					tile->pic = PIC_TALLDARKNESS;
				else {
					tile->pic = PIC_DARKNESS;
					tile->flags &= ~OFFSET_PIC;
				}
			} else
				MarkAsSeen(x + buffer->xStart,
					   y + buffer->yStart);
		}
		tile += X_TILES - buffer->width;
	}
}

#define TEST_LOS(x,y,dx,dy)   if (buffer->tiles[y+dy][x+dx].flags & (NO_SEE|shadowFlag)) \
                              buffer->tiles[y][x].flags |= shadowFlag

void LineOfSight(int xc, int yc, struct Buffer *buffer, int shadowFlag)
{
	int x, y;

	xc = xc / TILE_WIDTH - buffer->xStart;
	yc = yc / TILE_HEIGHT - buffer->yStart;

	for (x = xc - 2; x >= 0; x--)
		TEST_LOS(x, yc, 1, 0);
	for (x = xc + 2; x < buffer->width; x++)
		TEST_LOS(x, yc, -1, 0);
	for (y = yc - 1; y >= 0; y--) {
		TEST_LOS(xc, y, 0, 1);
		for (x = xc - 1; x >= 0; x--)
			TEST_LOS(x, y, 1, 1);
		for (x = xc + 1; x < buffer->width; x++)
			TEST_LOS(x, y, -1, 1);
	}
	for (y = yc + 1; y < Y_TILES; y++) {
		TEST_LOS(xc, y, 0, -1);
		for (x = xc - 1; x >= 0; x--)
			TEST_LOS(x, y, 1, -1);
		for (x = xc + 1; x < buffer->width; x++)
			TEST_LOS(x, y, -1, -1);
	}
}


void DrawWallColumn(int y, int xc, int yc, TTile * tile)
{
	while (y >= 0 && (tile->flags & IS_WALL) != 0) {
		DrawPic(xc, yc, gPics[tile->pic],
			gCompiledPics[tile->pic]);
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
	for (y = 0, yc = b->dy; y < Y_TILES - 1; y++, yc += TILE_HEIGHT) {
		for (x = 0, xc = b->dx + xOffset;
		     x < b->width; x++, tile++, xc += TILE_WIDTH) {
			if (tile->pic >= 0
			    && (tile->flags & (IS_WALL | OFFSET_PIC)) == 0)
				DrawPic(xc, yc, gPics[tile->pic],
					gCompiledPics[tile->pic]);
		}
		tile += X_TILES - b->width;
	}

	// Now draw walls and things in proper order
	tile = &b->tiles[0][0];
	yc = b->dy + cWallOffset.dy;
	for (y = 0; y < Y_TILES; y++, yc += TILE_HEIGHT) {
		displayList = NULL;
		xc = b->dx + cWallOffset.dx + xOffset;
		for (x = 0; x < b->width; x++, tile++, xc += TILE_WIDTH) {
			if (tile->flags & IS_WALL) {
				if ((tile->flags & DELAY_DRAW) == 0)
					DrawWallColumn(y, xc, yc, tile);
			} else if ((tile->flags & OFFSET_PIC) != 0) {
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
