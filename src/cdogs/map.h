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

	Copyright (c) 2013-2022 Cong Xu
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
#pragma once

#include <stdbool.h>

#include "map_object.h"
#include "pic.h"
#include "thing.h"
#include "tile.h"
#include "triggers.h"
#include "vector.h"

// Values for internal map
typedef enum
{
	MAP_FLOOR,
	MAP_WALL,
	MAP_DOOR,
	MAP_ROOM,
	MAP_NOTHING,
	MAP_SQUARE
} IMapType;
const char *IMapTypeStr(IMapType t);
IMapType StrIMapType(const char *s);

#define MAP_ACCESS_RED 0x800
#define MAP_ACCESS_BLUE 0x400
#define MAP_ACCESS_GREEN 0x200
#define MAP_ACCESS_YELLOW 0x100

#define MAP_MASKACCESS 0xFF
#define MAP_ACCESSBITS 0x0F00

typedef struct
{
	// Array of bools to set lines of sight
	CArray LOS; // of bool

	// Array of bools for tracking new tiles in line of sight, for delayed
	// messaging
	CArray Explored; // of bool
} LineOfSight;

typedef struct
{
	Rect2i R;
	int Mission;
	bool Hidden;
} Exit;

typedef struct
{
	map_t TileClasses;
	CArray Tiles; // of Tile
	struct vec2i Size;

	LineOfSight LOS;
	CArray access; // of uint16_t

	CArray triggers; // of Trigger *; owner
	int triggerId;

	int tilesSeen;
	int keyAccessCount;

	struct vec2i start;
	CArray exits; // of Exit

	int NumExplorableTiles;
} Map;

extern Map gMap;

uint16_t GetAccessMask(const int k);

Tile *MapGetTile(const Map *map, const struct vec2i pos);
bool MapIsTileIn(const Map *map, const struct vec2i pos);
int MapIsTileInExit(const Map *map, const Thing *ti, const int exit);

// TODO: remove this function
uint16_t MapGetAccessLevel(const Map *map, const struct vec2i pos);
uint16_t AccessCodeToFlags(const uint16_t code);
bool MapHasLockedRooms(const Map *map);
bool MapPosIsInLockedRoom(const Map *map, const struct vec2 pos);
int MapGetDoorKeycardFlag(Map *map, struct vec2i pos);

// Return false if cannot move to new position
bool MapTryMoveThing(Map *map, Thing *t, const struct vec2 pos);
void MapRemoveThing(Map *map, Thing *t);

void MapTerminate(Map *map);
void MapInit(Map *map, const struct vec2i size);
void MapPrintDebug(const Map *m);
bool MapIsPosOKForPlayer(
	const Map *map, const struct vec2 pos, const bool allowAllTiles);
bool MapIsTileAreaClear(
	const Map *map, const struct vec2 pos, const struct vec2i size);
bool MapHasExits(const Map *m);
void MapShowExitArea(Map *map, const int i);
// Returns the center of the tile that's the middle of the exit area
struct vec2 MapGetExitPos(const Map *m, const int i);
struct vec2i MapGetRandomTile(const Map *map);
struct vec2 MapGetRandomPos(const Map *map);
bool MapPlaceRandomPos(
	const Map *map, const PlacementAccessFlags paFlags,
	bool (*tryPlaceFunc)(const Map *, const struct vec2, void *), void *data);

void MapMarkAsVisited(Map *map, struct vec2i pos);
void MapMarkAllAsVisited(Map *map);
int MapGetExploredPercentage(Map *map);
void MapUpdate(Map *map);

typedef bool (*TileSelectFunc)(Map *, struct vec2i);
// Find a tile around the start that satisfies a condition
struct vec2i MapSearchTileAround(
	Map *map, struct vec2i start, TileSelectFunc func);
bool MapTileIsUnexplored(Map *map, struct vec2i tile);

// Map construction functions
struct vec2 MapGenerateFreePosition(Map *map, const struct vec2i size);

Trigger *MapNewTrigger(Map *map);
