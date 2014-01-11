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
#ifndef __MAP
#define __MAP

#include "gamedata.h"
#include "pic.h"
#include "tile.h"
#include "vector.h"

#define YMAX    128
#define XMAX    128

#define MAP_ACCESS_RED      256
#define MAP_ACCESS_BLUE     512
#define MAP_ACCESS_GREEN    1024
#define MAP_ACCESS_YELLOW   2048

typedef struct
{
	Tile tiles[YMAX][XMAX];

	// internal data structure to help build the map
	unsigned short iMap[YMAX][XMAX];

	int tilesSeen;
	int tilesTotal;
	int keyAccessCount;
} Map;

extern Map gMap;

Tile *MapGetTile(Map *map, Vec2i pos);
int MapIsTileIn(Map *map, Vec2i pos);

int MapHasLockedRooms(Map *map);
int MapPosIsHighAccess(Map *map, int x, int y);
int MapGetDoorKeycardFlag(Map *map, Vec2i pos);

void MapMoveTileItem(Map *map, TTileItem *t, Vec2i pos);
void MapRemoveTileItem(Map *map, TTileItem *t);

void MapLoad(Map *map, struct MissionOptions *mo);
int MapIsFullPosOKforPlayer(Map *map, int x, int y);
void MapChangeFloor(Map *map, Vec2i pos, Pic *normal, Pic *shadow);
void MapShowExitArea(Map *map);
void MapMarkAsVisited(Map *map, Vec2i pos);
void MapMarkAllAsVisited(Map *map);
int MapGetExploredPercentage(Map *map);
TTileItem *MapGetClosestEnemy(
	TTileItem *from, Vec2i offset, int flags, int player, int maxRadius);

// Map construction functions
unsigned short IMapGet(Map *map, Vec2i pos);
int MapIsAreaClear(Map *map, Vec2i pos, Vec2i size);
int MapIsValidStartForWall(Map *map, int x, int y);
void MapMakeSquare(Map *map, Vec2i pos, Vec2i size);
void MapMakeRoom(
	Map *map,
	int xOrigin, int yOrigin, int width, int height, int doors[4],
	int doorMin, int doorMax,
	unsigned short access_mask);
void MapMakePillar(Map *map, Vec2i pos, Vec2i size);
void MapMakeWall(Map *map, Vec2i pos);

#endif
