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
#include "hiscores.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "config.h"
#include "font.h"
#include "game_loop.h"
#include "gamedata.h"
#include "grafx.h"
#include "grafx_bg.h"
#include "blit.h"
#include "actors.h"
#include "events.h"
#include "keyboard.h"
#include "pics.h"
#include "sounds.h"
#include "files.h"
#include "utils.h"


// Warning: written as-is to file
struct Entry {
	char name[20];
	int32_t unused1;
	int32_t unused2;
	int32_t unused3;
	int32_t unused4;
	int32_t unused5;
	int32_t unused6;
	int32_t score;
	int32_t missions;
	int32_t lastMission;
};

#define MAX_ENTRY 20

static struct Entry allTimeHigh[MAX_ENTRY];
static struct Entry todaysHigh[MAX_ENTRY];


static int EnterTable(struct Entry *table, PlayerData *data)
{
	for (int i = 0; i < MAX_ENTRY; i++)
	{
		if (data->Totals.Score > table[i].score)
		{
			for (int j = MAX_ENTRY - 1; j > i; j--)
			{
				table[j] = table[j - 1];
			}

			strcpy(table[i].name, data->name);
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
	FontStrMask(s, Vec2iNew(x, y), mask);
}

static int DisplayEntry(int x, int y, int idx, struct Entry *e, int hilite)
{
	char s[10];

#define INDEX_OFFSET     15
#define SCORE_OFFSET     40
#define MISSIONS_OFFSET  60
#define MISSION_OFFSET   80
#define NAME_OFFSET      85

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
	const char *title, int idx, struct Entry *e,
	int highlights[MAX_LOCAL_PLAYERS])
{
	int x = 80;
	int y = 5 + FontH();

	FontStr(title, Vec2iNew(5, 5));
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
void DisplayAllTimeHighScores(GraphicsDevice *graphics)
{
	int highlights[MAX_LOCAL_PLAYERS];
	int idx = 0;
	for (int i = 0; i < (int)gPlayerDatas.size; i++, idx++)
	{
		const PlayerData *p = CArrayGet(&gPlayerDatas, i);
		if (!p->IsLocal)
		{
			idx--;
			continue;
		}
		highlights[idx] = p->allTime;
	}
	idx = 0;
	while (idx < MAX_ENTRY && allTimeHigh[idx].score > 0)
	{
		GraphicsClear(graphics);
		idx = DisplayPage(
			"All time high scores:", idx, allTimeHigh, highlights);
		GameLoopData gData = GameLoopDataNew(
			NULL, GameLoopWaitForAnyKeyOrButtonFunc, NULL, NULL);
		GameLoop(&gData);
	}
}
void DisplayTodaysHighScores(GraphicsDevice *graphics)
{
	int highlights[MAX_LOCAL_PLAYERS];
	int idx = 0;
	for (int i = 0; i < (int)gPlayerDatas.size; i++, idx++)
	{
		const PlayerData *p = CArrayGet(&gPlayerDatas, i);
		if (!p->IsLocal)
		{
			idx--;
			continue;
		}
		highlights[idx] = p->today;
	}
	idx = 0;
	while (idx < MAX_ENTRY && todaysHigh[idx].score > 0)
	{
		GraphicsClear(graphics);
		idx = DisplayPage(
			"Today's highest score:", idx, todaysHigh, highlights);
		GameLoopData gData = GameLoopDataNew(
			NULL, GameLoopWaitForAnyKeyOrButtonFunc, NULL, NULL);
		GameLoop(&gData);
	}
}


#define MAGIC        4711
#define SCORES_FILE "scores.dat"

void SaveHighScores(void)
{
	int magic;
	FILE *f;
	time_t t;
	struct tm *tp;

	debug(D_NORMAL, "begin\n");

	f = fopen(GetConfigFilePath(SCORES_FILE), "wb");
	if (f != NULL) {
		magic = MAGIC;

		fwrite(&magic, sizeof(magic), 1, f);
		fwrite(allTimeHigh, sizeof(allTimeHigh), 1, f);

		t = time(NULL);
		tp = localtime(&t);
		debug(D_NORMAL, "time now, y: %d m: %d d: %d\n", tp->tm_year, tp->tm_mon, tp->tm_mday);

		magic = tp->tm_year;
		fwrite(&magic, sizeof(magic), 1, f);
		magic = tp->tm_mon;
		fwrite(&magic, sizeof(magic), 1, f);
		magic = tp->tm_mday;
		fwrite(&magic, sizeof(magic), 1, f);

		debug(D_NORMAL, "writing today's high: %d\n", todaysHigh[0].score);
		fwrite(todaysHigh, sizeof(todaysHigh), 1, f);

		fclose(f);

		debug(D_NORMAL, "saved high scores\n");
	} else
		printf("Unable to open %s\n", SCORES_FILE);
}

void LoadHighScores(void)
{
	int magic;
	FILE *f;
	int y, m, d;
	time_t t;
	struct tm *tp;

	debug(D_NORMAL, "Reading hi-scores...\n");

	memset(allTimeHigh, 0, sizeof(allTimeHigh));
	memset(todaysHigh, 0, sizeof(todaysHigh));

	f = fopen(GetConfigFilePath(SCORES_FILE), "rb");
	if (f != NULL) {
		size_t elementsRead;
	#define CHECK_FREAD(count)\
		if (elementsRead != count) {\
			debug(D_NORMAL, "Error reading scores file\n");\
			fclose(f);\
			return;\
		}
		elementsRead = fread(&magic, sizeof(magic), 1, f);
		CHECK_FREAD(1)
		if (magic != MAGIC) {
			debug(D_NORMAL, "Scores file magic doesn't match!\n");
			fclose(f);
			return;
		}

		//for (i = 0; i < MAX_ENTRY; i++) {
		elementsRead = fread(allTimeHigh, sizeof(allTimeHigh), 1, f);
		CHECK_FREAD(1)
		//}

		t = time(NULL);
		tp = localtime(&t);
		elementsRead = fread(&y, sizeof(y), 1, f);
		CHECK_FREAD(1)
		elementsRead = fread(&m, sizeof(m), 1, f);
		CHECK_FREAD(1)
		elementsRead = fread(&d, sizeof(d), 1, f);
		CHECK_FREAD(1)
		debug(D_NORMAL, "scores time, y: %d m: %d d: %d\n", y, m, d);


		if (tp->tm_year == y && tp->tm_mon == m && tp->tm_mday == d) {
			elementsRead = fread(todaysHigh, sizeof(todaysHigh), 1, f);
			CHECK_FREAD(1)
			debug(D_NORMAL, "reading today's high: %d\n", todaysHigh[0].score);
		}

		fclose(f);
	} else {
		printf("Unable to open %s\n", SCORES_FILE);
	}
}
