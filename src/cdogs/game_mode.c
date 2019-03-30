/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (c) 2013-2017, 2019 Cong Xu
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
#include "game_mode.h"

#include "campaigns.h"
#include "config.h"
#include "player.h"
#include "utils.h"


const char *GameModeStr(const GameMode g)
{
	switch (g)
	{
		T2S(GAME_MODE_NORMAL, "Campaign");
		T2S(GAME_MODE_DOGFIGHT, "Dogfight");
		T2S(GAME_MODE_DEATHMATCH, "Deathmatch");
		T2S(GAME_MODE_QUICK_PLAY, "Quick Play");
	default:
		return "";
	}
}

bool IsIntroNeeded(const GameMode mode)
{
	return mode == GAME_MODE_NORMAL;
}

bool IsGameOptionsNeeded(const GameMode mode)
{
	// Clients can't set game options
	if (gCampaign.IsClient) return false;

	switch (mode)
	{
	case GAME_MODE_NORMAL:
		return !gCampaign.OptionsSet;
	case GAME_MODE_DOGFIGHT:
		return !gCampaign.OptionsSet;
	case GAME_MODE_DEATHMATCH:
		return true;
	case GAME_MODE_QUICK_PLAY:
		return !gCampaign.OptionsSet;
	default:
		CASSERT(false, "unknown game mode");
		return false;
	}
}

bool IsScoreNeeded(const GameMode mode)
{
	return mode != GAME_MODE_DOGFIGHT && mode != GAME_MODE_DEATHMATCH;
}

bool HasObjectives(const GameMode mode)
{
	return mode == GAME_MODE_NORMAL || mode == GAME_MODE_QUICK_PLAY;
}

bool IsAutoMapEnabled(const GameMode mode)
{
	return mode != GAME_MODE_DOGFIGHT && mode != GAME_MODE_DEATHMATCH;
}

bool IsPasswordAllowed(const GameMode mode)
{
	return mode == GAME_MODE_NORMAL;
}

bool IsMissionBriefingNeeded(const GameMode mode)
{
	return HasObjectives(mode) && GetNumPlayers(PLAYER_ANY, false, true) > 0;
}

bool AreKeysAllowed(const GameMode mode)
{
	return mode == GAME_MODE_NORMAL || mode == GAME_MODE_QUICK_PLAY;
}

bool AreHealthPickupsAllowed(const GameMode mode)
{
	return mode == GAME_MODE_NORMAL || mode == GAME_MODE_QUICK_PLAY;
}

bool IsMultiplayer(const GameMode mode)
{
	return mode == GAME_MODE_DOGFIGHT || mode == GAME_MODE_DEATHMATCH;
}

bool IsPVP(const GameMode mode)
{
	return mode == GAME_MODE_DOGFIGHT || mode == GAME_MODE_DEATHMATCH;
}

bool HasExit(const GameMode mode)
{
	return mode != GAME_MODE_DOGFIGHT && mode != GAME_MODE_DEATHMATCH;
}

bool HasRounds(const GameMode mode)
{
	return mode == GAME_MODE_DOGFIGHT;
}

int ModeMaxRoundsWon(const GameMode mode)
{
	CASSERT(IsPVP(mode), "Game mode does not have any max rounds");
	switch (mode)
	{
	case GAME_MODE_DOGFIGHT:
		return ConfigGetInt(&gConfig, "Dogfight.FirstTo");
	case GAME_MODE_DEATHMATCH:
		return 1;
	default:
		CASSERT(false, "unknown game mode");
		return 0;
	}
}

int ModeLives(const GameMode mode)
{
	switch (mode)
	{
	case GAME_MODE_DOGFIGHT:
		return 1;
	case GAME_MODE_DEATHMATCH:
		return ConfigGetInt(&gConfig, "Deathmatch.Lives");
	default:
		return ConfigGetInt(&gConfig, "Game.Lives");
	}
}

int ModeMaxHealth(const GameMode mode)
{
	switch (mode)
	{
	case GAME_MODE_DOGFIGHT:
		return 500 * ConfigGetInt(&gConfig, "Dogfight.PlayerHP") / 100;
	default:
		return 200 * ConfigGetInt(&gConfig, "Game.PlayerHP") / 100;
	}
}

bool ModeHasNPCs(const GameMode mode)
{
	return mode != GAME_MODE_DOGFIGHT && mode != GAME_MODE_DEATHMATCH;
}
