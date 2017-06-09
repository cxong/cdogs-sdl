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

    Copyright (c) 2013-2014, 2017 Cong Xu
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
#include "map_build.h"


#define EXIT_WIDTH  8
#define EXIT_HEIGHT 8

void MapMakeWall(Map *map, Vec2i pos)
{
	IMapSet(map, pos, MAP_WALL);
}

static void MapSetupTile(Map *map, const Vec2i pos, const Mission *m);

void MapSetTile(Map *map, Vec2i pos, unsigned short tileType, Mission *m)
{
	IMapSet(map, pos, tileType);
	// Update the tile as well, plus neighbours as they may be affected
	// by shadows etc. especially walls
	MapSetupTile(map, pos, m);
	MapSetupTile(map, Vec2iNew(pos.x - 1, pos.y), m);
	MapSetupTile(map, Vec2iNew(pos.x + 1, pos.y), m);
	MapSetupTile(map, Vec2iNew(pos.x, pos.y - 1), m);
	MapSetupTile(map, Vec2iNew(pos.x, pos.y + 1), m);
}

void MapSetupTilesAndWalls(Map *map, const Mission *m)
{
	// Pre-load the tile pics that this map will use
	// TODO: multiple styles and colours
	// Walls
	for (int i = 0; i < WALL_TYPE_COUNT; i++)
	{
		PicManagerGenerateMaskedStylePic(
			&gPicManager, "wall", m->WallStyle, IntWallType(i),
			m->WallMask, m->AltMask);
	}
	// Floors
	for (int i = 0; i < FLOOR_TYPES; i++)
	{
		PicManagerGenerateMaskedStylePic(
			&gPicManager, "tile", m->FloorStyle, IntTileType(i),
			m->FloorMask, m->AltMask);
	}
	// Rooms
	for (int i = 0; i < ROOMFLOOR_TYPES; i++)
	{
		PicManagerGenerateMaskedStylePic(
			&gPicManager, "tile", m->RoomStyle, IntTileType(i),
			m->RoomMask, m->AltMask);
	}

	Vec2i v;
	for (v.x = 0; v.x < map->Size.x; v.x++)
	{
		for (v.y = 0; v.y < map->Size.y; v.y++)
		{
			MapSetupTile(map, v, m);
		}
	}

	// Randomly change normal floor tiles to alternative floor tiles
	for (int i = 0; i < map->Size.x*map->Size.y / 22; i++)
	{
		Tile *t = MapGetTile(
			map, Vec2iNew(rand() % map->Size.x, rand() % map->Size.y));
		if (TileIsNormalFloor(t))
		{
			TileSetAlternateFloor(t, PicManagerGetMaskedStylePic(
				&gPicManager, "tile", m->FloorStyle, "alt1",
				m->FloorMask, m->AltMask));
		}
	}
	for (int i = 0; i < map->Size.x*map->Size.y / 16; i++)
	{
		Tile *t = MapGetTile(
			map, Vec2iNew(rand() % map->Size.x, rand() % map->Size.y));
		if (TileIsNormalFloor(t))
		{
			TileSetAlternateFloor(t, PicManagerGetMaskedStylePic(
				&gPicManager, "tile", m->FloorStyle, "alt2",
				m->FloorMask, m->AltMask));
		}
	}
}
static const char *MapGetWallPic(const Map *m, const Vec2i pos);
// Set tile properties for a map tile, such as picture to use
static void MapSetupTile(Map *map, const Vec2i pos, const Mission *m)
{
	Tile *tAbove = MapGetTile(map, Vec2iNew(pos.x, pos.y - 1));
	bool canSeeTileAbove = !(tAbove != NULL && !TileCanSee(tAbove));
	Tile *t = MapGetTile(map, pos);
	if (!t)
	{
		return;
	}
	switch (IMapGet(map, pos) & MAP_MASKACCESS)
	{
	case MAP_FLOOR:
	case MAP_SQUARE:
		t->pic = PicManagerGetMaskedStylePic(
			&gPicManager, "tile", m->FloorStyle,
			canSeeTileAbove ? "normal" : "shadow",
			m->FloorMask, m->AltMask);
		if (canSeeTileAbove)
		{
			// Normal floor tiles can be replaced randomly with
			// special floor tiles such as drainage
			t->flags |= MAPTILE_IS_NORMAL_FLOOR;
		}
		break;

	case MAP_ROOM:
	case MAP_DOOR:
		t->pic = PicManagerGetMaskedStylePic(
			&gPicManager, "tile", m->RoomStyle,
			canSeeTileAbove ? "normal" : "shadow",
			m->RoomMask, m->AltMask);
		break;

	case MAP_WALL:
		t->pic = PicManagerGetMaskedStylePic(
			&gPicManager, "wall", m->WallStyle, MapGetWallPic(map, pos),
			m->WallMask, m->AltMask);
		t->flags =
			MAPTILE_NO_WALK | MAPTILE_NO_SHOOT |
			MAPTILE_NO_SEE | MAPTILE_IS_WALL;
		break;

	case MAP_NOTHING:
		t->pic = NULL;
		t->flags =
			MAPTILE_NO_WALK | MAPTILE_IS_NOTHING;
		break;
	}
}
static int W(const Map *map, const int x, const int y);
static const char *MapGetWallPic(const Map *m, const Vec2i pos)
{
	const int x = pos.x;
	const int y = pos.y;
	if (W(m, x - 1, y) && W(m, x + 1, y) && W(m, x, y + 1) && W(m, x, y - 1))
	{
		return "x";
	}
	if (W(m, x - 1, y) && W(m, x + 1, y) && W(m, x, y + 1))
	{
		return "nt";
	}
	if (W(m, x - 1, y) && W(m, x + 1, y) && W(m, x, y - 1))
	{
		return "st";
	}
	if (W(m, x - 1, y) && W(m, x, y + 1) && W(m, x, y - 1))
	{
		return "et";
	}
	if (W(m, x + 1, y) && W(m, x, y + 1) && W(m, x, y - 1))
	{
		return "wt";
	}
	if (W(m, x + 1, y) && W(m, x, y + 1))
	{
		return "nw";
	}
	if (W(m, x + 1, y) && W(m, x, y - 1))
	{
		return "sw";
	}
	if (W(m, x - 1, y) && W(m, x, y + 1))
	{
		return "ne";
	}
	if (W(m, x - 1, y) && W(m, x, y - 1))
	{
		return "se";
	}
	if (W(m, x - 1, y) && W(m, x + 1, y))
	{
		return "h";
	}
	if (W(m, x, y + 1) && W(m, x, y - 1))
	{
		return "v";
	}
	if (W(m, x, y + 1))
	{
		return "n";
	}
	if (W(m, x, y - 1))
	{
		return "s";
	}
	if (W(m, x + 1, y))
	{
		return "w";
	}
	if (W(m, x - 1, y))
	{
		return "e";
	}
	return "o";
}
static int W(const Map *map, const int x, const int y)
{
	return IMapGet(map, Vec2iNew(x, y)) == MAP_WALL;
}

int MapIsValidStartForWall(
	Map *map, int x, int y, unsigned short tileType, int pad)
{
	Vec2i d;
	if (x == 0 || y == 0 || x == map->Size.x - 1 || y == map->Size.y - 1)
	{
		return 0;
	}
	for (d.x = x - pad; d.x <= x + pad; d.x++)
	{
		for (d.y = y - pad; d.y <= y + pad; d.y++)
		{
			if (IMapGet(map, d) != tileType)
			{
				return 0;
			}
		}
	}
	return 1;
}

Vec2i MapGetRandomTile(const Map *map)
{
	return Vec2iNew(rand() % map->Size.x, rand() % map->Size.y);
}

void MapMakeRoom(Map *map, int xOrigin, int yOrigin, int width, int height)
{
	int x, y;
	// Set the perimeter walls and interior
	// If the tile is a room interior already, do not turn it into a wall
	// This is due to overlapping rooms
	for (y = yOrigin; y < yOrigin + height; y++)
	{
		for (x = xOrigin; x < xOrigin + width; x++)
		{
			if (y == yOrigin || y == yOrigin + height - 1 ||
				x == xOrigin || x == xOrigin + width - 1)
			{
				if (IMapGet(map, Vec2iNew(x, y)) == MAP_FLOOR)
				{
					IMapSet(map, Vec2iNew(x, y), MAP_WALL);
				}
			}
			else
			{
				IMapSet(map, Vec2iNew(x, y), MAP_ROOM);
			}
		}
	}
	// Check perimeter again; if there are walls where both sides contain
	// rooms, remove the wall as the rooms have merged
	for (y = yOrigin; y < yOrigin + height; y++)
	{
		for (x = xOrigin; x < xOrigin + width; x++)
		{
			if (y == yOrigin || y == yOrigin + height - 1 ||
				x == xOrigin || x == xOrigin + width - 1)
			{
				if (((IMapGet(map, Vec2iNew(x + 1, y)) & MAP_MASKACCESS) == MAP_ROOM &&
					(IMapGet(map, Vec2iNew(x - 1, y)) & MAP_MASKACCESS) == MAP_ROOM) ||
					((IMapGet(map, Vec2iNew(x, y + 1)) & MAP_MASKACCESS) == MAP_ROOM &&
					(IMapGet(map, Vec2iNew(x, y - 1)) & MAP_MASKACCESS) == MAP_ROOM))
				{
					IMapSet(map, Vec2iNew(x, y), MAP_ROOM);
				}
			}
		}
	}
}
static bool TryPlaceDoorTile(
	Map *map, const Vec2i v, const Vec2i d, const unsigned short t);
void MapPlaceDoors(
	Map *map, Vec2i pos, Vec2i size,
	int hasDoors, int doors[4], int doorMin, int doorMax,
	unsigned short accessMask)
{
	int x, y;
	int i;
	unsigned short doorTile = hasDoors ? MAP_DOOR : MAP_ROOM;
	Vec2i v;

	// Set access mask
	for (y = pos.y + 1; y < pos.y + size.y - 1; y++)
	{
		for (x = pos.x + 1; x < pos.x + size.x - 1; x++)
		{
			if ((IMapGet(map, Vec2iNew(x, y)) & MAP_MASKACCESS) == MAP_ROOM)
			{
				IMapSet(map, Vec2iNew(x, y), MAP_ROOM | accessMask);
			}
		}
	}

	// Set the doors
	if (doors[0])
	{
		int doorSize = MIN(
			(doorMax > doorMin ? (rand() % (doorMax - doorMin + 1)) : 0) + doorMin,
			size.y - 4);
		for (i = -doorSize / 2; i < (doorSize + 1) / 2; i++)
		{
			v = Vec2iNew(pos.x, pos.y + size.y / 2 + i);
			if (!TryPlaceDoorTile(map, v, Vec2iNew(1, 0), doorTile))
			{
				break;
			}
		}
	}
	if (doors[1])
	{
		int doorSize = MIN(
			(doorMax > doorMin ? (rand() % (doorMax - doorMin + 1)) : 0) + doorMin,
			size.y - 4);
		for (i = -doorSize / 2; i < (doorSize + 1) / 2; i++)
		{
			v = Vec2iNew(pos.x + size.x - 1, pos.y + size.y / 2 + i);
			if (!TryPlaceDoorTile(map, v, Vec2iNew(1, 0), doorTile))
			{
				break;
			}
		}
	}
	if (doors[2])
	{
		int doorSize = MIN(
			(doorMax > doorMin ? (rand() % (doorMax - doorMin + 1)) : 0) + doorMin,
			size.x - 4);
		for (i = -doorSize / 2; i < (doorSize + 1) / 2; i++)
		{
			v = Vec2iNew(pos.x + size.x / 2 + i, pos.y);
			if (!TryPlaceDoorTile(map, v, Vec2iNew(0, 1), doorTile))
			{
				break;
			}
		}
	}
	if (doors[3])
	{
		int doorSize = MIN(
			(doorMax > doorMin ? (rand() % (doorMax - doorMin + 1)) : 0) + doorMin,
			size.x - 4);
		for (i = -doorSize / 2; i < (doorSize + 1) / 2; i++)
		{
			v = Vec2iNew(pos.x + size.x / 2 + i, pos.y + size.y - 1);
			if (!TryPlaceDoorTile(map, v, Vec2iNew(0, 1), doorTile))
			{
				break;
			}
		}
	}
}
static bool TryPlaceDoorTile(
	Map *map, const Vec2i v, const Vec2i d, const unsigned short t)
{
	if (IMapGet(map, v) == MAP_WALL &&
		IMapGet(map, Vec2iAdd(v, d)) != MAP_WALL &&
		IMapGet(map, Vec2iMinus(v, d)) != MAP_WALL)
	{
		IMapSet(map, v, t);
		return true;
	}
	return false;
}

bool MapIsAreaInside(const Map *map, const Vec2i pos, const Vec2i size)
{
	return pos.x >= 0 && pos.y >= 0 &&
		pos.x + size.x < map->Size.x && pos.y + size.y < map->Size.y;
}

int MapIsAreaClear(Map *map, Vec2i pos, Vec2i size)
{
	if (!MapIsAreaInside(map, pos, size))
	{
		return 0;
	}
	Vec2i v;
	for (v.y = pos.y; v.y < pos.y + size.y; v.y++)
	{
		for (v.x = pos.x; v.x < pos.x + size.x; v.x++)
		{
			if (IMapGet(map, v) != MAP_FLOOR)
			{
				return 0;
			}
		}
	}

	return 1;
}
static int MapTileIsPartOfRoom(Map *map, Vec2i pos)
{
	Vec2i v2;
	int isRoom = 0;
	int isFloor = 0;
	// Find whether a wall tile is part of a room perimeter
	// The surrounding tiles must have normal floor and room tiles
	// to be a perimeter
	for (v2.y = pos.y - 1; v2.y <= pos.y + 1; v2.y++)
	{
		for (v2.x = pos.x - 1; v2.x <= pos.x + 1; v2.x++)
		{
			if ((IMapGet(map, v2) & MAP_MASKACCESS) == MAP_ROOM)
			{
				isRoom = 1;
			}
			else if ((IMapGet(map, v2) & MAP_MASKACCESS) == MAP_FLOOR)
			{
				isFloor = 1;
			}
		}
	}
	return isRoom && isFloor;
}
int MapIsAreaClearOrRoom(Map *map, Vec2i pos, Vec2i size)
{
	Vec2i v;

	if (pos.x < 0 || pos.y < 0 ||
		pos.x + size.x >= map->Size.x || pos.y + size.y >= map->Size.y)
	{
		return 0;
	}

	for (v.y = pos.y; v.y < pos.y + size.y; v.y++)
	{
		for (v.x = pos.x; v.x < pos.x + size.x; v.x++)
		{
			unsigned short tile = IMapGet(map, v) & MAP_MASKACCESS;
			switch (tile)
			{
			case MAP_FLOOR:	// fallthrough
			case MAP_ROOM:
				break;
			case MAP_WALL:
				// Check if this wall is part of a room
				if (!MapTileIsPartOfRoom(map, v))
				{
					return 0;
				}
				break;
			default:
				return 0;
			}
		}
	}

	return 1;
}
int MapIsAreaClearOrWall(Map *map, Vec2i pos, Vec2i size)
{
	Vec2i v;

	if (pos.x < 0 || pos.y < 0 ||
		pos.x + size.x >= map->Size.x || pos.y + size.y >= map->Size.y)
	{
		return 0;
	}

	for (v.y = pos.y; v.y < pos.y + size.y; v.y++)
	{
		for (v.x = pos.x; v.x < pos.x + size.x; v.x++)
		{
			switch (IMapGet(map, v) & MAP_MASKACCESS)
			{
			case MAP_FLOOR:
				break;
			case MAP_WALL:
				// need to check if this is not a room wall
				if (MapTileIsPartOfRoom(map, v))
				{
					return 0;
				}
				break;
			default:
				return 0;
			}
		}
	}

	return 1;
}
// Find the size of the passage created by the overlap of two rooms
// To find whether an overlap is valid,
// collect the perimeter walls that overlap
// Two possible cases where there is a valid overlap:
// - if there are exactly two overlapping perimeter walls
//   i.e.
//          X
// room 2 XXXXXX
//        X X     <-- all tiles between either belong to room 1 or room 2
//     XXXXXX
//        X    room 1
//
// - if the collection of overlapping tiles are contiguous
//   i.e.
//        X room 1 X
//     XXXXXXXXXXXXXXX
//     X     room 2  X
//
// In both cases, the overlap is valid if all tiles in between are room or
// perimeter tiles. The size of the passage is given by the largest difference
// in the x or y coordinates between the first and last intersection tiles,
// minus 1
int MapGetRoomOverlapSize(
	Map *map, Vec2i pos, Vec2i size, unsigned short *overlapAccess)
{
	Vec2i v;
	int numOverlaps = 0;
	Vec2i overlapMin = Vec2iZero();
	Vec2i overlapMax = Vec2iZero();

	if (pos.x < 0 || pos.y < 0 ||
		pos.x + size.x >= map->Size.x || pos.y + size.y >= map->Size.y)
	{
		return 0;
	}

	// Find perimeter tiles that overlap
	for (v.y = pos.y; v.y < pos.y + size.y; v.y++)
	{
		for (v.x = pos.x; v.x < pos.x + size.x; v.x++)
		{
			// only check perimeter
			if (v.x != pos.x && v.x != pos.x + size.x - 1 &&
				v.y != pos.y && v.y != pos.y + size.y - 1)
			{
				continue;
			}
			switch (IMapGet(map, v))
			{
			case MAP_WALL:
				// Check if this wall is part of a room
				if (MapTileIsPartOfRoom(map, v))
				{
					// Get the access level of the room
					Vec2i v2;
					for (v2.y = v.y - 1; v2.y <= v.y + 1; v2.y++)
					{
						for (v2.x = v.x - 1; v2.x <= v.x + 1; v2.x++)
						{
							if ((IMapGet(map, v2) & MAP_MASKACCESS) == MAP_ROOM)
							{
								*overlapAccess |=
									IMapGet(map, v2) & MAP_ACCESSBITS;
							}
						}
					}
					if (numOverlaps == 0)
					{
						overlapMin = overlapMax = v;
					}
					else
					{
						overlapMin = Vec2iMin(overlapMin, v);
						overlapMax = Vec2iMax(overlapMax, v);
					}
					numOverlaps++;
				}
				break;
			default:
				break;
			}
		}
	}
	if (numOverlaps < 2)
	{
		return 0;
	}

	// Now check that all tiles between the first and last tiles are room or
	// perimeter tiles
	for (v.y = overlapMin.y; v.y <= overlapMax.y; v.y++)
	{
		for (v.x = overlapMin.x; v.x <= overlapMax.x; v.x++)
		{
			switch (IMapGet(map, v) & MAP_MASKACCESS)
			{
			case MAP_ROOM:
				break;
			case MAP_WALL:
				// Check if this wall is part of a room
				if (!MapTileIsPartOfRoom(map, v))
				{
					return 0;
				}
				break;
			default:
				// invalid tile type
				return 0;
			}
		}
	}

	return MAX(overlapMax.x - overlapMin.x, overlapMax.y - overlapMin.y) - 1;
}
// Check that this area does not overlap two or more "walls"
int MapIsLessThanTwoWallOverlaps(Map *map, Vec2i pos, Vec2i size)
{
	Vec2i v;
	int numOverlaps = 0;
	Vec2i overlapMin = Vec2iZero();
	Vec2i overlapMax = Vec2iZero();

	if (pos.x < 0 || pos.y < 0 ||
		pos.x + size.x >= map->Size.x || pos.y + size.y >= map->Size.y)
	{
		return 0;
	}

	for (v.y = pos.y; v.y < pos.y + size.y; v.y++)
	{
		for (v.x = pos.x; v.x < pos.x + size.x; v.x++)
		{
			// only check perimeter
			if (v.x != pos.x && v.x != pos.x + size.x - 1 &&
				v.y != pos.y && v.y != pos.y + size.y - 1)
			{
				continue;
			}
			switch (IMapGet(map, v))
			{
			case MAP_WALL:
				// Check if this wall is part of a room
				if (!MapTileIsPartOfRoom(map, v))
				{
					if (numOverlaps == 0)
					{
						overlapMin = overlapMax = v;
					}
					else
					{
						overlapMin = Vec2iMin(overlapMin, v);
						overlapMax = Vec2iMax(overlapMax, v);
					}
					numOverlaps++;
				}
				break;
			default:
				break;
			}
		}
	}
	if (numOverlaps < 2)
	{
		return 1;
	}

	// Now check that all tiles between the first and last tiles are
	// pillar tiles
	for (v.y = overlapMin.y; v.y <= overlapMax.y; v.y++)
	{
		for (v.x = overlapMin.x; v.x <= overlapMax.x; v.x++)
		{
			switch (IMapGet(map, v) & MAP_MASKACCESS)
			{
			case MAP_WALL:
				// Check if this wall is not part of a room
				if (MapTileIsPartOfRoom(map, v))
				{
					return 0;
				}
				break;
			default:
				// invalid tile type
				return 0;
			}
		}
	}

	return 1;
}

void MapMakeSquare(Map *map, Vec2i pos, Vec2i size)
{
	Vec2i v;
	for (v.y = pos.y; v.y < pos.y + size.y; v.y++)
	{
		for (v.x = pos.x; v.x < pos.x + size.x; v.x++)
		{
			IMapSet(map, v, MAP_SQUARE);
		}
	}
}
void MapMakePillar(Map *map, Vec2i pos, Vec2i size)
{
	Vec2i v;
	for (v.y = pos.y; v.y < pos.y + size.y; v.y++)
	{
		for (v.x = pos.x; v.x < pos.x + size.x; v.x++)
		{
			IMapSet(map, v, MAP_WALL);
		}
	}
}

unsigned short GenerateAccessMask(int *accessLevel)
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

void MapGenerateRandomExitArea(Map *map)
{
	const Tile *t = NULL;
	for (int i = 0; i < 10000 && (t == NULL ||!TileCanWalk(t)); i++)
	{
		map->ExitStart.x = (rand() % (abs(map->Size.x) - EXIT_WIDTH - 1));
		map->ExitEnd.x = map->ExitStart.x + EXIT_WIDTH + 1;
		map->ExitStart.y = (rand() % (abs(map->Size.y) - EXIT_HEIGHT - 1));
		map->ExitEnd.y = map->ExitStart.y + EXIT_HEIGHT + 1;
		// Check that the exit area is walkable
		const Vec2i center = Vec2iNew(
			(map->ExitStart.x + map->ExitEnd.x) / 2,
			(map->ExitStart.y + map->ExitEnd.y) / 2);
		t = MapGetTile(map, center);
	}
}
