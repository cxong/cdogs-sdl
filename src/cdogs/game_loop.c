/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.

    Copyright (c) 2014, Cong Xu
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

#include "events.h"
#include "sounds.h"


GameLoopData GameLoopDataNew(
	void *updateData, GameLoopResult (*updateFunc)(void *),
	const void *drawData, void (*drawFunc)(const void *))
{
	GameLoopData g;
	memset(&g, 0, sizeof g);
	g.UpdateData = updateData;
	g.UpdateFunc = updateFunc;
	g.DrawData = drawData;
	g.DrawFunc = drawFunc;
	g.FrameDelayMs = 16;
	return g;
}

GameLoopResult GameLoop(GameLoopData *data)
{
	EventReset(&gEventHandlers, gEventHandlers.mouse.cursor);

	GameLoopResult result = UPDATE_RESULT_OK;
	for (; result != UPDATE_RESULT_EXIT; SDL_Delay(data->FrameDelayMs))
	{
#ifndef RUN_WITHOUT_APP_FOCUS
		MusicSetPlaying(&gSoundDevice, SDL_GetAppState() & SDL_APPINPUTFOCUS);
#endif
		// Update
		EventPoll(&gEventHandlers, SDL_GetTicks());
		result = data->UpdateFunc(data->UpdateData);
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
		if (draw)
		{
			data->DrawFunc(data->DrawData);
			BlitFlip(&gGraphicsDevice, &gConfig.Graphics);
			data->HasDrawnFirst = true;
		}
	}
	return result;
}
