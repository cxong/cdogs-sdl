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

#define VERSION 1


const char *MapTypeStr(MapType t)
{
	switch (t)
	{
	case MAPTYPE_CLASSIC:
		return "Classic";
	default:
		return "";
	}
}
MapType StrMapType(const char *s)
{
	if (strcmp(s, "Classic") == 0)
	{
		return MAPTYPE_CLASSIC;
	}
	return MAPTYPE_CLASSIC;
}
static int GetMapTypeVersion(MapType t)
{
	switch (t)
	{
	case MAPTYPE_CLASSIC:
		return 1;
	default:
		return 1;
	}
}


void MissionInit(Mission *m)
{
	memset(m, 0, sizeof *m);
	CArrayInit(&m->Objectives, sizeof(MissionObjective));
	CArrayInit(&m->Enemies, sizeof(int));
	CArrayInit(&m->SpecialChars, sizeof(int));
	CArrayInit(&m->Items, sizeof(int));
	CArrayInit(&m->ItemDensities, sizeof(int));
}
void MissionCopy(Mission *dst, Mission *src)
{
	CSTRDUP(dst->Title, src->Title);
	CSTRDUP(dst->Description, src->Description);
	dst->Type = src->Type;
	dst->Size = src->Size;

	dst->WallStyle = src->WallStyle;
	dst->FloorStyle = src->FloorStyle;
	dst->RoomStyle = src->RoomStyle;
	dst->ExitStyle = src->ExitStyle;
	dst->KeyStyle = src->KeyStyle;
	dst->DoorStyle = src->DoorStyle;

	CArrayCopy(&dst->Objectives, &src->Objectives);
	CArrayCopy(&dst->Enemies, &src->Enemies);
	CArrayCopy(&dst->SpecialChars, &src->SpecialChars);
	CArrayCopy(&dst->Items, &src->Items);
	CArrayCopy(&dst->ItemDensities, &src->ItemDensities);

	dst->EnemyDensity = src->EnemyDensity;
	memcpy(dst->Weapons, src->Weapons, sizeof dst->Weapons);

	memcpy(dst->Song, src->Song, sizeof dst->Song);

	dst->WallColor = src->WallColor;
	dst->FloorColor = src->FloorColor;
	dst->RoomColor = src->RoomColor;
	dst->AltColor = src->AltColor;

	memcpy(&dst->u, &src->u, sizeof dst->u);
}
void MissionTerminate(Mission *m)
{
	CFREE(m->Title);
	CFREE(m->Description);
	CArrayTerminate(&m->Objectives);
	CArrayTerminate(&m->Enemies);
	CArrayTerminate(&m->SpecialChars);
	CArrayTerminate(&m->Items);
	CArrayTerminate(&m->ItemDensities);
}

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
gun_e GetNthAvailableWeapon(int weapons[GUN_COUNT], int index)
{
	int i;
	int n = 0;
	for (i = 0; i < GUN_COUNT; i++)
	{
		if (weapons[i])
		{
			if (index == n)
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
	return err;
}

static void LoadMissions(CArray *missions, json_t *missionsNode);
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
	LoadMissions(&c->Missions, json_find_first_label(root, "Missions")->child);
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
static void LoadIntArray(CArray *a, json_t *node);
static void LoadWeapons(int weapons[GUN_COUNT], json_t *weaponsNode);
static void LoadClassicRooms(Mission *m, json_t *roomsNode);
static void LoadClassicDoors(Mission *m, json_t *node, char *name);
static void LoadClassicPillars(Mission *m, json_t *node, char *name);
static void LoadMissions(CArray *missions, json_t *missionsNode)
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
		LoadIntArray(&m.Enemies, json_find_first_label(child, "Enemies")->child);
		LoadIntArray(&m.SpecialChars, json_find_first_label(child, "SpecialChars")->child);
		LoadIntArray(&m.Items, json_find_first_label(child, "Items")->child);
		LoadIntArray(&m.ItemDensities, json_find_first_label(child, "ItemDensities")->child);
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
			LoadClassicRooms(
				&m, json_find_first_label(child, "Rooms")->child);
			LoadInt(&m.u.Classic.Squares, child, "Squares");
			LoadClassicDoors(&m, child, "Doors");
			LoadClassicPillars(&m, child, "Pillars");
			break;
		default:
			assert(0 && "unknown map type");
			continue;
		}
		CArrayPushBack(missions, &m);
	}
}
static void LoadCharacters(CharacterStore *c, json_t *charactersNode)
{
	json_t *child = charactersNode->child;
	CharacterStoreTerminate(c);
	CharacterStoreInit(c);
	while (child)
	{
		Character *ch = CharacterStoreAddOther(c);
		LoadInt(&ch->looks.armedBody, child, "armedBody");
		LoadInt(&ch->looks.unarmedBody, child, "unarmedBody");
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
		LoadInt(&ch->bot.probabilityToMove, child, "probabilityToMove");
		LoadInt(&ch->bot.probabilityToTrack, child, "probabilityToTrack");
		LoadInt(&ch->bot.probabilityToShoot, child, "probabilityToShoot");
		LoadInt(&ch->bot.actionDelay, child, "actionDelay");
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
static void LoadIntArray(CArray *a, json_t *node)
{
	json_t *child;
	for (child = node->child; child; child = child->next)
	{
		int n = atoi(child->text);
		CArrayPushBack(a, &n);
	}
}
static void LoadWeapons(int weapons[GUN_COUNT], json_t *weaponsNode)
{
	json_t *child;
	for (child = weaponsNode->child; child; child = child->next)
	{
		gun_e gun = StrGunName(child->text);
		weapons[gun] = 1;
	}
}
static void LoadClassicRooms(Mission *m, json_t *roomsNode)
{
	LoadInt(&m->u.Classic.Rooms.Count, roomsNode, "Count");
	LoadInt(&m->u.Classic.Rooms.Min, roomsNode, "Min");
	LoadInt(&m->u.Classic.Rooms.Max, roomsNode, "Max");
	LoadInt(&m->u.Classic.Rooms.Edge, roomsNode, "Edge");
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
	LoadInt(&m->u.Classic.Doors.Enabled, child, "Enabled");
	LoadInt(&m->u.Classic.Doors.Min, child, "Min");
	LoadInt(&m->u.Classic.Doors.Max, child, "Max");
}

static json_t *SaveMissions(CArray *a);
static json_t *SaveCharacters(CharacterStore *s);
int MapNewSave(const char *filename, CampaignSetting *c)
{
	FILE *f;
	char *text = NULL;
	char buf[CDOGS_PATH_MAX];
	json_t *root;
	if (SDL_strcasecmp(StrGetFileExt(filename), "cpn") == 0)
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
static json_t *SaveWeapons(int weapons[GUN_COUNT]);
static json_t *SaveClassicRooms(Mission *m);
static json_t *SaveClassicDoors(Mission *m);
static json_t *SaveClassicPillars(Mission *m);
static json_t *SaveMissions(CArray *a)
{
	json_t *missionsNode = json_new_array();
	int i;
	for (i = 0; i < (int)a->size; i++)
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
			json_insert_pair_into_object(
				node, "Rooms", SaveClassicRooms(mission));
			AddIntPair(node, "Squares", mission->u.Classic.Squares);
			json_insert_pair_into_object(
				node, "Doors", SaveClassicDoors(mission));
			json_insert_pair_into_object(
				node, "Pillars", SaveClassicPillars(mission));
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
		AddIntPair(node, "armedBody", c->looks.armedBody);
		AddIntPair(node, "unarmedBody", c->looks.unarmedBody);
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
		AddIntPair(node, "probabilityToMove", c->bot.probabilityToMove);
		AddIntPair(node, "probabilityToTrack", c->bot.probabilityToTrack);
		AddIntPair(node, "probabilityToShoot", c->bot.probabilityToShoot);
		AddIntPair(node, "actionDelay", c->bot.actionDelay);
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
	AddIntPair(node, "Edge", m->u.Classic.Rooms.Edge);
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
	AddIntPair(node, "Enabled", m->u.Classic.Doors.Enabled);
	AddIntPair(node, "Min", m->u.Classic.Doors.Min);
	AddIntPair(node, "Max", m->u.Classic.Doors.Max);
	return node;
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
