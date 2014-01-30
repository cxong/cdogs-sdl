/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (C) 1995 Ronny Wester
    Copyright (C) 2003 Jeremy Chin
    Copyright (C) 2003-2007 Lucas Martin-King

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    This file incorporates work covered by the following copyright and
    permission notice:

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
#include "mission.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "files.h"
#include "game_events.h"
#include "gamedata.h"
#include "map.h"
#include "map_new.h"
#include "palette.h"
#include "defs.h"
#include "pic_manager.h"
#include "actors.h"


const char *MapTypeStr(MapType t)
{
	switch (t)
	{
	case MAPTYPE_CLASSIC:
		return "Classic";
	case MAPTYPE_STATIC:
		return "Static";
	default:
		return "";
	}
}
MapType StrMapType(const char *s)
{
	if (strcmp(s, "Classic") == 0)
	{
		return MAPTYPE_CLASSIC;
	}
	else if (strcmp(s, "Static") == 0)
	{
		return MAPTYPE_STATIC;
	}
	return MAPTYPE_CLASSIC;
}


void MissionInit(Mission *m)
{
	memset(m, 0, sizeof *m);
	CArrayInit(&m->Objectives, sizeof(MissionObjective));
	CArrayInit(&m->Enemies, sizeof(int));
	CArrayInit(&m->SpecialChars, sizeof(int));
	CArrayInit(&m->Items, sizeof(int));
	CArrayInit(&m->ItemDensities, sizeof(int));
}
void MissionCopy(Mission *dst, Mission *src)
{
	MissionTerminate(dst);
	MissionInit(dst);
	if (src->Title)
	{
		CSTRDUP(dst->Title, src->Title);
	}
	if (src->Description)
	{
		CSTRDUP(dst->Description, src->Description);
	}
	dst->Type = src->Type;
	dst->Size = src->Size;

	dst->WallStyle = src->WallStyle;
	dst->FloorStyle = src->FloorStyle;
	dst->RoomStyle = src->RoomStyle;
	dst->ExitStyle = src->ExitStyle;
	dst->KeyStyle = src->KeyStyle;
	dst->DoorStyle = src->DoorStyle;

	CArrayCopy(&dst->Objectives, &src->Objectives);
	CArrayCopy(&dst->Enemies, &src->Enemies);
	CArrayCopy(&dst->SpecialChars, &src->SpecialChars);
	CArrayCopy(&dst->Items, &src->Items);
	CArrayCopy(&dst->ItemDensities, &src->ItemDensities);

	dst->EnemyDensity = src->EnemyDensity;
	memcpy(dst->Weapons, src->Weapons, sizeof dst->Weapons);

	memcpy(dst->Song, src->Song, sizeof dst->Song);

	dst->WallColor = src->WallColor;
	dst->FloorColor = src->FloorColor;
	dst->RoomColor = src->RoomColor;
	dst->AltColor = src->AltColor;

	switch (dst->Type)
	{
	case MAPTYPE_STATIC:
		CArrayInit(&dst->u.Static.Tiles, src->u.Static.Tiles.elemSize);
		CArrayCopy(&dst->u.Static.Tiles, &src->u.Static.Tiles);
		CArrayInit(&dst->u.Static.Items, src->u.Static.Items.elemSize);
		CArrayCopy(&dst->u.Static.Items, &src->u.Static.Items);

		dst->u.Static.Start = src->u.Static.Start;
		break;
	default:
		memcpy(&dst->u, &src->u, sizeof dst->u);
		break;
	}
}
void MissionTerminate(Mission *m)
{
	CFREE(m->Title);
	CFREE(m->Description);
	CArrayTerminate(&m->Objectives);
	CArrayTerminate(&m->Enemies);
	CArrayTerminate(&m->SpecialChars);
	CArrayTerminate(&m->Items);
	CArrayTerminate(&m->ItemDensities);
	switch (m->Type)
	{
	case MAPTYPE_CLASSIC:
		break;
	case MAPTYPE_STATIC:
		CArrayTerminate(&m->u.Static.Tiles);
		CArrayTerminate(&m->u.Static.Items);
		break;
	}
}


#define EXIT_WIDTH  8
#define EXIT_HEIGHT 8


// +--------------------+
// |  Color range info  |
// +--------------------+


#define WALL_COLORS       208
#define FLOOR_COLORS      216
#define ROOM_COLORS       232
#define ALT_COLORS        224

struct ColorRange {
	char name[20];
	color_t range[8];
};


static struct ColorRange cColorRanges[] =
{
	{"Red 1",		{{33,  0,  0, 255}, {29,  0,  0, 255}, {25,  0,  0, 255}, {21,  0,  0, 255}, {17,  0,  0, 255}, {13,  0,  0, 255}, { 9,  0,  0, 255}, { 5, 0,  0, 255}}},
	{"Red 2",		{{28,  0,  0, 255}, {23,  0,  0, 255}, {18,  0,  0, 255}, {14,  0,  0, 255}, {12,  0,  0, 255}, {10,  0,  0, 255}, { 8,  0,  0, 255}, { 5, 0,  0, 255}}},
	{"Red 3",		{{18,  0,  0, 255}, {15,  0,  0, 255}, {13,  0,  0, 255}, {10,  0,  0, 255}, { 8,  0,  0, 255}, { 5,  0,  0, 255}, { 3,  0,  0, 255}, { 0, 0,  0, 255}}},
	{"Green 1",		{{ 0, 33,  0, 255}, { 0, 29,  0, 255}, { 0, 25,  0, 255}, { 0, 21,  0, 255}, { 0, 17,  0, 255}, { 0, 13,  0, 255}, { 0,  9,  0, 255}, { 0, 5,  0, 255}}},
	{"Green 2",		{{ 0, 28,  0, 255}, { 0, 23,  0, 255}, { 0, 18,  0, 255}, { 0, 14,  0, 255}, { 0, 12,  0, 255}, { 0, 10,  0, 255}, { 0,  8,  0, 255}, { 0, 5,  0, 255}}},
	{"Green 3",		{{ 0, 18,  0, 255}, { 0, 15,  0, 255}, { 0, 13,  0, 255}, { 0, 10,  0, 255}, { 0,  8,  0, 255}, { 0,  5,  0, 255}, { 0,  3,  0, 255}, { 0, 0,  0, 255}}},
	{"Blue 1",		{{ 0,  0, 33, 255}, { 0,  0, 29, 255}, { 0,  0, 25, 255}, { 0,  0, 21, 255}, { 0,  0, 17, 255}, { 0,  0, 13, 255}, { 0,  0,  9, 255}, { 0, 0,  5, 255}}},
	{"Blue 2",		{{ 0,  0, 28, 255}, { 0,  0, 23, 255}, { 0,  0, 18, 255}, { 0,  0, 14, 255}, { 0,  0, 12, 255}, { 0,  0, 10, 255}, { 0,  0,  8, 255}, { 0, 0,  5, 255}}},
	{"Blue 3",		{{ 0,  0, 18, 255}, { 0,  0, 15, 255}, { 0,  0, 13, 255}, { 0,  0, 10, 255}, { 0,  0,  8, 255}, { 0,  0,  5, 255}, { 0,  0,  3, 255}, { 0, 0,  0, 255}}},
	{"Purple 1",	{{33,  0, 33, 255}, {29,  0, 29, 255}, {25,  0, 25, 255}, {21,  0, 21, 255}, {17,  0, 17, 255}, {13,  0, 13, 255}, { 9,  0,  9, 255}, { 5, 0,  5, 255}}},
	{"Purple 2",	{{28,  0, 28, 255}, {23,  0, 23, 255}, {18,  0, 18, 255}, {14,  0, 14, 255}, {12,  0, 12, 255}, {10,  0, 10, 255}, { 8,  0,  8, 255}, { 5, 0,  5, 255}}},
	{"Purple 3",	{{18,  0, 18, 255}, {15,  0, 15, 255}, {13,  0, 13, 255}, {10,  0, 10, 255}, { 8,  0,  8, 255}, { 5,  0,  5, 255}, { 3,  0,  3, 255}, { 0, 0,  0, 255}}},
	{"Gray 1",		{{33, 33, 33, 255}, {28, 28, 28, 255}, {23, 23, 23, 255}, {18, 18, 18, 255}, {15, 15, 15, 255}, {12, 12, 12, 255}, {10, 10, 10, 255}, { 8, 8,  8, 255}}},
	{"Gray 2",		{{28, 28, 28, 255}, {23, 23, 23, 255}, {18, 18, 18, 255}, {14, 14, 14, 255}, {12, 12, 12, 255}, {10, 10, 10, 255}, { 8,  8,  8, 255}, { 5, 5,  5, 255}}},
	{"Gray 3",		{{18, 18, 18, 255}, {15, 15, 15, 255}, {13, 13, 13, 255}, {10, 10, 10, 255}, { 8,  8,  8, 255}, { 5,  5,  5, 255}, { 3,  3,  3, 255}, { 0, 0,  0, 255}}},
	{"Gray/blue 1",	{{23, 23, 33, 255}, {20, 20, 29, 255}, {18, 18, 26, 255}, {15, 15, 23, 255}, {12, 12, 20, 255}, {10, 10, 17, 255}, { 8,  8, 14, 255}, { 5, 5, 10, 255}}},
	{"Gray/blue 2",	{{18, 18, 28, 255}, {16, 16, 25, 255}, {14, 14, 23, 255}, {12, 12, 20, 255}, {10, 10, 17, 255}, { 8,  8, 15, 255}, { 6,  6, 12, 255}, { 4, 4,  9, 255}}},
	{"Gray/blue 3",	{{13, 13, 23, 255}, {12, 12, 21, 255}, {10, 10, 19, 255}, { 9,  9, 17, 255}, { 7,  7, 14, 255}, { 6,  6, 12, 255}, { 4,  4, 10, 255}, { 3, 3,  8, 255}}},
	{"Yellowish 1",	{{37, 32, 11, 255}, {33, 28, 10, 255}, {29, 24,  9, 255}, {25, 21,  8, 255}, {21, 17,  7, 255}, {17, 13,  6, 255}, {14,  9,  4, 255}, {11, 6,  2, 255}}},
	{"Yellowish 2",	{{33, 28,  9, 255}, {29, 24,  8, 255}, {25, 21,  7, 255}, {21, 17,  6, 255}, {17, 13,  5, 255}, {14,  9,  4, 255}, {11,  6,  2, 255}, {11, 4,  1, 255}}},
	{"Yellowish 3",	{{29, 24,  7, 255}, {25, 21,  6, 255}, {21, 17,  5, 255}, {17, 13,  4, 255}, {14,  9,  3, 255}, {11,  6,  2, 255}, { 9,  4,  1, 255}, {11, 2,  0, 255}}},
	{"Brown 1",		{{33, 17,  0, 255}, {29, 15,  0, 255}, {25, 13,  0, 255}, {21, 11,  0, 255}, {17,  9,  0, 255}, {13,  7,  0, 255}, { 9,  5,  0, 255}, { 5, 3,  0, 255}}},
	{"Brown 2",		{{28, 14,  0, 255}, {23, 12,  0, 255}, {18,  9,  0, 255}, {14,  7,  0, 255}, {12,  6,  0, 255}, {10,  5,  0, 255}, { 8,  4,  0, 255}, { 5, 3,  0, 255}}},
	{"Brown 3",		{{18,  9,  0, 255}, {15,  8,  0, 255}, {13,  7,  0, 255}, {10,  5,  0, 255}, { 8,  4,  0, 255}, { 5,  3,  0, 255}, { 3,  2,  0, 255}, { 0, 0,  0, 255}}},
	{"Cyan 1",		{{ 0, 33, 33, 255}, { 0, 29, 29, 255}, { 0, 25, 25, 255}, { 0, 21, 21, 255}, { 0, 17, 17, 255}, { 0, 13, 13, 255}, { 0,  9,  9, 255}, { 0, 5,  5, 255}}},
	{"Cyan 2",		{{ 0, 28, 28, 255}, { 0, 23, 23, 255}, { 0, 18, 18, 255}, { 0, 14, 14, 255}, { 0, 12, 12, 255}, { 0, 10, 10, 255}, { 0,  8,  8, 255}, { 0, 5,  5, 255}}},
	{"Cyan 3",		{{ 0, 18, 18, 255}, { 0, 15, 15, 255}, { 0, 13, 13, 255}, { 0, 10, 10, 255}, { 0,  8,  8, 255}, { 0,  5,  5, 255}, { 0,  3,  3, 255}, { 0, 0,  0, 255}}}
};
#define COLORRANGE_COUNT (sizeof cColorRanges / sizeof(struct ColorRange))
int GetColorrangeCount(void) { return COLORRANGE_COUNT; }


// +----------------+
// |  Pickups info  |
// +----------------+


static int pickupItems[] = {
	OFSPIC_FOLDER,
	OFSPIC_DISK1,
	OFSPIC_DISK2,
	OFSPIC_DISK3,
	OFSPIC_BLUEPRINT,
	OFSPIC_CD,
	OFSPIC_BAG,
	OFSPIC_HOLO,
	OFSPIC_BOTTLE,
	OFSPIC_RADIO,
	OFSPIC_CIRCUIT,
	OFSPIC_PAPER
};
#define PICKUPS_COUNT (sizeof( pickupItems)/sizeof( int))


// +-------------+
// |  Keys info  |
// +-------------+


static int officeKeys[4] =
    { OFSPIC_KEYCARD_YELLOW, OFSPIC_KEYCARD_GREEN, OFSPIC_KEYCARD_BLUE,
	OFSPIC_KEYCARD_RED
};

static int dungeonKeys[4] =
    { OFSPIC_KEY_YELLOW, OFSPIC_KEY_GREEN, OFSPIC_KEY_BLUE,
	OFSPIC_KEY_RED
};

static int plainKeys[4] =
    { OFSPIC_KEY3_YELLOW, OFSPIC_KEY3_GREEN, OFSPIC_KEY3_BLUE,
	OFSPIC_KEY3_RED
};

static int cubeKeys[4] =
    { OFSPIC_KEY4_YELLOW, OFSPIC_KEY4_GREEN, OFSPIC_KEY4_BLUE,
	OFSPIC_KEY4_RED
};


static int *keyStyles[] = {
	officeKeys,
	dungeonKeys,
	plainKeys,
	cubeKeys
};
#define KEYSTYLE_COUNT (sizeof keyStyles / sizeof(int *))
int GetKeystyleCount(void) { return KEYSTYLE_COUNT; }


// +-------------+
// |  Door info  |
// +-------------+

// note that the horz pic in the last pair is a TILE pic, not an offset pic!

static struct DoorPic officeDoors[6] = { {OFSPIC_DOOR, OFSPIC_VDOOR},
{OFSPIC_HDOOR_YELLOW, OFSPIC_VDOOR_YELLOW},
{OFSPIC_HDOOR_GREEN, OFSPIC_VDOOR_GREEN},
{OFSPIC_HDOOR_BLUE, OFSPIC_VDOOR_BLUE},
{OFSPIC_HDOOR_RED, OFSPIC_VDOOR_RED},
{109, OFSPIC_VDOOR_OPEN}
};

static struct DoorPic dungeonDoors[6] = { {OFSPIC_DOOR2, OFSPIC_VDOOR2},
{OFSPIC_HDOOR2_YELLOW, OFSPIC_VDOOR2_YELLOW},
{OFSPIC_HDOOR2_GREEN, OFSPIC_VDOOR2_GREEN},
{OFSPIC_HDOOR2_BLUE, OFSPIC_VDOOR2_BLUE},
{OFSPIC_HDOOR2_RED, OFSPIC_VDOOR2_RED},
{342, OFSPIC_VDOOR2_OPEN}
};

static struct DoorPic pansarDoors[6] = { {OFSPIC_HDOOR3, OFSPIC_VDOOR3},
{OFSPIC_HDOOR3_YELLOW, OFSPIC_VDOOR3_YELLOW},
{OFSPIC_HDOOR3_GREEN, OFSPIC_VDOOR3_GREEN},
{OFSPIC_HDOOR3_BLUE, OFSPIC_VDOOR3_BLUE},
{OFSPIC_HDOOR3_RED, OFSPIC_VDOOR3_RED},
{P2 + 148, OFSPIC_VDOOR2_OPEN}
};

static struct DoorPic alienDoors[6] = { {OFSPIC_HDOOR4, OFSPIC_VDOOR4},
{OFSPIC_HDOOR4_YELLOW, OFSPIC_VDOOR4_YELLOW},
{OFSPIC_HDOOR4_GREEN, OFSPIC_VDOOR4_GREEN},
{OFSPIC_HDOOR4_BLUE, OFSPIC_VDOOR4_BLUE},
{OFSPIC_HDOOR4_RED, OFSPIC_VDOOR4_RED},
{P2 + 163, OFSPIC_VDOOR2_OPEN}
};


static struct DoorPic *doorStyles[] = {
	officeDoors,
	dungeonDoors,
	pansarDoors,
	alienDoors
};

#define DOORSTYLE_COUNT (sizeof doorStyles / sizeof(struct DoorPic *))
int GetDoorstyleCount(void) { return DOORSTYLE_COUNT; }


// +-------------+
// |  Exit info  |
// +-------------+


static int exitPics[] = {
	375, 376,	// hazard stripes
	380, 381	// yellow plates
};

// Every exit has TWO pics, so actual # of exits == # pics / 2!
#define EXIT_COUNT (sizeof( exitPics)/sizeof( int)/2)
int GetExitCount(void) { return EXIT_COUNT; }


// +----------------+
// |  Mission info  |
// +----------------+


struct MissionOld dogFight1 = {
	"",
	"",
	WALL_STYLE_STONE, FLOOR_STYLE_STONE, FLOOR_STYLE_WOOD, 0, 1, 1,
	32, 32,
	50, 25,
	4, 2,
	0, 0, 0, 0,
	0,
	{
	 {"", 0, 0, 0, 0, 0}
	 },

	0, {0},
	0, {0},
	8,
	{8, 9, 10, 11, 12, 13, 14, 15},
	{10, 10, 10, 10, 10, 10, 10, 10},

	0, 0,
	"", "",
	14, 13, 22, 1
};

struct MissionOld dogFight2 = {
	"",
	"",
	WALL_STYLE_STEEL, FLOOR_STYLE_BLUE, FLOOR_STYLE_WHITE, 0, 0, 0,
	64, 64,
	50, 50,
	10, 3,
	0, 0, 0, 0,
	0,
	{
	 {"", 0, 0, 0, 0, 0}
	 },

	0, {0},
	0, {0},
	8,
	{0, 1, 2, 3, 4, 5, 6, 7},
	{10, 10, 10, 10, 10, 10, 10, 10},

	0, 0,
	"", "",
	5, 2, 9, 4
};


// +-----------------+
// |  Campaign info  |
// +-----------------+

#include "files.h"
#include <missions/bem.h>
#include <missions/ogre.h>


static CampaignSettingOld df1 =
{
	"Dogfight in the dungeon",
	"", "",
	1, &dogFight1,
	0, NULL
};

static CampaignSettingOld df2 =
{
	"Cubicle wars",
	"", "",
	1, &dogFight2,
	0, NULL
};


// +---------------------------------------------------+
// |  Objective colors (for automap & status display)  |
// +---------------------------------------------------+


// TODO: no limit to objective colours
color_t objectiveColors[OBJECTIVE_MAX_OLD] =
{
	{ 0, 252, 252, 255 },
	{ 252, 224, 0, 255 },
	{ 252, 0, 0, 255 },
	{ 192, 0, 192, 255 },
	{ 112, 112, 112, 255 }
};


// +-----------------------+
// |  And now the code...  |
// +-----------------------+

static void SetupBadguysForMission(Mission *mission)
{
	int i;
	CharacterStore *s = &gCampaign.Setting.characters;

	CharacterStoreResetOthers(s);

	if (s->OtherChars.size == 0)
	{
		return;
	}

	for (i = 0; i < (int)mission->Objectives.size; i++)
	{
		MissionObjective *mobj = CArrayGet(&mission->Objectives, i);
		if (mobj->Type == OBJECTIVE_RESCUE)
		{
			CharacterStoreAddPrisoner(s, mobj->Index);
			break;	// TODO: multiple prisoners
		}
	}

	for (i = 0; i < (int)mission->Enemies.size; i++)
	{
		CharacterStoreAddBaddie(s, *(int *)CArrayGet(&mission->Enemies, i));
	}

	for (i = 0; i < (int)mission->SpecialChars.size; i++)
	{
		CharacterStoreAddSpecial(
			s, *(int *)CArrayGet(&mission->SpecialChars, i));
	}
}

int SetupBuiltinCampaign(int idx)
{
	switch (idx)
	{
	case 0:
		ConvertCampaignSetting(&gCampaign.Setting, &BEM_campaign);
		break;
	case 1:
		ConvertCampaignSetting(&gCampaign.Setting, &OGRE_campaign);
		break;
	default:
		ConvertCampaignSetting(&gCampaign.Setting, &OGRE_campaign);
		return 0;
	}
	return 1;
}

int SetupBuiltinDogfight(int idx)
{
	switch (idx)
	{
	case 0:
		ConvertCampaignSetting(&gCampaign.Setting, &df1);
		break;
	case 1:
		ConvertCampaignSetting(&gCampaign.Setting, &df2);
		break;
	default:
		ConvertCampaignSetting(&gCampaign.Setting, &df1);
		return 0;
	}
	return 1;
}

static void SetupObjectives(struct MissionOptions *mo, Mission *mission)
{
	int i;
	for (i = 0; i < (int)mission->Objectives.size; i++)
	{
		MissionObjective *mobj = CArrayGet(&mission->Objectives, i);
		struct Objective o;
		memset(&o, 0, sizeof o);
		assert(i < OBJECTIVE_MAX_OLD);
		o.color = objectiveColors[i];
		o.blowupObject = MapObjectGet(mobj->Index);
		o.pickupItem = pickupItems[mobj->Index % PICKUPS_COUNT];
		CArrayPushBack(&mo->Objectives, &o);
	}
}

static void CleanupPlayerInventory(
	struct PlayerData *data, int weapons[GUN_COUNT])
{
	int i;
	for (i = data->weaponCount - 1; i >= 0; i--)
	{
		if (!weapons[data->weapons[i]])
		{
			int j;
			for (j = i + 1; j < data->weaponCount; j++)
			{
				data->weapons[j - 1] = data->weapons[j];
			}
			data->weaponCount--;
		}
	}
}

static void SetupWeapons(
	struct PlayerData playerDatas[MAX_PLAYERS], int weapons[GUN_COUNT])
{
	int i;
	// Remove unavailable weapons from players inventories
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		CleanupPlayerInventory(&playerDatas[i], weapons);
	}
}

void SetRange(int start, int range)
{
	int i;

	for (i = 0; i < 8; i++)
	{
		gPicManager.palette[start + i] = cColorRanges[range].range[i];
	}
	CDogsSetPalette(gPicManager.palette);
}

void SetupMission(
	int buildTables, Mission *m, struct MissionOptions *mo, int missionIndex)
{
	int i;
	int x, y;

	MissionOptionsInit(mo);
	mo->index = missionIndex;
	mo->missionData = m;
	mo->doorPics =
	    doorStyles[abs(m->DoorStyle) % DOORSTYLE_COUNT];
	mo->keyPics = keyStyles[abs(m->KeyStyle) % KEYSTYLE_COUNT];
	for (i = 0; i < (int)m->Items.size; i++)
	{
		CArrayPushBack(
			&mo->MapObjects,
			MapObjectGet(*(int32_t *)CArrayGet(&m->Items, i)));
	}

	mo->exitPic = exitPics[2 * (abs(m->ExitStyle) % EXIT_COUNT)];
	mo->exitShadow = exitPics[2 * (abs(m->ExitStyle) % EXIT_COUNT) + 1];

	assert(m->Size.x > 0 && m->Size.y > 0 && "invalid map size");
	x = (rand() % (abs(m->Size.x) - EXIT_WIDTH));
	y = (rand() % (abs(m->Size.y) - EXIT_HEIGHT));
	mo->exitLeft = x;
	mo->exitRight = x + EXIT_WIDTH + 1;
	mo->exitTop = y;
	mo->exitBottom = y + EXIT_HEIGHT + 1;

	SetupObjectives(mo, m);
	SetupBadguysForMission(m);
	SetupWeapons(gPlayerDatas, m->Weapons);
	SetPaletteRanges(m->WallColor, m->FloorColor, m->RoomColor, m->AltColor);
	if (buildTables)
	{
		BuildTranslationTables(gPicManager.palette);
	}
}

void SetPaletteRanges(int wall_range, int floor_range, int room_range, int alt_range)
{
	SetRange(WALL_COLORS, abs(wall_range) % COLORRANGE_COUNT);
	SetRange(FLOOR_COLORS, abs(floor_range) % COLORRANGE_COUNT);
	SetRange(ROOM_COLORS, abs(room_range) % COLORRANGE_COUNT);
	SetRange(ALT_COLORS, abs(alt_range) % COLORRANGE_COUNT);
}

void MissionSetMessageIfComplete(struct MissionOptions *options)
{
	if (CanCompleteMission(options))
	{
		GameEvent msg;
		msg.Type = GAME_EVENT_SET_MESSAGE;
		strcpy(msg.u.SetMessage.Message, "Mission Complete");
		msg.u.SetMessage.Ticks = -1;
		GameEventsEnqueue(&gGameEvents, msg);
	}
}

int CheckMissionObjective(
	struct MissionOptions *options, int flags, ObjectiveType type)
{
	int idx;
	MissionObjective *mobj;
	struct Objective *o;
	if (!(flags & TILEITEM_OBJECTIVE))
	{
		return 0;
	}
	idx = ObjectiveFromTileItem(flags);
	mobj = CArrayGet(&options->missionData->Objectives, idx);
	if (mobj->Type != type)
	{
		return 0;
	}
	o = CArrayGet(&options->Objectives, idx);
	o->done++;
	MissionSetMessageIfComplete(options);
	return 1;
}

int CanCompleteMission(struct MissionOptions *options)
{
	int i;

	// Death is the only escape from dogfights and quick play
	if (gCampaign.Entry.mode == CAMPAIGN_MODE_DOGFIGHT)
	{
		return GetNumPlayersAlive() <= 1;
	}
	else if (gCampaign.Entry.mode == CAMPAIGN_MODE_QUICK_PLAY)
	{
		return GetNumPlayersAlive() == 0;
	}

	// Check all objective counts are enough
	for (i = 0; i < (int)options->Objectives.size; i++)
	{
		struct Objective *o = CArrayGet(&options->Objectives, i);
		MissionObjective *mobj =
			CArrayGet(&options->missionData->Objectives, i);
		if (o->done < mobj->Required)
		{
			return 0;
		}
	}

	return 1;
}

int IsMissionComplete(struct MissionOptions *options)
{
	int rescuesRequired = 0;
	int i;

	if (!CanCompleteMission(options))
	{
		return 0;
	}

	// Check if dogfight is complete
	if (gCampaign.Entry.mode == CAMPAIGN_MODE_DOGFIGHT &&
		GetNumPlayersAlive() <= 1)
	{
		return 1;
	}

	// Check that all surviving players are in exit zone
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		if (gPlayers[i] && !IsTileInExit(&gPlayers[i]->tileItem, options))
		{
			return 0;
		}
	}

	// Find number of rescues required
	// TODO: support multiple rescue objectives
	for (i = 0; i < (int)options->missionData->Objectives.size; i++)
	{
		MissionObjective *mobj =
			CArrayGet(&options->missionData->Objectives, i);
		if (mobj->Type == OBJECTIVE_RESCUE)
		{
			rescuesRequired = mobj->Required;
			break;
		}
	}
	// Check that enough prisoners are in exit zone
	if (rescuesRequired > 0)
	{
		int prisonersRescued = 0;
		TActor *a = ActorList();
		while (a != NULL)
		{
			if (a->character == CharacterStoreGetPrisoner(
				&gCampaign.Setting.characters, 0) &&
				IsTileInExit(&a->tileItem, options))
			{
				prisonersRescued++;
			}
			a = a->next;
		}
		if (prisonersRescued < rescuesRequired)
		{
			return 0;
		}
	}

	return 1;
}

struct EditorInfo GetEditorInfo(void)
{
	struct EditorInfo ei;
	ei.pickupCount = PICKUPS_COUNT;
	ei.keyCount = KEYSTYLE_COUNT;
	ei.doorCount = DOORSTYLE_COUNT;
	ei.exitCount = EXIT_COUNT;
	ei.rangeCount = COLORRANGE_COUNT;
	return ei;
}

const char *RangeName(int idx)
{
	if (idx >= 0 && idx < (int)COLORRANGE_COUNT)
	{
		return cColorRanges[idx].name;
	}
	else
	{
		return "Invalid";
	}
}
