/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (C) 1995 Ronny Wester
    Copyright (C) 2003 Jeremy Chin 
    Copyright (C) 2003-2007 Lucas Martin-King 

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    This file incorporates work covered by the following copyright and
    permission notice:

    Copyright (c) 2013-2015, 2017 Cong Xu
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

#ifdef __EMSCRIPTEN__
#include <SDL.h>
#include <SDL/SDL_mixer.h>
#else
#include <SDL_mixer.h>
#endif

#include "c_array.h"
#include "game_events.h"
#include "pic.h"
#include "proto/msg.pb.h"

typedef enum
{
	ACTION_NULL,
	ACTION_SETTRIGGER,
	ACTION_CLEARTRIGGER,
	ACTION_EVENT,
	ACTION_ACTIVATEWATCH,
	ACTION_DEACTIVATEWATCH,
	ACTION_SOUND
} ActionType;


// TODO: eliminate Action, replace with standard game events
typedef struct
{
	ActionType Type;
	union
	{
		struct vec pos;
		int index;
	} u;
	union
	{
		GameEvent Event;
		Mix_Chunk *Sound;
	} a;
} Action;


typedef struct
{
	int id;
	int flags;
	int isActive;
	int cannotActivateLock;
	CArray actions;	// of Action
} Trigger;


typedef enum
{
	CONDITION_TILECLEAR = 1
} ConditionType;
typedef struct
{
	ConditionType Type;
	// How many ticks has this condition been fulfilled
	// Reset to 0 when condition failed
	int Counter;
	int CounterMax;
	Vec2i Pos;
} Condition;


typedef struct
{
	int index;
	CArray conditions;	// of Condition
	CArray actions;		// of Action
	bool active;
} TWatch;


bool TriggerTryActivate(Trigger *t, const int flags, const Vec2i tilePos);
bool TriggerCannotActivate(const Trigger *t);
void TriggerSetCannotActivate(Trigger *t);
void TriggerActivate(Trigger *t, CArray *mapTriggers);
void UpdateWatches(CArray *mapTriggers, const int ticks);
Trigger *TriggerNew(void);
void TriggerTerminate(Trigger *t);
Action *TriggerAddAction(Trigger *t);

void WatchesInit(void);
void WatchesTerminate(void);

TWatch *WatchNew(void);
Condition *WatchAddCondition(
	TWatch *w, const ConditionType type, const int counterMax,
	const Vec2i pos);
Action *WatchAddAction(TWatch *w);
