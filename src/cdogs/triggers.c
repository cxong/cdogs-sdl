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

    Copyright (c) 2014-2015, 2017 Cong Xu
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

#include <stdlib.h>
#include <string.h>
#include "triggers.h"
#include "map.h"
#include "net_util.h"
#include "objs.h"
#include "sounds.h"
#include "utils.h"

CArray gWatches;	// of TWatch
static int watchIndex = 1;

// Number of frames to wait before repeating the "cannot activate" event
#define CANNOT_ACTIVATE_LOCK 50


Trigger *TriggerNew(void)
{
	Trigger *t;
	CCALLOC(t, sizeof *t);
	t->isActive = 1;
	CArrayInit(&t->actions, sizeof(Action));
	return t;
}
void TriggerTerminate(Trigger *t)
{
	CArrayTerminate(&t->actions);
	CFREE(t);
}
Action *TriggerAddAction(Trigger *t)
{
	Action a;
	memset(&a, 0, sizeof a);
	CArrayPushBack(&t->actions, &a);
	return CArrayGet(&t->actions, t->actions.size - 1);
}

TWatch *WatchNew(void)
{
	TWatch t;
	memset(&t, 0, sizeof(TWatch));
	t.index = watchIndex++;
	CArrayInit(&t.actions, sizeof(Action));
	CArrayInit(&t.conditions, sizeof(Condition));
	t.active = false;
	CArrayPushBack(&gWatches, &t);
	return CArrayGet(&gWatches, gWatches.size - 1);
}
Condition *WatchAddCondition(
	TWatch *w, const ConditionType type, const int counterMax,
	const Vec2i pos)
{
	Condition c;
	memset(&c, 0, sizeof c);
	c.Type = type;
	c.CounterMax = counterMax;
	c.Pos = pos;
	CArrayPushBack(&w->conditions, &c);
	return CArrayGet(&w->conditions, w->conditions.size - 1);
}
Action *WatchAddAction(TWatch *w)
{
	Action a;
	memset(&a, 0, sizeof a);
	CArrayPushBack(&w->actions, &a);
	return CArrayGet(&w->actions, w->actions.size - 1);
}

static void ActivateWatch(int idx)
{
	CA_FOREACH(TWatch, w, gWatches)
		if (w->index == idx)
		{
			w->active = true;

			// Reset all conditions related to watch
			for (int j = 0; j < (int)w->conditions.size; j++)
			{
				Condition *c = CArrayGet(&w->conditions, j);
				c->Counter = 0;
			}
			return;
		}
	CA_FOREACH_END()
	CASSERT(false, "Cannot find watch");
}

static void DeactivateWatch(int idx)
{
	CA_FOREACH(TWatch, w, gWatches)
		if (w->index == idx)
		{
			w->active = false;
			return;
		}
	CA_FOREACH_END()
	CASSERT(false, "Cannot find watch");
}

void WatchesInit(void)
{
	CArrayInit(&gWatches, sizeof(TWatch));
}
void WatchesTerminate(void)
{
	CA_FOREACH(TWatch, w, gWatches)
		CArrayTerminate(&w->conditions);
		CArrayTerminate(&w->actions);
	CA_FOREACH_END()
	CArrayTerminate(&gWatches);
}

static void ActionRun(Action *a, CArray *mapTriggers)
{
	switch (a->Type)
	{
	case ACTION_NULL:
		return;

	case ACTION_SETTRIGGER:
		for (int i = 0; i < (int)mapTriggers->size; i++)
		{
			Trigger *tr = *(Trigger **)CArrayGet(mapTriggers, i);
			if (tr->id == a->u.index)
			{
				tr->isActive = 1;
				break;
			}
		}
		break;

	case ACTION_CLEARTRIGGER:
		for (int i = 0; i < (int)mapTriggers->size; i++)
		{
			Trigger *tr = *(Trigger **)CArrayGet(mapTriggers, i);
			if (tr->id == a->u.index)
			{
				tr->isActive = 0;
				break;
			}
		}
		break;

	case ACTION_EVENT:
		GameEventsEnqueue(&gGameEvents, a->a.Event);
		break;

	case ACTION_ACTIVATEWATCH:
		ActivateWatch(a->u.index);
		break;

	case ACTION_DEACTIVATEWATCH:
		DeactivateWatch(a->u.index);
		break;

	case ACTION_SOUND:
		SoundPlayAt(&gSoundDevice, a->a.Sound, a->u.pos);
		break;
	}
}

static bool ConditionsMet(CArray *conditions, const int ticks)
{
	bool allConditionsMet = true;
	for (int i = 0; i < (int)conditions->size; i++)
	{
		Condition *c = CArrayGet(conditions, i);
		bool conditionMet = false;
		switch (c->Type)
		{
		case CONDITION_TILECLEAR:
			conditionMet = TileIsClear(MapGetTile(&gMap, c->Pos));
			break;
		}
		if (conditionMet)
		{
			c->Counter += ticks;
			allConditionsMet =
				allConditionsMet && c->Counter >= c->CounterMax;
		}
		else
		{
			c->Counter = 0;
			allConditionsMet = false;
		}
	}
	return allConditionsMet;
}

bool TriggerTryActivate(Trigger *t, const int flags, const Vec2i tilePos)
{
	const bool canActivate =
		t->isActive && (t->flags == 0 || (t->flags & flags));
	if (canActivate)
	{
		GameEvent e = GameEventNew(GAME_EVENT_TRIGGER);
		e.u.TriggerEvent.ID = t->id;
		e.u.TriggerEvent.Tile = Vec2i2Net(tilePos);
		GameEventsEnqueue(&gGameEvents, e);
	}
	else
	{
		if (t->cannotActivateLock > 0)
		{
			t->cannotActivateLock--;
		}
	}
	return canActivate;
}

bool TriggerCannotActivate(const Trigger *t)
{
	return t->cannotActivateLock == 0;
}

void TriggerSetCannotActivate(Trigger *t)
{
	t->cannotActivateLock = CANNOT_ACTIVATE_LOCK;
}

void TriggerActivate(Trigger *t, CArray *mapTriggers)
{
	CA_FOREACH(Action, a, t->actions)
		ActionRun(a, mapTriggers);
	CA_FOREACH_END()
}

void UpdateWatches(CArray *mapTriggers, const int ticks)
{
	CA_FOREACH(TWatch, w, gWatches)
		if (!w->active) continue;
		if (ConditionsMet(&w->conditions, ticks))
		{
			for (int j = 0; j < (int)w->actions.size; j++)
			{
				ActionRun(CArrayGet(&w->actions, j), mapTriggers);
			}
		}
	CA_FOREACH_END()
}
