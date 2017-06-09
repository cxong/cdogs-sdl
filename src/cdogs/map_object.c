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

    Copyright (c) 2014, 2016-2017 Cong Xu
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
#include "map_object.h"

#include "json_utils.h"
#include "log.h"
#include "map.h"
#include "pics.h"


MapObjects gMapObjects;


const char *PlacementFlagStr(const int i)
{
	switch (i)
	{
		T2S(PLACEMENT_OUTSIDE, "Outside");
		T2S(PLACEMENT_INSIDE, "Inside");
		T2S(PLACEMENT_NO_WALLS, "NoWalls");
		T2S(PLACEMENT_ONE_WALL, "OneWall");
		T2S(PLACEMENT_ONE_OR_MORE_WALLS, "OneOrMoreWalls");
		T2S(PLACEMENT_FREE_IN_FRONT, "FreeInFront");
		T2S(PLACEMENT_ON_WALL, "OnWall");
	default:
		return "";
	}
}
static PlacementFlags StrPlacementFlag(const char *s)
{
	S2T(PLACEMENT_OUTSIDE, "Outside");
	S2T(PLACEMENT_INSIDE, "Inside");
	S2T(PLACEMENT_NO_WALLS, "NoWalls");
	S2T(PLACEMENT_ONE_WALL, "OneWall");
	S2T(PLACEMENT_ONE_OR_MORE_WALLS, "OneOrMoreWalls");
	S2T(PLACEMENT_FREE_IN_FRONT, "FreeInFront");
	S2T(PLACEMENT_ON_WALL, "OnWall");
	return PLACEMENT_NONE;
}

static MapObjectType StrMapObjectType(const char *s)
{
	S2T(MAP_OBJECT_TYPE_NORMAL, "Normal");
	S2T(MAP_OBJECT_TYPE_PICKUP_SPAWNER, "PickupSpawner");
	CASSERT(false, "unknown map object name");
	return MAP_OBJECT_TYPE_NORMAL;
}

MapObject *StrMapObject(const char *s)
{
	if (s == NULL || strlen(s) == 0)
	{
		return NULL;
	}
	CA_FOREACH(MapObject, c, gMapObjects.CustomClasses)
		if (strcmp(s, c->Name) == 0)
		{
			return c;
		}
	CA_FOREACH_END()
	CA_FOREACH(MapObject, c, gMapObjects.Classes)
		if (strcmp(s, c->Name) == 0)
		{
			return c;
		}
	CA_FOREACH_END()
	return NULL;
}
MapObject *IntMapObject(const int m)
{
	// Note: do not edit; legacy integer mapping
	static const char *oldMapObjects[] =
	{
		"barrel_blue",
		"box",
		"box2",
		"cabinet",
		"plant",
		"bench",
		"chair",
		"column",
		"barrel_skull",
		"barrel_wood",
		"box_gray",
		"box_green",
		"statue_ogre",
		"table_wood_candle",
		"table_wood",
		"tree_dead",
		"bookshelf",
		"box_wood",
		"table_clothed",
		"table_steel",
		"tree_autumn",
		"tree",
		"box_metal_green",
		"safe",
		"box_red",
		"table_lab",
		"terminal",
		"barrel",
		"rocket",
		"egg",
		"bloodstain",
		"wall_skull",
		"bone_blood",
		"bulletmarks",
		"skull",
		"blood0",
		"scratch",
		"wall_stuff",
		"wall_goo",
		"goo"
	};
	if (m < 0 || m > (int)(sizeof oldMapObjects / sizeof oldMapObjects[0]))
	{
		LOG(LM_MAIN, LL_ERROR, "cannot find map object %d", m);
		return NULL;
	}
	return StrMapObject(oldMapObjects[m]);
}
MapObject *IndexMapObject(const int i)
{
	CASSERT(
		i >= 0 &&
		i < (int)gMapObjects.Classes.size + (int)gMapObjects.CustomClasses.size,
		"Map object index out of bounds");
	if (i < (int)gMapObjects.Classes.size)
	{
		return CArrayGet(&gMapObjects.Classes, i);
	}
	return CArrayGet(&gMapObjects.CustomClasses, i - gMapObjects.Classes.size);
}
int DestructibleMapObjectIndex(const MapObject *mo)
{
	if (mo == NULL)
	{
		return 0;
	}
	CA_FOREACH(const char *, name, gMapObjects.Destructibles)
		const MapObject *d = StrMapObject(*name);
		if (d == mo)
		{
			return _ca_index;
		}
	CA_FOREACH_END()
	CASSERT(false, "cannot find destructible map object");
	return -1;
}
MapObject *RandomBloodMapObject(const MapObjects *mo)
{
	const int idx = rand() % (int)mo->Bloods.size;
	const char **name = CArrayGet(&mo->Bloods, idx);
	return StrMapObject(*name);
}
int MapObjectGetFlags(const MapObject *mo)
{
	int flags = 0;
	if (mo->DrawLast)
	{
		flags |= TILEITEM_DRAW_LAST;
	}
	if (mo->Health > 0 && !(mo->Flags & (1 << PLACEMENT_ON_WALL)))
	{
		flags |= TILEITEM_IMPASSABLE;
		flags |= TILEITEM_CAN_BE_SHOT;
	}
	return flags;
}

#define VERSION 3

void MapObjectsInit(
	MapObjects *classes, const char *filename,
	const AmmoClasses *ammo, const GunClasses *guns)
{
	CArrayInit(&classes->Classes, sizeof(MapObject));
	CArrayInit(&classes->CustomClasses, sizeof(MapObject));
	CArrayInit(&classes->Destructibles, sizeof(char *));
	CArrayInit(&classes->Bloods, sizeof(char *));

	char buf[CDOGS_PATH_MAX];
	GetDataFilePath(buf, filename);
	FILE *f = fopen(buf, "r");
	json_t *root = NULL;
	if (f == NULL)
	{
		LOG(LM_MAIN, LL_ERROR, "Error: cannot load map objects file %s", buf);
		goto bail;
	}
	enum json_error e = json_stream_parse(f, &root);
	if (e != JSON_OK)
	{
		LOG(LM_MAIN, LL_ERROR, "Error parsing map objects file %s", buf);
		goto bail;
	}
	MapObjectsLoadJSON(&classes->Classes, root);

	// Load initial ammo/weapon spawners
	MapObjectsLoadAmmoAndGunSpawners(classes, ammo, guns, false);

bail:
	if (f != NULL)
	{
		fclose(f);
	}
	json_free_value(&root);
}
static bool TryLoadMapObject(MapObject *m, json_t *node, const int version);
static void ReloadDestructibles(MapObjects *mo);
void MapObjectsLoadJSON(CArray *classes, json_t *root)
{
	int version;
	LoadInt(&version, root, "Version");
	if (version > VERSION || version <= 0)
	{
		CASSERT(false, "cannot read map objects file version");
		return;
	}

	json_t *pickupsNode = json_find_first_label(root, "MapObjects")->child;
	for (json_t *child = pickupsNode->child; child; child = child->next)
	{
		MapObject m;
		if (TryLoadMapObject(&m, child, version))
		{
			CArrayPushBack(classes, &m);
		}
	}

	ReloadDestructibles(&gMapObjects);
	// Load blood objects
	CArrayClear(&gMapObjects.Bloods);
	for (int i = 0;; i++)
	{
		char buf[CDOGS_FILENAME_MAX];
		sprintf(buf, "blood%d", i);
		if (StrMapObject(buf) == NULL)
		{
			break;
		}
		char *tmp;
		CSTRDUP(tmp, buf);
		CArrayPushBack(&gMapObjects.Bloods, &tmp);
	}
}
static bool TryLoadMapObject(MapObject *m, json_t *node, const int version)
{
	memset(m, 0, sizeof *m);

	m->Name = GetString(node, "Name");

	// Pic
	json_t *normalNode = json_find_first_label(node, "Pic");
	if (normalNode != NULL && normalNode->child != NULL)
	{
		if (version < 2)
		{
			CPicLoadNormal(&m->Pic, normalNode->child);
		}
		else
		{
			CPicLoadJSON(&m->Pic, normalNode->child);
		}
		// Pic required for map object
		if (!CPicIsLoaded(&m->Pic))
		{
			LOG(LM_MAIN, LL_ERROR, "pic not found for map object(%s)", m->Name);
			goto bail;
		}
	}
	if (CPicIsLoaded(&m->Pic))
	{
		// Default offset: centered X, align bottom of tile and sprite
		const Vec2i size = CPicGetSize(&m->Pic);
		m->Offset = Vec2iNew(-size.x / 2, TILE_HEIGHT / 2 - size.y);
		LoadVec2i(&m->Offset, node, "Offset");
	}

	// Wreck
	if (version < 3)
	{
		// Assume old wreck pic is wreck
		json_t *wreckNode = json_find_first_label(node, "WreckPic");
		if (wreckNode != NULL && wreckNode->child != NULL)
		{
			if (version < 2)
			{
				LoadStr(&m->Wreck, node, "WreckPic");
			}
			else
			{
				LoadStr(&m->Wreck, wreckNode, "Pic");
			}
		}
	}
	else
	{
		LoadStr(&m->Wreck, node, "Wreck");
	}

	// Default tile size
	m->Size = TILE_SIZE;
	LoadVec2i(&m->Size, node, "Size");
	LoadVec2i(&m->PosOffset, node, "PosOffset");
	LoadInt(&m->Health, node, "Health");
	LoadBulletGuns(&m->DestroyGuns, node, "DestroyGuns");

	// Flags
	json_t *flagsNode = json_find_first_label(node, "Flags");
	if (flagsNode != NULL && flagsNode->child != NULL)
	{
		for (json_t *flagNode = flagsNode->child->child;
			flagNode;
			flagNode = flagNode->next)
		{
			m->Flags |= 1 << StrPlacementFlag(flagNode->text);
		}
	}

	LoadBool(&m->DrawLast, node, "DrawLast");

	// Special types
	JSON_UTILS_LOAD_ENUM(m->Type, node, "Type", StrMapObjectType);
	switch (m->Type)
	{
	case MAP_OBJECT_TYPE_NORMAL:
		// Do nothing
		break;
	case MAP_OBJECT_TYPE_PICKUP_SPAWNER:
		{
			char *tmp = GetString(node, "Pickup");
			m->u.PickupClass = StrPickupClass(tmp);
			CFREE(tmp);
		}
		break;
	default:
		CASSERT(false, "unknown error");
		break;
	}

	// DestroySpawn - pickups to spawn on destruction
	json_t *destroySpawnNode = json_find_first_label(node, "DestroySpawn");
	if (destroySpawnNode != NULL && destroySpawnNode->child != NULL)
	{
		CArrayInit(&m->DestroySpawn, sizeof(MapObjectDestroySpawn));
		for (json_t *dsNode = destroySpawnNode->child->child;
			dsNode;
			dsNode = dsNode->next)
		{
			MapObjectDestroySpawn mods;
			memset(&mods, 0, sizeof mods);
			JSON_UTILS_LOAD_ENUM(mods.Type, dsNode, "Type", StrPickupType);
			LoadDouble(&mods.SpawnChance, dsNode, "SpawnChance");
			CArrayPushBack(&m->DestroySpawn, &mods);
		}
	}

	return true;

bail:
	return false;
}
static void AddDestructibles(MapObjects *mo, const CArray *classes);
static void ReloadDestructibles(MapObjects *mo)
{
	CA_FOREACH(char *, s, mo->Destructibles)
		CFREE(*s);
	CA_FOREACH_END()
	CArrayClear(&mo->Destructibles);
	AddDestructibles(mo, &mo->Classes);
	AddDestructibles(mo, &mo->CustomClasses);
}
static void AddDestructibles(MapObjects *m, const CArray *classes)
{
	for (int i = 0; i < (int)classes->size; i++)
	{
		const MapObject *mo = CArrayGet(classes, i);
		if (mo->Health > 0)
		{
			char *s;
			CSTRDUP(s, mo->Name);
			CArrayPushBack(&m->Destructibles, &s);
		}
	}
}

static void LoadAmmoSpawners(CArray *classes, const CArray *ammo);
static void LoadGunSpawners(CArray *classes, const CArray *guns);
void MapObjectsLoadAmmoAndGunSpawners(
	MapObjects *classes, const AmmoClasses *ammo, const GunClasses *guns,
	const bool isCustom)
{
	if (isCustom)
	{
		LoadAmmoSpawners(&classes->CustomClasses, &ammo->CustomAmmo);
		LoadGunSpawners(&classes->CustomClasses, &guns->CustomGuns);
	}
	else
	{
		// Load built-in classes
		LoadAmmoSpawners(&classes->Classes, &ammo->Ammo);
		LoadGunSpawners(&classes->Classes, &guns->Guns);
	}
}

static void SetupSpawner(
	MapObject *m, const char *spawnerName, const char *pickupClassName);

static void LoadAmmoSpawners(CArray *classes, const CArray *ammo)
{
	for (int i = 0; i < (int)ammo->size; i++)
	{
		const Ammo *a = CArrayGet(ammo, i);
		MapObject m;
		char spawnerName[256];
		char pickupClassName[256];
		sprintf(spawnerName, "%s ammo spawner", a->Name);
		sprintf(pickupClassName, "ammo_%s", a->Name);
		SetupSpawner(&m, spawnerName, pickupClassName);
		CArrayPushBack(classes, &m);
	}
}
static void LoadGunSpawners(CArray *classes, const CArray *guns)
{
	for (int i = 0; i < (int)guns->size; i++)
	{
		const GunDescription *g = CArrayGet(guns, i);
		if (!g->IsRealGun)
		{
			continue;
		}
		MapObject m;
		char spawnerName[256];
		char pickupClassName[256];
		sprintf(spawnerName, "%s spawner", g->name);
		sprintf(pickupClassName, "gun_%s", g->name);
		SetupSpawner(&m, spawnerName, pickupClassName);
		CArrayPushBack(classes, &m);
	}
}

static void SetupSpawner(
	MapObject *m, const char *spawnerName, const char *pickupClassName)
{
	memset(m, 0, sizeof *m);
	CSTRDUP(m->Name, spawnerName);
	m->Pic.Type = PICTYPE_NORMAL;
	m->Pic.u.Pic = PicManagerGetPic(&gPicManager, "spawn_pad");
	m->Pic.UseMask = true;
	m->Pic.u1.Mask = colorWhite;
	const Vec2i size = CPicGetSize(&m->Pic);
	m->Offset = Vec2iNew(-size.x / 2, TILE_HEIGHT / 2 - size.y);
	m->Size = TILE_SIZE;
	m->Health = 0;
	m->DrawLast = true;
	m->Type = MAP_OBJECT_TYPE_PICKUP_SPAWNER;
	m->u.PickupClass = StrPickupClass(pickupClassName);
}

void MapObjectsClear(CArray *classes)
{
	for (int i = 0; i < (int)classes->size; i++)
	{
		MapObject *c = CArrayGet(classes, i);
		CFREE(c->Name);
		CArrayTerminate(&c->DestroyGuns);
		CArrayTerminate(&c->DestroySpawn);
	}
	CArrayClear(classes);
}
void MapObjectsTerminate(MapObjects *classes)
{
	MapObjectsClear(&classes->Classes);
	CArrayTerminate(&classes->Classes);
	MapObjectsClear(&classes->CustomClasses);
	CArrayTerminate(&classes->CustomClasses);
	CA_FOREACH(char *, s, classes->Destructibles)
		CFREE(*s);
	CA_FOREACH_END()
	CArrayTerminate(&classes->Destructibles);
	CA_FOREACH(char *, s, classes->Bloods)
		CFREE(*s);
	CA_FOREACH_END()
	CArrayTerminate(&classes->Bloods);
}

int MapObjectsCount(const MapObjects *classes)
{
	return (int)classes->Classes.size + (int)classes->CustomClasses.size;
}


const Pic *MapObjectGetPic(const MapObject *mo, Vec2i *offset)
{
	*offset = mo->Offset;
	return CPicGetPic(&mo->Pic, 0);
}


bool MapObjectIsTileOK(
	const MapObject *obj, unsigned short tile, const bool isEmpty,
	unsigned short tileAbove)
{
	tile &= MAP_MASKACCESS;
	if (tile != MAP_FLOOR && tile != MAP_SQUARE && tile != MAP_ROOM)
	{
		return 0;
	}
	if (!isEmpty)
	{
		return 0;
	}
	tileAbove &= MAP_MASKACCESS;
	if ((obj->Flags & (1 << PLACEMENT_ON_WALL)) && tileAbove != MAP_WALL)
	{
		return 0;
	}
	return 1;
}
bool MapObjectIsTileOKStrict(
	const MapObject *obj, const unsigned short tile, const bool isEmpty,
	const unsigned short tileAbove, const unsigned short tileBelow,
	const int numWallsAdjacent, const int numWallsAround)
{
	if (!MapObjectIsTileOK(obj, tile, isEmpty, tileAbove))
	{
		return 0;
	}
	unsigned short tileAccess = tile & MAP_MASKACCESS;
	if (tile & MAP_LEAVEFREE)
	{
		return 0;
	}

	if (obj->Flags & (1 << PLACEMENT_OUTSIDE) && tileAccess == MAP_ROOM)
	{
		return false;
	}
	if ((obj->Flags & (1 << PLACEMENT_INSIDE)) && tileAccess != MAP_ROOM)
	{
		return false;
	}
	if ((obj->Flags & (1 << PLACEMENT_NO_WALLS)) && numWallsAround != 0)
	{
		return false;
	}
	if ((obj->Flags & (1 << PLACEMENT_ONE_WALL)) && numWallsAdjacent != 1)
	{
		return false;
	}
	if ((obj->Flags & (1 << PLACEMENT_ONE_OR_MORE_WALLS)) && numWallsAdjacent < 1)
	{
		return false;
	}
	if ((obj->Flags & (1 << PLACEMENT_FREE_IN_FRONT)) &&
		(tileBelow & MAP_MASKACCESS) != MAP_FLOOR &&
		(tileBelow & MAP_MASKACCESS) != MAP_SQUARE &&
		(tileBelow & MAP_MASKACCESS) != MAP_ROOM)
	{
		return false;
	}

	return true;
}
Vec2i MapObjectGetPlacementPos(const MapObject *mo, const Vec2i tilePos)
{
	Vec2i pos = Vec2iCenterOfTile(tilePos);
	pos = Vec2iAdd(pos, mo->PosOffset);
	// For on-wall objects, set their position to the top of the tile
	// This guarantees that they are drawn last
	if (mo->Flags & (1 << PLACEMENT_ON_WALL))
	{
		pos.y -= TILE_HEIGHT / 2 + 1;
	}
	return pos;
}
