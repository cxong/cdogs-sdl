/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (c) 2013-2016, Cong Xu
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

#include <cdogs/drawtools.h>
#include <cdogs/font.h>

#include "editor_ui.h"


typedef struct
{
	color_t Color;
	Vec2i SwatchSize;
	Vec2i SwatchPad;
	void *Data;
	ColorPickerChangeFunc ChangeFunc;
} ColorPickerData;
static void ColorPickerChange(void *data, int d);
static void ColorPickerDrawSwatch(
	UIObject *o, GraphicsDevice *g, Vec2i pos, void *data);
UIObject *CreateColorPicker(
	const Vec2i pos,
	const uint8_t increment, const int levels, const int stride,
	const Vec2i swatchSize, const Vec2i swatchPad,
	void *data, ColorPickerChangeFunc changeFunc)
{
	CASSERT(increment * levels <= 255, "too many levels for colour picker");
	UIObject *c = UIObjectCreate(UITYPE_CONTEXT_MENU, 0, pos, Vec2iZero());
	c->IsDynamicData = true;
	c->Data = data;

	// Create 4x4 colour squares
	UIObject *o = UIObjectCreate(
		UITYPE_CUSTOM, 0, Vec2iZero(), Vec2iAdd(swatchSize, swatchPad));
	o->ChangeFunc = ColorPickerChange;
	o->u.CustomDrawFunc = ColorPickerDrawSwatch;
	Vec2i v = Vec2iZero();
	// Create palette
	const uint8_t minValue = (uint8_t)increment;
	const uint8_t maxValue = (uint8_t)(increment * levels);
	color_t colour = colorBlack;
	colour.r = colour.g = colour.b = minValue;
	for (int i = 0; ; i++)
	{
		UIObject *o2 = UIObjectCopy(o);
		o2->IsDynamicData = true;
		CMALLOC(o2->Data, sizeof(ColorPickerData));
		((ColorPickerData *)o2->Data)->Color = colour;
		((ColorPickerData *)o2->Data)->SwatchSize = swatchSize;
		((ColorPickerData *)o2->Data)->SwatchPad = swatchPad;
		((ColorPickerData *)o2->Data)->Data = data;
		((ColorPickerData *)o2->Data)->ChangeFunc = changeFunc;
		o2->Pos = v;
		UIObjectAddChild(c, o2);
		v.x += o->Size.x;
		if (((i + 1) % stride) == 0)
		{
			v.x = 0;
			v.y += o->Size.y;
		}
		// Get next colour: increment B first, then R, then G
		if (colour.b >= maxValue)
		{
			colour.b = minValue;
			if (colour.r >= maxValue)
			{
				colour.r = minValue;
				if (colour.g >= maxValue)
				{
					break;
				}
				else
				{
					colour.g += (uint8_t)increment;
				}
			}
			else
			{
				colour.r += (uint8_t)increment;
			}
		}
		else
		{
			colour.b += (uint8_t)increment;
		}
	}

	UIObjectDestroy(o);
	return c;
}
static void ColorPickerChange(void *data, int d)
{
	UNUSED(d);
	ColorPickerData *mc = data;
	mc->ChangeFunc(mc->Color, mc->Data);
}
static void ColorPickerDrawSwatch(
	UIObject *o, GraphicsDevice *g, Vec2i pos, void *data)
{
	UNUSED(o);
	const ColorPickerData *cpd = data;
	DrawRectangle(
		g,
		Vec2iAdd(Vec2iAdd(pos, o->Pos), Vec2iScaleDiv(cpd->SwatchPad, 2)),
		cpd->SwatchSize,
		cpd->Color,
		0);
}


typedef enum
{
	MISSION_COLOR_WALL,
	MISSION_COLOR_FLOOR,
	MISSION_COLOR_ROOM,
	MISSION_COLOR_EXTRA,
	MISSION_COLOR_COUNT
} MissionColorType;
typedef struct
{
	CampaignOptions *C;
	MissionColorType Type;
} MissionColorData;
static const char *MissionGetColorStr(UIObject *o, void *data);
static void MissionColorChange(const color_t c, void *data);
Vec2i CreateColorObjs(CampaignOptions *co, UIObject *c, Vec2i pos)
{
	const int th = FontH();

	UIObject *o = UIObjectCreate(
		UITYPE_LABEL, YC_MISSIONLOOKS, Vec2iZero(), Vec2iNew(100, th));
	o->ChangesData = true;
	o->u.LabelFunc = MissionGetColorStr;

	for (int i = 0; i < (int)MISSION_COLOR_COUNT; i++)
	{
		UIObject *o2 = UIObjectCopy(o);
		o2->IsDynamicData = true;
		CMALLOC(o2->Data, sizeof(MissionColorData));
		((MissionColorData *)o2->Data)->C = co;
		((MissionColorData *)o2->Data)->Type = (MissionColorType)i;
		o2->Pos = pos;

		MissionColorData *mcd;
		CMALLOC(mcd, sizeof *mcd);
		mcd->C = co;
		mcd->Type = (MissionColorType)i;
		UIObjectAddChild(o2, CreateColorPicker(
			Vec2iZero(), 32, 4, 16, Vec2iNew(6, 6), Vec2iNew(2, 2),
			mcd, MissionColorChange));
		UIObjectAddChild(c, o2);
		pos.y += th;
	}

	UIObjectDestroy(o);
	return pos;
}
static const char *MissionGetColorStr(UIObject *o, void *data)
{
	static char s[128];
	UNUSED(o);
	const MissionColorData *mc = data;
	const Mission *m = CampaignGetCurrentMission(mc->C);
	if (m == NULL) return NULL;
	static const char *colourTypeNames[] =
	{
		"Walls", "Floors", "Rooms", "Extra"
	};
	char c[8];
	switch (mc->Type)
	{
	case MISSION_COLOR_WALL: ColorStr(c, m->WallMask); break;
	case MISSION_COLOR_FLOOR: ColorStr(c, m->FloorMask); break;
	case MISSION_COLOR_ROOM: ColorStr(c, m->RoomMask); break;
	case MISSION_COLOR_EXTRA: ColorStr(c, m->AltMask); break;
	default:
		CASSERT(false, "Unexpected mission colour");
		break;
	}
	sprintf(s, "%s: #%s", colourTypeNames[(int)mc->Type], c);
	return s;
}
static void MissionColorChange(const color_t c, void *data)
{
	MissionColorData *mcd = data;
	Mission *m = CampaignGetCurrentMission(mcd->C);
	switch (mcd->Type)
	{
	case MISSION_COLOR_WALL: m->WallMask = c; break;
	case MISSION_COLOR_FLOOR: m->FloorMask = c; break;
	case MISSION_COLOR_ROOM: m->RoomMask = c; break;
	case MISSION_COLOR_EXTRA: m->AltMask = c; break;
	default:
		CASSERT(false, "Unexpected mission colour");
		break;
	}
}
