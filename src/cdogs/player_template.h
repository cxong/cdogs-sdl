/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.

	Copyright (c) 2013-2014, 2016-2018 Cong Xu
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

#include "character.h"
#include "character_class.h"

#define PLAYER_NAME_MAXLEN 20

typedef struct
{
	char name[PLAYER_NAME_MAXLEN];
	char *CharClassName;
	CharColors Colors;
} PlayerTemplate;

typedef struct
{
	CArray Classes;	// of PlayerTemplate
	CArray CustomClasses;	// of PlayerTemplate
} PlayerTemplates;
extern PlayerTemplates gPlayerTemplates;

void PlayerTemplatesLoad(PlayerTemplates *pt, const CharacterClasses *classes);
void PlayerTemplatesLoadJSON(CArray *classes, json_t *node);
void PlayerTemplatesClear(CArray *classes);
void PlayerTemplatesTerminate(PlayerTemplates *pt);

PlayerTemplate *PlayerTemplateGetById(PlayerTemplates *pt, const int id);
void PlayerTemplatesSave(const PlayerTemplates *pt);
