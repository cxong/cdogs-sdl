/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.
	Copyright (c) 2014-2015, 2017-2020, 2022 Cong Xu
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

static bool TreatAsGunPickup(const PickupEffect *pe, const TActor *a);
static bool TryPickupAmmo(TActor *a, const Pickup *p, const PickupEffect *pe);
static bool TryPickupGun(
	TActor *a, const PickupEffect *pe, const bool pickupAll,
	const char **sound);
void PickupPickup(TActor *a, Pickup *p, const bool pickupAll)
{
	if (p->PickedUp)
		return;
	CASSERT(a->PlayerUID >= 0, "NPCs cannot pickup");
	// can always pickup effect-less pickups
	bool canPickup = p->class->Effects.size == 0;
	const char *sound = p->class->Sound;
	const struct vec2 actorPos = a->thing.Pos;

	CA_FOREACH(const PickupEffect, pe, p->class->Effects)
	switch (pe->Type)
	{
	case PICKUP_JEWEL: {
		canPickup = true;
		GameEvent e = GameEventNew(GAME_EVENT_SCORE);
		e.u.Score.PlayerUID = a->PlayerUID;
		e.u.Score.Score = pe->u.Score;
		GameEventsEnqueue(&gGameEvents, e);

		e = GameEventNew(GAME_EVENT_ADD_PARTICLE);
		e.u.AddParticle.Class =
			StrParticleClass(&gParticleClasses, "score_text");
		e.u.AddParticle.ActorUID = a->uid;
		e.u.AddParticle.Pos = p->thing.Pos;
		e.u.AddParticle.DZ = 3;
		if (gCampaign.Setting.Ammo)
		{
			sprintf(e.u.AddParticle.Text, "$%d", pe->u.Score);
		}
		else
		{
			sprintf(e.u.AddParticle.Text, "+%d", pe->u.Score);
		}
		GameEventsEnqueue(&gGameEvents, e);

		UpdateMissionObjective(
			&gMission, p->thing.flags, OBJECTIVE_COLLECT, 1);
	}
	break;

	case PICKUP_HEALTH:
		// Don't pick up unless taken damage
		if (a->health < ActorGetCharacter(a)->maxHealth)
		{
			canPickup = true;
			GameEvent e = GameEventNew(GAME_EVENT_ACTOR_HEAL);
			e.u.Heal.UID = a->uid;
			e.u.Heal.PlayerUID = a->PlayerUID;
			e.u.Heal.Amount = pe->u.Health;
			e.u.Heal.IsRandomSpawned = p->IsRandomSpawned;
			GameEventsEnqueue(&gGameEvents, e);
		}
		break;

	case PICKUP_AMMO: // fallthrough
	case PICKUP_GUN:
		if (TreatAsGunPickup(pe, a))
		{
			canPickup = TryPickupGun(a, pe, pickupAll, &sound) || canPickup;
		}
		else
		{
			canPickup = TryPickupAmmo(a, p, pe) || canPickup;
		}
		break;

	case PICKUP_KEYCARD: {
		canPickup = true;
		GameEvent e = GameEventNew(GAME_EVENT_ADD_KEYS);
		e.u.AddKeys.KeyFlags = pe->u.Keys;
		e.u.AddKeys.Pos = Vec2ToNet(actorPos);
		GameEventsEnqueue(&gGameEvents, e);
		if (sound == NULL)
		{
			sound = "key";
		}
	}
	break;

	case PICKUP_SHOW_MAP: {
		canPickup = true;
		GameEvent e = GameEventNew(GAME_EVENT_EXPLORE_TILES);
		e.u.ExploreTiles.Runs_count = 1;
		e.u.ExploreTiles.Runs[0].Run = gMap.Size.x * gMap.Size.y;
		GameEventsEnqueue(&gGameEvents, e);
	}
	break;

	case PICKUP_LIVES: {
		canPickup = true;
		GameEvent e = GameEventNew(GAME_EVENT_PLAYER_ADD_LIVES);
		e.u.PlayerAddLives.UID = a->PlayerUID;
		e.u.PlayerAddLives.Lives = pe->u.Lives;
		GameEventsEnqueue(&gGameEvents, e);
	}
	break;

	default:
		CASSERT(false, "unexpected pickup type");
		break;
	}
	CA_FOREACH_END()

	if (canPickup)
	{
		if (sound != NULL)
		{
			GameEvent es = GameEventNew(GAME_EVENT_SOUND_AT);
			strcpy(es.u.SoundAt.Sound, sound);
			es.u.SoundAt.Pos = Vec2ToNet(actorPos);
			GameEventsEnqueue(&gGameEvents, es);
		}
		GameEvent e = GameEventNew(GAME_EVENT_REMOVE_PICKUP);
		e.u.RemovePickup.UID = p->UID;
		e.u.RemovePickup.SpawnerUID = p->SpawnerUID;
		GameEventsEnqueue(&gGameEvents, e);
		// Prevent multiple pickups by marking
		p->PickedUp = true;
		a->PickupAll = false;
	}
}

static bool HasGunUsingAmmo(const TActor *a, const int ammoId);
static bool TreatAsGunPickup(const PickupEffect *pe, const TActor *a)
{
	// Grenades can also be gun pickups; treat as gun pickup if the player
	// doesn't have its ammo
	switch (pe->Type)
	{
	case PICKUP_AMMO:
		if (!HasGunUsingAmmo(a, pe->u.Ammo.Id))
		{
			const Ammo *ammo = AmmoGetById(&gAmmo, pe->u.Ammo.Id);
			if (ammo->DefaultGun)
			{
				return true;
			}
		}
		return false;
	case PICKUP_GUN: {
		const WeaponClass *wc = IdWeaponClass(pe->u.GunId);
		return wc->Type != GUNTYPE_GRENADE ||
			   !HasGunUsingAmmo(a, wc->u.Normal.AmmoId);
	}
	default:
		CASSERT(false, "unexpected pickup type");
		return false;
	}
}
static bool HasGunUsingAmmo(const TActor *a, const int ammoId)
{
	for (int i = 0; i < MAX_WEAPONS; i++)
	{
		const WeaponClass *wc = a->guns[i].Gun;
		if (wc == NULL)
			continue;
		for (int j = 0; j < WeaponClassNumBarrels(wc); j++)
		{
			if (WC_BARREL_ATTR(*wc, AmmoId, j) == ammoId)
			{
				return true;
			}
		}
	}
	return false;
}

static bool TryPickupAmmo(TActor *a, const Pickup *p, const PickupEffect *pe)
{
	// Don't pickup if not using ammo
	if (!gCampaign.Setting.Ammo)
	{
		return false;
	}
	// Don't pickup if ammo full
	const Ammo *ammo = AmmoGetById(
		&gAmmo, pe->Type == PICKUP_AMMO
					? (int)pe->u.Ammo.Id
					: IdWeaponClass(pe->u.GunId)->u.Normal.AmmoId);
	const int current = *(int *)CArrayGet(&a->ammo, pe->u.Ammo.Id);
	if (ammo->Max > 0 && current >= ammo->Max)
	{
		return false;
	}

	// Take ammo
	GameEvent e = GameEventNew(GAME_EVENT_ACTOR_ADD_AMMO);
	e.u.AddAmmo.UID = a->uid;
	e.u.AddAmmo.PlayerUID = a->PlayerUID;
	e.u.AddAmmo.Ammo.Id = pe->u.Ammo.Id;
	e.u.AddAmmo.Ammo.Amount = pe->u.Ammo.Amount;
	e.u.AddAmmo.IsRandomSpawned = p->IsRandomSpawned;
	// Note: receiving end will prevent ammo from exceeding max
	GameEventsEnqueue(&gGameEvents, e);
	return true;
}
static bool TryPickupGun(
	TActor *a, const PickupEffect *pe, const bool pickupAll,
	const char **sound)
{
	// Guns can only be picked up manually
	if (!pickupAll)
	{
		return false;
	}

	// When picking up a gun, the actor always ends up with it equipped, but:
	// - If the player already has the gun:
	//   - Switch to the same gun and drop the same gun
	// - If the player doesn't have the gun:
	//   - If the player has an empty slot, pickup the gun into that slot
	//   - If the player doesn't have an empty slot, replace the current gun,
	//     dropping it in the process

	const WeaponClass *wc =
		pe->Type == PICKUP_GUN
			? IdWeaponClass(pe->u.GunId)
			: StrWeaponClass(AmmoGetById(&gAmmo, pe->u.Ammo.Id)->DefaultGun);

	ActorPickupGun(a, wc);

	// If the player has less ammo than the default amount,
	// replenish up to this amount
	// TODO: support multi gun
	const int ammoId = WC_BARREL_ATTR(*wc, AmmoId, 0);
	if (ammoId >= 0)
	{
		const Ammo *ammo = AmmoGetById(&gAmmo, ammoId);
		const int ammoDeficit = ammo->Amount * AMMO_STARTING_MULTIPLE -
								*(int *)CArrayGet(&a->ammo, ammoId);
		if (ammoDeficit > 0)
		{
			GameEvent e = GameEventNew(GAME_EVENT_ACTOR_ADD_AMMO);
			e.u.AddAmmo.UID = a->uid;
			e.u.AddAmmo.PlayerUID = a->PlayerUID;
			e.u.AddAmmo.Ammo.Id = ammoId;
			e.u.AddAmmo.Ammo.Amount = ammoDeficit;
			e.u.AddAmmo.IsRandomSpawned = false;
			GameEventsEnqueue(&gGameEvents, e);

			// Also play an ammo pickup sound
			*sound = ammo->Sound;
		}
	}

	return true;
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
