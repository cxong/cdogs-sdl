/*
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
#include "mainmenu.h"

#include <cdogs/blit.h>
#include <cdogs/gamedata.h>
#include <cdogs/joystick.h>
#include <cdogs/keyboard.h>
#include <cdogs/music.h>
#include <cdogs/sounds.h>
#include <cdogs/text.h>

#include "credits.h"
#include "menu.h"


menu_t *MenuCreateAll(custom_campaigns_t *campaigns);

int MainMenu(
	void *bkg,
	credits_displayer_t *creditsDisplayer,
	custom_campaigns_t *campaigns)
{
	int doPlay = 0;
	menu_t *mainMenu = MenuCreateAll(campaigns);
	menu_t *menu = mainMenu;

	BlitSetBrightness(gOptions.brightness);
	do
	{
		KeyPoll(&gKeyboard);
		JoyPoll(&gJoysticks);
		memcpy(GetDstScreen(), bkg, Screen_GetMemSize());
		ShowControls();
		MenuDisplay(menu, creditsDisplayer);
		CopyToScreen();
		if (menu->type == MENU_TYPE_KEYS && menu->u.normal.changeKeyMenu != NULL)
		{
			MenuProcessChangeKey(menu);
		}
		else
		{
			int cmd = GetMenuCmd();
			menu = MenuProcessCmd(menu, cmd);
		}
		SDL_Delay(10);
	} while (menu->type != MENU_TYPE_QUIT && menu->type != MENU_TYPE_CAMPAIGN_ITEM);
	doPlay = menu->type == MENU_TYPE_CAMPAIGN_ITEM;

	MenuDestroy(mainMenu);
	return doPlay;
}

menu_t *MenuCreateQuickPlay(const char *name, campaign_entry_t *entry);
menu_t *MenuCreateCampaigns(
	const char *name,
	const char *title,
	campaign_list_t *list,
	int is_two_player);
menu_t *MenuCreateOptions(const char *name);
menu_t *MenuCreateControls(const char *name);
menu_t *MenuCreateSound(const char *name);
menu_t *MenuCreateQuit(const char *name);

menu_t *MenuCreateAll(custom_campaigns_t *campaigns)
{
	menu_t *menu = MenuCreateNormal(
		"",
		"",
		MENU_TYPE_NORMAL,
		MENU_DISPLAY_ITEMS_CREDITS | MENU_DISPLAY_ITEMS_AUTHORS);
	MenuAddSubmenu(
		menu,
		MenuCreateQuickPlay("Quick Play", &campaigns->quickPlayEntry));
	MenuAddSubmenu(
		menu,
		MenuCreateCampaigns(
			"1 player",
			"Select a campaign:",
			&campaigns->campaignList,
			0));
	MenuAddSubmenu(
		menu,
		MenuCreateCampaigns(
			"2 players",
			"Select a campaign:",
			&campaigns->campaignList,
			1));
	MenuAddSubmenu(
		menu,
		MenuCreateCampaigns(
			"Dogfight",
			"Select a dogfight scenario:",
			&campaigns->dogfightList,
			1));
	MenuAddSubmenu(menu, MenuCreateOptions("Game options..."));
	MenuAddSubmenu(menu, MenuCreateControls("Controls..."));
	MenuAddSubmenu(menu, MenuCreateSound("Sound..."));
	MenuAddSubmenu(menu, MenuCreateQuit("Quit"));
	return menu;
}


menu_t *MenuCreateQuickPlay(const char *name, campaign_entry_t *entry)
{
	menu_t *menu = MenuCreate(name, MENU_TYPE_CAMPAIGN_ITEM);
	memcpy(
		&menu->u.campaign.campaignEntry,
		entry,
		sizeof(menu->u.campaign.campaignEntry));
	menu->u.campaign.is_two_player = 0;
	return menu;
}

menu_t *MenuCreateCampaignItem(
	campaign_entry_t *entry, int is_two_player);

menu_t *MenuCreateCampaigns(
	const char *name,
	const char *title,
	campaign_list_t *list,
	int is_two_player)
{
	menu_t *menu = MenuCreateNormal(
		name,
		title,
		MENU_TYPE_CAMPAIGNS,
		0);
	int i;
	for (i = 0; i < list->numSubFolders; i++)
	{
		char folderName[CDOGS_FILENAME_MAX];
		sprintf(folderName, "%s/", list->subFolders[i].name);
		MenuAddSubmenu(
			menu,
			MenuCreateCampaigns(
				folderName,
				title,
				&list->subFolders[i],
				is_two_player));
	}
	for (i = 0; i < list->num; i++)
	{
		MenuAddSubmenu(menu, MenuCreateCampaignItem(
			&list->list[i], is_two_player));
	}
	return menu;
}

menu_t *MenuCreateCampaignItem(
	campaign_entry_t *entry, int is_two_player)
{
	menu_t *menu = MenuCreate(entry->info, MENU_TYPE_CAMPAIGN_ITEM);
	memcpy(
		&menu->u.campaign.campaignEntry,
		entry,
		sizeof(menu->u.campaign.campaignEntry));
	menu->u.campaign.is_two_player = is_two_player;
	return menu;
}

menu_t *MenuCreateOptions(const char *name)
{
	menu_t *menu = MenuCreateNormal(
		name,
		"Game Options:",
		MENU_TYPE_OPTIONS,
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
			MENU_OPTION_DISPLAY_STYLE_INT_TO_STR_FUNC, (void (*)(void))DifficultyStr));
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
			MENU_OPTION_DISPLAY_STYLE_INT_TO_STR_FUNC, (void (*)(void))PercentStr));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionRange(
			"Non-player HP",
			&gOptions.npcHp,
			0, 200, 25,
			MENU_OPTION_DISPLAY_STYLE_INT_TO_STR_FUNC, (void (*)(void))PercentStr));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionRange(
			"Player HP",
			&gOptions.playerHp,
			0, 200, 25,
			MENU_OPTION_DISPLAY_STYLE_INT_TO_STR_FUNC, (void (*)(void))PercentStr));
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
			MENU_OPTION_DISPLAY_STYLE_INT_TO_STR_FUNC, (void (*)(void))ScaleStr));
	MenuAddSubmenu(menu, MenuCreateSeparator(""));
	MenuAddSubmenu(menu, MenuCreateBack("Done"));
	return menu;
}

menu_t *MenuCreateOptionChangeControl(
	const char *name, input_device_e *device0, input_device_e *device1);
menu_t *MenuCreateKeys(const char *name);

menu_t *MenuCreateControls(const char *name)
{
	menu_t *menu = MenuCreateNormal(
		name,
		"Configure Controls:",
		MENU_TYPE_OPTIONS,
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
			"Reset joysticks",
			GJoyReset,
			NULL, MENU_OPTION_DISPLAY_STYLE_NONE));
	MenuAddSubmenu(menu, MenuCreateSeparator(""));
	MenuAddSubmenu(menu, MenuCreateBack("Done"));
	return menu;
}

menu_t *MenuCreateSound(const char *name)
{
	menu_t *menu = MenuCreateNormal(
		name,
		"Configure Sound:",
		MENU_TYPE_OPTIONS,
		0);
	MenuAddSubmenu(
		menu,
		MenuCreateOptionRangeGetSet(
			"Sound effects",
			SoundGetVolume, SoundSetVolume,
			8, 64, 8,
			MENU_OPTION_DISPLAY_STYLE_INT_TO_STR_FUNC, (void (*)(void))Div8Str));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionRangeGetSet(
			"Music",
			MusicGetVolume, MusicSetVolume,
			8, 64, 8,
			MENU_OPTION_DISPLAY_STYLE_INT_TO_STR_FUNC, (void (*)(void))Div8Str));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionRangeGetSet(
			"FX channels",
			SoundGetChannels, SoundSetChannels,
			2, 8, 1,
			MENU_OPTION_DISPLAY_STYLE_INT, NULL));
	MenuAddSubmenu(menu, MenuCreateSeparator(""));
	MenuAddSubmenu(menu, MenuCreateBack("Done"));
	return menu;
}

menu_t *MenuCreateQuit(const char *name)
{
	return MenuCreate(name, MENU_TYPE_QUIT);
}


menu_t *MenuCreateOptionToggle(
	const char *name, int *config, menu_option_display_style_e style)
{
	menu_t *menu = MenuCreate(name, MENU_TYPE_SET_OPTION_TOGGLE);
	menu->u.option.uHook.optionToggle = config;
	menu->u.option.displayStyle = style;
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
	menu_t *menu = MenuCreateNormal(
		name,
		"",
		MENU_TYPE_KEYS,
		0);
	MenuCreateKeysSingleSection(
		menu, "Player 1", &gPlayer1Data.keys, &gPlayer2Data.keys);
	MenuCreateKeysSingleSection(
		menu, "Player 2", &gPlayer2Data.keys, &gPlayer1Data.keys);
	MenuAddSubmenu(
		menu,
		MenuCreateOptionChangeKey(
			"Map", KEY_CODE_MAP, &gPlayer1Data.keys, &gPlayer2Data.keys));
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
	menu_t *menu = MenuCreate(name, MENU_TYPE_SET_OPTION_CHANGE_KEY);
	menu->u.changeKey.code = code;
	menu->u.changeKey.keys = keys;
	menu->u.changeKey.keysOther = keysOther;
	return menu;
}
