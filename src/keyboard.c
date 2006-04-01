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

 keyboard.c - keyboard stuff... just think what was once here... (DOS interrupt
 handlers... *shudder*

*/

#include <string.h>
#include "SDL.h"
// #include <conio.h>
// #include <dos.h>
#include "keyboard.h"

void _dos_setvect(int intr_num, void *isr)
{
	return;
}

void *_dos_getvect(int x)
{
	return NULL;
}

char *keyNames[256] =
    { /* $00 */ NULL, "Esc", "1", "2", "3", "4", "5", "6",
	/* $08 */ "7", "8", "9", "0", "+", "Apostrophe", "Backspace",
	"Tab",
	/* $10 */ "Q", "W", "E", "R", "T", "Y", "U", "I",
	/* $18 */ "O", "P", "è", "^", "Enter", "Left Ctrl", "A", "S",
	/* $20 */ "D", "F", "G", "H", "J", "K", "L", "ô",
	/* $28 */ "é", "Paragraph", "Left shift", "'", "Z", "X", "C", "V",
	/* $30 */ "B", "N", "M", ",", ".", "-", "Right shift", "* (pad)",
	/* $38 */ "Alt", "Space", "Caps Lock", "F1", "F2", "F3", "F4",
	"F5",
	/* $40 */ "F6", "F7", "F8", "F9", "F10", "Num Lock", "Scroll Lock",
	"7 (pad)",
	/* $48 */ "8 (pad)", "9 (pad)", "- (pad)", "4 (pad)", "5 (pad)",
	"6 (pad)", "+ (pad)", "1 (pad)",
	/* $50 */ "2 (pad)", "3 (pad)", "0 (pad)", ", (pad)", "SysRq",
	NULL, "<", "F11", "F12",
	/* $59 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	/* $60 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	/* $70 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	/* $80 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	/* $90 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, "Enter (pad)", "Right Ctrl", NULL, NULL,
	/* $A0 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	/* $B0 */ NULL, NULL, NULL, NULL, NULL, "/ (pad)", NULL, "PrtScr",
	"Alt Gr", NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	/* $C0 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, "Home",
	/* $C8 */ "Up arrow", "Page Up", NULL, "Left arrow", NULL,
	"Right arrow", NULL, "End",
	/* $D0 */ "Down arrow", "Page Down", "Insert", "Delete", NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL,
	/* $E0 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	/* $F0 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL
};


static volatile unsigned char uKeys[256];

void InitKeyboard(void)
{

}

char KeyDown(int key)
{
//      printf("%i\n", key);
	SDL_PumpEvents();
	int *num = NULL;
	char *tmp = SDL_GetKeyState(num);
/*	if (tmp[key])
		printf("1\n");
	else
		printf("0 %i\n", key);
*/ return tmp[key];
}

int GetKeyDown(void)
{
	int i;
	SDL_PumpEvents();
	int num;
	char *tmp = SDL_GetKeyState(&num);
//      printf("got array: %i\n", num);
	for (i = 0; i < num; i++) {
//              printf("%i\n", i);
		if (tmp[i])
			return i;
	}
	return 0;
}

int AnyKeyDown(void)
{
	return (GetKeyDown() > 0);
}

void ClearKeys(void)
{
//      memset(&uKeys, 0, sizeof(uKeys));
}
