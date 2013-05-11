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


static CampaignSetting customSetting = {
/*	.title		=*/	"",
/*	.author		=*/	"",
/*	.description	=*/	"",
/*	.missionCount	=*/	0,
/*	.missions	=*/	NULL,
/*	.characterCount	=*/	0,
/*	.characters	=*/	NULL,
/*	.path =*/	""
};


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

int KeyAvailable(int key, int code, input_keys_t *keys, input_keys_t *keysOther)
{
	key_code_e i;

	if (key == keyEsc || key == keyF9 || key == keyF10)
		return 0;

	if (key == gOptions.mapKey && code >= 0)
		return 0;

	for (i = 0; i < KEY_CODE_MAP; i++)
		if ((int)i != code && InputGetKey(keys, i) == key)
			return 0;

	if (keysOther->left == key ||
		keysOther->right == key ||
		keysOther->up == key ||
		keysOther->down == key ||
		keysOther->button1 == key ||
		keysOther->button2 == key)
	{
		return 0;
	}

	return 1;
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

int MenuTypeHasSubMenus(menu_type_e type)
{
	return
		type == MENU_TYPE_NORMAL ||
		type == MENU_TYPE_OPTIONS ||
		type == MENU_TYPE_CAMPAIGNS ||
		type == MENU_TYPE_KEYS;
}

int MenuTypeLeftRightMoves(menu_type_e type)
{
	return
		type == MENU_TYPE_NORMAL ||
		type == MENU_TYPE_CAMPAIGNS ||
		type == MENU_TYPE_KEYS;
}

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
	MENU_SET_OPTIONS_TWOPLAYERS	= 0x01,
	MENU_SET_OPTIONS_DOGFIGHT	= 0x02
} menu_set_options_e;

typedef struct menu
{
	char name[64];
	menu_type_e type;
	struct menu *parentMenu;
	union
	{
		// normal menu, with sub menus
		struct
		{
			char title[64];
			struct menu *subMenus;
			int numSubMenus;
			int index;
			int scroll;
			int quitMenuIndex;
			int displayItems;
			int setOptions;
			struct menu *changeKeyMenu;	// if in change key mode, and which item
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
void MenuProcessChangeKey(menu_t *menu, int *cmd, int *prevCmd);

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
		CopyToScreen();
		if (menu->type == MENU_TYPE_KEYS && menu->u.normal.changeKeyMenu != NULL)
		{
			MenuProcessChangeKey(menu, &cmd, &prev);
		}
		else
		{
			GetMenuCmd(&cmd, &prev);
			menu = MenuProcessCmd(menu, cmd);
		}
		SDL_Delay(10);
	} while (menu->type != MENU_TYPE_QUIT && menu->type != MENU_TYPE_CAMPAIGN_ITEM);
	doPlay = menu->type == MENU_TYPE_CAMPAIGN_ITEM;

	MenuDestroy(mainMenu);
	WaitForRelease();
	return doPlay;
}

menu_t *MenuCreateNormal(
	const char *name,
	const char *title,
	menu_type_e type,
	int displayItems,
	int setOptions);
void MenuAddSubmenu(menu_t *menu, menu_t *subMenu);
menu_t *MenuCreateCampaigns(
	const char *name, const char *title, campaign_list_t *list, int options);
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
		MENU_DISPLAY_ITEMS_CREDITS | MENU_DISPLAY_ITEMS_AUTHORS,
		0);
	MenuAddSubmenu(
		menu,
		MenuCreateCampaigns(
			"1 player", "Select a campaign:", &campaigns->campaignList, 0));
	MenuAddSubmenu(
		menu,
		MenuCreateCampaigns(
			"2 players",
			"Select a campaign:",
			&campaigns->campaignList,
			MENU_SET_OPTIONS_TWOPLAYERS));
	MenuAddSubmenu(
		menu,
		MenuCreateCampaigns(
			"Dogfight",
			"Select a dogfight scenario:",
			&campaigns->dogfightList,
			MENU_SET_OPTIONS_DOGFIGHT));
	MenuAddSubmenu(menu, MenuCreateOptions("Game options..."));
	MenuAddSubmenu(menu, MenuCreateControls("Controls..."));
	MenuAddSubmenu(menu, MenuCreateSound("Sound..."));
	MenuAddSubmenu(menu, MenuCreateQuit("Quit"));
	return menu;
}

menu_t *MenuCreate(const char *name, menu_type_e type);

menu_t *MenuCreateNormal(
	const char *name,
	const char *title,
	menu_type_e type,
	int displayItems,
	int setOptions)
{
	menu_t *menu = MenuCreate(name, type);
	strcpy(menu->u.normal.title, title);
	menu->u.normal.displayItems = displayItems;
	menu->u.normal.setOptions = setOptions;
	menu->u.normal.changeKeyMenu = NULL;
	menu->u.normal.index = 0;
	menu->u.normal.scroll = 0;
	menu->u.normal.quitMenuIndex = -1;
	menu->u.normal.subMenus = NULL;
	menu->u.normal.numSubMenus = 0;
	return menu;
}

menu_t *MenuCreate(const char *name, menu_type_e type)
{
	menu_t *menu = sys_mem_alloc(sizeof(menu_t));
	strcpy(menu->name, name);
	menu->type = type;
	menu->parentMenu = NULL;
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
		subMenuLoc->parentMenu = menu;
		if (MenuTypeHasSubMenus(subMenuLoc->type))
		{
			int j;

			for (j = 0; j < subMenuLoc->u.normal.numSubMenus; j++)
			{
				menu_t *subSubMenu = &subMenuLoc->u.normal.subMenus[j];
				subSubMenu->parentMenu = subMenuLoc;
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

menu_t *MenuCreateCampaigns(
	const char *name, const char *title, campaign_list_t *list, int options)
{
	menu_t *menu = MenuCreateNormal(
		name,
		title,
		MENU_TYPE_CAMPAIGNS,
		0,
		options);
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
				options));
	}
	for (i = 0; i < list->num; i++)
	{
		MenuAddSubmenu(menu, MenuCreateCampaignItem(&list->list[i]));
	}
	return menu;
}

menu_t *MenuCreateCampaignItem(campaign_entry_t *entry)
{
	menu_t *menu = MenuCreate(entry->info, MENU_TYPE_CAMPAIGN_ITEM);
	memcpy(&menu->u.campaignEntry, entry, sizeof(menu->u.campaignEntry));
	return menu;
}

menu_t *MenuCreateOptionToggle(
	const char *name, int *config, menu_option_display_style_e style);
menu_t *MenuCreateOptionRange(
	const char *name,
	int *config,
	int low, int high, int increment,
	menu_option_display_style_e style, void (*func)(void));
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
	menu_option_display_style_e style, void (*func)(void));
menu_t *MenuCreateSeparator(const char *name);
menu_t *MenuCreateBack(const char *name);

menu_t *MenuCreateOptions(const char *name)
{
	menu_t *menu = MenuCreateNormal(
		name,
		"Game Options:",
		MENU_TYPE_OPTIONS,
		0, 0);
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
	menu_t *menu = MenuCreateNormal(
		name,
		"Configure Sound:",
		MENU_TYPE_OPTIONS,
		0, 0);
	MenuAddSubmenu(
		menu,
		MenuCreateOptionRangeGetSet(
			"Sound effects",
			FXVolume, SetFXVolume,
			8, 64, 8,
			MENU_OPTION_DISPLAY_STYLE_INT_TO_STR_FUNC, (void (*)(void))Div8Str));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionRangeGetSet(
			"Music",
			MusicVolume, SetMusicVolume,
			8, 64, 8,
			MENU_OPTION_DISPLAY_STYLE_INT_TO_STR_FUNC, (void (*)(void))Div8Str));
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

menu_t *MenuCreateOptionRange(
	const char *name,
	int *config,
	int low, int high, int increment,
	menu_option_display_style_e style, void (*func)(void))
{
	menu_t *menu = MenuCreate(name, MENU_TYPE_SET_OPTION_RANGE);
	menu->u.option.uHook.optionRange.option = config;
	menu->u.option.uHook.optionRange.low = low;
	menu->u.option.uHook.optionRange.high = high;
	menu->u.option.uHook.optionRange.increment = increment;
	menu->u.option.displayStyle = style;
	if (style == MENU_OPTION_DISPLAY_STYLE_STR_FUNC)
	{
		menu->u.option.uFunc.str = (char *(*)(void))func;
	}
	else if (style == MENU_OPTION_DISPLAY_STYLE_INT_TO_STR_FUNC)
	{
		menu->u.option.uFunc.intToStr = (char *(*)(int))func;
	}
	return menu;
}

menu_t *MenuCreateOptionSeed(const char *name, unsigned int *seed)
{
	menu_t *menu = MenuCreate(name, MENU_TYPE_SET_OPTION_SEED);
	menu->u.option.uHook.seed = seed;
	menu->u.option.displayStyle = MENU_OPTION_DISPLAY_STYLE_INT;
	return menu;
}

menu_t *MenuCreateOptionUpDownFunc(
	const char *name,
	void(*upFunc)(void), void(*downFunc)(void),
	menu_option_display_style_e style, char *(*strFunc)(void))
{
	menu_t *menu = MenuCreate(name, MENU_TYPE_SET_OPTION_UP_DOWN_VOID_FUNC_VOID);
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
	menu_t *menu = MenuCreate(name, MENU_TYPE_VOID_FUNC_VOID);
	menu->u.option.uHook.toggleFuncs.toggle = toggleFunc;
	menu->u.option.uHook.toggleFuncs.get = getFunc;
	menu->u.option.displayStyle = style;
	return menu;
}

menu_t *MenuCreateOptionRangeGetSet(
	const char *name,
	int(*getFunc)(void), void(*setFunc)(int),
	int low, int high, int increment,
	menu_option_display_style_e style, void (*func)(void))
{
	menu_t *menu = MenuCreate(name, MENU_TYPE_SET_OPTION_RANGE_GET_SET);
	menu->u.option.uHook.optionRangeGetSet.getFunc = getFunc;
	menu->u.option.uHook.optionRangeGetSet.setFunc = setFunc;
	menu->u.option.uHook.optionRangeGetSet.low = low;
	menu->u.option.uHook.optionRangeGetSet.high = high;
	menu->u.option.uHook.optionRangeGetSet.increment = increment;
	menu->u.option.displayStyle = style;
	// TODO: refactor saving of function based on style
	if (style == MENU_OPTION_DISPLAY_STYLE_INT_TO_STR_FUNC)
	{
		menu->u.option.uFunc.intToStr = (char *(*)(int))func;
	}
	return menu;
}

menu_t *MenuCreateSeparator(const char *name)
{
	return MenuCreate(name, MENU_TYPE_SEPARATOR);
}

menu_t *MenuCreateBack(const char *name)
{
	return MenuCreate(name, MENU_TYPE_BACK);
}

menu_t *MenuCreateOptionChangeControl(
	const char *name, input_device_e *device0, input_device_e *device1)
{
	menu_t *menu = MenuCreate(name, MENU_TYPE_SET_OPTION_CHANGE_CONTROL);
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
	menu_t *menu = MenuCreateNormal(
		name,
		"",
		MENU_TYPE_KEYS,
		0, 0);
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
	if (MenuTypeHasSubMenus(menu->type) && menu->u.normal.subMenus != NULL)
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
			int y = CenterY(12 * CDogsTextHeight());

		#define ARROW_UP	"\036"
		#define ARROW_DOWN	"\037"

			if (menu->u.normal.scroll != 0)
			{
				DisplayMenuItem(
					CenterX(CDogsTextWidth(ARROW_UP)),
					y - 2 - CDogsTextHeight(),
					ARROW_UP,
					0);
			}

			for (i = menu->u.normal.scroll;
				i < MIN(menu->u.normal.scroll + 12, menu->u.normal.numSubMenus);
				i++)
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
					const char *keyName;
					if (menu->u.normal.changeKeyMenu == subMenu)
					{
						keyName = "Press a key";
					}
					else if (subMenu->u.changeKey.code == KEY_CODE_MAP)
					{
						keyName = SDL_GetKeyName(gOptions.mapKey);
					}
					else
					{
						keyName = SDL_GetKeyName(InputGetKey(
							subMenu->u.changeKey.keys,
							subMenu->u.changeKey.code));
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
	else
	{
		menuToChange = menu->parentMenu;
	}
	return menuToChange;
}

void MenuSetOptions(int setOptions);
void MenuLoadCampaign(campaign_entry_t *entry);
void MenuActivate(menu_t *menu, int cmd);

menu_t *MenuProcessButtonCmd(menu_t *menu, int cmd)
{
	if (AnyButton(cmd) ||
		(!MenuTypeLeftRightMoves(menu->type) && (Left(cmd) || Right(cmd))))
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
			return menu->parentMenu;
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
		if (customSetting.missions)
		{
			sys_mem_free(customSetting.missions);
		}
		if (customSetting.characters)
		{
			sys_mem_free(customSetting.characters);
		}
		memset(&customSetting, 0, sizeof(customSetting));

		if (LoadCampaign(entry->path, &customSetting, 0, 0) != CAMPAIGN_OK)
		{
			printf("Failed to load campaign %s!\n", entry->path);
			assert(0);
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
		menu->parentMenu->u.normal.changeKeyMenu = menu;
		break;
	default:
		printf("Error unhandled menu type %d\n", menu->type);
		assert(0);
		break;
	}
}

void MenuChangeIndex(menu_t *menu, int cmd)
{
	int leftRightMoves = MenuTypeLeftRightMoves(menu->type);
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
	menu->u.normal.scroll =
		CLAMP(menu->u.normal.scroll,
			MAX(0, menu->u.normal.index - 11),
			MIN(menu->u.normal.numSubMenus - 1, menu->u.normal.index + 11));
	if (menu->u.normal.index < menu->u.normal.scroll)
	{
		menu->u.normal.scroll = menu->u.normal.index;
	}
}

void MenuProcessChangeKey(menu_t *menu, int *cmd, int *prevCmd)
{
	int key;
	int prevKey = GetKeyDown();
	do
	{
		key = GetKeyDown();
		if (key == 0)
		{
			prevKey = 0;
		}
	}
	while (key == 0 || key == prevKey);	// wait until user has pressed a new button

	if (key == keyEsc)
	{
		PlaySound(SND_PICKUP, 0, 255);
	}
	else if (KeyAvailable(
		key,
		menu->u.normal.changeKeyMenu->u.changeKey.code,
		menu->u.normal.changeKeyMenu->u.changeKey.keys,
		menu->u.normal.changeKeyMenu->u.changeKey.keysOther))
	{
		if (menu->u.normal.changeKeyMenu->u.changeKey.code != KEY_CODE_MAP)
		{
			InputSetKey(
				menu->u.normal.changeKeyMenu->u.changeKey.keys,
				key,
				menu->u.normal.changeKeyMenu->u.changeKey.code);
		}
		else
		{
			gOptions.mapKey = key;
		}
		PlaySound(SND_EXPLOSION, 0, 255);
	}
	else
	{
		PlaySound(SND_KILL4, 0, 255);
	}
	menu->u.normal.changeKeyMenu = NULL;

	// set the current command to what the user pressed, to prevent change key entering loop
	// TODO: refactor keyboard input routines, combine cmd/prevcmd
	*prevCmd = 0;
	GetMenuCmd(cmd, prevCmd);
	*prevCmd = *cmd;
}
