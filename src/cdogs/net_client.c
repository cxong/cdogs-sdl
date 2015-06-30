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
static void OnReceive(NetClient *n, ENetEvent event)
{
	const GameEventType msg = (GameEventType)*(uint32_t *)event.packet->data;
	LOG(LM_NET, LL_TRACE, "recv msg(%u)", msg);
	const GameEventEntry gee = GameEventGetEntry(msg);
	if (gee.Enqueue)
	{
		if (gee.GameStart && !gMission.HasStarted)
		{
			LOG(LM_NET, LL_TRACE, "ignore game start gameEvent(%d)",
				(int)gee.Type);
		}
		else
		{
			// Game event message; decode and add to event queue
			LOG(LM_NET, LL_TRACE, "recv gameEvent(%d)", (int)gee.Type);
			GameEvent e = GameEventNew(gee.Type);
			if (gee.Fields != NULL)
			{
				NetDecode(event.packet, &e.u, gee.Fields);
			}

			// For actor events, check if UID is not for local player
			// TODO: repeated code (see game_events.c)
			int actorUID = -1;
			bool actorIsLocal = false;
			switch (gee.Type)
			{
			case GAME_EVENT_ACTOR_ADD:
				// Note: ignore checking this event
				break;
			case GAME_EVENT_ACTOR_MOVE: actorUID = e.u.ActorMove.UID; break;
			case GAME_EVENT_ACTOR_STATE: actorUID = e.u.ActorState.UID; break;
			case GAME_EVENT_ACTOR_DIR: actorUID = e.u.ActorDir.UID; break;
			case GAME_EVENT_ACTOR_SWITCH_GUN: actorUID = e.u.ActorSwitchGun.UID; break;
			case GAME_EVENT_ACTOR_PICKUP_ALL: actorUID = e.u.ActorPickupAll.UID; break;
			case GAME_EVENT_ACTOR_USE_AMMO: actorUID = e.u.UseAmmo.UID; break;
			case GAME_EVENT_ADD_BULLET:
				actorIsLocal = PlayerIsLocal(e.u.AddBullet.PlayerUID);
				break;
			default: break;
			}
			if (actorUID >= 0)
			{
				actorIsLocal = ActorIsLocalPlayer(actorUID);
			}
			if (actorIsLocal)
			{
				LOG(LM_NET, LL_TRACE, "game event is for local player, ignoring");
			}
			else
			{
				GameEventsEnqueue(&gGameEvents, e);
			}
		}
	}
	else
	{
		switch (gee.Type)
		{
		case GAME_EVENT_CLIENT_ID:
			{
				CASSERT(
					n->ClientId == -1,
					"unexpected client ID message, already set");
				NClientId cid;
				NetDecode(event.packet, &cid, NClientId_fields);
				LOG(LM_NET, LL_DEBUG, "recv clientId(%u) uid(%u)",
					cid.Id, cid.FirstPlayerUID);
				n->ClientId = (int)cid.Id;
				n->FirstPlayerUID = (int)cid.FirstPlayerUID;
			}
			break;
		case GAME_EVENT_CAMPAIGN_DEF:
			if (gCampaign.IsLoaded)
			{
				LOG(LM_NET, LL_INFO, "WARNING: unexpected campaign def msg received");
			}
			else
			{
				LOG(LM_NET, LL_DEBUG, "NetClient: received campaign def, loading...");
				NCampaignDef def;
				NetDecode(event.packet, &def, NCampaignDef_fields);
				CampaignEntry entry;
				if (CampaignEntryTryLoad(
						&entry, def.Path, (GameMode)def.GameMode) &&
					CampaignLoad(&gCampaign, &entry))
				{
					gCampaign.IsClient = true;
					gCampaign.MissionIndex = def.Mission;
				}
				else
				{
					printf("Error: failed to load campaign def\n");
					gCampaign.IsError = true;
				}
			}
			break;
		case GAME_EVENT_OBJECTIVE_COUNT:
			{
				NObjectiveCount oc;
				NetDecode(event.packet, &oc, NObjectiveCount_fields);
				ObjectiveDef *o =
					CArrayGet(&gMission.Objectives, oc.ObjectiveId);
				o->done = oc.Count;
				LOG(LM_NET, LL_DEBUG, "recv objective count id(%d) done(%d)",
					oc.ObjectiveId, o->done);
			}
			break;
		case GAME_EVENT_NET_GAME_START:
			LOG(LM_NET, LL_DEBUG, "NetClient: received game start");
			gMission.HasStarted = true;
			break;
		default:
			CASSERT(false, "unexpected message type");
			break;
		}
	}
	enet_packet_destroy(event.packet);
}

void NetClientSendMsg(NetClient *n, const GameEventType e, const void *data)
{
	if (!n->client || !n->peer)
	{
		return;
	}

	LOG(LM_NET, LL_DEBUG, "NetClient: send msg type %d", (int)e);
	enet_peer_send(n->peer, 0, NetEncode(e, data));
	enet_host_flush(n->client);
}

bool NetClientIsConnected(const NetClient *n)
{
	return !!n->client;
}
