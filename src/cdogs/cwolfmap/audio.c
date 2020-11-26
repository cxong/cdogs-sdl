#include "audio.h"

#include <stdio.h>
#include <stdlib.h>

#include "audiowl6.h"

#define PATH_MAX 4096

int CWAudioLoadHead(CWAudioHead *head, const char *path)
{
	int err = 0;
	FILE *f = fopen(path, "rb");
	if (!f)
	{
		err = -1;
		fprintf(stderr, "Failed to read %s", path);
		goto bail;
	}
	fseek(f, 0, SEEK_END);
	const long fsize = ftell(f);
	fseek(f, 0, SEEK_SET);
	head->nOffsets = fsize / sizeof(uint32_t);
	head->offsets = malloc(head->nOffsets * sizeof(uint32_t));
	if (fread(head->offsets, sizeof(uint32_t), head->nOffsets, f) !=
		head->nOffsets)
	{
		err = -1;
		fprintf(stderr, "Failed to read audio head");
		goto bail;
	}

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
}

int CWAudioLoadAudioT(CWAudio *audio, const char *path)
{
	int err = 0;
	FILE *f = fopen(path, "rb");
	if (!f)
	{
		err = -1;
		fprintf(stderr, "Failed to read %s", path);
		goto bail;
	}
	const uint32_t len = audio->head.offsets[audio->head.nOffsets - 1];
	audio->data = malloc(len);
	if (fread(audio->data, 1, len, f) != len)
	{
		err = -1;
		fprintf(stderr, "Failed to read audio data");
		goto bail;
	}
	audio->nSound = LASTSOUND;
	audio->nMusic = LASTMUSIC;

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

int CWAudioGetAdlibSound(
	const CWAudio *audio, const int i, const char **data, size_t *len)
{
	int err = 0;
	const int off = audio->head.offsets[i + STARTADLIBSOUNDS];
	*len = audio->head.offsets[i + STARTADLIBSOUNDS + 1] - off;
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

int CWAudioGetMusic(
	const CWAudio *audio, const int i, const char **data, size_t *len)
{
	int err = 0;
	const int off = audio->head.offsets[i + STARTMUSIC];
	*len = audio->head.offsets[i + STARTMUSIC + 1] - off;
	if (*len == 0)
	{
		fprintf(stderr, "No music len for track %d\n", i);
		err = -1;
		goto bail;
	}
	*data = &audio->data[off];

bail:
	return err;
}
