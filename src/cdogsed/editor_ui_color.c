/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
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
#include "editor_ui_color.h"

#include <cdogs/files.h>
#include <cdogs/font.h>

#include "editor_ui.h"


static const char *MissionGetWallColorStr(UIObject *o, void *data)
{
	static char s[128];
	UNUSED(o);
	CampaignOptions *co = data;
	if (!CampaignGetCurrentMission(co)) return NULL;
	sprintf(s, "Walls: %s",
		ColorRangeName(ColorToRange(CampaignGetCurrentMission(co)->WallMask)));
	return s;
}
static const char *MissionGetFloorColorStr(UIObject *o, void *data)
{
	static char s[128];
	UNUSED(o);
	CampaignOptions *co = data;
	if (!CampaignGetCurrentMission(co)) return NULL;
	sprintf(s, "Floor: %s",
		ColorRangeName(ColorToRange(CampaignGetCurrentMission(co)->FloorMask)));
	return s;
}
static const char *MissionGeRoomColorStr(UIObject *o, void *data)
{
	static char s[128];
	UNUSED(o);
	CampaignOptions *co = data;
	if (!CampaignGetCurrentMission(co)) return NULL;
	sprintf(s, "Rooms: %s",
		ColorRangeName(ColorToRange(CampaignGetCurrentMission(co)->RoomMask)));
	return s;
}
static const char *MissionGeExtraColorStr(UIObject *o, void *data)
{
	static char s[128];
	UNUSED(o);
	CampaignOptions *co = data;
	if (!CampaignGetCurrentMission(co)) return NULL;
	sprintf(s, "Extra: %s",
		ColorRangeName(ColorToRange(CampaignGetCurrentMission(co)->AltMask)));
	return s;
}
static void MissionChangeWallColor(void *data, int d)
{
	CampaignOptions *co = data;
	CampaignGetCurrentMission(co)->WallMask =
		RangeToColor(CLAMP_OPPOSITE(
			ColorToRange(CampaignGetCurrentMission(co)->WallMask) + d,
			0, COLORRANGE_COUNT - 1));
}
static void MissionChangeFloorColor(void *data, int d)
{
	CampaignOptions *co = data;
	CampaignGetCurrentMission(co)->FloorMask =
		RangeToColor(CLAMP_OPPOSITE(
			ColorToRange(CampaignGetCurrentMission(co)->FloorMask) + d,
			0, COLORRANGE_COUNT - 1));
}
static void MissionChangeRoomColor(void *data, int d)
{
	CampaignOptions *co = data;
	CampaignGetCurrentMission(co)->RoomMask =
		RangeToColor(CLAMP_OPPOSITE(
			ColorToRange(CampaignGetCurrentMission(co)->RoomMask) + d,
			0, COLORRANGE_COUNT - 1));
}
static void MissionChangeExtraColor(void *data, int d)
{
	CampaignOptions *co = data;
	CampaignGetCurrentMission(co)->AltMask =
		RangeToColor(CLAMP_OPPOSITE(
			ColorToRange(CampaignGetCurrentMission(co)->AltMask) + d,
			0, COLORRANGE_COUNT - 1));
}

Vec2i CreateColorObjs(CampaignOptions *co, UIObject *c, Vec2i pos)
{
	const int th = FontH();

	UIObject *o = UIObjectCreate(
		UITYPE_LABEL, YC_MISSIONLOOKS, Vec2iZero(), Vec2iNew(100, th));
	o->ChangesData = true;

	UIObject *o2 = UIObjectCopy(o);
	o2->Id2 = XC_COLOR1;
	o2->u.LabelFunc = MissionGetWallColorStr;
	o2->Data = co;
	o2->ChangeFunc = MissionChangeWallColor;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.y += th;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_COLOR2;
	o2->u.LabelFunc = MissionGetFloorColorStr;
	o2->Data = co;
	o2->ChangeFunc = MissionChangeFloorColor;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.y += th;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_COLOR3;
	o2->u.LabelFunc = MissionGeRoomColorStr;
	o2->Data = co;
	o2->ChangeFunc = MissionChangeRoomColor;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.y += th;
	o2 = UIObjectCopy(o);
	o2->Id2 = XC_COLOR4;
	o2->u.LabelFunc = MissionGeExtraColorStr;
	o2->Data = co;
	o2->ChangeFunc = MissionChangeExtraColor;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);

	UIObjectDestroy(o);

	return pos;
}
