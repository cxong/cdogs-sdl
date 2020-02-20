/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (c) 2013-2017, 2019-2020 Cong Xu
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

#include "c_array.h"
#include "c_hashmap/hashmap.h"
#include "json_utils.h"
#include "map.h"
#include "map_object.h"
#include "mathc/mathc.h"
#include "tile_class.h"

typedef struct
{
	map_t TileClasses;	// of TileClass
	CArray Tiles;		// of int (tile ids)
	CArray Access;		// of int
	CArray Items;		// of MapObjectPositions
	CArray Characters;	// of CharacterPositions
	CArray Objectives;	// of ObjectivePositions
	CArray Keys;		// of KeyPositions
	struct vec2i Start;
	struct
	{
		struct vec2i Start;
		struct vec2i End;
	} Exit;
} MissionStatic;

bool MissionStaticTryLoadJSON(
	MissionStatic *m, json_t *node, const int version);
void MissionStaticFromMap(MissionStatic *m, const Map *map);
void MissionStaticTerminate(MissionStatic *m);
void MissionStaticSaveJSON(
	const MissionStatic *m, const struct vec2i size, json_t *node);

void MissionStaticCopy(MissionStatic *dst, const MissionStatic *src);

int MissionStaticGetTile(
	const MissionStatic *m, const struct vec2i size, const struct vec2i pos);
const TileClass *MissionStaticGetTileClass(
	const MissionStatic *m, const struct vec2i size, const struct vec2i pos);
const TileClass *MissionStaticIdTileClass(
	const MissionStatic *m, const int tile);
TileClass *MissionStaticAddTileClass(MissionStatic *m, const TileClass *base);
bool MissionStaticRemoveTileClass(MissionStatic *m, const int tile);
bool MissionStaticTrySetTile(
	MissionStatic *m, const struct vec2i size, const struct vec2i pos,
	const int tile);
void MissionStaticClearTile(
	MissionStatic *m, const struct vec2i size, const struct vec2i pos);

void MissionStaticLayout(
	MissionStatic *m, const struct vec2i size, const struct vec2i oldSize);
bool MissionStaticTryAddItem(
	MissionStatic *m, const MapObject *mo, const struct vec2i pos);
bool MissionStaticTryRemoveItemAt(MissionStatic *m, const struct vec2i pos);
bool MissionStaticTryAddCharacter(
	MissionStatic *m, const int ch, const struct vec2i pos);
bool MissionStaticTryRemoveCharacterAt(
	MissionStatic *m, const struct vec2i pos);
bool MissionStaticTryAddObjective(
	MissionStatic *m, const int idx, const int idx2, const struct vec2i pos);
bool MissionStaticTryRemoveObjectiveAt(
	MissionStatic *m, const struct vec2i pos);
bool MissionStaticTryAddKey(
	MissionStatic *m, const int k, const struct vec2i pos);
bool MissionStaticTryRemoveKeyAt(MissionStatic *m, const struct vec2i pos);
bool MissionStaticTrySetKey(
	MissionStatic *m, const int k,
	const struct vec2i size, const struct vec2i pos);
bool MissionStaticTryUnsetKeyAt(
	MissionStatic *m, const struct vec2i size, const struct vec2i pos);
