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
#ifndef __CONFIG
#define __CONFIG

#include "grafx.h"
#include "input.h"
#include "sounds.h"

#define CONFIG_FILE "options.cnf"

typedef enum
{
	DIFFICULTY_VERYEASY = 1,
	DIFFICULTY_EASY,
	DIFFICULTY_NORMAL,
	DIFFICULTY_HARD,
	DIFFICULTY_VERYHARD
} difficulty_e;

const char *DifficultyStr(difficulty_e d);
difficulty_e StrDifficulty(const char *str);

typedef struct
{
	input_device_e Device;
	input_keys_t Keys;
} KeyConfig;

typedef struct
{
	int SwapButtonsJoystick1;
	int SwapButtonsJoystick2;
	KeyConfig PlayerKeys[2];
} InputConfig;

typedef struct
{
	int FriendlyFire;
	unsigned int RandomSeed;
	difficulty_e Difficulty;
	int SlowMotion;
	int EnemyDensity;
	int NonPlayerHP;
	int PlayerHP;
} GameConfig;

typedef struct
{
	int ShowFPS;
	int ShowTime;
	int SplitscreenAlways;
} InterfaceConfig;

typedef struct
{
	GameConfig Game;
	GraphicsConfig Graphics;
	InputConfig Input;
	InterfaceConfig Interface;
	SoundConfig Sound;
} Config;

extern Config gConfig;

void ConfigLoad(Config *config, const char *filename);
void ConfigSave(Config *config, const char *filename);
void ConfigApply(Config *config);
void ConfigLoadDefault(Config *config);
int ConfigGetVersion(FILE *f);

#endif
