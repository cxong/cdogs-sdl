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
#include "map_classic.h"

#include <assert.h>

static int MapTryBuildSquare(Map *map, Mission *m);
static int MapTryBuildRoom(
	Map *map, Mission *m, int pad,
	int doorMin, int doorMax, int hasKeys);
static int MapTryBuildPillar(Map *map, Mission *m, int pad);
static int MapTryBuildWall(
	Map *map, Vec2i pos, Vec2i size,
	unsigned short tileType, int pad, int wallLength);
void MapClassicLoad(Map *map, Mission *mission)
{
	// place squares
	int pad = MAX(mission->u.Classic.CorridorWidth, 1);
	int count = 0;
	int i = 0;
	while (i < 1000 && count < mission->u.Classic.Squares)
	{
		if (MapTryBuildSquare(map, mission))
		{
			count++;
		}
		i++;
	}

	// place rooms
	map->keyAccessCount = 0;
	count = 0;
	i = 0;
	while (i < 1000 && count < mission->u.Classic.Rooms.Count)
	{
		int doorMin = CLAMP(mission->u.Classic.Doors.Min, 1, 6);
		int doorMax = CLAMP(mission->u.Classic.Doors.Max, doorMin, 6);
		if (MapTryBuildRoom(
			map, mission, pad,
			doorMin, doorMax, AreKeysAllowed(gCampaign.Entry.mode)))
		{
			count++;
		}
		i++;
	}

	// place pillars
	count = 0;
	i = 0;
	while (i < 1000 && count < mission->u.Classic.Pillars.Count)
	{
		if (MapTryBuildPillar(map, mission, pad))
		{
			count++;
		}
		i++;
	}

	// place walls
	count = 0;
	i = 0;
	while (i < 1000 && count < mission->u.Classic.Walls)
	{
		if (MapTryBuildWall(
			map,
			Vec2iNew((XMAX - mission->Size.x) / 2, (YMAX - mission->Size.y) / 2),
			mission->Size, MAP_FLOOR,
			pad, mission->u.Classic.WallLength))
		{
			count++;
		}
		i++;
	}
}

static Vec2i GuessCoords(Vec2i pos, Vec2i size);

static int MapTryBuildSquare(Map *map, Mission *m)
{
	Vec2i v = GuessCoords(
		Vec2iNew((XMAX - m->Size.x) / 2, (YMAX - m->Size.y) / 2),
		m->Size);
	Vec2i size;
	size.x = rand() % 9 + 7;
	size.y = rand() % 9 + 7;

	if (MapIsAreaClear(
		map, Vec2iNew(v.x - 1, v.y - 1), Vec2iNew(size.x + 2, size.y + 2)))
	{
		MapMakeSquare(map, v, size);
		return 1;
	}
	return 0;
}
static unsigned short GenerateAccessMask(int *accessLevel);
static int MapTryBuildRoom(
	Map *map, Mission *m, int pad,
	int doorMin, int doorMax, int hasKeys)
{
	// make sure rooms are large enough to accomodate doors
	int roomMin = MAX(m->u.Classic.Rooms.Min, doorMin + 4);
	int roomMax = MAX(m->u.Classic.Rooms.Max, doorMin + 4);
	int w = rand() % (roomMax - roomMin + 1) + roomMin;
	int h = rand() % (roomMax - roomMin + 1) + roomMin;
	Vec2i pos = GuessCoords(
		Vec2iNew((XMAX - m->Size.x) / 2, (YMAX - m->Size.y) / 2), m->Size);
	Vec2i clearPos = Vec2iNew(pos.x - pad, pos.y - pad);
	Vec2i clearSize = Vec2iNew(w + 2 * pad, h + 2 * pad);
	int doors[4];

	// left, right, top, bottom
	doors[0] = doors[1] = doors[2] = doors[3] = 1;
	if (m->u.Classic.Rooms.Edge)
	{
		// Check if room is at edge; if so only check if clear inside edge
		if (pos.x == (XMAX - m->Size.x) / 2 ||
			pos.x == (XMAX - m->Size.x) / 2 + 1)
		{
			clearPos.x = (XMAX - m->Size.x) / 2 + 1;
			doors[0] = 0;
		}
		else if (pos.x + w == (XMAX + m->Size.x) / 2 - 2 ||
			pos.x + w == (XMAX + m->Size.x) / 2 - 1)
		{
			clearSize.x = (XMAX + m->Size.x) / 2 - 2 - pos.x;
			doors[1] = 0;
		}
		if (pos.y == (YMAX - m->Size.y) / 2 ||
			pos.y == (YMAX - m->Size.y) / 2 + 1)
		{
			clearPos.y = (YMAX - m->Size.y) / 2 + 1;
			doors[2] = 0;
		}
		else if (pos.y + h == (YMAX + m->Size.y) / 2 - 2 ||
			pos.y + h == (YMAX + m->Size.y) / 2 - 1)
		{
			clearSize.y = (YMAX + m->Size.y) / 2 - 2 - pos.y;
			doors[3] = 0;
		}
	}

	if (MapIsAreaClear(map, clearPos, clearSize))
	{
		int doormask = rand() % 15 + 1;
		int doorsUnplaced = 0;
		int i;
		unsigned short accessMask = 0;
		int count;
		if (hasKeys && m->u.Classic.Doors.Enabled)
		{
			accessMask = GenerateAccessMask(&map->keyAccessCount);
		}

		// Try to place doors according to the random mask
		// If we cannot place a door, remember this and try to place it
		// on the next door
		for (i = 0; i < 4; i++)
		{
			if ((doormask & (1 << i)) && !doors[i])
			{
				doorsUnplaced++;
			}
		}
		for (i = 0; i < 4; i++)
		{
			if (!(doormask & (1 << i)))
			{
				if (doorsUnplaced == 0)
				{
					doors[i] = 0;
				}
				else
				{
					doorsUnplaced--;
				}
			}
		}
		MapMakeRoom(
			map, pos.x, pos.y, w, h,
			m->u.Classic.Doors.Enabled,
			doors, doorMin, doorMax, accessMask);
		if (hasKeys)
		{
			if (map->keyAccessCount < 1)
			{
				map->keyAccessCount = 1;
			}
		}

		// Try to place room walls
		count = 0;
		i = 0;
		while (i < 100 && count < m->u.Classic.Rooms.Walls)
		{
			if (MapTryBuildWall(
				map, pos, Vec2iNew(w, h),
				MAP_ROOM, MAX(m->u.Classic.Rooms.WallPad, 2),
				m->u.Classic.Rooms.WallLength))
			{
				count++;
			}
			i++;
		}

		return 1;
	}
	return 0;
}
static int MapTryBuildPillar(Map *map, Mission *m, int pad)
{
	int pillarMin = m->u.Classic.Pillars.Min;
	int pillarMax = m->u.Classic.Pillars.Max;
	Vec2i size = Vec2iNew(
		rand() % (pillarMax - pillarMin + 1) + pillarMin,
		rand() % (pillarMax - pillarMin + 1) + pillarMin);
	Vec2i pos = GuessCoords(
		Vec2iNew((XMAX - m->Size.x) / 2, (YMAX - m->Size.y) / 2), m->Size);
	Vec2i clearPos = Vec2iNew(pos.x - pad, pos.y - pad);
	Vec2i clearSize = Vec2iNew(size.x + 2 * pad, size.y + 2 * pad);

	// Check if pillar is at edge; if so only check if clear inside edge
	if (pos.x == (XMAX - m->Size.x) / 2 ||
		pos.x == (XMAX - m->Size.x) / 2 + 1)
	{
		clearPos.x = (XMAX - m->Size.x) / 2 + 1;
	}
	else if (pos.x + size.x == (XMAX + m->Size.x) / 2 - 2 ||
		pos.x + size.x == (XMAX + m->Size.x) / 2 - 1)
	{
		clearSize.x = (XMAX + m->Size.x) / 2 - 2 - pos.x;
	}
	if (pos.y == (YMAX - m->Size.y) / 2 ||
		pos.y == (YMAX - m->Size.y) / 2 + 1)
	{
		clearPos.y = (YMAX - m->Size.y) / 2 + 1;
	}
	else if (pos.y + size.y == (YMAX + m->Size.y) / 2 - 2 ||
		pos.y + size.y == (YMAX + m->Size.y) / 2 - 1)
	{
		clearSize.y = (YMAX + m->Size.y) / 2 - 2 - pos.y;
	}

	if (MapIsAreaClear(map, clearPos, clearSize))
	{
		MapMakePillar(map, pos, size);
		return 1;
	}
	return 0;
}
static void MapGrowWall(
	Map *map, int x, int y,
	unsigned short tileType, int pad, int d, int length);
static int MapTryBuildWall(
	Map *map, Vec2i pos, Vec2i size,
	unsigned short tileType, int pad, int wallLength)
{
	Vec2i v = GuessCoords(pos, size);
	if (MapIsValidStartForWall(map, v.x, v.y, tileType, pad))
	{
		MapMakeWall(map, v);
		MapGrowWall(map, v.x, v.y, tileType, pad, rand() & 3, wallLength);
		return 1;
	}
	return 0;
}
static void MapGrowWall(
	Map *map, int x, int y,
	unsigned short tileType, int pad, int d, int length)
{
	int l;
	Vec2i v;

	if (length <= 0)
		return;

	switch (d) {
	case 0:
		if (y < 2 + pad)
		{
			return;
		}
		// Check tiles above
		// xxxxx
		//  xxx
		//   o
		for (v.y = y - 2; v.y > y - 2 - pad; v.y--)
		{
			int level = v.y - (y - 2);
			for (v.x = x - 1 - level; v.x <= x + 1 + level; v.x++)
			{
				if (IMapGet(map, v) != tileType)
				{
					return;
				}
			}
		}
		y--;
		break;
	case 1:
		if (x > XMAX - 2 - pad)
		{
			return;
		}
		// Check tiles to the right
		//   x
		//  xx
		// oxx
		//  xx
		//   x
		for (v.x = x + 2; v.x < x + 2 + pad; v.x++)
		{
			int level = v.x - (x + 2);
			for (v.y = y - 1 - level; v.y <= y + 1 + level; v.y++)
			{
				if (IMapGet(map, v) != tileType)
				{
					return;
				}
			}
		}
		x++;
		break;
	case 2:
		if (y > YMAX - 2 - pad)
		{
			return;
		}
		// Check tiles below
		//   o
		//  xxx
		// xxxxx
		for (v.y = y + 2; v.y < y + 2 + pad; v.y++)
		{
			int level = v.y - (y + 2);
			for (v.x = x - 1 - level; v.x <= x + 1 + level; v.x++)
			{
				if (IMapGet(map, v) != tileType)
				{
					return;
				}
			}
		}
		y++;
		break;
	case 4:
		if (x < 2 + pad)
		{
			return;
		}
		// Check tiles to the left
		// x
		// xx
		// xxo
		// xx
		// x
		for (v.x = x - 2; v.x > x - 2 - pad; v.x--)
		{
			int level = v.x - (x - 2);
			for (v.y = y - 1 - level; v.y <= y + 1 + level; v.y++)
			{
				if (IMapGet(map, v) != tileType)
				{
					return;
				}
			}
		}
		x--;
		break;
	}
	MapMakeWall(map, Vec2iNew(x, y));
	length--;
	if (length > 0 && (rand() & 3) == 0)
	{
		l = rand() % length;
		MapGrowWall(map, x, y, tileType, pad, rand() & 3, l);
		length -= l;
	}
	MapGrowWall(map, x, y, tileType, pad, d, length);
}

static Vec2i GuessCoords(Vec2i pos, Vec2i size)
{
	return Vec2iNew(
		pos.x + (rand() % size.x), pos.y + (rand() % size.y));
}

static unsigned short GenerateAccessMask(int *accessLevel)
{
	unsigned short accessMask = 0;
	switch (rand() % 20)
	{
	case 0:
		if (*accessLevel >= 4)
		{
			accessMask = MAP_ACCESS_RED;
			*accessLevel = 5;
		}
		break;
	case 1:
	case 2:
		if (*accessLevel >= 3)
		{
			accessMask = MAP_ACCESS_BLUE;
			if (*accessLevel < 4)
			{
				*accessLevel = 4;
			}
		}
		break;
	case 3:
	case 4:
	case 5:
		if (*accessLevel >= 2)
		{
			accessMask = MAP_ACCESS_GREEN;
			if (*accessLevel < 3)
			{
				*accessLevel = 3;
			}
		}
		break;
	case 6:
	case 7:
	case 8:
	case 9:
		if (*accessLevel >= 1)
		{
			accessMask = MAP_ACCESS_YELLOW;
			if (*accessLevel < 2)
			{
				*accessLevel = 2;
			}
		}
		break;
	}
	return accessMask;
}
