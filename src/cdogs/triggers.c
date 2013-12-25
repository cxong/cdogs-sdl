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

-------------------------------------------------------------------------------

 triggers.c - trigger related functions

 Author: $Author$
 Rev:    $Revision$
 URL:    $HeadURL$
 ID:     $Id$

*/

#include <stdlib.h>
#include <string.h>
#include "triggers.h"
#include "map.h"
#include "sounds.h"
#include "utils.h"

static Trigger *root = NULL;
static TWatch *activeWatches = NULL;
static TWatch *inactiveWatches = NULL;
static int watchIndex = 1;


static Action *AddActions(int count)
{
	Action *a;
	CCALLOC(a, sizeof *a * (count + 1));
	return a;
}

Trigger *AddTrigger(Vec2i pos, int actionCount)
{
	Trigger *t;
	Trigger **h;

	CCALLOC(t, sizeof *t);
	t->pos = pos;

	h = &root;
	while (*h)
	{
		if ((*h)->pos.y < pos.y ||
			((*h)->pos.y == pos.y && (*h)->pos.x < pos.x))
		{
			h = &((*h)->right);
		}
		else
		{
			h = &((*h)->left);
		}
	}
	*h = t;
	t->actions = AddActions(actionCount);
	return t;
}

void FreeTrigger(Trigger * t)
{
	if (!t)
		return;

	FreeTrigger(t->left);
	FreeTrigger(t->right);
	CFREE(t->actions);
	CFREE(t);
}

static int RemoveAllTriggers(void)
{
	FreeTrigger(root);
	root = NULL;

	return 0;
}

static Condition *AddConditions(int count)
{
	Condition *a;
	CCALLOC(a, sizeof *a * (count + 1));
	return a;
}

TWatch *AddWatch(int conditionCount, int actionCount)
{
	TWatch *t;
	CCALLOC(t, sizeof(TWatch));
	t->index = watchIndex++;
	t->next = inactiveWatches;
	inactiveWatches = t;
	t->actions = AddActions(actionCount);
	t->conditions = AddConditions(conditionCount);
	return t;
}

static TWatch *FindWatch(int idx)
{
	TWatch *t = inactiveWatches;

	while (t && t->index != idx)
	{
		t = t->next;
	}
	return t;
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

static void RemoveAllWatches(void)
{
	TWatch *t;

	while (activeWatches) {
		t = activeWatches;
		activeWatches = t->next;
		CFREE(t->conditions);
		CFREE(t->actions);
		CFREE(t);
	}
	while (inactiveWatches) {
		t = inactiveWatches;
		inactiveWatches = t->next;
		CFREE(t->conditions);
		CFREE(t->actions);
		CFREE(t);
	}
}

void FreeTriggersAndWatches(void)
{
	RemoveAllTriggers();
	RemoveAllWatches();
}

static void ActionRun(Action * a)
{
	for (;;)
	{
		Tile *t;
		switch (a->action)
		{
		case ACTION_NULL:
			return;

		case ACTION_SOUND:
			SoundPlayAt(&gSoundDevice, a->tileFlags, a->u.pos);
			break;

		case ACTION_SETTRIGGER:
			MapGetTile(&gMap, a->u.pos)->flags |= MAPTILE_TILE_TRIGGER;
			break;

		case ACTION_CLEARTRIGGER:
			MapGetTile(&gMap, a->u.pos)->flags &= ~MAPTILE_TILE_TRIGGER;
			break;

		case ACTION_CHANGETILE:
			t = MapGetTile(&gMap, a->u.pos);
			t->flags = a->tileFlags;
			t->pic = a->tilePic;
			t->picAlt = a->tilePicAlt;
			break;

		case ACTION_ACTIVATEWATCH:
			ActivateWatch(a->u.index);
			break;

		case ACTION_DEACTIVATEWATCH:
			DeactivateWatch(a->u.index);
			break;
		}
		a++;
	}
}

static int ConditionMet(Condition *c)
{
	for (;;)
	{
		switch (c->condition)
		{
		case CONDITION_NULL:
			return 1;

		case CONDITION_TILECLEAR:
			if (!TileIsClear(MapGetTile(&gMap, c->pos)))
			{
				return 0;
			}
			break;
		}
		c++;
	}
}

static Trigger *FindTrigger(Trigger *t, Vec2i pos)
{
	if (!t)
	{
		return NULL;
	}

	if (Vec2iEqual(t->pos, pos))
	{
		return t;
	}
	if (pos.y > t->pos.y || (pos.y == t->pos.y && pos.x > t->pos.x))
	{
		return FindTrigger(t->right, pos);
	}
	return FindTrigger(t->left, pos);
}

void TriggerAt(Vec2i pos, int flags)
{
	Trigger *t = FindTrigger(root, pos);
	while (t)
	{
		if (t->flags == 0 || (t->flags & flags))
		{
			ActionRun(t->actions);
		}
		t = FindTrigger(t->left, pos);
	}
}

void UpdateWatches(void)
{
	TWatch *a = activeWatches;
	TWatch *current;

	while (a) {
		current = a;
		a = a->next;
		if (ConditionMet(current->conditions))
		{
			ActionRun(current->actions);
		}
	}
}
