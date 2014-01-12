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
#include "map_build.h"


void MapMakeWall(Map *map, Vec2i pos)
{
	map->iMap[pos.y][pos.x] = MAP_WALL;
}

int MapIsValidStartForWall(
	Map *map, int x, int y, unsigned short tileType, int pad)
{
	Vec2i d;
	if (x == 0 || y == 0 || x == XMAX - 1 || y == YMAX - 1)
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
		for (i = -(doorSize - 1) / 2; i < (doorSize + 2) / 2; i++)
		{
			v = Vec2iNew(pos.x, pos.y + size.y / 2 + i);
			if (IMapGet(map, Vec2iNew(v.x + 1, v.y)) != MAP_WALL &&
				IMapGet(map, Vec2iNew(v.x - 1, v.y)) != MAP_WALL)
			{
				IMapSet(map, v, doorTile);
			}
		}
	}
	if (doors[1])
	{
		int doorSize = MIN(
			(doorMax > doorMin ? (rand() % (doorMax - doorMin + 1)) : 0) + doorMin,
			size.y - 4);
		for (i = -(doorSize - 1) / 2; i < (doorSize + 2) / 2; i++)
		{
			v = Vec2iNew(pos.x + size.x - 1, pos.y + size.y / 2 + i);
			if (IMapGet(map, Vec2iNew(v.x + 1, v.y)) != MAP_WALL &&
				IMapGet(map, Vec2iNew(v.x - 1, v.y)) != MAP_WALL)
			{
				IMapSet(map, v, doorTile);
			}
		}
	}
	if (doors[2])
	{
		int doorSize = MIN(
			(doorMax > doorMin ? (rand() % (doorMax - doorMin + 1)) : 0) + doorMin,
			size.x - 4);
		for (i = -(doorSize - 1) / 2; i < (doorSize + 2) / 2; i++)
		{
			v = Vec2iNew(pos.x + size.x / 2 + i, pos.y);
			if (IMapGet(map, Vec2iNew(v.x, v.y + 1)) != MAP_WALL &&
				IMapGet(map, Vec2iNew(v.x, v.y - 1)) != MAP_WALL)
			{
				IMapSet(map, v, doorTile);
			}
		}
	}
	if (doors[3])
	{
		int doorSize = MIN(
			(doorMax > doorMin ? (rand() % (doorMax - doorMin + 1)) : 0) + doorMin,
			size.x - 4);
		for (i = -(doorSize - 1) / 2; i < (doorSize + 2) / 2; i++)
		{
			v = Vec2iNew(pos.x + size.x / 2 + i, pos.y + size.y - 1);
			if (IMapGet(map, Vec2iNew(v.x, v.y + 1)) != MAP_WALL &&
				IMapGet(map, Vec2iNew(v.x, v.y - 1)) != MAP_WALL)
			{
				IMapSet(map, v, doorTile);
			}
		}
	}
}

int MapIsAreaClear(Map *map, Vec2i pos, Vec2i size)
{
	Vec2i v;

	if (pos.x < 0 || pos.y < 0 ||
		pos.x + size.x >= XMAX || pos.y + size.y >= YMAX)
	{
		return 0;
	}

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
		pos.x + size.x >= XMAX || pos.y + size.y >= YMAX)
	{
		return 0;
	}

	for (v.y = pos.y; v.y < pos.y + size.y; v.y++)
	{
		for (v.x = pos.x; v.x < pos.x + size.x; v.x++)
		{
			switch (IMapGet(map, v) & MAP_MASKACCESS)
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
		pos.x + size.x >= XMAX || pos.y + size.y >= YMAX)
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
	Vec2i overlap1 = Vec2iZero();
	Vec2i overlap2 = Vec2iZero();

	if (pos.x < 0 || pos.y < 0 ||
		pos.x + size.x >= XMAX || pos.y + size.y >= YMAX)
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
						overlap1 = v;
					}
					else
					{
						overlap2 = v;
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
	// Swap the x/y for convenience; this shouldn't change the overlap area
	if (overlap1.x > overlap2.x)
	{
		int temp = overlap1.x;
		overlap1.x = overlap2.x;
		overlap2.x = temp;
	}
	if (overlap1.y > overlap2.y)
	{
		int temp = overlap1.y;
		overlap1.y = overlap2.y;
		overlap2.y = temp;
	}
	for (v.y = overlap1.y; v.y <= overlap2.y; v.y++)
	{
		for (v.x = overlap1.x; v.x <= overlap2.x; v.x++)
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

	return MAX(overlap2.x - overlap1.x, overlap2.y - overlap1.y) - 1;
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
