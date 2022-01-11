/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.

	Copyright (c) 2014-2017, 2019-2022 Cong Xu
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

#include "campaigns.h"
#include "game_events.h"
#include "map.h"
#include "player.h"

#define NET_LISTEN_PORT 34219

#define NET_PROTOCOL_VERSION 12

// Messages

// All messages start with 4 bytes message type followed by the message struct
#define NET_MSG_SIZE sizeof(uint32_t)

ENetPacket *NetEncode(const GameEventType e, const void *data);
bool NetDecode(ENetPacket *packet, void *dest, const pb_msgdesc_t *fields);

NPlayerData NMakePlayerData(const PlayerData *p);
NCampaignDef NMakeCampaignDef(const Campaign *co);
NMissionComplete NMakeMissionComplete(const struct MissionOptions *mo);

struct vec2i Net2Vec2i(const NVec2i v);
NVec2i Vec2i2Net(const struct vec2i v);
struct vec2 NetToVec2(const NVec2 v);
NVec2 Vec2ToNet(const struct vec2 v);
NColor Color2Net(const color_t c);
color_t Net2Color(const NColor c);
NCharColors CharColors2Net(const CharColors c);
CharColors Net2CharColors(const NCharColors c);
void Ammo2Net(pb_size_t *ammoCount, NAmmo *ammo, const CArray *a);
