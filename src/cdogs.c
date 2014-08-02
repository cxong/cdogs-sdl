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
#include <cdogs/font.h>
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
#include <cdogs/particle.h>
#include <cdogs/pic_manager.h>
#include <cdogs/pics.h>
#include <cdogs/player_template.h>
#include <cdogs/sounds.h>
#include <cdogs/text.h>
#include <cdogs/triggers.h>
#include <cdogs/utils.h>

#include <cdogs/physfs/physfs.h>

#include "autosave.h"
#include "credits.h"
#include "game.h"
#include "mainmenu.h"
#include "password.h"
#include "player_select_menus.h"
#include "prep.h"
#include "XGetopt.h"



static void DrawObjectiveInfo(int idx, int x, int y, struct MissionOptions *mo)
{
	TOffsetPic pic;
	TranslationTable *table = NULL;
	int i = 0;
	Character *cd;
	MissionObjective *mobj = CArrayGet(&mo->missionData->Objectives, idx);
	struct Objective *o = CArrayGet(&mo->Objectives, idx);

	switch (mobj->Type)
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
		i = o->pickupItem;
		pic = cGeneralPics[i];
		break;
	case OBJECTIVE_DESTROY:
		i = o->blowupObject->pic;
		pic = cGeneralPics[i];
		break;
	case OBJECTIVE_INVESTIGATE:
		pic.dx = pic.dy = 0;
		pic.picIndex = -1;
		return;
	default:
		i = o->pickupItem;
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

int CampaignIntro(GraphicsDevice *device)
{
	int x;
	int y;
	char s[1024];
	int w = device->cachedConfig.Res.x;
	int h = device->cachedConfig.Res.y;

	debug(D_NORMAL, "\n");

	GraphicsBlitBkg(device);

	y = h / 4;

	sprintf(s, "%s by %s", gCampaign.Setting.Title, gCampaign.Setting.Author);
	CDogsTextStringSpecial(s, TEXT_TOP | TEXT_XCENTER, 0, (y - 25));

	FontSplitLines(gCampaign.Setting.Description, s, w * 5 / 6);
	x = w / 6 / 2;
	FontStr(s, Vec2iNew(x, y));

	BlitFlip(device, &gConfig.Graphics);
	int result = WaitForAnyKeyOrButton(&gEventHandlers);
	if (result)
	{
		SoundPlay(&gSoundDevice, StrSound("mg"));
	}
	return result;
}

void MissionBriefing(GraphicsDevice *device)
{
	char s[512];
	int y;
	int w = device->cachedConfig.Res.x;
	int h = device->cachedConfig.Res.y;
	int typewriterCount;
	char description[1024];
	char typewriterBuf[1024];
	int descriptionHeight;
	
	FontSplitLines(gMission.missionData->Description, description, w * 5 / 6);
	descriptionHeight = FontStrH(description);
	
	// Save password if we're not on the first mission
	if (gMission.index > 0)
	{
		MissionSave ms;
		MissionSaveInit(&ms);
		ms.Campaign = gCampaign.Entry;
		strcpy(ms.Password, MakePassword(gMission.index, 0));
		ms.MissionsCompleted = gMission.index;
		AutosaveAddMission(&gAutosave, &ms, ms.Campaign.BuiltinIndex);
		AutosaveSave(&gAutosave, GetConfigFilePath(AUTOSAVE_FILE));
	}
	
	EventReset(&gEventHandlers, gEventHandlers.mouse.cursor);

	for (typewriterCount = 0;
		typewriterCount <= (int)strlen(description);
		typewriterCount++)
	{
		Vec2i pos;
		int i;
		int cmds[MAX_PLAYERS];
		
		// Check for player input; if any then skip to the end of the briefing
		memset(cmds, 0, sizeof cmds);
		EventPoll(&gEventHandlers, SDL_GetTicks());
		GetPlayerCmds(&gEventHandlers, &cmds, gPlayerDatas);
		for (i = 0; i < MAX_PLAYERS; i++)
		{
			if (AnyButton(cmds[i]))
			{
				typewriterCount = (int)strlen(description);
				break;
			}
		}
		
		GraphicsBlitBkg(device);

		// Mission title
		y = h / 4;
		sprintf(s, "Mission %d: %s",
			gMission.index + 1, gMission.missionData->Title);
		CDogsTextStringSpecial(s, TEXT_TOP | TEXT_XCENTER, 0, (y - 25));

		// Display password
		if (gMission.index > 0)
		{
			char str[512];
			sprintf(str, "Password: %s", gAutosave.LastMission.Password);
			CDogsTextStringSpecial(str, TEXT_TOP | TEXT_XCENTER, 0, (y - 15));
		}

		// Display description with typewriter effect
		pos = Vec2iNew(w / 6 / 2, y);
		strncpy(typewriterBuf, description, typewriterCount);
		typewriterBuf[typewriterCount] = '\0';
		FontStr(typewriterBuf, pos);
		y += descriptionHeight;

		y += h / 10;

		// Display objectives
		for (i = 0; i < (int)gMission.missionData->Objectives.size; i++)
		{
			MissionObjective *o =
				CArrayGet(&gMission.missionData->Objectives, i);
			// Do not brief optional objectives
			if (o->Required == 0)
			{
				continue;
			}
			FontStr(o->Description, Vec2iNew(w / 6, y));
			DrawObjectiveInfo(i, w - (w / 6), y + FontH(), &gMission);
			y += h / 12;
		}

		BlitFlip(device, &gConfig.Graphics);
		
		SDL_Delay(10);
	}
	WaitForAnyKeyOrButton(&gEventHandlers);
}

// Display compact player summary, with player on left half and score summaries
// on right half
void Summary(Vec2i pos, Vec2i size, struct PlayerData *data, int character)
{
	char s[50];
	int totalTextHeight = FontH() * 7;
	// display text on right half
	Vec2i textPos = Vec2iNew(
		pos.x + size.x / 2, CENTER_Y(pos, size, totalTextHeight));

	DisplayCharacterAndName(
		Vec2iAdd(pos, Vec2iNew(size.x / 4, size.y / 2)),
		&gCampaign.Setting.characters.players[character],
		data->name);

	if (data->survived)
	{
		FontStr("Completed mission", textPos);
	}
	else
	{
		FontStrMask("Failed mission", textPos, colorRed);
	}

	textPos.y += 2 * FontH();
	sprintf(s, "Score: %d", data->score);
	FontStr(s, textPos);
	textPos.y += FontH();
	sprintf(s, "Total: %d", data->totalScore);
	FontStr(s, textPos);
	textPos.y += FontH();
	sprintf(s, "Missions: %d", data->missions + (data->survived ? 1 : 0));
	FontStr(s, textPos);
	textPos.y += FontH();

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
		FontStr(s, textPos);
		textPos.y += FontH();
	}

	if (data->friendlies > 0 && data->friendlies > data->kills / 2)
	{
		sprintf(s, "Butcher penalty: %d", 100 * data->friendlies);
		FontStr(s, textPos);
		textPos.y += FontH();
	}
	else if (data->weaponCount == 1 &&
		!data->weapons[0]->CanShoot && data->kills > 0)
	{
		sprintf(s, "Ninja bonus: %d", 50 * data->kills);
		FontStr(s, textPos);
		textPos.y += FontH();
	}
	else if (data->kills == 0 && data->friendlies == 0)
	{
		sprintf(s, "Friendly bonus: %d", 500);
		FontStr(s, textPos);
		textPos.y += FontH();
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
	int y = (gGraphicsDevice.cachedConfig.Res.y / 2) + (gGraphicsDevice.cachedConfig.Res.y / 10);
	int x = gGraphicsDevice.cachedConfig.Res.x / 6;
	int access_bonus = 0;
	int idx = 1;
	char s[100];
	int bonus = 0;

	for (i = 0; i < (int)gMission.missionData->Objectives.size; i++)
	{
		struct Objective *o = CArrayGet(&gMission.Objectives, i);
		MissionObjective *mo = CArrayGet(&gMission.missionData->Objectives, i);

		// Do not mention optional objectives with none completed
		if (o->done == 0 && mo->Required == 0)
		{
			continue;
		}
		
		DrawObjectiveInfo(i, x - 26, y + FontH(), &gMission);
		sprintf(s, "Objective %d: %d of %d, %d required",
			idx, o->done, mo->Count, mo->Required);
		if (mo->Required > 0)
		{
			CDogsTextStringSpecial(s, TEXT_LEFT | TEXT_TOP, x, y);
		}
		else
		{
			CDogsTextStringSpecial(
				s, TEXT_LEFT | TEXT_TOP | TEXT_PURPLE, x, y);
		}
		if (o->done < mo->Required)
		{
			CDogsTextStringSpecial(
				"Failed", TEXT_RIGHT | TEXT_TOP | TEXT_FLAMED, x, y);
		}
		else if (
			o->done == mo->Count && o->done > mo->Required && AreAnySurvived())
		{
			CDogsTextStringSpecial(
				"Perfect: 500", TEXT_RIGHT | TEXT_TOP, x, y);
			bonus += 500;
		}
		else if (mo->Required > 0)
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

	access_bonus += KeycardCount(gMission.flags) * 50;
	if (access_bonus > 0 && AreAnySurvived())
	{
		sprintf(s, "Access bonus: %d", access_bonus);
		FontStr(s, Vec2iNew(x, y));
		y += FontH() + 1;
		bonus += access_bonus;
	}

	i = 60 +
		(int)gMission.missionData->Objectives.size * 30 -
		gMission.time / FPS_FRAMELIMIT;

	if (i > 0 && AreAnySurvived())
	{
		int timeBonus = i * 25;
		sprintf(s, "Time bonus: %d secs x 25 = %d", i, timeBonus);
		FontStr(s, Vec2iNew(x, y));
		bonus += timeBonus;
	}

	// only survivors get the spoils!
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		if (gPlayerDatas[i].survived)
		{
			gPlayerDatas[i].totalScore += bonus;

			// Other per-player bonuses

			if (gPlayerDatas[i].hp > 150)
			{
				// health bonus
				gPlayerDatas[i].totalScore += (gPlayerDatas[i].hp - 150) * 10;
			}
			else if (gPlayerDatas[i].hp <= 0)
			{
				// resurrection fee
				gPlayerDatas[i].totalScore -= 500;
			}

			if (gPlayerDatas[i].friendlies > 5 &&
				gPlayerDatas[i].friendlies > gPlayerDatas[i].kills / 2)
			{
				// butcher penalty
				gPlayerDatas[i].totalScore -= 100 * gPlayerDatas[i].friendlies;
			}
			else if (gPlayerDatas[i].weaponCount == 1 &&
				!gPlayerDatas[i].weapons[0]->CanShoot &&
				gPlayerDatas[i].friendlies == 0 &&
				gPlayerDatas[i].kills > 5)
			{
				// Ninja bonus
				gPlayerDatas[i].totalScore += 50 * gPlayerDatas[i].kills;
			}
			else if (gPlayerDatas[i].kills == 0 &&
				gPlayerDatas[i].friendlies == 0)
			{
				// friendly bonus
				gPlayerDatas[i].totalScore += 500;
			}
		}
	}
}

void MissionSummary(GraphicsDevice *device)
{
	int w = device->cachedConfig.Res.x;
	int h = device->cachedConfig.Res.y;
	Vec2i size;
	GraphicsBlitBkg(device);

	Bonuses();

	if (strlen(gAutosave.LastMission.Password) > 0)
	{
		char s1[512];
		sprintf(s1, "Last password: %s", gAutosave.LastMission.Password);
		CDogsTextStringSpecial(
			s1,
			TEXT_BOTTOM | TEXT_XCENTER,
			0,
			gGraphicsDevice.cachedConfig.Res.y / 12);
	}

	switch (gOptions.numPlayers)
	{
		case 1:
			size = Vec2iNew(w, h / 2);
			Summary(Vec2iZero(), size, &gPlayerDatas[0], 0);
			break;
		case 2:
			// side by side
			size = Vec2iNew(w / 2, h / 2);
			Summary(Vec2iZero(), size, &gPlayerDatas[0], 0);
			Summary(Vec2iNew(w / 2, 0), size, &gPlayerDatas[1], 1);
			break;
		case 3:	// fallthrough
		case 4:
			// 2x2
			size = Vec2iNew(w / 2, h / 4);
			Summary(Vec2iZero(), size, &gPlayerDatas[0], 0);
			Summary(Vec2iNew(w / 2, 0), size, &gPlayerDatas[1], 1);
			Summary(Vec2iNew(0, h / 4), size, &gPlayerDatas[2], 2);
			if (gOptions.numPlayers == 4)
			{
				Summary(Vec2iNew(w / 2, h / 4), size, &gPlayerDatas[3], 3);
			}
			break;
		default:
			assert(0 && "not implemented");
			break;
	}

	BlitFlip(device, &gConfig.Graphics);
	WaitForAnyKeyOrButton(&gEventHandlers);
}

static void ShowPlayerScore(Vec2i pos, Character *c, char *name, int score)
{
	Vec2i scorePos;
	char s[10];
	DisplayCharacterAndName(pos, c, name);
	sprintf(s, "Score: %d", score);
	scorePos = Vec2iNew(pos.x - FontStrW(s) / 2, pos.y + 20);
	FontStr(s, scorePos);
}

void ShowScore(GraphicsDevice *device, int scores[MAX_PLAYERS])
{
	int w = device->cachedConfig.Res.x;
	int h = device->cachedConfig.Res.y;
	int i;

	GraphicsBlitBkg(device);

	debug(D_NORMAL, "\n");

	assert(gOptions.numPlayers >= 2 && gOptions.numPlayers <= 4 &&
		"Invalid number of players for dogfight");
	for (i = 0; i < gOptions.numPlayers; i++)
	{
		Vec2i pos = Vec2iZero();
		pos.x = w / 4 + (i & 1) * w / 2;
		pos.y = gOptions.numPlayers == 2 ? h / 2 : h / 4 + (i / 2) * h / 2;
		ShowPlayerScore(
			pos, &gCampaign.Setting.characters.players[i],
			gPlayerDatas[i].name, scores[i]);
	}

	BlitFlip(device, &gConfig.Graphics);
	WaitForAnyKeyOrButton(&gEventHandlers);
}

void FinalScore(GraphicsDevice *device, int scores[MAX_PLAYERS])
{
	int i;
	int isTie = 0;
	int maxScore = 0;
	int maxScorePlayer = 0;
	int w = device->cachedConfig.Res.x;
	int h = device->cachedConfig.Res.y;
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		if (scores[i] > maxScore)
		{
			maxScore = scores[i];
			maxScorePlayer = i;
			isTie = 0;
		}
		else if (scores[i] == maxScore)
		{
			isTie = 1;
		}
	}

	GraphicsBlitBkg(device);

	// Draw players and their names spread evenly around the screen.
	// If it's a tie, display the message in the centre,
	// otherwise display the winner just below the winning player
#define IS_DRAW		"It's a draw!"
#define IS_WINNER	"Winner!"

	assert(gOptions.numPlayers >= 2 && gOptions.numPlayers <= 4 &&
		"Invalid number of players for dogfight");
	for (i = 0; i < gOptions.numPlayers; i++)
	{
		Vec2i pos = Vec2iZero();
		pos.x = w / 4 + (i & 1) * w / 2;
		pos.y = gOptions.numPlayers == 2 ? h / 2 : h / 4 + (i / 2) * h / 2;
		DisplayCharacterAndName(
			pos, &gCampaign.Setting.characters.players[i],
			gPlayerDatas[i].name);
		if (!isTie && maxScorePlayer == i)
		{
			Vec2i msgPos =
				Vec2iNew(pos.x - FontStrW(IS_WINNER) / 2, pos.y + 20);
			FontStr(IS_WINNER, msgPos);
		}
	}
	if (isTie)
	{
		CDogsTextStringAtCenter("It's a draw!");
	}
	BlitFlip(device, &gConfig.Graphics);
	WaitForAnyKeyOrButton(&gEventHandlers);
}


// TODO: move these words into an external file
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
	int w = graphics->cachedConfig.Res.x;
	int h = graphics->cachedConfig.Res.y;

	for (i = 0; i < MAX_PLAYERS; i++)
	{
		if (gPlayerDatas[i].survived)
		{
			gPlayerDatas[i].missions++;
		}
	}

	// Save that this mission has been completed
	MissionSave ms;
	MissionSaveInit(&ms);
	ms.Campaign = gCampaign.Entry;
	strcpy(ms.Password, MakePassword(gMission.index, 0));
	ms.MissionsCompleted = gMission.index + 1;
	AutosaveAddMission(&gAutosave, &ms, ms.Campaign.BuiltinIndex);
	AutosaveSave(&gAutosave, GetConfigFilePath(AUTOSAVE_FILE));

	GraphicsBlitBkg(graphics);

	x = 160 - FontStrW(CONGRATULATIONS) / 2;
	FontStr(CONGRATULATIONS, Vec2iNew(x, 100));
	x = 160 - FontStrW(gCampaign.Setting.Title) / 2;
	FontStrMask(gCampaign.Setting.Title, Vec2iNew(x, 115), colorRed);

	switch (gOptions.numPlayers)
	{
		case 1:
			i = sizeof(finalWords1P) / sizeof(char *);
			i = rand() % i;
			s = finalWords1P[i];
			DisplayCharacterAndName(
				Vec2iNew(w / 4, h / 4),
				&gCampaign.Setting.characters.players[0],
				gPlayerDatas[0].name);
			break;
		case 2:
			i = sizeof(finalWords2P) / sizeof(char *);
			i = rand() % i;
			s = finalWords2P[i];
			// side by side
			DisplayCharacterAndName(
				Vec2iNew(w / 8, h / 4),
				&gCampaign.Setting.characters.players[0],
				gPlayerDatas[0].name);
			DisplayCharacterAndName(
				Vec2iNew(w / 8 + w / 2, h / 4),
				&gCampaign.Setting.characters.players[1],
				gPlayerDatas[1].name);
			break;
		case 3:	// fallthrough
		case 4:
			i = sizeof(finalWords2P) / sizeof(char *);
			i = rand() % i;
			s = finalWords2P[i];
			// 2x2
			DisplayCharacterAndName(
				Vec2iNew(w / 8, h / 8),
				&gCampaign.Setting.characters.players[0],
				gPlayerDatas[0].name);
			DisplayCharacterAndName(
				Vec2iNew(w / 8 + w / 2, h / 8),
				&gCampaign.Setting.characters.players[1],
				gPlayerDatas[1].name);
			DisplayCharacterAndName(
				Vec2iNew(w / 8, h / 8 + h / 4),
				&gCampaign.Setting.characters.players[2],
				gPlayerDatas[2].name);
			if (gOptions.numPlayers == 4)
			{
				DisplayCharacterAndName(
					Vec2iNew(w / 8 + w / 2, h / 8 + h / 4),
					&gCampaign.Setting.characters.players[3],
					gPlayerDatas[3].name);
			}
			break;
		default:
			assert(0 && "not implemented");
			break;
	}

	Vec2i pos = Vec2iNew((w - FontStrW(s)) / 2, h / 2 + 20);
	pos = FontChMask('"', pos, colorDarker);
	pos = FontStrMask(s, pos, colorPurple);
	pos = FontChMask('"', pos, colorDarker);

	SoundPlay(&gSoundDevice, StrSound("hahaha"));

	BlitFlip(graphics, &gConfig.Graphics);
	WaitForAnyKeyOrButton(&gEventHandlers);
}


static void PlaceActor(TActor * actor)
{
	Vec2i pos;
	do
	{
		pos.x = ((rand() % (gMap.Size.x * TILE_WIDTH)) << 8);
		pos.y = ((rand() % (gMap.Size.y * TILE_HEIGHT)) << 8);
	}
	while (!MapIsFullPosOKforPlayer(&gMap, pos, false) ||
		!TryMoveActor(actor, pos));
}
static void PlaceActorNear(TActor *actor, Vec2i near, bool allowAllTiles)
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
	if (MapIsFullPosOKforPlayer(\
		&gMap, Vec2iAdd(near, Vec2iNew(dx, dy)), allowAllTiles) && \
		TryMoveActor(actor, Vec2iAdd(near, Vec2iNew(dx, dy))))\
	{\
		return;\
	}
	int dx = 0;
	int dy = 0;
	TRY_LOCATION();
	int inc = 1 << 8;
	for (int radius = 12 << 8;; radius += 12 << 8)
	{
		// Going from right to below
		for (dx = radius, dy = 0; dy < radius; dx -= inc, dy += inc)
		{
			TRY_LOCATION();
		}
		// below to left
		for (dx = 0, dy = radius; dy > 0; dx -= inc, dy -= inc)
		{
			TRY_LOCATION();
		}
		// left to above
		for (dx = -radius, dy = 0; dx < 0; dx += inc, dy -= inc)
		{
			TRY_LOCATION();
		}
		// above to right
		for (dx = 0, dy = -radius; dy < 0; dx += inc, dy += inc)
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
	ActorsTerminate();
	ObjsTerminate();
	MobObjsTerminate();
	ParticlesTerminate(&gParticles);
	RemoveAllWatches();
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		gPlayerIds[i] = -1;
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
	TActor *firstPlayer = NULL;
	for (i = 0; i < numPlayers; i++)
	{
		gPlayerDatas[i].lastMission = mission;
		gPlayerIds[i] = ActorAdd(
			&gCampaign.Setting.characters.players[i], &gPlayerDatas[i]);
		TActor *player = CArrayGet(&gActors, gPlayerIds[i]);
		player->health = maxHealth;
		player->character->maxHealth = maxHealth;
		
		if (gMission.missionData->Type == MAPTYPE_STATIC &&
			!Vec2iIsZero(gMission.missionData->u.Static.Start))
		{
			// place players near the start point
			Vec2i startPoint = Vec2iReal2Full(Vec2iCenterOfTile(
				gMission.missionData->u.Static.Start));
			PlaceActorNear(player, startPoint, true);
		}
		else if (gConfig.Interface.Splitscreen == SPLITSCREEN_NEVER &&
			firstPlayer != NULL)
		{
			// If never split screen, try to place players near the first player
			PlaceActorNear(player, firstPlayer->Pos, true);
		}
		else
		{
			PlaceActor(player);
		}
		firstPlayer = player;
	}
}

static void PlayGameSong(void)
{
	int success = 0;
	// Play a tune
	// Start by trying to play a mission specific song,
	// otherwise pick one from the general collection...
	MusicStop(&gSoundDevice);
	if (strlen(gMission.missionData->Song) > 0)
	{
		char buf[CDOGS_PATH_MAX];
		size_t pathLen = MAX(
			strrchr(gCampaign.Entry.Path, '\\'),
			strrchr(gCampaign.Entry.Path, '/')) - gCampaign.Entry.Path;
		strncpy(buf, gCampaign.Entry.Path, pathLen);
		buf[pathLen] = '\0';

		strcat(buf, "/");
		strcat(buf, gMission.missionData->Song);
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


int Game(GraphicsDevice *graphics, CampaignOptions *co)
{
	bool run = false;
	bool gameOver = true;
	do
	{
		CampaignAndMissionSetup(1, co, &gMission);
		if (IsMissionBriefingNeeded(co->Entry.Mode))
		{
			MissionBriefing(graphics);
		}
		if (PlayerEquip(gOptions.numPlayers, graphics))
		{
			MapLoad(&gMap, &gMission, &co->Setting.characters);
			srand((unsigned int)time(NULL));
			InitializeBadGuys();
			const int maxHealth = 200 * gConfig.Game.PlayerHP / 100;
			InitPlayers(gOptions.numPlayers, maxHealth, co->MissionIndex);
			CreateEnemies();
			PlayGameSong();
			run = gameloop();

			const int survivingPlayers = GetNumPlayersAlive();
			gameOver = survivingPlayers == 0 ||
				co->MissionIndex == (int)gCampaign.Setting.Missions.size - 1;

			int i;
			for (i = 0; i < MAX_PLAYERS; i++)
			{
				gPlayerDatas[i].survived = IsPlayerAlive(i);
				if (IsPlayerAlive(i))
				{
					TActor *player = CArrayGet(&gActors, gPlayerIds[i]);
					gPlayerDatas[i].hp = player->health;
				}
			}

			CleanupMission();
			PlayMenuSong();

			if (run)
			{
				MissionSummary(graphics);
				// Note: must use cached value because players get cleaned up
				// in CleanupMission()
				if (gameOver && survivingPlayers > 0)
				{
					Victory(graphics);
				}
			}

			bool allTime = false;
			bool todays = false;
			for (i = 0; i < gOptions.numPlayers; i++)
			{
				if ((run && !gPlayerDatas[i].survived) || gameOver)
				{
					EnterHighScore(&gPlayerDatas[i]);
					allTime |= gPlayerDatas[i].allTime >= 0;
					todays |= gPlayerDatas[i].today >= 0;
				}
				DataUpdate(co->MissionIndex, &gPlayerDatas[i]);
			}
			if (allTime)
			{
				DisplayAllTimeHighScores(graphics);
			}
			if (todays)
			{
				DisplayTodaysHighScores(graphics);
			}

			co->MissionIndex++;
		}

		// Need to terminate the mission later as it is used in calculating scores
		MissionOptionsTerminate(&gMission);
	}
	while (run && !gameOver);
	return run;
}

int Campaign(GraphicsDevice *graphics, CampaignOptions *co)
{
	int i;
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		InitData(&gPlayerDatas[i]);
	}

	if (IsPasswordAllowed(co->Entry.Mode))
	{
		MissionSave m;
		AutosaveLoadMission(
			&gAutosave, &m, co->Entry.Path, co->Entry.BuiltinIndex);
		co->MissionIndex = EnterPassword(graphics, m.Password);
	}
	else
	{
		co->MissionIndex = 0;
	}

	return Game(graphics, co);
}

#define DOGFIGHT_MAX_SCORE 5

void DogFight(GraphicsDevice *graphicsDevice, CampaignOptions *co)
{
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

	bool run = false;
	do
	{
		CampaignAndMissionSetup(1, co, &gMission);
		if (PlayerEquip(gOptions.numPlayers, graphicsDevice))
		{
			MapLoad(&gMap, &gMission, &co->Setting.characters);
			srand((unsigned int)time(NULL));
			InitPlayers(gOptions.numPlayers, 500, 0);
			PlayGameSong();
			run = gameloop();

			for (i = 0; i < MAX_PLAYERS; i++)
			{
				if (IsPlayerAlive(i))
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
		}

		// Need to terminate the mission later as it is used in calculating scores
		MissionOptionsTerminate(&gMission);
	} while (run && maxScore < DOGFIGHT_MAX_SCORE);

	gOptions.badGuys = 1;
	gOptions.numPlayers = numPlayers;

	if (run)
	{
		FinalScore(graphicsDevice, scores);
	}
}

void MainLoop(credits_displayer_t *creditsDisplayer, custom_campaigns_t *campaigns)
{
	while (
		gCampaign.IsLoaded ||
		MainMenu(&gGraphicsDevice, creditsDisplayer, campaigns))
	{
		debug(D_NORMAL, ">> Entering campaign\n");
		if (IsIntroNeeded(gCampaign.Entry.Mode))
		{
			if (!CampaignIntro(&gGraphicsDevice))
			{
				gCampaign.IsLoaded = false;
				continue;
			}
		}

		debug(D_NORMAL, ">> Select number of players\n");
		if (!NumPlayersSelection(
			&gOptions.numPlayers, gCampaign.Entry.Mode,
			&gGraphicsDevice, &gEventHandlers))
		{
			gCampaign.IsLoaded = false;
			continue;
		}

		debug(D_NORMAL, ">> Entering selection\n");
		if (!PlayerSelection(gOptions.numPlayers, &gGraphicsDevice))
		{
			gCampaign.IsLoaded = false;
			continue;
		}

		debug(D_NORMAL, ">> Starting campaign\n");
		if (gCampaign.Entry.Mode == CAMPAIGN_MODE_DOGFIGHT)
		{
			DogFight(&gGraphicsDevice, &gCampaign);
		}
		else if (Campaign(&gGraphicsDevice, &gCampaign))
		{
			DisplayAllTimeHighScores(&gGraphicsDevice);
			DisplayTodaysHighScores(&gGraphicsDevice);
		}
		gCampaign.IsLoaded = false;
	}
	debug(D_NORMAL, ">> Leaving Main Game Loop\n");
}

void PrintTitle(void)
{
	printf("C-Dogs SDL %s\n", CDOGS_SDL_VERSION);

	printf("Original Code Copyright Ronny Wester 1995\n");
	printf("Game Data Copyright Ronny Wester 1995\n");
	printf("SDL Port by Jeremy Chin, Lucas Martin-King and Cong Xu, Copyright 2003-2014\n\n");
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
	int err = 0;
	const char *loadCampaign = NULL;

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
	gLastConfig = gConfig;
	LoadCredits(&creditsDisplayer, colorPurple, colorDarker);
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
					&gConfig.Graphics.Res.x,
					&gConfig.Graphics.Res.y);
				debug(D_NORMAL, "Video mode %dx%d set...\n",
					gConfig.Graphics.Res.x,
					gConfig.Graphics.Res.y);
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
				goto bail;
			default:
				PrintHelp();
				err = EXIT_FAILURE;
				goto bail;
			}
		}
		if (optind < argc)
		{
			// non-option ARGV-elements
			for (; optind < argc; optind++)
			{
				// Load campaign
				loadCampaign = argv[optind];
			}
		}
	}

	debug(D_NORMAL, "Initialising SDL...\n");
	if (SDL_Init(SDL_INIT_TIMER | snd_flag | SDL_INIT_VIDEO | js_flag) != 0)
	{
		fprintf(stderr, "Could not initialise SDL: %s\n", SDL_GetError());
		err = EXIT_FAILURE;
		goto bail;
	}
	if (SDLNet_Init() == -1)
	{
		fprintf(stderr, "SDLNet_Init: %s\n", SDLNet_GetError());
		err = EXIT_FAILURE;
		goto bail;
	}

	char buf[CDOGS_PATH_MAX];
	char buf2[CDOGS_PATH_MAX];
	GetDataFilePath(buf, "");
	printf("Data directory:\t\t%s\n", buf);
	printf("Config directory:\t%s\n\n",	GetConfigFilePath(""));

	if (isSoundEnabled)
	{
		GetDataFilePath(buf, "sounds");
		SoundInitialize(&gSoundDevice, &gConfig.Sound, buf);
		if (!gSoundDevice.isInitialised)
		{
			printf("Sound initialization failed!\n");
		}
	}

	LoadHighScores();

	debug(D_NORMAL, "Loading song lists...\n");
	LoadSongs();
	LoadPlayerTemplates(gPlayerTemplates, PLAYER_TEMPLATE_FILE);

	PlayMenuSong();

	EventInit(&gEventHandlers, NULL, true);

	PHYSFS_init(argv[0]);

	if (wait)
	{
		printf("Press the enter key to continue...\n");
		getchar();
	}
	if (!PicManagerTryInit(
		&gPicManager, "graphics/cdogs.px", "graphics/cdogs2.px"))
	{
		err = EXIT_FAILURE;
		goto bail;
	}
	memcpy(origPalette, gPicManager.palette, sizeof(origPalette));
	GetDataFilePath(buf, "graphics/font.px");
	TextManagerInit(&gTextManager, buf);
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
		gLastConfig = gConfig;
		GraphicsInitialize(
			&gGraphicsDevice, &gConfig.Graphics, gPicManager.palette,
			forceResolution);
	}
	if (!gGraphicsDevice.IsInitialized)
	{
		printf("Video didn't init!\n");
		err = EXIT_FAILURE;
		goto bail;
	}
	else
	{
		GetDataFilePath(buf, "graphics/font.png");
		GetDataFilePath(buf2, "graphics/font.json");
		FontLoad(&gFont, buf, buf2);
		TextManagerGenerateOldPics(&gTextManager, &gGraphicsDevice);
		GetDataFilePath(buf, "graphics");
		PicManagerLoadDir(&gPicManager, buf);

		GetDataFilePath(buf, "data/particles.json");
		ParticleClassesInit(&gParticleClasses, buf);
		GetDataFilePath(buf, "data/bullets.json");
		GetDataFilePath(buf2, "data/guns.json");
		BulletAndWeaponInitialize(
			&gBulletClasses, &gGunDescriptions, buf, buf2);
		CampaignInit(&gCampaign);
		LoadAllCampaigns(&campaigns);
		PlayerDataInitialize();
		MapInit(&gMap);

		GrafxMakeRandomBackground(
			&gGraphicsDevice, &gCampaign, &gMission, &gMap);

		debug(D_NORMAL, ">> Entering main loop\n");
		// Attempt to pre-load campaign if requested
		if (loadCampaign != NULL)
		{
			CampaignEntry entry;
			if (CampaignEntryTryLoad(
				&entry, loadCampaign, CAMPAIGN_MODE_NORMAL))
			{
				CampaignLoad(&gCampaign, &entry);
			}
		}
		MainLoop(&creditsDisplayer, &campaigns);
	}

bail:
	debug(D_NORMAL, ">> Shutting down...\n");
	PHYSFS_deinit();
	MapTerminate(&gMap);
	ParticleClassesTerminate(&gParticleClasses);
	WeaponTerminate(&gGunDescriptions);
	BulletTerminate(&gBulletClasses);
	MissionOptionsTerminate(&gMission);
	EventTerminate(&gEventHandlers);
	GraphicsTerminate(&gGraphicsDevice);

	PicManagerTerminate(&gPicManager);
	TextManagerTerminate(&gTextManager);
	AutosaveSave(&gAutosave, GetConfigFilePath(AUTOSAVE_FILE));
	AutosaveTerminate(&gAutosave);
	ConfigSave(&gConfig, GetConfigFilePath(CONFIG_FILE));
	SavePlayerTemplates(gPlayerTemplates, PLAYER_TEMPLATE_FILE);
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
	SDLNet_Quit();
	SDL_Quit();

	exit(err);
}

