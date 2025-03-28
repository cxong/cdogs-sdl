/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.
	Copyright (c) 2014-2016, 2018-2020, 2022-2023, 2025 Cong Xu
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

#include "character.h"
#include "weapon_usage.h"

#define MAX_GUNS 3
#define MELEE_SLOT 2
#define GRENADE_SLOT 3
#define MAX_GRENADES 1
#define MAX_WEAPONS (MAX_GUNS + MAX_GRENADES)
// TODO: track accuracy
// this requires sending the bullet type with the ActorHit event,
// since we want to exclude melee and explosives from accuracy calcuation
typedef struct
{
	int ActorUID; // -1 if dead
	bool IsLocal; // whether this is a local-machine player
	// Whether this player is ready to start, for remote players
	bool Ready;
	Character Char;
	char name[20];
	const WeaponClass *guns[MAX_WEAPONS];
	CArray ammo; // of int
	int Lives;
	int HP;

	NPlayerStats Stats;
	NPlayerStats Totals;
	WeaponUsages WeaponUsages;

	// Used for end-of-game score tallying
	int hp;
	int survived;
	int missions;
	int lastMission;

	input_device_e inputDevice;
	int deviceIndex;
	int UID;
} PlayerData;

extern CArray gPlayerDatas; // of PlayerData

#define MAX_LOCAL_PLAYERS 4

void PlayerDataInit(CArray *p);
void PlayerDataAddOrUpdate(const NPlayerData pd);
void PlayerRemove(const int uid);
NPlayerData PlayerDataDefault(const int idx);
NPlayerData PlayerDataMissionReset(const PlayerData *p);
void PlayerDataTerminate(CArray *p);

PlayerData *PlayerDataGetByUID(const int uid);
int FindLocalPlayerIndex(const int uid);

typedef enum
{
	PLAYER_ANY,
	PLAYER_ALIVE,
	PLAYER_ALIVE_OR_DYING
} PlayerAliveOptions;
int GetNumPlayers(
	const PlayerAliveOptions alive, const bool human, const bool local);
bool AreAllPlayersDeadAndNoLives(void);
const PlayerData *GetFirstPlayer(
	const bool alive, const bool human, const bool local);
bool IsPlayerAlive(const PlayerData *player);
bool IsPlayerHuman(const PlayerData *player);
bool IsPlayerHumanAndAlive(const PlayerData *player);
bool IsPlayerAliveOrDying(const PlayerData *player);
bool IsPlayerScreen(const PlayerData *p);
struct vec2 PlayersGetMidpoint(void);
void PlayersGetBoundingRectangle(struct vec2 *min, struct vec2 *max);
int PlayersNumUseAmmo(const int ammoId);
bool PlayerIsLocal(const int uid);

void PlayerScore(PlayerData *p, const int points);
// Return false if player already assigned this input device
bool PlayerTrySetInputDevice(
	PlayerData *p, const input_device_e d, const int idx);
// Return false if player already assigned a device,
// or this device/index is already used by another player
bool PlayerTrySetUnusedInputDevice(
	PlayerData *p, const input_device_e d, const int idx);

int PlayerGetNumWeapons(const PlayerData *p);
bool PlayerHasGrenadeButton(const PlayerData *p);
bool PlayerHasWeapon(const PlayerData *p, const WeaponClass *wc);
bool PlayerHasWeaponUpgrade(const PlayerData *p, const WeaponClass *wc);
void PlayerAddWeapon(PlayerData *p, const WeaponClass *wc);
void PlayerAddWeaponToSlot(
	PlayerData *p, const WeaponClass *wc, const int slot);
void PlayerRemoveWeapon(PlayerData *p, const int slot);
void PlayerAddMinimalWeapons(PlayerData *p);
bool PlayerUsesAmmo(const PlayerData *p, const int ammoId);
bool PlayerUsesAnyAmmo(const PlayerData *p);
int PlayerGetAmmoAmount(const PlayerData *p, const int ammoId);
void PlayerAddAmmo(
	PlayerData *p, const int ammoId, const int amount, const bool isFree);
void PlayerSetHP(PlayerData *p, const int hp);
void PlayerSetLives(PlayerData *p, const int lives);

int CharacterGetStartingHealth(const Character *c, const PlayerData *p);
