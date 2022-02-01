/*
	Copyright (c) 2013-2022 Cong Xu
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

#include <cdogs/actor_placement.h>
#include <cdogs/ai.h>
#include <cdogs/config.h>
#include <cdogs/config_io.h>
#include <cdogs/files.h>
#include <cdogs/font.h>
#include <cdogs/grafx_bg.h>
#include <cdogs/handle_game_events.h>
#include <cdogs/log.h>
#include <cdogs/los.h>
#include <cdogs/map_build.h>
#include <cdogs/music.h>
#include <cdogs/net_client.h>
#include <cdogs/net_server.h>
#include <cdogs/quick_play.h>

#include "autosave.h"
#include "briefing_screens.h"
#include "game.h"
#include "loading_screens.h"
#include "menu.h"
#include "prep.h"

typedef struct
{
	MenuSystem ms;
	GraphicsDevice *graphics;
	credits_displayer_t creditsDisplayer;
	CustomCampaigns campaigns;
	GameMode lastGameMode;
	bool wasClient;
	DrawBuffer buffer;
	HSV bgTint;
	RunGameData rData;
} MainMenuData;
static void MenuResetSize(MenuSystem *ms);
static void MenuCreateAll(
	MainMenuData *data, LoopRunner *l, EventHandlers *handlers);
static void MainMenuReset(MainMenuData *data);
static void MainMenuTerminate(GameLoopData *data);
static void MainMenuOnEnter(GameLoopData *data);
static void MainMenuOnExit(GameLoopData *data);
static GameLoopResult MainMenuUpdate(GameLoopData *data, LoopRunner *l);
static void MainMenuDraw(GameLoopData *data);
GameLoopData *MainMenu(GraphicsDevice *graphics, LoopRunner *l)
{
	MainMenuData *data;
	CMALLOC(data, sizeof *data);
	data->graphics = graphics;
	memset(&data->creditsDisplayer, 0, sizeof data->creditsDisplayer);
	LoadCredits(&data->creditsDisplayer, colorPurple, colorDarker);
	memset(&data->campaigns, 0, sizeof data->campaigns);
	LoadAllCampaigns(&data->campaigns);
	data->lastGameMode = GAME_MODE_QUICK_PLAY;
	data->wasClient = false;
	MenuCreateAll(data, l, &gEventHandlers);
	MenuSetCreditsDisplayer(&data->ms, &data->creditsDisplayer);

	return GameLoopDataNew(
		data, MainMenuTerminate, MainMenuOnEnter, MainMenuOnExit, NULL,
		MainMenuUpdate, MainMenuDraw);
}
static void GenerateLiveBackground(MainMenuData *data)
{
	MissionOptionsTerminate(&gMission);
	CampaignTerminate(&gCampaign);

	CampaignSettingInit(&gCampaign.Setting);
	SetupQuickPlayCampaign(&gCampaign.Setting, true);
	CampaignAndMissionSetup(&gCampaign, &gMission);
	GameEventsInit(&gGameEvents);
	gCampaign.MissionIndex = 0;

	MapBuild(&gMap, gMission.missionData, true, gMission.index, GAME_MODE_NORMAL, &gCampaign.Setting.characters);

	// Add AI player
	GameEvent e = GameEventNew(GAME_EVENT_PLAYER_DATA);
	e.u.PlayerData = PlayerDataDefault(0);
	e.u.PlayerData.UID = gNetClient.FirstPlayerUID;
	GameEventsEnqueue(&gGameEvents, e);
	HandleGameEvents(&gGameEvents, NULL, NULL, NULL, NULL);
	CA_FOREACH(PlayerData, p, gPlayerDatas)
	p->inputDevice = INPUT_DEVICE_AI;
	PlacePlayer(&gMap, p, svec2_zero(), true);
	CA_FOREACH_END()

	const HSV tint = {rand() * 360.0 / RAND_MAX, rand() * 1.0 / RAND_MAX, 0.5};
	data->bgTint = tint;
	DrawBufferInit(&data->buffer, svec2i(X_TILES, Y_TILES), data->graphics);
	InitializeBadGuys();
	CreateEnemies();
	MapMarkAllAsVisited(&gMap);

	GameInit(&data->rData, &gCampaign, &gMission, &gMap);
}
static void MainMenuReset(MainMenuData *data)
{
	GenerateLiveBackground(data);

	MenuResetSize(&data->ms);
}
static void MainMenuTerminate(GameLoopData *data)
{
	MainMenuData *mData = data->Data;

	MenuSystemTerminate(&mData->ms);
	UnloadCredits(&mData->creditsDisplayer);
	UnloadAllCampaigns(&mData->campaigns);
	CFREE(mData);
}
static menu_t *FindSubmenuByName(menu_t *menu, const char *name);
static void MainMenuOnEnter(GameLoopData *data)
{
	MainMenuData *mData = data->Data;

	if (gCampaign.IsLoaded)
	{
		// Loaded game already; skip menus and go straight to game
		return;
	}

	MusicPlayGeneral(&gSoundDevice.music, MUSIC_MENU);
	// Reset config - could have been set to other values by server
	ConfigResetChanged(&gConfig);
	CampaignSettingTerminateAll(&gCampaign.Setting);

	MainMenuReset(mData);
	NetClientDisconnect(&gNetClient);
	NetServerClose(&gNetServer);
	GameEventsTerminate(&gGameEvents);

	// Auto-enter the submenu corresponding to the last game mode
	menu_t *startMenu = FindSubmenuByName(mData->ms.root, "Start");
	if (mData->wasClient)
	{
		mData->ms.current = startMenu;
	}
	else
	{
		switch (mData->lastGameMode)
		{
		case GAME_MODE_NORMAL:
			mData->ms.current = FindSubmenuByName(startMenu, "Campaign");
			break;
		case GAME_MODE_DOGFIGHT:
			mData->ms.current = FindSubmenuByName(startMenu, "Dogfight");
			break;
		case GAME_MODE_DEATHMATCH:
			mData->ms.current = FindSubmenuByName(startMenu, "Deathmatch");
			break;
		default:
			mData->ms.current = mData->ms.root;
			break;
		}
	}
}
static menu_t *FindSubmenuByName(menu_t *menu, const char *name)
{
	CASSERT(menu->type == MENU_TYPE_NORMAL, "invalid menu type");
	CA_FOREACH(menu_t, submenu, menu->u.normal.subMenus)
	if (strcmp(submenu->name, name) == 0)
		return submenu;
	CA_FOREACH_END()
	return menu;
}
static void MainMenuOnExit(GameLoopData *data)
{
	MainMenuData *mData = data->Data;

	// Reset player datas
	PlayerDataTerminate(&gPlayerDatas);
	PlayerDataInit(&gPlayerDatas);
	// Initialise game events; we need this for init as well as the game
	GameEventsInit(&gGameEvents);

	mData->lastGameMode = gCampaign.Entry.Mode;
	mData->wasClient = gCampaign.IsClient;
}
static GameLoopResult MainMenuUpdate(GameLoopData *data, LoopRunner *l)
{
	MainMenuData *mData = data->Data;

	if (gCampaign.IsLoaded)
	{
		// Loaded game already; skip menus and go straight to game
		LoopRunnerPush(l, ScreenCampaignIntro(&gCampaign.Setting));
		return UPDATE_RESULT_OK;
	}

	LOSSetAllVisible(&mData->rData.map->LOS);
	GameUpdate(&mData->rData, 1, NULL);

	const GameLoopResult result = MenuUpdate(&mData->ms);
	if (result == UPDATE_RESULT_OK)
	{
		if (gCampaign.IsLoaded)
		{
			LoopRunnerPush(l, ScreenCampaignIntro(&gCampaign.Setting));
		}
		else
		{
			LoopRunnerPop(l);
		}
	}
	if (gEventHandlers.HasResolutionChanged)
	{
		MainMenuReset(mData);
	}
	return result;
}
static void MainMenuDraw(GameLoopData *data)
{
	MainMenuData *mData = data->Data;
	MenuDraw(&mData->ms);
	const struct vec2 pos =
		Vec2CenterOfTile(svec2i_scale_divide(gMap.Size, 2));
	DrawBufferArgs args;
	memset(&args, 0, sizeof args);
	GrafxDrawBackground(
		mData->graphics, &mData->buffer, mData->bgTint, pos, &args);
}

static menu_t *MenuCreateStart(
	const char *name, MenuSystem *ms, LoopRunner *l,
	CustomCampaigns *campaigns);
static menu_t *MenuCreateOptions(const char *name, MainMenuData *data);
menu_t *MenuCreateQuit(const char *name);

static void MenuCreateAll(
	MainMenuData *data, LoopRunner *l, EventHandlers *handlers)
{
	MenuSystemInit(
		&data->ms, handlers, data->graphics, svec2i_zero(),
		data->graphics->cachedConfig.Res);
	data->ms.root = data->ms.current = MenuCreateNormal(
		"", "", MENU_TYPE_NORMAL,
		MENU_DISPLAY_ITEMS_CREDITS | MENU_DISPLAY_ITEMS_AUTHORS);
	MenuAddSubmenu(
		data->ms.root,
		MenuCreateStart("Start", &data->ms, l, &data->campaigns));
	MenuAddSubmenu(data->ms.root, MenuCreateOptions("Options...", data));
#ifndef __EMSCRIPTEN__
	MenuAddSubmenu(data->ms.root, MenuCreateQuit("Quit"));
	MenuAddExitType(&data->ms, MENU_TYPE_QUIT);
#endif
	MenuAddExitType(&data->ms, MENU_TYPE_RETURN);
}

typedef struct
{
	// The index of the join game menu item
	// so we can enable it if LAN servers are found
	int MenuJoinIndex;
} CheckLANServerData;
static menu_t *MenuCreateContinue(const char *name, const CampaignEntry *entry);
static menu_t *MenuCreateQuickPlay(const char *name, const CampaignEntry *entry);
static menu_t *MenuCreateCampaigns(
	const char *name, const char *title, CampaignList *list,
	const GameMode mode);
static menu_t *CreateJoinLANGame(
	const char *name, const char *title, MenuSystem *ms, LoopRunner *l);
static void CheckLANServers(menu_t *menu, void *data);
static menu_t *MenuCreateStart(
	const char *name, MenuSystem *ms, LoopRunner *l,
	CustomCampaigns *campaigns)
{
	menu_t *menu = MenuCreateNormal(name, "Start:", MENU_TYPE_NORMAL, 0);
	const CampaignSave *cs = AutosaveGetLastCampaign(&gAutosave);
	MenuAddSubmenu(
		menu, MenuCreateContinue("Continue", &cs->Campaign));
	const int menuContinueIndex = (int)menu->u.normal.subMenus.size - 1;
	MenuAddSubmenu(
		menu, MenuCreateCampaigns(
				  "Campaign", "Select a campaign:", &campaigns->campaignList,
				  GAME_MODE_NORMAL));
	MenuAddSubmenu(
		menu, MenuCreateQuickPlay("Quick Play", &campaigns->quickPlayEntry));
	MenuAddSubmenu(
		menu, MenuCreateCampaigns(
				  "Dogfight", "Select a dogfight scenario:",
				  &campaigns->dogfightList, GAME_MODE_DOGFIGHT));
	MenuAddSubmenu(
		menu, MenuCreateCampaigns(
				  "Deathmatch", "Select a deathmatch scenario:",
				  &campaigns->dogfightList, GAME_MODE_DEATHMATCH));
	MenuAddSubmenu(
		menu, CreateJoinLANGame("Join LAN game", "Choose LAN server", ms, l));
	CheckLANServerData *cdata;
	CMALLOC(cdata, sizeof *cdata);
	cdata->MenuJoinIndex = (int)menu->u.normal.subMenus.size - 1;
	MenuAddSubmenu(menu, MenuCreateSeparator(""));
	MenuAddSubmenu(menu, MenuCreateBack("Back"));

	if (!CampaignSaveIsValid(cs))
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
	const CampaignEntry *Entry;
} StartGameModeData;
static void StartGameMode(menu_t *menu, void *data);

static menu_t *CreateStartGameMode(
	const char *name, GameMode mode, const CampaignEntry *entry)
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
	LoadingScreenReload(&gLoadingScreen);
	LoadingScreenDraw(&gLoadingScreen, "Loading game...", 1.0f);
	UNUSED(menu);
	StartGameModeData *mData = data;
	gCampaign.Entry.Mode = mData->GameMode;
	if (!CampaignLoad(&gCampaign, mData->Entry))
	{
		// Failed to load
		printf("Error: cannot load campaign %s\n", mData->Entry->Info);
	}
}
static menu_t *MenuCreateContinue(const char *name, const CampaignEntry *entry)
{
	return CreateStartGameMode(name, GAME_MODE_NORMAL, entry);
}
static menu_t *MenuCreateQuickPlay(const char *name, const CampaignEntry *entry)
{
	return CreateStartGameMode(name, GAME_MODE_QUICK_PLAY, entry);
}

static menu_t *MenuCreateCampaignItem(
	CampaignEntry *entry, const GameMode mode);

static void CampaignsDisplayFilename(
	const menu_t *menu, GraphicsDevice *g, const struct vec2i pos,
	const struct vec2i size, const void *data)
{
	if (menu->u.normal.subMenus.size == 0)
	{
		return;
	}
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
	const char *name, const char *title, CampaignList *list,
	const GameMode mode)
{
	menu_t *menu = MenuCreateNormal(name, title, MENU_TYPE_NORMAL, 0);
	menu->u.normal.maxItems = 20;
	menu->u.normal.align = MENU_ALIGN_CENTER;
	CA_FOREACH(CampaignList, subList, list->subFolders)
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
	const CampaignSave *m = AutosaveGetCampaign(&gAutosave, entry->Path);
	if (m != NULL)
	{
		if ((int)m->MissionsCompleted.size == entry->NumMissions)
		{
			// Completed campaign
			menu->color = colorGray;
		}
		else if (m->MissionsCompleted.size > 0)
		{
			// Campaign in progress
			menu->color = colorYellow;
		}
	}

	return menu;
}

typedef struct
{
	MenuSystem *MS;
	LoopRunner *L;
} CreateJoinLANGameData;
static void CreateLANServerMenuItems(menu_t *menu, void *data);
static menu_t *CreateJoinLANGame(
	const char *name, const char *title, MenuSystem *ms, LoopRunner *l)
{
	menu_t *menu = MenuCreateNormal(name, title, MENU_TYPE_NORMAL, 0);
	// We'll create our menu items dynamically after entering
	// Creating an item for each scanned server address
	CreateJoinLANGameData *data;
	CMALLOC(data, sizeof *data);
	data->MS = ms;
	data->L = l;
	MenuSetPostEnterFunc(menu, CreateLANServerMenuItems, data, true);
	return menu;
}
typedef struct
{
	MenuSystem *MS;
	LoopRunner *L;
	int AddrIndex;
} JoinLANGameData;
static void JoinLANGame(menu_t *menu, void *data);
static void CreateLANServerMenuItems(menu_t *menu, void *data)
{
	CreateJoinLANGameData *cData = data;

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
	// e.g. "Bob's Server (123.45.67.89:12345) - Campaign: Ogre Rampage #4, p:
	// 4/16 350ms"
	sprintf(
		buf, "%s (%s:%u) - %s: %s (# %d), p: %d/%d %dms",
		si->ServerInfo.Hostname, ipbuf, si->Addr.port,
		GameModeStr(si->ServerInfo.GameMode), si->ServerInfo.CampaignName,
		si->ServerInfo.MissionNumber, si->ServerInfo.NumPlayers,
		si->ServerInfo.MaxPlayers, si->LatencyMS);
	menu_t *serverMenu = MenuCreate(buf, MENU_TYPE_RETURN);
	serverMenu->enterSound = MENU_SOUND_START;
	JoinLANGameData *jdata;
	CMALLOC(jdata, sizeof *jdata);
	jdata->MS = cData->MS;
	jdata->L = cData->L;
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
	const ScanInfo *sinfo =
		CArrayGet(&gNetClient.ScannedAddrs, jdata->AddrIndex);
	LOG(LM_MAIN, LL_INFO, "joining LAN game...");
	if (NetClientTryConnect(&gNetClient, sinfo->Addr))
	{
		LoopRunnerPush(jdata->L, ScreenWaitForCampaignDef());
		goto bail;
	}
	else
	{
		LOG(LM_MAIN, LL_INFO, "failed to connect to LAN game");
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

static void PostInputConfigApply(menu_t *menu, int cmd, void *data)
{
	UNUSED(menu);
	UNUSED(cmd);
	MainMenuData *mData = data;
	bool resetBg = false;
	if (!ConfigApply(&gConfig, &resetBg))
	{
		LOG(LM_MAIN, LL_ERROR, "Failed to apply config; reset to last used");
		ConfigResetChanged(&gConfig);
	}
	else
	{
		// Save config immediately
		ConfigSave(&gConfig, GetConfigFilePath(CONFIG_FILE));
		if (resetBg)
		{
			GenerateLiveBackground(mData);
		}
	}

	MenuResetSize(&mData->ms);
}

static menu_t *MenuCreateConfigOptions(
	const char *name, const char *title, const Config *c, MainMenuData *data,
	const bool backOrReturn)
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

static menu_t *MenuCreateOptionsGraphics(const char *name, MainMenuData *data);
#if !defined(__ANDROID__) && !defined(__GCWZERO__)
static menu_t *MenuCreateOptionsControls(const char *name, MainMenuData *data);
#endif

static menu_t *MenuCreateOptions(const char *name, MainMenuData *data)
{
	menu_t *menu = MenuCreateNormal(name, "Options:", MENU_TYPE_NORMAL, 0);
	MenuAddSubmenu(
		menu, MenuCreateConfigOptions(
				  "Game...", "Game Options:", ConfigGet(&gConfig, "Game"),
				  data, true));
	MenuAddSubmenu(menu, MenuCreateOptionsGraphics("Graphics...", data));
	MenuAddSubmenu(
		menu, MenuCreateConfigOptions(
				  "Interface...", "Interface Options:",
				  ConfigGet(&gConfig, "Interface"), data, true));
#if !defined(__ANDROID__) && !defined(__GCWZERO__)
	MenuAddSubmenu(menu, MenuCreateOptionsControls("Controls...", data));
#endif
	MenuAddSubmenu(
		menu, MenuCreateConfigOptions(
				  "Sound...", "Configure Sound:", ConfigGet(&gConfig, "Sound"),
				  data, true));
	MenuAddSubmenu(
		menu, MenuCreateConfigOptions(
				  "Quick Play...", "Quick Play Options:",
				  ConfigGet(&gConfig, "QuickPlay"), data, true));
	MenuAddSubmenu(menu, MenuCreateSeparator(""));
	MenuAddSubmenu(menu, MenuCreateBack("Back"));
	return menu;
}

static menu_t *MenuCreateOptionsGraphics(const char *name, MainMenuData *data)
{
	menu_t *menu =
		MenuCreateNormal(name, "Graphics Options:", MENU_TYPE_OPTIONS, 0);
	MenuAddConfigOptionsItem(menu, ConfigGet(&gConfig, "Graphics.Brightness"));
#ifndef __GCWZERO__
#ifndef __ANDROID__
	MenuAddConfigOptionsItem(menu, ConfigGet(&gConfig, "Graphics.Fullscreen"));
	// TODO: fix second window rendering
	// MenuAddConfigOptionsItem(
	//	menu, ConfigGet(&gConfig, "Graphics.SecondWindow"));
#endif // ANDROID

	MenuAddConfigOptionsItem(
		menu, ConfigGet(&gConfig, "Graphics.ScaleFactor"));
	MenuAddConfigOptionsItem(menu, ConfigGet(&gConfig, "Graphics.ScaleMode"));
#endif // GCWZERO
	MenuAddConfigOptionsItem(menu, ConfigGet(&gConfig, "Graphics.Shadows"));
	MenuAddConfigOptionsItem(menu, ConfigGet(&gConfig, "Graphics.Gore"));
	MenuAddConfigOptionsItem(menu, ConfigGet(&gConfig, "Graphics.Brass"));
	MenuAddSubmenu(menu, MenuCreateSeparator(""));
	MenuAddSubmenu(menu, MenuCreateBack("Done"));
	MenuSetPostInputFunc(menu, PostInputConfigApply, data);
	return menu;
}

static menu_t *MenuCreateKeys(const char *name, MainMenuData *data);

#if !defined(__ANDROID__) && !defined(__GCWZERO__)
static menu_t *MenuCreateOptionsControls(const char *name, MainMenuData *data)
{
	menu_t *menu =
		MenuCreateNormal(name, "Configure Controls:", MENU_TYPE_OPTIONS, 0);
	MenuAddSubmenu(menu, MenuCreateKeys("Redefine keys...", data));
	MenuAddSubmenu(menu, MenuCreateSeparator(""));
	MenuAddSubmenu(menu, MenuCreateBack("Done"));
	MenuSetPostInputFunc(menu, PostInputConfigApply, data);
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
	const key_code_e code, const int playerIndex, const bool isOptional);

static menu_t *MenuCreateKeys(const char *name, MainMenuData *data)
{
	menu_t *menu = MenuCreateNormal(name, "", MENU_TYPE_OPTIONS, 0);
	MenuCreateKeysSingleSection(menu, "Keyboard 1", 0);
	MenuCreateKeysSingleSection(menu, "Keyboard 2", 1);
	MenuAddSubmenu(menu, MenuCreateOptionChangeKey(KEY_CODE_MAP, 0, true));
	MenuAddSubmenu(menu, MenuCreateSeparator(""));
	MenuAddSubmenu(menu, MenuCreateBack("Done"));
	MenuSetPostInputFunc(menu, PostInputConfigApply, data);
	return menu;
}

static void MenuCreateKeysSingleSection(
	menu_t *menu, const char *sectionName, const int playerIndex)
{
	MenuAddSubmenu(menu, MenuCreateSeparator(sectionName));
	MenuAddSubmenu(
		menu, MenuCreateOptionChangeKey(KEY_CODE_LEFT, playerIndex, false));
	MenuAddSubmenu(
		menu, MenuCreateOptionChangeKey(KEY_CODE_RIGHT, playerIndex, false));
	MenuAddSubmenu(
		menu, MenuCreateOptionChangeKey(KEY_CODE_UP, playerIndex, false));
	MenuAddSubmenu(
		menu, MenuCreateOptionChangeKey(KEY_CODE_DOWN, playerIndex, false));
	MenuAddSubmenu(
		menu, MenuCreateOptionChangeKey(KEY_CODE_BUTTON1, playerIndex, false));
	MenuAddSubmenu(
		menu, MenuCreateOptionChangeKey(KEY_CODE_BUTTON2, playerIndex, false));
	MenuAddSubmenu(
		menu, MenuCreateOptionChangeKey(KEY_CODE_GRENADE, playerIndex, true));
	MenuAddSubmenu(menu, MenuCreateSeparator(""));
}

static menu_t *MenuCreateOptionChangeKey(
	const key_code_e code, const int playerIndex, const bool isOptional)
{
	menu_t *menu =
		MenuCreate(KeycodeStr(code), MENU_TYPE_SET_OPTION_CHANGE_KEY);
	menu->u.changeKey.code = code;
	menu->u.changeKey.playerIndex = playerIndex;
	menu->u.changeKey.isOptional = isOptional;
	return menu;
}

static void MenuResetSize(MenuSystem *ms)
{
	// Update menu system so that resolution changes don't
	// affect menu positions
	ms->pos = svec2i_zero();
	ms->size = svec2i(
		ms->graphics->cachedConfig.Res.x, ms->graphics->cachedConfig.Res.y);
}
