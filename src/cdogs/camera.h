/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (c) 2015-2016, Cong Xu

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

#include "draw/draw_buffer.h"
#include "hud/hud.h"
#include "screen_shake.h"

#define CAMERA_SPLIT_PADDING 40

typedef enum
{
	SPECTATE_NONE,
	SPECTATE_FOLLOW,
	SPECTATE_FREE
} SpectateMode;

typedef struct
{
	DrawBuffer Buffer;
	Vec2i lastPosition;
	HUD HUD;
	ScreenShake shake;
	SpectateMode spectateMode;
	// UID of player to follow; only used if camera is in follow mode
	int FollowPlayerUID;
	// Immediately enter follow mode on the next player that joins the game
	// This is used for when the game has no players; all spectators should
	// immediately follow the next player to join
	bool FollowNextPlayer;
} Camera;

void CameraInit(Camera *camera);
void CameraTerminate(Camera *camera);

void CameraInput(Camera *camera, const int cmd, const int lastCmd);
void CameraUpdate(Camera *camera, const int ticks, const int ms);
void CameraDraw(
	Camera *camera, const input_device_e pausingDevice,
	const bool controllerUnplugged);

bool CameraIsSingleScreen(void);
