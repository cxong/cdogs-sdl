/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.

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
#include "music.h"

#include <string.h>

#include <SDL.h>
#ifdef __EMSCRIPTEN__
#include <SDL2/SDL_mixer.h>
#else
#include <SDL_mixer.h>
#endif

#include "config.h"
#include "gamedata.h"
#include "log.h"
#include "sounds.h"


Mix_Music *MusicLoad(const char *path)
{
	// Only load music from known extensions
	const char *ext = strrchr(path, '.');
	if (ext == NULL || !(
		strcmp(ext, ".it") == 0 || strcmp(ext, ".IT") == 0 ||
		strcmp(ext, ".mod") == 0 || strcmp(ext, ".MOD") == 0 ||
		strcmp(ext, ".ogg") == 0 || strcmp(ext, ".OGG") == 0 ||
		strcmp(ext, ".s3m") == 0 || strcmp(ext, ".S3M") == 0 ||
		strcmp(ext, ".xm") == 0 || strcmp(ext, ".XM") == 0))
	{
		return NULL;
	}
	LOG(LM_MAIN, LL_TRACE, "loading music file %s", path);
	return Mix_LoadMUS(path);
}

static bool MusicPlay(SoundDevice *device, const char *path)
{
	if (!device->isInitialised)
	{
		return true;
	}

	if (path == NULL || strlen(path) == 0)
	{
		LOG(LM_SOUND, LL_WARN, "Attempting to play song with empty name");
		return false;
	}

	device->music = MusicLoad(path);
	if (device->music == NULL)
	{
		strcpy(device->musicErrorMessage, SDL_GetError());
		device->musicStatus = MUSIC_NOLOAD;
		return false;
	}

	Mix_PlayMusic(device->music, -1);
	device->musicStatus = MUSIC_PLAYING;

	if (ConfigGetInt(&gConfig, "Sound.MusicVolume") == 0)
	{
		MusicPause(device);
	}

	device->musicErrorMessage[0] = '\0';

	return true;
}

void MusicPlayGame(
	SoundDevice *device, const char *missionPath, const char *music)
{
	// Play a tune
	// Start by trying to play a mission specific song,
	// otherwise pick one from the general collection...
	MusicStop(device);
	bool played = false;
	if (music != NULL && strlen(music) != 0)
	{
		char buf[CDOGS_PATH_MAX];
		// First, try to play music from the same directory
		// This may be a new-style directory campaign
		GetDataFilePath(buf, missionPath);
		strcat(buf, "/");
		strcat(buf, music);
		played = MusicPlay(device, buf);
		if (!played)
		{
			char buf2[CDOGS_PATH_MAX];
			GetDataFilePath(buf2, missionPath);
			PathGetDirname(buf, buf2);
			strcat(buf, music);
			played = MusicPlay(device, buf);
		}
	}
	if (!played && gGameSongs != NULL)
	{
		MusicPlay(device, gGameSongs->path);
		ShiftSongs(&gGameSongs);
	}
}
void MusicPlayMenu(SoundDevice *device)
{
	MusicStop(device);
	if (gMenuSongs)
	{
		MusicPlay(device, gMenuSongs->path);
		ShiftSongs(&gMenuSongs);
	}
}

void MusicStop(SoundDevice *device)
{
	if (device->music != NULL)
	{
		Mix_HaltMusic();
		Mix_FreeMusic(device->music);
		device->music = NULL;
	}
}

void MusicPause(SoundDevice *device)
{
	if (device->musicStatus == MUSIC_PLAYING)
	{
		Mix_PauseMusic();
		device->musicStatus = MUSIC_PAUSED;
	}
}

void MusicResume(SoundDevice *device)
{
	if (device->musicStatus == MUSIC_PAUSED)
	{
		Mix_ResumeMusic();
		device->musicStatus = MUSIC_PLAYING;
	}
}

void MusicSetPlaying(SoundDevice *device, int isPlaying)
{
	if (isPlaying)
	{
		MusicResume(device);
	}
	else
	{
		MusicPause(device);
	}
}

int MusicGetStatus(SoundDevice *device)
{
	return device->musicStatus;
}

const char *MusicGetErrorMessage(SoundDevice *device)
{
	return device->musicErrorMessage;
}
