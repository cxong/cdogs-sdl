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

 mainmenu.c - main menu functions 

*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "mainmenu.h"
#include "defs.h"
#include "input.h"
#include "grafx.h"
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

static const char *mainMenu[MAIN_COUNT] = {
	"1 player game",
	"2 player game",
	"Dog fight",
	"Game options...",
	"Controls...",
	"Sound...",
	"Quit"
};

#define OPTIONS_COUNT   13

static const char *optionsMenu[OPTIONS_COUNT] = {
	"Players shots hurt",
	"FPS monitor",
	"Clock",
	"Copy to video",
	"Brightness",
	"Splitscreen always",
	"Random seed",
	"Difficulty",
	"Slowmotion",
	"Density",
	"Non-player hp",
	"Player hp",
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

#define VOLUME_COUNT   5

static const char *volumeMenu[VOLUME_COUNT] = {
	"Sound effects",
	"Music",
	"FX channels",
	"Disable interrupts during game",
	"Done"
};

struct Credit {
	char *name;
	char *message;
};

static struct Credit credits[] = {
	{"Ronny Wester",
	 "That's me! I designed and coded this game and I did all the graphics too"},
	{"Joey Lau",
	 "My love. Although I had to alter the knife rating to survive the dogfights ;-)"},
	{"Jan-Olof Hendig",
	 "A friend with whom I've spent far too much time on the phone far too late"},
	{"Joakim Johansson",
	 "Friend and colleague. We keep each other sane...or is it the other way around?"},
	{"Victor Putz",
	 "Kindred spirit and a great guy to brainstorm with"},
	{"Ken Gorley",
	 "Found the 1st bug in the release: didn't work with a single joystick"},
	{"Adrian Stacey", "Most helpful in tracking down the vsync bug"},
	{"Antti Hukkanen",
	 "Helped locate a bug when using maximum number of characters in a mission"},
	{"Tim McEvoy", "Instrumental in getting the 1.05 crash bug fixed"},
	{"Daniel Jansson", "Friend and fellow aikido instructor"},
	{"Chilok Lau", "Joey's brother and a relentless game-player"},
	{"Niklas Wester",
	 "My LITTLE brother...yeah, I know he is taller...mumble, grumble..."},
	{"Anneli L”fgren",
	 "My mother. Hi Ma! Sorry, no launcher this time..."},
	{"Otto Chrons", "Author of the DSMI sound and music library"},
	{"Ethan Brodsky",
	 "Author of free SB stuff. Helped me out when I got started"},
	{"Tech support at DPT",
	 "The best support staff I've ever come across. Kudos!"},
	{"Cyberdogs fans wherever",
	 "Thanks! Without your support this program would never have come about"},
	
	/* C-Dogs SDL Credits :D */
	{"Lucas Martin-King",
	 "He procrastinated about releasing C-Dogs SDL! ...and cleaned up after Jeremy"},
	{"Jeremy Chin",
	 "He did all the hard porting work! ;)"}
};

#define CREDIT_PERIOD   10


static TCampaignSetting customSetting = {
	.title		=	"",
	.author		=	"",
	.description	=	"",
	.missionCount	=	0,
	.missions	=	NULL,
	.characterCount	=	0,
	.characters	=	NULL };


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
		else
			SetupBuiltin(dogFight, 0);
//  gCampaign.seed = 0;
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
		TextStringAt(50, 30, "Pick a dogfight scenario");
	else
		TextStringAt(50, 30, "Select campaign");

	y = 50;
	for (i = 0, f = list; f != NULL && i <= *index - 12;
	     f = f->next, i++);
	if (i)
		DisplayMenuItem(25, y - TextHeight(), "\036", 0);

	for (j = 0; f != NULL && j < 12; f = f->next, i++, j++) {
		DisplayMenuItem(25, y, f->name, i == *index);
		DisplayMenuItem(125, y, f->info, i == *index);
		y += TextHeight();
	}
	if (f)
		DisplayMenuItem(25, y, "\037", 0);

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

	DrawTPic(125, 4, gPics[PIC_LOGO], gCompiledPics[PIC_LOGO]);
	TextStringAt(20, 20, CDOGS_VERSION);

	DisplayMenu(132, mainMenu, MAIN_COUNT, index);
	return MODE_MAIN;
}

unsigned char FitColor(int c)
{
	if (c > 63)
		return 63;
	else if (c < 0)
		return 0;
	else
		return (c & 0xFF);
}

static void PaletteAdjust(void)
{
	int i;
	double f;

	f = 1.0 + gOptions.brightness / 33.0;
	for (i = 0; i < 255; i++) {
		gPalette[i].red = FitColor(f * origPalette[i].red);
		gPalette[i].green = FitColor(f * origPalette[i].green);
		gPalette[i].blue = FitColor(f * origPalette[i].blue);
	}
	SetPalette(gPalette);
}

int SelectOptions(int cmd)
{
	static int index = 0;
	char s[10];

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
			if (gOptions.copyMode == COPY_REPMOVSD)
				gOptions.copyMode = COPY_DEC_JNZ;
			else
				gOptions.copyMode = COPY_REPMOVSD;
			PlaySound(SND_EXPLOSION, 0, 255);
			break;
		case 4:
			if (Left(cmd) && gOptions.brightness > -10)
				gOptions.brightness--;
			else if (Right(cmd) && gOptions.brightness < 10)
				gOptions.brightness++;
			else
				break;
			PlaySound(SND_POWERGUN, 0, 255);
			PaletteAdjust();
			break;
		case 5:
			if (gOptions.xSplit == 0) {
				gOptions.xSplit = SPLIT_X;
				gOptions.ySplit = SPLIT_Y;
			} else
				gOptions.xSplit = gOptions.ySplit = 0;
			PlaySound(SND_KILL3, 0, 255);
			break;
		case 6:
			if (Left(cmd)) {
				if (Button1(cmd) && Button2(cmd))
					gCampaign.seed -= 1000;
				else if (Button1(cmd))
					gCampaign.seed -= 10;
				else if (Button2(cmd))
					gCampaign.seed -= 100;
				else
					gCampaign.seed--;
			} else if Right
				(cmd) {
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
		case 7:
			if (Left(cmd)) {
				if (gOptions.difficulty >
				    DIFFICULTY_VERYEASY)
					gOptions.difficulty--;
			} else if (Right(cmd)) {
				if (gOptions.difficulty <
				    DIFFICULTY_VERYHARD)
					gOptions.difficulty++;
			}
			break;
		case 8:
			gOptions.slowmotion = !gOptions.slowmotion;
			break;
		case 9:
			if (Left(cmd)) {
				if (gOptions.density > 25)
					gOptions.density -= 25;
			} else if (Right(cmd)) {
				if (gOptions.density < 200)
					gOptions.density += 25;
			}
			break;
		case 10:
			if (Left(cmd)) {
				if (gOptions.npcHp > 25)
					gOptions.npcHp -= 25;
			} else if (Right(cmd)) {
				if (gOptions.npcHp < 200)
					gOptions.npcHp += 25;
			}
			break;
		case 11:
			if (Left(cmd)) {
				if (gOptions.playerHp > 25)
					gOptions.playerHp -= 25;
			} else if (Right(cmd)) {
				if (gOptions.playerHp < 200)
					gOptions.playerHp += 25;
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

	DisplayMenu(75, optionsMenu, OPTIONS_COUNT, index);

	TextStringAt(230, 50, gOptions.playersHurt ? "Yes" : "No");
	TextStringAt(230, 50 + TextHeight(),
		     gOptions.displayFPS ? "On" : "Off");
	TextStringAt(230, 50 + 2 * TextHeight(),
		     gOptions.displayTime ? "On" : "Off");
	TextStringAt(230, 50 + 3 * TextHeight(),
		     gOptions.copyMode ==
		     COPY_REPMOVSD ? "rep movsd" : "dec/jnz");
	sprintf(s, "%d", gOptions.brightness);
	TextStringAt(230, 50 + 4 * TextHeight(), s);
	TextStringAt(230, 50 + 5 * TextHeight(),
		     gOptions.xSplit ? "No" : "Yes");
	sprintf(s, "%u", gCampaign.seed);
	TextStringAt(230, 50 + 6 * TextHeight(), s);
	switch (gOptions.difficulty) {
	case DIFFICULTY_VERYEASY:
		strcpy(s, "Easiest");
		break;
	case DIFFICULTY_EASY:
		strcpy(s, "Easy");
		break;
	case DIFFICULTY_HARD:
		strcpy(s, "Hard");
		break;
	case DIFFICULTY_VERYHARD:
		strcpy(s, "Very hard");
		break;
	default:
		strcpy(s, "Normal");
		break;
	}
	TextStringAt(230, 50 + 7 * TextHeight(), s);
	TextStringAt(230, 50 + 8 * TextHeight(),
		     gOptions.slowmotion ? "Yes" : "No");
	sprintf(s, "%u%%", gOptions.density);
	TextStringAt(230, 50 + 9 * TextHeight(), s);
	sprintf(s, "%u%%", gOptions.npcHp);
	TextStringAt(230, 50 + 10 * TextHeight(), s);
	sprintf(s, "%u%%", gOptions.playerHp);
	TextStringAt(230, 50 + 11 * TextHeight(), s);

	return MODE_OPTIONS;
}

static void ChangeControl(struct PlayerData *data,
			  struct PlayerData *other)
{
	if (data->controls == JOYSTICK_ONE) {
		if (other->controls != JOYSTICK_TWO && gSticks[1].present)
			data->controls = JOYSTICK_TWO;
		else
			data->controls = KEYBOARD;
	} else if (data->controls == JOYSTICK_TWO)
		data->controls = KEYBOARD;
	else {
		if (other->controls != JOYSTICK_ONE && gSticks[0].present)
			data->controls = JOYSTICK_ONE;
		else if (gSticks[1].present)
			data->controls = JOYSTICK_TWO;
	}
}

static void DisplayControl(int x, int y, int controls)
{
	if (controls == JOYSTICK_ONE)
		TextStringAt(x, y, "Joystick 1");
	else if (controls == JOYSTICK_TWO)
		TextStringAt(x, y, "Joystick 2");
	else
		TextStringAt(x, y, "Keyboard");
}

int SelectControls(int cmd)
{
	static int index = 0;

	if (cmd == CMD_ESC)
		return MODE_MAIN;
	if (AnyButton(cmd) || Left(cmd) || Right(cmd)) {
		PlaySound(rand() % SND_COUNT, 0, 255);
		switch (index) {
		case 0:
			ChangeControl(&gPlayer1Data, &gPlayer2Data);
			break;

		case 1:
			ChangeControl(&gPlayer2Data, &gPlayer1Data);
			break;

		case 2:
			gOptions.swapButtonsJoy1 =
			    !gOptions.swapButtonsJoy1;
			break;

		case 3:
			gOptions.swapButtonsJoy2 =
			    !gOptions.swapButtonsJoy2;
			break;

		case 4:
			return MODE_KEYS;

		case 5:
			InitSticks();
			AutoCalibrate();
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

	DisplayMenu(75, controlsMenu, CONTROLS_COUNT, index);

	DisplayControl(230, 50, gPlayer1Data.controls);
	DisplayControl(230, 50 + TextHeight(), gPlayer2Data.controls);
	TextStringAt(230, 50 + 2 * TextHeight(),
		     gOptions.swapButtonsJoy1 ? "Yes" : "No");
	TextStringAt(230, 50 + 3 * TextHeight(),
		     gOptions.swapButtonsJoy2 ? "Yes" : "No");
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
	printf("x\n");
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
//              SoundTick();
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
}


#define SELECTKEY "Press a key"

static void DisplayKeys(int x, int x2, int y, char *title,
			struct PlayerData *data, int index, int change)
{
	int i;

	TextStringAt(x, y, title);
	TextStringAt(x, y + TextHeight(), "Left");
	TextStringAt(x, y + 2 * TextHeight(), "Right");
	TextStringAt(x, y + 3 * TextHeight(), "Up");
	TextStringAt(x, y + 4 * TextHeight(), "Down");
	TextStringAt(x, y + 5 * TextHeight(), "Fire");
	TextStringAt(x, y + 6 * TextHeight(), "Switch/slide");

	for (i = 0; i < 6; i++)
		if (change == i)
			DisplayMenuItem(x2, y + (i + 1) * TextHeight(),
					SELECTKEY, index == i);
		else
			DisplayMenuItem(x2, y + (i + 1) * TextHeight(),
					SDL_GetKeyName(data->keys[i]),
					index == i);
}

static void ShowAllKeys(int index, int change)
{
	DisplayKeys(75, 230, 20, "Player One", &gPlayer1Data, index,
		    change);
	DisplayKeys(75, 230, 100, "Player Two", &gPlayer2Data, index - 6,
		    change - 6);
	TextStringAt(75, 170, "Map");
	if (change == 12)
		DisplayMenuItem(230, 170, SELECTKEY, index == 12);
	else
		DisplayMenuItem(230, 170, SDL_GetKeyName(gOptions.mapKey),
				index == 12);
	DisplayMenuItem(75, 180, "Done", index == 13);
}

static void HighlightKey(int index)
{
	void *scr = GetDstScreen();

	CopyToScreen();
//      SetDstScreen((void *) 0xA0000);
	ShowAllKeys(index, index);
//      SetDstScreen(scr);
}

int SelectKeys(int cmd)
{
	static int index = 12;

	if (cmd == CMD_ESC)
		return MODE_CONTROLS;
	if (AnyButton(cmd)) {
//              PlaySound(rand() % SND_COUNT, 0, 255);
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

	DisplayMenu(75, volumeMenu, VOLUME_COUNT, index);

	sprintf(s, "%d", FXVolume() / 8);
	TextStringAt(230, 50, s);
	sprintf(s, "%d", MusicVolume() / 8);
	TextStringAt(230, 50 + TextHeight(), s);
	sprintf(s, "%d", FXChannels());
	TextStringAt(230, 50 + 2 * TextHeight(), s);
	TextStringAt(230, 50 + 3 * TextHeight(), "No");

	return MODE_VOLUME;
}

int MakeSelection(int mode, int cmd)
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

static void ShowCredits(void)
{
	static int creditIndex = 0;
	static int lastTick = 0;
	int t;

	TextStringWithTableAt(40, 130, "Credits:", &tableDarker);
	TextStringWithTableAt(50, 140, credits[creditIndex].name,
			      &tablePurple);
	TextStringWithTableAt(20, 140 + TextHeight(),
			      credits[creditIndex].message, &tableDarker);

	t = clock() / CLOCKS_PER_SEC;
	if (t > lastTick + CREDIT_PERIOD) {
		creditIndex++;
		if (creditIndex >= sizeof(credits) / sizeof(credits[0]))
			creditIndex = 0;
		lastTick = t;
	}
}

int MainMenu(void *bkg)
{
	int cmd, prev = 0;
	int mode;
	SDL_Event evt;

	mode = MODE_MAIN;
	while (mode != MODE_QUIT && mode != MODE_PLAY) {
		 memcpy(GetDstScreen(), bkg, 64000);
		ShowControls();
		if (mode == MODE_MAIN)
			ShowCredits();
		GetMenuCmd(&cmd);
		if (cmd == prev)
			cmd = 0;
		else
			prev = cmd;
		mode = MakeSelection(mode, cmd);
		CopyToScreen();
	}
	WaitForRelease();
	return mode == MODE_PLAY;
}


void LoadConfig(void)
{
	FILE *f;
	int fx, music, channels, musicChannels;
	int dynamic;
	char s[128];

	f = fopen(GetConfigFilePath("options.cnf"), "r");
	if (f) {
		fscanf(f, "%d %d %d %d %d %d %d %d %d\n",
		       &gOptions.displayFPS,
		       &gOptions.displayTime,
		       &gOptions.playersHurt,
		       &gOptions.copyMode,
		       &gOptions.brightness,
		       &gOptions.swapButtonsJoy1,
		       &gOptions.swapButtonsJoy2,
		       &gOptions.xSplit, &gOptions.ySplit);
		fscanf(f, "%d\n%d %d %d %d %d %d\n",
		       &gPlayer1Data.controls,
		       &gPlayer1Data.keys[0],
		       &gPlayer1Data.keys[1],
		       &gPlayer1Data.keys[2],
		       &gPlayer1Data.keys[3],
		       &gPlayer1Data.keys[4], &gPlayer1Data.keys[5]);
		fscanf(f, "%d\n%d %d %d %d %d %d\n",
		       &gPlayer2Data.controls,
		       &gPlayer2Data.keys[0],
		       &gPlayer2Data.keys[1],
		       &gPlayer2Data.keys[2],
		       &gPlayer2Data.keys[3],
		       &gPlayer2Data.keys[4], &gPlayer2Data.keys[5]);
		fscanf(f, "%d\n", &gOptions.mapKey);
		fscanf(f, "%d %d %d %d\n",
		       &fx, &music, &channels, &musicChannels);
		SetFXVolume(fx);
		SetMusicVolume(music);
		SetFXChannels(channels);
		SetMinMusicChannels(musicChannels);

		fscanf(f, "%d\n", &dynamic);
		fscanf(f, "%s\n", s);
		SetModuleDirectory(s);
		fscanf(f, "%u\n", &gCampaign.seed);
		fscanf(f, "%d %d\n", &gOptions.difficulty,
		       &gOptions.slowmotion);

		fscanf(f, "%d\n", &gOptions.density);
		if (gOptions.density < 25 || gOptions.density > 200)
			gOptions.density = 100;
		fscanf(f, "%d\n", &gOptions.npcHp);
		if (gOptions.npcHp < 25 || gOptions.npcHp > 200)
			gOptions.npcHp = 100;
		fscanf(f, "%d\n", &gOptions.playerHp);
		if (gOptions.playerHp < 25 || gOptions.playerHp > 200)
			gOptions.playerHp = 100;

		fclose(f);
	}
}

void SaveConfig(void)
{
	FILE *f;

	f = fopen(GetConfigFilePath("options.cnf"), "w");
	if (f) {
		fprintf(f, "%d %d %d %d %d %d %d %d %d\n",
			gOptions.displayFPS,
			gOptions.displayTime,
			gOptions.playersHurt,
			gOptions.copyMode,
			gOptions.brightness,
			gOptions.swapButtonsJoy1,
			gOptions.swapButtonsJoy2,
			gOptions.xSplit, gOptions.ySplit);
		fprintf(f, "%d\n%d %d %d %d %d %d\n",
			gPlayer1Data.controls,
			gPlayer1Data.keys[0],
			gPlayer1Data.keys[1],
			gPlayer1Data.keys[2],
			gPlayer1Data.keys[3],
			gPlayer1Data.keys[4], gPlayer1Data.keys[5]);
		fprintf(f, "%d\n%d %d %d %d %d %d\n",
			gPlayer2Data.controls,
			gPlayer2Data.keys[0],
			gPlayer2Data.keys[1],
			gPlayer2Data.keys[2],
			gPlayer2Data.keys[3],
			gPlayer2Data.keys[4], gPlayer2Data.keys[5]);
		fprintf(f, "%d\n", gOptions.mapKey);
		fprintf(f, "%d %d %d %d\n",
			FXVolume(),
			MusicVolume(), FXChannels(), MinMusicChannels());
		fprintf(f, "%d\n", 0); // DynamicInterrupts
		fprintf(f, "%s\n", ModuleDirectory());
		fprintf(f, "%u\n", gCampaign.seed);
		fprintf(f, "%d %d\n", gOptions.difficulty,
			gOptions.slowmotion);
		fprintf(f, "%d\n", gOptions.density);
		fprintf(f, "%d\n", gOptions.npcHp);
		fprintf(f, "%d\n", gOptions.playerHp);
		fclose(f);
	}
}
