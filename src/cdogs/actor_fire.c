/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (c) 2013-2017, 2019 Cong Xu
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
#include "actor_fire.h"

#include "net_util.h"


void ActorFire(Weapon *w, const TActor *a)
{
	if (w->state != GUNSTATE_FIRING && w->state != GUNSTATE_RECOIL)
	{
		GameEvent e = GameEventNew(GAME_EVENT_GUN_STATE);
		e.u.GunState.ActorUID = a->uid;
		e.u.GunState.State = GUNSTATE_FIRING;
		GameEventsEnqueue(&gGameEvents, e);
	}
	if (!w->Gun->CanShoot)
	{
		return;
	}

	const double radians = dir2radians[a->direction];
	const struct vec2 muzzleOffset = ActorGetMuzzleOffset(a, w);
	const struct vec2 muzzlePosition = svec2_add(a->Pos, muzzleOffset);
	const bool playSound = w->soundLock <= 0;
	WeaponClassFire(
		w->Gun, muzzlePosition,
		WeaponClassGetMuzzleHeight(w->Gun, GUNSTATE_FIRING), radians,
		a->flags, a->uid, playSound, true);
	WeaponOnFire(w);
}

void ActorFireUpdate(Weapon *w, const TActor *a, const int ticks)
{
	// Reload sound
	if (ConfigGetBool(&gConfig, "Sound.Reloads") &&
		w->lock > w->Gun->ReloadLead &&
		w->lock - ticks <= w->Gun->ReloadLead &&
		w->lock > 0 &&
		w->Gun->ReloadSound != NULL)
	{
		GameEvent e = GameEventNew(GAME_EVENT_GUN_RELOAD);
		e.u.GunReload.PlayerUID = a->PlayerUID;
		strcpy(e.u.GunReload.Gun, w->Gun->name);
		const struct vec2 muzzleOffset = ActorGetMuzzleOffset(a, w);
		const struct vec2 muzzlePosition = svec2_add(a->Pos, muzzleOffset);
		e.u.GunReload.Pos = Vec2ToNet(muzzlePosition);
		e.u.GunReload.Direction = (int)a->direction;
		GameEventsEnqueue(&gGameEvents, e);
	}
}
