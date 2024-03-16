#pragma once
#include "audio.h"

void CWAudioN3DLoadAudioT(CWAudio *audio);
int CWAudioN3DLoadAudioWAD(CWAudio *audio, const char *path);
int CWAudioN3DGetLevelMusic(const int level);
int CWAudioN3DGetSong(const CWSongType song);
