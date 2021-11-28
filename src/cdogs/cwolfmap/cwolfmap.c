#include "cwolfmap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _MSC_VER
#include <direct.h>
#include <io.h>
#define access _access
/* Values for the second argument to access.
   These may be OR'd together.  */
#define R_OK 4 /* Test for read permission.  */
#define W_OK 2 /* Test for write permission.  */
#define F_OK 0 /* Test for existence.  */
#else
#include <sys/time.h>
#include <unistd.h>
#endif

#include "audiowl6.h"
#include "expand.h"

#define MAGIC 0xABCD

#define PATH_MAX 4096

CWMapType CWGetType(
	const char *path, const char **ext, const char **ext1,
	const int spearMission)
{
	char pathBuf[PATH_MAX];
	sprintf(pathBuf, "%s/MAPHEAD.WL1", path);
	if (access(pathBuf, F_OK) != -1)
	{
		if (ext && ext1)
			*ext = *ext1 = "WL1";
		return CWMAPTYPE_WL1;
	}
	sprintf(pathBuf, "%s/MAPHEAD.WL6", path);
	if (access(pathBuf, F_OK) != -1)
	{
		if (ext && ext1)
			*ext = *ext1 = "WL6";
		return CWMAPTYPE_WL6;
	}
	sprintf(pathBuf, "%s/MAPHEAD.SD%d", path, spearMission);
	if (access(pathBuf, F_OK) != -1)
	{
		if (ext && ext1)
		{
			// Steam keeps common files as .SOD extension,
			// but specific files as .SD1/.SD2/.SD3 extension
			switch (spearMission)
			{
			case 1:
				*ext = "SD1";
				break;
			case 2:
				*ext = "SD2";
				break;
			case 3:
				*ext = "SD3";
				break;
			}
			*ext1 = "SOD";
		}
		return CWMAPTYPE_SOD;
	}
	else
	{
		sprintf(pathBuf, "%s/MAPHEAD.SOD", path);
		if (access(pathBuf, F_OK) != -1)
		{
			if (ext && ext1)
				*ext = *ext1 = "SOD";
			return CWMAPTYPE_SOD;
		}
	}
	return CWMAPTYPE_UNKNOWN;
}

static int LoadMapHead(CWolfMap *map, const char *path);
static int LoadMapData(CWolfMap *map, const char *path);
int CWLoad(CWolfMap *map, const char *path, const int spearMission)
{
	memset(map, 0, sizeof *map);
	char pathBuf[PATH_MAX];
	int err = 0;
	int loadErr = 0;

	const char *ext = "WL1";
	const char *ext1 = "WL1";
	map->type = CWGetType(path, &ext, &ext1, spearMission);
	if (map->type == CWMAPTYPE_UNKNOWN)
	{
		err = -1;
		fprintf(stderr, "Cannot find map at %s\n", path);
		goto bail;
	}

#define _TRY_LOAD(_fn, _loadFunc, ...)                                        \
	sprintf(pathBuf, "%s/" _fn ".%s", path, ext);                             \
	loadErr = _loadFunc(__VA_ARGS__);                                         \
	if (loadErr != 0)                                                         \
	{                                                                         \
		sprintf(pathBuf, "%s/" _fn ".%s", path, ext1);                        \
		loadErr = _loadFunc(__VA_ARGS__);                                     \
		if (loadErr != 0)                                                     \
		{                                                                     \
			err = loadErr;                                                    \
		}                                                                     \
	}
	_TRY_LOAD("MAPHEAD", LoadMapHead, map, pathBuf);

	_TRY_LOAD("GAMEMAPS", LoadMapData, map, pathBuf);

	_TRY_LOAD("AUDIOHED", CWAudioLoadHead, &map->audio.head, pathBuf);

	_TRY_LOAD("AUDIOT", CWAudioLoadAudioT, &map->audio, map->type, pathBuf);

	_TRY_LOAD("VSWAP", CWVSwapLoad, &map->vswap, pathBuf);

bail:
	return err;
}
static int LoadMapHead(CWolfMap *map, const char *path)
{
	int err = 0;
	memset(&map->mapHead, 0, sizeof map->mapHead);
	FILE *f = fopen(path, "rb");
	if (!f)
	{
		err = -1;
		fprintf(stderr, "Failed to read %s\n", path);
		goto bail;
	}
	const size_t size = sizeof map->mapHead;
	// Read as many maps as we can; some versions of the game (SOD MP) truncate
	// the headers
	(void)!fread((void *)&map->mapHead, 1, size, f);
	if (map->mapHead.magic != MAGIC)
	{
		err = -1;
		fprintf(stderr, "Unexpected magic value: %x\n", map->mapHead.magic);
		goto bail;
	}

bail:
	if (f)
	{
		fclose(f);
	}
	return err;
}

static void LevelsFree(CWolfMap *map);

static int LoadLevel(CWLevel *level, const unsigned char *data, const int off);
static int LoadMapData(CWolfMap *map, const char *path)
{
	int err = 0;
	unsigned char *buf = NULL;
	FILE *f = fopen(path, "rb");
	if (!f)
	{
		err = -1;
		fprintf(stderr, "Failed to read %s\n", path);
		goto bail;
	}
	fseek(f, 0, SEEK_END);
	const long fsize = ftell(f);
	fseek(f, 0, SEEK_SET);
	buf = malloc(fsize);
	if (fread(buf, 1, fsize, f) != (size_t)fsize)
	{
		err = -1;
		fprintf(stderr, "Failed to read file\n");
		goto bail;
	}

	LevelsFree(map);

	for (const int32_t *ptr = &map->mapHead.ptr[0]; *ptr > 0; ptr++)
	{
		map->nLevels++;
	}
	map->levels = malloc(sizeof(CWLevel) * map->nLevels);
	CWLevel *lPtr = map->levels;
	for (const int32_t *ptr = &map->mapHead.ptr[0]; *ptr > 0; ptr++, lPtr++)
	{
		err = LoadLevel(lPtr, buf, *ptr);
		if (err != 0)
		{
			goto bail;
		}
	}

bail:
	if (f)
	{
		fclose(f);
	}
	free(buf);
	if (err != 0)
	{
		CWFree(map);
	}
	return err;
}
static int LoadPlane(
	CWPlane *plane, const unsigned char *data, const uint32_t off,
	unsigned char *buf, const int bufSize);
static int LoadLevel(CWLevel *level, const unsigned char *data, const int off)
{
	int err = 0;
	unsigned char *buf = NULL;
	memcpy(&level->header, data + off, sizeof(level->header));

	const int bufSize =
		level->header.width * level->header.height * sizeof(uint16_t);
	buf = malloc(bufSize);
	for (int i = 0; i < NUM_PLANES; i++)
	{
		const uint32_t poff = *(&level->header.offPlane0 + i);
		err = LoadPlane(&level->planes[i], data, poff, buf, bufSize);
		if (err != 0)
		{
			goto bail;
		}
	}

	// Check if level has any player spawns
	level->hasPlayerSpawn = false;
	for (int y = 0; y < level->header.height && !level->hasPlayerSpawn; y++)
	{
		for (int x = 0; x < level->header.width && !level->hasPlayerSpawn; x++)
		{
			const uint16_t ch = CWLevelGetCh(level, 1, x, y);
			const CWEntity entity = CWChToEntity(ch);
			if (entity == CWENT_PLAYER_SPAWN_N ||
				entity == CWENT_PLAYER_SPAWN_E ||
				entity == CWENT_PLAYER_SPAWN_S ||
				entity == CWENT_PLAYER_SPAWN_W)
			{
				level->hasPlayerSpawn = true;
			}
		}
	}

bail:
	free(buf);
	return err;
}
static int LoadPlane(
	CWPlane *plane, const unsigned char *data, const uint32_t off,
	unsigned char *buf, const int bufSize)
{
	int err = 0;

	if (off == 0)
	{
		goto bail;
	}

	ExpandCarmack(data + off, buf);
	plane->len = bufSize;
	plane->plane = malloc(bufSize);
	ExpandRLEW(buf, (unsigned char *)plane->plane, MAGIC);

bail:
	return err;
}

void CWCopy(CWolfMap *dst, const CWolfMap *src)
{
	memcpy(dst, src, sizeof *dst);
	const size_t levelsSize = sizeof(CWLevel) * dst->nLevels;
	dst->levels = malloc(levelsSize);
	memcpy(dst->levels, src->levels, levelsSize);
	for (int i = 0; i < dst->nLevels; i++)
	{
		for (int j = 0; j < NUM_PLANES; j++)
		{
			const int len = dst->levels[i].planes[j].len;
			dst->levels[i].planes[j].plane = malloc(len);
			memcpy(
				dst->levels[i].planes[j].plane, src->levels[i].planes[j].plane,
				len);
		}
	}
	const size_t audioHeadLen = dst->audio.head.nOffsets * sizeof(uint32_t);
	dst->audio.head.offsets = malloc(audioHeadLen);
	memcpy(dst->audio.head.offsets, src->audio.head.offsets, audioHeadLen);
	const uint32_t audioDataLen =
		dst->audio.head.offsets[dst->audio.head.nOffsets - 1];
	dst->audio.data = malloc(audioDataLen);
	memcpy(dst->audio.data, src->audio.data, audioDataLen);
	dst->vswap.data = malloc(dst->vswap.dataLen);
	memcpy(dst->vswap.data, src->vswap.data, dst->vswap.dataLen);
	const size_t vswapSoundsLen = dst->vswap.nSounds * sizeof(CWVSwapSound);
	dst->vswap.sounds = malloc(vswapSoundsLen);
	memcpy(dst->vswap.sounds, src->vswap.sounds, vswapSoundsLen);
}

void CWFree(CWolfMap *map)
{
	if (map == NULL)
	{
		return;
	}
	LevelsFree(map);
	CWAudioFree(&map->audio);
	CWVSwapFree(&map->vswap);
	memset(map, 0, sizeof *map);
}
static void LevelFree(CWLevel *level);
static void LevelsFree(CWolfMap *map)
{
	for (int i = 0; i < map->nLevels; i++)
	{
		LevelFree(&map->levels[i]);
	}
	free(map->levels);
	map->nLevels = 0;
}
static void LevelFree(CWLevel *level)
{
	for (int i = 0; i < NUM_PLANES; i++)
	{
		free(level->planes[i].plane);
	}
}

// http://gaarabis.free.fr/_sites/specs/wlspec_index.html

uint16_t CWLevelGetCh(
	const CWLevel *level, const int planeIndex, const int x, const int y)
{
	const CWPlane *plane = &level->planes[planeIndex];
	return plane->plane[y * level->header.height + x];
}

static const CWTile tileMap[] = {
	// 0-63
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	// 64-79
	CWTILE_UNKNOWN,
	CWTILE_UNKNOWN,
	CWTILE_UNKNOWN,
	CWTILE_UNKNOWN,
	CWTILE_UNKNOWN,
	CWTILE_UNKNOWN,
	CWTILE_UNKNOWN,
	CWTILE_UNKNOWN,
	CWTILE_UNKNOWN,
	CWTILE_UNKNOWN,
	CWTILE_UNKNOWN,
	CWTILE_UNKNOWN,
	CWTILE_UNKNOWN,
	CWTILE_UNKNOWN,
	CWTILE_UNKNOWN,
	CWTILE_UNKNOWN,
	// 80
	CWTILE_UNKNOWN,
	CWTILE_UNKNOWN,
	CWTILE_UNKNOWN,
	CWTILE_UNKNOWN,
	CWTILE_UNKNOWN,
	CWTILE_AREA,
	CWTILE_UNKNOWN,
	CWTILE_UNKNOWN,
	CWTILE_UNKNOWN,
	CWTILE_UNKNOWN,
	// 90
	CWTILE_DOOR_V,
	CWTILE_DOOR_H,
	CWTILE_DOOR_GOLD_V,
	CWTILE_DOOR_GOLD_H,
	CWTILE_DOOR_SILVER_V,
	CWTILE_DOOR_SILVER_H,
	// 96-99
	CWTILE_UNKNOWN,
	CWTILE_UNKNOWN,
	CWTILE_UNKNOWN,
	CWTILE_UNKNOWN,
	// 100
	CWTILE_ELEVATOR_V,
	CWTILE_ELEVATOR_H,
	// 102-105
	CWTILE_UNKNOWN,
	CWTILE_UNKNOWN,
	CWTILE_UNKNOWN,
	CWTILE_UNKNOWN,
	// 106-143
	CWTILE_AREA,
	CWTILE_SECRET_EXIT,
	CWTILE_AREA,
	CWTILE_AREA,
	CWTILE_AREA,
	CWTILE_AREA,
	CWTILE_AREA,
	CWTILE_AREA,
	CWTILE_AREA,
	CWTILE_AREA,
	CWTILE_AREA,
	CWTILE_AREA,
	CWTILE_AREA,
	CWTILE_AREA,
	CWTILE_AREA,
	CWTILE_AREA,
	CWTILE_AREA,
	CWTILE_AREA,
	CWTILE_AREA,
	CWTILE_AREA,
	CWTILE_AREA,
	CWTILE_AREA,
	CWTILE_AREA,
	CWTILE_AREA,
	CWTILE_AREA,
	CWTILE_AREA,
	CWTILE_AREA,
	CWTILE_AREA,
	CWTILE_AREA,
	CWTILE_AREA,
	CWTILE_AREA,
	CWTILE_AREA,
	CWTILE_AREA,
	CWTILE_AREA,
	CWTILE_AREA,
	CWTILE_AREA,
	CWTILE_AREA,
	CWTILE_AREA,
};

CWTile CWChToTile(const uint16_t ch)
{
	if (ch < sizeof tileMap / sizeof tileMap[0])
	{
		return tileMap[ch];
	}
	return CWTILE_UNKNOWN;
}

static const CWWall wallMap[] = {
	CWWALL_UNKNOWN,
	CWWALL_GREY_BRICK_1,
	CWWALL_GREY_BRICK_2,
	CWWALL_GREY_BRICK_FLAG,
	CWWALL_GREY_BRICK_HITLER,
	CWWALL_CELL,
	CWWALL_GREY_BRICK_EAGLE,
	CWWALL_CELL_SKELETON,
	CWWALL_BLUE_BRICK_1,
	CWWALL_BLUE_BRICK_2,
	// 10
	CWWALL_WOOD_EAGLE,
	CWWALL_WOOD_HITLER,
	CWWALL_WOOD,
	CWWALL_ENTRANCE,
	CWWALL_STEEL_SIGN,
	CWWALL_STEEL,
	CWWALL_LANDSCAPE,
	CWWALL_RED_BRICK,
	CWWALL_RED_BRICK_SWASTIKA,
	CWWALL_PURPLE,
	// 20
	CWWALL_RED_BRICK_FLAG,
	CWWALL_ELEVATOR,
	CWWALL_DEAD_ELEVATOR,
	CWWALL_WOOD_IRON_CROSS,
	CWWALL_DIRTY_BRICK_1,
	CWWALL_PURPLE_BLOOD,
	CWWALL_DIRTY_BRICK_2,
	CWWALL_GREY_BRICK_3,
	CWWALL_GREY_BRICK_SIGN,
	CWWALL_BROWN_WEAVE,
	// 30
	CWWALL_BROWN_WEAVE_BLOOD_2,
	CWWALL_BROWN_WEAVE_BLOOD_3,
	CWWALL_BROWN_WEAVE_BLOOD_1,
	CWWALL_STAINED_GLASS,
	CWWALL_BLUE_WALL_SKULL,
	CWWALL_GREY_WALL_1,
	CWWALL_BLUE_WALL_SWASTIKA,
	CWWALL_GREY_WALL_VENT,
	CWWALL_MULTICOLOR_BRICK,
	CWWALL_GREY_WALL_2,
	// 40
	CWWALL_BLUE_WALL,
	CWWALL_BLUE_BRICK_SIGN,
	CWWALL_BROWN_MARBLE_1,
	CWWALL_GREY_WALL_MAP,
	CWWALL_BROWN_STONE_1,
	CWWALL_BROWN_STONE_2,
	CWWALL_BROWN_MARBLE_2,
	CWWALL_BROWN_MARBLE_FLAG,
	CWWALL_WOOD_PANEL,
	CWWALL_GREY_WALL_HITLER,
	// 50
	CWWALL_STONE_WALL_1,
	CWWALL_STONE_WALL_2,
	CWWALL_STONE_WALL_FLAG,
	CWWALL_STONE_WALL_WREATH,
	CWWALL_GREY_CONCRETE_LIGHT,
	CWWALL_GREY_CONCRETE_DARK,
	CWWALL_BLOOD_WALL,
	CWWALL_CONCRETE,
	CWWALL_RAMPART_STONE_1,
	CWWALL_RAMPART_STONE_2,
	// 60
	CWWALL_ELEVATOR_WALL,
	CWWALL_WHITE_PANEL,
	CWWALL_BROWN_CONCRETE,
	CWWALL_PURPLE_BRICK,
	CWWALL_UNKNOWN,
	CWWALL_UNKNOWN,
	CWWALL_UNKNOWN,
	CWWALL_UNKNOWN,
	CWWALL_UNKNOWN,
	CWWALL_UNKNOWN,
};

CWWall CWChToWall(const uint16_t ch)
{
	if (ch < sizeof wallMap / sizeof wallMap[0])
	{
		return wallMap[ch];
	}
	return CWWALL_UNKNOWN;
}

static const CWEntity entityMap[] = {
	CWENT_NONE,
	CWENT_UNKNOWN,
	CWENT_UNKNOWN,
	CWENT_UNKNOWN,
	CWENT_UNKNOWN,
	CWENT_UNKNOWN,
	CWENT_UNKNOWN,
	CWENT_UNKNOWN,
	CWENT_UNKNOWN,
	CWENT_UNKNOWN,
	CWENT_UNKNOWN,
	CWENT_UNKNOWN,
	CWENT_UNKNOWN,
	CWENT_UNKNOWN,
	CWENT_UNKNOWN,
	CWENT_UNKNOWN,
	CWENT_UNKNOWN,
	CWENT_UNKNOWN,
	CWENT_UNKNOWN,
	CWENT_PLAYER_SPAWN_N,
	// 20
	CWENT_PLAYER_SPAWN_E,
	CWENT_PLAYER_SPAWN_S,
	CWENT_PLAYER_SPAWN_W,
	CWENT_WATER,
	CWENT_OIL_DRUM,
	CWENT_TABLE_WITH_CHAIRS,
	CWENT_FLOOR_LAMP,
	CWENT_CHANDELIER,
	CWENT_HANGING_SKELETON,
	CWENT_DOG_FOOD,
	// 30
	CWENT_WHITE_COLUMN,
	CWENT_GREEN_PLANT,
	CWENT_SKELETON,
	CWENT_SINK_SKULLS_ON_STICK,
	CWENT_BROWN_PLANT,
	CWENT_VASE,
	CWENT_TABLE,
	CWENT_CEILING_LIGHT_GREEN,
	CWENT_UTENSILS_BROWN_CAGE_BLOODY_BONES,
	CWENT_ARMOR,
	// 40
	CWENT_CAGE,
	CWENT_CAGE_SKELETON,
	CWENT_BONES1,
	CWENT_KEY_GOLD,
	CWENT_KEY_SILVER,
	CWENT_BED_CAGE_SKULLS,
	CWENT_BASKET,
	CWENT_FOOD,
	CWENT_MEDKIT,
	CWENT_AMMO,
	// 50
	CWENT_MACHINE_GUN,
	CWENT_CHAIN_GUN,
	CWENT_CROSS,
	CWENT_CHALICE,
	CWENT_CHEST,
	CWENT_CROWN,
	CWENT_LIFE,
	CWENT_BONES_BLOOD,
	CWENT_BARREL,
	CWENT_WELL_WATER,
	// 60
	CWENT_WELL,
	CWENT_POOL_OF_BLOOD,
	CWENT_FLAG,
	CWENT_CEILING_LIGHT_RED_AARDWOLF,
	CWENT_BONES2,
	CWENT_BONES3,
	CWENT_BONES4,
	CWENT_UTENSILS_BLUE_COW_SKULL,
	CWENT_STOVE_WELL_BLOOD,
	CWENT_RACK_ANGEL_STATUE,
	// 70
	CWENT_VINES,
	CWENT_BROWN_COLUMN,
	CWENT_AMMO_BOX,
	CWENT_TRUCK_REAR,
	CWENT_SPEAR,
	CWENT_UNKNOWN,
	CWENT_UNKNOWN,
	CWENT_UNKNOWN,
	CWENT_UNKNOWN,
	CWENT_UNKNOWN,
	// 80
	CWENT_UNKNOWN,
	CWENT_UNKNOWN,
	CWENT_UNKNOWN,
	CWENT_UNKNOWN,
	CWENT_UNKNOWN,
	CWENT_UNKNOWN,
	CWENT_UNKNOWN,
	CWENT_UNKNOWN,
	CWENT_UNKNOWN,
	CWENT_UNKNOWN,
	// 90
	CWENT_TURN_E,
	CWENT_TURN_NE,
	CWENT_TURN_N,
	CWENT_TURN_NW,
	CWENT_TURN_W,
	CWENT_TURN_SW,
	CWENT_TURN_S,
	CWENT_TURN_SE,
	CWENT_PUSHWALL,
	CWENT_ENDGAME,
	// 100
	CWENT_UNKNOWN,
	CWENT_UNKNOWN,
	CWENT_UNKNOWN,
	CWENT_UNKNOWN,
	CWENT_UNKNOWN,
	CWENT_UNKNOWN,
	CWENT_GHOST,
	CWENT_ANGEL,
	CWENT_GUARD_E,
	CWENT_GUARD_N,
	// 110
	CWENT_GUARD_W,
	CWENT_GUARD_S,
	CWENT_GUARD_MOVING_E,
	CWENT_GUARD_MOVING_N,
	CWENT_GUARD_MOVING_W,
	CWENT_GUARD_MOVING_S,
	CWENT_OFFICER_E,
	CWENT_OFFICER_N,
	CWENT_OFFICER_W,
	CWENT_OFFICER_S,
	// 120
	CWENT_OFFICER_MOVING_E,
	CWENT_OFFICER_MOVING_N,
	CWENT_OFFICER_MOVING_W,
	CWENT_OFFICER_MOVING_S,
	CWENT_DEAD_GUARD,
	CWENT_TRANS,
	CWENT_SS_E,
	CWENT_SS_N,
	CWENT_SS_W,
	CWENT_SS_S,
	// 130
	CWENT_SS_MOVING_E,
	CWENT_SS_MOVING_N,
	CWENT_SS_MOVING_W,
	CWENT_SS_MOVING_S,
	CWENT_UNKNOWN,
	CWENT_UNKNOWN,
	CWENT_UNKNOWN,
	CWENT_UNKNOWN,
	CWENT_DOG_E,
	CWENT_DOG_N,
	// 140
	CWENT_DOG_W,
	CWENT_DOG_S,
	CWENT_UBER_MUTANT,
	CWENT_BARNACLE_WILHELM,
	CWENT_GUARD_E,
	CWENT_GUARD_N,
	CWENT_GUARD_W,
	CWENT_GUARD_S,
	CWENT_GUARD_MOVING_E,
	CWENT_GUARD_MOVING_N,
	// 150
	CWENT_GUARD_MOVING_W,
	CWENT_GUARD_MOVING_S,
	CWENT_OFFICER_E,
	CWENT_OFFICER_N,
	CWENT_OFFICER_W,
	CWENT_OFFICER_S,
	CWENT_OFFICER_MOVING_E,
	CWENT_OFFICER_MOVING_N,
	CWENT_OFFICER_MOVING_W,
	CWENT_OFFICER_MOVING_S,
	// 160
	CWENT_ROBED_HITLER,
	CWENT_DEATH_KNIGHT,
	CWENT_SS_E,
	CWENT_SS_N,
	CWENT_SS_W,
	CWENT_SS_S,
	CWENT_SS_MOVING_E,
	CWENT_SS_MOVING_N,
	CWENT_SS_MOVING_W,
	CWENT_SS_MOVING_S,
	// 170
	CWENT_UNKNOWN,
	CWENT_UNKNOWN,
	CWENT_UNKNOWN,
	CWENT_UNKNOWN,
	CWENT_DOG_E,
	CWENT_DOG_N,
	CWENT_DOG_W,
	CWENT_DOG_S,
	CWENT_HITLER,
	CWENT_FETTGESICHT,
	// 180
	CWENT_GUARD_E,
	CWENT_GUARD_N,
	CWENT_GUARD_W,
	CWENT_GUARD_S,
	CWENT_GUARD_MOVING_E,
	CWENT_GUARD_MOVING_N,
	CWENT_GUARD_MOVING_W,
	CWENT_GUARD_MOVING_S,
	CWENT_OFFICER_E,
	CWENT_OFFICER_N,
	// 190
	CWENT_OFFICER_W,
	CWENT_OFFICER_S,
	CWENT_OFFICER_MOVING_E,
	CWENT_OFFICER_MOVING_N,
	CWENT_OFFICER_MOVING_W,
	CWENT_OFFICER_MOVING_S,
	CWENT_SCHABBS,
	CWENT_GRETEL,
	CWENT_SS_E,
	CWENT_SS_N,
	// 200
	CWENT_SS_W,
	CWENT_SS_S,
	CWENT_SS_MOVING_E,
	CWENT_SS_MOVING_N,
	CWENT_SS_MOVING_W,
	CWENT_SS_MOVING_S,
	CWENT_UNKNOWN,
	CWENT_UNKNOWN,
	CWENT_UNKNOWN,
	CWENT_UNKNOWN,
	// 210
	CWENT_DOG_E,
	CWENT_DOG_N,
	CWENT_DOG_W,
	CWENT_DOG_S,
	CWENT_HANS,
	CWENT_OTTO,
	CWENT_MUTANT_E,
	CWENT_MUTANT_N,
	CWENT_MUTANT_W,
	CWENT_MUTANT_S,
	// 220
	CWENT_MUTANT_MOVING_E,
	CWENT_MUTANT_MOVING_N,
	CWENT_MUTANT_MOVING_W,
	CWENT_MUTANT_MOVING_S,
	CWENT_PACMAN_GHOST_RED,
	CWENT_PACMAN_GHOST_YELLOW,
	CWENT_PACMAN_GHOST_ROSE,
	CWENT_PACMAN_GHOST_BLUE,
	CWENT_UNKNOWN,
	CWENT_UNKNOWN,
	// 230
	CWENT_UNKNOWN,
	CWENT_UNKNOWN,
	CWENT_UNKNOWN,
	CWENT_UNKNOWN,
	CWENT_MUTANT_E,
	CWENT_MUTANT_N,
	CWENT_MUTANT_W,
	CWENT_MUTANT_S,
	CWENT_MUTANT_MOVING_E,
	CWENT_MUTANT_MOVING_N,
	// 240
	CWENT_MUTANT_MOVING_W,
	CWENT_MUTANT_MOVING_S,
	CWENT_UNKNOWN,
	CWENT_UNKNOWN,
	CWENT_UNKNOWN,
	CWENT_UNKNOWN,
	CWENT_UNKNOWN,
	CWENT_UNKNOWN,
	CWENT_UNKNOWN,
	CWENT_UNKNOWN,
	// 250
	CWENT_UNKNOWN,
	CWENT_UNKNOWN,
	CWENT_MUTANT_E,
	CWENT_MUTANT_N,
	CWENT_MUTANT_W,
	CWENT_MUTANT_S,
	CWENT_MUTANT_MOVING_E,
	CWENT_MUTANT_MOVING_N,
	CWENT_MUTANT_MOVING_W,
	CWENT_MUTANT_MOVING_S,
};

CWEntity CWChToEntity(const uint16_t ch)
{
	if (ch < sizeof entityMap / sizeof entityMap[0])
	{
		return entityMap[ch];
	}
	return CWENT_UNKNOWN;
}
