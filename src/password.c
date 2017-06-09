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
#include "password.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <SDL_timer.h>

#include <cdogs/actors.h>
#include <cdogs/blit.h>
#include <cdogs/config.h>
#include <cdogs/defs.h>
#include <cdogs/font.h>
#include <cdogs/gamedata.h>
#include <cdogs/grafx.h>
#include <cdogs/grafx_bg.h>
#include <cdogs/joystick.h>
#include <cdogs/keyboard.h>
#include <cdogs/sounds.h>

#include "autosave.h"
#include "menu.h"

#define DONE          "Done"


const char *MakePassword(int mission, int isTwoPlayers)
{
	static char s[PASSWORD_MAX + 1];
	int sum1, sum2, count;
	size_t i, x;
	static char *alphabet1 = "0123456789abcdefghijklmnopqrstuvwxyz";
	static char *alphabet2 = "9876543210kjihgfedcbazyxwvutsrqponml";
	char *alphabet = isTwoPlayers ? alphabet2 : alphabet1;
	size_t base = strlen(alphabet);

	sum1 = sum2 = 0;
	for (i = 0; i < strlen(gCampaign.Setting.Title); i++)
	{
		sum1 += gCampaign.Setting.Title[i];
		sum2 ^= gCampaign.Setting.Title[i];
	}

	const int seed = ConfigGetInt(&gConfig, "Game.RandomSeed");
	x = ((sum2 << 23) | (mission << 16) | sum1) ^ (size_t)seed;
	count = 0;
	while (x > 0 && count < PASSWORD_MAX) {
		i = x % base;
		s[count++] = alphabet[i];
		x /= base;
	}
	s[count] = 0;
	return s;
}

static int TestPassword(const char *password)
{
	int i;

	for (i = 0; i < (int)gCampaign.Setting.Missions.size; i++)
	{
		// For legacy passwords, try both one and two players
		if (strcmp(password, MakePassword(i, 0)) == 0 ||
			strcmp(password, MakePassword(i, 1)) == 0)
		{
			return i;
		}
	}
	return -1;
}

// Give user an alpha-num entry screen and returns the decoded mission number.
// If the password is invalid, the screen won't exit.
// If the user cancels, 0 is returned.
typedef struct
{
	char Buffer[PASSWORD_MAX + 1];
	int Mission;
	int Selection;
} EnterCodeScreenData;
static GameLoopResult EnterCodeScreenUpdate(void *data);
static void EnterCodeScreenDraw(void *data);
static int EnterCodeScreen(const char *password)
{
	EnterCodeScreenData data;
	memset(&data, 0, sizeof data);
	data.Selection = -1;
	strcpy(data.Buffer, password);

	GameLoopData gData = GameLoopDataNew(
		&data, EnterCodeScreenUpdate,
		&data, EnterCodeScreenDraw);
	GameLoop(&gData);
	if (data.Mission > 0)
	{
		SoundPlay(&gSoundDevice, StrSound("mg"));
	}
	else
	{
		SoundPlay(&gSoundDevice, StrSound("switch"));
	}

	return data.Mission;
}
#define PASSWORD_ENTRY_COLS	10
#define PASSWORD_LETTERS "abcdefghijklmnopqrstuvwxyz0123456789"
static bool PasswordEntry(EnterCodeScreenData *data, const int cmd);
static GameLoopResult EnterCodeScreenUpdate(void *data)
{
	EnterCodeScreenData *eData = data;

	// Check if anyone pressed escape
	int cmds[MAX_LOCAL_PLAYERS];
	memset(cmds, 0, sizeof cmds);
	GetPlayerCmds(&gEventHandlers, &cmds);
	if (EventIsEscape(&gEventHandlers, cmds, GetMenuCmd(&gEventHandlers)))
	{
		eData->Mission = 0;
		return UPDATE_RESULT_EXIT;
	}

	const int cmd = GetMenuCmd(&gEventHandlers);
	// Returning 1 means we are still entering password
	if (PasswordEntry(eData, cmd))
	{
		return UPDATE_RESULT_DRAW;
	}

	if (strlen(eData->Buffer) == 0)
	{
		// Nothing in the password buffer; exit
		eData->Mission = 0;
		return UPDATE_RESULT_EXIT;
	}

	eData->Mission = TestPassword(eData->Buffer);
	if (eData->Mission == 0)
	{
		// Password doesn't work; play a bad sound
		SoundPlay(&gSoundDevice, StrSound("aargh/man"));
		return UPDATE_RESULT_OK;
	}

	// Password works; exit
	return UPDATE_RESULT_EXIT;
}
static bool PasswordEntry(EnterCodeScreenData *data, const int cmd)
{
	if (data->Selection == -1)
	{
		data->Selection = (int)strlen(PASSWORD_LETTERS);
	}

	if (cmd & CMD_BUTTON1)
	{
		if (data->Selection == (int)strlen(PASSWORD_LETTERS))
		{
			// Done
			return false;
		}

		if (strlen(data->Buffer) < PASSWORD_MAX)
		{
			// enter a letter
			data->Buffer[strlen(data->Buffer)] =
				PASSWORD_LETTERS[data->Selection];
			SoundPlay(&gSoundDevice, StrSound("mg"));
		}
		else
		{
			// Too many letters entered
			SoundPlay(&gSoundDevice, StrSound("aargh/man"));
		}
	}
	else if (cmd & CMD_BUTTON2)
	{
		if (strlen(data->Buffer) > 0)
		{
			// Delete a letter
			data->Buffer[strlen(data->Buffer) - 1] = 0;
			SoundPlay(&gSoundDevice, StrSound("bang"));
		}
		else
		{
			// No letters to delete
			SoundPlay(&gSoundDevice, StrSound("aargh/man"));
		}
	}
	else if (cmd & CMD_LEFT)
	{
		if (data->Selection > 0)
		{
			data->Selection--;
			SoundPlay(&gSoundDevice, StrSound("door"));
		}
	}
	else if (cmd & CMD_RIGHT)
	{
		if (data->Selection < (int)strlen(PASSWORD_LETTERS))
		{
			data->Selection++;
			SoundPlay(&gSoundDevice, StrSound("door"));
		}
	}
	else if (cmd & CMD_UP)
	{
		if (data->Selection >= PASSWORD_ENTRY_COLS)
		{
			data->Selection -= PASSWORD_ENTRY_COLS;
			SoundPlay(&gSoundDevice, StrSound("door"));
		}
	}
	else if (cmd & CMD_DOWN)
	{
		if (data->Selection <=
			(int)strlen(PASSWORD_LETTERS) - PASSWORD_ENTRY_COLS)
		{
			data->Selection += PASSWORD_ENTRY_COLS;
			SoundPlay(&gSoundDevice, StrSound("door"));
		}
	}

	return true;
}
static void EnterCodeScreenDraw(void *data)
{
	const EnterCodeScreenData *eData = data;

	GraphicsClear(&gGraphicsDevice);

	// Password display
	Vec2i pos = Vec2iNew(
		CenterX(FontStrW(eData->Buffer) + FontW('>') + FontW('<')),
		gGraphicsDevice.cachedConfig.Res.y / 4);
	pos = FontCh('>', pos);
	pos = FontStr(eData->Buffer, pos);
	FontCh('<', pos);

	FontOpts opts = FontOptsNew();
	opts.HAlign = ALIGN_CENTER;
	opts.Area = gGraphicsDevice.cachedConfig.Res;
	opts.Pad.y = gGraphicsDevice.cachedConfig.Res.y / 12;
	FontStrOpt("Enter code", Vec2iZero(), opts);

	// Draw password entry letters
#define	ENTRY_SPACING	12

	const int x =
		CenterX(ENTRY_SPACING * (PASSWORD_ENTRY_COLS - 1) + FontW('a'));
	const int y = (int)CenterY(
		FontH() * ((strlen(PASSWORD_LETTERS) - 1) / PASSWORD_ENTRY_COLS));
	for (int i = 0; i < (int)strlen(PASSWORD_LETTERS) + 1; i++)
	{
		pos = Vec2iNew(
			x + (i % PASSWORD_ENTRY_COLS) * ENTRY_SPACING,
			y + (i / PASSWORD_ENTRY_COLS) * FontH());
		color_t mask = (i == eData->Selection) ? colorRed : colorWhite;
		if (i < (int)strlen(PASSWORD_LETTERS))
		{
			FontChMask(PASSWORD_LETTERS[i], pos, mask);
		}
		else
		{
			FontStrMask(DONE, pos, mask);
		}
	}

	ShowControls();
}

typedef enum
{
	RETURN_CODE_CONTINUE = -1,
	RETURN_CODE_START = -2,
	RETURN_CODE_ENTER_CODE = -3
} ReturnCode;

static void MenuCreateStart(
	MenuSystem *ms, const int mission, const MissionSave *save);
int EnterPassword(GraphicsDevice *graphics, const MissionSave *save)
{
	MenuSystem startMenu;
	const int mission = TestPassword(save->Password);
	int res = 0;
	MenuSystemInit(
		&startMenu, &gEventHandlers, graphics, Vec2iZero(),
		Vec2iNew(
			graphics->cachedConfig.Res.x,
			graphics->cachedConfig.Res.y));
	MenuCreateStart(&startMenu, mission, save);
	for (;;)
	{
		int returnCode;
		MenuLoop(&startMenu);
		assert(startMenu.current->type == MENU_TYPE_RETURN);
		returnCode = startMenu.current->u.returnCode;
		switch (returnCode)
		{
		case RETURN_CODE_CONTINUE:
			res = mission;
			goto bail;
		case RETURN_CODE_START:
			goto bail;
		case RETURN_CODE_ENTER_CODE:
			{
				int enteredMission = EnterCodeScreen(save->Password);
				if (enteredMission > 0)
				{
					res = enteredMission;
					goto bail;
				}
				MenuReset(&startMenu);
			}
			break;
		default:
			// Return code represents the mission to start on
			CASSERT(returnCode >= 0, "Invalid return code for password menu");
			res = returnCode;
			goto bail;
		}
	}

bail:
	MenuSystemTerminate(&startMenu);
	return res;
}
static void MenuCreateStart(
	MenuSystem *ms, const int mission, const MissionSave *save)
{
	ms->root = ms->current = MenuCreateNormal("", "", MENU_TYPE_NORMAL, 0);

	menu_t *menuContinue = MenuCreateReturn("Continue", RETURN_CODE_CONTINUE);
	// Note: mission can be -1
	menuContinue->isDisabled = mission <= 0;
	MenuAddSubmenu(ms->root, menuContinue);

	// Create level select menus
	menu_t *levelSelect = MenuCreateNormal(
		"Level select...", "Select Level", MENU_TYPE_NORMAL, 0);
	for (int i = 0;
		i < MIN(save->MissionsCompleted + 1, (int)gCampaign.Setting.Missions.size);
		i++)
	{
		const Mission *m = CArrayGet(&gCampaign.Setting.Missions, i);
		char buf[CDOGS_FILENAME_MAX];
		sprintf(buf, "%d: %s", i + 1, m->Title);
		menu_t *l = MenuCreateReturn(buf, i);
		l->isDisabled = i > save->MissionsCompleted;
		MenuAddSubmenu(levelSelect, l);
	}
	levelSelect->isDisabled = save->MissionsCompleted == 0;
	MenuAddSubmenu(ms->root, levelSelect);

	MenuAddSubmenu(ms->root, MenuCreateReturn("Start campaign", RETURN_CODE_START));

	MenuAddSubmenu(ms->root, MenuCreateReturn("Enter code...", RETURN_CODE_ENTER_CODE));

	MenuAddExitType(ms, MENU_TYPE_RETURN);
}
