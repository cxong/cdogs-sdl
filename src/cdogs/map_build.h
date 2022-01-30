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

	Copyright (c) 2013-2015, 2017-2022 Cong Xu
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

#include "game_mode.h"
#include "map.h"
#include "mission.h"

typedef struct
{
	Map *Map;
	const Mission *mission;
	GameMode mode;
	const CharacterStore *characters;

	// internal data structures to help build the map
	CArray access;	  // of uint16_t
	CArray tiles;	  // of TileClass
	CArray leaveFree; // of bool
} MapBuilder;

void MapBuild(
	Map *m, const Mission *mission, const bool loadDynamic, const int missionIndex, const GameMode mode, const CharacterStore *characters);
void MapBuilderInit(
	MapBuilder *mb, Map *m, const Mission *mission, const GameMode mode, const CharacterStore *characters);
void MapBuilderTerminate(MapBuilder *mb);

void MapLoadDynamic(MapBuilder *mb);

uint16_t MapBuildGetAccess(const MapBuilder *mb, const struct vec2i pos);
void MapBuildSetAccess(MapBuilder *mb, struct vec2i pos, const uint16_t v);
const TileClass *MapBuilderGetTile(
	const MapBuilder *mb, const struct vec2i pos);
void MapBuilderSetTile(MapBuilder *mb, struct vec2i pos, const TileClass *t);

// Mark a tile so that it is left free of other map objects
void MapBuilderSetLeaveFree(
	MapBuilder *mb, const struct vec2i tile, const bool value);
bool MapBuilderIsLeaveFree(const MapBuilder *mb, const struct vec2i tile);

bool MapTryPlaceOneObject(
	MapBuilder *mb, const struct vec2i v, const MapObject *mo,
	const int extraFlags, const bool isStrictMode);
// TODO: refactor
void MapPlaceCollectible(
	const Mission *m, const int objective, const struct vec2 pos);
// TODO: refactor
void MapPlaceKey(
	MapBuilder *mb, const struct vec2i tilePos, const int keyIndex);
bool MapPlaceRandomTile(
	MapBuilder *mb, const PlacementAccessFlags paFlags,
	bool (*tryPlaceFunc)(MapBuilder *, const struct vec2i, void *),
	void *data);

bool MapIsAreaInside(
	const Map *map, const struct vec2i pos, const struct vec2i size);
bool MapBuilderIsAreaFunc(
	const MapBuilder *mb, const struct vec2i pos, const struct vec2i size,
	bool (*func)(const MapBuilder *, const struct vec2i));
bool MapIsAreaClear(
	const MapBuilder *mb, const struct vec2i pos, const struct vec2i size);
bool MapIsAreaClearOrRoom(
	const MapBuilder *mb, const struct vec2i pos, const struct vec2i size);
bool MapIsAreaClearOrWall(
	const MapBuilder *mb, struct vec2i pos, struct vec2i size);
bool MapGetRoomOverlapSize(
	const MapBuilder *mb, const Rect2i r, uint16_t *overlapAccess);
bool MapIsLessThanTwoWallOverlaps(
	const MapBuilder *mb, struct vec2i pos, struct vec2i size);
void MapFillRect(MapBuilder *mb, const Rect2i r, const TileClass *edge, const TileClass *fill);
struct vec2i MapGetRoomSize(const RoomParams r, const int doorMin);
void MapMakeRoom(
	MapBuilder *mb, const struct vec2i pos, const struct vec2i size,
	const bool walls, const TileClass *wall, const TileClass *room,
	const bool removeInterRoomWalls);
void MapMakeRoomWalls(
	MapBuilder *mb, const RoomParams r, const TileClass *wall, const Rect2i room);
bool MapTryBuildWall(
	MapBuilder *mb, const bool isRoom, const int pad, const int wallLength,
	const TileClass *wall, const Rect2i r);
void MapSetRoomAccessMask(
	MapBuilder *mb, const Rect2i r, const uint16_t accessMask);
void MapSetRoomAccessMaskOverlap(
	MapBuilder *mb, CArray *rooms, const uint16_t accessMask);
void MapPlaceDoors(
	MapBuilder *mb, const Rect2i r, const bool hasDoors, const bool doors[4],
	const int doorMin, const int doorMax, const uint16_t accessMask,
	const bool randomPos,
	const TileClass *door, const TileClass *floor);

void MapBuildTile(
	MapBuilder *mb, const struct vec2i pos, const TileClass *tile);

uint16_t GenerateAccessMask(int *accessLevel);

void SetupWallTileClasses(Map *m, PicManager *pm, const TileClass *base);
void SetupFloorTileClasses(Map *m, PicManager *pm, const TileClass *base);
void SetupDoorTileClasses(Map *m, PicManager *pm, const TileClass *base);
