/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.
	Copyright (c) 2014-2022 Cong Xu
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

#include "actor_fire.h"
#include "actor_placement.h"
#include "actors.h"
#include "ai_utils.h"
#include "damage.h"
#include "events.h"
#include "game_events.h"
#include "joystick.h"
#include "log.h"
#include "net_server.h"
#include "particle.h"
#include "pickup.h"
#include "thing.h"
#include "triggers.h"

#define RELOAD_DISTANCE_PLUS 200

static void HandleGameEvent(
	const GameEvent e, Camera *camera, PowerupSpawner *healthSpawner,
	CArray *ammoSpawners, SoundDevice *sd);
void HandleGameEvents(
	CArray *store, Camera *camera, PowerupSpawner *healthSpawner,
	CArray *ammoSpawners, SoundDevice *sd)
{
	for (int i = 0; i < (int)store->size; i++)
	{
		GameEvent *e = CArrayGet(store, i);
		e->Delay--;
		if (e->Delay >= 0)
		{
			continue;
		}
		HandleGameEvent(*e, camera, healthSpawner, ammoSpawners, sd);
	}
	GameEventsClear(store);
}
static void HandleGameEvent(
	const GameEvent e, Camera *camera, PowerupSpawner *healthSpawner,
	CArray *ammoSpawners, SoundDevice *sd)
{
	switch (e.Type)
	{
	case GAME_EVENT_PLAYER_DATA:
		PlayerDataAddOrUpdate(e.u.PlayerData);
		break;
	case GAME_EVENT_PLAYER_REMOVE:
		PlayerRemove(e.u.PlayerRemove.UID);
		if (gPlayerDatas.size == 0)
		{
			// Waiting for players to join, follow the first one
			camera->FollowNextPlayer = true;
		}
		break;
	case GAME_EVENT_TILE_SET: {
		struct vec2i pos = Net2Vec2i(e.u.TileSet.Pos);
		LOG(LM_MAP, LL_DEBUG, "set tile %s/%s/%s pos(%d, %d) x%d",
			e.u.TileSet.ClassName, e.u.TileSet.DoorClassName,
			e.u.TileSet.DoorClass2Name, pos.x, pos.y, e.u.TileSet.RunLength);
		const TileClass *tileClass = StrTileClass(gMap.TileClasses, e.u.TileSet.ClassName);
		const TileClass *doorClass = StrTileClass(gMap.TileClasses, e.u.TileSet.DoorClassName);
		const TileClass *doorClass2 = StrTileClass(gMap.TileClasses, e.u.TileSet.DoorClass2Name);
		for (int i = 0; i <= e.u.TileSet.RunLength; i++)
		{
			Tile *t = MapGetTile(&gMap, pos);
			t->Class = tileClass;
			t->Door.Class = doorClass;
			t->Door.Class2 = doorClass2;
			DoorStateInit(&t->Door, false);
			pos.x++;
			if (pos.x == gMap.Size.x)
			{
				pos.x = 0;
				pos.y++;
			}
		}
	}
	break;
	case GAME_EVENT_THING_DAMAGE:
		ThingDamage(e.u.ThingDamage);
		break;
	case GAME_EVENT_MAP_OBJECT_ADD:
		ObjAdd(e.u.MapObjectAdd);
		break;
	case GAME_EVENT_MAP_OBJECT_REMOVE:
		ObjRemove(e.u.MapObjectRemove);
		break;
	case GAME_EVENT_CONFIG: {
		// Temporarily set config
		Config *c = ConfigGet(&gConfig, e.u.Config.Name);
		switch (c->Type)
		{
		case CONFIG_TYPE_STRING:
			CASSERT(false, "unimplemented");
			break;
		case CONFIG_TYPE_INT:
			c->u.Int.Value = atoi(e.u.Config.Value);
			break;
		case CONFIG_TYPE_FLOAT:
			c->u.Float.Value = atof(e.u.Config.Value);
			break;
		case CONFIG_TYPE_BOOL:
			c->u.Bool.Value = strcmp(e.u.Config.Value, "true") == 0;
			break;
		case CONFIG_TYPE_ENUM:
			c->u.Enum.Value = atoi(e.u.Config.Value);
			break;
		case CONFIG_TYPE_GROUP:
			CASSERT(false, "Cannot send groups over net");
			break;
		default:
			CASSERT(false, "Unknown config type");
			break;
		}
	}
	break;
	case GAME_EVENT_SCORE:
		// No score for dogfight
		if (gCampaign.Entry.Mode != GAME_MODE_DOGFIGHT)
		{
			PlayerData *p = PlayerDataGetByUID(e.u.Score.PlayerUID);
			PlayerScore(p, e.u.Score.Score);
			if (camera != NULL)
			{
				HUDNumPopupsAdd(
					&camera->HUD.numPopups, NUMBER_POPUP_SCORE,
					e.u.Score.PlayerUID, e.u.Score.Score);
			}
		}
		break;
	case GAME_EVENT_SOUND_AT:
		SoundPlayAtPlusDistance(
			sd, StrSound(e.u.SoundAt.Sound), NetToVec2(e.u.SoundAt.Pos),
			e.u.SoundAt.Distance);
		break;
	case GAME_EVENT_SCREEN_SHAKE:
		if (e.u.Shake.CameraSubjectOnly &&
			e.u.Shake.ActorUID != camera->FollowActorUID)
		{
			break;
		}
		camera->shake = ScreenShakeAdd(
			camera->shake, e.u.Shake.Amount,
			ConfigGetInt(&gConfig, "Graphics.ShakeMultiplier"));
		// Weak rumble for all joysticks
		CA_FOREACH(Joystick, j, gEventHandlers.joysticks)
		JoyRumble(j->id, 0.3f, 500);
		CA_FOREACH_END()
		break;
	case GAME_EVENT_SET_MESSAGE:
		HUDDisplayMessage(
			&camera->HUD, e.u.SetMessage.Message, e.u.SetMessage.Ticks);
		break;
	case GAME_EVENT_GAME_START:
		gMission.HasStarted = true;
		gMission.HasBegun = false;
		break;
	case GAME_EVENT_GAME_BEGIN:
		MissionBegin(&gMission, e.u.GameBegin);
		break;
	case GAME_EVENT_ACTOR_ADD: {
		ActorAdd(e.u.ActorAdd);
		const TActor *a = ActorGetByUID(e.u.ActorState.UID);
		// Spawn sound for player actors
		if (e.u.ActorAdd.PlayerUID >= 0)
		{
			SoundPlayAt(sd, StrSound("spawn"), a->Pos);
		}
	}
	break;
	case GAME_EVENT_ACTOR_MOVE:
		ActorMove(e.u.ActorMove);
		break;
	case GAME_EVENT_ACTOR_STATE: {
		TActor *a = ActorGetByUID(e.u.ActorState.UID);
		if (!a->isInUse)
			break;
		a->anim =
			AnimationGetActorAnimation((ActorAnimation)e.u.ActorState.State);
	}
	break;
	case GAME_EVENT_ACTOR_DIR: {
		TActor *a = ActorGetByUID(e.u.ActorDir.UID);
		if (!a->isInUse)
			break;
		a->direction = (direction_e)e.u.ActorDir.Dir;
	}
	break;
	case GAME_EVENT_ACTOR_SLIDE: {
		TActor *a = ActorGetByUID(e.u.ActorSlide.UID);
		if (!a->isInUse)
			break;
		a->thing.Vel = NetToVec2(e.u.ActorSlide.Vel);
		// Slide sound
		if (ConfigGetBool(&gConfig, "Sound.Footsteps"))
		{
			SoundPlayAt(sd, StrSound("slide"), a->thing.Pos);
		}
	}
	break;
	case GAME_EVENT_ACTOR_IMPULSE: {
		TActor *a = ActorGetByUID(e.u.ActorImpulse.UID);
		if (!a->isInUse)
			break;
		a->thing.Vel =
			svec2_add(a->thing.Vel, NetToVec2(e.u.ActorImpulse.Vel));
		const struct vec2 pos = NetToVec2(e.u.ActorImpulse.Pos);
		if (!svec2_is_zero(pos))
		{
			a->Pos = pos;
		}
	}
	break;
	case GAME_EVENT_ACTOR_SWITCH_GUN:
		ActorSwitchGun(e.u.ActorSwitchGun);
		break;
	case GAME_EVENT_ACTOR_PICKUP_ALL: {
		TActor *a = ActorGetByUID(e.u.ActorPickupAll.UID);
		if (!a->isInUse)
			break;
		a->PickupAll = e.u.ActorPickupAll.PickupAll;
	}
	break;
	case GAME_EVENT_ACTOR_REPLACE_GUN:
		ActorReplaceGun(e.u.ActorReplaceGun);
		break;
	case GAME_EVENT_ACTOR_HEAL: {
		TActor *a = ActorGetByUID(e.u.Heal.UID);
		if (!a->isInUse || a->dead)
			break;
		ActorHeal(a, e.u.Heal.Amount);
		// Tell the spawner that we took a health so we can
		// spawn more (but only if we're the server)
		if (e.u.Heal.IsRandomSpawned && !gCampaign.IsClient)
		{
			PowerupSpawnerRemoveOne(healthSpawner);
		}
		if (e.u.Heal.PlayerUID >= 0)
		{
			GameEvent s = GameEventNew(GAME_EVENT_ADD_PARTICLE);
			s.u.AddParticle.Class =
				StrParticleClass(&gParticleClasses, "heal_text");
			s.u.AddParticle.Pos = a->Pos;
			s.u.AddParticle.Z = BULLET_Z * Z_FACTOR;
			s.u.AddParticle.DZ = 3;
			sprintf(s.u.AddParticle.Text, "+%d", (int)e.u.Heal.Amount);
			GameEventsEnqueue(&gGameEvents, s);
		}
	}
	break;
	case GAME_EVENT_ACTOR_ADD_AMMO: {
		TActor *a = ActorGetByUID(e.u.AddAmmo.UID);
		if (!a->isInUse || a->dead)
			break;
		ActorAddAmmo(a, e.u.AddAmmo.Ammo.Id, e.u.AddAmmo.Ammo.Amount);
		// Tell the spawner that we took ammo so we can
		// spawn more (but only if we're the server)
		if (e.u.AddAmmo.IsRandomSpawned && !gCampaign.IsClient)
		{
			PowerupSpawnerRemoveOne(
				CArrayGet(ammoSpawners, e.u.AddAmmo.Ammo.Id));
		}
		if (e.u.AddAmmo.PlayerUID >= 0)
		{
			GameEvent s = GameEventNew(GAME_EVENT_ADD_PARTICLE);
			s.u.AddParticle.Class =
				StrParticleClass(&gParticleClasses, "ammo_text");
			s.u.AddParticle.Pos = a->Pos;
			s.u.AddParticle.Z = BULLET_Z * Z_FACTOR;
			s.u.AddParticle.DZ = 10;
			const Ammo *ammo = AmmoGetById(&gAmmo, e.u.AddAmmo.Ammo.Id);
			sprintf(
				s.u.AddParticle.Text, "+%d %s", (int)e.u.AddAmmo.Ammo.Amount,
				ammo->Name);
			GameEventsEnqueue(&gGameEvents, s);
		}
	}
	break;
	case GAME_EVENT_ACTOR_USE_AMMO: {
		TActor *a = ActorGetByUID(e.u.UseAmmo.UID);
		if (!a->isInUse || a->dead)
			break;
		const int ammoBefore =
			*(int *)CArrayGet(&a->ammo, e.u.UseAmmo.Ammo.Id);
		const Ammo *ammo = AmmoGetById(&gAmmo, e.u.UseAmmo.Ammo.Id);
		const bool wasAmmoLow = AmmoIsLow(ammo, ammoBefore);
		ActorAddAmmo(a, e.u.UseAmmo.Ammo.Id, -(int)e.u.UseAmmo.Ammo.Amount);
		const PlayerData *p = PlayerDataGetByUID(e.u.UseAmmo.PlayerUID);
		if (p != NULL && p->IsLocal)
		{
			// Show low or no ammo notifications
			const int ammoAfter =
				*(int *)CArrayGet(&a->ammo, e.u.UseAmmo.Ammo.Id);
			const bool isAmmoLow = AmmoIsLow(ammo, ammoAfter);
			if (ammoAfter == 0)
			{
				// No ammo
				SoundPlay(sd, StrSound("ammo_none"));
			}
			else if (!wasAmmoLow && isAmmoLow)
			{
				// Low ammo
				SoundPlay(sd, StrSound("ammo_low"));
			}
		}
	}
	break;
	case GAME_EVENT_ACTOR_DIE: {
		TActor *a = ActorGetByUID(e.u.ActorDie.UID);

		// Check if the player has lives to revive
		PlayerData *p = PlayerDataGetByUID(a->PlayerUID);
		if (p != NULL)
		{
			p->Lives--;
			CASSERT(p->Lives >= 0, "Player has died too many times");
			if (p->Lives > 0 && !gCampaign.IsClient)
			{
				// Find the closest player alive; try to spawn next to that
				// position if no other suitable position exists
				struct vec2 defaultSpawnPosition = svec2_zero();
				const TActor *closestActor = AIGetClosestPlayer(a->Pos);
				if (closestActor != NULL)
				{
					defaultSpawnPosition = closestActor->Pos;
				}
				PlacePlayer(&gMap, p, defaultSpawnPosition, false);
			}
		}

		ActorDestroy(a);
	}
	break;
	case GAME_EVENT_ACTOR_MELEE:
		DamageMelee(e.u.Melee);
		break;
	case GAME_EVENT_ACTOR_PILOT:
		ActorPilot(e.u.Pilot);
		break;
	case GAME_EVENT_ADD_PICKUP:
		PickupAdd(e.u.AddPickup);
		// Play a spawn sound
		SoundPlayAt(sd, StrSound("spawn_item"), NetToVec2(e.u.AddPickup.Pos));
		break;
	case GAME_EVENT_REMOVE_PICKUP:
		PickupDestroy(e.u.RemovePickup.UID);
		if (e.u.RemovePickup.SpawnerUID >= 0)
		{
			TObject *o = ObjGetByUID(e.u.RemovePickup.SpawnerUID);
			o->counter = AMMO_SPAWNER_RESPAWN_TICKS;
		}
		break;
	case GAME_EVENT_BULLET_BOUNCE:
		BulletBounce(e.u.BulletBounce);
		break;
	case GAME_EVENT_REMOVE_BULLET: {
		TMobileObject *o = MobObjGetByUID(e.u.RemoveBullet.UID);
		if (o == NULL || !o->isInUse)
			break;
		BulletDestroy(o);
	}
	break;
	case GAME_EVENT_PARTICLE_REMOVE:
		ParticleDestroy(&gParticles, e.u.ParticleRemoveId);
		break;
	case GAME_EVENT_GUN_FIRE:
		OnGunFire(e.u.GunFire, sd);
		break;
	case GAME_EVENT_GUN_RELOAD: {
		const WeaponClass *wc = StrWeaponClass(e.u.GunReload.Gun);
		CASSERT(wc->Type != GUNTYPE_MULTI, "unexpected gun type");
		const struct vec2 pos = NetToVec2(e.u.GunReload.Pos);
		SoundPlayAtPlusDistance(
			sd, wc->u.Normal.ReloadSound, pos, RELOAD_DISTANCE_PLUS);
		// Brass shells
		if (wc->u.Normal.Brass)
		{
			WeaponClassAddBrass(wc, (direction_e)e.u.GunReload.Direction, pos);
		}
	}
	break;
	case GAME_EVENT_GUN_STATE: {
		TActor *a = ActorGetByUID(e.u.GunState.ActorUID);
		if (!a->isInUse)
			break;
		WeaponBarrelSetState(
			ACTOR_GET_WEAPON(a), e.u.GunState.Barrel,
			(gunstate_e)e.u.GunState.State);
	}
	break;
	case GAME_EVENT_ADD_BULLET:
		BulletAdd(e.u.AddBullet);
		break;
	case GAME_EVENT_ADD_PARTICLE:
		ParticleAdd(&gParticles, e.u.AddParticle);
		break;
	case GAME_EVENT_TRIGGER: {
		const Tile *t = MapGetTile(&gMap, Net2Vec2i(e.u.TriggerEvent.Tile));
		CA_FOREACH(Trigger *, tp, t->triggers)
		if ((*tp)->id == (int)e.u.TriggerEvent.ID)
		{
			TriggerActivate(*tp, &gMap.triggers);
			break;
		}
		CA_FOREACH_END()
	}
	break;
	case GAME_EVENT_EXPLORE_TILES:
		// Process runs of explored tiles
		for (int i = 0; i < (int)e.u.ExploreTiles.Runs_count; i++)
		{
			struct vec2i tile = Net2Vec2i(e.u.ExploreTiles.Runs[i].Tile);
			for (int j = 0; j < e.u.ExploreTiles.Runs[i].Run; j++)
			{
				MapMarkAsVisited(&gMap, tile);
				tile.x++;
				if (tile.x == gMap.Size.x)
				{
					tile.x = 0;
					tile.y++;
				}
			}
		}
		break;
	case GAME_EVENT_RESCUE_CHARACTER: {
		TActor *a = ActorGetByUID(e.u.Rescue.UID);
		if (!a->isInUse)
			break;
		a->flags &= ~FLAGS_PRISONER;
		// If the actor isn't a follower, make them automatically run
		// towards the exit
		if (!(a->flags & FLAGS_FOLLOWER))
		{
			a->flags |= FLAGS_RESCUED;
		}
		SoundPlayAt(sd, StrSound("rescue"), a->Pos);
	}
	break;
	case GAME_EVENT_OBJECTIVE_UPDATE: {
		Objective *o = CArrayGet(
			&gMission.missionData->Objectives,
			e.u.ObjectiveUpdate.ObjectiveId);
		o->done += e.u.ObjectiveUpdate.Count;
		// Display a text update effect for the objective
		if (camera != NULL)
		{
			HUDNumPopupsAdd(
				&camera->HUD.numPopups, NUMBER_POPUP_OBJECTIVE,
				e.u.ObjectiveUpdate.ObjectiveId, e.u.ObjectiveUpdate.Count);
		}
		MissionSetMessageIfComplete(&gMission);
	}
	break;
	case GAME_EVENT_ADD_KEYS: {
		gMission.KeyFlags |= e.u.AddKeys.KeyFlags;

		const struct vec2 pos = NetToVec2(e.u.AddKeys.Pos);

		if (!svec2_is_zero(pos))
		{
			GameEvent s = GameEventNew(GAME_EVENT_ADD_PARTICLE);
			s.u.AddParticle.Class =
				StrParticleClass(&gParticleClasses, "key_text");
			s.u.AddParticle.Pos = pos;
			s.u.AddParticle.Z = BULLET_Z * Z_FACTOR;
			s.u.AddParticle.DZ = 10;
			sprintf(s.u.AddParticle.Text, "+key");
			GameEventsEnqueue(&gGameEvents, s);
		}

		// Clear cache since we may now have new paths
		PathCacheClear(&gPathCache);
	}
	break;
	case GAME_EVENT_DOOR_TOGGLE: {
		Tile *t = MapGetTile(&gMap, Net2Vec2i(e.u.DoorToggle.Pos));
		DoorStateInit(&t->Door, e.u.DoorToggle.IsOpen);
	}
	break;
	case GAME_EVENT_MISSION_COMPLETE:
		if (e.u.MissionComplete.ShowMsg)
		{
			if (!gMission.MissionCompleted)
			{
				SoundPlay(sd, StrSound("mission_complete"));
			}
			if (camera != NULL)
			{
				HUDDisplayMessage(&camera->HUD, "Mission complete", -1);
			}
		}
		gMission.MissionCompleted = true;
		// Don't show exit area or arrow if PVP
		if (!IsPVP(gCampaign.Entry.Mode))
		{
			if (camera != NULL)
			{
				camera->HUD.showExit = true;
			}
			for (int i = 0; i < (int)gMap.exits.size; i++)
			{
				MapShowExitArea(&gMap, i);
			}
		}
		break;
	case GAME_EVENT_MISSION_INCOMPLETE:
		gMission.state = MISSION_STATE_PLAY;
		break;
	case GAME_EVENT_MISSION_PICKUP:
		gMission.state = MISSION_STATE_PICKUP;
		gMission.pickupTime = gMission.time;
		SoundPlay(sd, StrSound("whistle"));
		break;
	case GAME_EVENT_MISSION_END:
		MissionDone(&gMission, e.u.MissionEnd);
		if (e.u.MissionEnd.Msg[0] != '\0')
		{
			HUDDisplayMessage(&camera->HUD, e.u.MissionEnd.Msg, -1);
		}
		break;
	default:
		assert(0 && "unknown game event");
		break;
	}
}
