/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.

    Copyright (c) 2013, Cong Xu
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
#ifndef __GAME_EVENTS
#define __GAME_EVENTS

// Game events represent anything that is created within the game but is
// required by outside systems, e.g. sound events
// This is to prevent the game from depending on these external systems

typedef enum
{
	GAME_EVENT_SCREEN_SHAKE,
	GAME_EVENT_SET_MESSAGE
} GameEventType;

typedef struct
{
	GameEventType Type;
	union
	{
		int ShakeAmount;
		struct
		{
			char Message[256];
			int Ticks;
		} SetMessage;
	} u;
} GameEvent;

typedef struct
{
	GameEvent *events;
	int count;
	int max;
} GameEventStore;

extern GameEventStore gGameEvents;

void GameEventsInit(GameEventStore *store);
void GameEventsTerminate(GameEventStore *store);
void GameEventsEnqueue(GameEventStore *store, GameEvent e);
void GameEventsClear(GameEventStore *store);

#endif
