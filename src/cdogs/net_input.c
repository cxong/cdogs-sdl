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


void NetInputInit(NetInput *n)
{
	memset(n, 0, sizeof *n);
}
void NetInputTerminate(NetInput *n)
{
	SDLNet_UDP_Close(n->sock);
	n->sock = NULL;
}
void NetInputReset(NetInput *n)
{
	n->PrevCmd = n->Cmd = 0;
}

void NetInputOpen(NetInput *n)
{
	n->sock = SDLNet_UDP_Open(NET_INPUT_UDP_PORT);
	if (n->sock == NULL)
	{
		printf("SDLNet_UDP_Open: %s\n", SDLNet_GetError());
		return;
	}
}
void NetInputPoll(NetInput *n)
{
	if (n->sock == NULL)
	{
		return;
	}
	n->PrevCmd = n->Cmd;
	n->Cmd = 0;

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
			return;
		}
		else if (numrecv > 0)
		{
			// trivial protocol for now: 32-bit int for input cmd
			size_t readlen = sizeof(uint32_t);
			while (packet.len >= (int)readlen)
			{
				Uint32 cmd = SDLNet_Read32(packet.data);
				n->Cmd |= cmd;
				packet.len -= readlen;
				packet.data += readlen;
			}
		}
	} while (numrecv > 0);
}
