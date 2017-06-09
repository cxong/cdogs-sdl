/*
    Copyright (c) 2013-2016, Cong Xu
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


void MenuCreateAll(
	MenuSystem *ms,
	custom_campaigns_t *campaigns,
	EventHandlers *handlers,
	GraphicsDevice *graphics);

static menu_t *FindSubmenuByName(menu_t *menu, const char *name);
void MainMenu(
	GraphicsDevice *graphics,
	credits_displayer_t *creditsDisplayer,
	custom_campaigns_t *campaigns,
	const GameMode lastGameMode, const bool wasClient)
{
	MenuSystem ms;
	MenuCreateAll(&ms, campaigns, &gEventHandlers, graphics);
	MenuSetCreditsDisplayer(&ms, creditsDisplayer);
	// Auto-enter the submenu corresponding to the last game mode
	menu_t *startMenu = FindSubmenuByName(ms.root, "Start");
	if (wasClient)
	{
		ms.current = startMenu;
	}
	else
	{
		switch (lastGameMode)
		{
		case GAME_MODE_NORMAL:
			ms.current = FindSubmenuByName(startMenu, "Campaign");
			break;
		case GAME_MODE_DOGFIGHT:
			ms.current = FindSubmenuByName(startMenu, "Dogfight");
			break;
		case GAME_MODE_DEATHMATCH:
			ms.current = FindSubmenuByName(startMenu, "Deathmatch");
			break;
		default:
			// Do nothing
			break;
		}
	}
	MenuLoop(&ms);

	MenuSystemTerminate(&ms);
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

void MenuCreateAll(
	MenuSystem *ms,
	custom_campaigns_t *campaigns,
	EventHandlers *handlers,
	GraphicsDevice *graphics)
{
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
	MenuAddSubmenu(ms->root, MenuCreateStart("Start", ms, campaigns));
	MenuAddSubmenu(ms->root, MenuCreateOptions("Options...", ms));
	MenuAddSubmenu(ms->root, MenuCreateQuit("Quit"));
	MenuAddExitType(ms, MENU_TYPE_QUIT);
	MenuAddExitType(ms, MENU_TYPE_RETURN);
}

typedef struct
{
	// The index of the join game menu item
	// so we can enable it if LAN servers are found
	int MenuJoinIndex;
} CheckLANServerData;
static menu_t *MenuCreateContinue(const char *name, CampaignEntry *entry);
static menu_t *MenuCreateQuickPlay(const char *name, CampaignEntry *entry);
static menu_t *MenuCreateCampaigns(
	const char *name, const char *title,
	campaign_list_t *list, const GameMode mode);
static menu_t *CreateJoinLANGame(
	const char *name, const char *title, MenuSystem *ms);
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
	MenuAddSubmenu(
		menu, CreateJoinLANGame("Join LAN game", "Choose LAN server", ms));
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
	CA_FOREACH(campaign_list_t, subList, list->subFolders)
		char folderName[CDOGS_FILENAME_MAX];
		sprintf(folderName, "%s/", subList->Name);
		MenuAddSubmenu(
			menu, MenuCreateCampaigns(folderName, title, subList, mode));
	CA_FOREACH_END()
	CA_FOREACH(CampaignEntry, e, list->list)
		MenuAddSubmenu(menu, MenuCreateCampaignItem(e, mode));
	CA_FOREACH_END()
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

static void CreateLANServerMenuItems(menu_t *menu, void *data);
static menu_t *CreateJoinLANGame(
	const char *name, const char *title, MenuSystem *ms)
{
	menu_t *menu = MenuCreateNormal(name, title, MENU_TYPE_NORMAL, 0);
	// We'll create our menu items dynamically after entering
	// Creating an item for each scanned server address
	MenuSetPostEnterFunc(menu, CreateLANServerMenuItems, ms, false);
	return menu;
}
typedef struct
{
	MenuSystem *MS;
	int AddrIndex;
} JoinLANGameData;
static void JoinLANGame(menu_t *menu, void *data);
static void CreateLANServerMenuItems(menu_t *menu, void *data)
{
	MenuSystem *ms = data;

	// Clear and recreate all menu items
	MenuClearSubmenus(menu);
	CA_FOREACH(ScanInfo, si, gNetClient.ScannedAddrs)
		char buf[512];
		char ipbuf[256];
		if (enet_address_get_host_ip(&si->Addr, ipbuf, sizeof ipbuf) < 0)
		{
			LOG(LM_MAIN, LL_WARN, "cannot find host ip");
			ipbuf[0] = '?';
			ipbuf[1] = '\0';
		};
		// e.g. "Bob's Server (123.45.67.89:12345) - Campaign: Ogre Rampage #4, p: 4/16 350ms"
		sprintf(buf, "%s (%s:%u) - %s: %s (# %d), p: %d/%d %dms",
			si->ServerInfo.Hostname, ipbuf, si->Addr.port,
			GameModeStr(si->ServerInfo.GameMode),
			si->ServerInfo.CampaignName, si->ServerInfo.MissionNumber,
			si->ServerInfo.NumPlayers, si->ServerInfo.MaxPlayers,
			si->LatencyMS);
		menu_t *serverMenu = MenuCreate(buf, MENU_TYPE_RETURN);
		serverMenu->enterSound = MENU_SOUND_START;
		JoinLANGameData *jdata;
		CMALLOC(jdata, sizeof *jdata);
		jdata->MS = ms;
		jdata->AddrIndex = _ca_index;
		MenuSetPostEnterFunc(serverMenu, JoinLANGame, jdata, true);
		MenuAddSubmenu(menu, serverMenu);
	CA_FOREACH_END()
	MenuAddSubmenu(menu, MenuCreateSeparator(""));
	MenuAddSubmenu(menu, MenuCreateBack("Back"));
}
static void JoinLANGame(menu_t *menu, void *data)
{
	JoinLANGameData *jdata = data;
	if (jdata->AddrIndex >= (int)gNetClient.ScannedAddrs.size)
	{
		goto bail;
	}
	const ScanInfo *sinfo = CArrayGet(
		&gNetClient.ScannedAddrs, jdata->AddrIndex);
	LOG(LM_MAIN, LL_INFO, "joining LAN game...");
	if (NetClientTryConnect(&gNetClient, sinfo->Addr))
	{
		ScreenWaitForCampaignDef();
	}
	else
	{
		LOG(LM_MAIN, LL_INFO, "failed to connect to LAN game");
	}
	if (!gCampaign.IsLoaded)
	{
		goto bail;
	}
	return;

bail:
	// Don't activate the menu item
	jdata->MS->current = menu->parentMenu;
}
static void CheckLANServers(menu_t *menu, void *data)
{
	CheckLANServerData *cdata = data;
	if (gNetClient.ScannedAddrs.size > 0)
	{
		MenuEnableSubmenu(menu, cdata->MenuJoinIndex);
	}
	else
	{
		// We haven't found any LAN servers in the latest scan
		MenuDisableSubmenu(menu, cdata->MenuJoinIndex);
	}
	if (gNetClient.ScanTicks <= 0)
	{
		LOG(LM_MAIN, LL_DEBUG, "finding LAN server...");
		NetClientFindLANServers(&gNetClient);
	}
}

static menu_t *MenuCreateOptionsGraphics(const char *name, MenuSystem *ms);
#if !defined(__ANDROID__) && !defined(__GCWZERO__)
static menu_t *MenuCreateOptionsControls(const char *name, MenuSystem *ms);
#endif

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
#if !defined(__ANDROID__) && !defined(__GCWZERO__)
	MenuAddSubmenu(menu, MenuCreateOptionsControls("Controls...", ms));
#endif
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
	MenuAddConfigOptionsItem(menu, ConfigGet(&gConfig, "Graphics.Shadows"));
	MenuAddConfigOptionsItem(menu, ConfigGet(&gConfig, "Graphics.Gore"));
	MenuAddConfigOptionsItem(menu, ConfigGet(&gConfig, "Graphics.Brass"));
	MenuAddSubmenu(menu, MenuCreateSeparator(""));
	MenuAddSubmenu(menu, MenuCreateBack("Done"));
	MenuSetPostInputFunc(menu, PostInputConfigApply, ms);
	return menu;
}

menu_t *MenuCreateKeys(const char *name, MenuSystem *ms);

#if !defined(__ANDROID__) && !defined(__GCWZERO__)
menu_t *MenuCreateOptionsControls(const char *name, MenuSystem *ms)
{
	menu_t *menu = MenuCreateNormal(
		name,
		"Configure Controls:",
		MENU_TYPE_OPTIONS,
		0);
	MenuAddSubmenu(menu, MenuCreateKeys("Redefine keys...", ms));
	MenuAddSubmenu(menu, MenuCreateSeparator(""));
	MenuAddSubmenu(menu, MenuCreateBack("Done"));
	MenuSetPostInputFunc(menu, PostInputConfigApply, ms);
	return menu;
}
#endif

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
