/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (c) 2013-2014, Cong Xu
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
#ifndef __AI_CONTEXT
#define __AI_CONTEXT

#include "AStar.h"
#include "vector.h"

// State data for various AI routines

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
typedef struct
{
	Vec2i Goal;
	ASPath Path;
	int PathIndex;
	bool IsFollowing;
} AIGotoContext;
typedef struct
{
	// Delay in executing consecutive actions;
	// Used to let the AI perform one action for a set amount of time
	int Delay;
	AIConfusionState ConfusionState;
	AIGotoContext Goto;
} AIContext;

AIContext *AIContextNew(void);
void AIContextDestroy(AIContext *c);

#endif
