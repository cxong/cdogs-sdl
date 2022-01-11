/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.

	Copyright (c) 2013-2020 Cong Xu
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
#include "game_events.h"

#include <string.h>

#include "actors.h"
#include "net_client.h"
#include "net_server.h"
#include "pickup.h"
#include "utils.h"

CArray gGameEvents;

void GameEventsInit(CArray *store)
{
	CArrayInit(store, sizeof(GameEvent));
}
void GameEventsTerminate(CArray *store)
{
	CArrayTerminate(store);
}

// Array indexed by GameEvent
static GameEventEntry sGameEventEntries[] = {
	{GAME_EVENT_NONE, false, false, false, false, NULL},

	{GAME_EVENT_CLIENT_CONNECT, false, false, false, false, NULL},
	{GAME_EVENT_CLIENT_ID, false, false, false, false, NClientId_fields},
	{GAME_EVENT_CAMPAIGN_DEF, false, false, false, false, NCampaignDef_fields},
	{GAME_EVENT_PLAYER_DATA, true, false, true, false, NPlayerData_fields},
	{GAME_EVENT_PLAYER_REMOVE, true, false, true, false, NPlayerRemove_fields},
	{GAME_EVENT_TILE_SET, true, false, true, true, NTileSet_fields},

	{GAME_EVENT_THING_DAMAGE, true, false, true, true, NThingDamage_fields},
	{GAME_EVENT_MAP_OBJECT_ADD, true, false, true, true, NMapObjectAdd_fields},
	{GAME_EVENT_MAP_OBJECT_REMOVE, true, false, true, true,
	 NMapObjectRemove_fields},
	{GAME_EVENT_CLIENT_READY, false, false, false, false, NULL},
	{GAME_EVENT_NET_GAME_START, false, false, false, false, NULL},

	{GAME_EVENT_CONFIG, true, false, true, false, NConfig_fields},
	{GAME_EVENT_SCORE, true, true, true, true, NScore_fields},
	{GAME_EVENT_SOUND_AT, true, false, true, true, NSound_fields},
	{GAME_EVENT_SCREEN_SHAKE, false, false, true, true, NULL},
	{GAME_EVENT_SET_MESSAGE, false, false, true, true, NULL},

	{GAME_EVENT_GAME_START, true, false, true, true, NULL},
	{GAME_EVENT_GAME_BEGIN, true, false, true, true, NGameBegin_fields},

	{GAME_EVENT_ACTOR_ADD, true, false, true, true, NActorAdd_fields},
	{GAME_EVENT_ACTOR_MOVE, true, true, true, true, NActorMove_fields},
	{GAME_EVENT_ACTOR_STATE, true, true, true, true, NActorState_fields},
	{GAME_EVENT_ACTOR_DIR, true, true, true, true, NActorDir_fields},
	{GAME_EVENT_ACTOR_SLIDE, true, true, true, true, NActorSlide_fields},
	{GAME_EVENT_ACTOR_IMPULSE, true, false, true, true, NActorImpulse_fields},
	{GAME_EVENT_ACTOR_SWITCH_GUN, true, true, true, true,
	 NActorSwitchGun_fields},
	{GAME_EVENT_ACTOR_PICKUP_ALL, false, true, true, true,
	 NActorPickupAll_fields},
	{GAME_EVENT_ACTOR_REPLACE_GUN, true, false, true, true,
	 NActorReplaceGun_fields},
	{GAME_EVENT_ACTOR_HEAL, true, false, true, true, NActorHeal_fields},
	{GAME_EVENT_ACTOR_ADD_AMMO, true, false, true, true, NActorAddAmmo_fields},
	{GAME_EVENT_ACTOR_USE_AMMO, true, true, true, true, NActorUseAmmo_fields},
	{GAME_EVENT_ACTOR_DIE, true, false, true, true, NActorDie_fields},
	{GAME_EVENT_ACTOR_MELEE, true, true, true, true, NActorMelee_fields},
	{GAME_EVENT_ACTOR_PILOT, true, true, true, true, NActorPilot_fields},

	{GAME_EVENT_ADD_PICKUP, true, false, true, true, NAddPickup_fields},
	{GAME_EVENT_REMOVE_PICKUP, true, false, true, true, NRemovePickup_fields},

	{GAME_EVENT_BULLET_BOUNCE, true, false, true, true, NBulletBounce_fields},
	{GAME_EVENT_REMOVE_BULLET, true, false, true, true, NRemoveBullet_fields},
	{GAME_EVENT_PARTICLE_REMOVE, false, false, true, true, NULL},
	{GAME_EVENT_GUN_FIRE, true, true, true, true, NGunFire_fields},
	{GAME_EVENT_GUN_RELOAD, true, true, true, true, NGunReload_fields},
	{GAME_EVENT_GUN_STATE, true, true, true, true, NGunState_fields},
	{GAME_EVENT_ADD_BULLET, true, false, true, true, NAddBullet_fields},
	{GAME_EVENT_ADD_PARTICLE, false, false, true, true, NULL},
	{GAME_EVENT_TRIGGER, true, false, true, true, NTrigger_fields},
	{GAME_EVENT_EXPLORE_TILES, true, false, true, true, NExploreTiles_fields},
	{GAME_EVENT_RESCUE_CHARACTER, true, false, true, true,
	 NRescueCharacter_fields},
	{GAME_EVENT_OBJECTIVE_UPDATE, true, false, true, true,
	 NObjectiveUpdate_fields},
	{GAME_EVENT_ADD_KEYS, true, false, true, true, NAddKeys_fields},
	{GAME_EVENT_DOOR_TOGGLE, true, false, true, true, NDoorToggle_fields},

	{GAME_EVENT_MISSION_COMPLETE, true, false, true, true,
	 NMissionComplete_fields},

	{GAME_EVENT_MISSION_INCOMPLETE, true, false, true, true, NULL},
	{GAME_EVENT_MISSION_PICKUP, true, false, true, true, NULL},
	{GAME_EVENT_MISSION_END, true, false, true, true, NMissionEnd_fields}};
GameEventEntry GameEventGetEntry(const GameEventType e)
{
	return sGameEventEntries[(int)e];
}

void GameEventsEnqueue(CArray *store, GameEvent e)
{
	if (store->elemSize == 0)
	{
		return;
	}
	// If we're the server, broadcast any events that clients need
	// If we're the client, pass along to server, but only if it's for a local
	// player Otherwise we'd ping-pong the same updates from the server
	const GameEventEntry gee = sGameEventEntries[e.Type];
	if (gee.Broadcast)
	{
		NetServerSendMsg(&gNetServer, NET_SERVER_BCAST, gee.Type, &e.u);
	}
	if (gee.Submit)
	{
		int actorUID = -1;
		bool actorIsLocal = false;
		switch (e.Type)
		{
		case GAME_EVENT_ACTOR_MOVE:
			actorUID = e.u.ActorMove.UID;
			break;
		case GAME_EVENT_ACTOR_STATE:
			actorUID = e.u.ActorState.UID;
			break;
		case GAME_EVENT_ACTOR_DIR:
			actorUID = e.u.ActorDir.UID;
			break;
		case GAME_EVENT_ACTOR_SLIDE:
			actorUID = e.u.ActorSlide.UID;
			break;
		case GAME_EVENT_ACTOR_SWITCH_GUN:
			actorUID = e.u.ActorSwitchGun.UID;
			break;
		case GAME_EVENT_ACTOR_PICKUP_ALL:
			actorUID = e.u.ActorPickupAll.UID;
			break;
		case GAME_EVENT_ACTOR_USE_AMMO:
			actorUID = e.u.UseAmmo.UID;
			break;
		case GAME_EVENT_ACTOR_MELEE:
			actorUID = e.u.Melee.UID;
			break;
		case GAME_EVENT_ACTOR_PILOT:
			actorUID = e.u.Pilot.UID;
			break;
		case GAME_EVENT_GUN_FIRE:
			if (e.u.GunFire.IsGun)
			{
				actorUID = e.u.GunFire.ActorUID;
			}
			break;
		case GAME_EVENT_GUN_RELOAD:
			actorIsLocal = PlayerIsLocal(e.u.GunReload.PlayerUID);
			break;
		case GAME_EVENT_GUN_STATE:
			actorUID = e.u.GunState.ActorUID;
			break;
		default:
			break;
		}
		if (actorUID >= 0)
		{
			actorIsLocal = ActorIsLocalPlayer(actorUID);
		}
		if (actorIsLocal)
		{
			NetClientSendMsg(&gNetClient, gee.Type, &e.u);
		}
	}

	CArrayPushBack(store, &e);
}
static bool EventComplete(const void *elem);
void GameEventsClear(CArray *store)
{
	CArrayRemoveIf(store, EventComplete);
}
static bool EventComplete(const void *elem)
{
	return ((const GameEvent *)elem)->Delay < 0;
}

GameEvent GameEventNew(GameEventType type)
{
	GameEvent e;
	memset(&e, 0, sizeof e);
	e.Type = type;
	switch (type)
	{
	case GAME_EVENT_MAP_OBJECT_REMOVE:
		e.u.MapObjectRemove.ActorUID = -1;
		break;
	case GAME_EVENT_ADD_PICKUP:
		e.u.AddPickup.UID = PickupsGetNextUID();
		e.u.AddPickup.SpawnerUID = -1;
		e.u.AddPickup.has_Pos = true;
		break;
	case GAME_EVENT_ACTOR_ADD:
		e.u.ActorAdd.PilotUID = -1;
		e.u.ActorAdd.VehicleUID = -1;
		e.u.ActorAdd.PlayerUID = -1;
		e.u.ActorAdd.Direction = rand() % DIRECTION_COUNT;
		e.u.ActorAdd.has_Pos = true;
		break;
	case GAME_EVENT_ACTOR_ADD_AMMO:
		e.u.AddAmmo.has_Ammo = true;
		break;
	case GAME_EVENT_ACTOR_IMPULSE:
		e.u.ActorImpulse.has_Vel = true;
		e.u.ActorImpulse.has_Pos = true;
		break;
	case GAME_EVENT_ACTOR_MOVE:
		e.u.ActorMove.has_Pos = true;
		e.u.ActorMove.has_MoveVel = true;
		break;
	case GAME_EVENT_ACTOR_SLIDE:
		e.u.ActorSlide.has_Vel = true;
		break;
	case GAME_EVENT_ACTOR_USE_AMMO:
		e.u.UseAmmo.has_Ammo = true;
		break;
	case GAME_EVENT_ADD_BULLET:
		e.u.AddBullet.ActorUID = -1;
		e.u.AddBullet.has_MuzzlePos = true;
		break;
	case GAME_EVENT_ADD_KEYS:
		e.u.AddKeys.has_Pos = true;
		break;
	case GAME_EVENT_BULLET_BOUNCE:
		e.u.BulletBounce.has_Vel = true;
		e.u.BulletBounce.has_Pos = true;
		e.u.BulletBounce.has_BouncePos = true;
		break;
	case GAME_EVENT_GUN_FIRE:
		e.u.GunFire.has_MuzzlePos = true;
		break;
	case GAME_EVENT_GUN_RELOAD:
		e.u.GunReload.has_Pos = true;
		break;
	case GAME_EVENT_MAP_OBJECT_ADD:
		e.u.MapObjectAdd.has_Pos = true;
		e.u.MapObjectAdd.has_Mask = true;
		break;
	case GAME_EVENT_SOUND_AT:
		e.u.SoundAt.has_Pos = true;
		break;
	case GAME_EVENT_THING_DAMAGE:
		e.u.ThingDamage.has_Vel = true;
		break;
	case GAME_EVENT_TILE_SET:
		e.u.TileSet.has_Pos = true;
		break;
	case GAME_EVENT_TRIGGER:
		e.u.TriggerEvent.has_Tile = true;
		break;
	case GAME_EVENT_PLAYER_DATA:
		e.u.PlayerData.has_Colors = true;
		e.u.PlayerData.has_Stats = true;
		e.u.PlayerData.has_Totals = true;
		break;
	default:
		break;
	}
	return e;
}
GameEvent GameEventNewActorAdd(const struct vec2 pos, const Character *c, const bool isNPC)
{
	GameEvent e = GameEventNew(GAME_EVENT_ACTOR_ADD);
	e.u.ActorAdd.Pos = Vec2ToNet(pos);
	e.u.ActorAdd.UID = ActorsGetNextUID();
	if (!c->Class->Vehicle)
	{
		e.u.ActorAdd.PilotUID = e.u.ActorAdd.UID;
	}
	e.u.ActorAdd.Health = CharacterGetStartingHealth(c, isNPC && !c->Class->Vehicle);
	return e;
}
