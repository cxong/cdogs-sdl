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
#include "map.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "collision.h"
#include "config.h"
#include "pic_manager.h"
#include "objs.h"
#include "triggers.h"
#include "sounds.h"
#include "actors.h"
#include "gamedata.h"
#include "mission.h"
#include "utils.h"

// Values for internal map
#define MAP_FLOOR           0
#define MAP_WALL            1
#define MAP_DOOR            2
#define MAP_ROOM            3
#define MAP_SQUARE          4
#define MAP_NOTHING         6

#define MAP_ACCESS_RED      256
#define MAP_ACCESS_BLUE     512
#define MAP_ACCESS_GREEN    1024
#define MAP_ACCESS_YELLOW   2048
#define MAP_LEAVEFREE       4096
#define MAP_MASKACCESS      0xFF
#define MAP_ACCESSBITS      0x0F00

#define KEY_W 9
#define KEY_H 5
#define COLLECTABLE_W 4
#define COLLECTABLE_H 3


Map gMap;


static void AddItemToTile(TTileItem * t, Tile * tile)
{
	t->next = tile->things;
	tile->things = t;
}

static void RemoveItemFromTile(TTileItem * t, Tile * tile)
{
	TTileItem **h = &tile->things;

	while (*h && *h != t)
		h = &(*h)->next;
	if (*h) {
		*h = t->next;
		t->next = NULL;
	}
}

Tile *MapGetTile(Map *map, Vec2i pos)
{
	return &map->tiles[pos.y][pos.x];
}

int MapIsTileIn(Map *map, Vec2i pos)
{
	UNUSED(map);
	// Check that the tile pos is within the interior of the map
	// Note that the map always has a 1-wide perimeter
	// TODO: factor in different map sizes
	if (pos.x <= 0 || pos.y <= 0 || pos.x >= XMAX - 1 || pos.y >= YMAX - 1)
	{
		return 0;
	}
	return 1;
}

static Tile *MapGetTileOfItem(Map *map, TTileItem *t)
{
	Vec2i pos = Vec2iToTile(Vec2iNew(t->x, t->y));
	return MapGetTile(map, pos);
}

void MapMoveTileItem(Map *map, TTileItem *t, Vec2i pos)
{
	Vec2i t1 = Vec2iToTile(Vec2iNew(t->x, t->y));
	Vec2i t2 = Vec2iToTile(pos);

	t->x = pos.x;
	t->y = pos.y;
	if (Vec2iEqual(t1, t2))
	{
		return;
	}

	RemoveItemFromTile(t, MapGetTile(map, t1));
	AddItemToTile(t, MapGetTile(map, t2));
}

void MapRemoveTileItem(Map *map, TTileItem *t)
{
	Tile *tile = MapGetTileOfItem(map, t);
	RemoveItemFromTile(t, tile);
}

static Vec2i GuessCoords(struct MissionOptions *mo)
{
	Vec2i v;
	if (mo->missionData->Size.x)
	{
		v.x = (rand() % mo->missionData->Size.x) +
			(XMAX - mo->missionData->Size.x) / 2;
	}
	else
	{
		v.x = rand() % XMAX;
	}

	if (mo->missionData->Size.y)
	{
		v.y = (rand() % mo->missionData->Size.y) +
			(YMAX - mo->missionData->Size.y) / 2;
	}
	else
	{
		v.y = rand() % YMAX;
	}
	return v;
}

static Vec2i GuessPixelCoords(struct MissionOptions *mo)
{
	Vec2i v;
	if (mo->missionData->Size.x)
	{
		v.x = (rand() % (mo->missionData->Size.x * TILE_WIDTH)) +
			(XMAX - mo->missionData->Size.x) * TILE_WIDTH / 2;
	}
	else
	{
		v.x = rand() % (XMAX * TILE_WIDTH);
	}

	if (mo->missionData->Size.y)
	{
		v.y = (rand() % (mo->missionData->Size.y * TILE_HEIGHT)) +
			(YMAX - mo->missionData->Size.y) * TILE_HEIGHT / 2;
	}
	else
	{
		v.y = rand() % (YMAX * TILE_HEIGHT);
	}
	return v;
}

static unsigned short IMapGet(Map *map, Vec2i pos)
{
	return map->iMap[pos.y][pos.x];
}
static void IMapSet(Map *map, Vec2i pos, unsigned short v)
{
	map->iMap[pos.y][pos.x] = v;
}

static void MapGrow(Map *map, int x, int y, int d, int length)
{
	int l;

	if (length <= 0)
		return;

	switch (d) {
	case 0:
		if (y < 3 ||
			IMapGet(map, Vec2iNew(x - 1, y - 1)) ||
			IMapGet(map, Vec2iNew(x + 1, y - 1)) ||
			IMapGet(map, Vec2iNew(x - 1, y - 2)) ||
			IMapGet(map, Vec2iNew(x, y - 2)) ||
			IMapGet(map, Vec2iNew(x + 1, y - 2)))
		{
			return;
		}
		y--;
		break;
	case 1:
		if (x > XMAX - 3 ||
			IMapGet(map, Vec2iNew(x + 1, y - 1)) ||
			IMapGet(map, Vec2iNew(x + 1, y + 1)) ||
			IMapGet(map, Vec2iNew(x + 2, y - 1)) ||
			IMapGet(map, Vec2iNew(x + 2, y)) ||
			IMapGet(map, Vec2iNew(x + 2, y + 1)))
		{
			return;
		}
		x++;
		break;
	case 2:
		if (y > YMAX - 3 ||
			IMapGet(map, Vec2iNew(x - 1, y + 1)) ||
			IMapGet(map, Vec2iNew(x + 1, y + 1)) ||
			IMapGet(map, Vec2iNew(x - 1, y + 2)) ||
			IMapGet(map, Vec2iNew(x, y + 2)) ||
			IMapGet(map, Vec2iNew(x + 1, y + 2)))
		{
			return;
		}
		y++;
		break;
	case 4:
		if (x < 3 ||
			IMapGet(map, Vec2iNew(x - 1, y - 1)) ||
			IMapGet(map, Vec2iNew(x - 1, y + 1)) ||
			IMapGet(map, Vec2iNew(x - 2, y - 1)) ||
			IMapGet(map, Vec2iNew(x - 2, y)) ||
			IMapGet(map, Vec2iNew(x - 2, y + 1)))
		{
			return;
		}
		x--;
		break;
	}
	IMapSet(map, Vec2iNew(x, y), MAP_WALL);
	length--;
	if (length > 0 && (rand() & 3) == 0) {
		l = rand() % length;
		MapGrow(map, x, y, rand() & 3, l);
		length -= l;
	}
	MapGrow(map, x, y, d, length);
}

static int MapIsValidStartForWall(Map *map, int x, int y)
{
	if (x == 0 || y == 0 || x == XMAX - 1 || y == YMAX - 1)
	{
		return 1;
	}
	if (IMapGet(map, Vec2iNew(x - 1, y - 1)) == 0 &&
		IMapGet(map, Vec2iNew(x, y - 1)) == 0 &&
		IMapGet(map, Vec2iNew(x + 1, y - 1)) == 0 &&
		IMapGet(map, Vec2iNew(x - 1, y)) == 0 &&
		IMapGet(map, Vec2iNew(x, y)) == 0 &&
		IMapGet(map, Vec2iNew(x + 1, y)) == 0 &&
		IMapGet(map, Vec2iNew(x - 1, y + 1)) == 0 &&
		IMapGet(map, Vec2iNew(x, y + 1)) == 0 &&
		IMapGet(map, Vec2iNew(x + 1, y + 1)) == 0)
	{
		return 1;
	}
	return 0;
}

static int MapTryBuildWall(Map *map, int wallLength)
{
	Vec2i v = GuessCoords(&gMission);
	if (MapIsValidStartForWall(map, v.x, v.y))
	{
		IMapSet(map, v, MAP_WALL);
		MapGrow(map, v.x, v.y, rand() & 3, wallLength);
		return 1;
	}
	return 0;
}

static void MapMakeRoom(
	Map *map,
	int xOrigin, int yOrigin, int width, int height, int doors,
	unsigned short access_mask)
{
	int x, y;

	// Set the perimeter walls and interior
	for (y = yOrigin; y <= yOrigin + height; y++)
	{
		IMapSet(map, Vec2iNew(xOrigin, y), MAP_WALL);
		IMapSet(map, Vec2iNew(xOrigin + width, y), MAP_WALL);
	}
	for (x = xOrigin + 1; x < xOrigin + width; x++)
	{
		IMapSet(map, Vec2iNew(x, yOrigin), MAP_WALL);
		IMapSet(map, Vec2iNew(x, yOrigin + height), MAP_WALL);
		for (y = yOrigin + 1; y < yOrigin + height; y++)
		{
			IMapSet(map, Vec2iNew(x, y), MAP_ROOM | access_mask);
		}
	}

	// Set the doors
	if (doors & 1)
	{
		IMapSet(map, Vec2iNew(xOrigin, yOrigin + height / 2), MAP_DOOR);
	}
	if (doors & 2)
	{
		IMapSet(map, Vec2iNew(xOrigin + width, yOrigin + height / 2), MAP_DOOR);
	}
	if (doors & 4)
	{
		IMapSet(map, Vec2iNew(xOrigin + width / 2, yOrigin), MAP_DOOR);
	}
	if (doors & 8)
	{
		IMapSet(map, Vec2iNew(xOrigin + width / 2, yOrigin + height), MAP_DOOR);
	}
}

static int MapIsAreaClear(Map *map, Vec2i pos, Vec2i size)
{
	Vec2i v;

	if (pos.x < 0 || pos.y < 0 ||
		pos.x + size.x >= XMAX || pos.y + size.y >= YMAX)
	{
		return 0;
	}

	for (v.y = pos.y; v.y <= pos.y + size.y; v.y++)
	{
		for (v.x = pos.x; v.x <= pos.x + size.x; v.x++)
		{
			if (IMapGet(map, v) != MAP_FLOOR)
			{
				return 0;
			}
		}
	}

	return 1;
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

static int MapBuildRoom(Map *map, int hasKeys)
{
	int w, h;
	Vec2i pos = GuessCoords(&gMission);
	w = rand() % 6 + 5;
	h = rand() % 6 + 5;

	if (MapIsAreaClear(map, Vec2iNew(pos.x - 1, pos.y - 1), Vec2iNew(w + 2, h + 2)))
	{
		unsigned short accessMask = 0;
		if (hasKeys)
		{
			accessMask = GenerateAccessMask(&map->keyAccessCount);
		}
		MapMakeRoom(map, pos.x, pos.y, w, h, rand() % 15 + 1, accessMask);
		if (hasKeys)
		{
			if (map->keyAccessCount < 1)
			{
				map->keyAccessCount = 1;
			}
		}
		return 1;
	}
	return 0;
}

static void MapMakeSquare(Map *map, Vec2i pos, Vec2i size)
{
	Vec2i v;
	for (v.y = pos.y; v.y <= pos.y + size.y; v.y++)
	{
		for (v.x = pos.x; v.x <= pos.x + size.x; v.x++)
		{
			IMapSet(map, v, MAP_SQUARE);
		}
	}
}

static int MapTryBuildSquare(Map *map)
{
	Vec2i v = GuessCoords(&gMission);
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

static int W(Map *map, int x, int y)
{
	return x >= 0 && y >= 0 && x < XMAX && y < YMAX &&
		IMapGet(map, Vec2iNew(x, y)) == MAP_WALL;
}

static int MapGetWallPic(Map *m, int x, int y)
{
	if (W(m, x - 1, y) && W(m, x + 1, y) && W(m, x, y + 1) && W(m, x, y - 1))
	{
		return WALL_CROSS;
	}
	if (W(m, x - 1, y) && W(m, x + 1, y) && W(m, x, y + 1))
	{
		return WALL_TOP_T;
	}
	if (W(m, x - 1, y) && W(m, x + 1, y) && W(m, x, y - 1))
	{
		return WALL_BOTTOM_T;
	}
	if (W(m, x - 1, y) && W(m, x, y + 1) && W(m, x, y - 1))
	{
		return WALL_RIGHT_T;
	}
	if (W(m, x + 1, y) && W(m, x, y + 1) && W(m, x, y - 1))
	{
		return WALL_LEFT_T;
	}
	if (W(m, x + 1, y) && W(m, x, y + 1))
	{
		return WALL_TOPLEFT;
	}
	if (W(m, x + 1, y) && W(m, x, y - 1))
	{
		return WALL_BOTTOMLEFT;
	}
	if (W(m, x - 1, y) && W(m, x, y + 1))
	{
		return WALL_TOPRIGHT;
	}
	if (W(m, x - 1, y) && W(m, x, y - 1))
	{
		return WALL_BOTTOMRIGHT;
	}
	if (W(m, x - 1, y) && W(m, x + 1, y))
	{
		return WALL_HORIZONTAL;
	}
	if (W(m, x, y + 1) && W(m, x, y - 1))
	{
		return WALL_VERTICAL;
	}
	if (W(m, x, y + 1))
	{
		return WALL_TOP;
	}
	if (W(m, x, y - 1))
	{
		return WALL_BOTTOM;
	}
	if (W(m, x + 1, y))
	{
		return WALL_LEFT;
	}
	if (W(m, x - 1, y))
	{
		return WALL_RIGHT;
	}
	return WALL_SINGLE;
}

static void PicLoadOffset(Pic **pic, Pic *picAlt, int idx)
{
	*pic = PicManagerGetFromOld(&gPicManager, idx);
	PicFromPicPalettedOffset(
		picAlt,
		PicManagerGetOldPic(&gPicManager, cGeneralPics[idx].picIndex),
		&cGeneralPics[idx]);
}

static void MapSetupTilesAndWalls(Map *map, int floor, int room, int wall)
{
	Vec2i v;
	int i;

	for (v.x = 0; v.x < XMAX; v.x++)
	{
		for (v.y = 0; v.y < YMAX; v.y++)
		{
			Tile *tAbove = MapGetTile(map, Vec2iNew(v.x, v.y - 1));
			int canSeeTileAbove = !(v.y > 0 && !TileCanSee(tAbove));
			Tile *t = MapGetTile(map, v);
			switch (IMapGet(map, v) & MAP_MASKACCESS)
			{
			case MAP_FLOOR:
			case MAP_SQUARE:
				if (!canSeeTileAbove)
				{
					t->pic = PicManagerGetFromOld(
						&gPicManager, cFloorPics[floor][FLOOR_SHADOW]);
				}
				else
				{
					t->pic = PicManagerGetFromOld(
						&gPicManager, cFloorPics[floor][FLOOR_NORMAL]);
					// Normal floor tiles can be replaced randomly with
					// special floor tiles such as drainage
					t->flags |= MAPTILE_IS_NORMAL_FLOOR;
				}
				break;

			case MAP_ROOM:
			case MAP_DOOR:
				if (!canSeeTileAbove)
				{
					t->pic = PicManagerGetFromOld(
						&gPicManager, cRoomPics[room][ROOMFLOOR_SHADOW]);
				}
				else
				{
					t->pic = PicManagerGetFromOld(
						&gPicManager, cRoomPics[room][ROOMFLOOR_NORMAL]);
				}
				break;

			case MAP_WALL:
				t->pic = PicManagerGetFromOld(
					&gPicManager,
					cWallPics[wall][MapGetWallPic(map, v.x, v.y)]);
				t->flags =
					MAPTILE_NO_WALK | MAPTILE_NO_SEE | MAPTILE_IS_WALL;
				break;

			case MAP_NOTHING:
				t->flags =
					MAPTILE_NO_WALK | MAPTILE_NO_SEE | MAPTILE_IS_NOTHING;
				break;
			}
		}
	}

	// Randomly change normal floor tiles to drainage tiles
	for (i = 0; i < 50; i++)
	{
		// Make sure drain tiles aren't next to each other
		Tile *t = MapGetTile(map, Vec2iNew(
			(rand() % XMAX) & 0xFFFFFE, (rand() % YMAX) & 0xFFFFFE));
		if (TileIsNormalFloor(t))
		{
			TileSetAlternateFloor(t, PicManagerGetFromOld(
				&gPicManager, PIC_DRAINAGE));
			t->flags |= MAPTILE_IS_DRAINAGE;
		}
	}

	// Randomly change normal floor tiles to alternative floor tiles
	for (i = 0; i < 100; i++)
	{
		Tile *t = MapGetTile(map, Vec2iNew(rand() % XMAX, rand() % YMAX));
		if (TileIsNormalFloor(t))
		{
			TileSetAlternateFloor(t, PicManagerGetFromOld(
				&gPicManager, cFloorPics[floor][FLOOR_1]));
		}
	}
	for (i = 0; i < 150; i++)
	{
		Tile *t = MapGetTile(map, Vec2iNew(rand() % XMAX, rand() % YMAX));
		if (TileIsNormalFloor(t))
		{
			TileSetAlternateFloor(t, PicManagerGetFromOld(
				&gPicManager, cFloorPics[floor][FLOOR_2]));
		}
	}
}

void MapChangeFloor(Map *map, Vec2i pos, Pic *normal, Pic *shadow)
{
	Tile *tAbove = MapGetTile(map, Vec2iNew(pos.x, pos.y - 1));
	int canSeeTileAbove = !(pos.y > 0 && !TileCanSee(tAbove));
	Tile *t = MapGetTile(map, pos);
	if (t->flags & MAPTILE_IS_DRAINAGE)
	{
		return;
	}
	switch (IMapGet(map, pos) & MAP_MASKACCESS)
	{
	case MAP_FLOOR:
	case MAP_SQUARE:
	case MAP_ROOM:
		if (!canSeeTileAbove)
		{
			t->pic = shadow;
		}
		else
		{
			t->pic = normal;
		}
		break;
	default:
		// do nothing
		break;
	}
}

void MapShowExitArea(Map *map)
{
	int left, right, top, bottom;
	Vec2i v;
	Pic *exitPic = PicManagerGetFromOld(&gPicManager, gMission.exitPic);
	Pic *shadowPic = PicManagerGetFromOld(&gPicManager, gMission.exitShadow);

	left = gMission.exitLeft;
	right = gMission.exitRight;
	top = gMission.exitTop;
	bottom = gMission.exitBottom;

	v.y = top;
	for (v.x = left; v.x <= right; v.x++)
	{
		MapChangeFloor(map, v, exitPic, shadowPic);
	}
	v.y = bottom;
	for (v.x = left; v.x <= right; v.x++)
	{
		MapChangeFloor(map, v, exitPic, shadowPic);
	}
	v.x = left;
	for (v.y = top + 1; v.y < bottom; v.y++)
	{
		MapChangeFloor(map, v, exitPic, shadowPic);
	}
	v.x = right;
	for (v.y = top + 1; v.y < bottom; v.y++)
	{
		MapChangeFloor(map, v, exitPic, shadowPic);
	}
}

// Adjacent means to the left, right, above or below
static int MapGetNumWallsAdjacentTile(Map *map, Vec2i v)
{
	int count = 0;
	if (MapIsTileIn(map, v))
	{
		if (!TileCanWalk(MapGetTile(map, Vec2iNew(v.x - 1, v.y))))
		{
			count++;
		}
		if (!TileCanWalk(MapGetTile(map, Vec2iNew(v.x + 1, v.y))))
		{
			count++;
		}
		if (!TileCanWalk(MapGetTile(map, Vec2iNew(v.x, v.y - 1))))
		{
			count++;
		}
		if (!TileCanWalk(MapGetTile(map, Vec2iNew(v.x, v.y + 1))))
		{
			count++;
		}
	}
	return count;
}

// Around means the 8 tiles surrounding the tile
static int MapGetNumWallsAroundTile(Map *map, Vec2i v)
{
	int count = MapGetNumWallsAdjacentTile(map, v);
	if (MapIsTileIn(map, v))
	{
		// Having checked the adjacencies, check the diagonals
		if (!TileCanWalk(MapGetTile(map, Vec2iNew(v.x - 1, v.y - 1))))
		{
			count++;
		}
		if (!TileCanWalk(MapGetTile(map, Vec2iNew(v.x + 1, v.y + 1))))
		{
			count++;
		}
		if (!TileCanWalk(MapGetTile(map, Vec2iNew(v.x + 1, v.y - 1))))
		{
			count++;
		}
		if (!TileCanWalk(MapGetTile(map, Vec2iNew(v.x - 1, v.y + 1))))
		{
			count++;
		}
	}
	return count;
}

static int MapPlaceOneObject(
	Map *map, Vec2i v, TMapObject * mo, int extraFlags)
{
	int f = mo->flags;
	int oFlags = 0;
	Vec2i realPos = Vec2iCenterOfTile(v);
	int tileFlags = 0;
	Tile *t = MapGetTile(map, v);
	unsigned short iMap = IMapGet(map, v);
	unsigned short iMapAccess = iMap & MAP_MASKACCESS;

	if ((t->flags & ~MAPTILE_IS_NORMAL_FLOOR) ||
		t->things != NULL ||
		(iMap & MAP_LEAVEFREE))
	{
		return 0;
	}

	if ((f & MAPOBJ_ROOMONLY) && iMapAccess != MAP_ROOM)
	{
		return 0;
	}

	if ((f & MAPOBJ_NOTINROOM) && iMapAccess == MAP_ROOM)
	{
		return 0;
	}

	if ((f & MAPOBJ_ON_WALL) &&
		(IMapGet(map, Vec2iNew(v.x, v.y - 1)) & MAP_MASKACCESS) != MAP_WALL)
	{
		return 0;
	}

	if ((f & MAPOBJ_FREEINFRONT) != 0 &&
		(IMapGet(map, Vec2iNew(v.x, v.y + 1)) & MAP_MASKACCESS) != MAP_ROOM &&
		(IMapGet(map, Vec2iNew(v.x, v.y + 1)) & MAP_MASKACCESS) != MAP_FLOOR)
	{
		return 0;
	}

	if ((f & MAPOBJ_ONEWALL) && MapGetNumWallsAdjacentTile(map, v) != 1)
	{
		return 0;
	}

	if ((f & MAPOBJ_ONEWALLPLUS) && MapGetNumWallsAdjacentTile(map, v) < 1)
	{
		return 0;
	}

	if ((f & MAPOBJ_NOWALLS) && MapGetNumWallsAroundTile(map, v) != 0)
	{
		return 0;
	}

	if (f & MAPOBJ_FREEINFRONT)
	{
		IMapSet(map, v, iMap | MAP_LEAVEFREE);
	}

	if (f & MAPOBJ_EXPLOSIVE)
	{
		oFlags |= OBJFLAG_EXPLOSIVE;
	}
	if (f & MAPOBJ_FLAMMABLE)
	{
		oFlags |= OBJFLAG_FLAMMABLE;
	}
	if (f & MAPOBJ_POISONOUS)
	{
		oFlags |= OBJFLAG_POISONOUS;
	}
	if (f & MAPOBJ_QUAKE)
	{
		oFlags |= OBJFLAG_QUAKE;
	}

	if (f & MAPOBJ_ON_WALL)
	{
		realPos.y -= TILE_HEIGHT / 2 + 1;
	}

	if (f & MAPOBJ_IMPASSABLE)
	{
		tileFlags |= TILEITEM_IMPASSABLE;
	}

	if (f & MAPOBJ_CANBESHOT)
	{
		tileFlags |= TILEITEM_CAN_BE_SHOT;
	}

	AddDestructibleObject(
		realPos,
		mo->width, mo->height,
		&cGeneralPics[mo->pic], &cGeneralPics[mo->wreckedPic],
		mo->structure,
		oFlags, tileFlags | extraFlags);
	return 1;
}

int MapHasLockedRooms(Map *map)
{
	return map->keyAccessCount > 1;
}

// TODO: rename this function
int MapPosIsHighAccess(Map *map, int x, int y)
{
	Vec2i tilePos = Vec2iToTile(Vec2iNew(x, y));
	return IMapGet(map, tilePos) & MAP_ACCESSBITS;
}

static int MapTryPlaceCollectible(
	Map *map, Mission *mission, struct MissionOptions *mo, int objective)
{
	MissionObjective *mobj = CArrayGet(&mission->Objectives, objective);
	int hasLockedRooms =
		(mobj->Flags & OBJECTIVE_HIACCESS) && MapHasLockedRooms(map);
	int noaccess = mobj->Flags & OBJECTIVE_NOACCESS;
	int i = (noaccess || hasLockedRooms) ? 1000 : 100;

	while (i)
	{
		Vec2i size = Vec2iNew(COLLECTABLE_W, COLLECTABLE_H);
		Vec2i v = GuessPixelCoords(mo);
		// Collectibles all have size 4x3
		if (!IsCollisionWithWall(v, size))
		{
			if ((!hasLockedRooms || MapPosIsHighAccess(map, v.x, v.y)) &&
				(!noaccess || !MapPosIsHighAccess(map, v.x, v.y)))
			{
				struct Objective *o = CArrayGet(&mo->Objectives, objective);
				AddObject(
					v.x << 8, v.y << 8, size,
					&cGeneralPics[o->pickupItem],
					OBJ_JEWEL,
					TILEITEM_CAN_BE_TAKEN | ObjectiveToTileItem(objective));
				return 1;
			}
		}
		i--;
	}
	return 0;
}

static int MapTryPlaceBlowup(
	Map *map, Mission *mission, struct MissionOptions *mo, int objective)
{
	MissionObjective *mobj = CArrayGet(&mission->Objectives, objective);
	int hasLockedRooms =
		(mobj->Flags & OBJECTIVE_HIACCESS) && MapHasLockedRooms(map);
	int noaccess = mobj->Flags & OBJECTIVE_NOACCESS;
	int i = (noaccess || hasLockedRooms) ? 1000 : 100;

	while (i > 0)
	{
		Vec2i v = GuessCoords(&gMission);
		if ((!hasLockedRooms || (IMapGet(map, v) >> 8)) &&
			(!noaccess || (IMapGet(map, v) >> 8) == 0))
		{
			struct Objective *o = CArrayGet(&mo->Objectives, objective);
			if (MapPlaceOneObject(
					map,
					v,
					o->blowupObject,
					ObjectiveToTileItem(objective)))
			{
				return 1;
			}
		}
		i--;
	}
	return 0;
}

static void MapPlaceCard(Map *map, int pic, int card, int map_access)
{
	for (;;)
	{
		Vec2i v = GuessCoords(&gMission);
		Tile *t;
		Tile *tBelow;
		unsigned short iMap;
		t = MapGetTile(map, v);
		iMap = IMapGet(map, v);
		tBelow = MapGetTile(map, Vec2iNew(v.x, v.y + 1));
		if (!(t->flags & ~MAPTILE_IS_NORMAL_FLOOR) &&
			t->things == NULL &&
			(iMap & 0xF00) == map_access &&
			(iMap & MAP_MASKACCESS) == MAP_ROOM &&
			!(tBelow->flags & ~MAPTILE_IS_NORMAL_FLOOR) &&
			tBelow->things == NULL)
		{
			Vec2i full = Vec2iReal2Full(Vec2iCenterOfTile(v));
			AddObject(
				full.x, full.y,
				Vec2iNew(KEY_W, KEY_H),
				&cGeneralPics[gMission.keyPics[pic]],
				card,
				(int)TILEITEM_CAN_BE_TAKEN);
			return;
		}
	}
}

static void MapAddDoor(Map *map, Vec2i v, int floor, int room, int flags)
{
	Trigger *t;
	TWatch *w;
	Condition *c;
	Action *a;
	int pic;
	int openDoorPic;
	int tileFlags =
		MAPTILE_NO_SEE | MAPTILE_NO_WALK |
		MAPTILE_NO_SHOOT | MAPTILE_OFFSET_PIC;
	Tile *tile = MapGetTile(map, v);
	struct DoorPic *dp;
	int isHorizontal = IMapGet(map, Vec2iNew(v.x - 1, v.y)) == MAP_WALL;

	// Tiles to either side of the door
	Tile *tileA;
	Tile *tileB;
	Vec2i vA;
	Vec2i vB;
	if (isHorizontal)
	{
		vA = Vec2iNew(v.x, v.y - 1);
		vB = Vec2iNew(v.x, v.y + 1);
	}
	else
	{
		vA = Vec2iNew(v.x - 1, v.y);
		vB = Vec2iNew(v.x + 1, v.y);
	}
	tileA = MapGetTile(map, vA);
	tileB = MapGetTile(map, vB);
	assert(TileCanWalk(tileA) && "map gen error: entrance should be clear");
	assert(TileCanWalk(tileB) && "map gen error: entrance should be clear");

	switch (flags)
	{
	case FLAGS_KEYCARD_RED:
		dp = &gMission.doorPics[4];
		break;
	case FLAGS_KEYCARD_BLUE:
		dp = &gMission.doorPics[3];
		break;
	case FLAGS_KEYCARD_GREEN:
		dp = &gMission.doorPics[2];
		break;
	case FLAGS_KEYCARD_YELLOW:
		dp = &gMission.doorPics[1];
		break;
	default:
		dp = &gMission.doorPics[0];
		break;
	}
	if (isHorizontal)
	{
		pic = dp->horzPic;
		openDoorPic = gMission.doorPics[5].horzPic;
	}
	else
	{
		pic = dp->vertPic;
		openDoorPic = gMission.doorPics[5].vertPic;
	}

	PicLoadOffset(&tile->pic, &tile->picAlt, pic);
	tile->flags = tileFlags;
	tileA->flags |= MAPTILE_TILE_TRIGGER;
	tileB->flags |= MAPTILE_TILE_TRIGGER;
	if (isHorizontal)
	{
		// Change the tile below to shadow, cast by this door
		if (IMapGet(map, vB) == MAP_FLOOR)
		{
			tileB->pic = PicManagerGetFromOld(
				&gPicManager, cFloorPics[floor][FLOOR_SHADOW]);
		}
		else
		{
			tileB->pic = PicManagerGetFromOld(
				&gPicManager, cRoomPics[room][ROOMFLOOR_SHADOW]);
		}
	}

	// Create the watch responsible for closing the door
	w = AddWatch(3, 5);

	// The conditions are that the tile above, at and below the door are empty
	c = w->conditions;
	c[0].condition = CONDITION_TILECLEAR;
	c[0].pos = vA;
	c[1].condition = CONDITION_TILECLEAR;
	c[1].pos = v;
	c[2].condition = CONDITION_TILECLEAR;
	c[2].pos = vB;

	// Now the actions of the watch once it's triggered
	a = w->actions;

	// Deactivate itself
	a[0].action = ACTION_DEACTIVATEWATCH;
	a[0].u.index = w->index;

	// Reenable trigger to the left of the door
	a[1].action = ACTION_SETTRIGGER;
	a[1].u.pos = vA;

	// Close door
	a[2].action = ACTION_CHANGETILE;
	a[2].u.pos = v;
	a[2].tilePic = PicManagerGetFromOld(&gPicManager, pic);
	if (tileFlags & MAPTILE_OFFSET_PIC)
	{
		PicLoadOffset(&a[2].tilePic, &a[2].tilePicAlt, pic);
	}
	a[2].tileFlags = tileFlags;

	if (isHorizontal)
	{
		// Add shadow below door (also reenables trigger)
		a[3].action = ACTION_CHANGETILE;
		a[3].u.pos = vB;
		if (IMapGet(map, vB) == MAP_FLOOR)
		{
			a[3].tilePic = PicManagerGetFromOld(
				&gPicManager, cFloorPics[floor][FLOOR_SHADOW]);
		}
		else
		{
			a[3].tilePic = PicManagerGetFromOld(
				&gPicManager, cRoomPics[room][ROOMFLOOR_SHADOW]);
		}
		a[3].tileFlags = MAPTILE_TILE_TRIGGER;
	}
	else
	{
		// Reenable trigger to the right of the door
		a[3].action = ACTION_SETTRIGGER;
		a[3].u.pos = vB;
	}

	a[4].action = ACTION_SOUND;
	a[4].u.pos = Vec2iCenterOfTile(v);
	a[4].tileFlags = SND_DOOR;

	// Add trigger to the left of the door
	t = AddTrigger(vA, 5);
	t->flags = flags;

	a = t->actions;

	// Enable the watch to close the door
	a[0].action = ACTION_ACTIVATEWATCH;
	a[0].u.index = w->index;

	// Deactivate itself
	a[1].action = ACTION_CLEARTRIGGER;
	a[1].u.pos = t->pos;

	// Open door
	a[2].action = ACTION_CHANGETILE;
	a[2].u.pos = v;
	if (isHorizontal)
	{
		a[2].tilePic = PicManagerGetFromOld(&gPicManager, openDoorPic);
	}
	else
	{
		PicLoadOffset(&a[2].tilePic, &a[2].tilePicAlt, openDoorPic);
	}
	a[2].tileFlags = isHorizontal ? 0 : MAPTILE_OFFSET_PIC;

	if (isHorizontal)
	{
		// Remove shadow below door (also clears trigger)
		a[3].action = ACTION_CHANGETILE;
		a[3].u.pos = vB;
		if (IMapGet(map, vB) == MAP_FLOOR)
		{
			a[3].tilePic = PicManagerGetFromOld(
				&gPicManager, cFloorPics[floor][FLOOR_NORMAL]);
		}
		else
		{
			a[3].tilePic = PicManagerGetFromOld(
				&gPicManager, cRoomPics[room][ROOMFLOOR_SHADOW]);
		}
		a[3].tileFlags = 0;
	}
	else
	{
		// Deactivate other trigger
		a[3].action = ACTION_CLEARTRIGGER;
		a[3].u.pos = vB;
	}

	a[4].action = ACTION_SOUND;
	a[4].u.pos = Vec2iCenterOfTile(v);
	a[4].tileFlags = SND_DOOR;

	// Add trigger to the right of the door with identical actions as this one
	t = AddTrigger(vB, 5);
	t->flags = flags;
	memcpy(t->actions, a, sizeof *t->actions * 5);
}

static int AccessCodeToFlags(int code)
{
	if (code & MAP_ACCESS_RED)
		return FLAGS_KEYCARD_RED;
	if (code & MAP_ACCESS_BLUE)
		return FLAGS_KEYCARD_BLUE;
	if (code & MAP_ACCESS_GREEN)
		return FLAGS_KEYCARD_GREEN;
	if (code & MAP_ACCESS_YELLOW)
		return FLAGS_KEYCARD_YELLOW;
	return 0;
}

static int MapGetAccessLevel(Map *map, int x, int y)
{
	return AccessCodeToFlags(IMapGet(map, Vec2iNew(x, y)));
}

// Need to check the flags around the door tile because it's the
// triggers that contain the right flags
// TODO: refactor door
int MapGetDoorKeycardFlag(Map *map, Vec2i pos)
{
	int l = MapGetAccessLevel(map, pos.x - 1, pos.y);
	if (l) return l;
	l = MapGetAccessLevel(map, pos.x + 1, pos.y);
	if (l) return l;
	l = MapGetAccessLevel(map, pos.x, pos.y - 1);
	if (l) return l;
	return MapGetAccessLevel(map, pos.x, pos.y + 1);
}

static int MapGetAccessFlags(Map *map, int x, int y)
{
	int flags;
	flags = AccessCodeToFlags(IMapGet(map, Vec2iNew(x - 1, y)));
	if (flags) return flags;
	flags = AccessCodeToFlags(IMapGet(map, Vec2iNew(x + 1, y)));
	if (flags) return flags;
	flags = AccessCodeToFlags(IMapGet(map, Vec2iNew(x, y - 1)));
	if (flags) return flags;
	flags = AccessCodeToFlags(IMapGet(map, Vec2iNew(x, y + 1)));
	if (flags) return flags;
	return 0;
}

static void MapSetupDoors(Map *map, int floor, int room)
{
	Vec2i v;
	for (v.x = 0; v.x < XMAX; v.x++)
	{
		for (v.y = 0; v.y < YMAX; v.y++)
		{
			if (IMapGet(map, v) == MAP_DOOR)
			{
				MapAddDoor(
					map, v, floor, room, MapGetAccessFlags(map, v.x, v.y));
			}
		}
	}
}

static void MapSetupPerimeter(Map *map, int w, int h)
{
	Vec2i v;
	int dx = 0, dy = 0;

	if (w && w < XMAX)
		dx = (XMAX - w) / 2;
	if (h && h < YMAX)
		dy = (YMAX - h) / 2;

	for (v.y = 0; v.y < YMAX; v.y++)
	{
		for (v.x = 0; v.x < XMAX; v.x++)
		{
			if (v.y < dy || v.y > dy + h - 1 ||
				v.x < dx || v.x > dx + w - 1)
			{
				IMapSet(map, v, MAP_NOTHING);
			}
			else if (v.y == dy || v.y == dy + h - 1 ||
				v.x == dx || v.x == dx + w - 1)
			{
				IMapSet(map, v, MAP_WALL);
			}
		}
	}
}

void MapLoad(Map *map, struct MissionOptions *mo)
{
	int i, j, count;
	Mission *mission = mo->missionData;
	int floor = mission->FloorStyle % FLOOR_STYLE_COUNT;
	int wall = mission->WallStyle % WALL_STYLE_COUNT;
	int room = mission->RoomStyle % ROOMFLOOR_COUNT;
	int x, y, w, h;
	Vec2i v;

	PicManagerGenerateOldPics(&gPicManager);
	memset(map, 0, sizeof *map);
	for (v.y = 0; v.y < YMAX; v.y++)
	{
		for (v.x = 0; v.x < XMAX; v.x++)
		{
			Tile *t = MapGetTile(map, v);
			t->pic = &picNone;
			t->picAlt = picNone;
		}
	}
	map->tilesSeen = 0;

	w = mission->Size.x && mission->Size.x < XMAX ? mission->Size.x : XMAX;
	h = mission->Size.y && mission->Size.y < YMAX ? mission->Size.y : YMAX;
	x = (XMAX - w) / 2;
	y = (YMAX - h) / 2;
	map->tilesTotal = w * h;

	MapSetupPerimeter(map, mission->Size.x, mission->Size.y);

	if (mission->Type == MAPTYPE_CLASSIC)
	{
		count = 0;
		i = 0;
		while (i < 1000 && count < mission->u.Classic.Squares)
		{
			if (MapTryBuildSquare(map))
			{
				count++;
			}
			i++;
		}

		map->keyAccessCount = 0;
		count = 0;
		i = 0;
		while (i < 1000 && count < mission->u.Classic.Rooms)
		{
			if (MapBuildRoom(map, AreKeysAllowed(gCampaign.Entry.mode)))
			{
				count++;
			}
			i++;
		}

		count = 0;
		i = 0;
		while (i < 1000 && count < mission->u.Classic.Walls)
		{
			if (MapTryBuildWall(map, mission->u.Classic.WallLength))
			{
				count++;
			}
			i++;
		}
	}
	else
	{
		assert(0 && "not implemented");
	}

	MapSetupTilesAndWalls(map, floor, room, wall);
	MapSetupDoors(map, floor, room);

	for (i = 0; i < (int)mo->MapObjects.size; i++)
	{
		int itemDensity = *(int *)CArrayGet(&mission->ItemDensities, i);
		for (j = 0; j < (itemDensity * map->tilesTotal) / 1000; j++)
		{
			TMapObject *mapObj = CArrayGet(&mo->MapObjects, i);
			MapPlaceOneObject(
				map, Vec2iNew(x + rand() % w, y + rand() % h), mapObj, 0);
		}
	}

	// Try to add the objectives
	// If we are unable to place them all, make sure to reduce the totals
	// in case we create missions that are impossible to complete
	for (i = 0, j = 0; i < (int)mission->Objectives.size; i++)
	{
		MissionObjective *mobj = CArrayGet(&mo->missionData->Objectives, i);
		if (mobj->Type != OBJECTIVE_COLLECT && mobj->Type != OBJECTIVE_DESTROY)
		{
			continue;
		}
		count = 0;
		if (mobj->Type == OBJECTIVE_COLLECT)
		{
			for (j = 0; j < mobj->Count; j++)
			{
				if (MapTryPlaceCollectible(map, mission, mo, i))
				{
					count++;
				}
			}
		}
		else if (mobj->Type == OBJECTIVE_DESTROY)
		{
			for (j = 0; j < mobj->Count; j++)
			{
				if (MapTryPlaceBlowup(map, mission, mo, i))
				{
					count++;
				}
			}
		}
		mobj->Count = count;
		if (mobj->Count < mobj->Required)
		{
			mobj->Required = mobj->Count;
		}
	}

	if (map->keyAccessCount >= 5)
	{
		MapPlaceCard(map, 3, OBJ_KEYCARD_RED, MAP_ACCESS_BLUE);
	}
	if (map->keyAccessCount >= 4)
	{
		MapPlaceCard(map, 2, OBJ_KEYCARD_BLUE, MAP_ACCESS_GREEN);
	}
	if (map->keyAccessCount >= 3)
	{
		MapPlaceCard(map, 1, OBJ_KEYCARD_GREEN, MAP_ACCESS_YELLOW);
	}
	if (map->keyAccessCount >= 2)
	{
		MapPlaceCard(map, 0, OBJ_KEYCARD_YELLOW, 0);
	}
}

int MapIsFullPosOKforPlayer(Map *map, int x, int y)
{
	Vec2i tilePos = Vec2iToTile(Vec2iFull2Real(Vec2iNew(x, y)));
	return IMapGet(map, tilePos) == 0;
}

void MapMarkAsVisited(Map *map, Vec2i pos)
{
	Tile *t = MapGetTile(map, pos);
	if (!t->isVisited)
	{
		map->tilesSeen++;
		t->isVisited = 1;
	}
}

void MapMarkAllAsVisited(Map *map)
{
	Vec2i pos;
	for (pos.y = 0; pos.y < YMAX; pos.y++)
	{
		for (pos.x = 0; pos.x < XMAX; pos.x++)
		{
			MapGetTile(map, pos)->isVisited = 1;
		}
	}
}

int MapGetExploredPercentage(Map *map)
{
	return (100 * map->tilesSeen) / map->tilesTotal;
}
