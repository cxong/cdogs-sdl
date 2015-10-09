/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (c) 2013-2015, Cong Xu

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
#include "camera.h"

#include "actors.h"
#include "draw.h"
#include "drawtools.h"
#include "events.h"
#include "font.h"
#include "los.h"
#include "player.h"


#define PAN_SPEED 4

void CameraInit(Camera *camera)
{
	memset(camera, 0, sizeof *camera);
	DrawBufferInit(
		&camera->Buffer, Vec2iNew(X_TILES, Y_TILES), &gGraphicsDevice);
	camera->lastPosition = Vec2iZero();
	HUDInit(&camera->HUD, &gGraphicsDevice, &gMission);
	camera->shake = ScreenShakeZero();
}

void CameraTerminate(Camera *camera)
{
	DrawBufferTerminate(&camera->Buffer);
	HUDTerminate(&camera->HUD);
}

void CameraInput(Camera *camera, const int cmd, const int lastCmd)
{
	// Control the camera
	if (camera->spectateMode == SPECTATE_NONE)
	{
		return;
	}
	// Arrows: pan camera
	// CMD1/2: choose next player to follow
	if (CMD_HAS_DIRECTION(cmd))
	{
		camera->spectateMode = SPECTATE_FREE;
		const int pan = PAN_SPEED;
		if (cmd & CMD_LEFT)	camera->lastPosition.x -= pan;
		else if (cmd & CMD_RIGHT)	camera->lastPosition.x += pan;
		if (cmd & CMD_UP)		camera->lastPosition.y -= pan;
		else if (cmd & CMD_DOWN)	camera->lastPosition.y += pan;
	}
	else if (AnyButton(cmd) && !AnyButton(lastCmd))
	{
		camera->spectateMode = SPECTATE_FOLLOW;
		// Find index of player
		int playerIndex = -1;
		for (int i = 0; i < (int)gPlayerDatas.size; i++)
		{
			const PlayerData *p = CArrayGet(&gPlayerDatas, i);
			if (p->UID == camera->FollowPlayerUID)
			{
				playerIndex = i;
				break;
			}
		}
		CASSERT(playerIndex >= 0, "Cannot find player");
		// Get the next player by index that has an actor in the game
		const int d = (cmd & CMD_BUTTON1) ? 1 : -1;
		for (int i = playerIndex + d;; i += d)
		{
			i = CLAMP_OPPOSITE(i, 0, (int)gPlayerDatas.size - 1);
			// Check if clamping made us hit the termination condition
			if (i == playerIndex) break;
			const PlayerData *p = CArrayGet(&gPlayerDatas, i);
			if (p->ActorUID >= 0)
			{
				// Follow this player
				camera->FollowPlayerUID = p->UID;
			}
		}
	}
}

void CameraUpdate(Camera *camera, const int ticks, const int ms)
{
	HUDUpdate(&camera->HUD, ms);
	camera->shake = ScreenShakeUpdate(camera->shake, ticks);
}

static void FollowPlayer(Vec2i *pos, const int playerUID);
static void DoBuffer(
	DrawBuffer *b, Vec2i center, int w, Vec2i noise, Vec2i offset);
void CameraDraw(Camera *camera, const input_device_e pausingDevice)
{
	Vec2i centerOffset = Vec2iZero();
	const int numLocalPlayersAlive =
		GetNumPlayers(PLAYER_ALIVE_OR_DYING, false, true);
	const int numLocalPlayers = GetNumPlayers(PLAYER_ANY, false, true);
	const int w = gGraphicsDevice.cachedConfig.Res.x;
	const int h = gGraphicsDevice.cachedConfig.Res.y;

	for (int i = 0; i < GraphicsGetScreenSize(&gGraphicsDevice.cachedConfig); i++)
	{
		gGraphicsDevice.buf[i] = COLOR2PIXEL(colorBlack);
	}

	const Vec2i noise = ScreenShakeGetDelta(camera->shake);

	GraphicsResetBlitClip(&gGraphicsDevice);
	if (numLocalPlayersAlive == 0)
	{
		// Count the number of local players with lives left
		// If there are none, then try to spectate if there are remote players
		int firstRemotePlayerUID = -1;
		bool hasLocalPlayerLives = false;
		for (int i = 0; i < (int)gPlayerDatas.size; i++)
		{
			const PlayerData *p = CArrayGet(&gPlayerDatas, i);
			if (p->Lives > 0)
			{
				if (p->IsLocal)
				{
					hasLocalPlayerLives = true;
					break;
				}
				else
				{
					firstRemotePlayerUID = p->UID;
				}
			}
		}
		if (!hasLocalPlayerLives)
		{
			if (camera->spectateMode == SPECTATE_NONE)
			{
				// Enter spectator mode
				// If there are remote players, follow them
				if (firstRemotePlayerUID != -1)
				{
					camera->spectateMode = SPECTATE_FOLLOW;
					camera->FollowPlayerUID = firstRemotePlayerUID;
				}
				else
				{
					// Free-look mode
					camera->spectateMode = SPECTATE_FREE;
				}
			}
		}
		else
		{
			// Don't spectate
			camera->spectateMode = SPECTATE_NONE;
		}
		if (camera->spectateMode == SPECTATE_FOLLOW)
		{
			FollowPlayer(&camera->lastPosition, camera->FollowPlayerUID);
		}
		DoBuffer(
			&camera->Buffer,
			camera->lastPosition,
			X_TILES, noise, centerOffset);
		SoundSetEars(camera->lastPosition);
	}
	else
	{
		// Don't spectate
		camera->spectateMode = SPECTATE_NONE;
		const int numLocalHumanPlayersAlive =
			GetNumPlayers(PLAYER_ALIVE_OR_DYING, true, true);
		// Redo LOS if PVP, so that each split screen has its own LOS
		if (IsPVP(gCampaign.Entry.Mode) && numLocalHumanPlayersAlive > 0)
		{
			LOSReset(&gMap.LOS);
		}
		const bool onePlayer =
			numLocalHumanPlayersAlive == 1 || numLocalPlayersAlive == 1;
		const bool singleScreen = CameraIsSingleScreen();
		if (onePlayer || singleScreen)
		{
			// Single camera screen
			if (onePlayer)
			{
				const TActor *p = ActorGetByUID(
					(numLocalHumanPlayersAlive == 1 ?
					GetFirstPlayer(true, true, true) :
					GetFirstPlayer(true, false, true))->ActorUID);
				camera->lastPosition = Vec2iNew(p->tileItem.x, p->tileItem.y);
			}
			else if (singleScreen)
			{
				// One screen
				camera->lastPosition = PlayersGetMidpoint();
			}

			// Special case: if map is smaller than screen, center the camera
			// However, it is important to keep the ear positions unmodified
			// so that sounds don't get muffled just because there's a wall
			// between player and camera center
			const Vec2i earPos = camera->lastPosition;
			if (gMap.Size.x * TILE_WIDTH < gGraphicsDevice.cachedConfig.Res.x)
			{
				camera->lastPosition.x = gMap.Size.x * TILE_WIDTH / 2;
			}
			if (gMap.Size.y * TILE_HEIGHT < gGraphicsDevice.cachedConfig.Res.y)
			{
				camera->lastPosition.y = gMap.Size.y * TILE_HEIGHT / 2;
			}

			// Redo LOS for every local human player
			if (IsPVP(gCampaign.Entry.Mode))
			{
				for (int i = 0; i < (int)gPlayerDatas.size; i++)
				{
					const PlayerData *p = CArrayGet(&gPlayerDatas, i);
					if (!p->IsLocal || !IsPlayerAliveOrDying(p) ||
						!IsPlayerHuman(p))
					{
						continue;
					}
					const TActor *a = ActorGetByUID(p->ActorUID);
					LOSCalcFrom(
						&gMap,
						Vec2iToTile(Vec2iNew(a->tileItem.x, a->tileItem.y)),
						false);
				}
			}

			DoBuffer(
				&camera->Buffer,
				camera->lastPosition,
				X_TILES, noise, centerOffset);
			SoundSetEars(earPos);
		}
		else if (numLocalPlayers == 2)
		{
			CASSERT(
				numLocalPlayersAlive == 2,
				"Unexpected number of local players");
			// side-by-side split
			int idx = 0;
			for (int i = 0; i < (int)gPlayerDatas.size; i++, idx++)
			{
				const PlayerData *p = CArrayGet(&gPlayerDatas, i);
				if (!p->IsLocal)
				{
					idx--;
					continue;
				}
				const TActor *a = ActorGetByUID(p->ActorUID);
				camera->lastPosition = Vec2iNew(a->tileItem.x, a->tileItem.y);
				Vec2i centerOffsetPlayer = centerOffset;
				int clipLeft = (idx & 1) ? w / 2 : 0;
				int clipRight = (idx & 1) ? w - 1 : (w / 2) - 1;
				GraphicsSetBlitClip(
					&gGraphicsDevice, clipLeft, 0, clipRight, h - 1);
				if (idx == 1)
				{
					centerOffsetPlayer.x += w / 2;
				}

				LOSCalcFrom(&gMap, Vec2iToTile(camera->lastPosition), false);
				DoBuffer(
					&camera->Buffer,
					camera->lastPosition,
					X_TILES_HALF, noise, centerOffsetPlayer);
				SoundSetEarsSide(idx == 0, camera->lastPosition);
			}
			Draw_Line(w / 2 - 1, 0, w / 2 - 1, h - 1, colorBlack);
			Draw_Line(w / 2, 0, w / 2, h - 1, colorBlack);
		}
		else if (numLocalPlayers >= 3 && numLocalPlayers <= 4)
		{
			// 4 player split screen
			int idx = 0;
			bool isLocalPlayerAlive[4];
			memset(isLocalPlayerAlive, 0, sizeof isLocalPlayerAlive);
			for (int i = 0; i < (int)gPlayerDatas.size; i++, idx++)
			{
				const PlayerData *p = CArrayGet(&gPlayerDatas, i);
				if (!p->IsLocal)
				{
					idx--;
					continue;
				}
				Vec2i centerOffsetPlayer = centerOffset;
				const int clipLeft = (idx & 1) ? w / 2 : 0;
				const int clipTop = (idx < 2) ? 0 : h / 2 - 1;
				const int clipRight = (idx & 1) ? w - 1 : (w / 2) - 1;
				const int clipBottom = (idx < 2) ? h / 2 : h - 1;
				isLocalPlayerAlive[idx] = IsPlayerAliveOrDying(p);
				if (!isLocalPlayerAlive[idx])
				{
					continue;
				}
				const TActor *a = ActorGetByUID(p->ActorUID);
				camera->lastPosition = Vec2iNew(a->tileItem.x, a->tileItem.y);
				GraphicsSetBlitClip(
					&gGraphicsDevice,
					clipLeft, clipTop, clipRight, clipBottom);
				if (idx & 1)
				{
					centerOffsetPlayer.x += w / 2;
				}
				if (idx < 2)
				{
					centerOffsetPlayer.y -= h / 4;
				}
				else
				{
					centerOffsetPlayer.y += h / 4;
				}
				LOSCalcFrom(&gMap, Vec2iToTile(camera->lastPosition), false);
				DoBuffer(
					&camera->Buffer,
					camera->lastPosition,
					X_TILES_HALF, noise, centerOffsetPlayer);

				// Set the sound "ears"
				const bool isLeft = idx == 0 || idx == 2;
				const bool isUpper = idx <= 2;
				SoundSetEar(isLeft, isUpper ? 0 : 1, camera->lastPosition);
				// If any player is dead, that ear reverts to the other ear
				// of the same side of the remaining player
				const int otherIdxOnSameSide = idx ^ 2;
				if (!isLocalPlayerAlive[otherIdxOnSameSide])
				{
					SoundSetEar(
						isLeft, !isUpper ? 0 : 1, camera->lastPosition);
				}
				else if (!isLocalPlayerAlive[3 - idx] &&
					!isLocalPlayerAlive[3 - otherIdxOnSameSide])
				{
					// If both players of one side are dead,
					// those ears revert to any of the other remaining
					// players
					SoundSetEarsSide(!isLeft, camera->lastPosition);
				}
			}
			Draw_Line(w / 2 - 1, 0, w / 2 - 1, h - 1, colorBlack);
			Draw_Line(w / 2, 0, w / 2, h - 1, colorBlack);
			Draw_Line(0, h / 2 - 1, w - 1, h / 2 - 1, colorBlack);
			Draw_Line(0, h / 2, w - 1, h / 2, colorBlack);
		}
		else
		{
			assert(0 && "not implemented yet");
		}
	}
	GraphicsResetBlitClip(&gGraphicsDevice);

	HUDDraw(&camera->HUD, pausingDevice);

	// Draw camera mode
	char cameraNameBuf[256];
	bool drawCameraMode = false;
	switch (camera->spectateMode)
	{
	case SPECTATE_NONE:
		// do nothing
		break;
	case SPECTATE_FOLLOW:
		{
			const PlayerData *p = PlayerDataGetByUID(camera->FollowPlayerUID);
			if (p == NULL) break;
			sprintf(cameraNameBuf, "Following %s", p->name);
			drawCameraMode = true;
		}
		break;
	case SPECTATE_FREE:
		strcpy(cameraNameBuf, "Free-look Mode");
		drawCameraMode = true;
		break;
	default:
		CASSERT(false, "Unknown spectate mode");
		break;
	}
	if (drawCameraMode)
	{
		// Draw the message centered at the bottom
		FontStrMask(
			cameraNameBuf,
			Vec2iNew((w - FontStrW(cameraNameBuf)) / 2, h - FontH() * 2),
			colorYellow);

		// Show camera controls
		const PlayerData *p = GetFirstPlayer(false, true, true);
		CASSERT(p != NULL, "No human local players");
		char directionNames[256];
		InputGetDirectionNames(
			directionNames, p->inputDevice, p->deviceIndex);
		char controlsBuf[256];
		sprintf(controlsBuf, "%s/%s to follow player, %s to free-look",
			InputGetButtonName(p->inputDevice, p->deviceIndex, CMD_BUTTON1),
			InputGetButtonName(p->inputDevice, p->deviceIndex, CMD_BUTTON2),
			directionNames);
		FontStrMask(
			controlsBuf,
			Vec2iNew((w - FontStrW(controlsBuf)) / 2, h - FontH()),
			colorYellow);
	}
}
// Try to follow a player
static void FollowPlayer(Vec2i *pos, const int playerUID)
{
	const PlayerData *p = PlayerDataGetByUID(playerUID);
	if (p == NULL) return;
	const TActor *a = ActorGetByUID(p->ActorUID);
	if (a == NULL) return;
	*pos = Vec2iFull2Real(a->Pos);
}
static void DoBuffer(
	DrawBuffer *b, Vec2i center, int w, Vec2i noise, Vec2i offset)
{
	DrawBufferSetFromMap(b, &gMap, Vec2iAdd(center, noise), w);
	DrawBufferFix(b);
	DrawBufferDraw(b, offset, NULL);
}

bool CameraIsSingleScreen(void)
{
	if (ConfigGetEnum(&gConfig, "Interface.Splitscreen") == SPLITSCREEN_ALWAYS)
	{
		return false;
	}
	// Do split screen for PVP, unless whole map fits on camera, or there
	// are no human players alive or dying, then just do single screen
	if (IsPVP(gCampaign.Entry.Mode) &&
		(gMap.Size.x * TILE_WIDTH >= gGraphicsDevice.cachedConfig.Res.x ||
		gMap.Size.y * TILE_HEIGHT >= gGraphicsDevice.cachedConfig.Res.y) &&
		GetNumPlayers(PLAYER_ALIVE_OR_DYING, true, true) > 0)
	{
		return false;
	}
	Vec2i min;
	Vec2i max;
	PlayersGetBoundingRectangle(&min, &max);
	return
		max.x - min.x < gGraphicsDevice.cachedConfig.Res.x - CAMERA_SPLIT_PADDING &&
		max.y - min.y < gGraphicsDevice.cachedConfig.Res.y - CAMERA_SPLIT_PADDING;
}