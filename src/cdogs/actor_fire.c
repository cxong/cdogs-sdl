/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.
	Copyright (c) 2013-2017, 2019, 2021 Cong Xu
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

#include "ai.h"
#include "net_util.h"
#include "objs.h"

void ActorFireBarrel(Weapon *w, const TActor *a, const int barrel)
{
	if (w->barrels[barrel].state != GUNSTATE_FIRING &&
		w->barrels[barrel].state != GUNSTATE_RECOIL)
	{
		GameEvent e = GameEventNew(GAME_EVENT_GUN_STATE);
		e.u.GunState.ActorUID = a->uid;
		e.u.GunState.Barrel = barrel;
		e.u.GunState.State = GUNSTATE_FIRING;
		GameEventsEnqueue(&gGameEvents, e);
	}
	if (!WeaponClassCanShoot(w->Gun))
	{
		return;
	}

	const double radians = dir2radians[a->direction];
	const struct vec2 muzzleOffset = ActorGetMuzzleOffset(a, w, barrel);
	const struct vec2 muzzlePosition = svec2_add(a->Pos, muzzleOffset);
	const bool playSound = w->barrels[barrel].soundLock <= 0;
	const TActor *firingActor = ActorGetByUID(a->pilotUID);
	WeaponClassFire(
		WeaponClassGetBarrel(w->Gun, barrel), muzzlePosition,
		WeaponClassGetMuzzleHeight(w->Gun, GUNSTATE_FIRING, barrel), radians,
		firingActor->flags, firingActor->uid, playSound, true);
	WeaponBarrelOnFire(w, barrel);
}

void ActorFireUpdate(Weapon *w, const TActor *a, const int ticks)
{
	// Reload sound
	if (!ConfigGetBool(&gConfig, "Sound.Reloads"))
	{
		for (int i = 0; i < WeaponClassNumBarrels(w->Gun); i++)
		{
			const int reloadLead = WC_BARREL_ATTR(*(w->Gun), ReloadLead, i);
			if (w->barrels[i].lock > reloadLead &&
				w->barrels[i].lock - ticks <= reloadLead &&
				w->barrels[i].lock > 0)
			{
				GameEvent e = GameEventNew(GAME_EVENT_GUN_RELOAD);
				e.u.GunReload.PlayerUID = a->PlayerUID;
				strcpy(
					e.u.GunReload.Gun, WeaponClassGetBarrel(w->Gun, i)->name);
				const struct vec2 muzzleOffset = ActorGetMuzzleOffset(a, w, i);
				const struct vec2 muzzlePosition =
					svec2_add(a->Pos, muzzleOffset);
				e.u.GunReload.Pos = Vec2ToNet(muzzlePosition);
				e.u.GunReload.Direction = a->direction;
				GameEventsEnqueue(&gGameEvents, e);
			}
		}
	}
}

void OnGunFire(const NGunFire gf, SoundDevice *sd)
{
	const WeaponClass *wc = StrWeaponClass(gf.Gun);
	CASSERT(wc->Type != GUNTYPE_MULTI, "unexpected gun type");
	const struct vec2 pos = NetToVec2(gf.MuzzlePos);

	// Add bullets
	if (wc->u.Normal.Bullet && !gCampaign.IsClient)
	{
		// Find the starting angle of the spread (clockwise)
		// Keep in mind the fencepost problem, i.e. spread of 3 means a
		// total spread angle of 2x width
		const float spreadStartAngle =
			wc->u.Normal.AngleOffset -
			(wc->u.Normal.Spread.Count - 1) * wc->u.Normal.Spread.Width / 2;
		for (int i = 0; i < wc->u.Normal.Spread.Count; i++)
		{
			const float recoil = RAND_FLOAT(-0.5f, 0.5f) * wc->u.Normal.Recoil;
			const float finalAngle = gf.Angle + spreadStartAngle +
									 i * wc->u.Normal.Spread.Width + recoil;
			GameEvent ab = GameEventNew(GAME_EVENT_ADD_BULLET);
			ab.u.AddBullet.UID = MobObjsObjsGetNextUID();
			strcpy(ab.u.AddBullet.BulletClass, wc->u.Normal.Bullet->Name);
			ab.u.AddBullet.MuzzlePos = Vec2ToNet(pos);
			ab.u.AddBullet.MuzzleHeight = gf.Z;
			ab.u.AddBullet.Angle = finalAngle;
			ab.u.AddBullet.Elevation = RAND_INT(
				wc->u.Normal.ElevationLow, wc->u.Normal.ElevationHigh);
			ab.u.AddBullet.Flags = gf.Flags;
			ab.u.AddBullet.ActorUID = gf.ActorUID;
			GameEventsEnqueue(&gGameEvents, ab);
		}
	}

	// Add muzzle flash
	if (WeaponClassHasMuzzle(wc) && wc->u.Normal.MuzzleFlash != NULL)
	{
		GameEvent ap = GameEventNew(GAME_EVENT_ADD_PARTICLE);
		ap.u.AddParticle.Class = wc->u.Normal.MuzzleFlash;
		ap.u.AddParticle.Pos = pos;
		ap.u.AddParticle.Z = (float)gf.Z;
		ap.u.AddParticle.Angle = gf.Angle;
		GameEventsEnqueue(&gGameEvents, ap);
	}
	// Sound
	if (gf.Sound && wc->u.Normal.Sound)
	{
		SoundPlayAt(sd, wc->u.Normal.Sound, pos);

		// Alert sleeping AI
		AIWakeOnSoundAt(pos);
	}
	// Screen shake
	if (wc->u.Normal.Shake.Amount > 0)
	{
		GameEvent s = GameEventNew(GAME_EVENT_SCREEN_SHAKE);
		s.u.Shake.Amount = wc->u.Normal.Shake.Amount;
		s.u.Shake.CameraSubjectOnly = wc->u.Normal.Shake.CameraSubjectOnly;
		s.u.Shake.ActorUID = gf.ActorUID;
		GameEventsEnqueue(&gGameEvents, s);
	}
	// Brass shells
	// If we have a reload lead, defer the creation of shells until then
	if (wc->u.Normal.Brass && wc->u.Normal.ReloadLead == 0)
	{
		const direction_e d = RadiansToDirection(gf.Angle);
		WeaponClassAddBrass(wc, d, pos);
	}
}
