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

    Copyright (c) 2013-2015, 2017 Cong Xu
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

#include "map.h"

bool MapIsAreaInside(const Map *map, const struct vec2i pos, const struct vec2i size);
bool MapIsAreaClear(const Map *map, const struct vec2i pos, const struct vec2i size);
bool MapIsAreaClearOrRoom(const Map *map, const struct vec2i pos, const struct vec2i size);
int MapIsAreaClearOrWall(Map *map, struct vec2i pos, struct vec2i size);
bool MapGetRoomOverlapSize(
	const Map *map,
	const struct vec2i pos,
	const struct vec2i size,
	unsigned short *overlapAccess);
int MapIsLessThanTwoWallOverlaps(Map *map, struct vec2i pos, struct vec2i size);
int MapIsValidStartForWall(
	Map *map, int x, int y, unsigned short tileType, int pad);
void MapMakeSquare(Map *map, struct vec2i pos, struct vec2i size);
struct vec2i MapGetRoomSize(const RoomParams r, const int doorMin);
void MapMakeRoom(Map *map, const struct vec2i pos, const struct vec2i size, const bool walls);
void MapMakeRoomWalls(Map *map, const RoomParams r);
bool MapTryBuildWall(
	Map *map, const unsigned short tileType, const int pad,
	const int wallLength);
void MapSetRoomAccessMask(
	Map *map, const struct vec2i pos, const struct vec2i size,
	const unsigned short accessMask);
void MapSetRoomAccessMaskOverlap(
	Map *map, CArray *rooms, const unsigned short accessMask);
void MapPlaceDoors(
	Map *map, struct vec2i pos, struct vec2i size,
	int hasDoors, int doors[4], int doorMin, int doorMax,
	unsigned short accessMask);
void MapMakePillar(Map *map, struct vec2i pos, struct vec2i size);
void MapSetTile(Map *map, struct vec2i pos, unsigned short tileType, Mission *m);

void MapSetupTilesAndWalls(Map *map, const Mission *m);

unsigned short GenerateAccessMask(int *accessLevel);
void MapGenerateRandomExitArea(Map *map);
