/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.

    Copyright (c) 2013-2014, Cong Xu
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
#ifndef __GAME_EVENTS
#define __GAME_EVENTS

#include "actors.h"
#include "c_array.h"
#include "gamedata.h"
#include "objs.h"

// Game events represent anything that is created within the game but is
// required by outside systems, e.g. sound events
// This is to prevent the game from depending on these external systems

typedef enum
{
	GAME_EVENT_SCORE,
	GAME_EVENT_SOUND_AT,
	GAME_EVENT_SCREEN_SHAKE,
	GAME_EVENT_SET_MESSAGE,

	// Use to signal start of game; useless for single player,
	// but for networked games it's used to set game ticks 0
	GAME_EVENT_GAME_START,

	GAME_EVENT_ADD_HEALTH_PICKUP,
	GAME_EVENT_TAKE_HEALTH_PICKUP,
	GAME_EVENT_MOBILE_OBJECT_REMOVE,
	GAME_EVENT_PARTICLE_REMOVE,
	GAME_EVENT_ADD_BULLET,
	GAME_EVENT_ADD_PARTICLE,
	GAME_EVENT_ADD_FIREBALL,
	GAME_EVENT_HIT_CHARACTER,
	GAME_EVENT_ACTOR_IMPULSE,
	GAME_EVENT_DAMAGE_CHARACTER,
	GAME_EVENT_TRIGGER,
	GAME_EVENT_UPDATE_OBJECTIVE,

	// Can complete mission
	GAME_EVENT_MISSION_COMPLETE,

	// Left pickup area
	GAME_EVENT_MISSION_INCOMPLETE,
	// In pickup area
	GAME_EVENT_MISSION_PICKUP,
	GAME_EVENT_MISSION_END
} GameEventType;

typedef struct
{
	GameEventType Type;
	union
	{
		struct
		{
			int PlayerIndex;
			int Score;
		} Score;
		struct
		{
			Mix_Chunk *Sound;
			Vec2i Pos;
		} SoundAt;
		int ShakeAmount;
		struct
		{
			char Message[256];
			int Ticks;
		} SetMessage;
		Vec2i AddPos;
		int PickupPlayer;
		int MobileObjectRemoveId;
		int ParticleRemoveId;
		struct
		{
			BulletType Bullet;
			Vec2i MuzzlePos;
			int MuzzleHeight;
			double Angle;
			direction_e Direction;
			int Flags;
			int PlayerIndex;
		} AddBullet;
		AddParticle AddParticle;
		AddFireballEvent AddFireball;
		struct
		{
			int Flags;
			int PlayerIndex;
			int TargetId;
			special_damage_e Special;
			bool HasHitSound;
		} HitCharacter;
		struct
		{
			int Id;
			Vec2i Vel;
		} ActorImpulse;
		struct
		{
			int Power;
			int PlayerIndex;
			int TargetId;
			int TargetPlayerIndex;
		} DamageCharacter;
		struct
		{
			int Id;
			Vec2i TilePos;
		} Trigger;
		struct
		{
			int ObjectiveIndex;
			int Update;
			int PlayerIndex;
			Vec2i Pos;
		} UpdateObjective;
	} u;
} GameEvent;

extern CArray gGameEvents;	// of GameEvent

void GameEventsInit(CArray *store);
void GameEventsTerminate(CArray *store);
void GameEventsEnqueue(CArray *store, GameEvent e);
void GameEventsClear(CArray *store);

void GameEventAddFireball(
	const BulletClass *class,
	const Vec2i fullPos, const int flags, const int playerIndex,
	const int dz, const int count, const double angle);
void GameEventAddFireballWreckage(const Vec2i fullPos);
void GameEventAddMolotovFlame(
	const Vec2i fullPos, const int flags, const int playerIndex);
void GameEventAddGasCloud(
	const BulletClass *class,
	const Vec2i fullPos, const int flags, const int playerIndex);

#endif
