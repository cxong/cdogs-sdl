/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (c) 2013-2015, Cong Xu
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

#include <stdbool.h>

typedef enum
{
	GAME_MODE_NORMAL,
	GAME_MODE_DOGFIGHT,
	GAME_MODE_DEATHMATCH,
	GAME_MODE_QUICK_PLAY
} GameMode;

bool IsIntroNeeded(const GameMode mode);
bool IsGameOptionsNeeded(const GameMode mode);
bool IsScoreNeeded(const GameMode mode);
bool HasObjectives(const GameMode mode);
bool IsAutoMapEnabled(const GameMode mode);
bool IsPasswordAllowed(const GameMode mode);
bool IsMissionBriefingNeeded(const GameMode mode);
bool AreKeysAllowed(const GameMode mode);
bool AreHealthPickupsAllowed(const GameMode mode);
bool IsMultiplayer(const GameMode mode);
bool IsPVP(const GameMode mode);
bool HasExit(const GameMode mode);
bool HasRounds(const GameMode mode);
int ModeMaxRoundsWon(const GameMode mode);
int ModeLives(const GameMode mode);
int ModeMaxHealth(const GameMode mode);
bool ModeHasNPCs(const GameMode mode);
