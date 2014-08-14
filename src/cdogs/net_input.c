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
#include "net_input.h"

#include <string.h>

#include "campaign_entry.h"
#include "sys_config.h"
#include "utils.h"


void NetInputInit(NetInput *n)
{
	memset(n, 0, sizeof *n);
	CArrayInit(&n->peers, sizeof(ENetPeer *));
}
void NetInputTerminate(NetInput *n)
{
	enet_host_destroy(n->server);
	n->server = NULL;
	for (int i = 0; i < (int)n->peers.size; i++)
	{
		ENetPeer **p = CArrayGet(&n->peers, i);
		if (*p)
		{
			enet_peer_reset(*p);
		}
	}
	CArrayTerminate(&n->peers);
}
void NetInputReset(NetInput *n)
{
	n->PrevCmd = n->Cmd = 0;
}

void NetInputOpen(NetInput *n)
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

void NetInputPoll(NetInput *n)
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
				printf("A new client connected from %x:%u.\n",
					event.peer->address.host,
					event.peer->address.port);
				CArrayPushBack(&n->peers, &event.peer);
				/* Store any relevant client information here. */
				event.peer->data = (void *)n->peerId;
				n->peerId++;
				break;
			case ENET_EVENT_TYPE_RECEIVE:
				if (event.packet->dataLength > 0)
				{
					ClientMsg msg = *(uint32_t *)event.packet->data;
					switch (msg)
					{
					case CLIENT_MSG_CMD:
						{
							NetMsgCmd *nc =
								(NetMsgCmd *)(event.packet->data + NET_MSG_SIZE);
							CASSERT(
								event.packet->dataLength == NET_MSG_SIZE + sizeof *nc,
								"unexpected packet size");
							n->Cmd = nc->cmd;
						}
						break;
					default:
						printf("Unknown message type %d\n", msg);
						break;
					}
				}
				//printf("A packet of length %u containing %s was received from %s on channel %u.\n",
				//	event.packet->dataLength,
				//	event.packet->data,
				//	event.peer->data,
				//	event.channelID);
				/* Clean up the packet now that we're done using it. */
				enet_packet_destroy(event.packet);
				break;

			case ENET_EVENT_TYPE_DISCONNECT:
				{
					printf("disconnected %x:%u.\n",
						event.peer->address.host,
						event.peer->address.port);
					// Find the peer by id and delete
					bool found = false;
					for (int i = 0; i < (int)n->peers.size; i++)
					{
						ENetPeer **peer = CArrayGet(&n->peers, i);
						if ((int)(*peer)->data == (int)event.peer->data)
						{
							CArrayDelete(&n->peers, i);
							found = true;
							break;
						}
					}
					CASSERT(found, "Cannot find peer by id");
					/* Reset the peer's client information. */
					event.peer->data = NULL;
				}
				break;

			default:
				CASSERT(false, "Unknown event");
				break;
			}
		}
	} while (check > 0);
}

static ENetPacket *MakePacket(ServerMsg msg, const void *data);

void NetInputSendMsg(
	NetInput *n, const int peerIndex, ServerMsg msg, const void *data)
{
	if (!n->server)
	{
		return;
	}
	CASSERT(
		peerIndex >= 0 && peerIndex < (int)n->peers.size,
		"invalid peer index");

	// Find the peer and send
	for (int i = 0; i < (int)n->peers.size; i++)
	{
		ENetPeer **peer = CArrayGet(&n->peers, i);
		if ((int)(*peer)->data == peerIndex)
		{
			enet_peer_send(*peer, 0, MakePacket(msg, data));
			enet_host_flush(n->server);
			return;
		}
	}
	CASSERT(false, "Cannot find peer by id");
}

void NetInputBroadcastMsg(NetInput *n, ServerMsg msg, const void *data)
{
	if (!n->server)
	{
		return;
	}

	enet_host_broadcast(n->server, 0, MakePacket(msg, data));
	enet_host_flush(n->server);
}

static ENetPacket *MakePacketImpl(ServerMsg msg, const void *data, const int len);
static ENetPacket *MakePacket(ServerMsg msg, const void *data)
{
	switch (msg)
	{
	case SERVER_MSG_CAMPAIGN_DEF:
		{
			NetMsgCampaignDef def;
			memset(&def, 0, sizeof def);
			const CampaignEntry *entry = data;
			if (entry->Filename)
			{
				strcpy((char *)def.Path, entry->Filename);
			}
			def.CampaignMode = entry->Mode;
			return MakePacketImpl(msg, &def, sizeof def);
		}
		break;
	case SERVER_MSG_GAME_START:
		return MakePacketImpl(msg, NULL, 0);
	default:
		CASSERT(false, "Unknown message to make into packet");
		return NULL;
	}
}
static ENetPacket *MakePacketImpl(ServerMsg msg, const void *data, const int len)
{
	ENetPacket *packet = enet_packet_create(
		&msg, NET_MSG_SIZE + len, ENET_PACKET_FLAG_RELIABLE);
	memcpy(packet->data + NET_MSG_SIZE, data, len);
	return packet;
}
