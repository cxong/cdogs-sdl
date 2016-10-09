/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.

    Copyright (c) 2013-2016, Cong Xu
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
#include "mission.h"
#include "player.h"


// Numeric popup for health and score
// Displays as a small pop-up coloured text overlay
typedef struct
{
	union
	{
		int PlayerUID;
		int ObjectiveIndex;
	} u;
	int Amount;
	// Number of milliseconds that this update will last
	int TimerMax;
	int Timer;
} HUDNumPopup;

typedef struct
{
	HUDNumPopup score[MAX_LOCAL_PLAYERS];
	HUDNumPopup health[MAX_LOCAL_PLAYERS];
	HUDNumPopup ammo[MAX_LOCAL_PLAYERS];
	CArray objective; // of HUDNumPopup, one per objective
} HUDNumPopups;

void HUDNumPopupsInit(
	HUDNumPopups *popups, const struct MissionOptions *mission);
void HUDNumPopupsTerminate(HUDNumPopups *popups);

typedef enum
{
	NUMBER_POPUP_SCORE,
	NUMBER_POPUP_HEALTH,
	NUMBER_POPUP_AMMO,
	NUMBER_POPUP_OBJECTIVE
} HUDNumPopupType;
// idx is either player UID or objective index
void HUDNumPopupsAdd(
	HUDNumPopups *popups, const HUDNumPopupType type,
	const int idxOrUID, const int amount);

void HUDPopupsUpdate(HUDNumPopups *popups, const int ms);

void HUDNumPopupsDrawPlayer(
	const HUDNumPopups *popups, const int idx, const int drawFlags);
void HUDNumPopupsDrawObjective(
	const HUDNumPopups *popups, const int idx, const Vec2i pos);
