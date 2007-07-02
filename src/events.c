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

 events.c - event related functions 
 
 Author: $Author$
 Rev:    $Revision$
 URL:    $HeadURL$
 ID:     $Id$
 
*/

#include <stdlib.h>
#include "events.h"
#include "keyboard.h"
#include "SDL.h"


static struct MouseRect *localRects = NULL;
static struct MouseRect *localRects2 = NULL;


void InitMouse(void)
{
	SDL_ShowCursor(SDL_DISABLE);
}

void Mouse(int *x, int *y, int *button)
{	
	SDL_Event event;

	/* I'm not sure whether this works */
	if (SDL_PollEvent(&event)) {
		switch (event.type) {
			case SDL_MOUSEMOTION:
				*x = event.motion.x;
				*y = event.motion.y;
				*button = 0;
				break;
			case SDL_MOUSEBUTTONDOWN:
				*x = event.button.x;
				*y = event.button.y;
				*button = 1;
				break;
		}
	}	
}

int GetKey(void)
{

	while (!AnyKeyDown());

	return GetKeyDown();
}

void SetMouseRects(struct MouseRect *rects)
{
	localRects = rects;
	localRects2 = NULL;
}

void SetSecondaryMouseRects(struct MouseRect *rects)
{
	localRects2 = rects;
}

int GetMouseRectTag(int x, int y, int *tag)
{
	struct MouseRect *mRect;
	int i;

	for (i = 0, mRect = localRects; mRect && mRect->right > 0; i++, mRect++) {
		if (y >= mRect->top && y <= mRect->bottom &&
			x >= mRect->left && x <= mRect->right) {
			*tag = mRect->tag;
			return 1;
		}
	}
	for (i = 0, mRect = localRects2; mRect && mRect->right > 0; i++, mRect++) {
		if (y >= mRect->top && y <= mRect->bottom &&
			x >= mRect->left && x <= mRect->right) {
			*tag = mRect->tag;
			return 1;
		}
	}
	return 0;
}


int IsEventPending (Uint32 m) {
	if (SDL_PeepEvents(NULL, 0, SDL_PEEKEVENT, m))
		return 1;
	else
		return 0;
}
