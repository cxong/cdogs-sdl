/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (C) 1995 Ronny Wester
    Copyright (C) 2003 Jeremy Chin 
    Copyright (C) 2003 Lucas Martin-King 

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

*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// #include <io.h>
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
		memcpy(GetDstScreen(), bkg, 64000);
		index = DisplayPage("All time high", index, allTimeHigh,
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
		memcpy(GetDstScreen(), bkg, 64000);
		index = DisplayPage("Todays highest", index, todaysHigh,
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
	int f;
	time_t t;
	struct tm *tp;

	f = open(GetConfigFilePath(SCORES_FILE), O_CREAT | O_TRUNC, S_IWUSR | S_IRUSR);
	if (f >= 0) {
		magic = MAGIC;
		write(f, &magic, sizeof(magic));
		write(f, allTimeHigh, sizeof(allTimeHigh));
		t = time(NULL);
		tp = localtime(&t);
		magic = tp->tm_year;
		write(f, &magic, sizeof(magic));
		magic = tp->tm_mon;
		write(f, &magic, sizeof(magic));
		magic = tp->tm_mday;
		write(f, &magic, sizeof(magic));
		write(f, todaysHigh, sizeof(todaysHigh));

		/* I have to do this because open doesn't seem to set perms properly :(*/
		//fchmod(f, S_IWUSR|S_IRUSR); 
		close(f);
	} else
		printf("Unable to open %s\n", SCORES_FILE);
}

void LoadHighScores(void)
{
	int magic;
	int f;
	int y, m, d;
	time_t t;
	struct tm *tp;

	memset(allTimeHigh, 0, sizeof(allTimeHigh));
	memset(todaysHigh, 0, sizeof(todaysHigh));

	f = open(GetConfigFilePath(SCORES_FILE), O_RDONLY);
	if (f >= 0) {
		read(f, &magic, sizeof(magic));
		if (magic != MAGIC) {
			close(f);
			return;
		}
		read(f, allTimeHigh, sizeof(allTimeHigh));
		t = time(NULL);
		tp = localtime(&t);
		read(f, &y, sizeof(y));
		read(f, &m, sizeof(m));
		read(f, &d, sizeof(d));
		if (tp->tm_year == y && tp->tm_mon == m
		    && tp->tm_mday == d)
			read(f, todaysHigh, sizeof(todaysHigh));
		close(f);
	} else
		printf("Unable to open %s\n", SCORES_FILE);
}
