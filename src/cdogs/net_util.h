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
#include <stdint.h>

#include <enet/enet.h>

#include "proto/server.pb.h"

#include "campaign_entry.h"
#include "game_events.h"
#include "game_mode.h"
#include "player.h"
#include "sys_config.h"

#define NET_INPUT_PORT 34219

// Used in printf statements
#define NET_IP_TO_CIDR_FORMAT(_ip)\
	((_ip) >> 24) & 0xFF,\
	((_ip) >> 16) & 0xFF,\
	((_ip) >> 8) & 0xFF,\
	(_ip)& 0xFF

// Messages

// All messages start with 4 bytes message type followed by the message struct
#define NET_MSG_SIZE sizeof(uint32_t)

// Commands
typedef enum
{
	MSG_NONE,

	// Bidirectional messages
	MSG_PLAYER_DATA,
	MSG_ACTOR_MOVE,
	MSG_ACTOR_STATE,
	MSG_ACTOR_DIR,
	MSG_ADD_BULLET,

	// Client to server messages
	MSG_REQUEST_PLAYERS,
	MSG_NEW_PLAYERS,

	// Server to client messages
	MSG_CLIENT_ID,
	MSG_CAMPAIGN_DEF,
	MSG_ADD_PLAYERS,
	MSG_GAME_START,
	MSG_ACTOR_ADD,
	MSG_GAME_END
} NetMsg;


ENetPacket *NetEncode(const NetMsg msg, const void *data);
bool NetDecode(ENetPacket *packet, void *dest, const pb_field_t *fields);

NetMsgPlayerData NetMsgMakePlayerData(const PlayerData *p);
NetMsgCampaignDef NetMsgMakeCampaignDef(const CampaignEntry *e);
void NetMsgCampaignDefConvert(
	const NetMsgCampaignDef *def, char *outPath, GameMode *outMode);
void NetMsgPlayerDataUpdate(const NetMsgPlayerData *pd);

Vec2i Net2Vec2i(const NetMsgVec2i v);
NetMsgVec2i Vec2i2Net(const Vec2i v);

typedef struct
{
	NetMsg Msg;
	const pb_field_t *Fields;
	GameEventType Event;
} NetMsgEntry;
NetMsgEntry NetMsgGet(const NetMsg msg);
