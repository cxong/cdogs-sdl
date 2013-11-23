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

    Copyright (c) 2013, Cong Xu
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
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#include <SDL.h>

#include <cdogs/actors.h>
#include <cdogs/ai.h>
#include <cdogs/blit.h>
#include <cdogs/campaigns.h>
#include <cdogs/config.h>
#include <cdogs/draw.h>
#include <cdogs/events.h>
#include <cdogs/files.h>
#include <cdogs/gamedata.h>
#include <cdogs/grafx.h>
#include <cdogs/hiscores.h>
#include <cdogs/input.h>
#include <cdogs/joystick.h>
#include <cdogs/keyboard.h>
#include <cdogs/mission.h>
#include <cdogs/music.h>
#include <cdogs/objs.h>
#include <cdogs/palette.h>
#include <cdogs/pic_manager.h>
#include <cdogs/pics.h>
#include <cdogs/sounds.h>
#include <cdogs/text.h>
#include <cdogs/triggers.h>
#include <cdogs/utils.h>

#include "autosave.h"
#include "credits.h"
#include "game.h"
#include "mainmenu.h"
#include "password.h"
#include "player_select_menus.h"
#include "prep.h"
#include "XGetopt.h"



void DrawObjectiveInfo(int idx, int x, int y, struct Mission *mission)
{
	TOffsetPic pic;
	TranslationTable *table = NULL;
	int i = 0;
	Character *cd;

	switch (mission->objectives[idx].type)
	{
	case OBJECTIVE_KILL:
		cd = CharacterStoreGetSpecial(&gCampaign.Setting.characters, 0);
		i = cd->looks.face;
		table = &cd->table;
		pic.picIndex = cHeadPic[i][DIRECTION_DOWN][STATE_IDLE];
		pic.dx = cHeadOffset[i][DIRECTION_DOWN].dx;
		pic.dy = cHeadOffset[i][DIRECTION_DOWN].dy;
		break;
	case OBJECTIVE_RESCUE:
		cd = CharacterStoreGetPrisoner(&gCampaign.Setting.characters, 0);
		i = cd->looks.face;
		table = &cd->table;
		pic.picIndex = cHeadPic[i][DIRECTION_DOWN][STATE_IDLE];
		pic.dx = cHeadOffset[i][DIRECTION_DOWN].dx;
		pic.dy = cHeadOffset[i][DIRECTION_DOWN].dy;
		break;
	case OBJECTIVE_COLLECT:
		i = gMission.objectives[idx].pickupItem;
		pic = cGeneralPics[i];
		break;
	case OBJECTIVE_DESTROY:
		i = gMission.objectives[idx].blowupObject->pic;
		pic = cGeneralPics[i];
		break;
	case OBJECTIVE_INVESTIGATE:
		pic.dx = pic.dy = 0;
		pic.picIndex = -1;
		return;
	default:
		i = gMission.objectives[i].pickupItem;
		pic = cGeneralPics[i];
		break;
	}
	if (pic.picIndex >= 0)
	{
		if (table)
		{
			DrawTTPic(
				x + pic.dx, y + pic.dy,
				PicManagerGetOldPic(&gPicManager, pic.picIndex), table);
		}
		else
		{
			DrawTPic(
				x + pic.dx, y + pic.dy,
				PicManagerGetOldPic(&gPicManager, pic.picIndex));
		}
	}
}

int MissionDescription(int y, const char *description)
{
	int w, ix, x, lines;
	const char *ws, *word, *p, *s;

#define MAX_BOX_WIDTH (gGraphicsDevice.cachedConfig.ResolutionWidth - (gGraphicsDevice.cachedConfig.ResolutionWidth / 6))

	ix = x = CenterX((MAX_BOX_WIDTH));
	lines = 1;
	CDogsTextGoto(x, y);

	s = ws = word = description;

	while (*s) {
		// Find word
		ws = s;

		while (*s == ' ' || *s == '\n')
			s++;

		word = s;

		while (*s != 0 && *s != ' ' && *s != '\n')
			s++;

		for (w = 0, p = ws; p < s; p++)
			w += CDogsTextCharWidth(*p);

		//if (x + w > MAX_BOX_WIDTH && w < (MAX_BOX_WIDTH - 20)) {
		if (x + w > (MAX_BOX_WIDTH + ix) && w < MAX_BOX_WIDTH) {
			y += CDogsTextHeight();
			x = ix;
			lines++;
			ws = word;
		}

		for (p = ws; p < word; p++)
			x += CDogsTextCharWidth(*p);

		CDogsTextGoto(x, y);

		for (p = word; p < s; p++) {
			CDogsTextChar(*p);
			x += CDogsTextCharWidth(*p);
		}
	}

	return lines;
}

void CampaignIntro(GraphicsDevice *device)
{
	int y;
	char s[1024];

	debug(D_NORMAL, "\n");

	GraphicsBlitBkg(device);

	y = device->cachedConfig.ResolutionWidth / 4;

	sprintf(s, "%s by %s", gCampaign.Setting.title, gCampaign.Setting.author);
	CDogsTextStringSpecial(s, TEXT_TOP | TEXT_XCENTER, 0, (y - 25));

	MissionDescription(y, gCampaign.Setting.description);

	BlitFlip(device, &gConfig.Graphics);
	WaitForAnyKeyOrButton(&gInputDevices);
}

void MissionBriefing(GraphicsDevice *device)
{
	char s[512];
	int i, y;

	GraphicsBlitBkg(device);

	y = device->cachedConfig.ResolutionWidth / 4;

	sprintf(s, "Mission %d: %s", gMission.index + 1, gMission.missionData->title);
	CDogsTextStringSpecial(s, TEXT_TOP | TEXT_XCENTER, 0, (y - 25));

	// Save password if we're not on the first mission
	if (gMission.index > 0)
	{
		char str[512];
		MissionSave ms;
		ms.Campaign = gCampaign.Entry;
		strcpy(ms.Password, MakePassword(gMission.index, 0));
		AutosaveAddMission(&gAutosave, &ms);
		AutosaveSave(&gAutosave, GetConfigFilePath(AUTOSAVE_FILE));
		sprintf(str, "Password: %s", gAutosave.LastMission.Password);
		CDogsTextStringSpecial(str, TEXT_TOP | TEXT_XCENTER, 0, (y - 15));
	}

	y += CDogsTextHeight() * MissionDescription(y, gMission.missionData->description);

	y += device->cachedConfig.ResolutionHeight / 10;

	for (i = 0; i < gMission.missionData->objectiveCount; i++)
	{
		if (gMission.missionData->objectives[i].required > 0)
		{
			CDogsTextStringAt(
				device->cachedConfig.ResolutionWidth / 6,
				y,
				gMission.missionData->objectives[i].description);
			DrawObjectiveInfo(
				i,
				device->cachedConfig.ResolutionWidth - (device->cachedConfig.ResolutionWidth / 6),
				y + 8,
				gMission.missionData);

			y += device->cachedConfig.ResolutionHeight / 12;
		}
	}

	BlitFlip(device, &gConfig.Graphics);
	WaitForAnyKeyOrButton(&gInputDevices);
}

void Summary(int x, struct PlayerData *data, int character)
{
	char s[50];
	int y = gGraphicsDevice.cachedConfig.ResolutionHeight / 3;

	if (strlen(gAutosave.LastMission.Password) > 0)
	{
		char s1[512];
		sprintf(s1, "Last password: %s", gAutosave.LastMission.Password);
		CDogsTextStringSpecial(
			s1,
			TEXT_BOTTOM | TEXT_XCENTER,
			0,
			gGraphicsDevice.cachedConfig.ResolutionHeight / 12);
	}

	if (data->survived) {
		if (data->hp > 150)
			data->totalScore += (data->hp - 150) * 10;
		else if (data->hp <= 0)
			data->totalScore -= 500;

		if (data->friendlies > 5
		    && data->friendlies > data->kills / 2)
			data->totalScore -= 100 * data->friendlies;
		else if (data->weaponCount == 1 &&
			 data->weapons[0] == GUN_KNIFE &&
			 data->friendlies == 0 && data->kills > 5)
			data->totalScore += 50 * data->kills;
		else if (data->kills == 0 && data->friendlies == 0)
			data->totalScore += 500;
	}

	if (data->survived)
		CDogsTextStringAt(x, y, "Completed mission");
	else
		CDogsTextStringWithTableAt(x, y, "Failed mission", &tableFlamed);

	y += 2 * CDogsTextHeight();
	DisplayPlayer(
		x,
		data->name,
		&gCampaign.Setting.characters.players[character],
		0);
	sprintf(s, "Score: %d", data->score);
	CDogsTextStringAt(x, y, s);
	y += CDogsTextHeight();
	sprintf(s, "Total: %d", data->totalScore);
	CDogsTextStringAt(x, y, s);
	y += CDogsTextHeight();
	sprintf(s, "Missions: %d",
		data->missions + (data->survived ? 1 : 0));
	CDogsTextStringAt(x, y, s);
	y += CDogsTextHeight();

	if (data->survived && (data->hp > 150 || data->hp <= 0))
	{
		int maxHealth = (200 * gConfig.Game.PlayerHP) / 100;
		if (data->hp > maxHealth - 50)
		{
			sprintf(s, "Health bonus: %d", (data->hp + 50 - maxHealth) * 10);
		}
		else if (data->hp <= 0)
		{
			sprintf(s, "Resurrection fee: %d", -500);
		}
		CDogsTextStringAt(x, y, s);
		y += CDogsTextHeight();
	}

	if (data->friendlies > 0 && data->friendlies > data->kills / 2) {
		sprintf(s, "Butcher penalty: %d", 100 * data->friendlies);
		CDogsTextStringAt(x, y, s);
		y += CDogsTextHeight();
	} else if (data->weaponCount == 1 &&
			data->weapons[0] == GUN_KNIFE && data->kills > 0) {
		sprintf(s, "Ninja bonus: %d", 50 * data->kills);
		CDogsTextStringAt(x, y, s);
		y += CDogsTextHeight();
	} else if (data->kills == 0 && data->friendlies == 0) {
		sprintf(s, "Friendly bonus: %d", 500);
		CDogsTextStringAt(x, y, s);
		y += CDogsTextHeight();
	}
}

static int AreAnySurvived(void)
{
	int i;
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		if (gPlayerDatas[i].survived)
		{
			return 1;
		}
	}
	return 0;
}

void Bonuses(void)
{
	int i;
	int y = (gGraphicsDevice.cachedConfig.ResolutionHeight / 2) + (gGraphicsDevice.cachedConfig.ResolutionHeight / 10);
	int x = gGraphicsDevice.cachedConfig.ResolutionWidth / 6;
	int done, req, total;
	int access_bonus = 0;
	int idx = 1;
	char s[100];
	int bonus = 0;

	for (i = 0; i < gMission.missionData->objectiveCount; i++)
	{
		done = gMission.objectives[i].done;
		req = gMission.objectives[i].required;
		total = gMission.objectives[i].count;

		if (done > 0 || req > 0) {
			DrawObjectiveInfo(i, x - 26, y + 8, gMission.missionData);
			sprintf(s, "Objective %d: %d of %d, %d required",
				idx, done, total, req);
			if (req > 0)
			{
				CDogsTextStringSpecial(s, TEXT_LEFT | TEXT_TOP, x, y);
			}
			else
				CDogsTextStringSpecial(s,
						TEXT_LEFT | TEXT_TOP | TEXT_PURPLE,
						x, y);
			if (done < req)
			{
				CDogsTextStringSpecial(
					"Failed", TEXT_RIGHT | TEXT_TOP | TEXT_FLAMED, x, y);
			}
			else if (done == total && done > req && AreAnySurvived())
			{
				CDogsTextStringSpecial(
					"Perfect: 500", TEXT_RIGHT | TEXT_TOP, x, y);
				bonus += 500;
			}
			else if (req > 0)
			{
				CDogsTextStringSpecial("Done", TEXT_RIGHT | TEXT_TOP, x, y);
			}
			else
			{
				CDogsTextStringSpecial("Bonus!", TEXT_RIGHT | TEXT_TOP, x, y);
			}

			y += 15;
			idx++;
		}
	}

	if (gMission.flags & FLAGS_KEYCARD_YELLOW)	access_bonus += 50;
	if (gMission.flags & FLAGS_KEYCARD_GREEN)	access_bonus += 100;
	if (gMission.flags & FLAGS_KEYCARD_BLUE)	access_bonus += 150;
	if (gMission.flags & FLAGS_KEYCARD_RED)		access_bonus += 200;
	if (access_bonus > 0 && AreAnySurvived())
	{
		sprintf(s, "Access bonus: %d", access_bonus);
		CDogsTextStringAt(x, y, s);
		y += CDogsTextHeight() + 1;
		bonus += access_bonus;
	}

	i = 60 + gMission.missionData->objectiveCount * 30 - missionTime / 70;

	if (i > 0 && AreAnySurvived())
	{
		int timeBonus = i * 25;
		sprintf(s, "Time bonus: %d secs x 25 = %d", i, timeBonus);
		CDogsTextStringAt(x, y, s);
		bonus += timeBonus;
	}

	// only survivors get the spoils!
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		if (gPlayerDatas[i].survived)
		{
			gPlayerDatas[i].totalScore += bonus;
		}
	}
}

void MissionSummary(GraphicsDevice *device)
{
	GraphicsBlitBkg(device);

	Bonuses();

	switch (gOptions.numPlayers)
	{
		case 1:
			Summary(CenterX(60), &gPlayerDatas[0], CHARACTER_PLAYER1);
			break;
		case 2:
			Summary(CenterOfLeft(60), &gPlayerDatas[0], CHARACTER_PLAYER1);
			Summary(CenterOfRight(60), &gPlayerDatas[1], CHARACTER_PLAYER2);
			break;
		default:
			assert(0 && "not implemented");
			break;
	}

	BlitFlip(device, &gConfig.Graphics);
	WaitForAnyKeyOrButton(&gInputDevices);
}

void ShowScore(GraphicsDevice *device, int scores[MAX_PLAYERS])
{
	char s[10];

	GraphicsBlitBkg(device);

	debug(D_NORMAL, "\n");

	switch (gOptions.numPlayers)
	{
		case 1:
			DisplayPlayer(
				CenterX(TextGetStringWidth(s)),
				gPlayerDatas[0].name,
				&gCampaign.Setting.characters.players[0],
				0);
			break;
		case 2:
			DisplayPlayer(
				CenterOfLeft(60),
				gPlayerDatas[0].name,
				&gCampaign.Setting.characters.players[0],
				0);
			sprintf(s, "Score: %d", scores[0]);
			CDogsTextStringAt(
				CenterOfLeft(TextGetStringWidth(s)),
				device->cachedConfig.ResolutionWidth / 3,
				s);

			DisplayPlayer(
				CenterOfRight(60), gPlayerDatas[1].name,
				&gCampaign.Setting.characters.players[1],
				0);
			sprintf(s, "Score: %d", scores[1]);
			CDogsTextStringAt(
				CenterOfRight(TextGetStringWidth(s)),
				device->cachedConfig.ResolutionWidth / 3,
				s);
	}

	BlitFlip(device, &gConfig.Graphics);
	WaitForAnyKeyOrButton(&gInputDevices);
}

void FinalScore(GraphicsDevice *device, int scores[MAX_PLAYERS])
{
	int i;
	int isTie = 0;
	int maxScore = 0;
	int maxScorePlayer = 0;
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		if (scores[i] > maxScore)
		{
			maxScore = scores[i];
			maxScorePlayer = i;
		}
		else if (scores[i] == maxScore)
		{
			isTie = 1;
		}
	}

	GraphicsBlitBkg(device);

#define IS_DRAW		"It's a draw!"
#define IS_WINNER	"Winner!"

	if (isTie)
	{
		// TODO: more players
		DisplayPlayer(
			CenterOfLeft(60),
			gPlayerDatas[0].name,
			&gCampaign.Setting.characters.players[0],
			0);
		DisplayPlayer(
			CenterOfRight(60),
			gPlayerDatas[1].name,
			&gCampaign.Setting.characters.players[1],
			0);
		CDogsTextStringAtCenter("It's a draw!");
	}
	else if (maxScorePlayer == 0)	// TODO: more players
	{
		DisplayPlayer(
			CenterOfLeft(60),
			gPlayerDatas[0].name,
			&gCampaign.Setting.characters.players[0],
			0);
		CDogsTextStringAt(
			CenterOfLeft(TextGetStringWidth(IS_WINNER)),
			device->cachedConfig.ResolutionWidth / 2,
			IS_WINNER);
	}
	else
	{
		DisplayPlayer(
			CenterOfRight(60),
			gPlayerDatas[1].name,
			&gCampaign.Setting.characters.players[1],
			0);
		CDogsTextStringAt(
			CenterOfRight(TextGetStringWidth(IS_WINNER)),
			device->cachedConfig.ResolutionWidth / 2,
			IS_WINNER);
	}
	BlitFlip(device, &gConfig.Graphics);
	WaitForAnyKeyOrButton(&gInputDevices);
}


static const char *finalWords1P[] = {
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

static const char *finalWords2P[] = {
	"United we stand, divided we conquer",
	"Nothing like good teamwork, is there?",
	"Which way is the camera?",
	"We eat bullets for breakfast and have grenades as dessert",
	"We're so cool we have to wear mittens",
};

#define CONGRATULATIONS "Congratulations, you have completed "

void Victory(GraphicsDevice *graphics)
{
	int x, i;
	const char *s = NULL;

	GraphicsBlitBkg(graphics);

	x = 160 - TextGetStringWidth(CONGRATULATIONS) / 2;
	CDogsTextStringAt(x, 100, CONGRATULATIONS);
	x = 160 - TextGetStringWidth(gCampaign.Setting.title) / 2;
	CDogsTextStringWithTableAt(x, 115, gCampaign.Setting.title, &tableFlamed);

	switch (gOptions.numPlayers)
	{
		case 1:
			i = sizeof(finalWords1P) / sizeof(char *);
			i = rand() % i;
			s = finalWords1P[i];
			DisplayPlayer(
				125,
				gPlayerDatas[0].name,
				&gCampaign.Setting.characters.players[0],
				0);
			break;
		case 2:
			i = sizeof(finalWords2P) / sizeof(char *);
			i = rand() % i;
			s = finalWords2P[i];
			DisplayPlayer(
				50,
				gPlayerDatas[0].name,
				&gCampaign.Setting.characters.players[0],
				0);
			DisplayPlayer(
				200,
				gPlayerDatas[1].name,
				&gCampaign.Setting.characters.players[1],
				0);
			break;
		default:
			assert(0 && "not implemented");
			break;
	}
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		if (gPlayerDatas[i].survived)
		{
			gPlayerDatas[i].missions++;
		}
	}

	x = 160 - TextGetStringWidth(s) / 2;
	CDogsTextGoto(x, 140);
	CDogsTextCharWithTable('"', &tableDarker);
	CDogsTextStringWithTable(s, &tablePurple);
	CDogsTextCharWithTable('"', &tableDarker);

	SoundPlay(&gSoundDevice, SND_HAHAHA);

	BlitFlip(graphics, &gConfig.Graphics);
	WaitForAnyKeyOrButton(&gInputDevices);
}


static void PlaceActor(TActor * actor)
{
	int x, y;

	do {
		x = ((rand() % (XMAX * TILE_WIDTH)) << 8);
		y = ((rand() % (YMAX * TILE_HEIGHT)) << 8);
	}
	while (!OKforPlayer(x, y) || !MoveActor(actor, x, y));
}
static void PlaceActorNear(TActor *actor, Vec2i near)
{
	// Try a concentric rhombus pattern, clockwise from right
	// That is, start by checking right, below, left, above,
	// then continue with radius 2 right, below-right, below, below-left...
	// (start from S:)
	//      4
	//  9 3 S 1 5
	//    8 2 6
	//      7
#define TRY_LOCATION()\
if (OKforPlayer(near.x + dx, near.y + dy) &&\
MoveActor(actor, near.x + dx, near.y + dy))\
{\
	return;\
}
	int radius;
	for (radius = 1; ; radius++)
	{
		int dx, dy;
		// Going from right to below
		for (dx = radius, dy = 0; dy < radius; dx--, dy++)
		{
			TRY_LOCATION();
		}
		// below to left
		for (dx = 0, dy = radius; dy > 0; dx--, dy--)
		{
			TRY_LOCATION();
		}
		// left to above
		for (dx = -radius, dy = 0; dx < 0; dx++, dy--)
		{
			TRY_LOCATION();
		}
		// above to right
		for (dx = 0, dy = -radius; dy < 0; dx++, dy++)
		{
			TRY_LOCATION();
		}
	}
}

void InitData(struct PlayerData *data)
{
	data->totalScore = 0;
	data->missions = 0;
}

void DataUpdate(int mission, struct PlayerData *data)
{
	if (!data->survived) {
		data->totalScore = 0;
		data->missions = 0;
	} else
		data->missions++;
	data->lastMission = mission;
}

static void CleanupMission(void)
{
	int i;
	KillAllActors();
	KillAllMobileObjects(&gMobObjList);
	KillAllObjects();
	FreeTriggersAndWatches();
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		gPlayers[i] = NULL;
	}
}

static void InitPlayers(int numPlayers, int maxHealth, int mission)
{
	int i;
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		gPlayerDatas[i].score = 0;
		gPlayerDatas[i].kills = 0;
		gPlayerDatas[i].friendlies = 0;
		gPlayerDatas[i].allTime = -1;
		gPlayerDatas[i].today = -1;
	}
	for (i = 0; i < numPlayers; i++)
	{
		gPlayerDatas[i].lastMission = mission;
		gPlayers[i] = AddActor(
			&gCampaign.Setting.characters.players[i],
			&gPlayerDatas[i]);
		gPlayers[i]->weapon = WeaponCreate(gPlayerDatas[i].weapons[0]);
		gPlayers[i]->health = maxHealth;
		gPlayers[i]->character->maxHealth = maxHealth;
		
		// If never split screen, try to place players near the first player
		if (gConfig.Interface.Splitscreen == SPLITSCREEN_NEVER &&
			i > 0)
		{
			PlaceActorNear(
				gPlayers[i], Vec2iNew(gPlayers[0]->x, gPlayers[0]->y));
		}
		else
		{
			PlaceActor(gPlayers[i]);
		}
	}
}

static void PlayGameSong(void)
{
	int success = 0;
	// Play a tune
	// Start by trying to play a mission specific song,
	// otherwise pick one from the general collection...
	MusicStop(&gSoundDevice);
	if (strlen(gMission.missionData->song) > 0)
	{
		char buf[CDOGS_PATH_MAX];
		size_t pathLen = MAX(
			strrchr(gCampaign.Entry.path, '\\'),
			strrchr(gCampaign.Entry.path, '/')) - gCampaign.Entry.path;
		strncpy(buf, gCampaign.Entry.path, pathLen);
		buf[pathLen] = '\0';

		strcat(buf, "/");
		strcat(buf, gMission.missionData->song);
		success = !MusicPlay(&gSoundDevice, buf);
	}
	if (!success && gGameSongs != NULL)
	{
		MusicPlay(&gSoundDevice, gGameSongs->path);
		ShiftSongs(&gGameSongs);
	}
}

static void PlayMenuSong(void)
{
	MusicStop(&gSoundDevice);
	if (gMenuSongs)
	{
		MusicPlay(&gSoundDevice, gMenuSongs->path);
		ShiftSongs(&gMenuSongs);
	}
}


int Game(GraphicsDevice *graphics, int mission)
{
	int run, gameOver;
	int allTime, todays;
	int maxHealth;

	maxHealth = 200 * gConfig.Game.PlayerHP / 100;

	do
	{
		int i;
		SetupMission(mission, 1, &gCampaign);

		SetupMap();

		srand((unsigned int)time(NULL));
		InitializeBadGuys();
		if (IsMissionBriefingNeeded(gCampaign.Entry.mode))
		{
			MissionBriefing(graphics);
		}
		PlayerEquip(gOptions.numPlayers, graphics);

		InitPlayers(gOptions.numPlayers, maxHealth, mission);

		CreateEnemies();

		PlayGameSong();

		run = gameloop();

		gameOver = GetNumPlayersAlive()	== 0 ||
			mission == gCampaign.Setting.missionCount - 1;

		for (i = 0; i < MAX_PLAYERS; i++)
		{
			gPlayerDatas[i].survived = IsPlayerAlive(i);
			if (gPlayers[i])
			{
				gPlayerDatas[i].hp = gPlayers[i]->health;
			}
		}

		CleanupMission();

		PlayMenuSong();
		printf(">> Starting\n");

		if (run)
		{
			MissionSummary(graphics);
			if (gameOver && GetNumPlayersAlive() > 0)
			{
				Victory(graphics);
			}
		}

		allTime = todays = 0;
		for (i = 0; i < gOptions.numPlayers; i++)
		{
			if ((run && !gPlayerDatas[i].survived) || gameOver)
			{
				EnterHighScore(&gPlayerDatas[i]);
				allTime = gPlayerDatas[i].allTime >= 0;
				todays = gPlayerDatas[i].today >= 0;
			}
			DataUpdate(mission, &gPlayerDatas[i]);
		}
		if (allTime && !gameOver)
		{
			DisplayAllTimeHighScores(graphics);
		}
		if (todays && !gameOver)
		{
			DisplayTodaysHighScores(graphics);
		}

		mission++;
	}
	while (run && !gameOver);
	return run;
}

int Campaign(GraphicsDevice *graphics)
{
	int mission = 0;
	int i;
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		InitData(&gPlayerDatas[i]);
	}

	if (IsPasswordAllowed(gCampaign.Entry.mode))
	{
		MissionSave m;
		AutosaveLoadMission(&gAutosave, &m, gCampaign.Entry.path);
		mission = EnterPassword(graphics, m.Password);
	}

	return Game(graphics, mission);
}

void DogFight(GraphicsDevice *graphicsDevice)
{
	int run;
	int scores[MAX_PLAYERS];
	int maxScore = 0;
	int numPlayers = gOptions.numPlayers;
	int i;
	// TODO: seems like something is doing naughty things to our state

	for (i = 0; i < MAX_PLAYERS; i++)
	{
		scores[i] = 0;
		InitData(&gPlayerDatas[i]);
	}

	gOptions.badGuys = 0;

	do
	{
		SetupMission(0, 1, &gCampaign);
		SetupMap();

		if (PlayerEquip(gOptions.numPlayers, graphicsDevice))
		{
			srand((unsigned int)time(NULL));
			InitPlayers(gOptions.numPlayers, 500, 0);
			PlayGameSong();
			run = gameloop();
		}
		else
		{
			run = 0;
		}

		for (i = 0; i < MAX_PLAYERS; i++)
		{
			if (gPlayers[i])
			{
				scores[i]++;
				if (scores[i] > maxScore)
				{
					maxScore = scores[i];
				}
			}
		}

		CleanupMission();
		PlayMenuSong();

		if (run)
		{
			ShowScore(graphicsDevice, scores);
		}

	} while (run && maxScore < 5);

	gOptions.badGuys = 1;
	gOptions.numPlayers = numPlayers;

	if (run)
	{
		FinalScore(graphicsDevice, scores);
	}
}

void MainLoop(credits_displayer_t *creditsDisplayer, custom_campaigns_t *campaigns)
{
	while (MainMenu(&gGraphicsDevice, creditsDisplayer, campaigns))
	{
		debug(D_NORMAL, ">> Entering campaign\n");
		if (IsIntroNeeded(gCampaign.Entry.mode))
		{
			CampaignIntro(&gGraphicsDevice);
		}

		debug(D_NORMAL, ">> Select number of players\n");
		if (!NumPlayersSelection(
				&gOptions.numPlayers, gCampaign.Entry.mode,
				&gGraphicsDevice, &gInputDevices))
		{
			continue;
		}

		debug(D_NORMAL, ">> Entering selection\n");
		if (!PlayerSelection(gOptions.numPlayers, &gGraphicsDevice))
		{
			continue;
		}

		debug(D_NORMAL, ">> Starting campaign\n");
		if (gCampaign.Entry.mode == CAMPAIGN_MODE_DOGFIGHT)
		{
			DogFight(&gGraphicsDevice);
		}
		else if (Campaign(&gGraphicsDevice))
		{
			DisplayAllTimeHighScores(&gGraphicsDevice);
			DisplayTodaysHighScores(&gGraphicsDevice);
		}
	}
	debug(D_NORMAL, ">> Leaving Main Game Loop\n");
}

void PrintTitle(void)
{
	printf("C-Dogs SDL %s\n", CDOGS_SDL_VERSION);

	printf("Original Code Copyright Ronny Wester 1995\n");
	printf("Game Data Copyright Ronny Wester 1995\n");
	printf("SDL Port by Jeremy Chin, Lucas Martin-King and Cong Xu, Copyright 2003-2013\n\n");
	printf("%s%s%s%s",
		"C-Dogs SDL comes with ABSOLUTELY NO WARRANTY;\n",
		"see the file COPYING that came with this distibution...\n",
		"This is free software, and you are welcome to redistribute it\n",
		"under certain conditions; for details see COPYING.\n\n");
}

void PrintHelp (void)
{
	printf("%s\n",
		"Video Options:\n"
		"    --fullscreen     Try and use a fullscreen video mode.\n"
		"    --scale=n        Scale the window resolution up by a factor of n\n"
		"                       Factors: 2, 3, 4\n"
		"    --screen=WxH     Set virtual screen width to W x H\n"
		"                       Modes: 320x200, 320x240, 400x300, 640x480, 800x600, 1024x768\n"
		"    --forcemode      Don't check video mode sanity\n"
	);

	printf("%s\n",
		"Sound Options:\n"
		"    --nosound        Disable sound\n"
	);

	printf("%s\n",
		"Control Options:\n"
		"    --nojoystick     Disable joystick(s)\n"
	);

	printf("%s\n",
		"Game Options:\n"
		"    --wait           Wait for a key hit before initialising video.\n"
		"    --shakemult=n    Screen shaking multiplier (0 = disable).\n"
	);

	printf("%s\n",
		"The DEBUG environment variable can be set to show debug information.");

	printf(
		"The DEBUG_LEVEL environment variable can be set to between %d and %d.\n", D_NORMAL, D_MAX
	);
}

int main(int argc, char *argv[])
{
	int wait = 0;
	int snd_flag = SDL_INIT_AUDIO;
	int js_flag = SDL_INIT_JOYSTICK;
	int isSoundEnabled = 1;
	credits_displayer_t creditsDisplayer;
	custom_campaigns_t campaigns;
	int forceResolution = 0;

	srand((unsigned int)time(NULL));

	PrintTitle();

	{
		char *dbg;
		if (getenv("DEBUG") != NULL) debug = 1;
		if ((dbg = getenv("DEBUG_LEVEL")) != NULL) {
			debug_level = atoi(dbg);
			if (debug_level < 0) debug_level = 0;
			if (debug_level > D_MAX) debug_level = D_MAX;
		}
	}

	SetupConfigDir();
	ConfigLoadDefault(&gConfig);
	ConfigLoad(&gConfig, GetConfigFilePath(CONFIG_FILE));
	LoadCredits(&creditsDisplayer, &tablePurple, &tableDarker);
	AutosaveInit(&gAutosave);
	AutosaveLoad(&gAutosave, GetConfigFilePath(AUTOSAVE_FILE));

	{
		struct option longopts[] =
		{
			{"fullscreen",	no_argument,		NULL,	'f'},
			{"scale",		required_argument,	NULL,	's'},
			{"screen",		required_argument,	NULL,	'c'},
			{"forcemode",	no_argument,		NULL,	'o'},
			{"nosound",		no_argument,		NULL,	'n'},
			{"nojoystick",	no_argument,		NULL,	'j'},
			{"wait",		no_argument,		NULL,	'w'},
			{"shakemult",	required_argument,	NULL,	'm'},
			{"help",		no_argument,		NULL,	'h'},
			{0,				0,					NULL,	0}
		};
		int opt = 0;
		int idx = 0;
		while ((opt = getopt_long(argc, argv,"fs:c:onjwm:h", longopts, &idx)) != -1)
		{
			switch (opt)
			{
			case 'f':
				gConfig.Graphics.Fullscreen = 1;
				break;
			case 's':
				gConfig.Graphics.ScaleFactor = atoi(optarg);
				break;
			case 'c':
				sscanf(optarg, "%dx%d",
					&gConfig.Graphics.ResolutionWidth,
					&gConfig.Graphics.ResolutionHeight);
				debug(D_NORMAL, "Video mode %dx%d set...\n",
					gConfig.Graphics.ResolutionWidth,
					gConfig.Graphics.ResolutionHeight);
				break;
			case 'o':
				forceResolution = 1;
				break;
			case 'n':
				printf("Sound disabled!\n");
				snd_flag = 0;
				isSoundEnabled = 0;
				break;
			case 'j':
				debug(D_NORMAL, "nojoystick\n");
				js_flag = 0;
				break;
			case 'w':
				wait = 1;
				break;
			case 'm':
				{
					gConfig.Graphics.ShakeMultiplier = atoi(optarg);
					if (gConfig.Graphics.ShakeMultiplier < 0)
					{
						gConfig.Graphics.ShakeMultiplier = 0;
					}
					printf("Shake multiplier: %d\n", gConfig.Graphics.ShakeMultiplier);
				}
				break;
			case 'h':
				PrintHelp();
				exit(EXIT_SUCCESS);
			default:
				PrintHelp();
				exit(EXIT_FAILURE);
			}
		}
	}

	debug(D_NORMAL, "Initialising SDL...\n");
	if (SDL_Init(SDL_INIT_TIMER | snd_flag | SDL_INIT_VIDEO | js_flag) != 0) {
		printf("Failed to start SDL!\n");
		return -1;
	}

	printf("Data directory:\t\t%s\n",	GetDataFilePath(""));
	printf("Config directory:\t%s\n\n",	GetConfigFilePath(""));

	if (!PicManagerTryInit(
		&gPicManager, "graphics/cdogs.px", "graphics/cdogs2.px"))
	{
		exit(0);
	}
	memcpy(origPalette, gPicManager.palette, sizeof(origPalette));

	CDogsTextInit(GetDataFilePath("graphics/font.px"), -2);

	if (isSoundEnabled)
	{
		SoundInitialize(&gSoundDevice, &gConfig.Sound);
		if (!gSoundDevice.isInitialised)
		{
			printf("Sound initialization failed!\n");
		}
	}

	LoadHighScores();

	debug(D_NORMAL, "Loading song lists...\n");
	LoadSongs();
	LoadPlayerTemplates(gPlayerTemplates);

	PlayMenuSong();

	CampaignInit(&gCampaign);
	LoadAllCampaigns(&campaigns);
	InputInit(&gInputDevices, NULL);

	if (wait)
	{
		printf("Press the enter key to continue...\n");
		getchar();
	}

	BulletInitialize();
	WeaponInitialize();
	PlayerDataInitialize();
	GraphicsInit(&gGraphicsDevice);
	GraphicsInitialize(
		&gGraphicsDevice, &gConfig.Graphics, gPicManager.palette,
		forceResolution);
	if (!gGraphicsDevice.IsInitialized)
	{
		Config defaultConfig;
		printf("Cannot initialise video; trying default config\n");
		ConfigLoadDefault(&defaultConfig);
		gConfig.Graphics = defaultConfig.Graphics;
		GraphicsInitialize(
			&gGraphicsDevice, &gConfig.Graphics, gPicManager.palette,
			forceResolution);
	}
	if (!gGraphicsDevice.IsInitialized)
	{
		printf("Video didn't init!\n");
		exit(EXIT_FAILURE);
	}
	else
	{
		debug(D_NORMAL, ">> Entering main loop\n");
		MainLoop(&creditsDisplayer, &campaigns);
	}
	debug(D_NORMAL, ">> Shutting down...\n");
	InputTerminate(&gInputDevices);
	GraphicsTerminate(&gGraphicsDevice);

	PicManagerTerminate(&gPicManager);
	AutosaveSave(&gAutosave, GetConfigFilePath(AUTOSAVE_FILE));
	ConfigSave(&gConfig, GetConfigFilePath(CONFIG_FILE));
	SavePlayerTemplates(gPlayerTemplates);
	FreeSongs(&gMenuSongs);
	FreeSongs(&gGameSongs);
	SaveHighScores();
	UnloadCredits(&creditsDisplayer);
	UnloadAllCampaigns(&campaigns);
	CampaignTerminate(&gCampaign);

	if (isSoundEnabled)
	{
		debug(D_NORMAL, ">> Shutting down sound...\n");
		SoundTerminate(&gSoundDevice, 1);
	}

	debug(D_NORMAL, "SDL_Quit()\n");
	SDL_Quit();

	printf("Bye :)\n");

	exit(EXIT_SUCCESS);
}

