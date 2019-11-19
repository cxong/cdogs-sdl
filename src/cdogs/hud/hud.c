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
#include "player.h"
#include "player_hud.h"


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
	for (int i = 0; i < MAX_LOCAL_PLAYERS; i++)
	{
		HUDPlayerInit(&hud->hudPlayers[i]);
	}
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

void HUDUpdate(HUD *hud, const int ms)
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

	for (int i = 0; i < hud->DrawData.NumScreens; i++)
	{
		const PlayerData *p = hud->DrawData.Players[i];
		HUDPlayerUpdate(&hud->hudPlayers[i], p, ms);
	}
}

HUDDrawData HUDGetDrawData(void)
{
	HUDDrawData drawData;
	memset(&drawData, 0, sizeof drawData);
	CA_FOREACH(const PlayerData, p, gPlayerDatas)
		if (!IsPlayerScreen(p))
		{
			continue;
		}
		drawData.Players[drawData.NumScreens] = p;
		drawData.NumScreens++;
	}
	return drawData;
}


static void DrawSharedRadar(GraphicsDevice *device, bool showExit)
{
	int w = device->cachedConfig.Res.x;
	struct vec2i pos = svec2i(w / 2 - AUTOMAP_SIZE / 2, AUTOMAP_PADDING);
	const struct vec2i playerMidpoint = Vec2ToTile(PlayersGetMidpoint());
	AutomapDrawRegion(
		device->gameWindow.renderer,
		&gMap,
		pos,
		svec2i(AUTOMAP_SIZE, AUTOMAP_SIZE),
		playerMidpoint,
		AUTOMAP_FLAGS_MASK,
		showExit);
}

static void DrawPlayerAreas(HUD *hud, const int numViews);
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
	const bool controllerUnplugged, const int numViews)
{
	if (ConfigGetBool(&gConfig, "Graphics.ShowHUD"))
	{
		DrawPlayerAreas(hud, numViews);

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

static void DrawPlayerAreas(HUD *hud, const int numViews)
{
	int flags = 0;

	Rect2i r = Rect2iNew(svec2i_zero(), hud->device->cachedConfig.Res);
	if (hud->DrawData.NumScreens <= 1)
	{
		// Do nothing
	}
	else if (hud->DrawData.NumScreens == 2)
	{
		r.Size.x /= 2;
	}
	else if (hud->DrawData.NumScreens == 3 || hud->DrawData.NumScreens == 4)
	{
		r.Size.x /= 2;
		r.Size.y /= 2;
	}
	else
	{
		CASSERT(false, "not implemented");
	}

	if (hud->DrawData.NumScreens <= 1)
	{
		// Do nothing
	}
	else if (hud->DrawData.NumScreens > 1 &&
		ConfigGetEnum(&gConfig, "Interface.Splitscreen") == SPLITSCREEN_NEVER)
	{
		flags |= HUDFLAGS_SHARE_SCREEN;
	}
	else if (hud->DrawData.NumScreens == 2)
	{
		flags |= HUDFLAGS_HALF_SCREEN;
	}
	else if (hud->DrawData.NumScreens == 3 || hud->DrawData.NumScreens == 4)
	{
		flags |= HUDFLAGS_QUARTER_SCREEN;
	}
	else
	{
		CASSERT(false, "not implemented");
	}

	for (int i = 0; i < hud->DrawData.NumScreens; i++)
	{
		const PlayerData *p = hud->DrawData.Players[i];
		int drawFlags = flags;
		r.Pos = svec2i_zero();
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
		DrawPlayerHUD(hud, p, drawFlags, i, r, numViews);
	}

	// Only draw radar once if shared
	if (ConfigGetBool(&gConfig, "Interface.ShowHUDMap") &&
		(flags & HUDFLAGS_SHARE_SCREEN) &&
		IsAutoMapEnabled(gCampaign.Entry.Mode))
	{
		DrawSharedRadar(hud->device, hud->showExit);
	}
}

static void DrawDeathmatchScores(HUD *hud)
{
	// Only draw deathmatch scores if single screen and non-local players exist
	if (gCampaign.Entry.Mode != GAME_MODE_DEATHMATCH ||
		hud->DrawData.NumScreens != 1 ||
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
	FontStrOpt("Player", svec2i(0, y), opts);
	opts.Pad.x = livesColumn;
	FontStrOpt("Lives", svec2i(0, y), opts);
	opts.Pad.x = killsColumn;
	FontStrOpt("Kills", svec2i(0, y), opts);
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
		FontStrOpt(p->name, svec2i(0, y), opts);

		// lives; cyan if most lives
		opts.Mask = p->Lives == maxLives ? colorCyan : colorWhite;
		opts.Pad.x = livesColumn;
		char buf[32];
		sprintf(buf, "%d", p->Lives);
		FontStrOpt(buf, svec2i(0, y), opts);

		// kills; cyan if most kills
		opts.Mask = p->Stats.Kills == maxKills ? colorCyan : colorWhite;
		opts.Pad.x = killsColumn;
		sprintf(buf, "%d", p->Stats.Kills);
		FontStrOpt(buf, svec2i(0, y), opts);
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
		struct vec2i pos = svec2i_scale_divide(svec2i_subtract(
			gGraphicsDevice.cachedConfig.Res,
			FontStrSize("\x11Paused\x10\nFoobar\nPlease reconnect controller")), 2);
		const int x = pos.x;
		FontStr("\x11Paused\x10", pos);

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
		struct vec2i pos = svec2i_scale_divide(svec2i_subtract(
			gGraphicsDevice.cachedConfig.Res,
			FontStrSize("Foo\nPress foo or bar to unpause\nBaz")), 2);
		const int x = pos.x;
		FontStr("\x11Paused\x10", pos);

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
		struct vec2i pos = svec2i(
			(hud->device->cachedConfig.Res.x -
				FontStrW(hud->message)) / 2,
			AUTOMAP_SIZE + AUTOMAP_PADDING + AUTOMAP_PADDING);
		const HSV tint = { -1.0, 1.0, Pulse256(hud->mission->time) / 256.0};
		const color_t mask = ColorTint(colorCyan, tint);
		FontStrMask(hud->message, pos, mask);
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
			CPicDraw(
				hud->device,
				&KeyPickupClass(hud->mission->missionData->KeyStyle, i)->Pic,
				svec2i(CenterX(8) - xOffset, yOffset), NULL);
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
	opts.Pad.y = AUTOMAP_PADDING;
	FontStrOpt(s, svec2i_zero(), opts);
}

static void DrawObjectiveCounts(HUD *hud)
{
	int x = 45;
	int y = hud->device->cachedConfig.Res.y - 22;
	CA_FOREACH(const Objective, o, hud->mission->missionData->Objectives)
		// Don't draw anything for optional objectives
		if (!ObjectiveIsRequired(o))
		{
			continue;
		}

		// Objective color dot
		DrawRectangle(
			hud->device, svec2i(x, y + 3), svec2i(2, 2), o->color, false);

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
		FontStr(s, svec2i(x, y));

		HUDNumPopupsDrawObjective(
			&hud->numPopups, _ca_index, svec2i(x + FontStrW(s) - 8, y));

		x += 40;
	CA_FOREACH_END()
}
