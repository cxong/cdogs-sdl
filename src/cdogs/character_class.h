/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.

	Copyright (c) 2016-2017 Cong Xu
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

#include "cpic.h"
#include "defs.h"
#include "draw/char_sprites.h"
#include "json/json.h"


typedef struct
{
	char *Name;
	CPic HeadPics;
	const CharSprites *Sprites;
	struct
	{
		char *Aargh;
	} Sounds;
} CharacterClass;
typedef struct
{
	CArray Classes;	// of CharacterClass
	CArray CustomClasses;	// of CharacterClass
} CharacterClasses;
extern CharacterClasses gCharacterClasses;

const CharacterClass *StrCharacterClass(const char *s);
// Legacy character class from "face" index
const CharacterClass *IntCharacterClass(const int face);
const CharacterClass *IndexCharacterClass(const int i);
int CharacterClassIndex(const CharacterClass *c);

void CharacterClassesInitialize(CharacterClasses *c, const char *filename);
void CharacterClassesLoadJSON(CArray *classes, json_t *root);
void CharacterClassesClear(CArray *classes);
void CharacterClassesTerminate(CharacterClasses *c);
