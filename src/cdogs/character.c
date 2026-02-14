/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.

	Copyright (c) 2013-2014, 2016, 2019-2021, 2023-2026 Cong Xu
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
#include "log.h"
#include "player_template.h"
#include "yajl_utils.h"

#define CHARACTER_VERSION 14

#define YAJL_CHECK(func)                                                      \
	if (func != yajl_gen_status_ok)                                           \
	{                                                                         \
		LOG(LM_MAIN, LL_ERROR, "JSON generator error for character\n");       \
		res = false;                                                          \
		goto bail;                                                            \
	}

static void CharacterInit(Character *c)
{
	memset(c, 0, sizeof *c);
	CCALLOC(c->bot, sizeof *c->bot);
}
static void CharacterTerminate(Character *c)
{
	for (HeadPart hp = HEAD_PART_HAIR; hp < HEAD_PART_COUNT; hp++)
	{
		CFREE(c->HeadParts[hp]);
	}
	CFREE(c->bot);
}

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
	CharacterTerminate(c);
	CA_FOREACH_END()
	CArrayTerminate(&store->OtherChars);
	CArrayTerminate(&store->prisonerIds);
	CArrayTerminate(&store->baddieIds);
	CArrayTerminate(&store->specialIds);
	memset(store, 0, sizeof *store);
}

void CharacterStoreCopy(
	CharacterStore *dst, const CharacterStore *src, CArray *playerTemplates)
{
	CharacterStoreTerminate(dst);
	PlayerTemplatesClear(playerTemplates);
	CharacterStoreInit(dst);
	CArrayCopy(&dst->OtherChars, &src->OtherChars);
	CA_FOREACH(Character, c, dst->OtherChars)
	CharacterCopy(c, CArrayGet(&src->OtherChars, _ca_index), playerTemplates);
	CA_FOREACH_END()
	CArrayCopy(&dst->prisonerIds, &src->prisonerIds);
	CArrayCopy(&dst->baddieIds, &src->baddieIds);
	CArrayCopy(&dst->specialIds, &src->specialIds);
}

void CharacterStoreResetOthers(CharacterStore *store)
{
	CArrayClear(&store->prisonerIds);
	CArrayClear(&store->baddieIds);
	CArrayClear(&store->specialIds);
}

void CharacterLoadJSON(
	CharacterStore *c, CArray *playerTemplates, json_t *root, int version)
{
	LoadInt(&version, root, "Version");
	json_t *child = json_find_first_label(root, "Characters")->child->child;
	CharacterStoreTerminate(c);
	PlayerTemplatesClear(playerTemplates);
	CharacterStoreInit(c);
	while (child)
	{
		Character *ch = CharacterStoreAddOther(c);
		char *tmp;

		// Face
		if (version < 13)
		{
			// Old face names, before face + hair split
			if (version < 7)
			{
				int face;
				LoadInt(&face, child, "face");
				CSTRDUP(tmp, IntCharacterFace(face));
			}
			else
			{
				tmp = GetString(child, "Class");
			}
			char *face = NULL;
			CharacterOldFaceToHeadParts(tmp, &face, ch->HeadParts);
			CFREE(tmp);
			ch->Class = StrCharacterClass(face);
			CFREE(face);
		}
		else
		{
			tmp = GetString(child, "Class");
			ch->Class = StrCharacterClass(tmp);
			CFREE(tmp);
			tmp = NULL;
			LoadStr(&ch->HeadParts[HEAD_PART_HAIR], child, "HairType");
			if (version < 14)
			{
				CharacterOldHairToHeadParts(ch->HeadParts);
			}
			else
			{
				LoadStr(
					&ch->HeadParts[HEAD_PART_FACEHAIR], child, "FacehairType");
				LoadStr(&ch->HeadParts[HEAD_PART_HAT], child, "HatType");
				LoadStr(
					&ch->HeadParts[HEAD_PART_GLASSES], child, "GlassesType");
			}
		}
		CASSERT(ch->Class != NULL, "Cannot load character class");

		// Colours
		if (version < 7)
		{
			// Old version stored character looks as palette indices
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
			LoadColor(&ch->Colors.Skin, child, "Skin");
			LoadColor(&ch->Colors.Arms, child, "Arms");
			LoadColor(&ch->Colors.Body, child, "Body");
			LoadColor(&ch->Colors.Legs, child, "Legs");
			LoadColor(&ch->Colors.Hair, child, "Hair");
		}
		ch->Colors.Feet = ch->Colors.Legs;
		if (version < 13)
		{
			ConvertHairColors(ch, ch->Class->Name);
		}
		else
		{
			LoadColor(&ch->Colors.Feet, child, "Feet");
		}
		if (version < 14)
		{
			ch->Colors.Facehair = ch->Colors.Hair;
			ch->Colors.Hat = ch->Colors.Hair;
			ch->Colors.Glasses = ch->Colors.Hair;
		}
		else
		{
			LoadColor(&ch->Colors.Facehair, child, "Facehair");
			LoadColor(&ch->Colors.Hat, child, "Hat");
			LoadColor(&ch->Colors.Glasses, child, "Glasses");
		}

		LoadStr(&ch->PlayerTemplateName, child, "PlayerTemplateName");
		LoadFullInt(&ch->speed, child, "speed");
		tmp = GetString(child, "Gun");
		ch->Gun = StrWeaponClass(tmp);
		CFREE(tmp);
		tmp = NULL;
		LoadStr(&tmp, child, "Melee");
		if (tmp != NULL)
		{
			ch->Melee = StrWeaponClass(tmp);
			CFREE(tmp);
		}
		LoadInt(&ch->maxHealth, child, "maxHealth");
		ch->excessHealth = ch->maxHealth * 2;
		LoadInt(&ch->excessHealth, child, "excessHealth");
		int flags;
		LoadInt(&flags, child, "flags");
		ch->flags = flags;

		tmp = NULL;
		LoadStr(&tmp, child, "Drop");
		if (tmp != NULL)
		{
			ch->Drop = StrPickupClass(tmp);
			CFREE(tmp);
		}

		LoadInt(&ch->bot->probabilityToMove, child, "probabilityToMove");
		LoadInt(&ch->bot->probabilityToTrack, child, "probabilityToTrack");
		LoadInt(&ch->bot->probabilityToShoot, child, "probabilityToShoot");
		LoadInt(&ch->bot->actionDelay, child, "actionDelay");

		if (ch->PlayerTemplateName != NULL)
		{
			PlayerTemplateAddCharacter(playerTemplates, ch);
		}

		child = child->next;
	}
}

bool CharacterStoreSave(CharacterStore *s, const char *path)
{
	bool res = true;
	yajl_gen g = yajl_gen_alloc(NULL);
	if (g == NULL)
	{
		LOG(LM_MAIN, LL_ERROR,
			"Unable to alloc JSON generator for saving character\n");
		res = false;
		goto bail;
	}

	YAJL_CHECK(yajl_gen_map_open(g));
	YAJL_CHECK(YAJLAddIntPair(g, "Version", CHARACTER_VERSION));

	YAJL_CHECK(yajl_gen_string(
		g, (const unsigned char *)"Characters", strlen("Characters")));
	YAJL_CHECK(yajl_gen_array_open(g));
	CA_FOREACH(Character, c, s->OtherChars)
	if (!CharacterSave(g, c))
	{
		res = false;
		goto bail;
	}
	CA_FOREACH_END()
	YAJL_CHECK(yajl_gen_array_close(g));

	YAJL_CHECK(yajl_gen_map_close(g));
	char buf[CDOGS_PATH_MAX];
	sprintf(buf, "%s/characters.json", path);
	if (!YAJLTrySaveJSONFile(g, buf))
	{
		res = false;
		goto bail;
	}

bail:
	if (g)
	{
		yajl_gen_clear(g);
		yajl_gen_free(g);
	}
	return res;
}

Character *CharacterStoreAddOther(CharacterStore *store)
{
	return CharacterStoreInsertOther(store, store->OtherChars.size);
}
Character *CharacterStoreInsertOther(CharacterStore *store, const size_t idx)
{
	Character newChar;
	CharacterInit(&newChar);
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
	const int idx = RAND_INT(0, store->baddieIds.size);
	return *(int *)CArrayGet(&store->baddieIds, idx);
}
int CharacterStoreGetRandomSpecialId(const CharacterStore *store)
{
	const int idx = RAND_INT(0, store->specialIds.size);
	return *(int *)CArrayGet(&store->specialIds, idx);
}

void CharacterCopy(
	Character *dst, const Character *src, CArray *playerTemplates)
{
	memcpy(dst, src, sizeof *dst);
	const CharBot *cb = dst->bot;
	if (cb)
	{
		CMALLOC(dst->bot, sizeof *dst->bot);
		memcpy(dst->bot, cb, sizeof *cb);
	}
	for (HeadPart hp = HEAD_PART_HAIR; hp < HEAD_PART_COUNT; hp++)
	{
		if (dst->HeadParts[hp] != NULL)
		{
			char *hpCopy = NULL;
			CSTRDUP(hpCopy, dst->HeadParts[hp]);
			dst->HeadParts[hp] = hpCopy;
		}
	}
	if (dst->PlayerTemplateName != NULL && playerTemplates)
	{
		PlayerTemplateAddCharacter(playerTemplates, dst);
	}
}

bool CharacterSave(yajl_gen g, const Character *c)
{
	bool res = true;

	YAJL_CHECK(yajl_gen_map_open(g));
	YAJL_CHECK(YAJLAddStringPair(g, "Class", c->Class->Name));
	if (c->PlayerTemplateName)
	{
		YAJL_CHECK(
			YAJLAddStringPair(g, "PlayerTemplateName", c->PlayerTemplateName));
	}
	if (c->HeadParts[HEAD_PART_HAIR])
	{
		YAJL_CHECK(
			YAJLAddStringPair(g, "HairType", c->HeadParts[HEAD_PART_HAIR]));
	}
	if (c->HeadParts[HEAD_PART_FACEHAIR])
	{
		YAJL_CHECK(YAJLAddStringPair(
			g, "FacehairType", c->HeadParts[HEAD_PART_FACEHAIR]));
	}
	if (c->HeadParts[HEAD_PART_HAT])
	{
		YAJL_CHECK(
			YAJLAddStringPair(g, "HatType", c->HeadParts[HEAD_PART_HAT]));
	}
	if (c->HeadParts[HEAD_PART_GLASSES])
	{
		YAJL_CHECK(YAJLAddStringPair(
			g, "GlassesType", c->HeadParts[HEAD_PART_GLASSES]));
	}
	YAJL_CHECK(YAJLAddColorPair(g, "Skin", c->Colors.Skin));
	YAJL_CHECK(YAJLAddColorPair(g, "Arms", c->Colors.Arms));
	YAJL_CHECK(YAJLAddColorPair(g, "Body", c->Colors.Body));
	YAJL_CHECK(YAJLAddColorPair(g, "Legs", c->Colors.Legs));
	YAJL_CHECK(YAJLAddColorPair(g, "Hair", c->Colors.Hair));
	YAJL_CHECK(YAJLAddColorPair(g, "Feet", c->Colors.Feet));
	YAJL_CHECK(YAJLAddColorPair(g, "Facehair", c->Colors.Facehair));
	YAJL_CHECK(YAJLAddColorPair(g, "Hat", c->Colors.Hat));
	YAJL_CHECK(YAJLAddColorPair(g, "Glasses", c->Colors.Glasses));
	YAJL_CHECK(YAJLAddIntPair(g, "speed", (int)(c->speed * 256)));
	if (c->Gun)
	{
		YAJL_CHECK(YAJLAddStringPair(g, "Gun", c->Gun->name));
	}
	if (c->Melee != NULL)
	{
		YAJL_CHECK(YAJLAddStringPair(g, "Melee", c->Melee->name));
	}
	YAJL_CHECK(YAJLAddIntPair(g, "maxHealth", c->maxHealth));
	YAJL_CHECK(YAJLAddIntPair(g, "excessHealth", c->excessHealth));
	YAJL_CHECK(YAJLAddIntPair(g, "flags", c->flags));
	if (c->Drop != NULL)
	{
		YAJL_CHECK(YAJLAddStringPair(g, "Drop", c->Drop->Name));
	}
	if (c->bot)
	{
		YAJL_CHECK(
			YAJLAddIntPair(g, "probabilityToMove", c->bot->probabilityToMove));
		YAJL_CHECK(YAJLAddIntPair(
			g, "probabilityToTrack", c->bot->probabilityToTrack));
		YAJL_CHECK(YAJLAddIntPair(
			g, "probabilityToShoot", c->bot->probabilityToShoot));
		YAJL_CHECK(YAJLAddIntPair(g, "actionDelay", c->bot->actionDelay));
	}
	YAJL_CHECK(yajl_gen_map_close(g));

bail:
	return res;
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

void CharacterSetHeadPart(Character *c, const HeadPart hp, const char *name)
{
	CFREE(c->HeadParts[hp]);
	c->HeadParts[hp] = NULL;
	if (name)
	{
		CSTRDUP(c->HeadParts[hp], name);
	}
}

// Blacklist some secret hats
static const char *HAT_BLACKLIST[] = {"bunny",		"capotain", "horns",
									  "leprechaun", "party",	"santa",
									  "sombrero",	"witch"};

static color_t RandomColor(void);
void CharacterShuffleAppearance(Character *c)
{
	// Choose a random character class
	const int numCharClasses = (int)gCharacterClasses.Classes.size +
							   (int)gCharacterClasses.CustomClasses.size;
	const int charClass = rand() % numCharClasses;
	if (charClass < (int)gCharacterClasses.Classes.size)
	{
		c->Class = CArrayGet(&gCharacterClasses.Classes, charClass);
	}
	else
	{
		c->Class = CArrayGet(
			&gCharacterClasses.CustomClasses,
			charClass - gCharacterClasses.Classes.size);
	}

	for (HeadPart hp = HEAD_PART_HAIR; hp < HEAD_PART_COUNT; hp++)
	{
		const char *name = NULL;
		if (RAND_INT(0, 3) == 0)
		{
			const CArray *hpNames = &gPicManager.headPartNames[hp];
			name = *(char **)CArrayGet(hpNames, rand() % hpNames->size);
			if (hp == HEAD_PART_HAT)
			{
				for (size_t i = 0; i < sizeof(HAT_BLACKLIST) / sizeof(char *);
					 i++)
				{
					if (strcmp(name, HAT_BLACKLIST[i]) == 0)
					{
						name = NULL;
						break;
					}
				}
			}
		}
		CharacterSetHeadPart(c, hp, name);
	}

	c->Colors.Skin = RandomColor();
	c->Colors.Arms = RandomColor();
	c->Colors.Body = RandomColor();
	c->Colors.Legs = RandomColor();
	c->Colors.Hair = RandomColor();
	c->Colors.Feet = RandomColor();
	c->Colors.Facehair = c->Colors.Hair;
	c->Colors.Hat = RandomColor();
	c->Colors.Glasses = RandomColor();
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
