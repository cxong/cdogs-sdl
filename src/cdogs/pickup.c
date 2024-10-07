/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.
	Copyright (c) 2014-2015, 2017-2020, 2022-2024 Cong Xu
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
#include "pickup.h"

#include "json_utils.h"
#include "map.h"
#include "net_util.h"

CArray gPickups;
static unsigned int sPickupUIDs;
#define PICKUP_SIZE svec2i(8, 8)

void PickupsInit(void)
{
	CArrayInit(&gPickups, sizeof(Pickup));
	CArrayReserve(&gPickups, 128);
	sPickupUIDs = 0;
}
void PickupsTerminate(void)
{
	CA_FOREACH(const Pickup, p, gPickups)
	if (p->isInUse)
	{
		PickupDestroy(p->UID);
	}
	CA_FOREACH_END()
	CArrayTerminate(&gPickups);
}
int PickupsGetNextUID(void)
{
	return sPickupUIDs++;
}
static void PickupDraw(
	GraphicsDevice *g, const int id, const struct vec2i pos);
void PickupAdd(const NAddPickup ap)
{
	// Check if existing pickup
	Pickup *p = PickupGetByUID(ap.UID);
	if (p != NULL && p->isInUse)
	{
		PickupDestroy(ap.UID);
	}
	// Find an empty slot in pickup list
	p = NULL;
	int i;
	for (i = 0; i < (int)gPickups.size; i++)
	{
		Pickup *pu = CArrayGet(&gPickups, i);
		if (!pu->isInUse)
		{
			p = pu;
			break;
		}
	}
	if (p == NULL)
	{
		Pickup pu;
		memset(&pu, 0, sizeof pu);
		CArrayPushBack(&gPickups, &pu);
		i = (int)gPickups.size - 1;
		p = CArrayGet(&gPickups, i);
	}
	memset(p, 0, sizeof *p);
	p->UID = ap.UID;
	p->class = StrPickupClass(ap.PickupClass);
	ThingInit(&p->thing, i, KIND_PICKUP, PICKUP_SIZE, ap.ThingFlags);
	p->thing.CPic = p->class->Pic;
	p->thing.CPicFunc = PickupDraw;
	MapTryMoveThing(&gMap, &p->thing, NetToVec2(ap.Pos));
	p->IsRandomSpawned = ap.IsRandomSpawned;
	p->PickedUp = false;
	p->SpawnerUID = ap.SpawnerUID;
	p->isInUse = true;
}
void PickupAddGun(const WeaponClass *w, const struct vec2 pos)
{
	if (!w->CanDrop)
	{
		return;
	}
	GameEvent e = GameEventNew(GAME_EVENT_ADD_PICKUP);
	sprintf(e.u.AddPickup.PickupClass, "gun_%s", w->name);
	e.u.AddPickup.Pos = Vec2ToNet(pos);
	GameEventsEnqueue(&gGameEvents, e);
}
void PickupDestroy(const int uid)
{
	Pickup *p = PickupGetByUID(uid);
	CASSERT(p->isInUse, "Destroying not-in-use pickup");
	MapRemoveThing(&gMap, &p->thing);
	p->isInUse = false;
}

void PickupsUpdate(CArray *pickups, const int ticks)
{
	CA_FOREACH(Pickup, p, *pickups)
	if (!p->isInUse)
	{
		continue;
	}
	ThingUpdate(&p->thing, ticks);
	CA_FOREACH_END()
}

bool PickupIsManual(const Pickup *p)
{
	if (p->PickedUp)
		return false;
	CA_FOREACH(const PickupEffect, pe, p->class->Effects)
	switch (pe->Type)
	{
	case PICKUP_GUN:
		return true;
	case PICKUP_AMMO: {
		const Ammo *ammo = AmmoGetById(&gAmmo, pe->u.Ammo.Id);
		if (ammo->DefaultGun != NULL)
		{
			return true;
		}
	}
	break;
	case PICKUP_MENU:
		return true;
	default:
		break;
	}
	CA_FOREACH_END()
	return false;
}

static void PickupDraw(GraphicsDevice *g, const int id, const struct vec2i pos)
{
	const Pickup *p = CArrayGet(&gPickups, id);
	CASSERT(p->isInUse, "Cannot draw non-existent pickup");
	CPicDrawContext c = CPicDrawContextNew();
	const Pic *pic = CPicGetPic(&p->thing.CPic, c.Dir);
	if (pic != NULL)
	{
		c.Offset = svec2i_scale_divide(CPicGetSize(&p->class->Pic), -2);
	}
	CPicDraw(g, &p->thing.CPic, pos, &c);
}

Pickup *PickupGetByUID(const int uid)
{
	CA_FOREACH(Pickup, p, gPickups)
	if (p->UID == uid)
	{
		return p;
	}
	CA_FOREACH_END()
	return NULL;
}
