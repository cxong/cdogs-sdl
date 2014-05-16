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
	CArrayPushBack(store, &e);
}
void GameEventsClear(CArray *store)
{
	CArrayClear(store);
}

static GameEvent AddFireballEventNew(
	const Vec2i fullPos, const int flags, const int playerIndex,
	BulletUpdateFunc updateFunc,
	TileItemDrawFunc drawFunc, TileItemGetPicFunc getPicFunc,
	const Vec2i size, const int range)
{
	GameEvent e;
	memset(&e, 0, sizeof e);
	e.Type = GAME_EVENT_ADD_FIREBALL;
	e.u.AddFireball.FullPos = fullPos;
	e.u.AddFireball.Flags = flags;
	e.u.AddFireball.PlayerIndex = playerIndex;
	e.u.AddFireball.UpdateFunc = updateFunc;
	e.u.AddFireball.DrawFunc = drawFunc;
	e.u.AddFireball.GetPicFunc = getPicFunc;
	e.u.AddFireball.Size = size;
	e.u.AddFireball.Range = range;
	return e;
}
static GameEvent AddFireballEventNormalNew(
	const Vec2i fullPos, const int flags, const int playerIndex)
{
	GameEvent e = AddFireballEventNew(
		fullPos, flags, playerIndex, UpdateExplosion, DrawFireball, NULL,
		Vec2iNew(7, 5), FIREBALL_MAX * 4 - 1);
	return e;
}
void GameEventAddFireball(
	const Vec2i fullPos, const int flags, const int playerIndex,
	const Vec2i vel, const int dz, const int count)
{
	GameEvent e = AddFireballEventNormalNew(fullPos, flags, playerIndex);
	e.u.AddFireball.Vel = vel;
	e.u.AddFireball.DZ = dz;
	e.u.AddFireball.Count = count;
	e.u.AddFireball.Power = FIREBALL_POWER;
	GameEventsEnqueue(&gGameEvents, e);
}
void GameEventAddFireballWreckage(const Vec2i fullPos)
{
	GameEvent e = AddFireballEventNormalNew(fullPos, 0, -1);
	e.u.AddFireball.Count = 10;
	GameEventsEnqueue(&gGameEvents, e);
}
void GameEventAddMolotovFlame(
	const Vec2i fullPos, const int flags, const int playerIndex)
{
	GameEvent e = AddFireballEventNew(
		fullPos, flags, playerIndex, UpdateMolotovFlame, NULL, GetFlame,
		Vec2iNew(5, 5), (FLAME_RANGE + rand() % 8) * 4);
	e.u.AddFireball.Vel =
		Vec2iNew(16 * (rand() % 32) - 256, 12 * (rand() % 32) - 192);
	e.u.AddFireball.DZ = 4 + rand() % 4;
	e.u.AddFireball.Power = 2;
	GameEventsEnqueue(&gGameEvents, e);
}
void GameEventAddGasCloud(
	const Vec2i fullPos, const int flags, const int playerIndex,
	special_damage_e special)
{
	const double radians = (double)rand() / RAND_MAX * 2 * PI;
	const int speed = (256 + rand()) & 255;
	const Vec2i vel =
		Vec2iFull2Real(Vec2iScale(
		GetFullVectorsForRadians(radians), speed));
	const Vec2i fullPosOffset = Vec2iAdd(fullPos, Vec2iScale(vel, 6));
	GameEvent e = AddFireballEventNew(
		fullPosOffset, flags, playerIndex, UpdateGasCloud, DrawGasCloud, NULL,
		Vec2iNew(10, 10), (48 - (rand() % 8)) * 4 - 1);
	e.u.AddFireball.Vel = vel;
	e.u.AddFireball.DZ = 4 + rand() % 4;
	e.u.AddFireball.Power = 0;
	e.u.AddFireball.Special = special;
	GameEventsEnqueue(&gGameEvents, e);
}
