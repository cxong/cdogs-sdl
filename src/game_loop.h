/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.

	Copyright (c) 2014, 2017-2018, 2022 Cong Xu
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

#include <stdbool.h>

#include <cdogs/c_array.h>

// Result from calling update callback,
// what the game loop should do after update
typedef enum
{
	UPDATE_RESULT_OK,
	UPDATE_RESULT_DRAW
} GameLoopResult;

typedef struct
{
	CArray Loops; // of GameLoopData *
} LoopRunner;

// Generic game loop manager, with callbacks for update/draw
typedef struct sGameLoopData
{
	void *Data;
	void (*OnTerminate)(struct sGameLoopData *);
	void (*OnEnter)(struct sGameLoopData *);
	void (*OnExit)(struct sGameLoopData *);
	void (*InputFunc)(struct sGameLoopData *);
	GameLoopResult (*UpdateFunc)(struct sGameLoopData *, LoopRunner *);
	void (*DrawFunc)(struct sGameLoopData *);
	int FPS;
	bool SuperhotMode;
	bool InputEverySecondFrame;
	bool SkipNextFrame;
	int Frames; // total frames looped
	bool HasEntered;
	bool HasExited;
	bool HasDrawnFirst;
	bool IsUsed;
	bool DrawParent;
} GameLoopData;

GameLoopData *GameLoopDataNew(
	void *data, void (*onTerminate)(GameLoopData *),
	void (*onEnter)(GameLoopData *), void (*onExit)(GameLoopData *),
	void (*inputFunc)(GameLoopData *),
	GameLoopResult (*updateFunc)(GameLoopData *, LoopRunner *),
	void (*drawFunc)(GameLoopData *));

LoopRunner LoopRunnerNew(void);
void LoopRunnerTerminate(LoopRunner *l);
void LoopRunnerRun(LoopRunner *l);

void LoopRunnerChange(LoopRunner *l, GameLoopData *newData);
void LoopRunnerPush(LoopRunner *l, GameLoopData *newData);
void LoopRunnerPop(LoopRunner *l);
