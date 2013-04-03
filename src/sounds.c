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
*/
#include "sounds.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <SDL.h>
#include <SDL_mixer.h>

#include "files.h"
#include "utils.h"

static int soundInitialized = 0;
static int fxVolume = 64;
static int musicVolume = 64;
static int noOfFXChannels = 8;
static int maxModChannels = 0;


static int channelPriority[FX_MAXCHANNELS];
static int channelPosition[FX_MAXCHANNELS];
static int channelTime[FX_MAXCHANNELS];


static int moduleStatus = 0;
static char moduleMessage[128];
SDL_AudioSpec *spec;


struct SndData {
	char name[81];
	int exists;
	Mix_Chunk *data;
};

struct SndData snd[SND_COUNT] =
{
	{"sounds/booom.wav",	0,	NULL},
	{"sounds/launch.wav",	0,	NULL},
	{"sounds/mg.wav",		0,	NULL},
	{"sounds/flamer.wav",	0,	NULL},
	{"sounds/shotgun.wav",	0,	NULL},
	{"sounds/fusion.wav",	0,	NULL},
	{"sounds/switch.wav",	0,	NULL},
	{"sounds/scream.wav",	0,	NULL},
	{"sounds/aargh1.wav",	0,	NULL},
	{"sounds/aargh2.wav",	0,	NULL},
	{"sounds/aargh3.wav",	0,	NULL},
	{"sounds/hahaha.wav",	0,	NULL},
	{"sounds/bang.wav",		0,	NULL},
	{"sounds/pickup.wav",	0,	NULL},
	{"sounds/click.wav",	0,	NULL},
	{"sounds/whistle.wav",	0,	NULL},
	{"sounds/powergun.wav",	0,	NULL},
	{"sounds/mg.wav",		0,	NULL}
};


void ShutDownSound(void)
{
	if (!soundInitialized)
		return;

	debug(D_NORMAL, "shutting down sound\n");
	Mix_CloseAudio();
}

int InitSoundDevice(void)
{
	int i;
	struct stat st;

	if (Mix_OpenAudio(22050, AUDIO_S16, 2, 512) != 0)
	{
		printf("Couldn't open audio!: %s\n", SDL_GetError());
		return 0;
	}

	{
		int f;
		Uint16 fmt;
		int c;

		Mix_QuerySpec(&f, &fmt, &c);

		debug(D_NORMAL, "spec: f=%d fmt=%d c=%d\n", f, fmt, c);

		if (f != 22050 || fmt != AUDIO_S16 || c != 2) {
			printf("Audio not what we want.\n");
			return 0;
		}
	}

	if (Mix_AllocateChannels(noOfFXChannels) != noOfFXChannels) {
		printf("Couldn't allocate channels!\n");
		return 0;
	}

	// C-Dogs internals:
	for (i = 0; i < SND_COUNT; i++)
	{
		snd[i].exists = 0;

		if (stat(GetDataFilePath(snd[i].name), &st) == -1)
		{
			printf("Error finding sample '%s'\n",
				GetDataFilePath(snd[i].name));
			continue;
		}

		if ((snd[i].data = Mix_LoadWAV(
			GetDataFilePath(snd[i].name))) == NULL)
		{
			printf("Error loading sample '%s'\n",
				GetDataFilePath(snd[i].name));
			continue;
		}

		snd[i].exists = 1;
	}

	memset(channelPriority, 0, sizeof(channelPriority));
	memset(channelPosition, 0, sizeof(channelPosition));
	memset(channelTime, 0, sizeof(channelTime));

	soundInitialized = 1;

	return 1;
}

int InitializeSound(void)
{
	// load and init sound device
	if (!InitSoundDevice()) {
		printf("Unable to initalize sound device\n");
		return 0;
	}

	return 1;
}

Mix_Music *music = NULL;
int PlaySong(const char *path)
{
	if (!soundInitialized)
		return 0;

	debug(D_NORMAL, "Attempting to play song: %s\n", path);

	if (path == NULL || strlen(path) == 0)
	{
		debug(D_NORMAL, "Attempting to play song with empty name\n");
		return 1;
	}

	music = Mix_LoadMUS(path);
	if (music == NULL)
	{
		SetModuleMessage(SDL_GetError());
		SetModuleStatus(MODULE_NOLOAD);
		return 1;
	}

	debug(D_NORMAL, "Playing song: %s\n", path);

	Mix_PlayMusic(music, -1);
	SetModuleStatus(MODULE_PLAYING);
	SetMusicVolume(musicVolume);

	return 0;
}

void StopSong(void)
{
	if (music != NULL)
	{
		Mix_HaltMusic();
		Mix_FreeMusic(music);
		music = NULL;
	}
}

void CalcLeftRightVolumeFromPanning(Uint8 *left, Uint8 *right, int panning)
{
	if (panning == 0)
	{
		*left = *right = 255;
	}
	else
	{
		if (panning < 0)
		{
			*left = (unsigned char)(255 + panning);
		}
		else
		{
			*left = (unsigned char)(panning);
		}

		*right = 255 - *left;
	}
}

void PlaySound(sound_e sound, int panning, int volume)
{
	if (!soundInitialized)
	{
		return;
	}

	debug(D_VERBOSE, "sound: %d panning: %d volume: %d\n", sound, panning, volume);

	{
	int c;
	Uint8 left, right;
	CalcLeftRightVolumeFromPanning(&left, &right, panning);

	Mix_VolumeChunk(snd[sound].data,(volume * fxVolume) / 128 );
	c = Mix_PlayChannel(-1, snd[sound].data , 0);
	Mix_SetPanning(c, left, right);
	}
}

void SetFXVolume(int volume)
{
	debug(D_NORMAL, "volume: %d\n", volume);

	fxVolume = volume;

	if (!soundInitialized)
		return;

	Mix_Volume(-1, fxVolume);
}

int FXVolume(void)
{
	return fxVolume;
}

void SetMusicVolume(int volume)
{
	musicVolume = volume;
	if (!soundInitialized)
		return;

	debug(D_NORMAL, "volume: %d\n", volume);

	Mix_VolumeMusic(musicVolume);
}

int MusicVolume(void)
{
	return musicVolume;
}

#define RANGE_FULLVOLUME    70
#define RANGE_FACTOR       128


static int xLeft, yLeft;
static int xRight, yRight;


void SetLeftEar(int x, int y)
{
	xLeft = x;
	yLeft = y;
}

void SetRightEar(int x, int y)
{
	xRight = x;
	yRight = y;
}

void PlaySoundAt(int x, int y, sound_e sound)
{
	int d, dLeft, dRight;
	int leftVolume, rightVolume;
	int volume, panning;

	d = AXIS_DISTANCE(x, y, xLeft, yLeft);
	if (xLeft != xRight || yLeft != yRight)
	{
		dLeft = d;
		dRight = AXIS_DISTANCE(x, y, xRight, yRight);

		d = (dLeft >
		     RANGE_FULLVOLUME ? dLeft - RANGE_FULLVOLUME : 0);
		leftVolume = 255 - (RANGE_FACTOR * d) / 256;
		if (leftVolume < 0)
			leftVolume = 0;

		d = (dRight >
		     RANGE_FULLVOLUME ? dRight - RANGE_FULLVOLUME : 0);
		rightVolume = 255 - (RANGE_FACTOR * d) / 256;
		if (rightVolume < 0)
			rightVolume = 0;

		volume = leftVolume + rightVolume;
		if (volume > 256)
			volume = 256;

		panning = rightVolume - leftVolume;
		panning /= 4;
	}
	else
	{
		d -= d / 4;
		d = (d > RANGE_FULLVOLUME ? d - RANGE_FULLVOLUME : 0);
		volume = 255 - (RANGE_FACTOR * d) / 256;
		if (volume < 0)
			volume = 0;

		panning = (x - xLeft) / 4;
	}

	if (volume > 0)
		PlaySound(sound, panning, volume);
}

void SetFXChannels(int channels)
{
	if (channels >= 2 && channels <= FX_MAXCHANNELS)
		noOfFXChannels = channels;
}

int FXChannels(void)
{
	return noOfFXChannels;
}

void SetMinMusicChannels(int channels)
{
	if (channels >= 0 && channels <= 32)
		maxModChannels = channels;
}

int MinMusicChannels(void)
{
	return maxModChannels;
}

// toggle music track on/off
void ToggleTrack(int track)
{
	int status;

	UNUSED(track); /* may be unimplemented */

	if (!soundInitialized)
		return;

	status = ModuleStatus();

	switch (status) {
		case MODULE_PLAYING:
			Mix_PauseMusic();
			SetModuleStatus(MODULE_PAUSED);
		break;

		case MODULE_PAUSED:
			Mix_ResumeMusic();
			SetModuleStatus(MODULE_PLAYING);
		break;

		case MODULE_STOPPED:
			Mix_PlayMusic(music, 0);
			SetModuleStatus(MODULE_PLAYING);
		break;
	}
}

void SetModuleStatus(int s)
{
	moduleStatus = s;
}

int ModuleStatus(void)
{
	return moduleStatus;
}

void SetModuleMessage(const char *s)
{
	strncpy(moduleMessage, s, sizeof(moduleMessage) - 1);
}

const char *ModuleMessage(void)
{
	return moduleMessage;
}
