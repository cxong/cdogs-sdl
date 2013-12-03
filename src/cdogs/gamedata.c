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
				d->weapons[0] = GUN_SHOTGUN;
				d->weapons[1] = GUN_MG;
				d->weapons[2] = GUN_FRAGGRENADE;
				break;
			case 1:
				strcpy(d->name, "Ice");
				d->looks.face = FACE_ICE;
				d->looks.skin = SHADE_DARKSKIN;
				d->looks.arm = SHADE_RED;
				d->looks.body = SHADE_RED;
				d->looks.leg = SHADE_RED;
				d->looks.hair = SHADE_RED;
				d->weapons[0] = GUN_POWERGUN;
				d->weapons[1] = GUN_FLAMER;
				d->weapons[2] = GUN_GRENADE;
				break;
			case 2:
				strcpy(d->name, "Delta");
				d->looks.face = FACE_WARBABY;
				d->looks.skin = SHADE_SKIN;
				d->looks.arm = SHADE_GREEN;
				d->looks.body = SHADE_GREEN;
				d->looks.leg = SHADE_GREEN;
				d->looks.hair = SHADE_RED;
				d->weapons[0] = GUN_SNIPER;
				d->weapons[1] = GUN_KNIFE;
				d->weapons[2] = GUN_MOLOTOV;
				break;
			case 3:
				strcpy(d->name, "Hans");
				d->looks.face = FACE_HAN;
				d->looks.skin = SHADE_ASIANSKIN;
				d->looks.arm = SHADE_YELLOW;
				d->looks.body = SHADE_YELLOW;
				d->looks.leg = SHADE_YELLOW;
				d->looks.hair = SHADE_GOLDEN;
				d->weapons[0] = GUN_MG;
				d->weapons[1] = GUN_FLAMER;
				d->weapons[2] = GUN_DYNAMITE;
				break;
			default:
				assert(0 && "unsupported");
				break;
		}
		d->looks.armedBody = BODY_ARMED;
		d->looks.unarmedBody = BODY_UNARMED;
		d->weaponCount = 3;
		d->playerIndex = i;
	}
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

int IsTileInExit(TTileItem *tile, struct MissionOptions *options)
{
	return
		tile->x >= options->exitLeft &&
		tile->x <= options->exitRight &&
		tile->y >= options->exitTop &&
		tile->y <= options->exitBottom;
}


static int GetKeyboardCmd(
	keyboard_t *keyboard, input_keys_t *keys,
	int(*keyFunc)(keyboard_t *, int))
{
	int cmd = 0;

	if (keyFunc(keyboard, keys->left))			cmd |= CMD_LEFT;
	else if (keyFunc(keyboard, keys->right))	cmd |= CMD_RIGHT;

	if (keyFunc(keyboard, keys->up))			cmd |= CMD_UP;
	else if (keyFunc(keyboard, keys->down))		cmd |= CMD_DOWN;

	if (keyFunc(keyboard, keys->button1))		cmd |= CMD_BUTTON1;

	if (keyFunc(keyboard, keys->button2))		cmd |= CMD_BUTTON2;

	return cmd;
}

#define MOUSE_MOVE_DEAD_ZONE 12
static int GetMouseCmd(
	Mouse *mouse, int(*mouseFunc)(Mouse *, int), int useMouseMove, Vec2i pos)
{
	int cmd = 0;

	if (useMouseMove)
	{
		int dx = abs(mouse->currentPos.x - pos.x);
		int dy = abs(mouse->currentPos.y - pos.y);
		if (dx > MOUSE_MOVE_DEAD_ZONE || dy > MOUSE_MOVE_DEAD_ZONE)
		{
			if (2 * dx > dy)
			{
				if (pos.x < mouse->currentPos.x)			cmd |= CMD_RIGHT;
				else if (pos.x > mouse->currentPos.x)		cmd |= CMD_LEFT;
			}
			if (2 * dy > dx)
			{
				if (pos.y < mouse->currentPos.y)			cmd |= CMD_DOWN;
				else if (pos.y > mouse->currentPos.y)		cmd |= CMD_UP;
			}
		}
	}
	else
	{
		if (mouseFunc(mouse, SDL_BUTTON_WHEELUP))			cmd |= CMD_UP;
		else if (mouseFunc(mouse, SDL_BUTTON_WHEELDOWN))	cmd |= CMD_DOWN;
	}

	if (mouseFunc(mouse, SDL_BUTTON_LEFT))					cmd |= CMD_BUTTON1;
	if (mouseFunc(mouse, SDL_BUTTON_RIGHT))					cmd |= CMD_BUTTON2;
	if (mouseFunc(mouse, SDL_BUTTON_MIDDLE))				cmd |= CMD_BUTTON3;

	return cmd;
}

static int GetJoystickCmd(
	joystick_t *joystick, int(*joyFunc)(joystick_t *, int))
{
	int cmd = 0;

	if (joyFunc(joystick, CMD_LEFT))		cmd |= CMD_LEFT;
	else if (joyFunc(joystick, CMD_RIGHT))	cmd |= CMD_RIGHT;

	if (joyFunc(joystick, CMD_UP))			cmd |= CMD_UP;
	else if (joyFunc(joystick, CMD_DOWN))	cmd |= CMD_DOWN;

	if (joyFunc(joystick, CMD_BUTTON1))		cmd |= CMD_BUTTON1;

	if (joyFunc(joystick, CMD_BUTTON2))		cmd |= CMD_BUTTON2;

	if (joyFunc(joystick, CMD_BUTTON3))		cmd |= CMD_BUTTON3;

	if (joyFunc(joystick, CMD_BUTTON4))		cmd |= CMD_BUTTON4;

	return cmd;
}

static int GetOnePlayerCmd(
	KeyConfig *config,
	int(*keyFunc)(keyboard_t *, int),
	int(*mouseFunc)(Mouse *, int),
	int(*joyFunc)(joystick_t *, int),
	input_device_e device,
	int deviceIndex)
{
	int cmd = 0;
	switch (device)
	{
	case INPUT_DEVICE_KEYBOARD:
		cmd = GetKeyboardCmd(
			&gInputDevices.keyboard, &config->Keys, keyFunc);
		break;
	case INPUT_DEVICE_MOUSE:
		cmd = GetMouseCmd(&gInputDevices.mouse, mouseFunc, 0, Vec2iZero());
		break;
	case INPUT_DEVICE_JOYSTICK:
		{
			joystick_t *joystick = &gInputDevices.joysticks.joys[deviceIndex];
			cmd = GetJoystickCmd(joystick, joyFunc);
		}
		break;
	case INPUT_DEVICE_AI:
		// Do nothing; AI input is handled separately
		break;
	default:
		assert(0 && "unknown input device");
		break;
	}
	return cmd;
}

void GetPlayerCmds(
	int(*cmds)[MAX_PLAYERS], struct PlayerData playerDatas[MAX_PLAYERS])
{
	int(*keyFunc)(keyboard_t *, int) = KeyIsPressed;
	int(*mouseFunc)(Mouse *, int) = MouseIsPressed;
	int(*joyFunc)(joystick_t *, int) = JoyIsPressed;
	int i;
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		if (playerDatas[i].inputDevice == INPUT_DEVICE_UNSET)
		{
			continue;
		}
		(*cmds)[i] = GetOnePlayerCmd(
			&gConfig.Input.PlayerKeys[playerDatas[i].deviceIndex],
			keyFunc, mouseFunc, joyFunc,
			playerDatas[i].inputDevice, playerDatas[i].deviceIndex);
	}
}

int GetMenuCmd(struct PlayerData playerDatas[MAX_PLAYERS])
{
	int cmd;
	keyboard_t *kb = &gInputDevices.keyboard;
	if (KeyIsPressed(kb, SDLK_ESCAPE))
	{
		return CMD_ESC;
	}

	cmd = GetOnePlayerCmd(
		&gConfig.Input.PlayerKeys[0],
		KeyIsPressed, MouseIsPressed, JoyIsPressed,
		playerDatas[0].inputDevice, playerDatas[0].deviceIndex);
	if (!cmd)
	{
		if (KeyIsPressed(kb, SDLK_LEFT))		cmd |= CMD_LEFT;
		else if (KeyIsPressed(kb, SDLK_RIGHT))	cmd |= CMD_RIGHT;

		if (KeyIsPressed(kb, SDLK_UP))			cmd |= CMD_UP;
		else if (KeyIsPressed(kb, SDLK_DOWN))	cmd |= CMD_DOWN;

		if (KeyIsPressed(kb, SDLK_RETURN))		cmd |= CMD_BUTTON1;

		if (KeyIsPressed(kb, SDLK_BACKSPACE))	cmd |= CMD_BUTTON2;
	}

	return cmd;
}

int GetGameCmd(
	InputDevices *devices, InputConfig *config,
	struct PlayerData *playerData, Vec2i playerPos)
{
	int cmd = 0;
	joystick_t *joystick = &devices->joysticks.joys[0];

	switch (playerData->inputDevice)
	{
	case INPUT_DEVICE_KEYBOARD:
		cmd = GetKeyboardCmd(
			&devices->keyboard,
			&config->PlayerKeys[playerData->deviceIndex].Keys,
			KeyIsDown);
		break;
	case INPUT_DEVICE_MOUSE:
		cmd = GetMouseCmd(&devices->mouse, MouseIsDown, 1, playerPos);
		break;
	case INPUT_DEVICE_JOYSTICK:
		joystick =
			&devices->joysticks.joys[playerData->deviceIndex];
		cmd = GetJoystickCmd(joystick, JoyIsDown);
		break;
	default:
		// do nothing
		break;
	}

	return cmd;
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
