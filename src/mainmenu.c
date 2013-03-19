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
*/
#include "mainmenu.h"

#include <assert.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "credits.h"
#include "defs.h"
#include "input.h"
#include "grafx.h"
#include "drawtools.h"
#include "blit.h"
#include "text.h"
#include "sounds.h"
#include "actors.h"
#include "gamedata.h"
#include "mission.h"
#include "keyboard.h"
#include "joystick.h"
#include "pics.h"
#include "files.h"
#include "menu.h"
#include "utils.h"


#define MODE_MAIN       255
#define MODE_PLAY       0
#define MODE_OPTIONS    3
#define MODE_CONTROLS   4
#define MODE_VOLUME     5
#define MODE_QUIT       6
#define MODE_KEYS       7
#define MODE_CAMPAIGN   8
#define MODE_DOGFIGHT   9


#define MAIN_COUNT      7

static const char *mainMenuStr[MAIN_COUNT] = {
	"1 player game",
	"2 player game",
	"Dog fight",
	"Game options...",
	"Controls...",
	"Sound...",
	"Quit"
};

#define OPTIONS_COUNT   15

static const char *optionsMenu[OPTIONS_COUNT] = {
	"Friendly fire",
	"FPS monitor",
	"Clock",
	"Brightness",
	"Splitscreen always",
	"Random seed",
	"Difficulty",
	"Slowmotion",
	"Enemy density per mission",
	"Non-player hp",
	"Player hp",
	"Video fullscreen",
	"Video resolution (restart required)",
	"Video scale factor",
	"Done"
};

#define CONTROLS_COUNT   7

static const char *controlsMenu[CONTROLS_COUNT] = {
	"Player One",
	"Player Two",
	"Swap buttons joystick 1",
	"Swap buttons joystick 2",
	"Redefine keys...",
	"Calibrate joystick",
	"Done"
};

#define VOLUME_COUNT   4

static const char *volumeMenu[VOLUME_COUNT] = {
	"Sound effects",
	"Music",
	"FX channels",
	"Done"
};


static TCampaignSetting customSetting = {
/*	.title		=*/	"",
/*	.author		=*/	"",
/*	.description	=*/	"",
/*	.missionCount	=*/	0,
/*	.missions	=*/	NULL,
/*	.characterCount	=*/	0,
/*	.characters	=*/	NULL
};


static struct FileEntry *gCampaignList = NULL;
static struct FileEntry *gDogfightList = NULL;


void LookForCustomCampaigns(void)
{
	int i;

	printf("\nCampaigns:\n");

	gCampaignList = GetFilesFromDirectory(GetDataFilePath(CDOGS_CAMPAIGN_DIR));
	GetCampaignTitles(&gCampaignList);
	i = 0;
	while (SetupBuiltinCampaign(i)) {
		AddFileEntry(&gCampaignList, "", gCampaign.setting->title, i);
		i++;
	}

	printf("\nDogfights:\n");

	gDogfightList = GetFilesFromDirectory(GetDataFilePath(CDOGS_DOGFIGHT_DIR));
	GetCampaignTitles(&gDogfightList);
	i = 0;
	while (SetupBuiltinDogfight(i)) {
		AddFileEntry(&gDogfightList, "", gCampaign.setting->title, i);
		i++;
	}

	printf("\n");
}

static void SetupBuiltin(int dogFight, int index)
{
	if (dogFight)
		SetupBuiltinDogfight(index);
	else
		SetupBuiltinCampaign(index);
}

int SelectCampaign(int dogFight, int cmd)
{
	static int campaignIndex = 0;
	static int dogfightIndex = 0;
	int count, y, i, j;
	struct FileEntry *list = dogFight ? gDogfightList : gCampaignList;
	const char *prefix = dogFight ? CDOGS_DOGFIGHT_DIR : CDOGS_CAMPAIGN_DIR;
	int *index = dogFight ? &dogfightIndex : &campaignIndex;
	struct FileEntry *f;

	for (count = 0, f = list; f != NULL; f = f->next, count++);

	if (cmd == CMD_ESC)
		return MODE_MAIN;

	if (AnyButton(cmd)) {
		for (i = 0, f = list; f != NULL && i < *index;
		     f = f->next, i++);

		if (f && f->name[0]) {
			if (customSetting.missions)
				free(customSetting.missions);
			if (customSetting.characters)
				free(customSetting.characters);
			memset(&customSetting, 0, sizeof(customSetting));

			if (LoadCampaign(GetDataFilePath(join(prefix,f->name)), &customSetting, 0, 0) ==
			    CAMPAIGN_OK)
				gCampaign.setting = &customSetting;
			else
				SetupBuiltin(dogFight, 0);
		} else if (f)
			SetupBuiltin(dogFight, f->data);
		else {
			SetupBuiltin(dogFight, 0);
		}

		PlaySound(SND_HAHAHA, 0, 255);
		printf(">> Entering MODE_PLAY\n");
		return MODE_PLAY;
	}

	if (Left(cmd) || Up(cmd)) {
		(*index)--;
		if (*index < 0)
			*index = count - 1;
		PlaySound(SND_SWITCH, 0, 255);
	} else if (Right(cmd) || Down(cmd)) {
		(*index)++;
		if (*index >= count)
			*index = 0;
		PlaySound(SND_SWITCH, 0, 255);
	}

	if (dogFight)
		CDogsTextStringSpecial("Select a dogfight scenario:", TEXT_TOP | TEXT_XCENTER, 0, (SCREEN_WIDTH / 12));
	else
		CDogsTextStringSpecial("Select a campaign:", TEXT_TOP | TEXT_XCENTER, 0, (SCREEN_WIDTH / 12));

	y = CenterY(12 * CDogsTextHeight());

#define ARROW_UP	"\036"
#define ARROW_DOWN	"\037"

	for (i = 0, f = list; f != NULL && i <= *index - 12; f = f->next, i++);

	if (i)
		DisplayMenuItem(CenterX(CDogsTextWidth(ARROW_UP)), y - 2 - CDogsTextHeight(), ARROW_UP, 0);

	for (j = 0; f != NULL && j < 12; f = f->next, i++, j++) {
		DisplayMenuItem(CenterX(CDogsTextWidth(f->info)), y, f->info, i == *index);

		if (i == *index) {
			char s[255];

			if (strlen(f->name) == 0)
				sprintf(s, "( Internal )");
			else
				sprintf(s, "( %s )", f->name);

			CDogsTextStringSpecial(s, TEXT_XCENTER | TEXT_BOTTOM, 0, (SCREEN_WIDTH / 12));
		}

		y += CDogsTextHeight();
	}

	if (f)
		DisplayMenuItem(CenterX(CDogsTextWidth(ARROW_DOWN)), y + 2, ARROW_DOWN, 0);

	return dogFight ? MODE_DOGFIGHT : MODE_CAMPAIGN;
}

static int SelectMain(int cmd)
{
	static int index = 0;

	if (cmd == CMD_ESC) {
		if (index != MODE_QUIT)
			index = MODE_QUIT;
		else
			return MODE_QUIT;
	}
	if (AnyButton(cmd)) {
		switch (index) {
		case 0:
			gCampaign.dogFight = 0;
			gOptions.twoPlayers = 0;
			return MODE_CAMPAIGN;
		case 1:
			gCampaign.dogFight = 0;
			gOptions.twoPlayers = 1;
			return MODE_CAMPAIGN;
		case 2:
			gCampaign.dogFight = 1;
			return MODE_DOGFIGHT;
		}
		return index;
	}
	if (Left(cmd) || Up(cmd)) {
		index--;
		if (index < 0)
			index = MAIN_COUNT - 1;
		PlaySound(SND_SWITCH, 0, 255);
	} else if (Right(cmd) || Down(cmd)) {
		index++;
		if (index >= MAIN_COUNT)
			index = 0;
		PlaySound(SND_SWITCH, 0, 255);
	}

	DrawTPic((SCREEN_WIDTH - PicWidth(gPics[PIC_LOGO])) / 2, (SCREEN_HEIGHT / 12), gPics[PIC_LOGO], gCompiledPics[PIC_LOGO]);

	CDogsTextStringSpecial("Classic: " CDOGS_VERSION, TEXT_TOP | TEXT_LEFT, 20, 20);
	CDogsTextStringSpecial("SDL Port:  " CDOGS_SDL_VERSION, TEXT_TOP | TEXT_RIGHT, 20, 20);

	DisplayMenuAtCenter(mainMenuStr, MAIN_COUNT, index);

	return MODE_MAIN;
}

int SelectOptions(int cmd)
{
	static int index = 0;
	char s[10];
	int x, y;

	if (cmd == CMD_ESC)
		return MODE_MAIN;
	if (AnyButton(cmd) || Left(cmd) || Right(cmd)) {
		switch (index) {
		case 0:
			gOptions.playersHurt = !gOptions.playersHurt;
			PlaySound(SND_KILL, 0, 255);
			break;
		case 1:
			gOptions.displayFPS = !gOptions.displayFPS;
			PlaySound(SND_FLAMER, 0, 255);
			break;
		case 2:
			gOptions.displayTime = !gOptions.displayTime;
			PlaySound(SND_LAUNCH, 0, 255);
			break;
		case 3:
			if (Left(cmd) && gOptions.brightness > -10)
				gOptions.brightness--;
			else if (Right(cmd) && gOptions.brightness < 10)
				gOptions.brightness++;
			else
				break;

			PlaySound(SND_POWERGUN, 0, 255);
			break;
		case 4:
			gOptions.splitScreenAlways = !gOptions.splitScreenAlways;
			PlaySound(SND_KILL3, 0, 255);
			break;
		case 5:
			if (Left(cmd)) {
				if (Button1(cmd) && Button2(cmd))
					gCampaign.seed -= 1000;
				else if (Button1(cmd))
					gCampaign.seed -= 10;
				else if (Button2(cmd))
					gCampaign.seed -= 100;
				else
					gCampaign.seed--;
			} else if (Right(cmd)) {
				if (Button1(cmd) && Button2(cmd))
					gCampaign.seed += 1000;
				else if (Button1(cmd))
					gCampaign.seed += 10;
				else if (Button2(cmd))
					gCampaign.seed += 100;
				else
					gCampaign.seed++;
			}

			break;
		case 6:
			if (Left(cmd)) {
				if (gOptions.difficulty > DIFFICULTY_VERYEASY)
						gOptions.difficulty--;
			} else if (Right(cmd)) {
				if (gOptions.difficulty < DIFFICULTY_VERYHARD)
						gOptions.difficulty++;
			}

			if (gOptions.difficulty > DIFFICULTY_VERYHARD) gOptions.difficulty = DIFFICULTY_VERYHARD;
			if (gOptions.difficulty < DIFFICULTY_VERYEASY) gOptions.difficulty = DIFFICULTY_VERYEASY;

			break;
		case 7:
			gOptions.slowmotion = !gOptions.slowmotion;

			break;
		case 8:
			if (Left(cmd)) {
				if (gOptions.density > 25)
					gOptions.density -= 25;
			} else if (Right(cmd)) {
				if (gOptions.density < 200)
					gOptions.density += 25;
			}

			break;
		case 9:
			if (Left(cmd)) {
				if (gOptions.npcHp > 25)
					gOptions.npcHp -= 25;
			} else if (Right(cmd)) {
				if (gOptions.npcHp < 200)
					gOptions.npcHp += 25;
			}

			break;
		case 10:
			if (Left(cmd)) {
				if (gOptions.playerHp > 25)
					gOptions.playerHp -= 25;
			} else if (Right(cmd)) {
				if (gOptions.playerHp < 200)
					gOptions.playerHp += 25;
			}

			break;
		case 11:
			GrafxToggleFullscreen();
			break;

		case 12:
			if (Left(cmd))
			{
				GrafxTryPrevResolution();
			}
			else if (Right(cmd))
			{
				GrafxTryNextResolution();
			}
			break;

		case 13:
			{
				int fac = GrafxGetScale();

				if (Left(cmd)) {
					fac--;
				} else if (Right(cmd)) {
					fac++;
				}

				if (fac >= 1 && fac <= 4)
				{
					GrafxSetScale(fac);
				}
			}

			break;

		default:
			PlaySound(SND_BANG, 0, 255);

			return MODE_MAIN;
		}
	}

	if (Up(cmd)) {
		index--;
		if (index < 0)
			index = OPTIONS_COUNT - 1;
		PlaySound(SND_SWITCH, 0, 255);
	} else if (Down(cmd)) {
		index++;
		if (index >= OPTIONS_COUNT)
			index = 0;
		PlaySound(SND_SWITCH, 0, 255);
	}

	CDogsTextStringSpecial("Game Options:", TEXT_XCENTER | TEXT_TOP, 0, (SCREEN_WIDTH / 12));

	x = CenterX(MenuWidth(optionsMenu, OPTIONS_COUNT));
	y = CenterY(MenuHeight(OPTIONS_COUNT));

	DisplayMenuAt(x - 20, y, optionsMenu, OPTIONS_COUNT, index);

	x += MenuWidth(optionsMenu, OPTIONS_COUNT);
	x += 10;

	CDogsTextStringAt(x, y, gOptions.playersHurt ? "Yes" : "No");
	y += CDogsTextHeight();
	CDogsTextStringAt(x, y, gOptions.displayFPS ? "On" : "Off");
	y += CDogsTextHeight();
	CDogsTextStringAt(x, y, gOptions.displayTime ? "On" : "Off");
	y += CDogsTextHeight();
	sprintf(s, "%d", gOptions.brightness);
	CDogsTextStringAt(x, y, s);
	y += CDogsTextHeight();
	CDogsTextStringAt(x, y, gOptions.splitScreenAlways ? "Yes" : "No");
	y += CDogsTextHeight();
	sprintf(s, "%u", gCampaign.seed);
	CDogsTextStringAt(x, y, s);

	y += CDogsTextHeight();
	CDogsTextStringAt(x, y, DifficultyStr(gOptions.difficulty));
	y += CDogsTextHeight();
	CDogsTextStringAt(x, y, gOptions.slowmotion ? "Yes" : "No");
	y += CDogsTextHeight();
	sprintf(s, "%u%%", gOptions.density);
	CDogsTextStringAt(x, y, s);
	y += CDogsTextHeight();
	sprintf(s, "%u%%", gOptions.npcHp);
	CDogsTextStringAt(x, y, s);
	sprintf(s, "%u%%", gOptions.playerHp);
	y += CDogsTextHeight();
	CDogsTextStringAt(x, y, s);
	y += CDogsTextHeight();
	sprintf(s, "%s", Gfx_GetHint(HINT_FULLSCREEN) ? "Yes" : "No");
	CDogsTextStringAt(x, y, s);
	y += CDogsTextHeight();
	sprintf(s, "%dx%d", Gfx_GetHint(HINT_WIDTH), Gfx_GetHint(HINT_HEIGHT));
	CDogsTextStringAt(x, y, s);
	y += CDogsTextHeight();
	sprintf(s, "%dx", GrafxGetScale());
	CDogsTextStringAt(x, y, s);

	return MODE_OPTIONS;
}

// TODO: simplify into an iterate over struct controls_available
void ChangeControl(
	input_device_e *d, input_device_e *dOther, int joy0Present, int joy1Present)
{
	if (*d == INPUT_DEVICE_JOYSTICK_1)
	{
		if (*dOther != INPUT_DEVICE_JOYSTICK_2 && joy1Present)
		{
			*d = INPUT_DEVICE_JOYSTICK_2;
		}
		else
		{
			*d = INPUT_DEVICE_KEYBOARD;
		}
	}
	else if (*d == INPUT_DEVICE_JOYSTICK_2)
	{
		*d = INPUT_DEVICE_KEYBOARD;
	}
	else
	{
		if (*dOther != INPUT_DEVICE_JOYSTICK_1 && joy0Present)
		{
			*d = INPUT_DEVICE_JOYSTICK_1;
		}
		else if (joy1Present)
		{
			*d = INPUT_DEVICE_JOYSTICK_2;
		}
	}
	debug(D_NORMAL, "change control to: %s\n", InputDeviceStr(*d));
}

int SelectControls(int cmd)
{
	static int index = 0;
	int x, y;

	if (cmd == CMD_ESC)
		return MODE_MAIN;
	if (AnyButton(cmd) || Left(cmd) || Right(cmd)) {
		PlaySound(rand() % SND_COUNT, 0, 255);
		switch (index) {
		case 0:
			ChangeControl(
				&gPlayer1Data.inputDevice, &gPlayer2Data.inputDevice,
				gSticks[0].present, gSticks[1].present);
			break;

		case 1:
			ChangeControl(
				&gPlayer2Data.inputDevice, &gPlayer1Data.inputDevice,
				gSticks[0].present, gSticks[1].present);
			break;

		case 2:
			gOptions.swapButtonsJoy1 = !gOptions.swapButtonsJoy1;
			break;

		case 3:
			gOptions.swapButtonsJoy2 = !gOptions.swapButtonsJoy2;
			break;

		case 4:
			return MODE_KEYS;

		case 5:
			InitSticks();
			break;

		default:
			return MODE_MAIN;
		}
	}
	if (Up(cmd)) {
		index--;
		if (index < 0)
			index = CONTROLS_COUNT - 1;
		PlaySound(SND_SWITCH, 0, 255);
	} else if (Down(cmd)) {
		index++;
		if (index >= CONTROLS_COUNT)
			index = 0;
		PlaySound(SND_SWITCH, 0, 255);
	}

	CDogsTextStringSpecial("Configure Controls:", TEXT_XCENTER | TEXT_TOP, 0, (SCREEN_WIDTH / 12));

	x = CenterX(MenuWidth(controlsMenu, CONTROLS_COUNT));
	y = CenterY(MenuHeight(CONTROLS_COUNT));

	DisplayMenuAt(x - 20, y, controlsMenu, CONTROLS_COUNT, index);

	x += MenuWidth(controlsMenu, CONTROLS_COUNT);
	x += 10;

	CDogsTextStringAt(x, y, InputDeviceStr(gPlayer1Data.inputDevice));
	y += CDogsTextHeight();
	CDogsTextStringAt(x, y, InputDeviceStr(gPlayer2Data.inputDevice));
	y += CDogsTextHeight();
	CDogsTextStringAt(x, y, gOptions.swapButtonsJoy1 ? "Yes" : "No");
	y += CDogsTextHeight();
	CDogsTextStringAt(x, y, gOptions.swapButtonsJoy2 ? "Yes" : "No");
	return MODE_CONTROLS;
}

int KeyAvailable(int key, struct PlayerData *data, int code,
		 struct PlayerData *other)
{
	key_code_e i;

	if (key == keyEsc || key == keyF9 || key == keyF10)
		return 0;

	if (key == gOptions.mapKey && code >= 0)
		return 0;

	for (i = 0; i < KEY_CODE_MAP; i++)
		if ((int)i != code && InputGetKey(&data->keys, i) == key)
			return 0;

	if (other->keys.left == key ||
		other->keys.right == key ||
		other->keys.up == key ||
		other->keys.down == key ||
		other->keys.button1 == key ||
		other->keys.button2 == key)
	{
		return 0;
	}

	return 1;
}

void ChangeKey(struct PlayerData *data, struct PlayerData *other,
	       int index)
{
	int key = 0;

	while (GetKeyDown());

	while (1) {
		key = GetKeyDown();

		if (key == keyEsc)
			return;

		if (key != 0) {
			if (KeyAvailable(key, data, (key_code_e)index, other))
			{
				InputSetKey(&data->keys, key, (key_code_e)index);
				PlaySound(SND_EXPLOSION, 0, 255);
				return;
			} else
				PlaySound(SND_KILL4, 0, 255);
		}
	}
}

void ChangeMapKey(struct PlayerData *d1, struct PlayerData *d2)
{
	int key;

	while (1) {
		key = GetKeyDown();

		if (key == keyEsc)
			return;

		if (key != 0) {
			if (KeyAvailable(key, d1, -1, d2)) {
				gOptions.mapKey = key;
				PlaySound(SND_EXPLOSION, 0, 255);
				return;
			} else
				PlaySound(SND_KILL4, 0, 255);
		}
	}

	return;
}


#define SELECTKEY "Press a key"

static void DisplayKeys(int x, int x2, int y, char *title,
			struct PlayerData *data, key_code_e index, key_code_e change)
{
	key_code_e i;

	CDogsTextStringAt(x, y, title);
	CDogsTextStringAt(x, y + CDogsTextHeight(), "Left");
	CDogsTextStringAt(x, y + 2 * CDogsTextHeight(), "Right");
	CDogsTextStringAt(x, y + 3 * CDogsTextHeight(), "Up");
	CDogsTextStringAt(x, y + 4 * CDogsTextHeight(), "Down");
	CDogsTextStringAt(x, y + 5 * CDogsTextHeight(), "Fire");
	CDogsTextStringAt(x, y + 6 * CDogsTextHeight(), "Switch/slide");

	for (i = 0; i < 6; i++)
		if (change == i)
			DisplayMenuItem(x2, y + (i + 1) * CDogsTextHeight(),
					SELECTKEY, index == i);
		else
			DisplayMenuItem(x2, y + (i + 1) * CDogsTextHeight(),
					SDL_GetKeyName(InputGetKey(&data->keys, i)),
					index == i);
}

static void ShowAllKeys(int index, int change)
{
	int x1, x2, y1, y2;

	x1 = CenterX((CDogsTextCharWidth('a') * 10)) / 2;
	x2 = x1 * 3;

	y1 = (SCREEN_HEIGHT / 2) - (CDogsTextHeight() * 10);
	y2 = (SCREEN_HEIGHT / 2) - (CDogsTextHeight() * 2);

	DisplayKeys(x1, x2, y1, "Player One", &gPlayer1Data, index, change);
	DisplayKeys(x1, x2, y2, "Player Two", &gPlayer2Data, index - 6, change - 6);

	y2 += CDogsTextHeight() * 8;

	CDogsTextStringAt(x1, y2, "Map");

	if (change == 12)
		DisplayMenuItem(x2, y2, SELECTKEY, index == 12);
	else
		DisplayMenuItem(x2, y2, SDL_GetKeyName(gOptions.mapKey), index == 12);

#define DONE	"Done"

	y2 += CDogsTextHeight () * 2;

	DisplayMenuItem(CenterX(CDogsTextWidth(DONE)), y2, DONE, index == 13);
}

static void HighlightKey(int index)
{
	CopyToScreen();
	ShowAllKeys(index, index);

	return;
}

int SelectKeys(int cmd)
{
	static int index = 12;

	if (cmd == CMD_ESC)
		return MODE_CONTROLS;

	if (AnyButton(cmd)) {
		PlaySound(rand() % SND_COUNT, 0, 255);

		switch (index) {
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
			HighlightKey(index);
			ChangeKey(&gPlayer1Data, &gPlayer2Data, index);
			break;

		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
		case 11:
			HighlightKey(index);
			ChangeKey(&gPlayer2Data, &gPlayer1Data, index - 6);
			break;

		case 12:
			HighlightKey(index);
			ChangeMapKey(&gPlayer1Data, &gPlayer2Data);
			break;

		default:
			return MODE_CONTROLS;
		}
	} else if (index > 0 && Up(cmd)) {
		index--;
		PlaySound(SND_SWITCH, 0, 255);
	} else if (index < 13 && Down(cmd)) {
		index++;
		PlaySound(SND_SWITCH, 0, 255);
	}

	ShowAllKeys(index, -1);

	return MODE_KEYS;
}

int SelectVolume(int cmd)
{
	static int index = 0;
	char s[10];
	int x, y;

	if (cmd == CMD_ESC)
		return MODE_MAIN;

	if (AnyButton(cmd) && index == VOLUME_COUNT - 1)
		return MODE_MAIN;

	if (Left(cmd)) {
		switch (index) {
			case 0:
				if (FXVolume() > 8)
					SetFXVolume(FXVolume() - 8);
				break;
			case 1:
				if (MusicVolume() > 8)
					SetMusicVolume(MusicVolume() - 8);
				break;
			case 2:
				if (FXChannels() > 2)
					SetFXChannels(FXChannels() - 2);
				break;
			case 3:
				break;
		}

		PlaySound(SND_SWITCH, 0, 255);
	} else if (Right(cmd)) {
		switch (index) {
			case 0:
				if (FXVolume() < 64)
					SetFXVolume(FXVolume() + 8);
				break;
			case 1:
				if (MusicVolume() < 64)
					SetMusicVolume(MusicVolume() + 8);
				break;
			case 2:
				if (FXChannels() < 8)
					SetFXChannels(FXChannels() + 2);
				break;
			case 3:
				break;
		}

		PlaySound(SND_SWITCH, 0, 255);
	} else if (Up(cmd)) {
		index--;

		if (index < 0)
			index = VOLUME_COUNT - 1;

		PlaySound(SND_SWITCH, 0, 255);
	} else if (Down(cmd)) {
		index++;

		if (index >= VOLUME_COUNT)
			index = 0;

		PlaySound(SND_SWITCH, 0, 255);
	}

	CDogsTextStringSpecial("Configure Sound:", TEXT_XCENTER | TEXT_TOP, 0, (SCREEN_WIDTH / 12));

	x = CenterX(MenuWidth(volumeMenu, VOLUME_COUNT));
	y = CenterY(MenuHeight(VOLUME_COUNT));

	DisplayMenuAt(x - 20, y, volumeMenu, VOLUME_COUNT, index);

	x += MenuWidth(volumeMenu, VOLUME_COUNT);
	x += 10;

	sprintf(s, "%d", FXVolume() / 8);
	CDogsTextStringAt(x, y, s);
	sprintf(s, "%d", MusicVolume() / 8);
	CDogsTextStringAt(x, y + CDogsTextHeight(), s);
	sprintf(s, "%d", FXChannels());
	CDogsTextStringAt(x, y + 2 * CDogsTextHeight(), s);

	return MODE_VOLUME;
}

int MainMenuSelection(int mode, int cmd)
{
	switch (mode) {
		case MODE_MAIN:
			return SelectMain(cmd);
		case MODE_CAMPAIGN:
			return SelectCampaign(0, cmd);
		case MODE_DOGFIGHT:
			return SelectCampaign(1, cmd);
		case MODE_OPTIONS:
			return SelectOptions(cmd);
		case MODE_CONTROLS:
			return SelectControls(cmd);
		case MODE_KEYS:
			return SelectKeys(cmd);
		case MODE_VOLUME:
			return SelectVolume(cmd);
	}

	return MODE_MAIN;
}

int MainMenuOld(void *bkg, credits_displayer_t *creditsDisplayer)
{
	int cmd, prev = 0;
	int mode = MODE_MAIN;

	while (mode != MODE_QUIT && mode != MODE_PLAY) {
		memcpy(GetDstScreen(), bkg, SCREEN_MEMSIZE);
		ShowControls();

		if (mode == MODE_MAIN)
		{
			ShowCredits(creditsDisplayer);
		}

		GetMenuCmd(&cmd, &prev);

		mode = MainMenuSelection(mode, cmd);

		CopyToScreen();

		SDL_Delay(10);
	}

	WaitForRelease();

	return mode == MODE_PLAY;
}

typedef enum
{
	MENU_TYPE_NORMAL,				// normal menu with items, up/down/left/right moves cursor
	MENU_TYPE_OPTIONS,				// menu with items, only up/down moves
	MENU_TYPE_CAMPAIGNS,			// menu that scrolls, with items centred
	MENU_TYPE_KEYS,					// extra wide option menu
	MENU_TYPE_SET_OPTION_TOGGLE,	// no items, sets option on/off
	MENU_TYPE_SET_OPTION_RANGE,		// no items, sets option range low-high
	MENU_TYPE_SET_OPTION_SEED,		// set random seed
	MENU_TYPE_SET_OPTION_UP_DOWN_VOID_FUNC_VOID,	// set option using up/down functions
	MENU_TYPE_SET_OPTION_RANGE_GET_SET,	// set option range low-high using get/set functions
	MENU_TYPE_SET_OPTION_CHANGE_CONTROL,	// change control device
	MENU_TYPE_SET_OPTION_CHANGE_KEY,	// redefine key
	MENU_TYPE_VOID_FUNC_VOID,		// call a void(*f)(void) function
	MENU_TYPE_CAMPAIGN_ITEM,
	MENU_TYPE_BACK,
	MENU_TYPE_QUIT,
	MENU_TYPE_SEPARATOR
} menu_type_e;

typedef enum
{
	MENU_OPTION_DISPLAY_STYLE_NONE,
	MENU_OPTION_DISPLAY_STYLE_INT,
	MENU_OPTION_DISPLAY_STYLE_YES_NO,
	MENU_OPTION_DISPLAY_STYLE_ON_OFF,
	MENU_OPTION_DISPLAY_STYLE_STR_FUNC,	// use a function that returns string
	MENU_OPTION_DISPLAY_STYLE_INT_TO_STR_FUNC,	// function that converts int to string
} menu_option_display_style_e;

typedef enum
{
	MENU_DISPLAY_ITEMS_CREDITS	= 0x01,
	MENU_DISPLAY_ITEMS_AUTHORS	= 0x02
} menu_display_items_e;

typedef enum
{
	MENU_OPTION_TYPE_NONE,
	MENU_OPTION_TYPE_OPTIONS,
	MENU_OPTION_TYPE_CONTROLS,
	MENU_OPTION_TYPE_SOUND,
	MENU_OPTION_TYPE_KEYS
} menu_option_type_e;

typedef enum
{
	MENU_SET_OPTIONS_TWOPLAYERS	= 0x01,
	MENU_SET_OPTIONS_DOGFIGHT	= 0x02
} menu_set_options_e;

typedef struct menu
{
	char name[64];
	menu_type_e type;
	union
	{
		// normal menu, with sub menus
		struct
		{
			char title[64];
			struct menu *parentMenu;
			struct menu *subMenus;
			int numSubMenus;
			int index;
			int quitMenuIndex;
			int displayItems;
			menu_option_type_e optionType;
			int setOptions;
		} normal;
		// menu item only
		struct
		{
			union
			{
				int *optionToggle;
				struct
				{
					int *option;
					int low;
					int high;
					int increment;
				} optionRange;
				unsigned int *seed;
				// function to call
				struct
				{
					void (*upFunc)(void);
					void (*downFunc)(void);
				} upDownFuncs;
				struct
				{
					int (*getFunc)(void);
					void (*setFunc)(int);
					int low;
					int high;
					int increment;
				} optionRangeGetSet;
				struct
				{
					void (*toggle)(void);
					int (*get)(void);
				} toggleFuncs;
				struct
				{
					input_device_e *device0;
					input_device_e *device1;
				} changeControl;
			} uHook;
			menu_option_display_style_e displayStyle;
			union
			{
				char *(*str)(void);
				char *(*intToStr)(int);
			} uFunc;
		} option;
		campaign_entry_t campaignEntry;
		// change key
		struct
		{
			key_code_e code;
			input_keys_t *keys;
			input_keys_t *keysOther;
		} changeKey;
	} u;
} menu_t;

// TODO: create menu system type to hold menus and components such as credits_displayer_t

menu_t *MenuCreateAll(custom_campaigns_t *campaigns);
void MenuDestroy(menu_t *menu);
void MenuDisplay(menu_t *menu, credits_displayer_t *creditsDisplayer);
menu_t *MenuProcessCmd(menu_t *menu, int cmd);

int MainMenu(
	void *bkg,
	credits_displayer_t *creditsDisplayer,
	custom_campaigns_t *campaigns)
{
	int cmd, prev = 0;
	int doPlay = 0;
	menu_t *mainMenu = MenuCreateAll(campaigns);
	menu_t *menu = mainMenu;

	BlitSetBrightness(gOptions.brightness);
	do
	{
		memcpy(GetDstScreen(), bkg, SCREEN_MEMSIZE);
		ShowControls();
		MenuDisplay(menu, creditsDisplayer);
		GetMenuCmd(&cmd, &prev);
		menu = MenuProcessCmd(menu, cmd);
		CopyToScreen();
		SDL_Delay(10);
	} while (menu->type != MENU_TYPE_QUIT && menu->type != MENU_TYPE_CAMPAIGN_ITEM);
	doPlay = menu->type == MENU_TYPE_CAMPAIGN_ITEM;

	MenuDestroy(mainMenu);
	WaitForRelease();
	return doPlay;
}

menu_t *MenuCreate(
	const char *name,
	const char *title,
	menu_type_e type,
	int displayItems,
	menu_option_type_e optionType,
	int setOptions);
void MenuAddSubmenu(menu_t *menu, menu_t *subMenu);
menu_t *MenuCreateOnePlayer(const char *name, campaign_list_t *campaignList);
menu_t *MenuCreateTwoPlayers(const char *name, campaign_list_t *campaignList);
menu_t *MenuCreateDogfight(const char *name, campaign_list_t *dogfightList);
menu_t *MenuCreateOptions(const char *name);
menu_t *MenuCreateControls(const char *name);
menu_t *MenuCreateSound(const char *name);
menu_t *MenuCreateQuit(const char *name);

menu_t *MenuCreateAll(custom_campaigns_t *campaigns)
{
	menu_t *menu = MenuCreate(
		"",
		"",
		MENU_TYPE_NORMAL,
		MENU_DISPLAY_ITEMS_CREDITS | MENU_DISPLAY_ITEMS_AUTHORS,
		0, 0);
	MenuAddSubmenu(menu, MenuCreateOnePlayer("1 player", &campaigns->campaignList));
	MenuAddSubmenu(menu, MenuCreateTwoPlayers("2 players", &campaigns->campaignList));
	MenuAddSubmenu(menu, MenuCreateDogfight("Dogfight", &campaigns->dogfightList));
	MenuAddSubmenu(menu, MenuCreateOptions("Game options..."));
	MenuAddSubmenu(menu, MenuCreateControls("Controls..."));
	MenuAddSubmenu(menu, MenuCreateSound("Sound..."));
	MenuAddSubmenu(menu, MenuCreateQuit("Quit"));
	return menu;
}

menu_t *MenuCreate(
	const char *name,
	const char *title,
	menu_type_e type,
	int displayItems,
	menu_option_type_e optionType,
	int setOptions)
{
	menu_t *menu = sys_mem_alloc(sizeof(menu_t));
	strcpy(menu->name, name);
	menu->type = type;
	strcpy(menu->u.normal.title, title);
	menu->u.normal.displayItems = displayItems;
	menu->u.normal.optionType = optionType;
	menu->u.normal.setOptions = setOptions;
	menu->u.normal.index = 0;
	menu->u.normal.quitMenuIndex = -1;
	menu->u.normal.parentMenu = NULL;
	menu->u.normal.subMenus = NULL;
	menu->u.normal.numSubMenus = 0;
	return menu;
}

void MenuAddSubmenu(menu_t *menu, menu_t *subMenu)
{
	menu_t *subMenuLoc = NULL;
	int i;

	menu->u.normal.numSubMenus++;
	menu->u.normal.subMenus = sys_mem_realloc(
		menu->u.normal.subMenus, menu->u.normal.numSubMenus*sizeof(menu_t));
	subMenuLoc = &menu->u.normal.subMenus[menu->u.normal.numSubMenus - 1];
	memcpy(subMenuLoc, subMenu, sizeof(menu_t));
	if (subMenu->type == MENU_TYPE_QUIT)
	{
		menu->u.normal.quitMenuIndex = menu->u.normal.numSubMenus - 1;
	}
	sys_mem_free(subMenu);

	// update all parent pointers, in grandchild menus as well
	for (i = 0; i < menu->u.normal.numSubMenus; i++)
	{
		subMenuLoc = &menu->u.normal.subMenus[i];
		if (subMenuLoc->type == MENU_TYPE_NORMAL ||
			subMenuLoc->type == MENU_TYPE_OPTIONS ||
			subMenuLoc->type == MENU_TYPE_CAMPAIGNS ||
			subMenuLoc->type == MENU_TYPE_KEYS)
		{
			int j;

			subMenuLoc->u.normal.parentMenu = menu;

			for (j = 0; j < subMenuLoc->u.normal.numSubMenus; j++)
			{
				menu_t *subSubMenu = &subMenuLoc->u.normal.subMenus[j];
				if (subSubMenu->type == MENU_TYPE_NORMAL ||
					subSubMenu->type == MENU_TYPE_OPTIONS ||
					subSubMenu->type == MENU_TYPE_CAMPAIGNS ||
					subSubMenu->type == MENU_TYPE_KEYS)
				{
					subSubMenu->u.normal.parentMenu = subMenuLoc;
				}
			}
		}
	}

	// move cursor in case first menu item(s) are separators
	while (menu->u.normal.index < menu->u.normal.numSubMenus &&
		menu->u.normal.subMenus[menu->u.normal.index].type == MENU_TYPE_SEPARATOR)
	{
		menu->u.normal.index++;
	}
}

menu_t *MenuCreateCampaignItem(campaign_entry_t *entry);

menu_t *MenuCreateOnePlayer(const char *name, campaign_list_t *campaignList)
{
	menu_t *menu = MenuCreate(
		name,
		"Select a campaign:",
		MENU_TYPE_CAMPAIGNS,
		0, 0, 0);
	int i;
	for (i = 0; i < campaignList->num; i++)
	{
		MenuAddSubmenu(menu, MenuCreateCampaignItem(&campaignList->list[i]));
	}
	return menu;
}

menu_t *MenuCreateTwoPlayers(const char *name, campaign_list_t *campaignList)
{
	menu_t *menu = MenuCreate(
		name,
		"Select a campaign:",
		MENU_TYPE_CAMPAIGNS,
		0, 0,
		MENU_SET_OPTIONS_TWOPLAYERS);
	int i;
	for (i = 0; i < campaignList->num; i++)
	{
		MenuAddSubmenu(menu, MenuCreateCampaignItem(&campaignList->list[i]));
	}
	return menu;
}

menu_t *MenuCreateDogfight(const char *name, campaign_list_t *dogfightList)
{
	menu_t *menu = MenuCreate(
		name,
		"Select a dogfight scenario:",
		MENU_TYPE_CAMPAIGNS,
		0, 0,
		MENU_SET_OPTIONS_DOGFIGHT);
	int i;
	for (i = 0; i < dogfightList->num; i++)
	{
		MenuAddSubmenu(menu, MenuCreateCampaignItem(&dogfightList->list[i]));
	}
	return menu;
}

menu_t *MenuCreateCampaignItem(campaign_entry_t *entry)
{
	menu_t *menu = sys_mem_alloc(sizeof(menu_t));
	strcpy(menu->name, entry->info);
	menu->type = MENU_TYPE_CAMPAIGN_ITEM;
	memcpy(&menu->u.campaignEntry, entry, sizeof(menu->u.campaignEntry));
	return menu;
}

menu_t *MenuCreateOptionToggle(
	const char *name, int *config, menu_option_display_style_e style);
menu_t *MenuCreateOptionRange(
	const char *name,
	int *config,
	int low, int high, int increment,
	menu_option_display_style_e style, void *func);
menu_t *MenuCreateOptionSeed(const char *name, unsigned int *seed);
menu_t *MenuCreateOptionUpDownFunc(
	const char *name,
	void(*upFunc)(void), void(*downFunc)(void),
	menu_option_display_style_e style, char *(*strFunc)(void));
menu_t *MenuCreateOptionFunc(
	const char *name,
	void(*toggleFunc)(void), int(*getFunc)(void),
	menu_option_display_style_e style);
menu_t *MenuCreateOptionRangeGetSet(
	const char *name,
	int(*getFunc)(void), void(*setFunc)(int),
	int low, int high, int increment,
	menu_option_display_style_e style, void *func);
menu_t *MenuCreateSeparator(const char *name);
menu_t *MenuCreateBack(const char *name);

menu_t *MenuCreateOptions(const char *name)
{
	menu_t *menu = MenuCreate(
		name,
		"Game Options:",
		MENU_TYPE_OPTIONS,
		0,
		MENU_OPTION_TYPE_OPTIONS,
		0);
	MenuAddSubmenu(
		menu,
		MenuCreateOptionToggle(
			"Friendly fire",
			&gOptions.playersHurt,
			MENU_OPTION_DISPLAY_STYLE_YES_NO));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionToggle(
			"FPS monitor",
			&gOptions.displayFPS,
			MENU_OPTION_DISPLAY_STYLE_ON_OFF));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionToggle(
			"Display time",
			&gOptions.displayTime,
			MENU_OPTION_DISPLAY_STYLE_ON_OFF));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionRangeGetSet(
			"Brightness",
			BlitGetBrightness, BlitSetBrightness,
			-10, 10, 1,
			MENU_OPTION_DISPLAY_STYLE_INT, NULL));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionToggle(
			"Splitscreen always",
			&gOptions.splitScreenAlways,
			MENU_OPTION_DISPLAY_STYLE_YES_NO));
	MenuAddSubmenu(
		menu, MenuCreateOptionSeed("Random seed", &gCampaign.seed));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionRange(
			"Difficulty", (int *)&gOptions.difficulty,
			DIFFICULTY_VERYEASY, DIFFICULTY_VERYHARD, 1,
			MENU_OPTION_DISPLAY_STYLE_INT_TO_STR_FUNC, (void *)DifficultyStr));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionToggle(
			"Slowmotion",
			&gOptions.slowmotion,
			MENU_OPTION_DISPLAY_STYLE_YES_NO));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionRange(
			"Enemy density per mission",
			&gOptions.density,
			0, 200, 25,
			MENU_OPTION_DISPLAY_STYLE_INT_TO_STR_FUNC, (void *)PercentStr));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionRange(
			"Non-player HP",
			&gOptions.npcHp,
			0, 200, 25,
			MENU_OPTION_DISPLAY_STYLE_INT_TO_STR_FUNC, (void *)PercentStr));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionRange(
			"Player HP",
			&gOptions.playerHp,
			0, 200, 25,
			MENU_OPTION_DISPLAY_STYLE_INT_TO_STR_FUNC, (void *)PercentStr));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionFunc(
			"Video fullscreen",
			GrafxToggleFullscreen,
			GrafxIsFullscreen,
			MENU_OPTION_DISPLAY_STYLE_YES_NO));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionUpDownFunc(
			"Video resolution (restart required)",
			GrafxTryPrevResolution,
			GrafxTryNextResolution,
			MENU_OPTION_DISPLAY_STYLE_STR_FUNC,
			GrafxGetResolutionStr));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionRangeGetSet(
			"Video scale factor",
			GrafxGetScale, GrafxSetScale,
			1, 4, 1,
			MENU_OPTION_DISPLAY_STYLE_INT_TO_STR_FUNC, (void *)ScaleStr));
	MenuAddSubmenu(menu, MenuCreateSeparator(""));
	MenuAddSubmenu(menu, MenuCreateBack("Done"));
	return menu;
}

menu_t *MenuCreateOptionChangeControl(
	const char *name, input_device_e *device0, input_device_e *device1);
menu_t *MenuCreateKeys(const char *name);

menu_t *MenuCreateControls(const char *name)
{
	menu_t *menu = MenuCreate(
		name,
		"Configure Controls:",
		MENU_TYPE_OPTIONS,
		0,
		MENU_OPTION_TYPE_CONTROLS,
		0);
	MenuAddSubmenu(
		menu,
		MenuCreateOptionChangeControl(
			"Player 1", &gPlayer1Data.inputDevice, &gPlayer2Data.inputDevice));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionChangeControl(
			"Player 2", &gPlayer2Data.inputDevice, &gPlayer1Data.inputDevice));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionToggle(
			"Swap buttons joystick 1",
			&gOptions.swapButtonsJoy1,
			MENU_OPTION_DISPLAY_STYLE_YES_NO));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionToggle(
			"Swap buttons joystick 2",
			&gOptions.swapButtonsJoy2,
			MENU_OPTION_DISPLAY_STYLE_YES_NO));
	MenuAddSubmenu(menu, MenuCreateKeys("Redefine keys..."));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionFunc(
			"Calibrate joystick",
			InitSticks,
			NULL, MENU_OPTION_DISPLAY_STYLE_NONE));
	MenuAddSubmenu(menu, MenuCreateSeparator(""));
	MenuAddSubmenu(menu, MenuCreateBack("Done"));
	return menu;
}

menu_t *MenuCreateSound(const char *name)
{
	menu_t *menu = MenuCreate(
		name,
		"Configure Sound:",
		MENU_TYPE_OPTIONS,
		0,
		MENU_OPTION_TYPE_SOUND,
		0);
	MenuAddSubmenu(
		menu,
		MenuCreateOptionRangeGetSet(
			"Sound effects",
			FXVolume, SetFXVolume,
			8, 64, 8,
			MENU_OPTION_DISPLAY_STYLE_INT_TO_STR_FUNC, Div8Str));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionRangeGetSet(
			"Music",
			MusicVolume, SetMusicVolume,
			8, 64, 8,
			MENU_OPTION_DISPLAY_STYLE_INT_TO_STR_FUNC, Div8Str));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionRangeGetSet(
			"FX channels",
			FXChannels, SetFXChannels,
			2, 8, 1,
			MENU_OPTION_DISPLAY_STYLE_INT, NULL));
	MenuAddSubmenu(menu, MenuCreateSeparator(""));
	MenuAddSubmenu(menu, MenuCreateBack("Done"));
	return menu;
}

menu_t *MenuCreateQuit(const char *name)
{
	menu_t *menu = sys_mem_alloc(sizeof(menu_t));
	strcpy(menu->name, name);
	menu->type = MENU_TYPE_QUIT;
	return menu;
}


menu_t *MenuCreateOptionToggle(
	const char *name, int *config, menu_option_display_style_e style)
{
	menu_t *menu = sys_mem_alloc(sizeof(menu_t));
	strcpy(menu->name, name);
	menu->type = MENU_TYPE_SET_OPTION_TOGGLE;
	menu->u.option.uHook.optionToggle = config;
	menu->u.option.displayStyle = style;
	return menu;
}

menu_t *MenuCreateOptionRange(
	const char *name,
	int *config,
	int low, int high, int increment,
	menu_option_display_style_e style, void *func)
{
	menu_t *menu = sys_mem_alloc(sizeof(menu_t));
	strcpy(menu->name, name);
	menu->type = MENU_TYPE_SET_OPTION_RANGE;
	menu->u.option.uHook.optionRange.option = config;
	menu->u.option.uHook.optionRange.low = low;
	menu->u.option.uHook.optionRange.high = high;
	menu->u.option.uHook.optionRange.increment = increment;
	menu->u.option.displayStyle = style;
	if (style == MENU_OPTION_DISPLAY_STYLE_STR_FUNC)
	{
		menu->u.option.uFunc.str = func;
	}
	else if (style == MENU_OPTION_DISPLAY_STYLE_INT_TO_STR_FUNC)
	{
		menu->u.option.uFunc.intToStr = func;
	}
	return menu;
}

menu_t *MenuCreateOptionSeed(const char *name, unsigned int *seed)
{
	menu_t *menu = sys_mem_alloc(sizeof(menu_t));
	strcpy(menu->name, name);
	menu->type = MENU_TYPE_SET_OPTION_SEED;
	menu->u.option.uHook.seed = seed;
	menu->u.option.displayStyle = MENU_OPTION_DISPLAY_STYLE_INT;
	return menu;
}

menu_t *MenuCreateOptionUpDownFunc(
	const char *name,
	void(*upFunc)(void), void(*downFunc)(void),
	menu_option_display_style_e style, char *(*strFunc)(void))
{
	menu_t *menu = sys_mem_alloc(sizeof(menu_t));
	strcpy(menu->name, name);
	menu->type = MENU_TYPE_SET_OPTION_UP_DOWN_VOID_FUNC_VOID;
	menu->u.option.uHook.upDownFuncs.upFunc = upFunc;
	menu->u.option.uHook.upDownFuncs.downFunc = downFunc;
	menu->u.option.displayStyle = style;
	menu->u.option.uFunc.str = strFunc;
	return menu;
}

menu_t *MenuCreateOptionFunc(
	const char *name,
	void(*toggleFunc)(void), int(*getFunc)(void),
	menu_option_display_style_e style)
{
	menu_t *menu = sys_mem_alloc(sizeof(menu_t));
	strcpy(menu->name, name);
	menu->type = MENU_TYPE_VOID_FUNC_VOID;
	menu->u.option.uHook.toggleFuncs.toggle = toggleFunc;
	menu->u.option.uHook.toggleFuncs.get = getFunc;
	menu->u.option.displayStyle = style;
	return menu;
}

menu_t *MenuCreateOptionRangeGetSet(
	const char *name,
	int(*getFunc)(void), void(*setFunc)(int),
	int low, int high, int increment,
	menu_option_display_style_e style, void *func)
{
	menu_t *menu = sys_mem_alloc(sizeof(menu_t));
	strcpy(menu->name, name);
	menu->type = MENU_TYPE_SET_OPTION_RANGE_GET_SET;
	menu->u.option.uHook.optionRangeGetSet.getFunc = getFunc;
	menu->u.option.uHook.optionRangeGetSet.setFunc = setFunc;
	menu->u.option.uHook.optionRangeGetSet.low = low;
	menu->u.option.uHook.optionRangeGetSet.high = high;
	menu->u.option.uHook.optionRangeGetSet.increment = increment;
	menu->u.option.displayStyle = style;
	// TODO: refactor saving of function based on style
	if (style == MENU_OPTION_DISPLAY_STYLE_INT_TO_STR_FUNC)
	{
		menu->u.option.uFunc.intToStr = func;
	}
	return menu;
}

menu_t *MenuCreateSeparator(const char *name)
{
	menu_t *menu = sys_mem_alloc(sizeof(menu_t));
	strcpy(menu->name, name);
	menu->type = MENU_TYPE_SEPARATOR;
	return menu;
}

menu_t *MenuCreateBack(const char *name)
{
	menu_t *menu = sys_mem_alloc(sizeof(menu_t));
	strcpy(menu->name, name);
	menu->type = MENU_TYPE_BACK;
	return menu;
}

menu_t *MenuCreateOptionChangeControl(
	const char *name, input_device_e *device0, input_device_e *device1)
{
	menu_t *menu = sys_mem_alloc(sizeof(menu_t));
	strcpy(menu->name, name);
	menu->type = MENU_TYPE_SET_OPTION_CHANGE_CONTROL;
	menu->u.option.uHook.changeControl.device0 = device0;
	menu->u.option.uHook.changeControl.device1 = device1;
	menu->u.option.displayStyle = MENU_OPTION_DISPLAY_STYLE_INT_TO_STR_FUNC;
	menu->u.option.uFunc.intToStr = InputDeviceStr;
	return menu;
}

void MenuCreateKeysSingleSection(
	menu_t *menu, const char *sectionName,
	input_keys_t *keys, input_keys_t *keysOther);
menu_t *MenuCreateOptionChangeKey(
	const char *name, key_code_e code,
	input_keys_t *keys, input_keys_t *keysOther);

menu_t *MenuCreateKeys(const char *name)
{
	menu_t *menu = MenuCreate(
		name,
		"",
		MENU_TYPE_KEYS,
		0,
		MENU_OPTION_TYPE_KEYS,
		0);
	MenuCreateKeysSingleSection(
		menu, "Player 1", &gPlayer1Data.keys, &gPlayer2Data.keys);
	MenuCreateKeysSingleSection(
		menu, "Player 2", &gPlayer2Data.keys, &gPlayer1Data.keys);
	MenuAddSubmenu(
		menu,
		MenuCreateOptionChangeKey(
			"Map", KEY_CODE_MAP, NULL, NULL));
	MenuAddSubmenu(menu, MenuCreateSeparator(""));
	MenuAddSubmenu(menu, MenuCreateBack("Done"));
	return menu;
}

void MenuCreateKeysSingleSection(
	menu_t *menu, const char *sectionName,
	input_keys_t *keys, input_keys_t *keysOther)
{
	MenuAddSubmenu(menu, MenuCreateSeparator(sectionName));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionChangeKey("Left", KEY_CODE_LEFT, keys, keysOther));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionChangeKey(
			"Right", KEY_CODE_RIGHT, keys, keysOther));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionChangeKey(
			"Up", KEY_CODE_UP, keys, keysOther));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionChangeKey(
			"Down", KEY_CODE_DOWN, keys, keysOther));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionChangeKey(
			"Fire", KEY_CODE_BUTTON1, keys, keysOther));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionChangeKey(
			"Switch/slide", KEY_CODE_BUTTON2, keys, keysOther));
	MenuAddSubmenu(menu, MenuCreateSeparator(""));
}

menu_t *MenuCreateOptionChangeKey(
	const char *name, key_code_e code,
	input_keys_t *keys, input_keys_t *keysOther)
{
	menu_t *menu = sys_mem_alloc(sizeof(menu_t));
	strcpy(menu->name, name);
	menu->type = MENU_TYPE_SET_OPTION_CHANGE_KEY;
	menu->u.changeKey.code = code;
	menu->u.changeKey.keys = keys;
	menu->u.changeKey.keysOther = keysOther;
	return menu;
}

void MenuDestroySubmenus(menu_t *menu);

void MenuDestroy(menu_t *menu)
{
	if (menu == NULL)
	{
		return;
	}
	MenuDestroySubmenus(menu);
	sys_mem_free(menu);
}

void MenuDestroySubmenus(menu_t *menu)
{
	if (menu == NULL)
	{
		return;
	}
	if ((menu->type == MENU_TYPE_NORMAL ||
		menu->type == MENU_TYPE_OPTIONS ||
		menu->type == MENU_TYPE_CAMPAIGNS ||
		menu->type == MENU_TYPE_KEYS) &&
		menu->u.normal.subMenus != NULL)
	{
		int i;
		for (i = 0; i < menu->u.normal.numSubMenus; i++)
		{
			menu_t *subMenu = &menu->u.normal.subMenus[i];
			MenuDestroySubmenus(subMenu);
		}
		sys_mem_free(menu->u.normal.subMenus);
	}
}

void MenuDisplayItems(menu_t *menu, credits_displayer_t *creditsDisplayer);
void MenuDisplaySubmenus(menu_t *menu);

void MenuDisplay(menu_t *menu, credits_displayer_t *creditsDisplayer)
{
	MenuDisplayItems(menu, creditsDisplayer);

	if (strlen(menu->u.normal.title) != 0)
	{
		CDogsTextStringSpecial(
			menu->u.normal.title,
			TEXT_XCENTER | TEXT_TOP,
			0,
			SCREEN_WIDTH / 12);
	}

	MenuDisplaySubmenus(menu);
}

void MenuDisplayItems(menu_t *menu, credits_displayer_t *creditsDisplayer)
{
	int d = menu->u.normal.displayItems;
	if (d & MENU_DISPLAY_ITEMS_CREDITS)
	{
		ShowCredits(creditsDisplayer);
	}
	if (d & MENU_DISPLAY_ITEMS_AUTHORS)
	{
		DrawTPic(
			(SCREEN_WIDTH - PicWidth(gPics[PIC_LOGO])) / 2,
			SCREEN_HEIGHT / 12,
			gPics[PIC_LOGO],
			gCompiledPics[PIC_LOGO]);
		CDogsTextStringSpecial(
			"Classic: " CDOGS_VERSION, TEXT_TOP | TEXT_LEFT, 20, 20);
		CDogsTextStringSpecial(
			"SDL Port: " CDOGS_SDL_VERSION, TEXT_TOP | TEXT_RIGHT, 20, 20);
	}
}

int MenuOptionGetIntValue(menu_t *menu);

void MenuDisplaySubmenus(menu_t *menu)
{
	int i;
	int x = 0, yStart = 0;
	int maxWidth = 0;

	switch (menu->type)
	{
	// TODO: refactor the three menu types (normal, options, campaign) into one
	case MENU_TYPE_NORMAL:
	case MENU_TYPE_OPTIONS:
		{
			int isCentered = menu->type == MENU_TYPE_NORMAL;
			int xOptions;
			for (i = 0; i < menu->u.normal.numSubMenus; i++)
			{
				int width = CDogsTextWidth(menu->u.normal.subMenus[i].name);
				if (width > maxWidth)
				{
					maxWidth = width;
				}
			}
			x = CenterX(maxWidth);
			if (!isCentered)
			{
				x -= 20;
			}
			yStart = CenterY(menu->u.normal.numSubMenus * CDogsTextHeight());
			xOptions = x + maxWidth + 10;

			// Display normal menu items
			for (i = 0; i < menu->u.normal.numSubMenus; i++)
			{
				int y = yStart + i * CDogsTextHeight();
				menu_t *subMenu = &menu->u.normal.subMenus[i];

				// Display menu item
				const char *name = subMenu->name;
				if (i == menu->u.normal.index)
				{
					CDogsTextStringWithTableAt(x, y, name, &tableFlamed);
				}
				else
				{
					CDogsTextStringAt(x, y, name);
				}

				// display option value
				if (subMenu->type == MENU_TYPE_SET_OPTION_TOGGLE ||
					subMenu->type == MENU_TYPE_SET_OPTION_RANGE ||
					subMenu->type == MENU_TYPE_SET_OPTION_SEED ||
					subMenu->type == MENU_TYPE_SET_OPTION_UP_DOWN_VOID_FUNC_VOID ||
					subMenu->type == MENU_TYPE_SET_OPTION_RANGE_GET_SET ||
					subMenu->type == MENU_TYPE_SET_OPTION_CHANGE_CONTROL ||
					subMenu->type == MENU_TYPE_VOID_FUNC_VOID)
				{
					int optionInt = MenuOptionGetIntValue(subMenu);
					switch (subMenu->u.option.displayStyle)
					{
					case MENU_OPTION_DISPLAY_STYLE_INT:
						CDogsTextIntAt(xOptions, y, optionInt);
						break;
					case MENU_OPTION_DISPLAY_STYLE_YES_NO:
						CDogsTextStringAt(xOptions, y, optionInt ? "Yes" : "No");
						break;
					case MENU_OPTION_DISPLAY_STYLE_ON_OFF:
						CDogsTextStringAt(xOptions, y, optionInt ? "On" : "Off");
						break;
					case MENU_OPTION_DISPLAY_STYLE_STR_FUNC:
						CDogsTextStringAt(xOptions, y, subMenu->u.option.uFunc.str());
						break;
					case MENU_OPTION_DISPLAY_STYLE_INT_TO_STR_FUNC:
						CDogsTextStringAt(xOptions, y, subMenu->u.option.uFunc.intToStr(optionInt));
						break;
					default:
						break;
					}
				}
			}
		}
		break;
	case MENU_TYPE_CAMPAIGNS:
		{
			i = 0;	// current menu scroll
			int j;
			// TODO: display up/down arrows, handle scrolling

			int y = CenterY(12 * CDogsTextHeight());

		#define ARROW_UP	"\036"
		#define ARROW_DOWN	"\037"

			if (i != 0)
			{
				DisplayMenuItem(
					CenterX(CDogsTextWidth(ARROW_UP)),
					y - 2 - CDogsTextHeight(),
					ARROW_UP,
					0);
			}

			for (j = 0; j < MIN(12, menu->u.normal.numSubMenus); i++, j++)
			{
				int isSelected = i == menu->u.normal.index;
				menu_t *subMenu = &menu->u.normal.subMenus[i];
				const char *name = subMenu->name;
				// TODO: display subfolders
				DisplayMenuItem(
					CenterX(CDogsTextWidth(name)), y, name, isSelected);

				if (isSelected)
				{
					char s[255];
					const char *filename = subMenu->u.campaignEntry.filename;
					int isBuiltin = subMenu->u.campaignEntry.isBuiltin;
					sprintf(s, "( %s )", isBuiltin ? "Internal" : filename);
					CDogsTextStringSpecial(s, TEXT_XCENTER | TEXT_BOTTOM, 0, SCREEN_WIDTH / 12);
				}

				y += CDogsTextHeight();
			}

			if (i < menu->u.normal.numSubMenus - 1)
			{
				DisplayMenuItem(
					CenterX(CDogsTextWidth(ARROW_DOWN)),
					y + 2,
					ARROW_DOWN,
					0);
			}
		}
		break;
	case MENU_TYPE_KEYS:
		{
			int xKeys;
			x = CenterX((CDogsTextCharWidth('a') * 10)) / 2;
			xKeys = x * 3;
			yStart = (SCREEN_HEIGHT / 2) - (CDogsTextHeight() * 10);

			for (i = 0; i < menu->u.normal.numSubMenus; i++)
			{
				int y = yStart + i * CDogsTextHeight();
				int isSelected = i == menu->u.normal.index;
				menu_t *subMenu = &menu->u.normal.subMenus[i];

				const char *name = subMenu->name;
				if (isSelected &&
					subMenu->type != MENU_TYPE_SET_OPTION_CHANGE_KEY)
				{
					CDogsTextStringWithTableAt(x, y, name, &tableFlamed);
				}
				else
				{
					CDogsTextStringAt(x, y, name);
				}

				if (subMenu->type == MENU_TYPE_SET_OPTION_CHANGE_KEY)
				{
					int isChanging = 0;	// TODO: key changing
					const char *keyName = "Press a key";
					if (!isChanging)
					{
						if (subMenu->u.changeKey.code != KEY_CODE_MAP)
						{
							keyName = SDL_GetKeyName(InputGetKey(
								subMenu->u.changeKey.keys,
								subMenu->u.changeKey.code));
						}
					}
					DisplayMenuItem(xKeys, y, keyName, isSelected);
				}
			}
		}
		break;
	default:
		assert(0);
		break;
	}
}

int MenuOptionGetIntValue(menu_t *menu)
{
	switch (menu->type)
	{
	case MENU_TYPE_SET_OPTION_TOGGLE:
		return *menu->u.option.uHook.optionToggle;
	case MENU_TYPE_SET_OPTION_RANGE:
		return *menu->u.option.uHook.optionRange.option;
	case MENU_TYPE_SET_OPTION_SEED:
		return (int)*menu->u.option.uHook.seed;
	case MENU_TYPE_SET_OPTION_RANGE_GET_SET:
		return menu->u.option.uHook.optionRangeGetSet.getFunc();
	case MENU_TYPE_SET_OPTION_CHANGE_CONTROL:
		return *menu->u.option.uHook.changeControl.device0;
	case MENU_TYPE_VOID_FUNC_VOID:
		if (menu->u.option.uHook.toggleFuncs.get)
		{
			return menu->u.option.uHook.toggleFuncs.get();
		}
		return 0;
	default:
		return 0;
	}
}

// returns menu to change to, NULL if no change
menu_t *MenuProcessEscCmd(menu_t *menu);
menu_t *MenuProcessButtonCmd(menu_t *menu, int cmd);

void MenuChangeIndex(menu_t *menu, int cmd);

menu_t *MenuProcessCmd(menu_t *menu, int cmd)
{
	menu_t *menuToChange = NULL;
	if (cmd == CMD_ESC)
	{
		menuToChange = MenuProcessEscCmd(menu);
		if (menuToChange != NULL)
		{
			PlaySound(SND_PICKUP, 0, 255);
			return menuToChange;
		}
	}
	menuToChange = MenuProcessButtonCmd(menu, cmd);
	if (menuToChange != NULL)
	{
		debug(D_VERBOSE, "change to menu type %d\n", menuToChange->type);
		// TODO: refactor menu change sound
		if (menuToChange->type == MENU_TYPE_CAMPAIGN_ITEM)
		{
			PlaySound(SND_HAHAHA, 0, 255);
		}
		else
		{
			PlaySound(SND_MACHINEGUN, 0, 255);
		}
		return menuToChange;
	}
	MenuChangeIndex(menu, cmd);
	return menu;
}

menu_t *MenuProcessEscCmd(menu_t *menu)
{
	menu_t *menuToChange = NULL;
	int quitMenuIndex = menu->u.normal.quitMenuIndex;
	if (quitMenuIndex != -1)
	{
		if (menu->u.normal.index != quitMenuIndex)
		{
			PlaySound(SND_DOOR, 0, 255);
			menu->u.normal.index = quitMenuIndex;
		}
		else
		{
			menuToChange = &menu->u.normal.subMenus[quitMenuIndex];
		}
	}
	else if (menu->u.normal.parentMenu != NULL)
	{
		menuToChange = menu->u.normal.parentMenu;
	}
	return menuToChange;
}

void MenuSetOptions(int setOptions);
void MenuLoadCampaign(campaign_entry_t *entry);
void MenuActivate(menu_t *menu, int cmd);

menu_t *MenuProcessButtonCmd(menu_t *menu, int cmd)
{
	int leftRightMoves = menu->type == MENU_TYPE_NORMAL;
	if (AnyButton(cmd) ||
		(!leftRightMoves && (Left(cmd) || Right(cmd))))
	{
		menu_t *subMenu = &menu->u.normal.subMenus[menu->u.normal.index];
		MenuSetOptions(menu->u.normal.setOptions);
		switch (subMenu->type)
		{
		case MENU_TYPE_NORMAL:
		case MENU_TYPE_OPTIONS:
		case MENU_TYPE_CAMPAIGNS:
		case MENU_TYPE_KEYS:
			return subMenu;
		case MENU_TYPE_CAMPAIGN_ITEM:
			MenuLoadCampaign(&subMenu->u.campaignEntry);
			return subMenu;	// caller will check if subMenu type is CAMPAIGN_ITEM
		case MENU_TYPE_BACK:
			return menu->u.normal.parentMenu;
		case MENU_TYPE_QUIT:
			return subMenu;	// caller will check if subMenu type is QUIT
		default:
			MenuActivate(subMenu, cmd);
			break;
		}
	}
	return NULL;
}

void MenuSetOptions(int setOptions)
{
	if (setOptions)
	{
		gOptions.twoPlayers = !!(setOptions & MENU_SET_OPTIONS_TWOPLAYERS);
		gCampaign.dogFight = !!(setOptions & MENU_SET_OPTIONS_DOGFIGHT);
	}
}

void MenuLoadCampaign(campaign_entry_t *entry)
{
	if (entry->isBuiltin)
	{
		if (entry->isDogfight)
		{
			SetupBuiltinDogfight(entry->builtinIndex);
		}
		else
		{
			SetupBuiltinCampaign(entry->builtinIndex);
		}
	}
	else
	{
		const char *filename = entry->filename;
		const char *campaignFolder = entry->isDogfight ? CDOGS_DOGFIGHT_DIR : CDOGS_CAMPAIGN_DIR;
		if (customSetting.missions)
		{
			sys_mem_free(customSetting.missions);
		}
		if (customSetting.characters)
		{
			sys_mem_free(customSetting.characters);
		}
		memset(&customSetting, 0, sizeof(customSetting));

		if (LoadCampaign(
				GetDataFilePath(join(campaignFolder, filename)),
				&customSetting, 0, 0) != CAMPAIGN_OK)
		{
			assert(0);
			printf("Failed to load campaign %s!\n", filename);
		}
		gCampaign.setting = &customSetting;
	}

	printf(">> Loading campaign/dogfight\n");
}

void MenuActivate(menu_t *menu, int cmd)
{
	PlaySound(SND_SWITCH, 0, 255);
	switch (menu->type)
	{
	case MENU_TYPE_SET_OPTION_TOGGLE:
		*menu->u.option.uHook.optionToggle = !*menu->u.option.uHook.optionToggle;
		break;
	case MENU_TYPE_SET_OPTION_RANGE:
		{
			int option = *menu->u.option.uHook.optionRange.option;
			int increment = menu->u.option.uHook.optionRange.increment;
			if (Left(cmd))
			{
				if (menu->u.option.uHook.optionRange.low + increment > option)
				{
					option = menu->u.option.uHook.optionRange.low;
				}
				else
				{
					option -= increment;
				}
			}
			else if (Right(cmd))
			{
				if (menu->u.option.uHook.optionRange.high - increment < option)
				{
					option = menu->u.option.uHook.optionRange.high;
				}
				else
				{
					option += increment;
				}
			}
			*menu->u.option.uHook.optionRange.option = option;
		}
		break;
	case MENU_TYPE_SET_OPTION_SEED:
		{
			unsigned int seed = *menu->u.option.uHook.seed;
			unsigned int increment = 1;
			if (Button1(cmd))
			{
				increment *= 10;
			}
			if (Button2(cmd))
			{
				increment *= 100;
			}
			if (Left(cmd))
			{
				if (increment > seed)
				{
					seed = 0;
				}
				else
				{
					seed -= increment;
				}
			}
			else if (Right(cmd))
			{
				if (UINT_MAX - increment < seed)
				{
					seed = UINT_MAX;
				}
				else
				{
					seed += increment;
				}
			}
			*menu->u.option.uHook.seed = seed;
		}
		break;
	case MENU_TYPE_SET_OPTION_UP_DOWN_VOID_FUNC_VOID:
		if (Left(cmd))
		{
			menu->u.option.uHook.upDownFuncs.upFunc();
		}
		else if (Right(cmd))
		{
			menu->u.option.uHook.upDownFuncs.downFunc();
		}
		break;
	case MENU_TYPE_SET_OPTION_RANGE_GET_SET:
		{
			int option = menu->u.option.uHook.optionRangeGetSet.getFunc();
			int increment = menu->u.option.uHook.optionRangeGetSet.increment;
			if (Left(cmd))
			{
				if (menu->u.option.uHook.optionRangeGetSet.low + increment > option)
				{
					option = menu->u.option.uHook.optionRangeGetSet.low;
				}
				else
				{
					option -= increment;
				}
			}
			else if (Right(cmd))
			{
				if (menu->u.option.uHook.optionRangeGetSet.high - increment < option)
				{
					option = menu->u.option.uHook.optionRangeGetSet.high;
				}
				else
				{
					option += increment;
				}
			}
			menu->u.option.uHook.optionRangeGetSet.setFunc(option);
		}
		break;
	case MENU_TYPE_SET_OPTION_CHANGE_CONTROL:
		ChangeControl(
			menu->u.option.uHook.changeControl.device0,
			menu->u.option.uHook.changeControl.device1,
			gSticks[0].present, gSticks[1].present);
		break;
	case MENU_TYPE_VOID_FUNC_VOID:
		menu->u.option.uHook.toggleFuncs.toggle();
		break;
	case MENU_TYPE_SET_OPTION_CHANGE_KEY:
		// TODO: implement
		break;
	default:
		printf("Error unhandled menu type %d\n", menu->type);
		assert(0);
		break;
	}
}

void MenuChangeIndex(menu_t *menu, int cmd)
{
	int leftRightMoves = menu->type == MENU_TYPE_NORMAL;
	if (Up(cmd) || (leftRightMoves && Left(cmd)))
	{
		do
		{
			menu->u.normal.index--;
			if (menu->u.normal.index < 0)
			{
				menu->u.normal.index = menu->u.normal.numSubMenus - 1;
			}
		} while (menu->u.normal.subMenus[menu->u.normal.index].type ==
			MENU_TYPE_SEPARATOR);
		PlaySound(SND_DOOR, 0, 255);
	}
	else if (Down(cmd) || (leftRightMoves && Right(cmd)))
	{
		do
		{
			menu->u.normal.index++;
			if (menu->u.normal.index >= menu->u.normal.numSubMenus)
			{
				menu->u.normal.index = 0;
			}
		} while (menu->u.normal.subMenus[menu->u.normal.index].type ==
			MENU_TYPE_SEPARATOR);
		PlaySound(SND_DOOR, 0, 255);
	}
}
