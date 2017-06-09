/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (c) 2017 Cong Xu
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

#include "c_hashmap/hashmap.h"
#include "defs.h"
#include "utils.h"

typedef struct
{
	char *Name;
	BodyPart Order[DIRECTION_COUNT][BODY_PART_COUNT];
	struct
	{
		// Offsets by animation frame
		// of CArray of Vec2i, mapped by animation and indexed by frame
		map_t Frame[BODY_PART_COUNT];
		// Offsets by direction
		Vec2i Dir[BODY_PART_COUNT][DIRECTION_COUNT];
	} Offsets;
} CharSprites;
typedef struct
{
	map_t classes;
	map_t customClasses;
} CharSpriteClasses;
extern CharSpriteClasses gCharSpriteClasses;

const CharSprites *StrCharSpriteClass(const char *s);

void CharSpriteClassesInit(CharSpriteClasses *c);
void CharSpriteClassesLoadDir(map_t classes, const char *path);
void CharSpriteClassesClear(map_t classes);
void CharSpriteClassesTerminate(CharSpriteClasses *c);

Vec2i CharSpritesGetOffset(
	const map_t offsets, const char *anim, const int frame);
