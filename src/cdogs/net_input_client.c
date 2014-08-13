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
#include "net_input_client.h"

#include <string.h>

#include "net_input.h"
#include "utils.h"


#define CONNECTION_WAIT_MS 5000


void NetInputClientInit(NetInputClient *n)
{
	memset(n, 0, sizeof *n);
}
void NetInputClientTerminate(NetInputClient *n)
{
	enet_peer_reset(n->peer);
	enet_host_destroy(n->client);
}

void NetInputClientConnect(NetInputClient *n, const ENetAddress addr)
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
	enet_peer_reset(n->peer);
	enet_host_destroy(n->client);
}

void NetInputClientUpdate(NetInputClient *n)
{
	if (!n->client || !n->peer)
	{
		return;
	}
	// Service the connection
	ENetEvent event;
	enet_host_service(n->client, &event, 0);
}

void NetInputClientSend(NetInputClient *n, int cmd)
{
	if (!n->client || !n->peer)
	{
		return;
	}

	NetMsgCmd nc;
	nc.cmd = cmd;
	ENetPacket *packet =
		enet_packet_create(&nc, sizeof nc, ENET_PACKET_FLAG_RELIABLE);
	enet_peer_send(n->peer, 0, packet);
	enet_host_flush(n->client);
}
