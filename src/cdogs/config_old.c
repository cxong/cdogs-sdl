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
#include "config_old.h"

#include <stdio.h>

#include "grafx.h"
#include "keyboard.h"
#include "music.h"
#include "sounds.h"
#include "utils.h"


void ConfigLoadOld(Config *config, const char *filename)
{
	FILE *f = fopen(filename, "r");
	int dummy;
	int fscanfres;
	int splitscreenAlways;
	int i1, i2, i3;	// ints to read using fscanf

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
		&i1,
		&i2,
		&i3,
		&config->Graphics.Brightness,
		&dummy,
		&dummy,
		&splitscreenAlways);
	CHECK_FSCANF(7);
	config->Interface.ShowFPS = (bool)i1;
	config->Interface.ShowTime = (bool)i2;
	config->Game.FriendlyFire = (bool)i3;
	config->Interface.Splitscreen =
		splitscreenAlways ? SPLITSCREEN_ALWAYS : SPLITSCREEN_NORMAL;
	for (int i = 0; i < 2; i++)
	{
		fscanfres = fscanf(f, "%d\n%d %d %d %d %d %d\n",
			&dummy,
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
		&i1);
	CHECK_FSCANF(2);
	config->Game.SlowMotion = (bool)i1;
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
		&config->Graphics.Res.x,
		&config->Graphics.Res.y,
		&i1,
		&config->Graphics.ScaleFactor);
	CHECK_FSCANF(4);
	config->Graphics.Fullscreen = (bool)i1;

	fclose(f);
}

void ConfigSaveOld(Config *config, const char *filename)
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
		0,
		0,
		config->Interface.Splitscreen == SPLITSCREEN_ALWAYS);
	for (i = 0; i < 2; i++)
	{
		fprintf(f, "%d\n%d %d %d %d %d %d\n",
			INPUT_DEVICE_KEYBOARD,
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
		config->Graphics.Res.x,
		config->Graphics.Res.y,
		config->Graphics.Fullscreen,
		config->Graphics.ScaleFactor);

	fclose(f);
}

int ConfigIsOld(FILE *f)
{
	int x1, x2, x3, x4, x5, x6, x7;
	// See if the first line is made of numbers
	int fscanfres = fscanf(f, "%d %d %d %d %d %d %d\n",
		&x1, &x2, &x3, &x4, &x5, &x6, &x7);
	return fscanfres == 7;
}
