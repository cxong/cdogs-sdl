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
#include "mission.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "game_events.h"
#include "gamedata.h"
#include "map.h"
#include "palette.h"
#include "defs.h"
#include "pic_manager.h"
#include "actors.h"


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
#define COLORRANGE_COUNT (sizeof( cColorRanges)/sizeof( struct ColorRange))


// +--------------------+
// |  Map objects info  |
// +--------------------+


static TMapObject mapItems[] = {
	{OFSPIC_BARREL2, OFSPIC_WRECKEDBARREL2, 8, 6, 40,
	 MAPOBJ_OUTSIDE},

	{OFSPIC_BOX, OFSPIC_WRECKEDBOX, 8, 6, 20,
	 MAPOBJ_INOPEN},

	{OFSPIC_BOX2, OFSPIC_WRECKEDBOX, 8, 6, 20,
	 MAPOBJ_INOPEN},

	{OFSPIC_CABINET, OFSPIC_WRECKEDCABINET, 8, 6, 20,
	 MAPOBJ_INSIDE | MAPOBJ_ONEWALLPLUS | MAPOBJ_INTERIOR |
	 MAPOBJ_FREEINFRONT},

	{OFSPIC_PLANT, OFSPIC_WRECKEDPLANT, 4, 3, 20,
	 MAPOBJ_INSIDE | MAPOBJ_ONEWALLPLUS},

	{OFSPIC_TABLE, OFSPIC_WRECKEDTABLE, 8, 6, 20,
	 MAPOBJ_INSIDE | MAPOBJ_NOWALLS},

	{OFSPIC_CHAIR, OFSPIC_WRECKEDCHAIR, 4, 3, 20,
	 MAPOBJ_INSIDE | MAPOBJ_NOWALLS},

	{OFSPIC_ROD, OFSPIC_WRECKEDCHAIR, 4, 3, 60,
	 MAPOBJ_INOPEN},

	{OFSPIC_SKULLBARREL_WOOD, OFSPIC_WRECKEDBARREL_WOOD, 8, 6, 40,
	 MAPOBJ_OUTSIDE | MAPOBJ_EXPLOSIVE},

	{OFSPIC_BARREL_WOOD, OFSPIC_WRECKEDBARREL_WOOD, 8, 6, 40,
	 MAPOBJ_OUTSIDE},

	{OFSPIC_GRAYBOX, OFSPIC_WRECKEDBOX_WOOD, 8, 6, 20,
	 MAPOBJ_OUTSIDE},

	{OFSPIC_GREENBOX, OFSPIC_WRECKEDBOX_WOOD, 8, 6, 20,
	 MAPOBJ_OUTSIDE},

	{OFSPIC_OGRESTATUE, OFSPIC_WRECKEDSAFE, 8, 6, 80,
	 MAPOBJ_INSIDE | MAPOBJ_ONEWALLPLUS | MAPOBJ_INTERIOR |
	 MAPOBJ_FREEINFRONT},

	{OFSPIC_WOODTABLE_CANDLE, OFSPIC_WRECKEDTABLE, 8, 6, 20,
	 MAPOBJ_INSIDE | MAPOBJ_NOWALLS},

	{OFSPIC_WOODTABLE, OFSPIC_WRECKEDBOX_WOOD, 8, 6, 20,
	 MAPOBJ_INSIDE | MAPOBJ_NOWALLS},

	{OFSPIC_TREE, OFSPIC_TREE_REMAINS, 4, 3, 40,
	 MAPOBJ_INOPEN},

	{OFSPIC_BOOKSHELF, OFSPIC_WRECKEDBOX_WOOD, 8, 3, 20,
	 MAPOBJ_INSIDE | MAPOBJ_ONEWALLPLUS | MAPOBJ_INTERIOR |
	 MAPOBJ_FREEINFRONT},

	{OFSPIC_WOODENBOX, OFSPIC_WRECKEDBOX_WOOD, 8, 6, 20,
	 MAPOBJ_OUTSIDE},

	{OFSPIC_CLOTHEDTABLE, OFSPIC_WRECKEDBOX_WOOD, 8, 6, 20,
	 MAPOBJ_INSIDE | MAPOBJ_NOWALLS},

	{OFSPIC_STEELTABLE, OFSPIC_WRECKEDSAFE, 8, 6, 30,
	 MAPOBJ_INSIDE | MAPOBJ_NOWALLS},

	{OFSPIC_AUTUMNTREE, OFSPIC_AUTUMNTREE_REMAINS, 4, 3, 40,
	 MAPOBJ_INOPEN},

	{OFSPIC_GREENTREE, OFSPIC_GREENTREE_REMAINS, 8, 6, 40,
	 MAPOBJ_INOPEN},

// Used-to-be blow-ups

	{OFSPIC_BOX3, OFSPIC_WRECKEDBOX3, 8, 6, 40,
	 MAPOBJ_OUTSIDE | MAPOBJ_EXPLOSIVE | MAPOBJ_QUAKE},

	{OFSPIC_SAFE, OFSPIC_WRECKEDSAFE, 8, 6, 100,
	 MAPOBJ_INSIDE | MAPOBJ_ONEWALLPLUS | MAPOBJ_INTERIOR |
	 MAPOBJ_FREEINFRONT},

	{OFSPIC_REDBOX, OFSPIC_WRECKEDBOX_WOOD, 8, 6, 40,
	 MAPOBJ_OUTSIDE | MAPOBJ_FLAMMABLE},

	{OFSPIC_LABTABLE, OFSPIC_WRECKEDSAFE, 8, 6, 60,
	 MAPOBJ_INSIDE | MAPOBJ_ONEWALLPLUS | MAPOBJ_INTERIOR |
	 MAPOBJ_FREEINFRONT | MAPOBJ_POISONOUS},

	{OFSPIC_TERMINAL, OFSPIC_WRECKEDBOX_WOOD, 8, 6, 40,
	 MAPOBJ_INSIDE | MAPOBJ_NOWALLS},

	{OFSPIC_BARREL, OFSPIC_WRECKEDBARREL, 8, 6, 40,
	 MAPOBJ_OUTSIDE | MAPOBJ_FLAMMABLE},

	{OFSPIC_ROCKET, OFSPIC_BURN, 8, 6, 40,
	 MAPOBJ_OUTSIDE | MAPOBJ_EXPLOSIVE | MAPOBJ_QUAKE},

	{OFSPIC_EGG, OFSPIC_EGG_REMAINS, 8, 6, 30,
	 (MAPOBJ_IMPASSABLE | MAPOBJ_CANBESHOT)},

	{OFSPIC_BLOODSTAIN, 0, 0, 0, 0,
	 MAPOBJ_ON_WALL},

	{OFSPIC_WALL_SKULL, 0, 0, 0, 0,
	 MAPOBJ_ON_WALL},

	{OFSPIC_BONE_N_BLOOD, 0, 0, 0, 0, 0},

	{OFSPIC_BULLET_MARKS, 0, 0, 0, 0,
	 MAPOBJ_ON_WALL},

	{OFSPIC_SKULL, 0, 0, 0, 0, 0},

	{OFSPIC_BLOOD, 0, 0, 0, 0, 0},

	{OFSPIC_SCRATCH, 0, 0, 0, 0, MAPOBJ_ON_WALL},

	{OFSPIC_WALL_STUFF, 0, 0, 0, 0, MAPOBJ_ON_WALL},

	{OFSPIC_WALL_GOO, 0, 0, 0, 0, MAPOBJ_ON_WALL},

	{OFSPIC_GOO, 0, 0, 0, 0, 0}

};
#define ITEMS_COUNT (sizeof( mapItems)/sizeof( TMapObject))


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
#define KEYSTYLE_COUNT  (sizeof( keyStyles)/sizeof( int *))


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

#define DOORSTYLE_COUNT (sizeof( doorStyles)/sizeof( struct DoorPic *))


// +-------------+
// |  Exit info  |
// +-------------+


static int exitPics[] = {
	375, 376,	// hazard stripes
	380, 381	// yellow plates
};

// Every exit has TWO pics, so actual # of exits == # pics / 2!
#define EXIT_COUNT (sizeof( exitPics)/sizeof( int)/2)


// +----------------+
// |  Mission info  |
// +----------------+


struct Mission dogFight1 = {
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

struct Mission dogFight2 = {
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

struct Mission gQuickPlayMission;


// +-----------------+
// |  Campaign info  |
// +-----------------+

#include "files.h"
#include <missions/bem.h>
#include <missions/ogre.h>


static CampaignSetting df1 =
{
	"Dogfight in the dungeon",
	"", "",
	1, &dogFight1,
	0, NULL
};

static CampaignSetting df2 =
{
	"Cubicle wars",
	"", "",
	1, &dogFight2,
	0, NULL
};


// +---------------------------------------------------+
// |  Objective colors (for automap & status display)  |
// +---------------------------------------------------+


color_t objectiveColors[OBJECTIVE_MAX] =
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

static void SetupBadguysForMission(struct Mission *mission)
{
	int i;
	CharacterStore *s = &gCampaign.Setting.characters;

	CharacterStoreResetOthers(s);

	if (s->otherCount <= 0)
	{
		return;
	}

	for (i = 0; i < mission->objectiveCount; i++)
	{
		if (mission->objectives[i].type == OBJECTIVE_RESCUE)
		{
			CharacterStoreAddPrisoner(
				s, mission->objectives[i].index);
			break;	// TODO: multiple prisoners
		}
	}

	for (i = 0; i < mission->baddieCount; i++)
	{
		CharacterStoreAddBaddie(s, mission->baddies[i]);
	}

	for (i = 0; i < mission->specialCount; i++)
	{
		CharacterStoreAddSpecial(s, mission->specials[i]);
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

// Generate a random partition of an integer `total` into a pair of ints x, y
// With the restrictions that neither x, y are less than min, and
// neither x, y are greater than max
static void GenerateRandomPairPartitionWithRestrictions(
	int *x, int *y, int total, int min, int max)
{
	int xLow, xHigh;

	// Check for invalid input
	// Can't proceed if exactly half of total is greater than max,
	// or if total less than min
	if ((total + 1) / 2 > max || total < min)
	{
		assert(0);
		*x = total / 2;
		*y = total - *x;
		return;
	}

	// Find range of x first
	// Must be at least min, or total - max
	// Must be at most max, or total - min
	xLow = MAX(min, total - max);
	xHigh = MIN(max, total - min);
	*x = xLow + (rand() % (xHigh - xLow + 1));
	*y = total - *x;
	assert(*x >= min);
	assert(*y >= min);
	assert(*x <= max);
	assert(*y <= max);
}

static void SetupQuickPlayMapSize(
	QuickPlayQuantity size, int *width, int *height)
{
	const int minMapDim = 16;
	const int maxMapDim = 64;
	// Map sizes based on total dimensions (width + height)
	// Small: 32 - 64
	// Medium: 64 - 96
	// Large: 96 - 128
	// Restrictions: at least 16, at most 64 per side
	switch (size)
	{
	case QUICKPLAY_QUANTITY_ANY:
		GenerateRandomPairPartitionWithRestrictions(
			width, height,
			32 + (rand() % (128 - 32 + 1)),
			minMapDim, maxMapDim);
		break;
	case QUICKPLAY_QUANTITY_SMALL:
		GenerateRandomPairPartitionWithRestrictions(
			width, height,
			32 + (rand() % (64 - 32 + 1)),
			minMapDim, maxMapDim);
		break;
	case QUICKPLAY_QUANTITY_MEDIUM:
		GenerateRandomPairPartitionWithRestrictions(
			width, height,
			64 + (rand() % (96 - 64 + 1)),
			minMapDim, maxMapDim);
		break;
	case QUICKPLAY_QUANTITY_LARGE:
		GenerateRandomPairPartitionWithRestrictions(
			width, height,
			96 + (rand() % (128 - 96 + 1)),
			minMapDim, maxMapDim);
		break;
	default:
		assert(0);
		break;
	}
}

// Generate a quick play parameter based on the quantity setting, and various
// thresholds
// e.g. if qty is "small", generate random number between small and medium
static int GenerateQuickPlayParam(
	QuickPlayQuantity qty, int small, int medium, int large, int max)
{
	switch (qty)
	{
	case QUICKPLAY_QUANTITY_ANY:
		return small + (rand() % (max - small + 1));
	case QUICKPLAY_QUANTITY_SMALL:
		return small + (rand() % (medium - small + 1));
	case QUICKPLAY_QUANTITY_MEDIUM:
		return medium + (rand() % (large - medium + 1));
	case QUICKPLAY_QUANTITY_LARGE:
		return large + (rand() % (max - large + 1));
	default:
		assert(0);
		return 0;
	}
}

static void SetupQuickPlayEnemy(
	Character *enemy, const QuickPlayConfig *config, gun_e gun)
{
	enemy->looks.armedBody = BODY_ARMED;
	enemy->looks.unarmedBody = BODY_UNARMED;
	enemy->looks.face = rand() % FACE_COUNT;
	enemy->gun = gun;
	enemy->speed =
		GenerateQuickPlayParam(config->EnemySpeed, 64, 112, 160, 256);
	if (IsShortRange(enemy->gun))
	{
		enemy->speed = enemy->speed * 4 / 3;
	}
	if (IsShortRange(enemy->gun))
	{
		enemy->bot.probabilityToMove = 35 + (rand() % 35);
	}
	else
	{
		enemy->bot.probabilityToMove = 30 + (rand() % 30);
	}
	enemy->bot.probabilityToTrack = 10 + (rand() % 60);
	if (enemy->gun == GUN_KNIFE)
	{
		enemy->bot.probabilityToShoot = 0;
	}
	else if (IsHighDPS(enemy->gun))
	{
		enemy->bot.probabilityToShoot = 2 + (rand() % 10);
	}
	else
	{
		enemy->bot.probabilityToShoot = 15 + (rand() % 30);
	}
	enemy->bot.actionDelay = rand() % (50 + 1);
	enemy->looks.skin = rand() % SHADE_COUNT;
	enemy->looks.arm = rand() % SHADE_COUNT;
	enemy->looks.body = rand() % SHADE_COUNT;
	enemy->looks.leg = rand() % SHADE_COUNT;
	enemy->looks.hair = rand() % SHADE_COUNT;
	enemy->maxHealth =
		GenerateQuickPlayParam(config->EnemyHealth, 10, 20, 40, 60);
	enemy->flags = 0;
}

static void SetupQuickPlayEnemies(
	struct Mission *mission,
	CharacterStore *store,
	const QuickPlayConfig *config)
{
	int i;
	for (i = 0; i < mission->baddieCount; i++)
	{
		Character *ch;
		gun_e gun;
		mission->baddies[i] = i;

		for (;;)
		{
			gun = rand() % GUN_COUNT;
			// make at least one of each type of enemy:
			// - Short range weapon
			// - Long range weapon
			// - High explosive weapon
			if (i == 0 && !IsShortRange(gun))
			{
				continue;
			}
			if (i == 1 && !IsLongRange(gun))
			{
				continue;
			}
			if (i == 2 && config->EnemiesWithExplosives && !IsHighDPS(gun))
			{
				continue;
			}

			if (!config->EnemiesWithExplosives && IsHighDPS(gun))
			{
				continue;
			}
			break;
		}
		ch = CharacterStoreAddOther(store);
		SetupQuickPlayEnemy(ch, config, gun);
		CharacterSetLooks(ch, &ch->looks);
	}
}

void SetupQuickPlayCampaign(
	CampaignSettingNew *setting, const QuickPlayConfig *config)
{
	int i;
	strcpy(gQuickPlayMission.title, "");
	strcpy(gQuickPlayMission.description, "");
	gQuickPlayMission.wallStyle = rand() % WALL_STYLE_COUNT;
	gQuickPlayMission.floorStyle = rand() % FLOOR_STYLE_COUNT;
	gQuickPlayMission.roomStyle = rand() % FLOOR_STYLE_COUNT;
	gQuickPlayMission.exitStyle = rand() % EXIT_COUNT;
	gQuickPlayMission.keyStyle = rand() % KEYSTYLE_COUNT;
	gQuickPlayMission.doorStyle = rand() % DOORSTYLE_COUNT;
	SetupQuickPlayMapSize(
		config->MapSize,
		&gQuickPlayMission.mapWidth, &gQuickPlayMission.mapHeight);
	gQuickPlayMission.wallCount =
		GenerateQuickPlayParam(config->WallCount, 0, 5, 15, 30);
	gQuickPlayMission.wallLength =
		GenerateQuickPlayParam(config->WallLength, 1, 3, 6, 12);
	gQuickPlayMission.roomCount =
		GenerateQuickPlayParam(config->RoomCount, 0, 2, 5, 12);
	gQuickPlayMission.squareCount =
		GenerateQuickPlayParam(config->SquareCount, 0, 1, 3, 6);
	gQuickPlayMission.exitLeft = 0;
	gQuickPlayMission.exitTop = 0;
	gQuickPlayMission.exitRight = 0;
	gQuickPlayMission.exitBottom = 0;
	gQuickPlayMission.objectiveCount = 0;
	gQuickPlayMission.baddieCount =
		GenerateQuickPlayParam(config->EnemyCount, 3, 5, 8, 12);

	gQuickPlayMission.specialCount = 0;
	gQuickPlayMission.itemCount =
		GenerateQuickPlayParam(config->SquareCount, 0, 2, 5, 10);
	for (i = 0; i < gQuickPlayMission.itemCount; i++)
	{
		gQuickPlayMission.items[i] = i;
		gQuickPlayMission.itemDensity[i] =
			GenerateQuickPlayParam(config->SquareCount, 0, 5, 10, 20);
	}
	gQuickPlayMission.baddieDensity =
		(40 + (rand() % 20)) / gQuickPlayMission.baddieCount;
	gQuickPlayMission.weaponSelection = 0;
	strcpy(gQuickPlayMission.song, "");
	strcpy(gQuickPlayMission.map, "");
	gQuickPlayMission.wallRange = rand() % (COLORRANGE_COUNT - 1 + 1);
	gQuickPlayMission.floorRange = rand() % (COLORRANGE_COUNT - 1 + 1);
	gQuickPlayMission.roomRange = rand() % (COLORRANGE_COUNT - 1 + 1);
	gQuickPlayMission.altRange = rand() % (COLORRANGE_COUNT - 1 + 1);

	strcpy(setting->title, "Quick play");
	strcpy(setting->author, "");
	strcpy(setting->description, "");
	setting->missionCount = 1;
	CMALLOC(setting->missions, sizeof *setting->missions);
	memcpy(
		setting->missions,
		&gQuickPlayMission,
		sizeof *setting->missions * setting->missionCount);
	CharacterStoreInit(&setting->characters);
	SetupQuickPlayEnemies(&gQuickPlayMission, &setting->characters, config);
}

static void SetupObjective(int o, struct Mission *mission)
{
	gMission.objectives[o].done = 0;
	gMission.objectives[o].required = mission->objectives[o].required;
	gMission.objectives[o].count = mission->objectives[o].count;
	gMission.objectives[o].color = objectiveColors[o];
	gMission.objectives[o].blowupObject =
	    &mapItems[mission->objectives[o].index % ITEMS_COUNT];
	gMission.objectives[o].pickupItem =
	    pickupItems[mission->objectives[o].index % PICKUPS_COUNT];
}

static void SetupObjectives(struct Mission *mission)
{
	int i;

	for (i = 0; i < mission->objectiveCount; i++)
		SetupObjective(i, mission);
}

static void CleanupPlayerInventory(struct PlayerData *data, int weapons)
{
	int i, j;

	for (i = data->weaponCount - 1; i >= 0; i--)
		if ((weapons & (1 << data->weapons[i])) == 0) {
			for (j = i + 1; j < data->weaponCount; j++)
				data->weapons[j - 1] = data->weapons[j];
			data->weaponCount--;
		}
}

static void SetupWeapons(int weapons)
{
	int i;

	if (!weapons)
		weapons = -1;

	gMission.weaponCount = 0;
	for (i = 0; i < GUN_COUNT && gMission.weaponCount < WEAPON_MAX;
	     i++)
		if ((weapons & (1 << i)) != 0) {
			gMission.availableWeapons[gMission.weaponCount] =
			    i;
			gMission.weaponCount++;
		}
	// Now remove unavailable weapons from players inventories
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		CleanupPlayerInventory(&gPlayerDatas[i], weapons);
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

void SetupMission(int idx, int buildTables, CampaignOptions *campaign)
{
	int i;
	int x, y;
	struct Mission *m;

	memset(&gMission, 0, sizeof(gMission));
	gMission.index = idx;
	m = &campaign->Setting.missions[abs(idx) % campaign->Setting.missionCount];
	gMission.missionData = m;
	gMission.doorPics =
	    doorStyles[abs(m->doorStyle) % DOORSTYLE_COUNT];
	gMission.keyPics = keyStyles[abs(m->keyStyle) % KEYSTYLE_COUNT];
	gMission.objectCount = m->itemCount;

	for (i = 0; i < m->itemCount; i++)
		gMission.mapObjects[i] =
		    &mapItems[abs(m->items[i]) % ITEMS_COUNT];

	srand(10 * idx + campaign->seed);

	gMission.exitPic = exitPics[2 * (abs(m->exitStyle) % EXIT_COUNT)];
	gMission.exitShadow =
	    exitPics[2 * (abs(m->exitStyle) % EXIT_COUNT) + 1];
	if (m->exitLeft > 0) {
		gMission.exitLeft = m->exitLeft * TILE_WIDTH;
		gMission.exitRight = m->exitRight * TILE_WIDTH;
		gMission.exitTop = m->exitTop * TILE_HEIGHT;
		gMission.exitBottom = m->exitBottom * TILE_HEIGHT;
	} else {
		if (m->mapWidth)
			x = (rand() % (abs(m->mapWidth) - EXIT_WIDTH)) +
			    (XMAX - abs(m->mapWidth)) / 2;
		else
			x = rand() % (XMAX - EXIT_WIDTH);
		if (m->mapHeight)
			y = (rand() % (abs(m->mapHeight) - EXIT_HEIGHT)) +
			    (YMAX - abs(m->mapHeight)) / 2;
		else
			y = rand() % (YMAX - EXIT_HEIGHT);
		gMission.exitLeft = x * TILE_WIDTH;
		gMission.exitRight = (x + EXIT_WIDTH + 1) * TILE_WIDTH;
		gMission.exitTop = y * TILE_HEIGHT;
		gMission.exitBottom = (y + EXIT_HEIGHT + 1) * TILE_HEIGHT;
	}
	SetupObjectives(m);
	SetupBadguysForMission(m);
	SetupWeapons(m->weaponSelection);
	SetPaletteRanges(m->wallRange, m->floorRange, m->roomRange, m->altRange);
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

int CheckMissionObjective(
	struct MissionOptions *options, int flags, ObjectiveType type)
{
	int idx;
	if (!(flags & TILEITEM_OBJECTIVE))
	{
		return 0;
	}
	idx = ObjectiveFromTileItem(flags);
	if (options->missionData->objectives[idx].type != type)
	{
		return 0;
	}
	gMission.objectives[idx].done++;
	if (CanCompleteMission(options))
	{
		GameEvent msg;
		msg.Type = GAME_EVENT_SET_MESSAGE;
		strcpy(msg.u.SetMessage.Message, "Mission Complete");
		msg.u.SetMessage.Ticks = -1;
		GameEventsEnqueue(&gGameEvents, msg);
	}
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
	for (i = 0; i < options->missionData->objectiveCount; i++)
	{
		if (options->objectives[i].done < options->objectives[i].required)
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
	for (i = 0; i < gMission.missionData->objectiveCount; i++)
	{
		if (gMission.missionData->objectives[i].type == OBJECTIVE_RESCUE)
		{
			rescuesRequired = gMission.objectives[i].required;
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

void GetEditorInfo(struct EditorInfo *info)
{
	info->itemCount = ITEMS_COUNT;
	info->pickupCount = PICKUPS_COUNT;
	info->keyCount = KEYSTYLE_COUNT;
	info->doorCount = DOORSTYLE_COUNT;
	info->exitCount = EXIT_COUNT;
	info->rangeCount = COLORRANGE_COUNT;
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
