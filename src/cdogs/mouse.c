/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.

    Copyright (c) 2013-2015, Cong Xu
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

#include "blit.h"
#include "config.h"
#include "defs.h"

#define MOUSE_REPEAT_TICKS 150
#define MOUSE_MOVE_DEAD_ZONE 12
#define TRAIL_NUM_DOTS 4


void MouseInit(Mouse *mouse, Pic *cursor, Pic *trail, const bool hideMouse)
{
	memset(mouse, 0, sizeof *mouse);
	mouse->cursor = cursor;
	mouse->trail = trail;
	mouse->ticks = 0;
	mouse->repeatedTicks = 0;
	mouse->hideMouse = cursor != NULL || hideMouse;
	if (hideMouse)
	{
		SDL_ShowCursor(SDL_DISABLE);
	}
}

void MousePrePoll(Mouse *mouse)
{
	memset(mouse->pressedButtons, 0, sizeof mouse->pressedButtons);
	memcpy(
		mouse->previousButtons,
		mouse->currentButtons,
		sizeof mouse->previousButtons);
	mouse->wheel = svec2i_zero();
	mouse->previousPos = mouse->currentPos;
	SDL_GetMouseState(&mouse->currentPos.x, &mouse->currentPos.y);
	int scale = ConfigGetInt(&gConfig, "Graphics.ScaleFactor");
	if (scale == 0) scale = 1;
	mouse->currentPos = svec2i_scale_divide(mouse->currentPos, scale);
}


void MouseOnButtonDown(Mouse *mouse, Uint8 button)
{
	mouse->currentButtons[button] = 1;
}
void MouseOnButtonUp(Mouse *mouse, Uint8 button)
{
	// Certain mouse buttons can be pressed very quickly (e.g. mousewheel)
	// Detect these mouse presses before PostPoll
	if (mouse->currentButtons[button] && !mouse->previousButtons[button])
	{
		mouse->pressedButtons[button] = 1;
	}
	mouse->currentButtons[button] = 0;
}
void MouseOnWheel(Mouse *m, const Sint32 x, const Sint32 y)
{
	m->wheel = svec2i(x, y);
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
		for (i = 0; i < 8; i++)
		{
			mouse->pressedButtons[i] |= mouse->currentButtons[i];
		}
		memcpy(
			mouse->pressedButtons,
			mouse->currentButtons,
			sizeof mouse->pressedButtons);
	}
	else
	{
		for (i = 0; i < 8; i++)
		{
			mouse->pressedButtons[i] |=
				mouse->currentButtons[i] && !mouse->previousButtons[i];
		}
	}
	mouse->ticks = ticks;
}

bool MouseHasMoved(const Mouse *m)
{
	return !svec2i_is_equal(m->previousPos, m->currentPos);
}

int MouseGetPressed(const Mouse *m)
{
	for (int i = 0; i < 8; i++)
	{
		if (m->pressedButtons[i])
		{
			return i;
		}
	}
	return 0;
}

bool MouseIsPressed(const Mouse *m, const int button)
{
	return m->pressedButtons[button];
}

bool MouseIsDown(const Mouse *m, const int button)
{
	return m->currentButtons[button];
}

struct vec2i MouseWheel(const Mouse *m)
{
	return m->wheel;
}

int MouseGetMove(Mouse *mouse, const struct vec2i pos)
{
	int cmd = 0;
	const int dx = abs(mouse->currentPos.x - pos.x);
	const int dy = abs(mouse->currentPos.y - pos.y);
	mouse->mouseMovePos = pos;
	// HACK: player is typically drawn slightly to the right
	mouse->mouseMovePos.x += 8;
	const bool isInDeadZone =
		dx <= MOUSE_MOVE_DEAD_ZONE && dy <= MOUSE_MOVE_DEAD_ZONE;
	if (!isInDeadZone)
	{
		if (2 * dx > dy)
		{
			if (pos.x < mouse->currentPos.x)		cmd |= CMD_RIGHT;
			else if (pos.x > mouse->currentPos.x)	cmd |= CMD_LEFT;
		}
		if (2 * dy > dx)
		{
			if (pos.y < mouse->currentPos.y)		cmd |= CMD_DOWN;
			else if (pos.y > mouse->currentPos.y)	cmd |= CMD_UP;
		}
	}
	return cmd;
}

void MouseDraw(const Mouse *mouse)
{
	if (mouse->cursor)
	{
		BlitMasked(
			&gGraphicsDevice, mouse->cursor, mouse->currentPos, colorWhite, 1);
	}
	if (mouse->trail)
	{
		const int dx = abs(mouse->currentPos.x - mouse->mouseMovePos.x);
		const int dy = abs(mouse->currentPos.y - mouse->mouseMovePos.y);
		const bool isInDeadZone =
			dx <= MOUSE_MOVE_DEAD_ZONE && dy <= MOUSE_MOVE_DEAD_ZONE;
		if (!isInDeadZone)
		{
			// Draw a trail between the mouse move pos and mouse pos
			// The trail is made up of a fixed number of dots
			const struct vec2i d =
				svec2i_subtract(mouse->currentPos, mouse->mouseMovePos);
			for (int i = 1; i <= TRAIL_NUM_DOTS; i++)
			{
				const struct vec2i pos = svec2i_add(
					mouse->mouseMovePos,
					svec2i_scale_divide(svec2i_scale(d, i), TRAIL_NUM_DOTS + 1));
				BlitMasked(
					&gGraphicsDevice, mouse->trail, pos, colorWhite, false);
			}
		}
	}
}
