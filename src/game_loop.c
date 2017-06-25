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


GameLoopData GameLoopDataNew(
	void *data,
	void (*onTerminate)(GameLoopData *),
	void (*onEnter)(GameLoopData *), void (*onExit)(GameLoopData *),
	void (*inputFunc)(GameLoopData *),
	GameLoopResult (*updateFunc)(GameLoopData *),
	void (*drawFunc)(GameLoopData *))
{
	GameLoopData g;
	memset(&g, 0, sizeof g);
	g.Data = data;
	g.OnTerminate = onTerminate;
	g.OnEnter = onEnter;
	g.OnExit = onExit;
	g.InputFunc = inputFunc;
	g.UpdateFunc = updateFunc;
	g.DrawFunc = drawFunc;
	g.FPS = 30;
	return g;
}

static void GameLoopOnEnter(GameLoopData *data);

void GameLoop(GameLoopData *data)
{
	GameLoopOnEnter(data);
	GameLoopResult result = UPDATE_RESULT_OK;
	Uint32 ticksNow = SDL_GetTicks();
	Uint32 ticksElapsed = 0;
	int framesSkipped = 0;
	const int maxFrameskip = data->FPS / 5;
	for (; result != UPDATE_RESULT_EXIT; )
	{
		// Frame rate control
		const Uint32 ticksThen = ticksNow;
		ticksNow = SDL_GetTicks();
		ticksElapsed += ticksNow - ticksThen;
		if ((int)ticksElapsed < 1000 / data->FPS)
		{
			SDL_Delay(1);
			continue;
		}

		// Input
		if ((data->Frames & 1) || !data->InputEverySecondFrame)
		{
			EventPoll(&gEventHandlers, ticksNow);
			if (data->InputFunc)
			{
				data->InputFunc(data);
			}
		}

		NetClientPoll(&gNetClient);
		NetServerPoll(&gNetServer);

		// Update
		result = data->UpdateFunc(data);
		NetServerFlush(&gNetServer);
		NetClientFlush(&gNetClient);
		bool draw = !data->HasDrawnFirst;
		switch (result)
		{
		case UPDATE_RESULT_OK:
			// Do nothing
			break;
		case UPDATE_RESULT_DRAW:
			draw = true;
			break;
		case UPDATE_RESULT_EXIT:
			// Will exit
			break;
		default:
			CASSERT(false, "Unknown loop result");
			break;
		}
		ticksElapsed -= 1000 / data->FPS;
		data->Frames++;
		// frame skip
		if ((int)ticksElapsed > 1000 / data->FPS)
		{
			framesSkipped++;
			if (framesSkipped == maxFrameskip)
			{
				// We've skipped too many frames; give up
				ticksElapsed = 0;
			}
			else
			{
				continue;
			}
		}
		framesSkipped = 0;

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
	if (data->OnExit)
	{
		data->OnExit(data);
	}
}

void GameLoopTerminate(GameLoopData *data)
{
	if (data->OnTerminate)
	{
		data->OnTerminate(data);
	}
}

void GameLoopChange(GameLoopData *data, GameLoopData newData)
{
	if (data->OnExit)
	{
		data->OnExit(data);
	}
	GameLoopTerminate(data);
	memcpy(data, &newData, sizeof *data);
	GameLoopOnEnter(data);
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
