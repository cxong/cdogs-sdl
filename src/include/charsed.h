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

 charsed.h - <description here>

*/

#define MAX_MISSIONS    50
#define MAX_CHARACTERS  50

#define PAGEUP      0x4900
#define PAGEDOWN    0x5100
#define HOME        0x4700
#define END         0x4f00
#define INSERT      0x5200
#define DELETE      0x5300
#define ALT_X       0x2d00
#define ALT_C       0x2e00
#define ALT_V       0x2f00
#define ALT_Q       0x1000
#define ALT_E       0x1200
#define ARROW_UP    0x4800
#define ARROW_DOWN  0x5000
#define ARROW_LEFT  0x4b00
#define ARROW_RIGHT 0x4d00
#define BACKSPACE   8
#define ESCAPE      27
#define F1          0x3b00
#define F2          0x3c00
#define F3          0x3d00
#define F4          0x3e00
#define F10         0x4400
#define F11         0x8500
#define F12         0x8600
#define sF1         0x5400
#define sF2         0x5500
#define sF3         0x5600
#define sF4         0x5700
#define ALT_N       0x3100
#define ALT_S       0x1f00
#define ALT_M       0x3200
#define ALT_H       0x2300
#define ENTER       13
#define DUMMY       0x7FFF


extern int fileChanged;


void DisplayFlag(int x, int y, const char *s, int on, int hilite);
void GetEvent(int *key, int *x, int *y, int *buttons);
void AdjustInt(int *i, int min, int max, int wrap);
void EditCharacters(TCampaignSetting * setting);
