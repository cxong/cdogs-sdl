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
#ifndef __TILE
#define __TILE

#include "c_array.h"
#include "pic.h"
#include "vector.h"

#define TILE_WIDTH      16
#define TILE_HEIGHT     12

#define X_TILES			((gGraphicsDevice.cachedConfig.ResolutionWidth + TILE_WIDTH - 1) / TILE_WIDTH + 1)

#define X_TILES_HALF    ((X_TILES + 1) / 2)

// + 1 because walls from bottom row show up one row above
#define Y_TILES			((gGraphicsDevice.cachedConfig.ResolutionHeight + TILE_HEIGHT - 1) / TILE_HEIGHT + 2)
#define Y_TILES_HALF    ((Y_TILES + 1 / 2)

typedef enum
{
	MAPTILE_NO_WALK			= 0x0001,
	MAPTILE_NO_SEE			= 0x0002,
	MAPTILE_NO_SHOOT		= 0x0004,
	MAPTILE_IS_VISIBLE		= 0x0008,
	MAPTILE_IS_WALL			= 0x0010,
	MAPTILE_IS_NOTHING		= 0x0020,
	MAPTILE_IS_NORMAL_FLOOR	= 0x0040,
	MAPTILE_IS_DRAINAGE		= 0x0080,
	MAPTILE_OFFSET_PIC		= 0x0100,
// These constants are used internally in draw, it is never set in the map
	MAPTILE_DELAY_DRAW		= 0x0200,
	MAPTILE_OUT_OF_SIGHT	= 0x0400
} MapTileFlags;

#define KIND_CHARACTER      0
#define KIND_PIC            1
#define KIND_MOBILEOBJECT   2
#define KIND_OBJECT         3

#define TILEITEM_IMPASSABLE     1
#define TILEITEM_CAN_BE_SHOT    2
#define TILEITEM_CAN_BE_TAKEN   4
#define TILEITEM_OBJECTIVE      (8 + 16 + 32 + 64 + 128)
#define TILEITEM_IS_WRECK		256
#define OBJECTIVE_SHIFT         3


typedef Pic *(*TileItemGetPicFunc)(void *);
typedef void (*TileItemDrawFunc) (int, int, void *);

struct TileItem {
	int x, y;
	int w, h;
	int kind;
	int flags;
	void *data;
	TileItemGetPicFunc getPicFunc;
	TileItemDrawFunc drawFunc;
	void *actor;
	struct TileItem *next;
	struct TileItem *nextToDisplay;
};
typedef struct TileItem TTileItem;


typedef struct
{
	Pic *pic;
	Pic picAlt;
	int flags;
	int isVisited;
	CArray triggers;	// of Trigger *
	TTileItem *things;
} Tile;


Tile TileNone(void);
void TileInit(Tile *t);
void TileDestroy(Tile *t);
int IsTileItemInsideTile(TTileItem *i, Vec2i tilePos);
int TileCanSee(Tile *t);
int TileCanWalk(Tile *t);
int TileIsNormalFloor(Tile *t);
int TileIsClear(Tile *t);
int TileHasCharacter(Tile *t);
void TileSetAlternateFloor(Tile *t, Pic *p);

#endif
