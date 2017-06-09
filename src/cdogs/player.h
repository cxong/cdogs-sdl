/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (c) 2014-2016, Cong Xu
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

#define MAX_WEAPONS 3
// TODO: track accuracy
// this requires sending the bullet type with the ActorHit event,
// since we want to exclude melee and explosives from accuracy calcuation
typedef struct
{
	int ActorUID;	// -1 if dead
	bool IsLocal;	// whether this is a local-machine player
	// Whether this player is ready to start, for remote players
	bool Ready;
	Character Char;
	char name[20];
	int weaponCount;
	const GunDescription *weapons[MAX_WEAPONS];
	int Lives;

	NPlayerStats Stats;
	NPlayerStats Totals;

	// Used for end-of-game score tallying
	int survived;
	int hp;
	int missions;
	int lastMission;
	int allTime, today;

	input_device_e inputDevice;
	int deviceIndex;
	int UID;
} PlayerData;

extern CArray gPlayerDatas;	// of PlayerData
extern CArray gPlayerTemplates;	// of PlayerTemplate

#define MAX_LOCAL_PLAYERS 4


void PlayerDataInit(CArray *p);
void PlayerDataAddOrUpdate(const NPlayerData pd);
void PlayerRemove(const int uid);
NPlayerData PlayerDataDefault(const int idx);
NPlayerData PlayerDataMissionReset(const PlayerData *p);
void PlayerDataTerminate(CArray *p);

PlayerData *PlayerDataGetByUID(const int uid);

typedef enum
{
	PLAYER_ANY,
	PLAYER_ALIVE,
	PLAYER_ALIVE_OR_DYING
} PlayerAliveOptions;
int GetNumPlayers(
	const PlayerAliveOptions alive, const bool human, const bool local);
// Get number of players that should be shown on screen, for splitscreen modes
// Optionally gets the first matching player
int GetNumPlayersScreen(const PlayerData **p);
bool AreAllPlayersDeadAndNoLives(void);
const PlayerData *GetFirstPlayer(
	const bool alive, const bool human, const bool local);
bool IsPlayerAlive(const PlayerData *player);
bool IsPlayerHuman(const PlayerData *player);
bool IsPlayerHumanAndAlive(const PlayerData *player);
bool IsPlayerAliveOrDying(const PlayerData *player);
bool IsPlayerScreen(const PlayerData *p);
Vec2i PlayersGetMidpoint(void);
void PlayersGetBoundingRectangle(Vec2i *min, Vec2i *max);
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
