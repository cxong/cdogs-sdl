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

#include "net_input.h"
#include "utils.h"


void NetInputClientInit(NetInputClient *n)
{
	memset(n, 0, sizeof *n);
}
void NetInputClientTerminate(NetInputClient *n)
{
	NetInputChannelTerminate(&n->channel);
}

static bool TryParseSynAck(UDPpacket *packet, void *data);
void NetInputClientConnect(NetInputClient *n, Uint32 host)
{
	if (!NetInputChannelTryOpen(&n->channel, 0, host))
	{
		printf("Failed to open channel\n");
		return;
	}
	n->channel.otherPort = NET_INPUT_UDP_PORT;

	// Handshake

	// Send SYN
	UDPpacket packet = NetInputNewPacket(&n->channel, sizeof(NetMsgSyn));
	NetMsgSyn *packetSyn = (NetMsgSyn *)packet.data;
	n->channel.seq = rand() & (Uint16)-1;
	SDLNet_Write16(n->channel.seq, &packetSyn->seq);
	if (!NetInputTrySendPacket(&n->channel, packet))
	{
		printf("Failed to send SYN\n");
		goto bail;
	}
	n->channel.seq++;

	// Wait for SYN-ACK
	// TODO: don't wait forever
	n->channel.state = CHANNEL_STATE_WAIT_HANDSHAKE;
	while (!NetInputRecvBlocking(&n->channel, TryParseSynAck, &n->channel));

	// Send ACK
	CFREE(packet.data);
	packet = NetInputNewPacket(&n->channel, sizeof(NetMsgAck));
	NetMsgAck *packetAck = (NetMsgAck *)packet.data;
	SDLNet_Write16(n->channel.ack, &packetAck->ack);
	if (!NetInputTrySendPacket(&n->channel, packet))
	{
		printf("Failed to send ACK\n");
		goto bail;
	}

	n->channel.state = CHANNEL_STATE_CONNECTED;
	return;

bail:
	CFREE(packet.data);
}

static bool TryParseSynAck(UDPpacket *packet, void *data)
{
	if (packet->len != sizeof(NetMsgSynAck))
	{
		printf("Error: expected SYN-ACK, received packet len %d\n",
			packet->len);
		return false;
	}
	NetMsgSynAck *msg = (NetMsgSynAck *)packet->data;
	Uint16 ack = SDLNet_Read16(&msg->ack);
	NetInputChannel *n = data;
	if (ack != n->seq)
	{
		printf("Error: received invalid ack %d, expected %d\n",
			ack, n->seq);
		return false;
	}
	n->ack = SDLNet_Read16(&msg->seq) + 1;
	return true;
}

void NetInputClientSend(NetInputClient *n, int cmd)
{
	if (n->channel.sock == NULL || n->channel.state != CHANNEL_STATE_CONNECTED)
	{
		return;
	}

	UDPpacket packet = NetInputNewPacket(&n->channel, sizeof(NetMsgCmd));
	NetMsgCmd *packetCmd = (NetMsgCmd *)packet.data;
	SDLNet_Write32(n->ticks, &packetCmd->ticks);
	SDLNet_Write32(cmd, &packetCmd->cmd);
	if (!NetInputTrySendPacket(&n->channel, packet))
	{
		goto bail;
	}

bail:
	CFREE(packet.data);
}
