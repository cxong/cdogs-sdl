/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (c) 2014, Cong Xu
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
#include "editor_ui_common.h"

#include <cdogs/events.h>
#include <cdogs/font.h>
#include <cdogs/gamedata.h>
#include <cdogs/palette.h>


void DisplayMapItem(const Vec2i pos, const MapObject *mo)
{
	Vec2i offset;
	const Pic *pic = MapObjectGetPic(mo, &offset, false);
	Blit(&gGraphicsDevice, pic, Vec2iAdd(pos, offset));
}

void DrawKey(UIObject *o, GraphicsDevice *g, Vec2i pos, void *vData)
{
	EditorBrushAndCampaign *data = vData;
	if (data->Brush.ItemIndex == -1)
	{
		// No key; don't draw
		return;
	}
	const Pic *pic =
		KeyPickupClass(gMission.keyStyle, data->Brush.ItemIndex)->Pic;
	pos = Vec2iAdd(Vec2iAdd(pos, o->Pos), Vec2iScaleDiv(o->Size, 2));
	pos = Vec2iMinus(pos, Vec2iScaleDiv(pic->size, 2));
	Blit(g, pic, pos);
}

void InsertMission(CampaignOptions *co, Mission *mission, int idx)
{
	Mission defaultMission;
	if (mission == NULL)
	{
		MissionInit(&defaultMission);
		defaultMission.Size = Vec2iNew(48, 48);
		// Set some default values for the mission
		defaultMission.u.Classic.CorridorWidth = 1;
		defaultMission.u.Classic.Rooms.Min =
			defaultMission.u.Classic.Rooms.Max = 5;
		defaultMission.u.Classic.Rooms.WallLength = 1;
		defaultMission.u.Classic.Rooms.WallPad = 1;
		defaultMission.u.Classic.Doors.Min =
			defaultMission.u.Classic.Doors.Max = 1;
		defaultMission.u.Classic.Pillars.Min =
			defaultMission.u.Classic.Pillars.Max = 1;
		mission = &defaultMission;
	}
	CArrayInsert(&co->Setting.Missions, idx, mission);
}
void DeleteMission(CampaignOptions *co)
{
	CASSERT(
		co->MissionIndex < (int)co->Setting.Missions.size,
		"invalid mission index");
	MissionTerminate(CampaignGetCurrentMission(co));
	CArrayDelete(&co->Setting.Missions, co->MissionIndex);
	if (co->MissionIndex >= (int)co->Setting.Missions.size)
	{
		co->MissionIndex = MAX(0, (int)co->Setting.Missions.size - 1);
	}
}

bool ConfirmScreen(const char *info, const char *msg)
{
	int w = gGraphicsDevice.cachedConfig.Res.x;
	int h = gGraphicsDevice.cachedConfig.Res.y;
	ClearScreen(&gGraphicsDevice);
	FontStr(info, Vec2iNew((w - FontStrW(info)) / 2, (h - FontH()) / 2));
	FontStr(msg, Vec2iNew((w - FontStrW(msg)) / 2, (h + FontH()) / 2));
	BlitFlip(&gGraphicsDevice);

	int c = GetKey(&gEventHandlers);
	return (c == 'Y' || c == 'y');
}

void ClearScreen(GraphicsDevice *g)
{
	color_t color = { 32, 32, 60, 255 };
	const Uint32 pixel = COLOR2PIXEL(color);
	for (int i = 0; i < GraphicsGetScreenSize(&g->cachedConfig); i++)
	{
		g->buf[i] = pixel;
	}
}
