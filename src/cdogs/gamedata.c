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
#include "gamedata.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <SDL.h>
#include <SDL_mixer.h>
#include <tinydir/tinydir.h>

#include "actors.h"
#include "config.h"
#include "defs.h"
#include "keyboard.h"
#include "input.h"
#include "player_template.h"
#include "quick_play.h"
#include "sys_config.h"
#include "utils.h"


struct PlayerData gPlayerDatas[MAX_PLAYERS];

struct GameOptions gOptions = {
	0,	// twoPlayers
	1	// badGuys
};

CampaignOptions gCampaign;

struct MissionOptions gMission;


struct SongDef *gGameSongs = NULL;
struct SongDef *gMenuSongs = NULL;


void PlayerDataInitialize(void)
{
	int i;
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		struct PlayerData *d = &gPlayerDatas[i];
		memset(d, 0, sizeof *d);

		// Set default player 1 controls, as it's used in menus
		if (i == 0)
		{
			d->inputDevice = INPUT_DEVICE_KEYBOARD;
			d->deviceIndex = 0;
		}

		// load from template if available
		if (PlayerTemplatesGetCount(gPlayerTemplates) > i)
		{
			PlayerTemplate *t = &gPlayerTemplates[i];
			strcpy(d->name, t->name);
			d->looks.face = t->face;
			d->looks.skin = t->skin;
			d->looks.arm = t->arms;
			d->looks.body = t->body;
			d->looks.leg = t->legs;
			d->looks.hair = t->hair;
		}
		else
		{
			switch (i)
			{
			case 0:
				strcpy(d->name, "Jones");
				d->looks.face = FACE_JONES;
				d->looks.skin = SHADE_SKIN;
				d->looks.arm = SHADE_BLUE;
				d->looks.body = SHADE_BLUE;
				d->looks.leg = SHADE_BLUE;
				d->looks.hair = SHADE_RED;
				break;
			case 1:
				strcpy(d->name, "Ice");
				d->looks.face = FACE_ICE;
				d->looks.skin = SHADE_DARKSKIN;
				d->looks.arm = SHADE_RED;
				d->looks.body = SHADE_RED;
				d->looks.leg = SHADE_RED;
				d->looks.hair = SHADE_RED;
				break;
			case 2:
				strcpy(d->name, "Delta");
				d->looks.face = FACE_WARBABY;
				d->looks.skin = SHADE_SKIN;
				d->looks.arm = SHADE_GREEN;
				d->looks.body = SHADE_GREEN;
				d->looks.leg = SHADE_GREEN;
				d->looks.hair = SHADE_RED;
				break;
			case 3:
				strcpy(d->name, "Hans");
				d->looks.face = FACE_HAN;
				d->looks.skin = SHADE_ASIANSKIN;
				d->looks.arm = SHADE_YELLOW;
				d->looks.body = SHADE_YELLOW;
				d->looks.leg = SHADE_YELLOW;
				d->looks.hair = SHADE_GOLDEN;
				break;
			default:
				assert(0 && "unsupported");
				break;
			}
		}

		// weapons
		switch (i)
		{
			case 0:
				d->weapons[0] = StrGunDescription("Shotgun");
				d->weapons[1] = StrGunDescription("Machine gun");
				d->weapons[2] = StrGunDescription("Shrapnel bombs");
				break;
			case 1:
				d->weapons[0] = StrGunDescription("Powergun");
				d->weapons[1] = StrGunDescription("Flamer");
				d->weapons[2] = StrGunDescription("Grenades");
				break;
			case 2:
				d->weapons[0] = StrGunDescription("Sniper rifle");
				d->weapons[1] = StrGunDescription("Knife");
				d->weapons[2] = StrGunDescription("Molotovs");
				break;
			case 3:
				d->weapons[0] = StrGunDescription("Machine gun");
				d->weapons[1] = StrGunDescription("Flamer");
				d->weapons[2] = StrGunDescription("Dynamite");
				break;
			default:
				assert(0 && "unsupported");
				break;
		}
		d->weaponCount = 3;
		d->playerIndex = i;
	}
}

void CampaignLoad(CampaignOptions *co, CampaignEntry *entry)
{
	CASSERT(!co->IsLoaded, "loading campaign without unloading last one");
	co->Entry = *entry;
	CampaignSettingInit(&co->Setting);
	if (entry->IsBuiltin)
	{
		if (entry->Mode == CAMPAIGN_MODE_NORMAL)
		{
			SetupBuiltinCampaign(entry->BuiltinIndex);
			co->IsLoaded = true;
		}
		else if (entry->Mode == CAMPAIGN_MODE_DOGFIGHT)
		{
			SetupBuiltinDogfight(entry->BuiltinIndex);
			co->IsLoaded = true;
		}
		else if (entry->Mode == CAMPAIGN_MODE_QUICK_PLAY)
		{
			SetupQuickPlayCampaign(&co->Setting, &gConfig.QuickPlay);
			co->IsLoaded = true;
		}
		else
		{
			CASSERT(false, "Unknown game mode");
		}
	}
	else
	{
		CampaignSetting customSetting;
		CampaignSettingInit(&customSetting);

		if (MapNewLoad(entry->Path, &customSetting))
		{
			printf("Failed to load campaign %s!\n", entry->Path);
			CASSERT(false, "Failed to load campaign");
		}
		memcpy(&co->Setting, &customSetting, sizeof co->Setting);
		co->IsLoaded = true;
	}

	if (co->IsLoaded)
	{
		printf(">> Loaded campaign/dogfight\n");
	}
}

void MissionOptionsInit(struct MissionOptions *mo)
{
	memset(mo, 0, sizeof *mo);
	CArrayInit(&mo->Objectives, sizeof(struct Objective));
	CArrayInit(&mo->MapObjects, sizeof(MapObject));
}
void MissionOptionsTerminate(struct MissionOptions *mo)
{
	CArrayTerminate(&mo->Objectives);
	CArrayTerminate(&mo->MapObjects);
	memset(mo, 0, sizeof *mo);
}


void LoadSongList(struct SongDef **songList, const char *dirPath);


void AddSong(struct SongDef **songList, const char *path)
{
	struct SongDef *s;
	CMALLOC(s, sizeof(struct SongDef));
	strcpy(s->path, path);
	s->next = *songList;
	*songList = s;
}

void ShiftSongs(struct SongDef **songList)
{
	struct SongDef **h;

	if (!*songList || !(*songList)->next)
		return;

	h = songList;
	while (*h)
		h = &(*h)->next;

	*h = *songList;
	*songList = (*songList)->next;
	(*h)->next = NULL;
}

void FreeSongs(struct SongDef **songList)
{
	struct SongDef *s;

	while (*songList) {
		s = *songList;
		*songList = s->next;
		CFREE(s);
	}
}

void LoadSongs(void)
{
	debug(D_NORMAL, "loading game music %s\n", CDOGS_GAME_MUSIC_DIR);
	LoadSongList(&gGameSongs, CDOGS_GAME_MUSIC_DIR);
	debug(D_NORMAL, "loading menu music %s\n", CDOGS_MENU_MUSIC_DIR);
	LoadSongList(&gMenuSongs, CDOGS_MENU_MUSIC_DIR);
}

void LoadSongList(struct SongDef **songList, const char *dirPath)
{
	tinydir_dir dir;
	int errsv;
	if (tinydir_open(&dir, dirPath) == -1)
	{
		errsv = errno;
		printf("Cannot open music dir: %s\n", strerror(errsv));
		goto bail;
	}

	for (; dir.has_next; tinydir_next(&dir))
	{
		Mix_Music *m;
		tinydir_file file;
		if (tinydir_readfile(&dir, &file) == -1)
		{
			errsv = errno;
			debug(D_VERBOSE, "cannot read file: %s\n", strerror(errsv));
			goto bail;
		}
		if (!file.is_reg)
		{
			debug(D_VERBOSE, "not a regular file %s\n", file.name);
			continue;
		}

		m = Mix_LoadMUS(file.path);
		if (m == NULL)
		{
			debug(D_VERBOSE, "not a music file %s\n", file.name);
			continue;
		}
		Mix_FreeMusic(m);
		AddSong(songList, file.path);
	}

bail:
	tinydir_close(&dir);
}

int IsIntroNeeded(campaign_mode_e mode)
{
	return mode == CAMPAIGN_MODE_NORMAL;
}

int IsScoreNeeded(campaign_mode_e mode)
{
	return mode != CAMPAIGN_MODE_DOGFIGHT;
}

int HasObjectives(campaign_mode_e mode)
{
	return mode == CAMPAIGN_MODE_NORMAL;
}

int IsAutoMapEnabled(campaign_mode_e mode)
{
	return mode != CAMPAIGN_MODE_DOGFIGHT;
}

int IsPasswordAllowed(campaign_mode_e mode)
{
	return mode == CAMPAIGN_MODE_NORMAL;
}

int IsMissionBriefingNeeded(campaign_mode_e mode)
{
	return mode == CAMPAIGN_MODE_NORMAL;
}

int AreKeysAllowed(campaign_mode_e mode)
{
	return mode == CAMPAIGN_MODE_NORMAL;
}

int AreHealthPickupsAllowed(campaign_mode_e mode)
{
	return mode == CAMPAIGN_MODE_NORMAL || mode == CAMPAIGN_MODE_QUICK_PLAY;
}


int GameIsMouseUsed(struct PlayerData playerDatas[MAX_PLAYERS])
{
	int i;
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		if (playerDatas[i].inputDevice == INPUT_DEVICE_MOUSE)
		{
			return 1;
		}
	}
	return 0;
}
