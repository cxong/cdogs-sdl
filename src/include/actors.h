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

 actors.h - <description here>

*/

#include "map.h"
#include "grafx.h"


#ifndef __ACTORS
#define __ACTORS


#define CHARACTER_PLAYER1                 0
#define CHARACTER_PLAYER2                 1
#define CHARACTER_PRISONER                2
#define CHARACTER_OTHERS                  3
#define CHARACTER_COUNT                  21

#define GUN_KNIFE           0
#define GUN_MG              1
#define GUN_GRENADE         2
#define GUN_FLAMER          3
#define GUN_SHOTGUN         4
#define GUN_POWERGUN        5
#define GUN_FRAGGRENADE     6
#define GUN_MOLOTOV         7
#define GUN_SNIPER          8
#define GUN_MINE            9
#define GUN_DYNAMITE        10
#define GUN_GASBOMB         11
#define GUN_PETRIFY         12
#define GUN_BROWN           13
#define GUN_CONFUSEBOMB     14
#define GUN_GASGUN          15
#define GUN_COUNT           16

#define FLAGS_PLAYER1       (1 << 0)
#define FLAGS_PLAYER2       (1 << 1)
#define FLAGS_PLAYERS       (FLAGS_PLAYER1 | FLAGS_PLAYER2)
#define FLAGS_HURTALWAYS    (1 << 2)

// Players only
#define FLAGS_SPECIAL_USED  (1 << 4)

// Computer characters only
#define FLAGS_DETOURING     (1 << 5)
#define FLAGS_TRYRIGHT      (1 << 6)
#define FLAGS_SLEEPING      (1 << 7)
#define FLAGS_VISIBLE       (1 << 8)
#define FLAGS_ASBESTOS      (1 << 9)
#define FLAGS_IMMUNITY      (1 << 10)
#define FLAGS_SEETHROUGH    (1 << 11)

// All characters - but only set for players
#define FLAGS_KEYCARD_RED     (1 << 12)
#define FLAGS_KEYCARD_BLUE    (1 << 13)
#define FLAGS_KEYCARD_GREEN   (1 << 14)
#define FLAGS_KEYCARD_YELLOW  (1 << 15)

// Special flags
#define FLAGS_RUNS_AWAY       (1 << 16)
#define FLAGS_GOOD_GUY        (1 << 17)
#define FLAGS_PRISONER        (1 << 18)
#define FLAGS_INVULNERABLE    (1 << 19)
#define FLAGS_FOLLOWER        (1 << 20)
#define FLAGS_PENALTY         (1 << 21)
#define FLAGS_VICTIM          (1 << 22)
#define FLAGS_SNEAKY          (1 << 23)
#define FLAGS_SLEEPALWAYS     (1 << 24)
#define FLAGS_AWAKEALWAYS     (1 << 25)


// Color range defines
#define SKIN_START 2
#define SKIN_END   9
#define BODY_START 52
#define BODY_END   61
#define ARMS_START 68
#define ARMS_END   77
#define LEGS_START 84
#define LEGS_END   93
#define HAIR_START 132
#define HAIR_END   135

#define SHADE_BLUE          0
#define SHADE_SKIN          1
#define SHADE_BROWN         2
#define SHADE_GREEN         3
#define SHADE_YELLOW        4
#define SHADE_PURPLE        5
#define SHADE_RED           6
#define SHADE_LTGRAY        7
#define SHADE_GRAY          8
#define SHADE_DKGRAY        9
#define SHADE_ASIANSKIN     10
#define SHADE_DARKSKIN      11
#define SHADE_BLACK         12
#define SHADE_GOLDEN        13
#define SHADE_COUNT         14


struct CharacterDescription {
	int armedBodyPic;
	int unarmedBodyPic;
	int facePic;
	int speed;
	int probabilityToMove;
	int probabilityToTrack;
	int probabilityToShoot;
	int actionDelay;
	TranslationTable table;
	int defaultGun;
	int maxHealth;
	int flags;
};

struct GunDescription {
	int gunPic;
	char *gunName;
};

struct Actor {
	int x, y;		// These are the full coordinates, including fractions
	int direction;
	int state;
	int stateCounter;
	int lastCmd;
	int gunLock;
	int sndLock;
	int character;
	int gun;
	int dx, dy;

	int health;
	int dead;
	int flamed;
	int poisoned;
	int petrified;
	int confused;
	int flags;

	int turns;
	int delay;

	TTileItem tileItem;
	struct Actor *next;
};
typedef struct Actor TActor;


extern TActor *gPlayer1;
extern TActor *gPlayer2;
extern TActor *gPrisoner;

extern struct CharacterDescription characterDesc[CHARACTER_COUNT];
extern struct GunDescription gunDesc[GUN_COUNT];

extern TranslationTable tableFlamed;
extern TranslationTable tableGreen;
extern TranslationTable tablePoison;
extern TranslationTable tableGray;
extern TranslationTable tableBlack;
extern TranslationTable tableDarker;
extern TranslationTable tablePurple;


void SetCharacter(int index, int face, int skin, int hair, int body,
		  int arms, int legs);
void SetCharacterColors(TranslationTable * t, int arms, int body, int legs,
			int skin, int hair);
void DrawCharacter(int x, int y, TActor * actor);

void SetStateForActor(TActor * actor, int state);
void UpdateActorState(TActor * actor, int ticks);
int MoveActor(TActor * actor, int x, int y);
void CommandActor(TActor * actor, int cmd);
void SlideActor(TActor * actor, int cmd);
TActor *AddActor(int character);
void UpdateAllActors(int ticks);
TActor *ActorList(void);
void BuildTranslationTables(void);
void InitializeTranslationTables(void);
void Score(int flags, int points);
void InjureActor(TActor * actor, int injury);
void KillAllActors(void);

#endif
