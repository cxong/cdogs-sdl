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
#include "editor_ui_static.h"

#include <assert.h>

#include <SDL_image.h>

#include <cdogs/draw/draw.h>
#include <cdogs/events.h>
#include <cdogs/font.h>
#include <cdogs/map.h>

#include "editor_ui_common.h"
#include "editor_ui_static_additem.h"


MISSION_CHECK_TYPE_FUNC(MAPTYPE_STATIC)


static const char *BrushGetTypeStr(EditorBrush *brush, int isMain)
{
	static char s[128];
	sprintf(s, "Brush %d: %s",
		isMain ? 1 : 2,
		IMapTypeStr(isMain ? brush->MainType : brush->SecondaryType));
	return s;
}
static const char *BrushGetMainTypeStr(UIObject *o, void *data)
{
	UNUSED(o);
	EditorBrush *brush = data;
	return BrushGetTypeStr(brush, 1);
}
static const char *BrushGetSecondaryTypeStr(UIObject *o, void *data)
{
	UNUSED(o);
	EditorBrush *brush = data;
	return BrushGetTypeStr(brush, 0);
}
static const char *BrushGetSizeStr(UIObject *o, void *data)
{
	static char s[128];
	UNUSED(o);
	EditorBrush *brush = data;
	sprintf(s, "Brush Size: %d", brush->BrushSize);
	return s;
}
static char *BrushGetGuideImageStr(UIObject *o, void *data)
{
	UNUSED(o);
	EditorBrush *brush = data;
	return brush->GuideImage;
}
static const char *BrushGetGuideImageAlphaStr(UIObject *o, void *data)
{
	static char s[128];
	UNUSED(o);
	EditorBrush *brush = data;
	sprintf(s, "Guide Alpha: %d%%", (int)brush->GuideImageAlpha * 100 / 255);
	return s;
}


static void BrushChangeType(EditorBrush *b, int d, int isMain)
{
	unsigned short brushType = isMain ? b->MainType : b->SecondaryType;
	brushType = (unsigned short)CLAMP_OPPOSITE(
		(int)brushType + d, MAP_FLOOR, MAP_NOTHING);
	if (isMain)
	{
		b->MainType = brushType;
	}
	else
	{
		b->SecondaryType = brushType;
	}
}
static void BrushChangeMainType(void *data, int d)
{
	BrushChangeType(data, d, 1);
}
static void BrushChangeSecondaryType(void *data, int d)
{
	BrushChangeType(data, d, 0);
}
static void BrushChangeSize(void *data, int d)
{
	EditorBrush *b = data;
	b->BrushSize = CLAMP(b->BrushSize + d, 1, 5);
}
static void BrushLoadGuideImage(void *data, int d)
{
	UNUSED(d);
	EditorBrush *b = data;
	b->IsGuideImageNew = true;
	SDL_FreeSurface(b->GuideImageSurface);
	SDL_Surface *s = IMG_Load(b->GuideImage);
	if (s == NULL) return;
	b->GuideImageSurface = SDL_ConvertSurface(s, gGraphicsDevice.Format, 0);
	SDL_FreeSurface(s);
}
static void BrushChangeGuideImageAlpha(void *data, int d)
{
	EditorBrush *b = data;
	b->IsGuideImageNew = true;
	d *= 4;
	if (gEventHandlers.keyboard.modState & KMOD_SHIFT)
	{
		d *= 8;
	}
	b->GuideImageAlpha = (Uint8)CLAMP((int)b->GuideImageAlpha + d, 0, 255);
}
static int BrushIsBrushTypePoint(void *data)
{
	EditorBrush *b = data;
	return b->Type == BRUSHTYPE_POINT;
}
static int BrushIsBrushTypeLine(void *data)
{
	EditorBrush *b = data;
	return b->Type == BRUSHTYPE_LINE;
}
static int BrushIsBrushTypeBox(void *data)
{
	EditorBrush *b = data;
	return b->Type == BRUSHTYPE_BOX;
}
static int BrushIsBrushTypeBoxFilled(void *data)
{
	EditorBrush *b = data;
	return b->Type == BRUSHTYPE_BOX_FILLED;
}
static int BrushIsBrushTypeRoom(void *data)
{
	EditorBrush *b = data;
	return b->Type == BRUSHTYPE_ROOM;
}
static int BrushIsBrushTypeRoomPainter(void *data)
{
	EditorBrush *b = data;
	return b->Type == BRUSHTYPE_ROOM_PAINTER;
}
static int BrushIsBrushTypeSelect(void *data)
{
	EditorBrush *b = data;
	return b->Type == BRUSHTYPE_SELECT;
}
static int BrushIsBrushTypeFill(void *data)
{
	EditorBrush *b = data;
	return b->Type == BRUSHTYPE_FILL;
}
static int BrushIsBrushTypeAddItem(void *data)
{
	EditorBrush *b = data;
	return
		b->Type == BRUSHTYPE_SET_PLAYER_START ||
		b->Type == BRUSHTYPE_ADD_ITEM ||
		b->Type == BRUSHTYPE_ADD_CHARACTER ||
		b->Type == BRUSHTYPE_ADD_KEY;
}
static int BrushIsBrushTypeSetKey(void *data)
{
	EditorBrush *b = data;
	return b->Type == BRUSHTYPE_SET_KEY;
}
static int BrushIsBrushTypeSetExit(void *data)
{
	EditorBrush *b = data;
	return b->Type == BRUSHTYPE_SET_EXIT;
}
static void BrushSetBrushTypePoint(void *data, int d)
{
	UNUSED(d);
	EditorBrush *b = data;
	b->Type = BRUSHTYPE_POINT;
}
static void BrushSetBrushTypeLine(void *data, int d)
{
	UNUSED(d);
	EditorBrush *b = data;
	b->Type = BRUSHTYPE_LINE;
}
static void BrushSetBrushTypeBox(void *data, int d)
{
	UNUSED(d);
	EditorBrush *b = data;
	b->Type = BRUSHTYPE_BOX;
}
static void BrushSetBrushTypeBoxFilled(void *data, int d)
{
	UNUSED(d);
	EditorBrush *b = data;
	b->Type = BRUSHTYPE_BOX_FILLED;
}
static void BrushSetBrushTypeRoom(void *data, int d)
{
	UNUSED(d);
	EditorBrush *b = data;
	b->Type = BRUSHTYPE_ROOM;
}
static void BrushSetBrushTypeRoomPainter(void *data, int d)
{
	UNUSED(d);
	EditorBrush *b = data;
	b->Type = BRUSHTYPE_ROOM_PAINTER;
}
static void BrushSetBrushTypeSelect(void *data, int d)
{
	UNUSED(d);
	EditorBrush *b = data;
	b->Type = BRUSHTYPE_SELECT;
}
static void BrushSetBrushTypeFill(void *data, int d)
{
	UNUSED(d);
	EditorBrush *b = data;
	b->Type = BRUSHTYPE_FILL;
}
static void BrushSetBrushTypeSetExit(void *data, int d)
{
	UNUSED(d);
	EditorBrush *b = data;
	b->Type = BRUSHTYPE_SET_EXIT;
}
static void BrushSetBrushTypeSetKey(void *data, int d)
{
	UNUSED(d);
	IndexedEditorBrush *b = data;
	b->Brush->Type = BRUSHTYPE_SET_KEY;
	b->Brush->u.ItemIndex = b->u.ItemIndex;
}
static void ActivateBrush(UIObject *o, void *data)
{
	UNUSED(o);
	EditorBrush *b = data;
	b->IsActive = 1;
}
static bool DeactivateBrush(void *data)
{
	EditorBrush *b = data;
	b->IsActive = 0;
	return false;
}
static void ActivateIndexedEditorBrush(UIObject *o, void *data)
{
	UNUSED(o);
	IndexedEditorBrush *b = data;
	b->Brush->IsActive = true;
}
static bool DeactivateIndexedEditorBrush(void *data)
{
	IndexedEditorBrush *b = data;
	b->Brush->IsActive = false;
	return false;
}


static UIObject *CreateSetKeyObjs(Vec2i pos, EditorBrush *brush);
UIObject *CreateStaticMapObjs(
	Vec2i pos, CampaignOptions *co, EditorBrush *brush)
{
	int x = pos.x;
	const int th = FontH();
	UIObject *c = UIObjectCreate(UITYPE_NONE, 0, Vec2iZero(), Vec2iZero());
	UIObject *o2;
	// Check whether the map type matches, and set visibility
	c->CheckVisible = MissionCheckTypeFunc;
	c->Data = co;

	UIObject *o = UIObjectCreate(UITYPE_BUTTON, 0, Vec2iZero(), Vec2iZero());
	o->Data = brush;
	o->OnFocusFunc = ActivateBrush;
	o->OnUnfocusFunc = DeactivateBrush;
	o->ChangesData = false;
	o2 = UIObjectCopy(o);
	UIButtonSetPic(o2, PicManagerGetPic(&gPicManager, "editor/pencil"));
	o2->u.Button.IsDownFunc = BrushIsBrushTypePoint;
	o2->ChangeFunc = BrushSetBrushTypePoint;
	CSTRDUP(o2->Tooltip, "Point");
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.x += o2->Size.x;
	o2 = UIObjectCopy(o);
	UIButtonSetPic(o2, PicManagerGetPic(&gPicManager, "editor/line"));
	o2->u.Button.IsDownFunc = BrushIsBrushTypeLine;
	o2->ChangeFunc = BrushSetBrushTypeLine;
	CSTRDUP(o2->Tooltip, "Line");
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.x += o2->Size.x;
	o2 = UIObjectCopy(o);
	UIButtonSetPic(o2, PicManagerGetPic(&gPicManager, "editor/box"));
	o2->u.Button.IsDownFunc = BrushIsBrushTypeBox;
	o2->ChangeFunc = BrushSetBrushTypeBox;
	CSTRDUP(o2->Tooltip, "Box");
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.x += o2->Size.x;
	o2 = UIObjectCopy(o);
	UIButtonSetPic(o2, PicManagerGetPic(&gPicManager, "editor/box_filled"));
	o2->u.Button.IsDownFunc = BrushIsBrushTypeBoxFilled;
	o2->ChangeFunc = BrushSetBrushTypeBoxFilled;
	CSTRDUP(o2->Tooltip, "Box filled");
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.x += o2->Size.x;
	o2 = UIObjectCopy(o);
	UIButtonSetPic(o2, PicManagerGetPic(&gPicManager, "editor/room"));
	o2->u.Button.IsDownFunc = BrushIsBrushTypeRoom;
	o2->ChangeFunc = BrushSetBrushTypeRoom;
	CSTRDUP(o2->Tooltip, "Room");
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.x += o2->Size.x;
	o2 = UIObjectCopy(o);
	UIButtonSetPic(o2, PicManagerGetPic(&gPicManager, "editor/room_painter"));
	o2->u.Button.IsDownFunc = BrushIsBrushTypeRoomPainter;
	o2->ChangeFunc = BrushSetBrushTypeRoomPainter;
	CSTRDUP(o2->Tooltip, "Room painter");
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.x += o2->Size.x;
	o2 = UIObjectCopy(o);
	UIButtonSetPic(o2, PicManagerGetPic(&gPicManager, "editor/select"));
	o2->u.Button.IsDownFunc = BrushIsBrushTypeSelect;
	o2->ChangeFunc = BrushSetBrushTypeSelect;
	CSTRDUP(o2->Tooltip, "Select and move");
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.x += o2->Size.x;
	o2 = UIObjectCopy(o);
	UIButtonSetPic(o2, PicManagerGetPic(&gPicManager, "editor/bucket"));
	o2->u.Button.IsDownFunc = BrushIsBrushTypeFill;
	o2->ChangeFunc = BrushSetBrushTypeFill;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.x += o2->Size.x;
	o2 = UIObjectCopy(o);
	UIButtonSetPic(o2, PicManagerGetPic(&gPicManager, "editor/add"));
	o2->u.Button.IsDownFunc = BrushIsBrushTypeAddItem;
	CSTRDUP(o2->Tooltip, "Add items\nRight click to remove");
	o2->Pos = pos;
	o2->OnFocusFunc = NULL;
	o2->OnUnfocusFunc = NULL;
	UIObjectAddChild(o2, CreateAddItemObjs(o2->Size, brush, co));
	UIObjectAddChild(c, o2);
	pos.x += o2->Size.x;
	o2 = UIObjectCopy(o);
	UIButtonSetPic(o2, PicManagerGetPic(&gPicManager, "editor/set_key"));
	o2->u.Button.IsDownFunc = BrushIsBrushTypeSetKey;
	CSTRDUP(o2->Tooltip, "Set key required for door");
	o2->Pos = pos;
	o2->OnFocusFunc = NULL;
	o2->OnUnfocusFunc = NULL;
	UIObjectAddChild(o2, CreateSetKeyObjs(o2->Size, brush));
	UIObjectAddChild(c, o2);
	pos.x += o2->Size.x;
	o2 = UIObjectCopy(o);
	UIButtonSetPic(o2, PicManagerGetPic(&gPicManager, "editor/set_exit"));
	o2->u.Button.IsDownFunc = BrushIsBrushTypeSetExit;
	o2->ChangeFunc = BrushSetBrushTypeSetExit;
	CSTRDUP(o2->Tooltip, "Set exit area (box drag)");
	o2->Pos = pos;
	UIObjectAddChild(c, o2);

	UIObjectDestroy(o);
	o = UIObjectCreate(
		UITYPE_LABEL, 0, Vec2iZero(), Vec2iNew(60, th));
	pos.x = x;
	pos.y += o2->Size.y;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = BrushGetMainTypeStr;
	o2->Data = brush;
	o2->ChangeFunc = BrushChangeMainType;
	o2->OnFocusFunc = ActivateBrush;
	o2->OnUnfocusFunc = DeactivateBrush;
	CSTRDUP(o2->Tooltip, "Left click to paint the map with this tile type");
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.x += o2->Size.x;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = BrushGetSecondaryTypeStr;
	o2->Data = brush;
	o2->ChangeFunc = BrushChangeSecondaryType;
	o2->OnFocusFunc = ActivateBrush;
	o2->OnUnfocusFunc = DeactivateBrush;
	CSTRDUP(o2->Tooltip, "Right click to paint the map with this tile type");
	o2->Pos = pos;
	UIObjectAddChild(c, o2);

	pos.x = x;
	pos.y += th;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = BrushGetSizeStr;
	o2->Data = brush;
	o2->ChangeFunc = BrushChangeSize;
	o2->OnFocusFunc = ActivateBrush;
	o2->OnUnfocusFunc = DeactivateBrush;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);

	UIObjectDestroy(o);
	o = UIObjectCreate(UITYPE_TEXTBOX, 0, Vec2iZero(), Vec2iNew(100, th));
	pos.x = x;
	pos.y += th;
	o2 = UIObjectCopy(o);
	o2->u.Textbox.TextLinkFunc = BrushGetGuideImageStr;
	o2->u.Textbox.MaxLen = sizeof((EditorBrush *)0)->GuideImage - 1;
	o2->Data = brush;
	o2->ChangesData = 0;
	o2->ChangeFunc = BrushLoadGuideImage;
	CSTRDUP(o2->u.Textbox.Hint, "(Tracing guide image)");
	o2->Pos = pos;
	UIObjectAddChild(c, o2);

	UIObjectDestroy(o);
	o = UIObjectCreate(UITYPE_LABEL, 0, Vec2iZero(), Vec2iNew(100, th));
	pos.x = x;
	pos.y += th;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = BrushGetGuideImageAlphaStr;
	o2->Data = brush;
	o2->ChangeFunc = BrushChangeGuideImageAlpha;
	o2->Pos = pos;
	UIObjectAddChild(c, o2);

	UIObjectDestroy(o);
	return c;
}

static UIObject *CreateSetKeyObjs(Vec2i pos, EditorBrush *brush)
{
	UIObject *o2;
	UIObject *c = UIObjectCreate(UITYPE_CONTEXT_MENU, 0, pos, Vec2iZero());

	UIObject *o = UIObjectCreate(
		UITYPE_CUSTOM, 0,
		Vec2iZero(), Vec2iNew(TILE_WIDTH + 4, TILE_HEIGHT + 4));
	o->ChangeFunc = BrushSetBrushTypeSetKey;
	o->u.CustomDrawFunc = DrawKey;
	o->OnFocusFunc = ActivateIndexedEditorBrush;
	o->OnUnfocusFunc = DeactivateIndexedEditorBrush;
	pos = Vec2iZero();
	for (int i = -1; i < KEY_COUNT; i++)
	{
		o2 = UIObjectCopy(o);
		o2->IsDynamicData = 1;
		CMALLOC(o2->Data, sizeof(IndexedEditorBrush));
		((IndexedEditorBrush *)o2->Data)->Brush = brush;
		((IndexedEditorBrush *)o2->Data)->u.ItemIndex = i;
		o2->Pos = pos;
		if (i == -1)
		{
			// -1 means no key
			CSTRDUP(o2->Tooltip, "no key");
		}
		UIObjectAddChild(c, o2);
		pos.x += o->Size.x;
	}

	UIObjectDestroy(o);
	return c;
}

