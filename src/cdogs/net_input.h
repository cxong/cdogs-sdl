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
#ifndef __NET_INPUT
#define __NET_INPUT

#include <stdbool.h>

#include "c_array.h"
#include "net_util.h"

typedef struct
{
	ENetHost *server;
	int PrevCmd;
	int Cmd;
	int peerId;	// auto-incrementing id for the next connected peer
} NetInput;

typedef struct
{
	int Id;
} NetPeerData;

void NetInputInit(NetInput *n);
void NetInputTerminate(NetInput *n);
void NetInputReset(NetInput *n);

// Open a port and start listening for data
void NetInputOpen(NetInput *n);
// Service the recv buffer; if data is received then activate this device
void NetInputPoll(NetInput *n);

void NetInputSendMsg(
	NetInput *n, const int peerId, ServerMsg msg, const void *data);
// Send message to all peers
void NetInputBroadcastMsg(NetInput *n, ServerMsg msg, const void *data);

#endif
