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
#include "editor_ui_objectives.h"

#include <cdogs/draw.h>
#include <cdogs/font.h>

#include "editor_ui.h"


typedef struct
{
	CampaignOptions *Campaign;
	int MissionObjectiveIndex;
} MissionObjectiveData;
static char *MissionGetObjectiveDescription(UIObject *o, void *data)
{
	MissionObjectiveData *mData = data;
	Mission *m = CampaignGetCurrentMission(mData->Campaign);
	if (!m)
	{
		return NULL;
	}
	int i = mData->MissionObjectiveIndex;
	if ((int)m->Objectives.size <= i)
	{
		if (i == 0)
		{
			// first objective and mission has no objectives
			o->u.Textbox.IsEditable = false;
			return "-- mission objectives --";
		}
		return NULL;
	}
	o->u.Textbox.IsEditable = true;
	return ((const Objective *)CArrayGet(&m->Objectives, i))->Description;
}
static void MissionCheckObjectiveDescription(UIObject *o, void *data)
{
	MissionObjectiveData *mData = data;
	Mission *m = CampaignGetCurrentMission(mData->Campaign);
	if (!m)
	{
		o->IsVisible = false;
		return;
	}
	int i = mData->MissionObjectiveIndex;
	if ((int)m->Objectives.size <= i)
	{
		if (i == 0)
		{
			// first objective and mission has no objectives
			o->IsVisible = true;
			return;
		}
		o->IsVisible = false;
		return;
	}
	o->IsVisible = true;
}
static char **MissionGetObjectiveDescriptionSrc(void *data)
{
	MissionObjectiveData *mData = data;
	Mission *m = CampaignGetCurrentMission(mData->Campaign);
	if (!m)
	{
		return NULL;
	}
	int i = mData->MissionObjectiveIndex;
	if ((int)m->Objectives.size <= i)
	{
		return NULL;
	}
	return &((Objective *)CArrayGet(&m->Objectives, i))->Description;
}
typedef struct
{
	CampaignOptions *co;
	int index;
} MissionIndexData;
static const char *MissionGetObjectiveStr(UIObject *o, void *vData)
{
	UNUSED(o);
	MissionIndexData *data = vData;
	if (!CampaignGetCurrentMission(data->co)) return NULL;
	if ((int)CampaignGetCurrentMission(data->co)->Objectives.size <= data->index) return NULL;
	return ObjectiveTypeStr(((const Objective *)CArrayGet(
		&CampaignGetCurrentMission(data->co)->Objectives, data->index))->Type);
}
static void MissionDrawObjective(
	UIObject *o, GraphicsDevice *g, Vec2i pos, void *vData)
{
	MissionIndexData *data = vData;
	CharacterStore *store = &data->co->Setting.characters;
	const Character *c = NULL;
	UNUSED(g);
	const Mission *m = CampaignGetCurrentMission(data->co);
	if (m == NULL) return;
	if ((int)m->Objectives.size <= data->index) return;
	// TODO: only one kill and rescue objective allowed
	const Objective *obj = CArrayGet(&m->Objectives, data->index);
	const Pic *newPic = NULL;
	switch (obj->Type)
	{
	case OBJECTIVE_KILL:
		if (store->specialIds.size > 0)
		{
			c = CArrayGet(
				&store->OtherChars, CharacterStoreGetSpecialId(store, 0));
		}
		break;
	case OBJECTIVE_RESCUE:
		if (store->prisonerIds.size > 0)
		{
			c = CArrayGet(
				&store->OtherChars, CharacterStoreGetPrisonerId(store, 0));
		}
		break;
	case OBJECTIVE_COLLECT:
		newPic = obj->u.Pickup->Pic;
		break;
	case OBJECTIVE_DESTROY:
		newPic = obj->u.MapObject->Normal.Pic;
		break;
	case OBJECTIVE_INVESTIGATE:
		// no picture
		break;
	default:
		assert(0 && "Unknown objective type");
		return;
	}
	const Vec2i drawPos =
		Vec2iAdd(Vec2iAdd(pos, o->Pos), Vec2iScaleDiv(o->Size, 2));
	if (c != NULL)
	{
		DrawHead(c, DIRECTION_DOWN, STATE_IDLE, drawPos);
	}
	else if (newPic != NULL)
	{
		Blit(g, newPic, Vec2iMinus(drawPos, Vec2iScaleDiv(newPic->size, 2)));
	}
}
static Objective *GetMissionObjective(const Mission *m, const int idx)
{
	return CArrayGet(&m->Objectives, idx);
}
static const char *MissionGetObjectiveRequired(UIObject *o, void *vData)
{
	static char s[128];
	UNUSED(o);
	MissionIndexData *data = vData;
	if (!CampaignGetCurrentMission(data->co)) return NULL;
	if ((int)CampaignGetCurrentMission(data->co)->Objectives.size <=
		data->index)
	{
		return NULL;
	}
	sprintf(s, "%d", GetMissionObjective(
		CampaignGetCurrentMission(data->co), data->index)->Required);
	return s;
}
static const char *MissionGetObjectiveTotal(UIObject *o, void *vData)
{
	static char s[128];
	UNUSED(o);
	MissionIndexData *data = vData;
	if (!CampaignGetCurrentMission(data->co)) return NULL;
	if ((int)CampaignGetCurrentMission(data->co)->Objectives.size <=
		data->index)
	{
		return NULL;
	}
	sprintf(s, "out of %d", GetMissionObjective(
		CampaignGetCurrentMission(data->co), data->index)->Count);
	return s;
}
static const char *MissionGetObjectiveFlags(UIObject *o, void *vData)
{
	int flags;
	static char s[128];
	UNUSED(o);
	MissionIndexData *data = vData;
	if (!CampaignGetCurrentMission(data->co)) return NULL;
	if ((int)CampaignGetCurrentMission(data->co)->Objectives.size <=
		data->index)
	{
		return NULL;
	}
	flags = GetMissionObjective(
		CampaignGetCurrentMission(data->co), data->index)->Flags;
	if (!flags)
	{
		return "(normal)";
	}
	sprintf(s, "%s %s %s %s %s",
		(flags & OBJECTIVE_HIDDEN) ? "hidden" : "",
		(flags & OBJECTIVE_POSKNOWN) ? "pos.known" : "",
		(flags & OBJECTIVE_HIACCESS) ? "access" : "",
		(flags & OBJECTIVE_UNKNOWNCOUNT) ? "no-count" : "",
		(flags & OBJECTIVE_NOACCESS) ? "no-access" : "");
	return s;
}

typedef struct
{
	CampaignOptions *C;
	int ObjectiveIdx;
	ObjectiveType Type;
} ObjectiveChangeTypeData;
static void MissionChangeObjectiveIndex(void *vData, int d);
static void ObjectiveChangeType(void *vData, int d)
{
	UNUSED(d);
	ObjectiveChangeTypeData *data = vData;
	Objective *o = GetMissionObjective(
		CampaignGetCurrentMission(data->C), data->ObjectiveIdx);
	if (o->Type == data->Type)
	{
		return;
	}
	o->Type = data->Type;
	// Initialise the index/handle of the objective
	memset(&o->u, 0, sizeof o->u);
	MissionChangeObjectiveIndex(data, 0);
}
static void MissionChangeObjectiveIndex(void *vData, int d)
{
	MissionIndexData *data = vData;
	Objective *o = GetMissionObjective(
		CampaignGetCurrentMission(data->co), data->index);
	int idx;
	int limit;
	switch (o->Type)
	{
	case OBJECTIVE_COLLECT:
		limit = PickupClassesGetScoreCount(&gPickupClasses) - 1;
		idx = PickupClassesGetScoreIdx(o->u.Pickup);
		break;
	case OBJECTIVE_DESTROY:
		limit = (int)gMapObjects.Destructibles.size - 1;
		idx = DestructibleMapObjectIndex(o->u.MapObject);
		break;
	case OBJECTIVE_KILL:
	case OBJECTIVE_INVESTIGATE:
		limit = 0;
		idx = 0;
		break;
	case OBJECTIVE_RESCUE:
		limit = data->co->Setting.characters.OtherChars.size - 1;
		idx = o->u.Index;
		break;
	default:
		assert(0 && "Unknown objective type");
		return;
	}
	idx = CLAMP_OPPOSITE(idx + d, 0, limit);
	switch (o->Type)
	{
	case OBJECTIVE_COLLECT:
		o->u.Pickup = IntScorePickupClass(idx);
		break;
	case OBJECTIVE_DESTROY:
		{
			const char **destructibleName =
				CArrayGet(&gMapObjects.Destructibles, idx);
			o->u.MapObject = StrMapObject(*destructibleName);
		}
		CASSERT(o->u.MapObject != NULL, "cannot find map object");
		break;
	default:
		o->u.Index = idx;
		break;
	}
}
static void MissionChangeObjectiveRequired(void *vData, int d)
{
	MissionIndexData *data = vData;
	Objective *o = GetMissionObjective(
		CampaignGetCurrentMission(data->co), data->index);
	o->Required = CLAMP_OPPOSITE(o->Required + d, 0, MIN(100, o->Count));
}
static void MissionChangeObjectiveTotal(void *vData, int d)
{
	MissionIndexData *data = vData;
	const Mission *m = CampaignGetCurrentMission(data->co);
	Objective *o = GetMissionObjective(m, data->index);
	o->Count = CLAMP_OPPOSITE(o->Count + d, o->Required, 100);
	// Don't let the total reduce to less than static ones we've placed
	if (m->Type == MAPTYPE_STATIC)
	{
		CA_FOREACH(const ObjectivePositions, op, m->u.Static.Objectives)
			if (op->Index == data->index)
			{
				o->Count = MAX(o->Count, (int)op->Positions.size);
				break;
			}
		CA_FOREACH_END()
	}
}
static void MissionChangeObjectiveFlags(void *vData, int d)
{
	MissionIndexData *data = vData;
	Objective *o = GetMissionObjective(
		CampaignGetCurrentMission(data->co), data->index);
	// Max is combination of all flags, i.e. largest flag doubled less one
	o->Flags = CLAMP_OPPOSITE(o->Flags + d, 0, OBJECTIVE_NOACCESS * 2 - 1);
}


static UIObject *CreateObjectiveObjs(
	Vec2i pos, CampaignOptions *co, const int idx);
void CreateObjectivesObjs(CampaignOptions *co, UIObject *c, Vec2i pos)
{
	const int th = FontH();
	Vec2i objectivesPos = Vec2iNew(0, 7 * th);
	UIObject *o =
		UIObjectCreate(UITYPE_TEXTBOX, 0, Vec2iZero(), Vec2iNew(300, th));
	o->Flags = UI_SELECT_ONLY;

	for (int i = 0; i < OBJECTIVE_MAX_OLD; i++)
	{
		UIObject *o2 = UIObjectCopy(o);
		o2->Id = YC_OBJECTIVES + i;
		o2->Type = UITYPE_TEXTBOX;
		o2->u.Textbox.TextLinkFunc = MissionGetObjectiveDescription;
		o2->u.Textbox.TextSourceFunc = MissionGetObjectiveDescriptionSrc;
		o2->IsDynamicData = 1;
		CMALLOC(o2->Data, sizeof(MissionObjectiveData));
		((MissionObjectiveData *)o2->Data)->Campaign = co;
		((MissionObjectiveData *)o2->Data)->MissionObjectiveIndex = i;
		CSTRDUP(o2->u.Textbox.Hint, "(Objective description)");
		o2->Pos = pos;
		CSTRDUP(
			o2->Tooltip,
			"Insert/CTRL-i, Delete/CTRL+d: add/remove objective");
		o2->CheckVisible = MissionCheckObjectiveDescription;
		UIObjectAddChild(o2, CreateObjectiveObjs(objectivesPos, co, i));
		UIObjectAddChild(c, o2);
		pos.y += th;
	}
	UIObjectDestroy(o);
}
static UIObject *CreateObjectiveObjs(
	Vec2i pos, CampaignOptions *co, const int idx)
{
	const int th = FontH();
	UIObject *c;
	UIObject *o;
	UIObject *o2;
	c = UIObjectCreate(UITYPE_NONE, 0, Vec2iZero(), Vec2iZero());
	c->Flags = UI_ENABLED_WHEN_PARENT_HIGHLIGHTED_ONLY;

	o = UIObjectCreate(UITYPE_LABEL, 0, Vec2iZero(), Vec2iZero());
	o->Flags = UI_LEAVE_YC;
	o->ChangesData = 1;

	pos.y -= idx * th;
	// Drop-down menu for objective type
	o2 = UIObjectCopy(o);
	o2->Size = Vec2iNew(35, th);
	o2->u.LabelFunc = MissionGetObjectiveStr;
	CMALLOC(o2->Data, sizeof(MissionIndexData));
	o2->IsDynamicData = true;
	((MissionIndexData *)o2->Data)->co = co;
	((MissionIndexData *)o2->Data)->index = idx;
	o2->Pos = pos;
	CSTRDUP(o2->Tooltip, "Objective type");
	UIObject *oObjType =
		UIObjectCreate(UITYPE_CONTEXT_MENU, 0, Vec2iZero(), Vec2iZero());
	for (int i = 0; i < (int)OBJECTIVE_MAX; i++)
	{
		UIObject *oObjTypeChild =
			UIObjectCreate(UITYPE_LABEL, 0, Vec2iZero(), Vec2iNew(50, th));
		oObjTypeChild->ChangesData = true;
		oObjTypeChild->Pos.y = i * th;
		oObjTypeChild->Label = ObjectiveTypeStr((ObjectiveType)i);
		oObjTypeChild->IsDynamicData = true;
		CMALLOC(oObjTypeChild->Data, sizeof(ObjectiveChangeTypeData));
		ObjectiveChangeTypeData *octd = oObjTypeChild->Data;
		octd->C = co;
		octd->ObjectiveIdx = idx;
		octd->Type = (ObjectiveType)i;
		oObjTypeChild->ChangeFunc = ObjectiveChangeType;
		UIObjectAddChild(oObjType, oObjTypeChild);
	}
	UIObjectAddChild(o2, oObjType);
	UIObjectAddChild(c, o2);

	pos.x += 40;
	// Choose objective object/item
	// TODO: context menu
	o2 = UIObjectCopy(o);
	o2->Type = UITYPE_CUSTOM;
	o2->u.CustomDrawFunc = MissionDrawObjective;
	o2->ChangeFunc = MissionChangeObjectiveIndex;
	CMALLOC(o2->Data, sizeof(MissionIndexData));
	o2->IsDynamicData = 1;
	((MissionIndexData *)o2->Data)->co = co;
	((MissionIndexData *)o2->Data)->index = idx;
	o2->Pos = pos;
	o2->Size = Vec2iNew(30, th);
	UIObjectAddChild(c, o2);
	pos.x += 30;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = MissionGetObjectiveRequired;
	o2->ChangeFunc = MissionChangeObjectiveRequired;
	CMALLOC(o2->Data, sizeof(MissionIndexData));
	o2->IsDynamicData = 1;
	((MissionIndexData *)o2->Data)->co = co;
	((MissionIndexData *)o2->Data)->index = idx;
	o2->Pos = pos;
	o2->Size = Vec2iNew(20, th);
	CSTRDUP(o2->Tooltip, "0: optional objective");
	UIObjectAddChild(c, o2);
	pos.x += 20;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = MissionGetObjectiveTotal;
	o2->ChangeFunc = MissionChangeObjectiveTotal;
	CMALLOC(o2->Data, sizeof(MissionIndexData));
	o2->IsDynamicData = 1;
	((MissionIndexData *)o2->Data)->co = co;
	((MissionIndexData *)o2->Data)->index = idx;
	o2->Pos = pos;
	o2->Size = Vec2iNew(40, th);
	UIObjectAddChild(c, o2);
	pos.x += 45;
	o2 = UIObjectCopy(o);
	o2->u.LabelFunc = MissionGetObjectiveFlags;
	o2->ChangeFunc = MissionChangeObjectiveFlags;
	CMALLOC(o2->Data, sizeof(MissionIndexData));
	o2->IsDynamicData = 1;
	((MissionIndexData *)o2->Data)->co = co;
	((MissionIndexData *)o2->Data)->index = idx;
	o2->Pos = pos;
	o2->Size = Vec2iNew(100, th);
	CSTRDUP(o2->Tooltip,
		"hidden: not shown on map\n"
		"pos.known: always shown on map\n"
		"access: in locked room\n"
		"no-count: don't show completed count\n"
		"no-access: not in locked rooms");
	UIObjectAddChild(c, o2);

	UIObjectDestroy(o);
	return c;
}
