/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.
	Copyright (c) 2014, 2016, 2018-2021 Cong Xu
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

static int AddTileClass(any_t data, any_t item);
void MapStaticLoad(MapBuilder *mb)
{
	// Tile classes
	if (hashmap_iterate(
			mb->mission->u.Static.TileClasses, AddTileClass, mb->Map) != MAP_OK)
	{
		CASSERT(false, "failed to add static tile classes");
	}

	// Tiles
	const Rect2i r = Rect2iNew(svec2i_zero(), mb->Map->Size);
	RECT_FOREACH(r)
	MapStaticLoadTile(mb, _v);
	RECT_FOREACH_END()

	// Start/exit areas
	mb->Map->start = mb->mission->u.Static.Start;
	CArrayCopy(&mb->Map->exits, &mb->mission->u.Static.Exits);
}
static int AddTileClass(any_t data, any_t item)
{
	Map *m = data;
	TileClass *t = item;
	// Attach base style to tile class for convenience in editors etc
	CSTRDUP(t->StyleType, TileClassBaseStyleType(t->Type));
	TileClassesAdd(
		m->TileClasses, &gPicManager, t, t->Style, t->StyleType, t->Mask,
		t->MaskAlt);
	switch (t->Type)
	{
	case TILE_CLASS_DOOR:
		SetupDoorTileClasses(m, &gPicManager, t);
		break;
	case TILE_CLASS_WALL:
		SetupWallTileClasses(m, &gPicManager, t);
		break;
	case TILE_CLASS_FLOOR:
		SetupFloorTileClasses(m, &gPicManager, t);
		break;
	default:
		break;
	}
	return MAP_OK;
}

void MapStaticLoadTile(MapBuilder *mb, const struct vec2i v)
{
	if (!MapIsTileIn(mb->Map, v))
		return;
	const int idx = v.y * mb->Map->Size.x + v.x;
	uint16_t tileAccess =
		*(uint16_t *)CArrayGet(&mb->mission->u.Static.Access, idx);
	if (!AreKeysAllowed(mb->mode))
	{
		tileAccess = 0;
	}
	const TileClass *tc =
		MissionStaticGetTileClass(&mb->mission->u.Static, mb->Map->Size, v);
	MapBuilderSetTile(mb, v, tc);
	MapBuildSetAccess(mb, v, tileAccess);
}

static void AddCharacters(const MapBuilder *mb, const CArray *characters);
static void AddObjectives(MapBuilder *mb, const CArray *objectives);
static void AddKeys(MapBuilder *mb, const CArray *keys);
static void AddPickups(const CArray *pickups);
void MapStaticLoadDynamic(MapBuilder *mb)
{
	// Map objects
	CA_FOREACH(const MapObjectPositions, mop, mb->mission->u.Static.Items)
	for (int j = 0; j < (int)mop->Positions.size; j++)
	{
		const struct vec2i *pos = CArrayGet(&mop->Positions, j);
		MapTryPlaceOneObject(mb, *pos, mop->M, 0, false);
	}
	CA_FOREACH_END()

	if (ModeHasNPCs(mb->mode))
	{
		AddCharacters(mb, &mb->mission->u.Static.Characters);
	}

	if (HasObjectives(mb->mode))
	{
		AddObjectives(mb, &mb->mission->u.Static.Objectives);
	}

	if (AreKeysAllowed(mb->mode))
	{
		AddKeys(mb, &mb->mission->u.Static.Keys);
	}

	AddPickups(&mb->mission->u.Static.Pickups);

	// Process the events to place dynamic objects
	HandleGameEvents(&gGameEvents, NULL, NULL, NULL, NULL);
}
static void AddCharacter(const MapBuilder *mb, const CharacterPlaces *cps);
static void AddCharacters(const MapBuilder *mb, const CArray *characters)
{
	CA_FOREACH(const CharacterPlaces, cps, *characters)
	AddCharacter(mb, cps);
	CA_FOREACH_END()
}
static void AddCharacter(const MapBuilder *mb, const CharacterPlaces *cps)
{
	CASSERT(mb->characters != NULL, "cannot add characters");
	const Character *c = CArrayGet(
		&mb->characters->OtherChars, cps->Index);
	CA_FOREACH(const CharacterPlace, cp, cps->Places)
	GameEvent e = GameEventNewActorAdd(Vec2CenterOfTile(cp->Pos), c, true);
	e.u.ActorAdd.CharId = cps->Index;
	e.u.ActorAdd.Direction = cp->Dir;
	GameEventsEnqueue(&gGameEvents, e);
	CA_FOREACH_END()
}
static void AddObjective(MapBuilder *mb, const ObjectivePositions *op);
static void AddObjectives(MapBuilder *mb, const CArray *objectives)
{
	CA_FOREACH(const ObjectivePositions, op, *objectives)
	AddObjective(mb, op);
	CA_FOREACH_END()
}
static void AddObjective(MapBuilder *mb, const ObjectivePositions *op)
{
	if (op->Index >= (int)mb->mission->Objectives.size)
	{
		LOG(LM_MAP, LL_ERROR, "cannot add objective; objective #%d missing",
			op->Index);
		return;
	}
	Objective *o = CArrayGet(&mb->mission->Objectives, op->Index);
	CA_FOREACH(const PositionIndex, pi, op->PositionIndices)
	if (!MapIsTileIn(mb->Map, pi->Position))
	{
		LOG(LM_MAP, LL_ERROR, "objective outside map");
		continue;
	}

	const struct vec2 pos = Vec2CenterOfTile(pi->Position);
	switch (o->Type)
	{
	case OBJECTIVE_KILL: {
		CASSERT(mb->characters != NULL, "cannot add kill objective");
		const int charId = CharacterStoreGetSpecialId(mb->characters, pi->Index);
		const Character *c = CArrayGet(
			&mb->characters->OtherChars, charId);
		GameEvent e = GameEventNewActorAdd(pos, c, true);
		e.u.ActorAdd.CharId = charId;
		e.u.ActorAdd.ThingFlags = ObjectiveToThing(op->Index);
		GameEventsEnqueue(&gGameEvents, e);
	}
	break;
	case OBJECTIVE_COLLECT:
		MapPlaceCollectible(mb->mission, op->Index, pos);
		break;
	case OBJECTIVE_DESTROY:
		MapTryPlaceOneObject(
			mb, pi->Position, o->u.MapObject, ObjectiveToThing(op->Index),
			false);
		break;
	case OBJECTIVE_RESCUE: {
		CASSERT(mb->characters != NULL, "cannot add rescue objective");
		const int charId = CharacterStoreGetPrisonerId(
			mb->characters, pi->Index);
		const Character *c = CArrayGet(
			&mb->characters->OtherChars, charId);
		GameEvent e = GameEventNewActorAdd(pos, c, true);
		e.u.ActorAdd.CharId = charId;
		e.u.ActorAdd.ThingFlags = ObjectiveToThing(op->Index);
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
static void AddKey(MapBuilder *mb, const KeyPositions *kp);
static void AddKeys(MapBuilder *mb, const CArray *keys)
{
	CA_FOREACH(const KeyPositions, kp, *keys)
	AddKey(mb, kp);
	CA_FOREACH_END()
}
static void AddKey(MapBuilder *mb, const KeyPositions *kp)
{
	CA_FOREACH(const struct vec2i, pos, kp->Positions)
	MapPlaceKey(mb, *pos, kp->Index);
	CA_FOREACH_END()
}
static void AddPickup(const PickupPositions *pp);
static void AddPickups(const CArray *pickups)
{
	CA_FOREACH(const PickupPositions, pp, *pickups)
	AddPickup(pp);
	CA_FOREACH_END()
}
static void AddPickup(const PickupPositions *pp)
{
	CA_FOREACH(const struct vec2i, pos, pp->Positions)
	GameEvent e = GameEventNew(GAME_EVENT_ADD_PICKUP);
	e.u.AddPickup.Pos = Vec2ToNet(Vec2CenterOfTile(*pos));
	strcpy(e.u.AddPickup.PickupClass, pp->P->Name);
	GameEventsEnqueue(&gGameEvents, e);
	CA_FOREACH_END()
}
