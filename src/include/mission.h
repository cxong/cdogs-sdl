/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (C) 1995 Ronny Webster
    Copyright (C) 2003 Jeremy Chin 
    Copyright (C) 2003 Lucas Martin-King 

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

-------------------------------------------------------------------------------

 mission.h - <description here>

*/

#define TileItemIsObjective(f)   (((f) & TILEITEM_OBJECTIVE) != 0)
#define ObjectiveFromTileItem(f) ((((f) & TILEITEM_OBJECTIVE) >> OBJECTIVE_SHIFT)-1)
#define ObjectiveToTileItem(o)   (((o)+1) << OBJECTIVE_SHIFT)

struct EditorInfo {
	int itemCount;
	int pickupCount;
	int keyCount;
	int doorCount;
	int exitCount;
	int rangeCount;
};

int SetupBuiltinCampaign(int index);
int SetupBuiltinDogfight(int index);
void ResetCampaign(void);
void SetupMission(int index, int buildTables);
int CheckMissionObjective(int flags);
int MissionCompleted(void);


// Intended for use with the editor only

void SetupMissionCharacter(int index, const TBadGuy * b);
void GetEditorInfo(struct EditorInfo *info);
const char *RangeName(int index);
