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

    Copyright (c) 2013-2019, 2021 Cong Xu
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

#include <json/json.h>
#include "ammo.h"
#include "character.h"
#include "pic_manager.h"
#include "pickup_class.h"

typedef enum
{
	PLACEMENT_NONE = 0,
	PLACEMENT_OUTSIDE,
	PLACEMENT_INSIDE,
	PLACEMENT_NO_WALLS,
	PLACEMENT_ONE_WALL,
	PLACEMENT_ONE_OR_MORE_WALLS,
	PLACEMENT_FREE_IN_FRONT,
	PLACEMENT_ON_WALL,

	PLACEMENT_COUNT
} PlacementFlags;
const char *PlacementFlagStr(const int i);

typedef enum
{
	MAP_OBJECT_TYPE_NORMAL,
	MAP_OBJECT_TYPE_PICKUP_SPAWNER,
	MAP_OBJECT_TYPE_ACTOR_SPAWNER
} MapObjectType;

// Pickups to spawn when map objects are destroyed
typedef struct
{
	PickupType Type;
	double SpawnChance;
} MapObjectDestroySpawn;

// A static map object, taking up an entire tile
typedef struct
{
	char *Name;
	CPic Pic;
	struct vec2i Offset;
	struct
	{
		char *MO;
		Mix_Chunk *Sound;
		char *Bullet;
	} Wreck;
	struct vec2i Size;
	struct vec2 PosOffset;
	int Health;
	// Guns that are fired when this map object is destroyed
	// i.e. explosion on destruction
	CArray DestroyGuns;	// of const WeaponClass *
	// Bit field composed of bits shifted by PlacementFlags
	int Flags;
	bool DrawBelow;
	bool DrawAbove;
	char *FootstepSound;
	color_t FootprintMask;
	MapObjectType Type;
	union
	{
		const PickupClass *PickupClass;
		struct
		{
			int CharId;
			int Counter;
		} Character;
	} u;
	CArray DestroySpawn;	// of MapObjectDestroySpawn
	struct {
		float HealthThreshold;	// Smoke if map object damaged below this ratio
	} DamageSmoke;
} MapObject;
typedef struct
{
	CArray Classes;	// of MapObject
	CArray CustomClasses;	// of MapObject
	// Names of special types of map objects; for editor support
	// Reset on load
	CArray Destructibles;	// of char *
	// Map objects that match "blood%d" - left over when actors die
	CArray Bloods;	// of char *
} MapObjects;
extern MapObjects gMapObjects;

MapObject *StrMapObject(const char *s);
// Legacy map objects, integer based
MapObject *IntMapObject(const int m);
// Get map object by index; used by editor
MapObject *IndexMapObject(const int i);
// Get index of destructible map object; used by editor
int DestructibleMapObjectIndex(const MapObject *mo);
const MapObject *GetRandomBloodPool(void);
int MapObjectGetFlags(const MapObject *mo);

void MapObjectsInit(
	MapObjects *classes, const char *filename,
	const AmmoClasses *ammo, const WeaponClasses *guns);
void MapObjectsLoadJSON(CArray *classes, json_t *root);
void MapObjectsLoadAmmoAndGunSpawners(
	MapObjects *classes, const AmmoClasses *ammo, const WeaponClasses *guns,
	const bool isCustom);
void MapObjectsClear(CArray *classes);
void MapObjectsTerminate(MapObjects *classes);
int MapObjectsCount(const MapObjects *classes);

const Pic *MapObjectGetPic(const MapObject *mo, struct vec2i *offset);

bool MapObjectIsTileOK(
	const MapObject *obj, const Tile *tile, const Tile *tileAbove);
struct vec2 MapObjectGetPlacementPos(const MapObject *mo, const struct vec2i tilePos);
bool MapObjectIsOnWall(const MapObject *mo);
