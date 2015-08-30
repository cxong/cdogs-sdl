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

#define DOOR_TILE_FLAGS \
	(MAPTILE_NO_SEE | MAPTILE_NO_WALK | MAPTILE_NO_SHOOT | MAPTILE_OFFSET_PIC)


static int GetDoorCountInGroup(
	const Map *map, const Vec2i v, const bool isHorizontal);
static void PicLoadOffset(Pic *picAlt, const int idx);
static TWatch *CreateCloseDoorWatch(
	Map *map, const Mission *m, const Vec2i v,
	const bool isHorizontal, const int doorGroupCount,
	const int pic, const int floor, const int room);
static Trigger *CreateOpenDoorTrigger(
	Map *map, const Mission *m, const Vec2i v,
	const bool isHorizontal, const int doorGroupCount,
	const int openDoorPic, const int floor, const int room,
	const int keyFlags);
void MapAddDoorGroup(
	Map *map, const Mission *m, const struct MissionOptions *mo,
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

	const DoorPic *dp;
	switch (keyFlags)
	{
	case FLAGS_KEYCARD_RED:
		dp = &mo->doorPics->Red;
		break;
	case FLAGS_KEYCARD_BLUE:
		dp = &mo->doorPics->Blue;
		break;
	case FLAGS_KEYCARD_GREEN:
		dp = &mo->doorPics->Green;
		break;
	case FLAGS_KEYCARD_YELLOW:
		dp = &mo->doorPics->Yellow;
		break;
	default:
		dp = &mo->doorPics->Normal;
		break;
	}
	int pic;
	int openDoorPic;
	if (isHorizontal)
	{
		pic = dp->H;
		openDoorPic = mo->doorPics->Open.H;
	}
	else
	{
		pic = dp->V;
		openDoorPic = mo->doorPics->Open.V;
	}

	// set up the door pics
	for (int i = 0; i < doorGroupCount; i++)
	{
		const Vec2i vI = Vec2iAdd(v, Vec2iScale(dv, i));
		Tile *tile = MapGetTile(map, vI);
		PicLoadOffset(&tile->picAlt, pic);
		tile->pic = PicManagerGetMaskedStylePic(
			&gPicManager, "room", room, ROOMFLOOR_SHADOW,
			m->RoomMask, m->AltMask);
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
		map, m, v, isHorizontal, doorGroupCount, pic, floor, room);
	Trigger *t = CreateOpenDoorTrigger(
		map, m, v, isHorizontal, doorGroupCount, openDoorPic, floor, room,
		keyFlags);
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
static void PicLoadOffset(Pic *picAlt, const int idx)
{
	Pic *oldPic =
		PicManagerGetFromOld(&gPicManager, cGeneralPics[idx].picIndex);
	CASSERT(oldPic != NULL, "Cannot find pic");
	CASSERT(picAlt != NULL, "Cannot use null alt pic");
	picAlt->size = oldPic->size;
	picAlt->Data = oldPic->Data;
	picAlt->offset = Vec2iNew(cGeneralPics[idx].dx, cGeneralPics[idx].dy);
}
// 1 second to close doors
#define CLOSE_DOOR_TICKS FPS_FRAMELIMIT
// Create the watch responsible for closing the door
static TWatch *CreateCloseDoorWatch(
	Map *map, const Mission *m, const Vec2i v,
	const bool isHorizontal, const int doorGroupCount,
	const int pic, const int floor, const int room)
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
		a->Type = ACTION_CHANGETILE;
		a->u.pos = vI;
		PicLoadOffset(&a->a.ChangeTile.PicAlt, pic);
		a->a.ChangeTile.Pic = PicManagerGetMaskedStylePic(
			&gPicManager, "room", room, ROOMFLOOR_SHADOW,
			m->RoomMask, m->AltMask);
		a->a.ChangeTile.Flags = DOOR_TILE_FLAGS;
	}

	// Add shadows below doors
	if (isHorizontal)
	{
		for (int i = 0; i < doorGroupCount; i++)
		{
			const Vec2i vI = Vec2iAdd(v, Vec2iScale(dv, i));
			
			a = WatchAddAction(w);
			a->Type = ACTION_CHANGETILE;
			a->u.pos = Vec2iNew(vI.x + dAside.x, vI.y + dAside.y);
			const bool isFloor = IMapGet(map, a->u.pos) == MAP_FLOOR;
			a->a.ChangeTile.Pic = PicManagerGetMaskedStylePic(
				&gPicManager,
				isFloor ? "floor" : "room",
				isFloor ? floor : room,
				isFloor ? FLOOR_SHADOW : ROOMFLOOR_SHADOW,
				isFloor ? m->FloorMask : m->RoomMask,
				m->AltMask);
		}
	}

	return w;
}
static void TileAddTrigger(Tile *t, Trigger *tr);
static Trigger *CreateOpenDoorTrigger(
	Map *map, const Mission *m, const Vec2i v,
	const bool isHorizontal, const int doorGroupCount,
	const int openDoorPic, const int floor, const int room,
	const int keyFlags)
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
		a->Type = ACTION_CHANGETILE;
		a->u.pos = vI;
		if (isHorizontal)
		{
			a->a.ChangeTile.Pic = PicManagerGetFromOld(&gPicManager, openDoorPic);
		}
		else
		{
			if (i == 0)
			{
				// special door cavity picture
				PicLoadOffset(&a->a.ChangeTile.PicAlt, openDoorPic);
			}
			// room floor pic when the door opens
			a->a.ChangeTile.Pic = PicManagerGetMaskedStylePic(
				&gPicManager, "room", room, ROOMFLOOR_SHADOW,
				m->RoomMask, m->AltMask);
		}
		a->a.ChangeTile.Flags = (isHorizontal || i > 0) ? 0 : MAPTILE_OFFSET_PIC;
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
			a->Type = ACTION_CHANGETILE;
			a->u.pos = vIAside;
			const bool isFloor = IMapGet(map, vIAside) == MAP_FLOOR;
			a->a.ChangeTile.Pic = PicManagerGetMaskedStylePic(
				&gPicManager,
				isFloor ? "floor" : "room",
				isFloor? floor : room,
				isFloor ? FLOOR_NORMAL : ROOMFLOOR_NORMAL,
				isFloor ? m->FloorMask : m->RoomMask,
				m->AltMask);
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
