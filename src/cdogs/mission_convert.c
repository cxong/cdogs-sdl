/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.
	Copyright (c) 2014-2021 Cong Xu
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

static void CopyMissionTileClasses(MissionTileClasses *mtc, Mission *m);
void MissionConvertToType(Mission *m, Map *map, MapType type)
{
	MissionTileClasses mtc;
	RoomParams rooms;
	bool copyRooms = false;
	DoorParams doors;
	bool copyDoors = false;
	PillarParams pillars;
	bool copyPillars = false;
	switch (m->Type)
	{
		case MAPTYPE_CLASSIC:
			memcpy(&rooms, &m->u.Classic.Rooms, sizeof rooms);
			copyRooms = true;
			memcpy(&doors, &m->u.Classic.Doors, sizeof doors);
			copyDoors = true;
			memcpy(&pillars, &m->u.Classic.Pillars, sizeof pillars);
			copyPillars = true;
			CopyMissionTileClasses(&mtc, m);
			break;
		case MAPTYPE_CAVE:
			memcpy(&rooms, &m->u.Cave.Rooms, sizeof rooms);
			copyRooms = true;
			CopyMissionTileClasses(&mtc, m);
			break;
		case MAPTYPE_INTERIOR:
			memcpy(&rooms, &m->u.Classic.Rooms, sizeof rooms);
			copyRooms = true;
			memcpy(&doors, &m->u.Classic.Doors, sizeof doors);
			copyDoors = true;
			memcpy(&pillars, &m->u.Classic.Pillars, sizeof pillars);
			copyPillars = true;
			CopyMissionTileClasses(&mtc, m);
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
		if (copyRooms)
		{
			memcpy(&m->u.Classic.Rooms, &rooms, sizeof rooms);
			m->u.Classic.Rooms.Min = MAX(5, m->u.Classic.Rooms.Min);
			m->u.Classic.Rooms.Max = MAX(m->u.Classic.Rooms.Min, m->u.Classic.Rooms.Max);
		}
		else
		{
			m->u.Classic.Rooms.Count = 10;
			m->u.Classic.Rooms.Min = 5;
			m->u.Classic.Rooms.Max = 8;
			m->u.Classic.Rooms.Edge = true;
			m->u.Classic.Rooms.Overlap = true;
			m->u.Classic.Rooms.Walls = 1;
			m->u.Classic.Rooms.WallLength = 1;
			m->u.Classic.Rooms.WallPad = 1;
		}
		m->u.Classic.Squares = 1;
		m->u.Classic.ExitEnabled = true;
		if (copyDoors)
		{
			memcpy(&m->u.Classic.Doors, &doors, sizeof doors);
			m->u.Classic.Doors.Min = MAX(1, m->u.Classic.Doors.Min);
			m->u.Classic.Doors.Max = MAX(m->u.Classic.Doors.Min, m->u.Classic.Doors.Max);
		}
		else
		{
			m->u.Classic.Doors.Enabled = true;
			m->u.Classic.Doors.Min = 1;
			m->u.Classic.Doors.Max = 2;
		}
		if (copyPillars)
		{
			memcpy(&m->u.Classic.Pillars, &pillars, sizeof pillars);
			m->u.Classic.Pillars.Min = MAX(1, m->u.Classic.Pillars.Min);
			m->u.Classic.Pillars.Max = MAX(m->u.Classic.Pillars.Min, m->u.Classic.Pillars.Max);
		}
		else
		{
			m->u.Classic.Pillars.Count = 1;
			m->u.Classic.Pillars.Min = 2;
			m->u.Classic.Pillars.Max = 3;
		}
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
		if (copyRooms)
		{
			memcpy(&m->u.Cave.Rooms, &rooms, sizeof rooms);
		}
		else
		{
			m->u.Cave.Rooms.Count = 10;
			m->u.Cave.Rooms.Min = 5;
			m->u.Cave.Rooms.Max = 8;
			m->u.Cave.Rooms.Edge = true;
			m->u.Cave.Rooms.Overlap = true;
			m->u.Cave.Rooms.Walls = 1;
			m->u.Cave.Rooms.WallLength = 1;
			m->u.Cave.Rooms.WallPad = 1;
		}
		m->u.Cave.Squares = 1;
		m->u.Cave.ExitEnabled = true;
		memcpy(&m->u.Cave.TileClasses, &mtc, sizeof mtc);
		break;
	case MAPTYPE_INTERIOR:
		// Setup default parameters
		m->u.Interior.CorridorWidth = 2;
		if (copyRooms)
		{
			memcpy(&m->u.Interior.Rooms, &rooms, sizeof rooms);
			m->u.Interior.Rooms.Min = MAX(1, m->u.Interior.Rooms.Min);
			m->u.Interior.Rooms.Max = MAX(2 * m->u.Interior.Rooms.Min, m->u.Interior.Rooms.Max);
		}
		else
		{
			m->u.Interior.Rooms.Min = 5;
			m->u.Interior.Rooms.Max = 10;
			m->u.Interior.Rooms.Walls = 1;
			m->u.Interior.Rooms.WallLength = 1;
			m->u.Interior.Rooms.WallPad = 1;
		}
		m->u.Interior.ExitEnabled = true;
		if (copyDoors)
		{
			memcpy(&m->u.Interior.Doors, &doors, sizeof doors);
			m->u.Interior.Doors.Min = MAX(1, m->u.Interior.Doors.Min);
			m->u.Interior.Doors.Max = MAX(m->u.Interior.Doors.Min, m->u.Interior.Doors.Max);
		}
		else
		{
			m->u.Interior.Doors.Enabled = true;
			m->u.Interior.Doors.Min = 1;
			m->u.Interior.Doors.Max = 2;
		}
		if (copyPillars)
		{
			memcpy(&m->u.Interior.Pillars, &pillars, sizeof pillars);
		}
		else
		{
			m->u.Interior.Pillars.Count = 1;
		}
		memcpy(&m->u.Interior.TileClasses, &mtc, sizeof mtc);
		break;
	default:
		CASSERT(false, "unknown map type");
		break;
	}
	m->Type = type;
}
static void CopyMissionTileClasses(MissionTileClasses *mtc, Mission *m)
{
	MissionTileClasses *mtcOrig = MissionGetTileClasses(m);
	MissionTileClassesCopy(mtc, mtcOrig);
	MissionTileClassesTerminate(mtcOrig);
}
