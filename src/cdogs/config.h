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
#pragma once

#include <stdbool.h>
#include <stdio.h>

#include "c_array.h"

#define CONFIG_FILE "options.cnf"

typedef enum
{
	DIFFICULTY_VERYEASY = 1,
	DIFFICULTY_EASY,
	DIFFICULTY_NORMAL,
	DIFFICULTY_HARD,
	DIFFICULTY_VERYHARD
} difficulty_e;

const char *DifficultyStr(int d);
int StrDifficulty(const char *str);

typedef enum
{
	FIREMOVE_STOP = 0,
	FIREMOVE_NORMAL,
	FIREMOVE_STRAFE
} FireMoveStyle;
const char *FireMoveStyleStr(int s);
int StrFireMoveStyle(const char *s);

typedef enum
{
	SWITCHMOVE_SLIDE = 0,
	SWITCHMOVE_STRAFE,
	SWITCHMOVE_NONE
} SwitchMoveStyle;
const char *SwitchMoveStyleStr(int s);
int StrSwitchMoveStyle(const char *s);

typedef enum
{
	SCALE_MODE_NN,
	SCALE_MODE_BILINEAR
} ScaleMode;
const char *ScaleModeStr(int q);
int StrScaleMode(const char *str);

typedef enum
{
	GORE_NONE,
	GORE_LOW,
	GORE_MEDIUM,
	GORE_HIGH
} GoreAmount;
const char *GoreAmountStr(int g);
int StrGoreAmount(const char *s);

typedef enum
{
	LASER_SIGHT_NONE,
	LASER_SIGHT_PLAYERS,
	LASER_SIGHT_ALL
} LaserSight;
const char *LaserSightStr(int l);
int StrLaserSight(const char *s);

typedef enum
{
	SPLITSCREEN_NORMAL,
	SPLITSCREEN_ALWAYS,
	SPLITSCREEN_NEVER
} SplitscreenStyle;

const char *SplitscreenStyleStr(int s);
int StrSplitscreenStyle(const char *str);

typedef enum
{
	AICHATTER_NONE,
	AICHATTER_SELDOM,
	AICHATTER_OFTEN,
	AICHATTER_ALWAYS
} AIChatterFrequency;
const char *AIChatterStr(int c);
int StrAIChatter(const char *str);

typedef enum
{
	QUICKPLAY_QUANTITY_ANY,
	QUICKPLAY_QUANTITY_SMALL,
	QUICKPLAY_QUANTITY_MEDIUM,
	QUICKPLAY_QUANTITY_LARGE
} QuickPlayQuantity;
const char *QuickPlayQuantityStr(int s);
int StrQuickPlayQuantity(const char *str);

typedef enum
{
	CONFIG_TYPE_STRING,
	CONFIG_TYPE_INT,
	CONFIG_TYPE_FLOAT,
	CONFIG_TYPE_BOOL,
	CONFIG_TYPE_ENUM,
	CONFIG_TYPE_GROUP
} ConfigType;

// Generic config entry
typedef struct
{
	char *Name;
	ConfigType Type;
	union
	{
#define VALUES(_name, _type) \
		struct \
		{ \
			_type Value; \
			_type Last; \
			_type Default; \
			_type Min; \
			_type Max; \
			_type Increment; \
		} _name
		VALUES(String, char *);
		VALUES(Float, double);
		VALUES(Bool, bool);
#undef VALUES
#define FORMATTED_VALUES(_name, _type, _toStrType) \
		struct \
		{ \
			_type Value; \
			_type Last; \
			_type Default; \
			_type Min; \
			_type Max; \
			_type Increment; \
			_type (*StrTo##_name)(const char *); \
			_toStrType *(*_name##ToStr)(_type); \
		} _name
		FORMATTED_VALUES(Int, int, char);
		FORMATTED_VALUES(Enum, int, const char);
#undef FORMATTED_VALUES
		CArray Group;	// of Config
	} u;
} Config;

Config ConfigNewString(const char *name, const char *defaultValue);
Config ConfigNewInt(
	const char *name, const int defaultValue,
	const int minValue, const int maxValue, const int increment,
	int (*strToInt)(const char *), char *(*intToStr)(int));
Config ConfigNewFloat(
	const char *name, const double defaultValue,
	const double minValue, const double maxValue, const double increment);
Config ConfigNewBool(const char *name, const bool defaultValue);
Config ConfigNewEnum(
	const char *name, const int defaultValue,
	const int minValue, const int maxValue,
	int (*strToEnum)(const char *), const char *(*enumToStr)(int));
Config ConfigNewGroup(const char *name);
void ConfigDestroy(Config *c);
bool ConfigChangedAndApply(Config *c);

void ConfigGroupAdd(Config *group, Config child);

extern Config gConfig;

Config ConfigDefault(void);

// Find all config entries that begin with a prefix
// Returns array of ConfigEntry *
CArray ConfigFind(const Config *c, const char *prefix);

// Get a child config
// Note: child configs can be searched using dot-separated notation
// e.g. Foo.Bar.Baz
Config *ConfigGet(Config *c, const char *name);

// Check if this config, or any of its children, have changed
bool ConfigChanged(const Config *c);
// Reset the changed value to the last value
void ConfigResetChanged(Config *c);
// Set the last value to the current value
void ConfigSetChanged(Config *c);
// Reset the config values to default
void ConfigResetDefault(Config *c);

// Get the value of a config
// The type must be correct
// Note: child configs can be searched using dot-separated notation
// e.g. Foo.Bar.Baz
const char *ConfigGetString(Config *c, const char *name);
int ConfigGetInt(Config *c, const char *name);
double ConfigGetFloat(Config *c, const char *name);
bool ConfigGetBool(Config *c, const char *name);
int ConfigGetEnum(Config *c, const char *name);
CArray *ConfigGetGroup(Config *c, const char *name);

// Set config value
// Min/max range is also checked and enforced
void ConfigSetInt(Config *c, const char *name, const int value);
void ConfigSetFloat(Config *c, const char *name, const double value);
// Try to set config value from a string; return success
bool ConfigTrySetFromString(Config *c, const char *name, const char *value);

bool ConfigApply(Config *config);
int ConfigGetVersion(FILE *f);
