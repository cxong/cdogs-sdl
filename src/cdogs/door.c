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

    Copyright (c) 2013-2015, Cong Xu
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
#include "door.h"

#include "gamedata.h"
#include "net_util.h"


#define DOOR_TILE_FLAGS \
	(MAPTILE_NO_SEE | MAPTILE_NO_WALK | MAPTILE_NO_SHOOT | MAPTILE_OFFSET_PIC)


static int GetDoorCountInGroup(
	const Map *map, const Vec2i v, const bool isHorizontal);
static NamedPic *GetDoorBasePic(
	const PicManager *pm, const char *style, const bool isHorizontal);
static TWatch *CreateCloseDoorWatch(
	Map *map, const Mission *m, const Vec2i v,
	const bool isHorizontal, const int doorGroupCount,
	const char *picAltName, const int floor, const int room);
static Trigger *CreateOpenDoorTrigger(
	Map *map, const Mission *m, const Vec2i v,
	const bool isHorizontal, const int doorGroupCount,
	const int floor, const int room, const int keyFlags);
void MapAddDoorGroup(
	Map *map, const Mission *m,
	const Vec2i v, const int floor, const int room, const int keyFlags)
{
	const unsigned short tileLeftType =
		IMapGet(map, Vec2iNew(v.x - 1, v.y)) & MAP_MASKACCESS;
	const unsigned short tileRightType =
		IMapGet(map, Vec2iNew(v.x + 1, v.y)) & MAP_MASKACCESS;
	const bool isHorizontal =
		tileLeftType == MAP_WALL || tileRightType == MAP_WALL ||
		tileLeftType == MAP_DOOR || tileRightType == MAP_DOOR ||
		tileLeftType == MAP_NOTHING || tileRightType == MAP_NOTHING;
	const int doorGroupCount = GetDoorCountInGroup(map, v, isHorizontal);
	const Vec2i dv = Vec2iNew(isHorizontal ? 1 : 0, isHorizontal ? 0 : 1);
	const Vec2i dAside = Vec2iNew(dv.y, dv.x);

	const char *doorKey;
	switch (keyFlags)
	{
	case FLAGS_KEYCARD_RED:		doorKey = "red";	break;
	case FLAGS_KEYCARD_BLUE:	doorKey = "blue";	break;
	case FLAGS_KEYCARD_GREEN:	doorKey = "green";	break;
	case FLAGS_KEYCARD_YELLOW:	doorKey = "yellow";	break;
	default:					doorKey = "normal";	break;
	}
	NamedPic *doorPic =
		GetDoorPic(&gPicManager, m->DoorStyle, doorKey, isHorizontal);

	// set up the door pics
	for (int i = 0; i < doorGroupCount; i++)
	{
		const Vec2i vI = Vec2iAdd(v, Vec2iScale(dv, i));
		Tile *tile = MapGetTile(map, vI);
		tile->picAlt = doorPic;
		tile->pic = GetDoorBasePic(&gPicManager, m->DoorStyle, isHorizontal);
		tile->flags = DOOR_TILE_FLAGS;
		if (isHorizontal)
		{
			const Vec2i vB = Vec2iAdd(vI, dAside);
			Tile *tileB = MapGetTile(map, vB);
			CASSERT(TileCanWalk(MapGetTile(
				map, Vec2iNew(vI.x - dAside.x, vI.y - dAside.y))),
				"map gen error: entrance should be clear");
			CASSERT(TileCanWalk(tileB),
				"map gen error: entrance should be clear");
			// Change the tile below to shadow, cast by this door
			const bool isFloor = IMapGet(map, vB) == MAP_FLOOR;
			tileB->pic = PicManagerGetMaskedStylePic(
				&gPicManager,
				isFloor ? "floor" : "room",
				isFloor ? floor : room,
				isFloor ? FLOOR_SHADOW : ROOMFLOOR_SHADOW,
				isFloor ? m->FloorMask : m->RoomMask, m->AltMask);
		}
	}

	TWatch *w = CreateCloseDoorWatch(
		map, m, v, isHorizontal, doorGroupCount, doorPic->name, floor, room);
	Trigger *t = CreateOpenDoorTrigger(
		map, m, v, isHorizontal, doorGroupCount,
		floor, room, keyFlags);
	// Connect trigger and watch up
	Action *a = TriggerAddAction(t);
	a->Type = ACTION_ACTIVATEWATCH;
	a->u.index = w->index;
	a = WatchAddAction(w);
	a->Type = ACTION_SETTRIGGER;
	a->u.index = t->id;

	// Set tiles on and besides doors free
	for (int i = 0; i < doorGroupCount; i++)
	{
		const Vec2i vI = Vec2iAdd(v, Vec2iScale(dv, i));
		IMapSet(map, vI, IMapGet(map, vI) | MAP_LEAVEFREE);
		const Vec2i vI1 = Vec2iAdd(vI, dAside);
		IMapSet(map, vI1, IMapGet(map, vI1) | MAP_LEAVEFREE);
		const Vec2i vI2 = Vec2iMinus(vI, dAside);
		IMapSet(map, vI2, IMapGet(map, vI2) | MAP_LEAVEFREE);
	}
}

// Count the number of doors that are in the same group as this door
// Only check to the right/below
static int GetDoorCountInGroup(
	const Map *map, const Vec2i v, const bool isHorizontal)
{
	const Vec2i dv = Vec2iNew(isHorizontal ? 1 : 0, isHorizontal ? 0 : 1);
	int count = 0;
	for (Vec2i vi = v;
		(IMapGet(map, vi) & MAP_MASKACCESS) == MAP_DOOR;
		vi = Vec2iAdd(vi, dv))
	{
		count++;
	}
	return count;
}
// 1 second to close doors
#define CLOSE_DOOR_TICKS FPS_FRAMELIMIT
// Create the watch responsible for closing the door
static TWatch *CreateCloseDoorWatch(
	Map *map, const Mission *m, const Vec2i v,
	const bool isHorizontal, const int doorGroupCount,
	const char *picAltName, const int floor, const int room)
{
	TWatch *w = WatchNew();
	const Vec2i dv = Vec2iNew(isHorizontal ? 1 : 0, isHorizontal ? 0 : 1);
	const Vec2i dAside = Vec2iNew(dv.y, dv.x);

	// The conditions are that the tile above, at and below the doors are empty
	for (int i = 0; i < doorGroupCount; i++)
	{
		const Vec2i vI = Vec2iAdd(v, Vec2iScale(dv, i));

		WatchAddCondition(
			w, CONDITION_TILECLEAR, CLOSE_DOOR_TICKS, Vec2iMinus(vI, dAside));
		WatchAddCondition(
			w, CONDITION_TILECLEAR, CLOSE_DOOR_TICKS, vI);
		WatchAddCondition(
			w, CONDITION_TILECLEAR, CLOSE_DOOR_TICKS, Vec2iAdd(vI, dAside));
	}

	// Now the actions of the watch once it's triggered
	Action *a;

	// Deactivate itself
	a = WatchAddAction(w);
	a->Type = ACTION_DEACTIVATEWATCH;
	a->u.index = w->index;
	// play close sound at the center of the door group
	a = WatchAddAction(w);
	a->Type = ACTION_SOUND;
	a->u.pos = Vec2iCenterOfTile(
		Vec2iAdd(v, Vec2iScale(dv, doorGroupCount / 2)));
	a->a.Sound = StrSound("door_close");

	// Close doors
	for (int i = 0; i < doorGroupCount; i++)
	{
		const Vec2i vI = Vec2iAdd(v, Vec2iScale(dv, i));

		a = WatchAddAction(w);
		a->Type = ACTION_EVENT;
		a->a.Event = GameEventNew(GAME_EVENT_TILE_SET);
		a->a.Event.u.TileSet.Pos = Vec2i2Net(vI);
		a->a.Event.u.TileSet.Flags = DOOR_TILE_FLAGS;
		strcpy(
			a->a.Event.u.TileSet.PicName,
			GetDoorBasePic(&gPicManager, m->DoorStyle, isHorizontal)->name);
		strcpy(a->a.Event.u.TileSet.PicAltName, picAltName);
	}

	// Add shadows below doors
	if (isHorizontal)
	{
		for (int i = 0; i < doorGroupCount; i++)
		{
			const Vec2i vI = Vec2iAdd(v, Vec2iScale(dv, i));
			
			a = WatchAddAction(w);
			a->Type = ACTION_EVENT;
			a->a.Event = GameEventNew(GAME_EVENT_TILE_SET);
			const Vec2i vI2 = Vec2iNew(vI.x + dAside.x, vI.y + dAside.y);
			a->a.Event.u.TileSet.Pos = Vec2i2Net(vI2);
			const bool isFloor = IMapGet(map, vI2) == MAP_FLOOR;
			strcpy(
				a->a.Event.u.TileSet.PicName,
				PicManagerGetMaskedStylePic(
					&gPicManager,
					isFloor ? "floor" : "room",
					isFloor ? floor : room,
					isFloor ? FLOOR_SHADOW : ROOMFLOOR_SHADOW,
					isFloor ? m->FloorMask : m->RoomMask,
					m->AltMask)->name);
		}
	}

	return w;
}
static void TileAddTrigger(Tile *t, Trigger *tr);
static Trigger *CreateOpenDoorTrigger(
	Map *map, const Mission *m, const Vec2i v,
	const bool isHorizontal, const int doorGroupCount,
	const int floor, const int room, const int keyFlags)
{
	// All tiles on either side of the door group use the same trigger
	const Vec2i dv = Vec2iNew(isHorizontal ? 1 : 0, isHorizontal ? 0 : 1);
	const Vec2i dAside = Vec2iNew(dv.y, dv.x);
	Trigger *t = MapNewTrigger(map);
	t->flags = keyFlags;

	// Deactivate itself
	Action *a;
	a = TriggerAddAction(t);
	a->Type = ACTION_CLEARTRIGGER;
	a->u.index = t->id;

	// Open doors
	for (int i = 0; i < doorGroupCount; i++)
	{
		const Vec2i vI = Vec2iAdd(v, Vec2iScale(dv, i));
		a = TriggerAddAction(t);
		a->Type = ACTION_EVENT;
		a->a.Event = GameEventNew(GAME_EVENT_TILE_SET);
		a->a.Event.u.TileSet.Pos = Vec2i2Net(vI);
		a->a.Event.u.TileSet.Flags =
			(isHorizontal || i > 0) ? 0 : MAPTILE_OFFSET_PIC;
		strcpy(
			a->a.Event.u.TileSet.PicName,
			GetDoorBasePic(&gPicManager, m->DoorStyle, isHorizontal)->name);
		if (!isHorizontal && i == 0)
		{
			// special door cavity picture
			strcpy(
				a->a.Event.u.TileSet.PicAltName,
				GetDoorPic(&gPicManager, m->DoorStyle, "wall", false)->name);
		}
	}

	// Change tiles below the doors
	if (isHorizontal)
	{
		for (int i = 0; i < doorGroupCount; i++)
		{
			const Vec2i vI = Vec2iAdd(v, Vec2iScale(dv, i));
			const Vec2i vIAside = Vec2iAdd(vI, dAside);
			a = TriggerAddAction(t);
			// Remove shadows below doors
			a->Type = ACTION_EVENT;
			a->a.Event = GameEventNew(GAME_EVENT_TILE_SET);
			const bool isFloor = IMapGet(map, vIAside) == MAP_FLOOR;
			a->a.Event.u.TileSet.Pos = Vec2i2Net(vIAside);
			strcpy(
				a->a.Event.u.TileSet.PicName,
				PicManagerGetMaskedStylePic(
					&gPicManager,
					isFloor ? "floor" : "room",
					isFloor? floor : room,
					isFloor ? FLOOR_NORMAL : ROOMFLOOR_NORMAL,
					isFloor ? m->FloorMask : m->RoomMask,
					m->AltMask)->name);
		}
	}

	// Now place the two triggers on the tiles along either side of the doors
	for (int i = 0; i < doorGroupCount; i++)
	{
		const Vec2i vI = Vec2iAdd(v, Vec2iScale(dv, i));
		const Vec2i vIA = Vec2iMinus(vI, dAside);
		TileAddTrigger(MapGetTile(map, vIA), t);
		const Vec2i vIB = Vec2iAdd(vI, dAside);
		TileAddTrigger(MapGetTile(map, vIB), t);
	}

	/// play sound at the center of the door group
	a = TriggerAddAction(t);
	a->Type = ACTION_SOUND;
	a->u.pos = Vec2iCenterOfTile(
		Vec2iAdd(v, Vec2iScale(dv, doorGroupCount / 2)));
	a->a.Sound = StrSound("door");

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

static NamedPic *GetDoorBasePic(
	const PicManager *pm, const char *style, const bool isHorizontal)
{
	return GetDoorPic(pm, style, "open", isHorizontal);
}


// Old door pics definitions
typedef struct
{
	int H;
	int V;
} DoorPic;

typedef struct
{
	DoorPic Normal;
	DoorPic Yellow;
	DoorPic Green;
	DoorPic Blue;
	DoorPic Red;
	int OpenH;
	int Wall;
} DoorPics;

// note that the H pic in the last pair is a TILE pic, not an offset pic!
/*static DoorPics doorStyles[] =
{
	// Office doors
	{
		{OFSPIC_DOOR, OFSPIC_VDOOR},
		{OFSPIC_HDOOR_YELLOW, OFSPIC_VDOOR_YELLOW},
		{OFSPIC_HDOOR_GREEN, OFSPIC_VDOOR_GREEN},
		{OFSPIC_HDOOR_BLUE, OFSPIC_VDOOR_BLUE},
		{OFSPIC_HDOOR_RED, OFSPIC_VDOOR_RED},
		109,
		OFSPIC_VDOOR_OPEN
	},
	// Dungeon doors
	{
		{OFSPIC_DOOR2, OFSPIC_VDOOR2},
		{OFSPIC_HDOOR2_YELLOW, OFSPIC_VDOOR2_YELLOW},
		{OFSPIC_HDOOR2_GREEN, OFSPIC_VDOOR2_GREEN},
		{OFSPIC_HDOOR2_BLUE, OFSPIC_VDOOR2_BLUE},
		{OFSPIC_HDOOR2_RED, OFSPIC_VDOOR2_RED},
		342,
		OFSPIC_VDOOR2_OPEN
	},
	// "Blast" doors
	{
		{OFSPIC_HDOOR3, OFSPIC_VDOOR3},
		{OFSPIC_HDOOR3_YELLOW, OFSPIC_VDOOR3_YELLOW},
		{OFSPIC_HDOOR3_GREEN, OFSPIC_VDOOR3_GREEN},
		{OFSPIC_HDOOR3_BLUE, OFSPIC_VDOOR3_BLUE},
		{OFSPIC_HDOOR3_RED, OFSPIC_VDOOR3_RED},
		P2 + 148,
		OFSPIC_VDOOR2_OPEN
	},
	// Alien doors
	{
		{OFSPIC_HDOOR4, OFSPIC_VDOOR4},
		{OFSPIC_HDOOR4_YELLOW, OFSPIC_VDOOR4_YELLOW},
		{OFSPIC_HDOOR4_GREEN, OFSPIC_VDOOR4_GREEN},
		{OFSPIC_HDOOR4_BLUE, OFSPIC_VDOOR4_BLUE},
		{OFSPIC_HDOOR4_RED, OFSPIC_VDOOR4_RED},
		P2 + 163,
		OFSPIC_VDOOR2_OPEN
	}
};
#define DOORSTYLE_COUNT (sizeof doorStyles / sizeof(DoorPics))*/
#define DOORSTYLE_COUNT 4

NamedPic *GetDoorPic(
	const PicManager *pm, const char *style, const char *key,
	const bool isHorizontal)
{
	char buf[CDOGS_FILENAME_MAX];
	// Construct filename
	// If the key is "wall", it doesn't include orientation
	sprintf(
		buf, "door/%s_%s%s", style, key,
		strcmp(key, "wall") == 0 ? "" : (isHorizontal ? "_h" : "_v"));
/*
	// TODO: support using original pics
	// Requires original pics accessible via name, i.e. using the NamedPic
	// type

	// Find the original pic index, if available
	int oldIdx = -1;
	// Door style
	const DoorPics *dp = NULL;
	if (strcmp(style, "office") == 0) dp = &doorStyles[0];
	else if (strcmp(style, "dungeon") == 0) dp = &doorStyles[1];
	else if (strcmp(style, "blast") == 0) dp = &doorStyles[2];
	else if (strcmp(style, "alien") == 0) dp = &doorStyles[3];
	if (dp != NULL)
	{
		// Door key
		const DoorPic *d = NULL;
		if (strcmp(key, "normal") == 0) d = &dp->Normal;
		else if (strcmp(key, "yellow") == 0) d = &dp->Yellow;
		else if (strcmp(key, "green") == 0) d = &dp->Green;
		else if (strcmp(key, "blue") == 0) d = &dp->Blue;
		else if (strcmp(key, "red") == 0) d = &dp->Red;
		else if (strcmp(key, "wall") == 0) oldIdx = dp->Wall;
		else if (strcmp(key, "open") == 0 && isHorizontal) oldIdx = dp->OpenH;
		// Note that old pics don't contain the open V tile
		if (d != NULL)
		{
			// Door orientation
			oldIdx = cGeneralPics[isHorizontal ? d->H : d->V].picIndex;
		}
	}
*/
	return PicManagerGetNamedPic(pm, buf);
}

const char *DoorStyleStr(const int style)
{
	// fix bugs with old campaigns
	switch (abs(style) % DOORSTYLE_COUNT)
	{
		case 0: return "office";
		case 1: return "dungeon";
		case 2: return "blast";
		case 3: return "alien";
		default: return "office";
	}
}
