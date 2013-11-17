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
#include "weapon_menu.h"


int PlayerSelection(int numPlayers, GraphicsDevice *graphics)
{
	int i;
	PlayerSelectMenu menus[MAX_PLAYERS];
	for (i = 0; i < numPlayers; i++)
	{
		PlayerSelectMenusCreate(
			&menus[i], numPlayers, i,
			&gCampaign.Setting.characters.players[i], &gPlayerDatas[i],
			&gInputDevices, graphics, &gConfig.Input.PlayerKeys[i]);
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

int PlayerEquip(int numPlayers, GraphicsDevice *graphics)
{
	int i;
	WeaponMenu menus[MAX_PLAYERS];
	for (i = 0; i < numPlayers; i++)
	{
		WeaponMenuCreate(
			&menus[i], numPlayers, i,
			&gCampaign.Setting.characters.players[i], &gPlayerDatas[i],
			&gInputDevices, graphics, &gConfig.Input.PlayerKeys[i]);
	}

	debug(D_NORMAL, "\n");

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
			return 0; // hack to exit from menu
		}

		for (i = 0; i < numPlayers; i++)
		{
			MenuProcessCmd(&menus[i].ms, cmds[i]);
		}

		for (i = 0; i < numPlayers; i++)
		{
			if (!MenuIsExit(&menus[i].ms) || gPlayerDatas[i].weaponCount == 0)
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
