#include "audio_sod.h"

#include "AUDIOSOD.H"

void CWAudioSODLoadAudioT(CWAudio *audio)
{
	audio->nSound = LASTSOUND;
	audio->nMusic = LASTMUSIC;
	audio->startAdlibSounds = STARTADLIBSOUNDS;
	audio->startMusic = STARTMUSIC;
}
static const musicnames songs[] = {
	XTIPTOE_MUS,  XFUNKIE_MUS,	XDEATH_MUS,
	XGETYOU_MUS,  // DON'T KNOW
	ULTIMATE_MUS, // Trans Gr”sse

	DUNGEON_MUS,  GOINGAFT_MUS, POW_MUS,	  TWELFTH_MUS,
	ULTIMATE_MUS, // Barnacle Wilhelm BOSS

	NAZI_OMI_MUS, GETTHEM_MUS,	SUSPENSE_MUS, SEARCHN_MUS, ZEROHOUR_MUS,
	ULTIMATE_MUS, // Super Mutant BOSS

	XPUTIT_MUS,
	ULTIMATE_MUS, // Death Knight BOSS

	XJAZNAZI_MUS, // Secret level
	XFUNKIE_MUS,  // Secret level (DON'T KNOW)

	XEVIL_MUS // Angel of Death BOSS
};
int CWAudioSODGetLevelMusic(const int level)
{
	return songs[level];
}

int CWAudioSODGetSong(const CWSongType song)
{
	switch (song)
	{
	case SONG_INTRO:
		return XTOWER2_MUS;
	case SONG_MENU:
		return WONDERIN_MUS;
	case SONG_END:
		return ENDLEVEL_MUS;
	case SONG_ROSTER:
		return XAWARD_MUS;
	case SONG_VICTORY:
		return URAHERO_MUS;
	}
	return -1;
}
