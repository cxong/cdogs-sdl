/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.

	Copyright (c) 2013-2014, 2016 Cong Xu
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
#include "player_template.h"

#include <locale.h>

#include <json/json.h>

#include <cdogs/character.h>
#include <cdogs/files.h>
#include <cdogs/json_utils.h>

#define VERSION 2


static const char *faceNames[] =
{
	"Jones",
	"Ice",
	"Ogre",
	"Dragon",
	"WarBaby",
	"Bug-eye",
	"Smith",
	"Ogre Boss",
	"Grunt",
	"Professor",
	"Snake",
	"Wolf",
	"Bob",
	"Mad bug-eye",
	"Cyborg",
	"Robot",
	"Lady"
};
static int StrFaceIndex(const char *s)
{
	int i;
	for (i = 0; i < FACE_COUNT; i++)
	{
		if (strcmp(s, faceNames[i]) == 0)
		{
			return i;
		}
	}
	return 0;
}
const char *IndexToFaceStr(int idx)
{
	if (idx >= 0 && idx < FACE_COUNT)
	{
		return faceNames[idx];
	}
	return faceNames[0];
}

CArray gPlayerTemplates;

static void LoadPlayerTemplate(
	PlayerTemplate *t, json_t *node, const int version)
{
	strcpy(t->name, json_find_first_label(node, "Name")->child->text);
	t->Face = StrFaceIndex(json_find_first_label(node, "Face")->child->text);
	if (version == 1)
	{
		// Version 1 used integer palettes
		int skin, arms, body, legs, hair;
		LoadInt(&skin, node, "Skin");
		LoadInt(&arms, node, "Arms");
		LoadInt(&body, node, "Body");
		LoadInt(&legs, node, "Legs");
		LoadInt(&hair, node, "Hair");
		ConvertCharacterColors(skin, arms, body, legs, hair, &t->Colors);
	}
	else
	{
		LoadColor(&t->Colors.Skin, node, "Skin");
		LoadColor(&t->Colors.Arms, node, "Arms");
		LoadColor(&t->Colors.Body, node, "Body");
		LoadColor(&t->Colors.Legs, node, "Legs");
		LoadColor(&t->Colors.Hair, node, "Hair");
	}
}
void LoadPlayerTemplates(CArray *templates, const char *filename)
{
	json_t *root = NULL;
	int version = 1;

	// initialise templates
	CArrayInit(templates, sizeof(PlayerTemplate));
	FILE *f = fopen(GetConfigFilePath(filename), "r");
	if (!f)
	{
		printf("Error loading player templates '%s'\n", filename);
		goto bail;
	}

	if (json_stream_parse(f, &root) != JSON_OK)
	{
		printf("Error parsing player templates '%s'\n", filename);
		goto bail;
	}

	LoadInt(&version, root, "Version");

	if (json_find_first_label(root, "PlayerTemplates") == NULL)
	{
		printf("Error: unknown player templates format\n");
		goto bail;
	}
	json_t *child = json_find_first_label(root, "PlayerTemplates")->child->child;
	while (child != NULL)
	{
		PlayerTemplate t;
		LoadPlayerTemplate(&t, child, version);
		child = child->next;
		CArrayPushBack(templates, &t);
	}

bail:
	json_free_value(&root);
	if (f != NULL)
	{
		fclose(f);
	}
}

static void SavePlayerTemplate(PlayerTemplate *t, json_t *templates)
{
	json_t *template = json_new_object();
	json_insert_pair_into_object(template, "Name", json_new_string(t->name));
	json_insert_pair_into_object(
		template, "Face", json_new_string(faceNames[t->Face]));
	AddColorPair(template, "Body", t->Colors.Body);
	AddColorPair(template, "Arms", t->Colors.Arms);
	AddColorPair(template, "Legs", t->Colors.Legs);
	AddColorPair(template, "Skin", t->Colors.Skin);
	AddColorPair(template, "Hair", t->Colors.Hair);
	json_insert_child(templates, template);
}
void SavePlayerTemplates(const CArray *templates, const char *filename)
{
	FILE *f = fopen(GetConfigFilePath(filename), "w");
	char *text = NULL;
	char *formatText = NULL;
	json_t *root = json_new_object();

	if (f == NULL)
	{
		printf("Error saving player templates '%s'\n", filename);
		goto bail;
	}

	debug(D_NORMAL, "begin\n");

	setlocale(LC_ALL, "");

	json_insert_pair_into_object(
		root, "Version", json_new_number(TOSTRING(VERSION)));
	json_t *templatesNode = json_new_array();
	for (int i = 0; i < (int)templates->size; i++)
	{
		PlayerTemplate *t = CArrayGet(templates, i);
		SavePlayerTemplate(t, templatesNode);
	}
	json_insert_pair_into_object(root, "PlayerTemplates", templatesNode);

	json_tree_to_string(root, &text);
	formatText = json_format_string(text);
	fputs(formatText, f);

bail:
	// clean up
	free(formatText);
	free(text);
	json_free_value(&root);
	if (f != NULL)
	{
		fclose(f);
	}
}
