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

    Copyright (c) 2013-2014, Cong Xu
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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <SDL.h>

#include <tinydir/tinydir.h>

#include "files.h"
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
	Mix_QuerySpec(&qFrequency, &qFormat, &qChannels);
	debug(D_NORMAL, "spec: f=%d fmt=%d c=%d\n", qFrequency, qFormat, qChannels);
	if (qFrequency != frequency || qFormat != format || qChannels != channels)
	{
		printf("Audio not what we want.\n");
		return 1;
	}

	return 0;
}

static void LoadSound(SoundDevice *device, const char *name, const char *path)
{
	Mix_Chunk *data = Mix_LoadWAV(path);
	if (data == NULL)
	{
		return;
	}
	char buf[CDOGS_FILENAME_MAX];
	PathGetBasenameWithoutExtension(buf, name);
	SoundAdd(&device->sounds, buf, data);
}
void SoundAdd(CArray *sounds, const char *name, Mix_Chunk *data)
{
	SoundData sound;
	sound.data = data;
	strcpy(sound.Name, name);
	CArrayPushBack(sounds, &sound);
}

void SoundInitialize(
	SoundDevice *device, SoundConfig *config, const char *path)
{
	memset(device, 0, sizeof *device);
	if (OpenAudio(22050, AUDIO_S16, 2, 512) != 0)
	{
		return;
	}

	device->channels = 64;
	SoundReconfigure(device, config);

	CArrayInit(&device->sounds, sizeof(SoundData));
	CArrayInit(&device->customSounds, sizeof(SoundData));
	tinydir_dir dir;
	if (tinydir_open(&dir, path) == -1)
	{
		perror("Cannot open sound dir");
		goto bail;
	}
	for (; dir.has_next; tinydir_next(&dir))
	{
		tinydir_file file;
		if (tinydir_readfile(&dir, &file) == -1)
		{
			perror("Cannot read sound file");
			goto bail;
		}
		if (file.is_reg)
		{
			LoadSound(device, file.name, file.path);
		}
	}

	// Look for commonly used sounds to set our pointers
	device->footstepSound = StrSound("footstep");
	device->slideSound = StrSound("slide");
	device->switchSound = StrSound("switch");
	device->pickupSound = StrSound("pickup");
	device->healthSound = StrSound("health");
	device->keySound = StrSound("key");
	device->wreckSound = StrSound("bang");
	CArrayInit(&device->screamSounds, sizeof(Mix_Chunk *));
	for (int i = 0;; i++)
	{
		char buf[CDOGS_FILENAME_MAX];
		sprintf(buf, "aargh%d", i);
		Mix_Chunk *scream = StrSound(buf);
		if (scream == NULL)
		{
			break;
		}
		CArrayPushBack(&device->screamSounds, &scream);
	}

bail:
	tinydir_close(&dir);
}

void SoundReconfigure(SoundDevice *device, SoundConfig *config)
{
	device->isInitialised = 0;

	if (Mix_AllocateChannels(device->channels) != device->channels)
	{
		printf("Couldn't allocate channels!\n");
		return;
	}

	Mix_Volume(-1, config->SoundVolume);
	Mix_VolumeMusic(config->MusicVolume);
	if (config->MusicVolume > 0)
	{
		MusicResume(device);
	}
	else
	{
		MusicPause(device);
	}

	device->isInitialised = 1;
}

void SoundClear(CArray *sounds)
{
	for (int i = 0; i < (int)sounds->size; i++)
	{
		SoundData *sound = CArrayGet(sounds, i);
		Mix_FreeChunk(sound->data);
	}
	CArrayClear(sounds);
}
void SoundTerminate(SoundDevice *device, const bool waitForSoundsComplete)
{
	if (!device->isInitialised)
	{
		return;
	}

	debug(D_NORMAL, "shutting down sound\n");
	if (waitForSoundsComplete)
	{
		Uint32 waitStart = SDL_GetTicks();
		while (Mix_Playing(-1) > 0 &&
			SDL_GetTicks() - waitStart < 1000);
	}
	MusicStop(device);
	while (Mix_Init(0))
	{
		Mix_Quit();
	}
	Mix_CloseAudio();

	SoundClear(&device->sounds);
	CArrayTerminate(&device->sounds);
	SoundClear(&device->customSounds);
	CArrayTerminate(&device->customSounds);
}

void SoundPlayAtPosition(
	SoundDevice *device, Mix_Chunk *data, int distance, int bearing)
{
	if (data == NULL)
	{
		return;
	}
	distance /= 2;
	// Don't play anything if it's too distant
	// This means we don't waste sound channels
	if (distance > 255)
	{
		return;
	}

	if (!device->isInitialised)
	{
		return;
	}

	debug(D_VERBOSE, "sound: distance: %d bearing: %d\n", distance, bearing);

	int channel;
	for (;;)
	{
		channel = Mix_PlayChannel(-1, data, 0);
		if (channel >= 0 || device->channels > 128)
		{
			break;
		}
		// Check if we cannot play the sound; allocate more channels
		device->channels *= 2;
		if (Mix_AllocateChannels(device->channels) != device->channels)
		{
			printf("Couldn't allocate channels!\n");
			return;
		}
	}
	Mix_SetPosition(channel, (Sint16)bearing, (Uint8)distance);
}

void SoundPlay(SoundDevice *device, Mix_Chunk *data)
{
	if (!device->isInitialised)
	{
		return;
	}

	SoundPlayAtPosition(device, data, 0, 0);
}


void SoundSetLeftEar1(Vec2i pos)
{
	gSoundDevice.earLeft1 = pos;
}
void SoundSetLeftEar2(Vec2i pos)
{
	gSoundDevice.earLeft2 = pos;
}
void SoundSetRightEar1(Vec2i pos)
{
	gSoundDevice.earRight1 = pos;
}
void SoundSetRightEar2(Vec2i pos)
{
	gSoundDevice.earRight2 = pos;
}

void SoundSetLeftEars(Vec2i pos)
{
	SoundSetLeftEar1(pos);
	SoundSetLeftEar2(pos);
}
void SoundSetRightEars(Vec2i pos)
{
	SoundSetRightEar1(pos);
	SoundSetRightEar2(pos);
}

void SoundSetEars(Vec2i pos)
{
	SoundSetLeftEars(pos);
	SoundSetRightEars(pos);
}

void SoundPlayAt(SoundDevice *device, Mix_Chunk *data, const Vec2i pos)
{
	SoundPlayAtPlusDistance(device, data, pos, 0);
}

void SoundPlayAtPlusDistance(
	SoundDevice *device, Mix_Chunk *data,
	const Vec2i pos, const int plusDistance)
{
	int distance, bearing;
	Vec2i closestLeftEar, closestRightEar;
	Vec2i origin;

	// Find closest set of ears to the sound
	if (CHEBYSHEV_DISTANCE(
		pos.x, pos.y, device->earLeft1.x, device->earLeft1.y) <
		CHEBYSHEV_DISTANCE(
		pos.x, pos.y, device->earLeft2.x, device->earLeft2.y))
	{
		closestLeftEar = device->earLeft1;
	}
	else
	{
		closestLeftEar = device->earLeft2;
	}
	if (CHEBYSHEV_DISTANCE(
		pos.x, pos.y, device->earRight1.x, device->earRight1.y) <
		CHEBYSHEV_DISTANCE(
		pos.x, pos.y, device->earRight2.x, device->earRight2.y))
	{
		closestRightEar = device->earRight1;
	}
	else
	{
		closestRightEar = device->earRight2;
	}

	origin = CalcClosestPointOnLineSegmentToPoint(
		closestLeftEar, closestRightEar, pos);
	CalcChebyshevDistanceAndBearing(origin, pos, &distance, &bearing);
	SoundPlayAtPosition(&gSoundDevice, data, distance + plusDistance, bearing);
}

Mix_Chunk *StrSound(const char *s)
{
	if (s == NULL || strlen(s) == 0)
	{
		return NULL;
	}
	for (int i = 0; i < (int)gSoundDevice.customSounds.size; i++)
	{
		SoundData *sound = CArrayGet(&gSoundDevice.customSounds, i);
		if (strcmp(sound->Name, s) == 0)
		{
			return sound->data;
		}
	}
	for (int i = 0; i < (int)gSoundDevice.sounds.size; i++)
	{
		SoundData *sound = CArrayGet(&gSoundDevice.sounds, i);
		if (strcmp(sound->Name, s) == 0)
		{
			return sound->data;
		}
	}
	return NULL;
}

Mix_Chunk *SoundGetRandomScream(const SoundDevice *device)
{
	// Don't get the last scream used
	int idx = device->lastScream;
	while ((int)device->screamSounds.size > 1 && idx == device->lastScream)
	{
		idx = rand() % device->screamSounds.size;
	}
	Mix_Chunk **sound = CArrayGet(&device->screamSounds, idx);
	return *sound;
}
