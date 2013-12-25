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
#ifndef __GAMEDATA
#define __GAMEDATA

#include "campaigns.h"
#include "character.h"
#include "input.h"
#include "pics.h"
#include "tile.h"
#include "weapon.h"
#include "sys_config.h"

#define MAX_WEAPONS 3


struct PlayerData
{
	char name[20];
	CharLooks looks;
	int weaponCount;
	gun_e weapons[MAX_WEAPONS];

	int score;
	int totalScore;
	int survived;
	int hp;
	int missions;
	int lastMission;
	int allTime, today;
	int kills;
	int friendlies;

	input_device_e inputDevice;
	int deviceIndex;
	int playerIndex;
};

extern struct PlayerData gPlayerDatas[MAX_PLAYERS];

struct GameOptions {
	int numPlayers;
	int badGuys;
};

// Properties of map objects
#define MAPOBJ_EXPLOSIVE    (1 << 0)
#define MAPOBJ_IMPASSABLE   (1 << 1)
#define MAPOBJ_CANBESHOT    (1 << 2)
#define MAPOBJ_CANBETAKEN   (1 << 3)
#define MAPOBJ_ROOMONLY     (1 << 4)
#define MAPOBJ_NOTINROOM    (1 << 5)
#define MAPOBJ_FREEINFRONT  (1 << 6)
#define MAPOBJ_ONEWALL      (1 << 7)
#define MAPOBJ_ONEWALLPLUS  (1 << 8)
#define MAPOBJ_NOWALLS      (1 << 9)
#define MAPOBJ_HIDEINSIDE   (1 << 10)
#define MAPOBJ_INTERIOR     (1 << 11)
#define MAPOBJ_FLAMMABLE    (1 << 12)
#define MAPOBJ_POISONOUS    (1 << 13)
#define MAPOBJ_QUAKE        (1 << 14)
#define MAPOBJ_ON_WALL      (1 << 15)

#define MAPOBJ_OUTSIDE (MAPOBJ_IMPASSABLE | MAPOBJ_CANBESHOT | \
                        MAPOBJ_NOTINROOM | MAPOBJ_ONEWALL)
#define MAPOBJ_INOPEN (MAPOBJ_IMPASSABLE | MAPOBJ_CANBESHOT | \
                        MAPOBJ_NOTINROOM | MAPOBJ_NOWALLS)
#define MAPOBJ_INSIDE (MAPOBJ_IMPASSABLE | MAPOBJ_CANBESHOT | MAPOBJ_ROOMONLY)

// A static map object, taking up an entire tile
struct MapObject
{
	int pic, wreckedPic;
	int width, height;
	int structure;
	int flags;
};
typedef struct MapObject TMapObject;


#define BADGUYS_OGRES    0
#define BADGUYS_BEMS     1

#define BADGUY_COMMANDER    1


struct DoorPic {
	int horzPic;
	int vertPic;
};


typedef enum {
	OBJECTIVE_KILL,
	OBJECTIVE_COLLECT,
	OBJECTIVE_DESTROY,
	OBJECTIVE_RESCUE,
	OBJECTIVE_INVESTIGATE,
	OBJECTIVE_MAX
} ObjectiveType;

#define OBJECTIVE_HIDDEN        1
#define OBJECTIVE_POSKNOWN      2
#define OBJECTIVE_HIACCESS      4
#define OBJECTIVE_UNKNOWNCOUNT	8
#define OBJECTIVE_NOACCESS		16

// WARNING: written as-is to file
#ifdef _MSC_VER
#pragma pack(push, 1)
#endif
struct MissionObjective {
	char description[60];
	int32_t type;
	int32_t index;
	int32_t count;
	int32_t required;
	int32_t flags;
}
#ifndef _MSC_VER
__attribute__((packed))
#endif
;
#ifdef _MSC_VER
#pragma pack(pop)
#endif


// WARNING: affects file format
#define BADDIE_MAX  12
#define SPECIAL_MAX 6
#define ITEMS_MAX   16

#define WEAPON_MAX  11


// WARNING: written as-is to file
#ifdef _MSC_VER
#pragma pack(push, 1)
#endif
struct Mission {
	char title[60];
	char description[400];
	int32_t wallStyle;
	int32_t floorStyle;
	int32_t roomStyle;
	int32_t exitStyle;
	int32_t keyStyle;
	int32_t doorStyle;

	int32_t mapWidth, mapHeight;
	int32_t wallCount, wallLength;
	int32_t roomCount;
	int32_t squareCount;

	int32_t exitLeft, exitTop, exitRight, exitBottom;

	int32_t objectiveCount;
	struct MissionObjective objectives[OBJECTIVE_MAX];

	int32_t baddieCount;
	int32_t baddies[BADDIE_MAX];
	int32_t specialCount;
	int32_t specials[SPECIAL_MAX];
	int32_t itemCount;
	int32_t items[ITEMS_MAX];
	int32_t itemDensity[ITEMS_MAX];

	int32_t baddieDensity;
	int32_t weaponSelection;

	char song[80];
	char map[80];

	int32_t wallRange;
	int32_t floorRange;
	int32_t roomRange;
	int32_t altRange;
}
#ifndef _MSC_VER
__attribute__((packed))
#endif
;
#ifdef _MSC_VER
#pragma pack(pop)
#endif


struct Objective
{
	color_t color;
	int count;
	int done;
	int required;
	TMapObject *blowupObject;
	int pickupItem;
};

struct MissionOptions {
	int index;
	int flags;

	struct Mission *missionData;
	struct Objective objectives[OBJECTIVE_MAX];
	int exitLeft, exitTop, exitRight, exitBottom;
	int pickupTime;

	int objectCount;
	TMapObject *mapObjects[ITEMS_MAX];
	int *keyPics;
	struct DoorPic *doorPics;
	int exitPic, exitShadow;

	int weaponCount;
	gun_e availableWeapons[WEAPON_MAX];
};

extern struct GameOptions gOptions;
extern struct MissionOptions gMission;

struct SongDef {
	char path[255];
	struct SongDef *next;
};

extern struct SongDef *gGameSongs;
extern struct SongDef *gMenuSongs;

void AddSong(struct SongDef **songList, const char *path);
void ShiftSongs(struct SongDef **songList);
void FreeSongs(struct SongDef **songList);
void LoadSongs(void);

void PlayerDataInitialize(void);

int IsIntroNeeded(campaign_mode_e mode);
int IsScoreNeeded(campaign_mode_e mode);
int HasObjectives(campaign_mode_e mode);
int IsAutoMapEnabled(campaign_mode_e mode);
int IsPasswordAllowed(campaign_mode_e mode);
int IsMissionBriefingNeeded(campaign_mode_e mode);
int AreKeysAllowed(campaign_mode_e mode);

int IsTileInExit(TTileItem *tile, struct MissionOptions *options);

int GameIsMouseUsed(struct PlayerData playerDatas[MAX_PLAYERS]);

#endif
