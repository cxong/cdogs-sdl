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
#include <cdogs/draw/draw_actor.h>
#include <cdogs/events.h>
#include <cdogs/font.h>
#include <cdogs/map.h>

#include "editor_ui_common.h"



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
	b->Brush->u.MapObject = b->u.MapObject;
}
static void BrushSetBrushTypeAddCharacter(void *data, int d)
{
	UNUSED(d);
	IndexedEditorBrush *b = data;
	b->Brush->Type = BRUSHTYPE_ADD_CHARACTER;
	b->Brush->u.ItemIndex = b->u.ItemIndex;
}
static void BrushSetBrushTypeAddObjective(void *data, int d)
{
	UNUSED(d);
	IndexedEditorBrush *b = data;
	b->Brush->Type = BRUSHTYPE_ADD_OBJECTIVE;
	b->Brush->u.ItemIndex = b->u.ItemIndex;
	b->Brush->Index2 = b->Index2;
}
static void BrushSetBrushTypeAddKey(void *data, int d)
{
	UNUSED(d);
	IndexedEditorBrush *b = data;
	b->Brush->Type = BRUSHTYPE_ADD_KEY;
	b->Brush->u.ItemIndex = b->u.ItemIndex;
}


static void DrawPickupSpawner(
	UIObject *o, GraphicsDevice *g, Vec2i pos, void *vData)
{
	const IndexedEditorBrush *data = vData;
	const MapObject *mo = data->u.MapObject;
	DisplayMapItem(
		Vec2iAdd(Vec2iAdd(pos, o->Pos), Vec2iScaleDiv(o->Size, 2)), mo);
	const Pic *pic = mo->u.PickupClass->Pic;
	pos = Vec2iMinus(pos, Vec2iScaleDiv(pic->size, 2));
	Blit(g, pic, Vec2iAdd(Vec2iAdd(pos, o->Pos), Vec2iScaleDiv(o->Size, 2)));
}
static void DrawCharacter(
	UIObject *o, GraphicsDevice *g, Vec2i pos, void *vData)
{
	UNUSED(g);
	EditorBrushAndCampaign *data = vData;
	CharacterStore *store = &data->Campaign->Setting.characters;
	Character *c = CArrayGet(&store->OtherChars, data->Brush.u.ItemIndex);
	DrawCharacterSimple(
		c,
		Vec2iAdd(Vec2iAdd(pos, o->Pos), Vec2iScaleDiv(o->Size, 2)),
		DIRECTION_DOWN, false, false);
}
static void DrawObjective(
	UIObject *o, GraphicsDevice *g, Vec2i pos, void *vData)
{
	UNUSED(g);
	EditorBrushAndCampaign *data = vData;
	Mission *m = CampaignGetCurrentMission(data->Campaign);
	const Objective *obj = CArrayGet(&m->Objectives, data->Brush.u.ItemIndex);
	if (obj == NULL)
	{
		return;
	}
	CharacterStore *store = &data->Campaign->Setting.characters;
	pos = Vec2iAdd(Vec2iAdd(pos, o->Pos), Vec2iScaleDiv(o->Size, 2));
	switch (obj->Type)
	{
	case OBJECTIVE_KILL:
		{
			Character *c = CArrayGet(
				&store->OtherChars,
				CharacterStoreGetSpecialId(store, data->Brush.Index2));
			DrawCharacterSimple(c, pos, DIRECTION_DOWN, false, false);
		}
		break;
	case OBJECTIVE_RESCUE:
		{
			Character *c = CArrayGet(
				&store->OtherChars,
				CharacterStoreGetPrisonerId(store, data->Brush.Index2));
			DrawCharacterSimple(c, pos, DIRECTION_DOWN, false, false);
		}
		break;
	case OBJECTIVE_COLLECT:
		{
			const Pic *p = obj->u.Pickup->Pic;
			pos = Vec2iMinus(pos, Vec2iScaleDiv(p->size, 2));
			Blit(&gGraphicsDevice, p, pos);
		}
		break;
	case OBJECTIVE_DESTROY:
		DisplayMapItem(pos, obj->u.MapObject);
		break;
	default:
		assert(0 && "invalid objective type");
		break;
	}
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
static void ActivateEditorBrushAndCampaignBrush(UIObject *o, void *data)
{
	UNUSED(o);
	EditorBrushAndCampaign *b = data;
	b->Brush.Brush->IsActive = true;
}
static bool DeactivateEditorBrushAndCampaignBrush(void *data)
{
	EditorBrushAndCampaign *b = data;
	b->Brush.Brush->IsActive = false;
	return false;
}


static bool AddMapItemBrushObjFunc(UIObject *o, MapObject *mo, void *vData);
static bool AddPickupSpawnerBrushObjFunc(
	UIObject *o, MapObject *mo, void *vData);
static UIObject *CreateAddCharacterObjs(
	Vec2i pos, EditorBrush *brush, CampaignOptions *co);
static UIObject *CreateAddObjectiveObjs(
	Vec2i pos, EditorBrush *brush, CampaignOptions *co);
static UIObject *CreateAddKeyObjs(Vec2i pos, EditorBrush *brush);
UIObject *CreateAddItemObjs(
	Vec2i pos, EditorBrush *brush, CampaignOptions *co)
{
	const int th = FontH();
	UIObject *o2;
	UIObject *c = UIObjectCreate(UITYPE_CONTEXT_MENU, 0, pos, Vec2iZero());

	UIObject *o = UIObjectCreate(
		UITYPE_LABEL, 0, Vec2iZero(), Vec2iNew(65, th));
	o->Data = brush;

	pos = Vec2iZero();
	o2 = UIObjectCopy(o);
	o2->Label = "Player start";
	o2->ChangeFunc = BrushSetBrushTypeSetPlayerStart;
	o2->Pos = pos;
	CSTRDUP(o2->Tooltip, "Location where players start");
	o2->OnFocusFunc = ActivateBrush;
	o2->OnUnfocusFunc = DeactivateBrush;
	o2->Data = brush;
	UIObjectAddChild(c, o2);
	pos.y += th;
	o2 = UIObjectCopy(o);
	o2->Label = "Map item >";
	o2->Pos = pos;
	UIObjectAddChild(o2, CreateAddMapItemObjs(
		o2->Size, AddMapItemBrushObjFunc, brush, sizeof(IndexedEditorBrush),
		true));
	UIObjectAddChild(c, o2);
	pos.y += th;
	o2 = UIObjectCopy(o);
	o2->Label = "Pickup spawner >";
	o2->Pos = pos;
	UIObjectAddChild(o2, CreateAddPickupSpawnerObjs(
		o2->Size, AddPickupSpawnerBrushObjFunc, brush,
		sizeof(IndexedEditorBrush)));
	UIObjectAddChild(c, o2);
	pos.y += th;
	o2 = UIObjectCopy(o);
	o2->Label = "Character >";
	o2->Pos = pos;
	UIObjectAddChild(o2, CreateAddCharacterObjs(o2->Size, brush, co));
	UIObjectAddChild(c, o2);
	pos.y += th;
	o2 = UIObjectCopy(o);
	o2->Label = "Objective >";
	o2->Pos = pos;
	UIObjectAddChild(o2, CreateAddObjectiveObjs(o2->Size, brush, co));
	UIObjectAddChild(c, o2);
	pos.y += th;
	o2 = UIObjectCopy(o);
	o2->Label = "Key >";
	o2->Pos = pos;
	UIObjectAddChild(o2, CreateAddKeyObjs(o2->Size, brush));
	UIObjectAddChild(c, o2);

	UIObjectDestroy(o);
	return c;
}

static void DrawMapItem(
	UIObject *o, GraphicsDevice *g, Vec2i pos, void *vData);
static char *MakeMapObjectTooltip(const MapObject *mo);
static bool AddMapItemBrushObjFunc(UIObject *o, MapObject *mo, void *vData)
{
	if (mo->Type != MAP_OBJECT_TYPE_NORMAL)
	{
		return false;
	}
	o->ChangeFunc = BrushSetBrushTypeAddMapItem;
	o->u.CustomDrawFunc = DrawMapItem;
	o->OnFocusFunc = ActivateIndexedEditorBrush;
	o->OnUnfocusFunc = DeactivateIndexedEditorBrush;
	((IndexedEditorBrush *)o->Data)->Brush = vData;
	((IndexedEditorBrush *)o->Data)->u.MapObject = mo;
	o->Tooltip = MakeMapObjectTooltip(mo);
	return true;
}
static void DrawMapItem(
	UIObject *o, GraphicsDevice *g, Vec2i pos, void *vData)
{
	UNUSED(g);
	const EditorBrush *brush = vData;
	DisplayMapItem(
		Vec2iAdd(Vec2iAdd(pos, o->Pos), Vec2iScaleDiv(o->Size, 2)),
		brush->u.MapObject);
}
static char *MakeMapObjectTooltip(const MapObject *mo)
{
	// Add a descriptive tooltip for the map object
	char buf[512];
	// Construct text representing explosion guns
	char exBuf[256];
	strcpy(exBuf, "");
	if (mo->DestroyGuns.size > 0)
	{
		sprintf(exBuf, "\nExplodes: ");
		for (int i = 0; i < (int)mo->DestroyGuns.size; i++)
		{
			if (i > 0)
			{
				strcat(exBuf, ", ");
			}
			const GunDescription **g = CArrayGet(&mo->DestroyGuns, i);
			strcat(exBuf, (*g)->name);
		}
	}
	sprintf(buf, "%s\nHealth: %d%s", mo->Name, mo->Health, exBuf);
	char *tmp;
	CSTRDUP(tmp, buf);
	return tmp;
}

static char *MakePickupTooltip(const MapObject *mo);
static bool AddPickupSpawnerBrushObjFunc(
	UIObject *o, MapObject *mo, void *vData)
{
	if (mo->Type != MAP_OBJECT_TYPE_PICKUP_SPAWNER)
	{
		return false;
	}
	o->ChangeFunc = BrushSetBrushTypeAddMapItem;
	o->u.CustomDrawFunc = DrawPickupSpawner;
	o->OnFocusFunc = ActivateIndexedEditorBrush;
	o->OnUnfocusFunc = DeactivateIndexedEditorBrush;
	((IndexedEditorBrush *)o->Data)->Brush = vData;
	((IndexedEditorBrush *)o->Data)->u.MapObject = mo;
	o->Tooltip = MakePickupTooltip(mo);
	return true;
}
static char *MakePickupTooltip(const MapObject *mo)
{
	char *tmp;
	CSTRDUP(tmp, mo->u.PickupClass->Name);
	return tmp;
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
	c->Highlighted = NULL;
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
	o->OnFocusFunc = ActivateEditorBrushAndCampaignBrush;
	o->OnUnfocusFunc = DeactivateEditorBrushAndCampaignBrush;
	Vec2i pos = Vec2iZero();
	int width = 8;
	for (int i = 0; i < (int)store->OtherChars.size; i++)
	{
		UIObject *o2 = UIObjectCopy(o);
		Character *ch = CArrayGet(&store->OtherChars, i);
		CSTRDUP(o2->Tooltip, ch->Gun->name);
		o2->IsDynamicData = 1;
		CMALLOC(o2->Data, sizeof(EditorBrushAndCampaign));
		((EditorBrushAndCampaign *)o2->Data)->Brush.Brush = data->Brush.Brush;
		((EditorBrushAndCampaign *)o2->Data)->Campaign = data->Campaign;
		((EditorBrushAndCampaign *)o2->Data)->Brush.u.ItemIndex = i;
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
	Mission *m = CampaignGetCurrentMission(data->Campaign);
	// Check if the data is still the same; if so don't recreate the
	// child UI objects
	// This is because during the course of UI operations, this element
	// could be highlighted again; if we recreate then we invalidate
	// UI pointers.
	bool needToRecreate = false;
	int childIndex = 0;
	CA_FOREACH(const Objective, obj, m->Objectives)
		int secondaryCount = 1;
		const CharacterStore *store = &data->Campaign->Setting.characters;
		switch (obj->Type)
		{
		case OBJECTIVE_KILL:
			secondaryCount = (int)store->specialIds.size;
			break;
		case OBJECTIVE_COLLECT:
			break;
		case OBJECTIVE_DESTROY:
			break;
		case OBJECTIVE_RESCUE:
			secondaryCount = (int)store->prisonerIds.size;
			break;
		default:
			continue;
		}
		for (int j = 0; j < (int)secondaryCount; j++)
		{
			if ((int)c->Children.size <= childIndex)
			{
				needToRecreate = true;
				break;
			}
			UIObject *o2 = *(UIObject **)CArrayGet(&c->Children, childIndex);
			if (((EditorBrushAndCampaign *)o2->Data)->Brush.u.ItemIndex != _ca_index ||
				((EditorBrushAndCampaign *)o2->Data)->Brush.Index2 != j)
			{
				needToRecreate = true;
				break;
			}
			childIndex++;
		}
		if (needToRecreate)
		{
			break;
		}
	CA_FOREACH_END()
	if (!needToRecreate)
	{
		return;
	}

	// Recreate the child UI objects
	c->Highlighted = NULL;
	CA_FOREACH(UIObject *, obj, c->Children)
		UIObjectDestroy(*obj);
	CA_FOREACH_END()
	CArrayTerminate(&c->Children);
	CArrayInit(&c->Children, sizeof c);

	UIObject *o = UIObjectCreate(
		UITYPE_CUSTOM, 0,
		Vec2iZero(), Vec2iNew(TILE_WIDTH + 4, TILE_HEIGHT * 2 + 4));
	o->ChangeFunc = BrushSetBrushTypeAddObjective;
	o->u.CustomDrawFunc = DrawObjective;
	o->OnFocusFunc = ActivateEditorBrushAndCampaignBrush;
	o->OnUnfocusFunc = DeactivateEditorBrushAndCampaignBrush;
	Vec2i pos = Vec2iZero();
	CA_FOREACH(const Objective, obj, m->Objectives)
		int secondaryCount = 1;
		const CharacterStore *store = &data->Campaign->Setting.characters;
		switch (obj->Type)
		{
		case OBJECTIVE_KILL:
			secondaryCount = (int)store->specialIds.size;
			o->Size.y = TILE_HEIGHT * 2 + 4;
			break;
		case OBJECTIVE_COLLECT:
			o->Size.y = TILE_HEIGHT + 4;
			break;
		case OBJECTIVE_DESTROY:
			o->Size.y = TILE_HEIGHT * 2 + 4;
			break;
		case OBJECTIVE_RESCUE:
			secondaryCount = (int)store->prisonerIds.size;
			o->Size.y = TILE_HEIGHT * 2 + 4;
			break;
		default:
			continue;
		}
		for (int j = 0; j < (int)secondaryCount; j++)
		{
			UIObject *o2 = UIObjectCopy(o);
			CSTRDUP(o2->Tooltip, ObjectiveTypeStr(obj->Type));
			o2->IsDynamicData = true;
			CMALLOC(o2->Data, sizeof(EditorBrushAndCampaign));
			((EditorBrushAndCampaign *)o2->Data)->Brush.Brush =
				data->Brush.Brush;
			((EditorBrushAndCampaign *)o2->Data)->Campaign = data->Campaign;
			((EditorBrushAndCampaign *)o2->Data)->Brush.u.ItemIndex = _ca_index;
			((EditorBrushAndCampaign *)o2->Data)->Brush.Index2 = j;
			o2->Pos = pos;
			UIObjectAddChild(c, o2);
			pos.x += o->Size.x;
		}
		pos.x = 0;
		pos.y += o->Size.y;
	CA_FOREACH_END()
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
	o->OnFocusFunc = ActivateIndexedEditorBrush;
	o->OnUnfocusFunc = DeactivateIndexedEditorBrush;
	pos = Vec2iZero();
	int width = 4;
	for (int i = 0; i < KEY_COUNT; i++)
	{
		o2 = UIObjectCopy(o);
		o2->IsDynamicData = 1;
		CMALLOC(o2->Data, sizeof(IndexedEditorBrush));
		((IndexedEditorBrush *)o2->Data)->Brush = brush;
		((IndexedEditorBrush *)o2->Data)->u.ItemIndex = i;
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
