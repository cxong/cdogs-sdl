/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.

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
#ifndef __MOUSE
#define __MOUSE

#include <SDL_stdinc.h>

#include <cdogs/pic_file.h>
#include <cdogs/vector.h>

typedef struct
{
	int left, top, right, bottom;
	int tag;
} MouseRect;

typedef struct
{
	Uint32 previousButtons;
	Uint32 currentButtons;
	Vec2i previousPos;
	Vec2i currentPos;
	Pic *cursor;
	// C-Dogs editor uses rectangles to detect mouse presses on key areas
	// TODO: redesign this, is there a light-weight mouse-GUI framework?
	MouseRect *rects;
	MouseRect *rects2;
} Mouse;

extern Mouse gMouse;

void MouseInit(Mouse *mouse, Pic *cursor);
void MousePoll(Mouse *mouse);
int MouseHasMoved(Mouse *mouse);
void MouseSetRects(Mouse *mouse, MouseRect *rects, MouseRect *rects2);
void MouseSetSecondaryRects(Mouse *mouse, MouseRect *rects);
int MouseTryGetRectTag(Mouse *mouse, int *tag);
void MouseDraw(Mouse *mouse);

#endif
