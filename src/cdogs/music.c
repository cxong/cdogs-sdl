/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.

	Copyright (c) 2013-2016, 2019, 2021 Cong Xu
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

#include <tinydir/tinydir.h>

#include "config.h"
#include "gamedata.h"
#include "log.h"

static void LoadMusic(CArray *tracks, const char *path);
void MusicPlayerInit(MusicPlayer *mp)
{
	memset(mp, 0, sizeof *mp);

	// Load music
	LoadMusic(&mp->generalTracks[MUSIC_MENU], "music/menu");
	LoadMusic(&mp->generalTracks[MUSIC_BRIEFING], "music/briefing");
	LoadMusic(&mp->generalTracks[MUSIC_GAME], "music/game");
}
static void LoadMusic(CArray *tracks, const char *path)
{
	CArrayInit(tracks, sizeof(Mix_Music *));
	tinydir_dir dir;
	char buf[CDOGS_PATH_MAX];
	GetDataFilePath(buf, path);
	if (tinydir_open(&dir, buf) == -1)
	{
		LOG(LM_MAIN, LL_ERROR, "Cannot open music dir %s: %s", buf,
			strerror(errno));
		return;
	}

	for (; dir.has_next; tinydir_next(&dir))
	{
		Mix_Music *m;
		tinydir_file file;
		if (tinydir_readfile(&dir, &file) == -1)
		{
			goto bail;
		}
		if (!file.is_reg)
		{
			continue;
		}

		m = MusicLoad(file.path);
		if (m == NULL)
		{
			continue;
		}
		CArrayPushBack(tracks, &m);
	}

bail:
	tinydir_close(&dir);
}

static void UnloadMusic(CArray *tracks);
void MusicPlayerTerminate(MusicPlayer *mp)
{
	for (MusicType type = MUSIC_MENU; type < MUSIC_COUNT; type++)
	{
		UnloadMusic(&mp->generalTracks[type]);
	}
}
static void UnloadMusic(CArray *tracks)
{
	CA_FOREACH(Mix_Music *, m, *tracks)
	Mix_FreeMusic(*m);
	CA_FOREACH_END()
	CArrayTerminate(tracks);
}

Mix_Music *MusicLoad(const char *path)
{
	// Only load music from known extensions
	const char *ext = strrchr(path, '.');
	if (ext == NULL ||
		!(strcmp(ext, ".it") == 0 || strcmp(ext, ".IT") == 0 ||
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

static void PlayMusic(MusicPlayer *mp)
{
	MusicResume(mp);

	if (ConfigGetInt(&gConfig, "Sound.MusicVolume") == 0)
	{
		MusicPause(mp);
	}
}

static bool Play(MusicPlayer *mp, const char *path)
{
	if (!mp->isInitialised)
	{
		return true;
	}

	if (path == NULL || strlen(path) == 0)
	{
		LOG(LM_SOUND, LL_WARN, "Attempting to play song with empty name");
		return false;
	}

	mp->type = MUSIC_SRC_DYNAMIC;
	mp->u.dynamic = MusicLoad(path);
	if (mp->u.dynamic == NULL)
	{
		strcpy(mp->errorMessage, SDL_GetError());
		return false;
	}
	mp->errorMessage[0] = '\0';
	PlayMusic(mp);
	return true;
}

void MusicPlayGeneral(MusicPlayer *mp, const MusicType type)
{
	mp->type = MUSIC_SRC_GENERAL;
	CArray *tracks = &mp->generalTracks[type];
	if (tracks->size == 0)
	{
		return;
	}
	mp->u.general = *(Mix_Music **)CArrayGet(tracks, 0);
	// Shuffle tracks
	if (tracks->size > 1)
	{
		while (mp->u.general == *(Mix_Music **)CArrayGet(tracks, 0))
		{
			CArrayShuffle(tracks);
		}
	}
	PlayMusic(mp);
}
void MusicPlayFile(
	MusicPlayer *mp, const MusicType type, const char *missionPath,
	const char *music)
{
	MusicStop(mp);
	bool played = false;
	if (music != NULL && strlen(music) != 0)
	{
		char buf[CDOGS_PATH_MAX];
		// First, try to play music from the same directory
		// This may be a new-style directory campaign
		GetDataFilePath(buf, missionPath);
		strcat(buf, "/");
		strcat(buf, music);
		played = Play(mp, buf);
		if (!played)
		{
			char buf2[CDOGS_PATH_MAX];
			GetDataFilePath(buf2, missionPath);
			PathGetDirname(buf, buf2);
			strcat(buf, music);
			played = Play(mp, buf);
		}
	}
	if (!played)
	{
		MusicPlayGeneral(mp, type);
	}
}
void MusicPlayChunk(MusicPlayer *mp, const MusicType type, Mix_Chunk *chunk)
{
	MusicStop(mp);
	bool played = false;
	if (chunk != NULL)
	{
		mp->type = MUSIC_SRC_CHUNK;
		mp->u.chunk.chunk = chunk;
		mp->u.chunk.channel = Mix_PlayChannel(-1, chunk, -1);
		played = true;
	}
	if (!played)
	{
		MusicPlayGeneral(mp, type);
	}
}

void MusicStop(MusicPlayer *mp)
{
	switch (mp->type)
	{
	case MUSIC_SRC_GENERAL:
		Mix_HaltMusic();
		mp->u.general = NULL;
		break;
	case MUSIC_SRC_DYNAMIC:
		Mix_HaltMusic();
		if (mp->u.dynamic != NULL)
		{
			Mix_FreeMusic(mp->u.dynamic);
		}
		mp->u.dynamic = NULL;
		break;
	case MUSIC_SRC_CHUNK:
		if (mp->u.chunk.chunk != NULL)
		{
			Mix_HaltChannel(mp->u.chunk.channel);
		}
		mp->u.chunk.chunk = NULL;
		break;
	}
}

void MusicPause(MusicPlayer *mp)
{
	switch (mp->type)
	{
	case MUSIC_SRC_GENERAL:
	case MUSIC_SRC_DYNAMIC:
		if (Mix_PlayingMusic())
		{
			Mix_PauseMusic();
		}
		break;
	case MUSIC_SRC_CHUNK:
		Mix_Pause(mp->u.chunk.channel);
		break;
	}
	
}

void MusicResume(MusicPlayer *mp)
{
	switch (mp->type)
	{
	case MUSIC_SRC_GENERAL:
		if (mp->u.general == NULL)
		{
			return;
		}
		if (Mix_PausedMusic())
		{
			Mix_ResumeMusic();
		}
		else if (!Mix_PlayingMusic())
		{
			Mix_PlayMusic(mp->u.general, -1);
		}
		break;
	case MUSIC_SRC_DYNAMIC:
		if (mp->u.dynamic == NULL)
		{
			return;
		}
		if (Mix_PausedMusic())
		{
			Mix_ResumeMusic();
		}
		else if (!Mix_PlayingMusic())
		{
			Mix_PlayMusic(mp->u.dynamic, -1);
		}
		break;
	case MUSIC_SRC_CHUNK:
		Mix_Resume(mp->u.chunk.channel);
		break;
	}
}

void MusicSetPlaying(MusicPlayer *mp, const bool isPlaying)
{
	if (isPlaying)
	{
		MusicResume(mp);
	}
	else
	{
		MusicPause(mp);
	}
}

const char *MusicGetErrorMessage(const MusicPlayer *mp)
{
	return mp->errorMessage;
}
