/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
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
#include "map_static.h"

#include "actors.h"
#include "campaigns.h"
#include "events.h"
#include "game_events.h"
#include "gamedata.h"
#include "handle_game_events.h"
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
	for (int i = 0; i < (int)m->u.Static.Items.size; i++)
	{
		const MapObjectPositions *mop = CArrayGet(&m->u.Static.Items, i);
		for (int j = 0; j < (int)mop->Positions.size; j++)
		{
			const Vec2i *pos = CArrayGet(&mop->Positions, j);
			MapTryPlaceOneObject(map, *pos, mop->M, 0, false);
		}
	}

	// Wrecks
	for (int i = 0; i < (int)m->u.Static.Wrecks.size; i++)
	{
		const MapObjectPositions *mop = CArrayGet(&m->u.Static.Wrecks, i);
		for (int j = 0; j < (int)mop->Positions.size; j++)
		{
			const Vec2i *pos = CArrayGet(&mop->Positions, j);
			MapPlaceWreck(map, *pos, mop->M);
		}
	}

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
}
static void AddCharacters(const CArray *characters)
{
	for (int i = 0; i < (int)characters->size; i++)
	{
		const CharacterPositions *cp = CArrayGet(characters, i);
		for (int j = 0; j < (int)cp->Positions.size; j++)
		{
			NActorAdd aa = NActorAdd_init_default;
			aa.UID = ActorsGetNextUID();
			aa.CharId = cp->Index;
			aa.Direction = rand() % DIRECTION_COUNT;
			const Character *c =
				CArrayGet(&gCampaign.Setting.characters.OtherChars, aa.CharId);
			aa.Health = CharacterGetStartingHealth(c, true);
			const Vec2i *pos = CArrayGet(&cp->Positions, j);
			const Vec2i fullPos = Vec2iReal2Full(Vec2iCenterOfTile(*pos));
			aa.FullPos.x = fullPos.x;
			aa.FullPos.y = fullPos.y;
			GameEvent e = GameEventNew(GAME_EVENT_ACTOR_ADD);
			e.u.ActorAdd = aa;
			GameEventsEnqueue(&gGameEvents, e);

			// Process the events that actually place the players
			HandleGameEvents(&gGameEvents, NULL, NULL, NULL);
		}
	}
}
static void AddObjectives(
	Map *map, const struct MissionOptions *mo, const CharacterStore *store,
	const CArray *objectives)
{
	for (int i = 0; i < (int)objectives->size; i++)
	{
		const ObjectivePositions *op = CArrayGet(objectives, i);
		const MissionObjective *mobj =
			CArrayGet(&mo->missionData->Objectives, op->Index);
		ObjectiveDef *obj = CArrayGet(&mo->Objectives, op->Index);
		for (int j = 0; j < (int)op->Positions.size; j++)
		{
			const Vec2i *pos = CArrayGet(&op->Positions, j);
			const int *idx = CArrayGet(&op->Indices, j);
			const Vec2i realPos = Vec2iCenterOfTile(*pos);
			const Vec2i fullPos = Vec2iReal2Full(realPos);
			switch (mobj->Type)
			{
			case OBJECTIVE_KILL:
			{
				NActorAdd aa = NActorAdd_init_default;
				aa.UID = ActorsGetNextUID();
				aa.CharId = CharacterStoreGetSpecialId(store, *idx);
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
					obj->blowupObject,
					ObjectiveToTileItem(op->Index), 1);
				break;
			case OBJECTIVE_RESCUE:
			{
				NActorAdd aa = NActorAdd_init_default;
				aa.UID = ActorsGetNextUID();
				aa.CharId = CharacterStoreGetPrisonerId(store, *idx);
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
			obj->placed++;

			// Process the events that actually place the objectives
			HandleGameEvents(&gGameEvents, NULL, NULL, NULL);
		}
	}
}
static void AddKeys(
	Map *map, const struct MissionOptions *mo, const CArray *keys)
{
	for (int i = 0; i < (int)keys->size; i++)
	{
		const KeyPositions *kp = CArrayGet(keys, i);
		for (int j = 0; j < (int)kp->Positions.size; j++)
		{
			const Vec2i *pos = CArrayGet(&kp->Positions, j);
			MapPlaceKey(map, mo, *pos, kp->Index);
		}
	}
}
