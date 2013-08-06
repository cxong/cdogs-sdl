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

#include <cdogs/config.h>

Mouse gMouse;

void MouseInit(Mouse *mouse, Pic *cursor)
{
	memset(mouse, 0, sizeof *mouse);
	mouse->cursor = cursor;
	SDL_ShowCursor(SDL_DISABLE);
}

void MousePoll(Mouse *mouse)
{
	mouse->previousButtons = mouse->currentButtons;
	mouse->previousPos = mouse->currentPos;
	SDL_GetMouseState(&mouse->currentPos.x, &mouse->currentPos.y);
	mouse->currentPos =
		Vec2iScaleDiv(mouse->currentPos, gConfig.Graphics.ScaleFactor);
}

int MouseHasMoved(Mouse *mouse)
{
	return !Vec2iEqual(mouse->previousPos, mouse->currentPos);
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

void MouseDraw(Mouse *mouse)
{
	DrawTPic(mouse->currentPos.x, mouse->currentPos.y, mouse->cursor);
}
