/*
    Copyright (c) 2013-2016, Cong Xu
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
#include "screens_end.h"

#include <cdogs/events.h>
#include <cdogs/font.h>
#include <cdogs/game_loop.h>
#include <cdogs/grafx_bg.h>

#include "menu_utils.h"


// All ending screens will be drawn in a table/list format, with each player
// on a row. This is to support any number of players, since there could be
// many players especially from network multiplayer.

static void VictoryDraw(void *data);
void ScreenVictory(CampaignOptions *c)
{
	SoundPlay(&gSoundDevice, StrSound("victory"));
	GameLoopData gData = GameLoopDataNew(
		NULL, GameLoopWaitForAnyKeyOrButtonFunc,
		c, VictoryDraw);
	GameLoop(&gData);
	SoundPlay(&gSoundDevice, StrSound("hahaha"));
}
static void VictoryDraw(void *data)
{
	// This will only draw once
	const CampaignOptions *c = data;

	GraphicsBlitBkg(&gGraphicsDevice);
	const int w = gGraphicsDevice.cachedConfig.Res.x;
	FontOpts opts = FontOptsNew();
	opts.HAlign = ALIGN_CENTER;
	opts.Area = gGraphicsDevice.cachedConfig.Res;
	int y = 30;

	// Congratulations text
#define CONGRATULATIONS "Congratulations, you have completed "
	FontStrOpt(CONGRATULATIONS, Vec2iNew(0, y), opts);
	y += 15;
	opts.Mask = colorRed;
	FontStrOpt(c->Setting.Title, Vec2iNew(0, y), opts);
	y += 15;

	// Final words
	const char *finalWordsSingle[] = {
		"Ha, next time I'll use my good hand",
		"Over already? I was just warming up...",
		"There's just no good opposition to be found these days!",
		"Well, maybe I'll just do my monthly reload then",
		"Woof woof",
		"I'll just bury the bones in the back yard, he-he",
		"I just wish they'd let me try bare-handed",
		"Rambo? Who's Rambo?",
		"<in Austrian accent:> I'll be back",
		"Gee, my trigger finger is sore",
		"I need more practice. I think I missed a few shots at times"
	};
	const char *finalWordsMulti[] = {
		"United we stand, divided we conquer",
		"Nothing like good teamwork, is there?",
		"Which way is the camera?",
		"We eat bullets for breakfast and have grenades as dessert",
		"We're so cool we have to wear mittens",
	};
	const char *finalWords;
	if (GetNumPlayers(PLAYER_ANY, false, true) == 1)
	{
		const int numWords = sizeof finalWordsSingle / sizeof(char *);
		finalWords = finalWordsSingle[rand() % numWords];
	}
	else
	{
		const int numWords = sizeof finalWordsMulti / sizeof(char *);
		finalWords = finalWordsMulti[rand() % numWords];
	}
	Vec2i pos = Vec2iNew((w - FontStrW(finalWords)) / 2, y);
	pos = FontChMask('"', pos, colorDarker);
	pos = FontStrMask(finalWords, pos, colorPurple);
	FontChMask('"', pos, colorDarker);
	y += 15;

	// Display players
	const int xStart = 100 + (w - 320) / 2;
	int x = xStart;
	// Draw headers
	FontStrMask("Player", Vec2iNew(x, y), colorPurple);
	x += 100;
	FontStrMask("Score", Vec2iNew(x, y), colorPurple);
	y += FontH() + 20;
	CA_FOREACH(const PlayerData, p, gPlayerDatas)
		x = xStart;
		// Draw the players offset on alternate rows
		DisplayCharacterAndName(
			Vec2iNew(x + (_ca_index & 1) * 16, y + 4), &p->Char, p->name);
		x += 100;
		char buf[256];
		sprintf(buf, "%d", p->totalScore);
		FontStr(buf, Vec2iNew(x, y));
		y += 16;
	CA_FOREACH_END()
}

static void DogfightScoresDraw(void *data);
void ScreenDogfightScores(void)
{
	GameLoopData gData = GameLoopDataNew(
		NULL, GameLoopWaitForAnyKeyOrButtonFunc, NULL, DogfightScoresDraw);
	GameLoop(&gData);
	SoundPlay(&gSoundDevice, StrSound("mg"));
}
static void ShowPlayerScore(const Vec2i pos, const int score);
static void DogfightScoresDraw(void *data)
{
	UNUSED(data);

	// This will only draw once
	const int w = gGraphicsDevice.cachedConfig.Res.x;
	const int h = gGraphicsDevice.cachedConfig.Res.y;

	GraphicsBlitBkg(&gGraphicsDevice);

	const PlayerData *pds[MAX_LOCAL_PLAYERS];
	for (int i = 0, idx = 0; i < (int)gPlayerDatas.size; i++, idx++)
	{
		const PlayerData *pd = CArrayGet(&gPlayerDatas, i);
		if (!pd->IsLocal)
		{
			idx--;
			continue;
		}
		pds[idx] = pd;
	}
	const int numLocalPlayers = GetNumPlayers(PLAYER_ANY, false, true);
	CASSERT(
		numLocalPlayers >= 2 && numLocalPlayers <= 4,
		"Unimplemented number of players for dogfight");
	for (int i = 0; i < numLocalPlayers; i++)
	{
		const Vec2i pos = Vec2iNew(
			w / 4 + (i & 1) * w / 2,
			numLocalPlayers == 2 ? h / 2 : h / 4 + (i / 2) * h / 2);
		DisplayCharacterAndName(pos, &pds[i]->Char, pds[i]->name);
		ShowPlayerScore(pos, pds[i]->RoundsWon);
	}
}
static void ShowPlayerScore(const Vec2i pos, const int score)
{
	char s[16];
	sprintf(s, "Score: %d", score);
	const Vec2i scorePos = Vec2iNew(pos.x - FontStrW(s) / 2, pos.y + 20);
	FontStr(s, scorePos);
}

static void DogfightFinalScoresDraw(void *data);
void ScreenDogfightFinalScores(void)
{
	SoundPlay(&gSoundDevice, StrSound("victory"));
	GameLoopData gData = GameLoopDataNew(
		NULL, GameLoopWaitForAnyKeyOrButtonFunc,
		NULL, DogfightFinalScoresDraw);
	GameLoop(&gData);
	SoundPlay(&gSoundDevice, StrSound("mg"));
}
static void DogfightFinalScoresDraw(void *data)
{
	UNUSED(data);

	// This will only draw once
	const int w = gGraphicsDevice.cachedConfig.Res.x;
	const int h = gGraphicsDevice.cachedConfig.Res.y;

	GraphicsBlitBkg(&gGraphicsDevice);

	// Work out who's the winner, or if it's a tie
	int maxScore = 0;
	int playersWithMaxScore = 0;
	CA_FOREACH(const PlayerData, p, gPlayerDatas)
		if (p->RoundsWon > maxScore)
		{
			maxScore = p->RoundsWon;
			playersWithMaxScore = 1;
		}
		else if (p->RoundsWon == maxScore)
		{
			playersWithMaxScore++;
		}
	CA_FOREACH_END()
	const bool isTie = playersWithMaxScore == (int)gPlayerDatas.size;

	// Draw players and their names spread evenly around the screen.
	// If it's a tie, display the message in the centre,
	// otherwise display the winner just below the winning player
#define DRAW_TEXT	"It's a draw!"
#define WINNER_TEXT	"Winner!"
	const PlayerData *pds[MAX_LOCAL_PLAYERS];
	for (int i = 0, idx = 0; i < (int)gPlayerDatas.size; i++, idx++)
	{
		const PlayerData *pd = CArrayGet(&gPlayerDatas, i);
		if (!pd->IsLocal)
		{
			idx--;
			continue;
		}
		pds[idx] = pd;
	}
	const int numLocalPlayers = GetNumPlayers(PLAYER_ANY, false, true);
	CASSERT(
		numLocalPlayers >= 2 && numLocalPlayers <= 4,
		"Unimplemented number of players for dogfight");
	for (int i = 0; i < numLocalPlayers; i++)
	{
		const Vec2i pos = Vec2iNew(
			w / 4 + (i & 1) * w / 2,
			numLocalPlayers == 2 ? h / 2 : h / 4 + (i / 2) * h / 2);
		DisplayCharacterAndName(pos, &pds[i]->Char, pds[i]->name);
		ShowPlayerScore(pos, pds[i]->RoundsWon);
		if (!isTie && maxScore == pds[i]->RoundsWon)
		{
			FontStrMask(
				WINNER_TEXT,
				Vec2iNew(pos.x - FontStrW(WINNER_TEXT) / 2, pos.y + 30),
				colorGreen);
		}
	}
	if (isTie)
	{
		FontStrCenter(DRAW_TEXT);
	}
}

static void DeathmatchFinalScoresDraw(void *data);
void ScreenDeathmatchFinalScores(void)
{
	SoundPlay(&gSoundDevice, StrSound("victory"));
	GameLoopData gData = GameLoopDataNew(
		NULL, GameLoopWaitForAnyKeyOrButtonFunc,
		NULL, DeathmatchFinalScoresDraw);
	GameLoop(&gData);
	SoundPlay(&gSoundDevice, StrSound("mg"));
}
static void DeathmatchFinalScoresDraw(void *data)
{
	UNUSED(data);

	// This will only draw once
	const int w = gGraphicsDevice.cachedConfig.Res.x;
	const int h = gGraphicsDevice.cachedConfig.Res.y;

	GraphicsBlitBkg(&gGraphicsDevice);

	// Work out the highest kills
	int maxKills = 0;
	CA_FOREACH(const PlayerData, p, gPlayerDatas)
		if (p->kills > maxKills)
		{
			maxKills = p->kills;
		}
	CA_FOREACH_END()

	// Draw players and their names spread evenly around the screen.
	const PlayerData *pds[MAX_LOCAL_PLAYERS];
	for (int i = 0, idx = 0; i < (int)gPlayerDatas.size; i++, idx++)
	{
		const PlayerData *pd = CArrayGet(&gPlayerDatas, i);
		if (!pd->IsLocal)
		{
			idx--;
			continue;
		}
		pds[idx] = pd;
	}
	const int numLocalPlayers = GetNumPlayers(PLAYER_ANY, false, true);
	CASSERT(
		numLocalPlayers >= 2 && numLocalPlayers <= 4,
		"Unimplemented number of players for deathmatch");
#define LAST_MAN_TEXT	"Last man standing!"
	for (int i = 0; i < numLocalPlayers; i++)
	{
		const Vec2i pos = Vec2iNew(
			w / 4 + (i & 1) * w / 2,
			numLocalPlayers == 2 ? h / 2 : h / 4 + (i / 2) * h / 2);
		DisplayCharacterAndName(pos, &pds[i]->Char, pds[i]->name);
		
		// Kills
		char s[16];
		sprintf(s, "Kills: %d", pds[i]->kills);
		FontStrMask(
			s, Vec2iNew(pos.x - FontStrW(s) / 2, pos.y + 20),
			pds[i]->kills == maxKills ? colorGreen : colorWhite);

		// Last man standing?
		if (pds[i]->Lives > 0)
		{
			FontStrMask(
				LAST_MAN_TEXT,
				Vec2iNew(pos.x - FontStrW(LAST_MAN_TEXT) / 2, pos.y + 30),
				colorGreen);
		}
	}
}
