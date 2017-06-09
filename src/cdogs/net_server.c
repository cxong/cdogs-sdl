/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.

    Copyright (c) 2014-2017, Cong Xu
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
#include "ai_utils.h"
#include "campaign_entry.h"
#include "events.h"
#include "game_events.h"
#include "gamedata.h"
#include "handle_game_events.h"
#include "log.h"
#include "los.h"
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
	NetServerClose(n);
}
void NetServerReset(NetServer *n)
{
	n->PrevCmd = n->Cmd = 0;
}

static ENetHost *HostOpen(void);
static bool ListenSocketTryOpen(ENetSocket *listen);
void NetServerOpen(NetServer *n)
{
	if (n->server)
	{
		return;
	}
	
	n->server = HostOpen();
	if (n->server == NULL)
	{
		return;
	}

	// Start listen socket, to respond to UDP scans
	if (!ListenSocketTryOpen(&n->listen))
	{
		return;
	}

	// Save limited length hostname for LAN scanners
	// This is because getting hostname locally is much faster
	if (enet_address_get_host(
		&n->server->address, n->hostname, sizeof n->hostname) != 0)
	{
		LOG(LM_NET, LL_WARN, "Failed to get hostname");
		n->hostname[0] = '\0';
	}
}
static ENetHost *HostOpen(void)
{
	ENetAddress address;
	address.host = ENET_HOST_ANY;
	address.port = ENET_PORT_ANY;
	ENetHost *host = enet_host_create(&address, NET_SERVER_MAX_CLIENTS, 2, 0, 0);
	if (host == NULL)
	{
		LOG(LM_NET, LL_ERROR, "cannot create server host");
		return NULL;
	}
	else
	{
		char buf[256];
		enet_address_get_host_ip(&host->address, buf, sizeof buf);
		LOG(LM_NET, LL_INFO, "starting server on %s:%u",
			buf, host->address.port);
	}
	return host;
}
static bool ListenSocketTryOpen(ENetSocket *listen)
{
	*listen = enet_socket_create(ENET_SOCKET_TYPE_DATAGRAM);
	if (*listen == ENET_SOCKET_NULL)
	{
		LOG(LM_NET, LL_ERROR, "Failed to create socket");
		return false;
	}
	if (enet_socket_set_option(*listen, ENET_SOCKOPT_REUSEADDR, 1) != 0)
	{
		LOG(LM_NET, LL_ERROR, "Failed to enable reuse address");
		return false;
	}
	ENetAddress addr;
	addr.host = ENET_HOST_ANY;
	addr.port = NET_LISTEN_PORT;
	if (enet_socket_bind(*listen, &addr) != 0)
	{
		LOG(LM_NET, LL_ERROR, "failed to bind listen socket");
		return false;
	}
	if (enet_socket_get_address(*listen, &addr) != 0)
	{
		LOG(LM_NET, LL_ERROR, "cannot get listen socket address");
		return false;
	}
	LOG(LM_NET, LL_INFO, "listening for scans on port %u", addr.port);
	return true;
}

void NetServerClose(NetServer *n)
{
	if (n->server)
	{
		for (int i = 0; i < (int)n->server->peerCount; i++)
		{
			ENetPeer *peer = n->server->peers + i;
			enet_peer_disconnect_now(peer, 0);
		}
		enet_host_destroy(n->server);
	}
	n->server = NULL;
}

static void PollListener(NetServer *n);
static void OnReceive(NetServer *n, ENetEvent event);
static void OnDisconnect(const ENetEvent event);
void NetServerPoll(NetServer *n)
{
	if (!n->server)
	{
		return;
	}

	// Check our listening socket for scanning clients
	PollListener(n);

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
				{
					char buf[256];
					enet_address_get_host_ip(
						&event.peer->address, buf, sizeof buf);
					LOG(LM_NET, LL_INFO, "client connection from %s:%u",
						buf, event.peer->address.port);
				}
				break;
			case ENET_EVENT_TYPE_RECEIVE:
				OnReceive(n, event);
				break;
			case ENET_EVENT_TYPE_DISCONNECT:
				OnDisconnect(event);
				break;
			default:
				CASSERT(false, "Unknown event");
				break;
			}
		}
	} while (check > 0);

	NetServerFlush(n);
}
static void PollListener(NetServer *n)
{
	// Check for data to recv
	ENetSocketSet set;
	ENET_SOCKETSET_EMPTY(set);
	ENET_SOCKETSET_ADD(set, n->listen);
	if (enet_socketset_select(n->listen, &set, NULL, 0) <= 0)
	{
		return;
	}

	ENetAddress addr;
	char buf;
	ENetBuffer recvbuf;
	recvbuf.data = &buf;
	recvbuf.dataLength = 1;
	const int recvlen = enet_socket_receive(n->listen, &addr, &recvbuf, 1);
	if (recvlen <= 0)
	{
		return;
	}
	char addrbuf[256];
	enet_address_get_host_ip(&addr, addrbuf, sizeof addrbuf);
	LOG(LM_NET, LL_DEBUG, "listener received from %s:%u", addrbuf, addr.port);
	// Reply to scanner client with our server host/address
	NServerInfo sinfo;
	sinfo.ProtocolVersion = NET_PROTOCOL_VERSION;
	sinfo.ENetPort = n->server->address.port;
	sinfo.Hostname[0] = '\0';
	strncat(sinfo.Hostname, n->hostname, sizeof sinfo.Hostname - 1);
	sinfo.GameMode = gCampaign.Entry.Mode;
	sinfo.CampaignName[0] = '\0';
	strncat(sinfo.CampaignName, gCampaign.Setting.Title,
		sizeof sinfo.CampaignName - 1);
	sinfo.MissionNumber = gCampaign.MissionIndex + 1;
	sinfo.NumPlayers = GetNumPlayers(PLAYER_ANY, false, false);
	sinfo.MaxPlayers =
		NET_SERVER_MAX_CLIENTS * MAX_LOCAL_PLAYERS +
		GetNumPlayers(PLAYER_ANY, false, true);
	// Encode our packet
	uint8_t encbuf[1024];
	pb_ostream_t stream = pb_ostream_from_buffer(encbuf, sizeof encbuf);
	const bool status = pb_encode(&stream, NServerInfo_fields, &sinfo);
	CASSERT(status, "Failed to encode pb");
	recvbuf.data = encbuf;
	recvbuf.dataLength = stream.bytes_written;
	if (enet_socket_send(n->listen, &addr, &recvbuf, 1) != (int)recvbuf.dataLength)
	{
		LOG(LM_NET, LL_ERROR, "Failed to reply to scanner");
	}
}
static void OnConnect(NetServer *n, ENetEvent event);
static void OnReceive(NetServer *n, ENetEvent event)
{
	const GameEventType msg = (GameEventType)*(uint32_t *)event.packet->data;
	int peerId = -1;
	if (event.peer->data != NULL)
	{
		// We may not have assigned peer ID
		peerId = ((NetPeerData *)event.peer->data)->Id;
		LOG(LM_NET, LL_TRACE, "recv message from peerId(%d) msg(%d)",
			peerId, (int)msg);
	}
	const GameEventEntry gee = GameEventGetEntry(msg);
	if (gee.Enqueue)
	{
		// Game event message; decode and add to event queue
		LOG(LM_NET, LL_TRACE, "recv gameEvent(%d)", (int)gee.Type);
		GameEvent e = GameEventNew(gee.Type);
		NetDecode(event.packet, &e.u, gee.Fields);
		GameEventsEnqueue(&gGameEvents, e);
	}
	else
	{
		switch (gee.Type)
		{
		case GAME_EVENT_CLIENT_CONNECT:
			OnConnect(n, event);
			break;
		case GAME_EVENT_CLIENT_READY:
			CASSERT(peerId >= 0, "peer id unset");
			// Flush game events to make sure we add the players
			HandleGameEvents(&gGameEvents, NULL, NULL, NULL);
			// Reset player data
			for (int i = 0; i < MAX_LOCAL_PLAYERS; i++)
			{
				const int cid = (peerId + 1) * MAX_LOCAL_PLAYERS + i;
				const PlayerData *pData = PlayerDataGetByUID(cid);
				if (pData == NULL) continue;
				GameEvent e = GameEventNew(GAME_EVENT_PLAYER_DATA);
				e.u.PlayerData = PlayerDataMissionReset(pData);
				GameEventsEnqueue(&gGameEvents, e);
			}
			// Flush game events to make sure we reset player data
			HandleGameEvents(&gGameEvents, NULL, NULL, NULL);

			// Send game start messages if we've started already
			if (gMission.HasStarted)
			{
				NetServerSendGameStartMessages(n, peerId);

				// Find a current player to spawn next to
				Vec2i defaultSpawnPosition = Vec2iZero();
				const TActor *closestActor = AIGetClosestPlayer(Vec2iZero());
				if (closestActor != NULL) defaultSpawnPosition = closestActor->Pos;

				// Add the client's actors
				for (int i = 0; i < MAX_LOCAL_PLAYERS; i++)
				{
					const int cid = (peerId + 1) * MAX_LOCAL_PLAYERS + i;
					const PlayerData *pData = PlayerDataGetByUID(cid);
					if (pData != NULL)
					{
						defaultSpawnPosition = PlacePlayer(
							&gMap, pData, defaultSpawnPosition, true);
					}
				}
			}
			if (gMission.HasBegun)
			{
				NGameBegin gb = NGameBegin_init_default;
				gb.MissionTime = gMission.time;
				NetServerSendMsg(n, peerId, GAME_EVENT_GAME_BEGIN, &gb);
			}

			NetServerFlush(n);
			break;
		default:
			CASSERT(false, "unexpected message type");
			break;
		}
	}
	enet_packet_destroy(event.packet);
}
static void OnConnect(NetServer *n, ENetEvent event)
{
	char buf[256];
	enet_address_get_host_ip(&event.peer->address, buf, sizeof buf);
	LOG(LM_NET, LL_INFO, "new client connected from %s:%u",
		buf, event.peer->address.port);
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

	NetServerFlush(n);
}
static void OnDisconnect(const ENetEvent event)
{
	int peerId = -1;
	if (event.peer->data != NULL)
	{
		peerId = ((NetPeerData *)event.peer->data)->Id;
		CFREE(event.peer->data);
		event.peer->data = NULL;
	}
	CASSERT(peerId >= 0, "Cannot find disconnected peer id");
	char buf[256];
	enet_address_get_host_ip(&event.peer->address, buf, sizeof buf);
	LOG(LM_NET, LL_INFO, "peerId(%d) disconnected %s:%u",
		peerId, buf, event.peer->address.port);
	// Remove client's players
	for (int i = 0; i < MAX_LOCAL_PLAYERS; i++)
	{
		GameEvent e = GameEventNew(GAME_EVENT_PLAYER_REMOVE);
		e.u.PlayerRemove.UID = (peerId + 1) * MAX_LOCAL_PLAYERS + i;
		GameEventsEnqueue(&gGameEvents, e);
	}
}

void NetServerFlush(NetServer *n)
{
	if (n->server == NULL) return;
	enet_host_flush(n->server);
}

static void SendConfig(
	Config *config, const char *name, NetServer *n, const int peerId);
void NetServerSendGameStartMessages(NetServer *n, const int peerId)
{
	// Send details of all current players
	CA_FOREACH(const PlayerData, pOther, gPlayerDatas)
		NPlayerData pd = NMakePlayerData(pOther);
		NetServerSendMsg(n, peerId, GAME_EVENT_PLAYER_DATA, &pd);
		LOG(LM_NET, LL_DEBUG, "send player data uid(%d) maxHealth(%d)",
			(int)pd.UID, (int)pd.MaxHealth);
	CA_FOREACH_END()

	// Send all game-specific config values
	SendConfig(&gConfig, "Game.FriendlyFire", n, peerId);
	SendConfig(&gConfig, "Game.FPS", n, peerId);
	SendConfig(&gConfig, "Game.Ammo", n, peerId);
	SendConfig(&gConfig, "Game.Fog", n, peerId);
	SendConfig(&gConfig, "Game.SightRange", n, peerId);
	SendConfig(&gConfig, "Game.AllyCollision", n, peerId);

	NetServerSendMsg(n, peerId, GAME_EVENT_NET_GAME_START, NULL);

	// Send all actors
	CA_FOREACH(const TActor, a, gActors)
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
		LOG(LM_NET, LL_DEBUG, "send add actor UID(%d) playerUID(%d)",
			(int)aa.UID, (int)aa.PlayerUID);
		NetServerSendMsg(n, peerId, GAME_EVENT_ACTOR_ADD, &aa);
	CA_FOREACH_END()

	// Send key state
	NAddKeys ak = NAddKeys_init_default;
	ak.KeyFlags = gMission.KeyFlags;
	NetServerSendMsg(n, peerId, GAME_EVENT_ADD_KEYS, &ak);

	// Send objective counts
	CA_FOREACH(const Objective, o, gMission.missionData->Objectives)
		NObjectiveUpdate ou = NObjectiveUpdate_init_default;
		ou.ObjectiveId = _ca_index;
		ou.Count = o->done;
		NetServerSendMsg(n, peerId, GAME_EVENT_OBJECTIVE_UPDATE, &ou);
	CA_FOREACH_END()

	// Send all tiles, RLE
	const Tile *tLast = NULL;
	NTileSet ts = NTileSet_init_default;
	Vec2i pos;
	for (pos.y = 0; pos.y < gMap.Size.y; pos.y++)
	{
		for (pos.x = 0; pos.x < gMap.Size.x; pos.x++)
		{
			const Tile *t = MapGetTile(&gMap, pos);
			// Use RLE, so check if the current tile is the same as the last
			if (tLast != NULL &&
				t->pic == tLast->pic && t->picAlt == tLast->picAlt &&
				t->flags == tLast->flags)
			{
				ts.RunLength++;
			}
			else
			{
				// Send the last run
				if (tLast != NULL)
				{
					NetServerSendMsg(n, peerId, GAME_EVENT_TILE_SET, &ts);
				}
				// Begin the next run
				memset(&ts, 0, sizeof ts);
				ts.Pos = Vec2i2Net(pos);
				if (t->pic != NULL) strcpy(ts.PicName, t->pic->name);
				if (t->picAlt != NULL) strcpy(ts.PicAltName, t->picAlt->name);
				ts.Flags = t->flags;
				ts.RunLength = 0;
			}
			tLast = t;
		}
	}
	NetServerSendMsg(n, peerId, GAME_EVENT_TILE_SET, &ts);

	// Send all the tiles visited so far
	NExploreTiles et = NExploreTiles_init_default;
	et.Runs_count = 0;
	et.Runs[0].Run = 0;
	bool run = false;
	for (pos.y = 0; pos.y < gMap.Size.y; pos.y++)
	{
		for (pos.x = 0; pos.x < gMap.Size.x; pos.x++)
		{
			const Tile *t = MapGetTile(&gMap, pos);
			if (LOSAddRun(&et, &run, pos, t->isVisited))
			{
				NetServerSendMsg(n, peerId, GAME_EVENT_EXPLORE_TILES, &et);
				et.Runs_count = 0;
				et.Runs[0].Run = 0;
				run = false;
			}
		}
	}
	if (et.Runs_count > 0)
	{
		NetServerSendMsg(n, peerId, GAME_EVENT_EXPLORE_TILES, &et);
	}

	// Send all pickups
	CA_FOREACH(const Pickup, p, gPickups)
		if (!p->isInUse) continue;
		NAddPickup api = NAddPickup_init_default;
		api.UID = p->UID;
		strcpy(api.PickupClass, p->class->Name);
		api.IsRandomSpawned = p->IsRandomSpawned;
		api.SpawnerUID = p->SpawnerUID;
		api.TileItemFlags = p->tileItem.flags;
		api.Pos = Vec2i2Net(Vec2iNew(p->tileItem.x, p->tileItem.y));
		NetServerSendMsg(n, peerId, GAME_EVENT_ADD_PICKUP, &api);
	CA_FOREACH_END()

	// Send all map objects
	CA_FOREACH(const TObject, o, gObjs)
		if (!o->isInUse) continue;
		NMapObjectAdd amo = NMapObjectAdd_init_default;
		amo.UID = o->uid;
		strcpy(amo.MapObjectClass, o->Class->Name);
		amo.Pos = Vec2i2Net(Vec2iNew(o->tileItem.x, o->tileItem.y));
		amo.TileItemFlags = o->tileItem.flags;
		amo.Health = o->Health;
		LOG(LM_NET, LL_DEBUG,
			"send add map object UID(%d) pos(%d, %d) flags(%x) health(%d)",
			(int)amo.UID, amo.Pos.x, amo.Pos.y, amo.TileItemFlags, amo.Health);
		NetServerSendMsg(n, peerId, GAME_EVENT_MAP_OBJECT_ADD, &amo);
	CA_FOREACH_END()

	// If mission complete already, send message
	if (CanCompleteMission(&gMission))
	{
		NMissionComplete mc = NMakeMissionComplete(&gMission, &gMap);
		NetServerSendMsg(n, peerId, GAME_EVENT_MISSION_COMPLETE, &mc);
	}
}
static void SendConfig(
	Config *config, const char *name, NetServer *n, const int peerId)
{
	NConfig msg = NConfig_init_default;
	const Config *c = ConfigGet(config, name);
	strcpy(msg.Name, name);
	switch (c->Type)
	{
	case CONFIG_TYPE_STRING:
		CASSERT(false, "unimplemented");
		break;
	case CONFIG_TYPE_INT:
		sprintf(msg.Value, "%d", c->u.Int.Value);
		break;
	case CONFIG_TYPE_FLOAT:
		sprintf(msg.Value, "%f", c->u.Float.Value);
		break;
	case CONFIG_TYPE_BOOL:
		strcpy(msg.Value, c->u.Bool.Value ? "true" : "false");
		break;
	case CONFIG_TYPE_ENUM:
		sprintf(msg.Value, "%d", (int)c->u.Enum.Value);
		break;
	case CONFIG_TYPE_GROUP:
		CASSERT(false, "Cannot send groups over net");
		break;
	default:
		CASSERT(false, "Unknown config type");
		break;
	}
	NetServerSendMsg(n, peerId, GAME_EVENT_CONFIG, &msg);
}

void NetServerSendMsg(
	NetServer *n, const int peerId, const GameEventType e, const void *data)
{
	if (!n->server) return;

	if (peerId >= 0)
	{
		LOG(LM_NET, LL_TRACE, "send msg(%d) to peers(%d)",
			(int)e, (int)n->server->connectedPeers);
		// Find the peer and send
		for (int i = 0; i < (int)n->server->peerCount; i++)
		{
			ENetPeer *peer = n->server->peers + i;
			if (peer->data != NULL &&
				((NetPeerData *)peer->data)->Id == peerId)
			{
				enet_peer_send(peer, 0, NetEncode(e, data));
				return;
			}
		}
		CASSERT(false, "Cannot find peer by id");
	}
	else
	{
		LOG(LM_NET, LL_TRACE, "bcast msg(%d) to peers(%d)",
			(int)e, (int)n->server->connectedPeers);
		enet_host_broadcast(n->server, 0, NetEncode(e, data));
	}
}
