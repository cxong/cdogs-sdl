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
#include "net_util.h"

#include "proto/nanopb/pb_decode.h"
#include "proto/nanopb/pb_encode.h"


ENetPacket *NetEncode(int msgId, const void *data, const pb_field_t fields[])
{
	uint8_t buffer[1024];
	pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof buffer);
	bool status = data ? pb_encode(&stream, fields, data) : true;
	CASSERT(status, "Failed to encode pb");
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


NetMsgPlayerData NetMsgMakePlayerData(const PlayerData *p)
{
	NetMsgPlayerData d;
	const Character *c = &p->Char;
	d.IsUsed = p->IsUsed;
	strcpy(d.Name, p->name);
	d.Looks.Face = c->looks.face;
	d.Looks.Skin = c->looks.skin;
	d.Looks.Arm = c->looks.arm;
	d.Looks.Body = c->looks.body;
	d.Looks.Leg = c->looks.leg;
	d.Looks.Hair = c->looks.hair;
	d.Weapons_count = (pb_size_t)p->weaponCount;
	for (int i = 0; i < (int)d.Weapons_count; i++)
	{
		strcpy(d.Weapons[i], p->weapons[i]->name);
	}
	d.Score = p->score;
	d.TotalScore = p->totalScore;
	d.Kills = p->kills;
	d.Friendlies = p->friendlies;
	d.PlayerIndex = p->playerIndex;
	return d;
}

void NetMsgCampaignDefConvert(
	const NetMsgCampaignDef *def, char *outPath, campaign_mode_e *outMode)
{
	strcpy(outPath, def->Path);
	*outMode = def->CampaignMode;
}
void NetMsgPlayerDataUpdate(const NetMsgPlayerData *pd)
{
	PlayerData *p = CArrayGet(&gPlayerDatas, pd->PlayerIndex);
	p->IsUsed = pd->IsUsed;
	strcpy(p->name, pd->Name);
	p->Char.looks.face = pd->Looks.Face;
	p->Char.looks.skin = pd->Looks.Skin;
	p->Char.looks.arm = pd->Looks.Arm;
	p->Char.looks.body = pd->Looks.Body;
	p->Char.looks.leg = pd->Looks.Leg;
	p->Char.looks.hair = pd->Looks.Hair;
	CharacterSetColors(&p->Char);
	p->weaponCount = pd->Weapons_count;
	for (int i = 0; i < (int)pd->Weapons_count; i++)
	{
		p->weapons[i] = StrGunDescription(pd->Weapons[i]);
	}
	p->score = pd->Score;
	p->totalScore = pd->Score;
	p->kills = pd->Kills;
	p->friendlies = pd->Friendlies;
	if (!p->IsLocal)
	{
		p->inputDevice = INPUT_DEVICE_UNSET;
	}
	CASSERT(
		p->playerIndex == pd->PlayerIndex, "unexpected player index");
}
