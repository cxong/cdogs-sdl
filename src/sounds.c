/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (C) 1995 Ronny Wester
    Copyright (C) 2003 Jeremy Chin 
    Copyright (C) 2003 Lucas Martin-King 

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
 
 Author: $Author$
 Rev:    $Revision$
 URL:    $HeadURL$
 ID:     $Id$
 
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "SDL.h"

#ifdef SND_SDLMIXER
	#include "SDL_mixer.h"
#endif

#include "sounds.h"
#include "files.h"

#include "utils.h"

#define max(a,b) ({__typeof__(a) __a = (a); __typeof__(b) __b = (b); (__a > __b) ? __a : __b;})

static int soundInitialized = 0;
static int fxVolume = 64;
static int musicVolume = 64;
static int noOfFXChannels = 4;
static int maxModChannels = 0;


static int channelPriority[FX_MAXCHANNELS];
static int channelPosition[FX_MAXCHANNELS];
static int channelTime[FX_MAXCHANNELS];
static int fxChannel;

static int interruptEnabled = 0;
static int dynamicInterrupts = 0;


static int moduleStatus = 0;
static char moduleMessage[128];
static char moduleDirectory[128] = "/tmp";
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
//      int len;
	int size;
	int pos;
#ifdef SND_SDLMIXER
	Mix_Chunk *data;
#else
	char *data;
#endif
};

struct SndData snd[SND_COUNT] = {
	{"sounds/booom.wav", 20, 20, 0, 11025, 0, 82, 0, 0, 0},
	{"sounds/launch.wav", 8, 8, 0, 11025, 0, 27, 0, 0, 0},
	{"sounds/mg.wav", 10, 11, 0, 11025, 0, 36, 0, 0, 0},
	{"sounds/flamer.wav", 10, 10, 0, 11025, 0, 44, 0, 0},
	{"sounds/shotgun.wav", 12, 13, 0, 11025, 0, 118, 0, 0},
	{"sounds/fusion.wav", 12, 13, 0, 11025, 0, 90, 0, 0},
	{"sounds/switch.wav", 18, 19, 0, 11025, 0, 36, 0, 0},
	{"sounds/scream.wav", 15, 16, 0, 11025, 0, 70, 0, 0},
	{"sounds/aargh1.wav", 15, 16, 0, 8060, 0, 79, 0, 0},
	{"sounds/aargh2.wav", 15, 16, 0, 8060, 0, 66, 0, 0},
	{"sounds/aargh3.wav", 15, 16, 0, 11110, 0, 67, 0, 0},
	{"sounds/hahaha.wav", 22, 22, 0, 8060, 0, 58, 0, 0},
	{"sounds/bang.wav", 14, 15, 0, 11025, 0, 60, 0, 0},
	{"sounds/pickup.wav", 14, 15, 0, 11025, 0, 27, 0, 0},
	{"sounds/click.wav", 4, 5, 0, 11025, 0, 11, 0, 0},
	{"sounds/whistle.wav", 20, 20, 0, 11110, 0, 90, 0, 0},
	{"sounds/powergun.wav", 13, 14, 0, 11110, 0, 90, 0, 0},
	{"sounds/mg.wav", 10, 11, 0, 11025, 0, 36, 0, 0}
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
		memset(snd[i].name, 0, sizeof(snd[i].name));
		fscanf(f, "%80s %d\n", snd[i].name, &snd[i].freq);
		printf("%2d. File:'%s' at %dHz\n", i, snd[i].name,
		       snd[i].freq);
	}
	fclose(f);
}

void shutDown(void)
{
	// Shutdown goes here...
	SDL_CloseAudio();
}

#ifndef SND_NOMIX /* oldschool version */
void DoSound(int i, int len, void *data)
{
	int j;
	void *newbuffer;
	
	if (!soundInitialized)
		return;
	
	newbuffer = sys_mem_alloc(len);
	//firstly, find if we're going to need to have a buffer with zeros
	if (len + snd[i].pos > snd[i].size) {
		memset(newbuffer, 0, len);
		memcpy(newbuffer, snd[i].data + snd[i].pos, snd[i].size - snd[i].pos);
		//snd[i].pos = 0;
		snd[i].playFlag = 0;
    // Replace this line with a version that mixes
			for (j=0;j<len;j++)
			((Sint16 *)data)[j]+=
				((((Uint8 *)newbuffer)[j]-128)*snd[i].volume)/8;
	} else {
		// Replaced with mixing version
		for (j=0;j<len;j++)
			((Sint16 *)data)[j]+=
				((((Uint8 *)snd[i].data)[j+snd[i].pos]-128)*snd[i].volume)/8;
		snd[i].pos += len;
	}

	free(newbuffer);
}
#elif defined(SND_SDLMIXER) /* sdl mixer version */
void DoSound(int i, int len, void *data)
{
	debug("snd: %d (sdl-mixer version)\n", i);

	if (!soundInitialized)
		return;
	Mix_PlayChannel(-1, snd[i].data , 0);
}
#else /* other mixing version */
void DoSound(int i, int len, void *data)
{
	int j;
	void *newbuffer;
	
	if (!soundInitialized)
		return 0;
	
	newbuffer = sys_mem_alloc(len);
	//firstly, find if we're going to need to have a buffer with zeros
	if (len + snd[i].pos > snd[i].size) {
		memset(newbuffer, 0, len);
		memcpy(newbuffer, snd[i].data + snd[i].pos,
		       snd[i].size - snd[i].pos);
//              snd[i].pos = 0;
		snd[i].playFlag = 0;
    // Replace this line with a version that mixes
		memcpy(data, newbuffer, len);
		
	} else {
		// Replaced with mixing version
		memcpy(data, snd[i].data + snd[i].pos, len);
		snd[i].pos += len;
	}

	free(newbuffer);
}
#endif /* SND_NOMIX */

void SoundCallback(void *userdata, Uint8 * stream, int len)
{
	int i;

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
	SDL_AudioSpec tmpspec;

	// Initialization goes here...

	#ifdef SND_NOMIX
	debug("Old sound code enabled. =/\n");
	#endif
	#ifdef SND_SDLMIXER
	debug("Using SDL mixer...\n");
	#else
	debug("Using newish sound code..\n");
	#endif

	#ifdef SND_SDLMIXER
	if (Mix_OpenAudio(22050, AUDIO_S16, 2, 512) != 0) {
		printf("Couldn't open audio!: %s\n", SDL_GetError());
		return 0;
	} 
	Mix_AllocateChannels(noOfFXChannels);
	#endif

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

#ifdef SND_SDLMIXER
				if ((snd[i].data = Mix_LoadWAV(
					GetDataFilePath(snd[i].name))) == NULL)
#else
				if (SDL_LoadWAV
				    (GetDataFilePath(snd[i].name), &tmpspec, &snd[i].data,
				     &snd[i].size) == NULL)
#endif
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

	tmpspec.samples = 512;
	tmpspec.callback = &SoundCallback;

	#ifndef SND_SDLMIXER
	tmpspec.channels = 1;
	tmpspec.format = AUDIO_S16;
	#endif

	#ifdef SND_NOMIX
	tmpspec.samples = 128;
	tmpspec.format = AUDIO_U8;
	#endif

	#ifndef SND_SDLMIXER
	if (SDL_OpenAudio(&tmpspec, NULL) == -1) {
		printf("Couldn't open audio: %s\n", SDL_GetError());
		return 0;
	}
	SDL_PauseAudio(0);
	#endif

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

#ifdef SND_SDLMIXER
Mix_Music *music = NULL;
int PlaySong(char *name)
{
	if (!soundInitialized)
		return 0;

	debug("Attempting to play song: %s\n", name);

	if (name) {
		struct stat s;
		char *p;
	
		if (music != NULL) {
			Mix_HaltMusic();
			Mix_FreeMusic(music);
			music = NULL;
		}
	
		p = name;
	
		if (stat(name, &s) != 0) {
			char path[255];
			strcpy(path, ModuleDirectory());
			strcat(path, "/");
			strcat(path, name);
			p = path;
		}
		
		if (stat(p, &s) != 0) {
			return 1;
		}
	
		music = Mix_LoadMUS(p);
		if (music == NULL) {
			SetModuleMessage(SDL_GetError());
			SetModuleStatus(MODULE_NOLOAD);
			return 1;
		}
	
		debug("Playing song: %s\n", p);
	
		Mix_PlayMusic(music, -1);
		SetModuleStatus(MODULE_PLAYING);
		SetMusicVolume(musicVolume);
		return 0;
	}
	return 1;
}
#else
int PlaySong(char *name)
{
	return;
}
#endif

void PlaySound(int sound, int panning, int volume)
{
	if (!soundInitialized)
		return;

	debug("sound: %d panning: %d volume: %d\n", sound, panning, volume);

#ifdef SND_SDLMIXER
	Mix_VolumeChunk(snd[sound].data,(volume * fxVolume) / 128 );
	Mix_PlayChannel(-1, snd[sound].data , 0);
#else
	snd[sound].playFlag = 1;
	snd[sound].panning = panning;
	snd[sound].volume = (volume * fxVolume) / 256;
	snd[sound].pos = 0;
#endif
}

//TODO: Replace this with SDL callback
void DoSounds(void)
{
	return;
}

void SetFXVolume(int volume)
{
	debug("volume: %d\n", volume);

	fxVolume = volume;

	if (!soundInitialized)
		return;

#ifdef SND_SDLMIXER
	Mix_Volume(-1, fxVolume);
#endif
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

	debug("volume: %d\n", volume);

#ifdef SND_SDLMIXER
	Mix_VolumeMusic(musicVolume);
#endif

	// Set music volume
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
	if (!soundInitialized)
		return;

#ifdef SND_SDLMIXER
	int status = ModuleStatus();

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

#endif	
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
//      strcpy(moduleDirectory, dir);
}

const char *ModuleDirectory(void)
{
	return moduleDirectory;
}
