/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.

	Copyright (c) 2013-2014, 2016-2020 Cong Xu
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

#include <json/json.h>

#include <cdogs/character.h>
#include <cdogs/files.h>
#include <cdogs/json_utils.h>
#include <cdogs/log.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#define VERSION 3

PlayerTemplates gPlayerTemplates;

static void LoadPlayerTemplate(
	CArray *templates, json_t *node, const int version)
{
	PlayerTemplate t;
	memset(&t, 0, sizeof t);
	strncpy(
		t.name, json_find_first_label(node, "Name")->child->text,
		PLAYER_NAME_MAXLEN - 1);
	t.CharClassName = GetString(node, "Face");
	// Hair
	if (version < 3)
	{
		char *face;
		CSTRDUP(face, t.CharClassName);
		CharacterOldFaceToHair(t.CharClassName, &t.CharClassName, &t.Hair);
		CFREE(face);
	}
	else
	{
		LoadStr(&t.Hair, node, "HairType");
	}
	// Colors
	if (version == 1)
	{
		// Version 1 used integer palettes
		int skin, arms, body, legs, hair;
		LoadInt(&skin, node, "Skin");
		LoadInt(&arms, node, "Arms");
		LoadInt(&body, node, "Body");
		LoadInt(&legs, node, "Legs");
		LoadInt(&hair, node, "Hair");
		ConvertCharacterColors(skin, arms, body, legs, hair, &t.Colors);
	}
	else
	{
		LoadColor(&t.Colors.Skin, node, "Skin");
		LoadColor(&t.Colors.Arms, node, "Arms");
		LoadColor(&t.Colors.Body, node, "Body");
		LoadColor(&t.Colors.Legs, node, "Legs");
		LoadColor(&t.Colors.Hair, node, "Hair");
	}
	t.Colors.Feet = t.Colors.Legs;
	if (version >= 3)
	{
		LoadColor(&t.Colors.Feet, node, "Feet");
	}
	CArrayPushBack(templates, &t);
	LOG(LM_MAIN, LL_DEBUG, "loaded player template %s (%s)", t.name,
		t.CharClassName);
}

void PlayerTemplatesLoad(PlayerTemplates *pt, const CharacterClasses *classes)
{
	// Note: not used, but included in function to express dependency
	CASSERT(
		classes->Classes.size > 0,
		"cannot load player templates without character classes");
	json_t *root = NULL;

	CArrayInit(&pt->Classes, sizeof(PlayerTemplate));
	CArrayInit(&pt->CustomClasses, sizeof(PlayerTemplate));
	FILE *f = fopen(GetConfigFilePath(PLAYER_TEMPLATE_FILE), "r");
	if (!f)
	{
		LOG(LM_MAIN, LL_ERROR, "loading player templates '%s'",
			PLAYER_TEMPLATE_FILE);
		goto bail;
	}

	if (json_stream_parse(f, &root) != JSON_OK)
	{
		LOG(LM_MAIN, LL_ERROR, "parsing player templates '%s'",
			PLAYER_TEMPLATE_FILE);
		goto bail;
	}

	PlayerTemplatesLoadJSON(&pt->Classes, root);

bail:
	json_free_value(&root);
	if (f != NULL)
	{
		fclose(f);
	}
}

void PlayerTemplatesLoadJSON(CArray *classes, json_t *node)
{
	int version = 1;
	LoadInt(&version, node, "Version");

	if (json_find_first_label(node, "PlayerTemplates") == NULL)
	{
		LOG(LM_MAIN, LL_ERROR, "unknown player templates format");
		return;
	}
	json_t *child =
		json_find_first_label(node, "PlayerTemplates")->child->child;
	while (child != NULL)
	{
		LoadPlayerTemplate(classes, child, version);
		child = child->next;
	}
}

void PlayerTemplatesClear(CArray *classes)
{
	CA_FOREACH(PlayerTemplate, pt, *classes)
	CFREE(pt->CharClassName);
	CFREE(pt->Hair);
	CA_FOREACH_END()
	CArrayClear(classes);
}

void PlayerTemplatesTerminate(PlayerTemplates *pt)
{
	PlayerTemplatesClear(&pt->Classes);
	CArrayTerminate(&pt->Classes);
	PlayerTemplatesClear(&pt->CustomClasses);
	CArrayTerminate(&pt->Classes);
}

PlayerTemplate *PlayerTemplateGetById(PlayerTemplates *pt, const int id)
{
	if (id < (int)pt->CustomClasses.size)
	{
		return CArrayGet(&pt->CustomClasses, id);
	}
	else if (id < (int)(pt->CustomClasses.size + pt->Classes.size))
	{
		return CArrayGet(&pt->Classes, id - (int)pt->CustomClasses.size);
	}
	return NULL;
}

static void SavePlayerTemplate(const PlayerTemplate *t, json_t *templates)
{
	json_t *template = json_new_object();
	AddStringPair(template, "Name", t->name);
	AddStringPair(template, "Face", t->CharClassName);
	AddColorPair(template, "Body", t->Colors.Body);
	AddColorPair(template, "Arms", t->Colors.Arms);
	AddColorPair(template, "Legs", t->Colors.Legs);
	AddColorPair(template, "Skin", t->Colors.Skin);
	AddColorPair(template, "Hair", t->Colors.Hair);
	AddColorPair(template, "Feet", t->Colors.Feet);
	json_insert_child(templates, template);
}
void PlayerTemplatesSave(const PlayerTemplates *pt)
{
	FILE *f = fopen(GetConfigFilePath(PLAYER_TEMPLATE_FILE), "w");
	char *text = NULL;
	char *formatText = NULL;
	json_t *root = json_new_object();

	if (f == NULL)
	{
		LOG(LM_MAIN, LL_ERROR, "saving player templates '%s'",
			PLAYER_TEMPLATE_FILE);
		goto bail;
	}

	json_insert_pair_into_object(
		root, "Version", json_new_number(TOSTRING(VERSION)));
	json_t *templatesNode = json_new_array();
	CA_FOREACH(const PlayerTemplate, t, pt->Classes)
	SavePlayerTemplate(t, templatesNode);
	CA_FOREACH_END()
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
#ifdef __EMSCRIPTEN__
		EM_ASM(
			// persist changes
			FS.syncfs(
				false, function(err) { assert(!err); }););
#endif
	}
}

void PlayerTemplateAddCharacter(CArray *classes, const Character *c)
{
	PlayerTemplate t;
	memset(&t, 0, sizeof t);
	strncpy(t.name, c->PlayerTemplateName, PLAYER_NAME_MAXLEN - 1);
	CSTRDUP(t.CharClassName, c->Class->Name);
	if (c->Hair != NULL)
	{
		CSTRDUP(t.Hair, c->Hair);
	}
	t.Colors = c->Colors;
	CArrayPushBack(classes, &t);
	LOG(LM_MAIN, LL_DEBUG, "loaded player template from characters %s (%s)",
		t.name, t.CharClassName);
}
