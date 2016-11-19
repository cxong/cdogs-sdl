/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (c) 2014, 2016, Cong Xu
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
#include "map_static.h"

#include "actors.h"
#include "campaigns.h"
#include "events.h"
#include "game_events.h"
#include "gamedata.h"
#include "handle_game_events.h"
#include "log.h"
#include "map_build.h"
#include "net_util.h"


void MapStaticLoad(Map *map, const struct MissionOptions *mo)
{
	const Mission *m = mo->missionData;

	// Tiles
	Vec2i v;
	for (v.y = 0; v.y < m->Size.y; v.y++)
	{
		for (v.x = 0; v.x < m->Size.x; v.x++)
		{
			int idx = v.y * m->Size.x + v.x;
			unsigned short tile =
				*(unsigned short *)CArrayGet(&m->u.Static.Tiles, idx);
			if (!AreKeysAllowed(gCampaign.Entry.Mode))
			{
				tile &= MAP_MASKACCESS;
			}
			IMapSet(map, v, tile);
		}
	}
	
	// Exit area
	if (!Vec2iIsZero(m->u.Static.Exit.Start) &&
		!Vec2iIsZero(m->u.Static.Exit.End))
	{
		map->ExitStart = m->u.Static.Exit.Start;
		map->ExitEnd = m->u.Static.Exit.End;
	}
}

static void AddCharacters(const CArray *characters);
static void AddObjectives(
	Map *map, const struct MissionOptions *mo, const CharacterStore *store,
	const CArray *objectives);
static void AddKeys(
	Map *map, const struct MissionOptions *mo, const CArray *keys);
void MapStaticLoadDynamic(
	Map *map, const struct MissionOptions *mo, const CharacterStore *store)
{
	const Mission *m = mo->missionData;

	// Map objects
	CA_FOREACH(const MapObjectPositions, mop, m->u.Static.Items)
		for (int j = 0; j < (int)mop->Positions.size; j++)
		{
			const Vec2i *pos = CArrayGet(&mop->Positions, j);
			MapTryPlaceOneObject(map, *pos, mop->M, 0, false);
		}
	CA_FOREACH_END()

	if (ModeHasNPCs(gCampaign.Entry.Mode))
	{
		AddCharacters(&m->u.Static.Characters);
	}

	if (HasObjectives(gCampaign.Entry.Mode))
	{
		AddObjectives(map, mo, store, &m->u.Static.Objectives);
	}

	if (AreKeysAllowed(gCampaign.Entry.Mode))
	{
		AddKeys(map, mo, &m->u.Static.Keys);
	}

	// Process the events to place dynamic objects
	HandleGameEvents(&gGameEvents, NULL, NULL, NULL);
}
static void AddCharacter(const CharacterPositions *cp);
static void AddCharacters(const CArray *characters)
{
	CA_FOREACH(const CharacterPositions, cp, *characters)
		AddCharacter(cp);
	CA_FOREACH_END()
}
static void AddCharacter(const CharacterPositions *cp)
{
	NActorAdd aa = NActorAdd_init_default;
	aa.CharId = cp->Index;
	const Character *c =
		CArrayGet(&gCampaign.Setting.characters.OtherChars, aa.CharId);
	aa.Health = CharacterGetStartingHealth(c, true);
	CA_FOREACH(const Vec2i, pos, cp->Positions)
		aa.UID = ActorsGetNextUID();
		aa.Direction = rand() % DIRECTION_COUNT;
		const Vec2i fullPos = Vec2iReal2Full(Vec2iCenterOfTile(*pos));
		aa.FullPos = Vec2i2Net(fullPos);

		GameEvent e = GameEventNew(GAME_EVENT_ACTOR_ADD);
		e.u.ActorAdd = aa;
		GameEventsEnqueue(&gGameEvents, e);
	CA_FOREACH_END()
}
static void AddObjective(
	Map *map, const struct MissionOptions *mo, const CharacterStore *store,
	const ObjectivePositions *op);
static void AddObjectives(
	Map *map, const struct MissionOptions *mo, const CharacterStore *store,
	const CArray *objectives)
{
	CA_FOREACH(const ObjectivePositions, op, *objectives)
		AddObjective(map, mo, store, op);
	CA_FOREACH_END()
}
static void AddObjective(
	Map *map, const struct MissionOptions *mo, const CharacterStore *store,
	const ObjectivePositions *op)
{
	if (op->Index >= (int)mo->missionData->Objectives.size)
	{
		LOG(LM_MAP, LL_ERROR, "cannot add objective; objective #%d missing",
			op->Index);
		return;
	}
	Objective *o = CArrayGet(&mo->missionData->Objectives, op->Index);
	CA_FOREACH(const Vec2i, pos, op->Positions)
		const int *idx = CArrayGet(&op->Indices, _ca_index);
		const Vec2i realPos = Vec2iCenterOfTile(*pos);
		const Vec2i fullPos = Vec2iReal2Full(realPos);
		switch (o->Type)
		{
		case OBJECTIVE_KILL:
		{
			NActorAdd aa = NActorAdd_init_default;
			aa.UID = ActorsGetNextUID();
			aa.CharId = CharacterStoreGetSpecialId(store, *idx);
			aa.TileItemFlags = ObjectiveToTileItem(op->Index);
			aa.Direction = rand() % DIRECTION_COUNT;
			const Character *c =
				CArrayGet(&gCampaign.Setting.characters.OtherChars, aa.CharId);
			aa.Health = CharacterGetStartingHealth(c, true);
			aa.FullPos = Vec2i2Net(fullPos);
			GameEvent e = GameEventNew(GAME_EVENT_ACTOR_ADD);
			e.u.ActorAdd = aa;
			GameEventsEnqueue(&gGameEvents, e);
		}
		break;
		case OBJECTIVE_COLLECT:
			MapPlaceCollectible(mo, op->Index, realPos);
			break;
		case OBJECTIVE_DESTROY:
			MapTryPlaceOneObject(
				map,
				*pos,
				o->u.MapObject,
				ObjectiveToTileItem(op->Index), false);
			break;
		case OBJECTIVE_RESCUE:
		{
			NActorAdd aa = NActorAdd_init_default;
			aa.UID = ActorsGetNextUID();
			aa.CharId = CharacterStoreGetPrisonerId(store, *idx);
			aa.TileItemFlags = ObjectiveToTileItem(op->Index);
			aa.Direction = rand() % DIRECTION_COUNT;
			const Character *c =
				CArrayGet(&gCampaign.Setting.characters.OtherChars, aa.CharId);
			aa.Health = CharacterGetStartingHealth(c, true);
			aa.FullPos = Vec2i2Net(fullPos);
			GameEvent e = GameEventNew(GAME_EVENT_ACTOR_ADD);
			e.u.ActorAdd = aa;
			GameEventsEnqueue(&gGameEvents, e);
		}
		break;
		default:
			// do nothing
			break;
		}
		o->placed++;
	CA_FOREACH_END()
}
static void AddKey(
	Map *map, const struct MissionOptions *mo, const KeyPositions *kp);
static void AddKeys(
	Map *map, const struct MissionOptions *mo, const CArray *keys)
{
	CA_FOREACH(const KeyPositions, kp, *keys)
		AddKey(map, mo, kp);
	CA_FOREACH_END()
}
static void AddKey(
	Map *map, const struct MissionOptions *mo, const KeyPositions *kp)
{
	CA_FOREACH(const Vec2i, pos, kp->Positions)
		MapPlaceKey(map, mo, *pos, kp->Index);
	CA_FOREACH_END()
}
