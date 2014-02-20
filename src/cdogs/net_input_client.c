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
	SDLNet_UDP_Close(n->sock);
	n->sock = NULL;
}

void NetInputClientConnect(NetInputClient *n, Uint32 host)
{
	n->sock = SDLNet_UDP_Open(0);
	if (n->sock == NULL)
	{
		printf("SDLNet_UDP_Open: %s\n", SDLNet_GetError());
		return;
	}
	n->isActive = true;
	n->serverHost = host;
}
void NetInputClientSend(NetInputClient *n, int cmd)
{
	if (n->sock == NULL || !n->isActive)
	{
		return;
	}

	UDPpacket packet;
	SDLNet_Write32(n->serverHost, &packet.address.host);
	SDLNet_Write16(NET_INPUT_UDP_PORT, &packet.address.port);
	packet.maxlen = packet.len = sizeof(Uint32);
	CMALLOC(packet.data, sizeof(Uint32));
	SDLNet_Write32(cmd, packet.data);
	int numsent = SDLNet_UDP_Send(n->sock, -1, &packet);
	if (numsent == 0)
	{
		printf("SDLNet_UDP_Send: %s\n", SDLNet_GetError());
		n->isActive = false;
	}
}
