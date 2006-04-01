/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (C) 1995 Ronny Webster
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

 joystick.h - <description here>

*/

#ifndef YES
#define YES            -1
#define NO             0
#endif


#define JOYSTICK_LEFT       1
#define JOYSTICK_RIGHT      2
#define JOYSTICK_UP         4
#define JOYSTICK_DOWN       8
#define JOYSTICK_BUTTON1   16
#define JOYSTICK_BUTTON2   32
#define JOYSTICK_BUTTON3   64
#define JOYSTICK_BUTTON4  128
#define JOYSTICK_PRESENT  256


struct JoyRec {
	int present;
	int inUse;
	int xMid, yMid;
	int x, y;
	int buttons;
};


extern struct JoyRec gSticks[2];


void PollSticks(int maxWait);
void InitSticks(void);
void AutoCalibrate(void);
void PollDigiSticks(int *joy1, int *joy2);
void EnableSticks(int joy1, int joy2);
