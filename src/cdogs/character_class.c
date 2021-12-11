/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.

	Copyright (c) 2016-2017, 2019-2021 Cong Xu
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
#include "character_class.h"

#include "json_utils.h"
#include "log.h"

#define VERSION 2
#define FOOTSTEP_DISTANCE_PLUS 250

CharacterClasses gCharacterClasses;

// TODO: use map structure?
const CharacterClass *StrCharacterClass(const char *s)
{
	CA_FOREACH(const CharacterClass, c, gCharacterClasses.CustomClasses)
	if (strcmp(s, c->Name) == 0)
	{
		return c;
	}
	CA_FOREACH_END()
	CA_FOREACH(const CharacterClass, c, gCharacterClasses.Classes)
	if (strcmp(s, c->Name) == 0)
	{
		return c;
	}
	CA_FOREACH_END()
	LOG(LM_MAIN, LL_ERROR, "Cannot find character name: %s", s);
	return NULL;
}
static const char *characterNames[] = {
	"Jones", "Ice",			"Ogre",	  "Dragon",	   "WarBaby", "Bug-eye",
	"Smith", "Ogre Boss",	"Grunt",  "Professor", "Snake",	  "Wolf",
	"Bob",	 "Mad bug-eye", "Cyborg", "Robot",	   "Lady"};
const char *IntCharacterFace(const int face)
{
	return characterNames[face];
}
void CharacterOldFaceToHair(const char *face, char **newFace, char **hair)
{
	// Convert old faces to face + hair
	if (strcmp(face, "Bob") == 0)
	{
		CSTRDUP(*newFace, "Jones");
		CSTRDUP(*hair, "beard");
	}
	else if (strcmp(face, "Cyber Jones") == 0)
	{
		CSTRDUP(*newFace, "Cyborg");
		CSTRDUP(*hair, "cyber_shades");
	}
	else if (strcmp(face, "Cyber Smith") == 0)
	{
		CSTRDUP(*newFace, "Cyborg");
		CSTRDUP(*hair, "flattop");
	}
	else if (strcmp(face, "Cyber WarBaby") == 0)
	{
		CSTRDUP(*newFace, "Cyborg");
		CSTRDUP(*hair, "beret");
	}
	else if (strcmp(face, "Cyborg") == 0)
	{
		CSTRDUP(*newFace, "Cyborg");
		CSTRDUP(*hair, "cyborg");
	}
	else if (strcmp(face, "Dragon") == 0)
	{
		CSTRDUP(*newFace, "Jones");
		CSTRDUP(*hair, "hogan");
	}
	else if (strcmp(face, "Evil Ogre") == 0)
	{
		CSTRDUP(*newFace, "Ogre");
		CSTRDUP(*hair, "horns");
	}
	else if (strcmp(face, "Freeze") == 0)
	{
		CSTRDUP(*newFace, "Jones");
		CSTRDUP(*hair, "ski_goggles");
	}
	else if (strcmp(face, "Goggles") == 0)
	{
		CSTRDUP(*newFace, "Jones");
		CSTRDUP(*hair, "goggles");
	}
	else if (strcmp(face, "Grunt") == 0)
	{
		CSTRDUP(*newFace, "Jones");
		CSTRDUP(*hair, "riot_helmet");
	}
	else if (strcmp(face, "Ice") == 0)
	{
		CSTRDUP(*newFace, "Jones");
		CSTRDUP(*hair, "shades");
	}
	else if (strcmp(face, "Lady") == 0)
	{
		CSTRDUP(*newFace, "Lady");
		CSTRDUP(*hair, "ponytail");
	}
	else if (strcmp(face, "Ogre Boss") == 0)
	{
		CSTRDUP(*newFace, "Ogre");
		CSTRDUP(*hair, "mohawk");
	}
	else if (strcmp(face, "Professor") == 0)
	{
		CSTRDUP(*newFace, "Jones");
		CSTRDUP(*hair, "professor");
	}
	else if (strcmp(face, "Smith") == 0)
	{
		CSTRDUP(*newFace, "Jones");
		CSTRDUP(*hair, "flattop");
	}
	else if (strcmp(face, "Snake") == 0)
	{
		CSTRDUP(*newFace, "Jones");
		CSTRDUP(*hair, "eye_patch");
	}
	else if (strcmp(face, "Sweeper") == 0)
	{
		CSTRDUP(*newFace, "Jones");
		CSTRDUP(*hair, "helmet");
	}
	else if (strcmp(face, "WarBaby") == 0)
	{
		CSTRDUP(*newFace, "Jones");
		CSTRDUP(*hair, "beret");
	}
	else if (strcmp(face, "Wolf") == 0)
	{
		CSTRDUP(*newFace, "Jones");
		CSTRDUP(*hair, "dutch");
	}
	else
	{
		CSTRDUP(*newFace, face);
	}
}

const NamedSprites *CharacterClassGetDeathSprites(const CharacterClass *c, const PicManager *pm)
{
	char buf[256];
	sprintf(buf, "chars/%s", c->DeathSprites);
	return PicManagerGetSprites(pm, buf);
}
const CharacterClass *IndexCharacterClass(const int i)
{
	CASSERT(
		i >= 0 && i < (int)gCharacterClasses.Classes.size +
						  (int)gCharacterClasses.CustomClasses.size,
		"Character class index out of bounds");
	if (i < (int)gCharacterClasses.Classes.size)
	{
		return CArrayGet(&gCharacterClasses.Classes, i);
	}
	return CArrayGet(
		&gCharacterClasses.CustomClasses, i - gCharacterClasses.Classes.size);
}
int CharacterClassIndex(const CharacterClass *c)
{
	if (c == NULL)
	{
		return 0;
	}
	CA_FOREACH(const CharacterClass, cc, gCharacterClasses.Classes)
	if (cc == c)
	{
		return _ca_index;
	}
	CA_FOREACH_END()
	CA_FOREACH(const CharacterClass, cc, gCharacterClasses.CustomClasses)
	if (cc == c)
	{
		return _ca_index + (int)gCharacterClasses.Classes.size;
	}
	CA_FOREACH_END()
	CASSERT(false, "cannot find character class");
	return -1;
}

void CharacterClassGetSound(const CharacterClass *c, char *out, const char *sound)
{
	sprintf(out, "chars/%s/%s", sound, c->Sounds);
}

void CharacterClassesInitialize(CharacterClasses *c, const char *filename)
{
	memset(c, 0, sizeof *c);
	CArrayInit(&c->Classes, sizeof(CharacterClass));
	CArrayInit(&c->CustomClasses, sizeof(CharacterClass));

	char buf[CDOGS_PATH_MAX];
	GetDataFilePath(buf, filename);
	FILE *f = fopen(buf, "r");
	json_t *root = NULL;
	if (f == NULL)
	{
		LOG(LM_MAIN, LL_ERROR, "cannot load characters file %s", buf);
		goto bail;
	}
	enum json_error e = json_stream_parse(f, &root);
	if (e != JSON_OK)
	{
		LOG(LM_MAIN, LL_ERROR, "error parsing characters file %s", buf);
		goto bail;
	}
	CharacterClassesLoadJSON(&c->Classes, root);

bail:
	if (f != NULL)
	{
		fclose(f);
	}
	json_free_value(&root);
}
static void LoadCharacterClass(CharacterClass *c, json_t *node);
void CharacterClassesLoadJSON(CArray *classes, json_t *root)
{
	int version;
	LoadInt(&version, root, "Version");
	if (version > VERSION || version <= 0)
	{
		LOG(LM_MAIN, LL_ERROR, "Cannot read character file version: %d",
			version);
		return;
	}

	json_t *charactersNode = json_find_first_label(root, "Characters")->child;
	for (json_t *child = charactersNode->child; child; child = child->next)
	{
		CharacterClass cc;
		LoadCharacterClass(&cc, child);
		CArrayPushBack(classes, &cc);
	}
}
static void LoadCharacterClass(CharacterClass *c, json_t *node)
{
	memset(c, 0, sizeof *c);
	c->Name = GetString(node, "Name");
	LoadBool(&c->Vehicle, node, "Vehicle");
	// TODO: allow non-directional head sprites?
	json_t *headPics = json_find_first_label(node, "HeadPics")->child;
	c->HeadSprites = GetString(headPics, "Sprites");
	LoadStr(&c->Body, node, "Body");
	if (c->Body == NULL)
	{
		CSTRDUP(c->Body, "base");
	}
	LoadStr(&c->DeathSprites, node, "DeathSprites");
	if (c->DeathSprites == NULL)
	{
		CSTRDUP(c->DeathSprites, "death");
	}
	c->Mass = CHARACTER_DEFAULT_MASS;
	LoadInt(&c->Mass, node, "Mass");
	c->Sprites = StrCharSpriteClass(c->Body);

	LoadStr(&c->Sounds, node, "Sounds");
	
	LoadStr(&c->Footsteps, node, "Footsteps");
	c->FootstepsDistancePlus = FOOTSTEP_DISTANCE_PLUS;
	LoadInt(&c->FootstepsDistancePlus, node, "FootstepsDistancePlus");

	c->BloodColor = colorRed;
	LoadColor(&c->BloodColor, node, "BloodColor");

	c->HasHair = true;
	LoadBool(&c->HasHair, node, "HasHair");
	
	LoadStr(&c->Corpse, node, "Corpse");
}
static void CharacterClassFree(CharacterClass *c);
void CharacterClassesClear(CArray *classes)
{
	for (int i = 0; i < (int)classes->size; i++)
	{
		CharacterClassFree(CArrayGet(classes, i));
	}
	CArrayClear(classes);
}
static void CharacterClassFree(CharacterClass *c)
{
	CFREE(c->Name);
	CFREE(c->HeadSprites);
	CFREE(c->Body);
	CFREE(c->DeathSprites);
	CFREE(c->Sounds);
	CFREE(c->Footsteps);
	CFREE(c->Corpse);
}
void CharacterClassesTerminate(CharacterClasses *c)
{
	CharacterClassesClear(&c->Classes);
	CArrayTerminate(&c->Classes);
	CharacterClassesClear(&c->CustomClasses);
	CArrayTerminate(&c->CustomClasses);
}
