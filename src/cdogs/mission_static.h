/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.
	Copyright (c) 2013-2017, 2019-2021 Cong Xu
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
	const MapObject *M;
	CArray Positions; // of struct vec2i
} MapObjectPositions;
typedef struct
{
	int Index;
	CArray Positions; // of struct vec2i
} CharacterPositions;
typedef struct
{
	struct vec2i Position;
	int Index;
} PositionIndex;
typedef struct
{
	int Index;
	CArray PositionIndices; // of PositionIndex
} ObjectivePositions;
typedef struct
{
	int Index;
	CArray Positions; // of struct vec2i
} KeyPositions;
typedef struct
{
	const PickupClass *P;
	CArray Positions; // of struct vec2i
} PickupPositions;

typedef struct
{
	map_t TileClasses; // of TileClass
	CArray Tiles;	   // of int (tile ids)
	CArray Access;	   // of uint16_t
	CArray Items;	   // of MapObjectPositions
	CArray Characters; // of CharacterPositions
	CArray Objectives; // of ObjectivePositions
	CArray Keys;	   // of KeyPositions
	CArray Pickups;	   // of PickupPositions
	struct vec2i Start;
	CArray Exits; // of Exit
	bool AltFloorsEnabled;
} MissionStatic;

void MissionStaticInit(MissionStatic *m);
bool MissionStaticTryLoadJSON(
	MissionStatic *m, json_t *node, const struct vec2i size, const int version,
	const int mission);
void MissionStaticFromMap(MissionStatic *m, const Map *map);
void MissionStaticTerminate(MissionStatic *m);
void MissionStaticSaveJSON(
	const MissionStatic *m, const struct vec2i size, json_t *node);

void MissionStaticCopy(MissionStatic *dst, const MissionStatic *src);

int MissionStaticGetTile(
	const MissionStatic *m, const struct vec2i size, const struct vec2i pos);
const TileClass *MissionStaticGetTileClass(
	const MissionStatic *m, const struct vec2i size, const struct vec2i pos);
TileClass *MissionStaticIdTileClass(const MissionStatic *m, const int tile);
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
void MissionStaticAddCharacter(MissionStatic *m, const int ch, const struct vec2i pos);
bool MissionStaticTryRemoveCharacterAt(
	MissionStatic *m, const struct vec2i pos);
void MissionStaticAddKey(
	MissionStatic *m, const int k, const struct vec2i pos);
bool MissionStaticTryRemoveKeyAt(MissionStatic *m, const struct vec2i pos);
bool MissionStaticTryAddPickup(
	MissionStatic *m, const PickupClass *p, const struct vec2i pos);
bool MissionStaticTryRemovePickupAt(MissionStatic *m, const struct vec2i pos);
bool MissionStaticTrySetKey(
	MissionStatic *m, const int k, const struct vec2i size,
	const struct vec2i pos);
bool MissionStaticTryUnsetKeyAt(
	MissionStatic *m, const struct vec2i size, const struct vec2i pos);

bool MissionStaticTryAddExit(MissionStatic *m, const Exit *exit);

any_t TileClassCopyHashMap(any_t in);
