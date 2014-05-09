/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
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
#include "map_new.h"

#include <assert.h>
#include <locale.h>
#include <stdio.h>

#include <json/json.h>

#include "files.h"
#include "json_utils.h"

#define VERSION 2


int GetNumWeapons(int weapons[GUN_COUNT])
{
	int i;
	int num = 0;
	for (i = 0; i < GUN_COUNT; i++)
	{
		if (weapons[i])
		{
			num++;
		}
	}
	return num;
}
gun_e GetNthAvailableWeapon(int weapons[GUN_COUNT], int idx)
{
	int n = 0;
	for (int i = 0; i < GUN_COUNT; i++)
	{
		if (weapons[i])
		{
			if (idx == n)
			{
				return i;
			}
			n++;
		}
	}
	assert(0 && "cannot find available weapon");
	return GUN_KNIFE;
}


int MapNewScan(const char *filename, char **title, int *numMissions)
{
	int err = 0;
	int version;
	json_t *root = NULL;
	json_t *missionNode;
	FILE *f = NULL;
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
	LoadInt(&version, root, "Version");
	if (version > VERSION || version <= 0)
	{
		err = -1;
		goto bail;
	}
	*title = GetString(root, "Title");
	*numMissions = 0;
	for (missionNode = json_find_first_label(root, "Missions")->child->child;
		missionNode;
		missionNode = missionNode->next)
	{
		(*numMissions)++;
	}

bail:
	if (f)
	{
		fclose(f);
	}
	json_free_value(&root);
	return err;
}

static void LoadMissions(CArray *missions, json_t *missionsNode, int version);
static void LoadCharacters(CharacterStore *c, json_t *charactersNode);
int MapNewLoad(const char *filename, CampaignSetting *c)
{
	FILE *f = NULL;
	int err = 0;
	json_t *root = NULL;
	int version;

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

	// try to load the new map format
	f = fopen(filename, "r");
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
	if (version > VERSION || version <= 0)
	{
		assert(0 && "not implemented or unknown campaign");
		err = -1;
		goto bail;
	}
	CFREE(c->Title);
	c->Title = GetString(root, "Title");
	CFREE(c->Author);
	c->Author = GetString(root, "Author");
	CFREE(c->Description);
	c->Description = GetString(root, "Description");
	LoadMissions(&c->Missions, json_find_first_label(root, "Missions")->child, version);
	LoadCharacters(&c->characters, json_find_first_label(root, "Characters")->child);

bail:
	json_free_value(&root);
	if (f != NULL)
	{
		fclose(f);
	}
	return err;
}

static void LoadMissionObjectives(CArray *objectives, json_t *objectivesNode);
static void LoadIntArray(CArray *a, json_t *node, char *name);
static void LoadVec2i(Vec2i *v, json_t *node, char *name);
static void LoadWeapons(int weapons[GUN_COUNT], json_t *weaponsNode);
static void LoadClassicRooms(Mission *m, json_t *roomsNode);
static void LoadClassicDoors(Mission *m, json_t *node, char *name);
static void LoadClassicPillars(Mission *m, json_t *node, char *name);
static bool TryLoadStaticMap(Mission *m, json_t *node, int version);
static void LoadMissions(CArray *missions, json_t *missionsNode, int version)
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
		LoadInt(&m.WallStyle, child, "WallStyle");
		LoadInt(&m.FloorStyle, child, "FloorStyle");
		LoadInt(&m.RoomStyle, child, "RoomStyle");
		LoadInt(&m.ExitStyle, child, "ExitStyle");
		LoadInt(&m.KeyStyle, child, "KeyStyle");
		LoadInt(&m.DoorStyle, child, "DoorStyle");
		LoadMissionObjectives(&m.Objectives, json_find_first_label(child, "Objectives")->child);
		LoadIntArray(&m.Enemies, child, "Enemies");
		LoadIntArray(&m.SpecialChars, child, "SpecialChars");
		LoadIntArray(&m.Items, child, "Items");
		LoadIntArray(&m.ItemDensities, child, "ItemDensities");
		LoadInt(&m.EnemyDensity, child, "EnemyDensity");
		LoadWeapons(m.Weapons, json_find_first_label(child, "Weapons")->child);
		strcpy(m.Song, json_find_first_label(child, "Song")->child->text);
		LoadInt(&m.WallColor, child, "WallColor");
		LoadInt(&m.FloorColor, child, "FloorColor");
		LoadInt(&m.RoomColor, child, "RoomColor");
		LoadInt(&m.AltColor, child, "AltColor");
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
		default:
			assert(0 && "unknown map type");
			continue;
		}
		CArrayPushBack(missions, &m);
	}
}
static void LoadStaticItems(Mission *m, json_t *node, char *name);
static void LoadStaticWrecks(Mission *m, json_t *node, char *name);
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
	}

	LoadStaticItems(m, node, "StaticItems");
	LoadStaticWrecks(m, node, "StaticWrecks");
	LoadStaticCharacters(m, node, "StaticCharacters");
	LoadStaticObjectives(m, node, "StaticObjectives");
	LoadStaticKeys(m, node, "StaticKeys");

	LoadVec2i(&m->u.Static.Start, node, "Start");
	LoadStaticExit(m, node, "Exit");

	return true;
}
static void LoadCharacters(CharacterStore *c, json_t *charactersNode)
{
	json_t *child = charactersNode->child;
	CharacterStoreTerminate(c);
	CharacterStoreInit(c);
	while (child)
	{
		Character *ch = CharacterStoreAddOther(c);
		LoadInt(&ch->looks.face, child, "face");
		LoadInt(&ch->looks.skin, child, "skin");
		LoadInt(&ch->looks.arm, child, "arm");
		LoadInt(&ch->looks.body, child, "body");
		LoadInt(&ch->looks.leg, child, "leg");
		LoadInt(&ch->looks.hair, child, "hair");
		LoadInt(&ch->speed, child, "speed");
		JSON_UTILS_LOAD_ENUM(ch->gun, child, "Gun", StrGunName);
		LoadInt(&ch->maxHealth, child, "maxHealth");
		LoadInt(&ch->flags, child, "flags");
		LoadInt(&ch->bot->probabilityToMove, child, "probabilityToMove");
		LoadInt(&ch->bot->probabilityToTrack, child, "probabilityToTrack");
		LoadInt(&ch->bot->probabilityToShoot, child, "probabilityToShoot");
		LoadInt(&ch->bot->actionDelay, child, "actionDelay");
		CharacterSetLooks(ch, &ch->looks);
		child = child->next;
	}
}

static void LoadMissionObjectives(CArray *objectives, json_t *objectivesNode)
{
	json_t *child;
	for (child = objectivesNode->child; child; child = child->next)
	{
		MissionObjective mo;
		memset(&mo, 0, sizeof mo);
		mo.Description = GetString(child, "Description");
		JSON_UTILS_LOAD_ENUM(mo.Type, child, "Type", StrObjectiveType);
		LoadInt(&mo.Index, child, "Index");
		LoadInt(&mo.Count, child, "Count");
		LoadInt(&mo.Required, child, "Required");
		LoadInt(&mo.Flags, child, "Flags");
		CArrayPushBack(objectives, &mo);
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
static void LoadVec2i(Vec2i *v, json_t *node, char *name)
{
	json_t *child = json_find_first_label(node, name);
	if (!child || !child->child)
	{
		return;
	}
	child = child->child;
	child = child->child;
	v->x = atoi(child->text);
	child = child->next;
	v->y = atoi(child->text);
}
static void LoadWeapons(int weapons[GUN_COUNT], json_t *weaponsNode)
{
	if (!weaponsNode->child)
	{
		// enable all weapons
		int i;
		for (i = 0; i < GUN_COUNT; i++)
		{
			weapons[i] = 1;
		}
	}
	else
	{
		json_t *child;
		for (child = weaponsNode->child; child; child = child->next)
		{
			gun_e gun = StrGunName(child->text);
			weapons[gun] = 1;
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
static void LoadStaticItems(Mission *m, json_t *node, char *name)
{
	CArrayInit(&m->u.Static.Items, sizeof(MapObjectPositions));

	json_t *items = json_find_first_label(node, name);
	if (!items || !items->child)
	{
		return;
	}
	items = items->child;
	for (items = items->child; items; items = items->next)
	{
		MapObjectPositions mop;
		LoadInt(&mop.Index, items, "Index");
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
static void LoadStaticWrecks(Mission *m, json_t *node, char *name)
{
	CArrayInit(&m->u.Static.Wrecks, sizeof(MapObjectPositions));
	
	json_t *wrecks = json_find_first_label(node, name);
	if (!wrecks || !wrecks->child)
	{
		return;
	}
	wrecks = wrecks->child;
	for (wrecks = wrecks->child; wrecks; wrecks = wrecks->next)
	{
		MapObjectPositions mop;
		LoadInt(&mop.Index, wrecks, "Index");
		CArrayInit(&mop.Positions, sizeof(Vec2i));
		json_t *positions = json_find_first_label(wrecks, "Positions");
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
		CArrayPushBack(&m->u.Static.Wrecks, &mop);
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

static json_t *SaveMissions(CArray *a);
static json_t *SaveCharacters(CharacterStore *s);
int MapNewSave(const char *filename, CampaignSetting *c)
{
	FILE *f;
	char *text = NULL;
	char buf[CDOGS_PATH_MAX];
	json_t *root;
	if (strcmp(StrGetFileExt(filename), "cpn") == 0 ||
		strcmp(StrGetFileExt(filename), "CPN") == 0)
	{
		strcpy(buf, filename);
	}
	else
	{
		sprintf(buf, "%s.cpn", filename);
	}
	f = fopen(buf, "w");
	if (f == NULL)
	{
		perror("MapNewSave - couldn't write to file: ");
		return 0;
	}
	setlocale(LC_ALL, "");

	root = json_new_object();

	// Common fields
	AddIntPair(root, "Version", VERSION);
	AddStringPair(root, "Title", c->Title);
	AddStringPair(root, "Author", c->Author);
	AddStringPair(root, "Description", c->Description);

	json_insert_pair_into_object(root, "Missions", SaveMissions(&c->Missions));
	json_insert_pair_into_object(
		root, "Characters", SaveCharacters(&c->characters));

	json_tree_to_string(root, &text);
	fputs(json_format_string(text), f);

	// clean up
	CFREE(text);
	json_free_value(&root);

	fclose(f);

	return 1;
}

static json_t *SaveObjectives(CArray *a);
static json_t *SaveIntArray(CArray *a);
static json_t *SaveVec2i(Vec2i v);
static json_t *SaveWeapons(int weapons[GUN_COUNT]);
static json_t *SaveClassicRooms(Mission *m);
static json_t *SaveClassicDoors(Mission *m);
static json_t *SaveClassicPillars(Mission *m);
static json_t *SaveStaticTiles(Mission *m);
static json_t *SaveStaticItems(Mission *m);
static json_t *SaveStaticWrecks(Mission *m);
static json_t *SaveStaticCharacters(Mission *m);
static json_t *SaveStaticObjectives(Mission *m);
static json_t *SaveStaticKeys(Mission *m);
static json_t *SaveMissions(CArray *a)
{
	json_t *missionsNode = json_new_array();
	for (int i = 0; i < (int)a->size; i++)
	{
		json_t *node = json_new_object();
		Mission *mission = CArrayGet(a, i);
		AddStringPair(node, "Title", mission->Title);
		AddStringPair(node, "Description", mission->Description);
		AddStringPair(node, "Type", MapTypeStr(mission->Type));
		AddIntPair(node, "Width", mission->Size.x);
		AddIntPair(node, "Height", mission->Size.y);

		AddIntPair(node, "WallStyle", mission->WallStyle);
		AddIntPair(node, "FloorStyle", mission->FloorStyle);
		AddIntPair(node, "RoomStyle", mission->RoomStyle);
		AddIntPair(node, "ExitStyle", mission->ExitStyle);
		AddIntPair(node, "KeyStyle", mission->KeyStyle);
		AddIntPair(node, "DoorStyle", mission->DoorStyle);

		json_insert_pair_into_object(
			node, "Objectives", SaveObjectives(&mission->Objectives));
		json_insert_pair_into_object(
			node, "Enemies", SaveIntArray(&mission->Enemies));
		json_insert_pair_into_object(
			node, "SpecialChars", SaveIntArray(&mission->SpecialChars));
		json_insert_pair_into_object(
			node, "Items", SaveIntArray(&mission->Items));
		json_insert_pair_into_object(
			node, "ItemDensities", SaveIntArray(&mission->ItemDensities));

		AddIntPair(node, "EnemyDensity", mission->EnemyDensity);
		json_insert_pair_into_object(
			node, "Weapons", SaveWeapons(mission->Weapons));

		json_insert_pair_into_object(
			node, "Song", json_new_string(mission->Song));

		AddIntPair(node, "WallColor", mission->WallColor);
		AddIntPair(node, "FloorColor", mission->FloorColor);
		AddIntPair(node, "RoomColor", mission->RoomColor);
		AddIntPair(node, "AltColor", mission->AltColor);

		switch (mission->Type)
		{
		case MAPTYPE_CLASSIC:
			AddIntPair(node, "Walls", mission->u.Classic.Walls);
			AddIntPair(node, "WallLength", mission->u.Classic.WallLength);
			AddIntPair(
				node, "CorridorWidth", mission->u.Classic.CorridorWidth);
			json_insert_pair_into_object(
				node, "Rooms", SaveClassicRooms(mission));
			AddIntPair(node, "Squares", mission->u.Classic.Squares);
			json_insert_pair_into_object(
				node, "Doors", SaveClassicDoors(mission));
			json_insert_pair_into_object(
				node, "Pillars", SaveClassicPillars(mission));
			break;
		case MAPTYPE_STATIC:
			{
				json_insert_pair_into_object(
					node, "Tiles", SaveStaticTiles(mission));
				json_insert_pair_into_object(
					node, "StaticItems", SaveStaticItems(mission));
				json_insert_pair_into_object(
					node, "StaticWrecks", SaveStaticWrecks(mission));
				json_insert_pair_into_object(
					node, "StaticCharacters", SaveStaticCharacters(mission));
				json_insert_pair_into_object(
					node, "StaticObjectives", SaveStaticObjectives(mission));
				json_insert_pair_into_object(
					node, "StaticKeys", SaveStaticKeys(mission));

				json_insert_pair_into_object(
					node, "Start", SaveVec2i(mission->u.Static.Start));
				json_t *exitNode = json_new_object();
				json_insert_pair_into_object(
					exitNode, "Start",
					SaveVec2i(mission->u.Static.Exit.Start));
				json_insert_pair_into_object(
					exitNode, "End",
					SaveVec2i(mission->u.Static.Exit.End));
				json_insert_pair_into_object(node, "Exit", exitNode);
			}
			break;
		default:
			assert(0 && "unknown map type");
			break;
		}

		json_insert_child(missionsNode, node);
	}
	return missionsNode;
}
static json_t *SaveCharacters(CharacterStore *s)
{
	json_t *charNode = json_new_array();
	int i;
	for (i = 0; i < (int)s->OtherChars.size; i++)
	{
		json_t *node = json_new_object();
		Character *c = CArrayGet(&s->OtherChars, i);
		AddIntPair(node, "face", c->looks.face);
		AddIntPair(node, "skin", c->looks.skin);
		AddIntPair(node, "arm", c->looks.arm);
		AddIntPair(node, "body", c->looks.body);
		AddIntPair(node, "leg", c->looks.leg);
		AddIntPair(node, "hair", c->looks.hair);
		AddIntPair(node, "speed", c->speed);
		json_insert_pair_into_object(
			node, "Gun", json_new_string(GunGetName(c->gun)));
		AddIntPair(node, "maxHealth", c->maxHealth);
		AddIntPair(node, "flags", c->flags);
		AddIntPair(node, "probabilityToMove", c->bot->probabilityToMove);
		AddIntPair(node, "probabilityToTrack", c->bot->probabilityToTrack);
		AddIntPair(node, "probabilityToShoot", c->bot->probabilityToShoot);
		AddIntPair(node, "actionDelay", c->bot->actionDelay);
		json_insert_child(charNode, node);
	}
	return charNode;
}
static json_t *SaveClassicRooms(Mission *m)
{
	json_t *node = json_new_object();
	AddIntPair(node, "Count", m->u.Classic.Rooms.Count);
	AddIntPair(node, "Min", m->u.Classic.Rooms.Min);
	AddIntPair(node, "Max", m->u.Classic.Rooms.Max);
	AddBoolPair(node, "Edge", m->u.Classic.Rooms.Edge);
	AddBoolPair(node, "Overlap", m->u.Classic.Rooms.Overlap);
	AddIntPair(node, "Walls", m->u.Classic.Rooms.Walls);
	AddIntPair(node, "WallLength", m->u.Classic.Rooms.WallLength);
	AddIntPair(node, "WallPad", m->u.Classic.Rooms.WallPad);
	return node;
}
static json_t *SaveClassicPillars(Mission *m)
{
	json_t *node = json_new_object();
	AddIntPair(node, "Count", m->u.Classic.Pillars.Count);
	AddIntPair(node, "Min", m->u.Classic.Pillars.Min);
	AddIntPair(node, "Max", m->u.Classic.Pillars.Max);
	return node;
}
static json_t *SaveClassicDoors(Mission *m)
{
	json_t *node = json_new_object();
	AddBoolPair(node, "Enabled", m->u.Classic.Doors.Enabled);
	AddIntPair(node, "Min", m->u.Classic.Doors.Min);
	AddIntPair(node, "Max", m->u.Classic.Doors.Max);
	return node;
}

static json_t *SaveStaticTiles(Mission *m)
{
	// Create a text buffer for CSV
	// The buffer will contain n*5 chars (tiles, allow 5 chars each),
	// and n - 1 commas, so 6n total
	int size = (int)m->u.Static.Tiles.size;
	char *bigbuf;
	CCALLOC(bigbuf, size * 6);
	char *pBuf = bigbuf;
	for (int i = 0; i < size; i++)
	{
		char buf[32];
		sprintf(buf, "%d", *(unsigned short *)CArrayGet(
			&m->u.Static.Tiles, i));
		strcpy(pBuf, buf);
		pBuf += strlen(buf);
		if (i < size - 1)
		{
			*pBuf = ',';
			pBuf++;
		}
	}
	json_t *node = json_new_string(bigbuf);
	CFREE(bigbuf);
	return node;
}
static json_t *SaveStaticItems(Mission *m)
{
	json_t *items = json_new_array();
	for (int i = 0; i < (int)m->u.Static.Items.size; i++)
	{
		MapObjectPositions *mop =
			CArrayGet(&m->u.Static.Items, i);
		json_t *itemNode = json_new_object();
		AddIntPair(itemNode, "Index", mop->Index);
		json_t *positions = json_new_array();
		for (int j = 0; j < (int)mop->Positions.size; j++)
		{
			Vec2i *pos = CArrayGet(&mop->Positions, j);
			json_insert_child(positions, SaveVec2i(*pos));
		}
		json_insert_pair_into_object(
			itemNode, "Positions", positions);
		json_insert_child(items, itemNode);
	}
	return items;
}
static json_t *SaveStaticWrecks(Mission *m)
{
	json_t *wrecks = json_new_array();
	for (int i = 0; i < (int)m->u.Static.Wrecks.size; i++)
	{
		MapObjectPositions *mop =
			CArrayGet(&m->u.Static.Wrecks, i);
		json_t *wreckNode = json_new_object();
		AddIntPair(wreckNode, "Index", mop->Index);
		json_t *positions = json_new_array();
		for (int j = 0; j < (int)mop->Positions.size; j++)
		{
			Vec2i *pos = CArrayGet(&mop->Positions, j);
			json_insert_child(positions, SaveVec2i(*pos));
		}
		json_insert_pair_into_object(
			wreckNode, "Positions", positions);
		json_insert_child(wrecks, wreckNode);
	}
	return wrecks;
}
static json_t *SaveStaticCharacters(Mission *m)
{
	json_t *chars = json_new_array();
	for (int i = 0; i < (int)m->u.Static.Characters.size; i++)
	{
		CharacterPositions *cp =
			CArrayGet(&m->u.Static.Characters, i);
		json_t *charNode = json_new_object();
		AddIntPair(charNode, "Index", cp->Index);
		json_t *positions = json_new_array();
		for (int j = 0; j < (int)cp->Positions.size; j++)
		{
			Vec2i *pos = CArrayGet(&cp->Positions, j);
			json_insert_child(positions, SaveVec2i(*pos));
		}
		json_insert_pair_into_object(
			charNode, "Positions", positions);
		json_insert_child(chars, charNode);
	}
	return chars;
}
static json_t *SaveStaticObjectives(Mission *m)
{
	json_t *objs = json_new_array();
	for (int i = 0; i < (int)m->u.Static.Objectives.size; i++)
	{
		ObjectivePositions *op =
		CArrayGet(&m->u.Static.Objectives, i);
		json_t *objNode = json_new_object();
		AddIntPair(objNode, "Index", op->Index);
		json_t *positions = json_new_array();
		for (int j = 0; j < (int)op->Positions.size; j++)
		{
			Vec2i *pos = CArrayGet(&op->Positions, j);
			json_insert_child(positions, SaveVec2i(*pos));
		}
		json_insert_pair_into_object(
			objNode, "Positions", positions);
		json_insert_pair_into_object(
			objNode, "Indices", SaveIntArray(&op->Indices));
		json_insert_child(objs, objNode);
	}
	return objs;
}
static json_t *SaveStaticKeys(Mission *m)
{
	json_t *keys = json_new_array();
	for (int i = 0; i < (int)m->u.Static.Keys.size; i++)
	{
		KeyPositions *kp =
		CArrayGet(&m->u.Static.Keys, i);
		json_t *keyNode = json_new_object();
		AddIntPair(keyNode, "Index", kp->Index);
		json_t *positions = json_new_array();
		for (int j = 0; j < (int)kp->Positions.size; j++)
		{
			Vec2i *pos = CArrayGet(&kp->Positions, j);
			json_insert_child(positions, SaveVec2i(*pos));
		}
		json_insert_pair_into_object(
			keyNode, "Positions", positions);
		json_insert_child(keys, keyNode);
	}
	return keys;
}

static json_t *SaveObjectives(CArray *a)
{
	json_t *objectivesNode = json_new_array();
	int i;
	for (i = 0; i < (int)a->size; i++)
	{
		json_t *objNode = json_new_object();
		MissionObjective *mo = CArrayGet(a, i);
		AddStringPair(objNode, "Description", mo->Description);
		AddStringPair(objNode, "Type", ObjectiveTypeStr(mo->Type));
		AddIntPair(objNode, "Index", mo->Index);
		AddIntPair(objNode, "Count", mo->Count);
		AddIntPair(objNode, "Required", mo->Required);
		AddIntPair(objNode, "Flags", mo->Flags);
		json_insert_child(objectivesNode, objNode);
	}
	return objectivesNode;
}

static json_t *SaveIntArray(CArray *a)
{
	json_t *node = json_new_array();
	int i;
	for (i = 0; i < (int)a->size; i++)
	{
		char buf[32];
		sprintf(buf, "%d", *(int *)CArrayGet(a, i));
		json_insert_child(node, json_new_number(buf));
	}
	return node;
}
static json_t *SaveVec2i(Vec2i v)
{
	json_t *node = json_new_array();
	char buf[32];
	sprintf(buf, "%d", v.x);
	json_insert_child(node, json_new_number(buf));
	sprintf(buf, "%d", v.y);
	json_insert_child(node, json_new_number(buf));
	return node;
}

static json_t *SaveWeapons(int weapons[GUN_COUNT])
{
	json_t *node = json_new_array();
	int i;
	for (i = 0; i < GUN_COUNT; i++)
	{
		if (weapons[i])
		{
			json_insert_child(node, json_new_string(GunGetName(i)));
		}
	}
	return node;
}
