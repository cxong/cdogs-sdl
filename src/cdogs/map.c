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
#include "map_classic.h"
#include "map_static.h"
#include "pic_manager.h"
#include "objs.h"
#include "triggers.h"
#include "sounds.h"
#include "actors.h"
#include "gamedata.h"
#include "mission.h"
#include "utils.h"

#define MAP_LEAVEFREE       4096

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

static Vec2i GuessCoords(Vec2i mapSize)
{
	Vec2i v;
	if (mapSize.x)
	{
		v.x = (rand() % mapSize.x) + (XMAX - mapSize.x) / 2;
	}
	else
	{
		v.x = rand() % XMAX;
	}

	if (mapSize.y)
	{
		v.y = (rand() % mapSize.y) + (YMAX - mapSize.y) / 2;
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

unsigned short IMapGet(Map *map, Vec2i pos)
{
	if (pos.x < 0 || pos.x >= XMAX || pos.y < 0 || pos.y >= YMAX)
	{
		return MAP_NOTHING;
	}
	return map->iMap[pos.y][pos.x];
}
void IMapSet(Map *map, Vec2i pos, unsigned short v)
{
	map->iMap[pos.y][pos.x] = v;
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

static void PicLoadOffset(GraphicsDevice *g, Pic *picAlt, int idx)
{
	PicFromPicPalettedOffset(
		g,
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
		Vec2i v = GuessCoords(mission->Size);
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
		Vec2i v = GuessCoords(gMission.missionData->Size);
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

// Count the number of doors that are in the same group as this door
// Only check to the right/below
static int GetDoorCountInGroup(Map *map, Vec2i v, int isHorizontal)
{
	int count = 1;	// this tile
	Vec2i vNext;
	int dx = isHorizontal ? 1 : 0;
	int dy = isHorizontal ? 0 : 1;
	vNext = v;
	for (;;)
	{
		vNext = Vec2iNew(vNext.x + dx, vNext.y + dy);
		if (IMapGet(map, vNext) == MAP_DOOR)
		{
			count++;
		}
		else
		{
			break;
		}
	}
	return count;
}
// Create the watch responsible for closing the door
static TWatch *CreateCloseDoorWatch(
	Map *map, Vec2i v,
	int tileFlags, int isHorizontal, int doorGroupCount,
	int pic, int floor, int room)
{
	Action *a;
	TWatch *w = WatchNew();
	int i;
	Vec2i dv = Vec2iNew(isHorizontal ? 1 : 0, isHorizontal ? 0 : 1);
	Vec2i dAside = Vec2iNew(dv.y, dv.x);

	// The conditions are that the tile above, at and below the doors are empty
	for (i = 0; i < doorGroupCount; i++)
	{
		Vec2i vI = Vec2iNew(v.x + dv.x * i, v.y + dv.y * i);
		Condition *c = WatchAddCondition(w);
		c->condition = CONDITION_TILECLEAR;
		c->pos = Vec2iNew(vI.x - dAside.x, vI.y - dAside.y);
		c = WatchAddCondition(w);
		c->condition = CONDITION_TILECLEAR;
		c->pos = vI;
		c = WatchAddCondition(w);
		c->condition = CONDITION_TILECLEAR;
		c->pos = Vec2iNew(vI.x + dAside.x, vI.y + dAside.y);
	}

	// Now the actions of the watch once it's triggered

	// Deactivate itself
	a = WatchAddAction(w);
	a->action = ACTION_DEACTIVATEWATCH;
	a->u.index = w->index;
	// play sound at the center of the door group
	a = WatchAddAction(w);
	a->action = ACTION_SOUND;
	a->u.pos = Vec2iCenterOfTile(Vec2iNew(
		v.x + dv.x * doorGroupCount / 2,
		v.y + dv.y * doorGroupCount / 2));
	a->tileFlags = SND_DOOR;

	// Close doors
	for (i = 0; i < doorGroupCount; i++)
	{
		Vec2i vI = Vec2iNew(v.x + dv.x * i, v.y + dv.y * i);
		a = WatchAddAction(w);
		a->action = ACTION_CHANGETILE;
		a->u.pos = vI;
		a->tilePic = PicManagerGetFromOld(&gPicManager, pic);
		if (tileFlags & MAPTILE_OFFSET_PIC)
		{
			PicLoadOffset(&gGraphicsDevice, &a->tilePicAlt, pic);
			a->tilePic = PicManagerGetFromOld(
				&gPicManager, cRoomPics[room][ROOMFLOOR_SHADOW]);
		}
		a->tileFlags = tileFlags;
	}

	for (i = 0; i < doorGroupCount; i++)
	{
		Vec2i vI = Vec2iNew(v.x + dv.x * i, v.y + dv.y * i);
		a = WatchAddAction(w);
		if (isHorizontal)
		{
			// Add shadows below doors
			a->action = ACTION_CHANGETILE;
			a->u.pos = Vec2iNew(vI.x + dAside.x, vI.y + dAside.y);
			if (IMapGet(map, a->u.pos) == MAP_FLOOR)
			{
				a->tilePic = PicManagerGetFromOld(
					&gPicManager, cFloorPics[floor][FLOOR_SHADOW]);
			}
			else
			{
				a->tilePic = PicManagerGetFromOld(
					&gPicManager, cRoomPics[room][ROOMFLOOR_SHADOW]);
			}
		}
	}

	return w;
}
// Only creates the trigger, but does not place it
static Trigger *MapNewTrigger(Map *map)
{
	Trigger *t = TriggerNew();
	CArrayPushBack(&map->triggers, &t);
	t->id = map->triggerId++;
	return t;
}
static void TileAddTrigger(Tile *t, Trigger *tr)
{
	if (t->triggers.elemSize == 0)
	{
		CArrayInit(&t->triggers, sizeof(Trigger *));
	}
	CArrayPushBack(&t->triggers, &tr);
}
static Trigger *CreateOpenDoorTrigger(
	Map *map, Vec2i v,
	int isHorizontal, int doorGroupCount, int flags,
	int openDoorPic, int floor, int room)
{
	// Only add the trigger on the first tile
	// Subsequent tiles reference the trigger
	Vec2i dv = Vec2iNew(isHorizontal ? 1 : 0, isHorizontal ? 0 : 1);
	Vec2i dAside = Vec2iNew(dv.y, dv.x);
	Trigger *t = MapNewTrigger(map);
	int i;
	Action *a;

	t->flags = flags;

	/// play sound at the center of the door group
	a = TriggerAddAction(t);
	a->action = ACTION_SOUND;
	a->u.pos = Vec2iCenterOfTile(Vec2iNew(
		v.x + dv.x * doorGroupCount / 2,
		v.y + dv.y * doorGroupCount / 2));
	a->tileFlags = SND_DOOR;

	// Deactivate itself
	a = TriggerAddAction(t);
	a->action = ACTION_CLEARTRIGGER;
	a->u.index = t->id;

	// Open doors
	for (i = 0; i < doorGroupCount; i++)
	{
		Vec2i vI = Vec2iNew(v.x + dv.x * i, v.y + dv.y * i);
		a = TriggerAddAction(t);
		a->action = ACTION_CHANGETILE;
		a->u.pos = vI;
		if (isHorizontal)
		{
			a->tilePic = PicManagerGetFromOld(&gPicManager, openDoorPic);
		}
		else if (i == 0)
		{
			// special door cavity picture
			PicLoadOffset(&gGraphicsDevice, &a->tilePicAlt, openDoorPic);
			a->tilePic = PicManagerGetFromOld(
				&gPicManager, cRoomPics[room][ROOMFLOOR_SHADOW]);
		}
		else
		{
			// room floor pic
			a->tilePic = PicManagerGetFromOld(
				&gPicManager, cRoomPics[room][ROOMFLOOR_SHADOW]);
		}
		a->tileFlags = (isHorizontal || i > 0) ? 0 : MAPTILE_OFFSET_PIC;
	}

	// Change tiles below the doors
	if (isHorizontal)
	{
		for (i = 0; i < doorGroupCount; i++)
		{
			Vec2i vI = Vec2iNew(v.x + dv.x * i, v.y + dv.y * i);
			Vec2i vIAside = Vec2iNew(vI.x + dAside.x, vI.y + dAside.y);
			a = TriggerAddAction(t);
			// Remove shadows below doors
			a->action = ACTION_CHANGETILE;
			a->u.pos = vIAside;
			if (IMapGet(map, vIAside) == MAP_FLOOR)
			{
				a->tilePic = PicManagerGetFromOld(
					&gPicManager, cFloorPics[floor][FLOOR_NORMAL]);
			}
			else
			{
				a->tilePic = PicManagerGetFromOld(
					&gPicManager, cRoomPics[room][ROOMFLOOR_NORMAL]);
			}
		}
	}

	// Now place the two triggers on the tiles along either side of the doors
	for (i = 0; i < doorGroupCount; i++)
	{
		Vec2i vI = Vec2iNew(v.x + dv.x * i, v.y + dv.y * i);
		Vec2i vAside = Vec2iNew(vI.x - dAside.x, vI.y - dAside.y);
		TileAddTrigger(MapGetTile(map, vAside), t);
		vAside = Vec2iNew(vI.x + dAside.x, vI.y + dAside.y);
		TileAddTrigger(MapGetTile(map, vAside), t);
	}

	return t;
}
static void MapAddDoorGroup(Map *map, Vec2i v, int floor, int room, int flags)
{
	TWatch *w;
	Trigger *t;
	Action *a;
	int i;
	int pic;
	int openDoorPic;
	int tileFlags =
		MAPTILE_NO_SEE | MAPTILE_NO_WALK |
		MAPTILE_NO_SHOOT | MAPTILE_OFFSET_PIC;
	struct DoorPic *dp;
	int isHorizontal =
		IMapGet(map, Vec2iNew(v.x - 1, v.y)) == MAP_WALL ||
		IMapGet(map, Vec2iNew(v.x - 1, v.y)) == MAP_DOOR ||
		IMapGet(map, Vec2iNew(v.x + 1, v.y)) == MAP_WALL ||
		IMapGet(map, Vec2iNew(v.x + 1, v.y)) == MAP_DOOR;
	int doorGroupCount = GetDoorCountInGroup(map, v, isHorizontal);
	Vec2i dv = Vec2iNew(isHorizontal ? 1 : 0, isHorizontal ? 0 : 1);
	Vec2i dAside = Vec2iNew(dv.y, dv.x);

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

	// set up the door pics
	for (i = 0; i < doorGroupCount; i++)
	{
		Vec2i vI = Vec2iNew(v.x + dv.x * i, v.y + dv.y * i);
		Tile *tile = MapGetTile(map, vI);
		PicLoadOffset(&gGraphicsDevice, &tile->picAlt, pic);
		tile->pic = PicManagerGetFromOld(
			&gPicManager, cRoomPics[room][ROOMFLOOR_SHADOW]);
		tile->flags = tileFlags;
		if (isHorizontal)
		{
			Vec2i vB = Vec2iNew(vI.x + dAside.x, vI.y + dAside.y);
			Tile *tileB = MapGetTile(map, vB);
			assert(TileCanWalk(MapGetTile(
				map, Vec2iNew(vI.x - dAside.x, vI.y - dAside.y))) &&
				"map gen error: entrance should be clear");
			assert(TileCanWalk(tileB) &&
				"map gen error: entrance should be clear");
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
	}

	w = CreateCloseDoorWatch(
		map, v, tileFlags, isHorizontal, doorGroupCount, pic, floor, room);
	t = CreateOpenDoorTrigger(
		map, v,
		isHorizontal, doorGroupCount, flags, openDoorPic, floor, room);
	// Connect trigger and watch up
	a = TriggerAddAction(t);
	a->action = ACTION_ACTIVATEWATCH;
	a->u.index = w->index;
	a = WatchAddAction(w);
	a->action = ACTION_SETTRIGGER;
	a->u.index = t->id;
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
			// Check if this is the start of a door group
			// Top or left-most door
			if (IMapGet(map, v) == MAP_DOOR &&
				IMapGet(map, Vec2iNew(v.x - 1, v.y)) != MAP_DOOR &&
				IMapGet(map, Vec2iNew(v.x, v.y - 1)) != MAP_DOOR)
			{
				MapAddDoorGroup(
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

void MapInit(Map *map)
{
	Vec2i v;
	memset(map, 0, sizeof *map);
	CArrayInit(&map->triggers, sizeof(Trigger *));
	for (v.y = 0; v.y < YMAX; v.y++)
	{
		for (v.x = 0; v.x < XMAX; v.x++)
		{
			Tile *t = MapGetTile(map, v);
			TileInit(t);
		}
	}
}
void MapTerminate(Map *map)
{
	Vec2i v;
	int i;
	for (i = 0; i < (int)map->triggers.size; i++)
	{
		TriggerTerminate(*(Trigger **)CArrayGet(&map->triggers, i));
	}
	CArrayTerminate(&map->triggers);
	for (v.y = 0; v.y < YMAX; v.y++)
	{
		for (v.x = 0; v.x < XMAX; v.x++)
		{
			Tile *t = MapGetTile(map, v);
			TileDestroy(t);
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

	PicManagerGenerateOldPics(&gPicManager, &gGraphicsDevice);
	MapTerminate(map);
	MapInit(map);

	w = mission->Size.x && mission->Size.x < XMAX ? mission->Size.x : XMAX;
	h = mission->Size.y && mission->Size.y < YMAX ? mission->Size.y : YMAX;
	x = (XMAX - w) / 2;
	y = (YMAX - h) / 2;
	map->tilesTotal = w * h;

	MapSetupPerimeter(map, mission->Size.x, mission->Size.y);

	if (mission->Type == MAPTYPE_CLASSIC)
	{
		MapClassicLoad(map, mission);
	}
	else
	{
		MapStaticLoad(map, mission);
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
