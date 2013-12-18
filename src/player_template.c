/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.

	Copyright (c) 2013, Cong Xu
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

PlayerTemplate gPlayerTemplates[MAX_TEMPLATE];

static void LoadPlayerTemplate(PlayerTemplate *t, json_t *node)
{
	strcpy(t->name, json_find_first_label(node, "Name")->child->text);
	t->face = StrFaceIndex(json_find_first_label(node, "Face")->child->text);
	LoadInt(&t->body, node, "Body");
	LoadInt(&t->arms, node, "Arms");
	LoadInt(&t->legs, node, "Legs");
	LoadInt(&t->skin, node, "Skin");
	LoadInt(&t->hair, node, "Hair");
}
void LoadPlayerTemplates(
	PlayerTemplate templates[MAX_TEMPLATE], const char *filename)
{
	int i;
	FILE *f = fopen(GetConfigFilePath(filename), "r");
	json_t *root = NULL;
	json_t *child;

	// initialise templates
	// templates are zero-delimited
	memset(templates, 0, sizeof templates);
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

	if (json_find_first_label(root, "PlayerTemplates") == NULL)
	{
		return;
	}
	child = json_find_first_label(root, "PlayerTemplates")->child->child;
	i = 0;
	while (child != NULL && i < MAX_TEMPLATE)
	{
		PlayerTemplate *t = &templates[i];
		LoadPlayerTemplate(t, child);
		child = child->next;
		i++;
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
		template, "Face", json_new_string(faceNames[t->face]));
	AddIntPair(template, "Body", t->body);
	AddIntPair(template, "Arms", t->arms);
	AddIntPair(template, "Legs", t->legs);
	AddIntPair(template, "Skin", t->skin);
	AddIntPair(template, "Hair", t->hair);
	json_insert_child(templates, template);
}
void SavePlayerTemplates(
	PlayerTemplate templates[MAX_TEMPLATE], const char *filename)
{
	FILE *f = fopen(GetConfigFilePath(filename), "w");
	char *text = NULL;
	json_t *root;

	if (f == NULL)
	{
		printf("Error saving player templates '%s'\n", filename);
		return;
	}

	debug(D_NORMAL, "begin\n");

	setlocale(LC_ALL, "");

	root = json_new_object();
	json_insert_pair_into_object(root, "Version", json_new_number("1"));
	if (PlayerTemplatesGetCount(templates) > 0)
	{
		json_t *templatesNode = json_new_array();
		int i;
		for (i = 0; i < PlayerTemplatesGetCount(templates); i++)
		{
			PlayerTemplate *t = &templates[i];
			SavePlayerTemplate(t, templatesNode);
		}
		json_insert_pair_into_object(root, "PlayerTemplates", templatesNode);
	}

	json_tree_to_string(root, &text);
	fputs(json_format_string(text), f);

	// clean up
	free(text);
	json_free_value(&root);

	fclose(f);
}

int PlayerTemplatesGetCount(PlayerTemplate templates[MAX_TEMPLATE])
{
	int i;
	for (i = 0; i < MAX_TEMPLATE; i++)
	{
		if (strcmp(templates[i].name, "") == 0)
		{
			break;
		}
	}
	return i;
}
