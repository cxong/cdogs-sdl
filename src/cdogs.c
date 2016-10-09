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
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <SDL.h>
#ifdef __MINGW32__
// HACK: MinGW complains about redefinition of main
#undef main
#endif

#include <cdogs/ammo.h>
#include <cdogs/campaigns.h>
#include <cdogs/character_class.h>
#include <cdogs/collision.h>
#include <cdogs/config_io.h>
#include <cdogs/draw.h>
#include <cdogs/files.h>
#include <cdogs/font_utils.h>
#include <cdogs/grafx.h>
#include <cdogs/handle_game_events.h>
#include <cdogs/hiscores.h>
#include <cdogs/joystick.h>
#include <cdogs/keyboard.h>
#include <cdogs/log.h>
#include <cdogs/mission.h>
#include <cdogs/music.h>
#include <cdogs/net_client.h>
#include <cdogs/net_server.h>
#include <cdogs/objs.h>
#include <cdogs/palette.h>
#include <cdogs/particle.h>
#include <cdogs/pic_manager.h>
#include <cdogs/pickup.h>
#include <cdogs/pics.h>
#include <cdogs/player_template.h>
#include <cdogs/sounds.h>
#include <cdogs/SDL_JoystickButtonNames/SDL_joystickbuttonnames.h>
#include <cdogs/triggers.h>
#include <cdogs/utils.h>

#include "autosave.h"
#include "credits.h"
#include "mainmenu.h"
#include "player_select_menus.h"
#include "prep.h"
#include "screens.h"
#include "XGetopt.h"


void MainLoop(credits_displayer_t *creditsDisplayer, custom_campaigns_t *campaigns)
{
	GameMode lastGameMode = GAME_MODE_QUICK_PLAY;
	bool wasClient = false;
	for (;;)
	{
		GrafxMakeRandomBackground(
			&gGraphicsDevice, &gCampaign, &gMission, &gMap);
		if (!gCampaign.IsLoaded)
		{
			MainMenu(
				&gGraphicsDevice, creditsDisplayer, campaigns,
				lastGameMode, wasClient);
		}
		if (!gCampaign.IsLoaded)
		{
			break;
		}
		lastGameMode = gCampaign.Entry.Mode;
		wasClient = gCampaign.IsClient;
		ScreenStart();
		CampaignSettingTerminate(&gCampaign.Setting);
	}
	debug(D_NORMAL, ">> Leaving Main Game Loop\n");

	// Close net connection
	NetServerTerminate(&gNetServer);
}

void PrintTitle(void)
{
	printf("C-Dogs SDL %s\n", CDOGS_SDL_VERSION);

	printf("Original Code Copyright Ronny Wester 1995\n");
	printf("Game Data Copyright Ronny Wester 1995\n");
	printf("SDL Port by Jeremy Chin, Lucas Martin-King and Cong Xu, Copyright 2003-2016\n\n");
}

static void PrintHelp(void)
{
	printf("%s\n",
		"Video Options:\n"
		"    --fullscreen     Try and use a fullscreen video mode.\n"
		"    --scale=n        Scale the window resolution up by a factor of n\n"
		"                       Factors: 2, 3, 4\n"
		"    --screen=WxH     Set virtual screen width to W x H\n"
		"    --nohud          Disable in-game HUD; useful for recording\n"
	);

	printf("%s\n",
		"Sound Options:\n"
		"    --nosound        Disable sound\n"
	);

	printf("%s\n",
		"Control Options:\n"
		"    --nojoystick     Disable joystick(s)\n"
	);

	printf("%s\n",
		"Game Options:\n"
		"    --wait           Wait for a key hit before initialising video.\n"
		"    --shakemult=n    Screen shaking multiplier (0 = disable).\n"
	);

	printf(
		"Logging: logging is enabled per module and set at certain levels.\n"
		"Log modules are: "
	);
	for (int i = 0; i < (int)LM_COUNT; i++)
	{
		printf("%s", LogModuleName((LogModule)i));
		if (i < (int)LM_COUNT - 1) printf(", ");
		else printf("\n");
	}
	printf("Log levels are: ");
	for (int i = 0; i < (int)LL_COUNT; i++)
	{
		printf("%s", LogLevelName((LogLevel)i));
		if (i < (int)LL_COUNT - 1) printf(", ");
		else printf("\n");
	}
	printf(
		"    --log=M,L        Enable logging for module M at level L.\n\n"
	);
	printf(
		"    --log=L          Enable logging for all modules at level L.\n\n"
	);

	printf("%s\n",
		"Other:\n"
		"    --connect=host   (Experimental) connect to a game server\n"
		);

	printf("%s\n",
		"The DEBUG environment variable can be set to show debug information.");

	printf(
		"The DEBUG_LEVEL environment variable can be set to between %d and %d.\n", D_NORMAL, D_MAX
		);
}

int main(int argc, char *argv[])
{
	int wait = 0;
	int controllerFlag = SDL_INIT_GAMECONTROLLER;
	credits_displayer_t creditsDisplayer;
	memset(&creditsDisplayer, 0, sizeof creditsDisplayer);
	custom_campaigns_t campaigns;
	memset(&campaigns, 0, sizeof campaigns);
	int err = 0;
	const char *loadCampaign = NULL;
	ENetAddress connectAddr;
	memset(&connectAddr, 0, sizeof connectAddr);

	srand((unsigned int)time(NULL));
	LogInit();

	PrintTitle();

	if (getenv("DEBUG") != NULL)
	{
		debug = true;
		char *dbg;
		if ((dbg = getenv("DEBUG_LEVEL")) != NULL)
		{
			debug_level = CLAMP(atoi(dbg), D_NORMAL, D_MAX);
		}
	}

	SetupConfigDir();
	gConfig = ConfigLoad(GetConfigFilePath(CONFIG_FILE));
	// Set config options that are only set via command line
	ConfigGet(&gConfig, "Graphics.ShowHUD")->u.Bool.Value = true;
	ConfigGet(&gConfig, "Graphics.ShakeMultiplier")->u.Int.Value = 1;

	LoadCredits(&creditsDisplayer, colorPurple, colorDarker);
	AutosaveInit(&gAutosave);
	AutosaveLoad(&gAutosave, GetConfigFilePath(AUTOSAVE_FILE));

	if (enet_initialize() != 0)
	{
		LOG(LM_MAIN, LL_ERROR, "An error occurred while initializing ENet.");
		err = EXIT_FAILURE;
		goto bail;
	}
	NetClientInit(&gNetClient);

	// Print command line
	char buf[CDOGS_PATH_MAX];
	buf[0] = '\0';
	for (int i = 0; i < argc; i++)
	{
		strcat(buf, " ");
		// HACK: for OS X, blank out the -psn_XXXX argument so that it doesn't
		// break arg parsing
	#ifdef __APPLE__
		if (strncmp(argv[i], "-psn", strlen("-psn")) == 0)
		{
			argv[i] = "";
		}
	#endif
		strcat(buf, argv[i]);
	}
	LOG(LM_MAIN, LL_INFO, "Command line (%d args):%s", argc, buf);
	{
		struct option longopts[] =
		{
			{ "fullscreen",	no_argument,		NULL,	'f' },
			{ "scale",		required_argument,	NULL,	's' },
			{ "screen",		required_argument,	NULL,	'c' },
			{ "nohud",		no_argument,		NULL,	1001 },
			{ "nosound",	no_argument,		NULL,	'n' },
			{ "nojoystick",	no_argument,		NULL,	'j' },
			{ "wait",		no_argument,		NULL,	'w' },
			{ "shakemult",	required_argument,	NULL,	'm' },
			{ "connect",	required_argument,	NULL,	'x' },
			{ "debug",		required_argument,	NULL,	'd' },
			{ "log",		required_argument,	NULL,	1000 },
			{ "help",		no_argument,		NULL,	'h' },
			{ 0,			0,					NULL,	0 }
		};
		int opt = 0;
		int idx = 0;
		while ((opt = getopt_long(argc, argv, "fs:c:onjwm:xd\0h", longopts, &idx)) != -1)
		{
			switch (opt)
			{
			case 'f':
				ConfigGet(&gConfig, "Graphics.Fullscreen")->u.Bool.Value = true;
				break;
			case 's':
				ConfigSetInt(&gConfig, "Graphics.ScaleFactor", atoi(optarg));
				break;
			case 'c':
				sscanf(optarg, "%dx%d",
					&ConfigGet(&gConfig, "Graphics.ResolutionWidth")->u.Int.Value,
					&ConfigGet(&gConfig, "Graphics.ResolutionHeight")->u.Int.Value);
				LOG(LM_MAIN, LL_DEBUG, "Video mode %dx%d set...",
					ConfigGetInt(&gConfig, "Graphics.ResolutionWidth"),
					ConfigGetInt(&gConfig, "Graphics.ResolutionHeight"));
				break;
			case 1001:
				ConfigGet(&gConfig, "Graphics.ShowHUD")->u.Bool.Value = false;
				break;
			case 'n':
				LOG(LM_MAIN, LL_INFO, "Sound to 0 volume");
				ConfigGet(&gConfig, "Sound.SoundVolume")->u.Int.Value = 0;
				ConfigGet(&gConfig, "Sound.MusicVolume")->u.Int.Value = 0;
				break;
			case 'j':
				debug(D_NORMAL, "nojoystick\n");
				controllerFlag = 0;
				break;
			case 'w':
				wait = 1;
				break;
			case 'm':
				{
					ConfigGet(&gConfig, "Graphics.ShakeMultiplier")->u.Int.Value =
						MAX(atoi(optarg), 0);
					printf("Shake multiplier: %d\n",
						ConfigGetInt(&gConfig, "Graphics.ShakeMultiplier"));
				}
				break;
			case 'h':
				PrintHelp();
				goto bail;
			case 'd':
				// Set debug level
				debug = true;
				debug_level = CLAMP(atoi(optarg), D_NORMAL, D_MAX);
				break;
			case 1000:
				{
					char *comma = strchr(optarg, ',');
					if (comma)
					{
						// Set logging level for a single module
						// The module and level are comma separated
						*comma = '\0';
						const LogLevel ll = StrLogLevel(comma + 1);
						LogModuleSetLevel(StrLogModule(optarg), ll);
						printf("Logging %s at %s\n", optarg, LogLevelName(ll));
					}
					else
					{
						// Set logging level for all modules
						const LogLevel ll = StrLogLevel(optarg);
						for (int i = 0; i < (int)LM_COUNT; i++)
						{
							LogModuleSetLevel((LogModule)i, ll);
						}
						printf("Logging everything at %s\n", LogLevelName(ll));
					}
				}
				break;
			case 'x':
				if (enet_address_set_host(&connectAddr, optarg) != 0)
				{
					printf("Error: unknown host %s\n", optarg);
				}
				break;
			default:
				PrintHelp();
				// Ignore unknown arguments
				break;
			}
		}
		if (optind < argc)
		{
			// non-option ARGV-elements
			for (; optind < argc; optind++)
			{
				// Load campaign
				loadCampaign = argv[optind];
			}
		}
	}

	debug(D_NORMAL, "Initialising SDL...\n");
	const int sdlFlags =
		SDL_INIT_TIMER | SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_HAPTIC;
	if (SDL_Init(sdlFlags | controllerFlag) != 0)
	{
		LOG(LM_MAIN, LL_ERROR, "Could not initialise SDL: %s", SDL_GetError());
		err = EXIT_FAILURE;
		goto bail;
	}
	if (SDLJBN_Init() != 0)
	{
		LOG(LM_MAIN, LL_ERROR, "Could not initialise SDLJBN: %s",
			SDLJBN_GetError());
		err = EXIT_FAILURE;
		goto bail;
	}
	SDL_EventState(SDL_DROPFILE, SDL_DISABLE);

	GetDataFilePath(buf, "");
	LOG(LM_MAIN, LL_INFO, "data dir(%s)", buf);
	LOG(LM_MAIN, LL_INFO, "config dir(%s)", GetConfigFilePath(""));

	SoundInitialize(&gSoundDevice, "sounds");
	if (!gSoundDevice.isInitialised)
	{
		LOG(LM_MAIN, LL_ERROR, "Sound initialization failed!");
	}

	LoadHighScores();

	debug(D_NORMAL, "Loading song lists...\n");
	LoadSongs();

	MusicPlayMenu(&gSoundDevice);

	EventInit(&gEventHandlers, NULL, NULL, true);
	NetServerInit(&gNetServer);

	if (wait)
	{
		printf("Press the enter key to continue...\n");
		getchar();
	}
	PicManagerInit(&gPicManager);
	GraphicsInit(&gGraphicsDevice, &gConfig);
	GraphicsInitialize(&gGraphicsDevice);
	if (!gGraphicsDevice.IsInitialized)
	{
		LOG(LM_MAIN, LL_WARN, "Cannot initialise video; trying default config");
		ConfigResetDefault(ConfigGet(&gConfig, "Graphics"));
		GraphicsInit(&gGraphicsDevice, &gConfig);
		GraphicsInitialize(&gGraphicsDevice);
	}
	if (!gGraphicsDevice.IsInitialized)
	{
		LOG(LM_MAIN, LL_ERROR, "Video didn't init!");
		err = EXIT_FAILURE;
		goto bail;
	}
	FontLoadFromJSON(&gFont, "graphics/font.png", "graphics/font.json");
	PicManagerLoad(&gPicManager, "graphics");

	ParticleClassesInit(&gParticleClasses, "data/particles.json");
	AmmoInitialize(&gAmmo, "data/ammo.json");
	BulletAndWeaponInitialize(
		&gBulletClasses, &gGunDescriptions,
		"data/bullets.json", "data/guns.json");
	CharacterClassesInitialize(&gCharacterClasses, "data/character_classes.json");
	LoadPlayerTemplates(
		&gPlayerTemplates, &gCharacterClasses, PLAYER_TEMPLATE_FILE);
	PickupClassesInit(
		&gPickupClasses, "data/pickups.json", &gAmmo, &gGunDescriptions);
	MapObjectsInit(
		&gMapObjects, "data/map_objects.json", &gAmmo, &gGunDescriptions);
	CollisionSystemInit(&gCollisionSystem);
	CampaignInit(&gCampaign);
	LoadAllCampaigns(&campaigns);
	PlayerDataInit(&gPlayerDatas);

	debug(D_NORMAL, ">> Entering main loop\n");
	// Attempt to pre-load campaign if requested
	if (loadCampaign != NULL)
	{
		LOG(LM_MAIN, LL_INFO, "Loading campaign %s...", loadCampaign);
		gCampaign.Entry.Mode =
			strstr(loadCampaign, "/" CDOGS_DOGFIGHT_DIR "/") != NULL ?
			GAME_MODE_DOGFIGHT : GAME_MODE_NORMAL;
		CampaignEntry entry;
		if (!CampaignEntryTryLoad(&entry, loadCampaign, GAME_MODE_NORMAL) ||
			!CampaignLoad(&gCampaign, &entry))
		{
			LOG(LM_MAIN, LL_ERROR, "Failed to load campaign %s", loadCampaign);
		}
	}
	else if (connectAddr.host != 0)
	{
		if (NetClientTryScanAndConnect(&gNetClient, connectAddr.host))
		{
			ScreenWaitForCampaignDef();
		}
		else
		{
			printf("Failed to connect\n");
		}
	}
	LOG(LM_MAIN, LL_INFO, "Starting game");
	MainLoop(&creditsDisplayer, &campaigns);

bail:
	debug(D_NORMAL, ">> Shutting down...\n");
	MapTerminate(&gMap);
	PlayerDataTerminate(&gPlayerDatas);
	MapObjectsTerminate(&gMapObjects);
	PickupClassesTerminate(&gPickupClasses);
	ParticleClassesTerminate(&gParticleClasses);
	AmmoTerminate(&gAmmo);
	WeaponTerminate(&gGunDescriptions);
	BulletTerminate(&gBulletClasses);
	CharacterClassesTerminate(&gCharacterClasses);
	MissionOptionsTerminate(&gMission);
	NetClientTerminate(&gNetClient);
	atexit(enet_deinitialize);
	EventTerminate(&gEventHandlers);
	GraphicsTerminate(&gGraphicsDevice);
	CampaignTerminate(&gCampaign);

	PicManagerTerminate(&gPicManager);
	FontTerminate(&gFont);
	AutosaveSave(&gAutosave, GetConfigFilePath(AUTOSAVE_FILE));
	AutosaveTerminate(&gAutosave);
	CArrayTerminate(&gPlayerTemplates);
	FreeSongs(&gMenuSongs);
	FreeSongs(&gGameSongs);
	SaveHighScores();
	UnloadCredits(&creditsDisplayer);
	UnloadAllCampaigns(&campaigns);
	SoundTerminate(&gSoundDevice, true);
	ConfigDestroy(&gConfig);

	SDLJBN_Quit();
	SDL_Quit();

	return err;
}
