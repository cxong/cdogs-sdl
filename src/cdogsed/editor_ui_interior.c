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

MISSION_CHECK_TYPE_FUNC(MAPTYPE_INTERIOR)
static const char *MissionGetCorridorWidthStr(UIObject *o, void *data)
{
	static char s[128];
	UNUSED(o);
	Campaign *co = data;
	if (!CampaignGetCurrentMission(co))
		return NULL;
	sprintf(
		s, "CorridorWidth: %d",
		CampaignGetCurrentMission(co)->u.Interior.CorridorWidth);
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
		s, "RoomMin: %d", CampaignGetCurrentMission(co)->u.Interior.Rooms.Min);
	return s;
}
static const char *MissionGetRoomMaxStr(UIObject *o, void *data)
{
	static char s[128];
	UNUSED(o);
	Campaign *co = data;
	if (!CampaignGetCurrentMission(co))
		return NULL;
	sprintf(s, "RoomMax: %d", CampaignGetCurrentMission(co)->u.Interior.Rooms.Max);
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
		CampaignGetCurrentMission(co)->u.Interior.ExitEnabled,
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
		s, "RoomWalls: %d", CampaignGetCurrentMission(co)->u.Interior.Rooms.Walls);
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
		CampaignGetCurrentMission(co)->u.Interior.Rooms.WallLength);
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
		CampaignGetCurrentMission(co)->u.Interior.Rooms.WallPad);
	return s;
}
static void MissionDrawDoorEnabled(
	UIObject *o, GraphicsDevice *g, struct vec2i pos, void *data)
{
	UNUSED(o);
	UNUSED(g);
	Campaign *co = data;
	Mission *m = CampaignGetCurrentMission(co);
	if (!m)
		return;
	DisplayFlag(
		svec2i_add(pos, o->Pos), "Doors",
		m->u.Interior.Doors.Enabled,
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
		s, "DoorMin: %d", CampaignGetCurrentMission(co)->u.Interior.Doors.Min);
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
		s, "DoorMax: %d", CampaignGetCurrentMission(co)->u.Interior.Doors.Max);
	return s;
}
static void MissionDrawDoorRandomPos(
	UIObject *o, GraphicsDevice *g, struct vec2i pos, void *data)
{
	UNUSED(o);
	UNUSED(g);
	Campaign *co = data;
	Mission *m = CampaignGetCurrentMission(co);
	if (!m)
		return;
	DisplayFlag(
		svec2i_add(pos, o->Pos), "DoorRandomPos",
		m->u.Interior.Doors.RandomPos,
		UIObjectIsHighlighted(o));
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
		CampaignGetCurrentMission(co)->u.Interior.Pillars.Count);
	return s;
}

static EditorResult MissionChangeCorridorWidth(void *data, int d)
{
	Campaign *co = data;
	CampaignGetCurrentMission(co)->u.Interior.CorridorWidth = CLAMP(
		CampaignGetCurrentMission(co)->u.Interior.CorridorWidth + d, 1, 5);
	return EDITOR_RESULT_CHANGED;
}
static EditorResult MissionChangeRoomMin(void *data, int d)
{
	Campaign *co = data;
	Mission *m = CampaignGetCurrentMission(co);
	m->u.Interior.Rooms.Min = CLAMP(m->u.Interior.Rooms.Min + d, 5, 25);
	m->u.Interior.Rooms.Max =
		MAX(m->u.Interior.Rooms.Min * 2, m->u.Interior.Rooms.Max);
	return EDITOR_RESULT_CHANGED;
}
static EditorResult MissionChangeRoomMax(void *data, int d)
{
	Campaign *co = data;
	Mission *m = CampaignGetCurrentMission(co);
	m->u.Interior.Rooms.Max = CLAMP(m->u.Interior.Rooms.Max + d, 10, 50);
	m->u.Interior.Rooms.Min =
		MIN(m->u.Interior.Rooms.Min, m->u.Interior.Rooms.Max / 2);
	return EDITOR_RESULT_CHANGED;
}
static EditorResult MissionChangeExitEnabled(void *data, int d)
{
	UNUSED(d);
	Campaign *co = data;
	Mission *m = CampaignGetCurrentMission(co);
	m->u.Interior.ExitEnabled = !m->u.Interior.ExitEnabled;
	return EDITOR_RESULT_CHANGED;
}
static EditorResult MissionChangeRoomWallCount(void *data, int d)
{
	Campaign *co = data;
	Mission *m = CampaignGetCurrentMission(co);
	m->u.Interior.Rooms.Walls = CLAMP(m->u.Interior.Rooms.Walls + d, 0, 50);
	return EDITOR_RESULT_CHANGED;
}
static EditorResult MissionChangeRoomWallLen(void *data, int d)
{
	Campaign *co = data;
	Mission *m = CampaignGetCurrentMission(co);
	m->u.Interior.Rooms.WallLength = CLAMP(m->u.Interior.Rooms.WallLength + d, 1, 50);
	return EDITOR_RESULT_CHANGED;
}
static EditorResult MissionChangeRoomWallPad(void *data, int d)
{
	Campaign *co = data;
	Mission *m = CampaignGetCurrentMission(co);
	m->u.Interior.Rooms.WallPad = CLAMP(m->u.Interior.Rooms.WallPad + d, 1, 10);
	return EDITOR_RESULT_CHANGED;
}
static EditorResult MissionChangeDoorEnabled(void *data, int d)
{
	UNUSED(d);
	Campaign *co = data;
	Mission *m = CampaignGetCurrentMission(co);
	m->u.Interior.Doors.Enabled = !m->u.Interior.Doors.Enabled;
	return EDITOR_RESULT_CHANGED;
}
static EditorResult MissionChangeDoorMin(void *data, int d)
{
	Campaign *co = data;
	Mission *m = CampaignGetCurrentMission(co);
	m->u.Interior.Doors.Min = CLAMP(m->u.Interior.Doors.Min + d, 1, 6);
	m->u.Interior.Doors.Max =
		MAX(m->u.Interior.Doors.Min, m->u.Interior.Doors.Max);
	return EDITOR_RESULT_CHANGED;
}
static EditorResult MissionChangeDoorMax(void *data, int d)
{
	Campaign *co = data;
	Mission *m = CampaignGetCurrentMission(co);
	m->u.Interior.Doors.Max = CLAMP(m->u.Interior.Doors.Max + d, 1, 6);
	m->u.Interior.Doors.Min =
		MIN(m->u.Interior.Doors.Min, m->u.Interior.Doors.Max);
	return EDITOR_RESULT_CHANGED;
}
static EditorResult MissionChangeDoorRandomPos(void *data, int d)
{
	UNUSED(d);
	Campaign *co = data;
	Mission *m = CampaignGetCurrentMission(co);
	m->u.Interior.Doors.RandomPos = !m->u.Interior.Doors.RandomPos;
	return EDITOR_RESULT_CHANGED;
}
static EditorResult MissionChangePillarCount(void *data, int d)
{
	Campaign *co = data;
	Mission *m = CampaignGetCurrentMission(co);
	m->u.Interior.Pillars.Count = CLAMP(m->u.Interior.Pillars.Count + d, 0, 50);
	return EDITOR_RESULT_CHANGED;
}

UIObject *CreateInteriorMapObjs(struct vec2i pos, Campaign *co)
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
	o2->u.LabelFunc = MissionGetCorridorWidthStr;
	o2->Data = co;
	o2->ChangeFunc = MissionChangeCorridorWidth;
	o2->Pos = pos;
	o2->Size.x = 60;
	UIObjectAddChild(c, o2);

	pos.x = x;
	pos.y += th;
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

	pos.x = x;
	pos.y += th;
	o2 = UIObjectCreate(UITYPE_CUSTOM, 0, pos, o->Size);
	o2->u.CustomDrawFunc = MissionDrawExitEnabled;
	o2->Data = co;
	o2->ChangeFunc = MissionChangeExitEnabled;
	o2->Pos = pos;
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
	pos.x += o2->Size.x;
	o2 = UIObjectCreate(UITYPE_CUSTOM, 0, pos, o->Size);
	o2->u.CustomDrawFunc = MissionDrawDoorRandomPos;
	o2->Data = co;
	o2->ChangeFunc = MissionChangeDoorRandomPos;
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

	UIObjectDestroy(o);
	return c;
}
