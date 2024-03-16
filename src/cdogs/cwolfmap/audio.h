#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <SDL_audio.h>

#include "common.h"

#define MUSIC_SAMPLE_RATE 44100
#define MUSIC_AUDIO_FMT AUDIO_S16
#define MUSIC_AUDIO_CHANNELS 2

bool CWAudioInit(void);
void CWAudioTerminate(void);

int CWAudioLoadHead(CWAudioHead *head, const char *path);
void CWAudioHeadFree(CWAudioHead *head);

int CWAudioLoadAudioT(CWAudio *audio, const CWMapType type, const char *path);

void CWAudioFree(CWAudio *audio);

// http://www.vgmpf.com/Wiki/index.php?title=IMF
int CWAudioGetAdlibSoundRaw(
	const CWAudio *audio, const int i, const char **data, size_t *len);
int CWAudioGetAdlibSound(
	const CWAudio *audio, const int i, char **data, size_t *len);
int CWAudioGetMusicRaw(
	const CWAudio *audio, const int i, const char **data, size_t *len);
int CWAudioGetMusic(
	CWAudio *audio, const CWMapType type, const int idx, char **data,
	size_t *len);

typedef enum
{
	SONG_INTRO,
	SONG_MENU,
	SONG_END,
	SONG_ROSTER,
	SONG_VICTORY
} CWSongType;

int CWAudioGetLevelMusic(const CWMapType type, const int level);
int CWAudioGetSong(const CWMapType type, const CWSongType song);
