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

#include <stdbool.h>

#include <SDL_mixer.h>

#include "c_array.h"
#include "c_hashmap/hashmap.h"
#include "defs.h"
#include "sys_config.h"
#include "utils.h"
#include "vector.h"

typedef enum
{
	SOUND_NORMAL,
	SOUND_RANDOM
} SoundType;

typedef struct
{
	SoundType Type;
	union
	{
		Mix_Chunk *normal;
		struct
		{
			CArray sounds;	// of Mix_Chunk *
			int lastPlayed;
		} random;
	} u;
} SoundData;

typedef enum
{
	MUSIC_OK,
	MUSIC_NOLOAD,
	MUSIC_PLAYING,
	MUSIC_PAUSED
} music_status_e;

typedef struct
{
	int isInitialised;
	Mix_Music *music;
	music_status_e musicStatus;
	char musicErrorMessage[128];
	int channels;

	// Two sets of ears for 4-player split screen
	Vec2i earLeft1;
	Vec2i earLeft2;
	Vec2i earRight1;
	Vec2i earRight2;

	map_t sounds;		// of SoundData
	map_t customSounds;	// of SoundData
} SoundDevice;

extern SoundDevice gSoundDevice;

typedef struct
{
	int SoundVolume;
	int MusicVolume;
	bool Footsteps;
	bool Hits;
	bool Reloads;
} SoundConfig;

// DTO for playing certain sounds associated with collision
// for a bullet or weapon type
typedef struct
{
	char *Object;
	char *Flesh;
	char *Wall;
} HitSounds;

void SoundInitialize(SoundDevice *device, const char *path);
void SoundLoadDir(map_t sounds, const char *path, const char *prefix);
void SoundReconfigure(SoundDevice *s);
void SoundClear(map_t sounds);
void SoundTerminate(SoundDevice *device, const bool waitForSoundsComplete);
void SoundPlay(SoundDevice *device, Mix_Chunk *data);
void SoundSetEarsSide(const bool isLeft, const Vec2i pos);
void SoundSetEar(const bool isLeft, const int idx, Vec2i pos);
void SoundSetEars(Vec2i pos);
void SoundPlayAt(SoundDevice *device, Mix_Chunk *data, const Vec2i pos);

// Play a sound but with distance added
// Simulates a quieter sound by adding distance attenuation
void SoundPlayAtPlusDistance(
	SoundDevice *device, Mix_Chunk *data,
	const Vec2i pos, const int plusDistance);

Mix_Chunk *StrSound(const char *s);
