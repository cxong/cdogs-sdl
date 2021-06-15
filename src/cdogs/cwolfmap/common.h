#pragma once
#include <stddef.h>
#include <stdint.h>

typedef enum
{
	CWMAPTYPE_WL1,
	CWMAPTYPE_WL6,
	CWMAPTYPE_SOD,
} CWMapType;

typedef struct
{
	uint32_t *offsets;
	size_t nOffsets;
} CWAudioHead;

typedef struct
{
	CWAudioHead head;
	int nSound;
	int nMusic;
	int startAdlibSounds;
	int startMusic;
	char *data;
} CWAudio;
