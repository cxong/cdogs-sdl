#include <stdint.h>
#include <string.h>

#include <SDL_audio.h>

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
	char *data;
} CWAudio;

#define MUSIC_SAMPLE_RATE 44100
#define MUSIC_AUDIO_FMT AUDIO_S16
#define MUSIC_AUDIO_CHANNELS 2

int CWAudioLoadHead(CWAudioHead *head, const char *path);
void CWAudioHeadFree(CWAudioHead *head);

int CWAudioLoadAudioT(CWAudio *audio, const char *path);

void CWAudioFree(CWAudio *audio);

// http://www.vgmpf.com/Wiki/index.php?title=IMF
int CWAudioGetAdlibSound(
	const CWAudio *audio, const int i, const char **data, size_t *len);
int CWAudioGetMusicRaw(
	const CWAudio *audio, const int i, const char **data, size_t *len);
int CWAudioGetMusic(
	const CWAudio *audio, const int i, char **data, size_t *len);
