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
#include "hud.h"

#include <assert.h>
#include <math.h>
#include <time.h>

#include "actors.h"
#include "ammo.h"
#include "automap.h"
#include "draw.h"
#include "drawtools.h"
#include "events.h"
#include "font.h"
#include "game_events.h"
#include "mission.h"
#include "pic_manager.h"


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

	FontOpts opts = FontOptsNew();
	opts.HAlign = ALIGN_END;
	opts.VAlign = ALIGN_END;
	opts.Area = gGraphicsDevice.cachedConfig.Res;
	opts.Pad = Vec2iNew(10, 5 + FontH());
	FontStrOpt(s, Vec2iZero(), opts);
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

	FontOpts opts = FontOptsNew();
	opts.VAlign = ALIGN_END;
	opts.Area = gGraphicsDevice.cachedConfig.Res;
	opts.Pad = Vec2iNew(10, 5 + FontH());
	FontStrOpt(s, Vec2iZero(), opts);
}

void HUDInit(
	HUD *hud,
	GraphicsDevice *device,
	struct MissionOptions *mission)
{
	memset(hud, 0, sizeof *hud);
	hud->mission = mission;
	strcpy(hud->message, "");
	hud->messageTicks = 0;
	hud->device = device;
	FPSCounterInit(&hud->fpsCounter);
	WallClockInit(&hud->clock);
	CArrayInit(&hud->objectiveUpdates, sizeof(HUDNumUpdate));
	CArrayResize(&hud->objectiveUpdates, mission->Objectives.size, NULL);
	CArrayFillZero(&hud->objectiveUpdates);
	hud->showExit = false;
}
void HUDTerminate(HUD *hud)
{
	CArrayTerminate(&hud->objectiveUpdates);
}

void HUDDisplayMessage(HUD *hud, const char *msg, int ticks)
{
	strcpy(hud->message, msg);
	hud->messageTicks = ticks;
}

static int FindLocalPlayerIndex(const int playerUID)
{
	const PlayerData *p = PlayerDataGetByUID(playerUID);
	if (p == NULL || !p->IsLocal)
	{
		// This update was for a non-local player; abort
		return -1;
	}
	// Note: player UIDs divided by MAX_LOCAL_PLAYERS per client
	return playerUID % MAX_LOCAL_PLAYERS;
}

static void MergeUpdates(HUDNumUpdate *dst, const HUDNumUpdate src);
void HUDAddUpdate(
	HUD *hud, const HUDNumUpdateType type, const int idx, const int amount)
{
	HUDNumUpdate s;
	memset(&s, 0, sizeof s);

	// Index
	int localPlayerIdx = -1;
	switch (type)
	{
	case NUMBER_UPDATE_SCORE:
	case NUMBER_UPDATE_HEALTH:	// fallthrough
	case NUMBER_UPDATE_AMMO:	// fallthrough
		localPlayerIdx = FindLocalPlayerIndex(idx);
		if (localPlayerIdx)
		{
			// This update was for a non-local player; abort
			return;
		}
		s.u.PlayerUID = idx;
		break;
	case NUMBER_UPDATE_OBJECTIVE:
		s.u.ObjectiveIndex = idx;
		break;
	default:
		CASSERT(false, "unknown HUD update type");
		break;
	}

	s.Amount = amount;

	// Timers
	switch (type)
	{
	case NUMBER_UPDATE_SCORE:
	case NUMBER_UPDATE_HEALTH:	// fallthrough
	case NUMBER_UPDATE_AMMO:	// fallthrough
		s.Timer = NUM_UPDATE_TIMER_MS;
		s.TimerMax = NUM_UPDATE_TIMER_MS;
		break;
	case NUMBER_UPDATE_OBJECTIVE:
		s.Timer = NUM_UPDATE_TIMER_OBJECTIVE_MS;
		s.TimerMax = NUM_UPDATE_TIMER_OBJECTIVE_MS;
		break;
	default:
		CASSERT(false, "unknown HUD update type");
		break;
	}

	// Merge with existing updates
	switch (type)
	{
	case NUMBER_UPDATE_SCORE:
		MergeUpdates(&hud->scoreUpdates[localPlayerIdx], s);
		break;
	case NUMBER_UPDATE_HEALTH:
		MergeUpdates(&hud->healthUpdates[localPlayerIdx], s);
		break;
	case NUMBER_UPDATE_AMMO:
		MergeUpdates(&hud->ammoUpdates[localPlayerIdx], s);
		break;
	case NUMBER_UPDATE_OBJECTIVE:
		MergeUpdates(CArrayGet(&hud->objectiveUpdates, s.u.ObjectiveIndex), s);
		break;
	default:
		CASSERT(false, "unknown HUD update type");
		break;
	}
}
static void MergeUpdates(HUDNumUpdate *dst, const HUDNumUpdate src)
{
	// Combine update amounts
	if (dst->Timer <= 0)
	{
		// Old update finished; simply replace with new
		dst->Amount = src.Amount;
	}
	else
	{
		// Add the updates
		dst->Amount += src.Amount;
	}
	dst->Timer = src.Timer;
	dst->TimerMax = src.TimerMax;
}

static void NumUpdate(HUDNumUpdate *update, const int ms);
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
	for (int i = 0; i < MAX_LOCAL_PLAYERS; i++)
	{
		NumUpdate(&hud->scoreUpdates[i], ms);
		NumUpdate(&hud->healthUpdates[i], ms);
		NumUpdate(&hud->ammoUpdates[i], ms);
	}
	for (int i = 0; i < (int)hud->objectiveUpdates.size; i++)
	{
		NumUpdate(CArrayGet(&hud->objectiveUpdates, i), ms);
	}
}
static void NumUpdate(HUDNumUpdate *update, const int ms)
{
	if (update->Timer > 0)
	{
		update->Timer -= ms;
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
	const FontAlign hAlign, const FontAlign vAlign)
{
	Vec2i offset = Vec2iUnit();
	Vec2i barPos = Vec2iAdd(pos, offset);
	Vec2i barSize = Vec2iNew(MAX(0, innerWidth - 2), size.y - 2);
	if (hAlign == ALIGN_END)
	{
		int w = device->cachedConfig.Res.x;
		pos.x = w - pos.x - size.x - offset.x;
		barPos.x = w - barPos.x - barSize.x - offset.x;
	}
	if (vAlign == ALIGN_END)
	{
		int h = device->cachedConfig.Res.y;
		pos.y = h - pos.y - size.y - offset.y;
		barPos.y = h - barPos.y - barSize.y - offset.y;
	}
	DrawRectangle(device, pos, size, backColor, DRAW_FLAG_ROUNDED);
	DrawRectangle(device, barPos, barSize, barColor, 0);
}

#define GAUGE_WIDTH 60
#define GUN_ICON_PAD 10
static void DrawWeaponStatus(
	HUD *hud, const TActor *actor, Vec2i pos,
	const FontAlign hAlign, const FontAlign vAlign)
{
	const Weapon *weapon = ActorGetGun(actor);

	// Draw gun icon, and allocate padding to draw the gun icon
	const GunDescription *g = ActorGetGun(actor)->Gun;
	const Vec2i iconPos = Vec2iAligned(
		Vec2iNew(pos.x - 2, pos.y - 2),
		g->Icon->size, hAlign, vAlign, gGraphicsDevice.cachedConfig.Res);
	Blit(&gGraphicsDevice, g->Icon, iconPos);

	// don't draw gauge if not reloading
	if (weapon->lock > 0)
	{
		const Vec2i gaugePos = Vec2iAdd(pos, Vec2iNew(-1 + GUN_ICON_PAD, -1));
		const Vec2i size = Vec2iNew(GAUGE_WIDTH - GUN_ICON_PAD, FontH() + 2);
		const color_t barColor = { 0, 0, 255, 255 };
		const int maxLock = weapon->Gun->Lock;
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
			hud->device, gaugePos, size, innerWidth, barColor, backColor,
			hAlign, vAlign);
	}
	FontOpts opts = FontOptsNew();
	opts.HAlign = hAlign;
	opts.VAlign = vAlign;
	opts.Area = gGraphicsDevice.cachedConfig.Res;
	opts.Pad = Vec2iNew(pos.x + GUN_ICON_PAD, pos.y);
	char buf[128];
	if (ConfigGetBool(&gConfig, "Game.Ammo") && weapon->Gun->AmmoId >= 0)
	{
		// Include ammo counter
		sprintf(buf, "%s %d/%d",
			weapon->Gun->name,
			ActorGunGetAmmo(actor, weapon),
			AmmoGetById(&gAmmo, weapon->Gun->AmmoId)->Max);
	}
	else
	{
		strcpy(buf, weapon->Gun->name);
	}
	FontStrOpt(buf, Vec2iZero(), opts);
}

static void DrawHealth(
	GraphicsDevice *device, const TActor *actor, const Vec2i pos,
	const FontAlign hAlign, const FontAlign vAlign)
{
	char s[50];
	Vec2i gaugePos = Vec2iAdd(pos, Vec2iNew(-1, -1));
	Vec2i size = Vec2iNew(GAUGE_WIDTH, FontH() + 2);
	HSV hsv = { 0.0, 1.0, 1.0 };
	color_t barColor;
	int health = actor->health;
	const int maxHealth = ActorGetCharacter(actor)->maxHealth;
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
		device, gaugePos, size, innerWidth, barColor, backColor,
		hAlign, vAlign);
	sprintf(s, "%d", health);

	FontOpts opts = FontOptsNew();
	opts.HAlign = hAlign;
	opts.VAlign = vAlign;
	opts.Area = gGraphicsDevice.cachedConfig.Res;
	opts.Pad = pos;
	FontStrOpt(s, Vec2iZero(), opts);
}

static void DrawLives(
	const GraphicsDevice *device, const PlayerData *player, const Vec2i pos,
	const FontAlign hAlign, const FontAlign vAlign)
{
	const int xStep = hAlign == ALIGN_START ? 10 : -10;
	const Vec2i offset = Vec2iNew(5, 20);
	Vec2i drawPos = Vec2iAdd(pos, offset);
	if (hAlign == ALIGN_END)
	{
		const int w = device->cachedConfig.Res.x;
		drawPos.x = w - drawPos.x - offset.x;
	}
	if (vAlign == ALIGN_END)
	{
		const int h = device->cachedConfig.Res.y;
		drawPos.y = h - drawPos.y + offset.y + 5;
	}
	const TOffsetPic head = GetHeadPic(
		BODY_ARMED, DIRECTION_DOWN, player->Char.looks.Face, STATE_IDLE);
	for (int i = 0; i < player->Lives; i++)
	{
		BlitOld(
			drawPos.x + head.dx, drawPos.y + head.dy,
			PicManagerGetOldPic(&gPicManager, head.picIndex),
			&player->Char.table, BLIT_TRANSPARENT);
		drawPos.x += xStep;
	}
}

#define HUDFLAGS_PLACE_RIGHT	0x01
#define HUDFLAGS_PLACE_BOTTOM	0x02
#define HUDFLAGS_HALF_SCREEN	0x04
#define HUDFLAGS_QUARTER_SCREEN	0x08
#define HUDFLAGS_SHARE_SCREEN	0x10

#define AUTOMAP_PADDING	5
#define AUTOMAP_SIZE	45
static void DrawRadar(
	GraphicsDevice *device, const TActor *p,
	const int scale, const int flags, const bool showExit)
{
	Vec2i pos = Vec2iZero();
	int w = device->cachedConfig.Res.x;
	int h = device->cachedConfig.Res.y;

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

	if (!Vec2iIsZero(pos))
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

static void DrawSharedRadar(GraphicsDevice *device, int scale, bool showExit)
{
	int w = device->cachedConfig.Res.x;
	Vec2i pos = Vec2iNew(w / 2 - AUTOMAP_SIZE / 2, AUTOMAP_PADDING);
	Vec2i playerMidpoint = PlayersGetMidpoint();
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

#define LIVES_ROW_EXTRA_Y 6

static void DrawObjectiveCompass(
	GraphicsDevice *g, Vec2i playerPos, Rect2i r, bool showExit);
// Draw player's score, health etc.
static void DrawPlayerStatus(
	HUD *hud, const PlayerData *data, const TActor *p,
	const int flags, const Rect2i r)
{
	if (p != NULL)
	{
		DrawObjectiveCompass(
			hud->device, Vec2iFull2Real(p->Pos), r, hud->showExit);
	}

	Vec2i pos = Vec2iNew(5, 5);

	FontOpts opts = FontOptsNew();
	if (flags & HUDFLAGS_PLACE_RIGHT)
	{
		opts.HAlign = ALIGN_END;
	}
	if (flags & HUDFLAGS_PLACE_BOTTOM)
	{
		opts.VAlign = ALIGN_END;
		pos.y += BOTTOM_PADDING;
	}
	opts.Area = gGraphicsDevice.cachedConfig.Res;
	opts.Pad = pos;
	FontStrOpt(data->name, Vec2iZero(), opts);

	const int rowHeight = 1 + FontH();
	pos.y += rowHeight;
	char s[50];
	if (IsScoreNeeded(gCampaign.Entry.Mode))
	{
		if (ConfigGetBool(&gConfig, "Game.Ammo"))
		{
			// Display money instead of ammo
			sprintf(s, "Cash: $%d", data->score);
		}
		else
		{
			sprintf(s, "Score: %d", data->score);
		}
	}
	else
	{
		s[0] = 0;
	}
	if (p)
	{
		// Score/money
		opts.Pad = pos;
		FontStrOpt(s, Vec2iZero(), opts);

		// Health
		pos.y += rowHeight;
		DrawHealth(hud->device, p, pos, opts.HAlign, opts.VAlign);

		// Lives
		pos.y += rowHeight;
		DrawLives(hud->device, data, pos, opts.HAlign, opts.VAlign);

		// Weapon
		pos.y += rowHeight + LIVES_ROW_EXTRA_Y;
		DrawWeaponStatus(hud, p, pos, opts.HAlign, opts.VAlign);
	}
	else
	{
		opts.Pad = pos;
		FontStrOpt(s, Vec2iZero(), opts);
	}

	if (ConfigGetBool(&gConfig, "Interface.ShowHUDMap") &&
		!(flags & HUDFLAGS_SHARE_SCREEN) &&
		IsAutoMapEnabled(gCampaign.Entry.Mode))
	{
		DrawRadar(hud->device, p, RADAR_SCALE, flags, hud->showExit);
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
			for (int i = 0; i < (int)tile->things.size; i++)
			{
				TTileItem *ti =
					ThingIdGetTileItem(CArrayGet(&tile->things, i));
				if (!(ti->flags & TILEITEM_OBJECTIVE))
				{
					continue;
				}
				int objective = ObjectiveFromTileItem(ti->flags);
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
				const ObjectiveDef *o =
					CArrayGet(&gMission.Objectives, objective);
				color_t color = o->color;
				DrawCompassArrow(
					g, r, Vec2iNew(ti->x, ti->y), playerPos, color, NULL);
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
				const Pic *p = PicManagerGetPic(&gPicManager, "arrow_right");
				Vec2i drawPos = Vec2iNew(
					textPos.x - p->size.x, textPos.y - p->size.y / 2);
				BlitMasked(g, p, drawPos, mask, true);
			}
			else if (compassV.x < 0)
			{
				// left edge
				textPos = Vec2iNew(r.Pos.x, r.Pos.y + r.Size.y / 2 + yInt);
				const Pic *p = PicManagerGetPic(&gPicManager, "arrow_left");
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
			if (compassV.y > 0)
			{
				// bottom edge
				textPos = Vec2iNew(
					r.Pos.x + r.Size.x / 2 + xInt, r.Pos.y + r.Size.y);
				const Pic *p = PicManagerGetPic(&gPicManager, "arrow_down");
				Vec2i drawPos = Vec2iNew(
					textPos.x - p->size.x / 2, textPos.y - p->size.y);
				BlitMasked(g, p, drawPos, mask, true);
			}
			else if (compassV.y < 0)
			{
				// top edge
				textPos = Vec2iNew(r.Pos.x + r.Size.x / 2 + xInt, r.Pos.y);
				const Pic *p = PicManagerGetPic(&gPicManager, "arrow_up");
				Vec2i drawPos = Vec2iNew(textPos.x - p->size.x / 2, textPos.y);
				BlitMasked(g, p, drawPos, mask, true);
			}
		}
	}
	if (label && strlen(label) > 0)
	{
		Vec2i textSize = FontStrSize(label);
		// Center the text around the target position
		textPos.x -= textSize.x / 2;
		textPos.y -= textSize.y / 2;
		// Make sure the text is inside the screen
		int padding = 8;
		textPos.x = MAX(textPos.x, r.Pos.x + padding);
		textPos.x = MIN(textPos.x, r.Pos.x + r.Size.x - textSize.x - padding);
		textPos.y = MAX(textPos.y, r.Pos.y + padding);
		textPos.y = MIN(textPos.y, r.Pos.y + r.Size.y - textSize.y - padding);
		FontStrMask(label, textPos, mask);
	}
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
		if (hud->mission->KeyFlags & keyFlags[i])
		{
			const Pic *pic = KeyPickupClass(hud->mission->keyStyle, i)->Pic;
			Blit(
				&gGraphicsDevice,
				pic,
				Vec2iNew(CenterX(pic->size.x) - xOffset, yOffset));
		}
		xOffset += xOffsetIncr;
	}
}

static void DrawScoreUpdate(const HUDNumUpdate *u, const int flags);
static void DrawHealthUpdate(const HUDNumUpdate *u, const int flags);
static void DrawAmmoUpdate(const HUDNumUpdate *u, const int flags);
static void DrawObjectiveCounts(HUD *hud);
void HUDDraw(HUD *hud, const input_device_e pausingDevice)
{
	char s[50];
	int flags = 0;
	const int numPlayersAlive =
		GetNumPlayers(PLAYER_ALIVE_OR_DYING, false, false);
	const int numLocalPlayers = GetNumPlayers(PLAYER_ANY, false, true);
	const int numLocalPlayersAlive =
		GetNumPlayers(PLAYER_ALIVE_OR_DYING, false, true);

	Rect2i r;
	r.Size = Vec2iNew(
		hud->device->cachedConfig.Res.x,
		hud->device->cachedConfig.Res.y);
	if (numLocalPlayersAlive <= 1)
	{
		flags = 0;
	}
	else if (
		ConfigGetEnum(&gConfig, "Interface.Splitscreen") == SPLITSCREEN_NEVER)
	{
		flags |= HUDFLAGS_SHARE_SCREEN;
	}
	else if (numLocalPlayers == 2)
	{
		r.Size.x /= 2;
		flags |= HUDFLAGS_HALF_SCREEN;
	}
	else if (numLocalPlayers == 3 || numLocalPlayers == 4)
	{
		r.Size.x /= 2;
		r.Size.y /= 2;
		flags |= HUDFLAGS_QUARTER_SCREEN;
	}
	else
	{
		assert(0 && "not implemented");
	}

	int idx = 0;
	for (int i = 0; i < (int)gPlayerDatas.size; i++, idx++)
	{
		const PlayerData *p = CArrayGet(&gPlayerDatas, i);
		if (!p->IsLocal)
		{
			idx--;
			continue;
		}
		int drawFlags = flags;
		r.Pos = Vec2iZero();
		if (idx & 1)
		{
			r.Pos.x = r.Size.x;
			drawFlags |= HUDFLAGS_PLACE_RIGHT;
		}
		if (idx >= 2)
		{
			r.Pos.y = r.Size.y;
			drawFlags |= HUDFLAGS_PLACE_BOTTOM;
		}
		TActor *player = NULL;
		if (IsPlayerAlive(p))
		{
			player = ActorGetByUID(p->ActorUID);
		}
		DrawPlayerStatus(hud, p, player, drawFlags, r);
		DrawScoreUpdate(&hud->scoreUpdates[idx], drawFlags);
		DrawHealthUpdate(&hud->healthUpdates[idx], drawFlags);
		DrawAmmoUpdate(&hud->ammoUpdates[idx], drawFlags);
	}
	// Only draw radar once if shared
	if (ConfigGetBool(&gConfig, "Interface.ShowHUDMap") &&
		(flags & HUDFLAGS_SHARE_SCREEN) &&
		IsAutoMapEnabled(gCampaign.Entry.Mode))
	{
		DrawSharedRadar(hud->device, RADAR_SCALE, hud->showExit);
	}

	if (numPlayersAlive == 0)
	{
		if (AreAllPlayersDeadAndNoLives())
		{
			if (!IsPVP(gCampaign.Entry.Mode))
			{
				FontStrCenter("Game Over!");
			}
			else
			{
				FontStrCenter("All Kill!");
			}
		}
	}
	else if (hud->mission->state == MISSION_STATE_PICKUP)
	{
		int timeLeft = gMission.pickupTime + PICKUP_LIMIT - gMission.time;
		sprintf(s, "Pickup in %d seconds\n",
			(timeLeft + (FPS_FRAMELIMIT - 1)) / FPS_FRAMELIMIT);
		FontStrCenter(s);
	}

	if (pausingDevice != INPUT_DEVICE_UNSET)
	{
		Vec2i pos = Vec2iScaleDiv(Vec2iMinus(
			gGraphicsDevice.cachedConfig.Res,
			FontStrSize("Foo\nPress foo or bar to unpause\nBaz")), 2);
		const int x = pos.x;
		FontStr("<Paused>", pos);

		pos.y += FontH();
		pos = FontStr("Press ", pos);
		color_t c = colorWhite;
		const char *buttonName =
			InputGetButtonNameColor(pausingDevice, 0, CMD_ESC, &c);
		pos = FontStrMask(buttonName, pos, c);
		FontStr(" again to quit", pos);

		pos.x = x;
		pos.y += FontH();
		pos = FontStr("Press ", pos);
		buttonName = InputGetButtonNameColor(
			pausingDevice, 0, CMD_BUTTON1, &c);
		pos = FontStrMask(buttonName, pos, c);
		pos = FontStr(" or ", pos);
		buttonName = InputGetButtonNameColor(
			pausingDevice, 0, CMD_BUTTON2, &c);
		pos = FontStrMask(buttonName, pos, c);
		FontStr(" to unpause", pos);
	}

	if (hud->messageTicks > 0 || hud->messageTicks == -1)
	{
		// Draw the message centered, and just below the automap
		Vec2i pos = Vec2iNew(
			(hud->device->cachedConfig.Res.x -
			FontStrW(hud->message)) / 2,
			AUTOMAP_SIZE + AUTOMAP_PADDING + AUTOMAP_PADDING);
		FontStrMask(hud->message, pos, colorCyan);
	}

	if (ConfigGetBool(&gConfig, "Interface.ShowFPS"))
	{
		FPSCounterDraw(&hud->fpsCounter);
	}
	if (ConfigGetBool(&gConfig, "Interface.ShowTime"))
	{
		WallClockDraw(&hud->clock);
	}

	DrawKeycards(hud);

	// Draw elapsed mission time as MM:SS
	int missionTimeSeconds = gMission.time / FPS_FRAMELIMIT;
	sprintf(s, "%d:%02d",
		missionTimeSeconds / 60, missionTimeSeconds % 60);

	FontOpts opts = FontOptsNew();
	opts.HAlign = ALIGN_CENTER;
	opts.Area = hud->device->cachedConfig.Res;
	opts.Pad.y = 5;
	FontStrOpt(s, Vec2iZero(), opts);

	if (HasObjectives(gCampaign.Entry.Mode))
	{
		DrawObjectiveCounts(hud);
	}
}

static void DrawNumUpdate(
	const HUDNumUpdate *update,
	const char *formatText, int currentValue, Vec2i pos, int flags);
static void DrawScoreUpdate(const HUDNumUpdate *u, const int flags)
{
	if (!IsScoreNeeded(gCampaign.Entry.Mode))
	{
		return;
	}
	const PlayerData *p = PlayerDataGetByUID(u->u.PlayerUID);
	if (!IsPlayerAlive(p)) return;
	const int rowHeight = 1 + FontH();
	const int y = 5 + rowHeight;
	DrawNumUpdate(u, "Score: %d", p->score, Vec2iNew(5, y), flags);
}
static void DrawHealthUpdate(const HUDNumUpdate *u, const int flags)
{
	const PlayerData *p = PlayerDataGetByUID(u->u.PlayerUID);
	if (!IsPlayerAlive(p)) return;
	const int rowHeight = 1 + FontH();
	const int y = 5 + rowHeight * 2;
	const TActor *a = ActorGetByUID(p->ActorUID);
	DrawNumUpdate(u, "%d", a->health, Vec2iNew(5, y), flags);
}
static void DrawAmmoUpdate(const HUDNumUpdate *u, const int flags)
{
	const PlayerData *p = PlayerDataGetByUID(u->u.PlayerUID);
	if (!IsPlayerAlive(p)) return;
	const int rowHeight = 1 + FontH();
	const int y = 5 + rowHeight * 4 + LIVES_ROW_EXTRA_Y;
	const TActor *a = ActorGetByUID(p->ActorUID);
	const Weapon *w = ActorGetGun(a);
	char gunNameBuf[256];
	sprintf(gunNameBuf, "%s %%d", w->Gun->name);
	const int ammo = ActorGunGetAmmo(a, w);
	DrawNumUpdate(u, gunNameBuf, ammo, Vec2iNew(5 + GUN_ICON_PAD, y), flags);
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
	const HUDNumUpdate *update,
	const char *formatText, int currentValue, Vec2i pos, int flags)
{
	if (update->Timer <= 0 || update->Amount == 0)
	{
		return;
	}
	color_t color = update->Amount > 0 ? colorGreen : colorRed;

	char s[50];
	if (!(flags & HUDFLAGS_PLACE_RIGHT))
	{
		// Find the right position to draw the update
		// Make sure the update is displayed lined up with the lowest digits
		// Find the position of where the normal text is displayed,
		// and move to its right
		sprintf(s, formatText, currentValue);
		pos.x += FontStrW(s);
		// Then find the size of the update, and move left
		sprintf(s, "%s%d", update->Amount > 0 ? "+" : "", update->Amount);
		pos.x -= FontStrW(s);
		// The final position should ensure the score update's lowest digit
		// lines up with the normal score's lowest digit
	}
	else
	{
		sprintf(s, "%s%d", update->Amount > 0 ? "+" : "", update->Amount);
	}

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

	FontOpts opts = FontOptsNew();
	if (flags & HUDFLAGS_PLACE_RIGHT)
	{
		opts.HAlign = ALIGN_END;
	}
	if (flags & HUDFLAGS_PLACE_BOTTOM)
	{
		opts.VAlign = ALIGN_END;
		pos.y += BOTTOM_PADDING;
	}
	opts.Area = gGraphicsDevice.cachedConfig.Res;
	opts.Pad = pos;
	opts.Mask = color;
	opts.Blend = true;
	FontStrOpt(s, Vec2iZero(), opts);
}

static void DrawObjectiveCounts(HUD *hud)
{
	int x = 5 + GAUGE_WIDTH;
	int y = hud->device->cachedConfig.Res.y - 5 - FontH();
	for (int i = 0; i < (int)gMission.missionData->Objectives.size; i++)
	{
		MissionObjective *mo = CArrayGet(&gMission.missionData->Objectives, i);
		const ObjectiveDef *o = CArrayGet(&gMission.Objectives, i);

		// Don't draw anything for optional objectives
		if (mo->Required == 0)
		{
			continue;
		}

		// Objective color dot
		Draw_Rect(x, y + 3, 2, 2, o->color);

		x += 5;
		char s[32];
		int itemsLeft = mo->Required - o->done;
		if (itemsLeft > 0)
		{
			if (!(mo->Flags & OBJECTIVE_UNKNOWNCOUNT))
			{
				sprintf(s, "%s: %d", ObjectiveTypeStr(mo->Type), itemsLeft);
			}
			else
			{
				sprintf(s, "%s: ?", ObjectiveTypeStr(mo->Type));
			}
		}
		else
		{
			strcpy(s, "Done");
		}
		FontStr(s, Vec2iNew(x, y));

		DrawNumUpdate(
			CArrayGet(&hud->objectiveUpdates, i), "%d", o->done,
			Vec2iNew(x + FontStrW(s) - 8, y), 0);

		x += 40;
	}
}
