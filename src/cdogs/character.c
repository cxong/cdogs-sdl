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
#include "files.h"
#include "json_utils.h"

#define CHARACTER_VERSION 12


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

void CharacterLoadJSON(CharacterStore *c, json_t *root, int version)
{
	LoadInt(&version, root, "Version");
	json_t *child = json_find_first_label(root, "Characters")->child->child;
	CharacterStoreTerminate(c);
	CharacterStoreInit(c);
	while (child)
	{
		Character *ch = CharacterStoreAddOther(c);
		char *tmp;
		if (version < 7)
		{
			// Old version stored character looks as palette indices
			int face;
			LoadInt(&face, child, "face");
			ch->Class = IntCharacterClass(face);
			int skin, arm, body, leg, hair;
			LoadInt(&skin, child, "skin");
			LoadInt(&arm, child, "arm");
			LoadInt(&body, child, "body");
			LoadInt(&leg, child, "leg");
			LoadInt(&hair, child, "hair");
			ConvertCharacterColors(skin, arm, body, leg, hair, &ch->Colors);
		}
		else
		{
			tmp = GetString(child, "Class");
			ch->Class = StrCharacterClass(tmp);
			CFREE(tmp);
			LoadColor(&ch->Colors.Skin, child, "Skin");
			LoadColor(&ch->Colors.Arms, child, "Arms");
			LoadColor(&ch->Colors.Body, child, "Body");
			LoadColor(&ch->Colors.Legs, child, "Legs");
			LoadColor(&ch->Colors.Hair, child, "Hair");
		}
		// Hair colour correction; some characters had no hair but now with
		// specific parts of the head colourised using the hair colour; set
		// default "hair" colour based on the head type
		if (version < 12)
		{
			const color_t darkRed = {0xC0, 0, 0, 0xFF};
			if (strcmp(ch->Class->Name, "Cyborg") == 0)
			{
				// eye
				ch->Colors.Hair = colorRed;
			}
			else if (strcmp(ch->Class->Name, "Ice") == 0)
			{
				// shades
				ch->Colors.Hair = colorBlack;
			}
			else if (strcmp(ch->Class->Name, "Ogre") == 0)
			{
				// eyes
				ch->Colors.Hair = darkRed;
			}
			else if (strcmp(ch->Class->Name, "Snake") == 0)
			{
				// eyepatch
				ch->Colors.Hair = colorBlack;
			}
			else if (strcmp(ch->Class->Name, "WarBaby") == 0)
			{
				// beret
				ch->Colors.Hair = colorRed;
			}
		}
		LoadInt(&ch->speed, child, "speed");
		tmp = GetString(child, "Gun");
		ch->Gun = StrGunDescription(tmp);
		CFREE(tmp);
		LoadInt(&ch->maxHealth, child, "maxHealth");
		int flags;
		LoadInt(&flags, child, "flags");
		ch->flags = flags;
		LoadInt(&ch->bot->probabilityToMove, child, "probabilityToMove");
		LoadInt(&ch->bot->probabilityToTrack, child, "probabilityToTrack");
		LoadInt(&ch->bot->probabilityToShoot, child, "probabilityToShoot");
		LoadInt(&ch->bot->actionDelay, child, "actionDelay");
		child = child->next;
	}
}

bool CharacterSave(CharacterStore *s, const char *path)
{
	json_t *root = json_new_object();
	AddIntPair(root, "Version", CHARACTER_VERSION);
	bool res = true;
	
	json_t *charNode = json_new_array();
	CA_FOREACH(Character, c, s->OtherChars)
		json_t *node = json_new_object();
		AddStringPair(node, "Class", c->Class->Name);
		AddColorPair(node, "Skin", c->Colors.Skin);
		AddColorPair(node, "Arms", c->Colors.Arms);
		AddColorPair(node, "Body", c->Colors.Body);
		AddColorPair(node, "Legs", c->Colors.Legs);
		AddColorPair(node, "Hair", c->Colors.Hair);
		AddIntPair(node, "speed", c->speed);
		json_insert_pair_into_object(
			node, "Gun", json_new_string(c->Gun->name));
		AddIntPair(node, "maxHealth", c->maxHealth);
		AddIntPair(node, "flags", c->flags);
		AddIntPair(node, "probabilityToMove", c->bot->probabilityToMove);
		AddIntPair(node, "probabilityToTrack", c->bot->probabilityToTrack);
		AddIntPair(node, "probabilityToShoot", c->bot->probabilityToShoot);
		AddIntPair(node, "actionDelay", c->bot->actionDelay);
		json_insert_child(charNode, node);
	CA_FOREACH_END()
	json_insert_pair_into_object(root, "Characters", charNode);
	char buf[CDOGS_PATH_MAX];
	sprintf(buf, "%s/characters.json", path);
	if (!TrySaveJSONFile(root, buf))
	{
		res = false;
		goto bail;
	}

bail:
	json_free_value(&root);
	return res;
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
