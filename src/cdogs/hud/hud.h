/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.

    Copyright (c) 2013-2016, Cong Xu
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

#include "config.h"
#include "fps.h"
#include "gamedata.h"
#include "hud_num_popup.h"
#include "player.h"
#include "wall_clock.h"


typedef struct
{
	struct MissionOptions *mission;
	char message[256];
	int messageTicks;
	GraphicsDevice *device;
	FPSCounter fpsCounter;
	WallClock clock;
	HUDNumPopups numPopups;
	bool showExit;
} HUD;

void HUDInit(
	HUD *hud,
	GraphicsDevice *device,
	struct MissionOptions *mission);
void HUDTerminate(HUD *hud);

// Set ticks to -1 to display a message indefinitely
void HUDDisplayMessage(HUD *hud, const char *msg, int ticks);

void HUDUpdate(HUD *hud, int ms);
// INPUT_DEVICE_UNSET if not paused
void HUDDraw(
	HUD *hud, const input_device_e pausingDevice,
	const bool controllerUnplugged);
