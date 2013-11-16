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
#include "prep.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <SDL_timer.h>

#include <cdogs/actors.h>
#include <cdogs/blit.h>
#include <cdogs/config.h>
#include <cdogs/draw.h>
#include <cdogs/files.h>
#include <cdogs/grafx.h>
#include <cdogs/input.h>
#include <cdogs/joystick.h>
#include <cdogs/keyboard.h>
#include <cdogs/pic_manager.h>
#include <cdogs/sounds.h>
#include <cdogs/text.h>
#include <cdogs/utils.h>

#include "player_select_menus.h"


const char *endChoice = "(End)";


static void ShowPlayerControls(int x, KeyConfig *config)
{
	char s[256];
	int y = gGraphicsDevice.cachedConfig.ResolutionHeight - (gGraphicsDevice.cachedConfig.ResolutionHeight / 6);

	if (config->Device == INPUT_DEVICE_KEYBOARD)
	{
		sprintf(s, "(%s, %s, %s, %s, %s and %s)",
			SDL_GetKeyName(config->Keys.left),
			SDL_GetKeyName(config->Keys.right),
			SDL_GetKeyName(config->Keys.up),
			SDL_GetKeyName(config->Keys.down),
			SDL_GetKeyName(config->Keys.button1),
			SDL_GetKeyName(config->Keys.button2));
		if (TextGetStringWidth(s) < 125)
		{
			CDogsTextStringAt(x, y, s);
		}
		else
		{
			sprintf(s, "(%s, %s, %s,",
				SDL_GetKeyName(config->Keys.left),
				SDL_GetKeyName(config->Keys.right),
				SDL_GetKeyName(config->Keys.up));
			CDogsTextStringAt(x, y - 10, s);
			sprintf(s, "%s, %s and %s)",
				SDL_GetKeyName(config->Keys.down),
				SDL_GetKeyName(config->Keys.button1),
				SDL_GetKeyName(config->Keys.button2));
			CDogsTextStringAt(x, y, s);
		}
	}
	else if (config->Device == INPUT_DEVICE_MOUSE)
	{
		sprintf(s, "(mouse wheel to scroll, left and right click)");
		CDogsTextStringAt(x, y, s);
	}
	else
	{
		sprintf(s, "(%s)", InputDeviceName(config->Device));
		CDogsTextStringAt(x, y, s);
	}
}

static void ShowSelection(int x, struct PlayerData *data, Character *ch)
{
	DisplayPlayer(x, data->name, ch, 0);

	if (data->weaponCount == 0)
	{
		CDogsTextStringAt(
			x + 40,
			(gGraphicsDevice.cachedConfig.ResolutionHeight / 10) + 20,
			"None selected...");
	}
	else
	{
		int i;
		for (i = 0; i < data->weaponCount; i++)
		{
			CDogsTextStringAt(
				x + 40,
				(gGraphicsDevice.cachedConfig.ResolutionHeight / 10) + 20 + i * CDogsTextHeight(),
				gGunDescriptions[data->weapons[i]].gunName);
		}
	}
}

static void SetPlayer(Character *c, struct PlayerData *data)
{
	data->looks.armedBody = BODY_ARMED;
	data->looks.unarmedBody = BODY_UNARMED;
	CharacterSetLooks(c, &data->looks);
	c->speed = 256;
	c->maxHealth = 200;
}

static int WeaponSelection(
	int x, int idx, struct PlayerData *data, int cmd, int done)
{
	int i;
	int y;
	static int selection[2] = { 0, 0 };

	debug(D_VERBOSE, "\n");

	if (selection[idx] > gMission.weaponCount)
	{
		selection[idx] = gMission.weaponCount;
	}

	if (cmd & CMD_BUTTON1)
	{
		if (selection[idx] == gMission.weaponCount)
		{
			SoundPlay(&gSoundDevice, SND_KILL2);
			return data->weaponCount > 0 ? 0 : 1;
		}

		if (data->weaponCount < MAX_WEAPONS)
		{
			for (i = 0; i < data->weaponCount; i++)
			{
				if ((int)data->weapons[i] ==
						gMission.availableWeapons[selection[idx]])
				{
					return 1;
				}
			}

			data->weapons[data->weaponCount] =
				gMission.availableWeapons[selection[idx]];
			data->weaponCount++;

			SoundPlay(&gSoundDevice, SND_SHOTGUN);
		}
		else
		{
			SoundPlay(&gSoundDevice, SND_KILL);
		}
	} else if (cmd & CMD_BUTTON2) {
		if (data->weaponCount) {
			data->weaponCount--;
			SoundPlay(&gSoundDevice, SND_PICKUP);
			done = 0;
		}
	}
	else if (cmd & (CMD_LEFT | CMD_UP))
	{
		if (selection[idx] > 0)
		{
			selection[idx]--;
			SoundPlay(&gSoundDevice, SND_SWITCH);
		}
		else if (selection[idx] == 0)
		{
			selection[idx] = gMission.weaponCount;
			SoundPlay(&gSoundDevice, SND_SWITCH);
		}
		done = 0;
	}
	else if (cmd & (CMD_RIGHT | CMD_DOWN))
	{
		if (selection[idx] < gMission.weaponCount)
		{
			selection[idx]++;
			SoundPlay(&gSoundDevice, SND_SWITCH);
		}
		else if (selection[idx] == gMission.weaponCount)
		{
			selection[idx] = 0;
			SoundPlay(&gSoundDevice, SND_SWITCH);
		}
		done = 0;
	}

	if (!done) {
		y = CenterY((CDogsTextHeight() * gMission.weaponCount));

		for (i = 0; i < gMission.weaponCount; i++)
		{
			DisplayMenuItem(
				x,
				y + i * CDogsTextHeight(),
				gGunDescriptions[gMission.availableWeapons[i]].gunName,
				i == selection[idx]);
		}

		DisplayMenuItem(x, y + i * CDogsTextHeight(), endChoice, i == selection[idx]);
	}

	return !done;
}

int PlayerSelection(int numPlayers, GraphicsDevice *graphics)
{
	int i;
	PlayerSelectMenu menus[MAX_PLAYERS];
	for (i = 0; i < numPlayers; i++)
	{
		PlayerSelectMenusCreate(
			&menus[i], numPlayers, i,
			&gCampaign.Setting.characters.players[i], &gPlayerDatas[i],
			&gInputDevices, graphics);
	}

	KeyInit(&gInputDevices.keyboard);
	for (;;)
	{
		int cmds[MAX_PLAYERS];
		int isDone = 1;
		InputPoll(&gInputDevices, SDL_GetTicks());
		GraphicsBlitBkg(graphics);
		GetPlayerCmds(&cmds);

		for (i = 0; i < numPlayers; i++)
		{
			MenuDisplay(&menus[i].ms);
		}

		if (KeyIsPressed(&gInputDevices.keyboard, SDLK_ESCAPE))
		{
			// TODO: destroy menus
			return 0; // hack to allow exit
		}

		for (i = 0; i < numPlayers; i++)
		{
			MenuProcessCmd(&menus[i].ms, cmds[i]);
		}

		for (i = 0; i < numPlayers; i++)
		{
			if (!MenuIsExit(&menus[i].ms))
			{
				isDone = 0;
			}
		}
		if (isDone)
		{
			break;
		}

		BlitFlip(graphics, &gConfig.Graphics);
		SDL_Delay(10);
	}

	for (i = 0; i < numPlayers; i++)
	{
		MenuSystemTerminate(&menus[i].ms);
	}
	return 1;
}

int PlayerEquip(GraphicsDevice *graphics)
{
	int dones[MAX_PLAYERS];
	int i;

	debug(D_NORMAL, "\n");

	for (i = 0; i < MAX_PLAYERS; i++)
	{
		if (i < gOptions.numPlayers)
		{
			dones[i] = 0;
		}
		else
		{
			dones[i] = 1;
		}
	}

	for (;;)
	{
		int cmds[MAX_PLAYERS];
		int isDone = 1;
		InputPoll(&gInputDevices, SDL_GetTicks());
		GraphicsBlitBkg(graphics);
		GetPlayerCmds(&cmds);

		if (KeyIsPressed(&gInputDevices.keyboard, SDLK_ESCAPE))
		{
			return 0; // hack to exit from menu
		}

		switch (gOptions.numPlayers)
		{
			case 1:
				dones[0] = !WeaponSelection(CenterX(80), CHARACTER_PLAYER1, &gPlayerDatas[0], cmds[0], dones[0]);
				ShowSelection(
					CenterX(80),
					&gPlayerDatas[0],
					&gCampaign.Setting.characters.players[0]);
				ShowPlayerControls(CenterX(100), &gConfig.Input.PlayerKeys[0]);
				break;
			case 2:
				dones[0] = !WeaponSelection(CenterOfLeft(50), CHARACTER_PLAYER1, &gPlayerDatas[0], cmds[0], dones[0]);
				ShowSelection(
					CenterOfLeft(50),
					&gPlayerDatas[0],
					&gCampaign.Setting.characters.players[0]);
				ShowPlayerControls(CenterOfLeft(100), &gConfig.Input.PlayerKeys[0]);

				dones[1] = !WeaponSelection(CenterOfRight(50), CHARACTER_PLAYER2, &gPlayerDatas[1], cmds[1], dones[1]);
				ShowSelection(
					CenterOfRight(50),
					&gPlayerDatas[1],
					&gCampaign.Setting.characters.players[1]);
				ShowPlayerControls(CenterOfRight(100), &gConfig.Input.PlayerKeys[1]);
				break;
			default:
				assert(0 && "not implemented");
				break;
		}

		for (i = 0; i < MAX_PLAYERS; i++)
		{
			if (!dones[i])
			{
				isDone = 0;
			}
		}
		if (isDone)
		{
			break;
		}

		BlitFlip(graphics, &gConfig.Graphics);
		SDL_Delay(10);
	}

	return 1;
}
