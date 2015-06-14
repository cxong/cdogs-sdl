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

#include "ai_coop.h"
#include "ammo.h"
#include "events.h"
#include "game_events.h"
#include "json_utils.h"
#include "net_util.h"
#include "map.h"


CArray gPickups;
static int sPickupUIDs;


void PickupsInit(void)
{
	CArrayInit(&gPickups, sizeof(Pickup));
	CArrayReserve(&gPickups, 128);
	sPickupUIDs = 0;
}
void PickupsTerminate(void)
{
	for (int i = 0; i < (int)gPickups.size; i++)
	{
		const Pickup *p = CArrayGet(&gPickups, i);
		if (p->isInUse)
		{
			PickupDestroy(i);
		}
	}
	CArrayTerminate(&gPickups);
}
int PickupsGetNextUID(void)
{
	return sPickupUIDs;
}
static const Pic *GetPickupPic(const int id, Vec2i *offset);
void PickupAdd(const NAddPickup ap)
{
	// Find an empty slot in pickup list
	Pickup *p = NULL;
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
	while (ap.UID >= sPickupUIDs)
	{
		sPickupUIDs++;
	}
	p->class = StrPickupClass(ap.PickupClass);
	p->tileItem.x = p->tileItem.y = -1;
	p->tileItem.flags = ap.TileItemFlags;
	p->tileItem.kind = KIND_PICKUP;
	p->tileItem.getPicFunc = GetPickupPic;
	p->tileItem.getActorPicsFunc = NULL;
	p->tileItem.size = p->class->Pic->size;
	p->tileItem.id = i;
	MapTryMoveTileItem(&gMap, &p->tileItem, Net2Vec2i(ap.Pos));
	p->IsRandomSpawned = ap.IsRandomSpawned;
	p->SpawnerUID = ap.SpawnerUID;
	p->isInUse = true;
}
void PickupDestroy(const int id)
{
	Pickup *p = CArrayGet(&gPickups, id);
	CASSERT(p->isInUse, "Destroying not-in-use pickup");
	MapRemoveTileItem(&gMap, &p->tileItem);
	p->isInUse = false;
}

void PickupPickup(TActor *a, const Pickup *p)
{
	bool canPickup = true;
	Mix_Chunk *sound = NULL;
	const Vec2i actorPos = Vec2iNew(a->tileItem.x, a->tileItem.y);
	switch (p->class->Type)
	{
	case PICKUP_JEWEL:
		{
			GameEvent e = GameEventNew(GAME_EVENT_SCORE);
			e.u.Score.PlayerId = a->playerIndex;
			e.u.Score.Score = p->class->u.Score;
			GameEventsEnqueue(&gGameEvents, e);
			sound = gSoundDevice.pickupSound;
			UpdateMissionObjective(
				&gMission, p->tileItem.flags, OBJECTIVE_COLLECT);
		}
		break;

	case PICKUP_HEALTH:
		// Don't pick up unless taken damage
		canPickup = false;
		if (a->health < ActorGetCharacter(a)->maxHealth)
		{
			canPickup = true;
			GameEvent e = GameEventNew(GAME_EVENT_TAKE_HEALTH_PICKUP);
			e.u.Heal.PlayerIndex = a->playerIndex;
			e.u.Heal.Health = p->class->u.Health;
			e.u.Heal.IsRandomSpawned = p->IsRandomSpawned;
			GameEventsEnqueue(&gGameEvents, e);
			sound = gSoundDevice.healthSound;
		}
		break;

	case PICKUP_AMMO:
		{
			// Don't pickup if no guns can use ammo
			bool hasGunUsingAmmo = false;
			for (int i = 0; i < (int)a->guns.size; i++)
			{
				const Weapon *w = CArrayGet(&a->guns, i);
				if (w->Gun->AmmoId == p->class->u.Ammo.Id)
				{
					hasGunUsingAmmo = true;
					break;
				}
			}
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
			GameEvent e = GameEventNew(GAME_EVENT_TAKE_AMMO_PICKUP);
			e.u.AddAmmo.PlayerIndex = a->playerIndex;
			e.u.AddAmmo.AddAmmo = p->class->u.Ammo;
			e.u.AddAmmo.IsRandomSpawned = p->IsRandomSpawned;
			// Note: receiving end will prevent ammo from exceeding max
			GameEventsEnqueue(&gGameEvents, e);

			sound = StrSound(ammo->Sound);
		}
		break;

	case PICKUP_KEYCARD:
		gMission.flags |= p->class->u.Keys;
		sound = gSoundDevice.keySound;
		// Clear cache since we may now have new paths
		PathCacheClear(&gPathCache);
		break;

	case PICKUP_GUN:
		{
			if (a->PickupAll)
			{
				GameEvent e = GameEventNew(GAME_EVENT_ACTOR_REPLACE_GUN);
				e.u.ActorReplaceGun.UID = a->uid;
				e.u.ActorReplaceGun.GunIdx =
					a->guns.size == MAX_WEAPONS ?
					a->gunIndex : (int)a->guns.size;
				e.u.ActorReplaceGun.GunId = p->class->u.GunId;
				GameEventsEnqueue(&gGameEvents, e);

				// If the player has less ammo than the default amount,
				// replenish up to this amount
				const int ammoId = IdGunDescription(p->class->u.GunId)->AmmoId;
				if (ammoId >= 0)
				{
					const Ammo *ammo = AmmoGetById(&gAmmo, ammoId);
					const int ammoDeficit =
						ammo->Amount * 2 - *(int *)CArrayGet(&a->ammo, ammoId);
					if (ammoDeficit > 0)
					{
						e = GameEventNew(GAME_EVENT_USE_AMMO);
						e.u.UseAmmo.PlayerIndex = a->playerIndex;
						e.u.UseAmmo.UseAmmo.Id = ammoId;
						e.u.UseAmmo.UseAmmo.Amount = ammoDeficit;
						GameEventsEnqueue(&gGameEvents, e);
					}
				}

				sound = IdGunDescription(p->class->u.GunId)->SwitchSound;
			}
			else
			{
				a->CanPickupSpecial = true;
				canPickup = false;
				// "Say" that the weapon must be picked up using a command
				const PlayerData *pData =
					CArrayGet(&gPlayerDatas, a->playerIndex);
				const char *pickupKey = InputGetButtonName(
					pData->inputDevice, pData->deviceIndex, CMD_BUTTON2);
				if (pickupKey != NULL)
				{
					sprintf(a->Chatter, "%s to pick up\n%s",
						pickupKey,
						IdGunDescription(p->class->u.GunId)->name);
					a->ChatterCounter = 2;
				}
			}

			// If co-op AI, alert it so it can try to pick the gun up
			if (a->aiContext != NULL)
			{
				AICoopOnPickupGun(a, p->class->u.GunId);
			}
		}
		break;

	default:
		CASSERT(false, "unexpected pickup type");
		break;
	}
	if (canPickup)
	{
		SoundPlayAt(&gSoundDevice, sound, actorPos);

		// Alert spawner to start respawn process
		if (p->SpawnerUID >= 0)
		{
			GameEvent e = GameEventNew(GAME_EVENT_OBJECT_SET_COUNTER);
			e.u.ObjectSetCounter.UID = p->SpawnerUID;
			e.u.ObjectSetCounter.Count = AMMO_SPAWNER_RESPAWN_TICKS;
			GameEventsEnqueue(&gGameEvents, e);
		}

		PickupDestroy(p->tileItem.id);
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
	for (int i = 0; i < (int)gPickups.size; i++)
	{
		Pickup *p = CArrayGet(&gPickups, i);
		if (p->UID == uid)
		{
			return p;
		}
	}
	return NULL;
}
