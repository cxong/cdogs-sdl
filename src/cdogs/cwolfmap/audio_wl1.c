#include "audio_wl1.h"

#include "AUDIOWL1.H"

void CWAudioWL1LoadAudioT(CWAudio *audio)
{
	audio->nSound = LASTSOUND;
	audio->nMusic = LASTMUSIC;
	audio->startAdlibSounds = STARTADLIBSOUNDS;
	audio->startMusic = STARTMUSIC;
}
