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

    Copyright (c) 2013-2015, Cong Xu
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
#pragma once

#include <cdogs/c_array.h>
#include <cdogs/campaigns.h>
#include <cdogs/events.h>
#include <cdogs/gamedata.h>
#include <cdogs/grafx.h>

#include "credits.h"

typedef enum
{
	MENU_TYPE_NORMAL,				// normal menu with items, up/down/left/right moves cursor
	MENU_TYPE_OPTIONS,				// menu with items, only up/down moves
	MENU_TYPE_KEYS,					// extra wide option menu
	MENU_TYPE_BASIC,				// no items, does nothing (use custom callbacks)
	MENU_TYPE_SET_OPTION_TOGGLE,	// no items, sets option on/off
	MENU_TYPE_SET_OPTION_RANGE,		// no items, sets option range low-high
	MENU_TYPE_SET_OPTION_SEED,		// set random seed
	MENU_TYPE_SET_OPTION_UP_DOWN_VOID_FUNC_VOID,	// set option using up/down functions
	MENU_TYPE_SET_OPTION_RANGE_GET_SET,	// set option range low-high using get/set functions
	MENU_TYPE_SET_OPTION_CHANGE_KEY,	// redefine key
	MENU_TYPE_VOID_FUNC,			// call a void (*f)(void *) function
	MENU_TYPE_BACK,
	MENU_TYPE_QUIT,
	MENU_TYPE_RETURN,				// Return with a code
	MENU_TYPE_CUSTOM				// use custom callbacks for input and drawing
} menu_type_e;

typedef enum
{
	MENU_OPTION_DISPLAY_STYLE_NONE,
	MENU_OPTION_DISPLAY_STYLE_STR_FUNC,	// use a function that returns string
	MENU_OPTION_DISPLAY_STYLE_INT_TO_STR_FUNC,	// function that converts int to string
} menu_option_display_style_e;

typedef enum
{
	MENU_DISPLAY_ITEMS_CREDITS	= 0x01,
	MENU_DISPLAY_ITEMS_AUTHORS	= 0x02
} menu_display_items_e;

typedef struct menu menu_t;

// Callback for drawing custom menu
// menu, graphics, pos, size, data
typedef void (*MenuDisplayFunc)(
	const menu_t *, GraphicsDevice *, const Vec2i, const Vec2i, const void *);
// Callback for handling user input
// cmd, data
// returns: 0 if stay on menu, 1 if exit from menu
typedef int (*MenuInputFunc)(int, void *);
// Callback for processing custom function on menu
typedef void(*MenuFunc)(menu_t *, void *);
// Callback for processing custom menu function post input
// menu, cmd, data
typedef void(*MenuPostInputFunc)(menu_t *, int cmd, void *);

typedef enum
{
	MENU_ALIGN_CENTER,
	MENU_ALIGN_LEFT
} MenuAlignStyle;

typedef enum
{
	MENU_SOUND_ENTER,
	MENU_SOUND_BACK,
	MENU_SOUND_SWITCH,
	MENU_SOUND_START,
	MENU_SOUND_ERROR
} MenuSound;

struct menu
{
	char *name;
	menu_type_e type;
	struct menu *parentMenu;
	bool isDisabled;
	color_t color;
	MenuFunc customPostEnterFunc;
	void *customPostEnterData;
	MenuFunc customPostUpdateFunc;
	void *customPostUpdateData;
	bool isCustomPostUpdateDataDynamic;
	MenuPostInputFunc customPostInputFunc;
	void *customPostInputData;
	bool isCustomPostInputDataDynamic;
	MenuDisplayFunc customDisplayFunc;
	const void *customDisplayData;
	MenuSound enterSound;
	union
	{
		// normal menu, with sub menus
		struct
		{
			char title[64];
			CArray subMenus;	// of menu_t
			// whether to use alternate control to enter submenus
			bool isSubmenusAlt;
			int index;
			int scroll;
			int maxItems;	// 0 means unlimited
			MenuAlignStyle align;
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
				bool *optionToggle;
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
					void (*func)(void *);
					void *data;
				} voidFunc;
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
				const char *(*intToStr)(int);
			} uFunc;
		} option;
		// change key
		struct
		{
			key_code_e code;
			int playerIndex;
		} changeKey;
		int returnCode;
		struct
		{
			MenuDisplayFunc displayFunc;
			MenuInputFunc inputFunc;
			void *data;
		} customData;
	} u;
};

typedef struct
{
	MenuDisplayFunc Func;
	void *Data;
} MenuCustomDisplayFunc;
typedef struct
{
	menu_t *root;
	menu_t *current;
	CArray exitTypes;	// of menu_type_e
	credits_displayer_t *creditsDisplayer;
	EventHandlers *handlers;
	GraphicsDevice *graphics;
	Vec2i pos;
	Vec2i size;
	MenuAlignStyle align;
	bool allowAborts;
	bool hasAbort;
	CArray customDisplayFuncs;	// of MenuCustomDisplayFunc
} MenuSystem;


void MenuSystemInit(
	MenuSystem *ms,
	EventHandlers *handlers, GraphicsDevice *graphics, Vec2i pos, Vec2i size);
void MenuSystemTerminate(MenuSystem *ms);
void MenuSetCreditsDisplayer(MenuSystem *menu, credits_displayer_t *creditsDisplayer);
void MenuAddExitType(MenuSystem *menu, menu_type_e exitType);
void MenuSystemAddCustomDisplay(
	MenuSystem *ms, MenuDisplayFunc func, void *data);
int MenuIsExit(MenuSystem *ms);
void MenuLoop(MenuSystem *menu);
void MenuDisplay(const MenuSystem *ms);
void MenuProcessCmd(MenuSystem *ms, int cmd);
void MenuReset(MenuSystem *menu);
void MenuDisableSubmenu(menu_t *menu, int index);
void MenuEnableSubmenu(menu_t *menu, int index);
menu_t *MenuGetSubmenuByName(menu_t *menu, const char *name);

void ShowControls(void);
Vec2i DisplayMenuItem(
	Vec2i pos, const char *s, int selected, int isDisabled, color_t color);

menu_t *MenuCreate(const char *name, menu_type_e type);
menu_t *MenuCreateNormal(
	const char *name,
	const char *title,
	menu_type_e type,
	int displayItems);
void MenuAddSubmenu(menu_t *menu, menu_t *subMenu);
void MenuSetPostEnterFunc(
	menu_t *menu, MenuFunc func, void *data, const bool isDynamicData);
void MenuSetPostUpdateFunc(
	menu_t *menu, MenuFunc func, void *data, const bool isDynamicData);
void MenuSetPostInputFunc(menu_t *menu, MenuPostInputFunc func, void *data);
void MenuSetCustomDisplay(
	menu_t *menu, MenuDisplayFunc func, const void *data);

// Make an options menu using a group of configs
menu_t *MenuCreateConfigOptions(
	const char *name, const char *title, const Config *c, MenuSystem *ms,
	const bool backOrReturn);
void MenuAddConfigOptionsItem(menu_t *menu, Config *c);
menu_t *MenuCreateOptionToggle(const char *name, bool *config);
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
menu_t *MenuCreateVoidFunc(
	const char *name, void (*func)(void *), void *data);
menu_t *MenuCreateOptionRangeGetSet(
	const char *name,
	int(*getFunc)(void), void(*setFunc)(int),
	int low, int high, int increment,
	menu_option_display_style_e style, void (*func)(void));
menu_t *MenuCreateSeparator(const char *name);
menu_t *MenuCreateBack(const char *name);
menu_t *MenuCreateReturn(const char *name, int returnCode);
menu_t *MenuCreateCustom(
	const char *name,
	MenuDisplayFunc displayFunc, MenuInputFunc inputFunc,
	void *data);

void MenuPlaySound(MenuSound s);

void MenuDestroy(MenuSystem *menu);

void PostInputConfigApply(menu_t *menu, int cmd, void *data);
