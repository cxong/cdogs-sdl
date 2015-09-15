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
#pragma once

#include "game_loop.h"
#include "joystick.h"
#include "keyboard.h"
#include "mouse.h"
#include "player.h"

typedef struct
{
	keyboard_t keyboard;
	joysticks_t joysticks;
	Mouse mouse;

	bool HasResolutionChanged;
	bool HasQuit;
} EventHandlers;

extern EventHandlers gEventHandlers;

void EventInit(
	EventHandlers *handlers, Pic *mouseCursor, Pic *mouseTrail,
	const bool hideMouse);
void EventTerminate(EventHandlers *handlers);
void EventReset(EventHandlers *handlers, Pic *mouseCursor, Pic *mouseTrail);

void EventPoll(EventHandlers *handlers, Uint32 ticks);

int GetOnePlayerCmd(
	EventHandlers *handlers, const bool isPressed,
	const input_device_e device, const int deviceIndex);
int GetGameCmd(
	EventHandlers *handlers,
	const PlayerData *playerData, const Vec2i playerPos);
int GetKey(EventHandlers *handlers);
typedef struct
{
	bool IsOK;
} GameLoopWaitForAnyKeyOrButtonData;
GameLoopResult GameLoopWaitForAnyKeyOrButtonFunc(void *data);
void GetPlayerCmds(EventHandlers *handlers, int (*cmds)[MAX_LOCAL_PLAYERS]);
int GetMenuCmd(EventHandlers *handlers);
const char *InputGetButtonName(
	const input_device_e d, const int dIndex, const int cmd);
const char *InputGetButtonNameColor(
	const input_device_e d, const int dIndex, const int cmd, color_t *color);
// Return a string that shows the direction controls for an input device
void InputGetDirectionNames(
	char *buf, const input_device_e d, const int dIndex);

bool EventIsEscape(
	EventHandlers *handlers,
	const int cmds[MAX_LOCAL_PLAYERS], const int menuCmd);
