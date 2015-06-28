/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.

    Copyright (c) 2014-2015, Cong Xu
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
#include "net_server.h"

#include <string.h>

#include "proto/nanopb/pb_encode.h"

#include "actor_placement.h"
#include "campaign_entry.h"
#include "events.h"
#include "game_events.h"
#include "gamedata.h"
#include "handle_game_events.h"
#include "log.h"
#include "pickup.h"
#include "player.h"
#include "sys_config.h"
#include "utils.h"

NetServer gNetServer;


void NetServerInit(NetServer *n)
{
	memset(n, 0, sizeof *n);
}
void NetServerTerminate(NetServer *n)
{
	if (n->server)
	{
		enet_host_destroy(n->server);
	}
	n->server = NULL;
}
void NetServerReset(NetServer *n)
{
	n->PrevCmd = n->Cmd = 0;
}

void NetServerOpen(NetServer *n)
{
#if USE_NET == 1
	/* Bind the server to the default localhost.     */
	/* A specific host address can be specified by   */
	/* enet_address_set_host (& address, "x.x.x.x"); */
	ENetAddress address;
	address.host = ENET_HOST_ANY;
	address.port = NET_INPUT_PORT;
	n->server = enet_host_create(
		&address /* the address to bind the server host to */,
		32      /* allow up to 32 clients and/or outgoing connections */,
		2      /* allow up to 2 channels to be used, 0 and 1 */,
		0      /* assume any amount of incoming bandwidth */,
		0      /* assume any amount of outgoing bandwidth */);
	if (n->server == NULL)
	{
		fprintf(stderr,
			"An error occurred while trying to create an ENet server host.\n");
		return;
	}
#else
	UNUSED(n);
#endif
}

static void OnConnect(NetServer *n, ENetEvent event);
static void OnReceive(NetServer *n, ENetEvent event);
void NetServerPoll(NetServer *n)
{
	if (!n->server)
	{
		return;
	}

	n->PrevCmd = n->Cmd;
	n->Cmd = 0;
	int check;
	do
	{
		ENetEvent event;
		check = enet_host_service(n->server, &event, 0);
		if (check < 0)
		{
			fprintf(stderr, "Host check event failure\n");
			enet_host_destroy(n->server);
			n->server = NULL;
			return;
		}
		else if (check > 0)
		{
			switch (event.type)
			{
			case ENET_EVENT_TYPE_CONNECT:
				OnConnect(n, event);
				break;
			case ENET_EVENT_TYPE_RECEIVE:
				OnReceive(n, event);
				break;
			case ENET_EVENT_TYPE_DISCONNECT:
				{
					printf("disconnected %x:%u.\n",
						event.peer->address.host,
						event.peer->address.port);
					/* Reset the peer's client information. */
					CFREE(event.peer->data);
				}
				break;

			default:
				CASSERT(false, "Unknown event");
				break;
			}
		}
	} while (check > 0);
}
static void OnConnect(NetServer *n, ENetEvent event)
{
	LOG(LM_NET, LL_INFO, "new client connected from %x:%u.\n",
		event.peer->address.host, event.peer->address.port);
	/* Store any relevant client information here. */
	CMALLOC(event.peer->data, sizeof(NetPeerData));
	const int peerId = n->peerId;
	((NetPeerData *)event.peer->data)->Id = peerId;
	n->peerId++;

	// Send the client ID
	LOG(LM_NET, LL_DEBUG, "NetServer: sending client ID %d", peerId);
	NClientId cid;
	cid.Id = peerId;
	cid.FirstPlayerUID = (peerId + 1) * MAX_LOCAL_PLAYERS;
	NetServerSendMsg(n, peerId, GAME_EVENT_CLIENT_ID, &cid);

	// Send the current campaign details over
	LOG(LM_NET, LL_DEBUG, "NetServer: sending campaign entry");
	NCampaignDef def = NMakeCampaignDef(&gCampaign);
	NetServerSendMsg(n, peerId, GAME_EVENT_CAMPAIGN_DEF, &def);

	SoundPlay(&gSoundDevice, StrSound("hahaha"));
	LOG(LM_NET, LL_DEBUG, "NetServer: client connection complete");
}
static void SendGameStartMessages(NetServer *n, const int peerId);
static void OnReceive(NetServer *n, ENetEvent event)
{
	const GameEventType msg = (GameEventType)*(uint32_t *)event.packet->data;
	const int peerId = ((NetPeerData *)event.peer->data)->Id;
	LOG(LM_NET, LL_TRACE, "recv message from peerId(%d) msg(%d)",
		peerId, (int)msg);
	const GameEventEntry gee = GameEventGetEntry(msg);
	if (gee.Enqueue)
	{
		// Game event message; decode and add to event queue
		LOG(LM_NET, LL_DEBUG, "recv gameEvent(%d)", (int)gee.Type);
		GameEvent e = GameEventNew(gee.Type);
		NetDecode(event.packet, &e.u, gee.Fields);
		GameEventsEnqueue(&gGameEvents, e);
	}
	else
	{
		switch (gee.Type)
		{
		case GAME_EVENT_CLIENT_READY:
			// Send game start messages if we've started already
			if (gMission.HasStarted)
			{
				// Flush game events to make sure we add the players
				HandleGameEvents(
					&gGameEvents, NULL, NULL, NULL, NULL, &gEventHandlers);
				// Reset player data
				GameEvent e = GameEventNew(GAME_EVENT_ADD_PLAYERS);
				for (int i = 0; i < MAX_LOCAL_PLAYERS; i++)
				{
					const int cid = (peerId + 1) * MAX_LOCAL_PLAYERS + i;
					const PlayerData *pData = PlayerDataGetByUID(cid);
					if (pData != NULL)
					{
						const int idx = (int)e.u.AddPlayers.PlayerDatas_count;
						e.u.AddPlayers.PlayerDatas[idx] =
							PlayerDataMissionReset(pData);
						e.u.AddPlayers.PlayerDatas_count++;
					}
				}
				GameEventsEnqueue(&gGameEvents, e);
				// Flush game events to make sure we reset player data
				HandleGameEvents(
					&gGameEvents, NULL, NULL, NULL, NULL, &gEventHandlers);
				SendGameStartMessages(n, peerId);
				// Add the client's actors
				for (int i = 0; i < MAX_LOCAL_PLAYERS; i++)
				{
					const int cid = (peerId + 1) * MAX_LOCAL_PLAYERS + i;
					const PlayerData *pData = PlayerDataGetByUID(cid);
					if (pData != NULL)
					{
						PlacePlayer(&gMap, pData, Vec2iZero(), true);
					}
				}
			}
			break;
		default:
			CASSERT(false, "unexpected message type");
			break;
		}
	}
	enet_packet_destroy(event.packet);
}
static void SendGameStartMessages(NetServer *n, const int peerId)
{
	// Send details of all current players
	NAddPlayers ap = NAddPlayers_init_default;
	for (int i = 0; i < (int)gPlayerDatas.size; i++)
	{
		const PlayerData *pOther = CArrayGet(&gPlayerDatas, i);
		ap.PlayerDatas[i] = NMakePlayerData(pOther);
		ap.PlayerDatas_count++;
		LOG(LM_NET, LL_DEBUG, "send player data uid(%d) maxHealth(%d)",
			(int)ap.PlayerDatas[i].UID, (int)ap.PlayerDatas[i].MaxHealth);
	}
	NetServerSendMsg(n, peerId, GAME_EVENT_ADD_PLAYERS, &ap);

	NetServerSendMsg(n, peerId, GAME_EVENT_NET_GAME_START, NULL);

	// Send all actors
	for (int i = 0; i < (int)gActors.size; i++)
	{
		const TActor *a = CArrayGet(&gActors, i);
		if (!a->isInUse)
		{
			continue;
		}
		NActorAdd aa = NActorAdd_init_default;
		aa.UID = a->uid;
		aa.CharId = a->charId;
		aa.Health = a->health;
		aa.Direction = (int32_t)a->direction;
		aa.PlayerUID = a->PlayerUID;
		aa.TileItemFlags = a->tileItem.flags;
		aa.FullPos.x = a->Pos.x;
		aa.FullPos.y = a->Pos.y;
		LOG(LM_NET, LL_DEBUG, "send add player UID(%d) playerUID(%d)",
			(int)aa.UID, (int)aa.PlayerUID);
		NetServerSendMsg(n, peerId, GAME_EVENT_ACTOR_ADD, &aa);
	}

	// Send key state
	NAddKeys ak = NAddKeys_init_default;
	ak.KeyFlags = gMission.KeyFlags;
	NetServerSendMsg(n, peerId, GAME_EVENT_ADD_KEYS, &ak);

	// Send objective counts
	for (int i = 0; i < (int)gMission.Objectives.size; i++)
	{
		NObjectiveCount oc;
		oc.ObjectiveId = i;
		const ObjectiveDef *o = CArrayGet(&gMission.Objectives, i);
		oc.Count = o->done;
		NetServerSendMsg(n, peerId, GAME_EVENT_OBJECTIVE_COUNT, &oc);
	}

	// Send all the tiles visited so far
	Vec2i pos;
	for (pos.y = 0; pos.y < gMap.Size.y; pos.y++)
	{
		for (pos.x = 0; pos.x < gMap.Size.x; pos.x++)
		{
			const Tile *t = MapGetTile(&gMap, pos);
			if (!t->isVisited) continue;
			NExploreTile et;
			et.Tile = Vec2i2Net(pos);
			NetServerSendMsg(n, peerId, GAME_EVENT_EXPLORE_TILE, &et);
		}
	}

	// Send all pickups
	for (int i = 0; i < (int)gPickups.size; i++)
	{
		const Pickup *p = CArrayGet(&gPickups, i);
		if (!p->isInUse) continue;
		NAddPickup api = NAddPickup_init_default;
		api.UID = p->UID;
		strcpy(api.PickupClass, p->class->Name);
		api.IsRandomSpawned = p->IsRandomSpawned;
		api.SpawnerUID = p->SpawnerUID;
		api.TileItemFlags = p->tileItem.flags;
		api.Pos = Vec2i2Net(Vec2iNew(p->tileItem.x, p->tileItem.y));
		NetServerSendMsg(n, peerId, GAME_EVENT_ADD_PICKUP, &api);
	}

	// Send all map objects
	for (int i = 0; i < (int)gObjs.size; i++)
	{
		const TObject *o = CArrayGet(&gObjs, i);
		if (!o->isInUse) continue;
		NAddMapObject amo = NAddMapObject_init_default;
		amo.UID = o->uid;
		strcpy(amo.MapObjectClass, o->Class->Name);
		amo.Pos = Vec2i2Net(Vec2iNew(o->tileItem.x, o->tileItem.y));
		amo.TileItemFlags = o->tileItem.flags;
		amo.Health = o->Health;
		NetServerSendMsg(n, peerId, GAME_EVENT_ADD_MAP_OBJECT, &amo);
	}
}

void NetServerSendMsg(
	NetServer *n, const int peerId, const GameEventType e, const void *data)
{
	if (!n->server)
	{
		return;
	}
	LOG(LM_NET, LL_TRACE, "send msg(%d) to peers(%d)",
		(int)e, (int)n->server->connectedPeers);

	// Find the peer and send
	for (int i = 0; i < (int)n->server->connectedPeers; i++)
	{
		ENetPeer *peer = n->server->peers + i;
		if (((NetPeerData *)peer->data)->Id == peerId)
		{
			enet_peer_send(peer, 0, NetEncode(e, data));
			enet_host_flush(n->server);
			return;
		}
	}
	CASSERT(false, "Cannot find peer by id");
}

void NetServerBroadcastMsg(NetServer *n, const GameEventType e, const void *data)
{
	if (!n->server)
	{
		return;
	}

	enet_host_broadcast(n->server, 0, NetEncode(e, data));
	enet_host_flush(n->server);
}
