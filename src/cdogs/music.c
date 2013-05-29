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

    Copyright (c) 2013, Cong Xu
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
#include <SDL_mixer.h>

#include "sounds.h"


int MusicPlay(const char *path)
{
	if (!gSoundDevice.isInitialised)
	{
		return 0;
	}

	debug(D_NORMAL, "Attempting to play song: %s\n", path);

	if (path == NULL || strlen(path) == 0)
	{
		debug(D_NORMAL, "Attempting to play song with empty name\n");
		return 1;
	}

	gSoundDevice.music = Mix_LoadMUS(path);
	if (gSoundDevice.music == NULL)
	{
		strcpy(gSoundDevice.musicErrorMessage, SDL_GetError());
		gSoundDevice.musicStatus = MUSIC_NOLOAD;
		return 1;
	}

	debug(D_NORMAL, "Playing song: %s\n", path);

	Mix_PlayMusic(gSoundDevice.music, -1);
	gSoundDevice.musicStatus = MUSIC_PLAYING;
	MusicSetVolume(gSoundDevice.musicVolume);

	return 0;
}

void MusicStop(void)
{
	if (gSoundDevice.music != NULL)
	{
		Mix_HaltMusic();
		Mix_FreeMusic(gSoundDevice.music);
		gSoundDevice.music = NULL;
	}
}

void MusicSetVolume(int volume)
{
	gSoundDevice.musicVolume = volume;
	if (!gSoundDevice.isInitialised)
	{
		return;
	}

	debug(D_NORMAL, "volume: %d\n", volume);

	Mix_VolumeMusic(gSoundDevice.musicVolume);
}

int MusicGetVolume(void)
{
	return gSoundDevice.musicVolume;
}

int MusicGetStatus(void)
{
	return gSoundDevice.musicStatus;
}

const char *MusicGetErrorMessage(void)
{
	return gSoundDevice.musicErrorMessage;
}
