/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (c) 2014, Cong Xu
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
#ifndef __MAP_NEW
#define __MAP_NEW

#include "c_array.h"
#include "campaigns.h"

typedef enum
{
	MAPTYPE_CLASSIC
} MapType;
const char *MapTypeStr(MapType t);
MapType StrMapType(const char *s);

typedef struct
{
	char *Description;
	ObjectiveType Type;
	int Index;
	int Count;
	int Required;
	int Flags;
} MissionObjective;

typedef struct
{
	char *Title;
	char *Description;
	MapType Type;
	Vec2i Size;

	// styles
	int WallStyle;
	int FloorStyle;
	int RoomStyle;
	int ExitStyle;
	int KeyStyle;
	int DoorStyle;

	CArray Objectives;		// of MissionObjective
	CArray Enemies;			// of int (character index)
	CArray SpecialChars;	// of int
	CArray Items;			// of int
	CArray ItemDensities;	// of int

	int EnemyDensity;
	int Weapons[GUN_COUNT];

	char Song[CDOGS_PATH_MAX];

	// Colour ranges
	int WallColor;
	int FloorColor;
	int RoomColor;
	int AltColor;

	union
	{
		// Classic
		struct
		{
			int Walls;
			int WallLength;
			int Rooms;
			int Squares;
		} Classic;
	} u;
} Mission;

void MissionInit(Mission *m);
void MissionCopy(Mission *dst, Mission *src);
void MissionTerminate(Mission *m);

int GetNumWeapons(int weapons[GUN_COUNT]);
gun_e GetNthAvailableWeapon(int weapons[GUN_COUNT], int index);

int MapNewLoad(const char *filename, CampaignSetting *c);
int MapNewSave(const char *filename, CampaignSetting *c);

#endif
