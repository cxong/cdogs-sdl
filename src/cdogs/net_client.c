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
#include "net_client.h"

#include <string.h>

#include "proto/nanopb/pb_decode.h"
#include "proto/client.pb.h"
#include "campaigns.h"
#include "game_events.h"
#include "gamedata.h"
#include "log.h"
#include "net_server.h"
#include "player.h"
#include "utils.h"


NetClient gNetClient;


#define CONNECTION_WAIT_MS 5000


void NetClientInit(NetClient *n)
{
	memset(n, 0, sizeof *n);
	n->ClientId = -1;	// -1 is unset
}
void NetClientTerminate(NetClient *n)
{
	if (n->peer)
	{
		enet_peer_reset(n->peer);
	}
	n->peer = NULL;
	enet_host_destroy(n->client);
	n->client = NULL;
}

void NetClientConnect(NetClient *n, const ENetAddress addr)
{
	n->client = enet_host_create(
		NULL /* create a client host */,
		1 /* only allow 1 outgoing connection */,
		2 /* allow up 2 channels to be used, 0 and 1 */,
		57600 / 8 /* 56K modem with 56 Kbps downstream bandwidth */,
		14400 / 8 /* 56K modem with 14 Kbps upstream bandwidth */);
	if (n->client == NULL)
	{
		fprintf(stderr,
			"An error occurred while trying to create an ENet client host.\n");
		goto bail;
	}

	/* Initiate the connection, allocating the two channels 0 and 1. */
	n->peer = enet_host_connect(n->client, &addr, 2, 0);
	if (n->peer == NULL)
	{
		fprintf(stderr,
			"No available peers for initiating an ENet connection.\n");
		goto bail;
	}

	/* Wait milliseconds for the connection attempt to succeed. */
	ENetEvent event;
	if (enet_host_service(n->client, &event, CONNECTION_WAIT_MS) > 0 &&
		event.type == ENET_EVENT_TYPE_CONNECT)
	{
		fprintf(stderr, "Connected.\n");
	}
	else
	{
		/* Either the 5 seconds are up or a disconnect event was */
		/* received. Reset the peer in the event the 5 seconds   */
		/* had run out without any significant event.            */
		fprintf(stderr, "Connection failed.\n");
		goto bail;
	}
	return;

bail:
	NetClientTerminate(n);
}

static void OnReceive(NetClient *n, ENetEvent event);
void NetClientPoll(NetClient *n)
{
	if (!n->client || !n->peer)
	{
		return;
	}
	// Service the connection
	int check;
	do
	{
		ENetEvent event;
		check = enet_host_service(n->client, &event, 0);
		if (check < 0)
		{
			printf("Connection error %d\n", check);
			return;
		}
		else if (check > 0)
		{
			switch (event.type)
			{
			case ENET_EVENT_TYPE_RECEIVE:
				OnReceive(n, event);
				break;
			default:
				printf("Unexpected event type %d\n", event.type);
				break;
			}
		}
	} while (check > 0);
}
static void AddMissingPlayers(const int playerId);
static void OnReceive(NetClient *n, ENetEvent event)
{
	uint32_t msgType = *(uint32_t *)event.packet->data;
	LOG(LM_NET, LL_TRACE, "recv msgType(%u)", msgType);
	switch (msgType)
	{
	case MSG_CLIENT_ID:
		{
			CASSERT(
				n->ClientId == -1,
				"unexpected client ID message, already set");
			NetMsgClientId cid;
			NetDecode(event.packet, &cid, NetMsgClientId_fields);
			LOG(LM_NET, LL_DEBUG, "NetClient: received client ID %d", cid.Id);
			n->ClientId = cid.Id;
		}
		break;
	case MSG_CAMPAIGN_DEF:
		if (gCampaign.IsLoaded)
		{
			LOG(LM_NET, LL_INFO, "WARNING: unexpected campaign def msg received");
		}
		else
		{
			LOG(LM_NET, LL_DEBUG, "NetClient: received campaign def, loading...");
			NetMsgCampaignDef def;
			NetDecode(event.packet, &def, NetMsgCampaignDef_fields);
			char campaignPath[CDOGS_PATH_MAX];
			GameMode mode;
			NetMsgCampaignDefConvert(&def, campaignPath, &mode);
			CampaignEntry entry;
			if (CampaignEntryTryLoad(&entry, campaignPath, mode) &&
				CampaignLoad(&gCampaign, &entry))
			{
				gCampaign.IsClient = true;
			}
			else
			{
				printf("Error: failed to load campaign def\n");
				gCampaign.IsError = true;
			}
		}
		break;
	case MSG_PLAYER_DATA:
		{
			NetMsgPlayerData pd;
			NetDecode(event.packet, &pd, NetMsgPlayerData_fields);
			AddMissingPlayers(pd.PlayerIndex);
			LOG(LM_NET, LL_DEBUG, "recv player data name(%s) id(%d) total(%d)",
				pd.Name, pd.PlayerIndex, (int)gPlayerDatas.size);
			NetMsgPlayerDataUpdate(&pd);
		}
		break;
	case MSG_ADD_PLAYERS:
		{
			NetMsgAddPlayers ap;
			NetDecode(event.packet, &ap, NetMsgAddPlayers_fields);
			LOG(LM_NET, LL_DEBUG,
				"NetClient: received new players %d",
				(int)ap.PlayerIds_count);
			// Add new players
			// If they are local players, set them up with defaults
			const bool isLocal = ap.ClientId == n->ClientId;
			for (int i = 0; i < ap.PlayerIds_count; i++)
			{
				const int playerId = (int)ap.PlayerIds[i];
				AddMissingPlayers(playerId);
				PlayerData *p = CArrayGet(&gPlayerDatas, playerId);
				p->IsLocal = isLocal;
				if (isLocal)
				{
					PlayerDataSetLocalDefaults(p, i);
				}
			}
		}
		break;
	case MSG_GAME_START:
		LOG(LM_NET, LL_DEBUG, "NetClient: received game start");
		gMission.HasStarted = true;
		break;
	case MSG_ACTOR_ADD:
		{
			GameEvent e = GameEventNew(GAME_EVENT_ACTOR_ADD);
			NetDecode(event.packet, &e.u.ActorAdd, NetMsgActorAdd_fields);
			LOG(LM_NET, LL_DEBUG, "recv actor add uid(%d) playerId(%d)",
				e.u.ActorAdd.UID, e.u.ActorAdd.PlayerId);
			GameEventsEnqueue(&gGameEvents, e);
		}
		break;
	case MSG_ACTOR_MOVE:
		{
			GameEvent e = GameEventNew(GAME_EVENT_ACTOR_MOVE);
			NetDecode(event.packet, &e.u.ActorMove, NetMsgActorMove_fields);
			LOG(LM_NET, LL_DEBUG, "recv actor move UID(%d) pos(%d,%d)",
				(int)e.u.ActorMove.UID,
				(int)e.u.ActorMove.Pos.x, (int)e.u.ActorMove.Pos.y);
			if (!ActorIsLocalPlayer(e.u.ActorMove.UID))
			{
				GameEventsEnqueue(&gGameEvents, e);
			}
		}
		break;
	case MSG_ACTOR_STATE:
		{
			GameEvent e = GameEventNew(GAME_EVENT_ACTOR_STATE);
			NetDecode(event.packet, &e.u.ActorState, NetMsgActorState_fields);
			LOG(LM_NET, LL_DEBUG, "recv actor UID(%d) state(%d)",
				(int)e.u.ActorState.UID, (int)e.u.ActorState.State);
			if (!ActorIsLocalPlayer(e.u.ActorMove.UID))
			{
				GameEventsEnqueue(&gGameEvents, e);
			}
		}
		break;
	case MSG_ACTOR_DIR:
		{
			GameEvent e = GameEventNew(GAME_EVENT_ACTOR_DIR);
			NetDecode(event.packet, &e.u.ActorDir, NetMsgActorDir_fields);
			LOG(LM_NET, LL_DEBUG, "recv actor UID(%d) dir(%d)",
				(int)e.u.ActorDir.UID, (int)e.u.ActorDir.Dir);
			if (!ActorIsLocalPlayer(e.u.ActorMove.UID))
			{
				GameEventsEnqueue(&gGameEvents, e);
			}
		}
		break;
	case MSG_GAME_END:
		LOG(LM_NET, LL_DEBUG, "NetClient: received game end");
		gMission.isDone = true;
		break;
	default:
		CASSERT(false, "unexpected message type");
		break;
	}
	enet_packet_destroy(event.packet);
}
static void AddMissingPlayers(const int playerId)
{
	for (int i = (int)gPlayerDatas.size; i <= playerId; i++)
	{
		LOG(LM_NET, LL_DEBUG, "add remote playerId(%d)", playerId);
		PlayerData *p = PlayerDataAdd(&gPlayerDatas);
		p->IsLocal = false;
	}
}

void NetClientSendMsg(NetClient *n, const NetMsg msg, const void *data)
{
	if (!n->client || !n->peer)
	{
		return;
	}

	LOG(LM_NET, LL_DEBUG, "NetClient: send msg type %d", (int)msg);
	enet_peer_send(n->peer, 0, NetMakePacket(msg, data));
	enet_host_flush(n->client);
}

bool NetClientIsConnected(const NetClient *n)
{
	return !!n->client;
}
