/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.

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
#include "game_events.h"

#include <string.h>

#include "utils.h"

CArray gGameEvents;

void GameEventsInit(CArray *store)
{
	CArrayInit(store, sizeof(GameEvent));
}
void GameEventsTerminate(CArray *store)
{
	CArrayTerminate(store);
}
void GameEventsEnqueue(CArray *store, GameEvent e)
{
	// Hack: sometimes trigger events are added by placing enemies
	// before the game events has been initialised
	// Just ignore for now
	if (store->elemSize == 0)
	{
		return;
	}
	CArrayPushBack(store, &e);
}
void GameEventsClear(CArray *store)
{
	CArrayClear(store);
}

static GameEvent AddFireballEventNew(
	const BulletClass *class,
	const Vec2i fullPos, const int flags, const int playerIndex,
	const double angle)
{
	GameEvent e;
	memset(&e, 0, sizeof e);
	e.Type = GAME_EVENT_ADD_FIREBALL;
	e.u.AddFireball.Class = class;
	e.u.AddFireball.FullPos = fullPos;
	e.u.AddFireball.Flags = flags;
	e.u.AddFireball.PlayerIndex = playerIndex;
	e.u.AddFireball.Angle = angle;
	return e;
}
static GameEvent AddFireballEventNormalNew(
	const BulletClass *class,
	const Vec2i fullPos, const int flags, const int playerIndex,
	const double angle)
{
	GameEvent e = AddFireballEventNew(
	class, fullPos, flags, playerIndex, angle);
	return e;
}
void GameEventAddFireball(
	const BulletClass *class,
	const Vec2i fullPos, const int flags, const int playerIndex,
	const int dz, const double angle)
{
	GameEvent e = AddFireballEventNormalNew(
		class, fullPos, flags, playerIndex, angle);
	e.u.AddFireball.DZ = dz;
	GameEventsEnqueue(&gGameEvents, e);
}
void GameEventAddFireballWreckage(const Vec2i fullPos)
{
	GameEvent e = AddFireballEventNormalNew(
		StrBulletClass("fireball_wreck"), fullPos, 0, -1, 0);
	GameEventsEnqueue(&gGameEvents, e);
}
void GameEventAddMolotovFlame(
	const Vec2i fullPos, const int flags, const int playerIndex)
{
	GameEvent e = AddFireballEventNew(
		StrBulletClass("molotov_flame"),
		fullPos, flags, playerIndex, (double)rand() / RAND_MAX * 2 * PI);
	e.u.AddFireball.DZ = 4 + rand() % 4;
	GameEventsEnqueue(&gGameEvents, e);
}
void GameEventAddGasCloud(
	const BulletClass *class,
	const Vec2i fullPos, const int flags, const int playerIndex)
{
	GameEvent e = AddFireballEventNew(
		class,
		fullPos, flags, playerIndex, (double)rand() / RAND_MAX * 2 * PI);
	e.u.AddFireball.DZ = 4 + rand() % 4;
	GameEventsEnqueue(&gGameEvents, e);
}
