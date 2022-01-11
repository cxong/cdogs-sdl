/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.

    Copyright (c) 2013-2019, 2021 Cong Xu
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
#pragma once

#include "c_array.h"
#include "character.h"
#include "particle.h"
#include "proto/msg.pb.h"


// Game events represent anything that is created within the game but is
// required by outside systems, e.g. sound events
// This is to prevent the game from depending on these external systems

typedef enum
{
	GAME_EVENT_NONE,

	// Net initialisation messages
	GAME_EVENT_CLIENT_CONNECT,
	GAME_EVENT_CLIENT_ID,
	GAME_EVENT_CAMPAIGN_DEF,
	GAME_EVENT_PLAYER_DATA,
	GAME_EVENT_PLAYER_REMOVE,
	GAME_EVENT_TILE_SET,

	GAME_EVENT_THING_DAMAGE,
	GAME_EVENT_MAP_OBJECT_ADD,
	GAME_EVENT_MAP_OBJECT_REMOVE,
	GAME_EVENT_CLIENT_READY,
	GAME_EVENT_NET_GAME_START,

	GAME_EVENT_CONFIG,
	GAME_EVENT_SCORE,
	GAME_EVENT_SOUND_AT,
	GAME_EVENT_SCREEN_SHAKE,
	GAME_EVENT_SET_MESSAGE,

	// Use to signal start of game; useless for single player,
	// but for networked games it's used to set game ticks 0
	GAME_EVENT_GAME_START,
	GAME_EVENT_GAME_BEGIN,

	GAME_EVENT_ACTOR_ADD,
	GAME_EVENT_ACTOR_MOVE,
	GAME_EVENT_ACTOR_STATE,
	GAME_EVENT_ACTOR_DIR,
	GAME_EVENT_ACTOR_SLIDE,
	GAME_EVENT_ACTOR_IMPULSE,
	GAME_EVENT_ACTOR_SWITCH_GUN,
	GAME_EVENT_ACTOR_PICKUP_ALL,
	GAME_EVENT_ACTOR_REPLACE_GUN,
	GAME_EVENT_ACTOR_HEAL,
	GAME_EVENT_ACTOR_ADD_AMMO,
	GAME_EVENT_ACTOR_USE_AMMO,
	GAME_EVENT_ACTOR_DIE,
	GAME_EVENT_ACTOR_MELEE,
	GAME_EVENT_ACTOR_PILOT,

	GAME_EVENT_ADD_PICKUP,
	GAME_EVENT_REMOVE_PICKUP,

	GAME_EVENT_BULLET_BOUNCE,
	GAME_EVENT_REMOVE_BULLET,
	GAME_EVENT_PARTICLE_REMOVE,
	GAME_EVENT_GUN_FIRE,
	GAME_EVENT_GUN_RELOAD,
	GAME_EVENT_GUN_STATE,
	GAME_EVENT_ADD_BULLET,
	GAME_EVENT_ADD_PARTICLE,
	GAME_EVENT_TRIGGER,
	GAME_EVENT_EXPLORE_TILES,
	GAME_EVENT_RESCUE_CHARACTER,
	GAME_EVENT_OBJECTIVE_UPDATE,
	GAME_EVENT_ADD_KEYS,
	GAME_EVENT_DOOR_TOGGLE,

	// Can complete mission
	GAME_EVENT_MISSION_COMPLETE,

	// Left pickup area
	GAME_EVENT_MISSION_INCOMPLETE,
	// In pickup area
	GAME_EVENT_MISSION_PICKUP,
	GAME_EVENT_MISSION_END
} GameEventType;

// Which game events should be passed along to server or client
typedef struct
{
	GameEventType Type;
	// Whether this message is sent by server to clients
	bool Broadcast;
	// Whether clients send this message to the server
	bool Submit;
	// Whether to simply enqueue as game event; otherwise processed by handler
	bool Enqueue;
	// Whether to broadcast these events only after game start
	bool GameStart;
	const pb_msgdesc_t *Fields;
} GameEventEntry;
GameEventEntry GameEventGetEntry(const GameEventType e);

typedef struct
{
	GameEventType Type;
	int Delay;
	union
	{
		NPlayerData PlayerData;
		NPlayerRemove PlayerRemove;
		NTileSet TileSet;
		NThingDamage ThingDamage;
		NMapObjectAdd MapObjectAdd;
		NMapObjectRemove MapObjectRemove;
		NConfig Config;
		NScore Score;
		NSound SoundAt;
		struct
		{
			int Amount;
			bool CameraSubjectOnly;
			int ActorUID;
		} Shake;
		struct
		{
			char Message[256];
			int Ticks;
		} SetMessage;
		NGameBegin GameBegin;
		NActorAdd ActorAdd;
		NActorMove ActorMove;
		NActorState ActorState;
		NActorDir ActorDir;
		NActorSlide ActorSlide;
		NActorImpulse ActorImpulse;
		NActorSwitchGun ActorSwitchGun;
		NActorPickupAll ActorPickupAll;
		NActorReplaceGun ActorReplaceGun;
		NActorHeal Heal;
		NActorAddAmmo AddAmmo;
		NActorUseAmmo UseAmmo;
		NActorDie ActorDie;
		NActorMelee Melee;
		NActorPilot Pilot;
		NAddPickup AddPickup;
		NRemovePickup RemovePickup;
		struct
		{
			int UID;
			int Count;
		} ObjectSetCounter;
		NBulletBounce BulletBounce;
		NRemoveBullet RemoveBullet;
		int ParticleRemoveId;
		NGunFire GunFire;
		NGunReload GunReload;
		NGunState GunState;
		NAddBullet AddBullet;
		AddParticle AddParticle;
		NTrigger TriggerEvent;
		NExploreTiles ExploreTiles;
		NRescueCharacter Rescue;
		NObjectiveUpdate ObjectiveUpdate;
		NAddKeys AddKeys;
		NDoorToggle DoorToggle;
		NMissionComplete MissionComplete;
		NMissionEnd MissionEnd;
	} u;
} GameEvent;

extern CArray gGameEvents;	// of GameEvent

#define GAME_OVER_DELAY (FPS_FRAMELIMIT * 2)

void GameEventsInit(CArray *store);
void GameEventsTerminate(CArray *store);
void GameEventsEnqueue(CArray *store, GameEvent e);
void GameEventsClear(CArray *store);

GameEvent GameEventNew(GameEventType type);
GameEvent GameEventNewActorAdd(const struct vec2 pos, const Character *c, const bool isNPC);
