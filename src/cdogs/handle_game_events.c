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
#include "handle_game_events.h"

#include "damage.h"
#include "game_events.h"
#include "net_server.h"
#include "objs.h"
#include "particle.h"
#include "pickup.h"
#include "triggers.h"


static void HandleGameEvent(
	GameEvent *e,
	HUD *hud,
	ScreenShake *shake,
	PowerupSpawner *healthSpawner,
	CArray *ammoSpawners,
	EventHandlers *eventHandlers);
void HandleGameEvents(
	CArray *store,
	HUD *hud,
	ScreenShake *shake,
	PowerupSpawner *healthSpawner,
	CArray *ammoSpawners,
	EventHandlers *eventHandlers)
{
	for (int i = 0; i < (int)store->size; i++)
	{
		GameEvent *e = CArrayGet(store, i);
		HandleGameEvent(
			e, hud, shake, healthSpawner, ammoSpawners, eventHandlers);
	}
	GameEventsClear(store);
}
static void HandleGameEvent(
	GameEvent *e,
	HUD *hud,
	ScreenShake *shake,
	PowerupSpawner *healthSpawner,
	CArray *ammoSpawners,
	EventHandlers *eventHandlers)
{
	e->Delay--;
	if (e->Delay >= 0)
	{
		return;
	}
	switch (e->Type)
	{
	case GAME_EVENT_ADD_MAP_OBJECT:
		ObjAdd(e->u.AddMapObject);
		break;
	case GAME_EVENT_SCORE:
		{
			PlayerData *p = PlayerDataGetByUID(e->u.Score.PlayerUID);
			PlayerScore(p, e->u.Score.Score);
			HUDAddScoreUpdate(hud, e->u.Score.PlayerUID, e->u.Score.Score);
		}
		break;
	case GAME_EVENT_SOUND_AT:
		if (e->u.SoundAt.Sound)
		{
			SoundPlayAt(
				&gSoundDevice, e->u.SoundAt.Sound, e->u.SoundAt.Pos);
		}
		break;
	case GAME_EVENT_SCREEN_SHAKE:
		*shake = ScreenShakeAdd(
			*shake, e->u.ShakeAmount,
			ConfigGetInt(&gConfig, "Graphics.ShakeMultiplier"));
		break;
	case GAME_EVENT_SET_MESSAGE:
		HUDDisplayMessage(
			hud, e->u.SetMessage.Message, e->u.SetMessage.Ticks);
		break;
	case GAME_EVENT_GAME_START:
		gMission.HasStarted = true;
		break;
	case GAME_EVENT_ACTOR_ADD:
		ActorAdd(e->u.ActorAdd);
		break;
	case GAME_EVENT_ACTOR_MOVE:
		ActorMove(e->u.ActorMove);
		break;
	case GAME_EVENT_ACTOR_STATE:
		{
			TActor *a = ActorGetByUID(e->u.ActorState.UID);
			if (!a->isInUse)
			{
				break;
			}
			ActorSetState(a, (ActorAnimation)e->u.ActorState.State);
		}
		break;
	case GAME_EVENT_ACTOR_DIR:
		{
			TActor *a = ActorGetByUID(e->u.ActorDir.UID);
			if (!a->isInUse)
			{
				break;
			}
			a->direction = (direction_e)e->u.ActorDir.Dir;
		}
		break;
	case GAME_EVENT_ACTOR_REPLACE_GUN:
		{
			TActor *a = ActorGetByUID(e->u.ActorReplaceGun.UID);
			if (!a->isInUse)
			{
				break;
			}
			ActorReplaceGun(
				a, e->u.ActorReplaceGun.GunIdx,
				IdGunDescription(e->u.ActorReplaceGun.GunId));
		}
		break;
	case GAME_EVENT_ACTOR_HEAL:
		{
			TActor *a = CArrayGet(&gActors, e->u.Heal.UID);
			if (!a->isInUse || a->dead) break;
			ActorHeal(a, e->u.Heal.Amount);
			// Tell the spawner that we took a health so we can
			// spawn more (but only if we're the server)
			if (e->u.Heal.IsRandomSpawned && !gCampaign.IsClient)
			{
				PowerupSpawnerRemoveOne(healthSpawner);
			}
			if (e->u.Heal.PlayerUID >= 0)
			{
				HUDAddHealthUpdate(hud, e->u.Heal.PlayerUID, e->u.Heal.Amount);
			}
		}
		break;
	case GAME_EVENT_ACTOR_ADD_AMMO:
		{
			TActor *a = CArrayGet(&gActors, e->u.AddAmmo.UID);
			if (!a->isInUse || a->dead) break;
			ActorAddAmmo(a, e->u.AddAmmo.AmmoId, e->u.AddAmmo.Amount);
			// Tell the spawner that we took ammo so we can
			// spawn more (but only if we're the server)
			if (e->u.AddAmmo.IsRandomSpawned && !gCampaign.IsClient)
			{
				PowerupSpawnerRemoveOne(
					CArrayGet(ammoSpawners, e->u.AddAmmo.AmmoId));
			}
			if (e->u.AddAmmo.PlayerUID >= 0)
			{
				// TODO: some sort of text effect showing ammo grab
			}
		}
		break;
	case GAME_EVENT_ADD_PICKUP:
		{
			PickupAdd(e->u.AddPickup);
			// Play a spawn sound
			GameEvent sound = GameEventNew(GAME_EVENT_SOUND_AT);
			sound.u.SoundAt.Sound = StrSound("spawn_item");
			sound.u.SoundAt.Pos = Net2Vec2i(e->u.AddPickup.Pos);
			HandleGameEvent(
				&sound, hud, shake, healthSpawner, ammoSpawners,
				eventHandlers);
		}
		break;
	case GAME_EVENT_REMOVE_PICKUP:
		PickupDestroy(e->u.RemovePickup.UID);
		if (e->u.RemovePickup.SpawnerUID >= 0)
		{
			TObject *o = ObjGetByUID(e->u.RemovePickup.SpawnerUID);
			o->counter = AMMO_SPAWNER_RESPAWN_TICKS;
		}
		break;
	case GAME_EVENT_USE_AMMO:
		{
			const PlayerData *p = PlayerDataGetByUID(e->u.UseAmmo.PlayerUID);
			if (IsPlayerAlive(p))
			{
				TActor *a = ActorGetByUID(p->ActorUID);
				if (!a->isInUse)
				{
					break;
				}
				ActorAddAmmo(
					a, e->u.UseAmmo.UseAmmo.Id, e->u.UseAmmo.UseAmmo.Amount);
				// TODO: some sort of text effect showing ammo usage
			}
		}
		break;
	case GAME_EVENT_MOBILE_OBJECT_REMOVE:
		MobObjDestroy(e->u.MobileObjectRemoveId);
		break;
	case GAME_EVENT_PARTICLE_REMOVE:
		ParticleDestroy(&gParticles, e->u.ParticleRemoveId);
		break;
	case GAME_EVENT_ADD_BULLET:
		BulletAdd(e->u.AddBullet);
		break;
	case GAME_EVENT_ADD_PARTICLE:
		ParticleAdd(&gParticles, e->u.AddParticle);
		break;
	case GAME_EVENT_HIT_CHARACTER:
		ActorTakeHit(
			CArrayGet(&gActors, e->u.HitCharacter.TargetId),
			e->u.HitCharacter.Special);
		break;
	case GAME_EVENT_ACTOR_IMPULSE:
		{
			TActor *a = CArrayGet(&gActors, e->u.ActorImpulse.Id);
			if (!a->isInUse)
			{
				break;
			}
			a->Vel = Vec2iAdd(a->Vel, e->u.ActorImpulse.Vel);
		}
		break;
	case GAME_EVENT_DAMAGE_CHARACTER:
		DamageActor(e->u.ActorDamage);
		if (e->u.ActorDamage.Power != 0 &&
			e->u.ActorDamage.TargetPlayerUID >= 0)
		{
			HUDAddHealthUpdate(
				hud,
				e->u.ActorDamage.TargetPlayerUID,
				-e->u.ActorDamage.Power);
		}
		break;
	case GAME_EVENT_TRIGGER:
		{
			const Tile *t = MapGetTile(&gMap, e->u.TriggerEvent.TilePos);
			for (int i = 0; i < (int)t->triggers.size; i++)
			{
				Trigger **tp = CArrayGet(&t->triggers, i);
				if ((*tp)->id == e->u.TriggerEvent.Id)
				{
					TriggerActivate(*tp, &gMap.triggers);
					break;
				}
			}
		}
		break;
	case GAME_EVENT_EXPLORE_TILE:
		MapMarkAsVisited(&gMap, Net2Vec2i(e->u.ExploreTile.Tile));
		// Check if we need to update explore objectives
		for (int i = 0; i < (int)gMission.missionData->Objectives.size; i++)
		{
			const MissionObjective *mobj =
				CArrayGet(&gMission.missionData->Objectives, i);
			if (mobj->Type != OBJECTIVE_INVESTIGATE) continue;
			const ObjectiveDef *o = CArrayGet(&gMission.Objectives, i);
			const int update = MapGetExploredPercentage(&gMap) - o->done;
			if (update > 0)
			{
				GameEvent ou = GameEventNew(GAME_EVENT_OBJECTIVE_UPDATE);
				ou.u.ObjectiveUpdate.ObjectiveId = i;
				ou.u.ObjectiveUpdate.Count = update;
				HandleGameEvent(
					&ou, hud, shake, healthSpawner, ammoSpawners,
					eventHandlers);
			}
		}
		break;
	case GAME_EVENT_OBJECTIVE_UPDATE:
		{
			ObjectiveDef *o = CArrayGet(
				&gMission.Objectives, e->u.ObjectiveUpdate.ObjectiveId);
			o->done += e->u.ObjectiveUpdate.Count;
			// Display a text update effect for the objective
			HUDAddObjectiveUpdate(
				hud,
				e->u.ObjectiveUpdate.ObjectiveId,
				e->u.ObjectiveUpdate.Count);
			MissionSetMessageIfComplete(&gMission);
		}
		break;
	case GAME_EVENT_MISSION_COMPLETE:
		HUDDisplayMessage(hud, "Mission complete", -1);
		hud->showExit = true;
		MapShowExitArea(&gMap);
		break;
	case GAME_EVENT_MISSION_INCOMPLETE:
		gMission.state = MISSION_STATE_PLAY;
		break;
	case GAME_EVENT_MISSION_PICKUP:
		gMission.state = MISSION_STATE_PICKUP;
		gMission.pickupTime = gMission.time;
		SoundPlay(&gSoundDevice, StrSound("whistle"));
		break;
	case GAME_EVENT_MISSION_END:
		gMission.isDone = true;
		break;
	default:
		assert(0 && "unknown game event");
		break;
	}
}
