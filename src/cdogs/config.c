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

#include <cdogs/grafx.h>
#include <cdogs/keyboard.h>
#include <cdogs/music.h>
#include <cdogs/sounds.h>
#include <cdogs/utils.h>


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

Config gConfig;

void ConfigLoad(Config *config, const char *filename)
{
	FILE *f = fopen(filename, "r");
	int dummy;
	int fscanfres;
	int i;

	ConfigLoadDefault(config);
	if (f == NULL)
	{
		printf("Error loading config '%s'\n", filename);
		return;
	}

#define CHECK_FSCANF(count)\
	if (fscanfres < count)\
	{\
		printf("Error loading config\n");\
		fclose(f);\
		return;\
	}
	fscanfres = fscanf(f, "%d %d %d %d %d %d %d\n",
		&config->Interface.ShowFPS,
		&config->Interface.ShowTime,
		&config->Game.FriendlyFire,
		&config->Graphics.Brightness,
		&config->Input.SwapButtonsJoystick1,
		&config->Input.SwapButtonsJoystick2,
		&config->Interface.SplitscreenAlways);
	CHECK_FSCANF(7);
	for (i = 0; i < 2; i++)
	{
		fscanfres = fscanf(f, "%d\n%d %d %d %d %d %d\n",
			(int *)&config->Input.PlayerKeys[i].Device,
			&config->Input.PlayerKeys[i].Keys.left,
			&config->Input.PlayerKeys[i].Keys.right,
			&config->Input.PlayerKeys[i].Keys.up,
			&config->Input.PlayerKeys[i].Keys.down,
			&config->Input.PlayerKeys[i].Keys.button1,
			&config->Input.PlayerKeys[i].Keys.button2);
		CHECK_FSCANF(7);
	}
	fscanfres = fscanf(f, "%d\n", &config->Input.PlayerKeys[0].Keys.map);
	CHECK_FSCANF(1);
	fscanfres = fscanf(f, "%d %d %d %d\n",
		&config->Sound.SoundVolume,
		&config->Sound.MusicVolume,
		&config->Sound.SoundChannels,
		&dummy);
	CHECK_FSCANF(4);
	fscanfres = fscanf(f, "%u\n", &config->Game.RandomSeed);
	CHECK_FSCANF(1);
	fscanfres = fscanf(f, "%d %d\n",
		(int *)&config->Game.Difficulty,
		&config->Game.SlowMotion);
	CHECK_FSCANF(2);
	fscanfres = fscanf(f, "%d\n", &config->Game.EnemyDensity);
	CHECK_FSCANF(1);
	if (config->Game.EnemyDensity < 25 || config->Game.EnemyDensity > 200)
	{
		config->Game.EnemyDensity = 100;
	}
	fscanfres = fscanf(f, "%d\n", &config->Game.NonPlayerHP);
	CHECK_FSCANF(1);
	if (config->Game.NonPlayerHP < 25 || config->Game.NonPlayerHP > 200)
	{
		config->Game.NonPlayerHP = 100;
	}
	fscanfres = fscanf(f, "%d\n", &config->Game.PlayerHP);
	CHECK_FSCANF(1);
	if (config->Game.PlayerHP < 25 || config->Game.PlayerHP > 200)
	{
		config->Game.PlayerHP = 100;
	}
	fscanfres = fscanf(f, "%dx%d:%d:%d\n",
		&config->Graphics.ResolutionWidth,
		&config->Graphics.ResolutionHeight,
		&config->Graphics.Fullscreen,
		&config->Graphics.ScaleFactor);
	CHECK_FSCANF(4);

	fclose(f);
}

void ConfigSave(Config *config, const char *filename)
{
	FILE *f = fopen(filename, "w");
	int i;

	if (f == NULL)
	{
		printf("Error saving config '%s'\n", filename);
		return;
	}

	fprintf(f, "%d %d %d %d %d %d %d\n",
		config->Interface.ShowFPS,
		config->Interface.ShowTime,
		config->Game.FriendlyFire,
		config->Graphics.Brightness,
		config->Input.SwapButtonsJoystick1,
		config->Input.SwapButtonsJoystick2,
		config->Interface.SplitscreenAlways);
	for (i = 0; i < 2; i++)
	{
		fprintf(f, "%d\n%d %d %d %d %d %d\n",
			config->Input.PlayerKeys[i].Device,
			config->Input.PlayerKeys[i].Keys.left,
			config->Input.PlayerKeys[i].Keys.right,
			config->Input.PlayerKeys[i].Keys.up,
			config->Input.PlayerKeys[i].Keys.down,
			config->Input.PlayerKeys[i].Keys.button1,
			config->Input.PlayerKeys[i].Keys.button2);
	}
	fprintf(f, "%d\n", config->Input.PlayerKeys[0].Keys.map);
	fprintf(f, "%d %d %d %d\n",
		config->Sound.SoundVolume,
		config->Sound.MusicVolume,
		config->Sound.SoundChannels,
		0);
	fprintf(f, "%u\n", config->Game.RandomSeed);
	fprintf(f, "%d %d\n",
		config->Game.Difficulty,
		config->Game.SlowMotion);
	fprintf(f, "%d\n", config->Game.EnemyDensity);
	fprintf(f, "%d\n", config->Game.NonPlayerHP);
	fprintf(f, "%d\n", config->Game.PlayerHP);
	fprintf(f, "%dx%d:%d:%d\n",
		config->Graphics.ResolutionWidth,
		config->Graphics.ResolutionHeight,
		config->Graphics.Fullscreen,
		config->Graphics.ScaleFactor);

	fclose(f);
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
}
