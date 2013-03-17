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
*/
#include "input.h"

#include <stdlib.h>

#include "joystick.h"
#include "keyboard.h"
#include "defs.h"
#include "sounds.h"
#include "gamedata.h"


static int SwapButtons(int cmd)
{
	int c = (cmd & ~(CMD_BUTTON1 | CMD_BUTTON2));
	if (cmd & CMD_BUTTON1)
		c |= CMD_BUTTON2;
	if (cmd & CMD_BUTTON2)
		c |= CMD_BUTTON1;
	return c;
}

void GetOnePlayerCmd(
	struct PlayerData *data, int *cmd, int joy1, int joy2, int isSingle)
{
	if (cmd != NULL)
	{
		if (data->inputDevice == INPUT_DEVICE_KEYBOARD)
		{
			*cmd = 0;
			if (KeyDown(data->keys[0]))
			{
				*cmd |= CMD_LEFT;
			}
			else if (KeyDown(data->keys[1]))
			{
				*cmd |= CMD_RIGHT;
			}
			if (KeyDown(data->keys[2]))
			{
				*cmd |= CMD_UP;
			}
			else if (KeyDown(data->keys[3]))
			{
				*cmd |= CMD_DOWN;
			}
			if (KeyDown(data->keys[4]))
			{
				*cmd |= CMD_BUTTON1;
			}
			if (KeyDown(data->keys[5]))
			{
				*cmd |= CMD_BUTTON2;
			}
		}
		else
		{
			int cmdOther = 0;
			int swapButtons = 0;
			if (data->inputDevice == INPUT_DEVICE_JOYSTICK_1)
			{
				*cmd = joy1;
				cmdOther = joy2;
				swapButtons = gOptions.swapButtonsJoy1;
			}
			else if (data->inputDevice == INPUT_DEVICE_JOYSTICK_2)
			{
				*cmd = joy2;
				cmdOther = joy1;
				swapButtons = gOptions.swapButtonsJoy2;
			}
			if (swapButtons)
			{
				*cmd = SwapButtons(*cmd);
			}
			if (isSingle)
			{
				if (cmdOther & CMD_BUTTON1)
				{
					*cmd |= CMD_BUTTON3;
				}
				if (cmdOther & CMD_BUTTON2)
				{
					*cmd |= CMD_BUTTON4;
				}
			}
		}
	}
}


char *InputDeviceStr(int d)
{
	switch (d)
	{
	case INPUT_DEVICE_KEYBOARD:
		return "Keyboard";
	case INPUT_DEVICE_JOYSTICK_1:
		return "Joystick 1";
	case INPUT_DEVICE_JOYSTICK_2:
		return "Joystick 2";
	default:
		return "";
	}
}

void GetPlayerCmd(int *cmd1, int *cmd2)
{
	int joy1, joy2;

	PollDigiSticks(&joy1, &joy2);
	GetOnePlayerCmd(&gPlayer1Data, cmd1, joy1, joy2, cmd2 == NULL);
	GetOnePlayerCmd(&gPlayer2Data, cmd2, joy1, joy2, cmd1 == NULL);
}

void GetMenuCmd(int *cmd, int *prevCmd)
{
	if (KeyDown(keyEsc))
	{
		*cmd = CMD_ESC;
		goto checkPrevCmd;
	}
	if (KeyDown(keyF10))
	{
		AutoCalibrate();
	}
	if (KeyDown(keyF9))
	{
		gPlayer1Data.inputDevice = INPUT_DEVICE_KEYBOARD;
		gPlayer2Data.inputDevice = INPUT_DEVICE_KEYBOARD;
	}

	GetPlayerCmd(cmd, NULL);
	if (*cmd)
	{
		goto checkPrevCmd;
	}

	if (KeyDown(keyArrowLeft))
	{
		*cmd |= CMD_LEFT;
	}
	else if (KeyDown(keyArrowRight))
	{
		*cmd |= CMD_RIGHT;
	}
	if (KeyDown(keyArrowUp))
	{
		*cmd |= CMD_UP;
	}
	else if (KeyDown(keyArrowDown))
	{
		*cmd |= CMD_DOWN;
	}
	if (KeyDown(keyEnter))
	{
		*cmd |= CMD_BUTTON1;
	}
	if (KeyDown(keyBackspace))
	{
		*cmd |= CMD_BUTTON2;
	}

checkPrevCmd:
	if (*cmd == *prevCmd)
	{
		*cmd = 0;
	}
	else
	{
		*prevCmd = *cmd;
	}
}

void WaitForRelease(void)
{
	int cmd1, cmd2;
	int releaseCount = 0;

	do {
		GetPlayerCmd(&cmd1, &cmd2);
		if (((cmd1 | cmd2) & (CMD_BUTTON1 | CMD_BUTTON2)) != 0
		    || AnyKeyDown())
			releaseCount = 0;
		else
			releaseCount++;
	} while (releaseCount < 4);
}

void WaitForPress(void)
{
	int cmd1, cmd2;

	do {
		GetPlayerCmd(&cmd1, &cmd2);
	} while (((cmd1 | cmd2) & (CMD_BUTTON1 | CMD_BUTTON2)) == 0 && !AnyKeyDown());
}

void Wait(void)
{
	WaitForRelease();
	WaitForPress();
	WaitForRelease();
}
