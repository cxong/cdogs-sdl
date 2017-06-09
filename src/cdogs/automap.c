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

    Copyright (c) 2013-2014, 2016 Cong Xu
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
#include "draw/draw.h"
#include "draw/draw_actor.h"
#include "draw/drawtools.h"
#include "font.h"
#include "gamedata.h"
#include "map.h"
#include "mission.h"
#include "objs.h"
#include "pic_manager.h"
#include "pickup.h"


#define MAP_FACTOR 2
#define MASK_ALPHA 128;

color_t colorWall = { 72, 152, 72, 255 };
color_t colorFloor = { 12, 92, 12, 255 };
color_t colorRoom = { 24, 112, 24, 255 };
color_t colorDoor = { 172, 172, 172, 255 };
color_t colorYellowDoor = { 252, 224, 0, 255 };
color_t colorGreenDoor = { 0, 252, 0, 255 };
color_t colorBlueDoor = { 0, 252, 252, 255 };
color_t colorRedDoor = { 132, 0, 0, 255 };
color_t colorExit = { 255, 255, 255, 255 };



static void DisplayPlayer(const TActor *player, Vec2i pos, const int scale)
{
	Vec2i playerPos = Vec2iToTile(
		Vec2iNew(player->tileItem.x, player->tileItem.y));
	pos = Vec2iAdd(pos, Vec2iScale(playerPos, scale));
	if (scale >= 2)
	{
		DrawHead(ActorGetCharacter(player), DIRECTION_DOWN, pos);
	}
	else
	{
		Draw_Point(pos.x, pos.y, colorWhite);
	}
}

static void DisplayObjective(
	TTileItem *t, int objectiveIndex, Vec2i pos, int scale, int flags)
{
	Vec2i objectivePos = Vec2iNew(t->x / TILE_WIDTH, t->y / TILE_HEIGHT);
	const Objective *o =
		CArrayGet(&gMission.missionData->Objectives, objectiveIndex);
	color_t color = o->color;
	pos = Vec2iAdd(pos, Vec2iScale(objectivePos, scale));
	if (flags & AUTOMAP_FLAGS_MASK)
	{
		color.a = MASK_ALPHA;
	}
	if (scale >= 2)
	{
		DrawCross(&gGraphicsDevice, pos.x, pos.y, color);
	}
	else
	{
		Draw_Point(pos.x, pos.y, color);
	}
}

static void DisplayExit(Vec2i pos, int scale, int flags)
{
	Vec2i exitPos = gMap.ExitStart;
	Vec2i exitSize = Vec2iAdd(Vec2iMinus(gMap.ExitEnd, exitPos), Vec2iUnit());
	color_t color = colorExit;

	if (!HasExit(gCampaign.Entry.Mode))
	{
		return;
	}
	
	exitPos = Vec2iScale(exitPos, scale);
	exitSize = Vec2iScale(exitSize, scale);
	exitPos = Vec2iAdd(exitPos, pos);

	if (flags & AUTOMAP_FLAGS_MASK)
	{
		color.a = MASK_ALPHA;
	}
	DrawRectangle(&gGraphicsDevice, exitPos, exitSize, color, DRAW_FLAG_LINE);
}

static void DisplaySummary(void)
{
	char sScore[20];
	Vec2i pos;
	pos.y = gGraphicsDevice.cachedConfig.Res.y - 5 - FontH();

	CA_FOREACH(const Objective, o, gMission.missionData->Objectives)
		if (ObjectiveIsRequired(o)|| o->done > 0)
		{
			color_t textColor = colorWhite;
			pos.x = 5;
			// Objective color dot
			Draw_Rect(pos.x, (pos.y + 3), 2, 2, o->color);

			pos.x += 5;

			sprintf(sScore, "(%d)", o->done);

			if (!ObjectiveIsRequired(o))
			{
				textColor = colorPurple;
			}
			else if (ObjectiveIsComplete(o))
			{
				textColor = colorRed;
			}
			pos = FontStrMask(o->Description, pos, textColor);
			pos.x += 5;
			FontStrMask(sScore, pos, textColor);
			pos.y -= (FontH() + 1);
		}
	CA_FOREACH_END()
}

color_t DoorColor(int x, int y)
{
	int l = MapGetDoorKeycardFlag(&gMap, Vec2iNew(x, y));

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

void DrawDot(TTileItem *t, color_t color, Vec2i pos, int scale)
{
	Vec2i dotPos = Vec2iNew(t->x / TILE_WIDTH, t->y / TILE_HEIGHT);
	pos = Vec2iAdd(pos, Vec2iScale(dotPos, scale));
	Draw_Rect(pos.x, pos.y, scale, scale, color);
}

static void DrawMap(
	Map *map,
	Vec2i center, Vec2i centerOn, Vec2i size,
	int scale, int flags)
{
	int x, y;
	Vec2i mapPos = Vec2iAdd(center, Vec2iScale(centerOn, -scale));
	for (y = 0; y < gMap.Size.y; y++)
	{
		int i;
		for (i = 0; i < scale; i++)
		{
			for (x = 0; x < gMap.Size.x; x++)
			{
				Tile *tile = MapGetTile(map, Vec2iNew(x, y));
				if (!(tile->flags & MAPTILE_IS_NOTHING) &&
					(tile->isVisited || (flags & AUTOMAP_FLAGS_SHOWALL)))
				{
					int j;
					for (j = 0; j < scale; j++)
					{
						Vec2i drawPos = Vec2iNew(
							mapPos.x + x*scale + j,
							mapPos.y + y*scale + i);
						color_t color = colorRoom;
						if (tile->flags & MAPTILE_IS_WALL)
						{
							color = colorWall;
						}
						else if (tile->flags & MAPTILE_NO_WALK)
						{
							color = DoorColor(x, y);
						}
						else if (tile->flags & MAPTILE_IS_NORMAL_FLOOR)
						{
							color = colorFloor;
						}
						if (!ColorEquals(color, colorBlack))
						{
							if (flags & AUTOMAP_FLAGS_MASK)
							{
								color.a = MASK_ALPHA;
							}
							Draw_Point(drawPos.x, drawPos.y, color);
						}
					}
				}
			}
		}
	}
	if (flags & AUTOMAP_FLAGS_MASK)
	{
		color_t color = { 255, 255, 255, 128 };
		Draw_Rect(
			center.x - size.x / 2,
			center.y - size.y / 2,
			size.x, size.y,
			color);
	}
}

static void DrawTileItem(
	TTileItem *t, Tile *tile, Vec2i pos, int scale, int flags);
static void DrawObjectivesAndKeys(Map *map, Vec2i pos, int scale, int flags)
{
	for (int y = 0; y < map->Size.y; y++)
	{
		for (int x = 0; x < map->Size.x; x++)
		{
			Tile *tile = MapGetTile(map, Vec2iNew(x, y));
			CA_FOREACH(ThingId, tid, tile->things)
				DrawTileItem(
					ThingIdGetTileItem(tid), tile, pos, scale, flags);
			CA_FOREACH_END()
		}
	}
}
static void DrawTileItem(
	TTileItem *t, Tile *tile, Vec2i pos, int scale, int flags)
{
	if ((t->flags & TILEITEM_OBJECTIVE) != 0)
	{
		const int obj = ObjectiveFromTileItem(t->flags);
		const Objective *o =
			CArrayGet(&gMission.missionData->Objectives, obj);
		if (!(o->Flags & OBJECTIVE_HIDDEN) || (flags & AUTOMAP_FLAGS_SHOWALL))
		{
			if ((o->Flags & OBJECTIVE_POSKNOWN) ||
				tile->isVisited ||
				(flags & AUTOMAP_FLAGS_SHOWALL))
			{
				DisplayObjective(t, obj, pos, scale, flags);
			}
		}
	}
	else if (t->kind == KIND_PICKUP && tile->isVisited)
	{
		const Pickup *p = CArrayGet(&gPickups, t->id);
		if (p->class->Type == PICKUP_KEYCARD)
		{
			color_t dotColor = colorBlack;
			switch (((const Pickup *)CArrayGet(&gPickups, t->id))->class->u.Keys)
			{
			case FLAGS_KEYCARD_RED:
				dotColor = colorRedDoor;
				break;
			case FLAGS_KEYCARD_BLUE:
				dotColor = colorBlueDoor;
				break;
			case FLAGS_KEYCARD_GREEN:
				dotColor = colorGreenDoor;
				break;
			case FLAGS_KEYCARD_YELLOW:
				dotColor = colorYellowDoor;
				break;
			default:
				CASSERT(false, "Unknown key color");
				break;
			}
			DrawDot(t, dotColor, pos, scale);
		}
	}
}

void AutomapDraw(int flags, bool showExit)
{
	color_t mask = { 0, 128, 0, 255 };
	Vec2i mapCenter = Vec2iNew(
		gGraphicsDevice.cachedConfig.Res.x / 2,
		gGraphicsDevice.cachedConfig.Res.y / 2);
	Vec2i centerOn = Vec2iNew(gMap.Size.x / 2, gMap.Size.y / 2);
	Vec2i pos = Vec2iAdd(mapCenter, Vec2iScale(centerOn, -MAP_FACTOR));

	// Draw faded green overlay
	for (int y = 0; y < gGraphicsDevice.cachedConfig.Res.y; y++)
	{
		for (int x = 0; x < gGraphicsDevice.cachedConfig.Res.x; x++)
		{
			DrawPointMask(&gGraphicsDevice, Vec2iNew(x, y), mask);
		}
	}

	DrawMap(&gMap, mapCenter, centerOn, gMap.Size, MAP_FACTOR, flags);
	DrawObjectivesAndKeys(&gMap, pos, MAP_FACTOR, flags);

	CA_FOREACH(const PlayerData, p, gPlayerDatas)
		if (!IsPlayerAlive(p))
		{
			continue;
		}
		DisplayPlayer(ActorGetByUID(p->ActorUID), pos, MAP_FACTOR);
	CA_FOREACH_END()

	if (showExit)
	{
		DisplayExit(pos, MAP_FACTOR, flags);
	}
	DisplaySummary();
}

void AutomapDrawRegion(
	Map *map,
	Vec2i pos, Vec2i size, Vec2i mapCenter,
	int scale, int flags, bool showExit)
{
	const BlitClipping oldClip = gGraphicsDevice.clipping;
	GraphicsSetBlitClip(
		&gGraphicsDevice,
		pos.x, pos.y, pos.x + size.x - 1, pos.y + size.y - 1);
	pos = Vec2iAdd(pos, Vec2iScaleDiv(size, 2));
	DrawMap(map, pos, mapCenter, size, scale, flags);
	const Vec2i centerOn = Vec2iAdd(pos, Vec2iScale(mapCenter, -scale));
	CA_FOREACH(const PlayerData, p, gPlayerDatas)
		if (!IsPlayerAlive(p))
		{
			continue;
		}
		const TActor *player = ActorGetByUID(p->ActorUID);
		DisplayPlayer(player, centerOn, scale);
	CA_FOREACH_END()
	DrawObjectivesAndKeys(&gMap, centerOn, scale, flags);
	if (showExit)
	{
		DisplayExit(centerOn, scale, flags);
	}
	GraphicsSetBlitClip(
		&gGraphicsDevice,
		oldClip.left, oldClip.top, oldClip.right, oldClip.bottom);
}
