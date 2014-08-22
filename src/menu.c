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
#include "menu.h"

#include <assert.h>
#include <limits.h>
#include <stdarg.h>
#include <string.h>

#include <SDL.h>

#include <cdogs/actors.h>
#include <cdogs/blit.h>
#include <cdogs/config.h>
#include <cdogs/defs.h>
#include <cdogs/events.h>
#include <cdogs/files.h>
#include <cdogs/font.h>
#include <cdogs/gamedata.h>
#include <cdogs/grafx_bg.h>
#include <cdogs/mission.h>
#include <cdogs/music.h>
#include <cdogs/pic_manager.h>
#include <cdogs/sounds.h>
#include <cdogs/utils.h>

#include "autosave.h"


#define MS_CENTER_X(ms, w) CENTER_X((ms).pos, (ms).size, w)
#define MS_CENTER_Y(ms, h) CENTER_Y((ms).pos, (ms).size, h)

void MenuSystemInit(
	MenuSystem *ms,
	EventHandlers *handlers, GraphicsDevice *graphics, Vec2i pos, Vec2i size)
{
	memset(ms, 0, sizeof *ms);
	ms->root = ms->current = NULL;
	CArrayInit(&ms->exitTypes, sizeof(menu_type_e));
	CArrayInit(&ms->customDisplayFuncs, sizeof(MenuCustomDisplayFunc));
	ms->handlers = handlers;
	ms->graphics = graphics;
	ms->pos = pos;
	ms->size = size;
	ms->align = MENU_ALIGN_CENTER;
}

void MenuDestroySubmenus(menu_t *menu);

void MenuSystemTerminate(MenuSystem *ms)
{
	MenuDestroySubmenus(ms->root);
	CFREE(ms->root);
	CArrayTerminate(&ms->exitTypes);
	CArrayTerminate(&ms->customDisplayFuncs);
	memset(ms, 0, sizeof *ms);
}

void MenuSetCreditsDisplayer(MenuSystem *menu, credits_displayer_t *creditsDisplayer)
{
	menu->creditsDisplayer = creditsDisplayer;
}

int MenuHasExitType(MenuSystem *menu, menu_type_e exitType)
{
	for (int i = 0; i < (int)menu->exitTypes.size; i++)
	{
		if (*(menu_type_e *)CArrayGet(&menu->exitTypes, i) == exitType)
		{
			return 1;
		}
	}
	return 0;
}

void MenuAddExitType(MenuSystem *menu, menu_type_e exitType)
{
	if (MenuHasExitType(menu, exitType))
	{
		return;
	}
	CArrayPushBack(&menu->exitTypes, &exitType);
}

void MenuSystemAddCustomDisplay(
	MenuSystem *ms, MenuDisplayFunc func, void *data)
{
	MenuCustomDisplayFunc cdf;
	cdf.Func = func;
	cdf.Data = data;
	CArrayPushBack(&ms->customDisplayFuncs, &cdf);
}

int MenuIsExit(MenuSystem *ms)
{
	return MenuHasExitType(ms, ms->current->type);
}

void MenuProcessChangeKey(menu_t *menu);

static GameLoopResult MenuUpdate(void *data);
static void MenuDraw(const void *data);
void MenuLoop(MenuSystem *menu)
{
	CASSERT(menu->exitTypes.size > 0, "menu has no exit types");
	GameLoopData gData = GameLoopDataNew(
		menu, MenuUpdate, menu, MenuDraw);
	GameLoop(&gData);
}
static GameLoopResult MenuUpdate(void *data)
{
	MenuSystem *ms = data;
	if (ms->current->type == MENU_TYPE_KEYS &&
		ms->current->u.normal.changeKeyMenu != NULL)
	{
		MenuProcessChangeKey(ms->current);
	}
	else
	{
		const int cmd = GetMenuCmd(ms->handlers, gPlayerDatas);
		if (cmd)
		{
			MenuProcessCmd(ms, cmd);
		}
	}
	if (MenuIsExit(ms) || ms->handlers->HasQuit)
	{
		return UPDATE_RESULT_EXIT;
	}
	return UPDATE_RESULT_DRAW;
}
static void MenuDraw(const void *data)
{
	const MenuSystem *ms = data;
	GraphicsBlitBkg(ms->graphics);
	ShowControls();
	MenuDisplay(ms);
	BlitFlip(ms->graphics, &gConfig.Graphics);
}

void MenuReset(MenuSystem *menu)
{
	menu->current = menu->root;
}

static void MoveIndexToNextEnabledSubmenu(menu_t *menu, int isDown)
{
	int firstIndex = menu->u.normal.index;
	int isFirst = 1;
	// Move the selection to the next non-disabled submenu
	for (;;)
	{
		menu_t *currentSubmenu =
			CArrayGet(&menu->u.normal.subMenus, menu->u.normal.index);
		if (!currentSubmenu->isDisabled)
		{
			break;
		}
		if (menu->u.normal.index == firstIndex && !isFirst)
		{
			break;
		}
		isFirst = 0;
		if (isDown)
		{
			menu->u.normal.index++;
			if (menu->u.normal.index == (int)menu->u.normal.subMenus.size)
			{
				menu->u.normal.index = 0;
			}
		}
		else
		{
			menu->u.normal.index--;
			if (menu->u.normal.index == -1)
			{
				menu->u.normal.index = (int)menu->u.normal.subMenus.size - 1;
			}
		}
	}
}

void MenuDisableSubmenu(menu_t *menu, int idx)
{
	menu_t *subMenu = CArrayGet(&menu->u.normal.subMenus, idx);
	subMenu->isDisabled = true;
	MoveIndexToNextEnabledSubmenu(menu, 1);
}
void MenuEnableSubmenu(menu_t *menu, int idx)
{
	menu_t *subMenu = CArrayGet(&menu->u.normal.subMenus, idx);
	subMenu->isDisabled = false;
}

menu_t *MenuGetSubmenuByName(menu_t *menu, const char *name)
{
	for (int i = 0; i < (int)menu->u.normal.subMenus.size; i++)
	{
		menu_t *subMenu = CArrayGet(&menu->u.normal.subMenus, i);
		if (strcmp(subMenu->name, name) == 0)
		{
			return subMenu;
		}
	}
	return NULL;
}

void ShowControls(void)
{
	FontOpts opts = FontOptsNew();
	opts.HAlign = ALIGN_CENTER;
	opts.VAlign = ALIGN_END;
	opts.Area = gGraphicsDevice.cachedConfig.Res;
	opts.Pad.y = 10;
	FontStrOpt(
		"(use joystick 1 or arrow keys + Enter/Backspace)", Vec2iZero(), opts);
}

void DisplayMenuItem(
	Vec2i pos, const char *s, int selected, int isDisabled, color_t color)
{
	if (selected)
	{
		FontStrMask(s, pos, colorRed);
	}
	else if (isDisabled)
	{
		color_t dark = { 64, 64, 64, 255 };
		FontStrMask(s, pos, dark);
	}
	else if (!ColorEquals(color, colorBlack))
	{
		FontStrMask(s, pos, color);
	}
	else
	{
		FontStr(s, pos);
	}
}


int MenuTypeHasSubMenus(menu_type_e type)
{
	return
		type == MENU_TYPE_NORMAL ||
		type == MENU_TYPE_OPTIONS ||
		type == MENU_TYPE_KEYS;
}

int MenuTypeLeftRightMoves(menu_type_e type)
{
	return
		type == MENU_TYPE_NORMAL ||
		type == MENU_TYPE_KEYS;
}


menu_t *MenuCreate(const char *name, menu_type_e type)
{
	menu_t *menu;
	CCALLOC(menu, sizeof(menu_t));
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
	menu->u.normal.maxItems = 0;
	menu->u.normal.align = MENU_ALIGN_LEFT;
	menu->u.normal.quitMenuIndex = -1;
	CArrayInit(&menu->u.normal.subMenus, sizeof(menu_t));
	return menu;
}

void MenuAddSubmenu(menu_t *menu, menu_t *subMenu)
{
	CArrayPushBack(&menu->u.normal.subMenus, subMenu);
	if (subMenu->type == MENU_TYPE_QUIT)
	{
		menu->u.normal.quitMenuIndex = (int)menu->u.normal.subMenus.size - 1;
	}
	CFREE(subMenu);

	// update all parent pointers, in grandchild menus
	for (int i = 0; i < (int)menu->u.normal.subMenus.size; i++)
	{
		menu_t *subMenuLoc = CArrayGet(&menu->u.normal.subMenus, i);
		subMenuLoc->parentMenu = menu;
		if (MenuTypeHasSubMenus(subMenuLoc->type))
		{
			for (int j = 0; j < (int)subMenuLoc->u.normal.subMenus.size; j++)
			{
				menu_t *subSubMenu =
					CArrayGet(&subMenuLoc->u.normal.subMenus, j);
				subSubMenu->parentMenu = subMenuLoc;
			}
		}
	}

	// move cursor in case first menu item(s) are disabled
	MoveIndexToNextEnabledSubmenu(menu, 1);
}

void MenuSetPostInputFunc(menu_t *menu, MenuPostInputFunc func, void *data)
{
	menu->customPostInputFunc = func;
	menu->customPostInputData = data;
}

void MenuSetPostEnterFunc(menu_t *menu, MenuFunc func, void *data)
{
	menu->customPostEnterFunc = func;
	menu->customPostEnterData = data;
}

void MenuSetCustomDisplay(menu_t *menu, MenuDisplayFunc func, void *data)
{
	menu->customDisplayFunc = func;
	menu->customDisplayData = data;
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
		menu->u.option.uFunc.intToStr = (const char *(*)(int))func;
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

menu_t *MenuCreateVoidFunc(
	const char *name, void (*func)(void *), void *data)
{
	menu_t *menu = MenuCreate(name, MENU_TYPE_VOID_FUNC);
	menu->u.option.uHook.voidFunc.func = func;
	menu->u.option.uHook.voidFunc.data = data;
	menu->u.option.displayStyle = MENU_OPTION_DISPLAY_STYLE_NONE;
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
		menu->u.option.uFunc.intToStr = (const char *(*)(int))func;
	}
	return menu;
}

menu_t *MenuCreateSeparator(const char *name)
{
	menu_t *menu = MenuCreate(name, MENU_TYPE_BASIC);
	menu->isDisabled = 1;
	return menu;
}

menu_t *MenuCreateBack(const char *name)
{
	return MenuCreate(name, MENU_TYPE_BACK);
}

menu_t *MenuCreateReturn(const char *name, int returnCode)
{
	menu_t *menu = MenuCreate(name, MENU_TYPE_RETURN);
	menu->u.returnCode = returnCode;
	return menu;
}

menu_t *MenuCreateCustom(
	const char *name,
	MenuDisplayFunc displayFunc, MenuInputFunc inputFunc,
	void *data)
{
	menu_t *menu = MenuCreate(name, MENU_TYPE_CUSTOM);
	menu->u.customData.displayFunc = displayFunc;
	menu->u.customData.inputFunc = inputFunc;
	menu->u.customData.data = data;
	return menu;
}


static void MenuDisplayItems(const MenuSystem *ms);
static void MenuDisplaySubmenus(const MenuSystem *ms);
void MenuDisplay(const MenuSystem *ms)
{
	const menu_t *menu = ms->current;
	if (menu->type == MENU_TYPE_CUSTOM)
	{
		menu->u.customData.displayFunc(
			menu, ms->graphics, ms->pos, ms->size, menu->u.customData.data);
	}
	else
	{
		MenuDisplayItems(ms);

		if (strlen(menu->u.normal.title) != 0)
		{
			FontOpts opts = FontOptsNew();
			opts.HAlign = ALIGN_CENTER;
			opts.Area = ms->size;
			opts.Pad = Vec2iNew(20, 20);
			FontStrOpt(menu->u.normal.title, ms->pos, opts);
		}

		MenuDisplaySubmenus(ms);
	}
	for (int i = 0; i < (int)ms->customDisplayFuncs.size; i++)
	{
		MenuCustomDisplayFunc *cdf = CArrayGet(&ms->customDisplayFuncs, i);
		cdf->Func(NULL, ms->graphics, ms->pos, ms->size, cdf->Data);
	}
	if (menu->customDisplayFunc)
	{
		menu->customDisplayFunc(
			menu, ms->graphics, ms->pos, ms->size, menu->customDisplayData);
	}
}
static void MenuDisplayItems(const MenuSystem *ms)
{
	int d = ms->current->u.normal.displayItems;
	if ((d & MENU_DISPLAY_ITEMS_CREDITS) && ms->creditsDisplayer != NULL)
	{
		ShowCredits(ms->creditsDisplayer);
	}
	if (d & MENU_DISPLAY_ITEMS_AUTHORS)
	{
		PicPaletted *logo = PicManagerGetOldPic(&gPicManager, PIC_LOGO);
		DrawTPic(
			MS_CENTER_X(*ms, logo->w),
			ms->pos.y + ms->size.y / 12,
			logo);

		FontOpts opts = FontOptsNew();
		opts.HAlign = ALIGN_END;
		opts.Area = ms->size;
		opts.Pad = Vec2iNew(20, 20);
		FontStrOpt("Version: " CDOGS_SDL_VERSION, ms->pos, opts);
	}
}
static int MenuOptionGetIntValue(const menu_t *menu);
static void MenuDisplaySubmenus(const MenuSystem *ms)
{
	int x = 0, yStart = 0;
	int maxWidth = 0;
	const menu_t *menu = ms->current;

	switch (menu->type)
	{
	// TODO: refactor the three menu types (normal, options, campaign) into one
	case MENU_TYPE_NORMAL:
	case MENU_TYPE_OPTIONS:
		{
			int isCentered = menu->type == MENU_TYPE_NORMAL;
			int xOptions;
			int iStart = 0;
			int iEnd = (int)menu->u.normal.subMenus.size;
			for (int i = 0; i < iEnd; i++)
			{
				const menu_t *subMenu = CArrayGet(&menu->u.normal.subMenus, i);
				const int width = FontStrW(subMenu->name);
				if (width > maxWidth)
				{
					maxWidth = width;
				}
			}
			switch (ms->align)
			{
			case MENU_ALIGN_CENTER:
				x = MS_CENTER_X(*ms, maxWidth);
				if (!isCentered)
				{
					x -= 20;
				}
				break;
			case MENU_ALIGN_LEFT:
				x = ms->pos.x;
				break;
			default:
				assert(0 && "unknown alignment");
				break;
			}

			if (menu->u.normal.maxItems > 0)
			{
				// Calculate first/last indices
				if (menu->u.normal.scroll != 0)
				{
					iStart = menu->u.normal.scroll;
				}
				iEnd = MIN(
					iStart + menu->u.normal.maxItems,
					(int)menu->u.normal.subMenus.size);
			}

			yStart = MS_CENTER_Y(*ms, (iEnd - iStart) * FontH());
			if (menu->u.normal.maxItems > 0)
			{
				// Display scroll arrows
				if (menu->u.normal.scroll != 0)
				{
					DisplayMenuItem(
						Vec2iNew(
							MS_CENTER_X(*ms, FontW('^')),
							yStart - 2 - FontH()),
						"^",
						0, 0,
						colorBlack);
				}
				if (iEnd < (int)menu->u.normal.subMenus.size - 1)
				{
					DisplayMenuItem(
						Vec2iNew(
							MS_CENTER_X(*ms, FontW('v')),
							yStart + menu->u.normal.maxItems*FontH() + 2),
						"v",
						0, 0,
						colorBlack);
				}
			}
			xOptions = x + maxWidth + 10;

			// Display normal menu items
			for (int i = iStart; i < iEnd; i++)
			{
				int y = yStart + (i - iStart) * FontH();
				const menu_t *subMenu = CArrayGet(&menu->u.normal.subMenus, i);
				Vec2i pos = Vec2iNew(x, y);

				switch (menu->u.normal.align)
				{
				case MENU_ALIGN_CENTER:
					pos.x = MS_CENTER_X(*ms, FontStrW(subMenu->name));
					break;
				case MENU_ALIGN_LEFT:
					// Do nothing
					break;
				default:
					assert(0 && "unknown alignment");
					break;
				}

				DisplayMenuItem(
					pos,
					subMenu->name,
					i == menu->u.normal.index,
					subMenu->isDisabled,
					subMenu->color);

				// display option value
				if (subMenu->type == MENU_TYPE_SET_OPTION_TOGGLE ||
					subMenu->type == MENU_TYPE_SET_OPTION_RANGE ||
					subMenu->type == MENU_TYPE_SET_OPTION_SEED ||
					subMenu->type == MENU_TYPE_SET_OPTION_UP_DOWN_VOID_FUNC_VOID ||
					subMenu->type == MENU_TYPE_SET_OPTION_RANGE_GET_SET)
				{
					const int optionInt = MenuOptionGetIntValue(subMenu);
					const Vec2i value_pos = Vec2iNew(xOptions, y);
					switch (subMenu->u.option.displayStyle)
					{
					case MENU_OPTION_DISPLAY_STYLE_INT:
						{
							char buf[32];
							sprintf(buf, "%d", optionInt);
							FontStr(buf, value_pos);
						}
						break;
					case MENU_OPTION_DISPLAY_STYLE_YES_NO:
						FontStr(optionInt ? "Yes" : "No", value_pos);
						break;
					case MENU_OPTION_DISPLAY_STYLE_ON_OFF:
						FontStr(optionInt ? "On" : "Off", value_pos);
						break;
					case MENU_OPTION_DISPLAY_STYLE_STR_FUNC:
						FontStr(subMenu->u.option.uFunc.str(), value_pos);
						break;
					case MENU_OPTION_DISPLAY_STYLE_INT_TO_STR_FUNC:
						FontStr(
							subMenu->u.option.uFunc.intToStr(optionInt),
                            value_pos);
						break;
					default:
						break;
					}
				}
			}
		}
		break;
	case MENU_TYPE_KEYS:
		{
			int xKeys;
			x = MS_CENTER_X(*ms, (FontW('a') * 10)) / 2;
			xKeys = x * 3;
			yStart = (gGraphicsDevice.cachedConfig.Res.y / 2) - (FontH() * 10);

			for (int i = 0; i < (int)menu->u.normal.subMenus.size; i++)
			{
				int y = yStart + i * FontH();
				int isSelected = i == menu->u.normal.index;
				const menu_t *subMenu = CArrayGet(&menu->u.normal.subMenus, i);

				const char *name = subMenu->name;
				if (isSelected &&
					subMenu->type != MENU_TYPE_SET_OPTION_CHANGE_KEY)
				{
					FontStrMask(name, Vec2iNew(x, y), colorRed);
				}
				else
				{
					FontStr(name, Vec2iNew(x, y));
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
						keyName = SDL_GetKeyName(gConfig.Input.PlayerKeys[0].Keys.map);
					}
					else
					{
						keyName = SDL_GetKeyName(InputGetKey(
							subMenu->u.changeKey.keys,
							subMenu->u.changeKey.code));
					}
					DisplayMenuItem(
						Vec2iNew(xKeys, y),
						keyName,
						isSelected,
						0,
						colorBlack);
				}
			}
		}
		break;
	default:
		// No submenus, don't display anything
		break;
	}
}


void MenuPlaySound(MenuSound s)
{
	switch (s)
	{
	case MENU_SOUND_ENTER:
		SoundPlay(&gSoundDevice, StrSound("mg"));
		break;
	case MENU_SOUND_BACK:
		SoundPlay(&gSoundDevice, StrSound("pickup"));
		break;
	case MENU_SOUND_SWITCH:
		SoundPlay(&gSoundDevice, StrSound("door"));
		break;
	case MENU_SOUND_START:
		SoundPlay(&gSoundDevice, StrSound("hahaha"));
		break;
	case MENU_SOUND_ERROR:
		SoundPlay(&gSoundDevice, SoundGetRandomScream(&gSoundDevice));
		break;
	default:
		break;
	}
}


void MenuDestroy(MenuSystem *menu)
{
	if (menu == NULL || menu->root == NULL)
	{
		return;
	}
	MenuSystemTerminate(menu);
	CFREE(menu);
}

void MenuDestroySubmenus(menu_t *menu)
{
	if (menu == NULL)
	{
		return;
	}
	if (MenuTypeHasSubMenus(menu->type))
	{
		for (int i = 0; i < (int)menu->u.normal.subMenus.size; i++)
		{
			menu_t *subMenu = CArrayGet(&menu->u.normal.subMenus, i);
			MenuDestroySubmenus(subMenu);
		}
		CArrayTerminate(&menu->u.normal.subMenus);
	}
}

static int MenuOptionGetIntValue(const menu_t *menu)
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
	default:
		return 0;
	}
}

// returns menu to change to, NULL if no change
menu_t *MenuProcessEscCmd(menu_t *menu);
menu_t *MenuProcessButtonCmd(MenuSystem *ms, menu_t *menu, int cmd);
void MenuChangeIndex(menu_t *menu, int cmd);

void MenuProcessCmd(MenuSystem *ms, int cmd)
{
	menu_t *menu = ms->current;
	menu_t *menuToChange = NULL;
	if (cmd == CMD_ESC || (cmd & CMD_BUTTON2))
	{
		menuToChange = MenuProcessEscCmd(menu);
		if (menuToChange != NULL)
		{
			MenuPlaySound(MENU_SOUND_BACK);
			ms->current = menuToChange;
			goto bail;
		}
	}
	if (menu->type == MENU_TYPE_CUSTOM)
	{
		if (menu->u.customData.inputFunc(cmd, menu->u.customData.data))
		{
			ms->current = menu->parentMenu;
			goto bail;
		}
	}
	else
	{
		menuToChange = MenuProcessButtonCmd(ms, menu, cmd);
		if (menuToChange != NULL)
		{
			debug(D_VERBOSE, "change to menu type %d\n", menuToChange->type);
			if (menuToChange->type == MENU_TYPE_CAMPAIGN_ITEM)
			{
				MenuPlaySound(MENU_SOUND_START);
			}
			else
			{
				MenuPlaySound(MENU_SOUND_ENTER);
			}
			ms->current = menuToChange;
			goto bail;
		}
		MenuChangeIndex(menu, cmd);
	}

bail:
	if (menu->customPostInputFunc)
	{
		menu->customPostInputFunc(menu, cmd, menu->customPostInputData);
	}
	if (menuToChange && menuToChange->customPostEnterFunc)
	{
		menuToChange->customPostEnterFunc(
			menuToChange, menuToChange->customPostEnterData);
	}
}

menu_t *MenuProcessEscCmd(menu_t *menu)
{
	menu_t *menuToChange = NULL;
	int quitMenuIndex = menu->u.normal.quitMenuIndex;
	if (quitMenuIndex != -1)
	{
		if (menu->u.normal.index != quitMenuIndex)
		{
			MenuPlaySound(MENU_SOUND_SWITCH);
			menu->u.normal.index = quitMenuIndex;
		}
		else if (menu->u.normal.subMenus.size > 0)
		{
			menuToChange = CArrayGet(&menu->u.normal.subMenus, quitMenuIndex);
		}
	}
	else
	{
		menuToChange = menu->parentMenu;
	}
	return menuToChange;
}


void MenuActivate(MenuSystem *ms, menu_t *menu, int cmd);

menu_t *MenuProcessButtonCmd(MenuSystem *ms, menu_t *menu, int cmd)
{
	if (AnyButton(cmd) ||
		(!MenuTypeLeftRightMoves(menu->type) && (Left(cmd) || Right(cmd))))
	{
		// Ignore if menu contains no submenus
		if (menu->u.normal.subMenus.size == 0)
		{
			return NULL;
		}
		menu_t *subMenu =
			CArrayGet(&menu->u.normal.subMenus, menu->u.normal.index);

		// Only allow menu switching on button 1

		switch (subMenu->type)
		{
		case MENU_TYPE_NORMAL:
		case MENU_TYPE_OPTIONS:
		case MENU_TYPE_KEYS:
		case MENU_TYPE_CUSTOM:
			if (cmd & CMD_BUTTON1)
			{
				return subMenu;
			}
			break;
		case MENU_TYPE_CAMPAIGN_ITEM:
			if (cmd & CMD_BUTTON1)
			{
				CampaignLoad(&gCampaign, &subMenu->u.campaign);
				return subMenu;	// caller will check if subMenu type is CAMPAIGN_ITEM
			}
			break;
		case MENU_TYPE_BACK:
			if (cmd & CMD_BUTTON1)
			{
				return menu->parentMenu;
			}
			break;
		case MENU_TYPE_QUIT:
			if (cmd & CMD_BUTTON1)
			{
				return subMenu;	// caller will check if subMenu type is QUIT
			}
			break;
		case MENU_TYPE_RETURN:
			if (cmd & CMD_BUTTON1)
			{
				return subMenu;
			}
			break;
		default:
			MenuActivate(ms, subMenu, cmd);
			break;
		}
	}
	return NULL;
}

int KeyAvailable(int key, int code, input_keys_t *keys, input_keys_t *keysOther)
{
	key_code_e i;

	if (key == SDLK_ESCAPE || key == SDLK_F9 || key == SDLK_F10)
	{
		return 0;
	}
	if (key == gConfig.Input.PlayerKeys[0].Keys.map && code >= 0)
	{
		return 0;
	}

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
	int key = GetKey(&gEventHandlers);	// wait until user has pressed a new button

	if (key == SDLK_ESCAPE)
	{
		MenuPlaySound(MENU_SOUND_BACK);
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
			gConfig.Input.PlayerKeys[0].Keys.map = key;
		}
		MenuPlaySound(MENU_SOUND_ENTER);
	}
	else
	{
		MenuPlaySound(MENU_SOUND_ERROR);
	}
	menu->u.normal.changeKeyMenu = NULL;
}


void MenuChangeIndex(menu_t *menu, int cmd)
{
	int leftRightMoves = MenuTypeLeftRightMoves(menu->type);

	// Ignore if no submenus
	if (menu->u.normal.subMenus.size == 0)
	{
		return;
	}

	if (Up(cmd) || (leftRightMoves && Left(cmd)))
	{
		menu->u.normal.index--;
		if (menu->u.normal.index == -1)
		{
			menu->u.normal.index = (int)menu->u.normal.subMenus.size - 1;
		}
		MoveIndexToNextEnabledSubmenu(menu, 0);
		MenuPlaySound(MENU_SOUND_SWITCH);
	}
	else if (Down(cmd) || (leftRightMoves && Right(cmd)))
	{
		menu->u.normal.index++;
		if (menu->u.normal.index == (int)menu->u.normal.subMenus.size)
		{
			menu->u.normal.index = 0;
		}
		MoveIndexToNextEnabledSubmenu(menu, 1);
		MenuPlaySound(MENU_SOUND_SWITCH);
	}
	menu->u.normal.scroll =
		CLAMP(menu->u.normal.scroll,
			MAX(0, menu->u.normal.index - menu->u.normal.maxItems + 1),
			MIN((int)menu->u.normal.subMenus.size - 1, menu->u.normal.index + menu->u.normal.maxItems - 1));
	if (menu->u.normal.index < menu->u.normal.scroll)
	{
		menu->u.normal.scroll = menu->u.normal.index;
	}
}


void MenuActivate(MenuSystem *ms, menu_t *menu, int cmd)
{
	UNUSED(ms);
	MenuPlaySound(MENU_SOUND_SWITCH);
	switch (menu->type)
	{
	case MENU_TYPE_BASIC:
		// do nothing
		return;
	case MENU_TYPE_SET_OPTION_TOGGLE:
		*menu->u.option.uHook.optionToggle = !*menu->u.option.uHook.optionToggle;
		break;
	case MENU_TYPE_SET_OPTION_RANGE:
		{
			int option = *menu->u.option.uHook.optionRange.option;
			int increment = menu->u.option.uHook.optionRange.increment;
			int low = menu->u.option.uHook.optionRange.low;
			int high = menu->u.option.uHook.optionRange.high;
			if (Left(cmd))
			{
				if (low == option)
				{
					option = high;
				}
				else if (low + increment > option)
				{
					option = low;
				}
				else
				{
					option -= increment;
				}
			}
			else if (Right(cmd))
			{
				if (high == option)
				{
					option = low;
				}
				else if (high - increment < option)
				{
					option = high;
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
	case MENU_TYPE_VOID_FUNC:
		menu->u.option.uHook.voidFunc.func(menu->u.option.uHook.voidFunc.data);
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
