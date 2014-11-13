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
#include "net_server.h"

#include <string.h>

#include "proto/nanopb/pb_encode.h"
#include "proto/client.pb.h"

#include "actor_placement.h"
#include "campaign_entry.h"
#include "game_events.h"
#include "gamedata.h"
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
	ENetEvent event;
	int check;
	do
	{
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
	printf("A new client connected from %x:%u.\n",
		event.peer->address.host,
		event.peer->address.port);
	/* Store any relevant client information here. */
	CMALLOC(event.peer->data, sizeof(NetPeerData));
	const int peerId = n->peerId;
	((NetPeerData *)event.peer->data)->Id = peerId;
	n->peerId++;

	// Send the client ID
	debug(D_VERBOSE, "NetServer: sending client ID %d", peerId);
	NetServerSendMsg(n, peerId, SERVER_MSG_CLIENT_ID, &peerId);

	// Send the current campaign details over
	debug(D_VERBOSE, "NetServer: sending campaign entry");
	NetServerSendMsg(n, peerId, SERVER_MSG_CAMPAIGN_DEF, &gCampaign.Entry);
	// Send details of all current players
	for (int i = 0; i < (int)gPlayerDatas.size; i++)
	{
		const PlayerData *pOther = CArrayGet(&gPlayerDatas, i);
		debug(D_VERBOSE, "NetServer: sending player data index %d", i);
		NetServerSendMsg(n, peerId, SERVER_MSG_PLAYER_DATA, pOther);
	}

	SoundPlay(&gSoundDevice, StrSound("hahaha"));
	debug(D_VERBOSE, "NetServer: client connection complete");
}
static void SendGameStartMessages(
	NetServer *n, const int peerId, const PlayerData *pData);
static void OnReceive(NetServer *n, ENetEvent event)
{
	uint32_t msgType = *(uint32_t *)event.packet->data;
	const int peerId = ((NetPeerData *)event.peer->data)->Id;
	debug(D_NORMAL, "NetServer: Received message from client %d type %u\n",
		peerId, msgType);
	switch (msgType)
	{
	case CLIENT_MSG_NEW_PLAYERS:
		{
			NetMsgNewPlayers np;
			NetDecode(event.packet, &np, NetMsgNewPlayers_fields);
			debug(D_VERBOSE, "NetServer: received new players %d", np.NumPlayers);
			// Add the players, simultaneously building our broadcast message
			NetMsgAddPlayers ap;
			ap.ClientId = np.ClientId;
			ap.PlayerIds_count = (pb_size_t)np.NumPlayers;
			for (int i = 0; i < np.NumPlayers; i++)
			{
				PlayerData *p = PlayerDataAdd(&gPlayerDatas);
				p->IsLocal = false;
				ap.PlayerIds[i] = p->playerIndex;
			}
			// Broadcast the new players to all clients
			NetServerBroadcastMsg(n, SERVER_MSG_ADD_PLAYERS, &ap);
		}
		break;
	case CLIENT_MSG_PLAYER_DATA:
		{
			NetMsgPlayerData pd;
			NetDecode(event.packet, &pd, NetMsgPlayerData_fields);
			debug(D_VERBOSE,
				"NetServer: received player data id %d", pd.PlayerIndex);
			CASSERT(
				(int)gPlayerDatas.size > (int)pd.PlayerIndex,
				"unexpected player index");
			NetMsgPlayerDataUpdate(&pd);
			// Send it on to all clients
			const PlayerData *pData = CArrayGet(&gPlayerDatas, pd.PlayerIndex);
			NetServerBroadcastMsg(n, SERVER_MSG_PLAYER_DATA, pData);

			// Send game start messages if we've started already
			if (gMission.HasStarted)
			{
				SendGameStartMessages(n, peerId, pData);
			}
		}
		break;
	default:
		CASSERT(false, "unexpected message type");
		break;
	}
	enet_packet_destroy(event.packet);
}
static void SendGameStartMessages(
	NetServer *n, const int peerId, const PlayerData *pData)
{
	// Add the player's actor
	PlacePlayer(&gMap, pData, Vec2iZero(), true);

	// Send all players
	for (int i = 0; i < (int)gActors.size; i++)
	{
		const TActor *a = CArrayGet(&gActors, i);
		if (!a->isInUse)
		{
			continue;
		}
		NetMsgActorAdd aa = NetMsgActorAdd_init_default;
		aa.Id = a->tileItem.id;
		if (a->playerIndex < 0)
		{
			aa.CharId =
				a->Character -
				(Character *)gCampaign.Setting.characters.OtherChars.data;
		}
		aa.Health = a->health;
		aa.Direction = (int32_t)a->direction;
		aa.PlayerId = a->playerIndex;
		aa.TileItemFlags = a->tileItem.flags;
		aa.FullPos.x = a->Pos.x;
		aa.FullPos.y = a->Pos.y;
		NetServerSendMsg(&gNetServer, peerId, SERVER_MSG_ACTOR_ADD, &aa);
	}

	NetServerSendMsg(n, peerId, SERVER_MSG_GAME_START, NULL);
}

static ENetPacket *MakePacket(ServerMsg msg, const void *data);

void NetServerSendMsg(
	NetServer *n, const int peerId, ServerMsg msg, const void *data)
{
	if (!n->server)
	{
		return;
	}

	// Find the peer and send
	for (int i = 0; i < (int)n->server->connectedPeers; i++)
	{
		ENetPeer *peer = n->server->peers + i;
		if (((NetPeerData *)peer->data)->Id == peerId)
		{
			enet_peer_send(peer, 0, MakePacket(msg, data));
			enet_host_flush(n->server);
			return;
		}
	}
	CASSERT(false, "Cannot find peer by id");
}

void NetServerBroadcastMsg(NetServer *n, ServerMsg msg, const void *data)
{
	if (!n->server)
	{
		return;
	}

	enet_host_broadcast(n->server, 0, MakePacket(msg, data));
	enet_host_flush(n->server);
}

static ENetPacket *MakePacket(ServerMsg msg, const void *data)
{
	switch (msg)
	{
	case SERVER_MSG_CLIENT_ID:
		{
			NetMsgClientId cid;
			cid.Id = *(const int *)data;
			return NetEncode((int)msg, &cid, NetMsgClientId_fields);
		}
	case SERVER_MSG_CAMPAIGN_DEF:
		{
			NetMsgCampaignDef def;
			memset(&def, 0, sizeof def);
			const CampaignEntry *entry = data;
			if (entry->Path)
			{
				strcpy((char *)def.Path, entry->Path);
			}
			def.CampaignMode = entry->Mode;
			return NetEncode((int)msg, &def, NetMsgCampaignDef_fields);
		}
	case SERVER_MSG_PLAYER_DATA:
		{
			NetMsgPlayerData d = NetMsgMakePlayerData(data);
			return NetEncode((int)msg, &d, NetMsgPlayerData_fields);
		}
	case SERVER_MSG_ADD_PLAYERS:
		return NetEncode((int)msg, data, NetMsgAddPlayers_fields);
	case SERVER_MSG_GAME_START:
		return NetEncode((int)msg, NULL, 0);
	case SERVER_MSG_ACTOR_ADD:
		return NetEncode((int)msg, data, NetMsgActorAdd_fields);
	case SERVER_MSG_ACTOR_MOVE:
		return NetEncode((int)msg, data, NetMsgActorMove_fields);
	case SERVER_MSG_GAME_END:
		return NetEncode((int)msg, NULL, 0);
	default:
		CASSERT(false, "Unknown message to make into packet");
		return NULL;
	}
}
