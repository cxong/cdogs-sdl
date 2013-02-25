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


static struct FileEntry *campaignList = NULL;
static struct FileEntry *dogfightList = NULL;


void LookForCustomCampaigns(void)
{
	int i;

	printf("\nCampaigns:\n");

	campaignList = GetFilesFromDirectory(GetDataFilePath("missions/"));
	GetCampaignTitles(&campaignList);
	i = 0;
	while (SetupBuiltinCampaign(i)) {
		AddFileEntry(&campaignList, "", gCampaign.setting->title, i);
		i++;
	}

	printf("\nDogfights:\n");

	dogfightList = GetFilesFromDirectory(GetDataFilePath("dogfights/"));
	GetCampaignTitles(&dogfightList);
	i = 0;
	while (SetupBuiltinDogfight(i)) {
		AddFileEntry(&dogfightList, "", gCampaign.setting->title, i);
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
	struct FileEntry *list = dogFight ? dogfightList : campaignList;
	char *prefix = dogFight ? "dogfights/" : "missions/";
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

#define MAXCOLOUR 254
unsigned char FitColor(int c)
{
	if (c > MAXCOLOUR)
		return MAXCOLOUR;
	else if (c < 0)
		return 0;
	else
		return (c & 0xFF);
}

static void PaletteAdjust(void)
{
	int i;
	double f;

	f = 1.0 + gOptions.brightness / 33.3;
	for (i = 0; i < 255; i++) {
		gPalette[i].red = FitColor(f * origPalette[i].red);
		gPalette[i].green = FitColor(f * origPalette[i].green);
		gPalette[i].blue = FitColor(f * origPalette[i].blue);
	}
	CDogsSetPalette(gPalette);
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
			PaletteAdjust();
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

int KeyAvailable(int key, struct PlayerData *data, int index,
		 struct PlayerData *other)
{
	int i;

	if (key == keyEsc || key == keyF9 || key == keyF10)
		return 0;

	if (key == gOptions.mapKey && index >= 0)
		return 0;

	for (i = 0; i < 6; i++)
		if (i != index && data->keys[i] == key)
			return 0;

	for (i = 0; i < 6; i++)
		if (other->keys[i] == key)
			return 0;

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
			if (KeyAvailable(key, data, index, other)) {
				data->keys[index] = key;
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
			struct PlayerData *data, int index, int change)
{
	int i;

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
					SDL_GetKeyName(data->keys[i]),
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

int MainMenu(void *bkg, credits_displayer_t *creditsDisplayer)
{
	int cmd, prev = 0;
	int mode = MODE_MAIN;

	PaletteAdjust();

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
	MENU_TYPE_SET_OPTION_TOGGLE,	// no items, sets option on/off
	MENU_TYPE_SET_OPTION_RANGE,		// no items, sets option range low-high
	MENU_TYPE_SET_OPTION_SEED,		// set random seed
	MENU_TYPE_SET_OPTION_UP_DOWN_VOID_FUNC_VOID,	// set option using up/down functions
	MENU_TYPE_SET_OPTION_RANGE_GET_SET,	// set option range low-high using get/set functions
	MENU_TYPE_SET_OPTION_CHANGE_CONTROL,	// change control device
	MENU_TYPE_VOID_FUNC_VOID,		// call a void(*f)(void) function
	MENU_TYPE_BACK,
	MENU_TYPE_QUIT
} menu_type_e;

typedef enum
{
	MENU_DISPLAY_ITEMS_CREDITS	= 0x01,
	MENU_DISPLAY_ITEMS_AUTHORS	= 0x02
} menu_display_items_e;

typedef enum
{
	MENU_OPTION_TYPE_CAMPAIGNS,
	MENU_OPTION_TYPE_DOGFIGHTS,
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
			input_device_e *device0;
			input_device_e *device1;
		} optionChangeControl;
		void (*func)(void);
	} u;
} menu_t;

menu_t *MenuCreateAll(void);
void MenuDestroy(menu_t *menu);
void MenuDisplay(menu_t *menu, credits_displayer_t *creditsDisplayer);
menu_t *MenuProcessCmd(menu_t *menu, int cmd);

int MainMenuNew(void *bkg, credits_displayer_t *creditsDisplayer)
{
	int cmd, prev = 0;
	int mode = MODE_MAIN;
	menu_t *mainMenu = MenuCreateAll();
	menu_t *menu = mainMenu;

	PaletteAdjust();
	do
	{
		memcpy(GetDstScreen(), bkg, SCREEN_MEMSIZE);
		ShowControls();
		GetMenuCmd(&cmd, &prev);
		menu = MenuProcessCmd(menu, cmd);
		MenuDisplay(menu, creditsDisplayer);
		CopyToScreen();
		SDL_Delay(10);
	} while (menu->type != MENU_TYPE_QUIT || (1/*play*/));

	MenuDestroy(mainMenu);
	WaitForRelease();
	return mode == MODE_PLAY;
}

menu_t *MenuCreate(
	const char *name,
	const char *title,
	menu_type_e type,
	int displayItems,
	menu_option_type_e optionType,
	int setOptions,
	int initialIndex);
void MenuAddSubmenu(menu_t *menu, menu_t *subMenu);
menu_t *MenuCreateOnePlayer(const char *name);
menu_t *MenuCreateTwoPlayers(const char *name);
menu_t *MenuCreateDogfight(const char *name);
menu_t *MenuCreateOptions(const char *name);
menu_t *MenuCreateControls(const char *name);
menu_t *MenuCreateSound(const char *name);
menu_t *MenuCreateQuit(const char *name);

menu_t *MenuCreateAll(void)
{
	menu_t *menu = MenuCreate(
		"",
		"",
		MENU_TYPE_NORMAL,
		MENU_DISPLAY_ITEMS_CREDITS | MENU_DISPLAY_ITEMS_AUTHORS,
		0, 0, 0);
	MenuAddSubmenu(menu, MenuCreateOnePlayer("1 player"));
	MenuAddSubmenu(menu, MenuCreateTwoPlayers("2 players"));
	MenuAddSubmenu(menu, MenuCreateDogfight("Dogfight"));
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
	int setOptions,
	int initialIndex)
{
	menu_t *menu = sys_mem_alloc(sizeof(menu_t));
	strcpy(menu->name, name);
	menu->type = type;
	strcpy(menu->u.normal.title, title);
	menu->u.normal.displayItems = displayItems;
	menu->u.normal.optionType = optionType;
	menu->u.normal.setOptions = setOptions;
	menu->u.normal.index = initialIndex;
	menu->u.normal.quitMenuIndex = -1;
	menu->u.normal.parentMenu = NULL;
	menu->u.normal.subMenus = NULL;
	menu->u.normal.numSubMenus = 0;
	return menu;
}

// TODO: make this more efficient?
void MenuAddSubmenu(menu_t *menu, menu_t *subMenu)
{
	menu_t *subMenuLoc = NULL;
	menu->u.normal.numSubMenus++;
	menu->u.normal.subMenus = sys_mem_realloc(
		menu->u.normal.subMenus, menu->u.normal.numSubMenus*sizeof(menu_t));
	subMenuLoc = &menu->u.normal.subMenus[menu->u.normal.numSubMenus - 1];
	memcpy(subMenuLoc, subMenu, sizeof(menu_t));
	if (subMenu->type == MENU_TYPE_QUIT)
	{
		menu->u.normal.quitMenuIndex = menu->u.normal.numSubMenus - 1;
	}
	subMenuLoc->u.normal.parentMenu = menu;
	sys_mem_free(subMenu);
}

menu_t *MenuCreateOnePlayer(const char *name)
{
	menu_t *menu = MenuCreate(
		name,
		"Select a campaign:",
		MENU_TYPE_NORMAL,
		0,
		MENU_OPTION_TYPE_CAMPAIGNS,
		0, 0);
	return menu;
}

menu_t *MenuCreateTwoPlayers(const char *name)
{
	menu_t *menu = MenuCreate(
		name,
		"Select a campaign:",
		MENU_TYPE_NORMAL,
		0,
		MENU_OPTION_TYPE_CAMPAIGNS,
		MENU_SET_OPTIONS_TWOPLAYERS,
		0);
	return menu;
}

menu_t *MenuCreateDogfight(const char *name)
{
	menu_t *menu = MenuCreate(
		name,
		"Select a dogfight scenario:",
		MENU_TYPE_NORMAL,
		0,
		MENU_OPTION_TYPE_DOGFIGHTS,
		MENU_SET_OPTIONS_DOGFIGHT,
		0);
	return menu;
}

menu_t *MenuCreateOptionToggle(const char *name, int *config);
menu_t *MenuCreateOptionRange(
	const char *name, int *config, int low, int high, int increment);
menu_t *MenuCreateOptionSeed(const char *name, unsigned int *seed);
menu_t *MenuCreateOptionUpDownFunc(
	const char *name, void(*upFunc)(void), void(*downFunc)(void));
menu_t *MenuCreateOptionFunc(const char *name, void(*func)(void));
menu_t *MenuCreateOptionRangeGetSet(
	const char *name,
	int(*getFunc)(void), void(*setFunc)(int), int low, int high, int increment);
menu_t *MenuCreateBack(const char *name);

menu_t *MenuCreateOptions(const char *name)
{
	menu_t *menu = MenuCreate(
			name,
			"Game Options:",
			MENU_TYPE_OPTIONS,
			0,
			MENU_OPTION_TYPE_OPTIONS,
			0, 0);
	MenuAddSubmenu(
		menu, MenuCreateOptionToggle("Friendly fire", &gOptions.playersHurt));
	MenuAddSubmenu(
		menu, MenuCreateOptionToggle("FPS monitor", &gOptions.displayFPS));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionRange("Brightness", &gOptions.brightness, -10, 10, 1));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionToggle(
			"Splitscreen always", &gOptions.splitScreenAlways));
	MenuAddSubmenu(
		menu, MenuCreateOptionSeed("Random seed", &gCampaign.seed));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionRange(
			"Difficulty", (int *)&gOptions.difficulty,
			DIFFICULTY_VERYEASY, DIFFICULTY_VERYHARD, 1));
	MenuAddSubmenu(
		menu, MenuCreateOptionToggle("Slowmotion", &gOptions.slowmotion));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionRange(
			"Enemy density per mission",
			&gOptions.density,
			0, 200, 25));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionRange("Non-player HP", &gOptions.npcHp, 0, 200, 25));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionRange("Player HP", &gOptions.playerHp, 0, 200, 25));
	MenuAddSubmenu(
		menu, MenuCreateOptionFunc("Video fullscreen", GrafxToggleFullscreen));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionUpDownFunc(
			"Video resolution (restart required)",
			GrafxTryPrevResolution,
			GrafxTryNextResolution));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionRangeGetSet(
			"Video scale factor", GrafxGetScale, GrafxSetScale, 1, 4, 1));
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
			0, 0);
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
			"Swap buttons joystick 1", &gOptions.swapButtonsJoy1));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionToggle(
			"Swap buttons joystick 2", &gOptions.swapButtonsJoy2));
	MenuAddSubmenu(menu, MenuCreateKeys("Redefine keys..."));
	MenuAddSubmenu(
		menu, MenuCreateOptionFunc("Calibrate joystick", InitSticks));
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
		0, 0);
	MenuAddSubmenu(
		menu,
		MenuCreateOptionRangeGetSet(
			"Sound effects", FXVolume, SetFXVolume, 8, 64, 8));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionRangeGetSet(
			"Music", MusicVolume, SetMusicVolume, 8, 64, 8));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionRangeGetSet(
			"FX channels", FXChannels, SetFXChannels, 2, 8, 1));
	MenuAddSubmenu(menu, MenuCreateBack("Done"));
	return menu;
}

menu_t *MenuCreateQuit(const char *name)
{
	menu_t *menu = MenuCreate(name, "", MENU_TYPE_QUIT, 0, 0, 0, 0);
	return menu;
}


menu_t *MenuCreateOptionToggle(const char *name, int *config)
{
	menu_t *menu = sys_mem_alloc(sizeof(menu_t));
	strcpy(menu->name, name);
	menu->type = MENU_TYPE_SET_OPTION_TOGGLE;
	menu->u.optionToggle = config;
	return menu;
}

menu_t *MenuCreateOptionRange(
	const char *name, int *config, int low, int high, int increment)
{
	menu_t *menu = sys_mem_alloc(sizeof(menu_t));
	strcpy(menu->name, name);
	menu->type = MENU_TYPE_SET_OPTION_RANGE;
	menu->u.optionRange.option = config;
	menu->u.optionRange.low = low;
	menu->u.optionRange.high = high;
	menu->u.optionRange.increment = increment;
	return menu;
}

menu_t *MenuCreateOptionSeed(const char *name, unsigned int *seed)
{
	menu_t *menu = sys_mem_alloc(sizeof(menu_t));
	strcpy(menu->name, name);
	menu->type = MENU_TYPE_SET_OPTION_SEED;
	menu->u.seed = seed;
	return menu;
}

menu_t *MenuCreateOptionUpDownFunc(
	const char *name, void(*upFunc)(void), void(*downFunc)(void))
{
	menu_t *menu = sys_mem_alloc(sizeof(menu_t));
	strcpy(menu->name, name);
	menu->type = MENU_TYPE_SET_OPTION_UP_DOWN_VOID_FUNC_VOID;
	menu->u.upDownFuncs.upFunc = upFunc;
	menu->u.upDownFuncs.downFunc = downFunc;
	return menu;
}

menu_t *MenuCreateOptionFunc(const char *name, void(*func)(void))
{
	menu_t *menu = sys_mem_alloc(sizeof(menu_t));
	strcpy(menu->name, name);
	menu->type = MENU_TYPE_VOID_FUNC_VOID;
	menu->u.func = func;
	return menu;
}

menu_t *MenuCreateOptionRangeGetSet(
	const char *name,
	int(*getFunc)(void), void(*setFunc)(int), int low, int high, int increment)
{
	menu_t *menu = sys_mem_alloc(sizeof(menu_t));
	strcpy(menu->name, name);
	menu->type = MENU_TYPE_SET_OPTION_RANGE_GET_SET;
	menu->u.optionRangeGetSet.getFunc = getFunc;
	menu->u.optionRangeGetSet.setFunc = setFunc;
	menu->u.optionRangeGetSet.low = low;
	menu->u.optionRangeGetSet.high = high;
	menu->u.optionRangeGetSet.increment = increment;
	return menu;
}

menu_t *MenuCreateBack(const char *name)
{
	menu_t *menu = MenuCreate(name, "", MENU_TYPE_BACK, 0, 0, 0, 0);
	return menu;
}

menu_t *MenuCreateOptionChangeControl(
	const char *name, input_device_e *device0, input_device_e *device1)
{
	menu_t *menu = sys_mem_alloc(sizeof(menu_t));
	strcpy(menu->name, name);
	menu->type = MENU_TYPE_SET_OPTION_CHANGE_CONTROL;
	menu->u.optionChangeControl.device0 = device0;
	menu->u.optionChangeControl.device1 = device1;
	return menu;
}

menu_t *MenuCreateKeys(const char *name)
{
	menu_t *menu = MenuCreate(
		name,
		"",
		MENU_TYPE_OPTIONS,
		0,
		MENU_OPTION_TYPE_KEYS,
		0, 0);
	// TODO: convert keys to menu items
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
	if ((menu->type == MENU_TYPE_NORMAL || menu->type == MENU_TYPE_OPTIONS) &&
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
void MenuDisplaySubmenus(menu_t *menu, int isCentered);

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

	MenuDisplaySubmenus(menu, menu->type == MENU_TYPE_NORMAL);
}

void MenuDisplayItems(menu_t *menu, credits_displayer_t *creditsDisplayer)
{
	int d = menu->u.normal.displayItems;
	assert(menu->type == MENU_TYPE_NORMAL || menu->type == MENU_TYPE_OPTIONS);
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

void MenuDisplaySubmenus(menu_t *menu, int isCentered)
{
	int i;
	int x, yStart;
	int maxWidth = 0;
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
	yStart = menu->u.normal.numSubMenus * CDogsTextHeight();

	// Display normal menu items
	for (i = 0; i < menu->u.normal.numSubMenus; i++)
	{
		int y = yStart + i * CDogsTextHeight();
		const char *name = menu->u.normal.subMenus[i].name;
		if (i == menu->u.normal.index)
		{
			CDogsTextStringWithTableAt(x, y, name, &tableFlamed);
		}
		else
		{
			CDogsTextStringAt(x, y, name);
		}
	}

	// Display menu items for options
	switch (menu->u.normal.optionType)
	{
	case MENU_OPTION_TYPE_CAMPAIGNS:
		assert(0);
		break;
	case MENU_OPTION_TYPE_DOGFIGHTS:
		assert(0);
		break;
	case MENU_OPTION_TYPE_OPTIONS:
		assert(0);
		break;
	case MENU_OPTION_TYPE_CONTROLS:
		assert(0);
		break;
	case MENU_OPTION_TYPE_SOUND:
		{
			int y = yStart;
			x += maxWidth + 10;

			CDogsTextIntAt(x, y, FXVolume() / 8);
			y += CDogsTextHeight();
			CDogsTextIntAt(x, y, MusicVolume() / 8);
			y += CDogsTextHeight();
			CDogsTextIntAt(x, y, FXChannels());
		}
		break;
	case MENU_OPTION_TYPE_KEYS:
		assert(0);
		break;
	default:
		break;
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
			return menuToChange;
		}
	}
	menuToChange = MenuProcessButtonCmd(menu, cmd);
	if (menuToChange != NULL)
	{
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
			return subMenu;
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

void MenuActivate(menu_t *menu, int cmd)
{
	switch (menu->type)
	{
	case MENU_TYPE_SET_OPTION_TOGGLE:
		*menu->u.optionToggle = !*menu->u.optionToggle;
		break;
	case MENU_TYPE_SET_OPTION_RANGE:
		{
			int option = *menu->u.optionRange.option;
			int increment = menu->u.optionRange.increment;
			if (Left(cmd))
			{
				if (menu->u.optionRange.low + increment > option)
				{
					option = menu->u.optionRange.low;
				}
				else
				{
					option -= increment;
				}
			}
			else if (Right(cmd))
			{
				if (menu->u.optionRange.high - increment < option)
				{
					option = menu->u.optionRange.high;
				}
				else
				{
					option += increment;
				}
			}
			*menu->u.optionRange.option = option;
		}
		break;
	case MENU_TYPE_SET_OPTION_SEED:
		{
			unsigned int seed = *menu->u.seed;
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
			*menu->u.seed = seed;
		}
		break;
	case MENU_TYPE_SET_OPTION_UP_DOWN_VOID_FUNC_VOID:
		if (Left(cmd))
		{
			menu->u.upDownFuncs.upFunc();
		}
		else if (Right(cmd))
		{
			menu->u.upDownFuncs.downFunc();
		}
		break;
	case MENU_TYPE_SET_OPTION_RANGE_GET_SET:
		{
			int option = menu->u.optionRangeGetSet.getFunc();
			int increment = menu->u.optionRangeGetSet.increment;
			if (Left(cmd))
			{
				if (menu->u.optionRangeGetSet.low + increment > option)
				{
					option = menu->u.optionRangeGetSet.low;
				}
				else
				{
					option -= increment;
				}
			}
			else if (Right(cmd))
			{
				if (menu->u.optionRangeGetSet.high - increment < option)
				{
					option = menu->u.optionRangeGetSet.high;
				}
				else
				{
					option += increment;
				}
			}
			menu->u.optionRangeGetSet.setFunc(option);
		}
		break;
	case MENU_TYPE_SET_OPTION_CHANGE_CONTROL:
		ChangeControl(
			menu->u.optionChangeControl.device0,
			menu->u.optionChangeControl.device1,
			gSticks[0].present, gSticks[1].present);
		break;
	case MENU_TYPE_VOID_FUNC_VOID:
		menu->u.func();
		break;
	default:
		assert(0);
		break;
	}
}

void MenuChangeIndex(menu_t *menu, int cmd)
{
	int leftRightMoves = menu->type == MENU_TYPE_NORMAL;
	if (Up(cmd) || (leftRightMoves && Left(cmd)))
	{
		menu->u.normal.index--;
		if (menu->u.normal.index < 0)
		{
			menu->u.normal.index = menu->u.normal.numSubMenus - 1;
		}
		PlaySound(SND_DOOR, 0, 255);
	}
	else if (Down(cmd) || (leftRightMoves && Right(cmd)))
	{
		menu->u.normal.index++;
		if (menu->u.normal.index >= menu->u.normal.numSubMenus)
		{
			menu->u.normal.index = 0;
		}
		PlaySound(SND_DOOR, 0, 255);
	}
}
