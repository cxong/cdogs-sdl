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

    Copyright (c) 2014, Cong Xu
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
	for (int i = 0; i < (int)gMapObjects.CustomClasses.size; i++)
	{
		MapObject *c = CArrayGet(&gMapObjects.CustomClasses, i);
		if (strcmp(s, c->Name) == 0)
		{
			return c;
		}
	}
	for (int i = 0; i < (int)gMapObjects.Classes.size; i++)
	{
		MapObject *c = CArrayGet(&gMapObjects.Classes, i);
		if (strcmp(s, c->Name) == 0)
		{
			return c;
		}
	}
	return NULL;
}
MapObject *IntMapObject(const int m)
{
	for (int i = 0; i < (int)gMapObjects.CustomClasses.size; i++)
	{
		MapObject *c = CArrayGet(&gMapObjects.CustomClasses, i);
		if (c->Idx == m)
		{
			return c;
		}
	}
	for (int i = 0; i < (int)gMapObjects.Classes.size; i++)
	{
		MapObject *c = CArrayGet(&gMapObjects.Classes, i);
		if (c->Idx == m)
		{
			return c;
		}
	}
	CASSERT(false, "cannot find map object index");
	return NULL;
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
int MapObjectIndex(const MapObject *mo)
{
	int idx = 0;
	for (int i = 0; i < (int)gMapObjects.Classes.size; i++, idx++)
	{
		const MapObject *c = CArrayGet(&gMapObjects.Classes, i);
		if (c == mo)
		{
			return idx;
		}
	}
	for (int i = 0; i < (int)gMapObjects.CustomClasses.size; i++, idx++)
	{
		const MapObject *c = CArrayGet(&gMapObjects.CustomClasses, i);
		if (c == mo)
		{
			return idx;
		}
	}
	CASSERT(false, "cannot find map object");
	return -1;
}
MapObject *RandomBloodMapObject(const MapObjects *mo)
{
	const int idx = rand() % (int)mo->Bloods.size;
	const char **name = CArrayGet(&mo->Bloods, idx);
	return StrMapObject(*name);
}

#define VERSION 1

void MapObjectsInit(MapObjects *classes, const char *filename)
{
	CArrayInit(&classes->Classes, sizeof(MapObject));
	CArrayInit(&classes->CustomClasses, sizeof(MapObject));
	CArrayInit(&classes->Destructibles, sizeof(char *));
	CArrayInit(&classes->Bloods, sizeof(char *));

	FILE *f = fopen(filename, "r");
	json_t *root = NULL;
	if (f == NULL)
	{
		printf("Error: cannot load map objects file %s\n", filename);
		goto bail;
	}
	enum json_error e = json_stream_parse(f, &root);
	if (e != JSON_OK)
	{
		printf("Error parsing map objects file %s\n", filename);
		goto bail;
	}
	MapObjectsLoadJSON(&classes->Classes, root);

bail:
	if (f != NULL)
	{
		fclose(f);
	}
	json_free_value(&root);
}
static void LoadMapObject(MapObject *m, json_t *node);
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
		LoadMapObject(&m, child);
		CArrayPushBack(classes, &m);
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
static void LoadMapObject(MapObject *m, json_t *node)
{
	memset(m, 0, sizeof *m);
	m->Idx = -1;

	LoadInt(&m->Idx, node, "Index");
	m->Name = GetString(node, "Name");
	LoadPic(&m->Normal.Pic, node, "Pic", "OldPic");
	LoadPic(&m->Wreck.Pic, node, "WreckPic", "OldWreckPic");
	if (m->Normal.Pic)
	{
		// Default offset: centered X, align bottom of tile and sprite
		m->Normal.Offset = Vec2iNew(
			-m->Normal.Pic->size.x / 2,
			TILE_HEIGHT / 2 - m->Normal.Pic->size.y);
		LoadVec2i(&m->Normal.Offset, node, "Offset");
	}
	if (m->Wreck.Pic)
	{
		m->Wreck.Offset = Vec2iScaleDiv(m->Wreck.Pic->size, -2);
		LoadVec2i(&m->Wreck.Offset, node, "WreckOffset");
	}
	// Default tile size
	m->Size = Vec2iNew(TILE_WIDTH, TILE_HEIGHT);
	LoadVec2i(&m->Size, node, "Size");
	LoadInt(&m->Health, node, "Health");
	LoadBulletGuns(&m->DestroyGuns, node, "DestroyGuns");
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
}
static void AddDestructibles(MapObjects *mo, const CArray *classes);
static void ReloadDestructibles(MapObjects *mo)
{
	for (int i = 0; i < (int)mo->Destructibles.size; i++)
	{
		char **s = CArrayGet(&mo->Destructibles, i);
		CFREE(*s);
	}
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

static void LoadAmmoSpawners(MapObjects *classes, const CArray *ammo);
static void LoadGunSpawners(MapObjects *classes, const CArray *guns);
void MapObjectsLoadAmmoAndGunSpawners(
	MapObjects *classes, const AmmoClasses *ammo, const GunClasses *guns)
{
	LoadAmmoSpawners(classes, &ammo->Ammo);
	LoadAmmoSpawners(classes, &ammo->CustomAmmo);
	LoadGunSpawners(classes, &guns->Guns);
	LoadGunSpawners(classes, &guns->CustomGuns);
}
static void LoadAmmoSpawners(MapObjects *classes, const CArray *ammo)
{
	for (int i = 0; i < (int)ammo->size; i++)
	{
		const Ammo *a = CArrayGet(ammo, i);
		MapObject m;
		memset(&m, 0, sizeof m);
		m.Idx = -1;
		char buf[256];
		sprintf(buf, "%s spawner", a->Name);
		CSTRDUP(m.Name, buf);
		m.Normal.Pic = PicManagerGetPic(&gPicManager, "spawn_pad");
		m.Normal.Offset = Vec2iNew(
			-m.Normal.Pic->size.x / 2,
			TILE_HEIGHT / 2 - m.Normal.Pic->size.y);
		m.Size = Vec2iNew(TILE_WIDTH, TILE_HEIGHT);
		m.Health = 0;
		m.Type = MAP_OBJECT_TYPE_PICKUP_SPAWNER;
		sprintf(buf, "ammo_%s", a->Name);
		m.u.PickupClass = StrPickupClass(buf);
		CArrayPushBack(&classes->CustomClasses, &m);
	}
}
static void LoadGunSpawners(MapObjects *classes, const CArray *guns)
{
	for (int i = 0; i < (int)guns->size; i++)
	{
		const GunDescription *g = CArrayGet(guns, i);
		if (!g->IsRealGun)
		{
			continue;
		}
		MapObject m;
		memset(&m, 0, sizeof m);
		m.Idx = -1;
		char buf[256];
		sprintf(buf, "%s spawner", g->name);
		CSTRDUP(m.Name, buf);
		m.Normal.Pic = PicManagerGetPic(&gPicManager, "spawn_pad");
		m.Normal.Offset = Vec2iNew(
			-m.Normal.Pic->size.x / 2,
			TILE_HEIGHT / 2 - m.Normal.Pic->size.y);
		m.Size = Vec2iNew(TILE_WIDTH, TILE_HEIGHT);
		m.Health = 0;
		m.Type = MAP_OBJECT_TYPE_PICKUP_SPAWNER;
		sprintf(buf, "gun_%s", g->name);
		m.u.PickupClass = StrPickupClass(buf);
		CArrayPushBack(&classes->CustomClasses, &m);
	}
}

void MapObjectsClear(CArray *classes)
{
	for (int i = 0; i < (int)classes->size; i++)
	{
		MapObject *c = CArrayGet(classes, i);
		CFREE(c->Name);
		CArrayTerminate(&c->DestroyGuns);
	}
	CArrayClear(classes);
}
void MapObjectsTerminate(MapObjects *classes)
{
	MapObjectsClear(&classes->Classes);
	CArrayTerminate(&classes->Classes);
	MapObjectsClear(&classes->CustomClasses);
	CArrayTerminate(&classes->CustomClasses);
	for (int i = 0; i < (int)classes->Destructibles.size; i++)
	{
		char **s = CArrayGet(&classes->Destructibles, i);
		CFREE(*s);
	}
	CArrayTerminate(&classes->Destructibles);
	for (int i = 0; i < (int)classes->Bloods.size; i++)
	{
		char **s = CArrayGet(&classes->Bloods, i);
		CFREE(*s);
	}
	CArrayTerminate(&classes->Bloods);
}

int MapObjectsCount(const MapObjects *classes)
{
	return (int)classes->Classes.size + (int)classes->CustomClasses.size;
}


const Pic *MapObjectGetPic(
	const MapObject *mo, Vec2i *offset, const bool isWreck)
{
	const bool useWreck = isWreck && mo->Wreck.Pic;
	*offset = useWreck ? mo->Wreck.Offset : mo->Normal.Offset;
	return useWreck ? mo->Wreck.Pic : mo->Normal.Pic;
}

// Make sure the map objects get added with the right wreck flags
// Wrecks are considered debris and drawn last; this is useful for objects that
// are on the ground.
bool MapObjectIsWreck(const MapObject *mo)
{
	return !(mo->Flags & (1 << PLACEMENT_ON_WALL)) && mo->Health == 0;
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
