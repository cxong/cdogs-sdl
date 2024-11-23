/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.
	Copyright (c) 2020-2024 Cong Xu
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

#include <find_steam_game.h>

#include "log.h"

#include "cwolfmap/audio.h"
#include "cwolfmap/cwolfmap.h"
#include "map_archive.h"
#include "player_template.h"

CWolfMap *defaultWolfMap = NULL;
CWolfMap *defaultSpearMap = NULL;

#define WOLF_STEAM_NAME "Wolfenstein 3D"
#define SPEAR_STEAM_NAME "Spear of Destiny"
#define N3D_STEAM_NAME "Super 3-D Noah's Ark"
#define WOLF_GOG_ID "1441705046"
#define WOLF_SPEAR_GOG_ID "1441705226"
#define SPEAR_GOG_ID "1441705126"
#define N3D_GOG_ID "1672565562"
#define WOLF_DATA_DIR "data/.wolf3d/"

#define TILE_CLASS_WALL_OFFSET 63

// Generate shuffled numbers from 0-N, never repeating until we draw >N times,
// then repeat
typedef struct
{
	CArray indices;
	int idx;
} IdxShuffler;
static void IdxShufflerInit(IdxShuffler *s, const int n)
{
	CArrayInit(&s->indices, sizeof(int));
	s->idx = 0;
	for (int i = 0; i < n; i++)
	{
		CArrayPushBack(&s->indices, &i);
	}
	CArrayShuffle(&s->indices);
}
static int IdxShufflerDraw(IdxShuffler *s)
{
	const int *value = CArrayGet(&s->indices, s->idx);
	s->idx++;
	if (s->idx == (int)s->indices.size)
	{
		s->idx = 0;
	}
	return *value;
}
static void IdxShufflerTerminate(IdxShuffler *s)
{
	CArrayTerminate(&s->indices);
}

void MapWolfInit(void)
{
	defaultWolfMap = NULL;
	defaultSpearMap = NULL;
	if (!CWAudioInit())
	{
		CASSERT(false, "failed to init wolf audio!");
	}
}
void MapWolfTerminate(void)
{
	CWFree(defaultWolfMap);
	defaultWolfMap = NULL;
	CWFree(defaultSpearMap);
	defaultSpearMap = NULL;
	CWAudioTerminate();
}

static void GetCampaignPath(
	const CWMapType type, const int spearMission, char *buf)
{
	switch (type)
	{
	case CWMAPTYPE_WL1:
		GetDataFilePath(buf, WOLF_DATA_DIR "WL1.cdogscpn");
		break;
	case CWMAPTYPE_WL6:
		GetDataFilePath(buf, WOLF_DATA_DIR "WL6.cdogscpn");
		break;
	case CWMAPTYPE_SOD:
		switch (spearMission)
		{
		case 1:
			GetDataFilePath(buf, WOLF_DATA_DIR "SOD.cdogscpn");
			break;
		case 2:
			GetDataFilePath(buf, WOLF_DATA_DIR "SD2.cdogscpn");
			break;
		case 3:
			GetDataFilePath(buf, WOLF_DATA_DIR "SD3.cdogscpn");
			break;
		default:
			CASSERT(false, "Unknown spear mission");
			GetDataFilePath(buf, WOLF_DATA_DIR "SOD.cdogscpn");
			break;
		}
		break;
	case CWMAPTYPE_N3D:
		GetDataFilePath(buf, WOLF_DATA_DIR "N3D.cdogscpn");
		break;
	default:
		// TODO: implement unknown map types
		LOG(LM_MAP, LL_ERROR, "Unknown wolf map type %d", (int)type);
		buf[0] = '\0';
		break;
	}
}
static const char *soundsW1[] = {
	// 0-9
	"chars/alert/guard", "chars/alert/dog", "door_close", "door",
	"machine_gun", "pistol", "chain_gun", "chars/alert/ss", "chars/alert/hans",
	"chars/die/hans",
	// 10-15
	"dual_chain_gun", "machine_gun_burst", "chars/die/guard/",
	"chars/die/guard/", "chars/die/guard/", "secret_door", NULL, NULL, NULL,
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
	"chars/alert/guard", "chars/alert/dog/", "door_close", "door",
	"machine_gun", "pistol", "chain_gun", "chars/alert/ss", "chars/alert/hans",
	"chars/die/hans",
	// 10-19
	"dual_chain_gun", "machine_gun_burst", "chars/die/guard/",
	"chars/die/guard/", "chars/die/guard/", "secret_door", "chars/die/dog",
	"chars/die/mutant", "chars/alert/mecha_hitler", "chars/die/hitler",
	// 20-29
	"chars/die/ss", "pistol_guard", "gurgle", "chars/alert/fake_hitler",
	"chars/die/schabbs", "chars/alert/schabbs", "chars/die/fake_hitler",
	"chars/alert/officer", "chars/die/officer", "chars/alert/dog/",
	// 30-39
	"whistle", "footsteps/mech", "victory", "chars/die/mecha_hitler",
	"chars/die/guard/", "chars/die/guard/", "chars/die/otto",
	"chars/alert/otto", "chars/alert/fettgesicht", "fart",
	// 40-49
	"chars/die/guard/", "chars/die/guard/", "chars/die/guard/",
	"chars/alert/gretel", "chars/die/gretel", "chars/die/fettgesicht"};
static const char *soundsSOD[] = {
	// 0-9
	"chars/alert/guard", "chars/alert/dog/", "door_close", "door",
	"machine_gun", "pistol", "chain_gun", "chars/alert/ss", "dual_chain_gun",
	"machine_gun_burst",
	// 10-19
	"chars/die/guard/", "chars/die/guard/", "chars/die/guard/", "secret_door",
	"chars/die/dog", "chars/die/mutant", "chars/die/ss", "pistol_guard",
	"gurgle", "chars/alert/officer",
	// 20-29
	"chars/die/officer", "chars/alert/dog/", "whistle", "chars/die/guard/",
	"chars/die/guard/", "fart", "chars/die/guard/", "chars/die/guard/",
	"chars/die/guard/", "chars/alert/trans",
	// 30-39
	"chars/die/trans", "chars/alert/bill", "chars/die/bill",
	"chars/die/ubermutant", "chars/alert/knight", "chars/die/knight",
	"chars/alert/angel", "chars/die/angel", "chaingun_pickup", "spear"};
static const char *soundsN3D[] = {
	// 0-9
	"chars/alert/antelope", "chars/alert/bear", "pickup", "chars/alert/camel",
	"door_close", "cantaloupe", "cantaloupe_feeder", "goat_kick", "hits/food",
	"chars/die/antelope|chars/die/bear|chars/die/camel|chars/die/"
	"elephant|chars/die/giraffe|chars/die/goat|chars/die/kangaroo|chars/die/"
	"monkey|chars/die/ostrich|chars/die/ox|chars/die/sheep",
	// 10-19
	"chars/alert/elephant", "1up", "super_feeder", "chars/alert/giraffe",
	"chars/alert/goat", "small_launcher", "ricochet", "chars/alert/kangaroo",
	"whistle", "hand_feed",
	// 20-29
	"chars/alert/monkey", "footsteps/bear", "door", "chars/alert/ostrich",
	"chars/alert/ox", "hurt", "hahaha", "secret_door", "chars/alert/sheep",
	"large_launcher",
	// 30-32
	"spit", "watermelon", "watermelon_feeder"}; // TODO BS6:
// https://github.com/bibendovsky/bstone/blob/3dea1ef72a101519afd17aa95c881da40a18040d/src/bstone_audio_content_mgr.cpp#L320-L398
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
	case CWMAPTYPE_N3D:
		return soundsN3D[i];
	default:
		CASSERT(false, "unknown map type");
		return NULL;
	}
}

static const char *adlibSoundsW1[] = {
	NULL,		  // hit wall
	"menu_start", // select weapon
	NULL,		  // select item
	NULL,		  // heartbeat
	"menu_switch",
	NULL, // move gun 1
	"menu_error",
	NULL, // nazi hit player
	NULL, // nazi miss player
	NULL, // player death (unused because of some corruption at end of sound)
	NULL, // dog death (digi sound)
	NULL, // gatling (digi sound)
	"key",
	NULL, // no item
	NULL, // walk1
	NULL, // walk2
	NULL, // take damage
	NULL, // game over
	NULL, // open door (digi sound)
	NULL, // close door (digi sound)
	NULL, // do nothing (not used in C-Dogs)
	NULL, // guard alert (digi sound)
	NULL, // death 2 (digi sound)
	"hits/knife_flesh/",
	NULL, // pistol (digi sound)
	NULL, // death 3 (digi sound)
	NULL, // machine gun (digi sound)
	NULL, // hit enemy
	NULL, // shoot door
	NULL, // death 1 (digi sound)
	"machine_gun_switch",
	"ammo_pickup",
	"menu_enter",
	"health_small",
	"health_big",
	"pickup_cross",
	"pickup_chalice",
	"pickup_chest",
	"chaingun_pickup",
	"menu_back",
	"whistle",
	NULL,				// dog alert (digi sound)
	NULL,				// end bonus 1 (not used in C-Dogs)
	"mission_complete", // end bonus 2
	"1up",
	"pickup_crown",
	NULL, // push wall (digi sound)
	NULL, // no bonus (not used in C-Dogs)
	NULL, // 100% (not used in C-Dogs)
	NULL, // boss active
	NULL, // boss die
	NULL, // SS alert (digi sound)
	NULL, // aah (digi sound)
	NULL, // mecha hitler die (digi sound)
	NULL, // hitler die (digi sound)
	NULL, // hans alert (digi sound)
	NULL, // SS die (digi sound)
	NULL, // hans die (digi sound)
	NULL, // guard fire (digi sound)
	NULL, // boss chain gun (digi sound)
	NULL, // SS fire (digi sound)
	NULL, // slurpie (digi sound)
	NULL, // fake hitler alert (digi sound)
	NULL, // schabbs die (digi sound)
	NULL, // schabbs alert (digi sound)
	NULL, // hitler alert (digi sound)
	NULL, // officer alert (digi sound)
	NULL, // officer die (digi sound)
	NULL, // dog attack (digi sound)
};
static const char *adlibSoundsW6[] = {
	NULL,		  // hit wall
	"menu_start", // select weapon
	NULL,		  // select item
	NULL,		  // heartbeat
	"menu_switch",
	NULL, // move gun 1
	"menu_error",
	NULL, // nazi hit player
	"syringe",
	NULL, // player death (unused because of some corruption at end of sound)
	NULL, // dog death (digi sound)
	NULL, // gatling (digi sound)
	"key",
	NULL, // no item
	NULL, // walk1
	NULL, // walk2
	NULL, // take damage
	NULL, // game over
	NULL, // open door (digi sound)
	NULL, // close door (digi sound)
	NULL, // do nothing (not used in C-Dogs)
	NULL, // guard alert (digi sound)
	NULL, // death 2 (digi sound)
	"hits/knife_flesh/",
	NULL, // pistol (digi sound)
	NULL, // death 3 (digi sound)
	NULL, // machine gun (digi sound)
	NULL, // hit enemy
	NULL, // shoot door
	NULL, // death 1 (digi sound)
	"machine_gun_switch",
	"ammo_pickup",
	"menu_enter",
	"health_small",
	"health_big",
	"pickup_cross",
	"pickup_chalice",
	"pickup_chest",
	"chaingun_pickup",
	"menu_back",
	NULL,				// level end (digi sound)
	NULL,				// dog alert (digi sound)
	NULL,				// end bonus 1 (not used in C-Dogs)
	"mission_complete", // end bonus 2
	"1up",
	"pickup_crown",
	NULL, // push wall (digi sound)
	NULL, // no bonus (not used in C-Dogs)
	NULL, // 100% (not used in C-Dogs)
	NULL, // boss active
	NULL, // boss die
	NULL, // SS alert (digi sound)
	NULL, // aah (digi sound)
	NULL, // mecha hitler die (digi sound)
	NULL, // hitler die (digi sound)
	NULL, // hans alert (digi sound)
	NULL, // SS die (digi sound)
	NULL, // hans die (digi sound)
	NULL, // guard fire (digi sound)
	NULL, // boss chain gun (digi sound)
	NULL, // SS fire (digi sound)
	NULL, // slurpie (digi sound)
	NULL, // fake hitler alert (digi sound)
	NULL, // schabbs die (digi sound)
	NULL, // schabbs alert (digi sound)
	NULL, // hitler alert (digi sound)
	NULL, // officer alert (digi sound)
	NULL, // officer die (digi sound)
	NULL, // dog attack (digi sound)
	"flamethrower",
	NULL, // mech step (digi sound)
	NULL, // goobs
	NULL, // yeah (digi sound)
	NULL, // guard die 4 (digi sound)
	NULL, // guard die 5 (digi sound)
	NULL, // guard die 6 (digi sound)
	NULL, // guard die 7 (digi sound)
	NULL, // guard die 8 (digi sound)
	NULL, // guard die 9 (digi sound)
	NULL, // otto die (digi sound)
	NULL, // otto alert (digi sound)
	NULL, // fettgesicht alert (digi sound)
	NULL, // gretel alert (digi sound)
	NULL, // gretel die (digi sound)
	NULL, // fettgesicht die (digi sound)
	"rocket",
	"hits/rocket/",
};
static const char *adlibSoundsSOD[] = {
	NULL, // hit wall
	"hits/rocket/",
	"menu_start", // select item
	"chars/alert/ghost",
	"menu_switch",
	NULL, // move gun 1
	"menu_error",
	NULL, // nazi hit player
	"rocket",
	NULL, // player death (unused because of some corruption at end of sound)
	NULL, // dog death (digi sound)
	NULL, // gatling (digi sound)
	"key",
	NULL, // no item
	NULL, // walk1
	NULL, // walk2
	NULL, // take damage
	NULL, // game over
	NULL, // open door (digi sound)
	NULL, // close door (digi sound)
	NULL, // do nothing (not used in C-Dogs)
	NULL, // guard alert (digi sound)
	NULL, // death 2 (digi sound)
	"hits/knife_flesh/",
	NULL, // pistol (digi sound)
	NULL, // death 3 (digi sound)
	NULL, // machine gun (digi sound)
	NULL, // hit enemy
	NULL, // shoot door
	NULL, // death 1 (digi sound)
	"machine_gun_switch",
	"ammo_pickup",
	"menu_enter",
	"health_small",
	"health_big",
	"pickup_cross",
	"pickup_chalice",
	"pickup_chest",
	NULL, // chain gun pickup (digi sound)
	"menu_back",
	NULL,				// level end (digi sound)
	NULL,				// dog alert (digi sound)
	NULL,				// end bonus 1 (not used in C-Dogs)
	"mission_complete", // end bonus 2
	"1up",
	"pickup_crown",
	NULL, // push wall (digi sound)
	NULL, // no bonus (not used in C-Dogs)
	NULL, // 100% (not used in C-Dogs)
	NULL, // boss active
	NULL, // guard die 4 (digi sound)
	NULL, // SS alert (digi sound)
	NULL, // aah (digi sound)
	NULL, // guard die 5 (digi sound)
	NULL, // guard die 7 (digi sound)
	NULL, // guard die 8 (digi sound)
	NULL, // SS die (digi sound)
	NULL, // guard die 6 (digi sound)
	NULL, // guard fire (digi sound)
	NULL, // boss chain gun (digi sound)
	NULL, // SS fire (digi sound)
	NULL, // slurpie (digi sound)
	"chars/ghost/die",
	NULL, // guard die 9 (digi sound)
	"ammo_box",
	NULL, // angel alert (digi sound)
	NULL, // officer alert (digi sound)
	NULL, // officer die (digi sound)
	NULL, // dog attack (digi sound)
	"flamethrower",
	NULL, // trans alert (digi sound)
	NULL, // trans die (digi sound)
	NULL, // wilhelm alert (digi sound)
	NULL, // wilhelm die (digi sound)
	NULL, // uber die (digi sound)
	NULL, // knight alert (digi sound)
	NULL, // knight die (digi sound)
	NULL, // angel die (digi sound)
	"knight_rocket",
	NULL, // spear (digi sound)
	NULL, // angel tired (not used in C-Dogs)
};
static const char *adlibSoundsN3D[] = {
	// 0-9
	NULL, // empty
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	// 10-19
	NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, // score tick
	NULL, NULL,
	// 20-29
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "wrong", // answer wrong
	NULL,
	// 30-39
	NULL, NULL, NULL, NULL, NULL, NULL, "menu_back", "menu_switch",
	"menu_switch2", // TODO: the correct sound is menu_switch + this one; can
	// we concat the two?
	NULL,
	// 40-42
	"menu_enter", "menu_start", // not really used as this in N3D, arbitrarily
								// use this for C-Dogs
	"bonus",					// quiz correct, perfect score
};
static const char *GetAdlibSound(const CWMapType type, const int i)
{
	// Map sound index to string
	switch (type)
	{
	case CWMAPTYPE_WL1:
		return adlibSoundsW1[i];
	case CWMAPTYPE_WL6:
		return adlibSoundsW6[i];
	case CWMAPTYPE_SOD:
		return adlibSoundsSOD[i];
	case CWMAPTYPE_N3D:
		return adlibSoundsN3D[i];
	default:
		CASSERT(false, "unknown map type");
		return NULL;
	}
}

static const CWSongType songsCampaign[] = {
	SONG_INTRO,	  // menu
	SONG_MENU,	  // briefing
	0,			  // game
	SONG_END,	  // end
	SONG_ROSTER,  // lose
	SONG_VICTORY, // victory
};
static bool LoadMusic(CWolfMap *map, MusicChunk *chunk, const int i)
{
	char *data;
	size_t len;
	const int err = CWAudioGetMusic(&map->audio, map->type, i, &data, &len);
	if (err != 0)
	{
		return false;
	}
	if (map->type == CWMAPTYPE_N3D)
	{
		SDL_RWops *rwops = SDL_RWFromMem(data, (int)len);
		chunk->u.Music = Mix_LoadMUS_RW(rwops, 1);
		return true;
	}
	chunk->u.Chunk = Mix_QuickLoad_RAW((Uint8 *)data, (Uint32)len);
	return false;
}

static bool IsDefaultMap(const char *filename)
{
	char buf[CDOGS_PATH_MAX];
	RealPath(filename, buf);
	return strchr(buf, '/') != NULL &&
		   (
			   // GOG Windows
			   StrEndsWith(buf, "WOLFENSTEIN 3D") ||
			   StrEndsWith(buf, "SPEAR OF DESTINY/M1") ||
			   StrEndsWith(buf, "SPEAR OF DESTINY/M2") ||
			   StrEndsWith(buf, "SPEAR OF DESTINY/M3") ||
			   // Steam Windows
			   StrEndsWith(buf, "WOLFENSTEIN 3D/BASE") ||
			   StrEndsWith(buf, "SPEAR OF DESTINY/BASE") ||
			   // GOG/Steam Windows
			   StrEndsWith(buf, "Super 3-D Noah's Ark") ||
			   // Steam Linux
			   StrEndsWith(buf, "Wolfenstein 3D/base") ||
			   StrEndsWith(buf, "Spear of Destiny/base"));
}

static void AdjustCampaignTitle(const char *filename, char **title)
{
	// Add folder name to campaign if this is a custom map
	if (!IsDefaultMap(filename) && title)
	{
		char buf[CDOGS_PATH_MAX];
		RealPath(filename, buf);
		const char *basename = strrchr(buf, '/') + 1;
		char titleBuf[CDOGS_PATH_MAX];
		sprintf(titleBuf, "%s (%s)", *title, basename);
		CFREE(*title);
		CSTRDUP(*title, titleBuf);
	}
}

static bool LoadDefault(CWolfMap *map, const char *filename)
{
	memset(map, 0, sizeof *map);
	map->type = CWGetType(filename, NULL, NULL, 1);
	switch (map->type)
	{
	case CWMAPTYPE_WL6:
		if (defaultWolfMap != NULL)
		{
			CWCopy(map, defaultWolfMap);
			return true;
		}
		break;
	case CWMAPTYPE_SOD:
		if (defaultSpearMap != NULL)
		{
			CWCopy(map, defaultSpearMap);
			return true;
		}
		break;
	default:
		break;
	}
	return false;
}

int MapWolfScan(
	const char *filename, const int spearMission, char **title,
	int *numMissions)
{
	int err = 0;
	bool usedAsDefault = false;
	CWolfMap map;
	const bool loadedFromDefault = LoadDefault(&map, filename);
	err = CWLoad(&map, filename, spearMission);
	if (loadedFromDefault)
	{
		err = 0;
	}
	else if (err != 0)
	{
		goto bail;
	}

	// Look for a campaign.json in the folder and use that if available
	if (MapNewScanArchive(filename, title, numMissions) == 0)
	{
		goto bail;
	}

	char buf[CDOGS_PATH_MAX];
	GetCampaignPath(map.type, spearMission, buf);
	err = MapNewScanArchive(buf, title, NULL);
	if (err != 0)
	{
		goto bail;
	}
	AdjustCampaignTitle(filename, title);
	if (numMissions)
	{
		// Count the number of valid levels
		*numMissions = 0;
		for (int i = 0; i < map.nLevels; i++)
		{
			const CWLevel *level = &map.levels[i];
			if (level->hasPlayerSpawn)
			{
				(*numMissions)++;
			}
		}
	}

bail:
	if (err == 0 && IsDefaultMap(filename))
	{
		switch (map.type)
		{
		case CWMAPTYPE_WL6:
			if (defaultWolfMap == NULL)
			{
				LOG(LM_MAP, LL_INFO, "Using %s as default wolf map\n",
					filename);
				CMALLOC(defaultWolfMap, sizeof map);
				memcpy(defaultWolfMap, &map, sizeof map);
				usedAsDefault = true;
			}
			break;
		case CWMAPTYPE_SOD:
			if (defaultSpearMap == NULL)
			{
				LOG(LM_MAP, LL_INFO, "Using %s as default spear map\n",
					filename);
				CMALLOC(defaultSpearMap, sizeof map);
				memcpy(defaultSpearMap, &map, sizeof map);
				usedAsDefault = true;
			}
			break;
		default:
			break;
		}
	}
	if (!usedAsDefault)
	{
		CWFree(&map);
	}
	return err;
}

static void LoadSounds(const SoundDevice *s, const CWolfMap *map);
static void LoadN3DScrolls(const CWolfMap *map);
static void LoadMission(
	CampaignSetting *c, const map_t tileClasses, CWolfMap *map,
	const int spearMission, const int missionIndex, const int numMissions,
	IdxShuffler *scrollShuffler);
typedef struct
{
	CWolfMap *Map;
	MusicType Type;
} CampaignSongData;
static bool GetCampaignSong(MusicChunk *chunk, void *data)
{
	CampaignSongData *csd = data;
	const int songIndex = songsCampaign[csd->Type];
	return LoadMusic(
		csd->Map, chunk, CWAudioGetSong(csd->Map->type, songIndex));
}
int MapWolfLoad(
	const char *filename, const int spearMission, CampaignSetting *c)
{
	int err = 0;
	CWolfMap *map = NULL;
	CMALLOC(map, sizeof *map);
	c->CustomData = map;
	c->CustomDataTerminate = (void (*)(void *))CWFree;
	map_t tileClasses = NULL;
	CharacterStore cs;
	memset(&cs, 0, sizeof cs);
	IdxShuffler scrollShuffler;
	memset(&scrollShuffler, 0, sizeof scrollShuffler);

	const bool loadedFromDefault = LoadDefault(map, filename);
	err = CWLoad(map, filename, spearMission);
	if (loadedFromDefault)
	{
		err = 0;
	}
	else if (err != 0)
	{
		goto bail;
	}

	LoadSounds(&gSoundDevice, map);
	for (int i = 0; i < MUSIC_COUNT; i++)
	{
		CampaignSongData *csd;
		CMALLOC(csd, sizeof *csd);
		csd->Map = map;
		csd->Type = i;
		c->CustomSongs[i].Data = csd;
		c->CustomSongs[i].GetData = GetCampaignSong;
		c->CustomSongs[i].isMusic = false;
		c->CustomSongs[i].u.Chunk = NULL;
	}

	char buf[CDOGS_PATH_MAX];
	// Copy data from common campaign
	switch (map->type)
	{
	case CWMAPTYPE_N3D:
		GetDataFilePath(buf, WOLF_DATA_DIR "N3Ddata.cdogscpn");
		break;
	default:
		switch (spearMission)
		{
		case 2:
			GetDataFilePath(buf, WOLF_DATA_DIR "SD2data.cdogscpn");
			break;
		case 3:
			GetDataFilePath(buf, WOLF_DATA_DIR "SD3data.cdogscpn");
			break;
		default:
			GetDataFilePath(buf, WOLF_DATA_DIR "common.cdogscpn");
			break;
		}
		break;
	}
	CampaignSetting cCommon;
	CampaignSettingInit(&cCommon);
	err = MapNewLoadArchive(buf, &cCommon);
	if (err != 0)
	{
		goto bail;
	}
	Mission *m = CArrayGet(&cCommon.Missions, 0);
	tileClasses = hashmap_copy(m->u.Static.TileClasses, TileClassCopyHashMap);
	CharacterStoreCopy(
		&cs, &cCommon.characters, &gPlayerTemplates.CustomClasses);
	CampaignSettingTerminate(&cCommon);
	// Create walk-through copies of all the walls
	for (int i = 3; i <= 65; i++)
	{
		TileClass *orig;
		sprintf(buf, "%d", i);
		if (hashmap_get(tileClasses, buf, (any_t *)&orig) != MAP_OK)
		{
			LOG(LM_MAP, LL_ERROR, "failed to get tile class for copying");
			break;
		}
		TileClass *tc;
		CMALLOC(tc, sizeof *tc);
		TileClassCopy(tc, orig);
		tc->canWalk = true;
		// Slightly modify tile color because they are referenced by mask
		tc->Mask.a--;
		sprintf(buf, "%d", i + TILE_CLASS_WALL_OFFSET);
		if (hashmap_put(tileClasses, buf, tc) != MAP_OK)
		{
			LOG(LM_MAP, LL_ERROR, "failed to save tile class copy");
			break;
		}
	}

	GetCampaignPath(map->type, spearMission, buf);
	err = MapNewLoadArchive(buf, c);
	if (err != 0)
	{
		goto bail;
	}
	// Try to load campaign.json if available
	int numMissions = map->nLevels;
	if (MapLoadCampaignJSON(filename, c, NULL) == 0)
	{
		AdjustCampaignTitle(filename, &c->Title);
		MapNewScanArchive(filename, NULL, &numMissions);
	}

	CFREE(c->Description);
	const char *description = CWGetDescription(map, spearMission);
	CSTRDUP(c->Description, description);

	CharacterStoreCopy(&c->characters, &cs, &gPlayerTemplates.CustomClasses);

	// Special case for N3D: generate  scrolls with unique questions/answers
	if (map->type == CWMAPTYPE_N3D && map->nQuizzes > 0)
	{
		LoadN3DScrolls(map);
		IdxShufflerInit(&scrollShuffler, map->nQuizzes);
	}

	for (int i = 0; i < map->nLevels; i++)
	{
		LoadMission(
			c, tileClasses, map, spearMission, i, numMissions,
			&scrollShuffler);
	}

bail:
	if (err != 0)
	{
		CWFree(map);
	}
	hashmap_destroy(tileClasses, TileClassDestroy);
	CharacterStoreTerminate(&cs);
	IdxShufflerTerminate(&scrollShuffler);
	return err;
}

static Mix_Chunk *LoadSoundData(const CWolfMap *map, const int i);
static Mix_Chunk *LoadAdlibSoundData(const CWolfMap *map, const int i);
static void AddNormalSound(
	const SoundDevice *s, const char *name, Mix_Chunk *data);
static void AddRandomSound(
	const SoundDevice *s, const char *name, Mix_Chunk *data);
static void LoadSounds(const SoundDevice *s, const CWolfMap *map)
{
	if (!s->isInitialised)
	{
		return;
	}

	// Load adlib sounds
	for (int i = 0; i < map->audio.nSound; i++)
	{
		const char *name = GetAdlibSound(map->type, i);
		if (name == NULL)
		{
			continue;
		}
		Mix_Chunk *data = LoadAdlibSoundData(map, i);
		if (name[strlen(name) - 1] == '/')
		{
			AddRandomSound(s, name, data);
		}
		else
		{
			AddNormalSound(s, name, data);
		}
	}

	// Load digi sounds
	for (int i = 0; i < map->vswap.nSounds; i++)
	{
		const char *names = GetSound(map->type, i);
		if (names == NULL)
		{
			continue;
		}
		// Tokenise names
		char *namesCopy;
		CSTRDUP(namesCopy, names);
		char *name = strtok(namesCopy, "|");
		while (name != NULL)
		{
			Mix_Chunk *data = LoadSoundData(map, i);
			if (data == NULL)
			{
				continue;
			}
			if (name[strlen(name) - 1] == '/')
			{
				AddRandomSound(s, name, data);
			}
			else
			{
				AddNormalSound(s, name, data);
			}
			name = strtok(NULL, "|");
		}
		CFREE(namesCopy);
	}
}
static Mix_Chunk *LoadSoundData(const CWolfMap *map, const int i)
{
	const char *data;
	size_t len;
	const int err = CWVSwapGetSound(&map->vswap, i, &data, &len);
	if (err != 0)
	{
		LOG(LM_MAP, LL_ERROR, "Failed to load wolf sound %d: %d\n", i, err);
		return NULL;
	}
	if (len == 0)
	{
		LOG(LM_MAP, LL_ERROR, "Wolf sound %d has 0 len\n", i);
		return NULL;
	}
	SDL_AudioCVT cvt;
	SDL_BuildAudioCVT(
		&cvt, AUDIO_U8, 1, CWGetAudioSampleRate(map), CDOGS_SND_FMT,
		CDOGS_SND_CHANNELS, CDOGS_SND_RATE);
	cvt.len = (int)len;
	cvt.buf = (Uint8 *)SDL_malloc(cvt.len * cvt.len_mult);
	memcpy(cvt.buf, data, len);
	SDL_ConvertAudio(&cvt);
	return Mix_QuickLoad_RAW(cvt.buf, cvt.len_cvt);
}
static Mix_Chunk *LoadAdlibSoundData(const CWolfMap *map, const int i)
{
	char *data;
	size_t len;
	const int err = CWAudioGetAdlibSound(&map->audio, i, &data, &len);
	if (err != 0)
	{
		LOG(LM_MAP, LL_ERROR, "Failed to load adlib wolf sound %d: %d\n", i,
			err);
		return NULL;
	}
	if (len == 0)
	{
		LOG(LM_MAP, LL_ERROR, "Wolf sound %d has 0 len\n", i);
		return NULL;
	}
	return Mix_QuickLoad_RAW((Uint8 *)data, (Uint32)len);
}
static void AddNormalSound(
	const SoundDevice *s, const char *name, Mix_Chunk *data)
{
	SoundData *sound;
	CMALLOC(sound, sizeof *sound);
	sound->Type = SOUND_NORMAL;
	sound->u.normal = data;
	SoundAdd(s->customSounds, name, sound);
}
static void AddRandomSound(
	const SoundDevice *s, const char *name, Mix_Chunk *data)
{
	// Strip trailing slash and find the sound
	SoundData *sound;
	char nameBuf[CDOGS_PATH_MAX];
	strcpy(nameBuf, name);
	nameBuf[strlen(nameBuf) - 1] = '\0';
	const int err = hashmap_get(s->customSounds, nameBuf, (any_t *)&sound);
	if (err == MAP_OK && sound->Type == SOUND_RANDOM)
	{
		CArrayPushBack(&sound->u.random.sounds, &data);
	}
	else
	{
		CCALLOC(sound, sizeof *sound);
		sound->Type = SOUND_RANDOM;
		CArrayInit(&sound->u.random.sounds, sizeof(Mix_Chunk *));
		CArrayPushBack(&sound->u.random.sounds, &data);
		SoundAdd(s->customSounds, nameBuf, sound);
	}
}

static void LoadN3DScrolls(const CWolfMap *map)
{
	// Copy the effects from the "scroll" pickup
	const PickupClass *scroll = StrPickupClass("scroll");
	const PickupEffect *menuEffect = CArrayGet(&scroll->Effects, 0);
	const CArray *correctEffects =
		&((const PickupMenuItem *)CArrayGet(&menuEffect->u.Menu.Items, 0))
			 ->Effects;
	const CArray *wrongEffects =
		&((const PickupMenuItem *)CArrayGet(&menuEffect->u.Menu.Items, 1))
			 ->Effects;
	for (int i = 0; i < map->nQuizzes; i++)
	{
		PickupClass c;
		PickupClassInit(&c);
		char buf[256];
		sprintf(buf, "scroll+%d", i);
		CSTRDUP(c.Name, buf);
		CPicCopyPic(&c.Pic, &scroll->Pic);
		CSTRDUP(c.Sound, scroll->Sound);

		PickupEffect e;
		memset(&e, 0, sizeof e);
		e.Type = PICKUP_MENU;
		const CWN3DQuiz *quiz = &map->quizzes[i];
		CSTRDUP(e.u.Menu.Text, quiz->question);

		CArrayInit(&e.u.Menu.Items, sizeof(PickupMenuItem));
		for (int j = 0; j < quiz->nAnswers; j++)
		{
			PickupMenuItem m;
			PickupMenuItemInit(&m);
			CSTRDUP(m.Text, quiz->answers[j]);
			const CArray *effects =
				j == quiz->correctIdx ? correctEffects : wrongEffects;
			CA_FOREACH(const PickupEffect, pe, *effects)
			PickupEffect ec = PickupEffectCopy(pe);
			CArrayPushBack(&m.Effects, &ec);
			CA_FOREACH_END()
			CArrayPushBack(&e.u.Menu.Items, &m);
		}

		CArrayPushBack(&c.Effects, &e);

		CArrayPushBack(&gPickupClasses.CustomClasses, &c);
	}
}

static void LoadTile(
	MissionStatic *m, const uint16_t ch, const CWolfMap *map,
	const struct vec2i v, const int missionIndex);
static void TryLoadWallObject(
	MissionStatic *m, const uint16_t ch, const CWolfMap *map,
	const int spearMission, const struct vec2i v, const int missionIndex);
static void LoadEntity(
	Mission *m, const uint16_t ch, const CWolfMap *map, const int spearMission,
	const struct vec2i v, const int missionIndex, const int numMissions,
	int *bossObjIdx, int *spearObjIdx, IdxShuffler *scrollShuffler);

typedef struct
{
	CWolfMap *Map;
	int MissionIndex;
} MissionSongData;
static bool GetMissionSong(MusicChunk *chunk, void *data)
{
	MissionSongData *msd = data;
	return LoadMusic(
		msd->Map, chunk,
		CWAudioGetLevelMusic(msd->Map->type, msd->MissionIndex));
}
static void LoadMission(
	CampaignSetting *c, const map_t tileClasses, CWolfMap *map,
	const int spearMission, const int missionIndex, const int numMissions,
	IdxShuffler *scrollShuffler)
{
	const CWLevel *level = &map->levels[missionIndex];
	Mission m;
	MissionInit(&m);

	// If level has no player spawn it is a blank level, don't bother loading
	if (level->hasPlayerSpawn)
	{
		char titleBuf[17];
		titleBuf[16] = '\0';
		strncpy(titleBuf, level->header.name, 16);
		CSTRDUP(m.Title, titleBuf);
		m.Size = svec2i(level->header.width, level->header.height);
		m.Type = MAPTYPE_STATIC;
		strcpy(m.ExitStyle, "plate");
		strcpy(m.KeyStyle, "plain2");

		// TODO: objectives for treasure, kills (multiple items per obj)
		int bossObjIdx = -1;
		int spearObjIdx = -1;

		const WeaponClass *wc;
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			wc = StrWeaponClass("Small Launcher");
			break;
		default:
			wc = StrWeaponClass("Pistol");
			break;
		}
		CArrayPushBack(&m.Weapons, &wc);
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			wc = StrWeaponClass("Hand Feed");
			break;
		default:
			wc = StrWeaponClass("Knife");
			break;
		}
		CArrayPushBack(&m.Weapons, &wc);
		// Reset weapons at start of episodes
		switch (map->type)
		{
		case CWMAPTYPE_SOD:
			m.WeaponPersist = true;
			break;
		case CWMAPTYPE_N3D:
			m.WeaponPersist = true;
			break;
		default:
			m.WeaponPersist = (missionIndex % 10) != 0;
			break;
		}

		m.Music.Type = MUSIC_SRC_CHUNK;
		MissionSongData *msd;
		CMALLOC(msd, sizeof *msd);
		msd->Map = map;
		msd->MissionIndex = missionIndex;
		m.Music.Data.Chunk.Data = msd;
		m.Music.Data.Chunk.GetData = GetMissionSong;
		m.Music.Data.Chunk.isMusic = false;
		m.Music.Data.Chunk.u.Chunk = NULL;

		MissionStaticInit(&m.u.Static);

		m.u.Static.TileClasses =
			hashmap_copy(tileClasses, TileClassCopyHashMap);

		RECT_FOREACH(Rect2iNew(svec2i_zero(), m.Size))
		const uint16_t ch = CWLevelGetCh(level, 0, _v.x, _v.y);
		LoadTile(&m.u.Static, ch, map, _v, missionIndex);
		RECT_FOREACH_END()
		// Load objects after all tiles are loaded
		RECT_FOREACH(Rect2iNew(svec2i_zero(), m.Size))
		const uint16_t ch = CWLevelGetCh(level, 0, _v.x, _v.y);
		TryLoadWallObject(
			&m.u.Static, ch, map, spearMission, _v, missionIndex);
		const uint16_t ech = CWLevelGetCh(level, 1, _v.x, _v.y);
		LoadEntity(
			&m, ech, map, spearMission, _v, missionIndex, numMissions,
			&bossObjIdx, &spearObjIdx, scrollShuffler);
		RECT_FOREACH_END()

		if (m.u.Static.Exits.size == 0)
		{
			// This is a boss level where killing the boss ends the level
			// Make sure to skip over the secret level
			Exit e;
			e.Hidden = true;
			if (map->type == CWMAPTYPE_SOD)
			{
				if (missionIndex == 17)
				{
					// Skip over the two secret levels
					e.Mission = missionIndex + 3;
				}
				else
				{
					e.Mission = missionIndex + 1;
				}
			}
			else
			{
				// Skip over the secret level to the next episode
				e.Mission = missionIndex + 2;
			}
			e.R.Pos = svec2i_zero();
			e.R.Size = m.Size;
			CArrayPushBack(&m.u.Static.Exits, &e);
		}
		if (map->type == CWMAPTYPE_SOD && missionIndex == 17)
		{
			// Skip debrief and cut directly to angel boss level
			m.SkipDebrief = true;
		}

		m.u.Static.AltFloorsEnabled = false;
	}

	CArrayPushBack(&c->Missions, &m);
}

static int LoadWall(const uint16_t ch);
static bool IsElevator(const CWLevel *level, const struct vec2i v)
{
	if (v.x < 0 || v.x >= level->header.width || v.y < 0 ||
		v.y >= level->header.height)
	{
		return false;
	}
	const uint16_t ch = CWLevelGetCh(level, 0, v.x, v.y);
	const CWWall wall = CWChToWall(ch);
	return wall == CWWALL_ELEVATOR;
}

static void LoadTile(
	MissionStatic *m, const uint16_t ch, const CWolfMap *map,
	const struct vec2i v, const int missionIndex)
{
	UNUSED(missionIndex);
	const CWTile tile = CWChToTile(ch);
	int staticTile = 0;
	uint16_t staticAccess = 0;
	switch (tile)
	{
	case CWTILE_WALL:
		staticTile = LoadWall(ch);
		break;
	case CWTILE_DOOR_H:
	case CWTILE_DOOR_V:
		staticTile = 1;
		break;
	case CWTILE_DOOR_GOLD_H:
	case CWTILE_DOOR_GOLD_V:
		staticTile = 1;
		staticAccess = MAP_ACCESS_YELLOW;
		break;
	case CWTILE_DOOR_SILVER_H:
	case CWTILE_DOOR_SILVER_V:
		staticTile = 1;
		staticAccess = MAP_ACCESS_BLUE;
		break;
	case CWTILE_ELEVATOR_H:
	case CWTILE_ELEVATOR_V:
		staticTile = 2;
		break;
	case CWTILE_AREA:
		break;
	case CWTILE_BACKGROUND:
		// TODO: implement
		break;
	case CWTILE_SECRET_EXIT: {
		// Secret exits need an elevator tile on the left or right
		const struct vec2i vLeft = svec2i(v.x - 1, v.y);
		const struct vec2i vRight = svec2i(v.x + 1, v.y);
		const CWLevel *level = &map->levels[missionIndex];
		if (!IsElevator(level, vLeft) && !IsElevator(level, vRight))
		{
			break;
		}
		Exit e;
		e.Hidden = true;
		if (map->type == CWMAPTYPE_SOD)
		{
			// For SOD, missions 19/20 are always the secret ones
			if (missionIndex == 3)
			{
				e.Mission = 18;
			}
			else
			{
				e.Mission = 19;
			}
		}
		else
		{
			// Last map of the episode
			e.Mission = 9;
			while (e.Mission < missionIndex)
			{
				e.Mission += 10;
			}
		}
		e.R.Pos = v;
		e.R.Size = svec2i_zero();
		MissionStaticTryAddExit(m, &e);
	}
	break;
	default:
		break;
	}
	CArrayPushBack(&m->Tiles, &staticTile);
	CArrayPushBack(&m->Access, &staticAccess);
}

static int LoadWall(const uint16_t ch)
{
	const CWWall wall = CWChToWall(ch);
	if (wall == CWWALL_UNKNOWN)
	{
		return 0;
	}
	return (int)wall + 3;
}

static void TryLoadWallObject(
	MissionStatic *m, const uint16_t ch, const CWolfMap *map,
	const int spearMission, const struct vec2i v, const int missionIndex)
{
	const CWLevel *level = &map->levels[missionIndex];
	const struct vec2i levelSize =
		svec2i(level->header.width, level->header.height);
	const struct vec2i vBelow = svec2i_add(v, svec2i(0, 1));
	const CWWall wall = CWChToWall(ch);
	const char *moName = NULL;
	switch (wall)
	{
	case CWWALL_GREY_BRICK_2:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			moName = "wood_slime";
			break;
		default:
			switch (spearMission)
			{
			case 3:
				moName = "wall_light";
				break;
			}
			break;
		}
		break;
	case CWWALL_GREY_BRICK_FLAG:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			moName = "curtain_yellow";
			break;
		default:
			switch (spearMission)
			{
			case 1:
				moName = "heer_flag";
				break;
			case 2:
				moName = "wall_light";
				break;
			case 3:
				moName = "wall_light";
				break;
			}
		}
		break;
	case CWWALL_GREY_BRICK_HITLER:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			moName = "monkey_portrait";
			break;
		default:
			switch (spearMission)
			{
			case 1:
				moName = "hitler_portrait";
				break;
			case 3:
				moName = "no_sign";
				break;
			}
			break;
		}
		break;
	case CWWALL_CELL:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			break;
		default:
			switch (spearMission)
			{
			case 1:
				moName = "jail_cell";
				break;
			case 3:
				moName = "wall_goo2";
				break;
			}
			break;
		}
		break;
	case CWWALL_GREY_BRICK_EAGLE:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			moName = "noah_portrait";
			break;
		default:
			switch (spearMission)
			{
			case 1:
				moName = "brick_eagle";
				break;
			case 3:
				moName = "swastika_relief";
				break;
			}
			break;
		}
		break;
	case CWWALL_CELL_SKELETON:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			moName = "curtain_red";
			break;
		default:
			switch (spearMission)
			{
			case 1:
				moName = "jail_cell_skeleton";
				break;
			case 2:
				moName = "wall_light";
				break;
			case 3:
				moName = "map";
				break;
			}
			break;
		}
		break;
	case CWWALL_BLUE_BRICK_1:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			moName = "duck_picture";
			break;
		default:
			switch (spearMission)
			{
			case 2:
				moName = "ship_light";
				break;
			case 3:
				moName = "eagle_portrait";
				break;
			}
			break;
		}
		break;
	case CWWALL_BLUE_BRICK_2:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			moName = "wood_color";
			break;
		default:
			switch (spearMission)
			{
			case 2:
				moName = "hitler_poster";
				break;
			}
			break;
		}
		break;
	case CWWALL_WOOD_EAGLE:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			moName = "lattice_broken";
			break;
		default:
			switch (spearMission)
			{
			case 1:
				moName = "eagle_portrait";
				break;
			case 2:
				moName = "swastika_relief";
				break;
			case 3:
				moName = "swastika_relief";
				break;
			}
			break;
		}
		break;
	case CWWALL_WOOD_HITLER:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			break;
		default:
			switch (spearMission)
			{
			case 1:
				moName = "hitler_portrait";
				break;
			case 3:
				moName = "wall_light";
				break;
			}
			break;
		}
		break;
	case CWWALL_WOOD:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			moName = "jail_cell_broken";
			break;
		default:
			switch (spearMission)
			{
			case 2:
				moName = "wall_light";
				break;
			case 3:
				moName = "hitler_poster";
				break;
			}
			break;
		}
		break;
	case CWWALL_ENTRANCE:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			break;
		default:
			moName = "elevator_entrance";
			break;
		}
		break;
	case CWWALL_STEEL_SIGN:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			moName = "closet";
			break;
		default:
			switch (spearMission)
			{
			case 1:
				moName = "no_sign";
				break;
			case 2:
				moName = "wet_cobble";
				break;
			}
			break;
		}
		break;
	case CWWALL_STEEL:
		switch (map->type)
		{
		case CWMAPTYPE_N3D: {
			// Elevators only occur on east/west tiles
			for (int dx = -1; dx <= 1; dx += 2)
			{
				const struct vec2i exitV = svec2i(v.x + dx, v.y);
				const TileClass *tc =
					MissionStaticGetTileClass(m, levelSize, exitV);
				// Tile can be a vertical door
				const uint16_t chd = CWLevelGetCh(level, 0, exitV.x, exitV.y);
				const CWTile tile = CWChToTile(chd);
				const bool isVerticalDoor =
					tile == CWTILE_ELEVATOR_V || tile == CWTILE_DOOR_V ||
					tile == CWTILE_DOOR_GOLD_V || tile == CWTILE_DOOR_SILVER_V;
				if (tc != NULL &&
					(tc->Type == TILE_CLASS_FLOOR || isVerticalDoor))
				{
					Exit e;
					e.Hidden = true;
					e.Mission = missionIndex + 1;
					// Check if coming back from secret level
					// For boss levels, skip the secret level
					switch (missionIndex)
					{
					case 10:
						e.Mission++;
						break;
					case 11:
						e.Mission = 8;
						break;
					case 28:
						e.Mission++;
						break;
					case 29:
						e.Mission = 26;
						break;
					default:
						break;
					}
					e.R.Pos = exitV;
					e.R.Size = svec2i_zero();
					MissionStaticTryAddExit(m, &e);
					// Remove wall decorations from the wall
					MissionStaticTryRemoveItemAt(m, exitV);
					MissionStaticTryAddItem(m, StrMapObject("stairs"), exitV);
				}
			}
		}
		break;
		default:
			switch (spearMission)
			{
			case 3:
				moName = "no_sign";
				break;
			}
			break;
		}
		break;
	case CWWALL_RED_BRICK:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			break;
		default:
			switch (spearMission)
			{
			case 2:
				moName = "wall_chart";
				break;
			case 3:
				moName = "skull_wall";
				break;
			}
			break;
		}
		break;
	case CWWALL_RED_BRICK_SWASTIKA:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			moName = "naamah_portrait";
			break;
		default:
			switch (spearMission)
			{
			case 1:
				moName = "swastika_wreath";
				break;
			case 2:
				moName = "wall_nuke_sign";
				break;
			case 3:
				moName = "swastika_wall";
				break;
			}
			break;
		}
		break;
	case CWWALL_PURPLE:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			break;
		default:
			switch (spearMission)
			{
			case 2:
				moName = "heer_flag";
				break;
			}
			break;
		}
		break;
	case CWWALL_RED_BRICK_FLAG:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			moName = "noah_flag";
			break;
		default:
			switch (spearMission)
			{
			case 1:
				moName = "coat_of_arms_flag";
				break;
			case 3:
				moName = "swastika_relief";
				break;
			}
			break;
		}
		break;
	case CWWALL_ELEVATOR: {
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			break;
		default:
			// Elevators only occur on east/west tiles
			for (int dx = -1; dx <= 1; dx += 2)
			{
				const struct vec2i exitV = svec2i(v.x + dx, v.y);
				const TileClass *tc =
					MissionStaticGetTileClass(m, levelSize, exitV);
				// Tile can be a vertical door
				const uint16_t chd = CWLevelGetCh(level, 0, exitV.x, exitV.y);
				const CWTile tile = CWChToTile(chd);
				const bool isVerticalDoor =
					tile == CWTILE_ELEVATOR_V || tile == CWTILE_DOOR_V ||
					tile == CWTILE_DOOR_GOLD_V || tile == CWTILE_DOOR_SILVER_V;
				if (tc != NULL &&
					(tc->Type == TILE_CLASS_FLOOR || isVerticalDoor))
				{
					Exit e;
					e.Hidden = true;
					e.Mission = missionIndex + 1;
					// Check if coming back from secret level
					if (map->type == CWMAPTYPE_SOD)
					{
						if (missionIndex == 18)
						{
							e.Mission = 4;
						}
						else if (missionIndex == 19)
						{
							e.Mission = 12;
						}
					}
					else
					{
						switch (missionIndex)
						{
						case 9:
							e.Mission = 1;
							break;
						case 19:
							e.Mission = 11;
							break;
						case 29:
							e.Mission = 27;
							break;
						case 39:
							e.Mission = 33;
							break;
						case 49:
							e.Mission = 45;
							break;
						case 59:
							e.Mission = 53;
							break;
						default:
							break;
						}
					}
					e.R.Pos = exitV;
					e.R.Size = svec2i_zero();
					MissionStaticTryAddExit(m, &e);
				}
			}
		}
		break;
	}
	case CWWALL_DEAD_ELEVATOR:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			moName = "curtain_blue";
			break;
		default:
			if (MissionStaticGetTileClass(
					m, svec2i(level->header.width, level->header.height),
					vBelow)
					->Type == TILE_CLASS_FLOOR)
			{
				moName = "elevator_interior";
			}
			break;
		}
		break;
	case CWWALL_WOOD_IRON_CROSS:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			break;
		default:
			switch (spearMission)
			{
			case 1:
				moName = "iron_cross";
				break;
			case 2:
				moName = "wall_chart";
				break;
			case 3:
				moName = "wall_goo2";
				break;
			}
			break;
		}
		break;
	case CWWALL_DIRTY_BRICK_1:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			moName = "green_relief";
			break;
		default:
			switch (spearMission)
			{
			case 1:
				moName = "cobble_moss";
				break;
			case 3:
				moName = "wall_goo2";
				break;
			}
			break;
		}
		break;
	case CWWALL_PURPLE_BLOOD:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			break;
		default:
			switch (spearMission)
			{
			case 1:
				moName = "bloodstain";
				break;
			case 2:
				moName = "no_sign";
				break;
			case 3:
				moName = "wall_goo2";
				break;
			}
			break;
		}
		break;
	case CWWALL_DIRTY_BRICK_2:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			moName = "closet_fruit";
			break;
		default:
			switch (spearMission)
			{
			case 1:
				moName = "cobble_moss";
				break;
			case 2:
				moName = "jail_cell";
				break;
			}
			break;
		}
		break;
	case CWWALL_GREY_BRICK_3:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			break;
		default:
			switch (spearMission)
			{
			case 2:
				moName = "jail_cell_skeleton";
				break;
			case 3:
				moName = "swastika_blue";
				break;
			}
			break;
		}
		break;
	case CWWALL_GREY_BRICK_SIGN:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			moName = "closets";
			break;
		default:
			switch (spearMission)
			{
			case 1:
				moName = "no_sign";
				break;
			case 2:
				moName = "no_sign";
				break;
			case 3:
				moName = "skull_blue";
				break;
			}
			break;
		}
		break;
	case CWWALL_BROWN_WEAVE_BLOOD_2:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			break;
		default:
			switch (spearMission)
			{
			case 1:
				moName = "bloodstain";
				break;
			case 2:
				moName = "skull_blue";
				break;
			case 3:
				moName = "swastika_relief";
				break;
			}
			break;
		}
		break;
	case CWWALL_BROWN_WEAVE_BLOOD_3:
		switch (spearMission)
		{
		case 1:
			moName = "bloodstain1";
			break;
		case 2:
			moName = "swastika_blue";
			break;
		}
		break;
	case CWWALL_BROWN_WEAVE_BLOOD_1:
		switch (spearMission)
		{
		case 1:
			moName = "bloodstain2";
			break;
		}
		break;
	case CWWALL_STAINED_GLASS:
		switch (spearMission)
		{
		case 1:
			moName = "hitler_glass";
			break;
		case 2:
			moName = "swastika_relief";
			break;
		case 3:
			moName = "wall_light";
			break;
		}
		break;
	case CWWALL_BLUE_WALL_SKULL:
		switch (spearMission)
		{
		case 1:
			moName = "skull_blue";
			break;
		case 3:
			moName = "wall_light";
			break;
		}
		break;
	case CWWALL_GREY_WALL_1:
		switch (spearMission)
		{
		case 2:
			moName = "map";
			break;
		case 3:
			moName = "wall_light";
			break;
		}
		break;
	case CWWALL_BLUE_WALL_SWASTIKA:
		switch (spearMission)
		{
		case 1:
			moName = "swastika_blue";
			break;
		}
		break;
	case CWWALL_GREY_WALL_VENT:
		switch (spearMission)
		{
		case 1:
			moName = "wall_vent";
			break;
		case 2:
			moName = "no_sign";
			break;
		case 3:
			moName = "no_sign";
			break;
		}
		break;
	case CWWALL_MULTICOLOR_BRICK:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			break;
		default:
			switch (spearMission)
			{
			case 1:
				moName = "brick_color";
				break;
			case 3:
				moName = "jail_cell";
				break;
			}
			break;
		}
		break;
	case CWWALL_GREY_WALL_2:
		switch (spearMission)
		{
		case 2:
			moName = "bulletmarks";
			break;
		case 3:
			moName = "jail_cell_skeleton";
			break;
		}
		break;
	case CWWALL_BLUE_WALL:
		switch (spearMission)
		{
		case 2:
			moName = "ship_picture";
			break;
		case 3:
			moName = "no_sign";
			break;
		}
		break;
	case CWWALL_BLUE_BRICK_SIGN:
		switch (spearMission)
		{
		case 1:
			moName = "no_sign";
			break;
		case 2:
			moName = "hitler_poster";
			break;
		case 3:
			moName = "wet_cobble";
			break;
		}
		break;
	case CWWALL_BROWN_MARBLE_1:
		switch (spearMission)
		{
		case 2:
			moName = "wall_vent";
			break;
		case 3:
			moName = "wet_cobble";
			break;
		}
		break;
	case CWWALL_GREY_WALL_MAP:
		switch (spearMission)
		{
		case 1:
			moName = "map";
			break;
		case 2:
			moName = "heer_flag";
			break;
		}
		break;
	case CWWALL_BROWN_STONE_2:
		switch (spearMission)
		{
		case 2:
			moName = "swastika_relief";
			break;
		case 3:
			moName = "wscreen1";
			break;
		}
		break;
	case CWWALL_BROWN_MARBLE_2:
		switch (spearMission)
		{
		case 2:
			moName = "eagle_portrait";
			break;
		}
		break;
	case CWWALL_BROWN_MARBLE_FLAG:
		switch (spearMission)
		{
		case 1:
			moName = "heer_flag";
			break;
		case 2:
			moName = "green_relief";
			break;
		case 3:
			moName = "wall_vent";
			break;
		}
		break;
	case CWWALL_WOOD_PANEL:
		switch (spearMission)
		{
		case 1:
			moName = "panel";
			break;
		case 2:
			moName = "scratch";
			break;
		case 3:
			moName = "bulletmarks";
			break;
		}
		break;
	case CWWALL_GREY_WALL_HITLER:
		switch (spearMission)
		{
		case 1:
			moName = "hitler_poster";
			break;
		case 2:
			moName = "wall_goo2";
			break;
		case 3:
			moName = "hitler_poster";
			break;
		}
		break;
	case CWWALL_STONE_WALL_1:
		switch (spearMission)
		{
		case 1:
			moName = "stone_color";
			break;
		case 3:
			moName = "ship_picture";
			break;
		}
		break;
	case CWWALL_STONE_WALL_2:
		switch (spearMission)
		{
		case 1:
			moName = "stone_color";
			break;
		case 2:
			moName = "scratch";
			break;
		}
		break;
	case CWWALL_STONE_WALL_FLAG:
		switch (spearMission)
		{
		case 1:
			moName = "heer_flag";
			break;
		case 2:
			moName = "iron_cross";
			break;
		case 3:
			moName = "swastika_relief";
			break;
		}
		break;
	case CWWALL_STONE_WALL_WREATH:
		switch (spearMission)
		{
		case 1:
			moName = "swastika_wreath";
			break;
		case 2:
			moName = "wall_goo2";
			break;
		case 3:
			moName = "eagle_portrait";
			break;
		}
		break;
	case CWWALL_GREY_CONCRETE_LIGHT:
		switch (spearMission)
		{
		case 2:
			moName = "wall_nuke_sign";
			break;
		}
		break;
	case CWWALL_GREY_CONCRETE_DARK:
		switch (spearMission)
		{
		case 2:
			moName = "wall_light";
			break;
		case 3:
			moName = "wall_light";
			break;
		}
		break;
	case CWWALL_BLOOD_WALL:
		switch (spearMission)
		{
		case 2:
			moName = "scratch";
			break;
		case 3:
			moName = "green_relief";
			break;
		}
		break;
	case CWWALL_CONCRETE:
		switch (spearMission)
		{
		case 3:
			moName = "red_relief";
			break;
		}
		break;
	case CWWALL_RAMPART_STONE_1:
		switch (spearMission)
		{
		case 1:
			moName = "stone_color";
			break;
		case 2:
			moName = "no_sign";
			break;
		case 3:
			moName = "blue_relief";
			break;
		}
		break;
	case CWWALL_RAMPART_STONE_2:
		switch (spearMission)
		{
		case 1:
			moName = "stone_color";
			break;
		case 3:
			moName = "wscreen2";
			break;
		}
		break;
	case CWWALL_ELEVATOR_WALL:
		moName = "elevator_interior";
		break;
	case CWWALL_WHITE_PANEL:
		switch (spearMission)
		{
		case 3:
			moName = "wall_goo2";
			break;
		}
		break;
	default:
		break;
	}
	if (moName != NULL)
	{
		MissionStaticTryAddItem(m, StrMapObject(moName), vBelow);
	}
}

typedef enum
{
	CHAR_GUARD = 1,
	CHAR_DOG,
	CHAR_SS,
	CHAR_MUTANT,
	CHAR_OFFICER,
	CHAR_PACMAN_GHOST_RED,
	CHAR_PACMAN_GHOST_YELLOW,
	CHAR_PACMAN_GHOST_ROSE,
	CHAR_PACMAN_GHOST_BLUE,
	// 10
	CHAR_HANS,
	CHAR_SCHABBS,
	CHAR_FAKE_HITLER,
	CHAR_MECHA_HITLER,
	CHAR_HITLER,
	CHAR_OTTO,
	CHAR_GRETEL,
	CHAR_FETTGESICHT,
	CHAR_TRANS,
	CHAR_WILHELM,
	// 20
	CHAR_UBERMUTANT,
	CHAR_DEATH_KNIGHT,
	CHAR_GHOST,
	CHAR_ANGEL,
} WolfChar;

typedef enum
{
	CHAR_RAM = 1,
	CHAR_GOAT,
	CHAR_OSTRICH,
	CHAR_ANTELOPE,
	CHAR_BULL,
	CHAR_CAMEL,
	CHAR_GIRAFFE,
	CHAR_BEAR_BUSH,
	CHAR_BEAR,
	// 10
	CHAR_MONKEY,
	CHAR_KANGAROO,
	CHAR_ELEPHANT,
} N3DChar;

static bool MakeWallWalkable(Mission *m, const struct vec2i v);
static void LoadChar(
	Mission *m, const struct vec2i v, const direction_e d, const int charId,
	const bool moving, int *bossObjIdx);
static void AdjustTurningPoint(Mission *m, const struct vec2i v);
static void LoadEntity(
	Mission *m, const uint16_t ch, const CWolfMap *map, const int spearMission,
	const struct vec2i v, const int missionIndex, const int numMissions,
	int *bossObjIdx, int *spearObjIdx, IdxShuffler *scrollShuffler)
{
	const CWEntity entity = CWChToEntity(ch);
	switch (entity)
	{
	case CWENT_NONE:
		break;
	case CWENT_PLAYER_SPAWN_N:
	case CWENT_PLAYER_SPAWN_E:
	case CWENT_PLAYER_SPAWN_S:
	case CWENT_PLAYER_SPAWN_W:
		m->u.Static.Start = v;
		// Remove any exits that overlap with start
		// SOD starts the player in elevators
		CA_FOREACH(const Exit, e, m->u.Static.Exits)
		const Rect2i er =
			Rect2iNew(e->R.Pos, svec2i_add(e->R.Size, svec2i_one()));
		if (Rect2iIsInside(er, v))
		{
			CArrayDelete(&m->u.Static.Exits, _ca_index);
			_ca_index--;
		}
		CA_FOREACH_END()
		if (map->type == CWMAPTYPE_SOD && missionIndex == 20)
		{
			// Start the mission with a yellow key
			MissionStaticAddKey(&m->u.Static, 0, v);
		}
		break;
	case CWENT_WATER:
		MissionStaticTryAddItem(&m->u.Static, StrMapObject("pool_water"), v);
		break;
	case CWENT_OIL_DRUM:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			MissionStaticTryAddItem(
				&m->u.Static, StrMapObject("well_water"), v);
			break;
		default:
			MissionStaticTryAddItem(
				&m->u.Static, StrMapObject("barrel_green"), v);
			break;
		}
		break;
	case CWENT_TABLE_WITH_CHAIRS:
		MissionStaticTryAddItem(
			&m->u.Static, StrMapObject("table_and_chairs"), v);
		break;
	case CWENT_FLOOR_LAMP:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			MissionStaticTryAddItem(
				&m->u.Static, StrMapObject("barrel_wood"), v);
			break;
		default:
			MissionStaticTryAddItem(
				&m->u.Static, StrMapObject("rod_light"), v);
			MissionStaticTryAddItem(
				&m->u.Static, StrMapObject("spotlight"), v);
		}
		break;
	case CWENT_CHANDELIER:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			MissionStaticTryAddItem(
				&m->u.Static, StrMapObject("bookshelf2_wreck"), v);
			break;
		default:
			MissionStaticTryAddItem(
				&m->u.Static, StrMapObject("chandelier"), v);
			MissionStaticTryAddItem(
				&m->u.Static, StrMapObject("spotlight"), v);
			break;
		}
		break;
	case CWENT_HANGING_SKELETON:
		switch (map->type)
		{
		case CWMAPTYPE_N3D: {
			// Place a random scroll pickup, use shuffling
			char buf[256];
			sprintf(buf, "scroll+%d", IdxShufflerDraw(scrollShuffler));
			MissionStaticTryAddPickup(&m->u.Static, StrPickupClass(buf), v);
		}
		break;
		default:
			MissionStaticTryAddItem(
				&m->u.Static, StrMapObject("hanging_skeleton"), v);
			MissionStaticTryAddItem(&m->u.Static, StrMapObject("shadow"), v);
			break;
		}
		break;
	case CWENT_DOG_FOOD:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			MissionStaticTryAddItem(
				&m->u.Static, StrMapObject("well_feed"), v);
			break;
		default:
			MissionStaticTryAddPickup(
				&m->u.Static, StrPickupClass("dogfood"), v);
			break;
		}
		break;
	case CWENT_WHITE_COLUMN:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			MissionStaticTryAddItem(
				&m->u.Static, StrMapObject("pillar_candle"), v);
			break;
		default:
			MissionStaticTryAddItem(&m->u.Static, StrMapObject("pillar"), v);
			break;
		}
		break;
	case CWENT_GREEN_PLANT:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			MissionStaticTryAddItem(&m->u.Static, StrMapObject("pillar2"), v);
			break;
		default:
			MissionStaticTryAddItem(&m->u.Static, StrMapObject("plant"), v);
			break;
		}
		break;
	case CWENT_SKELETON:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			MissionStaticTryAddItem(&m->u.Static, StrMapObject("birdcage"), v);
			break;
		default:
			MissionStaticTryAddItem(
				&m->u.Static, StrMapObject("bone_blood"), v);
			break;
		}
		break;
	case CWENT_SINK_SKULLS_ON_STICK:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			MissionStaticTryAddItem(
				&m->u.Static, StrMapObject("water_pitcher"), v);
			break;
		case CWMAPTYPE_SOD:
			MissionStaticTryAddItem(
				&m->u.Static, StrMapObject("skull_pillar"), v);
			break;
		default:
			MissionStaticTryAddItem(&m->u.Static, StrMapObject("sink"), v);
			break;
		}
		break;
	case CWENT_BROWN_PLANT:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			MissionStaticTryAddItem(
				&m->u.Static, StrMapObject("water_pitcher_wreck"), v);
			break;
		default:
			MissionStaticTryAddItem(
				&m->u.Static, StrMapObject("plant_brown"), v);
			break;
		}
		break;
	case CWENT_VASE:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			MissionStaticTryAddItem(&m->u.Static, StrMapObject("beam"), v);
			MissionStaticTryAddItem(&m->u.Static, StrMapObject("shadow"), v);
			break;
		default:
			MissionStaticTryAddItem(&m->u.Static, StrMapObject("urn"), v);
			break;
		}
		break;
	case CWENT_TABLE:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			MissionStaticTryAddItem(
				&m->u.Static, StrMapObject("chandelier"), v);
			MissionStaticTryAddItem(&m->u.Static, StrMapObject("shadow"), v);
			break;
		default:
			switch (spearMission)
			{
			case 1:
				MissionStaticTryAddItem(
					&m->u.Static, StrMapObject("table_wood_round"), v);
				break;
			case 2:
				MissionStaticTryAddItem(
					&m->u.Static, StrMapObject("table_plastic"), v);
				break;
			case 3:
				break;
			}
		}
		break;
	case CWENT_CEILING_LIGHT_GREEN:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			MissionStaticTryAddItem(
				&m->u.Static, StrMapObject("water_trough"), v);
			break;
		default:
			MissionStaticTryAddItem(
				&m->u.Static, StrMapObject("spotlight"), v);
			break;
		}
		break;
	case CWENT_UTENSILS_BROWN_CAGE_BLOODY_BONES:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			MissionStaticTryAddItem(&m->u.Static, StrMapObject("candle"), v);
			break;
		case CWMAPTYPE_SOD:
			MissionStaticTryAddItem(
				&m->u.Static, StrMapObject("gibbet_bloody"), v);
			MissionStaticTryAddItem(&m->u.Static, StrMapObject("shadow"), v);
			break;
		default:
			MissionStaticTryAddItem(&m->u.Static, StrMapObject("knives"), v);
			break;
		}
		break;
	case CWENT_ARMOR:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			MissionStaticAddKey(&m->u.Static, 0, v);
			break;
		default:
			MissionStaticTryAddItem(
				&m->u.Static, StrMapObject("suit_of_armor"), v);
			break;
		}
		break;
	case CWENT_CAGE:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			MissionStaticAddKey(&m->u.Static, 2, v);
			break;
		default:
			MissionStaticTryAddItem(&m->u.Static, StrMapObject("gibbet"), v);
			MissionStaticTryAddItem(&m->u.Static, StrMapObject("shadow"), v);
			break;
		}
		break;
	case CWENT_CAGE_SKELETON:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			MissionStaticTryAddPickup(
				&m->u.Static, StrPickupClass("knapsack"), v);
			break;
		default:
			MissionStaticTryAddItem(
				&m->u.Static, StrMapObject("gibbet_skeleton"), v);
			MissionStaticTryAddItem(&m->u.Static, StrMapObject("shadow"), v);
			break;
		}
		break;
	case CWENT_BONES1:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			MissionStaticTryAddPickup(
				&m->u.Static, StrPickupClass("large_feed_box"), v);
			break;
		default:
			MissionStaticTryAddItem(&m->u.Static, StrMapObject("skull"), v);
			break;
		}
		break;
	case CWENT_KEY_GOLD:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			MissionStaticTryAddPickup(
				&m->u.Static, StrPickupClass("small_bandage"), v);
			break;
		default:
			MissionStaticAddKey(&m->u.Static, 0, v);
			break;
		}
		break;
	case CWENT_KEY_SILVER:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			MissionStaticTryAddPickup(
				&m->u.Static, StrPickupClass("large_bandage"), v);
			break;
		default:
			MissionStaticAddKey(&m->u.Static, 2, v);
			break;
		}
		break;
	case CWENT_BED_CAGE_SKULLS:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			MissionStaticTryAddPickup(
				&m->u.Static, StrPickupClass("small_feed_box"), v);
			break;
		case CWMAPTYPE_SOD:
			MissionStaticTryAddItem(
				&m->u.Static, StrMapObject("gibbet_skulls"), v);
			MissionStaticTryAddItem(&m->u.Static, StrMapObject("shadow"), v);
			break;
		default:
			MissionStaticTryAddItem(&m->u.Static, StrMapObject("bed"), v);
			break;
		}
		break;
	case CWENT_BASKET:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			MissionStaticTryAddPickup(
				&m->u.Static, StrPickupClass("gun_Large Launcher"), v);
			break;
		default:
			MissionStaticTryAddItem(&m->u.Static, StrMapObject("basket"), v);
			break;
		}
		break;
	case CWENT_FOOD:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			MissionStaticTryAddPickup(
				&m->u.Static, StrPickupClass("gun_Super Feeder"), v);
			break;
		default:
			MissionStaticTryAddPickup(&m->u.Static, StrPickupClass("meal"), v);
			break;
		}
		break;
	case CWENT_MEDKIT:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			MissionStaticTryAddPickup(
				&m->u.Static, StrPickupClass("banana"), v);
			break;
		default:
			MissionStaticTryAddPickup(
				&m->u.Static, StrPickupClass("medkit"), v);
			break;
		}
		break;
	case CWENT_AMMO:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			MissionStaticTryAddPickup(
				&m->u.Static, StrPickupClass("apple"), v);
			break;
		default:
			MissionStaticTryAddPickup(
				&m->u.Static, StrPickupClass("ammo_clip"), v);
			break;
		}
		break;
	case CWENT_MACHINE_GUN:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			MissionStaticTryAddPickup(
				&m->u.Static, StrPickupClass("grapes"), v);
			break;
		default:
			MissionStaticTryAddPickup(
				&m->u.Static, StrPickupClass("gun_Machine Gun"), v);
			break;
		}
		break;
	case CWENT_CHAIN_GUN:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			MissionStaticTryAddPickup(
				&m->u.Static, StrPickupClass("orange"), v);
			break;
		default:
			MissionStaticTryAddPickup(
				&m->u.Static, StrPickupClass("gun_Chain Gun"), v);
			break;
		}
		break;
	case CWENT_CROSS:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			MissionStaticTryAddPickup(
				&m->u.Static, StrPickupClass("heart"), v);
			break;
		default:
			MissionStaticTryAddPickup(
				&m->u.Static, StrPickupClass("cross"), v);
			break;
		}
		break;
	case CWENT_CHALICE:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			MissionStaticTryAddItem(&m->u.Static, StrMapObject("cage"), v);
			break;
		default:
			MissionStaticTryAddPickup(
				&m->u.Static, StrPickupClass("chalice"), v);
			break;
		}
		break;
	case CWENT_CHEST:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			MissionStaticTryAddItem(&m->u.Static, StrMapObject("pillar"), v);
			break;
		default:
			MissionStaticTryAddPickup(
				&m->u.Static, StrPickupClass("chest"), v);
			break;
		}
		break;
	case CWENT_CROWN:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			MissionStaticTryAddPickup(
				&m->u.Static, StrPickupClass("gun_Cantaloupe Feeder"), v);
			break;
		default:
			MissionStaticTryAddPickup(
				&m->u.Static, StrPickupClass("crown"), v);
			break;
		}
		break;
	case CWENT_LIFE:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			MissionStaticTryAddPickup(
				&m->u.Static, StrPickupClass("ammo_Cantaloupe"), v);
			break;
		default:
			MissionStaticTryAddPickup(
				&m->u.Static, StrPickupClass("heart"), v);
			break;
		}
		break;
	case CWENT_BONES_BLOOD:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			MissionStaticTryAddPickup(
				&m->u.Static, StrPickupClass("ammo_Watermelon"), v);
			break;
		default:
			MissionStaticTryAddItem(&m->u.Static, StrMapObject("gibs"), v);
			break;
		}
		break;
	case CWENT_BARREL:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			MissionStaticTryAddPickup(
				&m->u.Static, StrPickupClass("ammo_Watermelon"), v);
			break;
		default:
			MissionStaticTryAddItem(
				&m->u.Static, StrMapObject("barrel_wood2"), v);
			break;
		}
		break;
	case CWENT_WELL_WATER:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			MissionStaticTryAddPickup(&m->u.Static, StrPickupClass("map"), v);
			break;
		default:
			MissionStaticTryAddItem(
				&m->u.Static, StrMapObject("well_water"), v);
			break;
		}
		break;
	case CWENT_WELL:
		MissionStaticTryAddItem(&m->u.Static, StrMapObject("well"), v);
		break;
	case CWENT_POOL_OF_BLOOD:
		MissionStaticTryAddItem(&m->u.Static, StrMapObject("pool_blood"), v);
		break;
	case CWENT_FLAG:
		switch (spearMission)
		{
		case 1:
			MissionStaticTryAddItem(&m->u.Static, StrMapObject("flag"), v);
			break;
		case 2:
			MissionStaticTryAddItem(&m->u.Static, StrMapObject("rod"), v);
			break;
		case 3:
			break;
		}
		break;
	case CWENT_CEILING_LIGHT_RED_AARDWOLF:
		if (map->type == CWMAPTYPE_WL6)
		{
			MissionStaticTryAddItem(
				&m->u.Static, StrMapObject("spotlight"), v);
		}
		else
		{
			switch (spearMission)
			{
			case 1:
				MissionStaticTryAddItem(
					&m->u.Static, StrMapObject("skull2"), v);
				break;
			case 2:
				MissionStaticTryAddItem(
					&m->u.Static, StrMapObject("spotlight"), v);
				break;
			case 3:
				break;
			}
		}
		break;
	case CWENT_BONES2:
		MissionStaticTryAddItem(&m->u.Static, StrMapObject("bones"), v);
		break;
	case CWENT_BONES3:
		MissionStaticTryAddItem(&m->u.Static, StrMapObject("bones2"), v);
		break;
	case CWENT_BONES4:
		switch (spearMission)
		{
		case 1:
			MissionStaticTryAddItem(&m->u.Static, StrMapObject("bones3"), v);
			break;
		case 2:
			MissionStaticTryAddItem(&m->u.Static, StrMapObject("goo"), v);
			break;
		case 3:
			break;
		}
		break;
	case CWENT_UTENSILS_BLUE_COW_SKULL:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			break;
		case CWMAPTYPE_SOD:

			switch (spearMission)
			{
			case 1:
				MissionStaticTryAddItem(
					&m->u.Static, StrMapObject("cowskull_pillar"), v);
				break;
			case 2:
				MissionStaticTryAddItem(
					&m->u.Static, StrMapObject("table_lab"), v);
				break;
			case 3:
				break;
			}
			break;
		default:
			MissionStaticTryAddItem(&m->u.Static, StrMapObject("pots"), v);
			break;
		}
		break;
	case CWENT_STOVE_WELL_BLOOD:
		if (map->type == CWMAPTYPE_SOD)
		{
			switch (spearMission)
			{
			case 1:
				MissionStaticTryAddItem(
					&m->u.Static, StrMapObject("well_blood"), v);
				break;
			case 2:
				MissionStaticTryAddItem(
					&m->u.Static, StrMapObject("barrel"), v);
				break;
			case 3:
				break;
			}
		}
		else
		{
			MissionStaticTryAddItem(&m->u.Static, StrMapObject("stove"), v);
		}
		break;
	case CWENT_RACK_ANGEL_STATUE:
		if (map->type == CWMAPTYPE_SOD)
		{
			switch (spearMission)
			{
			case 1:
				MissionStaticTryAddItem(
					&m->u.Static, StrMapObject("statue_behemoth"), v);
				break;
			case 2:
				MissionStaticTryAddItem(
					&m->u.Static, StrMapObject("pillar_blue"), v);
				break;
			case 3:
				break;
			}
		}
		else
		{
			MissionStaticTryAddItem(&m->u.Static, StrMapObject("spears"), v);
		}
		break;
	case CWENT_VINES:
		switch (spearMission)
		{
		case 1:
			MissionStaticTryAddItem(&m->u.Static, StrMapObject("grass"), v);
			break;
		case 2:
			MissionStaticTryAddItem(&m->u.Static, StrMapObject("sprites"), v);
			break;
		case 3:
			break;
		}
		break;
	case CWENT_BROWN_COLUMN:
		switch (spearMission)
		{
		case 1:
			MissionStaticTryAddItem(
				&m->u.Static, StrMapObject("pillar_brown"), v);
			break;
		case 2:
			MissionStaticTryAddItem(
				&m->u.Static, StrMapObject("statue_behemoth"), v);
			break;
		case 3:
			break;
		}
		break;
	case CWENT_AMMO_BOX:
		MissionStaticTryAddPickup(&m->u.Static, StrPickupClass("ammo_box"), v);
		break;
	case CWENT_TRUCK_REAR:
		MissionStaticTryAddItem(&m->u.Static, StrMapObject("truck"), v);
		break;
	case CWENT_SPEAR:
		if (*spearObjIdx < 0)
		{
			Objective o;
			memset(&o, 0, sizeof o);
			o.Type = OBJECTIVE_COLLECT;
			ObjectiveSetPickup(&o, StrPickupClass("spear"));
			CSTRDUP(o.Description, "Collect spear");
			CArrayPushBack(&m->Objectives, &o);
			*spearObjIdx = (int)m->Objectives.size - 1;
		}
		Objective *spearObj = CArrayGet(&m->Objectives, *spearObjIdx);
		spearObj->Required++;
		MissionStaticAddObjective(m, &m->u.Static, *spearObjIdx, 0, v, false);
		break;
	case CWENT_PUSHWALL:
		MakeWallWalkable(m, v);
		break;
	case CWENT_ENDGAME:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			MakeWallWalkable(m, v);
			break;
		default: {
			Exit e;
			e.Hidden = true;
			// Skip over the secret level to the next episode
			e.Mission = (missionIndex + 10) / 10 * 10;
			// Skip to end of game if the next episode is blank, or if the
			// campaign should have less levels
			if ((e.Mission < map->nLevels &&
				 !map->levels[e.Mission].hasPlayerSpawn) ||
				e.Mission >= numMissions)
			{
				e.Mission = map->nLevels;
			}
			e.R.Pos = v;
			e.R.Size = svec2i_zero();
			MissionStaticTryAddExit(&m->u.Static, &e);
		}
		break;
		}
		break;
	case CWENT_NEXT_LEVEL:
		// TODO: implement
		break;
	case CWENT_SECRET_LEVEL:
		// TODO: implement
		break;
	case CWENT_GHOST:
		LoadChar(m, v, DIRECTION_DOWN, (int)CHAR_GHOST, false, bossObjIdx);
		break;
	case CWENT_ANGEL:
		LoadChar(m, v, DIRECTION_DOWN, (int)CHAR_ANGEL, false, bossObjIdx);
		break;
	case CWENT_DEAD_GUARD:
		MissionStaticTryAddItem(&m->u.Static, StrMapObject("dead_guard"), v);
		// holowall
		MakeWallWalkable(m, v);
		break;
	case CWENT_DOG_E:
		LoadChar(m, v, DIRECTION_RIGHT, (int)CHAR_DOG, false, bossObjIdx);
		break;
	case CWENT_DOG_N:
		LoadChar(m, v, DIRECTION_UP, (int)CHAR_DOG, false, bossObjIdx);
		break;
	case CWENT_DOG_W:
		LoadChar(m, v, DIRECTION_LEFT, (int)CHAR_DOG, false, bossObjIdx);
		break;
	case CWENT_DOG_S:
		LoadChar(m, v, DIRECTION_DOWN, (int)CHAR_DOG, false, bossObjIdx);
		break;
	case CWENT_GUARD_E:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			LoadChar(m, v, DIRECTION_DOWN, (int)CHAR_RAM, false, bossObjIdx);
			break;
		default:
			LoadChar(
				m, v, DIRECTION_RIGHT, (int)CHAR_GUARD, false, bossObjIdx);
			break;
		}
		break;
	case CWENT_GUARD_N:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			LoadChar(
				m, v, DIRECTION_DOWN, (int)CHAR_OSTRICH, false, bossObjIdx);
			break;
		default:
			LoadChar(m, v, DIRECTION_UP, (int)CHAR_GUARD, false, bossObjIdx);
			break;
		}
		break;
	case CWENT_GUARD_W:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			LoadChar(
				m, v, DIRECTION_DOWN, (int)CHAR_ANTELOPE, false, bossObjIdx);
			break;
		default:
			LoadChar(m, v, DIRECTION_LEFT, (int)CHAR_GUARD, false, bossObjIdx);
			break;
		}
		break;
	case CWENT_GUARD_S:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			LoadChar(m, v, DIRECTION_DOWN, (int)CHAR_GOAT, false, bossObjIdx);
			break;
		default:
			LoadChar(m, v, DIRECTION_DOWN, (int)CHAR_GUARD, false, bossObjIdx);
			break;
		}
		break;
	case CWENT_SS_E:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			LoadChar(m, v, DIRECTION_DOWN, (int)CHAR_RAM, false, bossObjIdx);
			break;
		default:
			LoadChar(m, v, DIRECTION_RIGHT, (int)CHAR_SS, false, bossObjIdx);
			break;
		}
		break;
	case CWENT_SS_N:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			LoadChar(
				m, v, DIRECTION_DOWN, (int)CHAR_OSTRICH, false, bossObjIdx);
			break;
		default:
			LoadChar(m, v, DIRECTION_UP, (int)CHAR_SS, false, bossObjIdx);
			break;
		}
		break;
	case CWENT_SS_W:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			LoadChar(
				m, v, DIRECTION_DOWN, (int)CHAR_ANTELOPE, false, bossObjIdx);
			break;
		default:
			LoadChar(m, v, DIRECTION_LEFT, (int)CHAR_SS, false, bossObjIdx);
			break;
		}
		break;
	case CWENT_SS_S:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			LoadChar(m, v, DIRECTION_DOWN, (int)CHAR_GOAT, false, bossObjIdx);
			break;
		default:
			LoadChar(m, v, DIRECTION_DOWN, (int)CHAR_SS, false, bossObjIdx);
			break;
		}
		break;
	case CWENT_MUTANT_E:
		LoadChar(m, v, DIRECTION_RIGHT, (int)CHAR_MUTANT, false, bossObjIdx);
		break;
	case CWENT_MUTANT_N:
		LoadChar(m, v, DIRECTION_UP, (int)CHAR_MUTANT, false, bossObjIdx);
		break;
	case CWENT_MUTANT_W:
		LoadChar(m, v, DIRECTION_LEFT, (int)CHAR_MUTANT, false, bossObjIdx);
		break;
	case CWENT_MUTANT_S:
		LoadChar(m, v, DIRECTION_DOWN, (int)CHAR_MUTANT, false, bossObjIdx);
		break;
	case CWENT_OFFICER_E:
		LoadChar(m, v, DIRECTION_RIGHT, (int)CHAR_OFFICER, false, bossObjIdx);
		break;
	case CWENT_OFFICER_N:
		LoadChar(m, v, DIRECTION_UP, (int)CHAR_OFFICER, false, bossObjIdx);
		break;
	case CWENT_OFFICER_W:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			LoadChar(
				m, v, DIRECTION_DOWN, (int)CHAR_BEAR_BUSH, false, bossObjIdx);
			break;
		default:
			LoadChar(
				m, v, DIRECTION_LEFT, (int)CHAR_OFFICER, false, bossObjIdx);
			break;
		}
		break;
	case CWENT_OFFICER_S:
		LoadChar(m, v, DIRECTION_DOWN, (int)CHAR_OFFICER, false, bossObjIdx);
		break;
	case CWENT_GUARD_MOVING_E:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			LoadChar(m, v, DIRECTION_DOWN, (int)CHAR_BULL, false, bossObjIdx);
			break;
		default:
			LoadChar(m, v, DIRECTION_RIGHT, (int)CHAR_GUARD, true, bossObjIdx);
			break;
		}
		break;
	case CWENT_GUARD_MOVING_N:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			LoadChar(m, v, DIRECTION_DOWN, (int)CHAR_CAMEL, false, bossObjIdx);
			break;
		default:
			LoadChar(m, v, DIRECTION_UP, (int)CHAR_GUARD, true, bossObjIdx);
			break;
		}
		break;
	case CWENT_GUARD_MOVING_W:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			LoadChar(
				m, v, DIRECTION_DOWN, (int)CHAR_MONKEY, false, bossObjIdx);
			break;
		default:
			LoadChar(m, v, DIRECTION_LEFT, (int)CHAR_GUARD, true, bossObjIdx);
			break;
		}
		break;
	case CWENT_GUARD_MOVING_S:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			LoadChar(
				m, v, DIRECTION_DOWN, (int)CHAR_GIRAFFE, false, bossObjIdx);
			break;
		default:
			LoadChar(m, v, DIRECTION_DOWN, (int)CHAR_GUARD, true, bossObjIdx);
			break;
		}
		break;
	case CWENT_SS_MOVING_E:
		switch (map->type)
		{
		case CWMAPTYPE_N3D:
			LoadChar(m, v, DIRECTION_DOWN, (int)CHAR_BULL, false, bossObjIdx);
			break;
		default:
			LoadChar(m, v, DIRECTION_RIGHT, (int)CHAR_SS, true, bossObjIdx);
			break;
		}
		break;
	case CWENT_SS_MOVING_N:
		LoadChar(m, v, DIRECTION_UP, (int)CHAR_SS, true, bossObjIdx);
		break;
	case CWENT_SS_MOVING_W:
		LoadChar(m, v, DIRECTION_LEFT, (int)CHAR_SS, true, bossObjIdx);
		break;
	case CWENT_SS_MOVING_S:
		LoadChar(m, v, DIRECTION_DOWN, (int)CHAR_SS, true, bossObjIdx);
		break;
	case CWENT_MUTANT_MOVING_E:
		LoadChar(m, v, DIRECTION_RIGHT, (int)CHAR_MUTANT, true, bossObjIdx);
		break;
	case CWENT_MUTANT_MOVING_N:
		LoadChar(m, v, DIRECTION_UP, (int)CHAR_MUTANT, true, bossObjIdx);
		break;
	case CWENT_MUTANT_MOVING_W:
		LoadChar(m, v, DIRECTION_LEFT, (int)CHAR_MUTANT, true, bossObjIdx);
		break;
	case CWENT_MUTANT_MOVING_S:
		LoadChar(m, v, DIRECTION_DOWN, (int)CHAR_MUTANT, true, bossObjIdx);
		break;
	case CWENT_OFFICER_MOVING_E:
		LoadChar(m, v, DIRECTION_RIGHT, (int)CHAR_OFFICER, true, bossObjIdx);
		break;
	case CWENT_OFFICER_MOVING_N:
		LoadChar(m, v, DIRECTION_UP, (int)CHAR_OFFICER, true, bossObjIdx);
		break;
	case CWENT_OFFICER_MOVING_W:
		LoadChar(m, v, DIRECTION_LEFT, (int)CHAR_OFFICER, true, bossObjIdx);
		break;
	case CWENT_OFFICER_MOVING_S:
		LoadChar(m, v, DIRECTION_DOWN, (int)CHAR_OFFICER, true, bossObjIdx);
		break;
	case CWENT_TURN_E:
		AdjustTurningPoint(m, svec2i(v.x + 1, v.y));
		break;
	case CWENT_TURN_N:
		AdjustTurningPoint(m, svec2i(v.x, v.y - 1));
		break;
	case CWENT_TURN_W:
		AdjustTurningPoint(m, svec2i(v.x - 1, v.y));
		break;
	case CWENT_TURN_S:
		AdjustTurningPoint(m, svec2i(v.x, v.y + 1));
		break;
	case CWENT_TRANS:
		LoadChar(m, v, DIRECTION_DOWN, (int)CHAR_TRANS, false, bossObjIdx);
		break;
	case CWENT_UBER_MUTANT:
		LoadChar(
			m, v, DIRECTION_DOWN, (int)CHAR_UBERMUTANT, false, bossObjIdx);
		break;
	case CWENT_BARNACLE_WILHELM:
		LoadChar(m, v, DIRECTION_DOWN, (int)CHAR_WILHELM, false, bossObjIdx);
		break;
	case CWENT_ROBED_HITLER:
		LoadChar(
			m, v, DIRECTION_DOWN, (int)CHAR_FAKE_HITLER, false, bossObjIdx);
		break;
	case CWENT_DEATH_KNIGHT:
		LoadChar(
			m, v, DIRECTION_DOWN, (int)CHAR_DEATH_KNIGHT, false, bossObjIdx);
		break;
	case CWENT_HITLER:
		LoadChar(
			m, v, DIRECTION_DOWN, (int)CHAR_MECHA_HITLER, false, bossObjIdx);
		LoadChar(m, v, DIRECTION_DOWN, (int)CHAR_HITLER, false, bossObjIdx);
		break;
	case CWENT_FETTGESICHT:
		LoadChar(
			m, v, DIRECTION_DOWN, (int)CHAR_FETTGESICHT, false, bossObjIdx);
		break;
	case CWENT_SCHABBS:
		LoadChar(m, v, DIRECTION_DOWN, (int)CHAR_SCHABBS, false, bossObjIdx);
		break;
	case CWENT_GRETEL:
		LoadChar(m, v, DIRECTION_DOWN, (int)CHAR_GRETEL, false, bossObjIdx);
		break;
	case CWENT_HANS:
		LoadChar(m, v, DIRECTION_DOWN, (int)CHAR_HANS, false, bossObjIdx);
		break;
	case CWENT_OTTO:
		LoadChar(m, v, DIRECTION_DOWN, (int)CHAR_OTTO, false, bossObjIdx);
		break;
	case CWENT_PACMAN_GHOST_RED:
		LoadChar(
			m, v, DIRECTION_DOWN, (int)CHAR_PACMAN_GHOST_RED, false,
			bossObjIdx);
		break;
	case CWENT_PACMAN_GHOST_YELLOW:
		LoadChar(
			m, v, DIRECTION_DOWN, (int)CHAR_PACMAN_GHOST_YELLOW, false,
			bossObjIdx);
		break;
	case CWENT_PACMAN_GHOST_ROSE:
		LoadChar(
			m, v, DIRECTION_DOWN, (int)CHAR_PACMAN_GHOST_ROSE, false,
			bossObjIdx);
		break;
	case CWENT_PACMAN_GHOST_BLUE:
		LoadChar(
			m, v, DIRECTION_DOWN, (int)CHAR_PACMAN_GHOST_BLUE, false,
			bossObjIdx);
		break;
	case CWENT_KERRY_KANGAROO:
		LoadChar(m, v, DIRECTION_DOWN, (int)CHAR_KANGAROO, false, bossObjIdx);
		break;
	case CWENT_ERNIE_ELEPHANT:
		LoadChar(m, v, DIRECTION_DOWN, (int)CHAR_ELEPHANT, false, bossObjIdx);
		break;
	default:
		break;
	}
}
static bool MakeWallWalkable(Mission *m, const struct vec2i v)
{
	int *tile = CArrayGet(&m->u.Static.Tiles, v.x + v.y * m->Size.x);
	const TileClass *tc = MissionStaticGetTileClass(&m->u.Static, m->Size, v);
	if (tc->Type == TILE_CLASS_WALL && !tc->canWalk)
	{
		*tile += TILE_CLASS_WALL_OFFSET;
		return true;
	}
	return false;
}
static void LoadChar(
	Mission *m, const struct vec2i v, const direction_e d, const int charId,
	const bool moving, int *bossObjIdx)
{
	CharacterPlace cp = {v, d};
	switch (charId)
	{
	case CHAR_SCHABBS:
	case CHAR_MECHA_HITLER:
	case CHAR_HITLER:
	case CHAR_OTTO:
	case CHAR_FETTGESICHT:
	case CHAR_ANGEL: {
		CArrayPushBack(&m->SpecialChars, &charId);
		if (*bossObjIdx < 0)
		{
			Objective o;
			memset(&o, 0, sizeof o);
			o.Type = OBJECTIVE_KILL;
			CSTRDUP(o.Description, "Kill boss");
			CArrayPushBack(&m->Objectives, &o);
			*bossObjIdx = (int)m->Objectives.size - 1;
		}
		Objective *bossObj = CArrayGet(&m->Objectives, *bossObjIdx);
		bossObj->Required++;
		MissionStaticAddObjective(
			m, &m->u.Static, *bossObjIdx, (int)m->SpecialChars.size - 1, v,
			true);
	}
	break;
	default:
		MissionStaticAddCharacter(&m->u.Static, charId, cp);
		break;
	}
	// holowall
	if (MakeWallWalkable(m, v) || moving)
	{
		// Make the wall in front of it holowall
		MakeWallWalkable(m, svec2i_add(v, Vec2iFromDirection(d)));
	}
}
static void AdjustTurningPoint(Mission *m, const struct vec2i v)
{
	if (!Rect2iIsInside(Rect2iNew(svec2i_zero(), m->Size), v))
	{
		return;
	}
	// HACK: locked doors can be opened by patrolling enemies walking
	// into them Therefore unlock any locked doors directly in front of
	// a turning point
	const uint16_t unlockedAccess = 0;
	CArraySet(&m->u.Static.Access, v.x + v.y * m->Size.y, &unlockedAccess);
}

static bool TryLoadCampaign(CampaignList *list, const char *path);
static int TryLoadSpearSteam(CampaignList *list);
static void TryLoadSpearGOG(CampaignList *list, const int numLoaded);
void MapWolfLoadCampaignsFromSystem(CampaignList *list)
{
	char buf[CDOGS_PATH_MAX];

	// Wolf
	fsg_get_steam_game_path(buf, WOLF_STEAM_NAME);
	if (strlen(buf) > 0)
	{
		// Steam installs to /base
		strcat(buf, "/base");
	}
	if (!TryLoadCampaign(list, buf))
	{
		fsg_get_gog_game_path(buf, WOLF_GOG_ID);
		TryLoadCampaign(list, buf);
	}

	// Spear
	const int numLoaded = TryLoadSpearSteam(list);
	if (numLoaded < 3)
	{
		TryLoadSpearGOG(list, numLoaded);
	}

	// Super 3D Noah's Ark
	fsg_get_steam_game_path(buf, N3D_STEAM_NAME);
	if (!TryLoadCampaign(list, buf))
	{
		fsg_get_gog_game_path(buf, N3D_GOG_ID);
		TryLoadCampaign(list, buf);
	}
}
static bool TryLoadSpearSteamVanilla(CampaignList *list);
static bool TryLoadSpearSteamInWolf3D(CampaignList *list);
static int TryLoadSpearSteam(CampaignList *list)
{
	if (TryLoadSpearSteamVanilla(list))
	{
		return 3;
	}
	return TryLoadSpearSteamInWolf3D(list) ? 1 : 0;
}
static bool TryLoadSpearSteamVanilla(CampaignList *list)
{
	char buf[CDOGS_PATH_MAX];
	fsg_get_steam_game_path(buf, SPEAR_STEAM_NAME);
	if (strlen(buf) == 0)
	{
		return false;
	}
	// Steam installs to /base
	strcat(buf, "/base");
	for (int i = 1; i <= 3; i++)
	{
		char buf2[CDOGS_PATH_MAX];
		// Append spear mission pack to path - we will handle this
		// later
		sprintf(buf2, "%s?%d", buf, i);
		if (!TryLoadCampaign(list, buf2))
		{
			return false;
		}
	}
	return true;
}
static bool TryLoadSpearSteamInWolf3D(CampaignList *list)
{
	// Steam also includes spear ep1 in Wolf3D
	char buf[CDOGS_PATH_MAX];
	fsg_get_steam_game_path(buf, WOLF_STEAM_NAME);
	if (strlen(buf) == 0)
	{
		return false;
	}
	// Steam installs to /base/m1
	strcat(buf, "/base/m1?1");
	return TryLoadCampaign(list, buf);
}
static bool TryLoadSpearGOGVanilla(CampaignList *list, const int numLoaded);
static bool TryLoadSpearGOGInWolf3D(CampaignList *list);
static void TryLoadSpearGOG(CampaignList *list, const int numLoaded)
{
	if (!TryLoadSpearGOGVanilla(list, numLoaded) && numLoaded < 1)
	{
		TryLoadSpearGOGInWolf3D(list);
	}
}
static bool TryLoadSpearGOGVanilla(CampaignList *list, const int numLoaded)
{
	char buf[CDOGS_PATH_MAX];
	fsg_get_gog_game_path(buf, SPEAR_GOG_ID);
	if (strlen(buf) > 0)
	{
		char buf2[CDOGS_PATH_MAX];
		bool oneLoaded = false;
		for (int i = 1 + numLoaded; i <= 3; i++)
		{
			sprintf(buf2, "%s/M%d?%d", buf, i, i);
			if (TryLoadCampaign(list, buf2) && !oneLoaded)
			{
				oneLoaded = true;
			}
		}
		return oneLoaded;
	}
	return false;
}
static bool TryLoadSpearGOGInWolf3D(CampaignList *list)
{
	// Steam also includes spear ep1 in Wolf3D
	char buf[CDOGS_PATH_MAX];
	fsg_get_gog_game_path(buf, WOLF_SPEAR_GOG_ID);
	if (strlen(buf) > 0)
	{
		char buf2[CDOGS_PATH_MAX];
		sprintf(buf2, "%s/M1?1", buf);
		return TryLoadCampaign(list, buf2);
	}
	return false;
}
static bool TryLoadCampaign(CampaignList *list, const char *path)
{
	if (strlen(path) > 0)
	{
		CampaignEntry entry;
		if (CampaignEntryTryLoad(&entry, path, GAME_MODE_NORMAL))
		{
			CArrayPushBack(&list->list, &entry);
			return true;
		}
	}
	return false;
}

void MapWolfN3DCheckAndLoadCustomQuiz(
	const Campaign *c, const CArray *playerDatas)
{
	// Special case for N3D: Epic of Gilgamesh quiz easter egg
	// This code is pretty cursed
	if (strcmp(c->Setting.Title, "Super 2D Noah's Ark") != 0)
	{
		return;
	}
	bool hasGilgameshName = false;
	CA_FOREACH(const PlayerData, p, *playerDatas)
	if (Stricmp(p->name, "Gilgamesh") == 0 ||
		Stricmp(p->name, "Enkidu") == 0 ||
		Stricmp(p->name, "Utnapishtim") == 0 ||
		Stricmp(p->name, "Ziusudra") == 0 ||
		Stricmp(p->name, "Atra-Hasis") == 0 ||
		Stricmp(p->name, "Atrahasis") == 0)
	{
		hasGilgameshName = true;
		break;
	}
	CA_FOREACH_END()
	if (!hasGilgameshName)
	{
		return;
	}

	// Load new quizzes
	char pathBuf[CDOGS_PATH_MAX];
	GetDataFilePath(pathBuf, WOLF_DATA_DIR "N3Ddata.cdogscpn/language.enu");
	char *languageBuf = ReadFileIntoBuf(pathBuf, "r");
	CWolfMap map;
	memset(&map, 0, sizeof map);
	CWN3DLoadQuizzes(&map, languageBuf);
	free(languageBuf);
	
	// Copy the effects from the "scroll" pickup
	const PickupClass *scroll = StrPickupClass("scroll");
	const PickupEffect *menuEffect = CArrayGet(&scroll->Effects, 0);
	const CArray *correctEffects =
		&((const PickupMenuItem *)CArrayGet(&menuEffect->u.Menu.Items, 0))
			 ->Effects;
	const CArray *wrongEffects =
		&((const PickupMenuItem *)CArrayGet(&menuEffect->u.Menu.Items, 1))
			 ->Effects;

	// Replace the scroll pickups with the quizzes
	for (int i = 0;; i++)
	{
		char buf[256];
		sprintf(buf, "scroll+%d", i);
		PickupClass *c = StrPickupClass(buf);
		if (c == NULL)
		{
			break;
		}
		CA_FOREACH(PickupEffect, e, c->Effects)
		PickupEffectTerminate(e);
		CA_FOREACH_END()
		CArrayClear(&c->Effects);

		PickupEffect e;
		memset(&e, 0, sizeof e);
		e.Type = PICKUP_MENU;
		const CWN3DQuiz *quiz = &map.quizzes[i];
		CSTRDUP(e.u.Menu.Text, quiz->question);

		CArrayInit(&e.u.Menu.Items, sizeof(PickupMenuItem));
		for (int j = 0; j < quiz->nAnswers; j++)
		{
			PickupMenuItem m;
			PickupMenuItemInit(&m);
			CSTRDUP(m.Text, quiz->answers[j]);
			const CArray *effects =
				j == quiz->correctIdx ? correctEffects : wrongEffects;
			CA_FOREACH(const PickupEffect, pe, *effects)
			PickupEffect ec = PickupEffectCopy(pe);
			CArrayPushBack(&m.Effects, &ec);
			CA_FOREACH_END()
			CArrayPushBack(&e.u.Menu.Items, &m);
		}

		CArrayPushBack(&c->Effects, &e);
	}
	
	CWFree(&map);
}
