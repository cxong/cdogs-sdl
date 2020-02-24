/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.
	Copyright (c) 2014, 2016, 2020 Cong Xu
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
#pragma once

#include <cdogs/campaigns.h>
#include <cdogs/map_object.h>
#include <cdogs/vector.h>

#include "editor_brush.h"
#include "ui_object.h"

typedef struct
{
	EditorBrush *Brush;
	union
	{
		int ItemIndex;
		MapObject *MapObject;
	} u;
	int Index2;
} IndexedEditorBrush;
typedef struct
{
	IndexedEditorBrush Brush;
	CampaignOptions *Campaign;
} EditorBrushAndCampaign;

void DisplayMapItem(const struct vec2i pos, const MapObject *mo);
void DisplayMapItemWithDensity(
	const struct vec2i pos, const MapObjectDensity *mod, const bool isHighlighted);
void DrawKey(UIObject *o, GraphicsDevice *g, struct vec2i pos, void *vData);

void InsertMission(CampaignOptions *co, Mission *mission, int idx);
void DeleteMission(CampaignOptions *co);

bool ConfirmScreen(const char *info, const char *msg);
void ClearScreen(GraphicsDevice *g);

void DisplayFlag(
	const struct vec2i pos, const char *s, const bool isOn, const bool isHighlighted);

UIObject *CreateCampaignSeedObj(const struct vec2i pos, CampaignOptions *co);
UIObject *CreateAddMapItemObjs(
	const struct vec2i pos, bool (*objFunc)(UIObject *, MapObject *, void *),
	void *data, const size_t dataSize, const bool expandDown);
UIObject *CreateAddPickupSpawnerObjs(
	const struct vec2i pos, bool (*objFunc)(UIObject *, MapObject *, void *),
	void *data, const size_t dataSize);

char *MakePlacementFlagTooltip(const MapObject *mo);

// Create a dummy label that can be clicked to close the context menu
void CreateCloseLabel(UIObject *c, const struct vec2i pos);

// Macro for creating the helper function to show/hide controls for specific
// map types
#define MISSION_CHECK_TYPE_FUNC(_type)\
static void MissionCheckTypeFunc(UIObject *o, void *data)\
{\
	CampaignOptions *co = data;\
	const Mission *m = CampaignGetCurrentMission(co);\
	if (!m || m->Type != (_type))\
	{\
		o->IsVisible = false;\
		/* Need to unhighlight to prevent children being drawn*/\
		UIObjectUnhighlight(o, false);\
		return;\
	}\
	o->IsVisible = true;\
}

void TileClassGetBrushName(char *buf, const TileClass *tc);
// Get a null-separated string; for nk_combo_separator
char *GetClassNames(const int len, const char *(*indexNameFunc)(const int));
