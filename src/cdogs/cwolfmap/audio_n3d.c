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
	// E1
	INTRO_MUS, ALLGOOD_MUS, BOSS_MUS,
	// E2
	SONG3_MUS, INTRO_MUS, ALLGOOD_MUS, BOSS_MUS,
	// E3
	SONG4_MUS, SONG3_MUS, INTRO_MUS, BOSS_MUS, MENU_MUS,
	// E4
	SONG6_MUS, SONG4_MUS, ALLGOOD_MUS, SONG3_MUS, BOSS_MUS,
	// E5
	SONG8_MUS, INTRO_MUS, SONG6_MUS, SONG4_MUS, HAPPYSNG_MUS, BOSS_MUS,
	// E6
	HIGHSCORE_MUS, ALLGOOD_MUS, SONG3_MUS, SONG8_MUS, SONG6_MUS, BOSS_MUS,
	// Outro
	ALLGOOD_MUS};
int CWAudioN3DGetLevelMusic(const int level)
{
	return songs[level];
}

int CWAudioN3DGetSong(const CWSongType song)
{
	switch (song)
	{
	case SONG_INTRO:
		return INTRO_MUS;
	case SONG_MENU:
		return MENU_MUS;
	case SONG_END:
		return FEEDTIME_MUS;
	case SONG_ROSTER:
		return HIGHSCORE_MUS;
	case SONG_VICTORY:
		return ALLGOOD_MUS;
	}
	return -1;
}
