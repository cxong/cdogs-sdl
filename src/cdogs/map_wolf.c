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

#include "cwolfmap/cwolfmap.h"
#include "map_archive.h"

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
	CArray *missions, const map_t tileClasses, const CWLevel *level,
	const CWMapType type, const int missionIndex);

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
	
	char buf[CDOGS_PATH_MAX];
	// Copy tile classes from common campaign
	// and use them for every mission
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
	CampaignSettingTerminate(&cCommon);
	
	GetCampaignPath(map.type, buf);
	err = MapNewLoadArchive(buf, c);
	if (err != 0)
	{
		goto bail;
	}

	LoadSounds(&gSoundDevice, &map);
	// TODO: Load music

	const CWLevel *level = map.levels;
	for (int i = 0; i < map.nLevels; i++, level++)
	{
		LoadMission(&c->Missions, tileClasses, level, map.type, i);
	}

bail:
	hashmap_destroy(tileClasses, TileClassDestroy);
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
	MissionStatic *m, const uint16_t ch, const struct vec2i v,
	const int missionIndex);
static void TryLoadWallObject(MissionStatic *m, const uint16_t ch, const struct vec2i v);
static void LoadEntity(
	MissionStatic *m, const uint16_t ch, const struct vec2i v,
	const int missionIndex);

static void LoadMission(
	CArray *missions, const map_t tileClasses, const CWLevel *level,
	const CWMapType type, const int missionIndex)
{
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
	wc = StrWeaponClass("Pistol");
	CArrayPushBack(&m.Weapons, &wc);
	// TODO: song
	MissionStaticInit(&m.u.Static);

	m.u.Static.TileClasses = hashmap_copy(tileClasses, TileClassCopyHashMap);

	UNUSED(type);
	RECT_FOREACH(Rect2iNew(svec2i_zero(), m.Size))
	const uint16_t ch = CWLevelGetCh(level, 0, _v.x, _v.y);
	LoadTile(&m.u.Static, ch, _v, missionIndex);
	TryLoadWallObject(&m.u.Static, ch, _v);
	const uint16_t ech = CWLevelGetCh(level, 1, _v.x, _v.y);
	LoadEntity(&m.u.Static, ech, _v, missionIndex);
	RECT_FOREACH_END()
	
	m.u.Static.AltFloorsEnabled = false;

	CArrayPushBack(missions, &m);
}

static int LoadWall(const uint16_t ch);

static void LoadTile(
	MissionStatic *m, const uint16_t ch, const struct vec2i v,
	const int missionIndex)
{
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
	case CWTILE_ELEVATOR_V: {
		staticTile = 1;
		// TODO: mission index for exit
		Exit e;
		e.Hidden = true;
		e.Mission = missionIndex + 1;
		e.R.Pos = v;
		e.R.Size = svec2i_zero();
		CArrayPushBack(&m->Exits, &e);
	}
	break;
	case CWTILE_AREA:
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
	return (int)wall + 2;
}

static void TryLoadWallObject(MissionStatic *m, const uint16_t ch, const struct vec2i v)
{
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
	case CWWALL_DEAD_ELEVATOR:	// fallthrough
		moName = "elevator_interior";
		break;
	case CWWALL_WOOD_IRON_CROSS:
		moName = "iron_cross";
		break;
	case CWWALL_DIRTY_BRICK_1:
	case CWWALL_DIRTY_BRICK_2:	// fallthrough
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
	case CWWALL_STONE_WALL_FLAG:
		moName = "heer_flag";
		break;
	case CWWALL_STONE_WALL_WREATH:
		moName = "swastika_wreath";
		break;
	case CWWALL_RAMPART_STONE_1:
	case CWWALL_RAMPART_STONE_2:	// fallthrough
		moName = "stone_color";
		break;
	case CWWALL_ELEVATOR_WALL:
		moName = "elevator_interior";
		break;
	default:
		break;
	}
	if (moName != NULL)
	{
		const struct vec2i wv = svec2i_add(v, svec2i(0, 1));
		MissionStaticTryAddItem(m, StrMapObject(moName), wv);
	}
}

static void LoadEntity(
	MissionStatic *m, const uint16_t ch, const struct vec2i v,
	const int missionIndex)
{
	UNUSED(missionIndex);
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
		break;
		/*case CWENT_WATER:
			setColor(LIGHTBLUE);
			c = 'O';
			break;
		case CWENT_OIL_DRUM:
			setColor(LIGHTGREEN);
			c = '|';
			break;
		case CWENT_TABLE_WITH_CHAIRS:
			setColor(RED);
			c = '#';
			break;
		case CWENT_FLOOR_LAMP:
			setColor(MAGENTA);
			c = '|';
			break;
		case CWENT_CHANDELIER:
			setBackgroundColor(GREY);
			setColor(YELLOW);
			c = '+';
			break;
		case CWENT_HANGING_SKELETON:
			setColor(LIGHTCYAN);
			c = '#';
			break;
		case CWENT_WHITE_COLUMN:
			setColor(WHITE);
			c = '|';
			break;
		case CWENT_DOG_FOOD:
			setColor(RED);
			c = '=';
			break;
		case CWENT_GREEN_PLANT:
			setBackgroundColor(GREEN);
			setColor(LIGHTGREEN);
			c = '+';
			break;
		case CWENT_SKELETON:
			setColor(WHITE);
			c = '#';
			break;
		case CWENT_SINK_SKULLS_ON_STICK:
			if (type == CWMAPTYPE_SOD)
			{
				setColor(LIGHTMAGENTA);
				c = '#';
			}
			else
			{
				setBackgroundColor(WHITE);
				setColor(BLACK);
				c = '+';
			}
			break;
		case CWENT_BROWN_PLANT:
			setBackgroundColor(GREEN);
			setColor(RED);
			c = '+';
			break;
		case CWENT_VASE:
			setColor(BLUE);
			c = 'O';
			break;
		case CWENT_TABLE:
			setColor(BROWN);
			c = '#';
			break;
		case CWENT_CEILING_LIGHT_GREEN:
			setBackgroundColor(GREY);
			setColor(GREEN);
			c = '+';
			break;
		case CWENT_UTENSILS_BROWN_CAGE_BLOODY_BONES:
			if (type == CWMAPTYPE_SOD)
			{
				setBackgroundColor(LIGHTMAGENTA);
				setColor(LIGHTRED);
				c = '+';
			}
			else
			{
				setBackgroundColor(CYAN);
				setColor(RED);
				c = '+';
			}
			break;
		case CWENT_ARMOR:
			setColor(BLUE);
			c = '|';
			break;
		case CWENT_CAGE:
			setBackgroundColor(LIGHTMAGENTA);
			setColor(BLACK);
			c = '+';
			break;
		case CWENT_CAGE_SKELETON:
			setBackgroundColor(LIGHTMAGENTA);
			setColor(WHITE);
			c = '+';
			break;
		case CWENT_BONES1:
			setColor(BROWN);
			c = 'H';
			break;
		case CWENT_KEY_GOLD:
			setColor(YELLOW);
			c = 'X';
			break;
		case CWENT_KEY_SILVER:
			setColor(LIGHTCYAN);
			c = 'X';
			break;
		case CWENT_BED_CAGE_SKULLS:
			if (type == CWMAPTYPE_SOD)
			{
				setBackgroundColor(LIGHTMAGENTA);
				setColor(RED);
				c = '+';
			}
			else
			{
				setColor(RED);
				c = '~';
			}
			break;
		case CWENT_BASKET:
			setBackgroundColor(RED);
			setColor(BLACK);
			c = '+';
			break;
		case CWENT_FOOD:
			setColor(LIGHTGREEN);
			c = '=';
			break;
		case CWENT_MEDKIT:
			setColor(LIGHTRED);
			c = '=';
			break;
		case CWENT_AMMO:
			setColor(CYAN);
			c = '=';
			break;
		case CWENT_MACHINE_GUN:
			setColor(CYAN);
			c = 'O';
			break;
		case CWENT_CHAIN_GUN:
			setColor(LIGHTCYAN);
			c = 'O';
			break;
		case CWENT_CROSS:
			setColor(LIGHTBLUE);
			c = '*';
			break;
		case CWENT_CHALICE:
			setColor(LIGHTMAGENTA);
			c = '*';
			break;
		case CWENT_CHEST:
			setColor(LIGHTCYAN);
			c = '*';
			break;
		case CWENT_CROWN:
			setColor(WHITE);
			c = '*';
			break;
		case CWENT_LIFE:
			setColor(LIGHTBLUE);
			c = '=';
			break;
		case CWENT_BONES_BLOOD:
			setColor(LIGHTRED);
			c = 'H';
			break;
		case CWENT_BARREL:
			setColor(RED);
			c = '|';
			break;
		case CWENT_WELL_WATER:
			setBackgroundColor(GREY);
			setColor(LIGHTBLUE);
			c = '+';
			break;
		case CWENT_WELL:
			setBackgroundColor(GREY);
			setColor(BLACK);
			c = '+';
			break;
		case CWENT_POOL_OF_BLOOD:
			setColor(LIGHTRED);
			c = 'O';
			break;
		case CWENT_FLAG:
			setColor(LIGHTRED);
			c = '|';
			break;
		case CWENT_CEILING_LIGHT_RED_AARDWOLF:
			if (type == CWMAPTYPE_WL6)
			{
				setBackgroundColor(LIGHTGREEN);
			}
			else
			{
				setBackgroundColor(GREY);
				setColor(LIGHTRED);
				c = '+';
				break;
			}
			break;
		case CWENT_BONES2:
			setColor(GREY);
			c = 'H';
			break;
		case CWENT_BONES3:
			setColor(YELLOW);
			c = 'H';
			break;
		case CWENT_BONES4:
			setColor(WHITE);
			c = 'H';
			break;
		case CWENT_UTENSILS_BLUE_COW_SKULL:
			if (type == CWMAPTYPE_SOD)
			{
				setColor(MAGENTA);
				c = '#';
			}
			else
			{
				setBackgroundColor(CYAN);
				setColor(LIGHTBLUE);
				c = '+';
			}
			break;
		case CWENT_STOVE_WELL_BLOOD:
			if (type == CWMAPTYPE_SOD)
			{
				setBackgroundColor(GREY);
				setColor(RED);
				c = '+';
			}
			else
			{
				setColor(GREY);
				c = '#';
			}
			break;
		case CWENT_RACK_ANGEL_STATUE:
			if (type == CWMAPTYPE_SOD)
			{
				setColor(CYAN);
				c = '#';
			}
			else
			{
				setBackgroundColor(RED);
				setColor(BLACK);
				c = '#';
			}
			break;
		case CWENT_VINES:
			setColor(GREEN);
			c = '#';
			break;
		case CWENT_BROWN_COLUMN:
			setColor(CYAN);
			c = '|';
			break;
		case CWENT_AMMO_BOX:
			setColor(CYAN);
			c = 'H';
			break;
		case CWENT_TRUCK_REAR:
			setColor(BLUE);
			c = '#';
			break;
		case CWENT_SPEAR:
			setColor(YELLOW);
			c = '$';
			break;
		case CWENT_PUSHWALL:
			setBackgroundColor(WHITE);
			break;
		case CWENT_ENDGAME:
			setColor(RED);
			c = '$';
			break;
		case CWENT_GHOST:
			setColor(LIGHTCYAN);
			c = '$';
			break;
		case CWENT_ANGEL:
			setColor(LIGHTRED);
			c = '$';
			break;
		case CWENT_DEAD_GUARD:
			setColor(MAGENTA);
			c = 'X';
			break;
		case CWENT_DOG_E:
			setColor(BROWN);
			c = '>';
			break;
		case CWENT_DOG_N:
			setColor(BROWN);
			c = '^';
			break;
		case CWENT_DOG_W:
			setColor(BROWN);
			c = '<';
			break;
		case CWENT_DOG_S:
			setColor(BROWN);
			c = 'v';
			break;
		case CWENT_GUARD_E:
			setColor(RED);
			c = '>';
			break;
		case CWENT_GUARD_N:
			setColor(RED);
			c = '^';
			break;
		case CWENT_GUARD_W:
			setColor(RED);
			c = '<';
			break;
		case CWENT_GUARD_S:
			setColor(RED);
			c = 'v';
			break;
		case CWENT_SS_E:
			setColor(BLUE);
			c = '>';
			break;
		case CWENT_SS_N:
			setColor(BLUE);
			c = '^';
			break;
		case CWENT_SS_W:
			setColor(BLUE);
			c = '<';
			break;
		case CWENT_SS_S:
			setColor(BLUE);
			c = 'v';
			break;
		case CWENT_MUTANT_E:
			setColor(GREEN);
			c = '>';
			break;
		case CWENT_MUTANT_N:
			setColor(GREEN);
			c = '^';
			break;
		case CWENT_MUTANT_W:
			setColor(GREEN);
			c = '<';
			break;
		case CWENT_MUTANT_S:
			setColor(GREEN);
			c = 'v';
			break;
		case CWENT_OFFICER_E:
			setColor(GREY);
			c = '>';
			break;
		case CWENT_OFFICER_N:
			setColor(GREY);
			c = '^';
			break;
		case CWENT_OFFICER_W:
			setColor(GREY);
			c = '<';
			break;
		case CWENT_OFFICER_S:
			setColor(GREY);
			c = 'v';
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
			setColor(GREEN);
			c = '$';
			break;
		case CWENT_UBER_MUTANT:
			setColor(LIGHTGREEN);
			c = '$';
			break;
		case CWENT_BARNACLE_WILHELM:
			setColor(LIGHTBLUE);
			c = '$';
			break;
		case CWENT_ROBED_HITLER:
			setColor(DARKGREY);
			c = '$';
			break;
		case CWENT_DEATH_KNIGHT:
			setColor(DARKGREY);
			c = '$';
			break;
		case CWENT_HITLER:
			setColor(LIGHTCYAN);
			c = '$';
			break;
		case CWENT_FETTGESICHT:
			setColor(LIGHTGREEN);
			c = '$';
			break;
		case CWENT_SCHABBS:
			setColor(LIGHTMAGENTA);
			c = '$';
			break;
		case CWENT_GRETEL:
			setColor(YELLOW);
			c = '$';
			break;
		case CWENT_HANS:
			setColor(LIGHTBLUE);
			c = '$';
			break;
		case CWENT_OTTO:
			setColor(WHITE);
			c = '$';
			break;
		case CWENT_PACMAN_GHOST_RED:
			setColor(LIGHTRED);
			c = 'G';
			break;
		case CWENT_PACMAN_GHOST_YELLOW:
			setColor(YELLOW);
			c = 'G';
			break;
		case CWENT_PACMAN_GHOST_ROSE:
			setColor(LIGHTMAGENTA);
			c = 'G';
			break;
		case CWENT_PACMAN_GHOST_BLUE:
			setColor(LIGHTBLUE);
			c = 'G';
			break;*/
		default:
			// CASSERT(false, "unknown entity");
			break;
	}
}
