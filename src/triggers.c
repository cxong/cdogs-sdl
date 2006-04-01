/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (C) 1995 Ronny Wester
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

 triggers.c - trigger related functions

*/

#include <stdlib.h>
#include <string.h>
#include "triggers.h"
#include "map.h"
#include "sounds.h"


static TTrigger *root = NULL;
static TWatch *activeWatches = NULL;
static TWatch *inactiveWatches = NULL;
static int watchIndex = 1;


static TAction *AddActions(int count)
{
	TAction *a = malloc(sizeof(TAction) * (count + 1));
	memset(a, 0, sizeof(TAction) * (count + 1));
	return a;
}

TTrigger *AddTrigger(int x, int y, int actionCount)
{
	TTrigger *t = malloc(sizeof(TTrigger));
	TTrigger **h;

	memset(t, 0, sizeof(TTrigger));
	t->x = x;
	t->y = y;

	h = &root;
	while (*h) {
		if ((*h)->y < y || ((*h)->y == y && (*h)->x < x))
			h = &((*h)->right);
		else
			h = &((*h)->left);
	}
	*h = t;
	t->actions = AddActions(actionCount);
	return t;
}

void FreeTrigger(TTrigger * t)
{
	if (!t)
		return;

	FreeTrigger(t->left);
	FreeTrigger(t->right);
	free(t->actions);
	free(t);
}

static RemoveAllTriggers(void)
{
	FreeTrigger(root);
	root = NULL;

	return 0;
}

static TCondition *AddConditions(int count)
{
	TCondition *a = malloc(sizeof(TCondition) * (count + 1));
	memset(a, 0, sizeof(TCondition) * (count + 1));
	return a;
}

TWatch *AddWatch(int conditionCount, int actionCount)
{
	TWatch *t = malloc(sizeof(TWatch));

	memset(t, 0, sizeof(TWatch));
	t->index = watchIndex++;
	t->next = inactiveWatches;
	inactiveWatches = t;
	t->actions = AddActions(actionCount);
	t->conditions = AddConditions(conditionCount);
	return t;
}

static TWatch *FindWatch(int index)
{
	TWatch *t = inactiveWatches;

	while (t && t->index != index)
		t = t->next;
	return t;
}

static void ActivateWatch(int index)
{
	TWatch **h = &inactiveWatches;
	TWatch *t;

	while (*h && (*h)->index != index)
		h = &((*h)->next);
	if (*h) {
		t = *h;
		*h = t->next;
		t->next = activeWatches;
		activeWatches = t;
	}
}

static void DeactivateWatch(int index)
{
	TWatch **h = &activeWatches;
	TWatch *t;

	while (*h && (*h)->index != index)
		h = &((*h)->next);
	if (*h) {
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
		free(t->conditions);
		free(t->actions);
		free(t);
	}
	while (inactiveWatches) {
		t = inactiveWatches;
		inactiveWatches = t->next;
		free(t->conditions);
		free(t->actions);
		free(t);
	}
}

void FreeTriggersAndWatches(void)
{
	RemoveAllTriggers();
	RemoveAllWatches();
}

static void Action(TAction * a)
{
	TWatch *t;
	TCondition *c;

	while (1) {
		switch (a->action) {
		case ACTION_NULL:
			return;

		case ACTION_SOUND:
			PlaySoundAt(a->x, a->y, a->tileFlags);
			break;

		case ACTION_SETTRIGGER:
			Map(a->x, a->y).flags |= TILE_TRIGGER;
			break;

		case ACTION_CLEARTRIGGER:
			Map(a->x, a->y).flags &= ~TILE_TRIGGER;
			break;

		case ACTION_CHANGETILE:
			Map(a->x, a->y).flags = a->tileFlags;
			Map(a->x, a->y).pic = a->tilePic;
			break;

		case ACTION_SETTIMEDWATCH:
			t = FindWatch(a->x);
			if (t) {
				c = t->conditions;
				while (c && c->condition != CONDITION_NULL) {
					if (c->condition ==
					    CONDITION_TIMEDDELAY) {
						c->x = a->y;
						break;
					}
					c++;
				}
				ActivateWatch(t->index);
			}
			break;

		case ACTION_ACTIVATEWATCH:
			ActivateWatch(a->x);
			break;

		case ACTION_DEACTIVATEWATCH:
			DeactivateWatch(a->x);
			break;
		}
		a++;
	}
}

static int ConditionMet(TCondition * c)
{
	while (1) {
		switch (c->condition) {
		case CONDITION_NULL:
			return 1;

		case CONDITION_TIMEDDELAY:
			c->x--;
			if (c->x > 0)
				return 0;
			break;

		case CONDITION_TILECLEAR:
			if (Map(c->x, c->y).things != NULL)
				return 0;
			break;
		}
		c++;
	}
}

static TTrigger *FindTrigger(TTrigger * t, int x, int y)
{
	if (!t)
		return NULL;

	if (t->x == x && t->y == y)
		return t;
	if (y > t->y || (y == t->y && x > t->x))
		return FindTrigger(t->right, x, y);
	return FindTrigger(t->left, x, y);
}

void TriggerAt(int x, int y, int flags)
{
	TTrigger *t = FindTrigger(root, x, y);
	while (t) {
		if (t->flags == 0 || (t->flags & flags) != 0)
			Action(t->actions);
		t = FindTrigger(t->left, x, y);
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
			Action(current->actions);
	}
}
