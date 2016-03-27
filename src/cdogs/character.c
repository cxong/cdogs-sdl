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
#include "character.h"

#include <assert.h>

#include "actors.h"


void CharacterStoreInit(CharacterStore *store)
{
	memset(store, 0, sizeof *store);
	CArrayInit(&store->OtherChars, sizeof(Character));
	CArrayInit(&store->prisonerIds, sizeof(int));
	CArrayInit(&store->baddieIds, sizeof(int));
	CArrayInit(&store->specialIds, sizeof(int));
}

void CharacterStoreTerminate(CharacterStore *store)
{
	CA_FOREACH(Character, c, store->OtherChars)
		CFREE(c->bot);
	CA_FOREACH_END()
	CArrayTerminate(&store->OtherChars);
	CArrayTerminate(&store->prisonerIds);
	CArrayTerminate(&store->baddieIds);
	CArrayTerminate(&store->specialIds);
	memset(store, 0, sizeof *store);
}

void CharacterStoreResetOthers(CharacterStore *store)
{
	CArrayClear(&store->prisonerIds);
	CArrayClear(&store->baddieIds);
	CArrayClear(&store->specialIds);
}

Character *CharacterStoreAddOther(CharacterStore *store)
{
	return CharacterStoreInsertOther(store, store->OtherChars.size);
}
Character *CharacterStoreInsertOther(CharacterStore *store, int idx)
{
	Character newChar;
	memset(&newChar, 0, sizeof newChar);
	CCALLOC(newChar.bot, sizeof *newChar.bot);
	CArrayInsert(&store->OtherChars, idx, &newChar);
	return CArrayGet(&store->OtherChars, idx);
}
void CharacterStoreDeleteOther(CharacterStore *store, int idx)
{
	CArrayDelete(&store->OtherChars, idx);
}

void CharacterStoreAddPrisoner(CharacterStore *store, int character)
{
	CArrayPushBack(&store->prisonerIds, &character);
}

void CharacterStoreAddBaddie(CharacterStore *store, int character)
{
	CArrayPushBack(&store->baddieIds, &character);
}
void CharacterStoreAddSpecial(CharacterStore *store, int character)
{
	CArrayPushBack(&store->specialIds, &character);
}
void CharacterStoreDeleteBaddie(CharacterStore *store, int idx)
{
	CArrayDelete(&store->baddieIds, idx);
}
void CharacterStoreDeleteSpecial(CharacterStore *store, int idx)
{
	CArrayDelete(&store->specialIds, idx);
}

int CharacterStoreGetPrisonerId(const CharacterStore *store, const int i)
{
	return *(int *)CArrayGet(&store->prisonerIds, i);
}

int CharacterStoreGetSpecialId(const CharacterStore *store, const int i)
{
	return *(int *)CArrayGet(&store->specialIds, i);
}
int CharacterStoreGetRandomBaddieId(const CharacterStore *store)
{
	return *(int *)CArrayGet(
		&store->baddieIds, rand() % store->baddieIds.size);
}
int CharacterStoreGetRandomSpecialId(const CharacterStore *store)
{
	return *(int *)CArrayGet(
		&store->specialIds, rand() % store->specialIds.size);
}

bool CharacterIsPrisoner(const CharacterStore *store, const Character *c)
{
	for (int i = 0; i < (int)store->prisonerIds.size; i++)
	{
		const Character *prisoner = CArrayGet(
			&store->OtherChars, CharacterStoreGetPrisonerId(store, i));
		if (prisoner == c)
		{
			return true;
		}
	}
	return false;
}

int CharacterGetStartingHealth(const Character *c, const bool isNPC)
{
	if (isNPC)
	{
		return MAX(
			(c->maxHealth * ConfigGetInt(&gConfig, "Game.NonPlayerHP")) / 100,
			1);
	}
	else
	{
		return c->maxHealth;
	}
}

static color_t RandomColor(void);
void CharacterShuffleAppearance(Character *c)
{
	// Choose a random character class
	const int numCharClasses =
		(int)gCharacterClasses.Classes.size +
		(int)gCharacterClasses.CustomClasses.size;
	const int charClass = rand() % numCharClasses;
	if (charClass < (int)gCharacterClasses.Classes.size)
	{
		c->Class = CArrayGet(&gCharacterClasses.Classes, charClass);
	}
	else
	{
		c->Class = CArrayGet(
			&gCharacterClasses.Classes,
			charClass - gCharacterClasses.Classes.size);
	}
	c->Colors.Skin = RandomColor();
	c->Colors.Arms = RandomColor();
	c->Colors.Body = RandomColor();
	c->Colors.Legs = RandomColor();
	c->Colors.Hair = RandomColor();
}
static color_t RandomColor(void)
{
	color_t c;
	c.r = rand() & 255;
	c.g = rand() & 255;
	c.b = rand() & 255;
	c.a = 255;
	return c;
}
