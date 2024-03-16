#include "audio_n3d.h"

#include "audion3d.h"

void CWAudioN3DLoadAudioT(CWAudio *audio)
{
	audio->nSound = LASTSOUND;
	audio->nMusic = LASTMUSIC;
	audio->startAdlibSounds = STARTADLIBSOUNDS;
	audio->startMusic = STARTMUSIC;
}
int CWAudioN3DLoadAudioWAD(CWAudio *audio, const char *path)
{
	int err = 0;
	audio->wad = WAD_Open(path);
	if (audio->wad == NULL)
	{
		err = -1;
		fprintf(stderr, "Failed to read %s\n", path);
		goto bail;
	}

bail:
	return err;
}
static const int16_t songs[] = {
	SONG1_MUS, ALLGOOD_MUS, SONG7_MUS,	  SONG3_MUS, SONG1_MUS,	 ALLGOOD_MUS,
	SONG7_MUS, SONG4_MUS,	SONG3_MUS,	  SONG1_MUS, SONG7_MUS,	 SONG6_MUS,
	SONG4_MUS, ALLGOOD_MUS, SONG3_MUS,	  SONG7_MUS, SONG8_MUS,	 SONG1_MUS,
	SONG6_MUS, SONG4_MUS,	HAPPYSNG_MUS, SONG7_MUS, SONG11_MUS, ALLGOOD_MUS,
	SONG3_MUS, SONG8_MUS,	SONG6_MUS,	  SONG7_MUS, SONG9_MUS,	 ALLGOOD_MUS};
int CWAudioN3DGetLevelMusic(const int level)
{
	return songs[level];
}

int CWAudioN3DGetSong(const CWSongType song)
{
	switch (song)
	{
	case SONG_INTRO:
		return SONG1_MUS;
	case SONG_MENU:
		return SONG9_MUS;
	case SONG_END:
		return FEEDTIME_MUS;
	case SONG_ROSTER:
		return SONG11_MUS;
	case SONG_VICTORY:
		return ALLGOOD_MUS;
	}
	return -1;
}
