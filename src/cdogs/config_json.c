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
#include "config_json.h"

#include <locale.h>
#include <stdio.h>

#include <json/json.h>

#include "keyboard.h"


static void AddIntPair(json_t *parent, const char *name, int number)
{
	char buf[32];
	sprintf(buf, "%d", number);
	json_insert_pair_into_object(parent, name, json_new_number(buf));
}

static void LoadGameConfigNode(GameConfig *config, json_t *node)
{
	config->FriendlyFire = json_find_first_label(node, "FriendlyFire")->child->type == JSON_TRUE;
	config->RandomSeed = atoi(json_find_first_label(node, "RandomSeed")->child->text);
	config->Difficulty = StrDifficulty(json_find_first_label(node, "Difficulty")->child->text);
	config->SlowMotion = json_find_first_label(node, "SlowMotion")->child->type == JSON_TRUE;
	config->EnemyDensity = atoi(json_find_first_label(node, "EnemyDensity")->child->text);
	config->NonPlayerHP = atoi(json_find_first_label(node, "NonPlayerHP")->child->text);
	config->PlayerHP = atoi(json_find_first_label(node, "PlayerHP")->child->text);
}
static void AddGameConfigNode(GameConfig *config, json_t *root)
{
	char buf[32];
	json_t *subConfig = json_new_object();
	json_insert_pair_into_object(
		subConfig, "FriendlyFire", json_new_bool(config->FriendlyFire));
	sprintf(buf, "%u", config->RandomSeed);
	json_insert_pair_into_object(
		subConfig, "RandomSeed", json_new_number(buf));
	json_insert_pair_into_object(
		subConfig, "Difficulty", json_new_string(DifficultyStr(config->Difficulty)));
	json_insert_pair_into_object(
		subConfig, "SlowMotion", json_new_bool(config->SlowMotion));
	AddIntPair(subConfig, "EnemyDensity", config->EnemyDensity);
	AddIntPair(subConfig, "NonPlayerHP", config->NonPlayerHP);
	AddIntPair(subConfig, "PlayerHP", config->PlayerHP);
	json_insert_pair_into_object(root, "Game", subConfig);
}

static void LoadGraphicsConfigNode(GraphicsConfig *config, json_t *node)
{
	config->Brightness = atoi(json_find_first_label(node, "Brightness")->child->text);
	config->ResolutionWidth = atoi(json_find_first_label(node, "ResolutionWidth")->child->text);
	config->ResolutionHeight = atoi(json_find_first_label(node, "ResolutionHeight")->child->text);
	config->Fullscreen = json_find_first_label(node, "Fullscreen")->child->type == JSON_TRUE;
	config->ScaleFactor = atoi(json_find_first_label(node, "ScaleFactor")->child->text);
}
static void AddGraphicsConfigNode(GraphicsConfig *config, json_t *root)
{
	json_t *subConfig = json_new_object();
	AddIntPair(subConfig, "Brightness", config->Brightness);
	AddIntPair(subConfig, "ResolutionWidth", config->ResolutionWidth);
	AddIntPair(subConfig, "ResolutionHeight", config->ResolutionHeight);
	json_insert_pair_into_object(
		subConfig, "Fullscreen", json_new_bool(config->Fullscreen));
	AddIntPair(subConfig, "ScaleFactor", config->ScaleFactor);
	json_insert_pair_into_object(root, "Graphics", subConfig);
}

static void LoadKeysNode(input_keys_t *keys, json_t *node)
{
	keys->left = atoi(json_find_first_label(node, "left")->child->text);
	keys->right = atoi(json_find_first_label(node, "right")->child->text);
	keys->up = atoi(json_find_first_label(node, "up")->child->text);
	keys->down = atoi(json_find_first_label(node, "down")->child->text);
	keys->button1 = atoi(json_find_first_label(node, "button1")->child->text);
	keys->button2 = atoi(json_find_first_label(node, "button2")->child->text);
	keys->map = atoi(json_find_first_label(node, "map")->child->text);
}
static void AddKeysNode(input_keys_t *keys, json_t *parent)
{
	json_t *subConfig = json_new_object();
	// TODO: key names (there's no key name to int conversion)
	AddIntPair(subConfig, "left", keys->left);
	AddIntPair(subConfig, "right", keys->right);
	AddIntPair(subConfig, "up", keys->up);
	AddIntPair(subConfig, "down", keys->down);
	AddIntPair(subConfig, "button1", keys->button1);
	AddIntPair(subConfig, "button2", keys->button2);
	AddIntPair(subConfig, "map", keys->map);
	json_insert_pair_into_object(parent, "Keys", subConfig);
}

static void LoadKeyConfigNode(KeyConfig *config, json_t *node)
{
	config->Device = StrInputDevice(json_find_first_label(node, "Device")->child->text);
	LoadKeysNode(&config->Keys, json_find_first_label(node, "Keys")->child);
}
static void AddKeyConfigNode(KeyConfig *config, json_t *parent)
{
	json_t *subConfig = json_new_object();
	json_insert_pair_into_object(
		subConfig, "Device", json_new_string(InputDeviceStr(config->Device)));
	AddKeysNode(&config->Keys, subConfig);
	json_insert_child(parent, subConfig);
}

static void LoadInputConfigNode(InputConfig *config, json_t *node)
{
	int i;
	json_t *keyNode;
	config->SwapButtonsJoystick1 = json_find_first_label(node, "SwapButtonsJoystick1")->child->type == JSON_TRUE;
	config->SwapButtonsJoystick2 = json_find_first_label(node, "SwapButtonsJoystick2")->child->type == JSON_TRUE;
	keyNode = json_find_first_label(node, "Keys")->child->child;
	for (i = 0; i < 2 && keyNode != NULL; i++)
	{
		LoadKeyConfigNode(&config->PlayerKeys[i], keyNode);
		keyNode = keyNode->next;
	}
}
static void AddInputConfigNode(InputConfig *config, json_t *root)
{
	int i;
	json_t *subConfig = json_new_object();
	json_t *keyConfigs = json_new_array();
	json_insert_pair_into_object(
		subConfig, "SwapButtonsJoystick1", json_new_bool(config->SwapButtonsJoystick1));
	json_insert_pair_into_object(
		subConfig, "SwapButtonsJoystick2", json_new_bool(config->SwapButtonsJoystick2));
	for (i = 0; i < 2; i++)
	{
		AddKeyConfigNode(&config->PlayerKeys[i], keyConfigs);
	}
	json_insert_pair_into_object(subConfig, "Keys", keyConfigs);
	json_insert_pair_into_object(root, "Input", subConfig);
}

static void LoadInterfaceConfigNode(InterfaceConfig *config, json_t *node)
{
	config->ShowFPS = json_find_first_label(node, "ShowFPS")->child->type == JSON_TRUE;
	config->ShowTime = json_find_first_label(node, "ShowTime")->child->type == JSON_TRUE;
	config->SplitscreenAlways = json_find_first_label(node, "SplitscreenAlways")->child->type == JSON_TRUE;
}
static void AddInterfaceConfigNode(InterfaceConfig *config, json_t *root)
{
	json_t *subConfig = json_new_object();
	json_insert_pair_into_object(
		subConfig, "ShowFPS", json_new_bool(config->ShowFPS));
	json_insert_pair_into_object(
		subConfig, "ShowTime", json_new_bool(config->ShowTime));
	json_insert_pair_into_object(
		subConfig, "SplitscreenAlways", json_new_bool(config->SplitscreenAlways));
	json_insert_pair_into_object(root, "Interface", subConfig);
}

static void LoadSoundConfigNode(SoundConfig *config, json_t *node)
{
	config->SoundVolume = atoi(json_find_first_label(node, "SoundVolume")->child->text);
	config->MusicVolume = atoi(json_find_first_label(node, "MusicVolume")->child->text);
	config->SoundChannels = atoi(json_find_first_label(node, "SoundChannels")->child->text);
}
static void AddSoundConfigNode(SoundConfig *config, json_t *root)
{
	json_t *subConfig = json_new_object();
	AddIntPair(subConfig, "SoundVolume", config->SoundVolume);
	AddIntPair(subConfig, "MusicVolume", config->MusicVolume);
	AddIntPair(subConfig, "SoundChannels", config->SoundChannels);
	json_insert_pair_into_object(root, "Sound", subConfig);
}


void ConfigLoadJSON(Config *config, const char *filename)
{
	FILE *f = fopen(filename, "r");
	json_t *root = NULL;

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
	LoadGameConfigNode(&config->Game, json_find_first_label(root, "Game")->child);
	LoadGraphicsConfigNode(&config->Graphics, json_find_first_label(root, "Graphics")->child);
	LoadInputConfigNode(&config->Input, json_find_first_label(root, "Input")->child);
	LoadInterfaceConfigNode(&config->Interface, json_find_first_label(root, "Interface")->child);
	LoadSoundConfigNode(&config->Sound, json_find_first_label(root, "Sound")->child);

bail:
	json_free_value(&root);
	if (f != NULL)
	{
		fclose(f);
	}
}

void ConfigSaveJSON(Config *config, const char *filename)
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
	json_insert_pair_into_object(root, "Version", json_new_number("1"));
	AddGameConfigNode(&config->Game, root);
	AddGraphicsConfigNode(&config->Graphics, root);
	AddInputConfigNode(&config->Input, root);
	AddInterfaceConfigNode(&config->Interface, root);
	AddSoundConfigNode(&config->Sound, root);

	json_tree_to_string(root, &text);
	fputs(json_format_string(text), f);

	// clean up
	free(text);
	json_free_value(&root);

	fclose(f);
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
