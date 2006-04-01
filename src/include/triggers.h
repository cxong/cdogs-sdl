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

 triggers.h - <description here>

*/

#ifndef __TRIGGERS
#define __TRIGGERS


#define ACTION_NULL             0
#define ACTION_SETTRIGGER       1
#define ACTION_CLEARTRIGGER     2
#define ACTION_CHANGETILE       3
#define ACTION_SETTIMEDWATCH    4
#define ACTION_ACTIVATEWATCH    5
#define ACTION_DEACTIVATEWATCH  6
#define ACTION_SOUND            7

#define CONDITION_NULL          0
#define CONDITION_TIMEDDELAY    1
#define CONDITION_TILECLEAR     2


struct Action {
	int action;
	int x, y;
	int tilePic;
	int tileFlags;
};
typedef struct Action TAction;


struct Trigger {
	int x, y;		// Tile coordinates, ie 0..XMAX-1/YMAX-1
	int flags;
	TAction *actions;
	struct Trigger *left, *right;
};
typedef struct Trigger TTrigger;


struct Condition {
	int condition;
	int x, y;
};
typedef struct Condition TCondition;


struct Watch {
	int index;
	TCondition *conditions;
	TAction *actions;
	struct Watch *next;
};
typedef struct Watch TWatch;


void TriggerAt(int x, int y, int flags);
void UpdateWatches(void);
TTrigger *AddTrigger(int x, int y, int actionCount);
TWatch *AddWatch(int conditionCount, int actionCount);
void FreeTriggersAndWatches(void);


#endif
