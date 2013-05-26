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

    Copyright (c) 2013, Cong Xu
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <SDL.h>
#include <SDL_mixer.h>
#include <tinydir/tinydir.h>

#include "actors.h"
#include "defs.h"
#include "keyboard.h"
#include "input.h"
#include "sys_config.h"
#include "utils.h"


struct PlayerData gPlayer1Data = {
	"Player 1", 0, SHADE_BLUE, SHADE_BLUE, SHADE_BLUE, 0, 0,
	3, {GUN_SHOTGUN, GUN_MG, GUN_FRAGGRENADE},
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	INPUT_DEVICE_KEYBOARD,
	{SDLK_LEFT, SDLK_RIGHT, SDLK_UP, SDLK_DOWN,
	 SDLK_RSHIFT, SDLK_RETURN}
};

struct PlayerData gPlayer2Data = {
	"Player 2", 1, SHADE_RED, SHADE_RED, SHADE_RED, 2, 0,
	3, {GUN_POWERGUN, GUN_FLAMER, GUN_GRENADE},
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	INPUT_DEVICE_KEYBOARD,
	{keyKeypad4, keyKeypad6, keyKeypad8, keyKeypad2, keyKeypad0,
	 keyKeypadEnter}
};

const char *DifficultyStr(difficulty_e d)
{
	switch (d)
	{
	case DIFFICULTY_VERYEASY:
		return "Easiest";
	case DIFFICULTY_EASY:
		return "Easy";
	case DIFFICULTY_NORMAL:
		return "Normal";
	case DIFFICULTY_HARD:
		return "Hard";
	case DIFFICULTY_VERYHARD:
		return "Very hard";
	default:
		return "";
	}
}

struct GameOptions gOptions = {
	0,	// twoPlayers
	1,	// badGuys
	0,	// splitScreenAlways
	0,	// displayFPS
	0,	// displayTime
	0,	// playersHurt
	0,	// displaySlices
	0,	// forceVSync
	0,	// brightness
	0, 0,	// swapButtonsJoy1/2
	keyTab,	// mapKey
	DIFFICULTY_NORMAL,	// difficulty
	100,	// density
	100,	// npcHp
	100,	// playerHp
	0	// slowmotion
};

CampaignOptions gCampaign =
{
	NULL, 0, CAMPAIGN_MODE_NORMAL
};

struct MissionOptions gMission;


struct SongDef *gGameSongs = NULL;
struct SongDef *gMenuSongs = NULL;


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

int IsTileInExit(TTileItem *tile, struct MissionOptions *options)
{
	return
		tile->x >= options->exitLeft &&
		tile->x <= options->exitRight &&
		tile->y >= options->exitTop &&
		tile->y <= options->exitBottom;
}
