#include "audio.h"

#include <stdio.h>
#include <stdlib.h>

#include "audio_sod.h"
#include "audio_wl1.h"
#include "audio_wl6.h"
#include "mame/fmopl.h"

#define PATH_MAX 4096
static int volume = 20;
static const int oplChip = 0;
#define OPL_CHANNELS 9
#define MUSIC_RATE 700
#define SOUND_RATE 140 // Also affects PC Speaker sounds
#define SOUND_TICKS (MUSIC_RATE / SOUND_RATE)
#define SAMPLES_PER_MUSIC_TICK (MUSIC_SAMPLE_RATE / MUSIC_RATE)

#pragma pack(push, 1)
typedef struct
{
	uint8_t mChar, cChar, mScale, cScale, mAttack, cAttack, mSus, cSus, mWave,
		cWave, nConn,

		// These are only for Muse - these bytes are really unused
		voice, mode;
	uint8_t unused[3];
} AlInstrument;

typedef struct
{
	uint32_t length;
	uint16_t priority;
	AlInstrument inst;
	uint8_t block;
	uint8_t data[1];
} AdLibSound;
#pragma pack(pop)

static const AlInstrument ChannelRelease = {
	0, 0, 0x3F,		0x3F, 0xFF, 0xFF, 0xF, 0xF, 0, 0, 0,

	0, 0, {0, 0, 0}};

#define alOut(n, b) YM3812Write(oplChip, n, b, &volume)

//      Register addresses
// Operator stuff
#define alChar 0x20
#define alScale 0x40
#define alAttack 0x60
#define alSus 0x80
#define alWave 0xe0
// Channel stuff
#define alFreqL 0xa0
#define alFreqH 0xb0
#define alFeedCon 0xc0
// Global stuff
#define alEffects 0xbd

static void AlSetChanInst(const AlInstrument *inst, unsigned int chan)
{
	static const uint8_t chanOps[OPL_CHANNELS] = {0,   1,	 2,	   8,	9,
												  0xA, 0x10, 0x11, 0x12};
	uint8_t c, m;

	m = chanOps[chan]; // modulator cell for channel
	c = m + 3;		   // carrier cell for channel
	alOut(m + alChar, inst->mChar);
	alOut(m + alScale, inst->mScale);
	alOut(m + alAttack, inst->mAttack);
	alOut(m + alSus, inst->mSus);
	alOut(m + alWave, inst->mWave);
	alOut(c + alChar, inst->cChar);
	alOut(c + alScale, inst->cScale);
	alOut(c + alAttack, inst->cAttack);
	alOut(c + alSus, inst->cSus);
	alOut(c + alWave, inst->cWave);

	alOut(chan + alFreqL, 0);
	alOut(chan + alFreqH, 0);
	alOut(chan + alFeedCon, 0);
}

bool CWAudioInit(void)
{
	// Init adlib
	if (YM3812Init(1, 3579545, MUSIC_SAMPLE_RATE))
	{
		fprintf(stderr, "Unable to create virtual OPL\n");
		return false;
	}
	return true;
}
void CWAudioTerminate(void)
{
	YM3812Shutdown();
}

int CWAudioLoadHead(CWAudioHead *head, const char *path)
{
	int err = 0;
	FILE *f = fopen(path, "rb");
	if (!f)
	{
		err = -1;
		fprintf(stderr, "Failed to read %s\n", path);
		goto bail;
	}

	CWAudioHeadFree(head);

	fseek(f, 0, SEEK_END);
	const long fsize = ftell(f);
	fseek(f, 0, SEEK_SET);
	head->nOffsets = fsize / sizeof(uint32_t);
	head->offsets = malloc(head->nOffsets * sizeof(uint32_t));
	if (fread(head->offsets, sizeof(uint32_t), head->nOffsets, f) !=
		head->nOffsets)
	{
		err = -1;
		fprintf(stderr, "Failed to read audio head\n");
		goto bail;
	}

	for (int i = 1; i < 0xf6; i++)
	{
		YM3812Write(oplChip, i, 0, &volume);
	}
	YM3812Write(oplChip, 1, 0x20, &volume); // Set WSE=1

bail:
	if (f)
	{
		fclose(f);
	}
	return err;
}

void CWAudioHeadFree(CWAudioHead *head)
{
	free(head->offsets);
	head->nOffsets = 0;
}

int CWAudioLoadAudioT(CWAudio *audio, const CWMapType type, const char *path)
{
	int err = 0;
	FILE *f = fopen(path, "rb");
	if (!f)
	{
		err = -1;
		fprintf(stderr, "Failed to read %s\n", path);
		goto bail;
	}

	free(audio->data);

	const uint32_t len = audio->head.offsets[audio->head.nOffsets - 1];
	audio->data = malloc(len);
	if (fread(audio->data, 1, len, f) != len)
	{
		err = -1;
		fprintf(stderr, "Failed to read audio data\n");
		goto bail;
	}
	switch (type)
	{
	case CWMAPTYPE_WL1:
		CWAudioWL1LoadAudioT(audio);
		break;
	case CWMAPTYPE_WL6:
		CWAudioWL6LoadAudioT(audio);
		break;
	case CWMAPTYPE_SOD:
		CWAudioSODLoadAudioT(audio);
		break;
	default:
		CASSERT(false, "unknown map type");
		err = -1;
		goto bail;
	}

bail:
	if (f)
	{
		fclose(f);
	}
	return err;
}

void CWAudioFree(CWAudio *audio)
{
	CWAudioHeadFree(&audio->head);
	free(audio->data);
}

int CWAudioGetAdlibSoundRaw(
	const CWAudio *audio, const int i, const char **data, size_t *len)
{
	int err = 0;
	const int off = audio->head.offsets[i + audio->startAdlibSounds];
	*len = audio->head.offsets[i + audio->startAdlibSounds + 1] - off;
	if (*len == 0)
	{
		fprintf(stderr, "No audio len for track %d\n", i);
		err = -1;
		goto bail;
	}
	*data = &audio->data[off];

bail:
	return err;
}

int CWAudioGetAdlibSound(
	const CWAudio *audio, const int idx, char **data, size_t *len)
{
	*data = NULL;
	*len = 0;
	const char *rawData;
	size_t rawLen;
	int err = CWAudioGetAdlibSoundRaw(audio, idx, &rawData, &rawLen);
	if (err != 0)
	{
		goto bail;
	}

	const AdLibSound *sound = (const AdLibSound *)rawData;
	const uint8_t alBlock = ((sound->block & 7) << 2) | 0x20;
	AlSetChanInst(&sound->inst, 0);

	const uint8_t *alSound = sound->data;
	*len = sound->length * SAMPLES_PER_MUSIC_TICK * SOUND_TICKS *
		   MUSIC_AUDIO_CHANNELS * 2;
	*data = malloc(*len);
	int16_t *stream16 = (int16_t *)*data;
	for (int alLengthLeft = (int)sound->length; alLengthLeft > 0;
		 alLengthLeft--)
	{
		// THIS is the way the original Wolfenstein 3-D code handled it!
		if (*alSound)
		{
			alOut(alFreqL, *alSound);
			alOut(alFreqH, alBlock);
		}
		else
			alOut(alFreqH, 0);
		alSound++;

		for (int i = 0; i < SOUND_TICKS; i++)
		{
			YM3812UpdateOne(oplChip, stream16, SAMPLES_PER_MUSIC_TICK);
			stream16 += SAMPLES_PER_MUSIC_TICK * MUSIC_AUDIO_CHANNELS;
		}
	}
	alOut(alFreqH, 0);

	return err;

bail:
	if (err != 0)
	{
		free(*data);
		*data = NULL;
	}
	return err;
}

int CWAudioGetMusicRaw(
	const CWAudio *audio, const int i, const char **data, size_t *len)
{
	int err = 0;
	const int off = audio->head.offsets[i + audio->startMusic];
	*len = audio->head.offsets[i + audio->startMusic + 1] - off;
	if (*len == 0)
	{
		fprintf(stderr, "No music len for track %d\n", i);
		err = -1;
		goto bail;
	}
	if (*len == 88)
	{
		*len = 0;
		goto bail;
	}
	*data = &audio->data[off];

bail:
	return err;
}

int CWAudioGetMusic(
	const CWAudio *audio, const int idx, char **data, size_t *len)
{
	*data = NULL;
	*len = 0;
	const char *rawData;
	size_t rawLen;
	int err = CWAudioGetMusicRaw(audio, idx, &rawData, &rawLen);
	if (err != 0)
	{
		goto bail;
	}
	if (rawLen == 0)
	{
		goto bail;
	}

	for (int i = 0; i < OPL_CHANNELS; i++)
	{
		AlSetChanInst(&ChannelRelease, i);
	}

	// Measure length of music
	const uint16_t *sqHack = (const uint16_t *)rawData;
	int sqHackLen;
	if (*sqHack == 0)
	{
		// LumpLength?
		sqHackLen = (int)rawLen;
	}
	else
	{
		sqHackLen = *sqHack++;
	}
	const uint16_t *sqHackPtr = sqHack;

	int sqHackTime = 0;
	int alTimeCount;
	for (alTimeCount = 0; sqHackLen > 0; alTimeCount++)
	{
		do
		{
			if (sqHackTime > alTimeCount)
				break;
			sqHackTime = alTimeCount + *(sqHackPtr + 1);
			sqHackPtr += 2;
			sqHackLen -= 4;
		} while (sqHackLen > 0);
	}

	// Decode music
	// 2 bytes per sample (16-bit audio fmt)
	*len = alTimeCount * SAMPLES_PER_MUSIC_TICK * MUSIC_AUDIO_CHANNELS * 2;
	*data = malloc(*len);
	int16_t *stream16 = (int16_t *)*data;

	sqHack = (const uint16_t *)rawData;
	if (*sqHack == 0)
	{
		// LumpLength?
		sqHackLen = (int)rawLen;
	}
	else
	{
		sqHackLen = *sqHack++;
	}
	sqHackPtr = sqHack;
	sqHackTime = 0;
	for (alTimeCount = 0; sqHackLen > 0; alTimeCount++)
	{
		do
		{
			if (sqHackTime > alTimeCount)
				break;
			sqHackTime = alTimeCount + *(sqHackPtr + 1);
			alOut(
				*(const uint8_t *)sqHackPtr,
				*(((const uint8_t *)sqHackPtr) + 1));
			sqHackPtr += 2;
			sqHackLen -= 4;
		} while (sqHackLen > 0);

		YM3812UpdateOne(oplChip, stream16, SAMPLES_PER_MUSIC_TICK);

		stream16 += SAMPLES_PER_MUSIC_TICK * MUSIC_AUDIO_CHANNELS;
	}

	return err;

bail:
	if (err != 0)
	{
		free(*data);
		*data = NULL;
	}
	return err;
}

int CWAudioGetLevelMusic(const CWMapType type, const int level)
{
	switch (type)
	{
	case CWMAPTYPE_WL1:
	case CWMAPTYPE_WL6:
		return CWAudioWL6GetLevelMusic(level);
	case CWMAPTYPE_SOD:
		return CWAudioSODGetLevelMusic(level);
	default:
		return -1;
	}
}

int CWAudioGetSong(const CWMapType type, const CWSongType song)
{
	switch (type)
	{
	case CWMAPTYPE_WL1:
	case CWMAPTYPE_WL6:
		return CWAudioWL6GetSong(song);
	case CWMAPTYPE_SOD:
		return CWAudioSODGetSong(song);
	default:
		return -1;
	}
}
