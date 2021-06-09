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

	Copyright (c) 2013, 2016-2017, 2019, 2021 Cong Xu
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

#ifdef __EMSCRIPTEN__
#include <SDL2/SDL_mixer.h>
#else
#include <SDL_mixer.h>
#endif
#include <stdbool.h>

#include "c_array.h"

typedef enum
{
	MUSIC_MENU,
	MUSIC_BRIEFING,
	MUSIC_GAME,
	// TODO: end music
	// TODO: lose music
	// TODO: victory music
	MUSIC_COUNT
} MusicType;

typedef enum
{
	MUSIC_SRC_GENERAL,
	MUSIC_SRC_DYNAMIC,
	MUSIC_SRC_CHUNK
} MusicSourceType;

typedef struct
{
	bool isInitialised;
	MusicSourceType type;
	union {
		Mix_Music *general;
		Mix_Music *dynamic;
		struct
		{
			Mix_Chunk *chunk;
			int channel;
		} chunk;
	} u;
	CArray generalTracks[MUSIC_COUNT]; // of Mix_Music *
	char errorMessage[128];
} MusicPlayer;

typedef struct
{
	void *Data;
	Mix_Chunk *(*GetData)(void *);
	Mix_Chunk *Chunk;
} MusicChunk;

void MusicPlayerInit(MusicPlayer *mp);
void MusicPlayerTerminate(MusicPlayer *mp);
Mix_Music *MusicLoad(const char *path);
void MusicPlayGeneral(MusicPlayer *mp, const MusicType type);
void MusicPlayFile(
	MusicPlayer *mp, const MusicType type, const char *missionPath,
	const char *music);
void MusicPlayChunk(MusicPlayer *mp, const MusicType type, Mix_Chunk *chunk);
void MusicStop(MusicPlayer *mp);
void MusicPause(MusicPlayer *mp);
void MusicResume(MusicPlayer *mp);
void MusicSetPlaying(MusicPlayer *mp, const bool isPlaying);
const char *MusicGetErrorMessage(const MusicPlayer *mp);

void MusicChunkTerminate(MusicChunk *chunk);
void MusicPlayFromChunk(
	MusicPlayer *mp, const MusicType type, MusicChunk *chunk);
