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
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <SDL.h>

#include <cdogs/ammo.h>
#include <cdogs/campaigns.h>
#include <cdogs/collision.h>
#include <cdogs/config.h>
#include <cdogs/draw.h>
#include <cdogs/files.h>
#include <cdogs/font.h>
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
	for (;;)
	{
		if (!gCampaign.IsLoaded)
		{
			MainMenu(
				&gGraphicsDevice, creditsDisplayer, campaigns, lastGameMode);
		}
		if (!gCampaign.IsLoaded)
		{
			break;
		}
		lastGameMode = gCampaign.Entry.Mode;
		ScreenStart();
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
	printf("SDL Port by Jeremy Chin, Lucas Martin-King and Cong Xu, Copyright 2003-2015\n\n");
}

static void PrintHelp(void)
{
	printf("%s\n",
		"Video Options:\n"
		"    --fullscreen     Try and use a fullscreen video mode.\n"
		"    --scale=n        Scale the window resolution up by a factor of n\n"
		"                       Factors: 2, 3, 4\n"
		"    --screen=WxH     Set virtual screen width to W x H\n"
		"                       Modes: 320x200, 320x240, 400x300, 640x480, 800x600, 1024x768\n"
		"    --forcemode      Don't check video mode sanity\n"
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
	printf("%s\n",
		"Levels can be set between %s and %s\n"
		"Available modules are: NET\n"
		"    --log=M,L        Enable logging for module M at level L.\n\n",
		LogLevelName(LL_TRACE), LogLevelName(LL_ERROR)
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
	int snd_flag = SDL_INIT_AUDIO;
	int js_flag = SDL_INIT_JOYSTICK;
	int isSoundEnabled = 1;
	credits_displayer_t creditsDisplayer;
	custom_campaigns_t campaigns;
	memset(&campaigns, 0, sizeof campaigns);
	int forceResolution = 0;
	int err = 0;
	const char *loadCampaign = NULL;
	ENetAddress connectAddr;
	memset(&connectAddr, 0, sizeof connectAddr);

	srand((unsigned int)time(NULL));

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
	LoadCredits(&creditsDisplayer, colorPurple, colorDarker);
	AutosaveInit(&gAutosave);
	AutosaveLoad(&gAutosave, GetConfigFilePath(AUTOSAVE_FILE));

	{
		struct option longopts[] =
		{
			{"fullscreen",	no_argument,		NULL,	'f'},
			{"scale",		required_argument,	NULL,	's'},
			{"screen",		required_argument,	NULL,	'c'},
			{"forcemode",	no_argument,		NULL,	'o'},
			{"nosound",		no_argument,		NULL,	'n'},
			{"nojoystick",	no_argument,		NULL,	'j'},
			{"wait",		no_argument,		NULL,	'w'},
			{"shakemult",	required_argument,	NULL,	'm'},
			{"connect",		required_argument,	NULL,	'x'},
			{"debug",		required_argument,	NULL,	'd'},
			{"log",			required_argument,	NULL,	1000},
			{"help",		no_argument,		NULL,	'h'},
			{0,				0,					NULL,	0}
		};
		int opt = 0;
		int idx = 0;
		while ((opt = getopt_long(argc, argv,"fs:c:onjwm:xd\0h", longopts, &idx)) != -1)
		{
			switch (opt)
			{
			case 'f':
				ConfigGet(&gConfig, "Graphics.Fullscreen")->u.Bool.Value = true;
				break;
			case 's':
				ConfigGet(&gConfig, "Graphics.ScaleFactor")->u.Int.Value = atoi(optarg);
				break;
			case 'c':
				sscanf(optarg, "%dx%d",
					&ConfigGet(&gConfig, "Graphics.ResolutionWidth")->u.Int.Value,
					&ConfigGet(&gConfig, "Graphics.ResolutionHeight")->u.Int.Value);
				debug(D_NORMAL, "Video mode %dx%d set...\n",
					ConfigGetInt(&gConfig, "Graphics.ResolutionWidth"),
					ConfigGetInt(&gConfig, "Graphics.ResolutionHeight"));
				break;
			case 'o':
				forceResolution = 1;
				break;
			case 'n':
				printf("Sound disabled!\n");
				snd_flag = 0;
				isSoundEnabled = 0;
				break;
			case 'j':
				debug(D_NORMAL, "nojoystick\n");
				js_flag = 0;
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
						*comma = '\0';
					}
					// Set logging level
					LogModuleSetLevel(StrLogModule(optarg), StrLogLevel(comma + 1));
					printf("Logging %s at %s\n", optarg, comma + 1);
				}
				break;
			case 'x':
				if (enet_address_set_host(&connectAddr, optarg) != 0)
				{
					printf("Error: unknown host %s\n", optarg);
				}
				else
				{
					connectAddr.port = NET_INPUT_PORT;
				}
				break;
			default:
				PrintHelp();
				err = EXIT_FAILURE;
				goto bail;
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
	if (SDL_Init(SDL_INIT_TIMER | snd_flag | SDL_INIT_VIDEO | js_flag) != 0)
	{
		fprintf(stderr, "Could not initialise SDL: %s\n", SDL_GetError());
		err = EXIT_FAILURE;
		goto bail;
	}
	if (enet_initialize() != 0)
	{
		fprintf(stderr, "An error occurred while initializing ENet.\n");
		err = EXIT_FAILURE;
		goto bail;
	}

	char buf[CDOGS_PATH_MAX];
	char buf2[CDOGS_PATH_MAX];
	GetDataFilePath(buf, "");
	printf("Data directory:\t\t%s\n", buf);
	printf("Config directory:\t%s\n\n",	GetConfigFilePath(""));

	if (isSoundEnabled)
	{
		GetDataFilePath(buf, "sounds");
		SoundInitialize(&gSoundDevice, buf);
		if (!gSoundDevice.isInitialised)
		{
			printf("Sound initialization failed!\n");
		}
	}

	LoadHighScores();

	debug(D_NORMAL, "Loading song lists...\n");
	LoadSongs();
	LoadPlayerTemplates(&gPlayerTemplates, PLAYER_TEMPLATE_FILE);

	MusicPlayMenu(&gSoundDevice);

	EventInit(&gEventHandlers, NULL, true);
	NetClientInit(&gNetClient);
	NetServerInit(&gNetServer);

	if (wait)
	{
		printf("Press the enter key to continue...\n");
		getchar();
	}
	if (!PicManagerTryInit(
		&gPicManager, "graphics/cdogs.px", "graphics/cdogs2.px"))
	{
		err = EXIT_FAILURE;
		goto bail;
	}
	memcpy(origPalette, gPicManager.palette, sizeof(origPalette));
	GraphicsInit(&gGraphicsDevice);
	GraphicsInitialize(&gGraphicsDevice, forceResolution);
	if (!gGraphicsDevice.IsInitialized)
	{
		printf("Cannot initialise video; trying default config\n");
		ConfigResetDefault(ConfigGet(&gConfig, "Graphics"));
		GraphicsInitialize(&gGraphicsDevice, forceResolution);
	}
	if (!gGraphicsDevice.IsInitialized)
	{
		printf("Video didn't init!\n");
		err = EXIT_FAILURE;
		goto bail;
	}
	GetDataFilePath(buf, "graphics/font.png");
	GetDataFilePath(buf2, "graphics/font.json");
	FontLoad(&gFont, buf, buf2);
	GetDataFilePath(buf, "graphics");
	PicManagerLoadDir(&gPicManager, buf);

	GetDataFilePath(buf, "data/particles.json");
	ParticleClassesInit(&gParticleClasses, buf);
	GetDataFilePath(buf, "data/ammo.json");
	AmmoInitialize(&gAmmo, buf);
	GetDataFilePath(buf, "data/bullets.json");
	GetDataFilePath(buf2, "data/guns.json");
	BulletAndWeaponInitialize(
		&gBulletClasses, &gGunDescriptions, buf, buf2);
	GetDataFilePath(buf, "data/pickups.json");
	PickupClassesInit(&gPickupClasses, buf, &gAmmo, &gGunDescriptions);
	GetDataFilePath(buf, "data/map_objects.json");
	MapObjectsInit(&gMapObjects, buf);
	CollisionSystemInit(&gCollisionSystem);
	CampaignInit(&gCampaign);
	LoadAllCampaigns(&campaigns);
	PlayerDataInit(&gPlayerDatas);
	MapInit(&gMap);

	GrafxMakeRandomBackground(
		&gGraphicsDevice, &gCampaign, &gMission, &gMap);

	debug(D_NORMAL, ">> Entering main loop\n");
	// Attempt to pre-load campaign if requested
	if (loadCampaign != NULL)
	{
		CampaignEntry entry;
		if (!CampaignEntryTryLoad(&entry, loadCampaign, GAME_MODE_NORMAL) ||
			!CampaignLoad(&gCampaign, &entry))
		{
			fprintf(stderr, "Failed to load campaign %s\n", loadCampaign);
		}
	}
	else if (connectAddr.port != 0)
	{
		NetClientConnect(&gNetClient, connectAddr);
		if (!NetClientIsConnected(&gNetClient))
		{
			printf("Failed to connect\n");
		}
		else
		{
			ScreenWaitForCampaignDef();
		}
	}
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
	MissionOptionsTerminate(&gMission);
	NetClientTerminate(&gNetClient);
	atexit(enet_deinitialize);
	EventTerminate(&gEventHandlers);
	GraphicsTerminate(&gGraphicsDevice);

	PicManagerTerminate(&gPicManager);
	AutosaveSave(&gAutosave, GetConfigFilePath(AUTOSAVE_FILE));
	AutosaveTerminate(&gAutosave);
	ConfigSave(&gConfig, GetConfigFilePath(CONFIG_FILE));
	SavePlayerTemplates(&gPlayerTemplates, PLAYER_TEMPLATE_FILE);
	CArrayTerminate(&gPlayerTemplates);
	FreeSongs(&gMenuSongs);
	FreeSongs(&gGameSongs);
	SaveHighScores();
	UnloadCredits(&creditsDisplayer);
	UnloadAllCampaigns(&campaigns);
	CampaignTerminate(&gCampaign);

	if (isSoundEnabled)
	{
		debug(D_NORMAL, ">> Shutting down sound...\n");
		SoundTerminate(&gSoundDevice, 1);
	}

	debug(D_NORMAL, "SDL_Quit()\n");
	SDL_Quit();

	exit(err);
}
