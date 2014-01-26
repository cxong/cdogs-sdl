/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (c) 2014, Cong Xu
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
#include "map_static.h"

#include "campaigns.h"
#include "gamedata.h"
#include "map_build.h"


static void SetAccessLevels(Map *map);
void MapStaticLoad(Map *map, Mission *m)
{
	Vec2i v;
	for (v.y = 0; v.y < m->Size.y; v.y++)
	{
		for (v.x = 0; v.x < m->Size.x; v.x++)
		{
			int index = v.y * m->Size.x + v.x;
			unsigned short *tile = CArrayGet(&m->u.Static.Tiles, index);
			IMapSet(map, v, *tile);
		}
	}

	if (AreKeysAllowed(gCampaign.Entry.mode))
	{
		SetAccessLevels(map);
	}
}

// Results used by FloodFill
#define FLOOD_FILL_HAS_ROOMS 1
#define FLOOD_FILL_HAS_FLOORS 2
#define FLOOD_FILL_HAS_DOORS 4
static int FloodFill(
	Map *map, CArray *tileRoomNumbers, Vec2i v, int roomNumberCounter);
static void SetAccessLevels(Map *map)
{
	Vec2i v;
	CArray tileRoomNumbers;
	int roomNumberCounter = 1;
	CArray roomAccessMasks;
	int i;
	CArrayInit(&tileRoomNumbers, sizeof(int));
	CArrayInit(&roomAccessMasks, sizeof(unsigned short));
	for (v.y = 0; v.y < map->Size.y; v.y++)
	{
		for (v.x = 0; v.x < map->Size.x; v.x++)
		{
			int zero = 0;
			CArrayPushBack(&tileRoomNumbers, &zero);
		}
	}

	// Use flood fill to find all the rooms,
	// record room numbers per tile
	// To keep track of invalid rooms (rooms surrounded by walls and at least
	// one door) use the special room number -1 to denote invalid rooms
	for (v.y = 0; v.y < map->Size.y; v.y++)
	{
		for (v.x = 0; v.x < map->Size.x; v.x++)
		{
			// Perform a flood fill from this tile,
			// setting the room number
			int fillResult =
				FloodFill(map, &tileRoomNumbers, v, roomNumberCounter);
			if (fillResult & FLOOD_FILL_HAS_ROOMS)
			{
				if (fillResult == (FLOOD_FILL_HAS_ROOMS | FLOOD_FILL_HAS_DOORS))
				{
					roomNumberCounter++;
				}
				else
				{
					// this is an invalid room; refill with -1
					FloodFill(map, &tileRoomNumbers, v, -1);
				}
			}
		}
	}

	// for each room, generate an access level
	for (i = 0; i < roomNumberCounter - 1; i++)
	{
		unsigned short accessMask = GenerateAccessMask(&map->keyAccessCount);
		CArrayPushBack(&roomAccessMasks, &accessMask);
		if (map->keyAccessCount < 1)
		{
			map->keyAccessCount = 1;
		}
	}

	// Shuffle the access masks
	if (roomNumberCounter > 1)
	{
		for (i = 0; i < roomNumberCounter - 1 - 1; i++)
		{
			int j = i + rand() / (RAND_MAX / (roomNumberCounter - 1 - i) + 1);
			unsigned short temp =
				*(unsigned short *)CArrayGet(&roomAccessMasks, j);
			*(unsigned short *)CArrayGet(&roomAccessMasks, j) =
				*(unsigned short *)CArrayGet(&roomAccessMasks, i);
			*(unsigned short *)CArrayGet(&roomAccessMasks, i) = temp;
		}
	}

	// Apply the masks on the rooms
	for (v.y = 0; v.y < map->Size.y; v.y++)
	{
		for (v.x = 0; v.x < map->Size.x; v.x++)
		{
			int index = v.y * map->Size.x + v.x;
			int *roomNumber = CArrayGet(&tileRoomNumbers, index);
			if (*roomNumber > 0)
			{
				unsigned short *accessMask = CArrayGet(
					&roomAccessMasks, *roomNumber - 1);
				IMapSet(map, v, IMapGet(map, v) | *accessMask);
			}
		}
	}
}

static int FloodFill(
	Map *map, CArray *tileRoomNumbers, Vec2i v, int roomNumberCounter)
{
	int index = v.y * map->Size.x + v.x;
	int *roomNumber = CArrayGet(tileRoomNumbers, index);
	unsigned short tile = IMapGet(map, v);
	int result = 0;
	int doFillAnyway = roomNumberCounter == -1 && *roomNumber != -1;
	if (tile == MAP_ROOM && (*roomNumber == 0 || doFillAnyway))
	{
		result |= FLOOD_FILL_HAS_ROOMS;
		*roomNumber = roomNumberCounter;
		result |= FloodFill(
			map, tileRoomNumbers, Vec2iNew(v.x - 1, v.y), roomNumberCounter);
		result |= FloodFill(
			map, tileRoomNumbers, Vec2iNew(v.x + 1, v.y), roomNumberCounter);
		result |= FloodFill(
			map, tileRoomNumbers, Vec2iNew(v.x, v.y - 1), roomNumberCounter);
		result |= FloodFill(
			map, tileRoomNumbers, Vec2iNew(v.x, v.y + 1), roomNumberCounter);
	}
	else if (tile == MAP_FLOOR)
	{
		result |= FLOOD_FILL_HAS_FLOORS;
	}
	else if (tile == MAP_DOOR)
	{
		// Check that the door leads to floor
		if (IMapGet(map, Vec2iNew(v.x - 1, v.y)) == MAP_FLOOR ||
			IMapGet(map, Vec2iNew(v.x + 1, v.y)) == MAP_FLOOR ||
			IMapGet(map, Vec2iNew(v.x, v.y - 1)) == MAP_FLOOR ||
			IMapGet(map, Vec2iNew(v.x, v.y + 1)) == MAP_FLOOR)
		{
			result |= FLOOD_FILL_HAS_DOORS;
		}
	}
	return result;
}
