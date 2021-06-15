#include "audio_wl6.h"

#include "audiowl6.h"

void CWAudioWL6LoadAudioT(CWAudio *audio)
{
	audio->nSound = LASTSOUND;
	audio->nMusic = LASTMUSIC;
	audio->startAdlibSounds = STARTADLIBSOUNDS;
	audio->startMusic = STARTMUSIC;
}
static const musicnames songs[] = {
	//
	// Episode One
	//
	GETTHEM_MUS, SEARCHN_MUS, POW_MUS, SUSPENSE_MUS, GETTHEM_MUS, SEARCHN_MUS,
	POW_MUS, SUSPENSE_MUS,

	WARMARCH_MUS, // Boss level
	CORNER_MUS,	  // Secret level

	//
	// Episode Two
	//
	NAZI_OMI_MUS, PREGNANT_MUS, GOINGAFT_MUS, HEADACHE_MUS, NAZI_OMI_MUS,
	PREGNANT_MUS, HEADACHE_MUS, GOINGAFT_MUS,

	WARMARCH_MUS, // Boss level
	DUNGEON_MUS,  // Secret level

	//
	// Episode Three
	//
	INTROCW3_MUS, NAZI_RAP_MUS, TWELFTH_MUS, ZEROHOUR_MUS, INTROCW3_MUS,
	NAZI_RAP_MUS, TWELFTH_MUS, ZEROHOUR_MUS,

	ULTIMATE_MUS, // Boss level
	PACMAN_MUS,	  // Secret level

	//
	// Episode Four
	//
	GETTHEM_MUS, SEARCHN_MUS, POW_MUS, SUSPENSE_MUS, GETTHEM_MUS, SEARCHN_MUS,
	POW_MUS, SUSPENSE_MUS,

	WARMARCH_MUS, // Boss level
	CORNER_MUS,	  // Secret level

	//
	// Episode Five
	//
	NAZI_OMI_MUS, PREGNANT_MUS, GOINGAFT_MUS, HEADACHE_MUS, NAZI_OMI_MUS,
	PREGNANT_MUS, HEADACHE_MUS, GOINGAFT_MUS,

	WARMARCH_MUS, // Boss level
	DUNGEON_MUS,  // Secret level

	//
	// Episode Six
	//
	INTROCW3_MUS, NAZI_RAP_MUS, TWELFTH_MUS, ZEROHOUR_MUS, INTROCW3_MUS,
	NAZI_RAP_MUS, TWELFTH_MUS, ZEROHOUR_MUS,

	ULTIMATE_MUS, // Boss level
	FUNKYOU_MUS	  // Secret level
};
int CWAudioWL6GetLevelMusic(const int level)
{
	return songs[level];
}

int CWAudioWL6GetSong(const CWSongType song)
{
	switch (song)
	{
	case SONG_INTRO:
		return NAZI_NOR_MUS;
	case SONG_MENU:
		return WONDERIN_MUS;
	case SONG_END:
		return ENDLEVEL_MUS;
	case SONG_ROSTER:
		return ROSTER_MUS;
	case SONG_VICTORY:
		return URAHERO_MUS;
	}
	return -1;
}
