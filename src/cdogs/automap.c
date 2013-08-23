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
#include "automap.h"

#include <stdio.h>
#include <string.h>

#include "actors.h"
#include "config.h"
#include "drawtools.h"
#include "map.h"
#include "mission.h"
#include "text.h"


#define MAP_XOFFS   60
#define MAP_YOFFS   10

#define MAP_FACTOR  2

color_t colorWall = { 72, 152, 72 };
color_t colorFloor = { 12, 92, 12 };
color_t colorDoor = { 172, 172, 172 };
color_t colorYellowDoor = { 252, 224, 0 };
color_t colorGreenDoor = { 0, 252, 0 };
color_t colorBlueDoor = { 0, 252, 252 };
color_t colorRedDoor = { 132, 0, 0 };
color_t colorExit = { 255, 255, 255 };



static void DisplayPlayer(TActor * player)
{
	int x, y;
	struct CharacterDescription *c;
	int pic;

	if (player) {
		c = &gCharacterDesc[player->character];
		pic = cHeadPic[c->facePic][DIRECTION_DOWN][STATE_IDLE];
		x = MAP_XOFFS +
		    MAP_FACTOR * player->tileItem.x / TILE_WIDTH;
		y = MAP_YOFFS +
		    MAP_FACTOR * player->tileItem.y / TILE_HEIGHT;
		x -= gPics[pic]->w / 2;
		y -= gPics[pic]->h / 2;
		DrawTTPic(x, y, gPics[pic], c->table);
	}
}

static void DisplayObjective(TTileItem * t, int objectiveIndex)
{
	int x = MAP_XOFFS + MAP_FACTOR * t->x / TILE_WIDTH;
	int y = MAP_YOFFS + MAP_FACTOR * t->y / TILE_HEIGHT;
	DrawCross(
		&gGraphicsDevice, x, y, gMission.objectives[objectiveIndex].color);
}

static void DisplayExit(void)
{
	Uint32 *scr = gGraphicsDevice.buf;
	int x1, x2, y1, y2;

	if (!CanCompleteMission(&gMission))
	{
		return;
	}

	x1 = MAP_FACTOR * gMission.exitLeft / TILE_WIDTH + MAP_XOFFS;
	y1 = MAP_FACTOR * gMission.exitTop / TILE_HEIGHT + MAP_YOFFS;
	x2 = MAP_FACTOR * gMission.exitRight / TILE_WIDTH + MAP_XOFFS;
	y2 = MAP_FACTOR * gMission.exitBottom / TILE_HEIGHT + MAP_YOFFS;

	DrawRectangle(scr, x1, y1, x2 - x1, y2 - y1, colorExit, DRAW_FLAG_LINE);
}

static void DisplaySummary(void)
{
	int i, y, x, x2;
	char sScore[20];

	y = gGraphicsDevice.cachedConfig.ResolutionHeight - 5 - CDogsTextHeight(); // 10 pixels from bottom

	for (i = 0; i < gMission.missionData->objectiveCount; i++) {
		if (gMission.objectives[i].required > 0 ||
			gMission.objectives[i].done > 0)
		{
			x = 5;
			// Objective color dot
			Draw_Rect(x, (y + 3), 2, 2, gMission.objectives[i].color);

			x += 5;
			x2 = x + TextGetStringWidth(gMission.missionData->objectives[i].description) + 5;

			sprintf(sScore, "(%d)", gMission.objectives[i].done);

			if (gMission.objectives[i].required <= 0) {
				CDogsTextStringWithTableAt(x, y,
						      gMission.missionData->objectives[i].description,
						      &tablePurple);
				CDogsTextStringWithTableAt(x2, y, sScore, &tablePurple);
			} else if (gMission.objectives[i].done >= gMission.objectives[i].required) {
				CDogsTextStringWithTableAt(x, y,
						      gMission.missionData->objectives[i].description,
						      &tableFlamed);
				CDogsTextStringWithTableAt(x2, y, sScore, &tableFlamed);
			} else {
				CDogsTextStringAt(x, y, gMission.missionData->objectives[i].description);
				CDogsTextStringAt(x2, y, sScore);
			}
			y -= (CDogsTextHeight() + 1);
		}
	}
}

static int MapLevel(int x, int y)
{
	int l;

	l = MapAccessLevel(x - 1, y);
	if (l)
		return l;
	l = MapAccessLevel(x + 1, y);
	if (l)
		return l;
	l = MapAccessLevel(x, y - 1);
	if (l)
		return l;
	return MapAccessLevel(x, y + 1);
}

color_t DoorColor(int x, int y)
{
	int l = MapLevel(x, y);

	switch (l) {
	case FLAGS_KEYCARD_YELLOW:
		return colorYellowDoor;
	case FLAGS_KEYCARD_GREEN:
		return colorGreenDoor;
	case FLAGS_KEYCARD_BLUE:
		return colorBlueDoor;
	case FLAGS_KEYCARD_RED:
		return colorRedDoor;
	default:
		return colorDoor;
	}
}

void DrawDot(TTileItem * t, color_t color)
{
	unsigned int x, y;

	x = MAP_XOFFS + MAP_FACTOR * t->x / TILE_WIDTH;
	y = MAP_YOFFS + MAP_FACTOR * t-> y / TILE_HEIGHT;

	Draw_Rect(x, y, 2, 2, color);
}

void AutomapDraw(int flags)
{
	int x, y, i, j;
	Uint32 *screen = gGraphicsDevice.buf;
	TTileItem *t;
	color_t mask = { 0, 128, 0 };

	// Draw faded green overlay
	for (y = 0; y < gGraphicsDevice.cachedConfig.ResolutionHeight; y++)
	{
		for (x = 0; x < gGraphicsDevice.cachedConfig.ResolutionWidth; x++)
		{
			DrawPointMask(&gGraphicsDevice, Vec2iNew(x, y), mask);
		}
	}

	screen += MAP_YOFFS * gGraphicsDevice.cachedConfig.ResolutionWidth + MAP_XOFFS;
	for (y = 0; y < YMAX; y++)
		for (i = 0; i < MAP_FACTOR; i++) {
			for (x = 0; x < XMAX; x++)
			{
				if (!(Map(x, y).flags & MAPTILE_IS_NOTHING) &&
					(Map(x, y).isVisited || (flags & AUTOMAP_FLAGS_SHOWALL)))
				{
					int tileFlags = Map(x, y).flags;
					for (j = 0; j < MAP_FACTOR; j++)
					{
						if (tileFlags & MAPTILE_IS_WALL)
						{
							*screen++ =
								PixelFromColor(&gGraphicsDevice, colorWall);
						}
						else if (tileFlags & MAPTILE_NO_WALK)
						{
							*screen++ =
								PixelFromColor(&gGraphicsDevice, DoorColor(x, y));
						}
						else
						{
							*screen++ =
								PixelFromColor(&gGraphicsDevice, colorFloor);
						}
					}
				}
				else
				{
					screen += MAP_FACTOR;
				}
			}
			screen += gGraphicsDevice.cachedConfig.ResolutionWidth - XMAX * MAP_FACTOR;
		}

	for (y = 0; y < YMAX; y++)
		for (x = 0; x < XMAX; x++) {
			t = Map(x, y).things;
			while (t)
			{
				if ((t->flags & TILEITEM_OBJECTIVE) != 0)
				{
					int obj = ObjectiveFromTileItem(t->flags);
					int objFlags = gMission.missionData->objectives[obj].flags;
					if (!(objFlags & OBJECTIVE_HIDDEN) ||
						(flags & AUTOMAP_FLAGS_SHOWALL))
					{
						if ((objFlags & OBJECTIVE_POSKNOWN) ||
							Map(x, y).isVisited ||
							(flags & AUTOMAP_FLAGS_SHOWALL))
						{
							DisplayObjective(t, obj);
						}
					}
				}
				else if (t->kind == KIND_OBJECT &&
					t->data &&
					Map(x, y).isVisited)
				{
					color_t dotColor = colorBlack;
					switch (((TObject *)t->data)->objectIndex)
					{
					case OBJ_KEYCARD_RED:
						dotColor = colorRedDoor;
						break;
					case OBJ_KEYCARD_BLUE:
						dotColor = colorBlueDoor;
						break;
					case OBJ_KEYCARD_GREEN:
						dotColor = colorGreenDoor;
						break;
					case OBJ_KEYCARD_YELLOW:
						dotColor = colorYellowDoor;
						break;
					default:
						break;
					}
					if (!ColorEquals(dotColor, colorBlack))
					{
						DrawDot(t, dotColor);
					}
				}

				t = t->next;
			}
		}


	DisplayPlayer(gPlayer1);
	DisplayPlayer(gPlayer2);

	DisplayExit();
	DisplaySummary();
}
