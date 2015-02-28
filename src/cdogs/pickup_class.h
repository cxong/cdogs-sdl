/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (c) 2015, Cong Xu
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

#include <json/json.h>

#include "ammo.h"
#include "utils.h"
#include "weapon.h"

// Effects for "pick up" objects
typedef enum
{
	PICKUP_NONE,
	PICKUP_JEWEL,
	PICKUP_HEALTH,
	PICKUP_AMMO,
	PICKUP_KEYCARD,
	PICKUP_GUN
} PickupType;
PickupType StrPickupType(const char *s);

typedef struct
{
	char *Name;
	PickupType Type;
	union
	{
		int Score;
		int Health;
		AddAmmo Ammo;
		int Keys;	// Refer to flags in mission.h
		int GunId;
	} u;
	const Pic *Pic;
} PickupClass;
typedef struct
{
	CArray Classes;	// of PickupClass
	CArray CustomClasses;	// of PickupClass
} PickupClasses;
extern PickupClasses gPickupClasses;


PickupClass *StrPickupClass(const char *s);
// Legacy pickup classes, integer based
PickupClass *IntPickupClass(const int i);
// Legacy key classes, style+integer based
PickupClass *KeyPickupClass(const int style, const int i);
PickupClass *PickupClassGetById(PickupClasses *classes, const int id);
int StrPickupClassId(const char *s);

void PickupClassesInit(
	PickupClasses *classes, const char *filename,
	const AmmoClasses *ammo, const GunClasses *guns);
void PickupClassesLoadJSON(CArray *classes, json_t *root);
void PickupClassesLoadAmmo(CArray *classes, const CArray *ammoClasses);
void PickupClassesLoadGuns(CArray *classes, const CArray *gunClasses);
void PickupClassesClear(CArray *classes);
void PickupClassesTerminate(PickupClasses *classes);

// Count the number of "Score" type pickups
int PickupClassesGetScoreCount(const PickupClasses *classes);

// Score for picking up an objective
#define PICKUP_SCORE 10

// TODO: more key styles
#define KEYSTYLE_COUNT 4
