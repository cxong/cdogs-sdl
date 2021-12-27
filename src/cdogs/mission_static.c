/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.
	Copyright (c) 2014-2021 Cong Xu
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
#include "mission_static.h"

#include "algorithms.h"
#include "json_utils.h"
#include "log.h"
#include "map.h"
#include "map_archive.h"
#include "map_new.h"
#include "mission.h"
#include "utils.h"

void MissionStaticInit(MissionStatic *m)
{
	memset(m, 0, sizeof *m);
	m->TileClasses = hashmap_new();
	CArrayInit(&m->Tiles, sizeof(int));
	CArrayInit(&m->Access, sizeof(uint16_t));
	CArrayInit(&m->Items, sizeof(MapObjectPositions));
	CArrayInit(&m->Characters, sizeof(CharacterPlaces));
	CArrayInit(&m->Objectives, sizeof(ObjectivePositions));
	CArrayInit(&m->Keys, sizeof(KeyPositions));
	CArrayInit(&m->Pickups, sizeof(PickupPositions));
	CArrayInit(&m->Exits, sizeof(Exit));
	m->AltFloorsEnabled = true;
}

static void LoadTileClasses(map_t tileClasses, const json_t *node);
static void LoadOldStaticTileCSV(CArray *tiles, char *tileCSV);
static void LoadStaticTileCSV(CArray *tiles, char *tileCSV);
static void ConvertOldTile(
	MissionStatic *m, const uint16_t t, const TileClass *base);
static void LoadStaticItems(
	MissionStatic *m, json_t *node, const char *name, const int version);
static void LoadStaticWrecks(
	MissionStatic *m, json_t *node, const char *name, const int version);
static void LoadStaticCharacters(MissionStatic *m, json_t *node, char *name);
static void LoadStaticObjectives(
	MissionStatic *m, const struct vec2i size, json_t *node, char *name);
static void LoadStaticKeys(MissionStatic *m, json_t *node, char *name);
static void LoadStaticPickups(MissionStatic *m, json_t *node, char *name);
static void LoadStaticExit(
	MissionStatic *m, const json_t *node, const char *name, const int mission);
static void LoadStaticExits(
	MissionStatic *m, const json_t *node, const char *name);
bool MissionStaticTryLoadJSON(
	MissionStatic *m, json_t *node, const struct vec2i size, const int version,
	const int mission)
{
	MissionStaticInit(m);
	if (version <= 14)
	{
		MissionTileClasses mtc;
		LoadMissionTileClasses(&mtc, node, version);
		// old tile type ints
		CArray oldTiles;
		CArrayInit(&oldTiles, sizeof(uint16_t));
		if (version == 1)
		{
			// JSON array
			json_t *tiles = json_find_first_label(node, "Tiles");
			if (!tiles || !tiles->child)
			{
				return false;
			}
			tiles = tiles->child;
			for (tiles = tiles->child; tiles; tiles = tiles->next)
			{
				const uint16_t n = (uint16_t)atoi(tiles->text);
				CArrayPushBack(&oldTiles, &n);
			}
		}
		else if (version <= 14)
		{
			// CSV string
			char *tileCSV = GetString(node, "Tiles");
			LoadOldStaticTileCSV(&oldTiles, tileCSV);
			CFREE(tileCSV);
		}
		// Convert old tiles to new
		CA_FOREACH(uint16_t, t, oldTiles)
		const uint16_t tileAccess = *t & MAP_ACCESSBITS;
		CArrayPushBack(&m->Access, &tileAccess);
		*t &= MAP_MASKACCESS;
		switch (*t)
		{
		case MAP_FLOOR:
		case MAP_SQUARE: // fallthrough
			ConvertOldTile(m, *t, &mtc.Floor);
			break;
		case MAP_WALL:
			ConvertOldTile(m, *t, &mtc.Wall);
			break;
		case MAP_DOOR:
			ConvertOldTile(m, *t, &mtc.Door);
			break;
		case MAP_ROOM:
			ConvertOldTile(m, *t, &mtc.Room);
			break;
		default:
			ConvertOldTile(m, *t, &gTileNothing);
			break;
		}
		CA_FOREACH_END()
		CArrayTerminate(&oldTiles);
		MissionTileClassesTerminate(&mtc);
	}
	else
	{
		// Tile class definitions
		LoadTileClasses(m->TileClasses, node);

		// CSV string per row
		const json_t *tile =
			json_find_first_label(node, "Tiles")->child->child;
		while (tile)
		{
			LoadStaticTileCSV(&m->Tiles, tile->text);
			tile = tile->next;
		}
		const json_t *a = json_find_first_label(node, "Access")->child->child;
		while (a)
		{
			CArray mAccess;
			CArrayInit(&mAccess, sizeof(int));
			LoadStaticTileCSV(&mAccess, a->text);
			CA_FOREACH(const int, aInt, mAccess)
			const uint16_t a16 = (uint16_t)*aInt;
			CArrayPushBack(&m->Access, &a16);
			CA_FOREACH_END()
			CArrayTerminate(&mAccess);
			a = a->next;
		}
	}

	LoadStaticItems(m, node, "StaticItems", version);
	if (version < 13)
	{
		LoadStaticWrecks(m, node, "StaticWrecks", version);
	}
	LoadStaticCharacters(m, node, "StaticCharacters");
	LoadStaticObjectives(m, size, node, "StaticObjectives");
	LoadStaticKeys(m, node, "StaticKeys");
	LoadStaticPickups(m, node, "StaticPickups");

	LoadVec2i(&m->Start, node, "Start");
	if (version < 16)
	{
		LoadStaticExit(m, node, "Exit", mission);
	}
	else
	{
		LoadStaticExits(m, node, "Exits");
	}

	m->AltFloorsEnabled = true;
	LoadBool(&m->AltFloorsEnabled, node, "AltFloorsEnabled");

	return true;
}
static void LoadTileClasses(map_t tileClasses, const json_t *node)
{
	const json_t *class = json_find_first_label(node, "TileClasses");
	if (!class || !class->child || !class->child->child)
	{
		CASSERT(false, "cannot load tile classes");
		return;
	}
	for (class = class->child->child; class; class = class->next)
	{
		json_t *classNode = class->child;
		TileClass *tc;
		CMALLOC(tc, sizeof *tc);
		TileClassLoadJSON(tc, classNode);
		if (hashmap_put(tileClasses, class->text, (any_t *)tc) != MAP_OK)
		{
			CASSERT(false, "cannot add tile class");
		}
	}
}
static void LoadOldStaticTileCSV(CArray *tiles, char *tileCSV)
{
	char *pch = strtok(tileCSV, ",");
	while (pch != NULL)
	{
		const uint16_t n = (uint16_t)atoi(pch);
		CArrayPushBack(tiles, &n);
		pch = strtok(NULL, ",");
	}
}
static void LoadStaticTileCSV(CArray *tiles, char *tileCSV)
{
	char *pch = strtok(tileCSV, ",");
	while (pch != NULL)
	{
		const int n = atoi(pch);
		CArrayPushBack(tiles, &n);
		pch = strtok(NULL, ",");
	}
}
static void ConvertOldTile(
	MissionStatic *m, const uint16_t t, const TileClass *base)
{
	char keyBuf[6];
	sprintf(keyBuf, "%d", (int)t);
	if (hashmap_get(m->TileClasses, keyBuf, NULL) == MAP_MISSING)
	{
		TileClass *tc;
		CMALLOC(tc, sizeof *tc);
		memcpy(tc, base, sizeof *tc);
		if (base->Name)
			CSTRDUP(tc->Name, base->Name);
		if (base->Style)
			CSTRDUP(tc->Style, base->Style);
		if (base->StyleType)
			CSTRDUP(tc->StyleType, base->StyleType);
		tc->Mask = base->Mask;
		tc->MaskAlt = base->MaskAlt;
		if (hashmap_put(m->TileClasses, keyBuf, tc) != MAP_OK)
		{
			CASSERT(false, "Failed to add tile class");
		}
	}
	const int tile = (int)t;
	CArrayPushBack(&m->Tiles, &tile);
}
static bool TryLoadPositions(CArray *a, const json_t *node);
static bool TryLoadPlaces(CArray *a, const json_t *node);
static const MapObject *LoadMapObjectRef(json_t *node, const int version);
static const MapObject *LoadMapObjectWreckRef(
	json_t *itemNode, const int version);
static void LoadStaticItems(
	MissionStatic *m, json_t *node, const char *name, const int version)
{
	json_t *items = json_find_first_label(node, name);
	if (!items || !items->child)
	{
		return;
	}
	items = items->child;
	for (items = items->child; items; items = items->next)
	{
		MapObjectPositions mop;
		mop.M = LoadMapObjectRef(items, version);
		if (mop.M == NULL)
		{
			continue;
		}
		if (!TryLoadPositions(&mop.Positions, items))
		{
			continue;
		}
		CArrayPushBack(&m->Items, &mop);
	}
}
static void LoadStaticWrecks(
	MissionStatic *m, json_t *node, const char *name, const int version)
{
	json_t *items = json_find_first_label(node, name);
	if (!items || !items->child)
	{
		return;
	}
	items = items->child;
	for (items = items->child; items; items = items->next)
	{
		MapObjectPositions mop;
		mop.M = LoadMapObjectWreckRef(items, version);
		if (mop.M == NULL)
		{
			continue;
		}
		if (!TryLoadPositions(&mop.Positions, items))
		{
			continue;
		}
		CArrayPushBack(&m->Items, &mop);
	}
}
static const MapObject *LoadMapObjectRef(json_t *itemNode, const int version)
{
	if (version <= 3)
	{
		int idx;
		LoadInt(&idx, itemNode, "Index");
		return IntMapObject(idx);
	}
	else
	{
		const char *moName =
			json_find_first_label(itemNode, "MapObject")->child->text;
		const MapObject *mo = StrMapObject(moName);
		if (mo == NULL && version <= 11 && StrEndsWith(moName, " spawner"))
		{
			// Old version had same name for ammo and gun spawner
			char itemName[256];
			strcpy(itemName, moName);
			itemName[strlen(moName) - strlen(" spawner")] = '\0';
			char buf[300];
			snprintf(buf, sizeof buf, "%s ammo spawner", itemName);
			mo = StrMapObject(buf);
		}
		if (mo == NULL)
		{
			LOG(LM_MAP, LL_ERROR, "Failed to load map object (%s)", moName);
		}
		return mo;
	}
}
static const MapObject *LoadMapObjectWreckRef(
	json_t *itemNode, const int version)
{
	if (version <= 3)
	{
		int idx;
		LoadInt(&idx, itemNode, "Index");
		return IntMapObject(idx);
	}
	const char *moName =
		json_find_first_label(itemNode, "MapObject")->child->text;
	const MapObject *mo = StrMapObject(moName);
	if (mo == NULL)
	{
		LOG(LM_MAP, LL_ERROR, "Failed to load map object (%s)", moName);
		return NULL;
	}
	const MapObject *wreck = StrMapObject(mo->Wreck.MO);
	return wreck;
}
static void LoadStaticCharacters(MissionStatic *m, json_t *node, char *name)
{
	json_t *chars = json_find_first_label(node, name);
	if (!chars || !chars->child)
	{
		return;
	}
	chars = chars->child;
	for (chars = chars->child; chars; chars = chars->next)
	{
		CharacterPlaces cps;
		LoadInt(&cps.Index, chars, "Index");
		CArrayInit(&cps.Places, sizeof(CharacterPlace));

		// Try loading positions
		CArray positions;
		memset(&positions, 0, sizeof positions);
		if (TryLoadPositions(&positions, chars))
		{
			CA_FOREACH(const struct vec2i, pos, positions)
			CharacterPlace cp;
			cp.Pos = *pos;
			cp.Dir = rand() % DIRECTION_COUNT;
			CArrayPushBack(&cps.Places, &cp);
			CA_FOREACH_END()
		}
		CArrayTerminate(&positions);

		TryLoadPlaces(&cps.Places, chars);

		CArrayPushBack(&m->Characters, &cps);
	}
}
static void LoadStaticObjectives(
	MissionStatic *m, const struct vec2i size, json_t *node, char *name)
{
	json_t *objs = json_find_first_label(node, name);
	if (!objs || !objs->child)
	{
		return;
	}
	objs = objs->child;
	for (objs = objs->child; objs; objs = objs->next)
	{
		ObjectivePositions op;
		LoadInt(&op.Index, objs, "Index");

		json_t *positions = json_find_first_label(objs, "Positions");
		if (!positions || !positions->child)
		{
			continue;
		}
		positions = positions->child;
		CArray opPositions;
		CArrayInit(&opPositions, sizeof(struct vec2i));
		for (positions = positions->child; positions;
			 positions = positions->next)
		{
			struct vec2i pos;
			json_t *position = positions->child;
			pos.x = atoi(position->text);
			position = position->next;
			if (position == NULL)
				continue;
			pos.y = atoi(position->text);
			// Ignore objectives outside map
			if (!Rect2iIsInside(Rect2iNew(svec2i_zero(), size), pos))
			{
				continue;
			}
			CArrayPushBack(&opPositions, &pos);
		}

		CArray opIndices;
		CArrayInit(&opIndices, sizeof(int));
		LoadIntArray(&opIndices, objs, "Indices");

		CArrayInit(&op.PositionIndices, sizeof(PositionIndex));
		for (int i = 0; i < (int)MIN(opPositions.size, opIndices.size); i++)
		{
			PositionIndex pi;
			pi.Position = *(struct vec2i *)CArrayGet(&opPositions, i);
			pi.Index = *(int *)CArrayGet(&opIndices, i);
			CArrayPushBack(&op.PositionIndices, &pi);
		}

		CArrayTerminate(&opPositions);
		CArrayTerminate(&opIndices);

		CArrayPushBack(&m->Objectives, &op);
	}
}
static void LoadStaticKeys(MissionStatic *m, json_t *node, char *name)
{
	json_t *keys = json_find_first_label(node, name);
	if (!keys || !keys->child)
	{
		return;
	}
	keys = keys->child;
	for (keys = keys->child; keys; keys = keys->next)
	{
		KeyPositions kp;
		LoadInt(&kp.Index, keys, "Index");
		if (!TryLoadPositions(&kp.Positions, keys))
		{
			continue;
		}
		CArrayPushBack(&m->Keys, &kp);
	}
}
static const PickupClass *LoadPickupRef(const json_t *itemNode);
static void LoadStaticPickups(MissionStatic *m, json_t *node, char *name)
{
	json_t *pickups = json_find_first_label(node, name);
	if (!pickups || !pickups->child)
	{
		return;
	}
	pickups = pickups->child;
	for (pickups = pickups->child; pickups; pickups = pickups->next)
	{
		PickupPositions pp;
		pp.P = LoadPickupRef(pickups);
		if (pp.P == NULL)
		{
			continue;
		}
		if (!TryLoadPositions(&pp.Positions, pickups))
		{
			continue;
		}
		CArrayPushBack(&m->Pickups, &pp);
	}
}
static const PickupClass *LoadPickupRef(const json_t *itemNode)
{
	const char *pName = json_find_first_label(itemNode, "Pickup")->child->text;
	const PickupClass *p = StrPickupClass(pName);
	if (p == NULL)
	{
		LOG(LM_MAP, LL_ERROR, "Failed to load pickup (%s)", pName);
	}
	return p;
}
static bool TryLoadPositions(CArray *a, const json_t *node)
{
	json_t *positions = json_find_first_label(node, "Positions");
	if (!positions || !positions->child)
	{
		return false;
	}
	positions = positions->child;
	CArrayInit(a, sizeof(struct vec2i));
	for (positions = positions->child; positions; positions = positions->next)
	{
		struct vec2i pos;
		json_t *position = positions->child;
		pos.x = atoi(position->text);
		position = position->next;
		pos.y = atoi(position->text);
		CArrayPushBack(a, &pos);
	}
	return true;
}
static bool TryLoadPlaces(CArray *a, const json_t *node)
{
	json_t *places = json_find_first_label(node, "Places");
	if (!places || !places->child)
	{
		return false;
	}
	places = places->child;
	for (places = places->child; places; places = places->next)
	{
		CharacterPlace cp;
		LoadVec2i(&cp.Pos, places, "Pos");
		int d;
		LoadInt(&d, places, "Dir");
		cp.Dir = (direction_e)d;
		CArrayPushBack(a, &cp);
	}
	return true;
}
static void LoadStaticExit(
	MissionStatic *m, const json_t *node, const char *name, const int mission)
{
	json_t *exitNode = json_find_first_label(node, name);
	if (!exitNode || !exitNode->child)
	{
		return;
	}
	exitNode = exitNode->child;
	Exit exit;
	memset(&exit, 0, sizeof exit);
	LoadVec2i(&exit.R.Pos, exitNode, "Start");
	struct vec2i end;
	LoadVec2i(&end, exitNode, "End");
	exit.R.Size = svec2i_subtract(end, exit.R.Pos);
	exit.Mission = mission + 1;
	CArrayPushBack(&m->Exits, &exit);
}
static void LoadStaticExits(
	MissionStatic *m, const json_t *node, const char *name)
{
	json_t *exits = json_find_first_label(node, name);
	if (!exits || !exits->child)
	{
		return;
	}
	exits = exits->child;
	for (exits = exits->child; exits; exits = exits->next)
	{
		Exit exit;
		memset(&exit, 0, sizeof exit);
		LoadRect2i(&exit.R, exits, "Rect");
		LoadInt(&exit.Mission, exits, "Mission");
		LoadBool(&exit.Hidden, exits, "Hidden");
		CArrayPushBack(&m->Exits, &exit);
	}
}

void MissionStaticFromMap(MissionStatic *m, const Map *map)
{
	MissionStaticInit(m);
	// Create map of tile class names to integers
	map_t tileClassMap = hashmap_new();
	// Take all the tiles from the current map and save them in the static map
	RECT_FOREACH(Rect2iNew(svec2i_zero(), map->Size))
	const Tile *t = MapGetTile(map, _v);
	intptr_t tile;
	char tcName[256];
	TileClassGetBaseName(tcName, t->Class);
	if (hashmap_get(tileClassMap, tcName, (any_t *)&tile) == MAP_MISSING)
	{
		TileClass *tc = MissionStaticAddTileClass(m, t->Class);
		if (tc == NULL)
		{
			continue;
		}
		tile = (intptr_t)hashmap_length(m->TileClasses) - 1;
		if (hashmap_put(tileClassMap, tcName, (any_t)tile) != MAP_OK)
		{
			LOG(LM_MAP, LL_ERROR, "Failed to add tile class (%s)", tcName);
			TileClassTerminate(tc);
			continue;
		}
		LOG(LM_MAP, LL_DEBUG, "Added tile class (%s)", tcName);
	}
	const int tileInt = (int)tile;
	CArrayPushBack(&m->Tiles, &tileInt);
	const uint16_t access = MapGetAccessLevel(map, _v);
	CArrayPushBack(&m->Access, &access);
	RECT_FOREACH_END()
	hashmap_free(tileClassMap);
	CArrayCopy(&m->Exits, &map->exits);
}

void MissionStaticTerminate(MissionStatic *m)
{
	hashmap_destroy(m->TileClasses, TileClassDestroy);
	CArrayTerminate(&m->Tiles);
	CArrayTerminate(&m->Access);
	CArrayTerminate(&m->Items);
	CArrayTerminate(&m->Characters);
	CArrayTerminate(&m->Objectives);
	CArrayTerminate(&m->Keys);
	CArrayTerminate(&m->Pickups);
	CArrayTerminate(&m->Exits);
}

static json_t *SaveStaticTileClasses(const MissionStatic *m);
static json_t *SaveStaticCSV(const CArray *values, const struct vec2i size);
static json_t *SaveStaticItems(const MissionStatic *m);
static json_t *SaveStaticCharacters(const MissionStatic *m);
static json_t *SaveStaticObjectives(const MissionStatic *m);
static json_t *SaveStaticKeys(const MissionStatic *m);
static json_t *SaveStaticPickups(const MissionStatic *m);
static json_t *SaveExits(const MissionStatic *m);
static json_t *SaveVec2i(struct vec2i v);
void MissionStaticSaveJSON(
	const MissionStatic *m, const struct vec2i size, json_t *node)
{
	json_insert_pair_into_object(
		node, "TileClasses", SaveStaticTileClasses(m));
	json_insert_pair_into_object(
		node, "Tiles", SaveStaticCSV(&m->Tiles, size));
	CArray mAccess;
	CArrayInit(&mAccess, sizeof(int));
	CA_FOREACH(const uint16_t, a, m->Access)
	const int aInt = (int)*a;
	CArrayPushBack(&mAccess, &aInt);
	CA_FOREACH_END()
	json_insert_pair_into_object(
		node, "Access", SaveStaticCSV(&mAccess, size));
	CArrayTerminate(&mAccess);
	json_insert_pair_into_object(node, "StaticItems", SaveStaticItems(m));
	json_insert_pair_into_object(
		node, "StaticCharacters", SaveStaticCharacters(m));
	json_insert_pair_into_object(
		node, "StaticObjectives", SaveStaticObjectives(m));
	json_insert_pair_into_object(node, "StaticKeys", SaveStaticKeys(m));
	json_insert_pair_into_object(node, "StaticPickups", SaveStaticPickups(m));

	json_insert_pair_into_object(node, "Start", SaveVec2i(m->Start));
	json_insert_pair_into_object(node, "Exits", SaveExits(m));

	AddBoolPair(node, "AltFloorsEnabled", m->AltFloorsEnabled);
}
typedef struct
{
	json_t *items;
	map_t tileClasses;
} SaveStaticTileClassData;
static int SaveStaticTileClass(any_t data, any_t key);
static json_t *SaveStaticTileClasses(const MissionStatic *m)
{
	json_t *items = json_new_object();
	SaveStaticTileClassData data = {items, m->TileClasses};
	if (hashmap_iterate_keys_sorted(
			m->TileClasses, SaveStaticTileClass, &data) != MAP_OK)
	{
		CASSERT(false, "Failed to save static tile classes");
	}
	return items;
}
static int SaveStaticTileClass(any_t data, any_t key)
{
	SaveStaticTileClassData *sData = (SaveStaticTileClassData *)data;
	TileClass *tc;
	const int error =
		hashmap_get(sData->tileClasses, (const char *)key, (any_t *)&tc);
	if (error != MAP_OK)
	{
		CASSERT(false, "cannot find tile class");
		return error;
	}
	json_insert_pair_into_object(
		sData->items, (const char *)key, TileClassSaveJSON(tc));
	return MAP_OK;
}
static json_t *SaveStaticCSV(const CArray *values, const struct vec2i size)
{
	// Write out each row of tiles individually as a single CSV
	json_t *rows = json_new_array();
	// Create a text buffer for CSV
	// The buffer will contain n*5 chars (tiles, allow 5 chars each),
	// and n - 1 commas, so 6n total
	char *rowBuf;
	CMALLOC(rowBuf, size.x * 6);
	for (int i = 0; i < size.y; i++)
	{
		char *pBuf = rowBuf;
		*pBuf = '\0';
		for (int j = 0; j < size.x; j++)
		{
			char buf[6];
			snprintf(buf, 6, "%d", *(int *)CArrayGet(values, i * size.x + j));
			strcpy(pBuf, buf);
			pBuf += strlen(buf);
			if (j < size.x - 1)
			{
				*pBuf++ = ',';
			}
		}
		json_insert_child(rows, json_new_string(rowBuf));
	}
	CFREE(rowBuf);
	return rows;
}
static void SavePositions(json_t *node, const CArray *positions);
static void SaveCharacterPlaces(json_t *node, const CArray *cps);
static json_t *SaveStaticItems(const MissionStatic *m)
{
	json_t *items = json_new_array();
	CA_FOREACH(MapObjectPositions, mop, m->Items)
	json_t *itemNode = json_new_object();
	AddStringPair(itemNode, "MapObject", mop->M->Name);
	SavePositions(itemNode, &mop->Positions);
	json_insert_child(items, itemNode);
	CA_FOREACH_END()
	return items;
}
static json_t *SaveStaticCharacters(const MissionStatic *m)
{
	json_t *chars = json_new_array();
	CA_FOREACH(const CharacterPlaces, cp, m->Characters)
	json_t *charNode = json_new_object();
	AddIntPair(charNode, "Index", cp->Index);
	SaveCharacterPlaces(charNode, &cp->Places);
	json_insert_child(chars, charNode);
	CA_FOREACH_END()
	return chars;
}
static json_t *SaveStaticObjectives(const MissionStatic *m)
{
	json_t *objs = json_new_array();
	CA_FOREACH(ObjectivePositions, op, m->Objectives)
	json_t *objNode = json_new_object();
	AddIntPair(objNode, "Index", op->Index);
	json_t *positions = json_new_array();
	json_t *indices = json_new_array();
	for (int j = 0; j < (int)op->PositionIndices.size; j++)
	{
		const PositionIndex *pi = CArrayGet(&op->PositionIndices, j);
		json_insert_child(positions, SaveVec2i(pi->Position));
		char buf[32];
		sprintf(buf, "%d", pi->Index);
		json_insert_child(indices, json_new_number(buf));
	}
	json_insert_pair_into_object(objNode, "Positions", positions);
	json_insert_pair_into_object(objNode, "Indices", indices);
	json_insert_child(objs, objNode);
	CA_FOREACH_END()
	return objs;
}
static json_t *SaveStaticKeys(const MissionStatic *m)
{
	json_t *keys = json_new_array();
	CA_FOREACH(KeyPositions, kp, m->Keys)
	json_t *keyNode = json_new_object();
	AddIntPair(keyNode, "Index", kp->Index);
	SavePositions(keyNode, &kp->Positions);
	json_insert_child(keys, keyNode);
	CA_FOREACH_END()
	return keys;
}
static json_t *SaveStaticPickups(const MissionStatic *m)
{
	json_t *pickups = json_new_array();
	CA_FOREACH(const PickupPositions, pp, m->Pickups)
	json_t *pNode = json_new_object();
	AddStringPair(pNode, "Pickup", pp->P->Name);
	SavePositions(pNode, &pp->Positions);
	json_insert_child(pickups, pNode);
	CA_FOREACH_END()
	return pickups;
}
static void SavePositions(json_t *node, const CArray *positions)
{
	json_t *a = json_new_array();
	CA_FOREACH(const struct vec2i, pos, *positions)
	json_insert_child(a, SaveVec2i(*pos));
	CA_FOREACH_END()
	json_insert_pair_into_object(node, "Positions", a);
}
static void SaveCharacterPlaces(json_t *node, const CArray *cps)
{
	json_t *a = json_new_array();
	CA_FOREACH(const CharacterPlace, cp, *cps)
	json_t *cpNode = json_new_object();
	AddVec2iPair(cpNode, "Pos", cp->Pos);
	AddIntPair(cpNode, "Dir", (int)cp->Dir);
	json_insert_child(a, cpNode);
	CA_FOREACH_END()
	json_insert_pair_into_object(node, "Places", a);
}
static json_t *SaveExits(const MissionStatic *m)
{
	json_t *exits = json_new_array();
	CA_FOREACH(const Exit, exit, m->Exits)
	json_t *exitNode = json_new_object();
	AddRect2iPair(exitNode, "Rect", exit->R);
	AddIntPair(exitNode, "Mission", exit->Mission);
	AddBoolPair(exitNode, "Hidden", exit->Hidden);
	json_insert_child(exits, exitNode);
	CA_FOREACH_END()
	return exits;
}
static json_t *SaveVec2i(struct vec2i v)
{
	json_t *node = json_new_array();
	char buf[32];
	sprintf(buf, "%d", v.x);
	json_insert_child(node, json_new_number(buf));
	sprintf(buf, "%d", v.y);
	json_insert_child(node, json_new_number(buf));
	return node;
}

static void MapObjectPositionsCopy(CArray *dst, const CArray *src);
static void CharacterPositionsCopy(CArray *dst, const CArray *src);
static void ObjectivePositionsCopy(CArray *dst, const CArray *src);
static void KeyPositionsCopy(CArray *dst, const CArray *src);
static void PickupPositionsCopy(CArray *dst, const CArray *src);
void MissionStaticCopy(MissionStatic *dst, const MissionStatic *src)
{
	memcpy(dst, src, sizeof *dst);
	dst->TileClasses = hashmap_copy(src->TileClasses, TileClassCopyHashMap);
	memset(&dst->Tiles, 0, sizeof dst->Tiles);
	CArrayCopy(&dst->Tiles, &src->Tiles);
	memset(&dst->Access, 0, sizeof dst->Access);
	CArrayCopy(&dst->Access, &src->Access);
	MapObjectPositionsCopy(&dst->Items, &src->Items);
	CharacterPositionsCopy(&dst->Characters, &src->Characters);
	ObjectivePositionsCopy(&dst->Objectives, &src->Objectives);
	KeyPositionsCopy(&dst->Keys, &src->Keys);
	PickupPositionsCopy(&dst->Pickups, &src->Pickups);
	memset(&dst->Exits, 0, sizeof dst->Exits);
	CArrayCopy(&dst->Exits, &src->Exits);
}
any_t TileClassCopyHashMap(any_t in)
{
	TileClass *tc;
	CMALLOC(tc, sizeof *tc);
	TileClassCopy(tc, (const TileClass *)in);
	return (any_t)tc;
}
static void MapObjectPositionsCopy(CArray *dst, const CArray *src)
{
	CArrayInit(dst, src->elemSize);
	CA_FOREACH(const MapObjectPositions, p, *src)
	MapObjectPositions pCopy;
	memset(&pCopy, 0, sizeof pCopy);
	pCopy.M = p->M;
	CArrayCopy(&pCopy.Positions, &p->Positions);
	CArrayPushBack(dst, &pCopy);
	CA_FOREACH_END()
}
static void CharacterPositionsCopy(CArray *dst, const CArray *src)
{
	CArrayInit(dst, src->elemSize);
	CA_FOREACH(const CharacterPlaces, cp, *src)
	CharacterPlaces cpCopy;
	memset(&cpCopy, 0, sizeof cpCopy);
	cpCopy.Index = cp->Index;
	CArrayCopy(&cpCopy.Places, &cp->Places);
	CArrayPushBack(dst, &cpCopy);
	CA_FOREACH_END()
}
static void ObjectivePositionsCopy(CArray *dst, const CArray *src)
{
	CArrayInit(dst, src->elemSize);
	CA_FOREACH(const ObjectivePositions, p, *src)
	ObjectivePositions pCopy;
	memset(&pCopy, 0, sizeof pCopy);
	pCopy.Index = p->Index;
	CArrayCopy(&pCopy.PositionIndices, &p->PositionIndices);
	CArrayPushBack(dst, &pCopy);
	CA_FOREACH_END()
}
static void KeyPositionsCopy(CArray *dst, const CArray *src)
{
	CArrayInit(dst, src->elemSize);
	CA_FOREACH(const KeyPositions, p, *src)
	KeyPositions pCopy;
	memset(&pCopy, 0, sizeof pCopy);
	pCopy.Index = p->Index;
	CArrayCopy(&pCopy.Positions, &p->Positions);
	CArrayPushBack(dst, &pCopy);
	CA_FOREACH_END()
}
static void PickupPositionsCopy(CArray *dst, const CArray *src)
{
	CArrayInit(dst, src->elemSize);
	CA_FOREACH(const PickupPositions, p, *src)
	PickupPositions pCopy;
	memset(&pCopy, 0, sizeof pCopy);
	pCopy.P = p->P;
	CArrayCopy(&pCopy.Positions, &p->Positions);
	CArrayPushBack(dst, &pCopy);
	CA_FOREACH_END()
}

int MissionStaticGetTile(
	const MissionStatic *m, const struct vec2i size, const struct vec2i pos)
{
	CASSERT(
		(int)m->Tiles.size == size.x * size.y,
		"static mission tiles size mismatch");
	if (!Rect2iIsInside(Rect2iNew(svec2i_zero(), size), pos))
	{
		return -1;
	}
	return *(int *)CArrayGet(&m->Tiles, size.x * pos.y + pos.x);
}
const TileClass *MissionStaticGetTileClass(
	const MissionStatic *m, const struct vec2i size, const struct vec2i pos)
{
	const int tile = MissionStaticGetTile(m, size, pos);
	if (tile < 0)
	{
		return NULL;
	}
	return MissionStaticIdTileClass(m, tile);
}
TileClass *MissionStaticIdTileClass(const MissionStatic *m, const int tile)
{
	char keyBuf[6];
	sprintf(keyBuf, "%d", tile);
	TileClass *tc = NULL;
	const int error = hashmap_get(m->TileClasses, keyBuf, (any_t)&tc);
	if (error != MAP_OK && error != MAP_MISSING)
	{
		CASSERT(false, "error getting tile id");
	}
	return tc;
}
bool MissionStaticTrySetTile(
	MissionStatic *m, const struct vec2i size, const struct vec2i pos,
	const int tile)
{
	if (!Rect2iIsInside(Rect2iNew(svec2i_zero(), size), pos))
	{
		return false;
	}
	const int idx = pos.y * size.x + pos.x;
	CArraySet(&m->Tiles, idx, &tile);
	return true;
}
void MissionStaticClearTile(
	MissionStatic *m, const struct vec2i size, const struct vec2i pos)
{
	char *key;
	if (hashmap_get_one_key(m->TileClasses, (any_t *)&key) != MAP_OK)
	{
		CASSERT(false, "No default tile for static map");
		return;
	}
	const int tile = atoi(key);
	MissionStaticTrySetTile(m, size, pos, tile);
}

TileClass *MissionStaticAddTileClass(MissionStatic *m, const TileClass *base)
{
	TileClass *tc;
	CMALLOC(tc, sizeof *tc);
	TileClassCopy(tc, base);
	// Try to find an empty slot to add the new tile class
	int i;
	for (i = 0; MissionStaticIdTileClass(m, i) != NULL; i++)
		;
	char buf[12];
	snprintf(buf, sizeof buf - 1, "%d", i);
	if (hashmap_put(m->TileClasses, buf, (any_t *)tc) != MAP_OK)
	{
		char tcName[256];
		TileClassGetBaseName(tcName, base);
		LOG(LM_MAP, LL_ERROR, "Failed to add tile class (%s)", tcName);
		TileClassTerminate(tc);
		return NULL;
	}
	return tc;
}
bool MissionStaticRemoveTileClass(MissionStatic *m, const int tile)
{
	char keyBuf[6];
	sprintf(keyBuf, "%d", tile);
	if (hashmap_remove(m->TileClasses, keyBuf) != MAP_OK)
	{
		CASSERT(false, "cannot remove tile id");
		return false;
	}
	if (hashmap_length(m->TileClasses) == 0)
	{
		return true;
	}
	// Set all tiles using this to the first one
	int i;
	for (i = 0; MissionStaticIdTileClass(m, i) == NULL; i++)
		;
	CA_FOREACH(int, t, m->Tiles)
	if (*t == tile)
	{
		*t = i;
	}
	CA_FOREACH_END()
	return true;
}

void MissionStaticLayout(
	MissionStatic *m, const struct vec2i size, const struct vec2i oldSize)
{
	// re-layout the static map after a resize
	// Simply try to "paint" the old tiles to the new mission
	CArray oldTiles, oldAccess;
	CArrayInit(&oldTiles, m->Tiles.elemSize);
	CArrayInit(&oldAccess, m->Access.elemSize);
	CArrayCopy(&oldTiles, &m->Tiles);
	CArrayCopy(&oldAccess, &m->Access);
	CArrayResize(&m->Tiles, size.x * size.y, NULL);
	CArrayFillZero(&m->Tiles);
	const uint16_t noAccess = 0;
	CArrayResize(&m->Access, size.x * size.y, &noAccess);

	// Paint the old tiles back
	RECT_FOREACH(Rect2iNew(svec2i_zero(), size))
	if (_v.x >= oldSize.x || _v.y >= oldSize.y)
	{
		MissionStaticClearTile(m, size, _v);
	}
	else
	{
		const int idx = _v.y * oldSize.x + _v.x;
		const int *tile = CArrayGet(&oldTiles, idx);
		MissionStaticTrySetTile(m, size, _v, *tile);
		const uint16_t *a = CArrayGet(&oldAccess, idx);
		CArraySet(&m->Access, _v.y * size.x + _v.x, a);
	}
	RECT_FOREACH_END()

	CArrayTerminate(&oldTiles);

	m->Start = svec2i_clamp(
		m->Start, svec2i_zero(), svec2i_subtract(size, svec2i_one()));
}

static bool TryRemovePosition(CArray *positions, const struct vec2i pos);
static bool TryRemoveCharacterPlace(CArray *cps, const struct vec2i pos);
static bool TryRotateCharacterPlace(CArray *cps, const struct vec2i pos);

bool MissionStaticTryAddItem(
	MissionStatic *m, const MapObject *mo, const struct vec2i pos)
{
	CASSERT(mo != NULL, "adding NULL map object");
	// Check if the item already has an entry, and add to its list
	// of positions
	bool hasAdded = false;
	for (int i = 0; i < (int)m->Items.size; i++)
	{
		MapObjectPositions *mop = CArrayGet(&m->Items, i);
		if (mop->M == mo)
		{
			// Check if map object already added at same position
			// This can happen for wall map items that don't take up
			// any space
			CA_FOREACH(const struct vec2i, mpos, mop->Positions)
			if (svec2i_is_equal(pos, *mpos))
			{
				return false;
			}
			CA_FOREACH_END()
			CArrayPushBack(&mop->Positions, &pos);
			hasAdded = true;
			break;
		}
	}
	// If not, create a new entry
	if (!hasAdded)
	{
		MapObjectPositions mop;
		mop.M = mo;
		CArrayInit(&mop.Positions, sizeof(struct vec2i));
		CArrayPushBack(&mop.Positions, &pos);
		CArrayPushBack(&m->Items, &mop);
	}
	return true;
}
bool MissionStaticTryRemoveItemAt(MissionStatic *m, const struct vec2i pos)
{
	CA_FOREACH(MapObjectPositions, mop, m->Items)
	if (TryRemovePosition(&mop->Positions, pos))
	{
		if (mop->Positions.size == 0)
		{
			CArrayDelete(&m->Items, _ca_index);
		}
		return true;
	}
	CA_FOREACH_END()
	return false;
}

void MissionStaticAddCharacter(
	MissionStatic *m, const int ch, const CharacterPlace cp)
{
	// Check if the character already has an entry, and add to its list
	// of positions
	bool hasAdded = false;
	CA_FOREACH(CharacterPlaces, cps, m->Characters)
	if (cps->Index == ch)
	{
		CArrayPushBack(&cps->Places, &cp);
		hasAdded = true;
		break;
	}
	CA_FOREACH_END()
	// If not, create a new entry
	if (!hasAdded)
	{
		CharacterPlaces cps;
		cps.Index = ch;
		CArrayInit(&cps.Places, sizeof(CharacterPlace));
		CArrayPushBack(&cps.Places, &cp);
		CArrayPushBack(&m->Characters, &cps);
	}
}
bool MissionStaticTryRemoveCharacterAt(
	MissionStatic *m, const struct vec2i pos)
{
	CA_FOREACH(CharacterPlaces, cps, m->Characters)
	if (TryRemoveCharacterPlace(&cps->Places, pos))
	{
		if (cps->Places.size == 0)
		{
			CArrayDelete(&m->Characters, _ca_index);
		}
		return true;
	}
	CA_FOREACH_END()
	return false;
}
bool MissionStaticTryRotateCharacterAt(
	MissionStatic *m, const struct vec2i pos)
{
	CA_FOREACH(CharacterPlaces, cps, m->Characters)
	if (TryRotateCharacterPlace(&cps->Places, pos))
	{
		return true;
	}
	CA_FOREACH_END()
	return false;
}

void MissionStaticAddKey(MissionStatic *m, const int k, const struct vec2i pos)
{
	// Check if the item already has an entry, and add to its list
	// of positions
	bool hasAdded = false;
	CA_FOREACH(KeyPositions, kp, m->Keys)
	if (kp->Index == k)
	{
		CArrayPushBack(&kp->Positions, &pos);
		hasAdded = true;
		break;
	}
	CA_FOREACH_END()
	// If not, create a new entry
	if (!hasAdded)
	{
		KeyPositions kp;
		kp.Index = k;
		CArrayInit(&kp.Positions, sizeof(struct vec2i));
		CArrayPushBack(&kp.Positions, &pos);
		CArrayPushBack(&m->Keys, &kp);
	}
}
bool MissionStaticTryRemoveKeyAt(MissionStatic *m, const struct vec2i pos)
{
	CA_FOREACH(KeyPositions, kp, m->Keys)
	if (TryRemovePosition(&kp->Positions, pos))
	{
		if (kp->Positions.size == 0)
		{
			CArrayDelete(&m->Keys, _ca_index);
		}
		return true;
	}
	CA_FOREACH_END()
	return false;
}

bool MissionStaticTryAddPickup(
	MissionStatic *m, const PickupClass *p, const struct vec2i pos)
{
	// Check if there are existing pickups here
	CA_FOREACH(const PickupPositions, pp, m->Pickups)
	for (int i = 0; i < (int)pp->Positions.size; i++)
	{
		const struct vec2i *aPos = CArrayGet(&pp->Positions, i);
		if (svec2i_is_equal(*aPos, pos))
		{
			return false;
		}
	}
	CA_FOREACH_END()
	// Check if the item already has an entry, and add to its list
	// of positions
	CA_FOREACH(PickupPositions, pp, m->Pickups)
	if (pp->P == p)
	{
		CArrayPushBack(&pp->Positions, &pos);
		return true;
	}
	CA_FOREACH_END()
	// If not, create a new entry
	PickupPositions pp;
	pp.P = p;
	CArrayInit(&pp.Positions, sizeof(struct vec2i));
	CArrayPushBack(&pp.Positions, &pos);
	CArrayPushBack(&m->Pickups, &pp);
	return true;
}
bool MissionStaticTryRemovePickupAt(MissionStatic *m, const struct vec2i pos)
{
	CA_FOREACH(PickupPositions, pp, m->Pickups)
	if (TryRemovePosition(&pp->Positions, pos))
	{
		if (pp->Positions.size == 0)
		{
			CArrayDelete(&m->Pickups, _ca_index);
		}
		return true;
	}
	CA_FOREACH_END()
	return false;
}

static bool TryRemovePosition(CArray *positions, const struct vec2i pos)
{
	CA_FOREACH(const struct vec2i, aPos, *positions)
	if (svec2i_is_equal(*aPos, pos))
	{
		CArrayDelete(positions, _ca_index);
		if (positions->size == 0)
		{
			CArrayTerminate(positions);
		}
		return true;
	}
	CA_FOREACH_END()
	return false;
}
static bool TryRemoveCharacterPlace(CArray *cps, const struct vec2i pos)
{
	CA_FOREACH(const CharacterPlace, cp, *cps)
	if (svec2i_is_equal(cp->Pos, pos))
	{
		CArrayDelete(cps, _ca_index);
		if (cps->size == 0)
		{
			CArrayTerminate(cps);
		}
		return true;
	}
	CA_FOREACH_END()
	return false;
}
static bool TryRotateCharacterPlace(CArray *cps, const struct vec2i pos)
{
	CA_FOREACH(CharacterPlace, cp, *cps)
	if (svec2i_is_equal(cp->Pos, pos))
	{
		cp->Dir = DirectionRotate(cp->Dir, 1);
		return true;
	}
	CA_FOREACH_END()
	return false;
}

typedef struct
{
	MissionStatic *m;
	struct vec2i size;
	uint16_t tileAccess;
} MissionFloodFillData;
static void FloodFillSetAccess(void *data, const struct vec2i v);
static bool FloodFillIsAccessSame(void *data, const struct vec2i v);
bool MissionStaticTrySetKey(
	MissionStatic *m, const int k, const struct vec2i size,
	const struct vec2i pos)
{
	FloodFillData data;
	data.Fill = FloodFillSetAccess;
	data.IsSame = FloodFillIsAccessSame;
	MissionFloodFillData mData;
	mData.m = m;
	mData.size = size;
	mData.tileAccess = GetAccessMask(k);
	data.data = &mData;
	return CFloodFill(pos, &data);
}
static void FloodFillSetAccess(void *data, const struct vec2i v)
{
	MissionFloodFillData *mData = data;
	CArraySet(
		&mData->m->Access, mData->size.x * v.y + v.x, &mData->tileAccess);
}
static bool FloodFillIsAccessSame(void *data, const struct vec2i v)
{
	MissionFloodFillData *mData = data;
	if (!Rect2iIsInside(Rect2iNew(svec2i_zero(), mData->size), v))
	{
		return false;
	}
	const uint16_t tileAccess =
		*(uint16_t *)CArrayGet(&mData->m->Access, mData->size.x * v.y + v.x);
	const TileClass *tc = MissionStaticGetTileClass(mData->m, mData->size, v);
	const bool isDoor = tc->Type == TILE_CLASS_DOOR;
	const bool changeToUnlocked = mData->tileAccess == 0;
	return (!changeToUnlocked == isDoor) && tileAccess != mData->tileAccess;
}

bool MissionStaticTryUnsetKeyAt(
	MissionStatic *m, const struct vec2i size, const struct vec2i pos)
{
	// -1 for no access level
	return MissionStaticTrySetKey(m, -1, size, pos);
}

bool MissionStaticTryAddExit(MissionStatic *m, const Exit *exit)
{
	// Exits are sized 1, 1 larger than they are stored
	const Rect2i exitR =
		Rect2iNew(exit->R.Pos, svec2i_add(exit->R.Size, svec2i_one()));
	CA_FOREACH(const Exit, e, m->Exits)
	const Rect2i er = Rect2iNew(e->R.Pos, svec2i_add(e->R.Size, svec2i_one()));
	if (Rect2iOverlap(er, exitR))
	{
		return false;
	}
	CA_FOREACH_END()
	CArrayPushBack(&m->Exits, exit);
	return true;
}
