/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.

    Copyright (c) 2014-2016, Cong Xu
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

#include <time.h>

#include "net_util.h"

// Stored information about game servers scanned
typedef struct
{
	NServerInfo ServerInfo;
	ENetAddress Addr;
	int LatencyMS;
} ScanInfo;

typedef struct
{
	ENetHost *client;
	ENetPeer *peer;
	int ClientId;
	int FirstPlayerUID;
	bool Ready;
	// Socket used to scan for LAN servers
	ENetSocket scanner;
	// Only scan for a period; if > 0 then we are scanning
	int ScanTicks;
	// Addresses of scanned LAN servers
	CArray ScannedAddrs;		// of ScanInfo
	// Buffer of scanned addresses - new ones will be scanned here
	CArray scannedAddrBuf;	// of ScanInfo
} NetClient;

extern NetClient gNetClient;

void NetClientInit(NetClient *n);
void NetClientTerminate(NetClient *n);

// Start searching for LAN servers; note that the result will be returned in
// NetClient's boolean
void NetClientFindLANServers(NetClient *n);
// Attempt to connect to a server
bool NetClientTryConnect(NetClient *n, const ENetAddress addr);
// Attempt to scan a host for a game server and connect
bool NetClientTryScanAndConnect(NetClient *n, const enet_uint32 host);
void NetClientDisconnect(NetClient *n);
void NetClientPoll(NetClient *n);
void NetClientFlush(NetClient *n);
// Send a command to the server
void NetClientSendMsg(NetClient *n, const GameEventType e, const void *data);

bool NetClientIsConnected(const NetClient *n);
