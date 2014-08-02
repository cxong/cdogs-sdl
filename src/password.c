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
#include <cdogs/input.h>
#include <cdogs/joystick.h>
#include <cdogs/keyboard.h>
#include <cdogs/sounds.h>
#include <cdogs/text.h>

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

	x = ((sum2 << 23) | (mission << 16) | sum1) ^ gCampaign.seed;
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

static int PasswordEntry(int cmd, char *buffer)
{
	int i, x, y;
	static const char letters[] = "abcdefghijklmnopqrstuvwxyz0123456789";
	static size_t selection;
	static int isFirst = 1;

	// Kludge since Watcom won't let me initialize selection with a strlen()
	if (isFirst)
	{
		selection = strlen(letters);
		isFirst = 0;
	}


	if (cmd & CMD_BUTTON1)
	{
		if (selection == strlen(letters))
		{
			SoundPlay(&gSoundDevice, StrSound("launch"));
			return 0;
		}

		if (strlen(buffer) < PASSWORD_MAX)
		{
			size_t l = strlen(buffer);
			buffer[l + 1] = 0;
			buffer[l] = letters[selection];
			SoundPlay(&gSoundDevice, StrSound("mg"));
		}
		else
		{
			SoundPlay(&gSoundDevice, SoundGetRandomScream(&gSoundDevice));
		}
	}
	else if (cmd & CMD_BUTTON2)
	{
		if (buffer[0]) {
			buffer[strlen(buffer) - 1] = 0;
			SoundPlay(&gSoundDevice, StrSound("bang"));
		}
		else
		{
			SoundPlay(&gSoundDevice, SoundGetRandomScream(&gSoundDevice));
		}
	}
	else if (cmd & CMD_LEFT)
	{
		if (selection > 0) {
			selection--;
			SoundPlay(&gSoundDevice, StrSound("door"));
		}
	}
	else if (cmd & CMD_RIGHT)
	{
		if (selection < strlen(letters))
		{
			selection++;
			SoundPlay(&gSoundDevice, StrSound("door"));
		}
	} else if (cmd & CMD_UP) {
		if (selection > 9) {
			selection -= 10;
			SoundPlay(&gSoundDevice, StrSound("door"));
		}
	}
	else if (cmd & CMD_DOWN)
	{
		if (selection < strlen(letters) - 9)
		{
			selection += 10;
			SoundPlay(&gSoundDevice, StrSound("door"));
		}
	}
	
	#define ENTRY_COLS	10
	#define	ENTRY_SPACING	12
	
	x = CenterX(((ENTRY_SPACING * (ENTRY_COLS - 1)) + FontW('a')));
	y = (int)CenterY(((FontH() * ((strlen(letters) - 1) / ENTRY_COLS) )));
	
	// Draw selection
	for (i = 0; i < (int)strlen(letters) + 1; i++)
	{
		Vec2i pos = Vec2iNew(
			x + (i % ENTRY_COLS) * ENTRY_SPACING,
			y + (i / ENTRY_COLS) * FontH());
		color_t mask = (i == (int)selection) ? colorRed : colorWhite;
		if (i < (int)strlen(letters))
		{
			FontChMask(letters[i], pos, mask);
		}
		else
		{
			FontStrMask(DONE, pos, mask);
		}
	}

	return 1;
}

static int EnterCode(GraphicsDevice *graphics, const char *password)
{
	int mission = 0;
	int done = 0;
	char buffer[PASSWORD_MAX + 1];

	strcpy(buffer, password);
	while (!done)
	{
		int cmd;
		EventPoll(&gEventHandlers, SDL_GetTicks());
		GraphicsBlitBkg(graphics);
		cmd = GetMenuCmd(&gEventHandlers, gPlayerDatas);
		if (!PasswordEntry(cmd, buffer))
		{
			if (!buffer[0])
			{
				mission = 0;
				done = 1;
			} else {
				mission = TestPassword(buffer);
				if (mission > 0)
					done = 1;
				else
				{
					SoundPlay(
						&gSoundDevice, SoundGetRandomScream(&gSoundDevice));
				}
			}
		}

		Vec2i pos = Vec2iNew(
			CenterX(FontStrW(buffer) + FontW('>') + FontW('<')),
			graphics->cachedConfig.Res.y / 4);
		pos = FontCh('>', pos);
		pos = FontStr(buffer, pos);
		pos = FontCh('<', pos);

		FontOpts opts = FontOptsNew();
		opts.HAlign = ALIGN_CENTER;
		opts.Area = gGraphicsDevice.cachedConfig.Res;
		opts.Pad.y = graphics->cachedConfig.Res.y / 12;
		FontStrOpt("Enter code", Vec2iZero(), opts);

		ShowControls();

		BlitFlip(graphics, &gConfig.Graphics);
	}

	SoundPlay(&gSoundDevice, StrSound("switch"));

	return mission;
}

typedef enum
{
	RETURN_CODE_CONTINUE,
	RETURN_CODE_START,
	RETURN_CODE_ENTER_CODE
} ReturnCode;

static void MenuCreateStart(MenuSystem *ms, int hasPassword);

int EnterPassword(GraphicsDevice *graphics, const char *password)
{
	MenuSystem startMenu;
	int mission = TestPassword(password);
	int hasPassword = mission > 0;
	int res = 0;
	MenuSystemInit(
		&startMenu, &gEventHandlers, graphics, Vec2iZero(),
		Vec2iNew(
			graphics->cachedConfig.Res.x,
			graphics->cachedConfig.Res.y));
	MenuCreateStart(&startMenu, hasPassword);
	for (;;)
	{
		int returnCode;
		MenuLoop(&startMenu);
		assert(startMenu.current->type == MENU_TYPE_RETURN);
		returnCode = startMenu.current->u.returnCode;
		if (returnCode == RETURN_CODE_CONTINUE)
		{
			res = mission;
			goto bail;
		}
		else if (returnCode == RETURN_CODE_START)
		{
			goto bail;
		}
		else if (returnCode == RETURN_CODE_ENTER_CODE)
		{
			int enteredMission = EnterCode(graphics, password);
			if (enteredMission > 0)
			{
				res = enteredMission;
				goto bail;
			}
			MenuReset(&startMenu);
		}
	}

bail:
	MenuSystemTerminate(&startMenu);
	return res;
}

static void MenuCreateStart(MenuSystem *ms, int hasPassword)
{
	ms->root = ms->current = MenuCreateNormal("", "", MENU_TYPE_NORMAL, 0);
	if (hasPassword)
	{
		MenuAddSubmenu(ms->root, MenuCreateReturn("Continue", RETURN_CODE_CONTINUE));
	}
	MenuAddSubmenu(ms->root, MenuCreateReturn("Start campaign", RETURN_CODE_START));
	MenuAddSubmenu(ms->root, MenuCreateReturn("Enter code...", RETURN_CODE_ENTER_CODE));
	MenuAddExitType(ms, MENU_TYPE_RETURN);
}
