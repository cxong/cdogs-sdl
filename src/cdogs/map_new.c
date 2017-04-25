/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (c) 2014-2017 Cong Xu
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
#include "map_new.h"

#include <assert.h>
#include <locale.h>
#include <stdio.h>

#include "door.h"
#include "files.h"
#include "json_utils.h"
#include "log.h"
#include "map_archive.h"


int MapNewScan(const char *filename, char **title, int *numMissions)
{
	int err = 0;
	json_t *root = NULL;
	FILE *f = NULL;

	if (strcmp(StrGetFileExt(filename), "cdogscpn") == 0 ||
		strcmp(StrGetFileExt(filename), "CDOGSCPN") == 0)
	{
		return MapNewScanArchive(filename, title, numMissions);
	}
	if (IsCampaignOldFile(filename))
	{
		return ScanCampaignOld(filename, title, numMissions);
	}
	f = fopen(filename, "r");
	if (f == NULL)
	{
		debug(D_NORMAL, "MapNewLoad - invalid path!\n");
		err = -1;
		goto bail;
	}
	if (json_stream_parse(f, &root) != JSON_OK)
	{
		err = -1;
		goto bail;
	}
	err = MapNewScanJSON(root, title, numMissions);
	if (err < 0)
	{
		goto bail;
	}

bail:
	if (f)
	{
		fclose(f);
	}
	json_free_value(&root);
	return err;
}
int MapNewScanJSON(json_t *root, char **title, int *numMissions)
{
	int err = 0;
	int version;
	LoadInt(&version, root, "Version");
	if (version > MAP_VERSION || version <= 0)
	{
		err = -1;
		goto bail;
	}
	*title = GetString(root, "Title");
	*numMissions = 0;
	if (version < 3)
	{
		for (json_t *missionNode =
			json_find_first_label(root, "Missions")->child->child;
			missionNode;
			missionNode = missionNode->next)
		{
			(*numMissions)++;
		}
	}
	else
	{
		LoadInt(numMissions, root, "Missions");
	}

bail:
	return err;
}

int MapNewLoad(const char *filename, CampaignSetting *c)
{
	int err = 0;

	debug(D_NORMAL, "Loading map %s\n", filename);

	if (IsCampaignOldFile(filename))
	{
		CampaignSettingOld cOld;
		memset(&cOld, 0, sizeof cOld);
		err = LoadCampaignOld(filename, &cOld);
		if (!err)
		{
			ConvertCampaignSetting(c, &cOld);
		}
		CFREE(cOld.missions);
		CFREE(cOld.characters);
		return err;
	}

	if (strcmp(StrGetFileExt(filename), "cdogscpn") == 0 ||
		strcmp(StrGetFileExt(filename), "CDOGSCPN") == 0)
	{
		return MapNewLoadArchive(filename, c);
	}

	// try to load the new map format
	json_t *root = NULL;
	int version;
	FILE *f = fopen(filename, "r");
	if (f == NULL)
	{
		debug(D_NORMAL, "MapNewLoad - invalid path!\n");
		err = -1;
		goto bail;
	}
	if (json_stream_parse(f, &root) != JSON_OK)
	{
		printf("Error parsing campaign '%s'\n", filename);
		err = -1;
		goto bail;
	}
	LoadInt(&version, root, "Version");
	if (version > 2 || version <= 0)
	{
		assert(0 && "not implemented or unknown campaign");
		err = -1;
		goto bail;
	}
	MapNewLoadCampaignJSON(root, c);
	LoadMissions(&c->Missions, json_find_first_label(root, "Missions")->child, version);
	CharacterLoadJSON(&c->characters, root, version);

bail:
	json_free_value(&root);
	if (f != NULL)
	{
		fclose(f);
	}
	return err;
}

void MapNewLoadCampaignJSON(json_t *root, CampaignSetting *c)
{
	CFREE(c->Title);
	c->Title = GetString(root, "Title");
	CFREE(c->Author);
	c->Author = GetString(root, "Author");
	CFREE(c->Description);
	c->Description = GetString(root, "Description");
}

static void LoadMissionObjectives(
	CArray *objectives, json_t *objectivesNode, const int version);
static void LoadIntArray(CArray *a, json_t *node, char *name);
static void LoadWeapons(CArray *weapons, json_t *weaponsNode);
static void LoadClassicRooms(Mission *m, json_t *roomsNode);
static void LoadClassicDoors(Mission *m, json_t *node, char *name);
static void LoadClassicPillars(Mission *m, json_t *node, char *name);
static bool TryLoadStaticMap(Mission *m, json_t *node, int version);
void LoadMissions(CArray *missions, json_t *missionsNode, int version)
{
	json_t *child;
	for (child = missionsNode->child; child; child = child->next)
	{
		Mission m;
		MissionInit(&m);
		m.Title = GetString(child, "Title");
		m.Description = GetString(child, "Description");
		JSON_UTILS_LOAD_ENUM(m.Type, child, "Type", StrMapType);
		LoadInt(&m.Size.x, child, "Width");
		LoadInt(&m.Size.y, child, "Height");
		if (version <= 10)
		{
			int style;
			LoadInt(&style, child, "WallStyle");
			strcpy(m.WallStyle, IntWallStyle(style));
			LoadInt(&style, child, "FloorStyle");
			strcpy(m.FloorStyle, IntFloorStyle(style));
			LoadInt(&style, child, "RoomStyle");
			strcpy(m.RoomStyle, IntRoomStyle(style));
		}
		else
		{
			char *tmp = GetString(child, "WallStyle");
			strcpy(m.WallStyle, tmp);
			CFREE(tmp);
			tmp = GetString(child, "FloorStyle");
			strcpy(m.FloorStyle, tmp);
			CFREE(tmp);
			tmp = GetString(child, "RoomStyle");
			strcpy(m.RoomStyle, tmp);
			CFREE(tmp);
		}
		if (version <= 9)
		{
			int style;
			LoadInt(&style, child, "ExitStyle");
			strcpy(m.ExitStyle, IntExitStyle(style));
		}
		else
		{
			char *tmp = GetString(child, "ExitStyle");
			strcpy(m.ExitStyle, tmp);
			CFREE(tmp);
		}
		if (version <= 8)
		{
			int keyStyle;
			LoadInt(&keyStyle, child, "KeyStyle");
			strcpy(m.KeyStyle, IntKeyStyle(keyStyle));
		}
		else
		{
			char *tmp = GetString(child, "KeyStyle");
			strcpy(m.KeyStyle, tmp);
			CFREE(tmp);
		}
		if (version <= 5)
		{
			int doorStyle;
			LoadInt(&doorStyle, child, "DoorStyle");
			strcpy(m.DoorStyle, IntDoorStyle(doorStyle));
		}
		else
		{
			char *tmp = GetString(child, "DoorStyle");
			strcpy(m.DoorStyle, tmp);
			CFREE(tmp);
		}
		LoadMissionObjectives(
			&m.Objectives,
			json_find_first_label(child, "Objectives")->child,
			version);
		LoadIntArray(&m.Enemies, child, "Enemies");
		LoadIntArray(&m.SpecialChars, child, "SpecialChars");
		if (version <= 3)
		{
			CArray items;
			CArrayInit(&items, sizeof(int));
			LoadIntArray(&items, child, "Items");
			CArray densities;
			CArrayInit(&densities, sizeof(int));
			LoadIntArray(&densities, child, "ItemDensities");
			for (int i = 0; i < (int)items.size; i++)
			{
				MapObjectDensity mod;
				mod.M = IntMapObject(*(int *)CArrayGet(&items, i));
				mod.Density = *(int *)CArrayGet(&densities, i);
				CArrayPushBack(&m.MapObjectDensities, &mod);
			}
		}
		else
		{
			json_t *modsNode =
				json_find_first_label(child, "MapObjectDensities");
			if (modsNode && modsNode->child)
			{
				modsNode = modsNode->child;
				for (json_t *modNode = modsNode->child;
					modNode;
					modNode = modNode->next)
				{
					MapObjectDensity mod;
					mod.M = StrMapObject(
						json_find_first_label(modNode, "MapObject")->child->text);
					LoadInt(&mod.Density, modNode, "Density");
					CArrayPushBack(&m.MapObjectDensities, &mod);
				}
			}
		}
		LoadInt(&m.EnemyDensity, child, "EnemyDensity");
		LoadWeapons(
			&m.Weapons, json_find_first_label(child, "Weapons")->child);
		strcpy(m.Song, json_find_first_label(child, "Song")->child->text);
		if (version <= 4)
		{
			// Load colour indices
			int wc, fc, rc, ac;
			LoadInt(&wc, child, "WallColor");
			LoadInt(&fc, child, "FloorColor");
			LoadInt(&rc, child, "RoomColor");
			LoadInt(&ac, child, "AltColor");
			m.WallMask = RangeToColor(wc);
			m.FloorMask = RangeToColor(fc);
			m.RoomMask = RangeToColor(rc);
			m.AltMask = RangeToColor(ac);
		}
		else
		{
			LoadColor(&m.WallMask, child, "WallMask");
			LoadColor(&m.FloorMask, child, "FloorMask");
			LoadColor(&m.RoomMask, child, "RoomMask");
			LoadColor(&m.AltMask, child, "AltMask");
		}
		switch (m.Type)
		{
		case MAPTYPE_CLASSIC:
			LoadInt(&m.u.Classic.Walls, child, "Walls");
			LoadInt(&m.u.Classic.WallLength, child, "WallLength");
			LoadInt(&m.u.Classic.CorridorWidth, child, "CorridorWidth");
			LoadClassicRooms(
				&m, json_find_first_label(child, "Rooms")->child);
			LoadInt(&m.u.Classic.Squares, child, "Squares");
			LoadClassicDoors(&m, child, "Doors");
			LoadClassicPillars(&m, child, "Pillars");
			break;
		case MAPTYPE_STATIC:
			if (!TryLoadStaticMap(&m, child, version))
			{
				continue;
			}
			break;
		case MAPTYPE_CAVE:
			LoadInt(&m.u.Cave.FillPercent, child, "FillPercent");
			LoadInt(&m.u.Cave.Repeat, child, "Repeat");
			LoadInt(&m.u.Cave.R1, child, "R1");
			LoadInt(&m.u.Cave.R2, child, "R2");
			LoadInt(&m.u.Cave.Squares, child, "Squares");
			break;
		default:
			assert(0 && "unknown map type");
			continue;
		}
		CArrayPushBack(missions, &m);
	}
}
static void LoadStaticItems(
	Mission *m, json_t *node, const char *name, const int version);
static void LoadStaticCharacters(Mission *m, json_t *node, char *name);
static void LoadStaticObjectives(Mission *m, json_t *node, char *name);
static void LoadStaticKeys(Mission *m, json_t *node, char *name);
static void LoadStaticExit(Mission *m, json_t *node, char *name);
static bool TryLoadStaticMap(Mission *m, json_t *node, int version)
{
	CArrayInit(&m->u.Static.Tiles, sizeof(unsigned short));
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
			unsigned short n = (unsigned short)atoi(tiles->text);
			CArrayPushBack(&m->u.Static.Tiles, &n);
		}
	}
	else
	{
		// CSV string
		char *tileCSV = GetString(node, "Tiles");
		char *pch = strtok(tileCSV, ",");
		while (pch != NULL)
		{
			unsigned short n = (unsigned short)atoi(pch);
			CArrayPushBack(&m->u.Static.Tiles, &n);
			pch = strtok(NULL, ",");
		}
		CFREE(tileCSV);
	}

	CArrayInit(&m->u.Static.Items, sizeof(MapObjectPositions));
	LoadStaticItems(m, node, "StaticItems", version);
	if (version < 13)
	{
		LoadStaticItems(m, node, "StaticWrecks", version);
	}
	LoadStaticCharacters(m, node, "StaticCharacters");
	LoadStaticObjectives(m, node, "StaticObjectives");
	LoadStaticKeys(m, node, "StaticKeys");

	LoadVec2i(&m->u.Static.Start, node, "Start");
	LoadStaticExit(m, node, "Exit");

	return true;
}

static void LoadMissionObjectives(
	CArray *objectives, json_t *objectivesNode, const int version)
{
	json_t *child;
	for (child = objectivesNode->child; child; child = child->next)
	{
		Objective o;
		ObjectiveLoadJSON(&o, child, version);
		CArrayPushBack(objectives, &o);
	}
}
static void LoadIntArray(CArray *a, json_t *node, char *name)
{
	json_t *child = json_find_first_label(node, name);
	if (!child || !child->child)
	{
		return;
	}
	child = child->child;
	for (child = child->child; child; child = child->next)
	{
		int n = atoi(child->text);
		CArrayPushBack(a, &n);
	}
}
static void AddWeapon(CArray *weapons, const CArray *guns);
static void LoadWeapons(CArray *weapons, json_t *weaponsNode)
{
	if (!weaponsNode->child)
	{
		// enable all weapons
		AddWeapon(weapons, &gGunDescriptions.Guns);
		AddWeapon(weapons, &gGunDescriptions.CustomGuns);
	}
	else
	{
		for (json_t *child = weaponsNode->child; child; child = child->next)
		{
			const GunDescription *g = StrGunDescription(child->text);
			if (g == NULL)
			{
				continue;
			}
			CArrayPushBack(weapons, &g);
		}
	}
}
static void AddWeapon(CArray *weapons, const CArray *guns)
{
	for (int i = 0; i < (int)guns->size; i++)
	{
		const GunDescription *g = CArrayGet(guns, i);
		if (g->IsRealGun)
		{
			CArrayPushBack(weapons, &g);
		}
	}
}
static void LoadClassicRooms(Mission *m, json_t *roomsNode)
{
	LoadInt(&m->u.Classic.Rooms.Count, roomsNode, "Count");
	LoadInt(&m->u.Classic.Rooms.Min, roomsNode, "Min");
	LoadInt(&m->u.Classic.Rooms.Max, roomsNode, "Max");
	LoadBool(&m->u.Classic.Rooms.Edge, roomsNode, "Edge");
	LoadBool(&m->u.Classic.Rooms.Overlap, roomsNode, "Overlap");
	LoadInt(&m->u.Classic.Rooms.Walls, roomsNode, "Walls");
	LoadInt(&m->u.Classic.Rooms.WallLength, roomsNode, "WallLength");
	LoadInt(&m->u.Classic.Rooms.WallPad, roomsNode, "WallPad");
}
static void LoadClassicPillars(Mission *m, json_t *node, char *name)
{
	json_t *child = json_find_first_label(node, name);
	if (!child || !child->child)
	{
		return;
	}
	child = child->child;
	LoadInt(&m->u.Classic.Pillars.Count, child, "Count");
	LoadInt(&m->u.Classic.Pillars.Min, child, "Min");
	LoadInt(&m->u.Classic.Pillars.Max, child, "Max");
}
static void LoadClassicDoors(Mission *m, json_t *node, char *name)
{
	json_t *child = json_find_first_label(node, name);
	if (!child || !child->child)
	{
		return;
	}
	child = child->child;
	LoadBool(&m->u.Classic.Doors.Enabled, child, "Enabled");
	LoadInt(&m->u.Classic.Doors.Min, child, "Min");
	LoadInt(&m->u.Classic.Doors.Max, child, "Max");
}
static const MapObject *LoadMapObjectRef(json_t *node, const int version);
static void LoadStaticItems(
	Mission *m, json_t *node, const char *name, const int version)
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
		CArrayInit(&mop.Positions, sizeof(Vec2i));
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
			Vec2i pos;
			json_t *position = positions->child;
			pos.x = atoi(position->text);
			position = position->next;
			pos.y = atoi(position->text);
			CArrayPushBack(&mop.Positions, &pos);
		}
		CArrayPushBack(&m->u.Static.Items, &mop);
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
			char buf[256];
			// Old version had same name for ammo and gun spawner
			char itemName[256];
			strncpy(itemName, moName, strlen(moName) - strlen(" spawner"));
			itemName[strlen(moName) - strlen(" spawner")] = '\0';
			snprintf(buf, 256, "%s ammo spawner", itemName);
			mo = StrMapObject(buf);
		}
		if (mo == NULL)
		{
			LOG(LM_MAP, LL_ERROR, "Failed to load map object (%s)", moName);
		}
		return mo;
	}
}
static void LoadStaticCharacters(Mission *m, json_t *node, char *name)
{
	CArrayInit(&m->u.Static.Characters, sizeof(CharacterPositions));

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
		CArrayInit(&cp.Positions, sizeof(Vec2i));
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
			Vec2i pos;
			json_t *position = positions->child;
			pos.x = atoi(position->text);
			position = position->next;
			pos.y = atoi(position->text);
			CArrayPushBack(&cp.Positions, &pos);
		}
		CArrayPushBack(&m->u.Static.Characters, &cp);
	}
}
static void LoadStaticObjectives(Mission *m, json_t *node, char *name)
{
	CArrayInit(&m->u.Static.Objectives, sizeof(ObjectivePositions));
	
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
		CArrayInit(&op.Positions, sizeof(Vec2i));
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
			Vec2i pos;
			json_t *position = positions->child;
			pos.x = atoi(position->text);
			position = position->next;
			pos.y = atoi(position->text);
			CArrayPushBack(&op.Positions, &pos);
		}
		LoadIntArray(&op.Indices, objs, "Indices");
		CArrayPushBack(&m->u.Static.Objectives, &op);
	}
}
static void LoadStaticKeys(Mission *m, json_t *node, char *name)
{
	CArrayInit(&m->u.Static.Keys, sizeof(KeyPositions));
	
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
		CArrayInit(&kp.Positions, sizeof(Vec2i));
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
			Vec2i pos;
			json_t *position = positions->child;
			pos.x = atoi(position->text);
			position = position->next;
			pos.y = atoi(position->text);
			CArrayPushBack(&kp.Positions, &pos);
		}
		CArrayPushBack(&m->u.Static.Keys, &kp);
	}
}
static void LoadStaticExit(Mission *m, json_t *node, char *name)
{
	json_t *exitNode = json_find_first_label(node, name);
	if (!exitNode || !exitNode->child)
	{
		return;
	}
	exitNode = exitNode->child;
	LoadVec2i(&m->u.Static.Exit.Start, exitNode, "Start");
	LoadVec2i(&m->u.Static.Exit.End, exitNode, "End");
}
