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

-------------------------------------------------------------------------------

 sounds.c - um... guess what?

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
static char moduleDirectory[128] = CDOGS_MUSIC_DIR;
SDL_AudioSpec *spec;


struct SndData {
	char name[81];
	int playPrio;
	int cmpPrio;
	int exists;
	int freq;
	int playFlag;
	int time;
	int panning;
	int volume;
	Uint32 size;
	int pos;
	Mix_Chunk *data;
};

struct SndData snd[SND_COUNT] =
{
	{"sounds/booom.wav",	20,	20,	0,	11025,	0,	82,		0,	0,	0,	0,	NULL},
	{"sounds/launch.wav",	8,	8,	0,	11025,	0,	27,		0,	0,	0,	0,	NULL},
	{"sounds/mg.wav",		10,	11,	0,	11025,	0,	36,		0,	0,	0,	0,	NULL},
	{"sounds/flamer.wav",	10,	10,	0,	11025,	0,	44,		0,	0,	0,	0,	NULL},
	{"sounds/shotgun.wav",	12,	13,	0,	11025,	0,	118,	0,	0,	0,	0,	NULL},
	{"sounds/fusion.wav",	12,	13,	0,	11025,	0,	90,		0,	0,	0,	0,	NULL},
	{"sounds/switch.wav",	18,	19,	0,	11025,	0,	36,		0,	0,	0,	0,	NULL},
	{"sounds/scream.wav",	15,	16,	0,	11025,	0,	70,		0,	0,	0,	0,	NULL},
	{"sounds/aargh1.wav",	15,	16,	0,	8060,	0,	79,		0,	0,	0,	0,	NULL},
	{"sounds/aargh2.wav",	15,	16,	0,	8060,	0,	66,		0,	0,	0,	0,	NULL},
	{"sounds/aargh3.wav",	15,	16,	0,	11110,	0,	67,		0,	0,	0,	0,	NULL},
	{"sounds/hahaha.wav",	22,	22,	0,	8060,	0,	58,		0,	0,	0,	0,	NULL},
	{"sounds/bang.wav",		14,	15,	0,	11025,	0,	60,		0,	0,	0,	0,	NULL},
	{"sounds/pickup.wav",	14,	15,	0,	11025,	0,	27,		0,	0,	0,	0,	NULL},
	{"sounds/click.wav",	4,	5,	0,	11025,	0,	11,		0,	0,	0,	0,	NULL},
	{"sounds/whistle.wav",	20,	20,	0,	11110,	0,	90,		0,	0,	0,	0,	NULL},
	{"sounds/powergun.wav",	13,	14,	0,	11110,	0,	90,		0,	0,	0,	0,	NULL},
	{"sounds/mg.wav",		10,	11,	0,	11025,	0,	36,		0,	0,	0,	0,	NULL}
};

static void loadSampleConfiguration(void)
{
	int i;
	FILE *f;


	f = fopen(GetConfigFilePath("sound_fx.cfg"), "r");
	if (!f)
		return;

	printf("Reading SOUND_FX.CFG\n");
	for (i = 0; i < SND_COUNT; i++) {
		int fscanfres;
		memset(snd[i].name, 0, sizeof(snd[i].name));
		fscanfres = fscanf(f, "%80s %d\n", snd[i].name, &snd[i].freq);
		if (fscanfres < 2) {
			printf("%2d. Error reading sound config\n", i);
			fclose(f);
			return;
		}
		printf("%2d. File:'%s' at %dHz\n", i, snd[i].name,
		       snd[i].freq);
	}
	fclose(f);
}

void ShutDownSound(void)
{
	if (!soundInitialized)
		return;

	debug(D_NORMAL, "shutting down sound\n");
	Mix_CloseAudio();
}

void DoSound(int i, int len, void *data)
{
	UNUSED(len);
	UNUSED(data);
	if (!soundInitialized)
		return;
	Mix_PlayChannel(-1, snd[i].data , 0);
}

void SoundCallback(void *userdata, Uint8 * stream, int len)
{
	int i;

	debug(D_VERBOSE, "sound callback(%p, %p, %d)\n", userdata, stream, len);

	memset(stream, 0, len);
	for (i = 0; i < SND_COUNT; i++) {
		if (snd[i].playFlag && snd[i].exists) {
			#if defined(SND_NOMIX) || defined(SND_SDLMIXER)
			DoSound(i, len, stream);
			return;
			#else /* newish version */
			DoSound(i, len/2, stream);
			#endif
		}
	}
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
	loadSampleConfiguration();
	for (i = 0; i < SND_COUNT; i++) {
		if (!snd[i].name[0] || snd[i].freq <= 0)
			snd[i].exists = 0;
		else {
			if (stat(GetDataFilePath(snd[i].name), &st) == -1)
				snd[i].exists = 0;
			else {
				snd[i].exists = 1;

				if ((snd[i].data = Mix_LoadWAV(
					GetDataFilePath(snd[i].name))) == NULL)
					snd[i].exists = 0;
				else
					snd[i].exists = 1;
			}
			if (!snd[i].exists)
				printf("Error loading sample '%s'\n",
				       GetDataFilePath(snd[i].name));
		}
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
int PlaySong(char *name) 
{
	if (!soundInitialized)
		return 0;

	debug(D_NORMAL, "Attempting to play song: %s\n", name);

	if (name) {
		struct stat s;
		char *p;
		char path[255];
	
		if (music != NULL) {
			Mix_HaltMusic();
			Mix_FreeMusic(music);
			music = NULL;
		}
	
		p = name;
	
		debug(D_NORMAL, "song path 1: %s\n", name);
		if (stat(name, &s) != 0) {
			strcpy(path, ModuleDirectory());
			strcat(path, "/");
			strcat(path, name);
			p = path;
		}
		
		debug(D_NORMAL, "song path 2: %s\n", p);
		if (stat(p, &s) != 0) {
			return 1;
		}
	
		music = Mix_LoadMUS(p);
		if (music == NULL) {
			SetModuleMessage(SDL_GetError());
			SetModuleStatus(MODULE_NOLOAD);
			return 1;
		}
	
		debug(D_NORMAL, "Playing song: %s\n", p);
	
		Mix_PlayMusic(music, -1);
		SetModuleStatus(MODULE_PLAYING);
		SetMusicVolume(musicVolume);
		return 0;
	}
	return 1;
}

void PlaySound(int sound, int panning, int volume)
{
	if (!soundInitialized)
		return;

	debug(D_VERBOSE, "sound: %d panning: %d volume: %d\n", sound, panning, volume);

	{
	int c;
	//Uint8 p;
	Uint8 left, right;

	if (panning == 0) {
		left = right = 255;
	} else {
		if (panning < 0) {
			left = (255 + panning);
		} else {
			left = panning;
		}

		right = 255 - left;
	}

	Mix_VolumeChunk(snd[sound].data,(volume * fxVolume) / 128 );
	c = Mix_PlayChannel(-1, snd[sound].data , 0);
	Mix_SetPanning(c, left, right);
	}
}

//TODO: Replace this with SDL callback
void DoSounds(void)
{
	return;
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

void PlaySoundAt(int x, int y, int sound)
{
	int d, dLeft, dRight;
	int leftVolume, rightVolume;
	int volume, panning;

	if (xLeft != xRight || yLeft != yRight) {
		dLeft = max(abs(x - xLeft), abs(y - yLeft));
		dRight = max(abs(x - xRight), abs(y - yRight));

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
	} else {
		d = max(abs(x - xLeft), abs(y - yLeft));
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

void SetModuleDirectory(const char *dir)
{
	strcpy(moduleDirectory, dir);
}

const char *ModuleDirectory(void)
{
	return moduleDirectory;
}
