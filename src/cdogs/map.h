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
#ifndef __MAP
#define __MAP

#include <stdbool.h>

#include "map_object.h"
#include "mission.h"
#include "pic.h"
#include "tile.h"
#include "vector.h"

// Values for internal map
typedef enum
{
	MAP_FLOOR,
	MAP_WALL,
	MAP_DOOR,
	MAP_ROOM,
	MAP_NOTHING,
	MAP_SQUARE,
	MAP_UNSET	// internal use by editor
} IMapType;
const char *IMapTypeStr(IMapType t);
IMapType StrIMapType(const char *s);

#define MAP_ACCESS_RED      0x800
#define MAP_ACCESS_BLUE     0x400
#define MAP_ACCESS_GREEN    0x200
#define MAP_ACCESS_YELLOW   0x100

#define MAP_MASKACCESS      0xFF
#define MAP_ACCESSBITS      0x0F00

#define MAP_LEAVEFREE       4096

typedef struct
{
	CArray Tiles;	// of Tile
	Vec2i Size;

	// internal data structure to help build the map
	CArray iMap;	// of unsigned short

	CArray triggers;	// of Trigger *; owner
	int triggerId;

	int tilesSeen;
	int keyAccessCount;
	
	Vec2i ExitStart;
	Vec2i ExitEnd;

	int NumExplorableTiles;
} Map;

extern Map gMap;

unsigned short GetAccessMask(int k);

Tile *MapGetTile(Map *map, Vec2i pos);
int MapIsTileIn(Map *map, Vec2i pos);
bool MapIsTileInExit(Map *map, TTileItem *t);

int MapHasLockedRooms(Map *map);
int MapPosIsHighAccess(Map *map, int x, int y);
int MapGetDoorKeycardFlag(Map *map, Vec2i pos);

void MapMoveTileItem(Map *map, TTileItem *t, Vec2i pos);
void MapRemoveTileItem(Map *map, TTileItem *t);

void MapInit(Map *map);
void MapTerminate(Map *map);
void MapLoad(Map *map, struct MissionOptions *mo, CharacterStore *store);
bool MapIsFullPosOKforPlayer(Map *map, Vec2i pos, bool allowAllTiles);
void MapChangeFloor(Map *map, Vec2i pos, Pic *normal, Pic *shadow);
void MapShowExitArea(Map *map);
// Returns the center of the tile that's the middle of the exit area
// Used for compass arrows
Vec2i MapGetExitPos(Map *m);
void MapMarkAsVisited(Map *map, Vec2i pos);
void MapMarkAllAsVisited(Map *map);
int MapGetExploredPercentage(Map *map);
TTileItem *MapGetClosestEnemy(
	TTileItem *from, Vec2i offset, int flags, int player, int maxRadius);

// Map construction functions
unsigned short IMapGet(Map *map, Vec2i pos);
void IMapSet(Map *map, Vec2i pos, unsigned short v);
Vec2i MapGenerateFreePosition(Map *map, Vec2i size);
int MapTryPlaceOneObject(
	Map *map, Vec2i v, MapObject *mo, int extraFlags, int isStrictMode);
void MapPlaceWreck(Map *map, Vec2i v, MapObject *mo);
void MapPlaceCollectible(
	struct MissionOptions *mo, int objective, Vec2i realPos);
void MapPlaceHealth(Vec2i pos);
void MapPlaceKey(Map *map, struct MissionOptions *mo, Vec2i pos, int keyIndex);

#endif
