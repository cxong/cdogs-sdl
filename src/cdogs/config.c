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
#include "config.h"

#include <stdio.h>

#include "config_json.h"
#include "config_old.h"
#include "grafx.h"
#include "keyboard.h"
#include "music.h"
#include "sounds.h"
#include "utils.h"


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

Config gConfig;

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
		ConfigLoadJSON(config, filename);
		break;
	default:
		printf("Unknown config version\n");
		// try loading anyway
		ConfigLoadJSON(config, filename);
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
	config->Graphics.Brightness = 0;
	config->Graphics.Fullscreen = 0;
	config->Graphics.ResolutionHeight = 240;
	config->Graphics.ResolutionWidth = 320;
	config->Graphics.ScaleFactor = 1;
	config->Input.PlayerKeys[0].Device = INPUT_DEVICE_KEYBOARD;
	config->Input.PlayerKeys[0].Keys.left = SDLK_LEFT;
	config->Input.PlayerKeys[0].Keys.right = SDLK_RIGHT;
	config->Input.PlayerKeys[0].Keys.up = SDLK_UP;
	config->Input.PlayerKeys[0].Keys.down = SDLK_DOWN;
	config->Input.PlayerKeys[0].Keys.button1 = SDLK_RSHIFT;
	config->Input.PlayerKeys[0].Keys.button2 = SDLK_RETURN;
	config->Input.PlayerKeys[1].Device = INPUT_DEVICE_KEYBOARD;
	config->Input.PlayerKeys[1].Keys.left = keyKeypad4;
	config->Input.PlayerKeys[1].Keys.right = keyKeypad6;
	config->Input.PlayerKeys[1].Keys.up = keyKeypad8;
	config->Input.PlayerKeys[1].Keys.down = keyKeypad2;
	config->Input.PlayerKeys[1].Keys.button1 = keyKeypad0;
	config->Input.PlayerKeys[1].Keys.button2 = keyKeypadEnter;
	for (i = 0; i < 2; i++)
	{
		config->Input.PlayerKeys[i].Keys.map = keyTab;
	}
	config->Input.SwapButtonsJoystick1 = 0;
	config->Input.SwapButtonsJoystick2 = 0;
	config->Interface.ShowFPS = 0;
	config->Interface.ShowTime = 0;
	config->Interface.SplitscreenAlways = 0;
	config->Sound.MusicVolume = 64;
	config->Sound.SoundChannels = 8;
	config->Sound.SoundVolume = 64;
	config->Sound.Footsteps = 0;
	config->Sound.Hits = 1;
	config->Sound.Reloads = 1;
}
