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
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#include <SDL.h>

#include <cdogs/ai.h>
#include <cdogs/campaigns.h>
#include <cdogs/config.h>
#include <cdogs/draw.h>
#include <cdogs/files.h>
#include <cdogs/font.h>
#include <cdogs/gamedata.h>
#include <cdogs/grafx.h>
#include <cdogs/hiscores.h>
#include <cdogs/input.h>
#include <cdogs/joystick.h>
#include <cdogs/keyboard.h>
#include <cdogs/mission.h>
#include <cdogs/music.h>
#include <cdogs/net_client.h>
#include <cdogs/net_server.h>
#include <cdogs/objs.h>
#include <cdogs/palette.h>
#include <cdogs/particle.h>
#include <cdogs/pic_manager.h>
#include <cdogs/pics.h>
#include <cdogs/player_template.h>
#include <cdogs/sounds.h>
#include <cdogs/triggers.h>
#include <cdogs/utils.h>

#include <cdogs/physfs/physfs.h>

#include "autosave.h"
#include "briefing_screens.h"
#include "credits.h"
#include "game.h"
#include "mainmenu.h"
#include "password.h"
#include "player_select_menus.h"
#include "prep.h"
#include "XGetopt.h"


static void PlaceActor(TActor * actor)
{
	Vec2i pos;
	do
	{
		pos.x = ((rand() % (gMap.Size.x * TILE_WIDTH)) << 8);
		pos.y = ((rand() % (gMap.Size.y * TILE_HEIGHT)) << 8);
	}
	while (!MapIsFullPosOKforPlayer(&gMap, pos, false) ||
		!ActorIsPosClear(actor, pos));
	TryMoveActor(actor, pos);
}
static void PlaceActorNear(TActor *actor, Vec2i nearPos, bool allowAllTiles)
{
	// Try a concentric rhombus pattern, clockwise from right
	// That is, start by checking right, below, left, above,
	// then continue with radius 2 right, below-right, below, below-left...
	// (start from S:)
	//      4
	//  9 3 S 1 5
	//    8 2 6
	//      7
#define TRY_LOCATION()\
	if (MapIsFullPosOKforPlayer(\
		&gMap, Vec2iAdd(nearPos, Vec2iNew(dx, dy)), allowAllTiles) && \
		ActorIsPosClear(actor, Vec2iAdd(nearPos, Vec2iNew(dx, dy))))\
	{\
		TryMoveActor(actor, Vec2iAdd(nearPos, Vec2iNew(dx, dy)));\
		return;\
	}
	int dx = 0;
	int dy = 0;
	TRY_LOCATION();
	int inc = 1 << 8;
	for (int radius = 12 << 8;; radius += 12 << 8)
	{
		// Going from right to below
		for (dx = radius, dy = 0; dy < radius; dx -= inc, dy += inc)
		{
			TRY_LOCATION();
		}
		// below to left
		for (dx = 0, dy = radius; dy > 0; dx -= inc, dy -= inc)
		{
			TRY_LOCATION();
		}
		// left to above
		for (dx = -radius, dy = 0; dx < 0; dx += inc, dy -= inc)
		{
			TRY_LOCATION();
		}
		// above to right
		for (dx = 0, dy = -radius; dy < 0; dx += inc, dy += inc)
		{
			TRY_LOCATION();
		}
	}
}

static void InitPlayers(int maxHealth, int mission)
{
	for (int i = 0; i < (int)gPlayerDatas.size; i++)
	{
		PlayerData *p = CArrayGet(&gPlayerDatas, i);
		p->score = 0;
		p->kills = 0;
		p->friendlies = 0;
		p->allTime = -1;
		p->today = -1;
	}
	TActor *firstPlayer = NULL;
	for (int i = 0; i < (int)gPlayerDatas.size; i++)
	{
		PlayerData *p = CArrayGet(&gPlayerDatas, i);
		p->lastMission = mission;
		p->Id = ActorAdd(&p->Char, p->playerIndex);
		TActor *player = CArrayGet(&gActors, p->Id);
		player->health = maxHealth;
		p->Char.maxHealth = maxHealth;
		
		if (gCampaign.Entry.Mode == CAMPAIGN_MODE_DOGFIGHT)
		{
			// In a dogfight, always place players apart
			PlaceActor(player);
		}
		else if (gMission.missionData->Type == MAPTYPE_STATIC &&
			!Vec2iIsZero(gMission.missionData->u.Static.Start))
		{
			// place players near the start point
			Vec2i startPoint = Vec2iReal2Full(Vec2iCenterOfTile(
				gMission.missionData->u.Static.Start));
			PlaceActorNear(player, startPoint, true);
		}
		else if (gConfig.Interface.Splitscreen == SPLITSCREEN_NEVER &&
			firstPlayer != NULL)
		{
			// If never split screen, try to place players near the first player
			PlaceActorNear(player, firstPlayer->Pos, true);
		}
		else
		{
			PlaceActor(player);
		}
		firstPlayer = player;
	}
}

static void PlayGameSong(void)
{
	int success = 0;
	// Play a tune
	// Start by trying to play a mission specific song,
	// otherwise pick one from the general collection...
	MusicStop(&gSoundDevice);
	if (strlen(gMission.missionData->Song) > 0)
	{
		char buf[CDOGS_PATH_MAX];
		size_t pathLen = MAX(
			strrchr(gCampaign.Entry.Path, '\\'),
			strrchr(gCampaign.Entry.Path, '/')) - gCampaign.Entry.Path;
		strncpy(buf, gCampaign.Entry.Path, pathLen);
		buf[pathLen] = '\0';

		strcat(buf, "/");
		strcat(buf, gMission.missionData->Song);
		success = !MusicPlay(&gSoundDevice, buf);
	}
	if (!success && gGameSongs != NULL)
	{
		MusicPlay(&gSoundDevice, gGameSongs->path);
		ShiftSongs(&gGameSongs);
	}
}

static void PlayMenuSong(void)
{
	MusicStop(&gSoundDevice);
	if (gMenuSongs)
	{
		MusicPlay(&gSoundDevice, gMenuSongs->path);
		ShiftSongs(&gMenuSongs);
	}
}


int Game(GraphicsDevice *graphics, CampaignOptions *co)
{
	bool run = false;
	bool gameOver = true;
	do
	{
		CampaignAndMissionSetup(1, co, &gMission);
		if (IsMissionBriefingNeeded(co->Entry.Mode))
		{
			if (!ScreenMissionBriefing(&gMission))
			{
				run = false;
				goto bail;
			}
		}
		if (!PlayerEquip())
		{
			run = false;
			goto bail;
		}

		MapLoad(&gMap, &gMission, co, &co->Setting.characters);
		// Note: place players first, as bad guys are placed away from players
		const int maxHealth = 200 * gConfig.Game.PlayerHP / 100;
		InitPlayers(maxHealth, co->MissionIndex);
		InitializeBadGuys();
		CreateEnemies();
		PlayGameSong();
		run = RunGame(&gMission, &gMap);

		const int survivingPlayers = GetNumPlayers(true, false, false);
		gameOver = survivingPlayers == 0 ||
			co->MissionIndex == (int)gCampaign.Setting.Missions.size - 1;

		for (int i = 0; i < (int)gPlayerDatas.size; i++)
		{
			PlayerData *p = CArrayGet(&gPlayerDatas, i);
			p->survived = IsPlayerAlive(p);
			if (IsPlayerAlive(p))
			{
				TActor *player = CArrayGet(&gActors, p->Id);
				p->hp = player->health;
			}
		}

		MissionEnd();
		PlayMenuSong();

		if (run)
		{
			ScreenMissionSummary(&gCampaign, &gMission);
			// Note: must use cached value because players get cleaned up
			// in CleanupMission()
			if (gameOver && survivingPlayers > 0)
			{
				ScreenVictory(&gCampaign);
			}
		}

		bool allTime = false;
		bool todays = false;
		for (int i = 0; i < (int)gPlayerDatas.size; i++)
		{
			PlayerData *p = CArrayGet(&gPlayerDatas, i);
			if (((run && !p->survived) || gameOver) && p->IsLocal)
			{
				EnterHighScore(p);
				allTime |= p->allTime >= 0;
				todays |= p->today >= 0;
			}

			if (!p->survived)
			{
				p->totalScore = 0;
				p->missions = 0;
			}
			else
			{
				p->missions++;
			}
			p->lastMission = co->MissionIndex;
		}
		if (allTime)
		{
			DisplayAllTimeHighScores(graphics);
		}
		if (todays)
		{
			DisplayTodaysHighScores(graphics);
		}

		co->MissionIndex++;

	bail:
		// Need to terminate the mission later as it is used in calculating scores
		MissionOptionsTerminate(&gMission);
	}
	while (run && !gameOver);
	return run;
}

int Campaign(GraphicsDevice *graphics, CampaignOptions *co)
{
	PlayerDataReset(&gPlayerDatas);

	if (IsPasswordAllowed(co->Entry.Mode))
	{
		MissionSave m;
		AutosaveLoadMission(
			&gAutosave, &m, co->Entry.Path, co->Entry.BuiltinIndex);
		co->MissionIndex = EnterPassword(graphics, m.Password);
	}
	else
	{
		co->MissionIndex = 0;
	}

	return Game(graphics, co);
}

#define DOGFIGHT_MAX_SCORE 5

void DogFight(CampaignOptions *co)
{
	CArray scores;
	CArrayInit(&scores, sizeof(int));
	for (int i = 0; i < (int)gPlayerDatas.size; i++)
	{
		int score = 0;
		CArrayPushBack(&scores, &score);
	}
	int maxScore = 0;

	PlayerDataReset(&gPlayerDatas);

	co->MissionIndex = 0;

	bool run = false;
	do
	{
		CampaignAndMissionSetup(1, co, &gMission);
		PlayerEquip();
		MapLoad(&gMap, &gMission, co, &co->Setting.characters);
		srand((unsigned int)time(NULL));
		InitPlayers(500, 0);
		PlayGameSong();

		// Don't quit if all players died, that's normal for dogfights
		run =
			RunGame(&gMission, &gMap) ||
			GetNumPlayers(true, false, false) == 0;

		for (int i = 0; i < (int)gPlayerDatas.size; i++)
		{
			PlayerData *p = CArrayGet(&gPlayerDatas, i);
			if (IsPlayerAlive(p))
			{
				int *score = CArrayGet(&scores, i);
				(*score)++;
				if (*score > maxScore)
				{
					maxScore = *score;
				}
			}
		}

		MissionEnd();
		PlayMenuSong();

		if (run)
		{
			ScreenDogfightScores(&scores);
		}

		// Need to terminate the mission later as it is used in calculating scores
		MissionOptionsTerminate(&gMission);
	} while (run && maxScore < DOGFIGHT_MAX_SCORE);

	if (run)
	{
		ScreenDogfightFinalScores(&scores);
	}

	CArrayTerminate(&scores);
}

void MainLoop(credits_displayer_t *creditsDisplayer, custom_campaigns_t *campaigns)
{
	while (
		gCampaign.IsLoaded ||
		MainMenu(&gGraphicsDevice, creditsDisplayer, campaigns))
	{
		debug(D_NORMAL, ">> Entering campaign\n");
		if (IsIntroNeeded(gCampaign.Entry.Mode))
		{
			if (!ScreenCampaignIntro(&gCampaign.Setting))
			{
				gCampaign.IsLoaded = false;
				continue;
			}
		}

		if (gCampaign.IsClient)
		{
			debug(D_NORMAL, ">> Waiting for number of players from server\n");
			if (!ScreenWaitForRemotePlayers())
			{
				gCampaign.IsLoaded = false;
				continue;
			}
		}

		debug(D_NORMAL, ">> Select number of players\n");
		if (!NumPlayersSelection(
			gCampaign.Entry.Mode, &gGraphicsDevice, &gEventHandlers))
		{
			gCampaign.IsLoaded = false;
			continue;
		}

		debug(D_NORMAL, ">> Entering selection\n");
		if (!PlayerSelection())
		{
			gCampaign.IsLoaded = false;
			continue;
		}

		debug(D_NORMAL, ">> Starting campaign\n");
		if (gCampaign.Entry.Mode == CAMPAIGN_MODE_DOGFIGHT)
		{
			DogFight(&gCampaign);
		}
		else if (Campaign(&gGraphicsDevice, &gCampaign))
		{
			DisplayAllTimeHighScores(&gGraphicsDevice);
			DisplayTodaysHighScores(&gGraphicsDevice);
		}
		gCampaign.IsLoaded = false;
	}
	debug(D_NORMAL, ">> Leaving Main Game Loop\n");

	// Close net connection
	NetServerTerminate(&gNetServer);

	// Reset player datas
	PlayerDataTerminate(&gPlayerDatas);
	PlayerDataInit(&gPlayerDatas);
}

void PrintTitle(void)
{
	printf("C-Dogs SDL %s\n", CDOGS_SDL_VERSION);

	printf("Original Code Copyright Ronny Wester 1995\n");
	printf("Game Data Copyright Ronny Wester 1995\n");
	printf("SDL Port by Jeremy Chin, Lucas Martin-King and Cong Xu, Copyright 2003-2014\n\n");
	printf("%s%s%s%s",
		"C-Dogs SDL comes with ABSOLUTELY NO WARRANTY;\n",
		"see the file COPYING that came with this distibution...\n",
		"This is free software, and you are welcome to redistribute it\n",
		"under certain conditions; for details see COPYING.\n\n");
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
	int forceResolution = 0;
	int err = 0;
	const char *loadCampaign = NULL;
	ENetAddress connectAddr;
	memset(&connectAddr, 0, sizeof connectAddr);

	srand((unsigned int)time(NULL));

	PrintTitle();

	{
		char *dbg;
		if (getenv("DEBUG") != NULL) debug = 1;
		if ((dbg = getenv("DEBUG_LEVEL")) != NULL) {
			debug_level = atoi(dbg);
			if (debug_level < 0) debug_level = 0;
			if (debug_level > D_MAX) debug_level = D_MAX;
		}
	}

	SetupConfigDir();
	ConfigLoadDefault(&gConfig);
	ConfigLoad(&gConfig, GetConfigFilePath(CONFIG_FILE));
	gLastConfig = gConfig;
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
			{"help",		no_argument,		NULL,	'h'},
			{0,				0,					NULL,	0}
		};
		int opt = 0;
		int idx = 0;
		while ((opt = getopt_long(argc, argv,"fs:c:onjwm:xdh", longopts, &idx)) != -1)
		{
			switch (opt)
			{
			case 'f':
				gConfig.Graphics.Fullscreen = 1;
				break;
			case 's':
				gConfig.Graphics.ScaleFactor = atoi(optarg);
				break;
			case 'c':
				sscanf(optarg, "%dx%d",
					&gConfig.Graphics.Res.x,
					&gConfig.Graphics.Res.y);
				debug(D_NORMAL, "Video mode %dx%d set...\n",
					gConfig.Graphics.Res.x,
					gConfig.Graphics.Res.y);
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
					gConfig.Graphics.ShakeMultiplier = atoi(optarg);
					if (gConfig.Graphics.ShakeMultiplier < 0)
					{
						gConfig.Graphics.ShakeMultiplier = 0;
					}
					printf("Shake multiplier: %d\n", gConfig.Graphics.ShakeMultiplier);
				}
				break;
			case 'h':
				PrintHelp();
				goto bail;
			case 'd':
				// Set debug level
				debug = 1;
				debug_level = atoi(optarg);
				if (debug_level < 0) debug_level = 0;
				if (debug_level > D_MAX) debug_level = D_MAX;
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
		SoundInitialize(&gSoundDevice, &gConfig.Sound, buf);
		if (!gSoundDevice.isInitialised)
		{
			printf("Sound initialization failed!\n");
		}
	}

	LoadHighScores();

	debug(D_NORMAL, "Loading song lists...\n");
	LoadSongs();
	LoadPlayerTemplates(&gPlayerTemplates, PLAYER_TEMPLATE_FILE);

	PlayMenuSong();

	EventInit(&gEventHandlers, NULL, true);
	NetClientInit(&gNetClient);
	NetServerInit(&gNetServer);

	PHYSFS_init(argv[0]);

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
	GraphicsInitialize(
		&gGraphicsDevice, &gConfig.Graphics, gPicManager.palette,
		forceResolution);
	if (!gGraphicsDevice.IsInitialized)
	{
		Config defaultConfig;
		printf("Cannot initialise video; trying default config\n");
		ConfigLoadDefault(&defaultConfig);
		gConfig.Graphics = defaultConfig.Graphics;
		gLastConfig = gConfig;
		GraphicsInitialize(
			&gGraphicsDevice, &gConfig.Graphics, gPicManager.palette,
			forceResolution);
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
	GetDataFilePath(buf, "data/bullets.json");
	GetDataFilePath(buf2, "data/guns.json");
	BulletAndWeaponInitialize(
		&gBulletClasses, &gGunDescriptions, buf, buf2);
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
		if (CampaignEntryTryLoad(
			&entry, loadCampaign, CAMPAIGN_MODE_NORMAL))
		{
			CampaignLoad(&gCampaign, &entry);
		}
		else
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
	PHYSFS_deinit();
	MapTerminate(&gMap);
	PlayerDataTerminate(&gPlayerDatas);
	ParticleClassesTerminate(&gParticleClasses);
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
