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

	Copyright (c) 2013-2014, 2017, 2021, 2025 Cong Xu
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
#include "hiscores.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <cdogs/events.h>
#include <cdogs/files.h>
#include <cdogs/font.h>
#include <cdogs/log.h>
#include <cdogs/yajl_utils.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#define MAX_ENTRY 20

typedef struct
{
	char *Name;
	time_t Time;
	CharacterClass Character;
	CharColors Colors;
	NPlayerStats Stats;
	int Missions;
	int LastMission;
	map_t WeaponUsages; // of name -> usage (TODO: shots, hits)
} HighScoreEntry;
// High score structure is:
// campaign name > entries, sorted by score
static map_t sHighScores; // TODO: merge with autosaves

static void LoadHighScores(void);

// Get player high score if it applies, or 0 otherwise
static int GetPlayerHighScore(const Campaign *co, const PlayerData *p)
{
	const bool isPlayerComplete =
		(!co->IsQuit && !p->survived) || co->IsComplete;
	if (isPlayerComplete && p->IsLocal && IsPlayerHuman(p))
	{
		return p->Totals.Score;
	}
	return 0;
}

// Dummy screen to calculate high scores and switch to high scores screens if
// required
typedef struct
{
	Campaign *co;
	GraphicsDevice *g;
} HighScoresScreenData;
static void HighScoresScreenTerminate(GameLoopData *data);
static GameLoopResult HighScoresScreenUpdate(
	GameLoopData *data, LoopRunner *l);
GameLoopData *HighScoresScreen(Campaign *co, GraphicsDevice *g)
{
	HighScoresScreenData *data;
	CMALLOC(data, sizeof *data);
	data->co = co;
	data->g = g;
	return GameLoopDataNew(
		data, HighScoresScreenTerminate, NULL, NULL, NULL,
		HighScoresScreenUpdate, NULL);
}
static void HighScoresScreenTerminate(GameLoopData *data)
{
	HighScoresScreenData *hData = data->Data;
	CFREE(hData);
}
static bool EnterHighScore(const PlayerData *data, CArray *scores);
static GameLoopData *DisplayAllTimeHighScores(
	GraphicsDevice *graphics, CArray *scores);
static GameLoopResult HighScoresScreenUpdate(GameLoopData *data, LoopRunner *l)
{
	// Make copy before popping loop
	HighScoresScreenData hData = *(HighScoresScreenData *)data->Data;
	LoopRunnerPop(l);
	if (!IsPVP(hData.co->Entry.Mode) &&
		GetNumPlayers(PLAYER_ANY, false, true) > 0)
	{
		LoadHighScores();
		CArray *scores = NULL;
		if (hashmap_get(sHighScores, hData.co->Entry.Path, (any_t *)&scores) ==
			MAP_MISSING)
		{
			CMALLOC(scores, sizeof *scores);
			CArrayInit(scores, sizeof(HighScoreEntry));
			hashmap_put(sHighScores, hData.co->Entry.Path, scores);
		}

		bool allTime = false;
		CA_FOREACH(PlayerData, p, gPlayerDatas)
		if (GetPlayerHighScore(hData.co, p) > 0)
		{
			if (EnterHighScore(p, scores))
			{
				allTime = true;
			}
		}

		if (!p->survived)
		{
			// Reset scores because we died :(
			memset(&p->Totals, 0, sizeof p->Totals);
			p->missions = 0;
		}
		else
		{
			p->missions++;
		}
		p->lastMission = hData.co->MissionIndex;
		CA_FOREACH_END()
		SaveHighScores();

		// Show high scores screen if high enough
		if (allTime)
		{
			LoopRunnerPush(l, DisplayAllTimeHighScores(hData.g, scores));
		}
		else
		{
			// TODO: terminate high scores
		}
	}

	return UPDATE_RESULT_OK;
}

static bool EnterHighScore(const PlayerData *data, CArray *scores)
{
	// Find insertion point based on score
	HighScoreEntry *entry = NULL;
	CA_FOREACH(const HighScoreEntry, e, *scores)
	if (e->Stats.Score <= data->Totals.Score)
	{
		HighScoreEntry new;
		memset(&new, 0, sizeof new);
		entry = CArrayInsert(scores, _ca_index, &new);
		break;
	}
	CA_FOREACH_END()
	if (entry == NULL)
	{
		if (scores->size >= MAX_ENTRY)
		{
			// score not high enough; exit
			return false;
		}
		HighScoreEntry new;
		memset(&new, 0, sizeof new);
		entry = CArrayPushBack(scores, &new);
	}
	// TODO: get existing entry
	CSTRDUP(entry->Name, data->name);
	CharacterClassCopy(&entry->Character, data->Char.Class);
	entry->Colors = data->Char.Colors;
	entry->Stats = data->Totals;
	entry->Missions = data->missions;
	entry->LastMission = data->lastMission;
	entry->WeaponUsages = hashmap_copy(data->WeaponUsages, NULL);
	return true;
}

static void DisplayAt(int x, int y, const char *s, int hilite)
{
	color_t mask = hilite ? colorRed : colorWhite;
	FontStrMask(s, svec2i(x, y), mask);
}

static int DisplayEntry(
	const int x, const int y, const int idx, const HighScoreEntry *e,
	const bool hilite)
{
	char s[10];

#define INDEX_OFFSET 15
#define SCORE_OFFSET 40
#define MISSIONS_OFFSET 60
#define MISSION_OFFSET 80
#define NAME_OFFSET 85

	sprintf(s, "%d.", idx + 1);
	DisplayAt(x + INDEX_OFFSET - FontStrW(s), y, s, hilite);
	sprintf(s, "%d", (int)e->Stats.Score);
	DisplayAt(x + SCORE_OFFSET - FontStrW(s), y, s, hilite);
	sprintf(s, "%d", e->Missions);
	DisplayAt(x + MISSIONS_OFFSET - FontStrW(s), y, s, hilite);
	sprintf(s, "(%d)", e->LastMission + 1);
	DisplayAt(x + MISSION_OFFSET - FontStrW(s), y, s, hilite);
	DisplayAt(x + NAME_OFFSET, y, e->Name, hilite);
	// TODO: show other columns: kills, time, favourite weapon, character body

	return 1 + FontH();
}

static int DisplayPage(const int idxStart, const CArray *entries)
{
	int x = 80;
	int y = 5 + FontH();

	FontStr("High Scores:", svec2i(5, 5));

	// Find all the high scores for local players
	int localScores[MAX_LOCAL_PLAYERS];
	memset(localScores, 0, sizeof localScores);
	int pIdx = 0;
	CA_FOREACH(const PlayerData, p, gPlayerDatas)
	const int score = GetPlayerHighScore(&gCampaign, p);
	if (score > 0)
	{
		localScores[pIdx] = score;
		pIdx++;
	}
	CA_FOREACH_END()

	int idx = idxStart;
	CA_FOREACH(const HighScoreEntry, e, *entries)
	bool isHighlighted = false;
	for (int i = 0; i < MAX_LOCAL_PLAYERS; i++)
	{
		if (e->Stats.Score == localScores[i])
		{
			// TODO: don't just highlight, display character next to entry
			isHighlighted = true;
			localScores[i] = 0;
			break;
		}
	}
	y += DisplayEntry(x, y, idx, e, isHighlighted);
	if (y > 198 - FontH())
	{
		y = 20;
		x += 100;
	}
	idx++;
	CA_FOREACH_END()
	return idx;
}
typedef struct
{
	GraphicsDevice *g;
	CArray scores;	//  of HighScoreEntry
	int highlights[MAX_LOCAL_PLAYERS];
	int scoreIdx;
} HighScoresData;
static void HighScoreTerminate(GameLoopData *data);
static GameLoopResult HighScoreUpdate(GameLoopData *data, LoopRunner *l);
static void HighScoreDraw(GameLoopData *data);
static GameLoopData *DisplayAllTimeHighScores(
	GraphicsDevice *graphics, CArray *scores)
{
	HighScoresData *data;
	CMALLOC(data, sizeof *data);
	data->g = graphics;
	data->scoreIdx = 0;
	// Take a copy of the scores as the parent will get destroyed
	CArrayInit(&data->scores, sizeof(HighScoreEntry));
	CArrayCopy(&data->scores, scores);
	return GameLoopDataNew(
		data, HighScoreTerminate, NULL, NULL, NULL, HighScoreUpdate,
		HighScoreDraw);
}
static void HighScoreTerminate(GameLoopData *data)
{
	HighScoresData *hData = data->Data;

	CArrayTerminate(&hData->scores);
	// TODO: terminate scores entries
	CFREE(hData);
}
static void HighScoreDraw(GameLoopData *data)
{
	HighScoresData *hData = data->Data;

	BlitClearBuf(hData->g);
	hData->scoreIdx = DisplayPage(hData->scoreIdx, &hData->scores);
	BlitUpdateFromBuf(hData->g, hData->g->screen);
}
static GameLoopResult HighScoreUpdate(GameLoopData *data, LoopRunner *l)
{
	UNUSED(data);
	const EventWaitResult result = EventWaitForAnyKeyOrButton();

	if (result != EVENT_WAIT_CONTINUE)
	{
		LoopRunnerPop(l);
	}
	return UPDATE_RESULT_OK;
}

static int SaveHighScoreEntries(any_t data, any_t key)
{
	yajl_gen g = (yajl_gen)data;
	CArray *entries;
	const int error =
		hashmap_get(sHighScores, (const char *)key, (any_t *)&entries);
	if (error != MAP_OK)
	{
		CASSERT(false, "cannot find high score entry");
		return error;
	}
#define YAJL_CHECK(func)                                                      \
	{                                                                         \
		yajl_gen_status _status = func;                                       \
		if (_status != yajl_gen_status_ok)                                    \
		{                                                                     \
			LOG(LM_MAIN, LL_ERROR,                                            \
				"JSON generator error for high score %s: %d\n",               \
				(const char *)key, (int)_status);                             \
			return MAP_MISSING;                                               \
		}                                                                     \
	}
	YAJL_CHECK(yajl_gen_string(g, key, strlen(key)));
	YAJL_CHECK(yajl_gen_array_open(g));

	CA_FOREACH(const HighScoreEntry, entry, *entries)
	YAJL_CHECK(yajl_gen_map_open(g));

	YAJL_CHECK(YAJLAddStringPair(g, "Name", entry->Name));
	YAJL_CHECK(YAJLAddIntPair(g, "Time", (int)entry->Time));

	YAJL_CHECK(yajl_gen_string(
		g, (const unsigned char *)"Character", strlen("Character")));
	YAJL_CHECK(yajl_gen_map_open(g));
	YAJL_CHECK(YAJLAddStringPair(g, "Name", entry->Character.Name));
	YAJL_CHECK(YAJLAddStringPair(g, "Head", entry->Character.HeadSprites));
	YAJL_CHECK(YAJLAddStringPair(g, "Body", entry->Character.Body));
	YAJL_CHECK(YAJLAddBoolPair(
		g, "HasHair", entry->Character.HasHeadParts[HEAD_PART_HAIR]));
	YAJL_CHECK(YAJLAddBoolPair(
		g, "HasFacehair", entry->Character.HasHeadParts[HEAD_PART_FACEHAIR]));
	YAJL_CHECK(YAJLAddBoolPair(
		g, "HasHat", entry->Character.HasHeadParts[HEAD_PART_HAT]));
	YAJL_CHECK(YAJLAddBoolPair(
		g, "HasGlasses", entry->Character.HasHeadParts[HEAD_PART_GLASSES]));
	YAJL_CHECK(yajl_gen_map_close(g));

	YAJL_CHECK(
		yajl_gen_string(g, (const unsigned char *)"Colors", strlen("Colors")));
	YAJL_CHECK(yajl_gen_map_open(g));
	YAJL_CHECK(YAJLAddColorPair(g, "Skin", entry->Colors.Skin));
	YAJL_CHECK(YAJLAddColorPair(g, "Arms", entry->Colors.Arms));
	YAJL_CHECK(YAJLAddColorPair(g, "Body", entry->Colors.Body));
	YAJL_CHECK(YAJLAddColorPair(g, "Legs", entry->Colors.Legs));
	YAJL_CHECK(YAJLAddColorPair(g, "Hair", entry->Colors.Hair));
	YAJL_CHECK(YAJLAddColorPair(g, "Feet", entry->Colors.Feet));
	YAJL_CHECK(YAJLAddColorPair(g, "Facehair", entry->Colors.Facehair));
	YAJL_CHECK(YAJLAddColorPair(g, "Hat", entry->Colors.Hat));
	YAJL_CHECK(YAJLAddColorPair(g, "Glasses", entry->Colors.Glasses));
	YAJL_CHECK(yajl_gen_map_close(g));

	// TODO: base64 encode
	YAJL_CHECK(
		yajl_gen_string(g, (const unsigned char *)"Stats", strlen("Stats")));
	YAJL_CHECK(yajl_gen_map_open(g));
	YAJL_CHECK(YAJLAddIntPair(g, "Score", entry->Stats.Score));
	YAJL_CHECK(YAJLAddIntPair(g, "Kills", entry->Stats.Kills));
	YAJL_CHECK(YAJLAddIntPair(g, "Suicides", entry->Stats.Suicides));
	YAJL_CHECK(YAJLAddIntPair(g, "Friendlies", entry->Stats.Friendlies));
	YAJL_CHECK(YAJLAddIntPair(g, "TimeTicks", entry->Stats.TimeTicks));
	YAJL_CHECK(yajl_gen_map_close(g));

	YAJL_CHECK(YAJLAddIntPair(g, "Missions", entry->Missions));
	YAJL_CHECK(YAJLAddIntPair(g, "LastMission", entry->LastMission));

	YAJL_CHECK(yajl_gen_string(
		g, (const unsigned char *)"WeaponUsages", strlen("WeaponUsages")));
	YAJL_CHECK(yajl_gen_map_open(g));
	// TODO: weapon usage
	YAJL_CHECK(yajl_gen_map_close(g));

	YAJL_CHECK(yajl_gen_map_close(g));
	CA_FOREACH_END()
	YAJL_CHECK(yajl_gen_array_close(g));
#undef YAJL_CHECK
	return MAP_OK;
}
void SaveHighScores(void)
{
	FILE *f = NULL;
	yajl_gen g = yajl_gen_alloc(NULL);
	if (g == NULL)
	{
		LOG(LM_MAIN, LL_ERROR,
			"Unable to alloc JSON generator for saving high scores\n");
		goto bail;
	}

#define YAJL_CHECK(func)                                                      \
	if (func != yajl_gen_status_ok)                                           \
	{                                                                         \
		LOG(LM_MAIN, LL_ERROR, "JSON generator error for high scores\n");     \
		goto bail;                                                            \
	}

	YAJL_CHECK(yajl_gen_map_open(g));
	if (hashmap_iterate_keys_sorted(
			sHighScores, SaveHighScoreEntries, (any_t *)g) != MAP_OK)
	{
		LOG(LM_MAIN, LL_ERROR, "Failed to generate high score entries\n");
		goto bail;
	}
	YAJL_CHECK(yajl_gen_map_close(g));

	const char *buf;
	size_t len;
	yajl_gen_get_buf(g, (const unsigned char **)&buf, &len);
	const char *path = GetConfigFilePath(SCORES_FILE);
	f = fopen(path, "w");
	if (f == NULL)
	{
		LOG(LM_MAIN, LL_ERROR, "Unable to save %s\n", path);
		goto bail;
	}
	fwrite(buf, 1, len, f);

#ifdef __EMSCRIPTEN__
	EM_ASM(
		// persist changes
		FS.syncfs(false, function(err) { assert(!err); }););
#endif

bail:
	if (g)
	{
		yajl_gen_clear(g);
		yajl_gen_free(g);
	}
	if (f != NULL)
	{
		fclose(f);
	}
}

static void LoadHighScores(void)
{
	sHighScores = hashmap_new();
	const char *path = GetConfigFilePath(SCORES_FILE);
	yajl_val node = YAJLReadFile(path);
	if (node == NULL)
	{
		LOG(LM_MAIN, LL_INFO, "Error reading high scores '%s'", path);
		goto bail;
	}
	if (!YAJL_IS_OBJECT(node))
	{
		LOG(LM_MAIN, LL_ERROR, "Unexpected format for high scores\n");
		goto bail;
	}
	for (int i = 0; i < (int)node->u.object.len; i++)
	{
		const char *path = node->u.object.keys[i];
		yajl_array entriesNode = YAJL_GET_ARRAY(node->u.object.values[i]);
		if (!entriesNode)
		{
			LOG(LM_MAIN, LL_ERROR, "Unexpected format for high scores %s\n",
				path);
			continue;
		}
		CArray *entries;
		CMALLOC(entries, sizeof *entries);
		CArrayInit(entries, sizeof(HighScoreEntry));
		for (int j = 0; j < (int)entriesNode->len; j++)
		{
			yajl_val entryNode = entriesNode->values[j];
			HighScoreEntry entry;
			YAJLStr(&entry.Name, entryNode, "Name");
			int value;
			YAJLInt(&value, entryNode, "Time");
			entry.Time = (time_t)value;

			const char *charPath[] = {"Character", NULL};
			yajl_val charNode =
				yajl_tree_get(entryNode, charPath, yajl_t_object);
			if (!charNode)
			{
				LOG(LM_MAIN, LL_ERROR,
					"Unexpected format for high score entry %s %d\n", path, j);
				continue;
			}
			YAJLStr(&entry.Character.Name, charNode, "Name");
			YAJLStr(&entry.Character.HeadSprites, charNode, "Head");
			YAJLStr(&entry.Character.Body, charNode, "Body");
			YAJLBool(
				&entry.Character.HasHeadParts[HEAD_PART_HAIR], charNode,
				"HasHair");
			YAJLBool(
				&entry.Character.HasHeadParts[HEAD_PART_FACEHAIR], charNode,
				"HasFacehair");
			YAJLBool(
				&entry.Character.HasHeadParts[HEAD_PART_HAT], charNode,
				"HasHat");
			YAJLBool(
				&entry.Character.HasHeadParts[HEAD_PART_GLASSES], charNode,
				"HasGlasses");

			const char *colorPath[] = {"Colors", NULL};
			yajl_val colorNode =
				yajl_tree_get(entryNode, colorPath, yajl_t_object);
			if (!colorNode)
			{
				LOG(LM_MAIN, LL_ERROR,
					"Unexpected format for high score entry %s %d\n", path, j);
				continue;
			}
			YAJLLoadColor(&entry.Colors.Skin, colorNode, "Skin");
			YAJLLoadColor(&entry.Colors.Arms, colorNode, "Arms");
			YAJLLoadColor(&entry.Colors.Body, colorNode, "Body");
			YAJLLoadColor(&entry.Colors.Legs, colorNode, "Legs");
			YAJLLoadColor(&entry.Colors.Hair, colorNode, "Hair");
			YAJLLoadColor(&entry.Colors.Feet, colorNode, "Feet");
			YAJLLoadColor(&entry.Colors.Facehair, colorNode, "Facehair");
			YAJLLoadColor(&entry.Colors.Hat, colorNode, "Hat");
			YAJLLoadColor(&entry.Colors.Glasses, colorNode, "Glasses");

			// TODO: base64 decode
			const char *statsPath[] = {"Stats", NULL};
			yajl_val statsNode =
				yajl_tree_get(entryNode, statsPath, yajl_t_object);
			if (!statsNode)
			{
				LOG(LM_MAIN, LL_ERROR,
					"Unexpected format for high score entry %s %d\n", path, j);
				continue;
			}
			YAJLInt((int *)&entry.Stats.Score, statsNode, "Score");
			YAJLInt((int *)&entry.Stats.Kills, statsNode, "Kills");
			YAJLInt((int *)&entry.Stats.Suicides, statsNode, "Suicides");
			YAJLInt((int *)&entry.Stats.Friendlies, statsNode, "Friendlies");
			YAJLInt((int *)&entry.Stats.TimeTicks, statsNode, "TimeTicks");

			YAJLInt(&entry.Missions, entryNode, "Missions");
			YAJLInt(&entry.LastMission, entryNode, "LastMission");

			const char *wuPath[] = {"WeaponUsages", NULL};
			yajl_val wuNode = yajl_tree_get(entryNode, wuPath, yajl_t_object);
			if (!wuNode)
			{
				LOG(LM_MAIN, LL_ERROR,
					"Unexpected format for high score entry %s %d\n", path, j);
				continue;
			}
			// TODO: weapon usage
			CArrayPushBack(entries, &entry);
		}
		if (hashmap_put(sHighScores, path, (any_t *)entries) != MAP_OK)
		{
			LOG(LM_MAIN, LL_ERROR, "failed to load high scores (%s)", path);
			continue;
		}
	}

bail:
	yajl_tree_free(node);
}

// TODO: terminate high scores
