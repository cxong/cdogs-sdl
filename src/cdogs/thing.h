/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.

    Copyright (c) 2013-2019 Cong Xu
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

#include <stdbool.h>

#include "c_array.h"
#include "cpic.h"
#include "mathc/mathc.h"
#include "pic.h"
#include "proto/msg.pb.h"
#include "vector.h"

typedef enum
{
	KIND_CHARACTER,
	KIND_PARTICLE,
	KIND_MOBILEOBJECT,
	KIND_OBJECT,
	KIND_PICKUP
} ThingKind;

#define THING_IMPASSABLE     1
#define THING_CAN_BE_SHOT    2
#define THING_OBJECTIVE      (8 + 16 + 32 + 64 + 128)
#define THING_DRAW_BELOW     256
#define THING_DRAW_ABOVE     512
#define OBJECTIVE_SHIFT      3


typedef struct
{
	int MobObjId;
	struct vec2 Scale;
	union
	{
		struct
		{
			const CArray *Sprites;
			direction_e Dir;
			color_t Color;
		} MuzzleFlash;
	} u;
} ThingDrawFuncData;
typedef void (*ThingDrawFunc)(const struct vec2i, const ThingDrawFuncData *);
typedef struct
{
	struct vec2 Pos;
	struct vec2 LastPos;
	struct vec2 Vel;
	struct vec2i size;
	ThingKind kind;
	int id;	// Id of item (actor, mobobj or obj)
	int flags;
	ThingDrawFunc drawFunc;
	ThingDrawFuncData drawData;
	CPic CPic;
	DrawCPicFunc CPicFunc;
	struct vec2 drawShake;
	struct vec2i ShadowSize;
	int SoundLock;
} Thing;
#define SOUND_LOCK_THING 12


typedef struct
{
	// TODO: add an entity id system
	int Id;
	ThingKind Kind;
} ThingId;


bool IsThingInsideTile(const Thing *i, const struct vec2i tilePos);

void ThingInit(
	Thing *t, const int id, const ThingKind kind, const struct vec2i size,
	const int flags);
void ThingUpdate(Thing *t, const int ticks);
void ThingAddDrawShake(Thing *t, const struct vec2 shake);
void ThingDamage(const NThingDamage d);

Thing *ThingGetByUID(const ThingKind kind, const int uid);
Thing *ThingIdGetThing(const ThingId *tid);
bool ThingDrawBelow(const Thing *t);
bool ThingDrawAbove(const Thing *t);
