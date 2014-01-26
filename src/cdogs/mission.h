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
#ifndef __MISSION
#define __MISSION

#include "config.h"
#include "sys_config.h"

#define ObjectiveFromTileItem(f) ((((f) & TILEITEM_OBJECTIVE) >> OBJECTIVE_SHIFT)-1)
#define ObjectiveToTileItem(o)   (((o)+1) << OBJECTIVE_SHIFT)

int GetExitCount(void);
int GetKeystyleCount(void);
int GetDoorstyleCount(void);
int GetColorrangeCount(void);

struct EditorInfo {
	int itemCount;
	int pickupCount;
	int keyCount;
	int doorCount;
	int exitCount;
	int rangeCount;
};

typedef enum
{
	MAPTYPE_CLASSIC,
	MAPTYPE_STATIC
} MapType;
const char *MapTypeStr(MapType t);
MapType StrMapType(const char *s);

typedef struct
{
	char *Description;
	ObjectiveType Type;
	int Index;
	int Count;
	int Required;
	int Flags;
} MissionObjective;

typedef struct
{
	char *Title;
	char *Description;
	MapType Type;
	Vec2i Size;

	// styles
	int WallStyle;
	int FloorStyle;
	int RoomStyle;
	int ExitStyle;
	int KeyStyle;
	int DoorStyle;

	CArray Objectives;		// of MissionObjective
	CArray Enemies;			// of int (character index)
	CArray SpecialChars;	// of int
	CArray Items;			// of int
	CArray ItemDensities;	// of int

	int EnemyDensity;
	int Weapons[GUN_COUNT];

	char Song[CDOGS_PATH_MAX];

	// Colour ranges
	int WallColor;
	int FloorColor;
	int RoomColor;
	int AltColor;

	union
	{
		// Classic
		struct
		{
			int Walls;
			int WallLength;
			int CorridorWidth;
			struct
			{
				int Count;
				int Min;
				int Max;
				int Edge;
				int Overlap;
				int Walls;
				int WallLength;
				int WallPad;
			} Rooms;
			int Squares;
			struct
			{
				int Enabled;
				int Min;
				int Max;
			} Doors;
			struct
			{
				int Count;
				int Min;
				int Max;
			} Pillars;
		} Classic;
		// Static
		struct
		{
			CArray Tiles;	// of unsigned short (map tile)
			Vec2i Start;
		} Static;
	} u;
} Mission;

struct MissionOptions
{
	int index;
	int flags;

	Mission *missionData;
	CArray Objectives;	// of struct Objective
	int exitLeft, exitTop, exitRight, exitBottom;
	int pickupTime;

	CArray MapObjects;	// of TMapObject
	int *keyPics;
	struct DoorPic *doorPics;
	int exitPic, exitShadow;
};

void MissionInit(Mission *m);
void MissionCopy(Mission *dst, Mission *src);
void MissionTerminate(Mission *m);

int SetupBuiltinCampaign(int index);
int SetupBuiltinDogfight(int index);
void SetupMission(
	int buildTables, Mission *m, struct MissionOptions *mo, int missionIndex);
void SetPaletteRanges(int wall_range, int floor_range, int room_range, int alt_range);

void MissionSetMessageIfComplete(struct MissionOptions *options);
// If object is a mission objective, complete it and return true
int CheckMissionObjective(
	struct MissionOptions *options, int flags, ObjectiveType type);
int CanCompleteMission(struct MissionOptions *options);
int IsMissionComplete(struct MissionOptions *options);


// Intended for use with the editor only

struct EditorInfo GetEditorInfo(void);
const char *RangeName(int index);

#endif
