#include "vswap.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// http://gaarabis.free.fr/_sites/specs/wlspec_index.html

#define PATH_MAX 4096

int CWVSwapLoad(CWVSwap *vswap, const char *path)
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
	vswap->data = malloc(fsize);
	if (fread(vswap->data, 1, fsize, f) != (size_t)fsize)
	{
		err = -1;
		fprintf(stderr, "Failed to read chunk data");
		goto bail;
	}
	memcpy(&vswap->head, vswap->data, sizeof vswap->head);
	vswap->chunkOffset = (uint32_t *)(vswap->data + sizeof vswap->head);
	vswap->chunkLength = (uint16_t *)(vswap->chunkOffset + vswap->head.chunkCount);

	// Read sounds
	// Sounds can span multiple chunks
	// Last chunk is not a sound
	for (int i = vswap->head.firstSound; i < vswap->head.chunkCount - 1; i++)
	{
		const int size = vswap->chunkLength[i];
		if (size != 4096)
		{
			vswap->nSounds++;
		}
	}
	vswap->sounds = calloc(vswap->nSounds, sizeof(CWVSwapSound));
	for (int i = vswap->head.firstSound, sound = 0; i < vswap->head.chunkCount - 1; i++, sound++)
	{
		const uint32_t off = vswap->chunkOffset[i];
		uint16_t size = vswap->chunkLength[i];
		vswap->sounds[sound].off = off;
		vswap->sounds[sound].len = size;
		while (size == 4096)
		{
			i++;
			size = vswap->chunkLength[i];
			vswap->sounds[sound].len += size;
		}
	}

bail:
	if (f)
	{
		fclose(f);
	}
	return err;
}

void CWVSwapFree(CWVSwap *vswap)
{
	free(vswap->data);
	free(vswap->sounds);
}

int CWVSwapGetNumSounds(const CWVSwap *vswap)
{
	return vswap->head.chunkCount - vswap->head.firstSound;
}
int CWVSwapGetSound(
	const CWVSwap *vswap, const int i, const char **data, size_t *len)
{
	int err = 0;
	if (vswap->sounds[i].off == 0)
	{
		err = -1;
		goto bail;
	}
	*len = vswap->sounds[i].len;
	*data = &vswap->data[vswap->sounds[i].off];

bail:
	return err;
}
