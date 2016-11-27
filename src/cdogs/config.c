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
#include "config.h"

#include <limits.h>
#include <stdio.h>

#include "blit.h"
#include "collision.h"
#include "config_json.h"
#include "config_old.h"
#include "keyboard.h"
#include "music.h"
#include "sounds.h"
#include "utils.h"


const char *DifficultyStr(int d)
{
	switch (d)
	{
		T2S(DIFFICULTY_VERYEASY, "Easiest");
		T2S(DIFFICULTY_EASY, "Easy");
		T2S(DIFFICULTY_NORMAL, "Normal");
		T2S(DIFFICULTY_HARD, "Hard");
		T2S(DIFFICULTY_VERYHARD, "Very hard");
	default:
		return "";
	}
}
int StrDifficulty(const char *s)
{
	S2T(DIFFICULTY_VERYEASY, "Easiest");
	S2T(DIFFICULTY_EASY, "Easy");
	S2T(DIFFICULTY_NORMAL, "Normal");
	S2T(DIFFICULTY_HARD, "Hard");
	S2T(DIFFICULTY_VERYHARD, "Very hard");
	return DIFFICULTY_NORMAL;
}
const char *FireMoveStyleStr(int s)
{
	switch (s)
	{
		T2S(FIREMOVE_STOP, "Stop");
		T2S(FIREMOVE_NORMAL, "Normal");
		T2S(FIREMOVE_STRAFE, "Strafe");
	default:
		return "";
	}
}
int StrFireMoveStyle(const char *s)
{
	S2T(FIREMOVE_STOP, "Stop");
	S2T(FIREMOVE_NORMAL, "Normal");
	S2T(FIREMOVE_STRAFE, "Strafe");
	return FIREMOVE_STOP;
}
const char *SwitchMoveStyleStr(int s)
{
	switch (s)
	{
		T2S(SWITCHMOVE_SLIDE, "Slide");
		T2S(SWITCHMOVE_STRAFE, "Strafe");
		T2S(SWITCHMOVE_NONE, "None");
	default:
		return "";
	}
}
int StrSwitchMoveStyle(const char *s)
{
	S2T(SWITCHMOVE_SLIDE, "Slide");
	S2T(SWITCHMOVE_STRAFE, "Strafe");
	S2T(SWITCHMOVE_NONE, "None");
	return SWITCHMOVE_SLIDE;
}
const char *ScaleModeStr(int s)
{
	switch (s)
	{
		T2S(SCALE_MODE_NN, "Nearest neighbor");
		T2S(SCALE_MODE_BILINEAR, "Bilinear");
	default:
		return "";
	}
}
int StrScaleMode(const char *s)
{
	S2T(SCALE_MODE_NN, "Nearest neighbor");
	S2T(SCALE_MODE_BILINEAR, "Bilinear");
	return SCALE_MODE_NN;
}
const char *GoreAmountStr(int g)
{
	switch (g)
	{
		T2S(GORE_NONE, "None");
		T2S(GORE_LOW, "Trickle");
		T2S(GORE_MEDIUM, "Buckets");
		T2S(GORE_HIGH, "Torrents");
	default:
		return "";
	}
}
int StrGoreAmount(const char *s)
{
	S2T(GORE_NONE, "None");
	S2T(GORE_LOW, "Trickle");
	S2T(GORE_MEDIUM, "Buckets");
	S2T(GORE_HIGH, "Torrents");
	return GORE_NONE;
}
const char *LaserSightStr(int l)
{
	switch (l)
	{
		T2S(LASER_SIGHT_NONE, "None");
		T2S(LASER_SIGHT_PLAYERS, "Players only");
		T2S(LASER_SIGHT_ALL, "All");
	default:
		return "";
	}
}
int StrLaserSight(const char *s)
{
	S2T(LASER_SIGHT_NONE, "None");
	S2T(LASER_SIGHT_PLAYERS, "Players only");
	S2T(LASER_SIGHT_ALL, "All");
	return LASER_SIGHT_NONE;
}
const char *SplitscreenStyleStr(int s)
{
	switch (s)
	{
		T2S(SPLITSCREEN_NORMAL, "Normal");
		T2S(SPLITSCREEN_ALWAYS, "Always");
		T2S(SPLITSCREEN_NEVER, "Never");
		default:
			return "";
	}
}
int StrSplitscreenStyle(const char *s)
{
	S2T(SPLITSCREEN_NORMAL, "Normal");
	S2T(SPLITSCREEN_ALWAYS, "Always");
	S2T(SPLITSCREEN_NEVER, "Never");
	return SPLITSCREEN_NORMAL;
}
const char *AIChatterStr(int c)
{
	switch (c)
	{
		T2S(AICHATTER_NONE, "None");
		T2S(AICHATTER_SELDOM, "Seldom");
		T2S(AICHATTER_OFTEN, "Often");
		T2S(AICHATTER_ALWAYS, "Always");
	default:
		return "";
	}
}
int StrAIChatter(const char *s)
{
	S2T(AICHATTER_NONE, "None");
	S2T(AICHATTER_SELDOM, "Seldom");
	S2T(AICHATTER_OFTEN, "Often");
	S2T(AICHATTER_ALWAYS, "Always");
	return AICHATTER_NONE;
}
const char *QuickPlayQuantityStr(int q)
{
	switch (q)
	{
		T2S(QUICKPLAY_QUANTITY_ANY, "Any");
		T2S(QUICKPLAY_QUANTITY_SMALL, "Small");
		T2S(QUICKPLAY_QUANTITY_MEDIUM, "Medium");
		T2S(QUICKPLAY_QUANTITY_LARGE, "Large");
	default:
		return "";
	}
}
int StrQuickPlayQuantity(const char *s)
{
	S2T(QUICKPLAY_QUANTITY_ANY, "Any");
	S2T(QUICKPLAY_QUANTITY_SMALL, "Small");
	S2T(QUICKPLAY_QUANTITY_MEDIUM, "Medium");
	S2T(QUICKPLAY_QUANTITY_LARGE, "Large");
	return QUICKPLAY_QUANTITY_ANY;
}


Config gConfig;

static Config ConfigNew(const char *name, const ConfigType type);
Config ConfigNewString(const char *name, const char *defaultValue)
{
	Config c = ConfigNew(name, CONFIG_TYPE_STRING);
	UNUSED(defaultValue);
	CASSERT(false, "unimplemented");
	return c;
}
Config ConfigNewInt(
	const char *name, const int defaultValue,
	const int minValue, const int maxValue, const int increment,
	int (*strToInt)(const char *), char *(*intToStr)(int))
{
	Config c = ConfigNew(name, CONFIG_TYPE_INT);
	c.u.Int.Default = c.u.Int.Value = c.u.Int.Last = defaultValue;
	c.u.Int.Min = minValue;
	c.u.Int.Max = maxValue;
	c.u.Int.Increment = increment;
	c.u.Int.StrToInt = strToInt ? strToInt : atoi;
	c.u.Int.IntToStr = intToStr ? intToStr : IntStr;
	return c;
}
Config ConfigNewFloat(
	const char *name, const double defaultValue,
	const double minValue, const double maxValue, const double increment)
{
	Config c = ConfigNew(name, CONFIG_TYPE_FLOAT);
	c.u.Float.Default = c.u.Float.Value = c.u.Float.Last = defaultValue;
	c.u.Float.Min = minValue;
	c.u.Float.Max = maxValue;
	c.u.Float.Increment = increment;
	return c;
}
Config ConfigNewBool(const char *name, const bool defaultValue)
{
	Config c = ConfigNew(name, CONFIG_TYPE_BOOL);
	c.u.Bool.Default = c.u.Bool.Value = c.u.Bool.Last = defaultValue;
	return c;
}
Config ConfigNewEnum(
	const char *name, const int defaultValue,
	const int minValue, const int maxValue,
	int (*strToEnum)(const char *), const char *(*enumToStr)(int))
{
	Config c = ConfigNew(name, CONFIG_TYPE_ENUM);
	c.u.Enum.Default = c.u.Enum.Value = c.u.Enum.Last = defaultValue;
	c.u.Enum.Min = minValue;
	c.u.Enum.Max = maxValue;
	c.u.Enum.StrToEnum = strToEnum;
	c.u.Enum.EnumToStr = enumToStr;
	return c;
}
Config ConfigNewGroup(const char *name)
{
	Config c = ConfigNew(name, CONFIG_TYPE_GROUP);
	CArrayInit(&c.u.Group, sizeof(Config));
	return c;
}
static Config ConfigNew(const char *name, const ConfigType type)
{
	Config c;
	memset(&c, 0, sizeof c);
	if (name != NULL)
	{
		CSTRDUP(c.Name, name);
	}
	c.Type = type;
	return c;
}

void ConfigDestroy(Config *c)
{
	CFREE(c->Name);
	if (c->Type == CONFIG_TYPE_GROUP)
	{
		CA_FOREACH(Config, child, c->u.Group)
			ConfigDestroy(child);
		CA_FOREACH_END()
		CArrayTerminate(&c->u.Group);
	}
}

void ConfigGroupAdd(Config *group, Config child)
{
	CASSERT(group->Type == CONFIG_TYPE_GROUP, "Invalid config type");
	CArrayPushBack(&group->u.Group, &child);
}

int ConfigGetVersion(FILE *f)
{
	if (ConfigIsOld(f))
	{
		return 0;
	}
	rewind(f);
	return ConfigGetJSONVersion(f);
}

Config *ConfigGet(Config *c, const char *name)
{
	char *nameCopy;
	CSTRDUP(nameCopy, name);
	char *pch = strtok(nameCopy, ".");
	while (pch != NULL)
	{
		if (c->Type != CONFIG_TYPE_GROUP)
		{
			CASSERT(false, "Invalid config type");
			goto bail;
		}
		bool found = false;
		CA_FOREACH(Config, child, c->u.Group)
			if (strcmp(child->Name, pch) == 0)
			{
				c = child;
				found = true;
				break;
			}
		CA_FOREACH_END()
		if (!found)
		{
			CASSERT(false, "Config not found");
			goto bail;
		}
		pch = strtok(NULL, ".");
	}
bail:
	CFREE(nameCopy);
	return c;
}

bool ConfigChanged(const Config *c)
{
	switch (c->Type)
	{
	case CONFIG_TYPE_STRING:
		return strcmp(c->u.String.Value, c->u.String.Last) != 0;
	case CONFIG_TYPE_INT:
		return c->u.Int.Value != c->u.Int.Last;
	case CONFIG_TYPE_FLOAT:
		return c->u.Float.Value != c->u.Float.Last;
	case CONFIG_TYPE_BOOL:
		return c->u.Bool.Value != c->u.Bool.Last;
	case CONFIG_TYPE_ENUM:
		return c->u.Enum.Value != c->u.Enum.Last;
	case CONFIG_TYPE_GROUP:
		CA_FOREACH(Config, child, c->u.Group)
			if (ConfigChanged(child))
			{
				return true;
			}
		CA_FOREACH_END()
		return false;
	default:
		CASSERT(false, "Unknown config type");
		return false;
	}
}

void ConfigResetChanged(Config *c)
{
	switch (c->Type)
	{
	case CONFIG_TYPE_STRING:
		CFREE(c->u.String.Value);
		if (c->u.String.Last != NULL)
		{
			CSTRDUP(c->u.String.Value, c->u.String.Last);
		}
		break;
	case CONFIG_TYPE_INT:
		c->u.Int.Value = c->u.Int.Last;
		break;
	case CONFIG_TYPE_FLOAT:
		c->u.Float.Value = c->u.Float.Last;
		break;
	case CONFIG_TYPE_BOOL:
		c->u.Bool.Value = c->u.Bool.Last;
		break;
	case CONFIG_TYPE_ENUM:
		c->u.Enum.Value = c->u.Enum.Last;
		break;
	case CONFIG_TYPE_GROUP:
		CA_FOREACH(Config, child, c->u.Group)
			ConfigResetChanged(child);
		CA_FOREACH_END()
		break;
	default:
		CASSERT(false, "Unknown config type");
		break;
	}
}

void ConfigSetChanged(Config *c)
{
	switch (c->Type)
	{
	case CONFIG_TYPE_STRING:
		CFREE(c->u.String.Last);
		if (c->u.String.Value != NULL)
		{
			CSTRDUP(c->u.String.Last, c->u.String.Value);
		}
		break;
	case CONFIG_TYPE_INT:
		c->u.Int.Last = c->u.Int.Value;
		break;
	case CONFIG_TYPE_FLOAT:
		c->u.Float.Last = c->u.Float.Value;
		break;
	case CONFIG_TYPE_BOOL:
		c->u.Bool.Last = c->u.Bool.Value;
		break;
	case CONFIG_TYPE_ENUM:
		c->u.Enum.Last = c->u.Enum.Value;
		break;
	case CONFIG_TYPE_GROUP:
		CA_FOREACH(Config, child, c->u.Group)
			ConfigSetChanged(child);
		CA_FOREACH_END()
		break;
	default:
		CASSERT(false, "Unknown config type");
		break;
	}
}

void ConfigResetDefault(Config *c)
{
	switch (c->Type)
	{
	case CONFIG_TYPE_STRING:
		CFREE(c->u.String.Value);
		if (c->u.String.Default != NULL)
		{
			CSTRDUP(c->u.String.Value, c->u.String.Default);
		}
		break;
	case CONFIG_TYPE_INT:
		c->u.Int.Value = c->u.Int.Default;
		break;
	case CONFIG_TYPE_FLOAT:
		c->u.Float.Value = c->u.Float.Default;
		break;
	case CONFIG_TYPE_BOOL:
		c->u.Bool.Value = c->u.Bool.Default;
		break;
	case CONFIG_TYPE_ENUM:
		c->u.Enum.Value = c->u.Enum.Default;
		break;
	case CONFIG_TYPE_GROUP:
		CA_FOREACH(Config, child, c->u.Group)
			ConfigResetDefault(child);
		CA_FOREACH_END()
		break;
	default:
		CASSERT(false, "Unknown config type");
		break;
	}
}

const char *ConfigGetString(Config *c, const char *name)
{
	c = ConfigGet(c, name);
	CASSERT(c->Type == CONFIG_TYPE_STRING, "wrong config type");
	return c->u.String.Value;
}
int ConfigGetInt(Config *c, const char *name)
{
	c = ConfigGet(c, name);
	CASSERT(c->Type == CONFIG_TYPE_INT, "wrong config type");
	return c->u.Int.Value;
}
double ConfigGetFloat(Config *c, const char *name)
{
	c = ConfigGet(c, name);
	CASSERT(c->Type == CONFIG_TYPE_FLOAT, "wrong config type");
	return c->u.Float.Value;
}
bool ConfigGetBool(Config *c, const char *name)
{
	c = ConfigGet(c, name);
	CASSERT(c->Type == CONFIG_TYPE_BOOL, "wrong config type");
	return c->u.Bool.Value;
}
int ConfigGetEnum(Config *c, const char *name)
{
	c = ConfigGet(c, name);
	CASSERT(c->Type == CONFIG_TYPE_ENUM, "wrong config type");
	return c->u.Enum.Value;
}
CArray *ConfigGetGroup(Config *c, const char *name)
{
	c = ConfigGet(c, name);
	CASSERT(c->Type == CONFIG_TYPE_GROUP, "wrong config type");
	return &c->u.Group;
}

void ConfigSetInt(Config *c, const char *name, const int value)
{
	c = ConfigGet(c, name);
	CASSERT(c->Type == CONFIG_TYPE_INT, "wrong config type");
	c->u.Int.Value = CLAMP(value, c->u.Int.Min, c->u.Int.Max);
}

void ConfigSetFloat(Config *c, const char *name, const double value)
{
	c = ConfigGet(c, name);
	CASSERT(c->Type == CONFIG_TYPE_FLOAT, "wrong config type");
	c->u.Float.Value = CLAMP(value, c->u.Float.Min, c->u.Float.Max);
}

bool ConfigTrySetFromString(Config *c, const char *name, const char *value)
{
	Config *child = ConfigGet(c, name);
	switch (child->Type)
	{
	case CONFIG_TYPE_STRING:
		CASSERT(false, "unimplemented");
		return false;
	case CONFIG_TYPE_INT:
		ConfigSetInt(c, name, atoi(value));
		return true;
	case CONFIG_TYPE_FLOAT:
		ConfigSetFloat(c, name, atof(value));
		return true;
	case CONFIG_TYPE_BOOL:
		child->u.Bool.Value = strcmp(value, "true") == 0;
		return false;
	case CONFIG_TYPE_ENUM:
		CASSERT(false, "unimplemented");
		return false;
	case CONFIG_TYPE_GROUP:
		CASSERT(false, "Cannot set group config");
		return false;
	default:
		CASSERT(false, "Unknown config type");
		return false;
	}
}


Config ConfigDefault(void)
{
	Config root = ConfigNewGroup(NULL);
	
	Config game = ConfigNewGroup("Game");
	ConfigGroupAdd(&game, ConfigNewBool("FriendlyFire", false));
	ConfigGroupAdd(&game,
		ConfigNewInt("RandomSeed", 0, 0, UINT_MAX, 1, NULL, NULL));
	ConfigGroupAdd(&game, ConfigNewEnum(
		"Difficulty", DIFFICULTY_NORMAL,
		DIFFICULTY_VERYEASY, DIFFICULTY_VERYHARD,
		StrDifficulty, DifficultyStr));
	ConfigGroupAdd(&game, ConfigNewInt("FPS", 70, 10, 120, 10, NULL, NULL));
	ConfigGroupAdd(&game,
		ConfigNewInt("EnemyDensity", 100, 25, 200, 25, NULL, PercentStr));
	ConfigGroupAdd(&game,
		ConfigNewInt("NonPlayerHP", 100, 25, 200, 25, NULL, PercentStr));
	ConfigGroupAdd(&game,
		ConfigNewInt("PlayerHP", 75, 25, 200, 25, NULL, PercentStr));
	ConfigGroupAdd(&game,
		ConfigNewInt("Lives", 2, 1, 5, 1, NULL, NULL));
	ConfigGroupAdd(&game, ConfigNewBool("HealthPickups", true));
	ConfigGroupAdd(&game, ConfigNewBool("Ammo", false));
	ConfigGroupAdd(&game, ConfigNewBool("Fog", true));
	ConfigGroupAdd(&game,
		ConfigNewInt("SightRange", 15, 8, 40, 1, NULL, NULL));
	ConfigGroupAdd(&game, ConfigNewEnum(
		"FireMoveStyle", FIREMOVE_STOP, FIREMOVE_STOP, FIREMOVE_STRAFE,
		StrFireMoveStyle, FireMoveStyleStr));
	ConfigGroupAdd(&game, ConfigNewEnum(
		"SwitchMoveStyle", SWITCHMOVE_SLIDE,
		SWITCHMOVE_SLIDE, SWITCHMOVE_NONE,
		StrSwitchMoveStyle, SwitchMoveStyleStr));
	ConfigGroupAdd(&game, ConfigNewEnum(
		"AllyCollision", ALLYCOLLISION_REPEL,
		ALLYCOLLISION_NORMAL, ALLYCOLLISION_NONE,
		StrAllyCollision, AllyCollisionStr));
	ConfigGroupAdd(&game, ConfigNewEnum(
		"LaserSight", LASER_SIGHT_NONE, LASER_SIGHT_NONE, LASER_SIGHT_ALL,
		StrLaserSight, LaserSightStr));
	ConfigGroupAdd(&root, game);

	Config dm = ConfigNewGroup("Deathmatch");
	ConfigGroupAdd(&dm, ConfigNewInt("Lives", 10, 1, 20, 1, NULL, NULL));
	ConfigGroupAdd(&root, dm);

	Config df = ConfigNewGroup("Dogfight");
	ConfigGroupAdd(&df,
		ConfigNewInt("PlayerHP", 100, 25, 200, 25, NULL, PercentStr));
	ConfigGroupAdd(&df, ConfigNewInt("FirstTo", 5, 1, 10, 1, NULL, NULL));
	ConfigGroupAdd(&root, df);

	Config gfx = ConfigNewGroup("Graphics");
	ConfigGroupAdd(&gfx, ConfigNewInt(
		"Brightness", 0, BLIT_BRIGHTNESS_MIN, BLIT_BRIGHTNESS_MAX, 1,
		NULL, NULL));
	ConfigGroupAdd(&gfx, ConfigNewBool("Fullscreen",
#ifdef __GCWZERO__
		true
#else
		false
#endif
		));
	ConfigGroupAdd(&gfx,
		ConfigNewInt("ResolutionWidth", 320, 0, 0, 0, NULL, NULL));
	ConfigGroupAdd(&gfx,
		ConfigNewInt("ResolutionHeight", 240, 0, 0, 0, NULL, NULL));
	ConfigGroupAdd(&gfx, ConfigNewInt("ScaleFactor",
#ifdef __GCWZERO__
		1
#else
		2
#endif
		, 1, 4, 1, NULL, NULL));
	ConfigGroupAdd(&gfx,
		ConfigNewInt("ShakeMultiplier", 1, 0, 10, 1, NULL, NULL));
	ConfigGroupAdd(&gfx, ConfigNewBool("ShowHUD", true));
	ConfigGroupAdd(&gfx, ConfigNewEnum(
		"ScaleMode", SCALE_MODE_NN, SCALE_MODE_NN, SCALE_MODE_BILINEAR,
		StrScaleMode, ScaleModeStr));
	ConfigGroupAdd(&gfx, ConfigNewBool("Shadows", true));
	ConfigGroupAdd(&gfx, ConfigNewEnum(
		"Gore", GORE_LOW, GORE_NONE, GORE_HIGH, StrGoreAmount, GoreAmountStr));
	ConfigGroupAdd(&gfx, ConfigNewBool("Brass", true));
	ConfigGroupAdd(&root, gfx);

	Config input = ConfigNewGroup("Input");
	Config pk0 = ConfigNewGroup("PlayerCodes0");
	ConfigGroupAdd(&pk0, ConfigNewInt("left", SDL_SCANCODE_LEFT, 0, 0, 0, NULL, NULL));
	ConfigGroupAdd(&pk0, ConfigNewInt("right", SDL_SCANCODE_RIGHT, 0, 0, 0, NULL, NULL));
	ConfigGroupAdd(&pk0, ConfigNewInt("up", SDL_SCANCODE_UP, 0, 0, 0, NULL, NULL));
	ConfigGroupAdd(&pk0, ConfigNewInt("down", SDL_SCANCODE_DOWN, 0, 0, 0, NULL, NULL));
#ifdef __GCWZERO__
	ConfigGroupAdd(&pk0, ConfigNewInt("button1", SDL_SCANCODE_LCTRL, 0, 0, 0, NULL, NULL));
	ConfigGroupAdd(&pk0, ConfigNewInt("button2", SDL_SCANCODE_LALT, 0, 0, 0, NULL, NULL));
#else
	ConfigGroupAdd(&pk0, ConfigNewInt("button1", SDL_SCANCODE_RETURN, 0, 0, 0, NULL, NULL));
	ConfigGroupAdd(&pk0, ConfigNewInt("button2", SDL_SCANCODE_RSHIFT, 0, 0, 0, NULL, NULL));
#endif
	ConfigGroupAdd(&pk0, ConfigNewInt("map", SDL_SCANCODE_TAB, 0, 0, 0, NULL, NULL));
	ConfigGroupAdd(&input, pk0);
	Config pk1 = ConfigNewGroup("PlayerCodes1");
	ConfigGroupAdd(&pk1, ConfigNewInt("left", SDL_SCANCODE_KP_4, 0, 0, 0, NULL, NULL));
	ConfigGroupAdd(&pk1, ConfigNewInt("right", SDL_SCANCODE_KP_6, 0, 0, 0, NULL, NULL));
	ConfigGroupAdd(&pk1, ConfigNewInt("up", SDL_SCANCODE_KP_8, 0, 0, 0, NULL, NULL));
	ConfigGroupAdd(&pk1, ConfigNewInt("down", SDL_SCANCODE_KP_2, 0, 0, 0, NULL, NULL));
	ConfigGroupAdd(&pk1, ConfigNewInt("button1", SDL_SCANCODE_KP_0, 0, 0, 0, NULL, NULL));
	ConfigGroupAdd(&pk1, ConfigNewInt("button2", SDL_SCANCODE_KP_ENTER, 0, 0, 0, NULL, NULL));
	ConfigGroupAdd(&pk1, ConfigNewInt("map", SDL_SCANCODE_KP_PERIOD, 0, 0, 0, NULL, NULL));
	ConfigGroupAdd(&input, pk1);
	ConfigGroupAdd(&root, input);

	Config itf = ConfigNewGroup("Interface");
	ConfigGroupAdd(&itf, ConfigNewBool("ShowFPS", false));
	ConfigGroupAdd(&itf, ConfigNewBool("ShowTime", false));
	ConfigGroupAdd(&itf, ConfigNewBool("ShowHUDMap", true));
	ConfigGroupAdd(&itf, ConfigNewEnum(
		"AIChatter", AICHATTER_SELDOM, AICHATTER_NONE, AICHATTER_ALWAYS,
		StrAIChatter, AIChatterStr));
	ConfigGroupAdd(&itf, ConfigNewEnum(
		"Splitscreen", SPLITSCREEN_NEVER,
		SPLITSCREEN_NORMAL, SPLITSCREEN_NEVER,
		StrSplitscreenStyle, SplitscreenStyleStr));
	ConfigGroupAdd(&itf, ConfigNewBool("SplitscreenAI", false));
	ConfigGroupAdd(&root, itf);

	Config snd = ConfigNewGroup("Sound");
	ConfigGroupAdd(&snd,
		ConfigNewInt("MusicVolume", 32, 0, 64, 8, NULL, Div8Str));
	ConfigGroupAdd(&snd,
		ConfigNewInt("SoundVolume", 64, 0, 64, 8, NULL, Div8Str));
	ConfigGroupAdd(&snd, ConfigNewBool("Footsteps", true));
	ConfigGroupAdd(&snd, ConfigNewBool("Hits", true));
	ConfigGroupAdd(&snd, ConfigNewBool("Reloads", true));
	ConfigGroupAdd(&root, snd);

	Config qp = ConfigNewGroup("QuickPlay");
	ConfigGroupAdd(&qp, ConfigNewEnum(
		"MapSize", QUICKPLAY_QUANTITY_ANY,
		QUICKPLAY_QUANTITY_ANY, QUICKPLAY_QUANTITY_LARGE,
		StrQuickPlayQuantity, QuickPlayQuantityStr));
	ConfigGroupAdd(&qp, ConfigNewEnum(
		"WallCount", QUICKPLAY_QUANTITY_ANY,
		QUICKPLAY_QUANTITY_ANY, QUICKPLAY_QUANTITY_LARGE,
		StrQuickPlayQuantity, QuickPlayQuantityStr));
	ConfigGroupAdd(&qp, ConfigNewEnum(
		"WallLength", QUICKPLAY_QUANTITY_ANY,
		QUICKPLAY_QUANTITY_ANY, QUICKPLAY_QUANTITY_LARGE,
		StrQuickPlayQuantity, QuickPlayQuantityStr));
	ConfigGroupAdd(&qp, ConfigNewEnum(
		"RoomCount", QUICKPLAY_QUANTITY_ANY,
		QUICKPLAY_QUANTITY_ANY, QUICKPLAY_QUANTITY_LARGE,
		StrQuickPlayQuantity, QuickPlayQuantityStr));
	ConfigGroupAdd(&qp, ConfigNewEnum(
		"SquareCount", QUICKPLAY_QUANTITY_ANY,
		QUICKPLAY_QUANTITY_ANY, QUICKPLAY_QUANTITY_LARGE,
		StrQuickPlayQuantity, QuickPlayQuantityStr));
	ConfigGroupAdd(&qp, ConfigNewEnum(
		"EnemyCount", QUICKPLAY_QUANTITY_ANY,
		QUICKPLAY_QUANTITY_ANY, QUICKPLAY_QUANTITY_LARGE,
		StrQuickPlayQuantity, QuickPlayQuantityStr));
	ConfigGroupAdd(&qp, ConfigNewEnum(
		"EnemySpeed", QUICKPLAY_QUANTITY_ANY,
		QUICKPLAY_QUANTITY_ANY, QUICKPLAY_QUANTITY_LARGE,
		StrQuickPlayQuantity, QuickPlayQuantityStr));
	ConfigGroupAdd(&qp, ConfigNewEnum(
		"EnemyHealth", QUICKPLAY_QUANTITY_ANY,
		QUICKPLAY_QUANTITY_ANY, QUICKPLAY_QUANTITY_LARGE,
		StrQuickPlayQuantity, QuickPlayQuantityStr));
	ConfigGroupAdd(&qp, ConfigNewBool("EnemiesWithExplosives", true));
	ConfigGroupAdd(&qp, ConfigNewEnum(
		"ItemCount", QUICKPLAY_QUANTITY_ANY,
		QUICKPLAY_QUANTITY_ANY, QUICKPLAY_QUANTITY_LARGE,
		StrQuickPlayQuantity, QuickPlayQuantityStr));
	ConfigGroupAdd(&root, qp);
	
	ConfigGroupAdd(&root, ConfigNewBool("StartServer", false));

	return root;
}
