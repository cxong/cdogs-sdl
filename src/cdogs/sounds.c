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

	Copyright (c) 2013-2017, 2019-2020 Cong Xu
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
#include "sounds.h"

#include <ctype.h>
#include <math.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <SDL.h>

#include <tinydir/tinydir.h>

#include "algorithms.h"
#include "files.h"
#include "log.h"
#include "map.h"
#include "music.h"
#include "vector.h"

SoundDevice gSoundDevice;

int OpenAudio(int frequency, Uint16 format, int channels, int chunkSize)
{
	int qFrequency;
	Uint16 qFormat;
	int qChannels;

	if (Mix_OpenAudio(frequency, format, channels, chunkSize) != 0)
	{
		printf("Couldn't open audio!: %s\n", SDL_GetError());
		return 1;
	}

	// Check that we got the specs we wanted
#ifndef __EMSCRIPTEN__
	Mix_QuerySpec(&qFrequency, &qFormat, &qChannels);
	if (qFrequency != frequency || qFormat != format || qChannels != channels)
	{
		printf("Audio not what we want.\n");
		return 1;
	}
#endif

	return 0;
}

static Mix_Chunk *LoadSound(const char *path);
static void SoundLoad(map_t sounds, const char *name, const char *path)
{
	// If the sound basename is a number, it is part of a group of random
	// sounds
	char basename[CDOGS_FILENAME_MAX];
	PathGetBasenameWithoutExtension(basename, name);
	char nameNoExt[CDOGS_PATH_MAX];
	PathGetWithoutExtension(nameNoExt, name);
	bool isNumber = true;
	for (const char *c = basename; *c != '\0'; c++)
	{
		if (!isdigit(*c))
		{
			isNumber = false;
			break;
		}
	}
	if (isNumber)
	{
		// Sound is random
		// Only add the 0 number
		const int n = atoi(basename);
		if (n != 0)
		{
			return;
		}
		SoundData *sound;
		CCALLOC(sound, sizeof *sound);
		sound->Type = SOUND_RANDOM;
		CArrayInit(&sound->u.random.sounds, sizeof(Mix_Chunk *));
		// Remove "0.<ext>" from path
		const char *ext = StrGetFileExt(path);
		const int len = (int)(ext - path - 2);
		char fmt[CDOGS_FILENAME_MAX];
		strncpy(fmt, path, len);
		// Create format string path/to/sound/%d.<ext>
		sprintf(fmt + len, "%%d.%s", ext);
		for (int i = 0;; i++)
		{
			char buf[CDOGS_PATH_MAX];
			sprintf(buf, fmt, i);
			Mix_Chunk *data = LoadSound(buf);
			if (data == NULL)
				break;
			CArrayPushBack(&sound->u.random.sounds, &data);
		}
		// Remove "/0" from name and add
		*strrchr(nameNoExt, '/') = '\0';
		SoundAdd(sounds, nameNoExt, sound);
	}
	else
	{
		Mix_Chunk *data = LoadSound(path);
		if (data != NULL)
		{
			SoundData *sound;
			CMALLOC(sound, sizeof *sound);
			sound->Type = SOUND_NORMAL;
			sound->u.normal = data;
			SoundAdd(sounds, nameNoExt, sound);
		}
	}
}
static Mix_Chunk *LoadSound(const char *path)
{
	// Only load sounds from known extensions
	const char *ext = strrchr(path, '.');
	if (ext == NULL ||
		!(strcmp(ext, ".ogg") == 0 || strcmp(ext, ".OGG") == 0 ||
		  strcmp(ext, ".wav") == 0 || strcmp(ext, ".WAV") == 0 ||
		  strcmp(ext, ".mp3") == 0 || strcmp(ext, ".MP3") == 0))
	{
		return NULL;
	}
	LOG(LM_MAIN, LL_TRACE, "loading sound file %s", path);
	return Mix_LoadWAV(path);
}
static void SoundDataTerminate(any_t data);
void SoundAdd(map_t sounds, const char *name, SoundData *sound)
{
	const int error = hashmap_put(sounds, name, sound);
	if (error != MAP_OK)
	{
		LOG(LM_MAIN, LL_ERROR, "failed to add sound %s: %d", name, error);
		SoundDataTerminate((any_t)sound);
	}
}

void SoundInitialize(SoundDevice *device, const char *path)
{
	memset(device, 0, sizeof *device);
	SoundReopen(device);

	device->sounds = hashmap_new();
	device->customSounds = hashmap_new();
	char buf[CDOGS_PATH_MAX];
	GetDataFilePath(buf, path);
	SoundLoadDir(device->sounds, buf, NULL);
	MusicPlayerInit(&device->music);
}
void SoundLoadDir(map_t sounds, const char *path, const char *prefix)
{
	tinydir_dir dir;
	if (tinydir_open(&dir, path) == -1)
	{
		if (errno != ENOENT)
		{
			LOG(LM_MAIN, LL_ERROR, "Cannot open sound dir '%s': %s", path,
				strerror(errno));
		}
		goto bail;
	}
	for (; dir.has_next; tinydir_next(&dir))
	{
		tinydir_file file;
		if (tinydir_readfile(&dir, &file) == -1)
		{
			LOG(LM_MAIN, LL_ERROR, "Cannot read sound file '%s'", file.path);
			continue;
		}
		if (file.name[0] == '.')
		{
			continue;
		}
		char buf[CDOGS_PATH_MAX];
		if (prefix != NULL)
		{
			sprintf(buf, "%s/%s", prefix, file.name);
		}
		else
		{
			strcpy(buf, file.name);
		}
		if (file.is_reg)
		{
			SoundLoad(sounds, buf, file.path);
		}
		else if (file.is_dir)
		{
			SoundLoadDir(sounds, file.path, buf);
		}
	}

bail:
	tinydir_close(&dir);
}

static void SoundClose(SoundDevice *s, const bool waitForSoundsComplete)
{
	if (!s->isInitialised)
	{
		return;
	}

	if (waitForSoundsComplete)
	{
		Uint32 waitStart = SDL_GetTicks();
		while (Mix_Playing(-1) > 0 && SDL_GetTicks() - waitStart < 1000)
			;
		// Don't stop the music unless we're reopening
		MusicStop(&s->music);
	}
	while (Mix_Init(0))
	{
		Mix_Quit();
	}
	Mix_CloseAudio();
}

void SoundReconfigure(SoundDevice *s)
{
	s->isInitialised = false;
	s->music.isInitialised = false;

	if (Mix_AllocateChannels(s->channels) != s->channels)
	{
		printf("Couldn't allocate channels!\n");
		return;
	}

	const int sVol = ConfigGetInt(&gConfig, "Sound.SoundVolume");
	Mix_Volume(-1, sVol);
	const int mVol = ConfigGetInt(&gConfig, "Sound.MusicVolume");
	Mix_VolumeMusic(mVol);
	MusicSetPlaying(&s->music, mVol > 0);

	s->isInitialised = true;
	s->music.isInitialised = true;
}

void SoundReopen(SoundDevice *s)
{
	SoundClose(s, false);
	if (OpenAudio(CDOGS_SND_RATE, CDOGS_SND_FMT, CDOGS_SND_CHANNELS, 1024) !=
		0)
	{
		return;
	}

	s->channels = 64;
	SoundReconfigure(s);
}

void SoundClear(map_t sounds)
{
	hashmap_clear(sounds, SoundDataTerminate);
}
void SoundTerminate(SoundDevice *device, const bool waitForSoundsComplete)
{
	SoundClose(device, waitForSoundsComplete);

	hashmap_destroy(device->sounds, SoundDataTerminate);
	hashmap_destroy(device->customSounds, SoundDataTerminate);

	MusicPlayerTerminate(&device->music);
}
static void SoundDataTerminate(any_t data)
{
	SoundData *s = data;
	switch (s->Type)
	{
	case SOUND_NORMAL:
		Mix_FreeChunk(s->u.normal);
		break;
	case SOUND_RANDOM:
		CA_FOREACH(Mix_Chunk *, chunk, s->u.random.sounds)
		Mix_FreeChunk(*chunk);
		CA_FOREACH_END()
		CArrayTerminate(&s->u.random.sounds);
		break;
	default:
		CASSERT(false, "Unknown sound data type");
		break;
	}
	CFREE(s);
}

#define OUT_OF_SIGHT_DISTANCE_PLUS 100
static int GetChannel(SoundDevice *s, Mix_Chunk *data);
static void MuffleEffect(int chan, void *stream, int len, void *udata)
{
	UNUSED(chan);
	UNUSED(udata);
	const int channels = 2;
	const int chunk = channels * 2;
	for (int i = 0; i < len / chunk - 2; i++)
	{
		int16_t *samples = (int16_t *)((char *)stream + i * chunk);
		samples[0] = (samples[0] + samples[2] + samples[4]) / 3;
		samples[1] = (samples[1] + samples[3] + samples[5]) / 3;
	}
}
static void SetSoundEffect(
	const int channel, const Sint16 bearingDegrees, const Uint8 distance,
	const bool isMuffled);
static void SoundPlayAtPosition(
	SoundDevice *device, Mix_Chunk *data, const struct vec2 dp,
	const bool isMuffled)
{
	if (!device->isInitialised || data == NULL)
	{
		return;
	}

	int distance = 0;
	Sint16 bearingDegrees = 0;
	const float screen = (float)gGraphicsDevice.cachedConfig.Res.x;
	const float halfScreen = screen / 2;
	if (!svec2_is_zero(dp))
	{
		// Calculate distance and bearing
		// Sound position is calculated from an imaginary camera that's half as
		// distant from the centre of the screen as the screen width, i.e.
		//
		//         centre-+
		//                v
		// Screen: |------+------|
		//                |
		//                |
		//     camera---> +
		// Calculate real distance using Pythagoras
		const float d = svec2_length(dp);
		// Scale so that sounds more than a full screen from centre have
		// maximum distance (255)
		const float maxDistance =
			sqrtf(screen * screen + halfScreen * halfScreen);
		distance = (int)(d * 255 / maxDistance);

		// Calculate bearing
		const double bearing = atan((double)dp.x / halfScreen);
		bearingDegrees = (Sint16)(bearing * 180 / MPI);
		if (bearingDegrees < 0)
		{
			bearingDegrees += 360;
		}
	}
	if (isMuffled)
	{
		distance += OUT_OF_SIGHT_DISTANCE_PLUS;
	}
	// Don't play anything if it's too distant
	// This means we don't waste sound channels
	if (distance > 255)
	{
		return;
	}

	LOG(LM_SOUND, LL_TRACE, "distance(%d) bearing(%d)", distance,
		bearingDegrees);

	// Get sound channel to play sound
	const int channel = GetChannel(device, data);
	if (channel < 0)
	{
		return;
	}

	SetSoundEffect(channel, bearingDegrees, (Uint8)distance, isMuffled);
}
static int GetChannel(SoundDevice *s, Mix_Chunk *data)
{
	for (;;)
	{
		const int channel = Mix_PlayChannel(-1, data, 0);
		if (channel >= 0 || s->channels > 128)
		{
			return channel;
		}
		// Check if we cannot play the sound; allocate more channels
		s->channels *= 2;
		if (Mix_AllocateChannels(s->channels) != s->channels)
		{
			LOG(LM_SOUND, LL_ERROR, "Cannot allocate channels");
			return -1;
		}
		// When allocating new channels, need to reset their volume
		Mix_Volume(-1, ConfigGetInt(&gConfig, "Sound.SoundVolume"));
	}
}
static void SetSoundEffect(
	const int channel, const Sint16 bearingDegrees, const Uint8 distance,
	const bool isMuffled)
{
#ifndef __EMSCRIPTEN__
	Mix_SetPosition(channel, bearingDegrees, (Uint8)distance);
	if (isMuffled)
	{
		if (!Mix_RegisterEffect(channel, MuffleEffect, NULL, NULL))
		{
			fprintf(stderr, "Mix_RegisterEffect: %s\n", Mix_GetError());
		}
	}
#else
	// Mix_SetPosition and Mix_RegisterEffect not supported by emscripten;
	// use plain panning instead

	// Calculate left/right channel as values from 0-180
	int left;
	if (bearingDegrees < 90)
		left = 90 - bearingDegrees;
	else if (bearingDegrees < 270)
		left = bearingDegrees - 90;
	else
		left = 450 - bearingDegrees;
	const int right = 180 - left;
	Mix_SetPanning(
		channel, (Uint8)(left * distance / 180),
		(Uint8)(right * distance / 180));
#endif
}

void SoundPlay(SoundDevice *device, Mix_Chunk *data)
{
	if (!device->isInitialised)
	{
		return;
	}

	SoundPlayAtPosition(device, data, svec2_zero(), false);
}

void SoundSetEar(const bool isLeft, const int idx, const struct vec2 pos)
{
	if (isLeft)
	{
		if (idx == 0)
		{
			gSoundDevice.earLeft1 = pos;
		}
		else
		{
			gSoundDevice.earLeft2 = pos;
		}
	}
	else
	{
		if (idx == 0)
		{
			gSoundDevice.earRight1 = pos;
		}
		else
		{
			gSoundDevice.earRight2 = pos;
		}
	}
}

void SoundSetEarsSide(const bool isLeft, const struct vec2 pos)
{
	SoundSetEar(isLeft, 0, pos);
	SoundSetEar(isLeft, 1, pos);
}

void SoundSetEars(const struct vec2 pos)
{
	SoundSetEarsSide(true, pos);
	SoundSetEarsSide(false, pos);
}

void SoundPlayAt(SoundDevice *device, Mix_Chunk *data, const struct vec2 pos)
{
	SoundPlayAtPlusDistance(device, data, pos, 0);
}

static bool IsPosNoSee(void *data, struct vec2i pos)
{
	const Tile *t = MapGetTile(data, Vec2iToTile(pos));
	return t != NULL && TileIsOpaque(t);
}
void SoundPlayAtPlusDistance(
	SoundDevice *device, Mix_Chunk *data, const struct vec2 pos,
	const int plusDistance)
{
	if (device == NULL || !device->isInitialised)
	{
		return;
	}
	struct vec2 closestLeftEar, closestRightEar;

	// Find closest set of ears to the sound
	if (svec2_distance_squared(pos, device->earLeft1) <
		svec2_distance_squared(pos, device->earLeft2))
	{
		closestLeftEar = device->earLeft1;
	}
	else
	{
		closestLeftEar = device->earLeft2;
	}
	if (svec2_distance_squared(pos, device->earRight1) <
		svec2_distance_squared(pos, device->earRight2))
	{
		closestRightEar = device->earRight1;
	}
	else
	{
		closestRightEar = device->earRight2;
	}

	const struct vec2 origin = CalcClosestPointOnLineSegmentToPoint(
		closestLeftEar, closestRightEar, pos);
	bool isMuffled = false;
	// Don't bother checking muffled if the distance is really close
	// This is for player's own sounds like footsteps
	if (svec2_distance_squared(pos, origin) > SQUARED(TILE_WIDTH))
	{
		HasClearLineData lineData;
		lineData.IsBlocked = IsPosNoSee;
		lineData.data = &gMap;
		if (!HasClearLineJMRaytrace(
				svec2i_assign_vec2(pos), svec2i_assign_vec2(origin), &lineData))
		{
			isMuffled = true;
		}
	}
	const struct vec2 dp = svec2_subtract(pos, origin);
	SoundPlayAtPosition(
		&gSoundDevice, data, svec2(dp.x, fabsf(dp.y) + plusDistance),
		isMuffled);
}

static Mix_Chunk *SoundDataGet(SoundData *s);
Mix_Chunk *StrSound(const char *s)
{
	if (s == NULL || strlen(s) == 0 || !gSoundDevice.isInitialised)
	{
		return NULL;
	}
	SoundData *sound;
	int error = hashmap_get(gSoundDevice.customSounds, s, (any_t *)&sound);
	if (error == MAP_OK)
	{
		return SoundDataGet(sound);
	}
	error = hashmap_get(gSoundDevice.sounds, s, (any_t *)&sound);
	if (error == MAP_OK)
	{
		return SoundDataGet(sound);
	}
	return NULL;
}
static Mix_Chunk *SoundDataGet(SoundData *s)
{
	switch (s->Type)
	{
	case SOUND_NORMAL:
		return s->u.normal;
	case SOUND_RANDOM:
		if (s->u.random.sounds.size == 0)
		{
			return NULL;
		}
		else
		{
			// Don't get the last sound used
			int idx = s->u.random.lastPlayed;
			while ((int)s->u.random.sounds.size > 1 &&
				   idx == s->u.random.lastPlayed)
			{
				idx = rand() % s->u.random.sounds.size;
			}
			Mix_Chunk **sound = CArrayGet(&s->u.random.sounds, idx);
			s->u.random.lastPlayed = idx;
			return *sound;
		}
	default:
		CASSERT(false, "Unknown sound data type");
		return NULL;
	}
}
