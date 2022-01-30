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

	Copyright (c) 2013-2014, 2018-2019, 2021 Cong Xu
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

#include "campaigns.h"
#include "map_build.h"

static int MapTryBuildSquare(MapBuilder *mb);
static bool MapIsAreaClearForClassicRoom(
	const MapBuilder *mb, const struct vec2i pos, const struct vec2i size,
	const int pad, bool *isOverlapRoom, uint16_t *overlapAccess);
static void MapBuildRoom(
	MapBuilder *mb, const struct vec2i pos, const struct vec2i size,
	const int doorMin, const int doorMax, const bool hasKeys,
	const bool isOverlapRoom, const uint16_t overlapAccess);
static bool MapTryBuildPillar(MapBuilder *mb, const int pad);
void MapClassicLoad(MapBuilder *mb)
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

	// TODO: multiple tile types
	MissionSetupTileClasses(mb->Map, &gPicManager, &mb->mission->u.Classic.TileClasses);

	MapFillRect(
		mb, Rect2iNew(svec2i_zero(), mb->mission->Size),
				&mb->mission->u.Classic.TileClasses.Wall, &mb->mission->u.Classic.TileClasses.Floor);

	const int pad = MAX(mb->mission->u.Classic.CorridorWidth, 1);

	// place squares
	int count = 0;
	int i = 0;
	while (i < 1000 && count < mb->mission->u.Classic.Squares)
	{
		if (MapTryBuildSquare(mb))
		{
			count++;
		}
		i++;
	}

	// place rooms
	count = 0;
	for (i = 0; i < 1000 && count < mb->mission->u.Classic.Rooms.Count; i++)
	{
		const struct vec2i v = MapGetRandomTile(mb->Map);
		const int doorMin = CLAMP(mb->mission->u.Classic.Doors.Min, 1, 6);
		const int doorMax =
			CLAMP(mb->mission->u.Classic.Doors.Max, doorMin, 6);
		const struct vec2i size =
			MapGetRoomSize(mb->mission->u.Classic.Rooms, doorMin);
		bool isOverlapRoom;
		uint16_t overlapAccess;
		if (!MapIsAreaClearForClassicRoom(
				mb, v, size, pad, &isOverlapRoom, &overlapAccess))
		{
			continue;
		}
		MapBuildRoom(
			mb, v, size, doorMin, doorMax,
			AreKeysAllowed(mb->mode), isOverlapRoom,
			overlapAccess);
		count++;
	}

	// place pillars
	count = 0;
	i = 0;
	while (i < 1000 && count < mb->mission->u.Classic.Pillars.Count)
	{
		if (MapTryBuildPillar(mb, pad))
		{
			count++;
		}
		i++;
	}

	// place walls
	count = 0;
	i = 0;
	while (i < 1000 && count < mb->mission->u.Classic.Walls)
	{
		if (MapTryBuildWall(
				mb, false, pad, mb->mission->u.Classic.WallLength,
				&mb->mission->u.Classic.TileClasses.Wall, Rect2iZero()))
		{
			count++;
		}
		i++;
	}
}

static int MapTryBuildSquare(MapBuilder *mb)
{
	const struct vec2i v = MapGetRandomTile(mb->Map);
	struct vec2i size = svec2i(rand() % 9 + 8, rand() % 9 + 8);
	if (MapIsAreaClear(mb, v, size))
	{
		MapFillRect(
			mb, Rect2iNew(v, size), &mb->mission->u.Cave.TileClasses.Floor, &mb->mission->u.Cave.TileClasses.Floor);
		return 1;
	}
	return 0;
}

static bool MapIsAreaClearForClassicRoom(
	const MapBuilder *mb, const struct vec2i pos, const struct vec2i size,
	const int pad, bool *isOverlapRoom, uint16_t *overlapAccess)
{
	struct vec2i clearPos = svec2i(pos.x - pad, pos.y - pad);
	struct vec2i clearSize = svec2i(size.x + 2 * pad, size.y + 2 * pad);
	bool isEdgeRoom = false;
	*isOverlapRoom = false;
	*overlapAccess = 0;

	if (mb->mission->u.Classic.Rooms.Edge)
	{
		// Check if room is at edge; if so only check if clear inside edge
		if (pos.x == 0 || pos.x == 1)
		{
			const int dx = 1 - clearPos.x;
			clearPos.x += dx;
			clearSize.x -= dx;
			isEdgeRoom = true;
		}
		else if (
			pos.x + size.x == mb->Map->Size.x - 2 ||
			pos.x + size.x == mb->Map->Size.x - 1)
		{
			clearSize.x = mb->Map->Size.x - 1 - pos.x;
			isEdgeRoom = true;
		}
		if (pos.y == 0 || pos.y == 1)
		{
			const int dy = 1 - clearPos.y;
			clearPos.y += dy;
			clearSize.y -= dy;
			isEdgeRoom = true;
		}
		else if (
			pos.y + size.y == mb->Map->Size.y - 2 ||
			pos.y + size.y == mb->Map->Size.y - 1)
		{
			clearSize.y = mb->Map->Size.y - 1 - pos.y;
			isEdgeRoom = true;
		}
	}
	bool isClear = MapIsAreaClear(mb, clearPos, clearSize);
	// Don't let rooms be both edge rooms and overlap rooms
	// Otherwise dead pockets will be created
	if (!isClear && !isEdgeRoom)
	{
		// If room overlap is enabled, check if it overlaps with a room
		const bool isOverlap = mb->mission->u.Classic.Rooms.Overlap &&
							   MapIsAreaClearOrRoom(mb, clearPos, clearSize);
		// Now check if the overlapping rooms will create a passage
		// large enough
		const int roomOverlapSize =
			MapGetRoomOverlapSize(mb, Rect2iNew(pos, size), overlapAccess);
		isClear = isOverlap &&
				  roomOverlapSize >= mb->mission->u.Classic.CorridorWidth;
		*isOverlapRoom = true;
	}
	return isClear;
}

static void MapFindAvailableDoors(
	const MapBuilder *mb, const struct vec2i pos, const struct vec2i size,
	const int doorMin, bool doors[4]);

static void MapBuildRoom(
	MapBuilder *mb, const struct vec2i pos, const struct vec2i size,
	const int doorMin, const int doorMax, const bool hasKeys,
	const bool isOverlapRoom, const uint16_t overlapAccess)
{
	int doormask = rand() % 15 + 1;
	bool doors[4];
	int doorsUnplaced = 0;
	int i;
	uint16_t accessMask = 0;

	MapMakeRoom(
		mb, pos, size, true, &mb->mission->u.Classic.TileClasses.Wall,
		&mb->mission->u.Classic.TileClasses.Room, true);
	// Check which walls we can place doors
	// If we cannot place doors, remember this and try to place them
	// on other walls
	// We cannot place doors on: the perimeter, and on overlaps
	MapFindAvailableDoors(mb, pos, size, doorMin, doors);
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
				doors[i] = false;
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
	if (hasKeys && mb->mission->u.Classic.Doors.Enabled)
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
			accessMask = GenerateAccessMask(&mb->Map->keyAccessCount);
		}
	}
	
	const Rect2i r = Rect2iNew(pos, size);
	MapPlaceDoors(
		mb, r, mb->mission->u.Classic.Doors.Enabled, doors,
		doorMin, doorMax, accessMask,
		mb->mission->u.Classic.Doors.RandomPos,
		&mb->mission->u.Classic.TileClasses.Door,
		&mb->mission->u.Classic.TileClasses.Floor);

	MapMakeRoomWalls(
		mb, mb->mission->u.Classic.Rooms,
		&mb->mission->u.Classic.TileClasses.Wall, r);
}

static bool MapTryBuildPillar(MapBuilder *mb, const int pad)
{
	const int pillarMin = mb->mission->u.Classic.Pillars.Min;
	const int pillarMax = mb->mission->u.Classic.Pillars.Max;
	struct vec2i size = svec2i(
		rand() % (pillarMax - pillarMin + 1) + pillarMin,
		rand() % (pillarMax - pillarMin + 1) + pillarMin);
	const struct vec2i pos = MapGetRandomTile(mb->Map);
	struct vec2i clearPos = svec2i(pos.x - pad, pos.y - pad);
	struct vec2i clearSize = svec2i(size.x + 2 * pad, size.y + 2 * pad);
	int isEdge = 0;
	int isClear = 0;

	// Check if pillar is at edge; if so only check if clear inside edge
	if (pos.x == 0 || pos.x == 1)
	{
		int dx = 1 - clearPos.x;
		clearPos.x += dx;
		clearSize.x -= dx;
		isEdge = 1;
	}
	else if (
		pos.x + size.x == mb->Map->Size.x - 2 ||
		pos.x + size.x == mb->Map->Size.x - 1)
	{
		clearSize.x = mb->Map->Size.x - 1 - pos.x;
		isEdge = 1;
	}
	if (pos.y == 0 || pos.y == 1)
	{
		int dy = 1 - clearPos.y;
		clearPos.y += dy;
		clearSize.y -= dy;
		isEdge = 1;
	}
	else if (
		pos.y + size.y == mb->Map->Size.y - 2 ||
		pos.y + size.y == mb->Map->Size.y - 1)
	{
		clearSize.y = mb->Map->Size.y - 1 - pos.y;
		isEdge = 1;
	}

	// Only place pillars if the area is totally clear,
	// or if the pillar only overlaps one of the edge or another
	// non-room wall
	// This is to prevent dead pockets
	isClear = MapIsAreaClear(mb, clearPos, clearSize);
	if (!isClear && !isEdge && MapIsAreaClearOrWall(mb, clearPos, clearSize))
	{
		// Also check that the pillar does not overlap two pillars
		isClear = MapIsLessThanTwoWallOverlaps(mb, clearPos, clearSize);
	}
	if (isClear)
	{
		MapFillRect(
			mb, Rect2iNew(pos, size),
			&mb->mission->u.Classic.TileClasses.Wall, &gTileNothing);
		return true;
	}
	return false;
}

// Find the maximum door size for a wall
static int FindWallRun(
	const MapBuilder *mb, const struct vec2i mid, const struct vec2i d,
	const int len);
static void MapFindAvailableDoors(
	const MapBuilder *mb, const struct vec2i pos, const struct vec2i size,
	const int doorMin, bool doors[4])
{
	for (int i = 0; i < 4; i++)
	{
		doors[i] = true;
	}
	// left
	if (pos.x <= 1)
	{
		doors[0] = false;
	}
	else if (
		FindWallRun(
			mb, svec2i(pos.x, pos.y + 1), svec2i(0, 1), size.y - 2) <
		doorMin)
	{
		doors[0] = false;
	}
	// right
	if (pos.x + size.x >= mb->Map->Size.x - 2)
	{
		doors[1] = false;
	}
	else if (
		FindWallRun(
			mb, svec2i(pos.x + size.x - 1, pos.y + 1), svec2i(0, 1),
			size.y - 2) < doorMin)
	{
		doors[1] = false;
	}
	// top
	if (pos.y <= 1)
	{
		doors[2] = false;
	}
	else if (
		FindWallRun(
			mb, svec2i(pos.x + 1, pos.y), svec2i(1, 0), size.x - 2) <
		doorMin)
	{
		doors[2] = false;
	}
	// bottom
	if (pos.y >= mb->Map->Size.y - 2)
	{
		doors[3] = false;
	}
	else if (
		FindWallRun(
			mb, svec2i(pos.x + 1, pos.y + size.y - 1), svec2i(1, 0),
			size.x - 2) < doorMin)
	{
		doors[3] = false;
	}
}
static int FindWallRun(
	const MapBuilder *mb, const struct vec2i mid, const struct vec2i d,
	const int len)
{
	int maxRun = 0;
	int run = 0;
	for (int i = 0; i < len; i++, run++)
	{
		// Check if this is a wall so we can add a door here
		// Also check if the two tiles aside are not walls

		// Note: we must look for runs
		const struct vec2i v = svec2i_add(mid, svec2i_scale(d, (float)i));

		if (MapBuilderGetTile(mb, v)->Type != TILE_CLASS_WALL ||
			MapBuilderGetTile(mb, svec2i(v.x + d.y, v.y + d.x))->Type ==
				TILE_CLASS_WALL ||
			MapBuilderGetTile(mb, svec2i(v.x - d.y, v.y - d.x))->Type ==
				TILE_CLASS_WALL)
		{
			if (maxRun < run)
			{
				maxRun = run;
			}
			run = 0;
		}
	}
	return MAX(run, maxRun);
}
