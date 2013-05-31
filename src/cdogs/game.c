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
#include "game.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <SDL.h>
#include "SDL_mutex.h"

#include "draw.h"
#include "music.h"
#include "objs.h"
#include "actors.h"
#include "pics.h"
#include "text.h"
#include "gamedata.h"
#include "ai.h"
#include "input.h"
#include "automap.h"
#include "mission.h"
#include "sounds.h"
#include "triggers.h"
#include "keyboard.h"
#include "blit.h"
#include "utils.h"

#include "drawtools.h" /* for Draw_Box and Draw_Point */

#define fLOS 1
#define FPS_FRAMELIMIT       70
#define CLOCK_LIMIT       2100

#define FPS_TARGET	30

#define SWITCH_TURNLIMIT     10

#define PICKUP_LIMIT         350

#define SPLIT_X  300
#define SPLIT_Y  180

static Uint32 ticks_now;
static Uint32 ticks_then;

static volatile int gameTicks = 0;
static int frames = 0;
static int fps = 0;
static char message[256];
static int messageTicks = 0;

static int timeHours, timeMinutes;

static int screenShaking = 0;
int shakeMultiplier = 1;

static int gameIsPaused = NO;
static int escExits = NO;
long oldtime;

// This is referenced from CDOGS.C to determine time bonus
int missionTime;

#define MICROSECS_PER_SEC 1000000
#define MILLISECS_PER_SEC 1000

#define TICKS_PER_SEC MILLISECS_PER_SEC

int PlayerSpecialCommands(TActor * actor, int cmd, struct PlayerData *data)
{
	if (!actor)
	{
		return NO;
	}

	if (((cmd | actor->lastCmd) & CMD_BUTTON2) == 0)
		actor->flags &= ~FLAGS_SPECIAL_USED;

	if ((cmd & CMD_BUTTON2) != 0 &&
	    (cmd & (CMD_LEFT | CMD_RIGHT | CMD_UP | CMD_DOWN)) != 0 &&
	    actor->dx == 0 && actor->dy == 0) {
		SlideActor(actor, cmd);
		actor->flags |= FLAGS_SPECIAL_USED;
	}
	else if (
		(actor->lastCmd & CMD_BUTTON2) != 0 &&
		(cmd & CMD_BUTTON2) == 0 &&
		(actor->flags & FLAGS_SPECIAL_USED) == 0 &&
		(cmd & (CMD_LEFT | CMD_RIGHT | CMD_UP | CMD_DOWN)) == 0 &&
		data->weaponCount > 1)
	{
		int i;
		for (i = 0; i < data->weaponCount; i++)
		{
			if (actor->weapon.gun == (gun_e)data->weapons[i])
			{
				break;
			}
		}
		i++;
		if (i >= data->weaponCount)
		{
			i = 0;
		}
		actor->weapon.gun = data->weapons[i];
		SoundPlayAt(SND_SWITCH, actor->tileItem.x, actor->tileItem.y);
	}
	else
	{
		return NO;
	}

	actor->lastCmd = cmd;
	return YES;
}

static void Ticks_Update(void)
{
	static int init = 0;

	if (init == 0) {
		ticks_then = SDL_GetTicks();
		ticks_now = SDL_GetTicks();
		init = 1;
	} else {
		ticks_then = ticks_now;
		ticks_now = SDL_GetTicks();
	}

	return;
}

static int Ticks_TimeElapsed(Uint32 msec)
{
	static Uint32 old_ticks = 0;

	if (old_ticks == 0) {
		old_ticks = ticks_now;
		return 0;
	} else {
		if (ticks_now - old_ticks > msec) {
			old_ticks = ticks_now;
			return 1;
		} else {
			return 0;
		}
	}
}

static void Ticks_FrameBegin(void)
{
	Ticks_Update();
}

static void Ticks_FrameEnd(void)
{
	Uint32 now = SDL_GetTicks();
	Uint32 ticksSpent = now - ticks_now;
	Uint32 ticksIdeal = 33;
	if (ticksSpent < ticksIdeal)
	{
		Uint32 ticksToDelay = ticksIdeal - ticksSpent;
		SDL_Delay(ticksToDelay);
		debug(D_VERBOSE, "Delaying %u ticks_now %u now %u\n", ticksToDelay, ticks_now, now);
	}
}

static int Ticks_Synchronize(void)
{
	int ticks = 1;
	TActor *actor;

	/*
	while (gameTicks <= 1) {
		//debug("delay... gameTicks=%d\n", gameTicks);
		SDL_Delay(30);
	}
	*/

/*	while (gameTicks > GAMETICKS_PER_FRAME) {
		debug("update... gameTicks=%d\n", gameTicks); */


		ticks++;
		if (!gameIsPaused) {
			UpdateMobileObjects();
			actor = ActorList();
			while (actor) {
				CommandActor(actor, actor->lastCmd);
				actor = actor->next;
			}
		}

/*	} 	*/


	return ticks;
}


void DoBuffer(struct Buffer *b, int x, int y, int dx, int w, int xn,
	      int yn)
{
	SetBuffer(x + xn, y + yn, b, w);
#ifdef fLOS
	LineOfSight(x, y, b, IS_SHADOW);
	FixBuffer(b, IS_SHADOW);
#endif
	DrawBuffer(b, dx);
}

void ShakeScreen(int amount)
{
	screenShaking = (screenShaking + amount) * shakeMultiplier;

	/* So we don't shake too much :) */
	if (screenShaking > 100)
		screenShaking = 100;
}

void BlackLine(void)
{
	int i;
	unsigned char *p = GetDstScreen();

	p += (SCREEN_WIDTH / 2) - 1;
	for (i = 0; i < SCREEN_HEIGHT; i++) {
		*p++ = 1;
		*p = 1;
		p += SCREEN_WIDTH - 1;
	}
}

void DrawScreen(struct Buffer *b, TActor * player1, TActor * player2)
{
	static int x = 0;
	static int y = 0;
	int xNoise, yNoise;

	if (screenShaking) {
		xNoise = rand() & 7;
		yNoise = rand() & 7;
	} else
		xNoise = yNoise = 0;

	if (player1 && player2) {
		if (!gOptions.splitScreenAlways &&
			abs(player1->tileItem.x - player2->tileItem.x) < SPLIT_X &&
			abs(player1->tileItem.y - player2->tileItem.y) < SPLIT_Y)
		{
			CDogsSetClip(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);
			// One screen
			x = (player1->tileItem.x +
			     player2->tileItem.x) / 2;
			y = (player1->tileItem.y +
			     player2->tileItem.y) / 2;

			SetBuffer(x + xNoise, y + yNoise, b, X_TILES);
		#ifdef fLOS
			LineOfSight(
				player1->tileItem.x,
				player1->tileItem.y, b,
				IS_SHADOW);
			LineOfSight(
				player2->tileItem.x,
				player2->tileItem.y, b,
				IS_SHADOW2);
			FixBuffer(b, IS_SHADOW | IS_SHADOW2);
		#endif
			DrawBuffer(b, 0);
		} else {
			CDogsSetClip(0, 0, (SCREEN_WIDTH / 2) - 1, SCREEN_HEIGHT - 1);
			DoBuffer(b, player1->tileItem.x, player1->tileItem.y, 0, X_TILES_HALF, xNoise, yNoise);
			SoundSetLeftEar(player1->tileItem.x, player1->tileItem.y);
			CDogsSetClip((SCREEN_WIDTH / 2) + 1, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);
			DoBuffer(b, player2->tileItem.x, player2->tileItem.y, (SCREEN_WIDTH / 2) + 1, X_TILES_HALF, xNoise, yNoise);
			SoundSetRightEar(player2->tileItem.x, player2->tileItem.y);
			x = player1->tileItem.x;
			y = player1->tileItem.y;
			BlackLine();
		}
	} else if (player1) {
		CDogsSetClip(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);
		DoBuffer(b, player1->tileItem.x, player1->tileItem.y, 0,
			 X_TILES, xNoise, yNoise);
		SoundSetEars(player1->tileItem.x, player1->tileItem.y);
		x = player1->tileItem.x;
		y = player1->tileItem.y;
	} else if (player2) {
		CDogsSetClip(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);
		DoBuffer(b, player2->tileItem.x, player2->tileItem.y, 0,
			 X_TILES, xNoise, yNoise);
		SoundSetEars(player2->tileItem.x, player2->tileItem.y);
		x = player2->tileItem.x;
		y = player2->tileItem.y;
	} else
		DoBuffer(b, x, y, 0, X_TILES, xNoise, yNoise);
	CDogsSetClip(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);
}

#define PLACE_LEFT	0
#define PLACE_RIGHT	1

void PlayerStatus(int placement, struct PlayerData *data, TActor * p)
{
	char s[50];

	int flags = TEXT_TOP;

	if (placement == PLACE_LEFT)	flags |= TEXT_LEFT;
	if (placement == PLACE_RIGHT)	flags |= TEXT_RIGHT;

	CDogsTextStringSpecial(data->name, flags, 5, 5);
	if (IsScoreNeeded(gCampaign.mode))
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
	} else
		CDogsTextStringSpecial(s, flags, 5, 5 + 1 * CDogsTextHeight());
}

static void DrawKeycard(int x, int y, const TOffsetPic * pic)
{
	DrawTPic(x + pic->dx, y + pic->dy, gPics[pic->picIndex],
		 gCompiledPics[pic->picIndex]);
}

static void DrawExitArea(void)
{
	int left, right, top, bottom;
	int x, y;

	left = gMission.exitLeft / TILE_WIDTH;
	right = gMission.exitRight / TILE_WIDTH;
	top = gMission.exitTop / TILE_HEIGHT;
	bottom = gMission.exitBottom / TILE_HEIGHT;

	for (x = left; x <= right; x++)
		ChangeFloor(x, top, gMission.exitPic, gMission.exitShadow);
	for (x = left; x <= right; x++)
		ChangeFloor(x, bottom, gMission.exitPic,
			    gMission.exitShadow);
	for (y = top + 1; y < bottom; y++)
		ChangeFloor(left, y, gMission.exitPic,
			    gMission.exitShadow);
	for (y = top + 1; y < bottom; y++)
		ChangeFloor(right, y, gMission.exitPic,
			    gMission.exitShadow);
}

static void MissionUpdateObjectives(void)
{
	unsigned char color;
	int i, left;
	char s[4];
	int allDone = 1;
	static int completed = 0;
	int x, y;

	x = 5;
	y = SCREEN_HEIGHT - 5 - CDogsTextHeight();
	for (i = 0; i < gMission.missionData->objectiveCount; i++) {
		if (gMission.missionData->objectives[i].type ==
		    OBJECTIVE_INVESTIGATE)
			gMission.objectives[i].done = ExploredPercentage();

		if (gMission.missionData->objectives[i].required > 0) {
			// Objective color dot
			color = gMission.objectives[i].color;

			y += 3;
			Draw_Rect(x, y, 2, 2, color);
			y -= 3;

			left = gMission.objectives[i].required - gMission.objectives[i].done;

			if (left > 0) {
				if ((gMission.missionData->objectives[i].flags & OBJECTIVE_UNKNOWNCOUNT) == 0) {
					sprintf(s, "%d", left);
				} else {
					strcpy(s, "?");
				}
				CDogsTextStringAt(x + 5, y, s);
				allDone = 0;
			} else {
				CDogsTextStringAt(x + 5, y, "Done");
			}
			x += 30;
		}
	}

	if (allDone && !completed)
	{
		completed = 1;
		DrawExitArea();
	}
	else if (!allDone)
	{
		completed = 0;
	}
}

void StatusDisplay(void)
{
	char s[50];
	static time_t ot = -1;
	static time_t t = 0;
	static time_t td = 0;

	PlayerStatus(PLACE_LEFT, &gPlayer1Data, gPlayer1);
	if (gOptions.twoPlayers)
		PlayerStatus(PLACE_RIGHT, &gPlayer2Data, gPlayer2);

	if (!gPlayer1 && !gPlayer2)
	{
		if (gCampaign.mode != CAMPAIGN_MODE_DOGFIGHT)
		{
			CDogsTextStringAtCenter("Game Over!");
		}
		else
		{
			CDogsTextStringAtCenter("Double Kill!");
		}
	}
	else if (IsMissionComplete(&gMission))
	{
		sprintf(s, "Pickup in %d seconds\n",
			(gMission.pickupTime + 69) / 70);
		CDogsTextStringAtCenter(s);
	}

	if (gameIsPaused) {
		if (escExits)
			CDogsTextStringAtCenter("Press Esc again to quit");
		else
			CDogsTextStringAtCenter("Paused");
	}

	if (messageTicks > 0)
		CDogsTextStringSpecial(message, TEXT_XCENTER | TEXT_TOP, 0, 20);

	if (gOptions.displayFPS) {
		sprintf(s, "FPS: %d", fps);
		CDogsTextStringSpecial(s, TEXT_RIGHT | TEXT_BOTTOM, 10, 10);
	}
	if (gOptions.displayTime) {
		sprintf(s, "%02d:%02d", timeHours, timeMinutes);
		CDogsTextStringSpecial(s, TEXT_LEFT | TEXT_BOTTOM, 10, 10);
	}

#define KEY_WIDTH(n) (PicWidth((const void *)&cGeneralPics[gMission.keyPics[n]]))

	if (gMission.flags & FLAGS_KEYCARD_YELLOW)
		DrawKeycard(CenterX(KEY_WIDTH(0)) - 30, 20, &cGeneralPics[gMission.keyPics[0]]);
	if (gMission.flags & FLAGS_KEYCARD_GREEN)
		DrawKeycard(CenterX(KEY_WIDTH(1)) - 10, 20, &cGeneralPics[gMission.keyPics[1]]);
	if (gMission.flags & FLAGS_KEYCARD_BLUE)
		DrawKeycard(CenterX(KEY_WIDTH(2)) + 10, 20, &cGeneralPics[gMission.keyPics[2]]);
	if (gMission.flags & FLAGS_KEYCARD_RED)
		DrawKeycard(CenterX(KEY_WIDTH(3)) + 30, 20, &cGeneralPics[gMission.keyPics[3]]);

	if (ot == -1 || missionTime == 0) /* set the original time properly */
		ot = time(NULL);

	t = time(NULL);

	if (!gameIsPaused) {
		td = t - ot;
	}

	sprintf(s, "%d:%02d", (int)(td / 60), (int)(td % 60));
	CDogsTextStringSpecial(s, TEXT_TOP | TEXT_XCENTER, 0, 5);

	if (HasObjectives(gCampaign.mode))
	{
		MissionUpdateObjectives();
	}
}

void DisplayMessage(const char *s)
{
	strcpy(message, s);
	messageTicks = 140;
}

int HandleKey(int *done, int cmd)
{
	while ((KeyIsDown(&gKeyboard, gOptions.mapKey) || (cmd & CMD_BUTTON3) != 0) &&
		IsAutoMapEnabled(gCampaign.mode))
	{
		DisplayAutoMap(0);
		while (KeyIsDown(&gKeyboard, gOptions.mapKey) ||
			(cmd & CMD_BUTTON3) != 0)
		{
			SDL_Delay(10);
			KeyPoll(&gKeyboard);
		}
	}

	if (((cmd & CMD_BUTTON4) != 0) && !gOptions.twoPlayers) {
		gameIsPaused = YES;
		escExits = NO;
	} else if (gameIsPaused && AnyButton(cmd))
		gameIsPaused = NO;

	if (!gPlayer1 && !gPlayer2)
	{
		*done = 1;
		return 0;
	}
	else if (KeyIsPressed(&gKeyboard, keyEsc))
	{
		if (gameIsPaused && escExits)
		{
			*done = 1;
			return 1;
		}
		else if (!gameIsPaused)
		{
			gameIsPaused = YES;
			escExits = YES;
		}
	}
	else if (KeyGetPressed(&gKeyboard))
	{
		gameIsPaused = 0;
	}

	return 0;
}

void GetPlayerInput(int *cmd1, int *cmd2)
{
	GetPlayerCmd(gPlayer1 ? cmd1 : NULL, gPlayer2 ? cmd2 : NULL, 0);
}

int gameloop(void)
{
	struct Buffer *buffer = NewBuffer();
	int ticks;
	int is_esc_pressed = 0;
	int done = NO;
	time_t t;
	struct tm *tp;

	CDogsSetClip(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);

	if (MusicGetStatus() != MUSIC_OK)
	{
		DisplayMessage(MusicGetErrorMessage());
	}

	gameIsPaused = NO;

	missionTime = 0;
	KeyInit(&gKeyboard);
	while (!done)
	{
		int cmd1 = 0, cmd2 = 0;

		frames++;

		Ticks_FrameBegin();

		ticks = Ticks_Synchronize();

		if (gOptions.displaySlices)
			SetColorZero(32, 0, 0);

		DrawScreen(buffer, gPlayer1, gPlayer2);

		if (gOptions.displaySlices)
			SetColorZero(0, 0, 0);

		if (screenShaking) {
			screenShaking -= ticks;
			if (screenShaking < 0)
				screenShaking = 0;
		}

		debug(D_VERBOSE, "frames... %d\n", frames);

		if (Ticks_TimeElapsed(TICKS_PER_SEC)) {
			fps = frames;
			debug(D_NORMAL, "fps = %d\n", fps);
			frames = 0;

			t = time(NULL);
			tp = localtime(&t);
			timeHours = tp->tm_hour;
			timeMinutes = tp->tm_min;
		}

		if (messageTicks > 0)
			messageTicks -= ticks;

		StatusDisplay();

		if (!gameIsPaused) {
			missionTime += ticks;
			if ((gPlayer1 || gPlayer2) && IsMissionComplete(&gMission))
			{
				if (gMission.pickupTime == PICKUP_LIMIT)
				{
					SoundPlay(&gSoundDevice, SND_DONE);
				}
				gMission.pickupTime -= ticks;
				if (gMission.pickupTime <= 0)
					done = YES;
			} else
				gMission.pickupTime = PICKUP_LIMIT;
		}

		if (gOptions.displaySlices)
			SetColorZero(0, 0, 32);

		if (gOptions.displaySlices)
			SetColorZero(0, 0, 0);

		CopyToScreen();

		KeyPoll(&gKeyboard);

		if (!gameIsPaused) {
			if (!gOptions.slowmotion || (frames & 1) == 0) {
				UpdateAllActors(ticks);
				UpdateMobileObjects();

				GetPlayerInput(&cmd1, &cmd2);

				if (gPlayer1 && !PlayerSpecialCommands(
							gPlayer1, cmd1, &gPlayer1Data)) {
					CommandActor(gPlayer1, cmd1);
				}
				if (gPlayer2 && !PlayerSpecialCommands(
							gPlayer2, cmd2, &gPlayer2Data)) {
					CommandActor(gPlayer2, cmd2);
				}

				if (gOptions.badGuys)
					CommandBadGuys();

				UpdateWatches();
			}
		} else {
			GetPlayerInput(&cmd1, &cmd2);
		}

		is_esc_pressed = HandleKey(&done, cmd1 | cmd2);

		Ticks_FrameEnd();
	}
	CFREE(buffer);

	return !is_esc_pressed;
}
