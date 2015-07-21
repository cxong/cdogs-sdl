/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.

    Copyright (c) 2014-2015, Cong Xu
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
#pragma once

#include <stdbool.h>

#include "c_array.h"
#include "net_util.h"


#define NET_SERVER_MAX_CLIENTS 32
#define NET_SERVER_BCAST -1

typedef struct
{
	ENetHost *server;
	int PrevCmd;
	int Cmd;
	int peerId;	// auto-incrementing id for the next connected peer
} NetServer;

extern NetServer gNetServer;

typedef struct
{
	int Id;
} NetPeerData;

void NetServerInit(NetServer *n);
void NetServerTerminate(NetServer *n);
void NetServerReset(NetServer *n);

// Open a port and start listening for data
void NetServerOpen(NetServer *n);
void NetServerClose(NetServer *n);
// Service the recv buffer; if data is received then activate this device
void NetServerPoll(NetServer *n);
void NetServerFlush(NetServer *n);

// If peerId is -1, broadcast
void NetServerSendMsg(
	NetServer *n, const int peerId, const GameEventType e, const void *data);

void NetServerSendGameStartMessages(NetServer *n, const int peerId);
