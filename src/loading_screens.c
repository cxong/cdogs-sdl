/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.
	Copyright (c) 2022 Cong Xu
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
#include "loading_screens.h"

#include <cdogs/draw/draw.h>
#include <cdogs/files.h>
#include <cdogs/font.h>
#include <cdogs/map_build.h>
#include <cdogs/pic_manager.h>
#include <cdogs/pics.h>
#include <cdogs/tile_class.h>

LoadingScreen gLoadingScreen;

void LoadingScreenInit(LoadingScreen *l, GraphicsDevice *g)
{
	memset(l, 0, sizeof *l);
	l->g = g;
}
void LoadingScreenTerminate(LoadingScreen *l)
{
	CArrayTerminate(&l->panelIndices);
}

static void LazyLoad(LoadingScreen *l)
{
	if (l->logo == NULL)
	{
		l->logo = PicManagerGetPic(&gPicManager, "logo");
	}
	if (l->panel == NULL)
	{
		l->panel = PicManagerGetPic(&gPicManager, "bigpanel");
	}

	// Init panel array large enough to cover the entire screen
	if (l->panel)
	{
		const struct vec2i panelsSize = svec2i_add(svec2i_divide(l->g->cachedConfig.Res, l->panel->size), svec2i_one());
		if (panelsSize.x * panelsSize.y != (int)l->panelIndices.size)
		{
			CArrayTerminate(&l->panelIndices);
			CArrayInit(&l->panelIndices, sizeof(int));
			for (int i = 0; i < panelsSize.x * panelsSize.y; i++)
			{
				CArrayPushBack(&l->panelIndices, &i);
			}
			CArrayShuffle(&l->panelIndices);
		}
	}

	if (gSoundDevice.isInitialised && l->sndTick == NULL)
	{
		l->sndTick = StrSound("click");
	}
}

static void LoadingScreenDrawInner(
	LoadingScreen *l, const char *loadingText, const float showPct)
{
	LazyLoad(l);

	if (l->panel)
	{
		const int stride = l->g->cachedConfig.Res.x / l->panel->size.x + 1;
		CA_FOREACH(const int, panelIdx, l->panelIndices)
		if (_ca_index > (int)(l->panelIndices.size * showPct))
		{
			break;
		}
		const int y = *panelIdx / stride;
		const int x = *panelIdx - (y * stride);
		PicRender(
			l->panel, l->g->gameWindow.renderer, svec2i_multiply(svec2i(x, y), l->panel->size), colorWhite, 0,
			svec2_one(), SDL_FLIP_NONE, Rect2iZero());
		CA_FOREACH_END()
	}

	if (l->logo)
	{
		const struct vec2i pos = svec2i(
			CENTER_X(svec2i_zero(), l->g->cachedConfig.Res, l->logo->size.x),
			CENTER_Y(
				svec2i_zero(), svec2i_scale_divide(l->g->cachedConfig.Res, 2),
				l->logo->size.y));
		PicRender(
			l->logo, l->g->gameWindow.renderer, pos, colorWhite, 0,
			svec2_one(), SDL_FLIP_NONE, Rect2iZero());
	}

	FontOpts opts = FontOptsNew();
	opts.HAlign = ALIGN_CENTER;
	opts.VAlign = ALIGN_CENTER;
	opts.Area = svec2i(l->g->cachedConfig.Res.x, l->g->cachedConfig.Res.y / 2);
	FontStrOpt(loadingText, svec2i(0, l->g->cachedConfig.Res.y / 2), opts);
}
void LoadingScreenDraw(
	LoadingScreen *l, const char *loadingText, const float showPct)
{
	WindowContextPreRender(&l->g->gameWindow);

	LoadingScreenDrawInner(l, loadingText, showPct);

	WindowContextPostRender(&l->g->gameWindow);

	SoundPlayAtPlusDistance(&gSoundDevice, l->sndTick, gSoundDevice.earLeft1, 255);

	SDL_Delay(70);
}

typedef struct
{
	LoadingScreen *l;
	bool ascending;
	const char *loadingText;
	float showPct;
	GameLoopData *nextLoop;
	bool removeParent;
	int count;
} ScreenLoadingData;
static void LoopTerminate(GameLoopData *data);
static GameLoopResult LoopUpdate(GameLoopData *data, LoopRunner *l);
static void LoopDraw(GameLoopData *data);

GameLoopData *ScreenLoading(
	const char *loadingText, const bool ascending, GameLoopData *nextLoop, const bool removeParent)
{
	ScreenLoadingData *sData;
	CCALLOC(sData, sizeof *sData);
	sData->l = &gLoadingScreen;
	sData->loadingText = loadingText;
	sData->ascending = ascending;
	sData->removeParent = removeParent;
	sData->showPct = ascending ? 0.0f : 1.0f;
	sData->nextLoop = nextLoop;
	GameLoopData *data = GameLoopDataNew(
		sData, LoopTerminate, NULL, NULL, NULL, LoopUpdate, LoopDraw);
	data->DrawParent = true;
	return data;
}

static void LoopTerminate(GameLoopData *data)
{
	ScreenLoadingData *sData = data->Data;
	CFREE(sData);
}
static GameLoopResult LoopUpdate(GameLoopData *data, LoopRunner *l)
{
	ScreenLoadingData *sData = data->Data;
	const bool complete = sData->showPct == (sData->ascending ? 1.0f : 0.0f);
	if ((sData->count % 2) == 0)
	{
		SoundPlayAtPlusDistance(&gSoundDevice, sData->l->sndTick, gSoundDevice.earLeft1, 255);
	}
	sData->count++;
	if (complete)
	{
		// Remove current loop, but copy its data before destroying it
		ScreenLoadingData sDataCopy = *sData;
		sData = NULL;
		LoopRunnerPop(l);
		if (sDataCopy.ascending)
		{
			if (sDataCopy.removeParent)
			{
				LoopRunnerPop(l);
			}
			if (sDataCopy.nextLoop)
			{
				LoopRunnerPush(l, sDataCopy.nextLoop);
				// Show a loading screen with tiles animating out
				LoopRunnerPush(l, ScreenLoading(sDataCopy.loadingText, false, NULL, false));
			}
		}
		return UPDATE_RESULT_OK;
	}
	sData->showPct += sData->ascending ? 0.04f : -0.06f;
	sData->showPct = CLAMP(sData->showPct, 0.0f, 1.0f);
	return UPDATE_RESULT_DRAW;
}
static void LoopDraw(GameLoopData *data)
{
	ScreenLoadingData *sData = data->Data;
	LoadingScreenDrawInner(sData->l, sData->loadingText, sData->showPct);
}
