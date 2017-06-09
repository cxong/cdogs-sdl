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
#include "hud.h"

#include <assert.h>
#include <math.h>

#include "actors.h"
#include "ammo.h"
#include "automap.h"
#include "draw/draw.h"
#include "draw/draw_actor.h"
#include "draw/drawtools.h"
#include "events.h"
#include "font.h"
#include "game_events.h"
#include "hud_defs.h"
#include "mission.h"
#include "pic_manager.h"


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
	HUDNumPopupsInit(&hud->numPopups, mission);
	hud->showExit = false;
}
void HUDTerminate(HUD *hud)
{
	HUDNumPopupsTerminate(&hud->numPopups);
}

void HUDDisplayMessage(HUD *hud, const char *msg, int ticks)
{
	strcpy(hud->message, msg);
	hud->messageTicks = ticks;
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
	HUDPopupsUpdate(&hud->numPopups, ms);
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
		pos.x = w - pos.x - size.x;
		barPos.x = w - barPos.x - barSize.x;
	}
	if (vAlign == ALIGN_END)
	{
		int h = device->cachedConfig.Res.y;
		pos.y = h - pos.y - size.y;
		barPos.y = h - barPos.y - barSize.y;
	}
	DrawRectangle(device, pos, size, backColor, DRAW_FLAG_ROUNDED);
	DrawRectangle(device, barPos, barSize, barColor, 0);
}

#define GAUGE_WIDTH 60
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
		const int maxLock = weapon->Gun->Lock;
		color_t barColor;
		const double reloadProgressColorMod = 0.5 +
			0.5 * (weapon->lock / (double) maxLock);
		HSV hsv = { 0.0, 1.0, reloadProgressColorMod };
		barColor = ColorTint(colorWhite, hsv);
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
	int lastHealth = actor->lastHealth;
	const int maxHealth = ActorGetCharacter(actor)->maxHealth;
	int innerWidthLastHealth;
	int innerWidthCurrentHealth;
	int innerWidth;
	color_t backColor = { 50, 0, 0, 255 };
	innerWidthLastHealth = MAX(1, size.x * lastHealth / maxHealth);
	innerWidthCurrentHealth = MAX(1, size.x * health / maxHealth);
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
	if (lastHealth > health)
	{
		barColor = colorRed;
		DrawGauge(
				device, gaugePos, size, innerWidthLastHealth, barColor,
				backColor, hAlign, vAlign);
		backColor.a = 0;
	}
	else if (lastHealth < health)
	{
		barColor = colorGreen;
		DrawGauge(
				device, gaugePos, size, innerWidthCurrentHealth, barColor,
				backColor, hAlign, vAlign);
		backColor.a = 0;
	}
	lastHealth < health ? innerWidth = innerWidthLastHealth:
		(innerWidth = innerWidthCurrentHealth); 
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
	const Vec2i offset = Vec2iNew(2, 5);
	Vec2i drawPos = Vec2iAdd(pos, offset);
	if (hAlign == ALIGN_END)
	{
		const int w = device->cachedConfig.Res.x;
		drawPos.x = w - drawPos.x - offset.x;
	}
	if (vAlign == ALIGN_END)
	{
		const int h = device->cachedConfig.Res.y;
		drawPos.y = h - drawPos.y - 1;
	}
	for (int i = 0; i < player->Lives; i++)
	{
		DrawHead(&player->Char, DIRECTION_DOWN, drawPos);
		drawPos.x += xStep;
	}
}

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
			sprintf(s, "Cash: $%d", data->Stats.Score);
		}
		else
		{
			sprintf(s, "Score: %d", data->Stats.Score);
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
			CA_FOREACH(ThingId, tid, tile->things)
				TTileItem *ti = ThingIdGetTileItem(tid);
				if (!(ti->flags & TILEITEM_OBJECTIVE))
				{
					continue;
				}
				const int objective = ObjectiveFromTileItem(ti->flags);
				const Objective *o =
					CArrayGet(&gMission.missionData->Objectives, objective);
				if (o->Flags & OBJECTIVE_HIDDEN)
				{
					continue;
				}
				if (!(o->Flags & OBJECTIVE_POSKNOWN) && !tile->isVisited)
				{
					continue;
				}
				DrawCompassArrow(
					g, r, Vec2iNew(ti->x, ti->y), playerPos, o->color, NULL);
			CA_FOREACH_END()
		}
	}
}

#define COMP_SATURATE_DIST 350
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
	// Saturate according to dist from screen edge
	int xDist = abs(pos.x - playerPos.x) - r.Size.x / 2;
	int yDist = abs(pos.y - playerPos.y) - r.Size.y / 2;
	int lDist;
	xDist > yDist ? lDist = xDist: (lDist = yDist);
	HSV hsv = { -1.0, 1.0,
		2.0 - 1.5 * MIN(lDist, COMP_SATURATE_DIST) / COMP_SATURATE_DIST };
	mask = ColorTint(mask, hsv);
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

static void DrawPlayerAreas(HUD *hud);
static void DrawDeathmatchScores(HUD *hud);
static void DrawStateMessage(
	HUD *hud, const input_device_e pausingDevice,
	const bool controllerUnplugged);
static void DrawHUDMessage(HUD *hud);
static void DrawKeycards(HUD *hud);
static void DrawMissionTime(HUD *hud);
static void DrawObjectiveCounts(HUD *hud);
void HUDDraw(
	HUD *hud, const input_device_e pausingDevice,
	const bool controllerUnplugged)
{
	if (ConfigGetBool(&gConfig, "Graphics.ShowHUD"))
	{
		DrawPlayerAreas(hud);
		DrawDeathmatchScores(hud);
		DrawHUDMessage(hud);
		if (ConfigGetBool(&gConfig, "Interface.ShowFPS"))
		{
			FPSCounterDraw(&hud->fpsCounter);
		}
		if (ConfigGetBool(&gConfig, "Interface.ShowTime"))
		{
			WallClockDraw(&hud->clock);
		}
		DrawKeycards(hud);
		DrawMissionTime(hud);
		if (HasObjectives(gCampaign.Entry.Mode))
		{
			DrawObjectiveCounts(hud);
		}
	}

	DrawStateMessage(hud, pausingDevice, controllerUnplugged);
}

static void DrawPlayerAreas(HUD *hud)
{
	int flags = 0;
	const int numPlayersCreen = GetNumPlayersScreen(NULL);

	Rect2i r;
	r.Size = Vec2iNew(
		hud->device->cachedConfig.Res.x,
		hud->device->cachedConfig.Res.y);
	if (numPlayersCreen <= 1)
	{
		flags = 0;
	}
	else if (
		ConfigGetEnum(&gConfig, "Interface.Splitscreen") == SPLITSCREEN_NEVER)
	{
		flags |= HUDFLAGS_SHARE_SCREEN;
	}
	else if (numPlayersCreen == 2)
	{
		r.Size.x /= 2;
		flags |= HUDFLAGS_HALF_SCREEN;
	}
	else if (numPlayersCreen == 3 || numPlayersCreen == 4)
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
		if (!IsPlayerScreen(p))
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
		HUDNumPopupsDrawPlayer(&hud->numPopups, idx, drawFlags);
	}

	// Only draw radar once if shared
	if (ConfigGetBool(&gConfig, "Interface.ShowHUDMap") &&
		(flags & HUDFLAGS_SHARE_SCREEN) &&
		IsAutoMapEnabled(gCampaign.Entry.Mode))
	{
		DrawSharedRadar(hud->device, RADAR_SCALE, hud->showExit);
	}
}

static void DrawDeathmatchScores(HUD *hud)
{
	// Only draw deathmatch scores if single screen and non-local players exist
	if (gCampaign.Entry.Mode != GAME_MODE_DEATHMATCH ||
		GetNumPlayersScreen(NULL) != 1 ||
		GetNumPlayers(PLAYER_ANY, false, false) <= 1)
	{
		return;
	}
	FontOpts opts = FontOptsNew();
	opts.Area = hud->device->cachedConfig.Res;
	opts.HAlign = ALIGN_END;
	opts.Mask = colorPurple;
	const int nameColumn = 45;
	const int livesColumn = 25;
	const int killsColumn = 5;
	int y = 5;
	opts.Pad.x = nameColumn;
	FontStrOpt("Player", Vec2iNew(0, y), opts);
	opts.Pad.x = livesColumn;
	FontStrOpt("Lives", Vec2iNew(0, y), opts);
	opts.Pad.x = killsColumn;
	FontStrOpt("Kills", Vec2iNew(0, y), opts);
	y += FontH();
	// Find the player(s) with the most lives and kills
	int maxLives = 0;
	int maxKills = 0;
	CA_FOREACH(const PlayerData, p, gPlayerDatas)
		maxLives = MAX(maxLives, p->Lives);
		maxKills = MAX(maxKills, p->Stats.Kills);
	CA_FOREACH_END()
	CA_FOREACH(const PlayerData, p, gPlayerDatas)
		// Player name; red if dead
		opts.Mask = p->Lives > 0 ? colorWhite : colorRed;
		opts.Pad.x = nameColumn;
		FontStrOpt(p->name, Vec2iNew(0, y), opts);

		// lives; cyan if most lives
		opts.Mask = p->Lives == maxLives ? colorCyan : colorWhite;
		opts.Pad.x = livesColumn;
		char buf[32];
		sprintf(buf, "%d", p->Lives);
		FontStrOpt(buf, Vec2iNew(0, y), opts);

		// kills; cyan if most kills
		opts.Mask = p->Stats.Kills == maxKills ? colorCyan : colorWhite;
		opts.Pad.x = killsColumn;
		sprintf(buf, "%d", p->Stats.Kills);
		FontStrOpt(buf, Vec2iNew(0, y), opts);
		y += FontH();
	CA_FOREACH_END()
}

static void DrawMissionState(HUD *hud);
static void DrawStateMessage(
	HUD *hud, const input_device_e pausingDevice,
	const bool controllerUnplugged)
{
	if (controllerUnplugged)
	{
		Vec2i pos = Vec2iScaleDiv(Vec2iMinus(
			gGraphicsDevice.cachedConfig.Res,
			FontStrSize("<Paused>\nFoobar\nPlease reconnect controller")), 2);
		const int x = pos.x;
		FontStr("<Paused>", pos);

		pos.y += FontH();
		pos = FontStr("Press ", pos);
		char buf[256];
		color_t c = colorWhite;
		InputGetButtonNameColor(pausingDevice, 0, CMD_ESC, buf, &c);
		pos = FontStrMask(buf, pos, c);
		FontStr(" to quit", pos);

		pos.x = x;
		pos.y += FontH();
		FontStr("Please reconnect controller", pos);
	}
	else if (pausingDevice != INPUT_DEVICE_UNSET)
	{
		Vec2i pos = Vec2iScaleDiv(Vec2iMinus(
			gGraphicsDevice.cachedConfig.Res,
			FontStrSize("Foo\nPress foo or bar to unpause\nBaz")), 2);
		const int x = pos.x;
		FontStr("<Paused>", pos);

		pos.y += FontH();
		pos = FontStr("Press ", pos);
		char buf[256];
		color_t c = colorWhite;
		InputGetButtonNameColor(pausingDevice, 0, CMD_ESC, buf, &c);
		pos = FontStrMask(buf, pos, c);
		FontStr(" again to quit", pos);

		pos.x = x;
		pos.y += FontH();
		pos = FontStr("Press ", pos);
		c = colorWhite;
		InputGetButtonNameColor(pausingDevice, 0, CMD_BUTTON1, buf, &c);
		pos = FontStrMask(buf, pos, c);
		pos = FontStr(" or ", pos);
		c = colorWhite;
		InputGetButtonNameColor(pausingDevice, 0, CMD_BUTTON2, buf, &c);
		pos = FontStrMask(buf, pos, c);
		FontStr(" to unpause", pos);
	}
	else
	{
		DrawMissionState(hud);
	}
}
static void DrawMissionState(HUD *hud)
{
	char s[50];
	const int numPlayersAlive =
		GetNumPlayers(PLAYER_ALIVE_OR_DYING, false, false);

	switch (hud->mission->state)
	{
	case MISSION_STATE_WAITING:
		FontStrCenter("Waiting for players...");
		break;
	case MISSION_STATE_PLAY:
		if (numPlayersAlive == 0 && AreAllPlayersDeadAndNoLives())
		{
			if (gPlayerDatas.size == 0)
			{
				FontStrCenter("Waiting for players...");
			}
			else if (!IsPVP(gCampaign.Entry.Mode))
			{
				FontStrCenter("Game Over!");
			}
			else
			{
				FontStrCenter("All Kill!");
			}
		}
		else if (MissionNeedsMoreRescuesInExit(&gMission))
		{
			FontStrCenter("More rescues needed");
		}
		break;
	case MISSION_STATE_PICKUP:
	{
		int timeLeft = gMission.pickupTime + PICKUP_LIMIT - gMission.time;
		sprintf(s, "Pickup in %d seconds\n",
			(timeLeft + (FPS_FRAMELIMIT - 1)) / FPS_FRAMELIMIT);
		FontStrCenter(s);
	}
	break;
	default:
		CASSERT(false, "unknown mission state");
		break;
	}
}

static void DrawHUDMessage(HUD *hud)
{
	if (hud->messageTicks > 0 || hud->messageTicks == -1)
	{
		// Draw the message centered, and just below the automap
		Vec2i pos = Vec2iNew(
			(hud->device->cachedConfig.Res.x -
				FontStrW(hud->message)) / 2,
			AUTOMAP_SIZE + AUTOMAP_PADDING + AUTOMAP_PADDING);
		FontStrMask(hud->message, pos, colorCyan);
	}
}

static void DrawKeycards(HUD *hud)
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
			const Pic *pic = KeyPickupClass(
				hud->mission->missionData->KeyStyle, i)->Pic;
			Blit(
				&gGraphicsDevice,
				pic,
				Vec2iNew(CenterX(pic->size.x) - xOffset, yOffset));
		}
		xOffset += xOffsetIncr;
	}
}

static void DrawMissionTime(HUD *hud)
{
	char s[50];
	// Draw elapsed mission time as MM:SS
	int missionTimeSeconds = gMission.time / FPS_FRAMELIMIT;
	sprintf(s, "%d:%02d",
		missionTimeSeconds / 60, missionTimeSeconds % 60);

	FontOpts opts = FontOptsNew();
	opts.HAlign = ALIGN_CENTER;
	opts.Area = hud->device->cachedConfig.Res;
	opts.Pad.y = 5;
	FontStrOpt(s, Vec2iZero(), opts);
}

static void DrawObjectiveCounts(HUD *hud)
{
	int x = 5 + GAUGE_WIDTH;
	int y = hud->device->cachedConfig.Res.y - 5 - FontH();
	CA_FOREACH(const Objective, o, hud->mission->missionData->Objectives)
		// Don't draw anything for optional objectives
		if (!ObjectiveIsRequired(o))
		{
			continue;
		}

		// Objective color dot
		Draw_Rect(x, y + 3, 2, 2, o->color);

		x += 5;
		char s[32];
		const int itemsLeft = o->Required - o->done;
		if (itemsLeft > 0)
		{
			if (!(o->Flags & OBJECTIVE_UNKNOWNCOUNT))
			{
				sprintf(s, "%s: %d", ObjectiveTypeStr(o->Type), itemsLeft);
			}
			else
			{
				sprintf(s, "%s: ?", ObjectiveTypeStr(o->Type));
			}
		}
		else
		{
			strcpy(s, "Done");
		}
		FontStr(s, Vec2iNew(x, y));

		HUDNumPopupsDrawObjective(
			&hud->numPopups, _ca_index, Vec2iNew(x + FontStrW(s) - 8, y));

		x += 40;
	CA_FOREACH_END()
}
