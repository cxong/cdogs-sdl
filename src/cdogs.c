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

    Copyright (c) 2013-2017, Cong Xu
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
#include <cdogs/collision/collision.h>
#include <cdogs/config_io.h>
#include <cdogs/draw/char_sprites.h>
#include <cdogs/draw/draw.h>
#include <cdogs/files.h>
#include <cdogs/font_utils.h>
#include <cdogs/grafx.h>
#include <cdogs/handle_game_events.h>
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
#include "command_line.h"
#include "credits.h"
#include "mainmenu.h"
#include "prep.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

int main(int argc, char *argv[])
{
#if defined(_MSC_VER) && !defined(NDEBUG)
	FreeConsole();
#endif
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

#ifdef __EMSCRIPTEN__
    // initialize IDBFS for Emscripten persistent storage
    EM_ASM(
        FS.mkdir('/persistent_data');
        FS.mount(IDBFS,{},'/persistent_data');

        Module.print("start file sync..");
        Module.syncdone = 0;

        FS.syncfs(true, function(err) {
                       assert(!err);
                       Module.print("end file sync..");
                       Module.syncdone = 1;
        });
    );

    SetupConfigDir();
    gConfig = ConfigDefault();
#else
	SetupConfigDir();
	gConfig = ConfigLoad(GetConfigFilePath(CONFIG_FILE));
#endif
	// Set config options that are only set via command line
	ConfigGet(&gConfig, "Graphics.ShowHUD")->u.Bool.Value = true;
	ConfigGet(&gConfig, "Graphics.ShakeMultiplier")->u.Int.Value = 1;

	AutosaveInit(&gAutosave);
#ifndef __EMSCRIPTEN__
	AutosaveLoad(&gAutosave, GetConfigFilePath(AUTOSAVE_FILE));
#endif

#ifndef __EMSCRIPTEN__
	if (enet_initialize() != 0)
	{
		LOG(LM_MAIN, LL_ERROR, "An error occurred while initializing ENet.");
		err = EXIT_FAILURE;
		goto bail;
	}
	NetClientInit(&gNetClient);
#endif

	// Print command line
	char buf[CDOGS_PATH_MAX];
	ProcessCommandLine(buf, argc, argv);
	LOG(LM_MAIN, LL_INFO, "Command line (%d args):%s", argc, buf);
	if (!ParseArgs(argc, argv, &connectAddr, &loadCampaign))
	{
		goto bail;
	}

	debug(D_NORMAL, "Initialising SDL...\n");
#ifndef __EMSCRIPTEN__
	const int sdlFlags =
		SDL_INIT_TIMER | SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_HAPTIC |
		SDL_INIT_GAMECONTROLLER;
#else
    const int sdlFlags = SDL_INIT_AUDIO | SDL_INIT_VIDEO;
#endif
	if (SDL_Init(sdlFlags) != 0)
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

	LoadSongs();

	MusicPlayMenu(&gSoundDevice);

	EventInit(&gEventHandlers, NULL, NULL, true);
	NetServerInit(&gNetServer);
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
	CharSpriteClassesInit(&gCharSpriteClasses);

	ParticleClassesInit(&gParticleClasses, "data/particles.json");
	AmmoInitialize(&gAmmo, "data/ammo.json");
	BulletAndWeaponInitialize(
		&gBulletClasses, &gGunDescriptions,
		"data/bullets.json", "data/guns.json");
	CharacterClassesInitialize(&gCharacterClasses, "data/character_classes.json");
#ifndef __EMSCRIPTEN__
	PlayerTemplatesLoad(
		&gPlayerTemplates, &gCharacterClasses, PLAYER_TEMPLATE_FILE);
#endif
	PickupClassesInit(
		&gPickupClasses, "data/pickups.json", &gAmmo, &gGunDescriptions);
	MapObjectsInit(
		&gMapObjects, "data/map_objects.json", &gAmmo, &gGunDescriptions);
	CollisionSystemInit(&gCollisionSystem);
	CampaignInit(&gCampaign);
	PlayerDataInit(&gPlayerDatas);

	LoopRunner l = LoopRunnerNew(NULL);
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
			LoopRunnerPush(&l, ScreenWaitForCampaignDef());
		}
		else
		{
			printf("Failed to connect\n");
		}
	}
	if (!gCampaign.IsLoaded)
	{
		LoopRunnerPush(&l, MainMenu(&gGraphicsDevice, &l));
	}
	LOG(LM_MAIN, LL_INFO, "Starting game");
	LoopRunnerRun(&l);
	LoopRunnerTerminate(&l);

bail:
	NetServerTerminate(&gNetServer);
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
	CollisionSystemTerminate(&gCollisionSystem);

	CharSpriteClassesTerminate(&gCharSpriteClasses);
	PicManagerTerminate(&gPicManager);
	FontTerminate(&gFont);
	AutosaveSave(&gAutosave, GetConfigFilePath(AUTOSAVE_FILE));
	AutosaveTerminate(&gAutosave);
	PlayerTemplatesTerminate(&gPlayerTemplates);
	FreeSongs(&gMenuSongs);
	FreeSongs(&gGameSongs);
	SoundTerminate(&gSoundDevice, true);
	ConfigDestroy(&gConfig);
	LogTerminate();

	SDLJBN_Quit();
	SDL_Quit();

	return err;
}
