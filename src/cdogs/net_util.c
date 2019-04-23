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
#include "net_util.h"

#include "proto/nanopb/pb_decode.h"
#include "proto/nanopb/pb_encode.h"


ENetPacket *NetEncode(const GameEventType e, const void *data)
{
	uint8_t buffer[1024];
	pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof buffer);
	const pb_field_t *fields = GameEventGetEntry(e).Fields;
	const bool status =
		(data && fields) ? pb_encode(&stream, fields, data) : true;
	CASSERT(status, "Failed to encode pb");
	int msgId = (int)e;
	ENetPacket *packet = enet_packet_create(
		&msgId, NET_MSG_SIZE + stream.bytes_written,
		ENET_PACKET_FLAG_RELIABLE);
	memcpy(packet->data + NET_MSG_SIZE, buffer, stream.bytes_written);
	return packet;
}

bool NetDecode(
	ENetPacket *packet, void *dest, const pb_field_t *fields)
{
	pb_istream_t stream = pb_istream_from_buffer(
		packet->data + NET_MSG_SIZE, packet->dataLength - NET_MSG_SIZE);
	bool status = pb_decode(&stream, fields, dest);
	CASSERT(status, "Failed to decode pb");
	return status;
}


NPlayerData NMakePlayerData(const PlayerData *p)
{
	NPlayerData d = NPlayerData_init_default;
	const Character *c = &p->Char;
	strcpy(d.Name, p->name);
	strcpy(d.CharacterClass, p->Char.Class->Name);
	if (p->Char.Hair)
	{
		strcpy(d.Hair, p->Char.Hair);
	}
	d.Colors = CharColors2Net(c->Colors);
	d.Weapons_count = MAX_WEAPONS;
	for (int i = 0; i < (int)d.Weapons_count; i++)
	{
		strcpy(d.Weapons[i], p->guns[i] != NULL ? p->guns[i]->name : "");
	}
	d.Lives = p->Lives;
	d.Stats = p->Stats;
	d.Totals = p->Totals;
	d.MaxHealth = p->Char.maxHealth;
	d.LastMission = p->lastMission;
	d.UID = p->UID;
	return d;
}
NCampaignDef NMakeCampaignDef(const CampaignOptions *co)
{
	NCampaignDef def;
	memset(&def, 0, sizeof def);
	if (co->Entry.Path)
	{
		strcpy((char *)def.Path, co->Entry.Path);
	}
	def.GameMode = co->Entry.Mode;
	def.Mission = co->MissionIndex;
	return def;
}
NMissionComplete NMakeMissionComplete(
	const struct MissionOptions *mo, const Map *map)
{
	NMissionComplete mc;
	mc.ShowMsg = MissionHasRequiredObjectives(mo);
	mc.ExitStart = Vec2i2Net(map->ExitStart);
	mc.ExitEnd = Vec2i2Net(map->ExitEnd);
	return mc;
}

struct vec2i Net2Vec2i(const NVec2i v)
{
	return svec2i(v.x, v.y);
}
NVec2i Vec2i2Net(const struct vec2i v)
{
	NVec2i nv;
	nv.x = v.x;
	nv.y = v.y;
	return nv;
}
struct vec2 NetToVec2(const NVec2 v)
{
	return svec2(v.x, v.y);
}
NVec2 Vec2ToNet(const struct vec2 v)
{
	NVec2 nv;
	nv.x = v.x;
	nv.y = v.y;
	return nv;
}
color_t Net2Color(const NColor c)
{
	color_t co;
	co.r = (uint8_t)((c.RGBA & 0xff000000) >> 24);
	co.g = (uint8_t)((c.RGBA & 0x00ff0000) >> 16);
	co.b = (uint8_t)((c.RGBA & 0x0000ff00) >> 8);
	co.a = (uint8_t)(c.RGBA & 0x000000ff);
	return co;
}
NColor Color2Net(const color_t c)
{
	NColor co;
	co.RGBA = (c.r << 24) | (c.g << 16) | (c.b << 8) | c.a;
	return co;
}
NCharColors CharColors2Net(const CharColors c)
{
	NCharColors co;
	co.Skin = Color2Net(c.Skin);
	co.Arms = Color2Net(c.Arms);
	co.Body = Color2Net(c.Body);
	co.Legs = Color2Net(c.Legs);
	co.Hair = Color2Net(c.Hair);
	return co;
}
CharColors Net2CharColors(const NCharColors c)
{
	CharColors co;
	co.Skin = Net2Color(c.Skin);
	co.Arms = Net2Color(c.Arms);
	co.Body = Net2Color(c.Body);
	co.Legs = Net2Color(c.Legs);
	co.Hair = Net2Color(c.Hair);
	return co;
}
