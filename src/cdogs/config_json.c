/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.

    Copyright (c) 2013-2014, Cong Xu
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
#include "config_json.h"

#include <locale.h>
#include <stdio.h>

#include <json/json.h>

#include "config.h"
#include "json_utils.h"
#include "keyboard.h"

#define VERSION "6"

static void ConfigLoadVisit(Config *c, json_t *node);
void ConfigLoadJSON(Config *config, const char *filename)
{
	FILE *f = fopen(filename, "r");
	json_t *root = NULL;
	int version;

	if (f == NULL)
	{
		printf("Error loading config '%s'\n", filename);
		goto bail;
	}

	if (json_stream_parse(f, &root) != JSON_OK)
	{
		printf("Error parsing config '%s'\n", filename);
		goto bail;
	}
	LoadInt(&version, root, "Version");
	ConfigLoadVisit(config, root);

	// Old config version stuff
	if (version < 5)
	{
		json_t *node = JSONFindNode(root, "Game");
		if (node != NULL)
		{
			bool moveWhenShooting = false;
			LoadBool(&moveWhenShooting, node, "MoveWhenShooting");
			Config *c = ConfigGet(config, "Game.FireMoveStyle");
			c->u.Enum.Value = moveWhenShooting ? FIREMOVE_NORMAL : FIREMOVE_STOP;
		}
	}
	if (version < 6)
	{
		json_t *node = JSONFindNode(root, "Input/Keys");
		if (node != NULL)
		{
			for (int i = 0; i < 2 && node != NULL; i++)
			{
				char buf[256];
				sprintf(buf, "Input.PlayerKeys%d", i);
				Config *c = ConfigGet(config, buf);
				ConfigLoadVisit(c, node->child);
				node = node->next;
			}
		}
	}
	if (version < 4)
	{
		json_t *node = JSONFindNode(root, "Interface");
		if (node != NULL)
		{
			bool splitscreenAlways;
			LoadBool(&splitscreenAlways, node, "SplitscreenAlways");
			Config *c = ConfigGet(config, "Interface.Splitscreen");
			c->u.Enum.Value =
				splitscreenAlways ? SPLITSCREEN_ALWAYS : SPLITSCREEN_NORMAL;
		}
	}

bail:
	json_free_value(&root);
	if (f != NULL)
	{
		fclose(f);
	}
}
static void ConfigLoadVisit(Config *c, json_t *node)
{
	if (node == NULL)
	{
		fprintf(stderr, "Error loading config: node %s not found\n",
			c->Name);
		return;
	}
	switch (c->Type)
	{
	case CONFIG_TYPE_STRING:
		CASSERT(false, "not implemented");
		break;
	case CONFIG_TYPE_INT:
		LoadInt(&c->u.Int.Value, node, c->Name);
		break;
	case CONFIG_TYPE_FLOAT:
		LoadDouble(&c->u.Float.Value, node, c->Name);
		break;
	case CONFIG_TYPE_BOOL:
		LoadBool(&c->u.Bool.Value, node, c->Name);
		break;
	case CONFIG_TYPE_ENUM:
		JSON_UTILS_LOAD_ENUM(
			c->u.Enum.Value, node, c->Name, c->u.Enum.StrToEnum);
		break;
	case CONFIG_TYPE_GROUP:
		{
			// If the config has no name, then it is the root element
			// Load children directly to the node
			// Otherwise, find the named child
			if (c->Name != NULL)
			{
				node = json_find_first_label(node, c->Name);
				if (node == NULL)
				{
					fprintf(stderr, "Error loading config: node %s not found\n",
						c->Name);
					return;
				}
				node = node->child;
			}
			for (int i = 0; i < (int)c->u.Group.size; i++)
			{
				ConfigLoadVisit(CArrayGet(&c->u.Group, i), node);
			}
		}
		break;
	default:
		CASSERT(false, "Unknown config type");
		break;
	}
}

static void ConfigSaveVisit(const Config *c, json_t *node);
void ConfigSaveJSON(const Config *config, const char *filename)
{
	FILE *f = fopen(filename, "w");
	char *text = NULL;
	json_t *root;

	if (f == NULL)
	{
		printf("Error saving config '%s'\n", filename);
		return;
	}

	setlocale(LC_ALL, "");

	root = json_new_object();
	json_insert_pair_into_object(root, "Version", json_new_number(VERSION));
	ConfigSaveVisit(config, root);

	json_tree_to_string(root, &text);
	char *formatText = json_format_string(text);
	fputs(formatText, f);

	// clean up
	CFREE(formatText);
	CFREE(text);
	json_free_value(&root);

	fclose(f);
}
static void ConfigSaveVisit(const Config *c, json_t *node)
{
	switch (c->Type)
	{
	case CONFIG_TYPE_STRING:
		CASSERT(false, "not implemented");
		break;
	case CONFIG_TYPE_INT:
		AddIntPair(node, c->Name, c->u.Int.Value);
		break;
	case CONFIG_TYPE_FLOAT:
		CASSERT(false, "not implemented");
		break;
	case CONFIG_TYPE_BOOL:
		json_insert_pair_into_object(
			node, c->Name, json_new_bool(c->u.Bool.Value));
		break;
	case CONFIG_TYPE_ENUM:
		JSON_UTILS_ADD_ENUM_PAIR(
			node, c->Name, c->u.Enum.Value, c->u.Enum.EnumToStr);
		break;
	case CONFIG_TYPE_GROUP:
		{
			json_t *child = node;
			// If the config has no name, then it is the root element
			// Add children directly to the node
			// Otherwise, create a new child
			if (c->Name != NULL)
			{
				child = json_new_object();
			}
			for (int i = 0; i < (int)c->u.Group.size; i++)
			{
				ConfigSaveVisit(CArrayGet(&c->u.Group, i), child);
			}
			if (c->Name != NULL)
			{
				json_insert_pair_into_object(node, c->Name, child);
			}
		}
		break;
	default:
		CASSERT(false, "Unknown config type");
		break;
	}
}

int ConfigGetJSONVersion(FILE *f)
{
	json_t *root = NULL;
	json_t *child = NULL;
	int version = -1;

	if (json_stream_parse(f, &root) != JSON_OK)
	{
		printf("Error parsing JSON config\n");
		goto bail;
	}
	child = json_find_first_label(root, "Version");
	if (child == NULL)
	{
		printf("Error parsing JSON config version\n");
		goto bail;
	}
	version = atoi(child->child->text);

bail:
	json_free_value(&root);
	return version;
}
