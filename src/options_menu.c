/*
	Copyright (c) 2024 Cong Xu
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
#include "options_menu.h"

#include <cdogs/config_io.h>
#include <cdogs/log.h>

#include "files.h"

static menu_t *MenuCreateConfigOptions(
	const char *name, const char *title, const Config *c,
	OptionsMenuData *data, const bool backOrReturn);
static menu_t *MenuCreateOptionsGraphics(
	const char *name, OptionsMenuData *data);

#if !defined(__ANDROID__) && !defined(__GCWZERO__)
static menu_t *MenuCreateOptionsControls(
	const char *name, OptionsMenuData *data);
#endif

menu_t *MenuCreateOptions(const char *name, OptionsMenuData *data)
{
	menu_t *menu = MenuCreateNormal(name, "Options:", MENU_TYPE_NORMAL, 0);
	MenuAddSubmenu(
		menu, MenuCreateConfigOptions(
				  "Game...", "Game Options:", ConfigGet(data->config, "Game"),
				  data, true));
	MenuAddSubmenu(menu, MenuCreateOptionsGraphics("Graphics...", data));
	MenuAddSubmenu(
		menu, MenuCreateConfigOptions(
				  "Interface...", "Interface Options:",
				  ConfigGet(data->config, "Interface"), data, true));
#if !defined(__ANDROID__) && !defined(__GCWZERO__)
	MenuAddSubmenu(menu, MenuCreateOptionsControls("Controls...", data));
#endif
	MenuAddSubmenu(
		menu, MenuCreateConfigOptions(
				  "Sound...", "Configure Sound:",
				  ConfigGet(data->config, "Sound"), data, true));
	MenuAddSubmenu(menu, MenuCreateSeparator(""));
	MenuAddSubmenu(menu, MenuCreateBack("Back"));
	return menu;
}

static void PostInputConfigApply(menu_t *menu, int cmd, void *data);

static menu_t *MenuCreateConfigOptions(
	const char *name, const char *title, const Config *c,
	OptionsMenuData *data, const bool backOrReturn)
{
	menu_t *menu = MenuCreateNormal(name, title, MENU_TYPE_OPTIONS, 0);
	CASSERT(
		c->Type == CONFIG_TYPE_GROUP,
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
	MenuSetPostInputFunc(menu, PostInputConfigApply, data);
	return menu;
}

static void PostInputConfigApply(menu_t *menu, int cmd, void *data)
{
	UNUSED(menu);
	UNUSED(cmd);
	OptionsMenuData *mData = data;
	bool resetBg = false;
	if (!ConfigApply(mData->config, &resetBg))
	{
		LOG(LM_MAIN, LL_ERROR, "Failed to apply config; reset to last used");
		ConfigResetChanged(mData->config);
	}
	else
	{
		// Save config immediately
		ConfigSave(mData->config, GetConfigFilePath(CONFIG_FILE));
	}

	if (mData->gfxChangeCallback)
	{
		mData->gfxChangeCallback(mData->gfxChangeData, resetBg);
	}
}

static menu_t *MenuCreateOptionsGraphics(
	const char *name, OptionsMenuData *data)
{
	menu_t *menu =
		MenuCreateNormal(name, "Graphics Options:", MENU_TYPE_OPTIONS, 0);
	MenuAddConfigOptionsItem(
		menu, ConfigGet(data->config, "Graphics.Brightness"));
#ifndef __GCWZERO__
#ifndef __ANDROID__
	MenuAddConfigOptionsItem(
		menu, ConfigGet(data->config, "Graphics.Fullscreen"));
	// TODO: fix second window rendering
	// MenuAddConfigOptionsItem(
	//	menu, ConfigGet(data->config, "Graphics.SecondWindow"));
#endif // ANDROID

	MenuAddConfigOptionsItem(
		menu, ConfigGet(data->config, "Graphics.ScaleFactor"));
	MenuAddConfigOptionsItem(
		menu, ConfigGet(data->config, "Graphics.ScaleMode"));
#endif // GCWZERO
	MenuAddConfigOptionsItem(
		menu, ConfigGet(data->config, "Graphics.Shadows"));
	MenuAddConfigOptionsItem(menu, ConfigGet(data->config, "Graphics.Gore"));
	MenuAddConfigOptionsItem(menu, ConfigGet(data->config, "Graphics.Brass"));
	MenuAddSubmenu(menu, MenuCreateSeparator(""));
	MenuAddSubmenu(menu, MenuCreateBack("Done"));
	MenuSetPostInputFunc(menu, PostInputConfigApply, data);
	return menu;
}

static void MenuCreateKeysSingleSection(
	menu_t *menu, const char *sectionName, const int playerIndex);
static menu_t *MenuCreateOptionChangeKey(
	const char *name, const key_code_e code, const int playerIndex,
	const bool isOptional);

#if !defined(__ANDROID__) && !defined(__GCWZERO__)
static menu_t *MenuCreateOptionsControls(
	const char *name, OptionsMenuData *data)
{
	menu_t *menu = MenuCreateNormal(name, "", MENU_TYPE_OPTIONS, 0);
	MenuCreateKeysSingleSection(menu, "Keyboard 1", 0);
	MenuCreateKeysSingleSection(menu, "Keyboard 2", 1);
	MenuAddSubmenu(
		menu, MenuCreateOptionChangeKey("map", KEY_CODE_MAP, 0, true));
	MenuAddSubmenu(menu, MenuCreateSeparator(""));
	MenuAddSubmenu(menu, MenuCreateBack("Done"));
	MenuSetPostInputFunc(menu, PostInputConfigApply, data);
	return menu;
}
#endif

static void MenuCreateKeysSingleSection(
	menu_t *menu, const char *sectionName, const int playerIndex)
{
	MenuAddSubmenu(menu, MenuCreateSeparator(sectionName));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionChangeKey("left", KEY_CODE_LEFT, playerIndex, false));
	MenuAddSubmenu(
		menu, MenuCreateOptionChangeKey(
				  "right", KEY_CODE_RIGHT, playerIndex, false));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionChangeKey("up", KEY_CODE_UP, playerIndex, false));
	MenuAddSubmenu(
		menu,
		MenuCreateOptionChangeKey("down", KEY_CODE_DOWN, playerIndex, false));
	MenuAddSubmenu(
		menu, MenuCreateOptionChangeKey(
				  "fire", KEY_CODE_BUTTON1, playerIndex, false));
	MenuAddSubmenu(
		menu, MenuCreateOptionChangeKey(
				  "switch/strafe", KEY_CODE_BUTTON2, playerIndex, false));
	MenuAddSubmenu(
		menu, MenuCreateOptionChangeKey(
				  "grenade", KEY_CODE_GRENADE, playerIndex, true));
	MenuAddSubmenu(menu, MenuCreateSeparator(""));
}

static menu_t *MenuCreateOptionChangeKey(
	const char *name, const key_code_e code, const int playerIndex,
	const bool isOptional)
{
	menu_t *menu = MenuCreate(name, MENU_TYPE_SET_OPTION_CHANGE_KEY);
	menu->u.changeKey.code = code;
	menu->u.changeKey.playerIndex = playerIndex;
	menu->u.changeKey.isOptional = isOptional;
	return menu;
}
