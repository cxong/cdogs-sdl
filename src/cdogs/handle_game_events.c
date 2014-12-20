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
#include "handle_game_events.h"

#include "damage.h"
#include "game_events.h"
#include "net_server.h"
#include "objs.h"
#include "particle.h"
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
		case GAME_EVENT_SCORE:
			if (e->u.Score.PlayerIndex >= 0)
			{
				PlayerData *p =
					CArrayGet(&gPlayerDatas, e->u.Score.PlayerIndex);
				PlayerScore(p, e->u.Score.Score);
				HUDAddScoreUpdate(
					hud, e->u.Score.PlayerIndex, e->u.Score.Score);
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
			NetServerBroadcastMsg(
				&gNetServer, SERVER_MSG_GAME_START, NULL);
			break;
		case GAME_EVENT_ACTOR_ADD:
			ActorAdd(e->u.ActorAdd);
			break;
		case GAME_EVENT_ACTOR_MOVE:
			{
				TActor *a = CArrayGet(&gActors, e->u.ActorMove.Id);
				if (!a->isInUse)
				{
					break;
				}
				a->Pos.x = e->u.ActorMove.Pos.x;
				a->Pos.y = e->u.ActorMove.Pos.y;
				MapTryMoveTileItem(&gMap, &a->tileItem, Vec2iFull2Real(a->Pos));
				if (MapIsTileInExit(&gMap, &a->tileItem))
				{
					a->action = ACTORACTION_EXITING;
				}
				else
				{
					a->action = ACTORACTION_MOVING;
				}
			}
			break;
		case GAME_EVENT_ADD_HEALTH_PICKUP:
			MapPlaceHealth(e->u.AddHealthPickup);
			break;
		case GAME_EVENT_TAKE_HEALTH_PICKUP:
			{
				const PlayerData *p =
					CArrayGet(&gPlayerDatas, e->u.Heal.PlayerIndex);
				if (IsPlayerAlive(p))
				{
					TActor *a = CArrayGet(&gActors, p->Id);
					if (!a->isInUse)
					{
						break;
					}
					ActorHeal(a, e->u.Heal.Health);
					if (e->u.Heal.IsRandomSpawned)
					{
						PowerupSpawnerRemoveOne(healthSpawner);
					}
					HUDAddHealthUpdate(
						hud, e->u.Heal.PlayerIndex, e->u.Heal.Health);
				}
			}
			break;
		case GAME_EVENT_ADD_AMMO_PICKUP:
			MapPlaceAmmo(e->u.AddAmmoPickup);
			break;
		case GAME_EVENT_TAKE_AMMO_PICKUP:
		{
			const PlayerData *p =
				CArrayGet(&gPlayerDatas, e->u.Heal.PlayerIndex);
			if (IsPlayerAlive(p))
			{
				TActor *a = CArrayGet(&gActors, p->Id);
				if (!a->isInUse)
				{
					break;
				}
				ActorAddAmmo(a, e->u.AddAmmo.AddAmmo);
				if (e->u.AddAmmo.IsRandomSpawned)
				{
					PowerupSpawnerRemoveOne(
						CArrayGet(ammoSpawners, e->u.AddAmmo.AddAmmo.Id));
				}
				// TODO: some sort of text effect showing ammo grab
			}
		}
		break;
		case GAME_EVENT_USE_AMMO:
		{
			const PlayerData *p =
				CArrayGet(&gPlayerDatas, e->u.UseAmmo.PlayerIndex);
			if (IsPlayerAlive(p))
			{
				TActor *a = CArrayGet(&gActors, p->Id);
				if (!a->isInUse)
				{
					break;
				}
				ActorAddAmmo(a, e->u.UseAmmo.UseAmmo);
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
			DamageCharacter(
				e->u.DamageCharacter.Power,
				e->u.DamageCharacter.PlayerIndex,
				CArrayGet(&gActors, e->u.DamageCharacter.TargetId));
			if (e->u.DamageCharacter.Power != 0 &&
				e->u.DamageCharacter.TargetPlayerIndex >= 0)
			{
				HUDAddHealthUpdate(
					hud,
					e->u.DamageCharacter.TargetPlayerIndex,
					-e->u.DamageCharacter.Power);
			}
			break;
		case GAME_EVENT_TRIGGER:
			{
				const Tile *t = MapGetTile(&gMap, e->u.Trigger.TilePos);
				for (int i = 0; i < (int)t->triggers.size; i++)
				{
					Trigger **tp = CArrayGet(&t->triggers, i);
					if ((*tp)->id == e->u.Trigger.Id)
					{
						TriggerActivate(*tp, &gMap.triggers);
						break;
					}
				}
			}
			break;
		case GAME_EVENT_UPDATE_OBJECTIVE:
			{
				struct Objective *o = CArrayGet(
					&gMission.Objectives, e->u.UpdateObjective.ObjectiveIndex);
				o->done += e->u.UpdateObjective.Update;
				MissionObjective *mo = CArrayGet(
					&gMission.missionData->Objectives,
					e->u.UpdateObjective.ObjectiveIndex);
				switch (mo->Type)
				{
				case OBJECTIVE_COLLECT:
					// Do nothing (already handled in pickups)
					break;
				case OBJECTIVE_DESTROY:
					{
						GameEvent e1 = GameEventNew(GAME_EVENT_SCORE);
						e1.u.Score.PlayerIndex =
							e->u.UpdateObjective.PlayerIndex;
						e1.u.Score.Score = OBJECT_SCORE;
						HandleGameEvent(
							&e1, hud, shake, healthSpawner, ammoSpawners,
							eventHandlers);
					}
					break;
				default:
					// No other special objective handling
					break;
				}
				// Display a text update effect for the objective
				HUDAddObjectiveUpdate(
					hud,
					e->u.UpdateObjective.ObjectiveIndex,
					e->u.UpdateObjective.Update);
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
