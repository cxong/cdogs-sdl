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
#include "game_events.h"

#include <string.h>

#include "net_client.h"
#include "net_server.h"
#include "utils.h"

CArray gGameEvents;

void GameEventsInit(CArray *store)
{
	CArrayInit(store, sizeof(GameEvent));
}
void GameEventsTerminate(CArray *store)
{
	CArrayTerminate(store);
}
void GameEventsEnqueue(CArray *store, GameEvent e)
{
	if (store->elemSize == 0)
	{
		return;
	}
	// If we're the server, broadcast any events that clients need
	// If we're the client, pass along to server, but only if it's for a local player
	// Otherwise we'd ping-pong the same updates from the server
	switch (e.Type)
	{
	case GAME_EVENT_ACTOR_ADD:
		NetServerBroadcastMsg(
			&gNetServer, MSG_ACTOR_ADD, &e.u.ActorAdd);
		break;
	case GAME_EVENT_ACTOR_MOVE:
		NetServerBroadcastMsg(&gNetServer, MSG_ACTOR_MOVE, &e.u.ActorMove);
		if (ActorIsLocalPlayer(e.u.ActorMove.UID))
			NetClientSendMsg(&gNetClient, MSG_ACTOR_MOVE, &e.u.ActorMove);
		break;
	case GAME_EVENT_ACTOR_STATE:
		NetServerBroadcastMsg(&gNetServer, MSG_ACTOR_STATE, &e.u.ActorState);
		if (ActorIsLocalPlayer(e.u.ActorState.UID))
			NetClientSendMsg(&gNetClient, MSG_ACTOR_STATE, &e.u.ActorState);
		break;
	case GAME_EVENT_ACTOR_DIR:
		NetServerBroadcastMsg(&gNetServer, MSG_ACTOR_DIR, &e.u.ActorDir);
		if (ActorIsLocalPlayer(e.u.ActorDir.UID))
			NetClientSendMsg(&gNetClient, MSG_ACTOR_DIR, &e.u.ActorDir);
		break;
	case GAME_EVENT_MISSION_END:
		NetServerBroadcastMsg(&gNetServer, MSG_GAME_END, NULL);
		break;
	default:
		// do nothing
		break;
	}
	CArrayPushBack(store, &e);
}
static bool EventComplete(const void *elem);
void GameEventsClear(CArray *store)
{
	CArrayRemoveIf(store, EventComplete);
}
static bool EventComplete(const void *elem)
{
	return ((const GameEvent *)elem)->Delay < 0;
}

GameEvent GameEventNew(GameEventType type)
{
	GameEvent e;
	memset(&e, 0, sizeof e);
	e.Type = type;
	switch (type)
	{
	case GAME_EVENT_ADD_PICKUP:
		e.u.AddPickup.SpawnerUID = -1;
		break;
	default:
		// Do nothing
		break;
	}
	return e;
}
