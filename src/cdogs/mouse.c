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

    Copyright (c) 2013, Cong Xu
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
#include "mouse.h"

#include <SDL_events.h>
#include <SDL_mouse.h>

#include "config.h"

#define MOUSE_REPEAT_TICKS 150

void MouseInit(Mouse *mouse, Pic *cursor)
{
	memset(mouse, 0, sizeof *mouse);
	mouse->cursor = cursor;
	mouse->ticks = 0;
	mouse->repeatedTicks = 0;
	SDL_ShowCursor(SDL_DISABLE);
}

void MousePrePoll(Mouse *mouse)
{
	memcpy(
		mouse->previousButtons,
		mouse->currentButtons,
		sizeof mouse->previousButtons);
	mouse->previousPos = mouse->currentPos;
	SDL_GetMouseState(&mouse->currentPos.x, &mouse->currentPos.y);
	mouse->currentPos =
		Vec2iScaleDiv(mouse->currentPos, gConfig.Graphics.ScaleFactor);
}


void MouseOnButtonDown(Mouse *mouse, Uint8 button)
{
	mouse->currentButtons[button] = 1;
}
void MouseOnButtonUp(Mouse *mouse, Uint8 button)
{
	mouse->currentButtons[button] = 0;
}

void MousePostPoll(Mouse *mouse, Uint32 ticks)
{
	int areSameButtonsPressed = 1;
	int i;
	for (i = 0; i < 8; i++)
	{
		if (mouse->previousButtons[i] ^ mouse->currentButtons[i])
		{
			areSameButtonsPressed = 0;
			break;
		}
	}
	// If same buttons have been pressed, remember how long they have been pressed
	if (areSameButtonsPressed)
	{
		Uint32 ticksElapsed = ticks - mouse->ticks;
		mouse->repeatedTicks += ticksElapsed;
	}
	else
	{
		mouse->repeatedTicks = 0;
	}
	// If more time has elapsed, forget about previous buttons for repeating
	if (mouse->repeatedTicks > MOUSE_REPEAT_TICKS)
	{
		mouse->repeatedTicks -= MOUSE_REPEAT_TICKS;
		memcpy(
			mouse->pressedButtons,
			mouse->currentButtons,
			sizeof mouse->pressedButtons);
	}
	else
	{
		for (i = 0; i < 8; i++)
		{
			mouse->pressedButtons[i] =
				mouse->currentButtons[i] && !mouse->previousButtons[i];
		}
	}
	mouse->ticks = ticks;
}

int MouseHasMoved(Mouse *mouse)
{
	return !Vec2iEqual(mouse->previousPos, mouse->currentPos);
}

int MouseGetPressed(Mouse *mouse)
{
	int i;
	for (i = 0; i < 8; i++)
	{
		if (mouse->pressedButtons[i])
		{
			return i;
		}
	}
	return 0;
}

void MouseSetRects(Mouse *mouse, MouseRect *rects, MouseRect *rects2)
{
	mouse->rects = rects;
	mouse->rects2 = rects2;
}

void MouseSetSecondaryRects(Mouse *mouse, MouseRect *rects)
{
	mouse->rects2 = rects;
}

static int TryGetRectTag(MouseRect *rects, Vec2i pos, int *tag)
{
	while (rects && rects->right > 0)
	{
		if (pos.y >= rects->top && pos.y <= rects->bottom &&
			pos.x >= rects->left && pos.x <= rects->right)
		{
			*tag = rects->tag;
			return 1;
		}
		rects++;
	}
	return 0;
}

int MouseTryGetRectTag(Mouse *mouse, int *tag)
{
	if (TryGetRectTag(mouse->rects, mouse->currentPos, tag))
	{
		return 1;
	}
	if (TryGetRectTag(mouse->rects2, mouse->currentPos, tag))
	{
		return 1;
	}
	return 0;
}

MouseRect *MouseGetRectByTag(Mouse *mouse, int tag)
{
	MouseRect *rects = mouse->rects;
	while (rects && rects->right > 0)
	{
		if (rects->tag == tag)
		{
			return rects;
		}
		rects++;
	}
	rects = mouse->rects2;
	while (rects && rects->right > 0)
	{
		if (rects->tag == tag)
		{
			return rects;
		}
		rects++;
	}
	return NULL;
}

void MouseDraw(Mouse *mouse)
{
	DrawTPic(mouse->currentPos.x, mouse->currentPos.y, mouse->cursor);
}
