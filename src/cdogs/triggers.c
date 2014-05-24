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

    Copyright (c) 2014, Cong Xu
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
#include "sounds.h"
#include "utils.h"

static TWatch *activeWatches = NULL;
static TWatch *inactiveWatches = NULL;
static int watchIndex = 1;


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
	TWatch *t;
	CCALLOC(t, sizeof(TWatch));
	t->index = watchIndex++;
	t->next = inactiveWatches;
	inactiveWatches = t;
	CArrayInit(&t->actions, sizeof(Action));
	CArrayInit(&t->conditions, sizeof(Condition));
	return t;
}
Condition *WatchAddCondition(TWatch *w)
{
	Condition c;
	memset(&c, 0, sizeof c);
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
	TWatch **h = &inactiveWatches;
	TWatch *t;

	while (*h && (*h)->index != idx)
	{
		h = &((*h)->next);
	}
	if (*h)
	{
		t = *h;
		*h = t->next;
		t->next = activeWatches;
		activeWatches = t;
	}
}

static void DeactivateWatch(int idx)
{
	TWatch **h = &activeWatches;
	TWatch *t;

	while (*h && (*h)->index != idx)
	{
		h = &((*h)->next);
	}
	if (*h)
	{
		t = *h;
		*h = t->next;
		t->next = inactiveWatches;
		inactiveWatches = t;
	}
}

void RemoveAllWatches(void)
{
	TWatch *t;

	while (activeWatches) {
		t = activeWatches;
		activeWatches = t->next;
		CArrayTerminate(&t->conditions);
		CArrayTerminate(&t->actions);
		CFREE(t);
	}
	while (inactiveWatches) {
		t = inactiveWatches;
		inactiveWatches = t->next;
		CArrayTerminate(&t->conditions);
		CArrayTerminate(&t->actions);
		CFREE(t);
	}
}

static void ActionRun(Action *a, CArray *mapTriggers)
{
	int i;
	switch (a->action)
	{
	case ACTION_NULL:
		return;

	case ACTION_SOUND:
		SoundPlayAt(&gSoundDevice, a->tileFlags, a->u.pos);
		break;

	case ACTION_SETTRIGGER:
		for (i = 0; i < (int)mapTriggers->size; i++)
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
		for (i = 0; i < (int)mapTriggers->size; i++)
		{
			Trigger *tr = *(Trigger **)CArrayGet(mapTriggers, i);
			if (tr->id == a->u.index)
			{
				tr->isActive = 0;
				break;
			}
		}
		break;

	case ACTION_CHANGETILE:
		{
			Tile *t= MapGetTile(&gMap, a->u.pos);
			t->flags = a->tileFlags;
			t->pic = a->tilePic;
			t->picAlt = a->tilePicAlt;
		}
		break;

	case ACTION_ACTIVATEWATCH:
		ActivateWatch(a->u.index);
		break;

	case ACTION_DEACTIVATEWATCH:
		DeactivateWatch(a->u.index);
		break;
	}
}

static int ConditionsMet(CArray *conditions)
{
	int i;
	for (i = 0; i < (int)conditions->size; i++)
	{
		Condition *c = CArrayGet(conditions, i);
		switch (c->condition)
		{
		case CONDITION_TILECLEAR:
			if (!TileIsClear(MapGetTile(&gMap, c->pos)))
			{
				return 0;
			}
			break;
		}
	}
	return 1;
}

bool TriggerCanActivate(const Trigger *t, const int flags)
{
	return t->isActive && (t->flags == 0 || (t->flags & flags));
}

void TriggerActivate(Trigger *t, CArray *mapTriggers)
{
	for (int i = 0; i < (int)t->actions.size; i++)
	{
		ActionRun(CArrayGet(&t->actions, i), mapTriggers);
	}
}

void UpdateWatches(CArray *mapTriggers)
{
	TWatch *a = activeWatches;
	TWatch *current;

	while (a) {
		current = a;
		a = a->next;
		if (ConditionsMet(&current->conditions))
		{
			int i;
			for (i = 0; i < (int)current->actions.size; i++)
			{
				ActionRun(CArrayGet(&current->actions, i), mapTriggers);
			}
		}
	}
}
