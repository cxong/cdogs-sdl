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
#include "log.h"
#include "player_template.h"
#include "quick_play.h"
#include "sys_config.h"
#include "utils.h"

CampaignOptions gCampaign;

struct MissionOptions gMission;

struct SongDef *gGameSongs = NULL;
struct SongDef *gMenuSongs = NULL;


bool CampaignLoad(CampaignOptions *co, CampaignEntry *entry)
{
	CASSERT(!co->IsLoaded, "loading campaign without unloading last one");
	// Note: use the mode already set by the menus
	const GameMode mode = co->Entry.Mode;
	co->Entry = *entry;
	co->Entry.Mode = mode;
	CampaignSettingInit(&co->Setting);
	if (entry->Mode == GAME_MODE_QUICK_PLAY)
	{
		SetupQuickPlayCampaign(&co->Setting);
		co->IsLoaded = true;
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
		else
		{
			memcpy(&co->Setting, &customSetting, sizeof co->Setting);
			co->IsLoaded = true;
		}
	}

	if (co->IsLoaded)
	{
		LOG(LM_MAIN, LL_INFO, "loaded campaign/dogfight");
	}
	return co->IsLoaded;
}
void CampaignUnload(CampaignOptions *co)
{
	co->IsLoaded = false;
	co->IsClient = false;	// TODO: select is client from menu
	co->OptionsSet = false;
}

void MissionOptionsInit(struct MissionOptions *mo)
{
	memset(mo, 0, sizeof *mo);
	CArrayInit(&mo->Weapons, sizeof(GunDescription *));
	CArrayInit(&mo->Objectives, sizeof(ObjectiveDef));
}
void MissionOptionsTerminate(struct MissionOptions *mo)
{
	CArrayTerminate(&mo->Weapons);
	CArrayTerminate(&mo->Objectives);
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
		if (strcmp(file.extension, "txt") == 0 ||
			strcmp(file.extension, "TXT") == 0)
		{
			debug(D_VERBOSE, "Skipping text file %s\n", file.name);
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


bool GameIsMouseUsed(void)
{
	for (int i = 0; i < (int)gPlayerDatas.size; i++)
	{
		const PlayerData *p = CArrayGet(&gPlayerDatas, i);
		if (p->IsLocal && p->inputDevice == INPUT_DEVICE_MOUSE)
		{
			return true;
		}
	}
	return false;
}
