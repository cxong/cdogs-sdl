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

#include "gamedata.h"
#include "map_build.h"


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
	// The classic random map generator randomly attempts to place
	// a configured number of features on the map, in order:
	// 1. "Squares", almost-square areas of empty floor
	// 2. Rooms, rectangles of "room" squares surrounded by walls,
	//    some of which have doors
	// 3. "Pillars", or rectangles of solid wall
	// 4. Walls, single-thickness walls that sometimes bend and
	//    split
	// All features are placed by randomly picking a position in
	// the map and checking to see if it's possible to place the
	// feature at that point, and repeating this process until
	// either all features are placed or too many attempts have
	// been done.
	// Sometimes it's impossible to place features, either because
	// they overlap with other incompatible features, or it may
	// create inaccessible areas on the map.

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
	Vec2i size = Vec2iNew(rand() % 9 + 8, rand() % 9 + 8);
	if (MapIsAreaClear(map, v, size))
	{
		MapMakeSquare(map, v, size);
		return 1;
	}
	return 0;
}
static void MapFindAvailableDoors(
	Map *map, Mission *m, Vec2i pos, Vec2i size, int doorMin, int doors[4]);
static unsigned short GenerateAccessMask(int *accessLevel);
static int MapTryBuildRoom(
	Map *map, Mission *m, int pad,
	int doorMin, int doorMax, int hasKeys)
{
	// Work out dimensions of room
	// make sure room is large enough to accommodate doors
	int roomMin = MAX(m->u.Classic.Rooms.Min, doorMin + 4);
	int roomMax = MAX(m->u.Classic.Rooms.Max, doorMin + 4);
	int w = rand() % (roomMax - roomMin + 1) + roomMin;
	int h = rand() % (roomMax - roomMin + 1) + roomMin;
	Vec2i pos = GuessCoords(
		Vec2iNew((XMAX - m->Size.x) / 2, (YMAX - m->Size.y) / 2), m->Size);
	Vec2i clearPos = Vec2iNew(pos.x - pad, pos.y - pad);
	Vec2i clearSize = Vec2iNew(w + 2 * pad, h + 2 * pad);
	int isClear = 0;
	int isEdgeRoom = 0;
	int isOverlapRoom = 0;
	unsigned short overlapAccess = 0;

	if (m->u.Classic.Rooms.Edge)
	{
		// Check if room is at edge; if so only check if clear inside edge
		if (pos.x == (XMAX - m->Size.x) / 2 ||
			pos.x == (XMAX - m->Size.x) / 2 + 1)
		{
			int dx = (XMAX - m->Size.x) / 2 + 1 - clearPos.x;
			clearPos.x += dx;
			clearSize.x -= dx;
			isEdgeRoom = 1;
		}
		else if (pos.x + w == (XMAX + m->Size.x) / 2 - 2 ||
			pos.x + w == (XMAX + m->Size.x) / 2 - 1)
		{
			clearSize.x = (XMAX + m->Size.x) / 2 - 1 - pos.x;
			isEdgeRoom = 1;
		}
		if (pos.y == (YMAX - m->Size.y) / 2 ||
			pos.y == (YMAX - m->Size.y) / 2 + 1)
		{
			int dy = (YMAX - m->Size.y) / 2 + 1 - clearPos.y;
			clearPos.y += dy;
			clearSize.y -= dy;
			isEdgeRoom = 1;
		}
		else if (pos.y + h == (YMAX + m->Size.y) / 2 - 2 ||
			pos.y + h == (YMAX + m->Size.y) / 2 - 1)
		{
			clearSize.y = (YMAX + m->Size.y) / 2 - 1 - pos.y;
			isEdgeRoom = 1;
		}
	}
	isClear = MapIsAreaClear(map, clearPos, clearSize);
	// Don't let rooms be both edge rooms and overlap rooms
	// Otherwise dead pockets will be created
	if (!isClear && !isEdgeRoom)
	{
		// If room overlap is enabled, check if it overlaps with a room
		int isOverlap = m->u.Classic.Rooms.Overlap &&
			MapIsAreaClearOrRoom(map, clearPos, clearSize);
		// Now check if the overlapping rooms will create a passage
		// large enough
		int roomOverlapSize = MapGetRoomOverlapSize(
			map, pos, Vec2iNew(w, h), &overlapAccess);
		isClear = isOverlap && roomOverlapSize >= m->u.Classic.CorridorWidth;
		isOverlapRoom = 1;
	}
	if (isClear)
	{
		int doormask = rand() % 15 + 1;
		int doors[4];
		int doorsUnplaced = 0;
		int i;
		unsigned short accessMask = 0;
		int count;

		MapMakeRoom(map, pos.x, pos.y, w, h);
		// Check which walls we can place doors
		// If we cannot place doors, remember this and try to place them
		// on other walls
		// We cannot place doors on: the perimeter, and on overlaps
		MapFindAvailableDoors(map, m, pos, Vec2iNew(w, h), doorMin, doors);
		// Try to place doors according to the random mask
		// If we cannot place a door, remember this and try to place it
		// on other doors
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
					// If we don't need to place any doors,
					// set this door to unavailable
					doors[i] = 0;
				}
				else if (doors[i])
				{
					// Otherwise, if it's possible to place a door here
					// and we need to place doors, do so
					doorsUnplaced--;
				}
			}
		}

		// Work out what access level (i.e. key) this room has
		if (hasKeys && m->u.Classic.Doors.Enabled)
		{
			// If this room has overlapped another room, use the same
			// access level as that room
			if (isOverlapRoom)
			{
				accessMask = overlapAccess;
			}
			else
			{
				// Otherwise, generate an access level for this room
				accessMask = GenerateAccessMask(&map->keyAccessCount);
				if (map->keyAccessCount < 1)
				{
					map->keyAccessCount = 1;
				}
			}
		}
		MapPlaceDoors(map, pos, Vec2iNew(w, h),
			m->u.Classic.Doors.Enabled, doors, doorMin, doorMax, accessMask);

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
	int isEdge = 0;
	int isClear = 0;

	// Check if pillar is at edge; if so only check if clear inside edge
	if (pos.x == (XMAX - m->Size.x) / 2 ||
		pos.x == (XMAX - m->Size.x) / 2 + 1)
	{
		int dx = (XMAX - m->Size.x) / 2 + 1 - clearPos.x;
		clearPos.x += dx;
		clearSize.x -= dx;
		isEdge = 1;
	}
	else if (pos.x + size.x == (XMAX + m->Size.x) / 2 - 2 ||
		pos.x + size.x == (XMAX + m->Size.x) / 2 - 1)
	{
		clearSize.x = (XMAX + m->Size.x) / 2 - 1 - pos.x;
		isEdge = 1;
	}
	if (pos.y == (YMAX - m->Size.y) / 2 ||
		pos.y == (YMAX - m->Size.y) / 2 + 1)
	{
		int dy = (YMAX - m->Size.y) / 2 + 1 - clearPos.y;
		clearPos.y += dy;
		clearSize.y -= dy;
		isEdge = 1;
	}
	else if (pos.y + size.y == (YMAX + m->Size.y) / 2 - 2 ||
		pos.y + size.y == (YMAX + m->Size.y) / 2 - 1)
	{
		clearSize.y = (YMAX + m->Size.y) / 2 - 1 - pos.y;
		isEdge = 1;
	}

	// Only place pillars if the area is totally clear,
	// or if the pillar only overlaps one of the edge or another
	// non-room wall
	// This is to prevent dead pockets
	isClear = MapIsAreaClear(map, clearPos, clearSize);
	if (!isClear && !isEdge && MapIsAreaClearOrWall(map, clearPos, clearSize))
	{
		// Also check that the pillar does not overlap two pillars
		isClear = MapIsLessThanTwoWallOverlaps(map, clearPos, clearSize);
	}
	if (isClear)
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
		// Randomly try to grow the wall in a different direction
		l = rand() % length;
		MapGrowWall(map, x, y, tileType, pad, rand() & 3, l);
		length -= l;
	}
	// Keep growing wall in same direction
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

static int MapFindWallRun(Map *map, Vec2i start, Vec2i d, int len)
{
	int wallRun = 0;
	Vec2i v;
	int i;
	for (i = 0, v = start; i < len; i++, v = Vec2iAdd(v, d))
	{
		// Check if this is a wall so we can add a door here
		// Also check if the two tiles aside are not walls
		if (IMapGet(map, v) == MAP_WALL &&
			IMapGet(map, Vec2iNew(v.x + d.y, v.y + d.x)) != MAP_WALL &&
			IMapGet(map, Vec2iNew(v.x - d.y, v.y - d.x)) != MAP_WALL)
		{
			wallRun++;
		}
	}
	return wallRun;
}
static void MapFindAvailableDoors(
	Map *map, Mission *m, Vec2i pos, Vec2i size, int doorMin, int doors[4])
{
	int i;
	for (i = 0; i < 4; i++)
	{
		doors[i] = 1;
	}
	// left
	if (pos.x <= (XMAX - m->Size.x) / 2 + 1)
	{
		doors[0] = 0;
	}
	if (MapFindWallRun(
		map,
		Vec2iNew(pos.x, pos.y + 1),
		Vec2iNew(0, 1),
		size.y - 2) < doorMin)
	{
		doors[0] = 0;
	}
	// right
	if (pos.x + size.x >= (XMAX + m->Size.x) / 2 - 2)
	{
		doors[1] = 0;
	}
	if (MapFindWallRun(
		map,
		Vec2iNew(pos.x + size.x - 1, pos.y + 1),
		Vec2iNew(0, 1),
		size.y - 2) < doorMin)
	{
		doors[1] = 0;
	}
	// top
	if (pos.y <= (YMAX - m->Size.y) / 2 + 1)
	{
		doors[2] = 0;
	}
	if (MapFindWallRun(
		map,
		Vec2iNew(pos.x + 1, pos.y),
		Vec2iNew(1, 0),
		size.x - 2) < doorMin)
	{
		doors[2] = 0;
	}
	// bottom
	if (pos.y >= (YMAX + m->Size.y) / 2 - 2)
	{
		doors[3] = 0;
	}
	if (MapFindWallRun(
		map,
		Vec2iNew(pos.x + size.x - 1, pos.y),
		Vec2iNew(1, 0),
		size.x - 2) < doorMin)
	{
		doors[3] = 0;
	}
}
