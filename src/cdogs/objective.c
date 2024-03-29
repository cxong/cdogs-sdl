/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.
	Copyright (C) 1995 Ronny Wester
	Copyright (C) 2003 Jeremy Chin
	Copyright (C) 2003-2007 Lucas Martin-King

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

	This file incorporates work covered by the following copyright and
	permission notice:

	Copyright (c) 2014, 2016-2017, 2019, 2023 Cong Xu
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
#include "objective.h"

#include "json_utils.h"
#include "utils.h"

const char *ObjectiveTypeStr(const ObjectiveType t)
{
	switch (t)
	{
		T2S(OBJECTIVE_KILL, "Kill");
		T2S(OBJECTIVE_COLLECT, "Collect");
		T2S(OBJECTIVE_DESTROY, "Destroy");
		T2S(OBJECTIVE_RESCUE, "Rescue");
		T2S(OBJECTIVE_INVESTIGATE, "Explore");
	default:
		return "";
	}
}
ObjectiveType StrObjectiveType(const char *s)
{
	S2T(OBJECTIVE_KILL, "Kill");
	S2T(OBJECTIVE_COLLECT, "Collect");
	S2T(OBJECTIVE_DESTROY, "Destroy");
	S2T(OBJECTIVE_RESCUE, "Rescue");
	S2T(OBJECTIVE_INVESTIGATE, "Explore");
	CASSERT(false, "unknown objective name");
	return OBJECTIVE_KILL;
}
color_t ObjectiveTypeColor(const ObjectiveType t)
{
	switch (t)
	{
	case OBJECTIVE_KILL: // fallthrough
	case OBJECTIVE_DESTROY:
		return colorRed;
	case OBJECTIVE_COLLECT: // fallthrough
	case OBJECTIVE_RESCUE:
		return colorGreen;
	case OBJECTIVE_INVESTIGATE:
		return colorCyan;
	default:
		CASSERT(false, "unknown objective type");
		// Shouldn't get here but use a different colour in case
		return colorYellow;
	}
}

void ObjectiveSetPickup(Objective *o, const PickupClass *p)
{
	CASSERT(o->Type == OBJECTIVE_COLLECT, "invalid objective type");
	CArrayTerminate(&o->u.Pickups);
	CArrayInit(&o->u.Pickups, sizeof(const PickupClass *));
	CArrayPushBack(&o->u.Pickups, &p);
}

void ObjectiveSetDestroy(Objective *o, const MapObject *mo)
{
	CASSERT(o->Type == OBJECTIVE_DESTROY, "invalid objective type");
	CArrayTerminate(&o->u.MapObjects);
	CArrayInit(&o->u.MapObjects, sizeof(const MapObject *));
	CArrayPushBack(&o->u.MapObjects, &mo);
}

void ObjectiveLoadJSON(Objective *o, json_t *node, const int version)
{
	memset(o, 0, sizeof *o);
	o->Description = GetString(node, "Description");
	JSON_UTILS_LOAD_ENUM(o->Type, node, "Type", StrObjectiveType);
	if (version < 8)
	{
		// Index numbers used for all objective classes; convert them
		// to their class handles
		int index;
		LoadInt(&index, node, "Index");
		switch (o->Type)
		{
		case OBJECTIVE_COLLECT:
			ObjectiveSetPickup(o, IntPickupClass(index));
			break;
		case OBJECTIVE_DESTROY:
			ObjectiveSetDestroy(o, IntMapObject(index));
			break;
		default:
			o->u.Index = index;
			break;
		}
	}
	else
	{
		char *tmp = NULL;
		switch (o->Type)
		{
		case OBJECTIVE_COLLECT:
			LoadStr(&tmp, node, "Pickup");
			if (tmp)
			{
				ObjectiveSetPickup(o, StrPickupClass(tmp));
				CFREE(tmp);
			}
			else
			{
				CArrayInit(&o->u.Pickups, sizeof(const PickupClass *));
				const json_t *child = json_find_first_label(node, "Pickups");
				if (child && child->child)
				{
					for (child = child->child->child; child; child = child->next)
					{
						const PickupClass *p = StrPickupClass(child->text);
						CASSERT(p != NULL, "Cannot load pickup class");
						CArrayPushBack(&o->u.Pickups, &p);
					}
				}
			}
			break;
		case OBJECTIVE_DESTROY:
			LoadStr(&tmp, node, "MapObject");
			if (tmp)
			{
				ObjectiveSetDestroy(o, StrMapObject(tmp));
				CFREE(tmp);
			}
			else
			{
				CArrayInit(&o->u.MapObjects, sizeof(const MapObject *));
				const json_t *child = json_find_first_label(node, "MapObjects");
				if (child && child->child)
				{
					for (child = child->child->child; child; child = child->next)
					{
						const MapObject *mo = StrMapObject(child->text);
						CASSERT(mo != NULL, "Cannot load map object");
						CArrayPushBack(&o->u.MapObjects, &mo);
					}
				}
			}
			break;
		default:
			LoadInt(&o->u.Index, node, "Index");
			break;
		}
	}
	LoadInt(&o->Count, node, "Count");
	LoadInt(&o->Required, node, "Required");
	LoadInt(&o->Flags, node, "Flags");
	if (version < 14)
	{
		// Classic objective flags: rescue always in locked rooms, kill always
		// anywhere
		if (o->Type == OBJECTIVE_RESCUE)
		{
			o->Flags |= OBJECTIVE_HIACCESS;
			o->Flags &= ~OBJECTIVE_NOACCESS;
		}
		else if (o->Type == OBJECTIVE_KILL)
		{
			o->Flags &= ~(OBJECTIVE_HIACCESS | OBJECTIVE_NOACCESS);
		}
	}
}
void ObjectiveSetup(Objective *o)
{
	// Set objective colours based on type
	o->color = ObjectiveTypeColor(o->Type);
	o->placed = 0;
	o->done = 0;
}
void ObjectiveCopy(Objective *dst, const Objective *src)
{
	memcpy(dst, src, sizeof *dst);
	if (src->Description)
	{
		CSTRDUP(dst->Description, src->Description);
	}
	switch (src->Type)
	{
	case OBJECTIVE_COLLECT:
		memset(&dst->u.Pickups, 0, sizeof(dst->u.Pickups));
		CArrayCopy(&dst->u.Pickups, &src->u.Pickups);
		break;
	case OBJECTIVE_DESTROY:
		memset(&dst->u.MapObjects, 0, sizeof(dst->u.MapObjects));
		CArrayCopy(&dst->u.MapObjects, &src->u.MapObjects);
		break;
	default:
		break;
	}
}
void ObjectiveTerminate(Objective *o)
{
	CFREE(o->Description);
	switch (o->Type)
	{
	case OBJECTIVE_COLLECT:
		CArrayTerminate(&o->u.Pickups);
		break;
	case OBJECTIVE_DESTROY:
		CArrayTerminate(&o->u.MapObjects);
		break;
	default:
		break;
	}
}

bool ObjectiveIsRequired(const Objective *o)
{
	return o->Required > 0;
}
bool ObjectiveIsComplete(const Objective *o)
{
	return o->done >= o->Required;
}
bool ObjectiveIsPerfect(const Objective *o)
{
	// Don't count objectives that are fully required anyway
	return o->done == o->Count && o->done > o->Required;
}

PlacementAccessFlags ObjectiveGetPlacementAccessFlags(const Objective *o)
{
	if (o->Flags & OBJECTIVE_HIACCESS)
	{
		return PLACEMENT_ACCESS_LOCKED;
	}
	if (o->Flags & OBJECTIVE_NOACCESS)
	{
		return PLACEMENT_ACCESS_NOT_LOCKED;
	}
	return PLACEMENT_ACCESS_ANY;
}
