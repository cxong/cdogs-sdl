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

    Copyright (c) 2013-2014, 2016, Cong Xu
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

#include <cdogs/config_io.h>
#include <cdogs/files.h>
#include <cdogs/font.h>
#include <cdogs/gamedata.h>
#include <cdogs/grafx_bg.h>
#include <cdogs/log.h>
#include <cdogs/mission.h>
#include <cdogs/music.h>
#include <cdogs/pic_manager.h>
#include <cdogs/sounds.h>
#include <cdogs/utils.h>


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

static void MenuTerminate(menu_t *menu);

void MenuSystemTerminate(MenuSystem *ms)
{
	MenuTerminate(ms->root);
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
	CA_FOREACH(menu_type_e, m, menu->exitTypes)
		if (*m == exitType)
		{
			return 1;
		}
	CA_FOREACH_END()
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
	return ms->current == NULL || MenuHasExitType(ms, ms->current->type);
}

void MenuProcessChangeKey(menu_t *menu);

static GameLoopResult MenuUpdate(void *data);
static void MenuDraw(void *data);
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
		const int cmd = GetMenuCmd(ms->handlers);
		if (cmd)
		{
			MenuProcessCmd(ms, cmd);
		}
	}
	// Check if anyone pressed escape, or we need a hard exit
	int cmds[MAX_LOCAL_PLAYERS];
	memset(cmds, 0, sizeof cmds);
	GetPlayerCmds(&gEventHandlers, &cmds);
	const bool aborted =
		ms->allowAborts &&
		EventIsEscape(&gEventHandlers, cmds, GetMenuCmd(&gEventHandlers));
	if (aborted || ms->handlers->HasQuit)
	{
		ms->hasAbort = true;
		return UPDATE_RESULT_EXIT;
	}
	if (MenuIsExit(ms))
	{
		return UPDATE_RESULT_EXIT;
	}
	if (ms->current->customPostUpdateFunc)
	{
		ms->current->customPostUpdateFunc(
			ms->current, ms->current->customPostUpdateData);
	}
	return UPDATE_RESULT_DRAW;
}
static void MenuDraw(void *data)
{
	const MenuSystem *ms = data;
	GraphicsClear(ms->graphics);
	ShowControls();
	MenuDisplay(ms);
}

void MenuReset(MenuSystem *menu)
{
	menu->current = menu->root;
}

static void MoveIndexToNextEnabledSubmenu(menu_t *menu, const bool isDown)
{
	if (menu->u.normal.index >= (int)menu->u.normal.subMenus.size)
	{
		menu->u.normal.index = (int)menu->u.normal.subMenus.size - 1;
	}
	const int firstIndex = menu->u.normal.index;
	bool isFirst = true;
	// Move the selection to the next non-disabled submenu
	for (;;)
	{
		const menu_t *currentSubmenu =
			CArrayGet(&menu->u.normal.subMenus, menu->u.normal.index);
		if (!currentSubmenu->isDisabled)
		{
			break;
		}
		if (menu->u.normal.index == firstIndex && !isFirst)
		{
			break;
		}
		isFirst = false;
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
	MoveIndexToNextEnabledSubmenu(menu, true);
}
void MenuEnableSubmenu(menu_t *menu, int idx)
{
	menu_t *subMenu = CArrayGet(&menu->u.normal.subMenus, idx);
	subMenu->isDisabled = false;
}

menu_t *MenuGetSubmenuByName(menu_t *menu, const char *name)
{
	CA_FOREACH(menu_t, subMenu, menu->u.normal.subMenus)
		if (strcmp(subMenu->name, name) == 0)
		{
			return subMenu;
		}
	CA_FOREACH_END()
	return NULL;
}

void ShowControls(void)
{
	FontOpts opts = FontOptsNew();
	opts.HAlign = ALIGN_CENTER;
	opts.VAlign = ALIGN_END;
	opts.Area = gGraphicsDevice.cachedConfig.Res;
	opts.Pad.y = 10;
#ifdef __GCWZERO__
	FontStrOpt(
		"(use joystick or D pad + START + SELECT)", Vec2iZero(), opts);
#else
	FontStrOpt(
		"(use joystick 1 or arrow keys + Enter/Backspace)", Vec2iZero(), opts);
#endif
}

Vec2i DisplayMenuItem(
	Vec2i pos, const char *s, int selected, int isDisabled, color_t color)
{
	if (selected)
	{
		return FontStrMask(s, pos, colorRed);
	}
	else if (isDisabled)
	{
		color_t dark = { 64, 64, 64, 255 };
		return FontStrMask(s, pos, dark);
	}
	else if (!ColorEquals(color, colorBlack))
	{
		return FontStrMask(s, pos, color);
	}
	else
	{
		return FontStr(s, pos);
	}
}


int MenuTypeHasSubMenus(menu_type_e type)
{
	return
		type == MENU_TYPE_NORMAL ||
		type == MENU_TYPE_OPTIONS ||
		type == MENU_TYPE_KEYS;
}


menu_t *MenuCreate(const char *name, menu_type_e type)
{
	menu_t *menu;
	CCALLOC(menu, sizeof(menu_t));
	CSTRDUP(menu->name, name);
	menu->type = type;
	menu->parentMenu = NULL;
	menu->enterSound = MENU_SOUND_ENTER;
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
	menu->u.normal.isSubmenusAlt = false;
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

static void UpdateSubmenuParentPtrs(menu_t *menu)
{
	CA_FOREACH(menu_t, subMenu, menu->u.normal.subMenus)
		subMenu->parentMenu = menu;
		if (MenuTypeHasSubMenus(subMenu->type))
		{
			UpdateSubmenuParentPtrs(subMenu);
		}
	CA_FOREACH_END()
}
void MenuAddSubmenu(menu_t *menu, menu_t *subMenu)
{
	CArrayPushBack(&menu->u.normal.subMenus, subMenu);
	if (subMenu->type == MENU_TYPE_QUIT)
	{
		menu->u.normal.quitMenuIndex = (int)menu->u.normal.subMenus.size - 1;
	}
	CFREE(subMenu);

	// update all parent pointers, in child menus
	UpdateSubmenuParentPtrs(menu);

	// move cursor in case first menu item(s) are disabled
	MoveIndexToNextEnabledSubmenu(menu, true);
}

void MenuSetPostInputFunc(menu_t *menu, MenuPostInputFunc func, void *data)
{
	menu->customPostInputFunc = func;
	menu->customPostInputData = data;
}

void MenuSetPostEnterFunc(
	menu_t *menu, MenuFunc func, void *data, const bool isDynamicData)
{
	menu->customPostEnterFunc = func;
	menu->customPostEnterData = data;
	menu->isCustomPostEnterDataDynamic = isDynamicData;
}

void MenuSetPostUpdateFunc(
	menu_t *menu, MenuFunc func, void *data, const bool isDynamicData)
{
	menu->customPostUpdateFunc = func;
	menu->customPostUpdateData = data;
	menu->isCustomPostUpdateDataDynamic = isDynamicData;
}

void MenuSetCustomDisplay(menu_t *menu, MenuDisplayFunc func, const void *data)
{
	menu->customDisplayFunc = func;
	menu->customDisplayData = data;
}

menu_t *MenuCreateConfigOptions(
	const char *name, const char *title, const Config *c, MenuSystem *ms,
	const bool backOrReturn)
{
	menu_t *menu = MenuCreateNormal(name, title, MENU_TYPE_OPTIONS, 0);
	CASSERT(c->Type == CONFIG_TYPE_GROUP,
		"Cannot make menu from non-group config");
	CA_FOREACH(Config, child, c->u.Group)
		MenuAddConfigOptionsItem(menu, child);
	CA_FOREACH_END()
	MenuAddSubmenu(menu, MenuCreateSeparator(""));
	if (backOrReturn)
	{
		MenuAddSubmenu(menu, MenuCreateBack("Done"));
	}
	else
	{
		MenuAddSubmenu(menu, MenuCreateReturn("Done", 0));
	}
	MenuSetPostInputFunc(menu, PostInputConfigApply, ms);
	return menu;
}
void MenuAddConfigOptionsItem(menu_t *menu, Config *c)
{
	char nameBuf[256];
	CASSERT(strlen(c->Name) < sizeof nameBuf, "buffer too small");
	CamelToTitle(nameBuf, c->Name);
	switch (c->Type)
	{
	case CONFIG_TYPE_STRING:
		CASSERT(false, "Unimplemented");
		break;
	case CONFIG_TYPE_INT:
		MenuAddSubmenu(
			menu,
			MenuCreateOptionRange(
			nameBuf, (int *)&c->u.Int.Value,
			c->u.Int.Min, c->u.Int.Max, c->u.Int.Increment,
			MENU_OPTION_DISPLAY_STYLE_INT_TO_STR_FUNC,
			(void (*)(void))c->u.Int.IntToStr));
		break;
	case CONFIG_TYPE_FLOAT:
		CASSERT(false, "Unimplemented");
		break;
	case CONFIG_TYPE_BOOL:
		MenuAddSubmenu(
			menu, MenuCreateOptionToggle(nameBuf, &c->u.Bool.Value));
		break;
	case CONFIG_TYPE_ENUM:
		MenuAddSubmenu(
			menu,
			MenuCreateOptionRange(
			nameBuf, (int *)&c->u.Enum.Value,
			c->u.Enum.Min, c->u.Enum.Max, 1,
			MENU_OPTION_DISPLAY_STYLE_INT_TO_STR_FUNC,
			(void(*)(void))c->u.Enum.EnumToStr));
		break;
	case CONFIG_TYPE_GROUP:
		// Do nothing
		break;
	default:
		CASSERT(false, "Unknown config type");
		break;
	}
}

menu_t *MenuCreateOptionToggle(const char *name, bool *config)
{
	menu_t *menu = MenuCreate(name, MENU_TYPE_SET_OPTION_TOGGLE);
	menu->u.option.uHook.optionToggle = config;
	menu->u.option.displayStyle = MENU_OPTION_DISPLAY_STYLE_NONE;
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
		menu->u.option.uFunc.intToStr = (const char *(*)(int))func;
	}
	return menu;
}

menu_t *MenuCreateOptionSeed(const char *name, unsigned int *seed)
{
	menu_t *menu = MenuCreate(name, MENU_TYPE_SET_OPTION_SEED);
	menu->u.option.uHook.seed = seed;
	menu->u.option.displayStyle = MENU_OPTION_DISPLAY_STYLE_INT_TO_STR_FUNC;
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
	CA_FOREACH(MenuCustomDisplayFunc, cdf, ms->customDisplayFuncs)
		cdf->Func(NULL, ms->graphics, ms->pos, ms->size, cdf->Data);
	CA_FOREACH_END()
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
		const Pic *logo = PicManagerGetPic(&gPicManager, "logo");
		const Vec2i pos = Vec2iNew(
			MS_CENTER_X(*ms, logo->size.x), ms->pos.y + ms->size.y / 12);
		Blit(&gGraphicsDevice, logo, pos);

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
	const menu_t *menu = ms->current;

	switch (menu->type)
	{
	// TODO: refactor the three menu types (normal, options, campaign) into one
	case MENU_TYPE_NORMAL:
	case MENU_TYPE_OPTIONS:
		{
			int iStart = 0;
			int iEnd;
			int numMenuLines = 0;
			int maxIEnd = (int)menu->u.normal.subMenus.size;
			if (menu->u.normal.maxItems > 0)
			{
				// Calculate first/last indices
				if (menu->u.normal.scroll != 0)
				{
					iStart = menu->u.normal.scroll;
				}
				maxIEnd = iStart + menu->u.normal.maxItems;
			}
			// Count the number of menu items that can fit
			// This is to account for multi-line items
			for (iEnd = iStart;
				iEnd < maxIEnd && iEnd < (int)menu->u.normal.subMenus.size;
				iEnd++)
			{
				const menu_t *subMenu =
					CArrayGet(&menu->u.normal.subMenus, iEnd);
				const int numLines = FontStrNumLines(subMenu->name);
				if (menu->u.normal.maxItems > 0 &&
					numMenuLines + numLines > menu->u.normal.maxItems)
				{
					break;
				}
				numMenuLines += numLines;
			}

			int maxWidth = 0;
			CA_FOREACH(const menu_t, subMenu, menu->u.normal.subMenus)
				const int width = FontStrW(subMenu->name);
				if (width > maxWidth)
				{
					maxWidth = width;
				}
			CA_FOREACH_END()
			// Limit max width if it is larger than the menu system size
			maxWidth = MIN(ms->size.x, maxWidth);
			const bool isCentered = menu->type == MENU_TYPE_NORMAL;
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

			yStart = MS_CENTER_Y(*ms, numMenuLines * FontH());
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
							yStart + numMenuLines*FontH() + 2),
						"v",
						0, 0,
						colorBlack);
				}
			}
			const int xOptions = x + maxWidth + 10;

			// Display normal menu items
			Vec2i pos = Vec2iNew(x, yStart);
			for (int i = iStart; i < iEnd; i++)
			{
				const menu_t *subMenu = CArrayGet(&menu->u.normal.subMenus, i);
				char *nameBuf;
				CMALLOC(nameBuf, strlen(subMenu->name) + 3);
				if (subMenu->type == MENU_TYPE_NORMAL &&
					subMenu->u.normal.isSubmenusAlt)
				{
					sprintf(nameBuf, "%s >", subMenu->name);
				}
				else
				{
					strcpy(nameBuf, subMenu->name);
				}

				switch (menu->u.normal.align)
				{
				case MENU_ALIGN_CENTER:
					pos.x = MS_CENTER_X(*ms, FontStrW(nameBuf));
					break;
				case MENU_ALIGN_LEFT:
					// Do nothing
					break;
				default:
					assert(0 && "unknown alignment");
					break;
				}

				const int yNext = DisplayMenuItem(
					pos,
					nameBuf,
					i == menu->u.normal.index,
					subMenu->isDisabled,
					subMenu->color).y + FontH();
				CFREE(nameBuf);

				// display option value
				const int optionInt = MenuOptionGetIntValue(subMenu);
				const Vec2i valuePos = Vec2iNew(xOptions, pos.y);
				if (subMenu->type == MENU_TYPE_SET_OPTION_RANGE ||
					subMenu->type == MENU_TYPE_SET_OPTION_SEED ||
					subMenu->type == MENU_TYPE_SET_OPTION_UP_DOWN_VOID_FUNC_VOID ||
					subMenu->type == MENU_TYPE_SET_OPTION_RANGE_GET_SET)
				{
					switch (subMenu->u.option.displayStyle)
					{
					case MENU_OPTION_DISPLAY_STYLE_NONE:
						// Do nothing
						break;
					case MENU_OPTION_DISPLAY_STYLE_STR_FUNC:
						FontStr(subMenu->u.option.uFunc.str(), valuePos);
						break;
					case MENU_OPTION_DISPLAY_STYLE_INT_TO_STR_FUNC:
						FontStr(
							subMenu->u.option.uFunc.intToStr(optionInt),
							valuePos);
						break;
					default:
						CASSERT(false, "unknown menu display type");
						break;
					}
				}
				else if (subMenu->type == MENU_TYPE_SET_OPTION_TOGGLE)
				{
					FontStr(optionInt ? "Yes" : "No", valuePos);
				}

				pos.y = yNext;
			}
		}
		break;
	case MENU_TYPE_KEYS:
		{
			x = MS_CENTER_X(*ms, (FontW('a') * 10)) / 2;
			const int xKeys = x * 3;
			yStart = (gGraphicsDevice.cachedConfig.Res.y / 2) - (FontH() * 10);

			CA_FOREACH(const menu_t, subMenu, menu->u.normal.subMenus)
				int y = yStart + _ca_index * FontH();
				bool isSelected = _ca_index == menu->u.normal.index;

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
					else
					{
						const int pi = subMenu->u.changeKey.playerIndex;
						const InputKeys *keys =
							&gEventHandlers.keyboard.PlayerKeys[pi];
						const SDL_Scancode sc = KeyGet(
							keys, subMenu->u.changeKey.code);
						keyName = SDL_GetScancodeName(sc);
						if (sc == SDL_SCANCODE_UNKNOWN || keyName == NULL)
						{
							keyName = "<Unset>";
						}
					}
					DisplayMenuItem(
						Vec2iNew(xKeys, y),
						keyName,
						isSelected,
						0,
						colorBlack);
				}
			CA_FOREACH_END()
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
		SoundPlay(&gSoundDevice, StrSound("aargh/man"));
		break;
	default:
		break;
	}
}

static void MenuTerminateSubmenus(menu_t *menu);
static void MenuTerminate(menu_t *menu)
{
	if (menu == NULL)
	{
		return;
	}
	CFREE(menu->name);
	if (menu->isCustomPostUpdateDataDynamic)
	{
		CFREE(menu->customPostUpdateData);
	}
	if (menu->isCustomPostEnterDataDynamic)
	{
		CFREE(menu->customPostEnterData);
	}
	MenuTerminateSubmenus(menu);
}
static void MenuTerminateSubmenus(menu_t *menu)
{
	if (!MenuTypeHasSubMenus(menu->type))
	{
		return;
	}
	CA_FOREACH(menu_t, subMenu, menu->u.normal.subMenus)
		MenuTerminate(subMenu);
	CA_FOREACH_END()
	CArrayTerminate(&menu->u.normal.subMenus);
}

void MenuClearSubmenus(menu_t *menu)
{
	if (!MenuTypeHasSubMenus(menu->type))
	{
		CASSERT(false, "attempt to clear submenus for invalid menu type");
		return;
	}
	MenuTerminateSubmenus(menu);
	CArrayInit(&menu->u.normal.subMenus, sizeof(menu_t));
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
	if (cmd == CMD_ESC || (cmd & CMD_BUTTON2) ||
		((cmd & CMD_LEFT) && menu->u.normal.isSubmenusAlt))
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
			MenuPlaySound(menuToChange->enterSound);
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
	if (AnyButton(cmd) || Left(cmd) || Right(cmd))
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
			if (subMenu->u.normal.isSubmenusAlt ?
				(cmd & CMD_RIGHT) : (cmd & CMD_BUTTON1))
			{
				return subMenu;
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

static bool KeyAvailable(
	const SDL_Scancode key, const key_code_e code, const int playerIndex)
{
	if (key == SDL_SCANCODE_ESCAPE ||
		key == SDL_SCANCODE_F9 || key == SDL_SCANCODE_F10)
	{
		return false;
	}
	if (key == (SDL_Scancode)ConfigGetInt(&gConfig, "Input.PlayerCodes0.map"))
	{
		return false;
	}

	// Check if the key is being used by another control
	char buf[256];
	sprintf(buf, "Input.PlayerCodes%d", playerIndex);
	const InputKeys keys = KeyLoadPlayerKeys(ConfigGet(&gConfig, buf));
	for (key_code_e i = 0; i < KEY_CODE_MAP; i++)
	{
		if (i != code && KeyGet(&keys, i) == key)
		{
			return false;
		}
	}

	// Check if the other player is using the key
	sprintf(buf, "Input.PlayerCodes%d", 1 - playerIndex);
	const InputKeys keysOther = KeyLoadPlayerKeys(ConfigGet(&gConfig, buf));
	if (keysOther.left == key ||
		keysOther.right == key ||
		keysOther.up == key ||
		keysOther.down == key ||
		keysOther.button1 == key ||
		keysOther.button2 == key)
	{
		return false;
	}

	return true;
}

void MenuProcessChangeKey(menu_t *menu)
{
	// wait until user has pressed a new button
	const SDL_Scancode key = GetKey(&gEventHandlers);
	const key_code_e code = menu->u.normal.changeKeyMenu->u.changeKey.code;
	const int pi = menu->u.normal.changeKeyMenu->u.changeKey.playerIndex;
	if (key == SDL_SCANCODE_ESCAPE)
	{
		MenuPlaySound(MENU_SOUND_BACK);
	}
	else if (KeyAvailable(key, code, pi))
	{
		if (code != KEY_CODE_MAP)
		{
			char buf[256];
			sprintf(buf, "Input.PlayerCodes%d.%s", pi, KeycodeStr(code));
			ConfigGet(&gConfig, buf)->u.Int.Value = key;
			sprintf(buf, "Input.PlayerCodes%d", pi);
			gEventHandlers.keyboard.PlayerKeys[pi] = KeyLoadPlayerKeys(
				ConfigGet(&gConfig, buf));
		}
		else
		{
			ConfigGet(&gConfig, "Input.PlayerCodes0.map")->u.Int.Value = key;
			gEventHandlers.keyboard.PlayerKeys[0].map = key;
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
	// Ignore if no submenus
	if (menu->u.normal.subMenus.size == 0)
	{
		return;
	}

	if (Up(cmd))
	{
		menu->u.normal.index--;
		if (menu->u.normal.index == -1)
		{
			menu->u.normal.index = (int)menu->u.normal.subMenus.size - 1;
		}
		MoveIndexToNextEnabledSubmenu(menu, false);
		MenuPlaySound(MENU_SOUND_SWITCH);
	}
	else if (Down(cmd))
	{
		menu->u.normal.index++;
		if (menu->u.normal.index == (int)menu->u.normal.subMenus.size)
		{
			menu->u.normal.index = 0;
		}
		MoveIndexToNextEnabledSubmenu(menu, true);
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
		if (AnyButton(cmd))
		{
			menu->u.option.uHook.voidFunc.func(
				menu->u.option.uHook.voidFunc.data);
		}
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

void PostInputConfigApply(menu_t *menu, int cmd, void *data)
{
	UNUSED(menu);
	UNUSED(cmd);
	if (!ConfigApply(&gConfig))
	{
		LOG(LM_MAIN, LL_ERROR, "Failed to apply config; reset to last used");
		ConfigResetChanged(&gConfig);
	}
	else
	{
		// Save config immediately
		ConfigSave(&gConfig, GetConfigFilePath(CONFIG_FILE));
	}

	// Update menu system so that resolution changes don't
	// affect menu positions
	MenuSystem *ms = data;
	ms->pos = Vec2iZero();
	ms->size = Vec2iNew(
		ms->graphics->cachedConfig.Res.x,
		ms->graphics->cachedConfig.Res.y);
}
