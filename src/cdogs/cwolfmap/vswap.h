#include <stdint.h>
#include <string.h>

#define SND_RATE 7042
// Sounds are unsigned 8-bit mono PCM

#pragma pack(push, 1)
typedef struct
{
	uint16_t chunkCount;
	uint16_t firstSprite;
	uint16_t firstSound;
} CWVSwapHead;
#pragma pack(pop)

typedef struct
{
	uint32_t off;
	uint16_t len;
} CWVSwapSound;

typedef struct
{
	CWVSwapHead head;
	uint32_t *chunkOffset;
	uint16_t *chunkLength;
	char *data;
	CWVSwapSound *sounds;
	int nSounds;
} CWVSwap;

int CWVSwapLoad(CWVSwap *vswap, const char *path);
void CWVSwapFree(CWVSwap *vswap);

int CWVSwapGetSound(
	const CWVSwap *vswap, const int i, const char **data, size_t *len);
