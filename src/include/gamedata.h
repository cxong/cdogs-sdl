/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (C) 1995 Ronny Webster
    Copyright (C) 2003 Jeremy Chin 
    Copyright (C) 2003 Lucas Martin-King 

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

-------------------------------------------------------------------------------

 gamedata.h - <description here>

*/

#ifndef __GAMEDATA
#define MAX_WEAPONS 3


struct PlayerData {
	char name[20];
	int head;
	int arms, body, legs, skin, hair;
	int weaponCount;
	int weapons[MAX_WEAPONS];

	int score;
	int totalScore;
	int survived;
	int hp;
	int missions;
	int lastMission;
	int allTime, today;
	int kills;
	int friendlies;

	int controls;
	int keys[6];
};

extern struct PlayerData gPlayer1Data;
extern struct PlayerData gPlayer2Data;


#define SPLIT_X  300
#define SPLIT_Y  180

#define COPY_REPMOVSD    0
#define COPY_DEC_JNZ     1

#define DIFFICULTY_VERYEASY		1
#define DIFFICULTY_EASY			2
#define DIFFICULTY_NORMAL		3
#define DIFFICULTY_HARD			4
#define DIFFICULTY_VERYHARD		5

struct GameOptions {
	int twoPlayers;
	int badGuys;
	int xSplit;
	int ySplit;
	int displayFPS;
	int displayTime;
	int playersHurt;
	int displaySlices;
	int copyMode;
	int forceVSync;
	int brightness;
	int swapButtonsJoy1;
	int swapButtonsJoy2;
	int mapKey;
	int difficulty;
	int density;
	int npcHp;
	int playerHp;
	int slowmotion;
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

// A map object
struct MapObject {
	int pic, wreckedPic;
	int width, height;
	int structure;
	int flags;
};
typedef struct MapObject TMapObject;


#define BADGUYS_OGRES    0
#define BADGUYS_BEMS     1

#define BADGUY_COMMANDER    1


struct BadGuy {
	int armedBodyPic;
	int unarmedBodyPic;
	int facePic;
	int speed;
	int probabilityToMove;
	int probabilityToTrack;
	int probabilityToShoot;
	int actionDelay;
	int gun;
	int skinColor;
	int armColor;
	int bodyColor;
	int legColor;
	int hairColor;
	int health;
	int flags;
};
typedef struct BadGuy TBadGuy;

struct DoorPic {
	int horzPic;
	int vertPic;
};


#define OBJECTIVE_KILL          0
#define OBJECTIVE_COLLECT       1
#define OBJECTIVE_DESTROY       2
#define OBJECTIVE_RESCUE        3
#define OBJECTIVE_INVESTIGATE   4

#define OBJECTIVE_HIDDEN        1
#define OBJECTIVE_POSKNOWN      2
#define OBJECTIVE_HIACCESS      4
#define OBJECTIVE_UNKNOWNCOUNT	8
#define OBJECTIVE_NOACCESS		16

#define OBJECTIVE_MAX           5

struct MissionObjective {
	char description[60];
	int type;
	int index;
	int count;
	int required;
	int flags;
};


#define BADDIE_MAX  12
#define SPECIAL_MAX 6
#define ITEMS_MAX   16
#define WEAPON_MAX  11


struct Mission {
	char title[60];
	char description[400];
	int wallStyle;
	int floorStyle;
	int roomStyle;
	int exitStyle;
	int keyStyle;
	int doorStyle;

	int mapWidth, mapHeight;
	int wallCount, wallLength;
	int roomCount;
	int squareCount;

	int exitLeft, exitTop, exitRight, exitBottom;

	int objectiveCount;
	struct MissionObjective objectives[OBJECTIVE_MAX];

	int baddieCount;
	int baddies[BADDIE_MAX];
	int specialCount;
	int specials[SPECIAL_MAX];
	int itemCount;
	int items[ITEMS_MAX];
	int itemDensity[ITEMS_MAX];

	int baddieDensity;
	int weaponSelection;

	char song[80];
	char map[80];

	int wallRange;
	int floorRange;
	int roomRange;
	int altRange;
};


struct CampaignSetting {
	char title[40];
	char author[40];
	char description[200];
	int missionCount;
	struct Mission *missions;
	int characterCount;
	TBadGuy *characters;
};
typedef struct CampaignSetting TCampaignSetting;

struct CampaignOptions {
	TCampaignSetting *setting;
	int seed;
	int dogFight;
};


struct Objective {
	int color;
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
	int availableWeapons[WEAPON_MAX];
};

extern struct GameOptions gOptions;
extern struct CampaignOptions gCampaign;
extern struct MissionOptions gMission;

struct SongDef {
	char path[81];
	struct SongDef *next;
};

extern struct SongDef *gGameSongs;
extern struct SongDef *gMenuSongs;

void AddSong(struct SongDef **songList, const char *path);
void ShiftSongs(struct SongDef **songList);
void FreeSongs(struct SongDef **songList);
void LoadSongs(const char *path, struct SongDef **songList);

#define __GAMEDATA
#endif
