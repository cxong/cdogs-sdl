/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.

	Copyright (c) 2014, 2016-2018, 2021 Cong Xu
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
#include "game_loop.h"

#include <SDL_timer.h>

#include "config.h"
#include "events.h"
#include "net_client.h"
#include "net_server.h"
#include "sounds.h"

#ifdef __EMSCRIPTEN__
#include <autosave.h>
#include <cdogs/character_class.h>
#include <cdogs/config_io.h>
#include <cdogs/config_json.h>
#include <cdogs/files.h>
#include <cdogs/player_template.h>
#include <emscripten.h>
#include <hiscores.h>

bool emscripten_fs_ready = false;

void emscriptenLoadFiles()
{
	// options.cnf
	FILE *file_options = fopen(GetConfigFilePath(CONFIG_FILE), "r");
	if (file_options)
	{
		fclose(file_options);
		ConfigLoadJSON(&gConfig, GetConfigFilePath(CONFIG_FILE));
		ConfigApply(&gConfig, NULL);
	}
	else
	{
		ConfigSave(&gConfig, GetConfigFilePath(CONFIG_FILE));
	}

	// autosave.json
	FILE *file_autosave = fopen(GetConfigFilePath(AUTOSAVE_FILE), "r");
	if (file_autosave)
	{
		fclose(file_autosave);
		AutosaveLoad(&gAutosave, GetConfigFilePath(AUTOSAVE_FILE));
	}
	else
	{
		AutosaveSave(&gAutosave, GetConfigFilePath(AUTOSAVE_FILE));
	}

	// scores.dat
	FILE *file_scores = fopen(GetConfigFilePath(SCORES_FILE), "r");
	if (file_scores)
	{
		fclose(file_scores);
	}
	else
	{
		SaveHighScores();
	}

	// players.cnf
	FILE *file_players = fopen(GetConfigFilePath(PLAYER_TEMPLATE_FILE), "r");
	if (file_players)
	{
		fclose(file_players);
		PlayerTemplatesLoad(&gPlayerTemplates, &gCharacterClasses);
	}
	else
	{
		PlayerTemplatesSave(&gPlayerTemplates);
	}
}

bool emscriptenPersistData()
{
	if (emscripten_fs_ready)
		return true;

	if (emscripten_run_script_int("Module.syncdone") == 1)
	{
		FILE *config_file = fopen(GetConfigFilePath(CONFIG_FILE), "r");
		if (config_file == NULL)
		{
			// persist Emscripten current data to Indexed Db
			EM_ASM(Module.print("Start File sync.."); Module.syncdone = 0;
				   FS.syncfs(
					   false, function(err) {
						   assert(!err);
						   Module.print("End File sync..");
						   Module.syncdone = 1;
					   }););
			emscriptenLoadFiles();
			return false;
		}
		else
		{
			fclose(config_file);
			emscripten_fs_ready = true;
			emscriptenLoadFiles();
			return true;
		}
	}
	return false;
}
#endif

GameLoopData *GameLoopDataNew(
	void *data, void (*onTerminate)(GameLoopData *),
	void (*onEnter)(GameLoopData *), void (*onExit)(GameLoopData *),
	void (*inputFunc)(GameLoopData *),
	GameLoopResult (*updateFunc)(GameLoopData *, LoopRunner *),
	void (*drawFunc)(GameLoopData *))
{
	GameLoopData *g;
	CCALLOC(g, sizeof *g);
	g->Data = data;
	g->OnTerminate = onTerminate;
	g->OnEnter = onEnter;
	g->OnExit = onExit;
	g->InputFunc = inputFunc;
	g->UpdateFunc = updateFunc;
	g->DrawFunc = drawFunc;
	g->FPS = 30;
	return g;
}

static GameLoopData *GetCurrentLoop(LoopRunner *l);

LoopRunner LoopRunnerNew(GameLoopData *newData)
{
	LoopRunner l;
	CArrayInit(&l.Loops, sizeof(GameLoopData *));
	if (newData != NULL)
	{
		LoopRunnerPush(&l, newData);
	}
	return l;
}
static void GameLoopTerminate(GameLoopData *data);
void LoopRunnerTerminate(LoopRunner *l)
{
	for (int i = (int)l->Loops.size - 1; i >= 0; i--)
	{
		GameLoopData *g = CArrayGet(&l->Loops, i);
		GameLoopTerminate(g);
	}
	CArrayTerminate(&l->Loops);
}

static void GameLoopOnEnter(GameLoopData *data);
static void GameLoopOnExit(GameLoopData *data);
typedef struct
{
	GameLoopResult Result;
	Uint32 TicksNow;
	Uint32 TicksElapsed;
	int FrameDurationMs;
	int FramesSkipped;
	int MaxFrameskip;
} LoopRunParams;
typedef struct
{
	LoopRunner *l;
	GameLoopData *data;
	LoopRunParams p;
} LoopRunInnerData;
static LoopRunParams LoopRunParamsNew(const GameLoopData *data);
static bool LoopRunParamsShouldSleep(LoopRunParams *p);
static bool LoopRunParamsShouldSkip(LoopRunParams *p);
bool LoopRunnerRunInner(LoopRunInnerData *ctx)
{
#ifndef __EMSCRIPTEN__
	// Frame rate control
	if (LoopRunParamsShouldSleep(&(ctx->p)))
	{
		SDL_Delay(1);
		return true;
	}
#endif

	// Input
	if ((ctx->data->Frames & 1) || !ctx->data->InputEverySecondFrame)
	{
		EventPoll(&gEventHandlers, ctx->p.TicksElapsed, NULL);
		if (ctx->data->InputFunc)
		{
			ctx->data->InputFunc(ctx->data);
		}
	}

	NetClientPoll(&gNetClient);
	NetServerPoll(&gNetServer);

	// Update
	ctx->p.Result = ctx->data->UpdateFunc(ctx->data, ctx->l);
	GameLoopData *newData = GetCurrentLoop(ctx->l);
	if (newData == NULL)
	{
		return false;
	}
	else if (newData != ctx->data)
	{
		// State change; restart loop
		GameLoopOnExit(ctx->data);
		ctx->data = newData;
		GameLoopOnEnter(ctx->data);
		ctx->p = LoopRunParamsNew(ctx->data);
		return true;
	}

	NetServerFlush(&gNetServer);
	NetClientFlush(&gNetClient);

	bool draw = !ctx->data->HasDrawnFirst;
	switch (ctx->p.Result)
	{
	case UPDATE_RESULT_OK:
		// Do nothing
		break;
	case UPDATE_RESULT_DRAW:
		draw = true;
		break;
	default:
		CASSERT(false, "Unknown loop result");
		break;
	}
	ctx->data->Frames++;
#ifndef __EMSCRIPTEN__
	// frame skip
	if (LoopRunParamsShouldSkip(&(ctx->p)))
	{
		return true;
	}
#endif

	// Draw
	if (draw)
	{
		WindowContextPreRender(&gGraphicsDevice.gameWindow);
		if (gGraphicsDevice.cachedConfig.SecondWindow)
		{
			WindowContextPreRender(&gGraphicsDevice.secondWindow);
		}
		if (ctx->data->DrawFunc)
		{
			ctx->data->DrawFunc(ctx->data);
		}
		WindowContextPostRender(&gGraphicsDevice.gameWindow);
		if (gGraphicsDevice.cachedConfig.SecondWindow)
		{
			WindowContextPostRender(&gGraphicsDevice.secondWindow);
		}
		ctx->data->HasDrawnFirst = true;
	}

	return true;
}

#ifdef __EMSCRIPTEN__
void EmscriptenMainLoop(void *arg)
{
	if (!emscriptenPersistData())
		return;

	LoopRunnerRunInner((LoopRunInnerData *)arg);
}
#endif
void LoopRunnerRun(LoopRunner *l)
{
	GameLoopData *data = GetCurrentLoop(l);
	if (data == NULL)
	{
		return;
	}
	GameLoopOnEnter(data);

	LoopRunInnerData ctx;
	ctx.l = l;
	ctx.data = data;
	ctx.p = LoopRunParamsNew(data);

#ifdef __EMSCRIPTEN__
	// TODO use GameLoopData->FPS instead of FPS_FRAMELIMIT?
	emscripten_set_main_loop_arg(EmscriptenMainLoop, &ctx, FPS_FRAMELIMIT, 1);
#else
	for (;;)
	{
		if (!LoopRunnerRunInner(&ctx))
		{
			break;
		}
	}
#endif
	GameLoopOnExit(ctx.data);
}
static LoopRunParams LoopRunParamsNew(const GameLoopData *data)
{
	LoopRunParams p;
	p.Result = UPDATE_RESULT_OK;
	p.TicksNow = SDL_GetTicks();
	p.TicksElapsed = 0;
	p.FrameDurationMs = 1000 / data->FPS;
	p.FramesSkipped = 0;
	p.MaxFrameskip = data->FPS / 5;
	return p;
}
static bool LoopRunParamsShouldSleep(LoopRunParams *p)
{
	const Uint32 ticksThen = p->TicksNow;
	p->TicksNow = SDL_GetTicks();
	p->TicksElapsed += p->TicksNow - ticksThen;
	return (int)p->TicksElapsed < p->FrameDurationMs;
}
static bool LoopRunParamsShouldSkip(LoopRunParams *p)
{
	p->TicksElapsed -= p->FrameDurationMs;
	// frame skip
	if ((int)p->TicksElapsed > p->FrameDurationMs)
	{
		p->FramesSkipped++;
		if (p->FramesSkipped == p->MaxFrameskip)
		{
			// We've skipped too many frames; give up
			p->TicksElapsed = 0;
		}
		else
		{
			return true;
		}
	}
	p->FramesSkipped = 0;
	return false;
}

void LoopRunnerChange(LoopRunner *l, GameLoopData *newData)
{
	LoopRunnerPop(l);
	LoopRunnerPush(l, newData);
}
void LoopRunnerPush(LoopRunner *l, GameLoopData *newData)
{
	CArrayPushBack(&l->Loops, &newData);
	newData->IsUsed = true;
}
void LoopRunnerPop(LoopRunner *l)
{
	GameLoopData *data = GetCurrentLoop(l);
	data->IsUsed = false;
	CArrayDelete(&l->Loops, l->Loops.size - 1);
}

static void GameLoopTerminate(GameLoopData *data)
{
	if (data->OnTerminate)
	{
		data->OnTerminate(data);
	}
	CFREE(data);
}

static void GameLoopOnEnter(GameLoopData *data)
{
	if (data->OnEnter)
	{
		data->OnEnter(data);
	}
	EventReset(&gEventHandlers);
}
static void GameLoopOnExit(GameLoopData *data)
{
	if (data->OnExit)
	{
		data->OnExit(data);
	}
	if (!data->IsUsed)
	{
		GameLoopTerminate(data);
	}
}

static GameLoopData *GetCurrentLoop(LoopRunner *l)
{
	if (l->Loops.size == 0)
	{
		return NULL;
	}
	return *(GameLoopData **)CArrayGet(&l->Loops, l->Loops.size - 1);
}
