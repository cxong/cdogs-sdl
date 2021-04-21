/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.
	Copyright (c) 2020-2021 Cong Xu
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

#include "log.h"

#include "cwolfmap/cwolfmap.h"
#include "map_archive.h"
#include "player_template.h"

#define TILE_CLASS_WALL_OFFSET 62

static void GetCampaignPath(const CWMapType type, char *buf)
{
	switch (type)
	{
	case CWMAPTYPE_WL1:
		GetDataFilePath(buf, "missions/.wolf3d/WL1.cdogscpn");
		break;
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
	if (numMissions)
	{
		*numMissions = map.nLevels;
	}

bail:
	CWFree(&map);
	return err;
}

static void LoadSounds(const SoundDevice *s, const CWolfMap *map);
static void LoadMission(
	CArray *missions, const map_t tileClasses, const CWolfMap *map,
	const int missionIndex);

int MapWolfLoad(const char *filename, CampaignSetting *c)
{
	int err = 0;
	CWolfMap map;
	map_t tileClasses = NULL;
	err = CWLoad(&map, filename);
	if (err != 0)
	{
		goto bail;
	}

	LoadSounds(&gSoundDevice, &map);
	// TODO: Load music

	char buf[CDOGS_PATH_MAX];
	// Copy data from common campaign and use them for every mission
	GetDataFilePath(buf, "missions/.wolf3d/common.cdogscpn");
	CampaignSetting cCommon;
	CampaignSettingInit(&cCommon);
	err = MapNewLoadArchive(buf, &cCommon);
	if (err != 0)
	{
		goto bail;
	}
	Mission *m = CArrayGet(&cCommon.Missions, 0);
	tileClasses = hashmap_copy(m->u.Static.TileClasses, TileClassCopyHashMap);
	CharacterStore cs;
	memset(&cs, 0, sizeof cs);
	CharacterStoreCopy(
		&cs, &cCommon.characters, &gPlayerTemplates.CustomClasses);
	CampaignSettingTerminate(&cCommon);
	// Create walk-through copies of all the walls
	for (int i = 3; i <= 64; i++)
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

	GetCampaignPath(map.type, buf);
	err = MapNewLoadArchive(buf, c);
	if (err != 0)
	{
		goto bail;
	}

	for (int i = 0; i < map.nLevels; i++)
	{
		LoadMission(&c->Missions, tileClasses, &map, i);
	}

	CharacterStoreCopy(&c->characters, &cs, &gPlayerTemplates.CustomClasses);

bail:
	hashmap_destroy(tileClasses, TileClassDestroy);
	CharacterStoreTerminate(&cs);
	CWFree(&map);
	return err;
}

static void LoadSounds(const SoundDevice *s, const CWolfMap *map)
{
	if (!s->isInitialised)
	{
		return;
	}
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

static void LoadTile(
	MissionStatic *m, const uint16_t ch, const CWolfMap *map,
	const struct vec2i v, const int missionIndex);
static void TryLoadWallObject(
	MissionStatic *m, const uint16_t ch, const CWolfMap *map,
	const struct vec2i v, const int missionIndex);
static void LoadEntity(
	MissionStatic *m, const uint16_t ch, const CWolfMap *map,
	const struct vec2i v, const int missionIndex);

static void LoadMission(
	CArray *missions, const map_t tileClasses, const CWolfMap *map,
	const int missionIndex)
{
	const CWLevel *level = &map->levels[missionIndex];
	Mission m;
	MissionInit(&m);
	CSTRDUP(m.Title, level->header.name);
	m.Size = svec2i(level->header.width, level->header.height);
	m.Type = MAPTYPE_STATIC;
	strcpy(m.ExitStyle, "plate");
	strcpy(m.KeyStyle, "plain2");
	// TODO: objectives for treasure, kills (multiple items per obj)
	const WeaponClass *wc = StrWeaponClass("Pistol");
	CArrayPushBack(&m.Weapons, &wc);
	wc = StrWeaponClass("Knife");
	CArrayPushBack(&m.Weapons, &wc);
	// TODO: song
	MissionStaticInit(&m.u.Static);

	m.u.Static.TileClasses = hashmap_copy(tileClasses, TileClassCopyHashMap);

	RECT_FOREACH(Rect2iNew(svec2i_zero(), m.Size))
	const uint16_t ch = CWLevelGetCh(level, 0, _v.x, _v.y);
	LoadTile(&m.u.Static, ch, map, _v, missionIndex);
	RECT_FOREACH_END()
	// Load objects after all tiles are loaded
	RECT_FOREACH(Rect2iNew(svec2i_zero(), m.Size))
	const uint16_t ch = CWLevelGetCh(level, 0, _v.x, _v.y);
	TryLoadWallObject(&m.u.Static, ch, map, _v, missionIndex);
	const uint16_t ech = CWLevelGetCh(level, 1, _v.x, _v.y);
	LoadEntity(&m.u.Static, ech, map, _v, missionIndex);
	RECT_FOREACH_END()

	m.u.Static.AltFloorsEnabled = false;

	CArrayPushBack(missions, &m);
}

static int LoadWall(const uint16_t ch);

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
	case CWTILE_SECRET_EXIT: {
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
		CASSERT(false, "unknown tile");
		break;
	}
	CArrayPushBack(&m->Tiles, &staticTile);
	CArrayPushBack(&m->Access, &staticAccess);
}

static int LoadWall(const uint16_t ch)
{
	const CWWall wall = CWChToWall(ch);
	return (int)wall + 3;
}

static void TryLoadWallObject(
	MissionStatic *m, const uint16_t ch, const CWolfMap *map,
	const struct vec2i v, const int missionIndex)
{
	const CWLevel *level = &map->levels[missionIndex];
	const struct vec2i levelSize =
		svec2i(level->header.width, level->header.height);
	const struct vec2i vBelow = svec2i_add(v, svec2i(0, 1));
	const CWWall wall = CWChToWall(ch);
	const char *moName = NULL;
	switch (wall)
	{
	case CWWALL_GREY_BRICK_FLAG:
		moName = "heer_flag";
		break;
	case CWWALL_GREY_BRICK_HITLER:
		moName = "hitler_portrait";
		break;
	case CWWALL_CELL:
		moName = "jail_cell";
		break;
	case CWWALL_GREY_BRICK_EAGLE:
		moName = "brick_eagle";
		break;
	case CWWALL_CELL_SKELETON:
		moName = "jail_cell_skeleton";
		break;
	case CWWALL_WOOD_EAGLE:
		moName = "eagle_portrait";
		break;
	case CWWALL_WOOD_HITLER:
		moName = "hitler_portrait";
		break;
	case CWWALL_ENTRANCE:
		moName = "elevator_entrance";
		break;
	case CWWALL_STEEL_SIGN:
		moName = "no_sign";
		break;
	case CWWALL_RED_BRICK_SWASTIKA:
		moName = "swastika_wreath";
		break;
	case CWWALL_RED_BRICK_FLAG:
		moName = "coat_of_arms_flag";
		break;
	case CWWALL_ELEVATOR:
		if (MissionStaticGetTileClass(m, levelSize, vBelow)->Type ==
			TILE_CLASS_FLOOR)
		{
			moName = "elevator_interior";
		}
		// Elevators only occur on east/west tiles
		for (int dx = -1; dx <= 1; dx += 2)
		{
			const struct vec2i exitV = svec2i(v.x + dx, v.y);
			const TileClass *tc =
				MissionStaticGetTileClass(m, levelSize, exitV);
			if (tc != NULL && tc->Type == TILE_CLASS_FLOOR)
			{
				Exit e;
				e.Hidden = true;
				e.Mission = missionIndex + 1;
				// Check if coming back from secret level
				if (map->type == CWMAPTYPE_SOD)
				{
					if (missionIndex == 19)
					{
						e.Mission = 4;
					}
					else if (missionIndex == 20)
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
		break;
	case CWWALL_DEAD_ELEVATOR:
		if (MissionStaticGetTileClass(
				m, svec2i(level->header.width, level->header.height), vBelow)
				->Type == TILE_CLASS_FLOOR)
		{
			moName = "elevator_interior";
		}
		break;
	case CWWALL_WOOD_IRON_CROSS:
		moName = "iron_cross";
		break;
	case CWWALL_DIRTY_BRICK_1:
	case CWWALL_DIRTY_BRICK_2: // fallthrough
		moName = "cobble_moss";
		break;
	case CWWALL_PURPLE_BLOOD:
		moName = "bloodstain";
		break;
	case CWWALL_GREY_BRICK_SIGN:
		moName = "no_sign";
		break;
	case CWWALL_BROWN_WEAVE_BLOOD_2:
		moName = "bloodstain";
		break;
	case CWWALL_BROWN_WEAVE_BLOOD_3:
		moName = "bloodstain1";
		break;
	case CWWALL_BROWN_WEAVE_BLOOD_1:
		moName = "bloodstain2";
		break;
	case CWWALL_STAINED_GLASS:
		moName = "hitler_glass";
		break;
	case CWWALL_BLUE_WALL_SKULL:
		moName = "skull_blue";
		break;
	case CWWALL_BLUE_WALL_SWASTIKA:
		moName = "swastika_blue";
		break;
	case CWWALL_GREY_WALL_VENT:
		moName = "wall_vent";
		break;
	case CWWALL_MULTICOLOR_BRICK:
		moName = "brick_color";
		break;
	case CWWALL_BLUE_BRICK_SIGN:
		moName = "no_sign";
		break;
	case CWWALL_GREY_WALL_MAP:
		moName = "map";
		break;
	case CWWALL_BROWN_MARBLE_FLAG:
		moName = "heer_flag";
		break;
	case CWWALL_WOOD_PANEL:
		moName = "panel";
		break;
	case CWWALL_GREY_WALL_HITLER:
		moName = "hitler_poster";
		break;
	case CWWALL_STONE_WALL_1:
	case CWWALL_STONE_WALL_2:	 // fallthrough
	case CWWALL_RAMPART_STONE_1: // fallthrough
	case CWWALL_RAMPART_STONE_2: // fallthrough
		moName = "stone_color";
		break;
	case CWWALL_STONE_WALL_FLAG:
		moName = "heer_flag";
		break;
	case CWWALL_STONE_WALL_WREATH:
		moName = "swastika_wreath";
		break;
	case CWWALL_ELEVATOR_WALL:
		moName = "elevator_interior";
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
	CHAR_UBERMUTANT,
	CHAR_DEATH_KNIGHT,
	CHAR_GHOST,
	CHAR_ANGEL
} WolfChar;

static void LoadEntity(
	MissionStatic *m, const uint16_t ch, const CWolfMap *map,
	const struct vec2i v, const int missionIndex)
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
		m->Start = v;
		// Remove any exits that overlap with start
		// SOD starts the player in elevators
		CA_FOREACH(const Exit, e, m->Exits)
		const Rect2i er =
			Rect2iNew(e->R.Pos, svec2i_add(e->R.Size, svec2i_one()));
		if (Rect2iIsInside(er, v))
		{
			CArrayDelete(&m->Exits, _ca_index);
			_ca_index--;
		}
		CA_FOREACH_END()
		break;
	case CWENT_WATER:
		MissionStaticTryAddItem(m, StrMapObject("pool_water"), v);
		break;
	case CWENT_OIL_DRUM:
		MissionStaticTryAddItem(m, StrMapObject("barrel_green"), v);
		break;
	case CWENT_TABLE_WITH_CHAIRS:
		MissionStaticTryAddItem(m, StrMapObject("table_and_chairs"), v);
		break;
	case CWENT_FLOOR_LAMP:
		MissionStaticTryAddItem(m, StrMapObject("rod_light"), v);
		MissionStaticTryAddItem(m, StrMapObject("spotlight"), v);
		break;
	case CWENT_CHANDELIER:
		MissionStaticTryAddItem(m, StrMapObject("chandelier"), v);
		MissionStaticTryAddItem(m, StrMapObject("spotlight"), v);
		break;
	case CWENT_HANGING_SKELETON:
		MissionStaticTryAddItem(m, StrMapObject("hanging_skeleton"), v);
		MissionStaticTryAddItem(m, StrMapObject("shadow"), v);
		break;
	case CWENT_DOG_FOOD:
		MissionStaticTryAddPickup(m, StrPickupClass("dogfood"), v);
		break;
	case CWENT_WHITE_COLUMN:
		MissionStaticTryAddItem(m, StrMapObject("pillar"), v);
		break;
	case CWENT_GREEN_PLANT:
		MissionStaticTryAddItem(m, StrMapObject("plant"), v);
		break;
	case CWENT_SKELETON:
		MissionStaticTryAddItem(m, StrMapObject("bone_blood"), v);
		break;
	case CWENT_SINK_SKULLS_ON_STICK:
		if (map->type == CWMAPTYPE_SOD)
		{
			MissionStaticTryAddItem(m, StrMapObject("skull_pillar"), v);
		}
		else
		{
			MissionStaticTryAddItem(m, StrMapObject("sink"), v);
		}
		break;
	case CWENT_BROWN_PLANT:
		MissionStaticTryAddItem(m, StrMapObject("plant_brown"), v);
		break;
	case CWENT_VASE:
		MissionStaticTryAddItem(m, StrMapObject("urn"), v);
		break;
	case CWENT_TABLE:
		MissionStaticTryAddItem(m, StrMapObject("table_wood_round"), v);
		break;
	case CWENT_CEILING_LIGHT_GREEN:
		MissionStaticTryAddItem(m, StrMapObject("spotlight"), v);
		break;
	case CWENT_UTENSILS_BROWN_CAGE_BLOODY_BONES:
		if (map->type == CWMAPTYPE_SOD)
		{
			MissionStaticTryAddItem(m, StrMapObject("gibbet_bloody"), v);
			MissionStaticTryAddItem(m, StrMapObject("shadow"), v);
		}
		else
		{
			MissionStaticTryAddItem(m, StrMapObject("knives"), v);
		}
		break;
	case CWENT_ARMOR:
		MissionStaticTryAddItem(m, StrMapObject("suit_of_armor"), v);
		break;
	case CWENT_CAGE:
		MissionStaticTryAddItem(m, StrMapObject("gibbet"), v);
		MissionStaticTryAddItem(m, StrMapObject("shadow"), v);
		break;
	case CWENT_CAGE_SKELETON:
		MissionStaticTryAddItem(m, StrMapObject("gibbet_skeleton"), v);
		MissionStaticTryAddItem(m, StrMapObject("shadow"), v);
		break;
	case CWENT_BONES1:
		MissionStaticTryAddItem(m, StrMapObject("skull"), v);
		break;
	case CWENT_KEY_GOLD:
		MissionStaticAddKey(m, 0, v);
		break;
	case CWENT_KEY_SILVER:
		MissionStaticAddKey(m, 2, v);
		break;
	case CWENT_BED_CAGE_SKULLS:
		if (map->type == CWMAPTYPE_SOD)
		{
			MissionStaticTryAddItem(m, StrMapObject("gibbet_skulls"), v);
			MissionStaticTryAddItem(m, StrMapObject("shadow"), v);
		}
		else
		{
			MissionStaticTryAddItem(m, StrMapObject("bed"), v);
		}
		break;
	case CWENT_BASKET:
		MissionStaticTryAddItem(m, StrMapObject("basket"), v);
		break;
	case CWENT_FOOD:
		MissionStaticTryAddPickup(m, StrPickupClass("meal"), v);
		break;
	case CWENT_MEDKIT:
		MissionStaticTryAddPickup(m, StrPickupClass("health"), v);
		break;
	case CWENT_AMMO:
		MissionStaticTryAddPickup(m, StrPickupClass("ammo_Ammo"), v);
		break;
	case CWENT_MACHINE_GUN:
		MissionStaticTryAddPickup(m, StrPickupClass("gun_Machine Gun"), v);
		break;
	case CWENT_CHAIN_GUN:
		MissionStaticTryAddPickup(m, StrPickupClass("gun_Chain Gun"), v);
		break;
	case CWENT_CROSS:
		MissionStaticTryAddPickup(m, StrPickupClass("cross"), v);
		break;
	case CWENT_CHALICE:
		MissionStaticTryAddPickup(m, StrPickupClass("chalice"), v);
		break;
	case CWENT_CHEST:
		MissionStaticTryAddPickup(m, StrPickupClass("chest"), v);
		break;
	case CWENT_CROWN:
		MissionStaticTryAddPickup(m, StrPickupClass("crown"), v);
		break;
	case CWENT_LIFE:
		MissionStaticTryAddPickup(m, StrPickupClass("heart"), v);
		break;
	case CWENT_BONES_BLOOD:
		MissionStaticTryAddItem(m, StrMapObject("gibs"), v);
		break;
	case CWENT_BARREL:
		MissionStaticTryAddItem(m, StrMapObject("barrel_wood2"), v);
		break;
	case CWENT_WELL_WATER:
		MissionStaticTryAddItem(m, StrMapObject("well_water"), v);
		break;
	case CWENT_WELL:
		MissionStaticTryAddItem(m, StrMapObject("well"), v);
		break;
	case CWENT_POOL_OF_BLOOD:
		MissionStaticTryAddItem(m, StrMapObject("pool_blood"), v);
		break;
	case CWENT_FLAG:
		MissionStaticTryAddItem(m, StrMapObject("flag"), v);
		break;
	case CWENT_CEILING_LIGHT_RED_AARDWOLF:
		if (map->type == CWMAPTYPE_WL6)
		{
			MissionStaticTryAddItem(m, StrMapObject("spotlight"), v);
		}
		else
		{
			MissionStaticTryAddItem(m, StrMapObject("skull2"), v);
			break;
		}
		break;
	case CWENT_BONES2:
		MissionStaticTryAddItem(m, StrMapObject("bones"), v);
		break;
	case CWENT_BONES3:
		MissionStaticTryAddItem(m, StrMapObject("bones2"), v);
		break;
	case CWENT_BONES4:
		MissionStaticTryAddItem(m, StrMapObject("bones3"), v);
		break;
	case CWENT_UTENSILS_BLUE_COW_SKULL:
		if (map->type == CWMAPTYPE_SOD)
		{
			MissionStaticTryAddItem(m, StrMapObject("cowskull_pillar"), v);
		}
		else
		{
			MissionStaticTryAddItem(m, StrMapObject("pots"), v);
		}
		break;
	case CWENT_STOVE_WELL_BLOOD:
		if (map->type == CWMAPTYPE_SOD)
		{
			MissionStaticTryAddItem(m, StrMapObject("well_blood"), v);
		}
		else
		{
			MissionStaticTryAddItem(m, StrMapObject("stove"), v);
		}
		break;
	case CWENT_RACK_ANGEL_STATUE:
		if (map->type == CWMAPTYPE_SOD)
		{
			MissionStaticTryAddItem(m, StrMapObject("statue_behemoth"), v);
		}
		else
		{
			MissionStaticTryAddItem(m, StrMapObject("spears"), v);
		}
		break;
	case CWENT_VINES:
		MissionStaticTryAddItem(m, StrMapObject("grass"), v);
		break;
	case CWENT_BROWN_COLUMN:
		MissionStaticTryAddItem(m, StrMapObject("pillar_brown"), v);
		break;
	case CWENT_AMMO_BOX:
		MissionStaticTryAddPickup(m, StrPickupClass("ammo_box"), v);
		break;
	case CWENT_TRUCK_REAR:
		MissionStaticTryAddItem(m, StrMapObject("truck"), v);
		break;
	case CWENT_SPEAR:
		MissionStaticTryAddPickup(m, StrPickupClass("spear"), v);
		break;
	case CWENT_PUSHWALL: {
		const CWLevel *level = &map->levels[missionIndex];
		int *tile = CArrayGet(&m->Tiles, v.x + v.y * level->header.width);
		*tile += TILE_CLASS_WALL_OFFSET;
	}
	break;
	case CWENT_ENDGAME: {
		Exit e;
		e.Hidden = true;
		e.Mission = map->nLevels;
		e.R.Pos = v;
		e.R.Size = svec2i_zero();
		CArrayPushBack(&m->Exits, &e);
	}
	break;
	case CWENT_GHOST:
		MissionStaticAddCharacter(m, (int)CHAR_GHOST, v);
		break;
	case CWENT_ANGEL:
		MissionStaticAddCharacter(m, (int)CHAR_ANGEL, v);
		break;
	case CWENT_DEAD_GUARD:
		MissionStaticTryAddItem(m, StrMapObject("dead_guard"), v);
		break;
	case CWENT_DOG_E:
	case CWENT_DOG_N: // fallthrough
	case CWENT_DOG_W: // fallthrough
	case CWENT_DOG_S: // fallthrough
		MissionStaticAddCharacter(m, (int)CHAR_DOG, v);
		break;
	case CWENT_GUARD_E:
	case CWENT_GUARD_N: // fallthrough
	case CWENT_GUARD_W: // fallthrough
	case CWENT_GUARD_S: // fallthrough
		MissionStaticAddCharacter(m, (int)CHAR_GUARD, v);
		break;
	case CWENT_SS_E:
	case CWENT_SS_N: // fallthrough
	case CWENT_SS_W: // fallthrough
	case CWENT_SS_S: // fallthrough
		MissionStaticAddCharacter(m, (int)CHAR_SS, v);
		break;
	case CWENT_MUTANT_E:
	case CWENT_MUTANT_N: // fallthrough
	case CWENT_MUTANT_W: // fallthrough
	case CWENT_MUTANT_S: // fallthrough
		MissionStaticAddCharacter(m, (int)CHAR_MUTANT, v);
		break;
	case CWENT_OFFICER_E:
	case CWENT_OFFICER_N: // fallthrough
	case CWENT_OFFICER_W: // fallthrough
	case CWENT_OFFICER_S: // fallthrough
		MissionStaticAddCharacter(m, (int)CHAR_OFFICER, v);
		break;
	case CWENT_TURN_E:
	case CWENT_TURN_NE:
	case CWENT_TURN_N:
	case CWENT_TURN_NW:
	case CWENT_TURN_W:
	case CWENT_TURN_SW:
	case CWENT_TURN_S:
	case CWENT_TURN_SE:
		break;
	case CWENT_TRANS:
		MissionStaticAddCharacter(m, (int)CHAR_TRANS, v);
		break;
	case CWENT_UBER_MUTANT:
		MissionStaticAddCharacter(m, (int)CHAR_UBERMUTANT, v);
		break;
	case CWENT_BARNACLE_WILHELM:
		MissionStaticAddCharacter(m, (int)CHAR_WILHELM, v);
		break;
	case CWENT_ROBED_HITLER:
		MissionStaticAddCharacter(m, (int)CHAR_FAKE_HITLER, v);
		break;
	case CWENT_DEATH_KNIGHT:
		MissionStaticAddCharacter(m, (int)CHAR_DEATH_KNIGHT, v);
		break;
	case CWENT_HITLER:
		MissionStaticAddCharacter(m, (int)CHAR_MECHA_HITLER, v);
		break;
	case CWENT_FETTGESICHT:
		MissionStaticAddCharacter(m, (int)CHAR_FETTGESICHT, v);
		break;
	case CWENT_SCHABBS:
		MissionStaticAddCharacter(m, (int)CHAR_SCHABBS, v);
		break;
	case CWENT_GRETEL:
		MissionStaticAddCharacter(m, (int)CHAR_GRETEL, v);
		break;
	case CWENT_HANS:
		MissionStaticAddCharacter(m, (int)CHAR_HANS, v);
		break;
	case CWENT_OTTO:
		MissionStaticAddCharacter(m, (int)CHAR_OTTO, v);
		break;
	case CWENT_PACMAN_GHOST_RED:
		MissionStaticAddCharacter(m, (int)CHAR_PACMAN_GHOST_RED, v);
		break;
	case CWENT_PACMAN_GHOST_YELLOW:
		MissionStaticAddCharacter(m, (int)CHAR_PACMAN_GHOST_YELLOW, v);
		break;
	case CWENT_PACMAN_GHOST_ROSE:
		MissionStaticAddCharacter(m, (int)CHAR_PACMAN_GHOST_ROSE, v);
		break;
	case CWENT_PACMAN_GHOST_BLUE:
		MissionStaticAddCharacter(m, (int)CHAR_PACMAN_GHOST_BLUE, v);
		break;
	default:
		CASSERT(false, "unknown entity");
		break;
	}
}
