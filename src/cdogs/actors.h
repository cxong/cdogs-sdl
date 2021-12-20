/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.
	Copyright (C) 1995 Ronny Wester
	Copyright (C) 2003 Jeremy Chin
	Copyright (C) 2003-2007 Lucas Martin-King

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

	This file incorporates work covered by the following copyright and
	permission notice:

	Copyright (c) 2013-2021 Cong Xu
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

#include "ai_context.h"
#include "animation.h"
#include "emitter.h"
#include "game_mode.h"
#include "grafx.h"
#include "mathc/mathc.h"
#include "player.h"
#include "thing.h"
#include "weapon.h"

#define ACTOR_W 14
#define ACTOR_H 10

#define FLAGS_HURTALWAYS (1 << 2)

// Computer characters only
#define FLAGS_DETOURING (1 << 5)
#define FLAGS_TRYRIGHT (1 << 6)
#define FLAGS_SLEEPING (1 << 7)
#define FLAGS_VISIBLE (1 << 8)
#define FLAGS_ASBESTOS (1 << 9)	   // Immune to fire
#define FLAGS_IMMUNITY (1 << 10)   // Immune to poison
#define FLAGS_SEETHROUGH (1 << 11) // Almost transparent
#define FLAGS_DEAF (1 << 12)
#define FLAGS_ALL_SEEING (1 << 13)
#define FLAGS_UNLOCK_DOORS (1 << 14)
#define FLAGS_WAKING (1 << 15)

// Special flags
#define FLAGS_RUNS_AWAY (1 << 16) // Move directly away from player
#define FLAGS_GOOD_GUY (1 << 17)  // Shots cannot hurt player
#define FLAGS_PRISONER (1 << 18)  // Won't move until touched by player
#define FLAGS_INVULNERABLE (1 << 19)
#define FLAGS_FOLLOWER (1 << 20) // Follows player
#define FLAGS_PENALTY (1 << 21)	 // Big score penalty if shot
#define FLAGS_VICTIM (1 << 22)	 // Can be shot by everyone
#define FLAGS_SNEAKY (1 << 23)	 // Always shoot back when player shoots
#define FLAGS_SLEEPALWAYS (1 << 24)
#define FLAGS_AWAKEALWAYS (1 << 25)
#define FLAGS_RESCUED (1 << 26) // Run towards exit

typedef enum
{
	ACTORACTION_MOVING,
	ACTORACTION_EXITING
} ActorAction;

typedef struct Actor
{
	struct vec2 Pos;
	// Vector that the player is attempting to move in, based on input
	struct vec2 MoveVel;
	direction_e direction;
	// Rotation used to draw the actor, which will lag behind the actual
	// rotation in order to show smooth rotation
	float DrawRadians;
	Animation anim;
	int stateCounter;
	int lastCmd;
	// Whether the player ran into something whilst trying to move
	// In this situation, we interrupt dead reckoning and resend the position
	bool hasCollided;
	// Whether the last special command was performed with a direction
	// This differentiates between a special command and weapon switch
	bool specialCmdDir;
	// If the actor wants to pick up everything, even those that need to be
	// manually picked up using special input
	bool PickupAll;
	// Flag to specify whether this actor is over a special pickup,
	// such as weapons which will replace the current weapon.
	// If the player presses switch when in this state, instead of switching
	// weapons, pick up.
	bool CanPickupSpecial;

	// -1 if human character (get from player data), otherwise index into
	// CharacterStore OtherChars
	int charId;
	int PlayerUID; // -1 unless a human player
	int uid;	   // unique ID across all actors
	int pilotUID;  // the actor that controls this
				  // (same as uid for normal actors)
	int vehicleUID; // -1 unless piloting a vehicle
	Weapon guns[MAX_WEAPONS];
	CArray ammo; // of int
	int gunIndex;
	// TODO: multiple grenade slots?
	int grenadeIndex;

	int health;
	// A counter for player death
	// If 0, player is alive
	// If >0 but less than deathPics, player is "dying" and this variable
	// is used as the frame in the death animation
	int dead;
	int flamed;
	int poisoned;
	int petrified;
	int confused;
	int flags;

	int turns;

	int slideLock;
	// For damage text
	int accumulatedDamage;
	// Facial expression when melee or taking damage
	int grimaceCounter;

	// What to say (text label appears above actor) and how long to say it
	char Chatter[256];
	int ChatterCounter;

	Emitter barrelSmoke;
	Emitter healEffect;

	// Gore emitters
	Emitter blood1;
	Emitter blood2;
	Emitter blood3;
	int bleedCounter;

	color_t footprintMask;
	int footprintCounter;

	// Signals to other AIs what this actor is doing
	ActorAction action;
	AIContext *aiContext;
	Thing thing;
	bool isInUse;
} TActor;

// Store all actors in-line in an array
// Destroying actors does not modify the array, only switches
// a boolean flag isInUse to denote whether this instance is
// in use.
// Actors should be identified by index
// There is a small chance of invalidating pointers if
// actors are added and the array must be resized.
// Therefore do not hold actor pointers and reuse.
extern CArray gActors; // of TActor

void ActorSetState(TActor *actor, const ActorAnimation state);
void UpdateActorState(TActor *actor, int ticks);
bool TryMoveActor(TActor *actor, struct vec2 pos);
void ActorMove(const NActorMove am);
void CommandActor(TActor *actor, int cmd, int ticks);
void SlideActor(TActor *actor, int cmd);
void UpdateAllActors(const int ticks);
void ActorsPilotVehicles(void);
void ActorHeal(TActor *actor, int health);
void InjureActor(TActor *actor, int injury);

void ActorAddAmmo(TActor *actor, const int ammoId, const int amount);
// Whether the actor has a gun that uses this ammo
bool ActorUsesAmmo(const TActor *actor, const int ammoId);
void ActorReplaceGun(const NActorReplaceGun rg);
void ActorSetAIState(TActor *actor, const AIState s);
void ActorPilot(const NActorPilot ap);

void ActorsInit(void);
void ActorsTerminate(void);
int ActorsGetNextUID(void);
int ActorsGetFreeIndex(void);
TActor *ActorAdd(NActorAdd aa);
void ActorDestroy(TActor *a);

TActor *ActorGetByUID(const int uid);
const Character *ActorGetCharacter(const TActor *a);
#define ACTOR_GET_GUN(a) (&(a)->guns[(a)->gunIndex])
#define ACTOR_GET_GRENADE(a) (&(a)->guns[(a)->grenadeIndex + MAX_GUNS])
#define ACTOR_GET_WEAPON(a)                                                   \
	(ACTOR_GET_GUN(a)->Gun != NULL ? ACTOR_GET_GUN(a) : ACTOR_GET_GRENADE(a))
struct vec2 ActorGetAverageWeaponMuzzleOffset(const TActor *a);
struct vec2 ActorGetMuzzleOffset(
	const TActor *a, const Weapon *w, const int barrel);
// Returns -1 if gun does not use ammo
int ActorWeaponGetAmmo(
	const TActor *a, const WeaponClass *wc, const int barrel);
// Returns -1 if no barrels can fire
int ActorGetCanFireBarrel(const TActor *a, const Weapon *w);
bool ActorTrySwitchWeapon(const TActor *a, const bool allGuns);
// Returns -1 if actor doesn't have gun
int ActorFindGun(const TActor *a, const WeaponClass *wc);
int ActorGetNumWeapons(const TActor *a);
int ActorGetNumGuns(const TActor *a);
int ActorGetNumGrenades(const TActor *a);
void ActorSwitchGun(const NActorSwitchGun sg);
bool ActorIsImmune(const TActor *actor, const special_damage_e damage);
bool ActorTakesDamage(const TActor *actor, const special_damage_e damage);
void ActorHit(const NThingDamage d);
bool ActorIsInvulnerable(
	const TActor *a, const int flags, const int playerUID, const GameMode mode,
	const special_damage_e special);

int ActorGetHealthPercent(const TActor *a);
bool ActorIsLowHealth(const TActor *a);
bool ActorIsGrimacing(const TActor *a);

bool ActorIsLocalPlayer(const int uid);
