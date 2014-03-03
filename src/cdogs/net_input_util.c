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
#include "net_input_util.h"

#include "utils.h"


void NetInputChannelTerminate(NetInputChannel *n)
{
	SDLNet_UDP_Close(n->sock);
	n->sock = NULL;
}

bool NetInputChannelTryOpen(NetInputChannel *n, Uint16 port, Uint32 host)
{
	n->sock = SDLNet_UDP_Open(port);
	if (n->sock == NULL)
	{
		printf("SDLNet_UDP_Open: %s\n", SDLNet_GetError());
		return false;
	}
	printf("Opened socket on port %d, other host %d.%d.%d.%d\n",
		port, NET_IP_TO_CIDR_FORMAT(host));
	n->state = CHANNEL_STATE_DISCONNECTED;
	n->otherHost = host;
	return true;
}

UDPpacket NetInputNewPacket(NetInputChannel *n, size_t len)
{
	UDPpacket packet;
	SDLNet_Write32(n->otherHost, &packet.address.host);
	SDLNet_Write16(n->otherPort, &packet.address.port);
	packet.maxlen = packet.len = len;
	CMALLOC(packet.data, len);
	return packet;
}

bool NetInputTrySendPacket(NetInputChannel *n, UDPpacket packet)
{
	if (n->sock == NULL || n->state == CHANNEL_STATE_CLOSED)
	{
		return false;
	}

	int numsent = SDLNet_UDP_Send(n->sock, -1, &packet);
	if (numsent == 0)
	{
		printf("SDLNet_UDP_Send: %s\n", SDLNet_GetError());
		n->state = CHANNEL_STATE_CLOSED;
		return false;
	}

	return true;
}

bool NetInputRecvBlocking(
	NetInputChannel *n, bool (*tryParseFunc)(UDPpacket *, void *), void *data)
{
	UDPpacket packet;
	packet.data = n->buf;
	packet.maxlen = NET_INPUT_MAX_PACKET_LEN;
	int numrecv;
	do
	{
		numrecv = SDLNet_UDP_Recv(n->sock, &packet);
		if (numrecv < 0)
		{
			printf("SDLNet_UDP_Recv: %s\n", SDLNet_GetError());
			return false;
		}
		else if (numrecv > 0)
		{
			if (!tryParseFunc(&packet, data))
			{
				return false;
			}
		}
	} while (numrecv == 0);
	return true;
}

bool NetInputRecvNonBlocking(
	NetInputChannel *n, bool (*tryParseFunc)(UDPpacket *, void *), void *data)
{
	UDPpacket packet;
	packet.data = n->buf;
	packet.maxlen = NET_INPUT_MAX_PACKET_LEN;
	int numrecv;
	do
	{
		numrecv = SDLNet_UDP_Recv(n->sock, &packet);
		if (numrecv < 0)
		{
			printf("SDLNet_UDP_Recv: %s\n", SDLNet_GetError());
			return false;
		}
		else if (numrecv > 0)
		{
			return tryParseFunc(&packet, data);
		}
	} while (numrecv > 0);
	return false;
}
