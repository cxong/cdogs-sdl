/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.
	Copyright (c) 2020 Cong Xu
	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met:

	Redistributions of source code must retain the above copyright notice, this
	list of conditions and the following disclaimer.
	Redistributions in binary form must reproduce the above copyright notice,
	this list of conditions and the following disclaimer in the documentation
	and/or other materials provided with the distribution.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
	AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
	IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
	ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
	LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
	CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
	SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
	INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
	CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
	ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
	POSSIBILITY OF SUCH DAMAGE.
*/
#include "map_wolf.h"

#include "cwolfmap/cwolfmap.h"
#include "map_archive.h"

static void GetCampaignPath(const CWMapType type, char *buf)
{
	switch (type)
	{
	case CWMAPTYPE_WL1:
		GetDataFilePath(buf, "missions/.wolf3d/WL1.cdogscpn");
	case CWMAPTYPE_WL6:
		GetDataFilePath(buf, "missions/.wolf3d/WL6.cdogscpn");
		break;
	case CWMAPTYPE_SOD:
		GetDataFilePath(buf, "missions/.wolf3d/SOD.cdogscpn");
		break;
	default:
		CASSERT(false, "unknown map type");
		break;
	}
}
static const char *soundsW1[] = {
	// 0-9
	"chars/alert/guard", "chars/alert/dog", "door_close", "door",
	"machine_gun", "pistol", "chain_gun", "chars/alert/ss", "chars/alert/hans",
	"chars/die/hans",
	// 10-15
	"dual_chain_gun", "machine_gun_burst", "chars/die/guard/0",
	"chars/die/guard/1", "chars/die/guard/2", "secret_door", NULL, NULL, NULL,
	NULL,
	// 20-29
	NULL, NULL, NULL, NULL, NULL, NULL, "chars/die/ss", "pistol_guard",
	"gurgle", NULL,
	// 30-39
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	// 40-49
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	// 50-57
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, "victory"};
static const char *soundsW6[] = {
	// 0-9
	"chars/alert/guard", "chars/alert/dog/0", "door_close", "door",
	"machine_gun", "pistol", "chain_gun", "chars/alert/ss", "chars/alert/hans",
	"chars/die/hans",
	// 10-19
	"dual_chain_gun", "machine_gun_burst", "chars/die/guard/0",
	"chars/die/guard/1", "chars/die/guard/2", "secret_door", "chars/die/dog",
	"chars/die/mutant", "chars/alert/mecha_hitler", "chars/die/hitler",
	// 20-29
	"chars/die/ss", "pistol_guard", "gurgle", "chars/alert/fake_hitler",
	"chars/die/schabbs", "chars/alert/schabbs", "chars/die/fake_hitler",
	"chars/alert/officer", "chars/die/officer", "chars/alert/dog/1",
	// 30-39
	"level_end", "mecha_step", "victory", "chars/die/mecha_hitler",
	"chars/die/guard/3", "chars/die/guard/4", "chars/die/otto",
	"chars/alert/otto", "chars/alert/fettgesicht", "fart",
	// 40-49
	"chars/die/guard/5", "chars/die/guard/6", "chars/die/guard/7",
	"chars/alert/gretel", "chars/die/gretel", "chars/die/fettgesicht"};
static const char *soundsSOD[] = {
	// 0-9
	"chars/alert/guard", "chars/alert/dog/0", "door_close", "door",
	"machine_gun", "pistol", "chain_gun", "chars/alert/ss", "dual_chain_gun",
	"machine_gun_burst",
	// 10-19
	"chars/die/guard/0", "chars/die/guard/1", "chars/die/guard/2",
	"secret_door", "chars/die/dog", "chars/die/mutant", "chars/die/ss",
	"pistol_guard", "gurgle", "chars/alert/officer",
	// 20-29
	"chars/die/officer", "chars/alert/dog/1", "level_end", "chars/die/guard/3",
	"chars/die/guard/4", "fart", "chars/die/guard/5", "chars/die/guard/6",
	"chars/die/guard/7", "chars/alert/trans",
	// 30-39
	"chars/die/trans", "chars/alert/bill", "chars/die/bill",
	"chars/die/ubermutant", "chars/alert/knight", "chars/die/knight",
	"chars/alert/angel", "chars/die/angel", "chaingun_pickup", "spear"};
static const char *GetSound(const CWMapType type, const int i)
{
	// Map sound index to string
	switch (type)
	{
	case CWMAPTYPE_WL1:
		return soundsW1[i];
	case CWMAPTYPE_WL6:
		return soundsW6[i];
	case CWMAPTYPE_SOD:
		return soundsSOD[i];
	default:
		CASSERT(false, "unknown map type");
		return NULL;
	}
}

int MapWolfScan(const char *filename, char **title, int *numMissions)
{
	int err = 0;
	CWolfMap map;
	err = CWLoad(&map, filename);
	if (err != 0)
	{
		goto bail;
	}
	char buf[CDOGS_PATH_MAX];
	GetCampaignPath(map.type, buf);
	err = MapNewScanArchive(buf, title, NULL);
	if (err != 0)
	{
		goto bail;
	}
	*numMissions = map.nLevels;

bail:
	CWFree(&map);
	return err;
}

static void LoadSounds(const SoundDevice *s, const CWolfMap *map);

int MapWolfLoad(const char *filename, CampaignSetting *c)
{
	int err = 0;
	CWolfMap map;
	err = CWLoad(&map, filename);
	if (err != 0)
	{
		goto bail;
	}

	char buf[CDOGS_PATH_MAX];
	GetCampaignPath(map.type, buf);
	err = MapNewLoadArchive(buf, c);
	if (err != 0)
	{
		goto bail;
	}

	LoadSounds(&gSoundDevice, &map);

	const CWLevel *level = map.levels;
	for (int i = 0; i < map.nLevels; i++, level++)
	{
		printf(
			"Level %d: %s (%dx%d)\n", i + 1, level->header.name,
			level->header.width, level->header.height);
		/*for (int x = 0; x < level->header.width; x++)
		{
			for (int y = 0; y < level->header.height; y++)
			{
				//PrintCh(level, x, y, map.type);
			}
			printf(" \n");
		}*/
	}

bail:
	CWFree(&map);
	return err;
}

static void LoadSounds(const SoundDevice *s, const CWolfMap *map)
{
	// TODO: load ad lib sounds
	int err = 0;
	for (int i = 0; i < map->vswap.nSounds; i++)
	{
		const char *data;
		size_t len;
		err = CWVSwapGetSound(&map->vswap, i, &data, &len);
		if (err != 0)
		{
			continue;
		}
		if (len == 0)
		{
			continue;
		}
		SDL_AudioCVT cvt;
		SDL_BuildAudioCVT(
			&cvt, AUDIO_U8, 1, SND_RATE, CDOGS_SND_FMT, CDOGS_SND_CHANNELS,
			CDOGS_SND_RATE);
		cvt.len = (int)len;
		cvt.buf = (Uint8 *)SDL_malloc(cvt.len * cvt.len_mult);
		memcpy(cvt.buf, data, len);
		SDL_ConvertAudio(&cvt);
		SoundData *sound;
		CMALLOC(sound, sizeof *sound);
		sound->Type = SOUND_NORMAL;
		sound->u.normal = Mix_QuickLoad_RAW(cvt.buf, cvt.len_cvt);
		SoundAdd(s->customSounds, GetSound(map->type, i), sound);
	}
}
