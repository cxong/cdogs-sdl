/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (c) 2013-2014, Cong Xu
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

#include <cdogs/draw.h>
#include <cdogs/events.h>
#include <cdogs/map.h>
#include <cdogs/text.h>

#include "editor_ui_common.h"


static void MissionCheckTypeStatic(
	UIObject *o, GraphicsDevice *g, Vec2i pos, void *data)
{
	CampaignOptions *co = data;
	Mission *m = CampaignGetCurrentMission(co);
	UNUSED(g);
	UNUSED(pos);
	if (!m || m->Type != MAPTYPE_STATIC)
	{
		o->IsVisible = 0;
		// Need to unhighlight to prevent children being drawn
		UIObjectUnhighlight(o);
		return;
	}
	o->IsVisible = 1;
}

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
typedef struct
{
	EditorBrush *Brush;
	int ItemIndex;
	int Index2;
} IndexedEditorBrush;
static void DrawMapItem(
	UIObject *o, GraphicsDevice *g, Vec2i pos, void *vData)
{
	UNUSED(g);
	IndexedEditorBrush *data = vData;
	MapObject *mo = MapObjectGet(data->ItemIndex);
	DisplayMapItem(
		Vec2iAdd(Vec2iAdd(pos, o->Pos), Vec2iScaleDiv(o->Size, 2)), mo);
}
static void DisplayWreck(Vec2i pos, MapObject *mo);
static void DrawWreck(
	UIObject *o, GraphicsDevice *g, Vec2i pos, void *vData)
{
	UNUSED(g);
	IndexedEditorBrush *data = vData;
	MapObject *mo = MapObjectGet(data->ItemIndex);
	DisplayWreck(
		Vec2iAdd(Vec2iAdd(pos, o->Pos), Vec2iScaleDiv(o->Size, 2)), mo);
}
static void DisplayWreck(Vec2i pos, MapObject *mo)
{
	const TOffsetPic *pic = &cGeneralPics[mo->wreckedPic];
	DrawTPic(
		pos.x + pic->dx, pos.y + pic->dy,
		PicManagerGetOldPic(&gPicManager, pic->picIndex));
}
typedef struct
{
	IndexedEditorBrush Brush;
	CampaignOptions *Campaign;
} EditorBrushAndCampaign;
static void DrawCharacter(
	UIObject *o, GraphicsDevice *g, Vec2i pos, void *vData)
{
	UNUSED(g);
	EditorBrushAndCampaign *data = vData;
	CharacterStore *store = &data->Campaign->Setting.characters;
	Character *c = CArrayGet(&store->OtherChars, data->Brush.ItemIndex);
	DisplayCharacter(
		Vec2iAdd(Vec2iAdd(pos, o->Pos), Vec2iScaleDiv(o->Size, 2)),
		c, 0, 0);
}
static void DisplayPickupItem(Vec2i pos, int pickupItem);
static void DrawObjective(
	UIObject *o, GraphicsDevice *g, Vec2i pos, void *vData)
{
	UNUSED(g);
	EditorBrushAndCampaign *data = vData;
	Mission *m = CampaignGetCurrentMission(data->Campaign);
	MissionObjective *mobj = CArrayGet(&m->Objectives, data->Brush.ItemIndex);
	CharacterStore *store = &data->Campaign->Setting.characters;
	pos = Vec2iAdd(Vec2iAdd(pos, o->Pos), Vec2iScaleDiv(o->Size, 2));
	switch (mobj->Type)
	{
		case OBJECTIVE_KILL:
		{
			Character *c = store->specials[data->Brush.Index2];
			DisplayCharacter(pos, c, 0, 0);
		}
			break;
		case OBJECTIVE_RESCUE:
		{
			Character *c = store->prisoners[data->Brush.Index2];
			DisplayCharacter(pos, c, 0, 0);
		}
			break;
		case OBJECTIVE_COLLECT:
		{
			struct Objective *obj =
				CArrayGet(&gMission.Objectives, data->Brush.ItemIndex);
			DisplayPickupItem(pos, obj->pickupItem);
		}
			break;
		case OBJECTIVE_DESTROY:
		{
			MapObject *mo = MapObjectGet(mobj->Index);
			DisplayMapItem(pos, mo);
		}
			break;
		default:
			assert(0 && "invalid objective type");
			break;
	}
}
static void DisplayPickupItem(Vec2i pos, int pickupItem)
{
	TOffsetPic pic = cGeneralPics[pickupItem];
	DrawTPic(
		pos.x + pic.dx, pos.y + pic.dy,
		PicManagerGetOldPic(&gPicManager, pic.picIndex));
}
static void DrawKey(UIObject *o, GraphicsDevice *g, Vec2i pos, void *vData)
{
	UNUSED(g);
	EditorBrushAndCampaign *data = vData;
	PicPaletted *keyPic = PicManagerGetOldPic(
		&gPicManager,
		cGeneralPics[gMission.keyPics[data->Brush.ItemIndex]].picIndex);
	pos = Vec2iAdd(Vec2iAdd(pos, o->Pos), Vec2iScaleDiv(o->Size, 2));
	DrawTPic(pos.x, pos.y, keyPic);
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
	SDL_Surface *image = IMG_Load(b->GuideImage);
	if (!image)
	{
		// If new guide image is empty, unload last image anyway
		if (strlen(b->GuideImage) == 0)
		{
			SDL_FreeSurface(b->GuideImageSurface);
			b->GuideImageSurface = NULL;
		}
		return;
	}
	if (b->GuideImageSurface)
	{
		SDL_FreeSurface(b->GuideImageSurface);
		b->GuideImageSurface = NULL;
	}
	b->GuideImageSurface = SDL_ConvertSurface(
		image, gGraphicsDevice.screen->format, SDL_SWSURFACE);
	SDL_FreeSurface(image);
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
static int BrushIsBrushTypeSelect(void *data)
{
	EditorBrush *b = data;
	return b->Type == BRUSHTYPE_SELECT;
}
static int BrushIsBrushTypeAddItem(void *data)
{
	EditorBrush *b = data;
	return
		b->Type == BRUSHTYPE_SET_PLAYER_START ||
		b->Type == BRUSHTYPE_ADD_ITEM ||
		b->Type == BRUSHTYPE_ADD_WRECK ||
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
static void BrushSetBrushTypeSelect(void *data, int d)
{
	UNUSED(d);
	EditorBrush *b = data;
	b->Type = BRUSHTYPE_SELECT;
}
static void BrushSetBrushTypeSetPlayerStart(void *data, int d)
{
	UNUSED(d);
	EditorBrush *b = data;
	b->Type = BRUSHTYPE_SET_PLAYER_START;
}
static void BrushSetBrushTypeAddMapItem(void *data, int d)
{
	UNUSED(d);
	IndexedEditorBrush *b = data;
	b->Brush->Type = BRUSHTYPE_ADD_ITEM;
	b->Brush->ItemIndex = b->ItemIndex;
}
static void BrushSetBrushTypeAddWreck(void *data, int d)
{
	UNUSED(d);
	IndexedEditorBrush *b = data;
	b->Brush->Type = BRUSHTYPE_ADD_WRECK;
	b->Brush->ItemIndex = b->ItemIndex;
}
static void BrushSetBrushTypeAddCharacter(void *data, int d)
{
	UNUSED(d);
	IndexedEditorBrush *b = data;
	b->Brush->Type = BRUSHTYPE_ADD_CHARACTER;
	b->Brush->ItemIndex = b->ItemIndex;
}
static void BrushSetBrushTypeAddObjective(void *data, int d)
{
	UNUSED(d);
	IndexedEditorBrush *b = data;
	b->Brush->Type = BRUSHTYPE_ADD_OBJECTIVE;
	b->Brush->ItemIndex = b->ItemIndex;
	b->Brush->Index2 = b->Index2;
}
static void BrushSetBrushTypeAddKey(void *data, int d)
{
	UNUSED(d);
	IndexedEditorBrush *b = data;
	b->Brush->Type = BRUSHTYPE_ADD_KEY;
	b->Brush->ItemIndex = b->ItemIndex;
}
static void BrushSetBrushTypeSetKey(void *data, int d)
{
	UNUSED(d);
	IndexedEditorBrush *b = data;
	b->Brush->Type = BRUSHTYPE_SET_KEY;
	b->Brush->ItemIndex = b->ItemIndex;
}
static void BrushSetBrushTypeSetExit(void *data, int d)
{
	UNUSED(d);
	EditorBrush *b = data;
	b->Type = BRUSHTYPE_SET_EXIT;
}
static void ActivateBrush(UIObject *o, void *data)
{
	UNUSED(o);
	EditorBrush *b = data;
	b->IsActive = 1;
}
static void DeactivateBrush(void *data)
{
	EditorBrush *b = data;
	b->IsActive = 0;
}


static UIObject *CreateAddItemObjs(
	Vec2i pos, EditorBrush *brush, CampaignOptions *co);
static UIObject *CreateSetKeyObjs(Vec2i pos, EditorBrush *brush);
UIObject *CreateStaticMapObjs(
	Vec2i pos, CampaignOptions *co, EditorBrush *brush)
{
	int x = pos.x;
	int th = CDogsTextHeight();
	UIObject *c = UIObjectCreate(UITYPE_CUSTOM, 0, Vec2iZero(), Vec2iZero());
	UIObject *o2;
	// Use a custom UIObject to check whether the map type matches,
	// and set visibility
	c->u.CustomDrawFunc = MissionCheckTypeStatic;
	c->Data = co;

	UIObject *o = UIObjectCreate(UITYPE_BUTTON, 0, Vec2iZero(), Vec2iZero());
	o->Data = brush;
	o->OnFocusFunc = ActivateBrush;
	o->OnUnfocusFunc = DeactivateBrush;
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
	UIButtonSetPic(o2, PicManagerGetPic(&gPicManager, "editor/select"));
	o2->u.Button.IsDownFunc = BrushIsBrushTypeSelect;
	o2->ChangeFunc = BrushSetBrushTypeSelect;
	CSTRDUP(o2->Tooltip, "Select and move");
	o2->Pos = pos;
	UIObjectAddChild(c, o2);
	pos.x += o2->Size.x;
	o2 = UIObjectCopy(o);
	UIButtonSetPic(o2, PicManagerGetPic(&gPicManager, "editor/add"));
	o2->u.Button.IsDownFunc = BrushIsBrushTypeAddItem;
	CSTRDUP(o2->Tooltip, "Add items\nRight click to remove");
	o2->Pos = pos;
	UIObjectAddChild(o2, CreateAddItemObjs(o2->Size, brush, co));
	UIObjectAddChild(c, o2);
	pos.x += o2->Size.x;
	o2 = UIObjectCopy(o);
	UIButtonSetPic(o2, PicManagerGetPic(&gPicManager, "editor/set_key"));
	o2->u.Button.IsDownFunc = BrushIsBrushTypeSetKey;
	CSTRDUP(o2->Tooltip, "Set key required for door");
	o2->Pos = pos;
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

static UIObject *CreateAddMapItemObjs(Vec2i pos, EditorBrush *brush);
static UIObject *CreateAddWreckObjs(Vec2i pos, EditorBrush *brush);
static UIObject *CreateAddCharacterObjs(
	Vec2i pos, EditorBrush *brush, CampaignOptions *co);
static UIObject *CreateAddObjectiveObjs(
	Vec2i pos, EditorBrush *brush, CampaignOptions *co);
static UIObject *CreateAddKeyObjs(Vec2i pos, EditorBrush *brush);
static UIObject *CreateAddItemObjs(
	Vec2i pos, EditorBrush *brush, CampaignOptions *co)
{
	int th = CDogsTextHeight();
	UIObject *o2;
	UIObject *c = UIObjectCreate(UITYPE_CONTEXT_MENU, 0, pos, Vec2iZero());

	UIObject *o = UIObjectCreate(
		UITYPE_LABEL, 0, Vec2iZero(), Vec2iNew(50, th));
	o->Data = brush;

	pos = Vec2iZero();
	o2 = UIObjectCopy(o);
	o2->Label = "Player start";
	o2->ChangeFunc = BrushSetBrushTypeSetPlayerStart;
	o2->Pos = pos;
	CSTRDUP(o2->Tooltip, "Location where players start");
	UIObjectAddChild(c, o2);
	pos.y += th;
	o2 = UIObjectCopy(o);
	o2->Label = "Map item";
	o2->Pos = pos;
	UIObjectAddChild(o2, CreateAddMapItemObjs(o2->Size, brush));
	UIObjectAddChild(c, o2);
	pos.y += th;
	o2 = UIObjectCopy(o);
	o2->Label = "Wreck";
	o2->Pos = pos;
	UIObjectAddChild(o2, CreateAddWreckObjs(o2->Size, brush));
	UIObjectAddChild(c, o2);
	pos.y += th;
	o2 = UIObjectCopy(o);
	o2->Label = "Character";
	o2->Pos = pos;
	UIObjectAddChild(o2, CreateAddCharacterObjs(o2->Size, brush, co));
	UIObjectAddChild(c, o2);
	pos.y += th;
	o2 = UIObjectCopy(o);
	o2->Label = "Objective";
	o2->Pos = pos;
	UIObjectAddChild(o2, CreateAddObjectiveObjs(o2->Size, brush, co));
	UIObjectAddChild(c, o2);
	pos.y += th;
	o2 = UIObjectCopy(o);
	o2->Label = "Key";
	o2->Pos = pos;
	UIObjectAddChild(o2, CreateAddKeyObjs(o2->Size, brush));
	UIObjectAddChild(c, o2);

	UIObjectDestroy(o);
	return c;
}
static UIObject *CreateAddMapItemObjs(Vec2i pos, EditorBrush *brush)
{
	UIObject *o2;
	UIObject *c = UIObjectCreate(UITYPE_CONTEXT_MENU, 0, pos, Vec2iZero());

	UIObject *o = UIObjectCreate(
		UITYPE_CUSTOM, 0,
		Vec2iZero(), Vec2iNew(TILE_WIDTH/* * 2*/ + 4, TILE_HEIGHT * /*3*/2 + 4));
	o->ChangeFunc = BrushSetBrushTypeAddMapItem;
	o->u.CustomDrawFunc = DrawMapItem;
	pos = Vec2iZero();
	int width = 8;
	for (int i = 0; i < MapObjectGetCount(); i++)
	{
		o2 = UIObjectCopy(o);
		o2->IsDynamicData = 1;
		CMALLOC(o2->Data, sizeof(IndexedEditorBrush));
		((IndexedEditorBrush *)o2->Data)->Brush = brush;
		((IndexedEditorBrush *)o2->Data)->ItemIndex = i;
		o2->Pos = pos;
		UIObjectAddChild(c, o2);
		pos.x += o->Size.x;
		if (((i + 1) % width) == 0)
		{
			pos.x = 0;
			pos.y += o->Size.y;
		}
	}

	UIObjectDestroy(o);
	return c;
}
static UIObject *CreateAddWreckObjs(Vec2i pos, EditorBrush *brush)
{
	UIObject *o2;
	UIObject *c = UIObjectCreate(UITYPE_CONTEXT_MENU, 0, pos, Vec2iZero());
	
	UIObject *o = UIObjectCreate(
		UITYPE_CUSTOM, 0,
		Vec2iZero(), Vec2iNew(TILE_WIDTH + 4, TILE_HEIGHT + 4));
	o->ChangeFunc = BrushSetBrushTypeAddWreck;
	o->u.CustomDrawFunc = DrawWreck;
	pos = Vec2iZero();
	int width = 8;
	for (int i = 0; i < MapObjectGetCount(); i++)
	{
		o2 = UIObjectCopy(o);
		o2->IsDynamicData = 1;
		CMALLOC(o2->Data, sizeof(IndexedEditorBrush));
		((IndexedEditorBrush *)o2->Data)->Brush = brush;
		((IndexedEditorBrush *)o2->Data)->ItemIndex = i;
		o2->Pos = pos;
		UIObjectAddChild(c, o2);
		pos.x += o->Size.x;
		if (((i + 1) % width) == 0)
		{
			pos.x = 0;
			pos.y += o->Size.y;
		}
	}
	
	UIObjectDestroy(o);
	return c;
}
static void CreateAddCharacterSubObjs(UIObject *c, void *vData);
static UIObject *CreateAddCharacterObjs(
	Vec2i pos, EditorBrush *brush, CampaignOptions *co)
{
	UIObject *c = UIObjectCreate(UITYPE_CONTEXT_MENU, 0, pos, Vec2iZero());
	// Need to update UI objects dynamically as new characters can be
	// added and removed
	c->OnFocusFunc = CreateAddCharacterSubObjs;
	c->IsDynamicData = 1;
	CMALLOC(c->Data, sizeof(EditorBrushAndCampaign));
	((EditorBrushAndCampaign *)c->Data)->Brush.Brush = brush;
	((EditorBrushAndCampaign *)c->Data)->Campaign = co;

	return c;
}
static void CreateAddCharacterSubObjs(UIObject *c, void *vData)
{
	EditorBrushAndCampaign *data = vData;
	CharacterStore *store = &data->Campaign->Setting.characters;
	if (c->Children.size == store->OtherChars.size)
	{
		return;
	}
	// Recreate the child UI objects
	UIObject **objs = c->Children.data;
	for (int i = 0; i < (int)c->Children.size; i++, objs++)
	{
		UIObjectDestroy(*objs);
	}
	CArrayTerminate(&c->Children);
	CArrayInit(&c->Children, sizeof c);

	UIObject *o = UIObjectCreate(
		UITYPE_CUSTOM, 0,
		Vec2iZero(), Vec2iNew(TILE_WIDTH + 4, TILE_HEIGHT * 2 + 4));
	o->ChangeFunc = BrushSetBrushTypeAddCharacter;
	o->u.CustomDrawFunc = DrawCharacter;
	Vec2i pos = Vec2iZero();
	int width = 8;
	for (int i = 0; i < (int)store->OtherChars.size; i++)
	{
		UIObject *o2 = UIObjectCopy(o);
		o2->IsDynamicData = 1;
		CMALLOC(o2->Data, sizeof(EditorBrushAndCampaign));
		((EditorBrushAndCampaign *)o2->Data)->Brush.Brush = data->Brush.Brush;
		((EditorBrushAndCampaign *)o2->Data)->Campaign = data->Campaign;
		((EditorBrushAndCampaign *)o2->Data)->Brush.ItemIndex = i;
		o2->Pos = pos;
		UIObjectAddChild(c, o2);
		pos.x += o->Size.x;
		if (((i + 1) % width) == 0)
		{
			pos.x = 0;
			pos.y += o->Size.y;
		}
	}
	UIObjectDestroy(o);
}
static void CreateAddObjectiveSubObjs(UIObject *c, void *vData);
static UIObject *CreateAddObjectiveObjs(
	Vec2i pos, EditorBrush *brush, CampaignOptions *co)
{
	UIObject *c = UIObjectCreate(UITYPE_CONTEXT_MENU, 0, pos, Vec2iZero());
	// Need to update UI objects dynamically as new objectives can be
	// added and removed
	c->OnFocusFunc = CreateAddObjectiveSubObjs;
	CSTRDUP(c->Tooltip,
		"Manually place objectives\nThe rest will be randomly placed");
	c->IsDynamicData = 1;
	CMALLOC(c->Data, sizeof(EditorBrushAndCampaign));
	((EditorBrushAndCampaign *)c->Data)->Brush.Brush = brush;
	((EditorBrushAndCampaign *)c->Data)->Campaign = co;
	
	return c;
}
static void CreateAddObjectiveSubObjs(UIObject *c, void *vData)
{
	EditorBrushAndCampaign *data = vData;
	// Recreate the child UI objects
	UIObject **objs = c->Children.data;
	for (int i = 0; i < (int)c->Children.size; i++, objs++)
	{
		UIObjectDestroy(*objs);
	}
	CArrayTerminate(&c->Children);
	CArrayInit(&c->Children, sizeof c);
	
	UIObject *o = UIObjectCreate(
		UITYPE_CUSTOM, 0,
		Vec2iZero(), Vec2iNew(TILE_WIDTH + 4, TILE_HEIGHT * 2 + 4));
	o->ChangeFunc = BrushSetBrushTypeAddObjective;
	o->u.CustomDrawFunc = DrawObjective;
	Vec2i pos = Vec2iZero();
	Mission *m = CampaignGetCurrentMission(data->Campaign);
	for (int i = 0; i < (int)m->Objectives.size; i++)
	{
		MissionObjective *mobj = CArrayGet(&m->Objectives, i);
		int secondaryCount = 1;
		CharacterStore *store = &data->Campaign->Setting.characters;
		switch (mobj->Type)
		{
			case OBJECTIVE_KILL:
				secondaryCount = store->specialCount;
				o->Size.y = TILE_HEIGHT * 2 + 4;
				break;
			case OBJECTIVE_COLLECT:
				o->Size.y = TILE_HEIGHT + 4;
				break;
			case OBJECTIVE_DESTROY:
				o->Size.y = TILE_HEIGHT * 2 + 4;
				break;
			case OBJECTIVE_RESCUE:
				secondaryCount = store->prisonerCount;
				o->Size.y = TILE_HEIGHT * 2 + 4;
				break;
			default:
				continue;
		}
		for (int j = 0; j < (int)secondaryCount; j++)
		{
			UIObject *o2 = UIObjectCopy(o);
			CSTRDUP(c->Tooltip, ObjectiveTypeStr(mobj->Type));
			o2->IsDynamicData = 1;
			CMALLOC(o2->Data, sizeof(EditorBrushAndCampaign));
			((EditorBrushAndCampaign *)o2->Data)->Brush.Brush =
				data->Brush.Brush;
			((EditorBrushAndCampaign *)o2->Data)->Campaign = data->Campaign;
			((EditorBrushAndCampaign *)o2->Data)->Brush.ItemIndex = i;
			((EditorBrushAndCampaign *)o2->Data)->Brush.Index2 = j;
			o2->Pos = pos;
			UIObjectAddChild(c, o2);
			pos.x += o->Size.x;
		}
		pos.x = 0;
		pos.y += o->Size.y;
	}
	UIObjectDestroy(o);
}
static UIObject *CreateAddKeyObjs(Vec2i pos, EditorBrush *brush)
{
	UIObject *o2;
	UIObject *c = UIObjectCreate(UITYPE_CONTEXT_MENU, 0, pos, Vec2iZero());

	UIObject *o = UIObjectCreate(
		UITYPE_CUSTOM, 0,
		Vec2iZero(), Vec2iNew(TILE_WIDTH + 4, TILE_HEIGHT + 4));
	o->ChangeFunc = BrushSetBrushTypeAddKey;
	o->u.CustomDrawFunc = DrawKey;
	pos = Vec2iZero();
	int width = 4;
	for (int i = 0; i < KEY_COUNT; i++)
	{
		o2 = UIObjectCopy(o);
		o2->IsDynamicData = 1;
		CMALLOC(o2->Data, sizeof(IndexedEditorBrush));
		((IndexedEditorBrush *)o2->Data)->Brush = brush;
		((IndexedEditorBrush *)o2->Data)->ItemIndex = i;
		o2->Pos = pos;
		UIObjectAddChild(c, o2);
		pos.x += o->Size.x;
		if (((i + 1) % width) == 0)
		{
			pos.x = 0;
			pos.y += o->Size.y;
		}
	}
	
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
	pos = Vec2iZero();
	int width = 4;
	for (int i = 0; i < KEY_COUNT; i++)
	{
		o2 = UIObjectCopy(o);
		o2->IsDynamicData = 1;
		CMALLOC(o2->Data, sizeof(IndexedEditorBrush));
		((IndexedEditorBrush *)o2->Data)->Brush = brush;
		((IndexedEditorBrush *)o2->Data)->ItemIndex = i;
		o2->Pos = pos;
		UIObjectAddChild(c, o2);
		pos.x += o->Size.x;
		if (((i + 1) % width) == 0)
		{
			pos.x = 0;
			pos.y += o->Size.y;
		}
	}
	
	UIObjectDestroy(o);
	return c;
}
