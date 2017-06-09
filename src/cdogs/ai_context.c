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
#include "ai_context.h"


AIContext *AIContextNew(void)
{
	AIContext *c;
	CCALLOC(c, sizeof *c);
	// Initialise chatter counter so we don't say anything in the background
	c->ChatterCounter = 2;
	c->EnemyId = -1;
	c->GunRangeScalar = 1.0;
	return c;
}
void AIContextDestroy(AIContext *c)
{
	if (c)
	{
		CachedPathDestroy(&c->Goto.Path);
	}
	CFREE(c);
}

const char *AIStateGetChatterText(const AIState s)
{
	switch (s)
	{
	case AI_STATE_NONE:
		return "";
	case AI_STATE_IDLE:
		return "zzz";
	case AI_STATE_DIE:
		return "blarg";
	case AI_STATE_FOLLOW:
		return "let's go!";
	case AI_STATE_HUNT:
		return "!";
	case AI_STATE_TRACK:
		return "?";
	case AI_STATE_FLEE:
		return "aah!";
	case AI_STATE_CONFUSED:
		return "???";
	case AI_STATE_NEXT_OBJECTIVE:
		return "objective";
	default:
		CASSERT(false, "Unknown AI state");
		return "";
	}
}

bool AIContextShowChatter(const AIContext *c, const AIChatterFrequency f)
{
	return f != AICHATTER_NONE && c->ChatterCounter <= 0;
}

static void AIContextSetChatterDelay(AIContext *c, const AIChatterFrequency f);
bool AIContextSetState(AIContext *c, const AIState s)
{
	const bool isChange = c->State != s;
	c->State = s;
	if (isChange)
	{
		AIContextSetChatterDelay(
			c, ConfigGetEnum(&gConfig, "Interface.AIChatter"));
	}
	return isChange;
}
static void AIContextSetChatterDelay(AIContext *c, const AIChatterFrequency f)
{
	c->ChatterCounter--;
	if (c->ChatterCounter >= 0)
	{
		return;
	}
	switch (f)
	{
	case AICHATTER_SELDOM:
		c->ChatterCounter = 100;
		break;
	case AICHATTER_OFTEN:
		c->ChatterCounter = 30;
		break;
	case AICHATTER_ALWAYS:
		c->ChatterCounter = 0;
		break;
	default:
		// do nothing
		break;
	}
}
