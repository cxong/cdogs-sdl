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

    Copyright (c) 2013-2014, Cong Xu
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

#include <assert.h>
#include <math.h>
#include <time.h>

#include "actors.h"
#include "automap.h"
#include "drawtools.h"
#include "game_events.h"
#include "mission.h"
#include "pic_manager.h"
#include "text.h"

// Numeric update for health and score
// Displays as a small pop-up coloured text overlay
typedef struct
{
	int Index;	// could be player or objective
	int Amount;
	// Number of milliseconds that this update will last
	int TimerMax;
	int Timer;
} HUDNumUpdate;

// Total number of milliseconds that the numeric update lasts for
#define NUM_UPDATE_TIMER_MS 500

#define NUM_UPDATE_TIMER_OBJECTIVE_MS 1500


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
	CDogsTextStringSpecial(s, TEXT_RIGHT | TEXT_BOTTOM, 10, 5 + CDogsTextHeight());
}

void WallClockSetTime(WallClock *wc)
{
	time_t t = time(NULL);
	struct tm *tp = localtime(&t);
	wc->hours = tp->tm_hour;
	wc->minutes = tp->tm_min;
}
void WallClockInit(WallClock *wc)
{
	wc->elapsed = 0;
	WallClockSetTime(wc);
}
void WallClockUpdate(WallClock *wc, int ms)
{
	wc->elapsed += ms;
	int minuteMs = 60 * 1000;
	if (wc->elapsed > minuteMs)	// update every minute
	{
		WallClockSetTime(wc);
		wc->elapsed -= minuteMs;
	}
}
void WallClockDraw(WallClock *wc)
{
	char s[50];
	sprintf(s, "%02d:%02d", wc->hours, wc->minutes);
	CDogsTextStringSpecial(s, TEXT_LEFT | TEXT_BOTTOM, 10, 5 + CDogsTextHeight());
}

void HUDInit(
	HUD *hud,
	InterfaceConfig *config,
	GraphicsDevice *device,
	struct MissionOptions *mission)
{
	hud->mission = mission;
	strcpy(hud->message, "");
	hud->messageTicks = 0;
	hud->config = config;
	hud->device = device;
	FPSCounterInit(&hud->fpsCounter);
	WallClockInit(&hud->clock);
	CArrayInit(&hud->healthUpdates, sizeof(HUDNumUpdate));
	CArrayInit(&hud->scoreUpdates, sizeof(HUDNumUpdate));
	CArrayInit(&hud->objectiveUpdates, sizeof(HUDNumUpdate));
	hud->showExit = false;
}
void HUDTerminate(HUD *hud)
{
	CArrayTerminate(&hud->healthUpdates);
	CArrayTerminate(&hud->scoreUpdates);
	CArrayTerminate(&hud->objectiveUpdates);
}

void HUDDisplayMessage(HUD *hud, const char *msg, int ticks)
{
	strcpy(hud->message, msg);
	hud->messageTicks = ticks;
}

void HUDAddHealthUpdate(HUD *hud, int playerIndex, int health)
{
	HUDNumUpdate s;
	s.Index = playerIndex;
	s.Amount = health;
	s.Timer = NUM_UPDATE_TIMER_MS;
	s.TimerMax = NUM_UPDATE_TIMER_MS;
	CArrayPushBack(&hud->healthUpdates, &s);
}

void HUDAddScoreUpdate(HUD *hud, int playerIndex, int score)
{
	HUDNumUpdate s;
	s.Index = playerIndex;
	s.Amount = score;
	s.Timer = NUM_UPDATE_TIMER_MS;
	s.TimerMax = NUM_UPDATE_TIMER_MS;
	CArrayPushBack(&hud->scoreUpdates, &s);
}

void HUDAddObjectiveUpdate(HUD *hud, int objectiveIndex, int update)
{
	HUDNumUpdate u;
	u.Index = objectiveIndex;
	u.Amount = update;
	u.Timer = NUM_UPDATE_TIMER_OBJECTIVE_MS;
	u.TimerMax = NUM_UPDATE_TIMER_OBJECTIVE_MS;
	CArrayPushBack(&hud->objectiveUpdates, &u);
}

void HUDUpdate(HUD *hud, int ms)
{
	if (hud->messageTicks >= 0)
	{
		hud->messageTicks -= ms;
		if (hud->messageTicks < 0)
		{
			hud->messageTicks = 0;
		}
	}
	FPSCounterUpdate(&hud->fpsCounter, ms);
	WallClockUpdate(&hud->clock, ms);
	for (int i = 0; i < (int)hud->healthUpdates.size; i++)
	{
		HUDNumUpdate *health = CArrayGet(&hud->healthUpdates, i);
		health->Timer -= ms;
		if (health->Timer <= 0)
		{
			CArrayDelete(&hud->healthUpdates, i);
			i--;
		}
	}
	for (int i = 0; i < (int)hud->scoreUpdates.size; i++)
	{
		HUDNumUpdate *score = CArrayGet(&hud->scoreUpdates, i);
		score->Timer -= ms;
		if (score->Timer <= 0)
		{
			CArrayDelete(&hud->scoreUpdates, i);
			i--;
		}
	}
	for (int i = 0; i < (int)hud->objectiveUpdates.size; i++)
	{
		HUDNumUpdate *update = CArrayGet(&hud->objectiveUpdates, i);
		update->Timer -= ms;
		if (update->Timer <= 0)
		{
			CArrayDelete(&hud->objectiveUpdates, i);
			i--;
		}
	}
}


// Draw a gauge with an outer background and inner level
// +--------------+
// |XXXXXXXX|     |
// +--------------+
static void DrawGauge(
	GraphicsDevice *device,
	Vec2i pos, Vec2i size, int innerWidth,
	color_t barColor, color_t backColor,
	int textFlags)
{
	Vec2i offset = Vec2iUnit();
	Vec2i barPos = Vec2iAdd(pos, offset);
	Vec2i barSize = Vec2iNew(MAX(0, innerWidth - 2), size.y - 2);
	if (textFlags & TEXT_RIGHT)
	{
		int w = device->cachedConfig.ResolutionWidth;
		pos.x = w - pos.x - size.x - offset.x;
		barPos.x = w - barPos.x - barSize.x - offset.x;
	}
	if (textFlags & TEXT_BOTTOM)
	{
		int h = device->cachedConfig.ResolutionHeight;
		pos.y = h - pos.y - size.y - offset.y;
		barPos.y = h - barPos.y - barSize.y - offset.y;
	}
	DrawRectangle(device, pos, size, backColor, DRAW_FLAG_ROUNDED);
	DrawRectangle(device, barPos, barSize, barColor, 0);
}

#define GAUGE_WIDTH 50
static void DrawWeaponStatus(
	GraphicsDevice *device, const Weapon *weapon, Vec2i pos, int textFlags)
{
	// don't draw gauge if not reloading
	if (weapon->lock > 0)
	{
		Vec2i gaugePos = Vec2iAdd(pos, Vec2iNew(-1, -1));
		Vec2i size = Vec2iNew(GAUGE_WIDTH, CDogsTextHeight() + 1);
		color_t barColor = { 0, 0, 255, 255 };
		int maxLock = gGunDescriptions[weapon->gun].Lock;
		int innerWidth;
		color_t backColor = { 128, 128, 128, 255 };
		if (maxLock == 0)
		{
			innerWidth = 0;
		}
		else
		{
			innerWidth = MAX(1, size.x * (maxLock - weapon->lock) / maxLock);
		}
		DrawGauge(
			device, gaugePos, size, innerWidth, barColor, backColor, textFlags);
	}
	CDogsTextStringSpecial(GunGetName(weapon->gun), textFlags, pos.x, pos.y);
}

static void DrawHealth(
	GraphicsDevice *device, TActor *actor, Vec2i pos, int textFlags)
{
	char s[50];
	Vec2i gaugePos = Vec2iAdd(pos, Vec2iNew(-1, -1));
	Vec2i size = Vec2iNew(GAUGE_WIDTH, CDogsTextHeight() + 1);
	HSV hsv = { 0.0, 1.0, 1.0 };
	color_t barColor;
	int health = actor->health;
	int maxHealth = actor->character->maxHealth;
	int innerWidth;
	color_t backColor = { 50, 0, 0, 255 };
	innerWidth = MAX(1, size.x * health / maxHealth);
	if (actor->poisoned)
	{
		hsv.h = 120.0;
		hsv.v = 0.5;
	}
	else
	{
		double maxHealthHue = 50.0;
		double minHealthHue = 0.0;
		hsv.h =
			((maxHealthHue - minHealthHue) * health / maxHealth + minHealthHue);
	}
	barColor = ColorTint(colorWhite, hsv);
	DrawGauge(
		device, gaugePos, size, innerWidth, barColor, backColor, textFlags);
	sprintf(s, "%d", health);
	CDogsTextStringSpecial(s, textFlags, pos.x, pos.y);
}

#define HUDFLAGS_PLACE_RIGHT	0x01
#define HUDFLAGS_PLACE_BOTTOM	0x02
#define HUDFLAGS_HALF_SCREEN	0x04
#define HUDFLAGS_QUARTER_SCREEN	0x08
#define HUDFLAGS_SHARE_SCREEN	0x10

#define AUTOMAP_PADDING	5
#define AUTOMAP_SIZE	45
static void DrawRadar(
	GraphicsDevice *device, TActor *p, int scale, int flags, bool showExit)
{
	Vec2i pos = Vec2iZero();
	int w = device->cachedConfig.ResolutionWidth;
	int h = device->cachedConfig.ResolutionHeight;

	if (!p)
	{
		return;
	}

	// Possible map positions:
	// top-right (player 1 only)
	// top-left (player 2 only)
	// bottom-right (player 3 only)
	// bottom-left (player 4 only)
	// top-left-of-middle (player 1 when two players)
	// top-right-of-middle (player 2 when two players)
	// bottom-left-of-middle (player 3)
	// bottom-right-of-middle (player 4)
	// top (shared screen)
	if (flags & HUDFLAGS_HALF_SCREEN)
	{
		// two players
		pos.y = AUTOMAP_PADDING;
		if (flags & HUDFLAGS_PLACE_RIGHT)
		{
			// player 2
			pos.x = w / 2 + AUTOMAP_PADDING;
		}
		else
		{
			// player 1
			pos.x = w / 2 - AUTOMAP_SIZE - AUTOMAP_PADDING;
		}
	}
	else if (flags & HUDFLAGS_QUARTER_SCREEN)
	{
		// four players
		if (flags & HUDFLAGS_PLACE_RIGHT)
		{
			// player 2 or 4
			pos.x = w / 2 + AUTOMAP_PADDING;
		}
		else
		{
			// player 1 or 3
			pos.x = w / 2 - AUTOMAP_SIZE - AUTOMAP_PADDING;
		}

		if (flags & HUDFLAGS_PLACE_BOTTOM)
		{
			// player 3 or 4
			pos.y = h - AUTOMAP_SIZE - AUTOMAP_PADDING;
		}
		else
		{
			// player 1 or 2
			pos.y = AUTOMAP_PADDING;
		}
	}
	else
	{
		// one player
		if (flags & HUDFLAGS_PLACE_RIGHT)
		{
			// player 2 or 4
			pos.x = AUTOMAP_PADDING;
		}
		else
		{
			// player 1 or 3
			pos.x = w - AUTOMAP_SIZE - AUTOMAP_PADDING;
		}

		if (flags & HUDFLAGS_PLACE_BOTTOM)
		{
			// player 3 or 4
			pos.y = h - AUTOMAP_SIZE - AUTOMAP_PADDING;
		}
		else
		{
			// player 1 or 2
			pos.y = AUTOMAP_PADDING;
		}
	}

	if (!Vec2iEqual(pos, Vec2iZero()))
	{
		Vec2i playerPos = Vec2iNew(
			p->tileItem.x / TILE_WIDTH, p->tileItem.y / TILE_HEIGHT);
		AutomapDrawRegion(
			&gMap,
			pos,
			Vec2iNew(AUTOMAP_SIZE, AUTOMAP_SIZE),
			playerPos,
			scale,
			AUTOMAP_FLAGS_MASK,
			showExit);
	}
}

static void DrawSharedRadar(
	GraphicsDevice *device, TActor *players[MAX_PLAYERS], int scale,
	bool showExit)
{
	int w = device->cachedConfig.ResolutionWidth;
	Vec2i pos = Vec2iNew(w / 2 - AUTOMAP_SIZE / 2, AUTOMAP_PADDING);
	Vec2i playerMidpoint = PlayersGetMidpoint(players);
	playerMidpoint.x /= TILE_WIDTH;
	playerMidpoint.y /= TILE_HEIGHT;
	AutomapDrawRegion(
		&gMap,
		pos,
		Vec2iNew(AUTOMAP_SIZE, AUTOMAP_SIZE),
		playerMidpoint,
		scale,
		AUTOMAP_FLAGS_MASK,
		showExit);
}

#define RADAR_SCALE 1
// A bit of padding for drawing HUD elements at bottm,
// so that it doesn't overlap the objective information, clocks etc.
#define BOTTOM_PADDING 16

static void DrawObjectiveCompass(
	GraphicsDevice *g, Vec2i playerPos, Rect2i r, bool showExit);
// Draw player's score, health etc.
static void DrawPlayerStatus(
	GraphicsDevice *device, struct PlayerData *data, TActor *p, int flags,
	Rect2i r, bool showExit)
{
	if (p != NULL)
	{
		DrawObjectiveCompass(device, Vec2iFull2Real(p->Pos), r, showExit);
	}

	Vec2i pos = Vec2iNew(5, 5);
	char s[50];
	int textFlags = TEXT_TOP | TEXT_LEFT;
	if (flags & HUDFLAGS_PLACE_RIGHT)
	{
		textFlags |= TEXT_RIGHT;
	}
	if (flags & HUDFLAGS_PLACE_BOTTOM)
	{
		textFlags |= TEXT_BOTTOM;
		pos.y += BOTTOM_PADDING;
	}

	CDogsTextStringSpecial(data->name, textFlags, pos.x, pos.y);
	const int rowHeight = 1 + CDogsTextHeight();
	pos.y += rowHeight;
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
		DrawWeaponStatus(device, &p->weapon, pos, textFlags);
		pos.y += rowHeight;
		CDogsTextStringSpecial(s, textFlags, pos.x, pos.y);
		pos.y += rowHeight;
		DrawHealth(device, p, pos, textFlags);
	}
	else
	{
		CDogsTextStringSpecial(s, textFlags, pos.x, pos.y);
	}

	if (gConfig.Interface.ShowHUDMap && !(flags & HUDFLAGS_SHARE_SCREEN) &&
		gCampaign.Entry.mode != CAMPAIGN_MODE_DOGFIGHT)
	{
		DrawRadar(device, p, RADAR_SCALE, flags, showExit);
	}
}


static void DrawCompassArrow(
	GraphicsDevice *g, Rect2i r, Vec2i pos, Vec2i playerPos, color_t mask,
	const char *label);
static void DrawObjectiveCompass(
	GraphicsDevice *g, Vec2i playerPos, Rect2i r, bool showExit)
{
	// Draw exit position
	if (showExit)
	{
		DrawCompassArrow(
			g, r, MapGetExitPos(&gMap), playerPos, colorGreen, "Exit");
	}

	// Draw objectives
	Map *map = &gMap;
	Vec2i tilePos;
	for (tilePos.y = 0; tilePos.y < map->Size.y; tilePos.y++)
	{
		for (tilePos.x = 0; tilePos.x < map->Size.x; tilePos.x++)
		{
			Tile *tile = MapGetTile(map, tilePos);
			for (TTileItem *t = tile->things; t; t = t->next)
			{
				if (!(t->flags & TILEITEM_OBJECTIVE))
				{
					continue;
				}
				int objective = ObjectiveFromTileItem(t->flags);
				MissionObjective *mo =
					CArrayGet(&gMission.missionData->Objectives, objective);
				if (mo->Flags & OBJECTIVE_HIDDEN)
				{
					continue;
				}
				if (!(mo->Flags & OBJECTIVE_POSKNOWN) &&
					!tile->isVisited)
				{
					continue;
				}
				struct Objective *o =
					CArrayGet(&gMission.Objectives, objective);
				color_t color = o->color;
				DrawCompassArrow(
					g, r, Vec2iNew(t->x, t->y), playerPos, color, NULL);
			}
		}
	}
}

static void DrawCompassArrow(
	GraphicsDevice *g, Rect2i r, Vec2i pos, Vec2i playerPos, color_t mask,
	const char *label)
{
	Vec2i compassV = Vec2iMinus(pos, playerPos);
	// Don't draw if objective is on screen
	if (abs(pos.x - playerPos.x) < r.Size.x / 2 &&
		abs(pos.y - playerPos.y) < r.Size.y / 2)
	{
		return;
	}
	Vec2i textPos = Vec2iZero();
	// Find which edge of screen is the best
	bool hasDrawn = false;
	if (compassV.x != 0)
	{
		double sx = r.Size.x / 2.0 / compassV.x;
		int yInt = (int)floor(fabs(sx) * compassV.y + 0.5);
		if (yInt >= -r.Size.y / 2 && yInt <= r.Size.y / 2)
		{
			// Intercepts either left or right side
			hasDrawn = true;
			if (compassV.x > 0)
			{
				// right edge
				textPos = Vec2iNew(
					r.Pos.x + r.Size.x, r.Pos.y + r.Size.y / 2 + yInt);
				Pic *p = PicManagerGetPic(&gPicManager, "arrow_right");
				Vec2i drawPos = Vec2iNew(
					textPos.x - p->size.x, textPos.y - p->size.y / 2);
				BlitMasked(g, p, drawPos, mask, true);
			}
			else if (compassV.x < 0)
			{
				// left edge
				textPos = Vec2iNew(r.Pos.x, r.Pos.y + r.Size.y / 2 + yInt);
				Pic *p = PicManagerGetPic(&gPicManager, "arrow_left");
				Vec2i drawPos = Vec2iNew(textPos.x, textPos.y - p->size.y / 2);
				BlitMasked(g, p, drawPos, mask, true);
			}
		}
	}
	if (!hasDrawn && compassV.y != 0)
	{
		double sy = r.Size.y / 2.0 / compassV.y;
		int xInt = (int)floor(fabs(sy) * compassV.x + 0.5);
		if (xInt >= -r.Size.x / 2 && xInt <= r.Size.x / 2)
		{
			// Intercepts either top or bottom side
			hasDrawn = true;
			if (compassV.y > 0)
			{
				// bottom edge
				textPos = Vec2iNew(
					r.Pos.x + r.Size.x / 2 + xInt, r.Pos.y + r.Size.y);
				Pic *p = PicManagerGetPic(&gPicManager, "arrow_down");
				Vec2i drawPos = Vec2iNew(
					textPos.x - p->size.x / 2, textPos.y - p->size.y);
				BlitMasked(g, p, drawPos, mask, true);
			}
			else if (compassV.y < 0)
			{
				// top edge
				textPos = Vec2iNew(r.Pos.x + r.Size.x / 2 + xInt, r.Pos.y);
				Pic *p = PicManagerGetPic(&gPicManager, "arrow_up");
				Vec2i drawPos = Vec2iNew(textPos.x - p->size.x / 2, textPos.y);
				BlitMasked(g, p, drawPos, mask, true);
			}
		}
	}
	if (label && strlen(label) > 0)
	{
		Vec2i textSize = TextGetSize(label);
		// Center the text around the target position
		textPos.x -= textSize.x / 2;
		textPos.y -= textSize.y / 2;
		// Make sure the text is inside the screen
		int padding = 8;
		textPos.x = MAX(textPos.x, r.Pos.x + padding);
		textPos.x = MIN(textPos.x, r.Pos.x + r.Size.x - textSize.x - padding);
		textPos.y = MAX(textPos.y, r.Pos.y + padding);
		textPos.y = MIN(textPos.y, r.Pos.y + r.Size.y - textSize.y - padding);
		TextStringMasked(&gTextManager, label, g, textPos, mask);
	}
}

static void DrawKeycard(int x, int y, const TOffsetPic * pic)
{
	DrawTPic(
		x + pic->dx, y + pic->dy,
		PicManagerGetOldPic(&gPicManager, pic->picIndex));
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

static void DrawHealthUpdate(HUDNumUpdate *health, int flags);
static void DrawScoreUpdate(HUDNumUpdate *score, int flags);
static void DrawObjectiveCounts(HUD *hud);
void HUDDraw(HUD *hud, int isPaused)
{
	char s[50];
	int flags = 0;
	int numPlayersAlive = GetNumPlayersAlive();
	int i;

	Rect2i r;
	r.Size = Vec2iNew(
		hud->device->cachedConfig.ResolutionWidth,
		hud->device->cachedConfig.ResolutionHeight);
	if (numPlayersAlive <= 1)
	{
		flags = 0;
	}
	else if (gConfig.Interface.Splitscreen == SPLITSCREEN_NEVER)
	{
		flags |= HUDFLAGS_SHARE_SCREEN;
	}
	else if (gOptions.numPlayers == 2)
	{
		r.Size.x /= 2;
		flags |= HUDFLAGS_HALF_SCREEN;
	}
	else if (gOptions.numPlayers == 3 || gOptions.numPlayers == 4)
	{
		r.Size.x /= 2;
		r.Size.y /= 2;
		flags |= HUDFLAGS_QUARTER_SCREEN;
	}
	else
	{
		assert(0 && "not implemented");
	}

	for (i = 0; i < gOptions.numPlayers; i++)
	{
		int drawFlags = flags;
		r.Pos = Vec2iZero();
		if (i & 1)
		{
			r.Pos.x = r.Size.x;
			drawFlags |= HUDFLAGS_PLACE_RIGHT;
		}
		if (i >= 2)
		{
			r.Pos.y = r.Size.y;
			drawFlags |= HUDFLAGS_PLACE_BOTTOM;
		}
		DrawPlayerStatus(
			hud->device, &gPlayerDatas[i], gPlayers[i], drawFlags,
			r, hud->showExit);
		for (int j = 0; j < (int)hud->healthUpdates.size; j++)
		{
			HUDNumUpdate *health = CArrayGet(&hud->healthUpdates, j);
			if (health->Index == i)
			{
				DrawHealthUpdate(health, drawFlags);
			}
		}
		for (int j = 0; j < (int)hud->scoreUpdates.size; j++)
		{
			HUDNumUpdate *score = CArrayGet(&hud->scoreUpdates, j);
			if (score->Index == i)
			{
				DrawScoreUpdate(score, drawFlags);
			}
		}
	}
	// Only draw radar once if shared
	if (gConfig.Interface.ShowHUDMap && (flags & HUDFLAGS_SHARE_SCREEN) &&
		gCampaign.Entry.mode != CAMPAIGN_MODE_DOGFIGHT)
	{
		DrawSharedRadar(hud->device, gPlayers, RADAR_SCALE, hud->showExit);
	}

	if (numPlayersAlive == 0)
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
	else if (hud->mission->state == MISSION_STATE_PICKUP)
	{
		int timeLeft = gMission.pickupTime + PICKUP_LIMIT - gMission.time;
		sprintf(s, "Pickup in %d seconds\n",
			(timeLeft + (FPS_FRAMELIMIT - 1)) / FPS_FRAMELIMIT);
		CDogsTextStringAtCenter(s);
	}

	if (isPaused)
	{
		CDogsTextStringAtCenter("Press Esc again to quit");
	}

	if (hud->messageTicks > 0 || hud->messageTicks == -1)
	{
		// Draw the message centered, and just below the automap
		Vec2i pos = Vec2iNew(
			(hud->device->cachedConfig.ResolutionWidth -
			TextGetStringWidth(hud->message)) / 2,
			AUTOMAP_SIZE + AUTOMAP_PADDING + AUTOMAP_PADDING);
		TextStringMasked(
			&gTextManager, hud->message, hud->device, pos, colorCyan);
	}

	if (hud->config->ShowFPS)
	{
		FPSCounterDraw(&hud->fpsCounter);
	}
	if (hud->config->ShowTime)
	{
		WallClockDraw(&hud->clock);
	}

	DrawKeycards(hud);

	// Draw elapsed mission time as MM:SS
	int missionTimeSeconds = gMission.time / FPS_FRAMELIMIT;
	sprintf(s, "%d:%02d",
		missionTimeSeconds / 60, missionTimeSeconds % 60);
	DrawTextStringSpecial(
		s, TEXT_TOP | TEXT_XCENTER, Vec2iZero(),
		Vec2iNew(
			hud->device->cachedConfig.ResolutionWidth,
			hud->device->cachedConfig.ResolutionHeight),
		Vec2iNew(0, 5));

	if (HasObjectives(gCampaign.Entry.mode))
	{
		DrawObjectiveCounts(hud);
	}
}

static void DrawNumUpdate(
	HUDNumUpdate *update,
	const char *formatText, int currentValue, Vec2i pos, int flags);
static void DrawHealthUpdate(HUDNumUpdate *health, int flags)
{
	const int rowHeight = 1 + CDogsTextHeight();
	int y = 5 + 1 + CDogsTextHeight() + rowHeight * 2;
	DrawNumUpdate(
		health, "%d", gPlayers[health->Index]->health,
		Vec2iNew(5, y), flags);
}
static void DrawScoreUpdate(HUDNumUpdate *score, int flags)
{
	if (!IsScoreNeeded(gCampaign.Entry.mode))
	{
		return;
	}
	const int rowHeight = 1 + CDogsTextHeight();
	int y = 5 + 1 + CDogsTextHeight() + rowHeight;
	DrawNumUpdate(
		score, "Score: %d", gPlayerDatas[score->Index].score,
		Vec2iNew(5, y), flags);
}
// Parameters that define how the numeric update is animated
// The update animates in the following phases:
// 1. Pop up from text position to slightly above
// 2. Fall down from slightly above back to text position
// 3. Persist over text position
#define NUM_UPDATE_POP_UP_DURATION_MS 100
#define NUM_UPDATE_FALL_DOWN_DURATION_MS 100
#define NUM_UPDATE_POP_UP_HEIGHT 5
static void DrawNumUpdate(
	HUDNumUpdate *update,
	const char *formatText, int currentValue, Vec2i pos, int flags)
{
	CASSERT(update->Amount != 0, "num update with zero amount");
	color_t color = update->Amount > 0 ? colorGreen : colorRed;
	
	// Find the right position to draw the update
	// Make sure the update is displayed lined up with the lowest digits
	// Find the position of where the normal text is displayed,
	// and move to its right
	char s[50];
	sprintf(s, formatText, currentValue);
	pos.x += TextGetSize(s).x;
	// Then find the size of the update, and move left
	sprintf(s, "%s%d", update->Amount > 0 ? "+" : "", update->Amount);
	pos.x -= TextGetSize(s).x;
	// The final position should ensure the score update's lowest digit
	// lines up with the normal score's lowest digit

	// Now animate the score update based on its stage
	int timer = update->TimerMax - update->Timer;
	if (timer < NUM_UPDATE_POP_UP_DURATION_MS)
	{
		// update is still popping up
		// calculate height
		int popupHeight =
			timer * NUM_UPDATE_POP_UP_HEIGHT / NUM_UPDATE_POP_UP_DURATION_MS;
		pos.y -= popupHeight;
	}
	else if (timer <
		NUM_UPDATE_POP_UP_DURATION_MS + NUM_UPDATE_FALL_DOWN_DURATION_MS)
	{
		// update is falling down
		// calculate height
		timer -= NUM_UPDATE_POP_UP_DURATION_MS;
		timer = NUM_UPDATE_FALL_DOWN_DURATION_MS - timer;
		int popupHeight =
			timer * NUM_UPDATE_POP_UP_HEIGHT / NUM_UPDATE_FALL_DOWN_DURATION_MS;
		pos.y -= popupHeight;
	}
	else
	{
		// Change alpha so that the update fades away
		color.a = (Uint8)(update->Timer * 255 / update->TimerMax);
	}
	
	int textFlags = TEXT_TOP | TEXT_LEFT;
	if (flags & HUDFLAGS_PLACE_RIGHT)
	{
		textFlags |= TEXT_RIGHT;
	}
	if (flags & HUDFLAGS_PLACE_BOTTOM)
	{
		textFlags |= TEXT_BOTTOM;
	}

	Vec2i screenSize = Vec2iNew(
		gGraphicsDevice.cachedConfig.ResolutionWidth,
		gGraphicsDevice.cachedConfig.ResolutionHeight);
	DrawTextStringSpecialBlend(
		&gTextManager, s, &gGraphicsDevice, textFlags, Vec2iZero(),
		screenSize, pos, color);
}

static void DrawObjectiveCounts(HUD *hud)
{
	int x = 5 + GAUGE_WIDTH;
	int y = hud->device->cachedConfig.ResolutionHeight - 5 - CDogsTextHeight();
	for (int i = 0; i < (int)gMission.missionData->Objectives.size; i++)
	{
		MissionObjective *mo = CArrayGet(&gMission.missionData->Objectives, i);
		struct Objective *o = CArrayGet(&gMission.Objectives, i);

		// Don't draw anything for optional objectives
		if (mo->Required == 0)
		{
			continue;
		}

		// Objective color dot
		Draw_Rect(x, y + 3, 2, 2, o->color);

		x += 5;
		char s[8];
		int itemsLeft = mo->Required - o->done;
		if (itemsLeft > 0)
		{
			if (!(mo->Flags & OBJECTIVE_UNKNOWNCOUNT))
			{
				sprintf(s, "%d", itemsLeft);
			}
			else
			{
				strcpy(s, "?");
			}
		}
		else
		{
			strcpy(s, "Done");
		}
		CDogsTextStringAt(x, y, s);

		for (int j = 0; j < (int)hud->objectiveUpdates.size; j++)
		{
			HUDNumUpdate *update = CArrayGet(&hud->objectiveUpdates, j);
			if (update->Index == i)
			{
				DrawNumUpdate(update, "%d", o->done, Vec2iNew(x, y), 0);
			}
		}

		x += 25;
	}
}
