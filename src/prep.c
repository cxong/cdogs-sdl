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
#include "prep.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <SDL_timer.h>

#include <cdogs/ai_coop.h>
#include <cdogs/actors.h>
#include <cdogs/blit.h>
#include <cdogs/config.h>
#include <cdogs/draw.h>
#include <cdogs/files.h>
#include <cdogs/grafx.h>
#include <cdogs/input.h>
#include <cdogs/joystick.h>
#include <cdogs/keyboard.h>
#include <cdogs/music.h>
#include <cdogs/pic_manager.h>
#include <cdogs/sounds.h>
#include <cdogs/text.h>
#include <cdogs/utils.h>

#include "player_select_menus.h"
#include "weapon_menu.h"


int NumPlayersSelection(
	int *numPlayers, campaign_mode_e mode,
	GraphicsDevice *graphics, EventHandlers *handlers)
{
	MenuSystem ms;
	int i;
	int res = 0;
	MenuSystemInit(
		&ms, handlers, graphics,
		Vec2iZero(),
		Vec2iNew(
			graphics->cachedConfig.Res.x,
			graphics->cachedConfig.Res.y));
	ms.root = ms.current = MenuCreateNormal(
		"",
		"Select number of players",
		MENU_TYPE_NORMAL,
		0);
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		char buf[2];
		if (mode == CAMPAIGN_MODE_DOGFIGHT && i == 0)
		{
			// At least two players for dogfights
			continue;
		}
		sprintf(buf, "%d", i + 1);
		MenuAddSubmenu(ms.current, MenuCreateReturn(buf, i + 1));
	}
	MenuAddExitType(&ms, MENU_TYPE_RETURN);

	for (;;)
	{
#ifndef RUN_WITHOUT_APP_FOCUS
		MusicSetPlaying(&gSoundDevice, SDL_GetAppState() & SDL_APPINPUTFOCUS);
#endif
		int cmd;
		EventPoll(&gEventHandlers, SDL_GetTicks());
		if (KeyIsPressed(&gEventHandlers.keyboard, SDLK_ESCAPE) ||
			JoyIsPressed(&gEventHandlers.joysticks.joys[0], CMD_BUTTON4) ||
			gEventHandlers.HasQuit)
		{
			res = 0;
			break;	// hack to allow exit
		}
		cmd = GetMenuCmd(handlers, gPlayerDatas);
		MenuProcessCmd(&ms, cmd);
		if (MenuIsExit(&ms))
		{
			*numPlayers = ms.current->u.returnCode;
			res = 1;
			break;
		}

		GraphicsBlitBkg(graphics);
		MenuDisplay(&ms);
		BlitFlip(graphics, &gConfig.Graphics);
		SDL_Delay(10);
	}

	MenuSystemTerminate(&ms);
	return res;
}


static void AssignPlayerInputDevice(
	struct PlayerData *pData, input_device_e d, int idx)
{
	pData->inputDevice = d;
	pData->deviceIndex = idx;
}

static void AssignPlayerInputDevices(
	bool hasInputDevice[MAX_PLAYERS], int numPlayers,
	struct PlayerData playerDatas[MAX_PLAYERS],
	EventHandlers *handlers, InputConfig *inputConfig)
{
	bool assignedKeyboards[MAX_KEYBOARD_CONFIGS];
	bool assignedMouse = false;
	bool assignedJoysticks[MAX_JOYSTICKS];
	bool assignedNet = false;
	memset(assignedKeyboards, 0, sizeof assignedKeyboards);
	memset(assignedJoysticks, 0, sizeof assignedJoysticks);

	for (int i = 0; i < numPlayers; i++)
	{
		if (hasInputDevice[i])
		{
			// Find all the assigned devices
			switch (playerDatas[i].inputDevice)
			{
			case INPUT_DEVICE_KEYBOARD:
				assignedKeyboards[playerDatas[i].deviceIndex] = 1;
				break;
			case INPUT_DEVICE_MOUSE:
				assignedMouse = true;
				break;
			case INPUT_DEVICE_JOYSTICK:
				assignedJoysticks[playerDatas[i].deviceIndex] = 1;
				break;
			case INPUT_DEVICE_NET:
				assignedNet = true;
				break;
			default:
				// do nothing
				break;
			}
			continue;
		}

		// Try to assign devices to players
		// For each unassigned player, check if any device has button 1 pressed
		for (int j = 0; j < MAX_KEYBOARD_CONFIGS; j++)
		{
			if (KeyIsPressed(
				&handlers->keyboard,
				inputConfig->PlayerKeys[j].Keys.button1) &&
				!assignedKeyboards[j])
			{
				hasInputDevice[i] = 1;
				AssignPlayerInputDevice(
					&playerDatas[i], INPUT_DEVICE_KEYBOARD, j);
				assignedKeyboards[j] = true;
				SoundPlay(&gSoundDevice, SND_HAHAHA);
				continue;
			}
		}
		if (MouseIsPressed(&handlers->mouse, SDL_BUTTON_LEFT) &&
			!assignedMouse)
		{
			hasInputDevice[i] = 1;
			AssignPlayerInputDevice(&playerDatas[i], INPUT_DEVICE_MOUSE, 0);
			assignedMouse = true;
			SoundPlay(&gSoundDevice, SND_HAHAHA);
			continue;
		}
		for (int j = 0; j < handlers->joysticks.numJoys; j++)
		{
			if (JoyIsPressed(
				&handlers->joysticks.joys[j], CMD_BUTTON1) &&
				!assignedJoysticks[j])
			{
				hasInputDevice[i] = 1;
				AssignPlayerInputDevice(
					&playerDatas[i], INPUT_DEVICE_JOYSTICK, j);
				assignedJoysticks[j] = true;
				SoundPlay(&gSoundDevice, SND_HAHAHA);
				continue;
			}
		}
		if ((handlers->netInput.Cmd & CMD_BUTTON1 & ~handlers->netInput.PrevCmd) &&
			!assignedNet)
		{
			hasInputDevice[i] = 1;
			AssignPlayerInputDevice(&playerDatas[i], INPUT_DEVICE_NET, 0);
			assignedNet = true;
			SoundPlay(&gSoundDevice, SND_HAHAHA);
			continue;
		}
	}
}

int PlayerSelection(int numPlayers, GraphicsDevice *graphics)
{
	bool hasInputDevice[MAX_PLAYERS];
	PlayerSelectMenu menus[MAX_PLAYERS];
	for (int i = 0; i < numPlayers; i++)
	{
		PlayerSelectMenusCreate(
			&menus[i], numPlayers, i,
			&gCampaign.Setting.characters.players[i], &gPlayerDatas[i],
			&gEventHandlers, graphics, &gConfig.Input);
		hasInputDevice[i] = false;
	}

	bool res = true;
	bool hasNetInput = false;
	KeyInit(&gEventHandlers.keyboard);
	NetInputOpen(&gEventHandlers.netInput);
	for (;;)
	{
#ifndef RUN_WITHOUT_APP_FOCUS
		MusicSetPlaying(&gSoundDevice, SDL_GetAppState() & SDL_APPINPUTFOCUS);
#endif
		int cmds[MAX_PLAYERS];
		int isDone = 1;
		int hasAtLeastOneInput = 0;
		EventPoll(&gEventHandlers, SDL_GetTicks());
		if (KeyIsPressed(&gEventHandlers.keyboard, SDLK_ESCAPE) ||
			JoyIsPressed(&gEventHandlers.joysticks.joys[0], CMD_BUTTON4) ||
			gEventHandlers.HasQuit)
		{
			res = false;
			goto bail;
		}
		GetPlayerCmds(&gEventHandlers, &cmds, gPlayerDatas);
		for (int i = 0; i < numPlayers; i++)
		{
			if (hasInputDevice[i] && !MenuIsExit(&menus[i].ms))
			{
				MenuProcessCmd(&menus[i].ms, cmds[i]);
			}
		}

		// Conditions for exit: at least one player has selected "Done",
		// and no other players, if any, are still selecting their player
		// The "players" with no input device are turned into AIs
		for (int i = 0; i < numPlayers; i++)
		{
			if (hasInputDevice[i])
			{
				hasAtLeastOneInput = 1;
			}
			if (strcmp(menus[i].ms.current->name, "Done") != 0 &&
				hasInputDevice[i])
			{
				isDone = 0;
			}
		}
		if (isDone && hasAtLeastOneInput)
		{
			break;
		}

		AssignPlayerInputDevices(
			hasInputDevice, numPlayers,
			gPlayerDatas, &gEventHandlers, &gConfig.Input);

		GraphicsBlitBkg(graphics);
		for (int i = 0; i < numPlayers; i++)
		{
			if (hasInputDevice[i])
			{
				MenuDisplay(&menus[i].ms);
			}
			else
			{
				Vec2i center = Vec2iZero();
				const char *prompt = "Press Fire to join...";
				Vec2i offset = Vec2iScaleDiv(TextGetSize(prompt), -2);
				int w = graphics->cachedConfig.Res.x;
				int h = graphics->cachedConfig.Res.y;
				switch (numPlayers)
				{
				case 1:
					// Center of screen
					center = Vec2iNew(w / 2, h / 2);
					break;
				case 2:
					// Side by side
					center = Vec2iNew(i * w / 2 + w / 4, h / 2);
					break;
				case 3:
				case 4:
					// Four corners
					center = Vec2iNew(
						(i & 1) * w / 2 + w / 4, (i / 2) * h / 2 + h / 4);
					break;
				default:
					assert(0 && "not implemented");
					break;
				}
				TextString(
					&gTextManager, prompt, graphics, Vec2iAdd(center, offset));
			}
		}
		BlitFlip(graphics, &gConfig.Graphics);
		SDL_Delay(10);
	}

	// For any player slots not picked, turn them into AIs
	for (int i = 0; i < numPlayers; i++)
	{
		if (!hasInputDevice[i])
		{
			AssignPlayerInputDevice(&gPlayerDatas[i], INPUT_DEVICE_AI, 0);
		}
	}

	// If no net input devices selected, close the connection
	for (int i = 0; i < numPlayers; i++)
	{
		if (hasInputDevice[i] &&
			gPlayerDatas[i].inputDevice == INPUT_DEVICE_NET)
		{
			hasNetInput = true;
			break;
		}
	}

bail:
	if (!hasNetInput)
	{
		NetInputTerminate(&gEventHandlers.netInput);
	}

	for (int i = 0; i < numPlayers; i++)
	{
		MenuSystemTerminate(&menus[i].ms);
	}
	return res;
}

bool PlayerEquip(int numPlayers, GraphicsDevice *graphics)
{
	int i;
	WeaponMenu menus[MAX_PLAYERS];
	for (i = 0; i < numPlayers; i++)
	{
		WeaponMenuCreate(
			&menus[i], numPlayers, i,
			&gCampaign.Setting.characters.players[i], &gPlayerDatas[i],
			&gEventHandlers, graphics, &gConfig.Input);
		// For AI players, pre-pick their weapons and go straight to menu end
		if (gPlayerDatas[i].inputDevice == INPUT_DEVICE_AI)
		{
			int lastMenuIndex =
				(int)menus[i].ms.root->u.normal.subMenus.size - 1;
			menus[i].ms.current =
				CArrayGet(&menus[i].ms.root->u.normal.subMenus, lastMenuIndex);
			gPlayerDatas[i].weapons[0] = AICoopSelectWeapon(
				i, gMission.missionData->Weapons);
			gPlayerDatas[i].weaponCount = 1;
			// TODO: select more weapons, or select weapons based on mission
		}
	}

	debug(D_NORMAL, "\n");

	bool res = true;
	for (;;)
	{
#ifndef RUN_WITHOUT_APP_FOCUS
		MusicSetPlaying(&gSoundDevice, SDL_GetAppState() & SDL_APPINPUTFOCUS);
#endif
		int cmds[MAX_PLAYERS];
		int isDone = 1;
		EventPoll(&gEventHandlers, SDL_GetTicks());
		// Check exit
		if (KeyIsPressed(&gEventHandlers.keyboard, SDLK_ESCAPE) ||
			JoyIsPressed(&gEventHandlers.joysticks.joys[0], CMD_BUTTON4) ||
			gEventHandlers.HasQuit)
		{
			res = false;
			goto bail;
		}
		GetPlayerCmds(&gEventHandlers, &cmds, gPlayerDatas);
		for (i = 0; i < numPlayers; i++)
		{
			if (!MenuIsExit(&menus[i].ms))
			{
				MenuProcessCmd(&menus[i].ms, cmds[i]);
			}
			else if (gPlayerDatas[i].weaponCount == 0)
			{
				// Check exit condition; must have selected at least one weapon
				// Otherwise reset the current menu
				menus[i].ms.current = menus[i].ms.root;
			}
		}
		for (i = 0; i < numPlayers; i++)
		{
			if (strcmp(menus[i].ms.current->name, "(End)") != 0)
			{
				isDone = 0;
			}
		}
		if (isDone)
		{
			break;
		}

		GraphicsBlitBkg(graphics);
		for (i = 0; i < numPlayers; i++)
		{
			MenuDisplay(&menus[i].ms);
		}
		BlitFlip(graphics, &gConfig.Graphics);
		SDL_Delay(10);
	}

bail:
	for (i = 0; i < numPlayers; i++)
	{
		MenuSystemTerminate(&menus[i].ms);
	}

	return res;
}
