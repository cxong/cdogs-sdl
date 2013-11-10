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
#ifndef __CHARACTER
#define __CHARACTER

#include "weapon.h"

typedef struct
{
	int armedBody;
	int unarmedBody;
	int face;
	int skin;
	int arm;
	int body;
	int leg;
	int hair;
} CharLooks;

typedef struct
{
	int probabilityToMove;
	int probabilityToTrack;
	int probabilityToShoot;
	int actionDelay;
} CharBot;

typedef struct
{
	CharLooks looks;
	int speed;
	gun_e gun;
	int maxHealth;
	int flags;
	TranslationTable table;
	CharBot bot;
} Character;

#define MAX_PLAYERS 4
#define CHARACTER_OTHER_COUNT 64
#define CHARACTER_PLAYER1 0
#define CHARACTER_PLAYER2 1
typedef struct
{
	int playerCount;
	Character players[MAX_PLAYERS];	// human players
	int otherCount;
	Character others[CHARACTER_OTHER_COUNT];	// both normal baddies and special chars

	// References only
	int prisonerCount;
	Character **prisoners;
	int baddieCount;
	Character **baddies;
	int specialCount;
	Character **specials;
} CharacterStore;

void CharacterSetLooks(Character *c, const CharLooks *l);

void CharacterStoreInit(CharacterStore *store);
void CharacterStoreTerminate(CharacterStore *store);
void CharacterStoreResetOthers(CharacterStore *store);
Character *CharacterStoreAddOther(CharacterStore *store);
Character *CharacterStoreInsertOther(CharacterStore *store, int idx);
void CharacterStoreDeleteOther(CharacterStore *store, int idx);
void CharacterStoreAddPrisoner(CharacterStore *store, int character);
void CharacterStoreAddBaddie(CharacterStore *store, int character);
void CharacterStoreAddSpecial(CharacterStore *store, int character);
void CharacterStoreDeleteBaddie(CharacterStore *store, int idx);
void CharacterStoreDeleteSpecial(CharacterStore *store, int idx);
Character *CharacterStoreGetPrisoner(CharacterStore *store, int i);
Character *CharacterStoreGetSpecial(CharacterStore *store, int i);
Character *CharacterStoreGetRandomBaddie(CharacterStore *store);
Character *CharacterStoreGetRandomSpecial(CharacterStore *store);

#endif
