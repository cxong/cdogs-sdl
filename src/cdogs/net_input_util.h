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
#ifndef __NET_INPUT_UTIL
#define __NET_INPUT_UTIL

#include <stdbool.h>

#include <SDL_net.h>

#define NET_INPUT_UDP_PORT 34219
#define NET_INPUT_MAX_PACKET_LEN 1024

// Used in printf statements
#define NET_IP_TO_CIDR_FORMAT(_ip)\
	((_ip) >> 24) & 0xFF,\
	((_ip) >> 16) & 0xFF,\
	((_ip) >> 8) & 0xFF,\
	(_ip)& 0xFF

// Messages

// 3-way handshake
typedef struct
{
	Uint16 seq;
} NetMsgSyn;
typedef struct
{
	Uint16 seq;
	Uint16 ack;
} NetMsgSynAck;
typedef struct
{
	Uint16 ack;
} NetMsgAck;

// Commands (client to server)
typedef struct
{
	Uint32 ticks;
	Uint32 cmd;
} NetMsgCmd;

// Game events (server to client)
typedef enum
{
	SERVER_MSG_GAME_START
} ServerMsg;


typedef enum
{
	CHANNEL_STATE_CLOSED,
	CHANNEL_STATE_DISCONNECTED,
	CHANNEL_STATE_WAIT_HANDSHAKE,
	CHANNEL_STATE_CONNECTED
} NetInputChannelState;

typedef struct
{
	UDPsocket sock;
	NetInputChannelState state;
	Uint16 otherPort;
	Uint32 otherHost;
	Uint16 seq;
	Uint16 ack;

	Uint8 buf[NET_INPUT_MAX_PACKET_LEN];
} NetInputChannel;

void NetInputChannelTerminate(NetInputChannel *n);

bool NetInputChannelTryOpen(NetInputChannel *n, Uint16 port, Uint32 host);

UDPpacket NetInputNewPacket(NetInputChannel *n, size_t len);
bool NetInputTrySendPacket(NetInputChannel *n, UDPpacket packet);
bool NetInputRecvBlocking(
	NetInputChannel *n, bool (*tryParseFunc)(UDPpacket *, void *), void *data);

// Returns true only if successfully received expected packet
bool NetInputRecvNonBlocking(
	NetInputChannel *n, bool (*tryParseFunc)(UDPpacket *, void *), void *data);

#endif
