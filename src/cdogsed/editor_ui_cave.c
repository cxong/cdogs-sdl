/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (c) 2016-2017 Cong Xu
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
#include "editor_ui_cave.h"

#include <cdogs/events.h>
#include <cdogs/font.h>

#include "editor_ui_common.h"


MISSION_CHECK_TYPE_FUNC(MAPTYPE_CAVE)


static const char *MissionGetFillPercentStr(UIObject *o, void *data);
static void MissionChangeFillPercent(void *data, int d);
static const char *MissionGetRepeatStr(UIObject *o, void *data);
static void MissionChangeRepeat(void *data, int d);
static const char *MissionGetR1Str(UIObject *o, void *data);
static void MissionChangeR1(void *data, int d);
static const char *MissionGetR2Str(UIObject *o, void *data);
static void MissionChangeR2(void *data, int d);
static const char *MissionGetCorridorWidthStr(UIObject *o, void *data);
static void MissionChangeCorridorWidth(void *data, int d);
static const char *MissionGetSquareCountStr(UIObject *o, void *data);
static void MissionChangeSquareCount(void *data, int d);
UIObject *CreateCaveMapObjs(Vec2i pos, CampaignOptions *co)
{
	const int th = FontH();
	UIObject *c = UIObjectCreate(UITYPE_NONE, 0, Vec2iZero(), Vec2iZero());
	UIObject *o = UIObjectCreate(
		UITYPE_LABEL, 0, Vec2iZero(), Vec2iNew(50, th));
	const int x = pos.x;
	o->ChangesData = true;
	// Check whether the map type matches, and set visibility
	c->CheckVisible = MissionCheckTypeFunc;
	c->Data = co;


	UIObject *o2 = CreateCampaignSeedObj(pos, co);
	UIObjectAddChild(c, o2);

	pos.x += o2->Size.x;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = MissionGetFillPercentStr;
	o2->Data = co;
	o2->ChangeFunc = MissionChangeFillPercent;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.x += o2->Size.x;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = MissionGetRepeatStr;
	o2->Data = co;
	o2->ChangeFunc = MissionChangeRepeat;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);

	pos.x = x;
	pos.y += th;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = MissionGetR1Str;
	o2->Data = co;
	o2->ChangeFunc = MissionChangeR1;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.x += o2->Size.x;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = MissionGetR2Str;
	o2->Data = co;
	o2->ChangeFunc = MissionChangeR2;
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
	o2->u.LabelFunc = MissionGetSquareCountStr;
	o2->Data = co;
	o2->ChangeFunc = MissionChangeSquareCount;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);

	UIObjectDestroy(o);
	return c;
}
static const char *MissionGetFillPercentStr(UIObject *o, void *data)
{
	UNUSED(o);
	CampaignOptions *co = data;
	const Mission *m = CampaignGetCurrentMission(co);
	if (m == NULL) return NULL;
	static char s[128];
	sprintf(s, "Fill: %d%%", m->u.Cave.FillPercent);
	return s;
}
static void MissionChangeFillPercent(void *data, int d)
{
	CampaignOptions *co = data;
	Mission *m = CampaignGetCurrentMission(co);
	if (gEventHandlers.keyboard.modState & KMOD_SHIFT)
	{
		d *= 10;
	}
	m->u.Cave.FillPercent = CLAMP(m->u.Cave.FillPercent + d, 0, 100);
}
static const char *MissionGetRepeatStr(UIObject *o, void *data)
{
	UNUSED(o);
	CampaignOptions *co = data;
	const Mission *m = CampaignGetCurrentMission(co);
	if (m == NULL) return NULL;
	static char s[128];
	sprintf(s, "Repeat: %d", m->u.Cave.Repeat);
	return s;
}
static void MissionChangeRepeat(void *data, int d)
{
	CampaignOptions *co = data;
	Mission *m = CampaignGetCurrentMission(co);
	m->u.Cave.Repeat = CLAMP(m->u.Cave.Repeat + d, 0, 10);
}
static const char *MissionGetR1Str(UIObject *o, void *data)
{
	UNUSED(o);
	CampaignOptions *co = data;
	const Mission *m = CampaignGetCurrentMission(co);
	if (m == NULL) return NULL;
	static char s[128];
	sprintf(s, "R1: %d", m->u.Cave.R1);
	return s;
}
static void MissionChangeR1(void *data, int d)
{
	CampaignOptions *co = data;
	Mission *m = CampaignGetCurrentMission(co);
	m->u.Cave.R1 = CLAMP(m->u.Cave.R1 + d, 0, 8);
}
static const char *MissionGetR2Str(UIObject *o, void *data)
{
	UNUSED(o);
	CampaignOptions *co = data;
	const Mission *m = CampaignGetCurrentMission(co);
	if (m == NULL) return NULL;
	static char s[128];
	sprintf(s, "R2: %d", m->u.Cave.R2);
	return s;
}
static void MissionChangeR2(void *data, int d)
{
	CampaignOptions *co = data;
	Mission *m = CampaignGetCurrentMission(co);
	m->u.Cave.R2 = CLAMP(m->u.Cave.R2 + d, -1, 25);
}
static const char *MissionGetCorridorWidthStr(UIObject *o, void *data)
{
	UNUSED(o);
	CampaignOptions *co = data;
	const Mission *m = CampaignGetCurrentMission(co);
	if (m == NULL) return NULL;
	static char s[128];
	sprintf(s, "CorridorWidth: %d", m->u.Cave.CorridorWidth);
	return s;
}
static void MissionChangeCorridorWidth(void *data, int d)
{
	CampaignOptions *co = data;
	Mission *m = CampaignGetCurrentMission(co);
	m->u.Cave.CorridorWidth = CLAMP(m->u.Cave.CorridorWidth + d, 1, 5);
}
static const char *MissionGetSquareCountStr(UIObject *o, void *data)
{
	static char s[128];
	UNUSED(o);
	CampaignOptions *co = data;
	if (!CampaignGetCurrentMission(co)) return NULL;
	sprintf(s, "Sqr: %d", CampaignGetCurrentMission(co)->u.Cave.Squares);
	return s;
}
static void MissionChangeSquareCount(void *data, int d)
{
	CampaignOptions *co = data;
	CampaignGetCurrentMission(co)->u.Cave.Squares =
		CLAMP(CampaignGetCurrentMission(co)->u.Cave.Squares + d, 0, 100);
}
