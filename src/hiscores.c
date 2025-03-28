/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.
	Copyright (c) 2025 Cong Xu
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

#include <cdogs/draw/draw_actor.h>
#include <cdogs/events.h>
#include <cdogs/files.h>
#include <cdogs/font.h>
#include <cdogs/log.h>
#include <cdogs/yajl_utils.h>

#include "base64/base64.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#define MAX_ENTRY 20

typedef struct
{
	char *Name;
	time_t Time;
	Character Character;
	NPlayerStats Stats;
	WeaponUsages WeaponUsages;
} HighScoreEntry;
// High score structure is:
// campaign name > entries, sorted by score
// TODO: merge with autosaves
static map_t LoadHighScores(void);
static void SaveHighScores(map_t highScores);

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
		map_t highScores = LoadHighScores();
		CArray *scores = NULL;
		if (hashmap_get(highScores, hData.co->Entry.Path, (any_t *)&scores) ==
			MAP_MISSING)
		{
			CMALLOC(scores, sizeof *scores);
			CArrayInit(scores, sizeof(HighScoreEntry));
			hashmap_put(highScores, hData.co->Entry.Path, scores);
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
		SaveHighScores(highScores);

		// Show high scores screen if high enough
		if (allTime)
		{
			LoopRunnerPush(l, DisplayAllTimeHighScores(hData.g, scores));
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
	CharacterCopy(&entry->Character, &data->Char, NULL);
	entry->Stats = data->Totals;
	entry->WeaponUsages = hashmap_copy(data->WeaponUsages, NULL);
	return true;
}

static void DisplayAt(int x, int y, const char *s, int hilite)
{
	color_t mask = hilite ? colorRed : colorWhite;
	FontStrMask(s, svec2i(x, y), mask);
}

#define INDEX_OFFSET 0
#define SCORE_OFFSET (INDEX_OFFSET + 25)
#define TIME_OFFSET (SCORE_OFFSET + 30)
#define KILLS_OFFSET (TIME_OFFSET + 20)
#define ACC_OFFSET (KILLS_OFFSET + 20)
#define FAV_WEP_OFFSET (ACC_OFFSET + 30)
#define FACE_OFFSET (FAV_WEP_OFFSET + 10)
#define NAME_OFFSET (FACE_OFFSET + 10)

static int DisplayEntry(
	const int x, const int y, const int idx, const HighScoreEntry *e,
	const bool hilite)
{
	char s[10];

	sprintf(s, "%d.", idx + 1);
	DisplayAt(x + INDEX_OFFSET - FontStrW(s), y, s, hilite);
	sprintf(s, "%d", (int)e->Stats.Score);
	DisplayAt(x + SCORE_OFFSET - FontStrW(s), y, s, hilite);
	const int timeSeconds = e->Stats.TimeTicks / FPS_FRAMELIMIT;
	const int centiseconds =
		(e->Stats.TimeTicks - timeSeconds * FPS_FRAMELIMIT) * 100 /
		FPS_FRAMELIMIT;
	sprintf(
		s, "%d:%02d.%02d", timeSeconds / 60, timeSeconds % 60, centiseconds);
	DisplayAt(x + TIME_OFFSET - FontStrW(s), y, s, hilite);
	sprintf(s, "%d", (int)e->Stats.Kills);
	DisplayAt(x + KILLS_OFFSET - FontStrW(s), y, s, hilite);
	sprintf(s, "%d%%", (int)(WeaponUsagesGetAccuracy(e->WeaponUsages) * 100));
	DisplayAt(x + ACC_OFFSET - FontStrW(s), y, s, hilite);
	const WeaponClass *wc = WeaponUsagesGetFavorite(e->WeaponUsages);
	if (wc && wc->Icon)
	{
		PicRender(
			wc->Icon, gGraphicsDevice.gameWindow.renderer,
			svec2i(x + FAV_WEP_OFFSET - 15, y), colorWhite, 0, svec2_one(),
			SDL_FLIP_NONE, Rect2iZero());
	}
	DrawHead(
		gGraphicsDevice.gameWindow.renderer, &e->Character, DIRECTION_DOWN,
		svec2i(x + FACE_OFFSET, y + 4));
	DisplayAt(x + NAME_OFFSET, y, e->Name, hilite);

	return 1 + FontH();
}

static void DisplayPage(const CArray *entries)
{
	int x = 60;
	int y = 5 + FontH();

	FontStr("High Scores:", svec2i(5, 5));

	// Display headings
	const char *s;
	s = "Rank";
	DisplayAt(x + INDEX_OFFSET - FontStrW(s), y, s, false);
	s = "Score";
	DisplayAt(x + SCORE_OFFSET - FontStrW(s), y, s, false);
	s = "Time";
	DisplayAt(x + TIME_OFFSET - FontStrW(s), y, s, false);
	s = "Kills";
	DisplayAt(x + KILLS_OFFSET - FontStrW(s), y, s, false);
	s = "Acc.";
	DisplayAt(x + ACC_OFFSET - FontStrW(s), y, s, false);
	s = "Fav.Gun";
	DisplayAt(x + FAV_WEP_OFFSET - FontStrW(s), y, s, false);
	s = "Name";
	DisplayAt(x + NAME_OFFSET, y, s, false);
	y += 5 + FontH();

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

	int idx = 0;
	CA_FOREACH(const HighScoreEntry, e, *entries)
	bool isHighlighted = false;
	for (int i = 0; i < MAX_LOCAL_PLAYERS; i++)
	{
		if (e->Stats.Score == localScores[i])
		{
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
}
static void HighScoreTerminate(GameLoopData *data);
static GameLoopResult HighScoreUpdate(GameLoopData *data, LoopRunner *l);
static void HighScoreDraw(GameLoopData *data);
static GameLoopData *DisplayAllTimeHighScores(
	GraphicsDevice *graphics, CArray *scores)
{
	HighScoresData *data;
	CMALLOC(data, sizeof *data);
	data->g = graphics;
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
	HighScoresDataTerminate(hData);
	CFREE(hData);
}
static void HighScoreDraw(GameLoopData *data)
{
	HighScoresData *hData = data->Data;

	HighScoresDraw(hData);
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

typedef struct
{
	yajl_gen g;
	map_t highScores;
} SaveHighScoresData;
static int SaveWeaponUsage(any_t data, any_t item);
static int SaveHighScoreEntries(any_t data, any_t key)
{
	SaveHighScoresData *sData = data;
	yajl_gen g = sData->g;
	CArray *entries;
	const int error =
		hashmap_get(sData->highScores, (const char *)key, (any_t *)&entries);
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

	// Save entry as a base64-encoded YAJL string
	yajl_gen g2 = yajl_gen_alloc(NULL);
	if (g2 == NULL)
	{
		LOG(LM_MAIN, LL_ERROR,
			"Unable to alloc JSON generator for saving high score entry\n");
		return MAP_MISSING;
	}
	YAJL_CHECK(yajl_gen_map_open(g2));

	YAJL_CHECK(YAJLAddStringPair(g2, "Name", entry->Name));
	YAJL_CHECK(YAJLAddIntPair(g2, "Time", (int)entry->Time));

	YAJL_CHECK(yajl_gen_string(
		g2, (const unsigned char *)"Character", strlen("Character")));
	if (!CharacterSave(g2, &entry->Character))
	{
		return MAP_MISSING;
	}

	YAJL_CHECK(
		yajl_gen_string(g2, (const unsigned char *)"Stats", strlen("Stats")));
	YAJL_CHECK(yajl_gen_map_open(g2));
	YAJL_CHECK(YAJLAddIntPair(g2, "Score", entry->Stats.Score));
	YAJL_CHECK(YAJLAddIntPair(g2, "Kills", entry->Stats.Kills));
	YAJL_CHECK(YAJLAddIntPair(g2, "Suicides", entry->Stats.Suicides));
	YAJL_CHECK(YAJLAddIntPair(g2, "Friendlies", entry->Stats.Friendlies));
	YAJL_CHECK(YAJLAddIntPair(g2, "TimeTicks", entry->Stats.TimeTicks));
	YAJL_CHECK(yajl_gen_map_close(g2));

	YAJL_CHECK(yajl_gen_string(
		g2, (const unsigned char *)"WeaponUsages", strlen("WeaponUsages")));
	YAJL_CHECK(yajl_gen_map_open(g2));
	const int res = hashmap_iterate(entry->WeaponUsages, SaveWeaponUsage, g2);
	if (res != MAP_OK)
	{
		return res;
	}
	YAJL_CHECK(yajl_gen_map_close(g2));

	YAJL_CHECK(yajl_gen_map_close(g2));

	// Convert to base64 string
	const char *buf;
	size_t len;
	yajl_gen_get_buf(g2, (const unsigned char **)&buf, &len);
	char *encodeOut;
	CMALLOC(encodeOut, BASE64_ENCODE_OUT_SIZE(len));
	const unsigned encodeLen = base64_encode((const unsigned char *)buf, (unsigned int)len, encodeOut);
	YAJL_CHECK(
		yajl_gen_string(g, (const unsigned char *)encodeOut, encodeLen));
	if (g2)
	{
		yajl_gen_clear(g2);
		yajl_gen_free(g2);
	}
	CFREE(encodeOut);
	CA_FOREACH_END()
	YAJL_CHECK(yajl_gen_array_close(g));
#undef YAJL_CHECK
	return MAP_OK;
}
static int SaveWeaponUsage(any_t data, any_t item)
{
	yajl_gen g = data;
	NWeaponUsage *wu = item;
#define YAJL_CHECK(func)                                                      \
	{                                                                         \
		yajl_gen_status _status = func;                                       \
		if (_status != yajl_gen_status_ok)                                    \
		{                                                                     \
			LOG(LM_MAIN, LL_ERROR,                                            \
				"JSON generator error for high score entry %s: %d\n",         \
				(const char *)wu->Weapon, (int)_status);                      \
			return MAP_MISSING;                                               \
		}                                                                     \
	}
	YAJL_CHECK(yajl_gen_string(
		g, (const unsigned char *)wu->Weapon, strlen(wu->Weapon)));
	YAJL_CHECK(yajl_gen_map_open(g));
	YAJL_CHECK(YAJLAddIntPair(g, "Shots", (int)wu->Shots));
	YAJL_CHECK(YAJLAddIntPair(g, "Hits", (int)wu->Hits));
	YAJL_CHECK(yajl_gen_map_close(g));
#undef YAJL_CHECK
	return MAP_OK;
}
static void SaveHighScores(map_t highScores)
{
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
	SaveHighScoresData sData = {g, highScores};
	if (hashmap_iterate_keys_sorted(
			highScores, SaveHighScoreEntries, (any_t *)&sData) != MAP_OK)
	{
		LOG(LM_MAIN, LL_ERROR, "Failed to generate high score entries\n");
		goto bail;
	}
	YAJL_CHECK(yajl_gen_map_close(g));

	const char *path = GetConfigFilePath(SCORES_FILE);
	if (!YAJLTrySaveJSONFile(g, path))
	{
		goto bail;
	}

bail:
	if (g)
	{
		yajl_gen_clear(g);
		yajl_gen_free(g);
	}
}

static map_t LoadHighScores(void)
{
	map_t highScores = hashmap_new();
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
		yajl_array entriesNode = YAJL_GET_ARRAY(node->u.object.values[i]);
		const char *entriesPath = node->u.object.keys[i];
		if (!entriesNode)
		{
			LOG(LM_MAIN, LL_ERROR, "Unexpected format for high scores %s\n",
				entriesPath);
			continue;
		}
		CArray *entries;
		CMALLOC(entries, sizeof *entries);
		CArrayInit(entries, sizeof(HighScoreEntry));
		for (int j = 0; j < (int)entriesNode->len; j++)
		{
			// Decode base64-encoded entries
			const char *entryEncoded = YAJL_GET_STRING(entriesNode->values[j]);
			char *decodeOut;
			const size_t decodeLen = strlen(entryEncoded);
			CMALLOC(decodeOut, BASE64_DECODE_OUT_SIZE(decodeLen));
			base64_decode(entryEncoded, (unsigned int)decodeLen, (unsigned char *)decodeOut);
			char errbuf[1024];
			yajl_val entryNode =
				yajl_tree_parse(decodeOut, errbuf, sizeof errbuf);
			if (entryNode == NULL)
			{
				LOG(LM_MAIN, LL_ERROR,
					"Unexpected format for high score entry %s %d\n", path, j);
				continue;
			}
			CFREE(decodeOut);

			HighScoreEntry entry;
			memset(&entry, 0, sizeof entry);
			entry.WeaponUsages = WeaponUsagesNew();
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
			char *tmp = YAJLGetStr(charNode, "Class");
			entry.Character.Class = StrCharacterClass(tmp);
			CFREE(tmp);
			YAJLStr(
				&entry.Character.HeadParts[HEAD_PART_HAIR], charNode,
				"HairType");
			YAJLStr(
				&entry.Character.HeadParts[HEAD_PART_FACEHAIR], charNode,
				"FacehairType");
			YAJLStr(
				&entry.Character.HeadParts[HEAD_PART_HAT], charNode,
				"HatType");
			YAJLStr(
				&entry.Character.HeadParts[HEAD_PART_GLASSES], charNode,
				"GlassesType");
			YAJLLoadColor(&entry.Character.Colors.Skin, charNode, "Skin");
			YAJLLoadColor(&entry.Character.Colors.Arms, charNode, "Arms");
			YAJLLoadColor(&entry.Character.Colors.Body, charNode, "Body");
			YAJLLoadColor(&entry.Character.Colors.Legs, charNode, "Legs");
			YAJLLoadColor(&entry.Character.Colors.Hair, charNode, "Hair");
			YAJLLoadColor(&entry.Character.Colors.Feet, charNode, "Feet");
			YAJLLoadColor(
				&entry.Character.Colors.Facehair, charNode, "Facehair");
			YAJLLoadColor(&entry.Character.Colors.Hat, charNode, "Hat");
			YAJLLoadColor(
				&entry.Character.Colors.Glasses, charNode, "Glasses");

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

			const char *wuPath[] = {"WeaponUsages", NULL};
			yajl_val wuNode = yajl_tree_get(entryNode, wuPath, yajl_t_object);
			if (!wuNode || !YAJL_IS_OBJECT(wuNode))
			{
				LOG(LM_MAIN, LL_ERROR,
					"Unexpected format for high score entry %s %d\n", path, j);
				continue;
			}
			for (int k = 0; k < (int)wuNode->u.object.len; k++)
			{
				const char *weaponName = wuNode->u.object.keys[k];
				yajl_val usageNode = wuNode->u.object.values[k];
				NWeaponUsage *w = NULL;
				CMALLOC(w, sizeof *w);
				strcpy(w->Weapon, weaponName);
				YAJLInt((int *)&w->Shots, usageNode, "Shots");
				YAJLInt((int *)&w->Hits, usageNode, "Hits");
				if (hashmap_put(entry.WeaponUsages, weaponName, w) != MAP_OK)
				{
					LOG(LM_MAIN, LL_ERROR, "failed to load high scores (%s)",
						path);
					continue;
				}
			}
			CArrayPushBack(entries, &entry);
		}
		if (hashmap_put(highScores, entriesPath, (any_t *)entries) != MAP_OK)
		{
			LOG(LM_MAIN, LL_ERROR, "failed to load high scores (%s)", path);
			continue;
		}
	}

bail:
	yajl_tree_free(node);
	return highScores;
}

HighScoresData HighScoresDataLoad(const Campaign *co, GraphicsDevice *g)
{
	map_t highScores = LoadHighScores();
	HighScoresData hData;
	memset(&hData, 0, sizeof hData);
	hData.g = g;
	CArray *scores = NULL;
	if (hashmap_get(highScores, co->Entry.Path, (any_t *)&scores) !=
		MAP_MISSING)
	{
		// copy scores
		CArrayInit(&hData.scores, sizeof(HighScoreEntry));
		CArrayCopy(&hData.scores, scores);
		CA_FOREACH(HighScoreEntry, entry, hData.scores)
		const HighScoreEntry *src = CArrayGet(scores, _ca_index);
		CSTRDUP(entry->Name, src->Name);
		entry->WeaponUsages = hashmap_copy(src->WeaponUsages, NULL);
		CA_FOREACH_END()
	}
	return hData;
}

void HighScoresDataTerminate(HighScoresData *hData)
{
	CA_FOREACH(HighScoreEntry, entry, hData->scores)
	CFREE(entry->Name);
	WeaponUsagesTerminate(entry->WeaponUsages);
	CA_FOREACH_END()
	CArrayTerminate(&hData->scores);
}

void HighScoresDraw(const HighScoresData *hData)
{
	BlitClearBuf(&gGraphicsDevice);
	DisplayPage(&hData->scores);
	BlitUpdateFromBuf(&gGraphicsDevice, gGraphicsDevice.screen);
}
