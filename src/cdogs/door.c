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

	Copyright (c) 2013-2015, 2018-2021 Cong Xu
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
#include "log.h"
#include "net_util.h"

static DoorType GetDoorType(
	const bool isHorizontal, const int i, const int count)
{
	if (isHorizontal)
	{
		if (count == 1)
		{
			return DOORTYPE_H;
		}
		else if (i == 0)
		{
			return DOORTYPE_LEFT;
		}
		else if (i == count - 1)
		{
			return DOORTYPE_RIGHT;
		}
		else
		{
			return DOORTYPE_HMID;
		}
	}
	else
	{
		if (count == 1)
		{
			return DOORTYPE_V;
		}
		else if (i == 0)
		{
			return DOORTYPE_TOP;
		}
		else if (i == count - 1)
		{
			return DOORTYPE_BOTTOM;
		}
		else
		{
			return DOORTYPE_VMID;
		}
	}
}
static bool DoorTypeIsHorizontal(const DoorType type)
{
	return type == DOORTYPE_H || type == DOORTYPE_LEFT ||
		   type == DOORTYPE_HMID || type == DOORTYPE_RIGHT;
}

static bool DoorGroupIsHorizontal(const MapBuilder *mb, const struct vec2i v);
static void DoorGetClassName(
	char *buf, const TileClass *door, const char *key, const DoorType dType);
static int GetDoorCountInGroup(
	const MapBuilder *mb, const struct vec2i v, const bool isHorizontal);
static TWatch *CreateCloseDoorWatch(
	MapBuilder *mb, const struct vec2i v, const bool isHorizontal,
	const int doorGroupCount, const char *doorKey);
static Trigger *CreateOpenDoorTrigger(
	MapBuilder *mb, const struct vec2i v, const bool isHorizontal,
	const int doorGroupCount, const int keyFlags);
void MapAddDoorGroup(MapBuilder *mb, const struct vec2i v, const int keyFlags)
{
	const TileClass *door = MapBuilderGetTile(mb, v);
	const bool isHorizontal = DoorGroupIsHorizontal(mb, v);
	const int doorGroupCount = GetDoorCountInGroup(mb, v, isHorizontal);
	const struct vec2i dv = svec2i(isHorizontal ? 1 : 0, isHorizontal ? 0 : 1);
	const struct vec2i dAside = svec2i(dv.y, dv.x);

	const char *doorKey;
	switch (keyFlags)
	{
	case FLAGS_KEYCARD_RED:
		doorKey = "red";
		break;
	case FLAGS_KEYCARD_BLUE:
		doorKey = "blue";
		break;
	case FLAGS_KEYCARD_GREEN:
		doorKey = "green";
		break;
	case FLAGS_KEYCARD_YELLOW:
		doorKey = "yellow";
		break;
	default:
		doorKey = "normal";
		break;
	}

	// set up the door pics
	for (int i = 0; i < doorGroupCount; i++)
	{
		char doorClassName[CDOGS_FILENAME_MAX];
		const DoorType type = GetDoorType(isHorizontal, i, doorGroupCount);
		DoorGetClassName(doorClassName, door, doorKey, type);
		const TileClass *doorClass = StrTileClass(doorClassName);
		DoorGetClassName(doorClassName, door, "open", type);
		const TileClass *doorClassOpen = StrTileClass(doorClassName);
		const struct vec2i vI = svec2i_add(v, svec2i_scale(dv, (float)i));
		Tile *tile = MapGetTile(mb->Map, vI);
		tile->ClassAlt = doorClass;
		tile->Class = doorClassOpen;
		if (isHorizontal)
		{
			const struct vec2i vB = svec2i_add(vI, dAside);
			Tile *tileB = MapGetTile(mb->Map, vB);
			if (!TileCanWalk(MapGetTile(
					mb->Map, svec2i(vI.x - dAside.x, vI.y - dAside.y))))
			{
				LOG(LM_MAP, LL_ERROR,
					"map gen error: entrance above should be clear");
			}
			if (TileCanWalk(tileB))
			{
				// Change the tile below to shadow, cast by this door
				tileB->Class = TileClassesGetMaskedTile(
					tileB->Class, tileB->Class->Style, "shadow",
					tileB->Class->Mask, tileB->Class->MaskAlt);
			}
			else
			{
				LOG(LM_MAP, LL_ERROR,
					"map gen error: entrance below should be clear");
			}
		}
	}

	TWatch *w =
		CreateCloseDoorWatch(mb, v, isHorizontal, doorGroupCount, doorKey);
	Trigger *t =
		CreateOpenDoorTrigger(mb, v, isHorizontal, doorGroupCount, keyFlags);
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
		const struct vec2i vI = svec2i_add(v, svec2i_scale(dv, (float)i));
		MapBuilderSetLeaveFree(mb, vI, true);
		const struct vec2i vI1 = svec2i_add(vI, dAside);
		MapBuilderSetLeaveFree(mb, vI1, true);
		const struct vec2i vI2 = svec2i_subtract(vI, dAside);
		MapBuilderSetLeaveFree(mb, vI2, true);
	}
}
static bool DoorGroupIsHorizontal(const MapBuilder *mb, const struct vec2i v)
{
	const TileClass *tileLeftType =
		MapBuilderGetTile(mb, svec2i(v.x - 1, v.y));
	const TileClass *tileRightType =
		MapBuilderGetTile(mb, svec2i(v.x + 1, v.y));
	const TileClass *tileAboveType =
		MapBuilderGetTile(mb, svec2i(v.x, v.y - 1));
	const TileClass *tileBelowType =
		MapBuilderGetTile(mb, svec2i(v.x, v.y + 1));
	const bool tileLeftIsDoor =
		tileLeftType != NULL && tileLeftType->Type == TILE_CLASS_DOOR;
	const bool tileRightIsDoor =
		tileRightType != NULL && tileRightType->Type == TILE_CLASS_DOOR;
	const bool tileAboveIsDoor =
		tileAboveType != NULL && tileAboveType->Type == TILE_CLASS_DOOR;
	const bool tileBelowIsDoor =
		tileBelowType != NULL && tileBelowType->Type == TILE_CLASS_DOOR;
	const bool tileLeftCanWalk = tileLeftType != NULL && tileLeftType->canWalk;
	const bool tileRightCanWalk =
		tileRightType != NULL && tileRightType->canWalk;
	const bool tileAboveCanWalk = tileAboveType != NULL && tileAboveType->canWalk;
	const bool tileBelowCanWalk =
		tileBelowType != NULL && tileBelowType->canWalk;
	// If door is free all around, follow doors
	if (tileAboveCanWalk && tileBelowCanWalk && tileLeftCanWalk && tileRightCanWalk)
	{
		if (tileLeftIsDoor && tileRightIsDoor)
		{
			return true;
		}
		if (tileAboveIsDoor && tileBelowIsDoor)
		{
			return false;
		}
		return tileLeftIsDoor || tileRightIsDoor;
	}
	// If door is free both above/below or both left/right
	if (tileAboveCanWalk && tileBelowCanWalk)
	{
		return true;
	}
	if (tileLeftCanWalk && tileRightCanWalk)
	{
		return false;
	}
	// If door is free only one of above/below/left/right
	if (tileAboveCanWalk || tileBelowCanWalk)
	{
		return true;
	}
	return false;
}

// Count the number of doors that are in the same group as this door
// Only check to the right/below
static int GetDoorCountInGroup(
	const MapBuilder *mb, const struct vec2i v, const bool isHorizontal)
{
	const struct vec2i dv = svec2i(isHorizontal ? 1 : 0, isHorizontal ? 0 : 1);
	int count = 0;
	for (struct vec2i vi = v;
		 MapIsTileIn(mb->Map, vi) &&
		 MapBuilderGetTile(mb, vi)->Type == TILE_CLASS_DOOR;
		 vi = svec2i_add(vi, dv))
	{
		count++;
	}
	return count;
}
// Create the watch responsible for closing the door
static TWatch *CreateCloseDoorWatch(
	MapBuilder *mb, const struct vec2i v, const bool isHorizontal,
	const int doorGroupCount, const char *doorKey)
{
	TWatch *w = WatchNew();
	const struct vec2i dv = svec2i(isHorizontal ? 1 : 0, isHorizontal ? 0 : 1);
	const struct vec2i dAside = svec2i(dv.y, dv.x);

	// The conditions are that the tile above, at and below the doors are empty
	for (int i = 0; i < doorGroupCount; i++)
	{
		const struct vec2i vI = svec2i_add(v, svec2i_scale(dv, (float)i));

		for (int j = -1; j <= 1; j++)
		{
			WatchAddCondition(
				w, CONDITION_TILECLEAR, gCampaign.Setting.DoorOpenTicks,
				svec2i_add(vI, svec2i_scale(dAside, (float)j)));
		}
	}

	// Now the actions of the watch once it's triggered
	Action *a;

	// Deactivate itself
	a = WatchAddAction(w);
	a->Type = ACTION_DEACTIVATEWATCH;
	a->u.index = w->index;
	// play close sound at the center of the door group
	a = WatchAddAction(w);
	a->Type = ACTION_EVENT;
	a->u.Event = GameEventNew(GAME_EVENT_SOUND_AT);
	strcpy(a->u.Event.u.SoundAt.Sound, "door_close");
	a->u.Event.u.SoundAt.Pos = Vec2ToNet(Vec2CenterOfTile(svec2i_add(v, svec2i_scale(dv, (float)doorGroupCount / 2))));

	// Close doors
	const TileClass *door = MapBuilderGetTile(mb, v);
	for (int i = 0; i < doorGroupCount; i++)
	{
		const struct vec2i vI = svec2i_add(v, svec2i_scale(dv, (float)i));

		a = WatchAddAction(w);
		a->Type = ACTION_EVENT;
		a->u.Event = GameEventNew(GAME_EVENT_TILE_SET);
		a->u.Event.u.TileSet.Pos = Vec2i2Net(vI);
		const DoorType type = GetDoorType(isHorizontal, i, doorGroupCount);
		DoorGetClassName(
			a->u.Event.u.TileSet.ClassName, door, "open", type);

		char doorClassName[CDOGS_FILENAME_MAX];
		DoorGetClassName(doorClassName, door, doorKey, type);
		strcpy(a->u.Event.u.TileSet.ClassAltName, doorClassName);
	}

	// Add shadows below doors
	if (isHorizontal)
	{
		for (int i = 0; i < doorGroupCount; i++)
		{
			const struct vec2i vI = svec2i_add(v, svec2i_scale(dv, (float)i));

			a = WatchAddAction(w);
			a->Type = ACTION_EVENT;
			a->u.Event = GameEventNew(GAME_EVENT_TILE_SET);
			const struct vec2i vI2 = svec2i(vI.x + dAside.x, vI.y + dAside.y);
			a->u.Event.u.TileSet.Pos = Vec2i2Net(vI2);
			const TileClass *t = MapBuilderGetTile(mb, vI2);
			TileClassGetName(
				a->u.Event.u.TileSet.ClassName, t, t->Style, "shadow", t->Mask,
				t->MaskAlt);
		}
	}

	return w;
}
static void TileAddTrigger(Tile *t, Trigger *tr);
static Trigger *CreateOpenDoorTrigger(
	MapBuilder *mb, const struct vec2i v, const bool isHorizontal,
	const int doorGroupCount, const int keyFlags)
{
	// All tiles on either side of the door group use the same trigger
	const struct vec2i dv = svec2i(isHorizontal ? 1 : 0, isHorizontal ? 0 : 1);
	const struct vec2i dAside = svec2i(dv.y, dv.x);
	Trigger *t = MapNewTrigger(mb->Map);
	t->flags = keyFlags;

	// Deactivate itself
	Action *a;
	a = TriggerAddAction(t);
	a->Type = ACTION_CLEARTRIGGER;
	a->u.index = t->id;

	// Open doors
	const TileClass *door = MapBuilderGetTile(mb, v);
	for (int i = 0; i < doorGroupCount; i++)
	{
		const struct vec2i vI = svec2i_add(v, svec2i_scale(dv, (float)i));
		a = TriggerAddAction(t);
		a->Type = ACTION_EVENT;
		a->u.Event = GameEventNew(GAME_EVENT_TILE_SET);
		a->u.Event.u.TileSet.Pos = Vec2i2Net(vI);
		const DoorType type = GetDoorType(isHorizontal, i, doorGroupCount);
		DoorGetClassName(
			a->u.Event.u.TileSet.ClassName, door, "open", type);
		if (type == DOORTYPE_TOP || type == DOORTYPE_V)
		{
			// special door cavity picture
			DoorGetClassName(
				a->u.Event.u.TileSet.ClassAltName, door, "wall", type);
		}
	}

	// Change tiles below the doors
	if (isHorizontal)
	{
		for (int i = 0; i < doorGroupCount; i++)
		{
			const struct vec2i vI = svec2i_add(v, svec2i_scale(dv, (float)i));
			const struct vec2i vIAside = svec2i_add(vI, dAside);
			a = TriggerAddAction(t);
			// Remove shadows below doors
			a->Type = ACTION_EVENT;
			a->u.Event = GameEventNew(GAME_EVENT_TILE_SET);
			const TileClass *tc = MapBuilderGetTile(mb, vIAside);
			a->u.Event.u.TileSet.Pos = Vec2i2Net(vIAside);
			TileClassGetName(
				a->u.Event.u.TileSet.ClassName, tc, tc->Style, "normal",
				tc->Mask, tc->MaskAlt);
		}
	}

	// Now place the two triggers on the tiles along either side of the doors
	for (int i = 0; i < doorGroupCount; i++)
	{
		const struct vec2i vI = svec2i_add(v, svec2i_scale(dv, (float)i));
		const struct vec2i vIA = svec2i_subtract(vI, dAside);
		TileAddTrigger(MapGetTile(mb->Map, vIA), t);
		const struct vec2i vIB = svec2i_add(vI, dAside);
		TileAddTrigger(MapGetTile(mb->Map, vIB), t);
	}

	/// play sound at the center of the door group
	a = TriggerAddAction(t);
	a->Type = ACTION_EVENT;
	a->u.Event = GameEventNew(GAME_EVENT_SOUND_AT);
	strcpy(a->u.Event.u.SoundAt.Sound, "door");
	a->u.Event.u.SoundAt.Pos = Vec2ToNet(Vec2CenterOfTile(svec2i_add(v, svec2i_scale(dv, (float)doorGroupCount / 2))));

	return t;
}
static void TileAddTrigger(Tile *t, Trigger *tr)
{
	if (t == NULL) return;
	CArrayPushBack(&t->triggers, &tr);
}

// Get the tile class of a door; if it doesn't exist create it
// style: office/dungeon/blast/alien, or custom
// key: normal/yellow/green/blue/red/wall/open
static void DoorGetTypeName(char *buf, const char *key, const DoorType type)
{
	const char *typeStr = "";
	if (strcmp(key, "wall") == 0)
	{
		// no change
	}
	else if (strcmp(key, "open") == 0)
	{
		typeStr = DoorTypeIsHorizontal(type) ? "_h" : "_v";
	}
	else
	{
		switch (type)
		{
		case DOORTYPE_H:
			typeStr = "_h";
			break;
		case DOORTYPE_LEFT:
			typeStr = "_left";
			break;
		case DOORTYPE_HMID:
			typeStr = "_hmid";
			break;
		case DOORTYPE_RIGHT:
			typeStr = "_right";
			break;
		case DOORTYPE_V:
			typeStr = "_v";
			break;
		case DOORTYPE_TOP:
			typeStr = "_top";
			break;
		case DOORTYPE_VMID:
			typeStr = "_vmid";
			break;
		case DOORTYPE_BOTTOM:
			typeStr = "_bottom";
			break;
		default:
			CASSERT(false, "unknown doortype");
			break;
		}
	}
	sprintf(buf, "%s%s", key, typeStr);
}
static void DoorGetClassName(
	char *buf, const TileClass *door, const char *key, const DoorType dType)
{
	char type[256];
	DoorGetTypeName(type, key, dType);
	const color_t mask = strcmp(key, "normal") == 0 ? door->Mask : colorWhite;
	const color_t maskAlt = strcmp(key, "normal") == 0 ? door->MaskAlt : colorWhite;
	// If the key is "wall", it doesn't include orientation
	TileClassGetName(buf, door, door->Style, type, mask, maskAlt);
}
void DoorAddClass(
	TileClasses *c, PicManager *pm, const TileClass *base, const char *key,
	const DoorType type)
{
	char buf[CDOGS_FILENAME_MAX];
	DoorGetTypeName(buf, key, type);
	const color_t mask = strcmp(key, "normal") == 0 ? base->Mask : colorWhite;
	const color_t maskAlt = strcmp(key, "normal") == 0 ? base->MaskAlt : colorWhite;
	PicManagerGenerateMaskedStylePic(
		pm, "door", base->Style, buf, mask, maskAlt, true);
	TileClass *t =
		TileClassesAdd(c, pm, base, base->Style, buf, mask, maskAlt);
	CASSERT(t != NULL, "cannot add door class");
	const bool isOpenOrWallCavity =
		strcmp(key, "open") == 0 || strcmp(key, "wall") == 0;
	t->isOpaque = !isOpenOrWallCavity;
	t->canWalk = isOpenOrWallCavity;
	t->shootable = !isOpenOrWallCavity;
}

#define DOORSTYLE_COUNT 4
const char *IntDoorStyle(const int i)
{
	static const char *doorStyles[] = {"office", "dungeon", "blast", "alien"};
	// fix bugs with old campaigns
	return doorStyles[abs(i) % DOORSTYLE_COUNT];
}
