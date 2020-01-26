/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (c) 2014-2020 Cong Xu
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


static void MissionStaticInit(MissionStatic *m)
{
	memset(m, 0, sizeof *m);
	m->TileClasses = hashmap_new();
	CArrayInit(&m->Tiles, sizeof(int));
	CArrayInit(&m->Access, sizeof(int));
	CArrayInit(&m->Items, sizeof(MapObjectPositions));
	CArrayInit(&m->Characters, sizeof(CharacterPositions));
	CArrayInit(&m->Objectives, sizeof(ObjectivePositions));
	CArrayInit(&m->Keys, sizeof(KeyPositions));
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
static void LoadStaticObjectives(MissionStatic *m, json_t *node, char *name);
static void LoadStaticKeys(MissionStatic *m, json_t *node, char *name);
static void LoadStaticExit(MissionStatic *m, json_t *node, char *name);
bool MissionStaticTryLoadJSON(
	MissionStatic *m, json_t *node, const int version)
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
			const int tileAccess = *t & MAP_ACCESSBITS;
			CArrayPushBack(&m->Access, &tileAccess);
			*t &= MAP_MASKACCESS;
			switch (*t)
			{
				case MAP_FLOOR:
				case MAP_SQUARE:	// fallthrough
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
		const json_t *tile = json_find_first_label(node, "Tiles")->child->child;
		while (tile)
		{
			LoadStaticTileCSV(&m->Tiles, tile->text);
			tile = tile->next;
		}
		const json_t *a = json_find_first_label(node, "Access")->child->child;
		while (a)
		{
			LoadStaticTileCSV(&m->Access, a->text);
			a = a->next;
		}
	}

	LoadStaticItems(m, node, "StaticItems", version);
	if (version < 13)
	{
		LoadStaticWrecks(m, node, "StaticWrecks", version);
	}
	LoadStaticCharacters(m, node, "StaticCharacters");
	LoadStaticObjectives(m, node, "StaticObjectives");
	LoadStaticKeys(m, node, "StaticKeys");

	LoadVec2i(&m->Start, node, "Start");
	LoadStaticExit(m, node, "Exit");

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
		MissionLoadTileClass(tc, classNode);
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
		if (base->Name) CSTRDUP(tc->Name, base->Name);
		if (base->Style) CSTRDUP(tc->Style, base->Style);
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
		CArrayInit(&mop.Positions, sizeof(struct vec2i));
		json_t *positions = json_find_first_label(items, "Positions");
		if (!positions || !positions->child)
		{
			continue;
		}
		positions = positions->child;
		for (positions = positions->child;
			positions;
			positions = positions->next)
		{
			struct vec2i pos;
			json_t *position = positions->child;
			pos.x = atoi(position->text);
			position = position->next;
			pos.y = atoi(position->text);
			CArrayPushBack(&mop.Positions, &pos);
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
		CArrayInit(&mop.Positions, sizeof(struct vec2i));
		json_t *positions = json_find_first_label(items, "Positions");
		if (!positions || !positions->child)
		{
			continue;
		}
		positions = positions->child;
		for (positions = positions->child;
			positions;
			positions = positions->next)
		{
			struct vec2i pos;
			json_t *position = positions->child;
			pos.x = atoi(position->text);
			position = position->next;
			pos.y = atoi(position->text);
			CArrayPushBack(&mop.Positions, &pos);
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
	const MapObject *wreck = StrMapObject(mo->Wreck);
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
		CharacterPositions cp;
		LoadInt(&cp.Index, chars, "Index");
		CArrayInit(&cp.Positions, sizeof(struct vec2i));
		json_t *positions = json_find_first_label(chars, "Positions");
		if (!positions || !positions->child)
		{
			continue;
		}
		positions = positions->child;
		for (positions = positions->child;
			positions;
			positions = positions->next)
		{
			struct vec2i pos;
			json_t *position = positions->child;
			pos.x = atoi(position->text);
			position = position->next;
			pos.y = atoi(position->text);
			CArrayPushBack(&cp.Positions, &pos);
		}
		CArrayPushBack(&m->Characters, &cp);
	}
}
static void LoadStaticObjectives(MissionStatic *m, json_t *node, char *name)
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
		CArrayInit(&op.Positions, sizeof(struct vec2i));
		CArrayInit(&op.Indices, sizeof(int));
		json_t *positions = json_find_first_label(objs, "Positions");
		if (!positions || !positions->child)
		{
			continue;
		}
		positions = positions->child;
		for (positions = positions->child;
			 positions;
			 positions = positions->next)
		{
			struct vec2i pos;
			json_t *position = positions->child;
			pos.x = atoi(position->text);
			position = position->next;
			pos.y = atoi(position->text);
			CArrayPushBack(&op.Positions, &pos);
		}
		LoadIntArray(&op.Indices, objs, "Indices");
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
		CArrayInit(&kp.Positions, sizeof(struct vec2i));
		json_t *positions = json_find_first_label(keys, "Positions");
		if (!positions || !positions->child)
		{
			continue;
		}
		positions = positions->child;
		for (positions = positions->child;
			 positions;
			 positions = positions->next)
		{
			struct vec2i pos;
			json_t *position = positions->child;
			pos.x = atoi(position->text);
			position = position->next;
			pos.y = atoi(position->text);
			CArrayPushBack(&kp.Positions, &pos);
		}
		CArrayPushBack(&m->Keys, &kp);
	}
}
static void LoadStaticExit(MissionStatic *m, json_t *node, char *name)
{
	json_t *exitNode = json_find_first_label(node, name);
	if (!exitNode || !exitNode->child)
	{
		return;
	}
	exitNode = exitNode->child;
	LoadVec2i(&m->Exit.Start, exitNode, "Start");
	LoadVec2i(&m->Exit.End, exitNode, "End");
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
}

static json_t *SaveStaticTileClasses(const MissionStatic *m);
static json_t *SaveStaticCSV(const CArray *values, const struct vec2i size);
static json_t *SaveStaticItems(const MissionStatic *m);
static json_t *SaveStaticCharacters(const MissionStatic *m);
static json_t *SaveStaticObjectives(const MissionStatic *m);
static json_t *SaveStaticKeys(const MissionStatic *m);
static json_t *SaveVec2i(struct vec2i v);
void MissionStaticSaveJSON(
	const MissionStatic *m, const struct vec2i size, json_t *node)
{
	json_insert_pair_into_object(node, "TileClasses", SaveStaticTileClasses(m));
	json_insert_pair_into_object(node, "Tiles", SaveStaticCSV(&m->Tiles, size));
	json_insert_pair_into_object(
		node, "Access", SaveStaticCSV(&m->Access, size));
	json_insert_pair_into_object(node, "StaticItems", SaveStaticItems(m));
	json_insert_pair_into_object(
		node, "StaticCharacters", SaveStaticCharacters(m));
	json_insert_pair_into_object(
		node, "StaticObjectives", SaveStaticObjectives(m));
	json_insert_pair_into_object(node, "StaticKeys", SaveStaticKeys(m));

	json_insert_pair_into_object(node, "Start", SaveVec2i(m->Start));
	json_t *exitNode = json_new_object();
	json_insert_pair_into_object(exitNode, "Start", SaveVec2i(m->Exit.Start));
	json_insert_pair_into_object(exitNode, "End", SaveVec2i(m->Exit.End));
	json_insert_pair_into_object(node, "Exit", exitNode);
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
	SaveStaticTileClassData data = { items, m->TileClasses };
	if (hashmap_iterate_keys(
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
	const int error = hashmap_get(
		sData->tileClasses, (const char *)key, (any_t *)&tc);
	if (error != MAP_OK)
	{
		CASSERT(false, "cannot find tile class");
		return error;
	}
	json_insert_pair_into_object(
		sData->items, (const char *)key, MissionSaveTileClass(tc));
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
static json_t *SaveStaticItems(const MissionStatic *m)
{
	json_t *items = json_new_array();
	CA_FOREACH(MapObjectPositions, mop, m->Items)
		json_t *itemNode = json_new_object();
		AddStringPair(itemNode, "MapObject", mop->M->Name);
		json_t *positions = json_new_array();
		for (int j = 0; j < (int)mop->Positions.size; j++)
		{
			struct vec2i *pos = CArrayGet(&mop->Positions, j);
			json_insert_child(positions, SaveVec2i(*pos));
		}
		json_insert_pair_into_object(
			itemNode, "Positions", positions);
		json_insert_child(items, itemNode);
	CA_FOREACH_END()
	return items;
}
static json_t *SaveStaticCharacters(const MissionStatic *m)
{
	json_t *chars = json_new_array();
	CA_FOREACH(CharacterPositions, cp, m->Characters)
		json_t *charNode = json_new_object();
		AddIntPair(charNode, "Index", cp->Index);
		json_t *positions = json_new_array();
		for (int j = 0; j < (int)cp->Positions.size; j++)
		{
			struct vec2i *pos = CArrayGet(&cp->Positions, j);
			json_insert_child(positions, SaveVec2i(*pos));
		}
		json_insert_pair_into_object(
			charNode, "Positions", positions);
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
		for (int j = 0; j < (int)op->Positions.size; j++)
		{
			struct vec2i *pos = CArrayGet(&op->Positions, j);
			json_insert_child(positions, SaveVec2i(*pos));
		}
		json_insert_pair_into_object(
			objNode, "Positions", positions);
		AddIntArray(objNode, "Indices", &op->Indices);
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
		json_t *positions = json_new_array();
		for (int j = 0; j < (int)kp->Positions.size; j++)
		{
			struct vec2i *pos = CArrayGet(&kp->Positions, j);
			json_insert_child(positions, SaveVec2i(*pos));
		}
		json_insert_pair_into_object(
			keyNode, "Positions", positions);
		json_insert_child(keys, keyNode);
	CA_FOREACH_END()
	return keys;
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

static any_t TileClassCopyHashMap(any_t in);
static void MapObjectPositionsCopy(CArray *dst, const CArray *src);
static void CharacterPositionsCopy(CArray *dst, const CArray *src);
static void ObjectivePositionsCopy(CArray *dst, const CArray *src);
static void KeyPositionsCopy(CArray *dst, const CArray *src);
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
}
static any_t TileClassCopyHashMap(any_t in)
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
	CA_FOREACH(const CharacterPositions, p, *src)
		CharacterPositions pCopy;
		memset(&pCopy, 0, sizeof pCopy);
		pCopy.Index = p->Index;
		CArrayCopy(&pCopy.Positions, &p->Positions);
		CArrayPushBack(dst, &pCopy);
	CA_FOREACH_END()
}
static void ObjectivePositionsCopy(CArray *dst, const CArray *src)
{
	CArrayInit(dst, src->elemSize);
	CA_FOREACH(const ObjectivePositions, p, *src)
		ObjectivePositions pCopy;
		memset(&pCopy, 0, sizeof pCopy);
		pCopy.Index = p->Index;
		CArrayCopy(&pCopy.Positions, &p->Positions);
		CArrayCopy(&pCopy.Indices, &p->Indices);
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

int MissionStaticGetTile(
	const MissionStatic *m, const struct vec2i size, const struct vec2i pos)
{
	CASSERT(
		(int)m->Tiles.size == size.x * size.y,
		"static mission tiles size mismatch");
	return *(int *)CArrayGet(&m->Tiles, size.x * pos.y + pos.x);
}
const TileClass *MissionStaticGetTileClass(
	const MissionStatic *m, const struct vec2i size, const struct vec2i pos)
{
	const int tile = MissionStaticGetTile(m, size, pos);
	return MissionStaticIdTileClass(m, tile);
}
const TileClass *MissionStaticIdTileClass(
	const MissionStatic *m, const int tile)
{
	char keyBuf[6];
	sprintf(keyBuf, "%d", tile);
	const TileClass *tc;
	if (hashmap_get(m->TileClasses, keyBuf, (any_t)&tc) != MAP_OK)
	{
		CASSERT(false, "cannot find tile id");
	}
	return tc;
}
static bool HasDoorOrientedAt(
	const MissionStatic *m, const struct vec2i size, const struct vec2i pos,
	const bool isHorizontal);
static bool IsClear(
	const MissionStatic *m, const struct vec2i size, const struct vec2i pos);
bool MissionStaticTrySetTile(
	MissionStatic *m, const struct vec2i size, const struct vec2i pos,
	const int tile)
{
	if (!Rect2iIsInside(Rect2iNew(svec2i_zero(), size), pos))
	{
		return false;
	}
	const TileClass *tc = MissionStaticGetTileClass(m, size, pos);
	switch (tc->Type)
	{
	case TILE_CLASS_WALL:
		// Check that there are no incompatible doors
		if (HasDoorOrientedAt(m, size,  svec2i(pos.x - 1, pos.y), false) ||
			HasDoorOrientedAt(m, size,  svec2i(pos.x + 1, pos.y), false) ||
			HasDoorOrientedAt(m, size,  svec2i(pos.x, pos.y - 1), true) ||
			HasDoorOrientedAt(m, size,  svec2i(pos.x, pos.y + 1), true))
		{
			// Can't place this wall
			return false;
		}
		break;
	case TILE_CLASS_DOOR:
		{
			// Check that there is a clear passage through this door
			const bool isHClear =
				IsClear(m, size, svec2i(pos.x - 1, pos.y)) &&
				IsClear(m, size, svec2i(pos.x + 1, pos.y));
			const bool isVClear =
				IsClear(m, size, svec2i(pos.x, pos.y - 1)) &&
				IsClear(m, size, svec2i(pos.x, pos.y + 1));
			if (!isHClear && !isVClear)
			{
				return false;
			}
			// Check that there are no incompatible doors
			if (HasDoorOrientedAt(m, size,  svec2i(pos.x - 1, pos.y), false) ||
				HasDoorOrientedAt(m, size,  svec2i(pos.x + 1, pos.y), false) ||
				HasDoorOrientedAt(m, size,  svec2i(pos.x, pos.y - 1), true) ||
				HasDoorOrientedAt(m, size,  svec2i(pos.x, pos.y + 1), true))
			{
				// Can't place this door
				return false;
			}
		}
		break;
	default:
		break;
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
	char buf[12];
	snprintf(buf, sizeof buf - 1, "%d", hashmap_length(m->TileClasses));
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

static bool IsClear(
	const MissionStatic *m, const struct vec2i size, const struct vec2i pos)
{
	const TileClass *tc = MissionStaticGetTileClass(m, size, pos);
	return tc->canWalk;
}
// See if the tile located at a position is a door and also needs
// to be oriented in a certain way
// If there are walls or doors in the neighbourhood, they can force a certain
// orientation of the door
static bool HasDoorOrientedAt(
	const MissionStatic *m, const struct vec2i size, const struct vec2i pos,
	const bool isHorizontal)
{
	if (!Rect2iIsInside(Rect2iNew(svec2i_zero(), size), pos))
	{
		return false;
	}
	const TileClass *tc = MissionStaticGetTileClass(m, size, pos);
	if (tc->Type != TILE_CLASS_DOOR)
	{
		return false;
	}
	// Check for walls and doors that force the orientation of the door
	if (!MissionStaticGetTileClass(m, size, svec2i(pos.x - 1, pos.y))->canWalk ||
		!MissionStaticGetTileClass(m, size, svec2i(pos.x + 1, pos.y))->canWalk)
	{
		// There is a horizontal door
		return isHorizontal;
	}
	else if (
		!MissionStaticGetTileClass(m, size, svec2i(pos.x, pos.y - 1))->canWalk ||
		!MissionStaticGetTileClass(m, size, svec2i(pos.x, pos.y + 1))->canWalk)
	{
		// There is a vertical door
		return !isHorizontal;
	}
	// There is a door but it is free to be oriented in any way
	return false;
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
	const int firstTile = MissionStaticGetTile(m, oldSize, svec2i_zero());
	CArrayResize(&m->Tiles, size.x * size.y, &firstTile);
	CArrayFillZero(&m->Tiles);
	const int noAccess = 0;
	CArrayResize(&m->Access, size.x * size.y, &noAccess);
	CArrayFillZero(&m->Access);

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
			const int *a = CArrayGet(&oldAccess, idx);
			CArraySet(&m->Access, _v.y * size.x + _v.x, a);
		}
	RECT_FOREACH_END()

	CArrayTerminate(&oldTiles);

	m->Start = svec2i_clamp(
		m->Start, svec2i_zero(), svec2i_subtract(size, svec2i_one()));
}

static bool TryAddMapObject(
	const MapObject *mo, const struct vec2i pos, CArray *objs);
static bool TryRemoveMapObjectAt(const struct vec2i pos, CArray *objs);
bool MissionStaticTryAddItem(
	MissionStatic *m, const MapObject *mo, const struct vec2i pos)
{
	return TryAddMapObject(mo, pos, &m->Items);
}
bool MissionStaticTryRemoveItemAt(MissionStatic *m, const struct vec2i pos)
{
	return TryRemoveMapObjectAt(pos, &m->Items);
}
static bool TryAddMapObject(
	const MapObject *mo, const struct vec2i pos, CArray *objs)
{
	const Tile *tile = MapGetTile(&gMap, pos);
	const Tile *tileAbove = MapGetTile(&gMap, svec2i(pos.x, pos.y - 1));

	// Remove any items already there
	TryRemoveMapObjectAt(pos, objs);

	if (MapObjectIsTileOK(mo, tile, tileAbove))
	{
		// Check if the item already has an entry, and add to its list
		// of positions
		bool hasAdded = false;
		for (int i = 0; i < (int)objs->size; i++)
		{
			MapObjectPositions *mop = CArrayGet(objs, i);
			if (mop->M == mo)
			{
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
			CArrayPushBack(objs, &mop);
		}
		return true;
	}
	return false;
}
static bool TryRemoveMapObjectAt(const struct vec2i pos, CArray *objs)
{
	for (int i = 0; i < (int)objs->size; i++)
	{
		MapObjectPositions *mop = CArrayGet(objs, i);
		for (int j = 0; j < (int)mop->Positions.size; j++)
		{
			const struct vec2i *mopPos = CArrayGet(&mop->Positions, j);
			if (svec2i_is_equal(*mopPos, pos))
			{
				CArrayDelete(&mop->Positions, j);
				if (mop->Positions.size == 0)
				{
					CArrayTerminate(&mop->Positions);
					CArrayDelete(objs, i);
				}
				return true;
			}
		}
	}
	return false;
}

bool MissionStaticTryAddCharacter(
	MissionStatic *m, const int ch, const struct vec2i pos)
{
	// Remove any characters already there
	MissionStaticTryRemoveCharacterAt(m, pos);

	const Tile *tile = MapGetTile(&gMap, pos);
	if (TileIsClear(tile))
	{
		// Check if the character already has an entry, and add to its list
		// of positions
		bool hasAdded = false;
		CA_FOREACH(CharacterPositions, cp, m->Characters)
			if (cp->Index == ch)
			{
				CArrayPushBack(&cp->Positions, &pos);
				hasAdded = true;
				break;
			}
		CA_FOREACH_END()
		// If not, create a new entry
		if (!hasAdded)
		{
			CharacterPositions cp;
			cp.Index = ch;
			CArrayInit(&cp.Positions, sizeof(struct vec2i));
			CArrayPushBack(&cp.Positions, &pos);
			CArrayPushBack(&m->Characters, &cp);
		}
		return true;
	}
	return false;
}
bool MissionStaticTryRemoveCharacterAt(
	MissionStatic *m, const struct vec2i pos)
{
	CA_FOREACH(CharacterPositions, cp, m->Characters)
		for (int j = 0; j < (int)cp->Positions.size; j++)
		{
			struct vec2i *cpPos = CArrayGet(&cp->Positions, j);
			if (svec2i_is_equal(*cpPos, pos))
			{
				CArrayDelete(&cp->Positions, j);
				if (cp->Positions.size == 0)
				{
					CArrayTerminate(&cp->Positions);
					CArrayDelete(&m->Characters, _ca_index);
				}
				return true;
			}
		}
	CA_FOREACH_END()
	return false;
}

bool MissionStaticTryAddObjective(
	MissionStatic *m, const int idx, const int idx2, const struct vec2i pos)
{
	// Remove any objectives already there
	MissionStaticTryRemoveObjectiveAt(m, pos);

	const Tile *tile = MapGetTile(&gMap, pos);
	if (TileIsClear(tile))
	{
		// Check if the objective already has an entry, and add to its list
		// of positions
		int hasAdded = 0;
		ObjectivePositions *op = NULL;
		for (int i = 0; i < (int)m->Objectives.size; i++)
		{
			op = CArrayGet(&m->Objectives, i);
			if (op->Index == idx)
			{
				CArrayPushBack(&op->Positions, &pos);
				CArrayPushBack(&op->Indices, &idx2);
				hasAdded = 1;
				break;
			}
		}
		// If not, create a new entry
		if (!hasAdded)
		{
			ObjectivePositions newOp;
			newOp.Index = idx;
			CArrayInit(&newOp.Positions, sizeof(struct vec2i));
			CArrayInit(&newOp.Indices, sizeof(int));
			CArrayPushBack(&newOp.Positions, &pos);
			CArrayPushBack(&newOp.Indices, &idx2);
			CArrayPushBack(&m->Objectives, &newOp);
		}
		// Increase number of objectives
		Objective *o = CArrayGet(&m->Objectives, idx);
		o->Count++;
		return true;
	}
	return false;
}
bool MissionStaticTryRemoveObjectiveAt(
	MissionStatic *m, const struct vec2i pos)
{
	CA_FOREACH(ObjectivePositions, op, m->Objectives)
		for (int j = 0; j < (int)op->Positions.size; j++)
		{
			struct vec2i *opPos = CArrayGet(&op->Positions, j);
			if (svec2i_is_equal(*opPos, pos))
			{
				CArrayDelete(&op->Positions, j);
				CArrayDelete(&op->Indices, j);
				// Decrease number of objectives
				Objective *o = CArrayGet(&m->Objectives, op->Index);
				o->Count--;
				CASSERT(o->Count >= 0, "removing unknown objective");
				if (op->Positions.size == 0)
				{
					CArrayTerminate(&op->Positions);
					CArrayTerminate(&op->Indices);
					CArrayDelete(&m->Objectives, _ca_index);
				}
				return true;
			}
		}
	CA_FOREACH_END()
	return false;
}

bool MissionStaticTryAddKey(
	MissionStatic *m, const int k, const struct vec2i pos)
{
	// Remove any keys already there
	MissionStaticTryRemoveKeyAt(m, pos);

	const Tile *tile = MapGetTile(&gMap, pos);
	if (TileIsClear(tile))
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
		return true;
	}
	return false;
}
bool MissionStaticTryRemoveKeyAt(MissionStatic *m, const struct vec2i pos)
{
	CA_FOREACH(KeyPositions, kp, m->Keys)
		for (int j = 0; j < (int)kp->Positions.size; j++)
		{
			struct vec2i *kpPos = CArrayGet(&kp->Positions, j);
			if (svec2i_is_equal(*kpPos, pos))
			{
				CArrayDelete(&kp->Positions, j);
				if (kp->Positions.size == 0)
				{
					CArrayTerminate(&kp->Positions);
					CArrayDelete(&m->Keys, _ca_index);
				}
				return true;
			}
		}
	CA_FOREACH_END()
	return false;
}

typedef struct
{
	MissionStatic *m;
	struct vec2i size;
	int tileAccess;
} MissionFloodFillData;
static void FloodFillSetAccess(void *data, const struct vec2i v);
static bool FloodFillIsAccessSame(void *data, const struct vec2i v);
bool MissionStaticTrySetKey(
	MissionStatic *m, const int k,
	const struct vec2i size, const struct vec2i pos)
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
	CArraySet(&mData->m->Access, mData->size.x * v.y + v.x, &mData->tileAccess);
}
static bool FloodFillIsAccessSame(void *data, const struct vec2i v)
{
	MissionFloodFillData *mData = data;
	const int tileAccess = *(int *)CArrayGet(
		&mData->m->Access, mData->size.x * v.y + v.x);
	const TileClass *tc = MissionStaticGetTileClass(mData->m, mData->size, v);
	return tc->Type == TILE_CLASS_DOOR && tileAccess != mData->tileAccess;
}

bool MissionStaticTryUnsetKeyAt(
	MissionStatic *m, const struct vec2i size, const struct vec2i pos)
{
	// -1 for no access level
	return MissionStaticTrySetKey(m, -1, size, pos);
}
