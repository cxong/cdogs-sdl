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
*/
#ifndef __MAP
#define __MAP

#include "grafx.h" // for SCREEN_WIDTH etc

#define YMAX    64
#define XMAX    64

#define TILE_WIDTH      16
#define TILE_HEIGHT     12

//#define X_TILES         21
#define X_TILES			(SCREEN_WIDTH / TILE_WIDTH + 2)

#define X_TILES_HALF    ((X_TILES / 2) + 1)
//#define Y_TILES         19
#define Y_TILES			(SCREEN_HEIGHT / TILE_HEIGHT + 2)

#define NO_WALK           1
#define NO_SEE            2
#define NO_SHOOT          4
#define IS_SHADOW         8
#define IS_WALL          16
#define OFFSET_PIC       32
#define IS_SHADOW2       64
#define TILE_TRIGGER    128

// This constant is used internally in draw, it is never set in the map
#define DELAY_DRAW      256

#define KIND_CHARACTER      0
#define KIND_PIC            1
#define KIND_MOBILEOBJECT   2
#define KIND_OBJECT         3

#define TILEITEM_IMPASSABLE     1
#define TILEITEM_CAN_BE_SHOT    2
#define TILEITEM_CAN_BE_TAKEN   4
#define TILEITEM_OBJECTIVE      (8 + 16 + 32 + 64 + 128)
#define OBJECTIVE_SHIFT         3


typedef void (*TileItemDrawFunc) (int, int, void *);

struct TileItem {
	int x, y;
	int w, h;
	int kind;
	int flags;
	void *data;
	TileItemDrawFunc drawFunc;
	struct TileItem *next;
	struct TileItem *nextToDisplay;
};
typedef struct TileItem TTileItem;


struct Tile {
	int pic;
	int flags;
	TTileItem *things;
};
typedef struct Tile TTile;


struct Buffer {
	int xTop, yTop;
	int xStart, yStart;
	int dx, dy;
	int width;
	//TTile tiles[Y_TILES][X_TILES];
	TTile **tiles; // we're dynamic now
};

struct Buffer * NewBuffer(void);
void ClearBuffer(struct Buffer *b);

extern TTile gMap[YMAX][XMAX];
#define Map( x, y)  gMap[y][x]
#define HitWall(x,y) ((gMap[(y)/TILE_HEIGHT][(x)/TILE_WIDTH].flags & NO_WALK) != 0)

extern unsigned char gAutoMap[YMAX][XMAX];
#define AutoMap( x, y)  gAutoMap[y][x]

int CheckWall(int x, int y, int w, int h);
int HasHighAccess(void);
int IsHighAccess(int x, int y);
int MapAccessLevel(int x, int y);

void MoveTileItem(TTileItem * t, int x, int y);
void RemoveTileItem(TTileItem * t);
int ItemsCollide(TTileItem * item1, TTileItem * item2, int x, int y);
TTileItem *CheckTileItemCollision(TTileItem * item, int x, int y,
				  int mask);

void SetupMap(void);
int OKforPlayer(int x, int y);
void ChangeFloor(int x, int y, int normal, int shadow);
void MarkAsSeen(int x, int y);
int ExploredPercentage(void);

#endif
