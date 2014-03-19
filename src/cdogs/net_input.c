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

#include "utils.h"


void NetInputInit(NetInput *n)
{
	memset(n, 0, sizeof *n);
}
void NetInputTerminate(NetInput *n)
{
	NetInputChannelTerminate(&n->channel);
}
void NetInputReset(NetInput *n)
{
	n->PrevCmd = n->Cmd = 0;
}

void NetInputOpen(NetInput *n)
{
	NetInputChannelTryOpen(&n->channel, NET_INPUT_UDP_PORT, 0);
}

static bool TryRecvSynAndSendSynAck(NetInput *n);
static bool TryParseAck(UDPpacket *packet, void *data);
static void RecvCmd(NetInput *n);
void NetInputPoll(NetInput *n)
{
	if (n->channel.sock == NULL)
	{
		return;
	}

	switch (n->channel.state)
	{
	case CHANNEL_STATE_CLOSED:
		CASSERT(false, "Unexpected channel state closed");
		return;

	case CHANNEL_STATE_DISCONNECTED:
		if (!TryRecvSynAndSendSynAck(n))
		{
			return;
		}
		n->channel.state = CHANNEL_STATE_WAIT_HANDSHAKE;
		// fallthrough

	case CHANNEL_STATE_WAIT_HANDSHAKE:
		// listen for ACK
		if (!NetInputRecvNonBlocking(&n->channel, TryParseAck, &n->channel))
		{
			return;
		}
		n->channel.state = CHANNEL_STATE_CONNECTED;
		// fallthrough

	case CHANNEL_STATE_CONNECTED:
		RecvCmd(n);
		break;

	default:
		CASSERT(false, "Unknown channel state");
		break;
	}
}

static bool TryParseSyn(UDPpacket *packet, void *data);
static bool TryRecvSynAndSendSynAck(NetInput *n)
{
	// listen for SYN
	if (!NetInputRecvNonBlocking(&n->channel, TryParseSyn, &n->channel))
	{
		return false;
	}

	// Send SYN-ACK
	bool res = false;
	UDPpacket packet = NetInputNewPacket(&n->channel, sizeof(NetMsgSynAck));
	NetMsgSynAck *packetSyn = (NetMsgSynAck *)packet.data;
	n->channel.seq = rand() & (Uint16)-1;
	SDLNet_Write16(n->channel.seq, &packetSyn->seq);
	SDLNet_Write16(n->channel.ack, &packetSyn->ack);
	if (!NetInputTrySendPacket(&n->channel, packet))
	{
		goto bail;
	}
	n->channel.seq++;
	res = true;

bail:
	CFREE(packet.data);
	return res;
}

static bool TryParseSyn(UDPpacket *packet, void *data)
{
	if (packet->len != sizeof(NetMsgSyn))
	{
		printf("Error: expected SYN, received packet len %d\n",
			packet->len);
		return false;
	}
	NetMsgSyn *msg = (NetMsgSyn *)packet->data;
	NetInputChannel *n = data;
	n->ack = SDLNet_Read16(&msg->seq) + 1;
	n->otherHost = SDLNet_Read32(&packet->address.host);
	n->otherPort = SDLNet_Read16(&packet->address.port);
	printf("Received SYN from %d.%d.%d.%d:%d\n",
		NET_IP_TO_CIDR_FORMAT(n->otherHost), n->otherPort);
	return true;
}

static bool TryParseAck(UDPpacket *packet, void *data)
{
	if (packet->len != sizeof(NetMsgAck))
	{
		printf("Error: expected ACK, received packet len %d\n",
			packet->len);
		return false;
	}
	NetMsgAck *msg = (NetMsgAck *)packet->data;
	Uint16 ack = SDLNet_Read16(&msg->ack);
	NetInputChannel *n = data;
	if (ack != n->seq)
	{
		printf("Error: received invalid ack %d, expected %d\n",
			ack, n->seq);
		return false;
	}
	return true;
}

static bool TryParseCmd(UDPpacket *packet, void *data);
static void RecvCmd(NetInput *n)
{
	n->PrevCmd = n->Cmd;
	n->Cmd = 0;

	while (NetInputRecvNonBlocking(&n->channel, TryParseCmd, n));
}

static bool TryParseCmd(UDPpacket *packet, void *data)
{
	size_t readlen = sizeof(NetMsgCmd);
	NetInput *n = data;
	while (packet->len >= (int)readlen)
	{
		NetMsgCmd *msg = (NetMsgCmd *)packet->data;
		Uint32 cmd = SDLNet_Read32(&msg->cmd);
		n->Cmd |= cmd;
		packet->len -= readlen;
		packet->data += readlen;
	}
	return true;
}

void NetInputSendMsg(NetInput *n, ServerMsg msg)
{
	if (n->channel.sock == NULL || n->channel.state != CHANNEL_STATE_CONNECTED)
	{
		return;
	}

	switch (msg)
	{
	case SERVER_MSG_GAME_START:
		{
			UDPpacket packet = NetInputNewPacket(&n->channel, sizeof(Uint8));
			*packet.data = (Uint8)msg;
			NetInputTrySendPacket(&n->channel, packet);
		}
		break;
	default:
		CASSERT(false, "Unknown server msg type");
		break;
	}
}
