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

#include <cdogs/damage.h>
#include <cdogs/game_events.h>
#include <cdogs/objs.h>
#include <cdogs/triggers.h>


static void HandleGameEvent(
	GameEvent *e,
	HUD *hud,
	ScreenShake *shake,
	HealthPickups *hp,
	EventHandlers *eventHandlers);
void HandleGameEvents(
	CArray *store,
	HUD *hud,
	ScreenShake *shake,
	HealthPickups *hp,
	EventHandlers *eventHandlers)
{
	for (int i = 0; i < (int)store->size; i++)
	{
		GameEvent *e = CArrayGet(store, i);
		HandleGameEvent(e, hud, shake, hp, eventHandlers);
	}
	GameEventsClear(store);
}
static void HandleGameEvent(
	GameEvent *e,
	HUD *hud,
	ScreenShake *shake,
	HealthPickups *hp,
	EventHandlers *eventHandlers)
{
	switch (e->Type)
	{
		case GAME_EVENT_SCORE:
			Score(&gPlayerDatas[e->u.Score.PlayerIndex], e->u.Score.Score);
			HUDAddScoreUpdate(hud, e->u.Score.PlayerIndex, e->u.Score.Score);
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
				*shake, e->u.ShakeAmount, gConfig.Graphics.ShakeMultiplier);
			break;
		case GAME_EVENT_SET_MESSAGE:
			HUDDisplayMessage(
				hud, e->u.SetMessage.Message, e->u.SetMessage.Ticks);
			break;
		case GAME_EVENT_GAME_START:
			if (eventHandlers->netInput.channel.state ==
				CHANNEL_STATE_CONNECTED)
			{
				NetInputSendMsg(
					&eventHandlers->netInput, SERVER_MSG_GAME_START);
			}
			break;
		case GAME_EVENT_ADD_HEALTH_PICKUP:
			MapPlaceHealth(e->u.AddPos);
			break;
		case GAME_EVENT_TAKE_HEALTH_PICKUP:
			if (IsPlayerAlive(e->u.PickupPlayer))
			{
				TActor *player = CArrayGet(
					&gActors, gPlayerIds[e->u.PickupPlayer]);
				ActorHeal(player, HEALTH_PICKUP_HEAL_AMOUNT);
				HealthPickupsRemoveOne(hp);
				HUDAddHealthUpdate(
					hud, e->u.PickupPlayer, HEALTH_PICKUP_HEAL_AMOUNT);
			}
			break;
		case GAME_EVENT_MOBILE_OBJECT_REMOVE:
			MobObjDestroy(e->u.MobileObjectRemoveId);
			break;
		case GAME_EVENT_ADD_BULLET:
			BulletAdd(
				e->u.AddBullet.Bullet,
				e->u.AddBullet.MuzzlePos, e->u.AddBullet.MuzzleHeight,
				e->u.AddBullet.Angle, e->u.AddBullet.Direction,
				e->u.AddBullet.Flags, e->u.AddBullet.PlayerIndex);
			break;
		case GAME_EVENT_ADD_MUZZLE_FLASH:
			AddMuzzleFlash(
				e->u.AddMuzzleFlash.FullPos,
				e->u.AddMuzzleFlash.MuzzleHeight,
				e->u.AddMuzzleFlash.SpriteName,
				e->u.AddMuzzleFlash.Direction,
				e->u.AddMuzzleFlash.Color,
				e->u.AddMuzzleFlash.Duration);
			break;
		case GAME_EVENT_ADD_FIREBALL:
			AddFireball(e->u.AddFireball);
			break;
		case GAME_EVENT_HIT_CHARACTER:
			HitCharacter(
				e->u.HitCharacter.Flags,
				e->u.HitCharacter.PlayerIndex,
				CArrayGet(&gActors, e->u.HitCharacter.TargetId),
				e->u.HitCharacter.Special,
				e->u.HitCharacter.HasHitSound);
			break;
		case GAME_EVENT_ACTOR_IMPULSE:
			{
				TActor *a = CArrayGet(&gActors, e->u.ActorImpulse.Id);
				a->Vel = Vec2iAdd(a->Vel, e->u.ActorImpulse.Vel);
			}
			break;
		case GAME_EVENT_DAMAGE_CHARACTER:
			DamageCharacter(
				e->u.DamageCharacter.Power,
				e->u.DamageCharacter.PlayerIndex,
				CArrayGet(&gActors, e->u.DamageCharacter.TargetId));
			if (e->u.DamageCharacter.Power != 0)
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
					{
						GameEvent e1;
						e1.Type = GAME_EVENT_SCORE;
						e1.u.Score.PlayerIndex =
							e->u.UpdateObjective.PlayerIndex;
						e1.u.Score.Score = PICKUP_SCORE;
						HandleGameEvent(&e1, hud, shake, hp, eventHandlers);
					}
					break;
				case OBJECTIVE_DESTROY:
					{
						GameEvent e1;
						e1.Type = GAME_EVENT_SCORE;
						e1.u.Score.PlayerIndex =
							e->u.UpdateObjective.PlayerIndex;
						e1.u.Score.Score = OBJECT_SCORE;
						HandleGameEvent(&e1, hud, shake, hp, eventHandlers);
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
