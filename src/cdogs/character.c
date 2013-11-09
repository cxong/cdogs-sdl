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
#include "character.h"

#include <assert.h>

#include "actors.h"

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

typedef unsigned char ColorShade[10];

static ColorShade colorShades[SHADE_COUNT] =
{
	{52, 53, 54, 55, 56, 57, 58, 59, 60, 61},
	{2, 3, 4, 5, 6, 7, 8, 9, 9, 9},
	{68, 69, 70, 71, 72, 73, 74, 75, 76, 77},
	{84, 85, 86, 87, 88, 89, 90, 91, 92, 93},
	{100, 101, 102, 103, 104, 105, 106, 107, 107, 107},
	{116, 117, 118, 119, 120, 121, 122, 123, 124, 125},
	{132, 133, 134, 135, 136, 137, 138, 139, 140, 141},
	{32, 33, 34, 35, 36, 37, 38, 39, 40, 41},
	{36, 37, 38, 39, 40, 41, 42, 43, 44, 45},
	{41, 42, 43, 44, 45, 46, 47, 47, 47, 47},
	{144, 145, 146, 147, 148, 149, 150, 151, 151, 151},
	{4, 5, 6, 7, 8, 9, 9, 9, 9, 9},
	{1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
	{16, 17, 18, 19, 20, 21, 22, 23, 24, 25}
};


void SetShade(TranslationTable * table, int start, int end, int shade)
{
	int i;

	for (i = start; i <= end; i++)
		(*table)[i] = colorShades[shade][i - start];
}

void SetCharacterColors(TranslationTable *t, CharLooks looks)
{
	int f;
	for (f = 0; f < 256; f++)
	{
		(*t)[f] = f & 0xFF;
	}
	SetShade(t, BODY_START, BODY_END, looks.body);
	SetShade(t, ARMS_START, ARMS_END, looks.arm);
	SetShade(t, LEGS_START, LEGS_END, looks.leg);
	SetShade(t, SKIN_START, SKIN_END, looks.skin);
	SetShade(t, HAIR_START, HAIR_END, looks.hair);
}

void CharacterSetLooks(Character *c, const CharLooks *l)
{
	c->looks = *l;
	SetCharacterColors(&c->table, c->looks);
}


void CharacterStoreInit(CharacterStore *store)
{
	memset(store, 0, sizeof *store);
	store->playerCount = MAX_PLAYERS;
}

void CharacterStoreTerminate(CharacterStore *store)
{
	CFREE(store->prisoners);
	CFREE(store->baddies);
	CFREE(store->specials);
	memset(store, 0, sizeof *store);
}

void CharacterStoreResetOthers(CharacterStore *store)
{
	CFREE(store->prisoners);
	store->prisoners = NULL;
	store->prisonerCount = 0;
	CFREE(store->baddies);
	store->baddies = NULL;
	store->baddieCount = 0;
	CFREE(store->specials);
	store->specials = NULL;
	store->specialCount = 0;
}

Character *CharacterStoreAddOther(CharacterStore *store)
{
	return CharacterStoreInsertOther(store, store->otherCount);
}
Character *CharacterStoreInsertOther(CharacterStore *store, int idx)
{
	int i;
	for (i = store->otherCount; i > idx; i--)
	{
		memcpy(
			&store->others[i], &store->others[i - 1], sizeof store->others[i]);
	}
	store->otherCount++;
	return &store->others[idx];
}
void CharacterStoreDeleteOther(CharacterStore *store, int idx)
{
	int i;
	if (store->otherCount == 0)
	{
		return;
	}
	store->otherCount--;
	memset(&store->others[idx], 0, sizeof store->others[idx]);
	for (i = idx; i < store->otherCount; i++)
	{
		memcpy(
			&store->others[i],
			&store->others[i + 1],
			sizeof store->others[i]);
	}
}

void CharacterStoreAddPrisoner(CharacterStore *store, int character)
{
	store->prisonerCount++;
	CREALLOC(store->prisoners, store->prisonerCount * sizeof *store->prisoners);
	store->prisoners[store->prisonerCount - 1] = &store->others[character];
}

void CharacterStoreAddBaddie(CharacterStore *store, int character)
{
	store->baddieCount++;
	CREALLOC(store->baddies, store->baddieCount * sizeof *store->baddies);
	store->baddies[store->baddieCount - 1] = &store->others[character];
}
void CharacterStoreAddSpecial(CharacterStore *store, int character)
{
	store->specialCount++;
	CREALLOC(store->specials, store->specialCount * sizeof *store->specials);
	store->specials[store->specialCount - 1] = &store->others[character];
}

Character *CharacterStoreGetPrisoner(CharacterStore *store, int i)
{
	return store->prisoners[i];
}

Character *CharacterStoreGetSpecial(CharacterStore *store, int i)
{
	return store->specials[i];
}
Character *CharacterStoreGetRandomBaddie(CharacterStore *store)
{
	return store->baddies[rand() % store->baddieCount];
}
Character *CharacterStoreGetRandomSpecial(CharacterStore *store)
{
	return store->specials[rand() % store->specialCount];
}
