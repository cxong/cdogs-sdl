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
#include "screens.h"

#include <time.h>

#include <cdogs/ai.h>
#include <cdogs/gamedata.h>
#include <cdogs/game_events.h>
#include <cdogs/handle_game_events.h>
#include <cdogs/hiscores.h>
#include <cdogs/music.h>
#include <cdogs/net_client.h>
#include <cdogs/net_server.h>

#include "autosave.h"
#include "briefing_screens.h"
#include "game.h"
#include "password.h"
#include "prep.h"


static void Campaign(GraphicsDevice *graphics, CampaignOptions *co);

void ScreenStart(void)
{
	// Reset player datas
	PlayerDataTerminate(&gPlayerDatas);
	PlayerDataInit(&gPlayerDatas);

	// Initialise game events; we need this for init as well as the game
	GameEventsInit(&gGameEvents);

	debug(D_NORMAL, ">> Entering campaign\n");
	if (IsIntroNeeded(gCampaign.Entry.Mode))
	{
		if (!ScreenCampaignIntro(&gCampaign.Setting))
		{
			gCampaign.IsLoaded = false;
			goto bail;
		}
	}

	debug(D_NORMAL, ">> Select number of players\n");
	if (!NumPlayersSelection(
		gCampaign.Entry.Mode, &gGraphicsDevice, &gEventHandlers))
	{
		gCampaign.IsLoaded = false;
		goto bail;
	}

	debug(D_NORMAL, ">> Entering selection\n");
	if (!PlayerSelection())
	{
		gCampaign.IsLoaded = false;
		goto bail;
	}

	debug(D_NORMAL, ">> Starting campaign\n");
	Campaign(&gGraphicsDevice, &gCampaign);
	CampaignUnload(&gCampaign);
	
bail:
	NetClientDisconnect(&gNetClient);
	GameEventsTerminate(&gGameEvents);
}


static void Campaign(GraphicsDevice *graphics, CampaignOptions *co)
{
	if (co->IsClient)
	{
		// If connecting to a server, we've already received the mission index
		// Do nothing
	}
	else if (IsPasswordAllowed(co->Entry.Mode))
	{
		MissionSave m;
		AutosaveLoadMission(&gAutosave, &m, co->Entry.Path);
		co->MissionIndex = EnterPassword(graphics, &m);
	}
	else
	{
		co->MissionIndex = 0;
	}

	bool run = false;
	bool gameOver = true;
	do
	{
		// Unready all the players
		for (int i = 0; i < (int)gPlayerDatas.size; i++)
		{
			PlayerData *p = CArrayGet(&gPlayerDatas, i);
			p->Ready = false;
		}

		CampaignAndMissionSetup(1, co, &gMission);

		if (IsGameOptionsNeeded(co->Entry.Mode))
		{
			debug(D_NORMAL, ">> Game options\n");
			if (!GameOptions(co->Entry.Mode))
			{
				run = false;
				goto bail;
			}
			co->OptionsSet = true;

			// If enabled, start net server
			if (!co->IsClient && ConfigGetBool(&gConfig, "StartServer"))
			{
				NetServerOpen(&gNetServer);
			}
		}

		// Mission briefing
		if (IsMissionBriefingNeeded(co->Entry.Mode))
		{
			if (!ScreenMissionBriefing(&gMission))
			{
				run = false;
				goto bail;
			}
		}

		// Equip guns
		if (!PlayerEquip())
		{
			run = false;
			goto bail;
		}

		if (co->IsClient)
		{
			if (!ScreenWaitForGameStart())
			{
				run = false;
				goto bail;
			}
		}

		run = RunGame(co, &gMission, &gMap);
		// Don't quit if all players died, that's normal for PVP modes
		if (IsPVP(co->Entry.Mode) &&
			GetNumPlayers(PLAYER_ALIVE_OR_DYING, false, false) == 0)
		{
			run = true;
		}

		const int survivingPlayers =
			GetNumPlayers(PLAYER_ALIVE, false, false);
		// In co-op (non-PVP) modes, at least one player must survive
		if (!IsPVP(co->Entry.Mode))
		{
			gameOver = survivingPlayers == 0 ||
				co->MissionIndex == (int)gCampaign.Setting.Missions.size - 1;
		}

		int maxScore = 0;
		for (int i = 0; i < (int)gPlayerDatas.size; i++)
		{
			PlayerData *p = CArrayGet(&gPlayerDatas, i);
			p->survived = IsPlayerAlive(p);
			if (IsPlayerAlive(p))
			{
				TActor *player = ActorGetByUID(p->ActorUID);
				p->hp = player->health;
				p->RoundsWon++;
				maxScore = MAX(maxScore, p->RoundsWon);
			}
		}
		if (IsPVP(co->Entry.Mode))
		{
			gameOver = maxScore == ModeMaxRoundsWon(co->Entry.Mode);
			CASSERT(maxScore <= ModeMaxRoundsWon(co->Entry.Mode),
				"score exceeds max rounds won");
		}

		MissionEnd();
		MusicPlayMenu(&gSoundDevice);

		if (run)
		{
			switch (co->Entry.Mode)
			{
			case GAME_MODE_DOGFIGHT:
				ScreenDogfightScores();
				break;
			case GAME_MODE_DEATHMATCH:
				ScreenDeathmatchFinalScores();
				break;
			default:
				ScreenMissionSummary(&gCampaign, &gMission);
				// Note: must use cached value because players get cleaned up
				// in CleanupMission()
				if (gameOver && survivingPlayers > 0)
				{
					ScreenVictory(&gCampaign);
				}
				break;
			}
		}

		// Check if any scores exceeded high scores, if we're not a PVP mode
		if (!IsPVP(co->Entry.Mode))
		{
			bool allTime = false;
			bool todays = false;
			for (int i = 0; i < (int)gPlayerDatas.size; i++)
			{
				PlayerData *p = CArrayGet(&gPlayerDatas, i);
				if (((run && !p->survived) || gameOver) && p->IsLocal)
				{
					EnterHighScore(p);
					allTime |= p->allTime >= 0;
					todays |= p->today >= 0;
				}

				if (!p->survived)
				{
					p->totalScore = 0;
					p->missions = 0;
				}
				else
				{
					p->missions++;
				}
				p->lastMission = co->MissionIndex;
			}
			if (allTime)
			{
				DisplayAllTimeHighScores(graphics);
			}
			if (todays)
			{
				DisplayTodaysHighScores(graphics);
			}
		}
		if (!HasRounds(co->Entry.Mode))
		{
			co->MissionIndex++;
		}

	bail:
		// Need to terminate the mission later as it is used in calculating scores
		MissionOptionsTerminate(&gMission);
	} while (run && !gameOver);

	// Final screen
	if (run)
	{
		switch (co->Entry.Mode)
		{
		case GAME_MODE_DOGFIGHT:
			ScreenDogfightFinalScores();
			break;
		default:
			// no end screen
			break;
		}
	}

	NetServerClose(&gNetServer);
}
