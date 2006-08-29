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

 password.c - mission password functions
 
 Author: $Author$
 Rev:    $Revision$
 URL:    $HeadURL$
 ID:     $Id$
 
*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "input.h"
#include "grafx.h"
#include "blit.h"
#include "text.h"
#include "sounds.h"
#include "actors.h"
#include "defs.h"
#include "gamedata.h"
#include "menu.h"
#include "password.h"


#define DONE          "Done"


const char *MakePassword(int mission)
{
	static char s[PASSWORD_MAX + 1];
	int i, sum1, sum2, x, count;
	static char *alphabet1 = "0123456789abcdefghijklmnopqrstuvwxyz";
	static char *alphabet2 = "9876543210kjihgfedcbazyxwvutsrqponml";
	char *alphabet = (gOptions.twoPlayers ? alphabet2 : alphabet1);
	int base = strlen(alphabet);

	sum1 = sum2 = 0;
	for (i = 0; i < strlen(gCampaign.setting->title); i++) {
		sum1 += gCampaign.setting->title[i];
		sum2 ^= gCampaign.setting->title[i];
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

	for (i = 0; i < gCampaign.setting->missionCount; i++)
		if (strcmp(password, MakePassword(i)) == 0)
			return i;
	return -1;
}

static int PasswordEntry(int cmd, char *buffer)
{
	int i;
	static char letters[] = "abcdefghijklmnopqrstuvwxyz0123456789";
	static int selection = -1;

	// Kludge since Watcom won't let me initialize selection with a strlen()
	if (selection < 0)
		selection = strlen(letters);

	if (cmd & CMD_BUTTON1) {
		if (selection == strlen(letters)) {
			PlaySound(SND_LAUNCH, 0, 255);
			return 0;
		}

		if (strlen(buffer) < PASSWORD_MAX) {
			int l = strlen(buffer);
			buffer[l + 1] = 0;
			buffer[l] = letters[selection];
			PlaySound(SND_MACHINEGUN, 0, 255);
		} else
			PlaySound(SND_KILL, 0, 255);
	} else if (cmd & CMD_BUTTON2) {
		if (buffer[0]) {
			buffer[strlen(buffer) - 1] = 0;
			PlaySound(SND_BANG, 0, 255);
		} else
			PlaySound(SND_KILL, 0, 255);
	} else if (cmd & CMD_LEFT) {
		if (selection > 0) {
			selection--;
			PlaySound(SND_DOOR, 0, 255);
		}
	} else if (cmd & CMD_RIGHT) {
		if (selection < strlen(letters)) {
			selection++;
			PlaySound(SND_DOOR, 0, 255);
		}
	} else if (cmd & CMD_UP) {
		if (selection > 9) {
			selection -= 10;
			PlaySound(SND_DOOR, 0, 255);
		}
	} else if (cmd & CMD_DOWN) {
		if (selection < strlen(letters) - 9) {
			selection += 10;
			PlaySound(SND_DOOR, 0, 255);
		}
	}
	// Draw selection
	for (i = 0; i < strlen(letters); i++) {
		TextGoto(100 + (i % 10) * 12,
			 80 + (i / 10) * TextHeight());
		if (i == selection)
			TextCharWithTable(letters[i], &tableFlamed);
		else
			TextChar(letters[i]);
	}
	TextGoto(100 + (i % 10) * 12, 80 + (i / 10) * TextHeight());
	if (i == selection)
		TextStringWithTable(DONE, &tableFlamed);
	else
		TextString(DONE);

	return 1;
}

static int EnterCode(void *bkg, const char *password)
{
	int cmd, prev = 0;
	int mission = 0;
	int done = 0;
	char buffer[PASSWORD_MAX + 1];

	strcpy(buffer, password);
	while (!done) {
		memcpy(GetDstScreen(), bkg, SCREEN_MEMSIZE);
		GetMenuCmd(&cmd);
		if (cmd != prev)
			prev = cmd;
		else
			cmd = 0;
		if (!PasswordEntry(cmd, buffer)) {
			if (!buffer[0]) {
				mission = 0;
				done = 1;
			} else {
				mission = TestPassword(buffer);
				if (mission > 0)
					done = 1;
				else
					PlaySound(SND_KILL2, 0, 255);
			}
		}

		TextGoto(125, 50);
		TextChar('\020');
		TextString(buffer);
		TextChar('\021');

		TextStringAt(50, 5, "Enter code");
		ShowControls();

		CopyToScreen();
		DoSounds();
	}

	PlaySound(SND_SWITCH, 0, 255);
	WaitForRelease();

	return mission;
}

int EnterPassword(void *bkg, const char *password)
{
	int cmd, prev = 0;
	int mission;
	int index = 0;

	if (TestPassword(password) > 0)
		index = 1;

	while (1) {
		memcpy(GetDstScreen(), bkg, SCREEN_MEMSIZE);
		GetMenuCmd(&cmd);
		if (cmd != prev)
			prev = cmd;
		else
			cmd = 0;

		if (AnyButton(cmd)) {
			if (index == 1) {
				WaitForRelease();
				mission = EnterCode(bkg, password);
				if (mission)
					return mission;
			} else
				break;
		} else if (index > 0 && (Left(cmd) || Up(cmd))) {
			index--;
			PlaySound(SND_SWITCH, 0, 255);
		} else if (index == 0 && (Right(cmd) || Down(cmd))) {
			index++;
			PlaySound(SND_SWITCH, 0, 255);
		}

		DisplayMenuItem(125, 50, "Start campaign", index == 0);
		DisplayMenuItem(125, 50 + TextHeight(), "Enter code...",
				index == 1);
		ShowControls();

		CopyToScreen();
		DoSounds();
	}
	WaitForRelease();
	return 0;
}
