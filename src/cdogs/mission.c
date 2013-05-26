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

#include <string.h>
#include <stdlib.h>
#include "gamedata.h"
#include "map.h"
#include "defs.h"
#include "pics.h"
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


static struct ColorRange cColorRanges[] = {
	{"Red 1",
	 {{33, 0, 0}, {29, 0, 0}, {25, 0, 0}, {21, 0, 0}, {17, 0, 0},
	  {13, 0, 0}, {9, 0, 0}, {5, 0, 0}}},
	{"Red 2",
	 {{28, 0, 0}, {23, 0, 0}, {18, 0, 0}, {14, 0, 0}, {12, 0, 0},
	  {10, 0, 0}, {8, 0, 0}, {5, 0, 0}}},
	{"Red 3",
	 {{18, 0, 0}, {15, 0, 0}, {13, 0, 0}, {10, 0, 0}, {8, 0, 0},
	  {5, 0, 0}, {3, 0, 0}, {0, 0, 0}}},

	{"Green 1",
	 {{0, 33, 0}, {0, 29, 0}, {0, 25, 0}, {0, 21, 0}, {0, 17, 0},
	  {0, 13, 0}, {0, 9, 0}, {0, 5, 0}}},
	{"Green 2",
	 {{0, 28, 0}, {0, 23, 0}, {0, 18, 0}, {0, 14, 0}, {0, 12, 0},
	  {0, 10, 0}, {0, 8, 0}, {0, 5, 0}}},
	{"Green 3",
	 {{0, 18, 0}, {0, 15, 0}, {0, 13, 0}, {0, 10, 0}, {0, 8, 0},
	  {0, 5, 0}, {0, 3, 0}, {0, 0, 0}}},

	{"Blue 1",
	 {{0, 0, 33}, {0, 0, 29}, {0, 0, 25}, {0, 0, 21}, {0, 0, 17},
	  {0, 0, 13}, {0, 0, 9}, {0, 0, 5}}},
	{"Blue 2",
	 {{0, 0, 28}, {0, 0, 23}, {0, 0, 18}, {0, 0, 14}, {0, 0, 12},
	  {0, 0, 10}, {0, 0, 8}, {0, 0, 5}}},
	{"Blue 3",
	 {{0, 0, 18}, {0, 0, 15}, {0, 0, 13}, {0, 0, 10}, {0, 0, 8},
	  {0, 0, 5}, {0, 0, 3}, {0, 0, 0}}},

	{"Purple 1",
	 {{33, 0, 33}, {29, 0, 29}, {25, 0, 25}, {21, 0, 21}, {17, 0, 17},
	  {13, 0, 13}, {9, 0, 9}, {5, 0, 5}}},
	{"Purple 2",
	 {{28, 0, 28}, {23, 0, 23}, {18, 0, 18}, {14, 0, 14}, {12, 0, 12},
	  {10, 0, 10}, {8, 0, 8}, {5, 0, 5}}},
	{"Purple 3",
	 {{18, 0, 18}, {15, 0, 15}, {13, 0, 13}, {10, 0, 10}, {8, 0, 8},
	  {5, 0, 5}, {3, 0, 3}, {0, 0, 0}}},

	{"Gray 1",
	 {{33, 33, 33}, {28, 28, 28}, {23, 23, 23}, {18, 18, 18},
	  {15, 15, 15}, {12, 12, 12}, {10, 10, 10}, {8, 8, 8}}},
	{"Gray 2",
	 {{28, 28, 28}, {23, 23, 23}, {18, 18, 18}, {14, 14, 14},
	  {12, 12, 12}, {10, 10, 10}, {8, 8, 8}, {5, 5, 5}}},
	{"Gray 3",
	 {{18, 18, 18}, {15, 15, 15}, {13, 13, 13}, {10, 10, 10},
	  {8, 8, 8}, {5, 5, 5}, {3, 3, 3}, {0, 0, 0}}},

	{"Gray/blue 1",
	 {{23, 23, 33}, {20, 20, 29}, {18, 18, 26}, {15, 15, 23},
	  {12, 12, 20}, {10, 10, 17}, {8, 8, 14}, {5, 5, 10}}},
	{"Gray/blue 2",
	 {{18, 18, 28}, {16, 16, 25}, {14, 14, 23}, {12, 12, 20},
	  {10, 10, 17}, {8, 8, 15}, {6, 6, 12}, {4, 4, 9}}},
	{"Gray/blue 3",
	 {{13, 13, 23}, {12, 12, 21}, {10, 10, 19}, {9, 9, 17}, {7, 7, 14},
	  {6, 6, 12}, {4, 4, 10}, {3, 3, 8}}},

	{"Yellowish 1",
	 {{37, 32, 11}, {33, 28, 10}, {29, 24, 9}, {25, 21, 8},
	  {21, 17, 7}, {17, 13, 6}, {14, 9, 4}, {11, 6, 2}}},
	{"Yellowish 2",
	 {{33, 28, 9}, {29, 24, 8}, {25, 21, 7}, {21, 17, 6}, {17, 13, 5},
	  {14, 9, 4}, {11, 6, 2}, {11, 4, 1}}},
	{"Yellowish 3",
	 {{29, 24, 7}, {25, 21, 6}, {21, 17, 5}, {17, 13, 4}, {14, 9, 3},
	  {11, 6, 2}, {9, 4, 1}, {11, 2, 0}}},

	{"Brown 1",
	 {{33, 17, 0}, {29, 15, 0}, {25, 13, 0}, {21, 11, 0}, {17, 9, 0},
	  {13, 7, 0}, {9, 5, 0}, {5, 3, 0}}},
	{"Brown 2",
	 {{28, 14, 0}, {23, 12, 0}, {18, 9, 0}, {14, 7, 0}, {12, 6, 0},
	  {10, 5, 0}, {8, 4, 0}, {5, 3, 0}}},
	{"Brown 3",
	 {{18, 9, 0}, {15, 8, 0}, {13, 7, 0}, {10, 5, 0}, {8, 4, 0},
	  {5, 3, 0}, {3, 2, 0}, {0, 0, 0}}},

	{"Cyan 1",
	 {{0, 33, 33}, {0, 29, 29}, {0, 25, 25}, {0, 21, 21}, {0, 17, 17},
	  {0, 13, 13}, {0, 9, 9}, {0, 5, 5}}},
	{"Cyan 2",
	 {{0, 28, 28}, {0, 23, 23}, {0, 18, 18}, {0, 14, 14}, {0, 12, 12},
	  {0, 10, 10}, {0, 8, 8}, {0, 5, 5}}},
	{"Cyan 3",
	 {{0, 18, 18}, {0, 15, 15}, {0, 13, 13}, {0, 10, 10}, {0, 8, 8},
	  {0, 5, 5}, {0, 3, 3}, {0, 0, 0}}},
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

#include <missions/bem.h>
#include <missions/ogre.h>


static CampaignSetting df1 =
{
	"Dogfight in the dungeon",
	"", "",
	1, &dogFight1,
	0, NULL,
	""
};

static CampaignSetting df2 =
{
	"Cubicle wars",
	"", "",
	1, &dogFight2,
	0, NULL,
	""
};

static TBadGuy gQuickPlayEnemies[BADDIE_MAX];

static CampaignSetting gQuickPlayCampaignSetting =
{
	"Quick play",
	"", "",
	1, &gQuickPlayMission,
	BADDIE_MAX, gQuickPlayEnemies,
	""
};


// +---------------------------------------------------+
// |  Objective colors (for automap & status display)  |
// +---------------------------------------------------+


unsigned char objectiveColors[OBJECTIVE_MAX] = { 14, 16, 22, 31, 39 };


// +-----------------------+
// |  And now the code...  |
// +-----------------------+


void SetupMissionCharacter(int index, const TBadGuy * b)
{
	SetCharacter(index, b->facePic, b->skinColor, b->hairColor,
		     b->bodyColor, b->armColor, b->legColor);
	characterDesc[index].armedBodyPic = b->armedBodyPic;
	characterDesc[index].unarmedBodyPic = b->unarmedBodyPic;
	characterDesc[index].speed = b->speed;
	characterDesc[index].probabilityToMove = b->probabilityToMove;
	characterDesc[index].probabilityToTrack = b->probabilityToTrack;
	characterDesc[index].probabilityToShoot = b->probabilityToShoot;
	characterDesc[index].actionDelay = b->actionDelay;
	characterDesc[index].defaultGun = b->gun;
	characterDesc[index].maxHealth = b->health;
	characterDesc[index].flags = b->flags;
}

static void SetupBadguysForMission(struct Mission *mission)
{
	int i, index;
	const TBadGuy *b;
	CampaignSetting *s = gCampaign.setting;

	if (s->characterCount <= 0)
		return;

	for (i = 0; i < gMission.missionData->objectiveCount; i++)
		if (gMission.missionData->objectives[i].type ==
		    OBJECTIVE_RESCUE) {
			b = &s->characters[gMission.missionData->
					   objectives[i].index %
					   s->characterCount];
			SetupMissionCharacter(CHARACTER_PRISONER, b);
			break;
		}

	for (i = 0; i < mission->baddieCount; i++) {
		index = i + CHARACTER_OTHERS;
		if (index >= CHARACTER_COUNT)
			break;

		b = &s->characters[mission->baddies[i] %
				   s->characterCount];
		SetupMissionCharacter(index, b);
	}
	for (i = 0; i < mission->specialCount; i++) {
		index = i + mission->baddieCount + CHARACTER_OTHERS;
		if (index >= CHARACTER_COUNT)
			break;

		b = &s->characters[mission->specials[i] %
				   s->characterCount];
		SetupMissionCharacter(index, b);
	}
}

int SetupBuiltinCampaign(int index)
{
	switch (index) {
	case 0:
		gCampaign.setting = &BEM_campaign;
		break;
	case 1:
		gCampaign.setting = &OGRE_campaign;
		break;
	default:
		gCampaign.setting = &OGRE_campaign;
		return 0;
	}
	return 1;
}

int SetupBuiltinDogfight(int index)
{
	switch (index) {
	case 0:
		gCampaign.setting = &df1;
		break;
	case 1:
		gCampaign.setting = &df2;
		break;
	default:
		gCampaign.setting = &df1;
		return 0;
	}
	return 1;
}

CampaignSetting *SetupAndGetQuickPlay(void)
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
	gQuickPlayMission.mapWidth = 16 + (rand() % (64 - 16 + 1));
	gQuickPlayMission.mapHeight = 16 + (rand() % (64 - 16 + 1));
	gQuickPlayMission.wallCount = rand() % (200 + 1);
	gQuickPlayMission.wallLength = 1 + (rand() % (200 - 1 + 1));
	gQuickPlayMission.roomCount = rand() % (100 + 1);
	gQuickPlayMission.squareCount = rand() % (100 + 1);
	gQuickPlayMission.exitLeft = 0;
	gQuickPlayMission.exitTop = 0;
	gQuickPlayMission.exitRight = 0;
	gQuickPlayMission.exitBottom = 0;
	gQuickPlayMission.objectiveCount = 0;
	gQuickPlayMission.baddieCount = 1 + (rand() % (BADDIE_MAX - 1));
	for (i = 0; i < gQuickPlayMission.baddieCount; i++)
	{
		gQuickPlayMission.baddies[i] = i;
		gQuickPlayEnemies[i].armedBodyPic = BODY_ARMED;
		gQuickPlayEnemies[i].unarmedBodyPic = BODY_UNARMED;
		gQuickPlayEnemies[i].facePic = rand() % FACE_COUNT;
		gQuickPlayEnemies[i].speed = 128 + (rand() % (512 - 128 + 1));
		gQuickPlayEnemies[i].probabilityToMove = rand() % (100 + 1);
		gQuickPlayEnemies[i].probabilityToTrack = rand() % (100 + 1);
		gQuickPlayEnemies[i].probabilityToShoot = rand() % (100 + 1);
		gQuickPlayEnemies[i].actionDelay = rand() % (50 + 1);
		gQuickPlayEnemies[i].gun = rand() % GUN_COUNT;
		gQuickPlayEnemies[i].skinColor = rand() % SHADE_COUNT;
		gQuickPlayEnemies[i].armColor = rand() % SHADE_COUNT;
		gQuickPlayEnemies[i].bodyColor = rand() % SHADE_COUNT;
		gQuickPlayEnemies[i].legColor = rand() % SHADE_COUNT;
		gQuickPlayEnemies[i].hairColor = rand() % SHADE_COUNT;
		gQuickPlayEnemies[i].health = 10 + (rand() % (500 - 10 + 1));
		gQuickPlayEnemies[i].flags = 0;
	}
	gQuickPlayMission.specialCount = 0;
	gQuickPlayMission.itemCount = rand() % (ITEMS_MAX + 1);
	for (i = 0; i < gQuickPlayMission.itemCount; i++)
	{
		gQuickPlayMission.items[i] = i;
		gQuickPlayMission.itemDensity[i] = rand() % (512 + 1);
		gQuickPlayMission.itemDensity[i] /= 16;	// tone down the items
	}
	gQuickPlayMission.baddieDensity = rand() % (100 + 1);
	gQuickPlayMission.weaponSelection = 0;
	strcpy(gQuickPlayMission.song, "");
	strcpy(gQuickPlayMission.map, "");
	gQuickPlayMission.wallRange = rand() % (COLORRANGE_COUNT - 1 + 1);
	gQuickPlayMission.floorRange = rand() % (COLORRANGE_COUNT - 1 + 1);
	gQuickPlayMission.roomRange = rand() % (COLORRANGE_COUNT - 1 + 1);
	gQuickPlayMission.altRange = rand() % (COLORRANGE_COUNT - 1 + 1);
	return &gQuickPlayCampaignSetting;
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
	CleanupPlayerInventory(&gPlayer1Data, weapons);
	CleanupPlayerInventory(&gPlayer2Data, weapons);
}

void SetRange(int start, int range)
{
	int i;

	for (i = 0; i < 8; i++)
		gPalette[start + i] = cColorRanges[range].range[i];
	CDogsSetPalette(gPalette);
}

void SetupMission(int index, int buildTables, CampaignOptions *campaign)
{
	int i;
	int x, y;
	struct Mission *m;

	memset(&gMission, 0, sizeof(gMission));
	gMission.index = index;
	m = &campaign->setting->missions[abs(index) % campaign->setting->missionCount];
	gMission.missionData = m;
	gMission.doorPics =
	    doorStyles[abs(m->doorStyle) % DOORSTYLE_COUNT];
	gMission.keyPics = keyStyles[abs(m->keyStyle) % KEYSTYLE_COUNT];
	gMission.objectCount = m->itemCount;

	for (i = 0; i < m->itemCount; i++)
		gMission.mapObjects[i] =
		    &mapItems[abs(m->items[i]) % ITEMS_COUNT];

	srand(10 * index + campaign->seed);

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
		BuildTranslationTables();
}

void SetPaletteRanges(int wall_range, int floor_range, int room_range, int alt_range)
{
	SetRange(WALL_COLORS, abs(wall_range) % COLORRANGE_COUNT);
	SetRange(FLOOR_COLORS, abs(floor_range) % COLORRANGE_COUNT);
	SetRange(ROOM_COLORS, abs(room_range) % COLORRANGE_COUNT);
	SetRange(ALT_COLORS, abs(alt_range) % COLORRANGE_COUNT);
}

int CheckMissionObjective(int flags)
{
	int index;

	if (TileItemIsObjective(flags)) {
		index = ObjectiveFromTileItem(flags);
		gMission.objectives[index].done++;
		return 1;
	}
	return 0;
}

int MissionCompleted(void)
{
	int i;

	if (gCampaign.mode == CAMPAIGN_MODE_DOGFIGHT)
	{
		return !(gPlayer1 && gPlayer2);
	}

	for (i = 0; i < gMission.missionData->objectiveCount; i++)
		if (gMission.objectives[i].done <
		    gMission.objectives[i].required)
			return 0;

	if (gPlayer1 &&
	    (gPlayer1->tileItem.x < gMission.exitLeft ||
	     gPlayer1->tileItem.x > gMission.exitRight ||
	     gPlayer1->tileItem.y < gMission.exitTop ||
	     gPlayer1->tileItem.y > gMission.exitBottom))
		return 0;

	if (gPlayer2 &&
	    (gPlayer2->tileItem.x < gMission.exitLeft ||
	     gPlayer2->tileItem.x > gMission.exitRight ||
	     gPlayer2->tileItem.y < gMission.exitTop ||
	     gPlayer2->tileItem.y > gMission.exitBottom))
		return 0;

	if (gPrisoner &&
	    (gPrisoner->tileItem.x < gMission.exitLeft ||
	     gPrisoner->tileItem.x > gMission.exitRight ||
	     gPrisoner->tileItem.y < gMission.exitTop ||
	     gPrisoner->tileItem.y > gMission.exitBottom))
		return 0;

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

const char *RangeName(int index)
{
	if (index >= 0 && index < (int)COLORRANGE_COUNT)
		return cColorRanges[index].name;
	else
		return "Invalid";
}
