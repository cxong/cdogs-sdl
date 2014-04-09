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

#define VERSION "4"


static void LoadGameConfigNode(GameConfig *config, json_t *node)
{
	if (node == NULL)
	{
		return;
	}
	node = node->child;
	LoadBool(&config->FriendlyFire, node, "FriendlyFire");
	config->RandomSeed = atoi(json_find_first_label(node, "RandomSeed")->child->text);
	JSON_UTILS_LOAD_ENUM(config->Difficulty, node, "Difficulty", StrDifficulty);
	LoadBool(&config->SlowMotion, node, "SlowMotion");
	LoadInt(&config->EnemyDensity, node, "EnemyDensity");
	LoadInt(&config->NonPlayerHP, node, "NonPlayerHP");
	LoadInt(&config->PlayerHP, node, "PlayerHP");
	LoadBool(&config->Fog, node, "Fog");
	LoadInt(&config->SightRange, node, "SightRange");
	LoadBool(&config->Shadows, node, "Shadows");
	LoadBool(&config->MoveWhenShooting, node, "MoveWhenShooting");
	JSON_UTILS_LOAD_ENUM(
		config->SwitchMoveStyle, node, "SwitchMoveStyle", StrSwitchMoveStyle);
	LoadBool(&config->ShotsPushback, node, "ShotsPushback");
	JSON_UTILS_LOAD_ENUM(
		config->AllyCollision, node, "AllyCollision", StrAllyCollision);
	LoadBool(&config->HealthPickups, node, "HealthPickups");
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
	JSON_UTILS_ADD_ENUM_PAIR(subConfig, "Difficulty", config->Difficulty, DifficultyStr);
	json_insert_pair_into_object(
		subConfig, "SlowMotion", json_new_bool(config->SlowMotion));
	AddIntPair(subConfig, "EnemyDensity", config->EnemyDensity);
	AddIntPair(subConfig, "NonPlayerHP", config->NonPlayerHP);
	AddIntPair(subConfig, "PlayerHP", config->PlayerHP);
	json_insert_pair_into_object(
		subConfig, "Fog", json_new_bool(config->Fog));
	AddIntPair(subConfig, "SightRange", config->SightRange);
	json_insert_pair_into_object(
		subConfig, "Shadows", json_new_bool(config->Shadows));
	json_insert_pair_into_object(
		subConfig, "MoveWhenShooting", json_new_bool(config->MoveWhenShooting));
	JSON_UTILS_ADD_ENUM_PAIR(
		subConfig, "SwitchMoveStyle", config->SwitchMoveStyle, SwitchMoveStyleStr);
	json_insert_pair_into_object(
		subConfig, "ShotsPushback", json_new_bool(config->ShotsPushback));
	JSON_UTILS_ADD_ENUM_PAIR(
		subConfig, "AllyCollision", config->AllyCollision, AllyCollisionStr);
	json_insert_pair_into_object(
		subConfig, "HealthPickups", json_new_bool(config->HealthPickups));
	json_insert_pair_into_object(root, "Game", subConfig);
}

static void LoadGraphicsConfigNode(GraphicsConfig *config, json_t *node)
{
	if (node == NULL)
	{
		return;
	}
	node = node->child;
	LoadInt(&config->Brightness, node, "Brightness");
	LoadInt(&config->Res.x, node, "ResolutionWidth");
	LoadInt(&config->Res.y, node, "ResolutionHeight");
	LoadBool(&config->Fullscreen, node, "Fullscreen");
	LoadInt(&config->ScaleFactor, node, "ScaleFactor");
	LoadInt(&config->ShakeMultiplier, node, "ShakeMultiplier");
	JSON_UTILS_LOAD_ENUM(config->ScaleMode, node, "ScaleMode", StrScaleMode);
}
static void AddGraphicsConfigNode(GraphicsConfig *config, json_t *root)
{
	json_t *subConfig = json_new_object();
	AddIntPair(subConfig, "Brightness", config->Brightness);
	AddIntPair(subConfig, "ResolutionWidth", config->Res.x);
	AddIntPair(subConfig, "ResolutionHeight", config->Res.y);
	json_insert_pair_into_object(
		subConfig, "Fullscreen", json_new_bool(config->Fullscreen));
	AddIntPair(subConfig, "ScaleFactor", config->ScaleFactor);
	AddIntPair(subConfig, "ShakeMultiplier", config->ShakeMultiplier);
	JSON_UTILS_ADD_ENUM_PAIR(subConfig, "ScaleMode", config->ScaleMode, ScaleModeStr);
	json_insert_pair_into_object(root, "Graphics", subConfig);
}

static void LoadKeysNode(input_keys_t *keys, json_t *node)
{
	if (node == NULL)
	{
		return;
	}
	node = node->child;
	LoadInt(&keys->left, node, "left");
	LoadInt(&keys->right, node, "right");
	LoadInt(&keys->up, node, "up");
	LoadInt(&keys->down, node, "down");
	LoadInt(&keys->button1, node, "button1");
	LoadInt(&keys->button2, node, "button2");
	LoadInt(&keys->map, node, "map");
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
	LoadKeysNode(&config->Keys, json_find_first_label(node, "Keys"));
}
static void AddKeyConfigNode(KeyConfig *config, json_t *parent)
{
	json_t *subConfig = json_new_object();
	AddKeysNode(&config->Keys, subConfig);
	json_insert_child(parent, subConfig);
}

static void LoadInputConfigNode(InputConfig *config, json_t *node)
{
	int i;
	json_t *keyNode;
	if (node == NULL)
	{
		return;
	}
	node = node->child;
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
	for (i = 0; i < 2; i++)
	{
		AddKeyConfigNode(&config->PlayerKeys[i], keyConfigs);
	}
	json_insert_pair_into_object(subConfig, "Keys", keyConfigs);
	json_insert_pair_into_object(root, "Input", subConfig);
}

static void LoadInterfaceConfigNode(
	InterfaceConfig *config, json_t *node, int version)
{
	if (node == NULL)
	{
		return;
	}
	node = node->child;
	LoadBool(&config->ShowFPS, node, "ShowFPS");
	LoadBool(&config->ShowTime, node, "ShowTime");
	if (version < 4)
	{
		bool splitscreenAlways;
		LoadBool(&splitscreenAlways, node, "SplitscreenAlways");
		config->Splitscreen =
			splitscreenAlways ? SPLITSCREEN_ALWAYS : SPLITSCREEN_NORMAL;
	}
	else
	{
		config->Splitscreen = StrSplitscreenStyle(
			json_find_first_label(node, "Splitscreen")->child->text);
	}
	LoadBool(&config->ShowHUDMap, node, "ShowHUDMap");
}
static void AddInterfaceConfigNode(InterfaceConfig *config, json_t *root)
{
	json_t *subConfig = json_new_object();
	json_insert_pair_into_object(
		subConfig, "ShowFPS", json_new_bool(config->ShowFPS));
	json_insert_pair_into_object(
		subConfig, "ShowTime", json_new_bool(config->ShowTime));
	json_insert_pair_into_object(
		subConfig, "Splitscreen",
		json_new_string(SplitscreenStyleStr(config->Splitscreen)));
	json_insert_pair_into_object(
		subConfig, "ShowHUDMap", json_new_bool(config->ShowHUDMap));
	json_insert_pair_into_object(root, "Interface", subConfig);
}

static void LoadSoundConfigNode(SoundConfig *config, json_t *node)
{
	if (node == NULL)
	{
		return;
	}
	node = node->child;
	LoadInt(&config->SoundVolume, node, "SoundVolume");
	LoadInt(&config->MusicVolume, node, "MusicVolume");
	LoadInt(&config->SoundChannels, node, "SoundChannels");
	LoadBool(&config->Footsteps, node, "Footsteps");
	LoadBool(&config->Hits, node, "Hits");
	LoadBool(&config->Reloads, node, "Reloads");
}
static void AddSoundConfigNode(SoundConfig *config, json_t *root)
{
	json_t *subConfig = json_new_object();
	AddIntPair(subConfig, "SoundVolume", config->SoundVolume);
	AddIntPair(subConfig, "MusicVolume", config->MusicVolume);
	AddIntPair(subConfig, "SoundChannels", config->SoundChannels);
	json_insert_pair_into_object(
		subConfig, "Footsteps", json_new_bool(config->Footsteps));
	json_insert_pair_into_object(
		subConfig, "Hits", json_new_bool(config->Hits));
	json_insert_pair_into_object(
		subConfig, "Reloads", json_new_bool(config->Reloads));
	json_insert_pair_into_object(root, "Sound", subConfig);
}

static void LoadQuickPlayConfigNode(QuickPlayConfig *config, json_t *node)
{
	if (node == NULL)
	{
		return;
	}
	node = node->child;
	config->MapSize = StrQuickPlayQuantity(json_find_first_label(node, "MapSize")->child->text);
	config->WallCount = StrQuickPlayQuantity(json_find_first_label(node, "WallCount")->child->text);
	config->WallLength = StrQuickPlayQuantity(json_find_first_label(node, "WallLength")->child->text);
	config->RoomCount = StrQuickPlayQuantity(json_find_first_label(node, "RoomCount")->child->text);
	config->SquareCount = StrQuickPlayQuantity(json_find_first_label(node, "SquareCount")->child->text);
	config->EnemyCount = StrQuickPlayQuantity(json_find_first_label(node, "EnemyCount")->child->text);
	config->EnemySpeed = StrQuickPlayQuantity(json_find_first_label(node, "EnemySpeed")->child->text);
	config->EnemyHealth = StrQuickPlayQuantity(json_find_first_label(node, "EnemyHealth")->child->text);
	LoadBool(&config->EnemiesWithExplosives, node, "EnemiesWithExplosives");
	config->ItemCount = StrQuickPlayQuantity(json_find_first_label(node, "ItemCount")->child->text);
}
static void AddQuickPlayConfigNode(QuickPlayConfig *config, json_t *root)
{
	json_t *subConfig = json_new_object();
	json_insert_pair_into_object(
		subConfig, "MapSize", json_new_string(QuickPlayQuantityStr(config->MapSize)));
	json_insert_pair_into_object(
		subConfig, "WallCount", json_new_string(QuickPlayQuantityStr(config->WallCount)));
	json_insert_pair_into_object(
		subConfig, "WallLength", json_new_string(QuickPlayQuantityStr(config->WallLength)));
	json_insert_pair_into_object(
		subConfig, "RoomCount", json_new_string(QuickPlayQuantityStr(config->RoomCount)));
	json_insert_pair_into_object(
		subConfig, "SquareCount", json_new_string(QuickPlayQuantityStr(config->SquareCount)));
	json_insert_pair_into_object(
		subConfig, "EnemyCount", json_new_string(QuickPlayQuantityStr(config->EnemyCount)));
	json_insert_pair_into_object(
		subConfig, "EnemySpeed", json_new_string(QuickPlayQuantityStr(config->EnemySpeed)));
	json_insert_pair_into_object(
		subConfig, "EnemyHealth", json_new_string(QuickPlayQuantityStr(config->EnemyHealth)));
	json_insert_pair_into_object(
		subConfig, "EnemiesWithExplosives", json_new_bool(config->EnemiesWithExplosives));
	json_insert_pair_into_object(
		subConfig, "ItemCount", json_new_string(QuickPlayQuantityStr(config->ItemCount)));
	json_insert_pair_into_object(root, "QuickPlay", subConfig);
}


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
	LoadGameConfigNode(&config->Game, json_find_first_label(root, "Game"));
	LoadGraphicsConfigNode(&config->Graphics, json_find_first_label(root, "Graphics"));
	LoadInputConfigNode(&config->Input, json_find_first_label(root, "Input"));
	LoadInterfaceConfigNode(
		&config->Interface,
		json_find_first_label(root, "Interface"),
		version);
	LoadSoundConfigNode(&config->Sound, json_find_first_label(root, "Sound"));
	LoadQuickPlayConfigNode(&config->QuickPlay, json_find_first_label(root, "QuickPlay"));

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
	json_insert_pair_into_object(root, "Version", json_new_number(VERSION));
	AddGameConfigNode(&config->Game, root);
	AddGraphicsConfigNode(&config->Graphics, root);
	AddInputConfigNode(&config->Input, root);
	AddInterfaceConfigNode(&config->Interface, root);
	AddSoundConfigNode(&config->Sound, root);
	AddQuickPlayConfigNode(&config->QuickPlay, root);

	json_tree_to_string(root, &text);
	char *formatText = json_format_string(text);
	fputs(formatText, f);

	// clean up
	CFREE(formatText);
	CFREE(text);
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
