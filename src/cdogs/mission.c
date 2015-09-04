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

    Copyright (c) 2013-2015, Cong Xu
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
#include <string.h>
#include <stdlib.h>

#include "door.h"
#include "files.h"
#include "game_events.h"
#include "gamedata.h"
#include "map.h"
#include "map_new.h"
#include "objs.h"
#include "palette.h"
#include "particle.h"
#include "pickup.h"
#include "defs.h"
#include "pic_manager.h"
#include "actors.h"
#include "triggers.h"


int StrKeycard(const char *s)
{
	S2T(FLAGS_KEYCARD_YELLOW, "yellow");
	S2T(FLAGS_KEYCARD_GREEN, "green");
	S2T(FLAGS_KEYCARD_BLUE, "blue");
	S2T(FLAGS_KEYCARD_RED, "red");
	return 0;
}

const char *MapTypeStr(MapType t)
{
	switch (t)
	{
	case MAPTYPE_CLASSIC:
		return "Classic";
	case MAPTYPE_STATIC:
		return "Static";
	default:
		return "";
	}
}
MapType StrMapType(const char *s)
{
	if (strcmp(s, "Classic") == 0)
	{
		return MAPTYPE_CLASSIC;
	}
	else if (strcmp(s, "Static") == 0)
	{
		return MAPTYPE_STATIC;
	}
	return MAPTYPE_CLASSIC;
}


void MissionInit(Mission *m)
{
	memset(m, 0, sizeof *m);
	m->WallMask = colorBattleshipGrey;
	m->FloorMask = colorGravel;
	m->RoomMask = colorDoveGray;
	m->AltMask = colorOfficeGreen;
	CArrayInit(&m->Objectives, sizeof(MissionObjective));
	CArrayInit(&m->Enemies, sizeof(int));
	CArrayInit(&m->SpecialChars, sizeof(int));
	CArrayInit(&m->MapObjectDensities, sizeof(MapObjectDensity));
	CArrayInit(&m->Weapons, sizeof(const GunDescription *));
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
	dst->Type = src->Type;
	dst->Size = src->Size;

	dst->WallStyle = src->WallStyle;
	dst->FloorStyle = src->FloorStyle;
	dst->RoomStyle = src->RoomStyle;
	dst->ExitStyle = src->ExitStyle;
	dst->KeyStyle = src->KeyStyle;
	strcpy(dst->DoorStyle, src->DoorStyle);

	CArrayCopy(&dst->Objectives, &src->Objectives);
	for (int i = 0; i < (int)src->Objectives.size; i++)
	{
		MissionObjective *smo = CArrayGet(&src->Objectives, i);
		MissionObjective *dmo = CArrayGet(&dst->Objectives, i);
		if (smo->Description)
		{
			CSTRDUP(dmo->Description, smo->Description);
		}
	}
	CArrayCopy(&dst->Enemies, &src->Enemies);
	CArrayCopy(&dst->SpecialChars, &src->SpecialChars);
	CArrayCopy(&dst->MapObjectDensities, &src->MapObjectDensities);

	dst->EnemyDensity = src->EnemyDensity;
	CArrayCopy(&dst->Weapons, &src->Weapons);

	memcpy(dst->Song, src->Song, sizeof dst->Song);

	dst->WallMask = src->WallMask;
	dst->FloorMask = src->FloorMask;
	dst->RoomMask = src->RoomMask;
	dst->AltMask = src->AltMask;

	switch (dst->Type)
	{
	case MAPTYPE_STATIC:
		CArrayCopy(&dst->u.Static.Tiles, &src->u.Static.Tiles);
		CArrayCopy(&dst->u.Static.Items, &src->u.Static.Items);
		CArrayCopy(&dst->u.Static.Wrecks, &src->u.Static.Wrecks);
		CArrayCopy(&dst->u.Static.Characters, &src->u.Static.Characters);
		CArrayCopy(&dst->u.Static.Objectives, &src->u.Static.Objectives);
		CArrayCopy(&dst->u.Static.Keys, &src->u.Static.Keys);

		dst->u.Static.Start = src->u.Static.Start;
		dst->u.Static.Exit = src->u.Static.Exit;
		break;
	default:
		memcpy(&dst->u, &src->u, sizeof dst->u);
		break;
	}
}
void MissionTerminate(Mission *m)
{
	if (m == NULL) return;
	CFREE(m->Title);
	CFREE(m->Description);
	CA_FOREACH(MissionObjective, mo, m->Objectives)
		CFREE(mo->Description);
	CA_FOREACH_END()
	CArrayTerminate(&m->Objectives);
	CArrayTerminate(&m->Enemies);
	CArrayTerminate(&m->SpecialChars);
	CArrayTerminate(&m->MapObjectDensities);
	CArrayTerminate(&m->Weapons);
	switch (m->Type)
	{
	case MAPTYPE_CLASSIC:
		break;
	case MAPTYPE_STATIC:
		CArrayTerminate(&m->u.Static.Tiles);
		CArrayTerminate(&m->u.Static.Items);
		CArrayTerminate(&m->u.Static.Wrecks);
		CArrayTerminate(&m->u.Static.Characters);
		CArrayTerminate(&m->u.Static.Objectives);
		CArrayTerminate(&m->u.Static.Keys);
		break;
	}
}


// +-------------+
// |  Exit info  |
// +-------------+


/*static int exitPics[] = {
	375, 376,	// hazard stripes
	380, 381	// yellow plates
};*/
// TODO: arbitrary exit tile names
static const char *exitPicNames[] = {
	"hazard",
	"plate"
};

// Every exit has TWO pics, so actual # of exits == # pics / 2!
//#define EXIT_COUNT (sizeof( exitPics)/sizeof( int)/2)
#define EXIT_COUNT 2
int GetExitCount(void) { return EXIT_COUNT; }


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

	CA_FOREACH(const MissionObjective, mobj, mission->Objectives)
		if (mobj->Type == OBJECTIVE_RESCUE)
		{
			CharacterStoreAddPrisoner(s, mobj->Index);
			break;	// TODO: multiple prisoners
		}
	CA_FOREACH_END()

	CA_FOREACH(int, e, mission->Enemies)
		CharacterStoreAddBaddie(s, *e);
	CA_FOREACH_END()

	CA_FOREACH(int, sc, mission->SpecialChars)
		CharacterStoreAddSpecial(s, *sc);
	CA_FOREACH_END()
}

static void SetupObjectives(struct MissionOptions *mo, Mission *mission)
{
	CA_FOREACH(const MissionObjective, mobj, mission->Objectives)
		ObjectiveDef o;
		memset(&o, 0, sizeof o);
		assert(i < OBJECTIVE_MAX_OLD);
		// Set objective colours based on type
		o.color = ObjectiveTypeColor(mobj->Type);
		o.blowupObject = IntMapObject(mobj->Index);
		o.pickupClass = IntPickupClass(mobj->Index);
		CArrayPushBack(&mo->Objectives, &o);
	CA_FOREACH_END()
}

static void SetupWeapons(CArray *to, CArray *from)
{
	CArrayCopy(to, from);
}

void SetupMission(
	int buildTables, Mission *m, struct MissionOptions *mo, int missionIndex)
{
	MissionOptionsInit(mo);
	mo->index = missionIndex;
	mo->missionData = m;
	mo->keyStyle = m->KeyStyle;

	char exitPicBuf[256];
	const int exitIdx = abs(m->ExitStyle) % EXIT_COUNT;
	sprintf(exitPicBuf, "exit_%s", exitPicNames[exitIdx]);
	mo->exitPic = PicManagerGetNamedPic(&gPicManager, exitPicBuf);
	sprintf(exitPicBuf, "exit_%s_shadow", exitPicNames[exitIdx]);
	mo->exitShadow = PicManagerGetNamedPic(&gPicManager, exitPicBuf);

	ActorsInit();
	ObjsInit();
	MobObjsInit();
	PickupsInit();
	ParticlesInit(&gParticles);
	WatchesInit();
	SetupObjectives(mo, m);
	SetupBadguysForMission(m);
	SetupWeapons(&mo->Weapons, &m->Weapons);
	if (buildTables)
	{
		BuildTranslationTables(gPicManager.palette);
	}
}

void MissionEnd(void)
{
	ActorsTerminate();
	ObjsTerminate();
	MobObjsTerminate();
	PickupsTerminate();
	ParticlesTerminate(&gParticles);
	WatchesTerminate();
	CA_FOREACH(PlayerData, p, gPlayerDatas)
		p->ActorUID = -1;
	CA_FOREACH_END()
	gMission.HasStarted = false;
}

void MissionSetMessageIfComplete(struct MissionOptions *options)
{
	if (!gCampaign.IsClient && CanCompleteMission(options))
	{
		GameEvent msg = GameEventNew(GAME_EVENT_MISSION_COMPLETE);
		msg.u.MissionComplete.ShowMsg = MissionHasRequiredObjectives(options);
		GameEventsEnqueue(&gGameEvents, msg);
	}
}
bool MissionHasRequiredObjectives(const struct MissionOptions *mo)
{
	CA_FOREACH(const MissionObjective, o, mo->missionData->Objectives)
		if (o->Required > 0) return true;
	CA_FOREACH_END()
	return false;
}

void UpdateMissionObjective(
	const struct MissionOptions *options,
	const int flags, const ObjectiveType type)
{
	if (!(flags & TILEITEM_OBJECTIVE))
	{
		return;
	}
	int idx = ObjectiveFromTileItem(flags);
	MissionObjective *mobj = CArrayGet(&options->missionData->Objectives, idx);
	if (mobj->Type != type)
	{
		return;
	}
	if (!gCampaign.IsClient)
	{
		GameEvent e = GameEventNew(GAME_EVENT_OBJECTIVE_UPDATE);
		e.u.ObjectiveUpdate.ObjectiveId = idx;
		e.u.ObjectiveUpdate.Count = 1;
		GameEventsEnqueue(&gGameEvents, e);
	}
}

bool CanCompleteMission(const struct MissionOptions *options)
{
	// Death is the only escape from PVP and quick play
	if (IsPVP(gCampaign.Entry.Mode))
	{
		return GetNumPlayers(PLAYER_ALIVE_OR_DYING, false, false) <= 1;
	}
	else if (gCampaign.Entry.Mode == GAME_MODE_QUICK_PLAY)
	{
		return GetNumPlayers(PLAYER_ALIVE_OR_DYING, false, false) == 0;
	}

	// Check all objective counts are enough
	CA_FOREACH(const ObjectiveDef, o, options->Objectives)
		const MissionObjective *mobj =
			CArrayGet(&options->missionData->Objectives, i);
		if (o->done < mobj->Required) return false;
	CA_FOREACH_END()

	return true;
}

bool IsMissionComplete(const struct MissionOptions *options)
{
	int rescuesRequired = 0;

	if (!CanCompleteMission(options))
	{
		return 0;
	}

	// Check if dogfight is complete
	if (IsPVP(gCampaign.Entry.Mode) &&
		GetNumPlayers(PLAYER_ALIVE_OR_DYING, false, false) <= 1)
	{
		// Also check that only one player has lives left
		int numPlayersWithLives = 0;
		CA_FOREACH(const PlayerData, p, gPlayerDatas)
			if (p->Lives > 0) numPlayersWithLives++;
		CA_FOREACH_END()
		if (numPlayersWithLives <= 1)
		{
			return true;
		}
	}

	// Check that all surviving players are in exit zone
	CA_FOREACH(const PlayerData, p, gPlayerDatas)
		if (!IsPlayerAlive(p)) continue;
		const TActor *player = ActorGetByUID(p->ActorUID);
		if (!MapIsTileInExit(&gMap, &player->tileItem)) return false;
	CA_FOREACH_END()

	// Find number of rescues required
	// TODO: support multiple rescue objectives
	CA_FOREACH(const MissionObjective, mobj, options->missionData->Objectives)
		if (mobj->Type == OBJECTIVE_RESCUE)
		{
			rescuesRequired = mobj->Required;
			break;
		}
	CA_FOREACH_END()
	// Check that enough prisoners are in exit zone
	if (rescuesRequired > 0)
	{
		int prisonersRescued = 0;
		CA_FOREACH(const TActor, a, gActors)
			if (!a->isInUse) continue;
			if (CharacterIsPrisoner(&gCampaign.Setting.characters, ActorGetCharacter(a)) &&
				MapIsTileInExit(&gMap, &a->tileItem))
			{
				prisonersRescued++;
			}
		CA_FOREACH_END()
		if (prisonersRescued < rescuesRequired)
		{
			return 0;
		}
	}

	return 1;
}

int KeycardCount(int flags)
{
	int count = 0;
	if (flags & FLAGS_KEYCARD_RED) count++;
	if (flags & FLAGS_KEYCARD_BLUE) count++;
	if (flags & FLAGS_KEYCARD_GREEN) count++;
	if (flags & FLAGS_KEYCARD_YELLOW) count++;
	return count;
}


struct EditorInfo GetEditorInfo(void)
{
	struct EditorInfo ei;
	ei.keyCount = KEYSTYLE_COUNT;
	ei.exitCount = EXIT_COUNT;
	return ei;
}
