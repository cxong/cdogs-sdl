/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.

    Copyright (c) 2014, 2016-2017 Cong Xu
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
#include "game_loop.h"

#include <SDL_timer.h>

#include "config.h"
#include "events.h"
#include "net_client.h"
#include "net_server.h"
#include "sounds.h"


GameLoopData *GameLoopDataNew(
	void *data,
	void (*onTerminate)(GameLoopData *),
	void (*onEnter)(GameLoopData *), void (*onExit)(GameLoopData *),
	void (*inputFunc)(GameLoopData *),
	GameLoopResult (*updateFunc)(GameLoopData *, LoopRunner *),
	void (*drawFunc)(GameLoopData *))
{
	GameLoopData *g;
	CCALLOC(g, sizeof *g);
	g->Data = data;
	g->OnTerminate = onTerminate;
	g->OnEnter = onEnter;
	g->OnExit = onExit;
	g->InputFunc = inputFunc;
	g->UpdateFunc = updateFunc;
	g->DrawFunc = drawFunc;
	g->FPS = 30;
	return g;
}

static GameLoopData *GetCurrentLoop(LoopRunner *l);

LoopRunner LoopRunnerNew(GameLoopData *newData)
{
	LoopRunner l;
	CArrayInit(&l.Loops, sizeof(GameLoopData *));
	if (newData != NULL)
	{
		LoopRunnerPush(&l, newData);
	}
	return l;
}
static void GameLoopTerminate(GameLoopData *data);
void LoopRunnerTerminate(LoopRunner *l)
{
	for (int i = (int)l->Loops.size - 1; i >= 0; i--)
	{
		GameLoopData *g = CArrayGet(&l->Loops, i);
		GameLoopTerminate(g);
	}
	CArrayTerminate(&l->Loops);
}

static void GameLoopOnEnter(GameLoopData *data);
static void GameLoopOnExit(GameLoopData *data);

typedef struct
{
	GameLoopResult Result;
	Uint32 TicksNow;
	Uint32 TicksElapsed;
	int FrameDurationMs;
	int FramesSkipped;
	int MaxFrameskip;
} LoopRunParams;
static LoopRunParams LoopRunParamsNew(const GameLoopData *data);
static bool LoopRunParamsShouldSleep(LoopRunParams *p);
static bool LoopRunParamsShouldSkip(LoopRunParams *p);
void LoopRunnerRun(LoopRunner *l)
{
	GameLoopData *data = GetCurrentLoop(l);
	if (data == NULL)
	{
		return;
	}
	GameLoopOnEnter(data);

	LoopRunParams p = LoopRunParamsNew(data);
	for (;;)
	{
		// Frame rate control
		if (LoopRunParamsShouldSleep(&p))
		{
			SDL_Delay(1);
			continue;
		}

		// Input
		if ((data->Frames & 1) || !data->InputEverySecondFrame)
		{
			EventPoll(&gEventHandlers, p.TicksNow);
			if (data->InputFunc)
			{
				data->InputFunc(data);
			}
		}

		NetClientPoll(&gNetClient);
		NetServerPoll(&gNetServer);

		// Update
		p.Result = data->UpdateFunc(data, l);
		GameLoopData *newData = GetCurrentLoop(l);
		if (newData == NULL)
		{
			break;
		}
		else if (newData != data)
		{
			// State change; restart loop
			GameLoopOnExit(data);
			data = newData;
			GameLoopOnEnter(data);
			p = LoopRunParamsNew(data);
			continue;
		}

		NetServerFlush(&gNetServer);
		NetClientFlush(&gNetClient);

		bool draw = !data->HasDrawnFirst;
		switch (p.Result)
		{
		case UPDATE_RESULT_OK:
			// Do nothing
			break;
		case UPDATE_RESULT_DRAW:
			draw = true;
			break;
		default:
			CASSERT(false, "Unknown loop result");
			break;
		}
		data->Frames++;
		// frame skip
		if (LoopRunParamsShouldSkip(&p))
		{
			continue;
		}

		// Draw
		if (draw)
		{
			if (data->DrawFunc)
			{
				data->DrawFunc(data);
				BlitFlip(&gGraphicsDevice);
			}
			data->HasDrawnFirst = true;
		}
	}
	GameLoopOnExit(data);
}
static LoopRunParams LoopRunParamsNew(const GameLoopData *data)
{
	LoopRunParams p;
	p.Result = UPDATE_RESULT_OK;
	p.TicksNow = SDL_GetTicks();
	p.TicksElapsed = 0;
	p.FrameDurationMs = 1000 / data->FPS;
	p.FramesSkipped = 0;
	p.MaxFrameskip = data->FPS / 5;
	return p;
}
static bool LoopRunParamsShouldSleep(LoopRunParams *p)
{
	const Uint32 ticksThen = p->TicksNow;
	p->TicksNow = SDL_GetTicks();
	p->TicksElapsed += p->TicksNow - ticksThen;
	return (int)p->TicksElapsed < p->FrameDurationMs;
}
static bool LoopRunParamsShouldSkip(LoopRunParams *p)
{
	p->TicksElapsed -= p->FrameDurationMs;
	// frame skip
	if ((int)p->TicksElapsed > p->FrameDurationMs)
	{
		p->FramesSkipped++;
		if (p->FramesSkipped == p->MaxFrameskip)
		{
			// We've skipped too many frames; give up
			p->TicksElapsed = 0;
		}
		else
		{
			return true;
		}
	}
	p->FramesSkipped = 0;
	return false;
}

void LoopRunnerChange(LoopRunner *l, GameLoopData *newData)
{
	LoopRunnerPop(l);
	LoopRunnerPush(l, newData);
}
void LoopRunnerPush(LoopRunner *l, GameLoopData *newData)
{
	CArrayPushBack(&l->Loops, &newData);
	newData->IsUsed = true;
}
void LoopRunnerPop(LoopRunner *l)
{
	GameLoopData *data = GetCurrentLoop(l);
	data->IsUsed = false;
	CArrayDelete(&l->Loops, l->Loops.size - 1);
}

static void GameLoopTerminate(GameLoopData *data)
{
	if (data->OnTerminate)
	{
		data->OnTerminate(data);
	}
	CFREE(data);
}

static void GameLoopOnEnter(GameLoopData *data)
{
	if (data->OnEnter)
	{
		data->OnEnter(data);
	}
	EventReset(
		&gEventHandlers,
		gEventHandlers.mouse.cursor, gEventHandlers.mouse.trail);
}
static void GameLoopOnExit(GameLoopData *data)
{
	if (data->OnExit)
	{
		data->OnExit(data);
	}
	if (!data->IsUsed)
	{
		GameLoopTerminate(data);
	}
}

static GameLoopData *GetCurrentLoop(LoopRunner *l)
{
	if (l->Loops.size == 0)
	{
		return NULL;
	}
	return *(GameLoopData **)CArrayGet(&l->Loops, l->Loops.size - 1);
}
