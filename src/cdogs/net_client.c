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
#include "actors.h"
#include "campaigns.h"
#include "game_events.h"
#include "gamedata.h"
#include "log.h"
#include "net_server.h"
#include "player.h"
#include "utils.h"


NetClient gNetClient;


#define CONNECTION_WAIT_MS 5000
#define FIND_CONNECTION_WAIT_MS 1000
#define TIMEOUT_MS 5000


void NetClientInit(NetClient *n)
{
	memset(n, 0, sizeof *n);
	n->ClientId = -1;	// -1 is unset
	n->client = enet_host_create(NULL, 1, 2,
		57600 / 8 /* 56K modem with 56 Kbps downstream bandwidth */,
		14400 / 8 /* 56K modem with 14 Kbps upstream bandwidth */);
	if (n->client == NULL)
	{
		LOG(LM_NET, LL_ERROR, "cannot create ENet client host");
	}
}
void NetClientTerminate(NetClient *n)
{
	NetClientDisconnect(n);
	n->peer = NULL;
	enet_host_destroy(n->client);
	n->client = NULL;
}

void NetClientFindLANServers(NetClient *n)
{
	if (n->client == NULL)
	{
		LOG(LM_NET, LL_ERROR, "cannot look for LAN servers; host not created");
		return;
	}
	if (n->peer)
	{
		LOG(LM_NET, LL_TRACE, "cannot look for LAN servers when connected");
		return;
	}

	// Set to finding mode; here we only connect and disconnect as soon as
	// it is successful
	n->FindingLANServer = true;
	n->FoundLANServer = false;

	ENetAddress addr = NetClientLANAddress();
	n->peer = enet_host_connect(n->client, &addr, 2, 0);
	if (n->peer == NULL)
	{
		LOG(LM_NET, LL_INFO, "failed to connect to LAN servers");
	}
	enet_peer_timeout(n->peer, 0, 0, FIND_CONNECTION_WAIT_MS);
}

void NetClientConnect(NetClient *n, const ENetAddress addr)
{
	// Note: we can be connected from searching for servers
	NetClientDisconnect(n);

	/* Initiate the connection, allocating the two channels 0 and 1. */
	n->peer = enet_host_connect(n->client, &addr, 2, 0);
	if (n->peer == NULL)
	{
		LOG(LM_NET, LL_WARN, "No server connection found");
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
		LOG(LM_NET, LL_WARN, "connection failed");
		goto bail;
	}

	// Set disconnect timeout ms
	enet_peer_timeout(n->peer, 0, 0, TIMEOUT_MS);

	// Tell the server that this is a proper connection request
	NetClientSendMsg(n, GAME_EVENT_CLIENT_CONNECT, NULL);

	return;

bail:
	NetClientDisconnect(n);
}
void NetClientDisconnect(NetClient *n)
{
	if (n->peer)
	{
		LOG(LM_NET, LL_INFO, "disconnecting peer");
		enet_peer_disconnect_now(n->peer, 0);
		n->peer = NULL;
	}
	n->ClientId = -1;	// -1 is unset
	n->Ready = false;
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
			LOG(LM_NET, LL_ERROR, "connection error(%d)", check);
			NetClientTerminate(n);
			return;
		}
		else if (check > 0)
		{
			switch (event.type)
			{
			case ENET_EVENT_TYPE_CONNECT:
				n->FoundLANServer = true;
				if (n->FindingLANServer)
				{
					LOG(LM_NET, LL_INFO,
						"found server; disconnecting %u.%u.%u.%u:%d",
						NET_IP_TO_CIDR_FORMAT(event.peer->address.host),
						(int)event.peer->address.port);
					// Disconnect politely and wait for the disconnection event
					enet_peer_disconnect(n->peer, 0);
				}
				break;
			case ENET_EVENT_TYPE_RECEIVE:
				OnReceive(n, event);
				break;
			case ENET_EVENT_TYPE_DISCONNECT:
				LOG(LM_NET, LL_INFO, "disconnected");
				NetClientDisconnect(n);
				n->FindingLANServer = false;
				return;
			default:
				LOG(LM_NET, LL_ERROR, "Unexpected event type(%d)", (int)event.type);
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
			case GAME_EVENT_ACTOR_SLIDE: actorUID = e.u.ActorSlide.UID; break;
			case GAME_EVENT_ACTOR_SWITCH_GUN: actorUID = e.u.ActorSwitchGun.UID; break;
			case GAME_EVENT_ACTOR_PICKUP_ALL: actorUID = e.u.ActorPickupAll.UID; break;
			case GAME_EVENT_ACTOR_USE_AMMO: actorUID = e.u.UseAmmo.UID; break;
			case GAME_EVENT_ACTOR_MELEE: actorUID = e.u.Melee.UID; break;
			case GAME_EVENT_GUN_FIRE:
				if (e.u.GunFire.IsGun)
				{
					actorIsLocal = PlayerIsLocal(e.u.GunFire.PlayerUID);
				}
				break;
			case GAME_EVENT_GUN_RELOAD:
				actorIsLocal = PlayerIsLocal(e.u.GunReload.PlayerUID);
				break;
			case GAME_EVENT_GUN_STATE: actorUID = e.u.GunState.ActorUID; break;
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
				gCampaign.Entry.Mode = (GameMode)def.GameMode;
				CampaignEntry entry;
				if (CampaignEntryTryLoad(
						&entry, def.Path, GAME_MODE_NORMAL) &&
					CampaignLoad(&gCampaign, &entry))
				{
					gCampaign.IsClient = true;
					gCampaign.MissionIndex = def.Mission;
				}
				else
				{
					LOG(LM_NET, LL_ERROR, "failed to load campaign def");
					gCampaign.IsError = true;
				}
			}
			break;
		case GAME_EVENT_NET_GAME_START:
			LOG(LM_NET, LL_DEBUG, "NetClient: received game start");
			// Don't ready-up unless we're ready
			if (n->Ready)
			{
				gMission.HasStarted = true;
			}
			break;
		default:
			CASSERT(false, "unexpected message type");
			break;
		}
	}
	enet_packet_destroy(event.packet);
}

void NetClientFlush(NetClient *n)
{
	if (n->client == NULL) return;
	enet_host_flush(n->client);
}

void NetClientSendMsg(NetClient *n, const GameEventType e, const void *data)
{
	if (!NetClientIsConnected(n))
	{
		return;
	}

	LOG(LM_NET, LL_TRACE, "NetClient: send msg type %d", (int)e);
	enet_peer_send(n->peer, 0, NetEncode(e, data));
}

bool NetClientIsConnected(const NetClient *n)
{
	return n->client && n->peer;
}

ENetAddress NetClientLANAddress(void)
{
	ENetAddress addr;
	if (enet_address_set_host(&addr, "127.0.0.1") != 0)
	{
		CASSERT(false, "failed to set host");
	}
	addr.port = NET_PORT;
	return addr;
}
