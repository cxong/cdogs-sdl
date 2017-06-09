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

#define PLAYER_LIST_ROW_HEIGHT 16
typedef struct
{
	Vec2i pos;
	Vec2i size;
	int scroll;
	void (*drawFunc)(void *);
	void *data;
	// Whether to use a confirmation "Finish" menu at the end
	// Useful for final screens where we want to view the scores without
	// accidentally quitting
	bool hasMenu;
	bool showWinners;
	bool showLastMan;
	// Store player UIDs so we can display the list in a certain order
	CArray playerUIDs;	// of int
} PlayerList;
static int ComparePlayerScores(const void *v1, const void *v2);
static PlayerList PlayerListNew(
	void (*drawFunc)(void *), void *data,
	const bool hasMenu, const bool showWinners)
{
	PlayerList pl;
	pl.pos = Vec2iZero();
	pl.size = gGraphicsDevice.cachedConfig.Res;
	pl.scroll = 0;
	pl.drawFunc = drawFunc;
	pl.data = data;
	pl.hasMenu = hasMenu;
	pl.showWinners = showWinners;
	CArrayInit(&pl.playerUIDs, sizeof(int));
	// Collect all players, then order by score descending
	int playersAlive = 0;
	CA_FOREACH(const PlayerData, p, gPlayerDatas)
		CArrayPushBack(&pl.playerUIDs, &p->UID);
		if (p->Lives > 0)
		{
			playersAlive++;
		}
	CA_FOREACH_END()
	qsort(
		pl.playerUIDs.data,
		pl.playerUIDs.size,
		pl.playerUIDs.elemSize,
		ComparePlayerScores);
	pl.showLastMan = playersAlive == 1;
	return pl;
}
static int GetModeScore(const PlayerData *p)
{
	// For deathmatch, we count kills instead of score
	if (gCampaign.Entry.Mode == GAME_MODE_DEATHMATCH)
	{
		return p->Totals.Kills;
	}
	return p->Totals.Score;
}
static int ComparePlayerScores(const void *v1, const void *v2)
{
	const PlayerData *p1 = PlayerDataGetByUID(*(const int *)v1);
	const PlayerData *p2 = PlayerDataGetByUID(*(const int *)v2);
	int p1s = GetModeScore(p1);
	int p2s = GetModeScore(p2);
	if (p1s > p2s)
	{
		return -1;
	}
	else if (p1s < p2s)
	{
		return 1;
	}
	return 0;
}
static void PlayerListDraw(
	const menu_t *menu, GraphicsDevice *g, const Vec2i pos, const Vec2i size,
	const void *data);
static int PlayerListInput(int cmd, void *data);
static void PlayerListLoop(PlayerList *pl)
{
	MenuSystem ms;
	MenuSystemInit(&ms, &gEventHandlers, &gGraphicsDevice, pl->pos, pl->size);
	menu_t *menuScores = MenuCreateCustom(
		"View Scores", PlayerListDraw, PlayerListInput, pl);
	if (pl->hasMenu)
	{
		ms.root = MenuCreateNormal("", "", MENU_TYPE_NORMAL, 0);
		MenuAddSubmenu(ms.root, menuScores);
		MenuAddSubmenu(ms.root, MenuCreateReturn("Finish", 0));
		ms.current = MenuGetSubmenuByName(ms.root, "View Scores");
	}
	else
	{
		ms.root = ms.current = menuScores;
	}
	ms.allowAborts = true;
	MenuAddExitType(&ms, MENU_TYPE_RETURN);
	MenuLoop(&ms);
	// Free ourselves at the end
	CArrayTerminate(&pl->playerUIDs);
}
static int PlayerListMaxScroll(const PlayerList *pl);
static int PlayerListMaxRows(const PlayerList *pl);
static void PlayerListDraw(
	const menu_t *menu, GraphicsDevice *g, const Vec2i pos, const Vec2i size,
	const void *data)
{
	UNUSED(menu);
	UNUSED(g);
	// Draw players starting from the index
	// TODO: custom columns
	const PlayerList *pl = data;

	// First draw the headers
	const int xStart = pos.x + 80 + (size.x - 320) / 2;
	int x = xStart;
	int y = pos.y;
	FontStrMask("Player", Vec2iNew(x, y), colorPurple);
	x += 100;
	FontStrMask("Score", Vec2iNew(x, y), colorPurple);
	x += 32;
	FontStrMask("Kills", Vec2iNew(x, y), colorPurple);
	y += FontH() * 2 + PLAYER_LIST_ROW_HEIGHT + 4;
	// Then draw the player list
	int maxScore = -1;
	for (int i = pl->scroll;
		i < MIN((int)pl->playerUIDs.size, pl->scroll + PlayerListMaxRows(pl));
		i++)
	{
		const int *playerUID = CArrayGet(&pl->playerUIDs, i);
		PlayerData *p = PlayerDataGetByUID(*playerUID);
		if (p == NULL)
		{
			continue;
		}
		if (maxScore < GetModeScore(p))
		{
			maxScore = GetModeScore(p);
		}

		x = xStart;
		// Highlight local players using different coloured text
		const color_t textColor = p->IsLocal ? colorPurple : colorWhite;

		// Draw the players offset on alternate rows
		DisplayCharacterAndName(
			Vec2iNew(x + (i & 1) * 16, y + 4), &p->Char, DIRECTION_DOWN,
			p->name, textColor);

		// Draw score
		x += 100;
		char buf[256];
		sprintf(buf, "%d", p->Totals.Score);
		FontStrMask(buf, Vec2iNew(x, y), textColor);

		// Draw kills
		x += 32;
		sprintf(buf, "%d", p->Totals.Kills);
		FontStrMask(buf, Vec2iNew(x, y), textColor);

		// Draw winner/award text
		x += 32;
		if (pl->showWinners && GetModeScore(p) == maxScore)
		{
			FontStrMask("Winner!", Vec2iNew(x, y), colorGreen);
		}
		else if (pl->showLastMan && p->Lives > 0 &&
			gCampaign.Entry.Mode == GAME_MODE_DEATHMATCH)
		{
			// Only show last man standing on deathmatch mode
			FontStrMask("Last man standing!", Vec2iNew(x, y), colorGreen);
		}

		y += PLAYER_LIST_ROW_HEIGHT;
	}

	// Draw indicator arrows if there's enough to scroll
	if (pl->scroll > 0)
	{
		FontStr("^", Vec2iNew(
			CENTER_X(pos, size, FontStrW("^")), pos.y + FontH()));
	}
	if (pl->scroll < PlayerListMaxScroll(pl))
	{
		FontStr("v", Vec2iNew(
			CENTER_X(pos, size, FontStrW("v")), pos.y + size.y - FontH()));
	}

	// Finally draw any custom stuff
	if (pl->drawFunc)
	{
		pl->drawFunc(pl->data);
	}
}
static int PlayerListInput(int cmd, void *data)
{
	// Input: up/down scrolls list
	// CMD 1/2: exit
	PlayerList *pl = data;

	// Note: players can leave due to network disconnection
	// Update our lists
	CA_FOREACH(const int, playerUID, pl->playerUIDs)
		const PlayerData *p = PlayerDataGetByUID(*playerUID);
		if (p == NULL)
		{
			CArrayDelete(&pl->playerUIDs, _ca_index);
			_ca_index--;
		}
	CA_FOREACH_END()

	if (cmd == CMD_DOWN)
	{
		SoundPlay(&gSoundDevice, StrSound("door"));
		pl->scroll++;
	}
	else if (cmd == CMD_UP)
	{
		SoundPlay(&gSoundDevice, StrSound("door"));
		pl->scroll--;
	}
	else if (AnyButton(cmd))
	{
		SoundPlay(&gSoundDevice, StrSound("pickup"));
		return 1;
	}
	// Scroll wrap-around
	pl->scroll = CLAMP_OPPOSITE(pl->scroll, 0, PlayerListMaxScroll(pl));
	return 0;
}
static int PlayerListMaxScroll(const PlayerList *pl)
{
	return MAX((int)pl->playerUIDs.size - PlayerListMaxRows(pl), 0);
}
static int PlayerListMaxRows(const PlayerList *pl)
{
	return (pl->size.y - FontH() * 3) / PLAYER_LIST_ROW_HEIGHT - 2;
}


typedef struct
{
	const CampaignOptions *Campaign;
	const char *FinalWords;
} VictoryData;
static void VictoryDraw(void *data);
void ScreenVictory(CampaignOptions *c)
{
	SoundPlay(&gSoundDevice, StrSound("victory"));
	VictoryData data;
	data.Campaign = c;
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
	if (GetNumPlayers(PLAYER_ANY, false, true) == 1)
	{
		const int numWords = sizeof finalWordsSingle / sizeof(char *);
		data.FinalWords = finalWordsSingle[rand() % numWords];
	}
	else
	{
		const int numWords = sizeof finalWordsMulti / sizeof(char *);
		data.FinalWords = finalWordsMulti[rand() % numWords];
	}
	PlayerList pl = PlayerListNew(VictoryDraw, &data, true, false);
	pl.pos.y = 75;
	pl.size.y -= pl.pos.y;
	PlayerListLoop(&pl);
	SoundPlay(&gSoundDevice, StrSound("hahaha"));
}
static void VictoryDraw(void *data)
{
	const VictoryData *vd = data;

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
	FontStrOpt(vd->Campaign->Setting.Title, Vec2iNew(0, y), opts);
	y += 15;

	// Final words
	Vec2i pos = Vec2iNew((w - FontStrW(vd->FinalWords)) / 2, y);
	pos = FontChMask('"', pos, colorDarker);
	pos = FontStrMask(vd->FinalWords, pos, colorPurple);
	FontChMask('"', pos, colorDarker);
}

void ScreenDogfightScores(void)
{
	PlayerList pl = PlayerListNew(NULL, NULL, false, false);
	pl.pos.y = 24;
	pl.size.y -= pl.pos.y;
	PlayerListLoop(&pl);
	SoundPlay(&gSoundDevice, StrSound("mg"));
}

void ScreenDogfightFinalScores(void)
{
	SoundPlay(&gSoundDevice, StrSound("victory"));
	PlayerList pl = PlayerListNew(NULL, NULL, true, true);
	pl.pos.y = 24;
	pl.size.y -= pl.pos.y;
	PlayerListLoop(&pl);
	SoundPlay(&gSoundDevice, StrSound("mg"));
}

void ScreenDeathmatchFinalScores(void)
{
	SoundPlay(&gSoundDevice, StrSound("victory"));
	PlayerList pl = PlayerListNew(NULL, NULL, true, true);
	pl.pos.y = 24;
	pl.size.y -= pl.pos.y;
	PlayerListLoop(&pl);
	SoundPlay(&gSoundDevice, StrSound("mg"));
}
