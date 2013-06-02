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
#include "menu.h"

#include <assert.h>
#include <limits.h>
#include <string.h>

#include <SDL.h>

#include <cdogs/actors.h>
#include <cdogs/blit.h>
#include <cdogs/defs.h>
#include <cdogs/events.h>
#include <cdogs/files.h>
#include <cdogs/gamedata.h>
#include <cdogs/joystick.h>
#include <cdogs/mission.h>
#include <cdogs/pics.h>
#include <cdogs/sounds.h>
#include <cdogs/text.h>
#include <cdogs/utils.h>


void ShowControls(void)
{
	CDogsTextStringSpecial("(use player 1 controls or arrow keys + Enter/Backspace)", TEXT_BOTTOM | TEXT_XCENTER, 0, 10);
}

void DisplayMenuItem(int x, int y, const char *s, int selected)
{
	if (selected)
	{
		CDogsTextStringWithTableAt(x, y, s, &tableFlamed);
	}
	else
	{
		CDogsTextStringAt(x, y, s);
	}
}


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


menu_t *MenuCreate(const char *name, menu_type_e type)
{
	menu_t *menu;
	CMALLOC(menu, sizeof(menu_t));
	strcpy(menu->name, name);
	menu->type = type;
	menu->parentMenu = NULL;
	return menu;
}

menu_t *MenuCreateNormal(
	const char *name,
	const char *title,
	menu_type_e type,
	int displayItems)
{
	menu_t *menu = MenuCreate(name, type);
	strcpy(menu->u.normal.title, title);
	menu->u.normal.displayItems = displayItems;
	menu->u.normal.changeKeyMenu = NULL;
	menu->u.normal.index = 0;
	menu->u.normal.scroll = 0;
	menu->u.normal.quitMenuIndex = -1;
	menu->u.normal.subMenus = NULL;
	menu->u.normal.numSubMenus = 0;
	return menu;
}

void MenuAddSubmenu(menu_t *menu, menu_t *subMenu)
{
	menu_t *subMenuLoc = NULL;
	int i;

	menu->u.normal.numSubMenus++;
	CREALLOC(menu->u.normal.subMenus, menu->u.normal.numSubMenus*sizeof(menu_t));
	subMenuLoc = &menu->u.normal.subMenus[menu->u.normal.numSubMenus - 1];
	memcpy(subMenuLoc, subMenu, sizeof(menu_t));
	if (subMenu->type == MENU_TYPE_QUIT)
	{
		menu->u.normal.quitMenuIndex = menu->u.normal.numSubMenus - 1;
	}
	CFREE(subMenu);

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
					const char *filename = subMenu->u.campaign.campaignEntry.filename;
					int isBuiltin = subMenu->u.campaign.campaignEntry.isBuiltin;
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


void MenuDestroySubmenus(menu_t *menu);

void MenuDestroy(menu_t *menu)
{
	if (menu == NULL)
	{
		return;
	}
	MenuDestroySubmenus(menu);
	CFREE(menu);
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
		CFREE(menu->u.normal.subMenus);
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

menu_t *MenuProcessCmd(menu_t *menu, int cmd)
{
	menu_t *menuToChange = NULL;
	if (cmd == CMD_ESC)
	{
		menuToChange = MenuProcessEscCmd(menu);
		if (menuToChange != NULL)
		{
			SoundPlay(&gSoundDevice, SND_PICKUP);
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
			SoundPlay(&gSoundDevice, SND_HAHAHA);
		}
		else
		{
			SoundPlay(&gSoundDevice, SND_MACHINEGUN);
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
			SoundPlay(&gSoundDevice, SND_DOOR);
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

void MenuLoadCampaign(
	campaign_entry_t *entry, int is_two_player)
{
	gOptions.twoPlayers = is_two_player;
	gCampaign.mode = entry->mode;
	if (entry->isBuiltin)
	{
		if (entry->mode == CAMPAIGN_MODE_NORMAL)
		{
			SetupBuiltinCampaign(entry->builtinIndex);
		}
		else if (entry->mode == CAMPAIGN_MODE_DOGFIGHT)
		{
			SetupBuiltinDogfight(entry->builtinIndex);
		}
		else
		{
			gCampaign.setting = SetupAndGetQuickPlay();
		}
	}
	else
	{
		if (customSetting.missions)
		{
			CFREE(customSetting.missions);
		}
		if (customSetting.characters)
		{
			CFREE(customSetting.characters);
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

menu_t *MenuProcessButtonCmd(menu_t *menu, int cmd)
{
	if (AnyButton(cmd) ||
		(!MenuTypeLeftRightMoves(menu->type) && (Left(cmd) || Right(cmd))))
	{
		menu_t *subMenu = &menu->u.normal.subMenus[menu->u.normal.index];
		switch (subMenu->type)
		{
		case MENU_TYPE_NORMAL:
		case MENU_TYPE_OPTIONS:
		case MENU_TYPE_CAMPAIGNS:
		case MENU_TYPE_KEYS:
			return subMenu;
		case MENU_TYPE_CAMPAIGN_ITEM:
			MenuLoadCampaign(
				&subMenu->u.campaign.campaignEntry,
				subMenu->u.campaign.is_two_player);
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

void MenuProcessChangeKey(menu_t *menu)
{
	int key = GetKey(&gKeyboard);	// wait until user has pressed a new button

	if (key == keyEsc)
	{
		SoundPlay(&gSoundDevice, SND_PICKUP);
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
		SoundPlay(&gSoundDevice, SND_EXPLOSION);
	}
	else
	{
		SoundPlay(&gSoundDevice, SND_KILL4);
	}
	menu->u.normal.changeKeyMenu = NULL;
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
		SoundPlay(&gSoundDevice, SND_DOOR);
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
		SoundPlay(&gSoundDevice, SND_DOOR);
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


// TODO: simplify into an iterate over struct controls_available
void ChangeControl(
	input_device_e *d, input_device_e *dOther, int numJoysticks)
{
	if (*d == INPUT_DEVICE_JOYSTICK_1)
	{
		if (*dOther != INPUT_DEVICE_JOYSTICK_2 && numJoysticks >= 2)
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
		if (*dOther != INPUT_DEVICE_JOYSTICK_1 && numJoysticks >= 1)
		{
			*d = INPUT_DEVICE_JOYSTICK_1;
		}
		else if (numJoysticks >= 2)
		{
			*d = INPUT_DEVICE_JOYSTICK_2;
		}
	}
	debug(D_NORMAL, "change control to: %s\n", InputDeviceStr(*d));
}

void MenuActivate(menu_t *menu, int cmd)
{
	SoundPlay(&gSoundDevice, SND_SWITCH);
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
			gJoysticks.numJoys);
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
