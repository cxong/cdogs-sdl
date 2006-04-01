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

-------------------------------------------------------------------------------

 map.c - map related functions

*/

#include <string.h>
#include <stdlib.h>
#include "map.h"
#include "pics.h"
#include "objs.h"
#include "triggers.h"
#include "sounds.h"
#include "actors.h"
#include "gamedata.h"
#include "mission.h"


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


TTile gMap[YMAX][XMAX];
unsigned char gAutoMap[YMAX][XMAX];


static int accessCount;
static unsigned short internalMap[YMAX][XMAX];
static int tilesSeen = 0;
static int tilesTotal = XMAX * YMAX;
#define iMap( x, y) internalMap[y][x]


static void AddItemToTile(TTileItem * t, TTile * tile)
{
	t->next = tile->things;
	tile->things = t;
}

static void RemoveItemFromTile(TTileItem * t, TTile * tile)
{
	TTileItem **h = &tile->things;

	while (*h && *h != t)
		h = &(*h)->next;
	if (*h) {
		*h = t->next;
		t->next = NULL;
	}
}

void MoveTileItem(TTileItem * t, int x, int y)
{
	TTile *tile;
	int x1 = t->x / TILE_WIDTH;
	int y1 = t->y / TILE_HEIGHT;
	int x2 = x / TILE_WIDTH;
	int y2 = y / TILE_HEIGHT;

	t->x = x;
	t->y = y;
	if (x1 == x2 && y1 == y2)
		return;

	tile = &Map(x1, y1);
	RemoveItemFromTile(t, tile);
	tile = &Map(x2, y2);
	AddItemToTile(t, tile);
}

void RemoveTileItem(TTileItem * t)
{
	TTile *tile;
	int x = t->x / TILE_WIDTH;
	int y = t->y / TILE_HEIGHT;

	tile = &Map(x, y);
	RemoveItemFromTile(t, tile);
}

int CheckWall(int x, int y, int w, int h)
{
	x >>= 8;
	y >>= 8;
	if (HitWall(x - w, y - h) ||
	    HitWall(x - w, y) ||
	    HitWall(x - w, y + h) ||
	    HitWall(x, y + h) ||
	    HitWall(x + w, y + h) ||
	    HitWall(x + w, y) ||
	    HitWall(x + w, y - h) || HitWall(x, y - h))
		return 1;
	return 0;
}

int ItemsCollide(TTileItem * item1, TTileItem * item2, int x, int y)
{
	int dx = abs(x - item2->x);
	int dy = abs(y - item2->y);
	int rx = item1->w + item2->w;
	int ry = item1->h + item2->h;

	if (dx < rx && dy < ry) {
		int odx = abs(item1->x - item2->x);
		int ody = abs(item1->y - item2->y);

		if (dx <= odx || dy <= ody)
			return 1;
	}
	return 0;
}

TTileItem *CheckTileItemCollision(TTileItem * item, int x, int y, int mask)
{
	TTileItem *i;
	int tx, ty, dx, dy;
	TTile *tile;


	tx = x / TILE_WIDTH;
	ty = y / TILE_HEIGHT;

	if (tx == 0 || ty == 0 || tx >= XMAX - 1 || ty >= YMAX - 1)
		return NULL;

	for (dy = -1; dy <= 1; dy++)
		for (dx = -1; dx <= 1; dx++) {
			tile = &Map(tx + dx, ty + dy);
			i = tile->things;
			while (i) {
				if (item != i &&
				    (i->flags & mask) != 0 &&
				    ItemsCollide(item, i, x, y))
					return i;
				i = i->next;
			}
		}

	return NULL;
}

void GuessCoords(int *x, int *y)
{
	if (gMission.missionData->mapWidth)
		*x = (rand() % gMission.missionData->mapWidth) + (XMAX -
								  gMission.
								  missionData->
								  mapWidth)
		    / 2;
	else
		*x = rand() % XMAX;

	if (gMission.missionData->mapHeight)
		*y = (rand() % gMission.missionData->mapHeight) + (YMAX -
								   gMission.
								   missionData->
								   mapHeight)
		    / 2;
	else
		*y = rand() % YMAX;
}

void GuessPixelCoords(int *x, int *y)
{
	if (gMission.missionData->mapWidth)
		*x = (rand() %
		      (gMission.missionData->mapWidth * TILE_WIDTH)) +
		    (XMAX -
		     gMission.missionData->mapWidth) * TILE_WIDTH / 2;
	else
		*x = rand() % (XMAX * TILE_WIDTH);

	if (gMission.missionData->mapHeight)
		*y = (rand() %
		      (gMission.missionData->mapHeight * TILE_HEIGHT)) +
		    (YMAX -
		     gMission.missionData->mapHeight) * TILE_HEIGHT / 2;
	else
		*y = rand() % (YMAX * TILE_HEIGHT);
}

static void Grow(int x, int y, int d, int length)
{
	int l;

	if (length <= 0)
		return;

	switch (d) {
	case 0:
		if (y < 3 ||
		    iMap(x - 1, y - 1) != 0 ||
		    iMap(x + 1, y - 1) != 0 ||
		    iMap(x - 1, y - 2) != 0 ||
		    iMap(x, y - 2) != 0 || iMap(x + 1, y - 2) != 0)
			return;
		y--;
		break;
	case 1:
		if (x > XMAX - 3 ||
		    iMap(x + 1, y - 1) != 0 ||
		    iMap(x + 1, y + 1) != 0 ||
		    iMap(x + 2, y - 1) != 0 ||
		    iMap(x + 2, y) != 0 || iMap(x + 2, y + 1) != 0)
			return;
		x++;
		break;
	case 2:
		if (y > YMAX - 3 ||
		    iMap(x - 1, y + 1) != 0 ||
		    iMap(x + 1, y + 1) != 0 ||
		    iMap(x - 1, y + 2) != 0 ||
		    iMap(x, y + 2) != 0 || iMap(x + 1, y + 2) != 0)
			return;
		y++;
		break;
	case 4:
		if (x < 3 ||
		    iMap(x - 1, y - 1) != 0 ||
		    iMap(x - 1, y + 1) != 0 ||
		    iMap(x - 2, y - 1) != 0 ||
		    iMap(x - 2, y) != 0 || iMap(x - 2, y + 1) != 0)
			return;
		x--;
		break;
	}
	iMap(x, y) = MAP_WALL;
	length--;
	if (length > 0 && (rand() & 3) == 0) {
		l = rand() % length;
		Grow(x, y, rand() & 3, l);
		length -= l;
	}
	Grow(x, y, d, length);
}

static int ValidStart(int x, int y)
{
	if (x == 0 || y == 0 || x == XMAX - 1 || y == YMAX - 1)
		return YES;
	if (iMap(x - 1, y - 1) == 0 &&
	    iMap(x, y - 1) == 0 &&
	    iMap(x + 1, y - 1) == 0 &&
	    iMap(x - 1, y) == 0 &&
	    iMap(x, y) == 0 &&
	    iMap(x + 1, y) == 0 &&
	    iMap(x - 1, y + 1) == 0 &&
	    iMap(x, y + 1) == 0 && iMap(x + 1, y + 1) == 0)
		return YES;
	return NO;
}

static int BuildWall(int wallLength)
{
	int x, y;

	GuessCoords(&x, &y);
	if (ValidStart(x, y)) {
		iMap(x, y) = MAP_WALL;
		Grow(x, y, rand() & 3, wallLength);
		return 1;
	}
	return 0;
}

void MakeRoom(int xOrigin, int yOrigin, int width, int height, int doors,
	      int access)
{
	int x, y;

	for (y = yOrigin; y <= yOrigin + height; y++) {
		iMap(xOrigin, y) = MAP_WALL;
		iMap(xOrigin + width, y) = MAP_WALL;
	}
	for (x = xOrigin + 1; x < xOrigin + width; x++) {
		iMap(x, yOrigin) = MAP_WALL;
		iMap(x, yOrigin + height) = MAP_WALL;
		for (y = yOrigin + 1; y < yOrigin + height; y++)
			iMap(x, y) = MAP_ROOM | access;
	}
	if (doors & 1)
		iMap(xOrigin, yOrigin + height / 2) = MAP_DOOR;
	if (doors & 2)
		iMap(xOrigin + width, yOrigin + height / 2) = MAP_DOOR;
	if (doors & 4)
		iMap(xOrigin + width / 2, yOrigin) = MAP_DOOR;
	if (doors & 8)
		iMap(xOrigin + width / 2, yOrigin + height) = MAP_DOOR;
}

int AreaClear(int xOrigin, int yOrigin, int width, int height)
{
	int x, y;

	if (xOrigin < 0 || yOrigin < 0 ||
	    xOrigin + width >= XMAX || yOrigin + height >= YMAX)
		return NO;

	for (y = yOrigin; y <= yOrigin + height; y++)
		for (x = xOrigin; x <= xOrigin + width; x++)
			if (iMap(x, y) != MAP_FLOOR)
				return NO;
	return YES;
}

static int BuildRoom(void)
{
	int x, y, w, h, access = 0;

	GuessCoords(&x, &y);
	w = rand() % 6 + 5;
	h = rand() % 6 + 5;

	if (AreaClear(x - 1, y - 1, w + 2, h + 2)) {
		switch (rand() % 20) {
		case 0:
			if (accessCount >= 4) {
				access = MAP_ACCESS_RED;
				accessCount = 5;
				break;
			}
		case 1:
		case 2:
			if (accessCount >= 3) {
				access = MAP_ACCESS_BLUE;
				if (accessCount < 4)
					accessCount = 4;
				break;
			}
		case 3:
		case 4:
		case 5:
			if (accessCount >= 2) {
				access = MAP_ACCESS_GREEN;
				if (accessCount < 3)
					accessCount = 3;
				break;
			}
		case 6:
		case 7:
		case 8:
		case 9:
			if (accessCount >= 1) {
				access = MAP_ACCESS_YELLOW;
				if (accessCount < 2)
					accessCount = 2;
				break;
			}
		}
		MakeRoom(x, y, w, h, rand() % 15 + 1, access);
		if (accessCount < 1)
			accessCount = 1;
		return 1;
	}
	return 0;
}

static void MakeSquare(int xOrigin, int yOrigin, int width, int height)
{
	int x, y;

	for (x = xOrigin; x <= xOrigin + width; x++) {
		for (y = yOrigin; y <= yOrigin + height; y++)
			iMap(x, y) = MAP_SQUARE;
	}
}

static int BuildSquare(void)
{
	int x, y, w, h;

	GuessCoords(&x, &y);
	w = rand() % 9 + 7;
	h = rand() % 9 + 7;

	if (AreaClear(x - 1, y - 1, w + 2, h + 2)) {
		MakeSquare(x, y, w, h);
		return 1;
	}
	return 0;
}

static int W(int x, int y)
{
	return (x >= 0 && y >= 0 && x < XMAX && y < YMAX &&
		iMap(x, y) == MAP_WALL);
}

static int GetWallPic(int x, int y)
{
	if (W(x - 1, y) && W(x + 1, y) && W(x, y + 1) && W(x, y - 1))
		return WALL_CROSS;
	if (W(x - 1, y) && W(x + 1, y) && W(x, y + 1))
		return WALL_TOP_T;
	if (W(x - 1, y) && W(x + 1, y) && W(x, y - 1))
		return WALL_BOTTOM_T;
	if (W(x - 1, y) && W(x, y + 1) && W(x, y - 1))
		return WALL_RIGHT_T;
	if (W(x + 1, y) && W(x, y + 1) && W(x, y - 1))
		return WALL_LEFT_T;
	if (W(x + 1, y) && W(x, y + 1))
		return WALL_TOPLEFT;
	if (W(x + 1, y) && W(x, y - 1))
		return WALL_BOTTOMLEFT;
	if (W(x - 1, y) && W(x, y + 1))
		return WALL_TOPRIGHT;
	if (W(x - 1, y) && W(x, y - 1))
		return WALL_BOTTOMRIGHT;
	if (W(x - 1, y) && W(x + 1, y))
		return WALL_HORIZONTAL;
	if (W(x, y + 1) && W(x, y - 1))
		return WALL_VERTICAL;
	if (W(x, y + 1))
		return WALL_TOP;
	if (W(x, y - 1))
		return WALL_BOTTOM;
	if (W(x + 1, y))
		return WALL_LEFT;
	if (W(x - 1, y))
		return WALL_RIGHT;
	return WALL_SINGLE;
}

static void FixMap(int floor, int room, int wall)
{
	int x, y, i;

	for (x = 0; x < XMAX; x++)
		for (y = 0; y < YMAX; y++) {
			switch (iMap(x, y) & MAP_MASKACCESS) {
			case MAP_FLOOR:
			case MAP_SQUARE:
				if (y > 0
				    && (Map(x, y - 1).flags & NO_SEE) != 0)
					Map(x, y).pic = cFloorPics[floor]
					    [FLOOR_SHADOW];
				else
					Map(x, y).pic = cFloorPics[floor]
					    [FLOOR_NORMAL];
				break;

			case MAP_ROOM:
			case MAP_DOOR:
				if (y > 0
				    && (Map(x, y - 1).flags & NO_SEE) != 0)
					Map(x, y).pic = cRoomPics[room]
					    [ROOMFLOOR_SHADOW];
				else
					Map(x, y).pic = cRoomPics[room]
					    [ROOMFLOOR_NORMAL];
				break;

			case MAP_WALL:
				Map(x, y).pic =
				    cWallPics[wall][GetWallPic(x, y)];
				Map(x, y).flags =
				    NO_WALK | NO_SEE | IS_WALL;
				break;

			case MAP_NOTHING:
				Map(x, y).flags = NO_WALK | NO_SEE;
				break;
			}
		}

	for (i = 0; i < 50; i++) {
		x = (rand() % XMAX) & 0xFFFFFE;
		y = (rand() % YMAX) & 0xFFFFFE;
		if (Map(x, y).flags == 0 &&
		    Map(x, y).pic == cFloorPics[floor][FLOOR_NORMAL])
			Map(x, y).pic = PIC_DRAINAGE;
	}
	for (i = 0; i < 100; i++) {
		x = rand() % XMAX;
		y = rand() % YMAX;
		if (Map(x, y).flags == 0 &&
		    Map(x, y).pic == cFloorPics[floor][FLOOR_NORMAL])
			Map(x, y).pic = cFloorPics[floor][FLOOR_1];
	}
	for (i = 0; i < 150; i++) {
		x = rand() % XMAX;
		y = rand() % YMAX;
		if (Map(x, y).flags == 0 &&
		    Map(x, y).pic == cFloorPics[floor][FLOOR_NORMAL])
			Map(x, y).pic = cFloorPics[floor][FLOOR_2];
	}
}

void ChangeFloor(int x, int y, int normal, int shadow)
{
	switch (iMap(x, y) & MAP_MASKACCESS) {
	case MAP_FLOOR:
	case MAP_SQUARE:
	case MAP_ROOM:
		if (Map(x, y).pic == PIC_DRAINAGE)
			return;
		if (y > 0 && (Map(x, y - 1).flags & NO_SEE) != 0)
			Map(x, y).pic = shadow;
		else
			Map(x, y).pic = normal;
		break;
	}
}

/*
static int CheckForItems( int x, int y, int w, int h )
{
  TTileItem item;

  item.w = w;
  item.h = h;
  return CheckTileItemCollision( &item, x, y, TILEITEM_IMPASSABLE) != NULL;
}
*/

static int OneWall(int x, int y)
{
	int count = 0;
	if (x > 0 && y > 0 && x < XMAX - 1 && y < YMAX - 1) {
		if ((Map(x - 1, y).flags & NO_WALK) != 0)
			count++;
		if ((Map(x + 1, y).flags & NO_WALK) != 0)
			count++;
		if ((Map(x, y - 1).flags & NO_WALK) != 0)
			count++;
		if ((Map(x, y + 1).flags & NO_WALK) != 0)
			count++;
	}
	return (count == 1);
}

static int OneWallOrMore(int x, int y)
{
	int count = 0;
	if (x > 0 && y > 0 && x < XMAX - 1 && y < YMAX - 1) {
		if ((Map(x - 1, y).flags & NO_WALK) != 0)
			count++;
		if ((Map(x + 1, y).flags & NO_WALK) != 0)
			count++;
		if ((Map(x, y - 1).flags & NO_WALK) != 0)
			count++;
		if ((Map(x, y + 1).flags & NO_WALK) != 0)
			count++;
	}
	return (count >= 1);
}

static int NoWalls(int x, int y)
{
	if (x > 0 && y > 0 && x < XMAX - 1 && y < YMAX - 1) {
		if ((Map(x - 1, y).flags & NO_WALK) != 0)
			return 0;
		if ((Map(x + 1, y).flags & NO_WALK) != 0)
			return 0;
		if ((Map(x, y - 1).flags & NO_WALK) != 0)
			return 0;
		if ((Map(x, y + 1).flags & NO_WALK) != 0)
			return 0;
		if ((Map(x - 1, y - 1).flags & NO_WALK) != 0)
			return 0;
		if ((Map(x + 1, y + 1).flags & NO_WALK) != 0)
			return 0;
		if ((Map(x + 1, y - 1).flags & NO_WALK) != 0)
			return 0;
		if ((Map(x - 1, y + 1).flags & NO_WALK) != 0)
			return 0;
		return 1;
	}
	return 0;
}

static int PlaceOneObject(int x, int y, TMapObject * mo, int extraFlags)
{
	int f = mo->flags;
	int oFlags = 0;
	int yCoord;
	int tileFlags = 0;

	if (Map(x, y).flags != 0 ||
	    Map(x, y).things != NULL || (iMap(x, y) & MAP_LEAVEFREE) != 0)
		return 0;

	if ((f & MAPOBJ_ROOMONLY) != 0 &&
	    (iMap(x, y) & MAP_MASKACCESS) != MAP_ROOM)
		return 0;

	if ((f & MAPOBJ_NOTINROOM) != 0 &&
	    (iMap(x, y) & MAP_MASKACCESS) == MAP_ROOM)
		return 0;

	if ((f & MAPOBJ_ON_WALL) != 0 &&
	    (iMap(x, y - 1) & MAP_MASKACCESS) != MAP_WALL)
		return 0;

	if ((f & MAPOBJ_FREEINFRONT) != 0 &&
	    (iMap(x, y + 1) & MAP_MASKACCESS) != MAP_ROOM &&
	    (iMap(x, y + 1) & MAP_MASKACCESS) != MAP_FLOOR)
		return 0;

	if ((f & MAPOBJ_ONEWALL) != 0 && !OneWall(x, y))
		return 0;

	if ((f & MAPOBJ_ONEWALLPLUS) != 0 && !OneWallOrMore(x, y))
		return 0;

	if ((f & MAPOBJ_NOWALLS) != 0 && !NoWalls(x, y))
		return 0;

	if ((f & MAPOBJ_FREEINFRONT) != 0)
		iMap(x, y) |= MAP_LEAVEFREE;

	if ((f & MAPOBJ_EXPLOSIVE) != 0)
		oFlags |= OBJFLAG_EXPLOSIVE;
	if ((f & MAPOBJ_FLAMMABLE) != 0)
		oFlags |= OBJFLAG_FLAMMABLE;
	if ((f & MAPOBJ_POISONOUS) != 0)
		oFlags |= OBJFLAG_POISONOUS;
	if ((f & MAPOBJ_QUAKE) != 0)
		oFlags |= OBJFLAG_QUAKE;

	yCoord = y * TILE_HEIGHT;
	if ((f & MAPOBJ_ON_WALL) != 0)
		yCoord--;
	else
		yCoord += TILE_HEIGHT / 2;

	if ((f & MAPOBJ_IMPASSABLE) != 0)
		tileFlags |= TILEITEM_IMPASSABLE;

	if ((f & MAPOBJ_CANBESHOT) != 0)
		tileFlags |= TILEITEM_CAN_BE_SHOT;

	AddDestructibleObject((x * TILE_WIDTH + TILE_WIDTH / 2) << 8,
			      yCoord << 8,
			      mo->width, mo->height,
			      &cGeneralPics[mo->pic],
			      &cGeneralPics[mo->wreckedPic],
			      (int)(mo->structure),
			      oFlags, tileFlags | extraFlags);
	return 1;
}

int HasHighAccess(void)
{
	return accessCount > 1;
}

int IsHighAccess(int x, int y)
{
	return (iMap(x / TILE_WIDTH, y / TILE_HEIGHT) & MAP_ACCESSBITS) !=
	    0;
}

static void PlaceObject(int x, int y, int index)
{
	TMapObject *mo = gMission.mapObjects[index];
	PlaceOneObject(x, y, mo, 0);
}

static int PlaceCollectible(int objective)
{
	int access =
	    (gMission.missionData->objectives[objective].
	     flags & OBJECTIVE_HIACCESS) != 0 && accessCount > 1;
	int noaccess =
	    (gMission.missionData->objectives[objective].
	     flags & OBJECTIVE_NOACCESS) != 0;
	int x, y;
	int i = (noaccess || access) ? 1000 : 100;

	while (i) {
		GuessPixelCoords(&x, &y);
		if (!CheckWall(x << 8, y << 8, 4, 3)) {
			if ((!access
			     || (iMap(x / TILE_WIDTH, y / TILE_HEIGHT) &
				 MAP_ACCESSBITS) != 0) && (!noaccess
							   ||
							   (iMap
							    (x /
							     TILE_WIDTH,
							     y /
							     TILE_HEIGHT) &
							    MAP_ACCESSBITS)
							   == 0)) {
				AddObject(x << 8, y << 8, 3, 2,
					  &cGeneralPics[gMission.
							objectives
							[objective].
							pickupItem],
					  OBJ_JEWEL,
					  TILEITEM_CAN_BE_TAKEN |
					  (int)ObjectiveToTileItem(objective));
				return 1;
			}
		}
		i--;
	}
	return 0;
}

static int PlaceBlowup(int objective)
{
	int access =
	    (gMission.missionData->objectives[objective].
	     flags & OBJECTIVE_HIACCESS) != 0 && accessCount > 1;
	int noaccess =
	    (gMission.missionData->objectives[objective].
	     flags & OBJECTIVE_NOACCESS) != 0;
	int i = (noaccess || access) ? 1000 : 100;
	int x, y;

	while (i > 0) {
		GuessCoords(&x, &y);
		if ((!access || (iMap(x, y) >> 8) != 0) &&
		    (!noaccess || (iMap(x, y) >> 8) == 0)) {
			if (PlaceOneObject(x, y,
					   gMission.objectives[objective].
					   blowupObject,
					   ObjectiveToTileItem(objective)))
				return 1;
		}
		i--;
	}
	return 0;
}

static void PlaceCard(int pic, int card, int access)
{
	int x, y;

	while (1) {
		GuessCoords(&x, &y);
		if (y < YMAX - 1 &&
		    Map(x, y).flags == 0 &&
		    Map(x, y).things == NULL &&
		    (iMap(x, y) & 0xF00) == access &&
		    (iMap(x, y) & MAP_MASKACCESS) == MAP_ROOM &&
		    Map(x, y + 1).flags == 0 &&
		    Map(x, y + 1).things == NULL) {
			AddObject((x * TILE_WIDTH + TILE_WIDTH / 2) << 8,
				  (y * TILE_HEIGHT + TILE_HEIGHT / 2) << 8,
				  9, 5,
				  &cGeneralPics[gMission.keyPics[pic]],
				  card, (int)TILEITEM_CAN_BE_TAKEN);
			return;
		}
	}
}

/*
void PlacePuzzlePiece( int index )
{
  int x, y;

  while (1)
  {
    GuessCoords( &x, &y);
    if (y < YMAX-1 &&
        Map( x, y).flags == 0 &&
        Map( x, y).things == NULL &&
        Map( x, y+1).flags == 0)
    {
      AddPuzzlePiece( (x*TILE_WIDTH + TILE_WIDTH/2) << 8,
                      (y*TILE_HEIGHT + TILE_HEIGHT/2) << 8,
                      index);
      return;
    }
  }
}
*/

static void VertDoor(int x, int y, int flags)
{
	TTrigger *t;
	TWatch *w;
	TCondition *c;
	TAction *a;
	int pic;

	switch (flags) {
	case FLAGS_KEYCARD_RED:
		pic = gMission.doorPics[4].vertPic;
		break;
	case FLAGS_KEYCARD_BLUE:
		pic = gMission.doorPics[3].vertPic;
		break;
	case FLAGS_KEYCARD_GREEN:
		pic = gMission.doorPics[2].vertPic;
		break;
	case FLAGS_KEYCARD_YELLOW:
		pic = gMission.doorPics[1].vertPic;
		break;
	default:
		pic = gMission.doorPics[0].vertPic;
		break;
	}

	Map(x, y).pic = pic;
	Map(x, y).flags = NO_SEE | NO_WALK | NO_SHOOT | OFFSET_PIC;
	Map(x - 1, y).flags |= TILE_TRIGGER;
	Map(x + 1, y).flags |= TILE_TRIGGER;

	// Create the watch responsible for closing the door
	w = AddWatch(3, 5);

	// The conditions are that the tile above, at and below the door are empty
	c = w->conditions;
	c[0].condition = CONDITION_TILECLEAR;
	c[0].x = x - 1;
	c[0].y = y;
	c[1].condition = CONDITION_TILECLEAR;
	c[1].x = x;
	c[1].y = y;
	c[2].condition = CONDITION_TILECLEAR;
	c[2].x = x + 1;
	c[2].y = y;

	// Now the actions of the watch once it's triggered
	a = w->actions;

	// Deactivate itself
	a[0].action = ACTION_DEACTIVATEWATCH;
	a[0].x = w->index;

	// Reenable trigger to the left of the door
	a[1].action = ACTION_SETTRIGGER;
	a[1].x = x - 1;
	a[1].y = y;

	// Close door
	a[2].action = ACTION_CHANGETILE;
	a[2].x = x;
	a[2].y = y;
	a[2].tilePic = pic;
	a[2].tileFlags = NO_SEE | NO_WALK | NO_SHOOT | OFFSET_PIC;

	// Reenable trigger to the right of the door
	a[3].action = ACTION_SETTRIGGER;
	a[3].x = x + 1;
	a[3].y = y;

	a[4].action = ACTION_SOUND;
	a[4].x = x * TILE_WIDTH;
	a[4].y = y * TILE_HEIGHT;
	a[4].tileFlags = SND_DOOR;

	// Add trigger to the left of the door
	t = AddTrigger(x - 1, y, 5);
	t->flags = flags;

	a = t->actions;

	// Enable the watch to close the door
	a[0].action = ACTION_ACTIVATEWATCH;
	a[0].x = w->index;

	// Deactivate itself
	a[1].action = ACTION_CLEARTRIGGER;
	a[1].x = t->x;
	a[1].y = t->y;

	// Open door
	a[2].action = ACTION_CHANGETILE;
	a[2].x = x;
	a[2].y = y;
	a[2].tilePic = gMission.doorPics[5].vertPic;
	a[2].tileFlags = OFFSET_PIC;

	// Deactivate other trigger
	a[3].action = ACTION_CLEARTRIGGER;
	a[3].x = x + 1;
	a[3].y = y;

	a[4].action = ACTION_SOUND;
	a[4].x = x * TILE_WIDTH;
	a[4].y = y * TILE_HEIGHT;
	a[4].tileFlags = SND_DOOR;

	// Add trigger to the right of the door with identical actions as this one
	t = AddTrigger(x + 1, y, 5);
	t->flags = flags;
	memcpy(t->actions, a, sizeof(TAction) * 5);
}

static void HorzDoor(int x, int y, int floor, int room, int flags)
{
	TTrigger *t;
	TWatch *w;
	TCondition *c;
	TAction *a;
	int pic;

	switch (flags) {
	case FLAGS_KEYCARD_RED:
		pic = gMission.doorPics[4].horzPic;
		break;
	case FLAGS_KEYCARD_BLUE:
		pic = gMission.doorPics[3].horzPic;
		break;
	case FLAGS_KEYCARD_GREEN:
		pic = gMission.doorPics[2].horzPic;
		break;
	case FLAGS_KEYCARD_YELLOW:
		pic = gMission.doorPics[1].horzPic;
		break;
	default:
		pic = gMission.doorPics[0].horzPic;
		break;
	}

	Map(x, y).pic = pic;
	Map(x, y).flags = NO_SEE | NO_WALK | NO_SHOOT | OFFSET_PIC;
	Map(x, y - 1).flags |= TILE_TRIGGER;
	Map(x, y + 1).flags |= TILE_TRIGGER;
	if (iMap(x, y + 1) == MAP_FLOOR)
		Map(x, y + 1).pic = cFloorPics[floor][FLOOR_SHADOW];
	else
		Map(x, y + 1).pic = cRoomPics[room][ROOMFLOOR_SHADOW];

	// Create the watch responsible for closing the door
	w = AddWatch(3, 5);

	// The conditions are that the tile above, at and below the door are empty
	c = w->conditions;
	c[0].condition = CONDITION_TILECLEAR;
	c[0].x = x;
	c[0].y = y - 1;
	c[1].condition = CONDITION_TILECLEAR;
	c[1].x = x;
	c[1].y = y;
	c[2].condition = CONDITION_TILECLEAR;
	c[2].x = x;
	c[2].y = y + 1;

	// Now the actions of the watch once it's triggered
	a = w->actions;

	// Deactivate itself
	a[0].action = ACTION_DEACTIVATEWATCH;
	a[0].x = w->index;

	// Reenable trigger above door
	a[1].action = ACTION_SETTRIGGER;
	a[1].x = x;
	a[1].y = y - 1;

	// Close door
	a[2].action = ACTION_CHANGETILE;
	a[2].x = x;
	a[2].y = y;
	a[2].tilePic = pic;
	a[2].tileFlags = NO_SEE | NO_WALK | NO_SHOOT | OFFSET_PIC;

	// Add shadow below door (also reenables trigger)
	a[3].action = ACTION_CHANGETILE;
	a[3].x = x;
	a[3].y = y + 1;
	if (iMap(x, y + 1) == MAP_FLOOR)
		a[3].tilePic = cFloorPics[floor][FLOOR_SHADOW];
	else
		a[3].tilePic = cRoomPics[room][ROOMFLOOR_SHADOW];
	a[3].tileFlags = TILE_TRIGGER;

	a[4].action = ACTION_SOUND;
	a[4].x = x * TILE_WIDTH;
	a[4].y = y * TILE_HEIGHT;
	a[4].tileFlags = SND_DOOR;

	// Add trigger above door
	t = AddTrigger(x, y - 1, 5);
	t->flags = flags;

	a = t->actions;

	// Enable the watch to close the door
	a[0].action = ACTION_ACTIVATEWATCH;
	a[0].x = w->index;

	// Deactivate itself
	a[1].action = ACTION_CLEARTRIGGER;
	a[1].x = x;
	a[1].y = y - 1;

	// Open door
	a[2].action = ACTION_CHANGETILE;
	a[2].x = x;
	a[2].y = y;
	a[2].tilePic = gMission.doorPics[5].horzPic;
	a[2].tileFlags = 0;

	// Remove shadow below door (also clears trigger)
	a[3].action = ACTION_CHANGETILE;
	a[3].x = x;
	a[3].y = y + 1;
	if (iMap(x, y + 1) == MAP_FLOOR)
		a[3].tilePic = cFloorPics[floor][FLOOR_NORMAL];
	else
		a[3].tilePic = cRoomPics[room][ROOMFLOOR_SHADOW];
	a[3].tileFlags = 0;

	a[4].action = ACTION_SOUND;
	a[4].x = x * TILE_WIDTH;
	a[4].y = y * TILE_HEIGHT;
	a[4].tileFlags = SND_DOOR;

	// Add trigger below door with identical actions as the one above
	t = AddTrigger(x, y + 1, 5);
	t->flags = flags;
	memcpy(t->actions, a, sizeof(TAction) * 5);
}

static void CreateDoor(int x, int y, int floor, int room, int flags)
{
	if (iMap(x - 1, y) == MAP_WALL)
		HorzDoor(x, y, floor, room, flags);
	else
		VertDoor(x, y, flags);
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

int MapAccessLevel(int x, int y)
{
	return AccessCodeToFlags(iMap(x, y));
}

static int Access(int x, int y)
{
	int flags;

	if ((flags = AccessCodeToFlags(iMap(x - 1, y))) != 0)
		return flags;
	if ((flags = AccessCodeToFlags(iMap(x + 1, y))) != 0)
		return flags;
	if ((flags = AccessCodeToFlags(iMap(x, y - 1))) != 0)
		return flags;
	if ((flags = AccessCodeToFlags(iMap(x, y + 1))) != 0)
		return flags;
	return 0;
}

static void FixDoors(int floor, int room)
{
	int x, y;

	for (x = 0; x < XMAX; x++)
		for (y = 0; y < YMAX; y++)
			if (iMap(x, y) == MAP_DOOR) {
				CreateDoor(x, y, floor, room,
					   Access(x, y));
			}
}

void SetupPerimeter(int w, int h)
{
	int x, y, dx = 0, dy = 0;

	if (w && w < XMAX)
		dx = (XMAX - w) / 2;
	if (h && h < YMAX)
		dy = (YMAX - h) / 2;

	for (x = 0; x < dx; x++)
		for (y = 0; y < YMAX; y++)
			iMap(x, y) = MAP_NOTHING;
	for (x = XMAX - dx; x < XMAX; x++)
		for (y = 0; y < YMAX; y++)
			iMap(x, y) = MAP_NOTHING;

	for (x = 0; x < XMAX; x++)
		for (y = 0; y < dy; y++)
			iMap(x, y) = MAP_NOTHING;
	for (x = 0; x < XMAX; x++)
		for (y = YMAX - dy; y < YMAX; y++)
			iMap(x, y) = MAP_NOTHING;

	for (x = dx; x < XMAX - dx; x++) {
		iMap(x, dy) = MAP_WALL;
		iMap(x, YMAX - 1 - dy) = MAP_WALL;
	}

	for (y = dy; y < YMAX - 1 - dy; y++) {
		iMap(dx, y) = MAP_WALL;
		iMap(XMAX - 1 - dx, y) = MAP_WALL;
	}

}

void SetupMap(void)
{
	int i, j, count;
	struct Mission *mission = gMission.missionData;
	int floor = mission->floorStyle % FLOOR_COUNT;
	int wall = mission->wallStyle % WALL_COUNT;
	int room = mission->roomStyle % ROOMFLOOR_COUNT;
	int x, y, w, h;

	memset(gMap, 0, sizeof(gMap));
	memset(gAutoMap, 0, sizeof(gAutoMap));
	memset(internalMap, 0, sizeof(internalMap));
	tilesSeen = 0;

	w = mission->mapWidth
	    && mission->mapWidth < XMAX ? mission->mapWidth : XMAX;
	h = mission->mapHeight
	    && mission->mapHeight < YMAX ? mission->mapHeight : YMAX;
	x = (XMAX - w) / 2;
	y = (YMAX - h) / 2;
	tilesTotal = w * h;

	SetupPerimeter(mission->mapWidth, mission->mapHeight);

	count = 0;
	i = 0;
	while (i < 1000 && count < mission->squareCount) {
		if (BuildSquare())
			count++;
		i++;
	}

	accessCount = 0;
	count = 0;
	i = 0;
	while (i < 1000 && count < mission->roomCount) {
		if (BuildRoom())
			count++;
		i++;
	}

	count = 0;
	i = 0;
	while (i < 1000 && count < mission->wallCount) {
		if (BuildWall(mission->wallLength))
			count++;
		i++;
	}

	FixMap(floor, room, wall);
	FixDoors(floor, room);

	for (i = 0; i < gMission.objectCount; i++)
		for (j = 0;
		     j < (mission->itemDensity[i] * tilesTotal) / 1000;
		     j++)
			PlaceObject(x + rand() % w, y + rand() % h, i);

	for (i = 0, j = 0; i < mission->objectiveCount; i++)
		if (mission->objectives[i].type == OBJECTIVE_COLLECT) {
			for (j = 0, count = 0;
			     j < gMission.objectives[i].count; j++)
				if (PlaceCollectible(i))
					count++;
			gMission.objectives[i].count = count;
			if (gMission.objectives[i].count <
			    gMission.objectives[i].required)
				gMission.objectives[i].required =
				    gMission.objectives[i].count;
		} else if (mission->objectives[i].type ==
			   OBJECTIVE_DESTROY) {
			for (j = 0, count = 0;
			     j < gMission.objectives[i].count; j++)
				if (PlaceBlowup(i))
					count++;
			gMission.objectives[i].count = count;
			if (gMission.objectives[i].count <
			    gMission.objectives[i].required)
				gMission.objectives[i].required =
				    gMission.objectives[i].count;
		}

	if (accessCount >= 5)
		PlaceCard(3, OBJ_KEYCARD_RED, MAP_ACCESS_BLUE);
	if (accessCount >= 4)
		PlaceCard(2, OBJ_KEYCARD_BLUE, MAP_ACCESS_GREEN);
	if (accessCount >= 3)
		PlaceCard(1, OBJ_KEYCARD_GREEN, MAP_ACCESS_YELLOW);
	if (accessCount >= 2)
		PlaceCard(0, OBJ_KEYCARD_YELLOW, 0);
}

int OKforPlayer(int x, int y)
{
	return (iMap((x >> 8) / TILE_WIDTH, (y >> 8) / TILE_HEIGHT) == 0);
}

void MarkAsSeen(int x, int y)
{
	if (AutoMap(x, y) == 0) {
		tilesSeen++;
		AutoMap(x, y) = 1;
	}
}

int ExploredPercentage(void)
{
	return (100 * tilesSeen) / tilesTotal;
}
