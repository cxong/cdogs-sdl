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
#ifndef __INPUT
#define __INPUT

typedef enum
{
	INPUT_DEVICE_KEYBOARD,
	INPUT_DEVICE_JOYSTICK_1,
	INPUT_DEVICE_JOYSTICK_2
} input_device_e;

const char *InputDeviceStr(input_device_e d);
void GetPlayerCmd(int *cmd1, int *cmd2);
void GetMenuCmd(int *cmd, int *prevCmd);
void WaitForRelease(void);
void WaitForPress(void);
void Wait(void);

#endif
