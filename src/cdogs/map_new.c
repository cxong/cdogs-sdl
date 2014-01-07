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
	CArrayInit(&m->Weapons, sizeof(int));
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
	CArrayCopy(&dst->Weapons, &src->Weapons);

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
	CArrayTerminate(&m->Weapons);
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
	json_insert_pair_into_object(
		root, "Title", json_new_string(c->Title));
	json_insert_pair_into_object(
		root, "Author", json_new_string(c->Author));
	json_insert_pair_into_object(
		root, "Description", json_new_string(c->Description));

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
static json_t *SaveMissions(CArray *a)
{
	json_t *missionsNode = json_new_array();
	int i;
	for (i = 0; i < (int)a->size; i++)
	{
		json_t *node = json_new_object();
		Mission *mission = CArrayGet(a, i);
		json_insert_pair_into_object(
			node, "Title", json_new_string(mission->Title));
		json_insert_pair_into_object(
			node, "Description", json_new_string(mission->Description));
		json_insert_pair_into_object(
			node, "Type", json_new_string(MapTypeStr(mission->Type)));
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
			node, "Weapons", SaveIntArray(&mission->Weapons));

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
			AddIntPair(node, "Rooms", mission->u.Classic.Rooms);
			AddIntPair(node, "Squares", mission->u.Classic.Squares);
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
		AddIntPair(node, "speed", c->speed);
		AddIntPair(node, "gun", c->gun);
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

static json_t *SaveObjectives(CArray *a)
{
	json_t *objectivesNode = json_new_array();
	int i;
	for (i = 0; i < (int)a->size; i++)
	{
		json_t *objNode = json_new_object();
		MissionObjective *mo = CArrayGet(a, i);
		json_insert_pair_into_object(
			objNode, "Description", json_new_string(mo->Description));
		json_insert_pair_into_object(
			objNode, "Type", json_new_string(ObjectiveTypeStr(mo->Type)));
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
