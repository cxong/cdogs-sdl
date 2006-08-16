/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (C) 1995 Ronny Wester
    Copyright (C) 2003 Jeremy Chin 
    Copyright (C) 2003 Lucas Martin-King 

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

-------------------------------------------------------------------------------

 gamedata.c - game data related stuff 
 
 Author: $Author$
 Rev:    $Revision$
 URL:    $HeadURL$
 ID:     $Id$
 
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "SDL.h"
#include "gamedata.h"
#include "actors.h"
#include "defs.h"
#include "keyboard.h"
#include "input.h"

#include "utils.h"


struct PlayerData gPlayer1Data = {
	"Player 1", 0, SHADE_BLUE, SHADE_BLUE, SHADE_BLUE, 0, 0,
	3, {GUN_SHOTGUN, GUN_MG, GUN_FRAGGRENADE},
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	KEYBOARD,
	{SDLK_LEFT, SDLK_RIGHT, SDLK_UP, SDLK_DOWN,
	 SDLK_RSHIFT, SDLK_RETURN}
};

struct PlayerData gPlayer2Data = {
	"Player 2", 1, SHADE_RED, SHADE_RED, SHADE_RED, 2, 0,
	3, {GUN_POWERGUN, GUN_FLAMER, GUN_GRENADE},
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	KEYBOARD,
	{keyKeypad4, keyKeypad6, keyKeypad8, keyKeypad2, keyKeypad0,
	 keyKeypadEnter}
};

struct GameOptions gOptions = {
	0,
	1,
	0, 0,
	0,
	0,
	0,
	0,
	COPY_REPMOVSD,
	0,
	0,
	0, 0,
	keyTab,
	100,
	100,
	100,
	100,
	0,
	0	// not fullscreen by default
};

struct CampaignOptions gCampaign = {
	NULL, 0, 0
};

struct MissionOptions gMission;


struct SongDef *gGameSongs = NULL;
struct SongDef *gMenuSongs = NULL;


void AddSong(struct SongDef **songList, const char *path)
{
	struct SongDef *s;

	s = malloc(sizeof(struct SongDef));
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
		free(s);
	}
}

void LoadSongs(const char *path, struct SongDef **songList)
{
	FILE *f;
	char s[100], *p;

	debug("LoadSongs path: %s\n", path);

	f = fopen(path, "r");
	if (f) {
		while (fgets(s, sizeof(s), f)) {
			p = s + strlen(s);
			while (p >= s && !isgraph(*p))
				*p-- = 0;
			if (s[0]) {
				debug("Added song: %s\n", s);
				AddSong(songList, s);
			}
		}
		fclose(f);
	}
}
