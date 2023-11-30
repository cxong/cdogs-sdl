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

	Copyright (c) 2013-2014, 2017, 2021 Cong Xu
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

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

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
static GameLoopResult HighScoresScreenUpdate(GameLoopData *data, LoopRunner *l)
{
	// Make copy before popping loop
	HighScoresScreenData hData = *(HighScoresScreenData *)data->Data;
	LoopRunnerPop(l);
	if (!IsPVP(hData.co->Entry.Mode) &&
		GetNumPlayers(PLAYER_ANY, false, true) > 0)
	{
		LoadHighScores();
		bool allTime = false;
		bool todays = false;
		CA_FOREACH(PlayerData, p, gPlayerDatas)
		const bool isPlayerComplete =
			(!hData.co->IsQuit && !p->survived) || hData.co->IsComplete;
		if (isPlayerComplete && p->IsLocal && IsPlayerHuman(p))
		{
			EnterHighScore(p);
			allTime |= p->allTime >= 0;
			todays |= p->today >= 0;
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
		if (todays)
		{
			LoopRunnerPush(l, DisplayTodaysHighScores(hData.g));
		}
		if (allTime)
		{
			LoopRunnerPush(l, DisplayAllTimeHighScores(hData.g));
		}
	}

	return UPDATE_RESULT_OK;
}

// Warning: written as-is to file
#ifdef _MSC_VER
#pragma pack(push, 1)
#endif
typedef struct
{
	char name[20];
	int32_t timeTicks;
	int32_t kills;
	int32_t unused3;
	int32_t unused4;
	int32_t unused5;
	int32_t unused6;
	int32_t score;
	int32_t missions;
	int32_t lastMission;
}
#ifndef _MSC_VER
__attribute__((packed))
#endif
HighScoreEntry;
#ifdef _MSC_VER
#pragma pack(pop)
#endif

#define MAX_ENTRY 20

static HighScoreEntry allTimeHigh[MAX_ENTRY];
static HighScoreEntry todaysHigh[MAX_ENTRY];

static int EnterTable(HighScoreEntry *table, PlayerData *data)
{
	for (int i = 0; i < MAX_ENTRY; i++)
	{
		if ((int)data->Totals.Score > table[i].score)
		{
			for (int j = MAX_ENTRY - 1; j > i; j--)
			{
				table[j] = table[j - 1];
			}

			strcpy(table[i].name, data->name);
			table[i].timeTicks = data->Totals.TimeTicks;
			table[i].kills = data->Totals.Kills;
			table[i].score = data->Totals.Score;
			table[i].missions = data->missions;
			table[i].lastMission = data->lastMission;

			return i;
		}
	}
	return -1;
}

void EnterHighScore(PlayerData *data)
{
	data->allTime = EnterTable(allTimeHigh, data);
	data->today = EnterTable(todaysHigh, data);
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
	sprintf(s, "%d", e->score);
	DisplayAt(x + SCORE_OFFSET - FontStrW(s), y, s, hilite);
	sprintf(s, "%d", e->missions);
	DisplayAt(x + MISSIONS_OFFSET - FontStrW(s), y, s, hilite);
	sprintf(s, "(%d)", e->lastMission + 1);
	DisplayAt(x + MISSION_OFFSET - FontStrW(s), y, s, hilite);
	DisplayAt(x + NAME_OFFSET, y, e->name, hilite);

	return 1 + FontH();
}

static int DisplayPage(
	const char *title, const int idxStart, const HighScoreEntry *e,
	const int highlights[MAX_LOCAL_PLAYERS])
{
	int x = 80;
	int y = 5 + FontH();

	FontStr(title, svec2i(5, 5));
	int idx = idxStart;
	while (idx < MAX_ENTRY && e[idx].score > 0 && x < 300)
	{
		bool isHighlighted = false;
		for (int i = 0; i < MAX_LOCAL_PLAYERS; i++)
		{
			if (idx == highlights[i])
			{
				isHighlighted = true;
				break;
			}
		}
		y += DisplayEntry(x, y, idx, &e[idx], isHighlighted);
		if (y > 198 - FontH())
		{
			y = 20;
			x += 100;
		}
		idx++;
	}
	return idx;
}
typedef struct
{
	GraphicsDevice *g;
	const char *title;
	const HighScoreEntry *scores;
	int highlights[MAX_LOCAL_PLAYERS];
	int scoreIdx;
} HighScoresData;
static void HighScoreTerminate(GameLoopData *data);
static GameLoopResult HighScoreUpdate(GameLoopData *data, LoopRunner *l);
static void HighScoreDraw(GameLoopData *data);
GameLoopData *DisplayAllTimeHighScores(GraphicsDevice *graphics)
{
	HighScoresData *data;
	CMALLOC(data, sizeof *data);
	data->g = graphics;
	int idx = 0;
	for (int i = 0; i < (int)gPlayerDatas.size; i++, idx++)
	{
		const PlayerData *p = CArrayGet(&gPlayerDatas, i);
		if (!p->IsLocal)
		{
			idx--;
			continue;
		}
		data->highlights[idx] = p->allTime;
	}
	data->scoreIdx = 0;
	data->title = "All time high scores:";
	data->scores = allTimeHigh;
	return GameLoopDataNew(
		data, HighScoreTerminate, NULL, NULL, NULL, HighScoreUpdate,
		HighScoreDraw);
}
GameLoopData *DisplayTodaysHighScores(GraphicsDevice *graphics)
{
	HighScoresData *data;
	CMALLOC(data, sizeof *data);
	data->g = graphics;
	int idx = 0;
	for (int i = 0; i < (int)gPlayerDatas.size; i++, idx++)
	{
		const PlayerData *p = CArrayGet(&gPlayerDatas, i);
		if (!p->IsLocal)
		{
			idx--;
			continue;
		}
		data->highlights[idx] = p->today;
	}
	data->scoreIdx = 0;
	data->title = "Today's highest score:";
	data->scores = todaysHigh;
	return GameLoopDataNew(
		data, HighScoreTerminate, NULL, NULL, NULL, HighScoreUpdate,
		HighScoreDraw);
}
static void HighScoreTerminate(GameLoopData *data)
{
	HighScoresData *hData = data->Data;

	CFREE(hData);
}
static void HighScoreDraw(GameLoopData *data)
{
	HighScoresData *hData = data->Data;

	BlitClearBuf(hData->g);
	hData->scoreIdx = DisplayPage(
		hData->title, hData->scoreIdx, hData->scores, hData->highlights);
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

#define MAGIC 4711

void SaveHighScores(void)
{
	int magic;
	FILE *f;
	time_t t;
	struct tm *tp;

	f = fopen(GetConfigFilePath(SCORES_FILE), "wb");
	if (f != NULL)
	{
		magic = MAGIC;

		fwrite(&magic, sizeof(magic), 1, f);
		fwrite(allTimeHigh, sizeof(allTimeHigh), 1, f);

		t = time(NULL);
		tp = localtime(&t);

		magic = tp->tm_year;
		fwrite(&magic, sizeof(magic), 1, f);
		magic = tp->tm_mon;
		fwrite(&magic, sizeof(magic), 1, f);
		magic = tp->tm_mday;
		fwrite(&magic, sizeof(magic), 1, f);

		fwrite(todaysHigh, sizeof(todaysHigh), 1, f);

		fclose(f);

#ifdef __EMSCRIPTEN__
		EM_ASM(
			// persist changes
			FS.syncfs(
				false, function(err) { assert(!err); }););
#endif
	}
	else
		printf("Unable to open %s\n", SCORES_FILE);
}

void LoadHighScores(void)
{
	int magic;
	FILE *f;
	int y, m, d;
	time_t t;
	struct tm *tp;

	memset(allTimeHigh, 0, sizeof(allTimeHigh));
	memset(todaysHigh, 0, sizeof(todaysHigh));

	const char *path = GetConfigFilePath(SCORES_FILE);
	f = fopen(path, "rb");
	if (f != NULL)
	{
		size_t elementsRead;
#define CHECK_FREAD(count)                                                    \
	if (elementsRead != count)                                                \
	{                                                                         \
		fclose(f);                                                            \
		return;                                                               \
	}
		elementsRead = fread(&magic, sizeof(magic), 1, f);
		CHECK_FREAD(1)
		if (magic != MAGIC)
		{
			fclose(f);
			return;
		}

		elementsRead = fread(allTimeHigh, sizeof(allTimeHigh), 1, f);
		CHECK_FREAD(1)

		t = time(NULL);
		tp = localtime(&t);
		elementsRead = fread(&y, sizeof(y), 1, f);
		CHECK_FREAD(1)
		elementsRead = fread(&m, sizeof(m), 1, f);
		CHECK_FREAD(1)
		elementsRead = fread(&d, sizeof(d), 1, f);
		CHECK_FREAD(1)

		if (tp->tm_year == y && tp->tm_mon == m && tp->tm_mday == d)
		{
			elementsRead = fread(todaysHigh, sizeof(todaysHigh), 1, f);
			CHECK_FREAD(1)
		}

		fclose(f);
	}
	else
	{
		LOG(LM_MAIN, LL_WARN, "Unable to open %s\n", path);
	}
}
