/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (C) 1995 Ronny Wester
    Copyright (C) 2003 Jeremy Chin 
    Copyright (C) 2003 Lucas Martin-King 

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

-------------------------------------------------------------------------------

 game.c - game loop and related functions

*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <SDL.h>

#include "draw.h"
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
#include "game.h"
#include "keyboard.h"
#include "blit.h"


#define fLOS 1
#define FPS_FRAMELIMIT       70
#define CLOCK_LIMIT       2100

#define SWITCH_TURNLIMIT     10

#define PICKUP_LIMIT         350


static volatile int gameTicks = 0;
static volatile int fpsGameTicks = 0;
static int frames = 0;
static int fps = 0;
static char message[256];
static int messageTicks = 0;

static int timeHours, timeMinutes;

static int screenShaking = 0;

static int gameIsPaused = NO;
static int escExits = NO;
long oldtime;


// This is referenced from CDOGS.C to determine time bonus
int missionTime;


int PlayerSpecialCommands(TActor * actor, int cmd, struct PlayerData *data)
{
	int i;

	if (!actor)
		return NO;

	if (((cmd | actor->lastCmd) & CMD_BUTTON2) == 0)
		actor->flags &= ~FLAGS_SPECIAL_USED;

	if ((cmd & CMD_BUTTON2) != 0 &&
	    (cmd & (CMD_LEFT | CMD_RIGHT | CMD_UP | CMD_DOWN)) != 0 &&
	    actor->dx == 0 && actor->dy == 0) {
		SlideActor(actor, cmd);
		actor->flags |= FLAGS_SPECIAL_USED;
	} else if ((actor->lastCmd & CMD_BUTTON2) != 0 &&
		   (cmd & CMD_BUTTON2) == 0 &&
		   (actor->flags & FLAGS_SPECIAL_USED) == 0 &&
		   (cmd & (CMD_LEFT | CMD_RIGHT | CMD_UP | CMD_DOWN)) == 0
		   &&
//     actor->dx == 0 && actor->dy == 0 &&
		   data->weaponCount > 1) {
		for (i = 0; i < data->weaponCount; i++)
			if (actor->gun == data->weapons[i])
				break;
		i++;
		if (i >= data->weaponCount)
			i = 0;
		actor->gun = data->weapons[i];
		PlaySoundAt(actor->tileItem.x, actor->tileItem.y,
			    SND_SWITCH);
	} else
		return NO;

	actor->lastCmd = cmd;
	return YES;
}


// Timer interrupt routine to keep track of time
//typedef void (*intHandler)(void);
//TODO: replace with a SDL_Timer callback
Uint32 synchronizer(Uint32 interval, void *param)
{
	gameTicks++;
	fpsGameTicks++;
	return interval;
}

int Synchronize(void)
{
	int ticks = 1;
	TActor *actor;

	while (gameTicks > GAMETICKS_PER_FRAME) {
		ticks++;
		if (!gameIsPaused) {
			UpdateMobileObjects();
			actor = ActorList();
			while (actor) {
				CommandActor(actor, actor->lastCmd);
				actor = actor->next;
			}
		}
		gameTicks -= GAMETICKS_PER_FRAME;
	}
	return ticks;
}


void DoBuffer(struct Buffer *b, int x, int y, int dx, int w, int xn,
	      int yn)
{
	SetBuffer(x + xn, y + yn, b, w);
	if (fLOS) {
		LineOfSight(x, y, b, IS_SHADOW);
		FixBuffer(b, IS_SHADOW);
	}
	DrawBuffer(b, dx);
}

void ShakeScreen(int amount)
{
	screenShaking += amount;

	/* So we don't shake too much :) */
	if (screenShaking > 100)
		screenShaking = 100;
}

void BlackLine(void)
{
	int i;
	unsigned char *p = GetDstScreen();

	p += 159;
	for (i = 0; i < 200; i++) {
		*p++ = 1;
		*p = 1;
		p += 319;
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
		if (abs(player1->tileItem.x - player2->tileItem.x) <
		    gOptions.xSplit
		    && abs(player1->tileItem.y - player2->tileItem.y) <
		    gOptions.ySplit) {
			SetClip(0, 0, 319, 199);
			// One screen
			x = (player1->tileItem.x +
			     player2->tileItem.x) / 2;
			y = (player1->tileItem.y +
			     player2->tileItem.y) / 2;

			SetBuffer(x + xNoise, y + yNoise, b, X_TILES);
			if (fLOS) {
				LineOfSight(player1->tileItem.x,
					    player1->tileItem.y, b,
					    IS_SHADOW);
				LineOfSight(player2->tileItem.x,
					    player2->tileItem.y, b,
					    IS_SHADOW2);
				FixBuffer(b, IS_SHADOW | IS_SHADOW2);
			}
			DrawBuffer(b, 0);
		} else {
			SetClip(0, 0, 158, 199);
			DoBuffer(b, player1->tileItem.x,
				 player1->tileItem.y, 0, X_TILES_HALF,
				 xNoise, yNoise);
			SetLeftEar(player1->tileItem.x,
				   player1->tileItem.y);
			SetClip(161, 0, 319, 199);
			DoBuffer(b, player2->tileItem.x,
				 player2->tileItem.y, 160, X_TILES_HALF,
				 xNoise, yNoise);
			SetRightEar(player2->tileItem.x,
				    player2->tileItem.y);
			x = player1->tileItem.x;
			y = player1->tileItem.y;
			BlackLine();
		}
	} else if (player1) {
		SetClip(0, 0, 319, 199);
		DoBuffer(b, player1->tileItem.x, player1->tileItem.y, 0,
			 X_TILES, xNoise, yNoise);
		SetLeftEar(player1->tileItem.x, player1->tileItem.y);
		SetRightEar(player1->tileItem.x, player1->tileItem.y);
		x = player1->tileItem.x;
		y = player1->tileItem.y;
	} else if (player2) {
		SetClip(0, 0, 319, 199);
		DoBuffer(b, player2->tileItem.x, player2->tileItem.y, 0,
			 X_TILES, xNoise, yNoise);
		SetLeftEar(player2->tileItem.x, player2->tileItem.y);
		SetRightEar(player2->tileItem.x, player2->tileItem.y);
		x = player2->tileItem.x;
		y = player2->tileItem.y;
	} else
		DoBuffer(b, x, y, 0, X_TILES, xNoise, yNoise);
	SetClip(0, 0, 319, 199);
}

void PlayerStatus(int x, struct PlayerData *data, TActor * p)
{
	char s[50];

	TextStringAt(x, 5, data->name);
	if (!gCampaign.dogFight)
		sprintf(s, "Score: %d", data->score);
	else
		s[0] = 0;
	if (p) {
		TextStringAt(x, 5 + 2 + 2 * TextHeight(), s);
		TextStringAt(x, 5 + 1 + TextHeight(),
			     gunDesc[p->gun].gunName);
		sprintf(s, "%d hp", p->health);
		TextStringAt(x, 5 + 3 + 3 * TextHeight(), s);
	} else
		TextStringAt(x, 5 + 1 + TextHeight(), s);
}

static void DrawKeycard(int x, int y, const TOffsetPic * pic)
{
	DrawTPic(x + pic->dx, y + pic->dy, gPics[pic->picIndex],
		 gCompiledPics[pic->picIndex]);
}

static void MarkExit(void)
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

static void MissionStatus(void)
{
	unsigned char *scr = GetDstScreen();
	unsigned char color;
	int i, left;
	char s[4];
	int allDone = 1;
	static int completed = 0;
	int x;

	if (gCampaign.dogFight)
		return;

	x = 109;
	for (i = 0; i < gMission.missionData->objectiveCount; i++) {
		if (gMission.missionData->objectives[i].type ==
		    OBJECTIVE_INVESTIGATE)
			gMission.objectives[i].done = ExploredPercentage();

		if (gMission.missionData->objectives[i].required > 0) {
			// Objective color dot
			color = gMission.objectives[i].color;
			scr[x + 195 * 320] = color;
			scr[x + 1 + 195 * 320] = color;
			scr[x + 1 + 194 * 320] = color;
			scr[x + 194 * 320] = color;

			// Black frame
			scr[x - 1 + 193 * 320] = 1;
			scr[x - 1 + 194 * 320] = 1;
			scr[x - 1 + 195 * 320] = 1;
			scr[x - 1 + 196 * 320] = 1;
			scr[x + 193 * 320] = 1;
			scr[x + 196 * 320] = 1;
			scr[x + 1 + 193 * 320] = 1;
			scr[x + 1 + 196 * 320] = 1;
			scr[x + 2 + 193 * 320] = 1;
			scr[x + 2 + 194 * 320] = 1;
			scr[x + 2 + 195 * 320] = 1;
			scr[x + 2 + 196 * 320] = 1;

			left =
			    gMission.objectives[i].required -
			    gMission.objectives[i].done;
			if (left > 0) {
				if ((gMission.missionData->objectives[i].
				     flags & OBJECTIVE_UNKNOWNCOUNT) == 0)
					sprintf(s, "%d", left);
				else
					strcpy(s, "?");
				TextStringAt(x + 5, 190, s);
				allDone = 0;
			} else
				TextStringAt(x + 5, 190, "Done");
			x += 25;
		}
	}

	if (allDone && !completed) {
		completed = 1;
		MarkExit();
	} else if (!allDone)
		completed = 0;
}

void StatusDisplay(void)
{
	char s[50];

	PlayerStatus(5, &gPlayer1Data, gPlayer1);
	if (gOptions.twoPlayers)
		PlayerStatus(250, &gPlayer2Data, gPlayer2);

	if (!gPlayer1 && !gPlayer2) {
		if (!gCampaign.dogFight)
			TextStringAt(120, 95, "Game Over");
		else
			TextStringAt(120, 95, "Double kill");
	}

	else if (MissionCompleted()) {
		sprintf(s, "Pickup in %d seconds\n",
			(gMission.pickupTime + 69) / 70);
		TextStringAt(100, 95, s);
	}

	if (gameIsPaused) {
		if (escExits)
			TextStringAt(90, 80, "Esc again to quit");
		else
			TextStringAt(90, 5, "Paused");
	}

	if (messageTicks > 0)
		TextStringAt(90, 20, message);

	if (gOptions.displayFPS) {
		sprintf(s, "%d fps", fps);
		TextStringAt(250, 190, s);
	}
	if (gOptions.displayTime) {
		sprintf(s, "%02d:%02d", timeHours, timeMinutes);
		TextStringAt(10, 190, s);
	}
	if (gMission.flags & FLAGS_KEYCARD_YELLOW)
		DrawKeycard(155, 8, &cGeneralPics[gMission.keyPics[0]]);
	if (gMission.flags & FLAGS_KEYCARD_GREEN)
		DrawKeycard(170, 8, &cGeneralPics[gMission.keyPics[1]]);
	if (gMission.flags & FLAGS_KEYCARD_BLUE)
		DrawKeycard(185, 8, &cGeneralPics[gMission.keyPics[2]]);
	if (gMission.flags & FLAGS_KEYCARD_RED)
		DrawKeycard(200, 8, &cGeneralPics[gMission.keyPics[3]]);

	sprintf(s, "%d:%02d", missionTime / 4200, (missionTime / 70) % 60);
	TextStringAt(130, 5, s);

	MissionStatus();
}

void DisplayMessage(const char *s)
{
	strcpy(message, s);
	messageTicks = 140;
}

int HandleKey(int *done, int cmd)
{
	static int lastKey = 0;
	int key = GetKeyDown();

	if ((key == gOptions.mapKey || (cmd & CMD_BUTTON3) != 0) &&
	    !gCampaign.dogFight) {
		DisplayAutoMap(0);
		gameTicks = 0;
	}

	if (((cmd & CMD_BUTTON4) != 0) && !gOptions.twoPlayers) {
		gameIsPaused = YES;
		escExits = NO;
	} else if (gameIsPaused && AnyButton(cmd))
		gameIsPaused = NO;

	if (key == lastKey)
		return 0;

	lastKey = key;
	if (!key)
		return 0;

	if (!gPlayer1 && !gPlayer2)
		*done = YES;
	else if (key == keyEsc) {
		if (gameIsPaused && escExits)
			*done = YES;
		else if (!gameIsPaused) {
			gameIsPaused = YES;
			escExits = YES;
		}
	} else
		gameIsPaused = NO;

//  if (key >= key1 && key <= key0)
//    ToggleTrack( key - key1);

	return key;
}

static void GetPlayerInput(int *cmd1, int *cmd2)
{
	*cmd1 = *cmd2 = 0;
	GetPlayerCmd(gPlayer1 ? cmd1 : NULL, gPlayer2 ? cmd2 : NULL);
}

int gameloop(void)
{
	struct Buffer *buffer;
	int ticks;
	int cmd1, cmd2;
	int done = NO;
	int c;
	int timeTicks = CLOCK_LIMIT;
	time_t t;
	struct tm *tp;

	buffer = malloc(sizeof(struct Buffer));
	SetClip(0, 0, 319, 199);

	if (ModuleStatus() != MODULE_OK)
		DisplayMessage(ModuleMessage());

	gameIsPaused = NO;
	gameTicks = fpsGameTicks = frames = 0;
	missionTime = 0;
	screenShaking = 0;
	while (!done) {
		ticks = Synchronize();

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
		frames++;
		if (frames >= FPS_FRAMELIMIT && fpsGameTicks > 0) {
			fps =
			    (frames * GAMETICKS_PER_SECOND +
			     fpsGameTicks / 2) / fpsGameTicks;
			frames = fpsGameTicks = 0;
		}
		timeTicks += ticks;
		if (timeTicks >= CLOCK_LIMIT) {
			t = time(NULL);
			tp = localtime(&t);
			timeHours = tp->tm_hour;
			timeMinutes = tp->tm_min;
			timeTicks = 0;
		}
		if (messageTicks > 0)
			messageTicks -= ticks;

		if (!gameIsPaused) {
			missionTime += ticks;
			if ((gPlayer1 || gPlayer2) && MissionCompleted()) {
				if (gMission.pickupTime == PICKUP_LIMIT)
					PlaySound(SND_DONE, 0, 255);
				gMission.pickupTime -= ticks;
				if (gMission.pickupTime <= 0)
					done = YES;
			} else
				gMission.pickupTime = PICKUP_LIMIT;
		}

		StatusDisplay();

		if (gameTicks < GAMETICKS_PER_FRAME) {
			gameTicks = 0;
		} else {
			gameTicks -= GAMETICKS_PER_FRAME;
			if (gameTicks < 0)
				gameTicks = 0;
		}

		if (gOptions.displaySlices)
			SetColorZero(0, 0, 32);
		CopyToScreen();
		if (gOptions.displaySlices)
			SetColorZero(0, 0, 0);

		if (!gameIsPaused) {
			if (!gOptions.slowmotion || (frames & 1) == 0) {
				UpdateAllActors(ticks);
				UpdateMobileObjects();

				GetPlayerInput(&cmd1, &cmd2);

				if (gPlayer1
				    && !PlayerSpecialCommands(gPlayer1,
							      cmd1,
							      &gPlayer1Data))
					CommandActor(gPlayer1, cmd1);
				if (gPlayer2
				    && !PlayerSpecialCommands(gPlayer2,
							      cmd2,
							      &gPlayer2Data))
					CommandActor(gPlayer2, cmd2);

				if (gOptions.badGuys)
					CommandBadGuys();

				UpdateWatches();
			}
		} else
			GetPlayerInput(&cmd1, &cmd2);

		DoSounds();

		c = HandleKey(&done, cmd1 | cmd2);
	}
	free(buffer);

	return c != keyEsc;
}
