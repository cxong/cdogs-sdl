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
#include "pickup.h"

#include "ammo.h"
#include "game_events.h"
#include "gamedata.h"
#include "json_utils.h"
#include "net_util.h"
#include "map.h"


CArray gPickups;
static unsigned int sPickupUIDs;


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
static const Pic *GetPickupPic(const int id, Vec2i *offset);
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
	p->tileItem.x = p->tileItem.y = -1;
	p->tileItem.flags = ap.TileItemFlags;
	p->tileItem.kind = KIND_PICKUP;
	p->tileItem.getPicFunc = GetPickupPic;
	p->tileItem.size = p->class->Pic->size;
	p->tileItem.id = i;
	MapTryMoveTileItem(&gMap, &p->tileItem, Net2Vec2i(ap.Pos));
	p->IsRandomSpawned = ap.IsRandomSpawned;
	p->PickedUp = false;
	p->SpawnerUID = ap.SpawnerUID;
	p->isInUse = true;
}
void PickupDestroy(const int uid)
{
	Pickup *p = PickupGetByUID(uid);
	CASSERT(p->isInUse, "Destroying not-in-use pickup");
	MapRemoveTileItem(&gMap, &p->tileItem);
	p->isInUse = false;
}

void PickupPickup(TActor *a, Pickup *p, const bool pickupAll)
{
	if (p->PickedUp) return;
	CASSERT(a->PlayerUID >= 0, "NPCs cannot pickup");
	bool canPickup = true;
	const char *sound = NULL;
	const Vec2i actorPos = Vec2iNew(a->tileItem.x, a->tileItem.y);
	switch (p->class->Type)
	{
	case PICKUP_JEWEL:
		{
			GameEvent e = GameEventNew(GAME_EVENT_SCORE);
			e.u.Score.PlayerUID = a->PlayerUID;
			e.u.Score.Score = p->class->u.Score;
			GameEventsEnqueue(&gGameEvents, e);
			sound = "pickup";
			UpdateMissionObjective(
				&gMission, p->tileItem.flags, OBJECTIVE_COLLECT, 1);
		}
		break;

	case PICKUP_HEALTH:
		// Don't pick up unless taken damage
		canPickup = false;
		if (a->health < ActorGetCharacter(a)->maxHealth)
		{
			canPickup = true;
			GameEvent e = GameEventNew(GAME_EVENT_ACTOR_HEAL);
			e.u.Heal.UID = a->uid;
			e.u.Heal.PlayerUID = a->PlayerUID;
			e.u.Heal.Amount = p->class->u.Health;
			e.u.Heal.IsRandomSpawned = p->IsRandomSpawned;
			GameEventsEnqueue(&gGameEvents, e);
		}
		break;

	case PICKUP_AMMO:
		{
			// Don't pickup if no guns can use ammo
			bool hasGunUsingAmmo = false;
			CA_FOREACH(const Weapon, w, a->guns)
				if (w->Gun->AmmoId == p->class->u.Ammo.Id)
				{
					hasGunUsingAmmo = true;
					break;
				}
			CA_FOREACH_END()
			if (!hasGunUsingAmmo)
			{
				canPickup = false;
				break;
			}

			// Don't pickup if ammo full
			const Ammo *ammo = AmmoGetById(&gAmmo, p->class->u.Ammo.Id);
			const int current = *(int *)CArrayGet(&a->ammo, p->class->u.Ammo.Id);
			if (current >= ammo->Max)
			{
				canPickup = false;
				break;
			}

			// Take ammo
			GameEvent e = GameEventNew(GAME_EVENT_ACTOR_ADD_AMMO);
			e.u.AddAmmo.UID = a->uid;
			e.u.AddAmmo.PlayerUID = a->PlayerUID;
			e.u.AddAmmo.AmmoId = p->class->u.Ammo.Id;
			e.u.AddAmmo.Amount = p->class->u.Ammo.Amount;
			e.u.AddAmmo.IsRandomSpawned = p->IsRandomSpawned;
			// Note: receiving end will prevent ammo from exceeding max
			GameEventsEnqueue(&gGameEvents, e);

			sound = ammo->Sound;
		}
		break;

	case PICKUP_KEYCARD:
		{
			GameEvent e = GameEventNew(GAME_EVENT_ADD_KEYS);
			e.u.AddKeys.KeyFlags = p->class->u.Keys;
			e.u.AddKeys.Pos = Vec2i2Net(actorPos);
			GameEventsEnqueue(&gGameEvents, e);
		}
		break;

	case PICKUP_GUN:
		if (pickupAll)
		{
			const GunDescription *gun = IdGunDescription(p->class->u.GunId);
			GameEvent e = GameEventNew(GAME_EVENT_ACTOR_REPLACE_GUN);
			e.u.ActorReplaceGun.UID = a->uid;
			e.u.ActorReplaceGun.GunIdx =
				a->guns.size == MAX_WEAPONS ? a->gunIndex : (int)a->guns.size;
			CASSERT(e.u.ActorReplaceGun.GunIdx <= a->guns.size,
				"invalid replace gun index");
			strcpy(e.u.ActorReplaceGun.Gun, gun->name);
			GameEventsEnqueue(&gGameEvents, e);

			// If the player has less ammo than the default amount,
			// replenish up to this amount
			const int ammoId = gun->AmmoId;
			if (ammoId >= 0)
			{
				const Ammo *ammo = AmmoGetById(&gAmmo, ammoId);
				const int ammoDeficit =
					ammo->Amount * 2 - *(int *)CArrayGet(&a->ammo, ammoId);
				if (ammoDeficit > 0)
				{
					e = GameEventNew(GAME_EVENT_ACTOR_ADD_AMMO);
					e.u.AddAmmo.UID = a->uid;
					e.u.AddAmmo.PlayerUID = a->PlayerUID;
					e.u.AddAmmo.AmmoId = ammoId;
					e.u.AddAmmo.Amount = ammoDeficit;
					e.u.AddAmmo.IsRandomSpawned = false;
					GameEventsEnqueue(&gGameEvents, e);
				}
			}
		}
		else
		{
			canPickup = false;
		}
		break;

	default:
		CASSERT(false, "unexpected pickup type");
		break;
	}
	if (canPickup)
	{
		if (sound != NULL)
		{
			GameEvent es = GameEventNew(GAME_EVENT_SOUND_AT);
			strcpy(es.u.SoundAt.Sound, sound);
			es.u.SoundAt.Pos = Vec2i2Net(actorPos);
			es.u.SoundAt.IsHit = false;
			GameEventsEnqueue(&gGameEvents, es);
		}
		GameEvent e = GameEventNew(GAME_EVENT_REMOVE_PICKUP);
		e.u.RemovePickup.UID = p->UID;
		e.u.RemovePickup.SpawnerUID = p->SpawnerUID;
		GameEventsEnqueue(&gGameEvents, e);
		// Prevent multiple pickups by marking
		p->PickedUp = true;
	}
}
bool PickupIsManual(const Pickup *p)
{
	if (p->PickedUp) return false;
	switch (p->class->Type)
	{
	case PICKUP_GUN:
		return true;
	default:
		return false;
	}
}

static const Pic *GetPickupPic(const int id, Vec2i *offset)
{
	const Pickup *p = CArrayGet(&gPickups, id);
	*offset = Vec2iScaleDiv(p->class->Pic->size, -2);
	return p->class->Pic;
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
