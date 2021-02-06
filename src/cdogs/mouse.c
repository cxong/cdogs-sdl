/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.

	Copyright (c) 2013-2015, 2019-2021 Cong Xu
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
#include "log.h"

#define MOUSE_REPEAT_TICKS 150
#define MOUSE_MOVE_DEAD_ZONE 12
#define TRAIL_NUM_DOTS 4

void MouseInit(Mouse *mouse)
{
	memset(mouse, 0, sizeof *mouse);
	for (SDL_SystemCursor i = SDL_SYSTEM_CURSOR_ARROW;
		 i < SDL_NUM_SYSTEM_CURSORS; i++)
	{
		mouse->cursors[i] = SDL_CreateSystemCursor(i);
	}
}
void MouseTerminate(Mouse *m)
{
	SDL_FreeCursor(m->cursor);
	for (SDL_SystemCursor i = SDL_SYSTEM_CURSOR_ARROW;
		 i < SDL_NUM_SYSTEM_CURSORS; i++)
	{
		SDL_FreeCursor(m->cursors[i]);
	}
}
void MouseReset(Mouse *m)
{
	memset(m->previousButtons, 0, sizeof m->previousButtons);
	memset(m->currentButtons, 0, sizeof m->currentButtons);
	memset(m->pressedButtons, 0, sizeof m->pressedButtons);
	m->previousPos = svec2i_zero();
	m->currentPos = svec2i_zero();
	m->wheel = svec2i_zero();
	m->repeatedTicks = 0;
	m->mouseMovePos = svec2i_zero();
}

void MousePrePoll(Mouse *mouse)
{
	memset(mouse->pressedButtons, 0, sizeof mouse->pressedButtons);
	memcpy(
		mouse->previousButtons, mouse->currentButtons,
		sizeof mouse->previousButtons);
	mouse->wheel = svec2i_zero();
	mouse->previousPos = mouse->currentPos;
	SDL_GetMouseState(&mouse->currentPos.x, &mouse->currentPos.y);
	int scale = ConfigGetInt(&gConfig, "Graphics.ScaleFactor");
	if (scale == 0)
		scale = 1;
	mouse->currentPos = svec2i_scale_divide(mouse->currentPos, scale);
}

void MouseOnButtonDown(Mouse *mouse, Uint8 button)
{
	mouse->currentButtons[button] = true;
	// Capture mouse movement outside window for dragging
	SDL_CaptureMouse(SDL_TRUE);
}
void MouseOnButtonUp(Mouse *mouse, Uint8 button)
{
	// Certain mouse buttons can be pressed very quickly (e.g. mousewheel)
	// Detect these mouse presses before PostPoll
	if (mouse->currentButtons[button] && !mouse->previousButtons[button])
	{
		mouse->pressedButtons[button] = true;
	}
	mouse->currentButtons[button] = false;
	SDL_CaptureMouse(SDL_FALSE);
}
void MouseOnWheel(Mouse *m, const Sint32 x, const Sint32 y)
{
	m->wheel = svec2i(x, y);
}

void MousePostPoll(Mouse *mouse, const Uint32 ticks)
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
	// If same buttons have been pressed, remember how long they have been
	// pressed
	if (areSameButtonsPressed)
	{
		mouse->repeatedTicks += ticks;
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
			mouse->pressedButtons, mouse->currentButtons,
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

bool MouseIsDown(const Mouse *m, const int button)
{
	return m->currentButtons[button];
}

bool MouseIsPressed(const Mouse *m, const int button)
{
	return m->pressedButtons[button];
}

bool MouseIsReleased(const Mouse *m, const int button)
{
	return !m->currentButtons[button] && m->previousButtons[button];
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
			if (pos.x < mouse->currentPos.x)
				cmd |= CMD_RIGHT;
			else if (pos.x > mouse->currentPos.x)
				cmd |= CMD_LEFT;
		}
		if (2 * dy > dx)
		{
			if (pos.y < mouse->currentPos.y)
				cmd |= CMD_DOWN;
			else if (pos.y > mouse->currentPos.y)
				cmd |= CMD_UP;
		}
	}
	return cmd;
}

void MouseSetCursor(Mouse *m, const SDL_SystemCursor sc)
{
	SDL_FreeCursor(m->cursor);
	m->cursor = NULL;
	SDL_SetCursor(m->cursors[sc]);
	SDL_ShowCursor(SDL_ENABLE);
}
void MouseSetPicCursor(Mouse *m, const Pic *cursor, const Pic *trail)
{
	if (cursor)
	{
		SDL_FreeCursor(m->cursor);
		SDL_Surface *surf = SDL_CreateRGBSurfaceFrom(
			cursor->Data, cursor->size.x, cursor->size.y, 32,
			4 * cursor->size.x, gGraphicsDevice.Format->Rmask,
			gGraphicsDevice.Format->Gmask, gGraphicsDevice.Format->Bmask,
			0xff000000);
		m->cursor =
			SDL_CreateColorCursor(surf, -cursor->offset.x, -cursor->offset.y);
		SDL_FreeSurface(surf);
		if (m->cursor == NULL)
		{
			LOG(LM_MAIN, LL_WARN, "cannot load mouse cursor: %s",
				SDL_GetError());
		}
		else
		{
			SDL_SetCursor(m->cursor);
		}
	}
	else
	{
		if (m->cursor != NULL)
		{
			SDL_FreeCursor(m->cursor);
		}
		m->cursor = NULL;
	}
	m->trail = trail;
	SDL_ShowCursor(cursor != NULL ? SDL_ENABLE : SDL_DISABLE);
}

void MouseDraw(const Mouse *mouse)
{
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
					svec2i_scale_divide(
						svec2i_scale(d, (float)i), TRAIL_NUM_DOTS + 1));
				PicRender(
					mouse->trail, gGraphicsDevice.gameWindow.renderer, pos,
					colorWhite, 0, svec2_one(), SDL_FLIP_NONE, Rect2iZero());
			}
		}
	}
}
