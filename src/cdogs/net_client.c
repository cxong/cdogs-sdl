/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.

    Copyright (c) 2014-2016, Cong Xu
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
#define FIND_CONNECTION_WAIT_SECONDS 1
#define TIMEOUT_MS 5000


void NetClientInit(NetClient *n)
{
	memset(n, 0, sizeof *n);
	n->ClientId = -1;	// -1 is unset
	n->scanner = ENET_SOCKET_NULL;
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
	if (n->scanner != ENET_SOCKET_NULL)
	{
		if (enet_socket_shutdown(n->scanner, ENET_SOCKET_SHUTDOWN_READ_WRITE) != 0)
		{
			LOG(LM_NET, LL_ERROR, "Failed to shutdown listen socket");
		}
		enet_socket_destroy(n->scanner);
		n->scanner = ENET_SOCKET_NULL;
	}
}

static bool TryScanHost(NetClient *n, const enet_uint32 host);
void NetClientFindLANServers(NetClient *n)
{
	// If we've already begun scanning, wait for that to finish
	if (n->ScanTicks > 0)
	{
		return;
	}

	// Scan for servers on LAN using broadcast host
	if (!TryScanHost(n, ENET_HOST_BROADCAST))
	{
		return;
	}

	n->ScanTicks = FIND_CONNECTION_WAIT_SECONDS * FPS_FRAMELIMIT;
	n->ScannedAddr.host = 0;
	n->ScannedAddr.port = 0;
}
static bool TryScanHost(NetClient *n, const enet_uint32 host)
{
	// Create scanner socket if it has not been created yet
	if (n->scanner == ENET_SOCKET_NULL)
	{
		n->scanner = enet_socket_create(ENET_SOCKET_TYPE_DATAGRAM);
		if (n->scanner == ENET_SOCKET_NULL)
		{
			LOG(LM_NET, LL_ERROR, "Failed to create socket");
			goto bail;
		}
		if (enet_socket_set_option(n->scanner, ENET_SOCKOPT_BROADCAST, 1) != 0)
		{
			LOG(LM_NET, LL_ERROR, "Failed to enable broadcast socket");
			goto bail;
		}
	}

	// Send the scanning message
	ENetAddress addr;
	addr.host = host;
	addr.port = NET_LISTEN_PORT;
	// Send a dummy payload
	char data = 42;
	ENetBuffer sendbuf;
	sendbuf.data = &data;
	sendbuf.dataLength = 1;
	if (enet_socket_send(n->scanner, &addr, &sendbuf, 1) !=
		(int)sendbuf.dataLength)
	{
		LOG(LM_NET, LL_ERROR, "Failed to scan for server");
		goto bail;
	}

	return true;

bail:
	enet_socket_destroy(n->scanner);
	n->scanner = ENET_SOCKET_NULL;
	return false;
}

bool NetClientTryConnect(NetClient *n, const ENetAddress addr)
{
	NetClientDisconnect(n);

	char buf[256];
	enet_address_get_host_ip(&addr, buf, sizeof buf);
	LOG(LM_NET, LL_INFO, "Connecting client to %s:%u...", buf, addr.port);

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

	return NetClientIsConnected(n);

bail:
	NetClientDisconnect(n);
	return false;
}

static bool TryRecvScanForServerPort(
	NetClient *n, const int timeoutMs, ENetAddress *outAddr);
bool NetClientTryScanAndConnect(NetClient *n, const enet_uint32 host)
{
	if (!TryScanHost(n, host))
	{
		return false;
	}

	// Wait for the scan to return
	ENetAddress addr;
	if (!TryRecvScanForServerPort(n, CONNECTION_WAIT_MS, &addr))
	{
		return false;
	}

	// Connect to address
	return NetClientTryConnect(n, addr);
}
static bool TryRecvScanForServerPort(
	NetClient *n, const int timeoutMs, ENetAddress *outAddr)
{
	CASSERT(n->scanner != ENET_SOCKET_NULL,
		"cannot recv scans without scanner");
	// Check for the reply, which will give us the server address
	ENetSocketSet set;
	ENET_SOCKETSET_EMPTY(set);
	ENET_SOCKETSET_ADD(set, n->scanner);
	if (enet_socketset_select(n->scanner, &set, NULL, timeoutMs) <= 0)
	{
		return false;
	}

	// Receive the reply
	enet_uint16 recvport;
	ENetBuffer recvbuf;
	recvbuf.data = &recvport;
	recvbuf.dataLength = sizeof recvport;
	const int recvlen = enet_socket_receive(n->scanner, outAddr, &recvbuf, 1);
	if (recvlen <= 0)
	{
		return false;
	}
	outAddr->port = recvport;
	char ipbuf[256];
	enet_address_get_host_ip(outAddr, ipbuf, sizeof ipbuf);
	LOG(LM_NET, LL_DEBUG, "Found server at %s:%u", ipbuf, outAddr->port);
	return true;
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
static void Scanning(NetClient *n);
void NetClientPoll(NetClient *n)
{
	// Check to see if LAN servers have been scanned
	Scanning(n);

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
			case ENET_EVENT_TYPE_RECEIVE:
				OnReceive(n, event);
				break;
			case ENET_EVENT_TYPE_DISCONNECT:
				LOG(LM_NET, LL_INFO, "disconnected");
				NetClientDisconnect(n);
				return;
			default:
				LOG(LM_NET, LL_ERROR, "Unexpected event type(%d)", (int)event.type);
				break;
			}
		}
	} while (check > 0);
}
static void Scanning(NetClient *n)
{
	// If it's been too long, stop scanning
	if (n->ScanTicks <= 0)
	{
		return;
	}
	n->ScanTicks--;

	// Check to see if we have received the scan reply
	TryRecvScanForServerPort(n, 0, &n->ScannedAddr);
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
