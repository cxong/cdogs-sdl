// Contains code from bstone
// Copyright (c) 1992-2013 Apogee Entertainment, LLC
// Copyright(c) 2013 - 2022 Boris I.Bendovsky(bibendovsky @hotmail.com) and
// Contributors SPDX - License - Identifier : GPL - 2.0 - or -later
#include "audio_bs6.h"

#include "audiobs6.h"

void CWAudioBS6LoadAudioT(CWAudio *audio)
{
	/*
	if (assets_info.is_aog())
	{
		THEME_MUS = 16;
		LASTMUSIC = 19;
	}
	else
	{
		THEME_MUS = 20;
		LASTMUSIC = 21;
	}
	*/
	audio->nSound = LASTSOUND;
	audio->nMusic = LASTMUSIC;
	audio->startAdlibSounds = STARTADLIBSOUNDS;
	audio->startMusic = STARTMUSIC;
}
// https://github.com/bibendovsky/bstone/blob/46c9b7b146e57321fad87e1b3985a3d3e829bae7/src/3d_play.cpp#L204
// TODO: PS songs
static const int16_t songs[] = {
	// Episode 1
	INCNRATN_MUS, // secret1
	DRKHALLA_MUS,
	JUNGLEA_MUS,
	RACSHUFL_MUS,
	DRKHALLA_MUS,
	HIDINGA_MUS,
	JUNGLEA_MUS,
	RACSHUFL_MUS,
	HIDINGA_MUS,
	DRKHALLA_MUS,
	INCNRATN_MUS, // secret2

	// Episode 2
	FREEDOMA_MUS,
	DRKHALLA_MUS,
	STRUTA_MUS,
	INTRIGEA_MUS,
	MEETINGA_MUS,
	DRKHALLA_MUS,
	INCNRATN_MUS,
	RACSHUFL_MUS,
	JUNGLEA_MUS,
	GENEFUNK_MUS,
	THEME_MUS,

	// Episode 3
	LEVELA_MUS,
	HIDINGA_MUS,
	STRUTA_MUS,
	THEME_MUS,
	RACSHUFL_MUS,
	INCNRATN_MUS,
	GOLDA_MUS,
	JUNGLEA_MUS,
	DRKHALLA_MUS,
	THEWAYA_MUS,
	FREEDOMA_MUS,

	// Episode 4
	HIDINGA_MUS,
	DRKHALLA_MUS,
	GENEFUNK_MUS,
	JUNGLEA_MUS,
	INCNRATN_MUS,
	GOLDA_MUS,
	HIDINGA_MUS,
	JUNGLEA_MUS,
	DRKHALLA_MUS,
	THEWAYA_MUS,
	RUMBAA_MUS,

	// Episode 5
	RACSHUFL_MUS,
	SEARCHNA_MUS,
	JUNGLEA_MUS,
	HIDINGA_MUS,
	GENEFUNK_MUS,
	MEETINGA_MUS,
	S2100A_MUS,
	THEME_MUS,
	INCNRATN_MUS,
	DRKHALLA_MUS,
	THEWAYA_MUS,

	// Episode 6
	TIMEA_MUS,
	RACSHUFL_MUS,
	GENEFUNK_MUS,
	HIDINGA_MUS,
	S2100A_MUS,
	THEME_MUS,
	THEWAYA_MUS,
	JUNGLEA_MUS,
	MEETINGA_MUS,
	DRKHALLA_MUS,
	INCNRATN_MUS,
};
int CWAudioBS6GetLevelMusic(const int level)
{
	return songs[level];
}

int CWAudioBS6GetSong(const CWSongType song)
{
	/*
	if (assets_info.is_ps())
	{
		MENUSONG = LASTLAFF_MUS;
		ROSTER_MUS = HISCORE_MUS;
		TEXTSONG = TOHELL_MUS;
		TITLE_LOOP_MUSIC = PLOT_MUS;
	}
	else
	{
		MENUSONG = MEETINGA_MUS;
		ROSTER_MUS = LEVELA_MUS;
		TEXTSONG = RUMBAA_MUS;
		TITLE_LOOP_MUSIC = GOLDA_MUS;
	}
	*/
	switch (song)
	{
	case SONG_INTRO:
		return GOLDA_MUS;
	case SONG_MENU:
		return MEETINGA_MUS;
	case SONG_END:
		return -1; // blake stone has no end level music
	case SONG_ROSTER:
		return RUMBAA_MUS;
	case SONG_VICTORY:
		return LEVELA_MUS;
	}
	return -1;
}
