/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.

    Copyright (c) 2013-2014, 2016 Cong Xu
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

#include "c_array.h"
#include "character_class.h"
#include "weapon.h"

typedef struct
{
	int probabilityToMove;
	int probabilityToTrack;
	int probabilityToShoot;
	int actionDelay;
} CharBot;

typedef struct
{
	const CharacterClass *Class;
	int speed;
	const GunDescription *Gun;
	int maxHealth;
	unsigned int flags;
	CharColors Colors;
	CharBot *bot;
} Character;

typedef struct
{
	CArray OtherChars;	// of Character, both normal baddies and special chars

	// IDs only (of int)
	CArray prisonerIds;
	CArray baddieIds;
	CArray specialIds;
} CharacterStore;

void CharacterStoreInit(CharacterStore *store);
void CharacterStoreTerminate(CharacterStore *store);
void CharacterStoreResetOthers(CharacterStore *store);
void CharacterLoadJSON(CharacterStore *c, json_t *root, int version);
bool CharacterSave(CharacterStore *s, const char *path);
Character *CharacterStoreAddOther(CharacterStore *store);
Character *CharacterStoreInsertOther(CharacterStore *store, int idx);
void CharacterStoreDeleteOther(CharacterStore *store, int idx);
void CharacterStoreAddPrisoner(CharacterStore *store, int character);
void CharacterStoreAddBaddie(CharacterStore *store, int character);
void CharacterStoreAddSpecial(CharacterStore *store, int character);
void CharacterStoreDeleteBaddie(CharacterStore *store, int idx);
void CharacterStoreDeleteSpecial(CharacterStore *store, int idx);
int CharacterStoreGetPrisonerId(const CharacterStore *store, const int i);
int CharacterStoreGetSpecialId(const CharacterStore *store, const int i);
int CharacterStoreGetRandomBaddieId(const CharacterStore *store);
int CharacterStoreGetRandomSpecialId(const CharacterStore *store);

bool CharacterIsPrisoner(const CharacterStore *store, const Character *c);

int CharacterGetStartingHealth(const Character *c, const bool isNPC);
void CharacterShuffleAppearance(Character *c);
