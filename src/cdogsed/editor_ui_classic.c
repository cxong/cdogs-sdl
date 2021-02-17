/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.
	Copyright (c) 2013-2016, 2019-2021 Cong Xu
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
#include "editor_ui_classic.h"

#include <cdogs/draw/draw_actor.h>
#include <cdogs/font.h>
#include <cdogs/gamedata.h>

#include "editor_ui_common.h"

#define Y_ABS 200

static void DrawStyleArea(
	struct vec2i pos, const char *name, const Pic *pic, int idx, int count,
	int isHighlighted);

MISSION_CHECK_TYPE_FUNC(MAPTYPE_CLASSIC)
static const char *MissionGetWallCountStr(UIObject *o, void *data)
{
	static char s[128];
	UNUSED(o);
	Campaign *co = data;
	if (!CampaignGetCurrentMission(co))
		return NULL;
	sprintf(s, "Walls: %d", CampaignGetCurrentMission(co)->u.Classic.Walls);
	return s;
}
static const char *MissionGetWallLengthStr(UIObject *o, void *data)
{
	static char s[128];
	UNUSED(o);
	Campaign *co = data;
	if (!CampaignGetCurrentMission(co))
		return NULL;
	sprintf(s, "Len: %d", CampaignGetCurrentMission(co)->u.Classic.WallLength);
	return s;
}
static const char *MissionGetCorridorWidthStr(UIObject *o, void *data)
{
	static char s[128];
	UNUSED(o);
	Campaign *co = data;
	if (!CampaignGetCurrentMission(co))
		return NULL;
	sprintf(
		s, "CorridorWidth: %d",
		CampaignGetCurrentMission(co)->u.Classic.CorridorWidth);
	return s;
}
static const char *MissionGetRoomCountStr(UIObject *o, void *data)
{
	static char s[128];
	UNUSED(o);
	Campaign *co = data;
	if (!CampaignGetCurrentMission(co))
		return NULL;
	sprintf(
		s, "Rooms: %d", CampaignGetCurrentMission(co)->u.Classic.Rooms.Count);
	return s;
}
static const char *MissionGetRoomMinStr(UIObject *o, void *data)
{
	static char s[128];
	UNUSED(o);
	Campaign *co = data;
	if (!CampaignGetCurrentMission(co))
		return NULL;
	sprintf(
		s, "RoomMin: %d", CampaignGetCurrentMission(co)->u.Classic.Rooms.Min);
	return s;
}
static const char *MissionGetRoomMaxStr(UIObject *o, void *data)
{
	static char s[128];
	UNUSED(o);
	Campaign *co = data;
	if (!CampaignGetCurrentMission(co))
		return NULL;
	sprintf(
		s, "RoomMax: %d", CampaignGetCurrentMission(co)->u.Classic.Rooms.Max);
	return s;
}
static void MissionDrawEdgeRooms(
	UIObject *o, GraphicsDevice *g, struct vec2i pos, void *data)
{
	UNUSED(o);
	UNUSED(g);
	Campaign *co = data;
	if (!CampaignGetCurrentMission(co))
		return;
	DisplayFlag(
		svec2i_add(pos, o->Pos), "Edge rooms",
		CampaignGetCurrentMission(co)->u.Classic.Rooms.Edge,
		UIObjectIsHighlighted(o));
}
static void MissionDrawRoomsOverlap(
	UIObject *o, GraphicsDevice *g, struct vec2i pos, void *data)
{
	UNUSED(o);
	UNUSED(g);
	UNUSED(pos);
	Campaign *co = data;
	if (!CampaignGetCurrentMission(co))
		return;
	DisplayFlag(
		svec2i_add(pos, o->Pos), "Room overlap",
		CampaignGetCurrentMission(co)->u.Classic.Rooms.Overlap,
		UIObjectIsHighlighted(o));
}
static const char *MissionGetRoomWallCountStr(UIObject *o, void *data)
{
	static char s[128];
	UNUSED(o);
	Campaign *co = data;
	if (!CampaignGetCurrentMission(co))
		return NULL;
	sprintf(
		s, "RoomWalls: %d",
		CampaignGetCurrentMission(co)->u.Classic.Rooms.Walls);
	return s;
}
static const char *MissionGetRoomWallLenStr(UIObject *o, void *data)
{
	static char s[128];
	UNUSED(o);
	Campaign *co = data;
	if (!CampaignGetCurrentMission(co))
		return NULL;
	sprintf(
		s, "RoomWallLen: %d",
		CampaignGetCurrentMission(co)->u.Classic.Rooms.WallLength);
	return s;
}
static const char *MissionGetRoomWallPadStr(UIObject *o, void *data)
{
	static char s[128];
	UNUSED(o);
	Campaign *co = data;
	if (!CampaignGetCurrentMission(co))
		return NULL;
	sprintf(
		s, "RoomWallPad: %d",
		CampaignGetCurrentMission(co)->u.Classic.Rooms.WallPad);
	return s;
}
static const char *MissionGetSquareCountStr(UIObject *o, void *data)
{
	static char s[128];
	UNUSED(o);
	Campaign *co = data;
	if (!CampaignGetCurrentMission(co))
		return NULL;
	sprintf(s, "Sqr: %d", CampaignGetCurrentMission(co)->u.Classic.Squares);
	return s;
}
static void MissionDrawExitEnabled(
	UIObject *o, GraphicsDevice *g, struct vec2i pos, void *data)
{
	UNUSED(o);
	UNUSED(g);
	Campaign *co = data;
	if (!CampaignGetCurrentMission(co))
		return;
	DisplayFlag(
		svec2i_add(pos, o->Pos), "Exit",
		CampaignGetCurrentMission(co)->u.Classic.ExitEnabled,
		UIObjectIsHighlighted(o));
}
static void MissionDrawDoorEnabled(
	UIObject *o, GraphicsDevice *g, struct vec2i pos, void *data)
{
	UNUSED(o);
	UNUSED(g);
	Campaign *co = data;
	if (!CampaignGetCurrentMission(co))
		return;
	DisplayFlag(
		svec2i_add(pos, o->Pos), "Doors",
		CampaignGetCurrentMission(co)->u.Classic.Doors.Enabled,
		UIObjectIsHighlighted(o));
}
static const char *MissionGetDoorMinStr(UIObject *o, void *data)
{
	static char s[128];
	UNUSED(o);
	Campaign *co = data;
	if (!CampaignGetCurrentMission(co))
		return NULL;
	sprintf(
		s, "DoorMin: %d", CampaignGetCurrentMission(co)->u.Classic.Doors.Min);
	return s;
}
static const char *MissionGetDoorMaxStr(UIObject *o, void *data)
{
	static char s[128];
	UNUSED(o);
	Campaign *co = data;
	if (!CampaignGetCurrentMission(co))
		return NULL;
	sprintf(
		s, "DoorMax: %d", CampaignGetCurrentMission(co)->u.Classic.Doors.Max);
	return s;
}
static const char *MissionGetPillarCountStr(UIObject *o, void *data)
{
	static char s[128];
	UNUSED(o);
	Campaign *co = data;
	if (!CampaignGetCurrentMission(co))
		return NULL;
	sprintf(
		s, "Pillars: %d",
		CampaignGetCurrentMission(co)->u.Classic.Pillars.Count);
	return s;
}
static const char *MissionGetPillarMinStr(UIObject *o, void *data)
{
	static char s[128];
	UNUSED(o);
	Campaign *co = data;
	if (!CampaignGetCurrentMission(co))
		return NULL;
	sprintf(
		s, "PillarMin: %d",
		CampaignGetCurrentMission(co)->u.Classic.Pillars.Min);
	return s;
}
static const char *MissionGetPillarMaxStr(UIObject *o, void *data)
{
	static char s[128];
	UNUSED(o);
	Campaign *co = data;
	if (!CampaignGetCurrentMission(co))
		return NULL;
	sprintf(
		s, "PillarMax: %d",
		CampaignGetCurrentMission(co)->u.Classic.Pillars.Max);
	return s;
}

static EditorResult MissionChangeWallCount(void *data, int d)
{
	Campaign *co = data;
	CampaignGetCurrentMission(co)->u.Classic.Walls =
		CLAMP(CampaignGetCurrentMission(co)->u.Classic.Walls + d, 0, 200);
	return EDITOR_RESULT_CHANGED;
}
static EditorResult MissionChangeWallLength(void *data, int d)
{
	Campaign *co = data;
	CampaignGetCurrentMission(co)->u.Classic.WallLength =
		CLAMP(CampaignGetCurrentMission(co)->u.Classic.WallLength + d, 1, 100);
	return EDITOR_RESULT_CHANGED;
}
static EditorResult MissionChangeCorridorWidth(void *data, int d)
{
	Campaign *co = data;
	CampaignGetCurrentMission(co)->u.Classic.CorridorWidth = CLAMP(
		CampaignGetCurrentMission(co)->u.Classic.CorridorWidth + d, 1, 5);
	return EDITOR_RESULT_CHANGED;
}
static EditorResult MissionChangeRoomCount(void *data, int d)
{
	Campaign *co = data;
	CampaignGetCurrentMission(co)->u.Classic.Rooms.Count = CLAMP(
		CampaignGetCurrentMission(co)->u.Classic.Rooms.Count + d, 0, 100);
	return EDITOR_RESULT_CHANGED;
}
static EditorResult MissionChangeRoomMin(void *data, int d)
{
	Campaign *co = data;
	CampaignGetCurrentMission(co)->u.Classic.Rooms.Min =
		CLAMP(CampaignGetCurrentMission(co)->u.Classic.Rooms.Min + d, 5, 50);
	CampaignGetCurrentMission(co)->u.Classic.Rooms.Max =
		MAX(CampaignGetCurrentMission(co)->u.Classic.Rooms.Min,
			CampaignGetCurrentMission(co)->u.Classic.Rooms.Max);
	return EDITOR_RESULT_CHANGED;
}
static EditorResult MissionChangeRoomMax(void *data, int d)
{
	Campaign *co = data;
	CampaignGetCurrentMission(co)->u.Classic.Rooms.Max =
		CLAMP(CampaignGetCurrentMission(co)->u.Classic.Rooms.Max + d, 5, 50);
	CampaignGetCurrentMission(co)->u.Classic.Rooms.Min =
		MIN(CampaignGetCurrentMission(co)->u.Classic.Rooms.Min,
			CampaignGetCurrentMission(co)->u.Classic.Rooms.Max);
	return EDITOR_RESULT_CHANGED;
}
static EditorResult MissionChangeEdgeRooms(void *data, int d)
{
	UNUSED(d);
	Campaign *co = data;
	CampaignGetCurrentMission(co)->u.Classic.Rooms.Edge =
		!CampaignGetCurrentMission(co)->u.Classic.Rooms.Edge;
	return EDITOR_RESULT_CHANGED;
}
static EditorResult MissionChangeRoomsOverlap(void *data, int d)
{
	UNUSED(d);
	Campaign *co = data;
	CampaignGetCurrentMission(co)->u.Classic.Rooms.Overlap =
		!CampaignGetCurrentMission(co)->u.Classic.Rooms.Overlap;
	return EDITOR_RESULT_CHANGED;
}
static EditorResult MissionChangeRoomWallCount(void *data, int d)
{
	Campaign *co = data;
	CampaignGetCurrentMission(co)->u.Classic.Rooms.Walls =
		CLAMP(CampaignGetCurrentMission(co)->u.Classic.Rooms.Walls + d, 0, 50);
	return EDITOR_RESULT_CHANGED;
}
static EditorResult MissionChangeRoomWallLen(void *data, int d)
{
	Campaign *co = data;
	CampaignGetCurrentMission(co)->u.Classic.Rooms.WallLength = CLAMP(
		CampaignGetCurrentMission(co)->u.Classic.Rooms.WallLength + d, 1, 50);
	return EDITOR_RESULT_CHANGED;
}
static EditorResult MissionChangeRoomWallPad(void *data, int d)
{
	Campaign *co = data;
	CampaignGetCurrentMission(co)->u.Classic.Rooms.WallPad = CLAMP(
		CampaignGetCurrentMission(co)->u.Classic.Rooms.WallPad + d, 1, 10);
	return EDITOR_RESULT_CHANGED;
}
static EditorResult MissionChangeSquareCount(void *data, int d)
{
	Campaign *co = data;
	CampaignGetCurrentMission(co)->u.Classic.Squares =
		CLAMP(CampaignGetCurrentMission(co)->u.Classic.Squares + d, 0, 100);
	return EDITOR_RESULT_CHANGED;
}
static EditorResult MissionChangeExitEnabled(void *data, int d)
{
	UNUSED(d);
	Campaign *co = data;
	CampaignGetCurrentMission(co)->u.Classic.ExitEnabled =
		!CampaignGetCurrentMission(co)->u.Classic.ExitEnabled;
	return EDITOR_RESULT_CHANGED;
}
static EditorResult MissionChangeDoorEnabled(void *data, int d)
{
	UNUSED(d);
	Campaign *co = data;
	CampaignGetCurrentMission(co)->u.Classic.Doors.Enabled =
		!CampaignGetCurrentMission(co)->u.Classic.Doors.Enabled;
	return EDITOR_RESULT_CHANGED;
}
static EditorResult MissionChangeDoorMin(void *data, int d)
{
	Campaign *co = data;
	CampaignGetCurrentMission(co)->u.Classic.Doors.Min =
		CLAMP(CampaignGetCurrentMission(co)->u.Classic.Doors.Min + d, 1, 6);
	CampaignGetCurrentMission(co)->u.Classic.Doors.Max =
		MAX(CampaignGetCurrentMission(co)->u.Classic.Doors.Min,
			CampaignGetCurrentMission(co)->u.Classic.Doors.Max);
	return EDITOR_RESULT_CHANGED;
}
static EditorResult MissionChangeDoorMax(void *data, int d)
{
	Campaign *co = data;
	CampaignGetCurrentMission(co)->u.Classic.Doors.Max =
		CLAMP(CampaignGetCurrentMission(co)->u.Classic.Doors.Max + d, 1, 6);
	CampaignGetCurrentMission(co)->u.Classic.Doors.Min =
		MIN(CampaignGetCurrentMission(co)->u.Classic.Doors.Min,
			CampaignGetCurrentMission(co)->u.Classic.Doors.Max);
	return EDITOR_RESULT_CHANGED;
}
static EditorResult MissionChangePillarCount(void *data, int d)
{
	Campaign *co = data;
	CampaignGetCurrentMission(co)->u.Classic.Pillars.Count = CLAMP(
		CampaignGetCurrentMission(co)->u.Classic.Pillars.Count + d, 0, 50);
	return EDITOR_RESULT_CHANGED;
}
static EditorResult MissionChangePillarMin(void *data, int d)
{
	Campaign *co = data;
	CampaignGetCurrentMission(co)->u.Classic.Pillars.Min =
		CLAMP(CampaignGetCurrentMission(co)->u.Classic.Pillars.Min + d, 1, 50);
	CampaignGetCurrentMission(co)->u.Classic.Pillars.Max =
		MAX(CampaignGetCurrentMission(co)->u.Classic.Pillars.Min,
			CampaignGetCurrentMission(co)->u.Classic.Pillars.Max);
	return EDITOR_RESULT_CHANGED;
}
static EditorResult MissionChangePillarMax(void *data, int d)
{
	Campaign *co = data;
	CampaignGetCurrentMission(co)->u.Classic.Pillars.Max =
		CLAMP(CampaignGetCurrentMission(co)->u.Classic.Pillars.Max + d, 1, 50);
	CampaignGetCurrentMission(co)->u.Classic.Pillars.Min =
		MIN(CampaignGetCurrentMission(co)->u.Classic.Pillars.Min,
			CampaignGetCurrentMission(co)->u.Classic.Pillars.Max);
	return EDITOR_RESULT_CHANGED;
}
static EditorResult MissionChangeDensity(void *data, int d)
{
	Campaign *co = data;
	CampaignGetCurrentMission(co)->EnemyDensity =
		CLAMP(CampaignGetCurrentMission(co)->EnemyDensity + d, 0, 100);
	return EDITOR_RESULT_CHANGED;
}

UIObject *CreateClassicMapObjs(struct vec2i pos, Campaign *co)
{
	const int th = FontH();
	UIObject *c = UIObjectCreate(UITYPE_NONE, 0, svec2i_zero(), svec2i_zero());
	UIObject *o =
		UIObjectCreate(UITYPE_LABEL, 0, svec2i_zero(), svec2i(50, th));
	const int x = pos.x;
	// Check whether the map type matches, and set visibility
	c->CheckVisible = MissionCheckTypeFunc;
	c->Data = co;

	UIObject *o2 = CreateCampaignSeedObj(pos, co);
	UIObjectAddChild(c, o2);

	pos.x += o2->Size.x;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = MissionGetWallCountStr;
	o2->Data = co;
	o2->ChangeFunc = MissionChangeWallCount;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.x += o2->Size.x;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = MissionGetWallLengthStr;
	o2->Data = co;
	o2->ChangeFunc = MissionChangeWallLength;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.x += o2->Size.x;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = MissionGetCorridorWidthStr;
	o2->Data = co;
	o2->ChangeFunc = MissionChangeCorridorWidth;
	o2->Pos = pos;
	o2->Size.x = 60;
	UIObjectAddChild(c, o2);

	pos.x = x;
	pos.y += th;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = MissionGetRoomCountStr;
	o2->Data = co;
	o2->ChangeFunc = MissionChangeRoomCount;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.x += o2->Size.x;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = MissionGetRoomMinStr;
	o2->Data = co;
	o2->ChangeFunc = MissionChangeRoomMin;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.x += o2->Size.x;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = MissionGetRoomMaxStr;
	o2->Data = co;
	o2->ChangeFunc = MissionChangeRoomMax;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.x += o2->Size.x;
	o2 = UIObjectCreate(UITYPE_CUSTOM, 0, pos, svec2i(60, th));
	o2->u.CustomDrawFunc = MissionDrawEdgeRooms;
	o2->Data = co;
	o2->ChangeFunc = MissionChangeEdgeRooms;
	UIObjectAddChild(c, o2);
	pos.x += o2->Size.x;
	o2 = UIObjectCreate(UITYPE_CUSTOM, 0, pos, svec2i(60, th));
	o2->u.CustomDrawFunc = MissionDrawRoomsOverlap;
	o2->Data = co;
	o2->ChangeFunc = MissionChangeRoomsOverlap;
	UIObjectAddChild(c, o2);

	pos.x = x;
	pos.y += th;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = MissionGetRoomWallCountStr;
	o2->Data = co;
	o2->ChangeFunc = MissionChangeRoomWallCount;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.x += o2->Size.x;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = MissionGetRoomWallLenStr;
	o2->Data = co;
	o2->ChangeFunc = MissionChangeRoomWallLen;
	o2->Pos = pos;
	o2->Size.x = 60;
	UIObjectAddChild(c, o2);
	pos.x += o2->Size.x;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = MissionGetRoomWallPadStr;
	o2->Data = co;
	o2->ChangeFunc = MissionChangeRoomWallPad;
	o2->Pos = pos;
	o2->Size.x = 60;
	UIObjectAddChild(c, o2);

	pos.x = x;
	pos.y += th;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = MissionGetSquareCountStr;
	o2->Data = co;
	o2->ChangeFunc = MissionChangeSquareCount;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.x += o2->Size.x;
	o2 = UIObjectCreate(UITYPE_CUSTOM, 0, pos, o->Size);
	o2->u.CustomDrawFunc = MissionDrawExitEnabled;
	o2->Data = co;
	o2->ChangeFunc = MissionChangeExitEnabled;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);

	pos.x = x;
	pos.y += th;
	o2 = UIObjectCreate(UITYPE_CUSTOM, 0, pos, o->Size);
	o2->u.CustomDrawFunc = MissionDrawDoorEnabled;
	o2->Data = co;
	o2->ChangeFunc = MissionChangeDoorEnabled;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.x += o2->Size.x;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = MissionGetDoorMinStr;
	o2->Data = co;
	o2->ChangeFunc = MissionChangeDoorMin;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.x += o2->Size.x;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = MissionGetDoorMaxStr;
	o2->Data = co;
	o2->ChangeFunc = MissionChangeDoorMax;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);

	pos.x = x;
	pos.y += th;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = MissionGetPillarCountStr;
	o2->Data = co;
	o2->ChangeFunc = MissionChangePillarCount;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.x += o2->Size.x;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = MissionGetPillarMinStr;
	o2->Data = co;
	o2->ChangeFunc = MissionChangePillarMin;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.x += o2->Size.x;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = MissionGetPillarMaxStr;
	o2->Data = co;
	o2->ChangeFunc = MissionChangePillarMax;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);

	UIObjectDestroy(o);
	return c;
}
