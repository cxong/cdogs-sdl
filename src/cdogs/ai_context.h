/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (c) 2013-2015, Cong Xu
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

#include "config.h"
#include "objective.h"
#include "path_cache.h"
#include "vector.h"

// State data for various AI routines

// State for what the AI is currently doing
// Used for AI chatter
typedef enum
{
	AI_STATE_NONE,
	AI_STATE_IDLE,
	AI_STATE_DIE,
	AI_STATE_FOLLOW,
	AI_STATE_HUNT,
	AI_STATE_TRACK,
	AI_STATE_FLEE,
	AI_STATE_CONFUSED,
	AI_STATE_NEXT_OBJECTIVE
} AIState;
// State for what the AI is doing when confused
typedef enum
{
	AI_CONFUSION_CONFUSED,	// perform a random action
	AI_CONFUSION_CORRECT	// perform the right action (correct for confusion)
} AIConfusionType;
typedef struct
{
	AIConfusionType Type;
	int Cmd;
} AIConfusionState;
typedef enum
{
	AI_OBJECTIVE_TYPE_NORMAL,
	AI_OBJECTIVE_TYPE_KEY,
	AI_OBJECTIVE_TYPE_EXIT,
	AI_OBJECTIVE_TYPE_KILL,
	AI_OBJECTIVE_TYPE_PICKUP
} AIObjectiveType;
// State for AI attempting to complete an objective
// This is to prevent excessive pathfinding calls
typedef struct
{
	AIObjectiveType Type;
	bool IsDestructible;
	union
	{
		const ObjectiveDef *Obj;
		int UID;
	} u;
	int LastDone;
	Vec2i Goal;
} AIObjectiveState;
typedef struct
{
	Vec2i Goal;
	CachedPath Path;
	int PathIndex;
	bool IsFollowing;
} AIGotoContext;
typedef struct
{
	// Delay in executing consecutive actions;
	// Used to let the AI perform one action for a set amount of time
	int Delay;
	AIState State;

	// Counters to moderate amount of chatter
	int ChatterCounter;

	AIConfusionState ConfusionState;
	AIObjectiveState ObjectiveState;
	Vec2i LastTile;
	bool IsStuckTooLong;
	AIGotoContext Goto;
	int EnemyId;
	double GunRangeScalar;
	int OnGunId;
} AIContext;

AIContext *AIContextNew(void);
void AIContextDestroy(AIContext *c);

const char *AIStateGetChatterText(const AIState s);
bool AIContextShowChatter(const AIContext *c, const AIChatterFrequency f);
bool AIContextSetState(AIContext *c, const AIState s);
