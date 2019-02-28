/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (c) 2014-2019 Cong Xu
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
#include "mission_convert.h"

#include <assert.h>

#include "algorithms.h"
#include "map_build.h"


void MissionConvertToType(Mission *m, Map *map, MapType type)
{
	MissionTileClasses mtc;
	switch (m->Type)
	{
		case MAPTYPE_CLASSIC:
		case MAPTYPE_CAVE:	// fallthrough
		{
			MissionTileClasses *mtcOrig = MissionGetTileClasses(m);
			MissionTileClassesCopy(&mtc, mtcOrig);
			MissionTileClassesTerminate(mtcOrig);
		}
		break;
		default:
			MissionTileClassesInitDefault(&mtc);
			break;
	}

	memset(&m->u, 0, sizeof m->u);
	switch (type)
	{
	case MAPTYPE_CLASSIC:
		// Setup default parameters
		m->u.Classic.Walls = 10;
		m->u.Classic.WallLength = 5;
		m->u.Classic.CorridorWidth = 2;
		m->u.Classic.Rooms.Count = 10;
		m->u.Classic.Rooms.Min = 5;
		m->u.Classic.Rooms.Max = 8;;
		m->u.Classic.Rooms.Edge = true;
		m->u.Classic.Rooms.Overlap = true;
		m->u.Classic.Rooms.Walls = 1;
		m->u.Classic.Rooms.WallLength = 1;
		m->u.Classic.Rooms.WallPad = 1;
		m->u.Classic.Squares = 1;
		m->u.Classic.Doors.Enabled = true;
		m->u.Classic.Doors.Min = 1;
		m->u.Classic.Doors.Max = 2;
		m->u.Classic.Pillars.Count = 1;
		m->u.Classic.Pillars.Min = 2;
		m->u.Classic.Pillars.Max = 3;
		memcpy(&m->u.Classic.TileClasses, &mtc, sizeof mtc);
		break;
	case MAPTYPE_STATIC:
		MissionStaticFromMap(&m->u.Static, map);
		break;
	case MAPTYPE_CAVE:
		// Setup default parameters
		m->u.Cave.FillPercent = 40;
		m->u.Cave.Repeat = 4;
		m->u.Cave.R1 = 5;
		m->u.Cave.R2 = 2;
		m->u.Cave.CorridorWidth = 2;
		m->u.Cave.Rooms.Count = 10;
		m->u.Cave.Rooms.Min = 5;
		m->u.Cave.Rooms.Max = 8;;
		m->u.Cave.Rooms.Edge = true;
		m->u.Cave.Rooms.Overlap = true;
		m->u.Cave.Rooms.Walls = 1;
		m->u.Cave.Rooms.WallLength = 1;
		m->u.Cave.Rooms.WallPad = 1;
		m->u.Cave.Squares = 1;
		memcpy(&m->u.Cave.TileClasses, &mtc, sizeof mtc);
		break;
	default:
		CASSERT(false, "unknown map type");
		break;
	}
	m->Type = type;
}
