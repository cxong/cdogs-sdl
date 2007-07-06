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

-------------------------------------------------------------------------------

 hiscores.c - high score functions
 
 Author: $Author$
 Rev:    $Revision$
 URL:    $HeadURL$
 ID:     $Id$
 
*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#ifndef _MSC_VER
	#include <unistd.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "gamedata.h"
#include "hiscores.h"
#include "grafx.h"
#include "blit.h"
#include "actors.h"
#include "pics.h"
#include "text.h"
#include "sounds.h"
#include "input.h"
#include "files.h"
#include "utils.h"


struct Entry {
	char name[20];
	int head;
	int body;
	int arms;
	int legs;
	int skin;
	int hair;
	int score;
	int missions;
	int lastMission;
};

#define MAX_ENTRY 20

static struct Entry allTimeHigh[MAX_ENTRY];
static struct Entry todaysHigh[MAX_ENTRY];


static int EnterTable(struct Entry *table, struct PlayerData *data)
{
	int i, j;

	for (i = 0; i < MAX_ENTRY; i++) {
		if (data->totalScore > table[i].score) {
			for (j = MAX_ENTRY - 1; j > i; j--)
				table[j] = table[j - 1];

			strcpy(table[i].name, data->name);
			table[i].score = data->totalScore;
			table[i].missions = data->missions;
			table[i].lastMission = data->lastMission;
			table[i].head = data->head;
			table[i].arms = data->arms;
			table[i].body = data->body;
			table[i].legs = data->legs;
			table[i].skin = data->skin;
			table[i].hair = data->hair;

			return i;
		}
	}
	return -1;
}

void EnterHighScore(struct PlayerData *data)
{
	data->allTime = EnterTable(allTimeHigh, data);
	data->today = EnterTable(todaysHigh, data);
}

/*
static void DisplayCharacterUsed( int x, int y, struct Entry *entry )
{
  TOffsetPic body, head;
  TranslationTable table;

  SetCharacterColors( table, entry->arms, entry->body, entry->legs, entry->skin, entry->hair);

  body.dx = cBodyOffset[ BODY_UNARMED][ DIRECTION_DOWN].dx;
  body.dy = cBodyOffset[ BODY_UNARMED][ DIRECTION_DOWN].dy;
  body.picIndex = cBodyPic[ BODY_UNARMED][ DIRECTION_DOWN][ STATE_IDLE];

  head.dx = cNeckOffset[ BODY_UNARMED][ DIRECTION_DOWN].dx + cHeadOffset[ entry->head][ DIRECTION_DOWN].dx;
  head.dy = cNeckOffset[ BODY_UNARMED][ DIRECTION_DOWN].dy + cHeadOffset[ entry->head][ DIRECTION_DOWN].dy;
  head.picIndex = cHeadPic[ entry->head][ DIRECTION_DOWN][ STATE_IDLE];

  DrawTTPic( x + body.dx, y + body.dy, gPics[ body.picIndex], table, gRLEPics[ body.picIndex]);
  DrawTTPic( x + head.dx, y + head.dy, gPics[ head.picIndex], table, gRLEPics[ head.picIndex]);
}
*/

static void DisplayAt(int x, int y, const char *s, int hilite)
{
	if (hilite)
		TextStringWithTableAt(x, y, s, &tableFlamed);
	else
		TextStringAt(x, y, s);
}

static int DisplayEntry(int x, int y, int index, struct Entry *e,
			int hilite)
{
	char s[10];

#define INDEX_OFFSET     15
#define SCORE_OFFSET     40
#define MISSIONS_OFFSET  60
#define MISSION_OFFSET   80
#define NAME_OFFSET      85

	sprintf(s, "%d.", index + 1);
	DisplayAt(x + INDEX_OFFSET - TextWidth(s), y, s, hilite);
	sprintf(s, "%d", e->score);
	DisplayAt(x + SCORE_OFFSET - TextWidth(s), y, s, hilite);
	sprintf(s, "%d", e->missions);
	DisplayAt(x + MISSIONS_OFFSET - TextWidth(s), y, s, hilite);
	sprintf(s, "(%d)", e->lastMission + 1);
	DisplayAt(x + MISSION_OFFSET - TextWidth(s), y, s, hilite);
	DisplayAt(x + NAME_OFFSET, y, e->name, hilite);

	return 1 + TextHeight();
}

static int DisplayPage(const char *title, int index, struct Entry *e,
		       int hilite1, int hilite2)
{
	int x = 80;
	int y = 5;

	TextStringAt(5, 5, title);
	while (index < MAX_ENTRY && e[index].score > 0 && x < 300) {
		y += DisplayEntry(x, y, index, &e[index], index == hilite1
				  || index == hilite2);
		if (y > 198 - TextHeight()) {
			y = 20;
			x += 100;
		}
		index++;
	}
	CopyToScreen();
	return index;
}

void DisplayAllTimeHighScores(void *bkg)
{
	int index = 0;

	while (index < MAX_ENTRY && allTimeHigh[index].score > 0) {
		memcpy(GetDstScreen(), bkg, SCREEN_MEMSIZE);
		index = DisplayPage("All time high scores:", index, allTimeHigh,
				    gPlayer1Data.allTime,
				    gOptions.twoPlayers ? gPlayer2Data.
				    allTime : -1);
		Wait();
	}
}

void DisplayTodaysHighScores(void *bkg)
{
	int index = 0;

	while (index < MAX_ENTRY && todaysHigh[index].score > 0) {
		memcpy(GetDstScreen(), bkg, SCREEN_MEMSIZE);
		index = DisplayPage("Today's highest score:", index, todaysHigh,
				    gPlayer1Data.today,
				    gOptions.twoPlayers ? gPlayer2Data.
				    today : -1);
		Wait();
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

	debug("begin\n");

	f = fopen(GetConfigFilePath(SCORES_FILE), "wb");
	if (f != NULL) {
		magic = MAGIC;

		fwrite(&magic, sizeof(magic), 1, f);
		fwrite(allTimeHigh, sizeof(allTimeHigh), 1, f);

		t = time(NULL);
		tp = localtime(&t);
		debug("time now, y: %d m: %d d: %d\n", tp->tm_year, tp->tm_mon, tp->tm_mday);

		magic = tp->tm_year;
		fwrite(&magic, sizeof(magic), 1, f);
		magic = tp->tm_mon;
		fwrite(&magic, sizeof(magic), 1, f);
		magic = tp->tm_mday;
		fwrite(&magic, sizeof(magic), 1, f);

		debug("writing today's high: %d\n", todaysHigh[0].score);
		fwrite(todaysHigh, sizeof(todaysHigh), 1, f);

		fclose(f);

		debug("saved high scores\n");
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

	debug("Reading hi-scores...\n");

	memset(allTimeHigh, 0, sizeof(allTimeHigh));
	memset(todaysHigh, 0, sizeof(todaysHigh));

	f = fopen(GetConfigFilePath(SCORES_FILE), "rb");
	if (f != NULL) {
		int i;
	
		fread(&magic, sizeof(magic), 1, f);
		if (magic != MAGIC) {
			debug("Scores file magic doesn't match!\n");
			fclose(f);
			return;
		}
		
		//for (i = 0; i < MAX_ENTRY; i++) {
		fread(allTimeHigh, sizeof(allTimeHigh), 1, f);
		//}

		t = time(NULL);
		tp = localtime(&t);
		fread(&y, sizeof(y), 1, f);
		fread(&m, sizeof(m), 1, f);
		fread(&d, sizeof(d), 1, f);
		debug("scores time, y: %d m: %d d: %d\n", y, m, d);	


		if (tp->tm_year == y && tp->tm_mon == m && tp->tm_mday == d) {
			fread(todaysHigh, sizeof(todaysHigh), 1, f);
			debug("reading today's high: %d\n", todaysHigh[0].score);
		}

		fclose(f);
	} else {
		printf("Unable to open %s\n", SCORES_FILE);
	}
}
