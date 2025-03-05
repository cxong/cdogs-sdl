/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.

	Copyright (c) 2014-2016, 2020-2021, 2023 Cong Xu
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
	const pb_msgdesc_t *fields = GameEventGetEntry(e).Fields;
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

bool NetDecode(ENetPacket *packet, void *dest, const pb_msgdesc_t *fields)
{
	pb_istream_t stream = pb_istream_from_buffer(
		packet->data + NET_MSG_SIZE, packet->dataLength - NET_MSG_SIZE);
	bool status = pb_decode(&stream, fields, dest);
	CASSERT(status, "Failed to decode pb");
	return status;
}

typedef struct
{
	map_t src;
	NPlayerData *d;
} AddWeaponUsageData;
static int AddWeaponUsage(any_t data, any_t key)
{
	AddWeaponUsageData *aData = (AddWeaponUsageData *)data;
	// TODO: copy weapon usage
	NWeaponUsage *w;
	int error = hashmap_get(aData->src, (char *)key, (any_t *)&w);
	if (error != MAP_OK)
	{
		return error;
	}
	NWeaponUsage *n = &aData->d->WeaponUsages[aData->d->WeaponUsages_count];
	strcpy(n->Weapon, (char *)key);
	n->Shots = w->Shots;
	n->Hits = w->Hits;
	aData->d->WeaponUsages_count++;
	return MAP_OK;
}
NPlayerData NMakePlayerData(const PlayerData *p)
{
	NPlayerData d = NPlayerData_init_default;
	d.has_Colors = d.has_Stats = d.has_Totals = true;
	const Character *c = &p->Char;
	strcpy(d.Name, p->name);
	strcpy(d.CharacterClass, p->Char.Class->Name);
	if (p->Char.HeadParts[HEAD_PART_HAIR])
	{
		strcpy(d.Hair, p->Char.HeadParts[HEAD_PART_HAIR]);
	}
	if (p->Char.HeadParts[HEAD_PART_FACEHAIR])
	{
		strcpy(d.Facehair, p->Char.HeadParts[HEAD_PART_FACEHAIR]);
	}
	if (p->Char.HeadParts[HEAD_PART_HAT])
	{
		strcpy(d.Hat, p->Char.HeadParts[HEAD_PART_HAT]);
	}
	if (p->Char.HeadParts[HEAD_PART_GLASSES])
	{
		strcpy(d.Glasses, p->Char.HeadParts[HEAD_PART_GLASSES]);
	}
	d.Colors = CharColors2Net(c->Colors);
	d.Weapons_count = MAX_WEAPONS;
	for (int i = 0; i < (int)d.Weapons_count; i++)
	{
		strcpy(d.Weapons[i], p->guns[i] != NULL ? p->guns[i]->name : "");
	}
	d.WeaponUsages_count = 0;
	AddWeaponUsageData aData;
	aData.src = p->WeaponUsages;
	aData.d = &d;
	hashmap_iterate_keys(p->WeaponUsages, AddWeaponUsage, &aData);

	d.Lives = p->Lives;
	d.Stats = p->Stats;
	d.Totals = p->Totals;
	d.MaxHealth = p->Char.maxHealth;
	d.ExcessHealth = p->Char.excessHealth;
	d.HP = p->HP;
	d.LastMission = p->lastMission;
	d.UID = p->UID;
	Ammo2Net(&d.Ammo_count, d.Ammo, &p->ammo);
	return d;
}
NCampaignDef NMakeCampaignDef(const Campaign *co)
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
NMissionComplete NMakeMissionComplete(const struct MissionOptions *mo)
{
	NMissionComplete mc;
	mc.ShowMsg = MissionHasRequiredObjectives(mo);
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
	co.has_Skin = co.has_Arms = co.has_Body = co.has_Legs = co.has_Hair =
		co.has_Feet = co.has_Facehair = co.has_Hat = co.has_Glasses = true;
	co.Skin = Color2Net(c.Skin);
	co.Arms = Color2Net(c.Arms);
	co.Body = Color2Net(c.Body);
	co.Legs = Color2Net(c.Legs);
	co.Hair = Color2Net(c.Hair);
	co.Feet = Color2Net(c.Feet);
	co.Facehair = Color2Net(c.Facehair);
	co.Hat = Color2Net(c.Hat);
	co.Glasses = Color2Net(c.Glasses);
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
	co.Feet = Net2Color(c.Feet);
	co.Facehair = Net2Color(c.Facehair);
	co.Hat = Net2Color(c.Hat);
	co.Glasses = Net2Color(c.Glasses);
	return co;
}
void Ammo2Net(pb_size_t *ammoCount, NAmmo *ammo, const CArray *a)
{
	*ammoCount = 0;
	CA_FOREACH(int, amount, *a)
	if (*amount > 0)
	{
		ammo[*ammoCount].Id = _ca_index;
		ammo[*ammoCount].Amount = *amount;
		(*ammoCount)++;
	}
	CA_FOREACH_END()
}
