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

    Copyright (c) 2013, Cong Xu
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
#include "hud.h"

#include <time.h>

#include "actors.h"
#include "game.h"
#include "mission.h"
#include "text.h"

void FPSCounterInit(FPSCounter *counter)
{
	counter->elapsed = 0;
	counter->framesDrawn = 0;
	counter->fps = 0;
}
void FPSCounterUpdate(FPSCounter *counter, int ms)
{
	counter->elapsed += ms;
	if (counter->elapsed > 1000)
	{
		counter->fps = counter->framesDrawn;
		counter->framesDrawn = 0;
		counter->elapsed -= 1000;
	}
}
void FPSCounterDraw(FPSCounter *counter)
{
	char s[50];
	counter->framesDrawn++;
	sprintf(s, "FPS: %d", counter->fps);
	CDogsTextStringSpecial(s, TEXT_RIGHT | TEXT_BOTTOM, 10, 10);
}

void WallClockSetTime(WallClock *clock)
{
	time_t t = time(NULL);
	struct tm *tp = localtime(&t);
	clock->hours = tp->tm_hour;
	clock->minutes = tp->tm_min;
}
void WallClockInit(WallClock *clock)
{
	clock->elapsed = 0;
	WallClockSetTime(clock);
}
void WallClockUpdate(WallClock *clock, int ms)
{
	clock->elapsed += ms;
	if (clock->elapsed > 60*1000)	// update every minute
	{
		WallClockSetTime(clock);
		clock->elapsed -= 60*1000;
	}
}
void WallClockDraw(WallClock *clock)
{
	char s[50];
	sprintf(s, "%02d:%02d", clock->hours, clock->minutes);
	CDogsTextStringSpecial(s, TEXT_LEFT | TEXT_BOTTOM, 10, 10);
}

void HUDInit(HUD *hud, InterfaceConfig *config, struct MissionOptions *mission)
{
	hud->mission = mission;
	strcpy(hud->message, "");
	hud->messageTicks = 0;
	hud->config = *config;
	FPSCounterInit(&hud->fpsCounter);
	WallClockInit(&hud->clock);
}

void HUDDisplayMessage(HUD *hud, const char *msg)
{
	strcpy(hud->message, msg);
	hud->messageTicks = 140;
}

void HUDUpdate(HUD *hud, int ms)
{
	hud->messageTicks -= ms;
	if (hud->messageTicks < 0)
	{
		hud->messageTicks = 0;
	}
	FPSCounterUpdate(&hud->fpsCounter, ms);
	WallClockUpdate(&hud->clock, ms);
}


#define HUD_PLACE_LEFT	0
#define HUD_PLACE_RIGHT	1
// Draw player's score, health etc.
void DrawPlayerStatus(struct PlayerData *data, TActor *p, int placement)
{
	char s[50];

	int flags = TEXT_TOP;

	if (placement == HUD_PLACE_LEFT)	flags |= TEXT_LEFT;
	if (placement == HUD_PLACE_RIGHT)	flags |= TEXT_RIGHT;

	CDogsTextStringSpecial(data->name, flags, 5, 5);
	if (IsScoreNeeded(gCampaign.Entry.mode))
	{
		sprintf(s, "Score: %d", data->score);
	}
	else
	{
		s[0] = 0;
	}
	if (p)
	{
		CDogsTextStringSpecial(s, flags, 5, 5 + 2 + 2 * CDogsTextHeight());
		CDogsTextStringSpecial(
			GunGetName(p->weapon.gun), flags, 5, 5 + 1 + CDogsTextHeight());
		sprintf(s, "%d hp", p->health);
		CDogsTextStringSpecial(s, flags, 5, 5 + 3 + 3 * CDogsTextHeight());
	}
	else
	{
		CDogsTextStringSpecial(s, flags, 5, 5 + 1 * CDogsTextHeight());
	}
}

static void DrawKeycard(int x, int y, const TOffsetPic * pic)
{
	DrawTPic(x + pic->dx, y + pic->dy, gPics[pic->picIndex], gCompiledPics[pic->picIndex]);
}

void DrawKeycards(HUD *hud)
{
	int keyFlags[] =
	{
		FLAGS_KEYCARD_YELLOW,
		FLAGS_KEYCARD_GREEN,
		FLAGS_KEYCARD_BLUE,
		FLAGS_KEYCARD_RED
	};
	int i;
	int xOffset = -30;
	int xOffsetIncr = 20;
	int yOffset = 20;
	for (i = 0; i < 4; i++)
	{
		if (hud->mission->flags & keyFlags[i])
		{
			DrawKeycard(
				CenterX(cGeneralPics[hud->mission->keyPics[i]].dx) - xOffset,
				yOffset,
				&cGeneralPics[hud->mission->keyPics[i]]);
		}
		xOffset += xOffsetIncr;
	}
}

void HUDDraw(HUD *hud, int isPaused, int isEscExit)
{
	char s[50];
	static time_t ot = -1;
	static time_t t = 0;
	static time_t td = 0;

	DrawPlayerStatus(&gPlayer1Data, gPlayer1, HUD_PLACE_LEFT);
	if (gOptions.twoPlayers)
	{
		DrawPlayerStatus(&gPlayer2Data, gPlayer2, HUD_PLACE_RIGHT);
	}

	if (!gPlayer1 && !gPlayer2)
	{
		if (gCampaign.Entry.mode != CAMPAIGN_MODE_DOGFIGHT)
		{
			CDogsTextStringAtCenter("Game Over!");
		}
		else
		{
			CDogsTextStringAtCenter("Double Kill!");
		}
	}
	else if (IsMissionComplete(hud->mission))
	{
		sprintf(s, "Pickup in %d seconds\n",
			(gMission.pickupTime + 69) / 70);
		CDogsTextStringAtCenter(s);
	}

	if (isPaused)
	{
		if (isEscExit)
		{
			CDogsTextStringAtCenter("Press Esc again to quit");
		}
		else
		{
			CDogsTextStringAtCenter("Paused");
		}
	}

	if (hud->messageTicks > 0)
	{
		CDogsTextStringSpecial(hud->message, TEXT_XCENTER | TEXT_TOP, 0, 20);
	}

	if (hud->config.ShowFPS)
	{
		FPSCounterDraw(&hud->fpsCounter);
	}
	if (hud->config.ShowTime)
	{
		WallClockDraw(&hud->clock);
	}

	DrawKeycards(hud);

	if (ot == -1 || missionTime == 0) /* set the original time properly */
		ot = time(NULL);

	t = time(NULL);

	if (!isPaused)
	{
		td = t - ot;
	}

	sprintf(s, "%d:%02d", (int)(td / 60), (int)(td % 60));
	CDogsTextStringSpecial(s, TEXT_TOP | TEXT_XCENTER, 0, 5);
}
