/*
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
#include "mainmenu.h"

#include <stdio.h>
#include <string.h>

#include <cdogs/config.h>
#include <cdogs/font.h>
#include <cdogs/log.h>
#include <cdogs/net_client.h>

#include "autosave.h"
#include "menu.h"
#include "prep.h"


MenuSystem *MenuCreateAll(
	custom_campaigns_t *campaigns,
	EventHandlers *handlers,
	GraphicsDevice *graphics);

static menu_t *FindSubmenuByName(menu_t *menu, const char *name);
void MainMenu(
	GraphicsDevice *graphics,
	credits_displayer_t *creditsDisplayer,
	custom_campaigns_t *campaigns,
	const GameMode lastGameMode)
{
	MenuSystem *menu = MenuCreateAll(campaigns, &gEventHandlers, graphics);
	MenuSetCreditsDisplayer(menu, creditsDisplayer);
	// Auto-enter the submenu corresponding to the last game mode
	menu_t *startMenu = FindSubmenuByName(menu->root, "Start");
	switch (lastGameMode)
	{
	case GAME_MODE_NORMAL:
		menu->current = FindSubmenuByName(startMenu, "Campaign");
		break;
	case GAME_MODE_DOGFIGHT:
		menu->current = FindSubmenuByName(startMenu, "Dogfight");
		break;
	case GAME_MODE_DEATHMATCH:
		menu->current = FindSubmenuByName(startMenu, "Deathmatch");
		break;
	default:
		// Do nothing
		break;
	}
	MenuLoop(menu);

	MenuDestroy(menu);
}
static menu_t *FindSubmenuByName(menu_t *menu, const char *name)
{
	CASSERT(menu->type == MENU_TYPE_NORMAL, "invalid menu type");
	CA_FOREACH(menu_t, submenu, menu->u.normal.subMenus)
	if (strcmp(submenu->name, name) == 0) return submenu;
	CA_FOREACH_END()
	return menu;
}

static menu_t *MenuCreateStart(
	const char *name, MenuSystem *ms, custom_campaigns_t *campaigns);
static menu_t *MenuCreateOptions(const char *name, MenuSystem *ms);
menu_t *MenuCreateQuit(const char *name);

MenuSystem *MenuCreateAll(
	custom_campaigns_t *campaigns,
	EventHandlers *handlers,
	GraphicsDevice *graphics)
{
	MenuSystem *ms;
	CCALLOC(ms, sizeof *ms);
	MenuSystemInit(
		ms,
		handlers, graphics,
		Vec2iZero(),
		Vec2iNew(
			graphics->cachedConfig.Res.x,
			graphics->cachedConfig.Res.y));
	ms->root = ms->current = MenuCreateNormal(
		"",
		"",
		MENU_TYPE_NORMAL,
		MENU_DISPLAY_ITEMS_CREDITS | MENU_DISPLAY_ITEMS_AUTHORS);
	MenuAddSubmenu(ms->root,MenuCreateStart("Start", ms, campaigns));
	MenuAddSubmenu(ms->root, MenuCreateOptions("Options...", ms));
	MenuAddSubmenu(ms->root, MenuCreateQuit("Quit"));
	MenuAddExitType(ms, MENU_TYPE_QUIT);
	MenuAddExitType(ms, MENU_TYPE_RETURN);

	return ms;
}

typedef struct
{
	int MenuJoinIndex;
} CheckLANServerData;
static menu_t *MenuCreateContinue(const char *name, CampaignEntry *entry);
static menu_t *MenuCreateQuickPlay(const char *name, CampaignEntry *entry);
static menu_t *MenuCreateCampaigns(
	const char *name, const char *title,
	campaign_list_t *list, const GameMode mode);
static menu_t *CreateJoinLANGame(const char *name, MenuSystem *ms);
static void CheckLANServers(menu_t *menu, void *data);
static menu_t *MenuCreateStart(
	const char *name, MenuSystem *ms, custom_campaigns_t *campaigns)
{
	menu_t *menu = MenuCreateNormal(name, "Start:", MENU_TYPE_NORMAL, 0);
	MenuAddSubmenu(
		menu,
		MenuCreateContinue("Continue", &gAutosave.LastMission.Campaign));
	const int menuContinueIndex = (int)menu->u.normal.subMenus.size - 1;
	MenuAddSubmenu(
		menu,
		MenuCreateQuickPlay("Quick Play", &campaigns->quickPlayEntry));
	MenuAddSubmenu(
		menu,
		MenuCreateCampaigns(
		"Campaign",
		"Select a campaign:",
		&campaigns->campaignList,
		GAME_MODE_NORMAL));
	MenuAddSubmenu(
		menu,
		MenuCreateCampaigns(
		"Dogfight",
		"Select a scenario:",
		&campaigns->dogfightList,
		GAME_MODE_DOGFIGHT));
	MenuAddSubmenu(
		menu,
		MenuCreateCampaigns(
		"Deathmatch",
		"Select a scenario:",
		&campaigns->dogfightList,
		GAME_MODE_DEATHMATCH));
	MenuAddSubmenu(menu, CreateJoinLANGame("Join LAN game", ms));
	CheckLANServerData *cdata;
	CMALLOC(cdata, sizeof *cdata);
	cdata->MenuJoinIndex = (int)menu->u.normal.subMenus.size - 1;
	MenuAddSubmenu(menu, MenuCreateSeparator(""));
	MenuAddSubmenu(menu, MenuCreateBack("Back"));

	if (strlen(gAutosave.LastMission.Password) == 0 ||
		!gAutosave.LastMission.IsValid)
	{
		MenuDisableSubmenu(menu, menuContinueIndex);
	}

	MenuDisableSubmenu(menu, cdata->MenuJoinIndex);
	// Periodically check if LAN servers are available
	MenuSetPostUpdateFunc(menu, CheckLANServers, cdata, true);

	return menu;
}

typedef struct
{
	GameMode GameMode;
	CampaignEntry *Entry;
} StartGameModeData;
static void StartGameMode(menu_t *menu, void *data);

static menu_t *CreateStartGameMode(
	const char *name, GameMode mode, CampaignEntry *entry)
{
	menu_t *menu = MenuCreate(name, MENU_TYPE_RETURN);
	menu->enterSound = MENU_SOUND_START;
	StartGameModeData *data;
	CCALLOC(data, sizeof *data);
	data->GameMode = mode;
	data->Entry = entry;
	MenuSetPostEnterFunc(menu, StartGameMode, data, true);
	return menu;
}
static void StartGameMode(menu_t *menu, void *data)
{
	UNUSED(menu);
	StartGameModeData *mData = data;
	gCampaign.Entry.Mode = mData->GameMode;
	debug(D_NORMAL, "Starting game mode %s %d\n",
		mData->Entry->Path, (int)mData->GameMode);
	if (!CampaignLoad(&gCampaign, mData->Entry))
	{
		// Failed to load
		printf("Error: cannot load campaign %s\n", mData->Entry->Info);
	}
}
static menu_t *MenuCreateContinue(const char *name, CampaignEntry *entry)
{
	return CreateStartGameMode(name, GAME_MODE_NORMAL, entry);
}
static menu_t *MenuCreateQuickPlay(const char *name, CampaignEntry *entry)
{
	return CreateStartGameMode(name, GAME_MODE_QUICK_PLAY, entry);
}

static menu_t *MenuCreateCampaignItem(
	CampaignEntry *entry, const GameMode mode);

static void CampaignsDisplayFilename(
	const menu_t *menu, GraphicsDevice *g,
	const Vec2i pos, const Vec2i size, const void *data)
{
	const menu_t *subMenu =
		CArrayGet(&menu->u.normal.subMenus, menu->u.normal.index);
	UNUSED(g);
	UNUSED(data);
	if (subMenu->type != MENU_TYPE_BASIC ||
		subMenu->customPostInputData == NULL)
	{
		return;
	}
	const StartGameModeData *mData = subMenu->customPostInputData;
	char s[CDOGS_FILENAME_MAX];
	sprintf(s, "( %s )", mData->Entry->Filename);

	FontOpts opts = FontOptsNew();
	opts.HAlign = ALIGN_CENTER;
	opts.VAlign = ALIGN_END;
	opts.Area = size;
	opts.Pad.x = size.x / 12;
	FontStrOpt(s, pos, opts);
}
static menu_t *MenuCreateCampaigns(
	const char *name, const char *title,
	campaign_list_t *list, const GameMode mode)
{
	menu_t *menu = MenuCreateNormal(name, title, MENU_TYPE_NORMAL, 0);
	menu->u.normal.maxItems = 20;
	menu->u.normal.align = MENU_ALIGN_CENTER;
	for (int i = 0; i < (int)list->subFolders.size; i++)
	{
		char folderName[CDOGS_FILENAME_MAX];
		campaign_list_t *subList = CArrayGet(&list->subFolders, i);
		sprintf(folderName, "%s/", subList->Name);
		MenuAddSubmenu(
			menu, MenuCreateCampaigns(folderName, title, subList, mode));
	}
	for (int i = 0; i < (int)list->list.size; i++)
	{
		MenuAddSubmenu(
			menu, MenuCreateCampaignItem(CArrayGet(&list->list, i), mode));
	}
	MenuSetCustomDisplay(menu, CampaignsDisplayFilename, NULL);
	return menu;
}

static menu_t *MenuCreateCampaignItem(
	CampaignEntry *entry, const GameMode mode)
{
	menu_t *menu = CreateStartGameMode(entry->Info, mode, entry);
	// Special colors:
	// - Green for new campaigns
	// - White (normal) for in-progress campaigns
	// - Grey for complete campaigns
	MissionSave m;
	AutosaveLoadMission(&gAutosave, &m, entry->Path);
	if (m.MissionsCompleted == entry->NumMissions)
	{
		// Completed campaign
		menu->color = colorGray;
	}
	else if (m.MissionsCompleted > 0)
	{
		// Campaign in progress
		menu->color = colorYellow;
	}

	return menu;
}

static void JoinLANGame(menu_t *menu, void *data);
static menu_t *CreateJoinLANGame(const char *name, MenuSystem *ms)
{
	menu_t *menu = MenuCreate(name, MENU_TYPE_RETURN);
	menu->enterSound = MENU_SOUND_START;
	MenuSetPostEnterFunc(menu, JoinLANGame, ms, false);
	return menu;
}
static void JoinLANGame(menu_t *menu, void *data)
{
	MenuSystem *ms = data;
	LOG(LM_MAIN, LL_INFO, "joining LAN game...");
	NetClientConnect(&gNetClient, NetClientLANAddress());
	if (!NetClientIsConnected(&gNetClient))
	{
		LOG(LM_MAIN, LL_INFO, "failed to connect to LAN game");
	}
	else
	{
		ScreenWaitForCampaignDef();
	}
	if (!gCampaign.IsLoaded)
	{
		// Don't activate the menu item
		ms->current = menu->parentMenu;
	}
}
static void CheckLANServers(menu_t *menu, void *data)
{
	CheckLANServerData *cdata = data;
	if (!gNetClient.FindingLANServer || !NetClientIsConnected(&gNetClient))
	{
		if (gNetClient.FoundLANServer)
		{
			MenuEnableSubmenu(menu, cdata->MenuJoinIndex);
		}
		else
		{
			// We've finished looking for LAN servers and haven't found any
			MenuDisableSubmenu(menu, cdata->MenuJoinIndex);
		}
		LOG(LM_MAIN, LL_DEBUG, "finding LAN server...");
		NetClientFindLANServers(&gNetClient);
	}
}

static menu_t *MenuCreateOptionsGraphics(const char *name, MenuSystem *ms);
static menu_t *MenuCreateOptionsControls(const char *name, MenuSystem *ms);

static menu_t *MenuCreateOptions(const char *name, MenuSystem *ms)
{
	menu_t *menu = MenuCreateNormal(
		name,
		"Options:",
		MENU_TYPE_NORMAL,
		0);
	MenuAddSubmenu(menu, MenuCreateConfigOptions(
		"Game...", "Game Options:", ConfigGet(&gConfig, "Game"), ms, true));
	MenuAddSubmenu(menu, MenuCreateOptionsGraphics("Graphics...", ms));
	MenuAddSubmenu(menu, MenuCreateConfigOptions(
		"Interface...", "Interface Options:",
		ConfigGet(&gConfig, "Interface"), ms, true));
	MenuAddSubmenu(menu, MenuCreateOptionsControls("Controls...", ms));
	MenuAddSubmenu(menu, MenuCreateConfigOptions(
		"Sound...", "Configure Sound:", ConfigGet(&gConfig, "Sound"), ms,
		true));
	MenuAddSubmenu(menu, MenuCreateConfigOptions(
		"Quick Play...", "Quick Play Options:", ConfigGet(&gConfig, "QuickPlay"), ms,
		true));
	MenuAddSubmenu(menu, MenuCreateSeparator(""));
	MenuAddSubmenu(menu, MenuCreateBack("Back"));
	return menu;
}

menu_t *MenuCreateOptionsGraphics(const char *name, MenuSystem *ms)
{
	menu_t *menu = MenuCreateNormal(
		name,
		"Graphics Options:",
		MENU_TYPE_OPTIONS,
		0);
	MenuAddConfigOptionsItem(menu, ConfigGet(&gConfig, "Graphics.Brightness"));
#ifndef __GCWZERO__
#ifndef __ANDROID__
	MenuAddConfigOptionsItem(menu, ConfigGet(&gConfig, "Graphics.Fullscreen"));
#endif	// ANDROID

	MenuAddSubmenu(
		menu,
		MenuCreateOptionUpDownFunc(
			"Resolution",
			Gfx_ModePrev,
			Gfx_ModeNext,
			MENU_OPTION_DISPLAY_STYLE_STR_FUNC,
			GrafxGetModeStr));
	MenuAddConfigOptionsItem(menu, ConfigGet(&gConfig, "Graphics.ScaleMode"));
#endif	// GCWZERO
	MenuAddSubmenu(menu, MenuCreateSeparator(""));
	MenuAddSubmenu(menu, MenuCreateBack("Done"));
	MenuSetPostInputFunc(menu, PostInputConfigApply, ms);
	return menu;
}

menu_t *MenuCreateKeys(const char *name, MenuSystem *ms);

menu_t *MenuCreateOptionsControls(const char *name, MenuSystem *ms)
{
	menu_t *menu = MenuCreateNormal(
		name,
		"Configure Controls:",
		MENU_TYPE_OPTIONS,
		0);
#ifndef __ANDROID__
	MenuAddSubmenu(menu, MenuCreateKeys("Redefine keys...", ms));
#endif
	MenuAddSubmenu(menu, MenuCreateSeparator(""));
	MenuAddSubmenu(menu, MenuCreateBack("Done"));
	MenuSetPostInputFunc(menu, PostInputConfigApply, ms);
	return menu;
}

menu_t *MenuCreateQuit(const char *name)
{
	return MenuCreate(name, MENU_TYPE_QUIT);
}


static void MenuCreateKeysSingleSection(
	menu_t *menu, const char *sectionName, const int playerIndex);
static menu_t *MenuCreateOptionChangeKey(
	const key_code_e code, const int playerIndex);

menu_t *MenuCreateKeys(const char *name, MenuSystem *ms)
{
	menu_t *menu = MenuCreateNormal(
		name,
		"",
		MENU_TYPE_KEYS,
		0);
	MenuCreateKeysSingleSection(menu, "Keyboard 1", 0);
	MenuCreateKeysSingleSection(menu, "Keyboard 2", 1);
	MenuAddSubmenu(menu, MenuCreateOptionChangeKey(KEY_CODE_MAP, 0));
	MenuAddSubmenu(menu, MenuCreateSeparator(""));
	MenuAddSubmenu(menu, MenuCreateBack("Done"));
	MenuSetPostInputFunc(menu, PostInputConfigApply, ms);
	return menu;
}

static void MenuCreateKeysSingleSection(
	menu_t *menu, const char *sectionName, const int playerIndex)
{
	MenuAddSubmenu(menu, MenuCreateSeparator(sectionName));
	MenuAddSubmenu(
		menu, MenuCreateOptionChangeKey(KEY_CODE_LEFT, playerIndex));
	MenuAddSubmenu(
		menu, MenuCreateOptionChangeKey(KEY_CODE_RIGHT, playerIndex));
	MenuAddSubmenu(
		menu, MenuCreateOptionChangeKey(KEY_CODE_UP, playerIndex));
	MenuAddSubmenu(
		menu, MenuCreateOptionChangeKey(KEY_CODE_DOWN, playerIndex));
	MenuAddSubmenu(
		menu, MenuCreateOptionChangeKey(KEY_CODE_BUTTON1, playerIndex));
	MenuAddSubmenu(
		menu, MenuCreateOptionChangeKey(KEY_CODE_BUTTON2, playerIndex));
	MenuAddSubmenu(menu, MenuCreateSeparator(""));
}

static menu_t *MenuCreateOptionChangeKey(
	const key_code_e code, const int playerIndex)
{
	menu_t *menu =
		MenuCreate(KeycodeStr(code), MENU_TYPE_SET_OPTION_CHANGE_KEY);
	menu->u.changeKey.code = code;
	menu->u.changeKey.playerIndex = playerIndex;
	return menu;
}
