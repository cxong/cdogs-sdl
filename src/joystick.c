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

 joystick.c - um... you know what it is 
 
 Author: $Author$
 Rev:    $Revision$
 URL:    $HeadURL$
 ID:     $Id$
 
*/

#include <string.h>
#include <stdio.h>
// #include <conio.h>
// #include <i86.h>
#include "joystick.h"

/* some of this stuff could be better, but it works! */

struct JoyRec gSticks[2];

void PollSticks(int maxWait)
{
	SDL_Joystick *j;
	int idx;

	SDL_JoystickUpdate();

	gSticks[0].x = gSticks[0].y = gSticks[1].x = gSticks[1].y = 0;
	gSticks[0].buttons = gSticks[1].buttons = 0;

	for (idx = 0; idx < 2; idx++) {
		if (gSticks[idx].present && gSticks[idx].inUse) {
			int i;
			int max;
			int btn = 0;
			
			j = gSticks[idx].j;
			gSticks[idx].x = SDL_JoystickGetAxis(j, 0);
			gSticks[idx].y = SDL_JoystickGetAxis(j, 1);

			max = gSticks[idx].nr_buttons;
			if (max > 4) max = 4;

			for (i = 0; i < max; i++) {
				if (SDL_JoystickGetButton(j, i)) {
					btn |= (2^i);
				}
			}

			gSticks[idx].buttons = btn;
		}
	}
}

void InitSticks(void)
{
	int i;
	int n;
	
	gSticks[0].present = NO;
	gSticks[1].present = NO;
	gSticks[0].inUse = NO;
	gSticks[1].inUse = NO;

	printf("Checking for joysticks... ");

	if ((n = SDL_NumJoysticks()) == 0) {
		printf("None found.\n");
		return;
	}

	printf("%d found\n", n);

	for (i = 0; i < n; i++) {
		SDL_Joystick *j;

		if (i > 1) break; /* Only 2 joysticks supported ATM */

		if ((j = gSticks[i].j)) {
			printf("Closing joystick.\n");
			SDL_JoystickClose(j);
			j = NULL;
		}
		
		j = SDL_JoystickOpen(i);

		if (j) {
			int nb, na;
			nb = SDL_JoystickNumButtons(j);
			na = SDL_JoystickNumAxes(j);
			
			printf("Opened Joystick %d\n", i);
			printf(" -> %s\n", SDL_JoystickName(i));
			printf(" -> Axes: %d Buttons: %d\n", na, nb);
			gSticks[i].present = YES;
			gSticks[i].inUse = YES;
			gSticks[i].nr_buttons = nb;
			gSticks[i].nr_axes = na;
			gSticks[i].buttons = 0;
			gSticks[i].j = j;
		} else {
			printf("Failed to open joystick.\n");
		}
	}
	AutoCalibrate();
}

void AutoCalibrate(void)
{
	PollSticks(0);

/*
	gSticks[0].xMid = gSticks[0].x;
	gSticks[0].yMid = gSticks[0].y;
	gSticks[1].xMid = gSticks[1].x;
	gSticks[1].yMid = gSticks[1].y;
*/
	gSticks[0].xMid = 0;
	gSticks[0].yMid = 0;
	gSticks[1].xMid = 0;
	gSticks[1].yMid = 0;
}

#define JS_DEF_THRESHOLD	16384

int js1_threshold = JS_DEF_THRESHOLD;
int js2_threshold = JS_DEF_THRESHOLD;

void PollDigiSticks(int *joy1, int *joy2)
{
	PollSticks(0);

	if (joy1)
		*joy1 = 0;
	if (joy1 && gSticks[0].present) {
		if (gSticks[0].x < (gSticks[0].xMid - js1_threshold)) {
			*joy1 |= JOYSTICK_LEFT;
		} else if (gSticks[0].x > (gSticks[0].xMid + js1_threshold)) {
			*joy1 |= JOYSTICK_RIGHT;
		}
		
		if (gSticks[0].y < (gSticks[0].yMid - js1_threshold)) {
			*joy1 |= JOYSTICK_UP;
		} else if (gSticks[0].y > (gSticks[0].yMid + js1_threshold)) {
			*joy1 |= JOYSTICK_DOWN;
		}

		if ((gSticks[0].buttons & 1) != 0)
			*joy1 |= JOYSTICK_BUTTON1;
		if ((gSticks[0].buttons & 2) != 0)
			*joy1 |= JOYSTICK_BUTTON2;
		if ((gSticks[0].buttons & 4) != 0)
			*joy1 |= JOYSTICK_BUTTON3;
		if ((gSticks[0].buttons & 8) != 0)
			*joy1 |= JOYSTICK_BUTTON4;
	}
	if (joy2)
		*joy2 = 0;
	if (joy2 && gSticks[1].present) {
		if (gSticks[1].x < gSticks[1].xMid - js2_threshold) {
			*joy2 |= JOYSTICK_LEFT;
		} else if (gSticks[1].x > gSticks[1].xMid + js2_threshold) {
			*joy2 |= JOYSTICK_RIGHT;
		}
		
		if (gSticks[1].y < gSticks[1].yMid - js2_threshold) {
			*joy2 |= JOYSTICK_UP;
		} else if (gSticks[1].y > gSticks[1].yMid + js2_threshold) {
			*joy2 |= JOYSTICK_DOWN;
		}

		if ((gSticks[1].buttons & 1) != 0)
			*joy2 |= JOYSTICK_BUTTON1;
		if ((gSticks[1].buttons & 2) != 0)
			*joy2 |= JOYSTICK_BUTTON2;
		if ((gSticks[1].buttons & 4) != 0)
			*joy2 |= JOYSTICK_BUTTON3;
		if ((gSticks[1].buttons & 8) != 0)
			*joy2 |= JOYSTICK_BUTTON4;
	}
}

void EnableSticks(int joy1, int joy2)
{
	gSticks[0].inUse = joy1;
	gSticks[1].inUse = joy2;
}
