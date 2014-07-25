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
#include "config.h"

#include <stdio.h>

#include "config_json.h"
#include "config_old.h"
#include "grafx.h"
#include "keyboard.h"
#include "music.h"
#include "sounds.h"
#include "utils.h"


const char *AllyCollisionStr(AllyCollision a)
{
	switch (a)
	{
	case ALLYCOLLISION_NORMAL:
		return "Normal";
	case ALLYCOLLISION_REPEL:
		return "Repel";
	case ALLYCOLLISION_NONE:
		return "None";
	default:
		return "";
	}
}
AllyCollision StrAllyCollision(const char *str)
{
	if (strcmp(str, "Normal") == 0)
	{
		return ALLYCOLLISION_NORMAL;
	}
	else if (strcmp(str, "Repel") == 0)
	{
		return ALLYCOLLISION_REPEL;
	}
	else 
		if (strcmp(str, "None") == 0)
	{
		return ALLYCOLLISION_NONE;
	}
	else
	{
		return ALLYCOLLISION_NORMAL;
	}
}

const char *DifficultyStr(difficulty_e d)
{
	switch (d)
	{
	case DIFFICULTY_VERYEASY:
		return "Easiest";
	case DIFFICULTY_EASY:
		return "Easy";
	case DIFFICULTY_NORMAL:
		return "Normal";
	case DIFFICULTY_HARD:
		return "Hard";
	case DIFFICULTY_VERYHARD:
		return "Very hard";
	default:
		return "";
	}
}
difficulty_e StrDifficulty(const char *str)
{
	if (strcmp(str, "Easiest") == 0)
	{
		return DIFFICULTY_VERYEASY;
	}
	else if (strcmp(str, "Easy") == 0)
	{
		return DIFFICULTY_EASY;
	}
	else if (strcmp(str, "Normal") == 0)
	{
		return DIFFICULTY_NORMAL;
	}
	else if (strcmp(str, "Hard") == 0)
	{
		return DIFFICULTY_HARD;
	}
	else if (strcmp(str, "Very hard") == 0)
	{
		return DIFFICULTY_VERYHARD;
	}
	else
	{
		return DIFFICULTY_NORMAL;
	}
}
const char *SwitchMoveStyleStr(SwitchMoveStyle s)
{
	switch (s)
	{
	case SWITCHMOVE_SLIDE:
		return "Slide";
	case SWITCHMOVE_STRAFE:
		return "Strafe";
	case SWITCHMOVE_NONE:
		return "None";
	default:
		return "";
	}
}
SwitchMoveStyle StrSwitchMoveStyle(const char *str)
{
	if (strcmp(str, "Slide") == 0)
	{
		return SWITCHMOVE_SLIDE;
	}
	else if (strcmp(str, "Strafe") == 0)
	{
		return SWITCHMOVE_STRAFE;
	}
	else if (strcmp(str, "None") == 0)
	{
		return SWITCHMOVE_NONE;
	}
	else
	{
		return SWITCHMOVE_SLIDE;
	}
}
const char *ScaleModeStr(ScaleMode s)
{
	switch (s)
	{
	case SCALE_MODE_NN:
		return "Nearest neighbor";
	case SCALE_MODE_BILINEAR:
		return "Bilinear";
	case SCALE_MODE_HQX:
		return "hqx";
	default:
		return "";
	}
}
ScaleMode StrScaleMode(const char *str)
{
	if (strcmp(str, "Nearest neighbor") == 0)
	{
		return SCALE_MODE_NN;
	}
	else if (strcmp(str, "Bilinear") == 0)
	{
		return SCALE_MODE_BILINEAR;
	}
	else if (strcmp(str, "hqx") == 0)
	{
		return SCALE_MODE_HQX;
	}
	else
	{
		return SCALE_MODE_NN;
	}
}
const char *SplitscreenStyleStr(SplitscreenStyle s)
{
	switch (s)
	{
		case SPLITSCREEN_NORMAL:
			return "Normal";
		case SPLITSCREEN_ALWAYS:
			return "Always";
		case SPLITSCREEN_NEVER:
			return "Never";
		default:
			return "";
	}
}
SplitscreenStyle StrSplitscreenStyle(const char *str)
{
	if (strcmp(str, "Normal") == 0)
	{
		return SPLITSCREEN_NORMAL;
	}
	else if (strcmp(str, "Always") == 0)
	{
		return SPLITSCREEN_ALWAYS;
	}
	else if (strcmp(str, "Never") == 0)
	{
		return SPLITSCREEN_NEVER;
	}
	else
	{
		return SPLITSCREEN_NORMAL;
	}
}
const char *AIChatterStr(AIChatterFrequency c)
{
	switch (c)
	{
	case AICHATTER_NONE:
		return "None";
	case AICHATTER_SELDOM:
		return "Seldom";
	case AICHATTER_OFTEN:
		return "Often";
	case AICHATTER_ALWAYS:
		return "Always";
	default:
		return "";
	}
}
AIChatterFrequency StrAIChatter(const char *str)
{
	if (strcmp(str, "None") == 0)
	{
		return AICHATTER_NONE;
	}
	else if (strcmp(str, "Seldom") == 0)
	{
		return AICHATTER_SELDOM;
	}
	else if (strcmp(str, "Often") == 0)
	{
		return AICHATTER_OFTEN;
	}
	else if (strcmp(str, "Always") == 0)
	{
		return AICHATTER_ALWAYS;
	}
	else
	{
		return AICHATTER_NONE;
	}
}
const char *QuickPlayQuantityStr(QuickPlayQuantity q)
{
	switch (q)
	{
	case QUICKPLAY_QUANTITY_ANY:
		return "Any";
	case QUICKPLAY_QUANTITY_SMALL:
		return "Small";
	case QUICKPLAY_QUANTITY_MEDIUM:
		return "Medium";
	case QUICKPLAY_QUANTITY_LARGE:
		return "Large";
	default:
		return "";
	}
}
QuickPlayQuantity StrQuickPlayQuantity(const char *str)
{
	if (strcmp(str, "Any") == 0)
	{
		return QUICKPLAY_QUANTITY_ANY;
	}
	else if (strcmp(str, "Small") == 0)
	{
		return QUICKPLAY_QUANTITY_SMALL;
	}
	else if (strcmp(str, "Medium") == 0)
	{
		return QUICKPLAY_QUANTITY_MEDIUM;
	}
	else if (strcmp(str, "Large") == 0)
	{
		return QUICKPLAY_QUANTITY_LARGE;
	}
	else
	{
		return QUICKPLAY_QUANTITY_ANY;
	}
}


Config gConfig;
Config gLastConfig;

int ConfigGetVersion(FILE *f)
{
	if (ConfigIsOld(f))
	{
		return 0;
	}
	rewind(f);
	return ConfigGetJSONVersion(f);
}

void ConfigLoad(Config *config, const char *filename)
{
	// Load default values first
	ConfigLoadDefault(config);
	int configVersion = -1;
	FILE *f = fopen(filename, "r");
	if (f == NULL)
	{
		printf("Error loading config '%s'\n", filename);
		return;
	}
	configVersion = ConfigGetVersion(f);
	fclose(f);
	switch (configVersion)
	{
	case 0:
		ConfigLoadOld(config, filename);
		break;
	case 1:
	case 2:
	case 3:
	case 4:
		ConfigLoadJSON(config, filename);
		break;
	default:
		printf("Unknown config version\n");
		break;
	}
}

void ConfigSave(Config *config, const char *filename)
{
	ConfigSaveJSON(config, filename);
}

void ConfigLoadDefault(Config *config)
{
	int i;
	memset(config, 0, sizeof(Config));
	config->Game.Difficulty = DIFFICULTY_NORMAL;
	config->Game.EnemyDensity = 100;
	config->Game.FriendlyFire = 0;
	config->Game.NonPlayerHP = 100;
	config->Game.PlayerHP = 100;
	config->Game.RandomSeed = 0;
	config->Game.SlowMotion = 0;
	config->Game.Fog = 1;
	config->Game.SightRange = 15;
	config->Game.Shadows = 1;
	config->Game.MoveWhenShooting = 0;
	config->Game.SwitchMoveStyle = SWITCHMOVE_SLIDE;
	config->Game.ShotsPushback = 1;
	config->Game.AllyCollision = ALLYCOLLISION_REPEL;
	config->Game.HealthPickups = true;
	config->Graphics.Brightness = 0;
	config->Graphics.Fullscreen = 0;
	config->Graphics.Res.y = 240;
	config->Graphics.Res.x = 320;
	config->Graphics.ScaleFactor = 2;
	config->Graphics.ShakeMultiplier = 1;
	config->Graphics.ScaleMode = SCALE_MODE_NN;
	config->Graphics.IsEditor = 0;
	config->Input.PlayerKeys[0].Keys.left = SDLK_LEFT;
	config->Input.PlayerKeys[0].Keys.right = SDLK_RIGHT;
	config->Input.PlayerKeys[0].Keys.up = SDLK_UP;
	config->Input.PlayerKeys[0].Keys.down = SDLK_DOWN;
	config->Input.PlayerKeys[0].Keys.button1 = SDLK_RETURN;
	config->Input.PlayerKeys[0].Keys.button2 = SDLK_RSHIFT;
	config->Input.PlayerKeys[1].Keys.left = SDLK_KP4;
	config->Input.PlayerKeys[1].Keys.right = SDLK_KP6;
	config->Input.PlayerKeys[1].Keys.up = SDLK_KP8;
	config->Input.PlayerKeys[1].Keys.down = SDLK_KP2;
	config->Input.PlayerKeys[1].Keys.button1 = SDLK_KP0;
	config->Input.PlayerKeys[1].Keys.button2 = SDLK_KP_ENTER;
	for (i = 0; i < MAX_KEYBOARD_CONFIGS; i++)
	{
		config->Input.PlayerKeys[i].Keys.map = SDLK_TAB;
	}
	config->Interface.ShowFPS = 0;
	config->Interface.ShowTime = 0;
	config->Interface.Splitscreen = SPLITSCREEN_NEVER;
	config->Interface.ShowHUDMap = 1;
	config->Interface.AIChatter = AICHATTER_SELDOM;
	config->Sound.MusicVolume = 64;
	config->Sound.SoundVolume = 64;
	config->Sound.Footsteps = 1;
	config->Sound.Hits = 1;
	config->Sound.Reloads = 1;
	config->QuickPlay.MapSize = QUICKPLAY_QUANTITY_ANY;
	config->QuickPlay.WallCount = QUICKPLAY_QUANTITY_ANY;
	config->QuickPlay.WallLength = QUICKPLAY_QUANTITY_ANY;
	config->QuickPlay.RoomCount = QUICKPLAY_QUANTITY_ANY;
	config->QuickPlay.SquareCount = QUICKPLAY_QUANTITY_ANY;
	config->QuickPlay.EnemyCount = QUICKPLAY_QUANTITY_ANY;
	config->QuickPlay.EnemySpeed = QUICKPLAY_QUANTITY_ANY;
	config->QuickPlay.EnemyHealth = QUICKPLAY_QUANTITY_ANY;
	config->QuickPlay.EnemiesWithExplosives = 1;
	config->QuickPlay.ItemCount = QUICKPLAY_QUANTITY_ANY;
}
