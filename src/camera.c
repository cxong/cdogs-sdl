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

#include <cdogs/actors.h>
#include <cdogs/draw.h>
#include <cdogs/drawtools.h>
#include <cdogs/los.h>
#include <cdogs/player.h>


void CameraInit(Camera *camera)
{
	DrawBufferInit(
		&camera->Buffer, Vec2iNew(X_TILES, Y_TILES), &gGraphicsDevice);
	camera->lastPosition = Vec2iZero();
	camera->shake = ScreenShakeZero();
}

void CameraTerminate(Camera *camera)
{
	DrawBufferTerminate(&camera->Buffer);
}

void CameraUpdate(Camera *camera, const int ticks)
{
	camera->shake = ScreenShakeUpdate(camera->shake, ticks);
}

static void DoBuffer(
	DrawBuffer *b, Vec2i center, int w, Vec2i noise, Vec2i offset);
void CameraDraw(Camera *camera)
{
	Vec2i centerOffset = Vec2iZero();
	const int numLocalPlayersAlive =
		GetNumPlayers(PLAYER_ALIVE_OR_DYING, false, true);
	const int numLocalPlayers = GetNumPlayers(PLAYER_ANY, false, true);
	const int w = gGraphicsDevice.cachedConfig.Res.x;
	const int h = gGraphicsDevice.cachedConfig.Res.y;

	for (int i = 0; i < GraphicsGetScreenSize(&gGraphicsDevice.cachedConfig); i++)
	{
		gGraphicsDevice.buf[i] = PixelFromColor(&gGraphicsDevice, colorBlack);
	}

	const Vec2i noise = ScreenShakeGetDelta(camera->shake);

	GraphicsResetBlitClip(&gGraphicsDevice);
	if (numLocalPlayersAlive == 0)
	{
		DoBuffer(
			&camera->Buffer,
			camera->lastPosition,
			X_TILES, noise, centerOffset);
	}
	else
	{
		const int numLocalHumanPlayersAlive =
			GetNumPlayers(PLAYER_ALIVE_OR_DYING, true, true);
		if (numLocalHumanPlayersAlive == 1 || numLocalPlayersAlive == 1)
		{
			const TActor *p = ActorGetByUID(
				(numLocalHumanPlayersAlive == 1 ?
				GetFirstPlayer(true, true, true) :
				GetFirstPlayer(true, false, true))->ActorUID);
			camera->lastPosition = Vec2iNew(p->tileItem.x, p->tileItem.y);
			DoBuffer(
				&camera->Buffer,
				camera->lastPosition,
				X_TILES, noise, centerOffset);
			SoundSetEars(camera->lastPosition);
		}
		else if (CameraIsSingleScreen())
		{
			// One screen
			camera->lastPosition = PlayersGetMidpoint();
			DoBuffer(
				&camera->Buffer,
				camera->lastPosition,
				X_TILES, noise, centerOffset);
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
				const TActor *player = ActorGetByUID(p->ActorUID);
				camera->lastPosition = Vec2iNew(
					player->tileItem.x, player->tileItem.y);
				Vec2i centerOffsetPlayer = centerOffset;
				int clipLeft = (idx & 1) ? w / 2 : 0;
				int clipRight = (idx & 1) ? w - 1 : (w / 2) - 1;
				GraphicsSetBlitClip(
					&gGraphicsDevice, clipLeft, 0, clipRight, h - 1);
				if (idx == 1)
				{
					centerOffsetPlayer.x += w / 2;
				}

				// Redo LOS if PVP, so that each split screen has its own LOS
				if (IsPVP(gCampaign.Entry.Mode))
				{
					LOSReset(&gMap);
					LOSCalcFrom(
						&gMap, Vec2iToTile(camera->lastPosition), false);
				}
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
				isLocalPlayerAlive[idx] = IsPlayerAlive(p);
				if (!isLocalPlayerAlive[idx])
				{
					continue;
				}
				const TActor *player = ActorGetByUID(p->ActorUID);
				camera->lastPosition =
					Vec2iNew(player->tileItem.x, player->tileItem.y);
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
				// Redo LOS if PVP, so that each split screen has its own LOS
				if (IsPVP(gCampaign.Entry.Mode))
				{
					LOSReset(&gMap);
					LOSCalcFrom(
						&gMap, Vec2iToTile(camera->lastPosition), false);
				}
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
	// Always do split screen for PVP
	if (IsPVP(gCampaign.Entry.Mode)) return false;
	Vec2i min;
	Vec2i max;
	PlayersGetBoundingRectangle(&min, &max);
	return
		max.x - min.x < gGraphicsDevice.cachedConfig.Res.x - CAMERA_SPLIT_PADDING &&
		max.y - min.y < gGraphicsDevice.cachedConfig.Res.y - CAMERA_SPLIT_PADDING;
}