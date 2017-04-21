/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (c) 2014, 2016-2017 Cong Xu
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
	const Pic *pic = MapObjectGetPic(mo, &offset);
	Blit(&gGraphicsDevice, pic, Vec2iAdd(pos, offset));
}

void DisplayMapItemWithDensity(
	const Vec2i pos, const MapObjectDensity *mod, const bool isHighlighted)
{
	DisplayMapItem(pos, mod->M);
	if (isHighlighted)
	{
		FontCh('>', Vec2iAdd(pos, Vec2iNew(-8, -4)));
	}
	char s[10];
	sprintf(s, "%d", mod->Density);
	FontStr(s, Vec2iAdd(pos, Vec2iNew(-8, 5)));
}

void DrawKey(UIObject *o, GraphicsDevice *g, Vec2i pos, void *vData)
{
	const IndexedEditorBrush *data = vData;
	if (data->u.ItemIndex == -1)
	{
		// No key; don't draw
		return;
	}
	const Mission *m = CampaignGetCurrentMission(&gCampaign);
	const Pic *pic = KeyPickupClass(m->KeyStyle, data->u.ItemIndex)->Pic;
	pos = Vec2iAdd(Vec2iAdd(pos, o->Pos), Vec2iScaleDiv(o->Size, 2));
	pos = Vec2iMinus(pos, Vec2iScaleDiv(pic->size, 2));
	Blit(g, pic, pos);
}

void InsertMission(CampaignOptions *co, Mission *mission, int idx)
{
	Mission m;
	if (mission == NULL)
	{
		MissionInit(&m);
		m.Size = Vec2iNew(48, 48);
		// Set some default values for the mission
		m.u.Classic.CorridorWidth = 1;
		m.u.Classic.Rooms.Min = m.u.Classic.Rooms.Max = 5;
		m.u.Classic.Rooms.WallLength = 1;
		m.u.Classic.Rooms.WallPad = 1;
		m.u.Classic.Doors.Min = m.u.Classic.Doors.Max = 1;
		m.u.Classic.Pillars.Min = m.u.Classic.Pillars.Max = 1;
		mission = &m;
	}
	else
	{
		memset(&m, 0, sizeof m);
		MissionCopy(&m, mission);
	}
	CArrayInsert(&co->Setting.Missions, idx, &m);
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

	SDL_Keycode k = SDL_GetKeyFromScancode(GetKey(&gEventHandlers));
	return k == SDLK_y;
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

void DisplayFlag(
	const Vec2i pos, const char *s, const bool isOn, const bool isHighlighted)
{
	color_t labelMask = isHighlighted ? colorRed : colorWhite;
	Vec2i p = FontStrMask(s, pos, labelMask);
	p = FontChMask(':', p, labelMask);
	FontStrMask(isOn ? "On" : "Off", p, isOn ? colorPurple : colorWhite);
}

static const char *CampaignGetSeedStr(UIObject *o, void *data);
static void CampaignChangeSeed(void *data, int d);
UIObject *CreateCampaignSeedObj(const Vec2i pos, CampaignOptions *co)
{
	const int th = FontH();
	UIObject *o = UIObjectCreate(
		UITYPE_LABEL, 0, Vec2iZero(), Vec2iNew(50, th));
	o->ChangesData = true;
	o->u.LabelFunc = CampaignGetSeedStr;
	o->Data = co;
	o->ChangeFunc = CampaignChangeSeed;
	CSTRDUP(o->Tooltip, "Preview with different random seed");
	o->Pos = pos;
	return o;
}
static const char *CampaignGetSeedStr(UIObject *o, void *data)
{
	static char s[128];
	UNUSED(o);
	CampaignOptions *co = data;
	if (!CampaignGetCurrentMission(co)) return NULL;
	sprintf(s, "Seed: %d", ConfigGetInt(&gConfig, "Game.RandomSeed"));
	return s;
}
static void CampaignChangeSeed(void *data, int d)
{
	UNUSED(data);
	if (gEventHandlers.keyboard.modState & KMOD_SHIFT)
	{
		d *= 10;
	}
	ConfigSetInt(
		&gConfig, "Game.RandomSeed",
		ConfigGetInt(&gConfig, "Game.RandomSeed") + d);
}

typedef struct
{
	bool (*ObjFunc)(UIObject *, MapObject *, void *);
	void *Data;
	// Data size required for map item change checking
	size_t DataSize;
	Vec2i GridItemSize;
	Vec2i GridSize;
	Vec2i PageOffset;
} CreateAddMapItemObjsImplData;
static UIObject *CreateAddMapItemObjsImpl(
	Vec2i pos, CreateAddMapItemObjsImplData data);
UIObject *CreateAddMapItemObjs(
	const Vec2i pos, bool (*objFunc)(UIObject *, MapObject *, void *),
	void *data, const size_t dataSize, const bool expandDown)
{
	CreateAddMapItemObjsImplData d;
	d.ObjFunc = objFunc;
	d.Data = data;
	d.DataSize = dataSize;
	d.GridItemSize = Vec2iNew(TILE_WIDTH + 4, TILE_HEIGHT * 2 + 4);
	d.GridSize = Vec2iNew(8, 6);
	if (expandDown)
	{
		d.PageOffset = Vec2iNew(-5, 5);
	}
	else
	{
		d.PageOffset =
			Vec2iNew(-5, -d.GridItemSize.y * d.GridSize.y - FontH() - 5);
	}
	return CreateAddMapItemObjsImpl(pos, d);
}
UIObject *CreateAddPickupSpawnerObjs(
	const Vec2i pos, bool (*objFunc)(UIObject *, MapObject *, void *),
	void *data, const size_t dataSize)
{
	CreateAddMapItemObjsImplData d;
	d.ObjFunc = objFunc;
	d.Data = data;
	d.DataSize = dataSize;
	d.GridItemSize = Vec2iNew(TILE_WIDTH + 4, TILE_HEIGHT + 4);
	d.GridSize = Vec2iNew(5, 9);
	d.PageOffset = Vec2iNew(-5, 5);
	return CreateAddMapItemObjsImpl(pos, d);
}
static void CreateAddMapItemSubObjs(UIObject *c, void *vData);
static UIObject *CreateAddMapItemObjsImpl(
	Vec2i pos, CreateAddMapItemObjsImplData data)
{
	UIObject *c = UIObjectCreate(UITYPE_CONTEXT_MENU, 0, pos, Vec2iZero());
	c->OnFocusFunc = CreateAddMapItemSubObjs;
	c->IsDynamicData = true;
	CMALLOC(c->Data, sizeof(CreateAddMapItemObjsImplData));
	memcpy(c->Data, &data, sizeof data);

	return c;
}
static int CountAddMapItemSubObjs(const UIObject *o);
static void CreateAddMapItemSubObjs(UIObject *c, void *vData)
{
	const CreateAddMapItemObjsImplData *data = vData;
	const int pageSize = data->GridSize.x * data->GridSize.y;
	// Check if we need to recreate the objs
	// TODO: this is a very heavyweight way to do it
	int count = 0;
	bool allChildrenSame = true;
	for (int i = 0; i < MapObjectsCount(&gMapObjects); i++)
	{
		MapObject *mo = IndexMapObject(i);
		UIObject *o2 =
			UIObjectCreate(UITYPE_CUSTOM, 0, Vec2iZero(), data->GridItemSize);
		o2->IsDynamicData = true;
		CCALLOC(o2->Data, data->DataSize);
		if (!data->ObjFunc(o2, mo, data->Data))
		{
			UIObjectDestroy(o2);
			continue;
		}
		const int pageIdx = count / pageSize;
		if (pageIdx >= (int)c->Children.size)
		{
			allChildrenSame = false;
			break;
		}
		const UIObject **op = CArrayGet(&c->Children, pageIdx);
		const UIObject **octx = CArrayGet(&(*op)->Children, 0);
		const int idx = count % pageSize;
		if (idx >= (int)(*octx)->Children.size)
		{
			allChildrenSame = false;
			break;
		}
		const UIObject **oc = CArrayGet(&(*octx)->Children, idx);
		if (memcmp(o2->Data, (*oc)->Data, data->DataSize) != 0)
		{
			allChildrenSame = false;
			UIObjectDestroy(o2);
			break;
		}
		count++;
		UIObjectDestroy(o2);
	}
	int cCount = CountAddMapItemSubObjs(c);
	if (cCount == count && allChildrenSame)
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
	CArrayClear(&c->Children);

	// Create pagination
	int pageNum = 1;
	UIObject *pageLabel = NULL;
	UIObject *page = NULL;
	UIObject *o =
		UIObjectCreate(UITYPE_CUSTOM, 0, Vec2iZero(), data->GridItemSize);
	o->ChangesData = true;
	const Vec2i gridStart = Vec2iZero();
	Vec2i pos = Vec2iZero();
	count = 0;
	for (int i = 0; i < MapObjectsCount(&gMapObjects); i++)
	{
		// Only add normal map objects
		MapObject *mo = IndexMapObject(i);
		UIObject *o2 = UIObjectCopy(o);
		o2->IsDynamicData = true;
		CCALLOC(o2->Data, data->DataSize);
		if (!data->ObjFunc(o2, mo, data->Data))
		{
			UIObjectDestroy(o2);
			continue;
		}
		o2->Pos = pos;
		if (count == 0)
		{
			pageLabel = UIObjectCreate(
				UITYPE_LABEL, 0,
				Vec2iNew((pageNum - 1) * 10, 0), Vec2iNew(10, FontH()));
			char buf[32];
			sprintf(buf, "%d", pageNum);
			UIObjectSetDynamicLabel(pageLabel, buf);
			page = UIObjectCreate(
				UITYPE_CONTEXT_MENU, 0,
				Vec2iAdd(data->PageOffset, pageLabel->Size), Vec2iZero());
			UIObjectAddChild(pageLabel, page);
			pageNum++;
		}
		UIObjectAddChild(page, o2);
		pos.x += o->Size.x;
		if (((count + 1) % data->GridSize.x) == 0)
		{
			pos.x = gridStart.x;
			pos.y += o->Size.y;
		}
		count++;
		if (count == pageSize)
		{
			count = 0;
			pos = gridStart;
			UIObjectAddChild(c, pageLabel);
		}
	}
	if (pageLabel != NULL)
	{
		UIObjectAddChild(c, pageLabel);
	}

	UIObjectDestroy(o);
}
static int CountAddMapItemSubObjs(const UIObject *o)
{
	int count = 0;
	CA_FOREACH(const UIObject *, page, o->Children)
		const UIObject **ctx = CArrayGet(&(*page)->Children, 0);
		count += (int)(*ctx)->Children.size;
	CA_FOREACH_END()
	return count;
}

char *MakePlacementFlagTooltip(const MapObject *mo)
{
	// Add a descriptive tooltip for the map object
	char buf[512];
	// Construct text representing the placement flags
	char pfBuf[128];
	if (mo->Flags == 0)
	{
		sprintf(pfBuf, "anywhere\n");
	}
	else
	{
		strcpy(pfBuf, "");
		for (int i = 1; i < PLACEMENT_COUNT; i++)
		{
			if (mo->Flags & (1 << i))
			{
				if (strlen(pfBuf) > 0)
				{
					strcat(pfBuf, ", ");
				}
				strcat(pfBuf, PlacementFlagStr(i));
			}
		}
	}
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
	sprintf(
		buf, "%s\nHealth: %d\nPlacement: %s%s",
		mo->Name, mo->Health, pfBuf, exBuf);
	char *tmp;
	CSTRDUP(tmp, buf);
	return tmp;
}

static void CloseChange(void *data, int d);
void CreateCloseLabel(UIObject *c, const Vec2i pos)
{
	char *closeLabel = "Close";
	const Vec2i closeSize = FontStrSize(closeLabel);
	UIObject *oClose = UIObjectCreate(UITYPE_LABEL, 0, pos, closeSize);
	oClose->Label = closeLabel;
	oClose->ReloadData = true;
	// Have a dummy change func so that the context menu is closed
	oClose->ChangeFunc = CloseChange;
	UIObjectAddChild(c, oClose);
}
static void CloseChange(void *data, int d)
{
	UNUSED(data);
	UNUSED(d);
}
