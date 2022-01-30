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

	Copyright (c) 2013-2017, 2019-2021 Cong Xu
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

#include <stdbool.h>

#include "config.h"
#include "map_object.h"
#include "mission_static.h"
#include "objective.h"
#include "proto/msg.pb.h"
#include "sys_config.h"

#define ObjectiveFromThing(f) ((((f)&THING_OBJECTIVE) >> OBJECTIVE_SHIFT) - 1)
#define ObjectiveToThing(o) (((o) + 1) << OBJECTIVE_SHIFT)

#define KEY_COUNT 4

typedef enum
{
	MAPTYPE_CLASSIC,
	MAPTYPE_STATIC,
	MAPTYPE_CAVE,
	MAPTYPE_INTERIOR,
	MAPTYPE_COUNT
} MapType;
const char *MapTypeStr(MapType t);
MapType StrMapType(const char *s);

// Keys that have been collected in this level
// Applies to all players
#define FLAGS_KEYCARD_YELLOW 0x1
#define FLAGS_KEYCARD_GREEN 0x2
#define FLAGS_KEYCARD_BLUE 0x4
#define FLAGS_KEYCARD_RED 0x8
int StrKeycard(const char *s);
color_t KeyColor(const int flags);

typedef struct
{
	TileClass Wall;
	TileClass Floor;
	TileClass Room;
	TileClass Door;
} MissionTileClasses;
typedef struct
{
	const MapObject *M;
	int Density;
} MapObjectDensity;
typedef struct
{
	int Count;
	int Min;
	int Max;
	bool Edge;
	bool Overlap;
	int Walls;
	int WallLength;
	int WallPad;
} RoomParams;
typedef struct
{
	bool Enabled;
	int Min;
	int Max;
	bool RandomPos;
} DoorParams;
typedef struct
{
	int Count;
	int Min;
	int Max;
} PillarParams;
typedef struct
{
	char *Title;
	char *Description;
	MapType Type;
	struct vec2i Size;

	// styles
	char ExitStyle[CDOGS_FILENAME_MAX];
	char KeyStyle[CDOGS_FILENAME_MAX];

	CArray Objectives;		   // of Objective
	CArray Enemies;			   // of int (character index)
	CArray SpecialChars;	   // of int
	CArray MapObjectDensities; // of MapObjectDensity

	int EnemyDensity;
	CArray Weapons; // of WeaponClass *

	struct
	{
		MusicSourceType Type;
		union {
			char *Filename;
			MusicChunk Chunk;
		} Data;
	} Music;
	
	bool WeaponPersist;
	bool SkipDebrief;

	union {
		// Classic
		struct
		{
			// TODO: multiple tile classes
			MissionTileClasses TileClasses;
			int Walls;
			int WallLength;
			int CorridorWidth;
			RoomParams Rooms;
			int Squares;
			bool ExitEnabled;
			DoorParams Doors;
			PillarParams Pillars;
		} Classic;
		MissionStatic Static;
		// Cave
		struct
		{
			// TODO: multiple tile classes
			MissionTileClasses TileClasses;
			int FillPercent;
			int Repeat;
			int R1;
			int R2;
			int CorridorWidth;
			RoomParams Rooms;
			int Squares;
			bool ExitEnabled;
			bool DoorsEnabled;
		} Cave;
		struct
		{
			// TODO: multiple tile classes
			MissionTileClasses TileClasses;
			int CorridorWidth;
			RoomParams Rooms;
			bool ExitEnabled;
			DoorParams Doors;
			PillarParams Pillars;
		} Interior;
	} u;
} Mission;

typedef enum
{
	MISSION_STATE_WAITING,
	MISSION_STATE_PLAY,
	MISSION_STATE_PICKUP
} MissionState;

struct MissionOptions
{
	int index;
	int KeyFlags;

	Mission *missionData;
	CArray Weapons; // of WeaponClass *
	int time;
	// Time when players first entered pickup area
	int pickupTime;
	MissionState state;
	// Whether the mission has loaded
	bool HasStarted;
	// Whether the mission has begun (can complete objectives etc.)
	bool HasBegun;
	bool isDone;
	int DoneCounter;
	int NextMission;
	bool MissionCompleted;
};

void MissionInit(Mission *m);
void MissionCopy(Mission *dst, const Mission *src);
void MissionTerminate(Mission *m);

MissionTileClasses *MissionGetTileClasses(Mission *m);

void SetupMission(Mission *m, struct MissionOptions *mo, int missionIndex);
void MissionSetupTileClasses(Map *m, PicManager *pm, const MissionTileClasses *mtc);
void MissionTileClassesInitDefault(MissionTileClasses *mtc);
void MissionTileClassesCopy(
	MissionTileClasses *dst, const MissionTileClasses *src);
void MissionTileClassesTerminate(MissionTileClasses *mtc);

void MissionSetMessageIfComplete(struct MissionOptions *options);
// If object is a mission objective, send an update event
void UpdateMissionObjective(
	const struct MissionOptions *options, const int flags,
	const ObjectiveType type, const int count);
bool MissionCanBegin(void);
void MissionBegin(struct MissionOptions *m, const NGameBegin gb);
bool CanCompleteMission(const struct MissionOptions *options);
bool MissionAllObjectivesComplete(const struct MissionOptions *mo);
bool IsMissionComplete(const struct MissionOptions *mo);
// Returns exit index or -1 if not all players in same exit
int AllSurvivingPlayersInSameExit(void);
bool MissionNeedsMoreRescuesInExit(const struct MissionOptions *mo);
bool MissionHasRequiredObjectives(const struct MissionOptions *mo);
void MissionDone(struct MissionOptions *mo, const NMissionEnd end);

// Count the number of keys in the flags
int KeycardCount(int flags);

void MissionStaticAddObjective(
	Mission *m, MissionStatic *ms, const int idx, const int idx2,
	const struct vec2i pos, const bool force);
bool MissionStaticTryRemoveObjective(
	Mission *m, MissionStatic *ms, const struct vec2i pos);
