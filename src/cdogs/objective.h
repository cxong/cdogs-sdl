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

    Copyright (c) 2013-2017, 2023 Cong Xu
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

#include "map_object.h"
#include "pickup_class.h"


// Warning: only add to the back of this enum;
// Used by classic maps
typedef enum
{
	OBJECTIVE_KILL,
	OBJECTIVE_COLLECT,
	OBJECTIVE_DESTROY,
	OBJECTIVE_RESCUE,
	OBJECTIVE_INVESTIGATE,
	OBJECTIVE_MAX
} ObjectiveType;
#define OBJECTIVE_MAX_OLD 5
const char *ObjectiveTypeStr(const ObjectiveType t);
ObjectiveType StrObjectiveType(const char *s);
// Use specific colours for objective types
color_t ObjectiveTypeColor(const ObjectiveType t);

// Objective flags
#define OBJECTIVE_HIDDEN        1
#define OBJECTIVE_POSKNOWN      2
#define OBJECTIVE_HIACCESS      4
#define OBJECTIVE_UNKNOWNCOUNT	8
#define OBJECTIVE_NOACCESS		16

typedef struct
{
	char *Description;
	ObjectiveType Type;
	union
	{
		int Index;
		CArray MapObjects;	// of const MapObject *
		CArray Pickups;  // of const PickupClass *
	} u;
	int Count;
	int Required;
	int Flags;
	color_t color;
	int placed;
	int done;
} Objective;

void ObjectiveLoadJSON(Objective *o, json_t *node, const int version);
// Setup the objective counters for mission initialisation
void ObjectiveSetup(Objective *o);
void ObjectiveCopy(Objective *dst, const Objective *src);
void ObjectiveTerminate(Objective *o);

// Legacy helper functions to set a single objective
void ObjectiveSetPickup(Objective *o, const PickupClass *p);
void ObjectiveSetDestroy(Objective *o, const MapObject *mo);

bool ObjectiveIsRequired(const Objective *o);
bool ObjectiveIsComplete(const Objective *o);
bool ObjectiveIsPerfect(const Objective *o);

PlacementAccessFlags ObjectiveGetPlacementAccessFlags(const Objective *o);
