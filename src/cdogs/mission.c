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

	Copyright (c) 2013-2017, 2019-2021 Cong Xu
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
#include "mission.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "actors.h"
#include "color.h"
#include "defs.h"
#include "door.h"
#include "files.h"
#include "game_events.h"
#include "gamedata.h"
#include "map.h"
#include "map_new.h"
#include "music.h"
#include "net_util.h"
#include "objs.h"
#include "palette.h"
#include "particle.h"
#include "pic_manager.h"
#include "pickup.h"
#include "triggers.h"

color_t colorDoor = {172, 172, 172, 255};
color_t colorYellowDoor = {252, 224, 0, 255};
color_t colorGreenDoor = {0, 252, 0, 255};
color_t colorBlueDoor = {0, 252, 252, 255};
color_t colorRedDoor = {132, 0, 0, 255};

int StrKeycard(const char *s)
{
	S2T(FLAGS_KEYCARD_YELLOW, "yellow");
	S2T(FLAGS_KEYCARD_GREEN, "green");
	S2T(FLAGS_KEYCARD_BLUE, "blue");
	S2T(FLAGS_KEYCARD_RED, "red");
	return 0;
}
color_t KeyColor(const int flags)
{
	switch (flags)
	{
	case FLAGS_KEYCARD_YELLOW:
		return colorYellowDoor;
	case FLAGS_KEYCARD_GREEN:
		return colorGreenDoor;
	case FLAGS_KEYCARD_BLUE:
		return colorBlueDoor;
	case FLAGS_KEYCARD_RED:
		return colorRedDoor;
	default:
		return colorDoor;
	}
}

const char *MapTypeStr(MapType t)
{
	switch (t)
	{
		T2S(MAPTYPE_CLASSIC, "Classic");
		T2S(MAPTYPE_STATIC, "Static");
		T2S(MAPTYPE_CAVE, "Cave");
		T2S(MAPTYPE_INTERIOR, "Interior");
	default:
		return "";
	}
}
MapType StrMapType(const char *s)
{
	S2T(MAPTYPE_CLASSIC, "Classic");
	S2T(MAPTYPE_STATIC, "Static");
	S2T(MAPTYPE_CAVE, "Cave");
	S2T(MAPTYPE_INTERIOR, "Interior");
	return MAPTYPE_CLASSIC;
}

void MissionInit(Mission *m)
{
	memset(m, 0, sizeof *m);
	// Initialise with default styles
	strcpy(m->ExitStyle, IntExitStyle(0));
	strcpy(m->KeyStyle, IntKeyStyle(0));
	CArrayInit(&m->Objectives, sizeof(Objective));
	CArrayInit(&m->Enemies, sizeof(int));
	CArrayInit(&m->SpecialChars, sizeof(int));
	CArrayInit(&m->MapObjectDensities, sizeof(MapObjectDensity));
	CArrayInit(&m->Weapons, sizeof(const WeaponClass *));
	m->Type = MAPTYPE_CLASSIC;
	m->u.Classic.ExitEnabled = true;
	MissionTileClassesInitDefault(&m->u.Classic.TileClasses);
}

static const MissionTileClasses *MissionGetTileClassesC(const Mission *m)
{
	switch (m->Type)
	{
	case MAPTYPE_CLASSIC:
		return &m->u.Classic.TileClasses;
	case MAPTYPE_STATIC:
		return NULL;
	case MAPTYPE_CAVE:
		return &m->u.Cave.TileClasses;
	case MAPTYPE_INTERIOR:
		return &m->u.Interior.TileClasses;
	default:
		return NULL;
	}
}
void MissionCopy(Mission *dst, const Mission *src)
{
	if (src == NULL)
	{
		return;
	}
	MissionTerminate(dst);
	MissionInit(dst);
	if (src->Title)
	{
		CSTRDUP(dst->Title, src->Title);
	}
	if (src->Description)
	{
		CSTRDUP(dst->Description, src->Description);
	}
	dst->Size = src->Size;

	strcpy(dst->ExitStyle, src->ExitStyle);
	strcpy(dst->KeyStyle, src->KeyStyle);

	CA_FOREACH(const Objective, srco, src->Objectives)
	Objective dsto;
	ObjectiveCopy(&dsto, srco);
	CArrayPushBack(&dst->Objectives, &dsto);
	CA_FOREACH_END()
	CArrayCopy(&dst->Enemies, &src->Enemies);
	CArrayCopy(&dst->SpecialChars, &src->SpecialChars);
	CArrayCopy(&dst->MapObjectDensities, &src->MapObjectDensities);

	dst->EnemyDensity = src->EnemyDensity;
	CArrayCopy(&dst->Weapons, &src->Weapons);

	dst->Music = src->Music;
	switch (src->Music.Type)
	{
	case MUSIC_SRC_GENERAL:
		break;
	case MUSIC_SRC_DYNAMIC:
		if (src->Music.Data.Filename)
		{
			CSTRDUP(dst->Music.Data.Filename, src->Music.Data.Filename);
		}
		break;
	case MUSIC_SRC_CHUNK:
		// TODO: can't copy music chunks, only used by editor anyway
		memset(&dst->Music.Data.Chunk, 0, sizeof dst->Music.Data.Chunk);
		break;
	default:
		CASSERT(false, "unsupported music type");
		break;
	}

	memcpy(&dst->u, &src->u, sizeof dst->u);
	switch (src->Type)
	{
	case MAPTYPE_CLASSIC:
		MissionTileClassesCopy(
			MissionGetTileClasses(dst), MissionGetTileClassesC(src));
		break;
	case MAPTYPE_STATIC:
		MissionStaticCopy(&dst->u.Static, &src->u.Static);
		break;
	case MAPTYPE_CAVE:
		MissionTileClassesCopy(
			MissionGetTileClasses(dst), MissionGetTileClassesC(src));
		break;
	case MAPTYPE_INTERIOR:
		MissionTileClassesCopy(
			MissionGetTileClasses(dst), MissionGetTileClassesC(src));
		break;
	default:
		break;
	}

	// Copy type at the end so we can do type-specific conversions before this
	dst->Type = src->Type;
}

void MissionTerminate(Mission *m)
{
	if (m == NULL)
		return;
	CFREE(m->Title);
	CFREE(m->Description);
	CA_FOREACH(Objective, o, m->Objectives)
	ObjectiveTerminate(o);
	CA_FOREACH_END()
	CArrayTerminate(&m->Objectives);
	CArrayTerminate(&m->Enemies);
	CArrayTerminate(&m->SpecialChars);
	CArrayTerminate(&m->MapObjectDensities);
	CArrayTerminate(&m->Weapons);
	switch (m->Type)
	{
	case MAPTYPE_CLASSIC:
		MissionTileClassesTerminate(MissionGetTileClasses(m));
		break;
	case MAPTYPE_STATIC:
		MissionStaticTerminate(&m->u.Static);
		break;
	case MAPTYPE_CAVE:
		MissionTileClassesTerminate(MissionGetTileClasses(m));
		break;
	case MAPTYPE_INTERIOR:
		MissionTileClassesTerminate(MissionGetTileClasses(m));
		break;
	default:
		CASSERT(false, "unknown map type");
		break;
	}
	switch (m->Music.Type)
	{
	case MUSIC_SRC_DYNAMIC:
		CFREE(m->Music.Data.Filename);
		break;
	case MUSIC_SRC_CHUNK:
		MusicChunkTerminate(&m->Music.Data.Chunk);
		break;
	default:
		break;
	}
	memset(m, 0, sizeof *m);
}

MissionTileClasses *MissionGetTileClasses(Mission *m)
{
	switch (m->Type)
	{
	case MAPTYPE_CLASSIC:
		return &m->u.Classic.TileClasses;
	case MAPTYPE_STATIC:
		return NULL;
	case MAPTYPE_CAVE:
		return &m->u.Cave.TileClasses;
	case MAPTYPE_INTERIOR:
		return &m->u.Interior.TileClasses;
	default:
		return NULL;
	}
}

// +-----------------------+
// |  And now the code...  |
// +-----------------------+

static void SetupBadguysForMission(Mission *mission)
{
	CharacterStore *s = &gCampaign.Setting.characters;

	CharacterStoreResetOthers(s);

	if (s->OtherChars.size == 0)
	{
		return;
	}

	CA_FOREACH(const Objective, o, mission->Objectives)
	if (o->Type == OBJECTIVE_RESCUE)
	{
		CharacterStoreAddPrisoner(s, o->u.Index);
		break; // TODO: multiple prisoners
	}
	CA_FOREACH_END()

	CA_FOREACH(int, e, mission->Enemies)
	CharacterStoreAddBaddie(s, *e);
	CA_FOREACH_END()

	CA_FOREACH(int, sc, mission->SpecialChars)
	CharacterStoreAddSpecial(s, *sc);
	CA_FOREACH_END()
}

static void SetupObjectives(Mission *m)
{
	CA_FOREACH(Objective, o, m->Objectives)
	ObjectiveSetup(o);
	CASSERT(_ca_index < OBJECTIVE_MAX_OLD, "too many objectives");
	CA_FOREACH_END()
}

static void SetupWeapons(CArray *to, CArray *from)
{
	CArrayCopy(to, from);
}

void SetupMission(Mission *m, struct MissionOptions *mo, int missionIndex)
{
	MissionOptionsInit(mo);
	mo->index = missionIndex;
	mo->missionData = m;

	ActorsInit();
	ObjsInit();
	MobObjsInit();
	PickupsInit();
	ParticlesInit(&gParticles);
	WatchesInit();
	SetupObjectives(m);
	SetupBadguysForMission(m);
	SetupWeapons(&mo->Weapons, &m->Weapons);
}
void MissionSetupTileClasses(Map *m, PicManager *pm, const MissionTileClasses *mtc)
{
	SetupWallTileClasses(m, pm, &mtc->Wall);
	SetupFloorTileClasses(m, pm, &mtc->Floor);
	SetupFloorTileClasses(m, pm, &mtc->Room);
	SetupDoorTileClasses(m, pm, &mtc->Door);
}
void MissionTileClassesInitDefault(MissionTileClasses *mtc)
{
	TileClassInit(
		&mtc->Wall, &gPicManager, &gTileWall, IntWallStyle(0),
		TileClassBaseStyleType(TILE_CLASS_WALL), colorBattleshipGrey,
		colorOfficeGreen);
	TileClassInit(
		&mtc->Floor, &gPicManager, &gTileFloor, IntFloorStyle(0),
		TileClassBaseStyleType(TILE_CLASS_FLOOR), colorGravel,
		colorOfficeGreen);
	TileClassInit(
		&mtc->Room, &gPicManager, &gTileRoom, IntRoomStyle(0),
		TileClassBaseStyleType(TILE_CLASS_FLOOR), colorDoveGray,
		colorOfficeGreen);
	TileClassInit(
		&mtc->Door, &gPicManager, &gTileDoor, IntDoorStyle(0),
		TileClassBaseStyleType(TILE_CLASS_DOOR), colorWhite, colorWhite);
}
void MissionTileClassesCopy(
	MissionTileClasses *dst, const MissionTileClasses *src)
{
	if (dst == NULL || src == NULL)
		return;
	TileClassCopy(&dst->Door, &src->Door);
	TileClassCopy(&dst->Floor, &src->Floor);
	TileClassCopy(&dst->Wall, &src->Wall);
	TileClassCopy(&dst->Room, &src->Room);
}
void MissionTileClassesTerminate(MissionTileClasses *mtc)
{
	TileClassTerminate(&mtc->Wall);
	TileClassTerminate(&mtc->Floor);
	TileClassTerminate(&mtc->Room);
	TileClassTerminate(&mtc->Door);
}

static int ObjectiveActorsAlive(const int objective);
void MissionSetMessageIfComplete(struct MissionOptions *options)
{
	if (!gCampaign.IsClient)
	{
		if (CanCompleteMission(options) && !gMission.MissionCompleted)
		{
			GameEvent msg = GameEventNew(GAME_EVENT_MISSION_COMPLETE);
			msg.u.MissionComplete = NMakeMissionComplete(options);
			GameEventsEnqueue(&gGameEvents, msg);
		}
		else if (options->HasBegun && gCampaign.Entry.Mode == GAME_MODE_NORMAL)
		{
			// Check if the game is impossible to end
			// i.e. not enough rescue objectives left alive
			CA_FOREACH(const Objective, o, options->missionData->Objectives)
			if (o->Type == OBJECTIVE_RESCUE)
			{
				if (ObjectiveActorsAlive(_ca_index) < o->Required)
				{
					GameEvent e = GameEventNew(GAME_EVENT_MISSION_END);
					e.u.MissionEnd.Delay = GAME_OVER_DELAY;
					strcpy(e.u.MissionEnd.Msg, "Mission failed");
					GameEventsEnqueue(&gGameEvents, e);
				}
			}
			CA_FOREACH_END()
		}
	}
}
// Get the number of actors alive for an objective
static int ObjectiveActorsAlive(const int objective)
{
	int count = 0;
	CA_FOREACH(const TActor, a, gActors)
	if (a->isInUse && a->health > 0 &&
		ObjectiveFromThing(a->thing.flags) == objective)
	{
		count++;
	}
	CA_FOREACH_END()
	return count;
}

bool MissionHasRequiredObjectives(const struct MissionOptions *mo)
{
	CA_FOREACH(const Objective, o, mo->missionData->Objectives)
	if (ObjectiveIsRequired(o))
		return true;
	CA_FOREACH_END()
	return false;
}

void UpdateMissionObjective(
	const struct MissionOptions *options, const int flags,
	const ObjectiveType type, const int count)
{
	if (!(flags & THING_OBJECTIVE))
	{
		return;
	}
	const int idx = ObjectiveFromThing(flags);
	const Objective *o = CArrayGet(&options->missionData->Objectives, idx);
	if (o->Type != type)
	{
		return;
	}
	if (!gCampaign.IsClient)
	{
		GameEvent e = GameEventNew(GAME_EVENT_OBJECTIVE_UPDATE);
		e.u.ObjectiveUpdate.ObjectiveId = idx;
		e.u.ObjectiveUpdate.Count = count;
		GameEventsEnqueue(&gGameEvents, e);
	}
}

bool MissionCanBegin(void)
{
	// Need at least two players to begin PVP
	if (IsPVP(gCampaign.Entry.Mode))
	{
		return GetNumPlayers(PLAYER_ALIVE_OR_DYING, false, false) > 1;
	}
	// Otherwise, just one player will do
	return GetNumPlayers(PLAYER_ALIVE_OR_DYING, false, false) > 0;
}

void MissionBegin(struct MissionOptions *m, const NGameBegin gb)
{
	m->HasBegun = true;
	m->state = MISSION_STATE_PLAY;
	switch (m->missionData->Music.Type)
	{
	case MUSIC_SRC_GENERAL:
		MusicPlayGeneral(&gSoundDevice.music, MUSIC_GAME);
		break;
	case MUSIC_SRC_DYNAMIC:
		MusicPlayFile(
			&gSoundDevice.music, MUSIC_GAME, gCampaign.Entry.Path,
			m->missionData->Music.Data.Filename);
		break;
	case MUSIC_SRC_CHUNK:
		MusicPlayFromChunk(
			&gSoundDevice.music, MUSIC_GAME,
			&m->missionData->Music.Data.Chunk);
		break;
	default:
		CASSERT(false, "unsupported music type");
		break;
	}
	const char *musicErrorMsg = MusicGetErrorMessage(&gSoundDevice.music);
	if (strlen(musicErrorMsg) > 0)
	{
		// Display music error message for 2 seconds
		GameEvent e = GameEventNew(GAME_EVENT_SET_MESSAGE);
		strncat(
			e.u.SetMessage.Message, musicErrorMsg,
			sizeof e.u.SetMessage.Message - 1);
		e.u.SetMessage.Ticks = 2000;
		GameEventsEnqueue(&gGameEvents, e);
	}
	m->time = gb.MissionTime;
	m->pickupTime = 0;
}

bool CanCompleteMission(const struct MissionOptions *options)
{
	// Can't complete if not started yet
	if (!options->HasBegun)
	{
		return false;
	}

	// Death is the only escape from PVP and quick play
	if (IsPVP(gCampaign.Entry.Mode))
	{
		// If we're in deathmatch with 1 player only, never complete the game
		// Instead we'll be showing a "waiting for players..." message
		return GetNumPlayers(PLAYER_ANY, false, false) > 1 &&
			   GetNumPlayers(PLAYER_ALIVE_OR_DYING, false, false) <= 1;
	}

	return MissionAllObjectivesComplete(options);
}

bool MissionAllObjectivesComplete(const struct MissionOptions *mo)
{
	// Check all objective counts are enough
	CA_FOREACH(const Objective, o, mo->missionData->Objectives)
	if (!ObjectiveIsComplete(o))
		return false;
	CA_FOREACH_END()
	return true;
}

static bool MoreRescuesNeeded(const struct MissionOptions *mo, const int exit);

bool IsMissionComplete(const struct MissionOptions *mo)
{
	if (!CanCompleteMission(mo))
	{
		return false;
	}

	// Check if dogfight is complete
	if (IsPVP(gCampaign.Entry.Mode) &&
		GetNumPlayers(PLAYER_ALIVE_OR_DYING, false, false) <= 1)
	{
		// Also check that only one player has lives left
		int numPlayersWithLives = 0;
		CA_FOREACH(const PlayerData, p, gPlayerDatas)
		if (p->Lives > 0)
			numPlayersWithLives++;
		CA_FOREACH_END()
		if (numPlayersWithLives <= 1)
		{
			return true;
		}
	}

	const int exit = AllSurvivingPlayersInSameExit();
	if (MoreRescuesNeeded(mo, exit))
	{
		return false;
	}

	return true;
}

bool MissionNeedsMoreRescuesInExit(const struct MissionOptions *mo)
{
	const int exit = AllSurvivingPlayersInSameExit();
	return CanCompleteMission(mo) && exit != -1 && MoreRescuesNeeded(mo, exit);
}

int AllSurvivingPlayersInSameExit(void)
{
	// Check that all surviving players are in same exit, and return that exit
	// Return -1 otherwise
	// Note: players are still in the exit area if they are dying there;
	// this is the basis for the "resurrection penalty"
	int exit = -1;
	CA_FOREACH(const PlayerData, p, gPlayerDatas)
	if (!IsPlayerAliveOrDying(p))
		continue;
	const TActor *player = ActorGetByUID(p->ActorUID);
	const int playerExit = MapIsTileInExit(&gMap, &player->thing, exit);
	if (playerExit == -1)
		return -1;
	exit = playerExit;
	CA_FOREACH_END()
	return exit;
}

static bool MoreRescuesNeeded(const struct MissionOptions *mo, const int exit)
{
	int rescuesRequired = 0;
	// Find number of rescues required
	// TODO: support multiple rescue objectives
	CA_FOREACH(const Objective, o, mo->missionData->Objectives)
	if (o->Type == OBJECTIVE_RESCUE)
	{
		rescuesRequired = o->Required;
		break;
	}
	CA_FOREACH_END()
	// Check that enough prisoners are in exit zone
	if (rescuesRequired > 0)
	{
		int prisonersRescued = 0;
		CA_FOREACH(const TActor, a, gActors)
		if (!a->isInUse)
			continue;
		if (CharacterIsPrisoner(
				&gCampaign.Setting.characters, ActorGetCharacter(a)) &&
			MapIsTileInExit(&gMap, &a->thing, exit) == exit)
		{
			prisonersRescued++;
		}
		CA_FOREACH_END()
		if (prisonersRescued < rescuesRequired)
		{
			return true;
		}
	}
	return false;
}

void MissionDone(struct MissionOptions *mo, const NMissionEnd end)
{
	mo->isDone = true;
	mo->DoneCounter = end.Delay;
	gCampaign.IsQuit = end.IsQuit;
	mo->NextMission = end.Mission;
}

int KeycardCount(int flags)
{
	int count = 0;
	if (flags & FLAGS_KEYCARD_RED)
		count++;
	if (flags & FLAGS_KEYCARD_BLUE)
		count++;
	if (flags & FLAGS_KEYCARD_GREEN)
		count++;
	if (flags & FLAGS_KEYCARD_YELLOW)
		count++;
	return count;
}

void MissionStaticAddObjective(
	Mission *m, MissionStatic *ms, const int idx, const int idx2,
	const struct vec2i pos, const bool force)
{
	CASSERT(m->Type == MAPTYPE_STATIC, "mission is not static type");
	if (!force)
	{
		// Remove any objectives already there
		MissionStaticTryRemoveObjective(m, ms, pos);
	}

	// Check if the objective already has an entry, and add to its list
	// of positions
	bool hasAdded = false;
	PositionIndex pi = {pos, idx2};
	for (int i = 0; i < (int)ms->Objectives.size; i++)
	{
		ObjectivePositions *op = CArrayGet(&ms->Objectives, i);
		if (op->Index == idx)
		{
			CArrayPushBack(&op->PositionIndices, &pi);
			hasAdded = true;
			break;
		}
	}
	// If not, create a new entry
	if (!hasAdded)
	{
		ObjectivePositions newOp;
		newOp.Index = idx;
		CArrayInit(&newOp.PositionIndices, sizeof(PositionIndex));
		CArrayPushBack(&newOp.PositionIndices, &pi);
		CArrayPushBack(&ms->Objectives, &newOp);
	}
	// Increase number of objectives
	Objective *o = CArrayGet(&m->Objectives, idx);
	o->Count++;
}
bool MissionStaticTryRemoveObjective(
	Mission *m, MissionStatic *ms, const struct vec2i pos)
{
	CA_FOREACH(ObjectivePositions, op, ms->Objectives)
	for (int j = 0; j < (int)op->PositionIndices.size; j++)
	{
		PositionIndex *pi = CArrayGet(&op->PositionIndices, j);
		if (svec2i_is_equal(pi->Position, pos))
		{
			CArrayDelete(&op->PositionIndices, j);
			// Decrease number of objectives
			Objective *o = CArrayGet(&m->Objectives, op->Index);
			o->Count--;
			CASSERT(o->Count >= 0, "removing unknown objective");
			if (op->PositionIndices.size == 0)
			{
				CArrayTerminate(&op->PositionIndices);
				CArrayDelete(&ms->Objectives, _ca_index);
			}
			return true;
		}
	}
	CA_FOREACH_END()
	return false;
}
