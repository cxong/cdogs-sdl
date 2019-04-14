/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (C) 1995 Ronny Wester
    Copyright (C) 2003 Jeremy Chin
    Copyright (C) 2003-2007 Lucas Martin-King

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    This file incorporates work covered by the following copyright and
    permission notice:

    Copyright (c) 2013-2016, 2018-2019 Cong Xu
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
#include "files.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <SDL.h>

#include "door.h"
#include "log.h"
#include "map_new.h"
#include "palette.h"
#include "sys_specifics.h"
#include "utils.h"

#define MAX_STRING_LEN 1000

#define CAMPAIGN_MAGIC    690304
#define CAMPAIGN_VERSION  6

#if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
static const int bendian = 1;
#else
static const int bendian = 0;
#endif

void swap32 (void *d)
{
	if (bendian)
	{
		*((int32_t *)d) = SDL_Swap32(*((int32_t *)d));
	}
}

size_t f_read(FILE *f, void *buf, size_t size)
{
	return fread(buf, size, 1, f);
}

size_t f_read32(FILE *f, void *buf, size_t size)
{
	size_t ret = 0;
	if (buf) {
		ret = f_read(f, buf, size);
		swap32((int *)buf);
	}
	return ret;
}

void swap16 (void *d)
{
	if (bendian)
	{
		*((int16_t *)d) = SDL_Swap16(*((int16_t *)d));
	}
}

size_t f_read16(FILE *f, void *buf, size_t size)
{
	size_t ret = 0;
	if (buf)
	{
		ret = f_read(f, buf, size);
		swap16((int16_t *)buf);
	}
	return ret;
}

int fwrite32(FILE *f, void *buf)
{
	size_t ret = fwrite(buf, 4, 1, f);
	if (ret != 1)
	{
		return 0;
	}
	return 1;
}


int IsCampaignOldFile(const char *filename)
{
	int32_t i;
	FILE *f = fopen(filename, "rb");
	if (f == NULL)
	{
		return 0;
	}
	f_read32(f, &i, sizeof i);
	fclose(f);
	return i == CAMPAIGN_MAGIC;
}

int ScanCampaignOld(const char *filename, char **title, int *missions)
{
	FILE *f;
	int i;
	CampaignSettingOld setting;

	f = fopen(filename, "rb");
	if (f != NULL)
	{
		f_read32(f, &i, sizeof(i));

		if (i != CAMPAIGN_MAGIC) {
			fclose(f);
			return -1;
		}

		f_read32(f, &i, sizeof(i));
		if (i != CAMPAIGN_VERSION) {
			fclose(f);
			return -1;
		}

		f_read(f, setting.title, sizeof(setting.title));
		f_read(f, setting.author, sizeof(setting.author));
		f_read(f, setting.description, sizeof(setting.description));
		f_read32(f, &setting.missionCount, sizeof(setting.missionCount));
		CSTRDUP(*title, setting.title);
		*missions = setting.missionCount;

		fclose(f);

		return 0;
	}
	perror("ScanCampaignOld - couldn't read file:");
	return -1;
}

#define R32(v) { int32_t _n; f_read32(f, &_n, sizeof _n); (v) = _n; }

static void load_mission_objective(FILE *f, struct MissionObjectiveOld *o)
{
	f_read(f, o->description, sizeof(o->description));
	f_read32(f, &o->type, sizeof(o->type));
	f_read32(f, &o->index, sizeof(o->index));
	f_read32(f, &o->count, sizeof(o->count));
	f_read32(f, &o->required, sizeof(o->required));
	f_read32(f, &o->flags, sizeof(o->flags));
}

static void load_mission(FILE *f, struct MissionOld *m)
{
	int i;

	f_read(f, m->title, sizeof(m->title));
	f_read(f, m->description, sizeof(m->description));

	R32(m->wallStyle);
	R32(m->floorStyle);
	R32(m->roomStyle);
	R32(m->exitStyle);
	R32(m->keyStyle);
	R32(m->doorStyle);

	R32(m->mapWidth); R32(m->mapHeight);
	R32(m->wallCount); R32(m->wallLength);
	R32(m->roomCount);
	R32(m->squareCount);

	R32(m->exitLeft); R32(m->exitTop); R32(m->exitRight); R32(m->exitBottom);

	R32(m->objectiveCount);

	for (i = 0; i < OBJECTIVE_MAX; i++) {
		load_mission_objective(f, &m->objectives[i]);
	}

	R32(m->baddieCount);
	for (i = 0; i < BADDIE_MAX; i++) {
		f_read32(f, &m->baddies[i], sizeof(int));
	}

	R32(m->specialCount);
	for (i = 0; i < SPECIAL_MAX; i++) {
		f_read32(f, &m->specials[i], sizeof(int));
	}

	R32(m->itemCount);
	for (i = 0; i < ITEMS_MAX; i++) {
		f_read32(f, &m->items[i], sizeof(int));
	}
	for (i = 0; i < ITEMS_MAX; i++) {
		f_read32(f, &m->itemDensity[i], sizeof(int));
	}

	R32(m->baddieDensity);
	R32(m->weaponSelection);

	f_read(f, m->song, sizeof(m->song));
	f_read(f, m->map, sizeof(m->map));

	R32(m->wallRange);
	R32(m->floorRange);
	R32(m->roomRange);
	R32(m->altRange);
}


// Taken from the 5th colour in the old ColorShades
// See original character.c
static unsigned char cShadePalettes[] =
{
	56,
	6,
	72,
	88,
	104,
	120,
	136,
	36,
	40,
	45,
	148,
	8,
	1,
	20
};
void ConvertCharacterColors(
	const int skin, const int arm, const int body, const int leg,
	const int hair, CharColors *c)
{
	// Brighten the shades to be compatible with new system
	const HSV tint = {-1.0, 1.0, 1.7};
	c->Skin = ColorTint(PaletteToColor(cShadePalettes[skin]), tint);
	c->Arms = ColorTint(PaletteToColor(cShadePalettes[arm]), tint);
	c->Body = ColorTint(PaletteToColor(cShadePalettes[body]), tint);
	c->Legs = ColorTint(PaletteToColor(cShadePalettes[leg]), tint);
	c->Hair = ColorTint(PaletteToColor(cShadePalettes[hair]), tint);
}

// Hair colour correction; some characters had no hair but now with
// specific parts of the head colourised using the hair colour; set
// default "hair" colour based on the head type
void ConvertHairColors(Character *ch)
{
	const color_t darkRed = { 0xC0, 0, 0, 0xFF };
	if (strcmp(ch->Class->Name, "Cyborg") == 0)
	{
		// eye
		ch->Colors.Hair = colorRed;
	}
	else if (strcmp(ch->Class->Name, "Ice") == 0)
	{
		// shades
		ch->Colors.Hair = colorBlack;
	}
	else if (strcmp(ch->Class->Name, "Ogre") == 0)
	{
		// eyes
		ch->Colors.Hair = darkRed;
	}
	else if (strcmp(ch->Class->Name, "Snake") == 0)
	{
		// eyepatch
		ch->Colors.Hair = colorBlack;
	}
	else if (strcmp(ch->Class->Name, "WarBaby") == 0)
	{
		// beret
		ch->Colors.Hair = colorRed;
	}
}

void load_character(FILE *f, TBadGuy *b)
{
	R32(b->armedBodyPic);
	R32(b->unarmedBodyPic);
	R32(b->facePic);
	R32(b->speed);
	R32(b->probabilityToMove);
	R32(b->probabilityToTrack);
	R32(b->probabilityToShoot);
	R32(b->actionDelay);
	R32(b->gun);
	R32(b->skinColor);
	R32(b->armColor);
	R32(b->bodyColor);
	R32(b->legColor);
	R32(b->hairColor);
	R32(b->health);
	R32(b->flags);
}
void ConvertCharacter(Character *c, TBadGuy *b)
{
	const char *face = IntCharacterFace(b->facePic);
	char *newFace = NULL;
	CharacterOldFaceToHair(face, &newFace, &c->Hair);
	c->Class = StrCharacterClass(newFace);
	CFREE(newFace);
	c->speed = b->speed / 256.0f;
	c->bot->probabilityToMove = b->probabilityToMove;
	c->bot->probabilityToTrack = b->probabilityToTrack;
	c->bot->probabilityToShoot = b->probabilityToShoot;
	c->bot->actionDelay = b->actionDelay;
	c->Gun = CArrayGet(&gWeaponClasses.Guns, b->gun);
	ConvertCharacterColors(
		b->skinColor, b->armColor, b->bodyColor, b->legColor, b->hairColor,
		&c->Colors);
	ConvertHairColors(c);
	c->maxHealth = b->health;
	c->flags = b->flags;
}
static void ConvertObjective(
	Objective *dest, struct MissionObjectiveOld *src)
{
	CSTRDUP(dest->Description, src->description);
	dest->Type = src->type;
	// Set objective colours based on type
	dest->color = ObjectiveTypeColor(dest->Type);
	// Index numbers used for all objective classes; convert them
	// to their class handles
	dest->u.Index = src->index;
	switch (dest->Type)
	{
	case OBJECTIVE_COLLECT:
		dest->u.Pickup = IntPickupClass(dest->u.Index);
		break;
	case OBJECTIVE_DESTROY:
		dest->u.MapObject = IntMapObject(dest->u.Index);
		break;
	default:
		// do nothing
		break;
	}
	dest->Count = src->count;
	dest->Required = src->required;
	if (dest->Required < 0)
	{
		dest->Required = 0;
	}
	dest->Flags = src->flags;
}
static void ConvertMission(
	Mission *dest, struct MissionOld *src, const int charCount)
{
	CFREE(dest->Title);
	CSTRDUP(dest->Title, src->title);
	CFREE(dest->Description);
	CSTRDUP(dest->Description, src->description);
	dest->Type = MAPTYPE_CLASSIC;
	dest->Size = svec2i(src->mapWidth, src->mapHeight);
	strcpy(dest->ExitStyle, IntExitStyle(src->exitStyle));
	strcpy(dest->KeyStyle, IntKeyStyle(src->keyStyle));
	for (int i = 0; i < src->objectiveCount; i++)
	{
		Objective o;
		ConvertObjective(&o, &src->objectives[i]);
		CArrayPushBack(&dest->Objectives, &o);
	}
	// Note: modulo for compatibility with older, buggy missions
	for (int i = 0; i < src->baddieCount; i++)
	{
		int n = src->baddies[i] % charCount;
		CArrayPushBack(&dest->Enemies, &n);
	}
	for (int i = 0; i < src->specialCount; i++)
	{
		int n = src->specials[i] % charCount;
		CArrayPushBack(&dest->SpecialChars, &n);
	}
	for (int i = 0; i < src->itemCount; i++)
	{
		MapObjectDensity mod;
		mod.M = IntMapObject(src->items[i]);
		mod.Density = src->itemDensity[i];
		CArrayPushBack(&dest->MapObjectDensities, &mod);
	}
	dest->EnemyDensity = src->baddieDensity;
	CArrayClear(&dest->Weapons);
	for (int i = 0; i < WEAPON_MAX; i++)
	{
		if ((src->weaponSelection & (1 << i)) || !src->weaponSelection)
		{
			WeaponClass *wc = CArrayGet(&gWeaponClasses.Guns, i);
			CArrayPushBack(&dest->Weapons, &wc);
		}
	}
	strcpy(dest->Song, src->song);
	const color_t maskAlt = RangeToColor(abs(src->altRange) % COLORRANGE_COUNT);

	TileClassInit(
		&dest->u.Classic.TileClasses.Wall, &gPicManager, &gTileWall,
		IntWallStyle(src->wallStyle), NULL,
		RangeToColor(abs(src->wallRange) % COLORRANGE_COUNT), maskAlt);
	TileClassInit(
		&dest->u.Classic.TileClasses.Floor, &gPicManager, &gTileFloor,
		IntFloorStyle(src->floorStyle), NULL,
		RangeToColor(abs(src->floorRange) % COLORRANGE_COUNT), maskAlt);
	TileClassInit(
		&dest->u.Classic.TileClasses.Room, &gPicManager, &gTileRoom,
		IntRoomStyle(src->roomStyle), NULL,
		RangeToColor(abs(src->roomRange) % COLORRANGE_COUNT), maskAlt);
	TileClassInit(
		&dest->u.Classic.TileClasses.Door, &gPicManager, &gTileDoor,
		IntDoorStyle(src->doorStyle), NULL, colorWhite, colorWhite);
	dest->u.Classic.Walls = src->wallCount;
	dest->u.Classic.WallLength = src->wallLength;
	dest->u.Classic.CorridorWidth = 1;
	dest->u.Classic.Rooms.Count = src->roomCount;
	dest->u.Classic.Rooms.Min = 6;
	dest->u.Classic.Rooms.Max = 10;
	dest->u.Classic.Rooms.Edge = 0;
	dest->u.Classic.Rooms.Walls = 0;
	dest->u.Classic.Squares = src->squareCount;
	dest->u.Classic.Doors.Enabled = 1;
	dest->u.Classic.Doors.Min = dest->u.Classic.Doors.Max = 1;
	dest->u.Classic.Pillars.Count = 0;
}

void ConvertCampaignSetting(CampaignSetting *dest, CampaignSettingOld *src)
{
	int i;
	CFREE(dest->Title);
	CSTRDUP(dest->Title, src->title);
	CFREE(dest->Author);
	CSTRDUP(dest->Author, src->author);
	CFREE(dest->Description);
	CSTRDUP(dest->Description, src->description);
	for (i = 0; i < src->missionCount; i++)
	{
		Mission m;
		MissionInit(&m);
		ConvertMission(&m, &src->missions[i], src->characterCount);
		CArrayPushBack(&dest->Missions, &m);
	}
	CharacterStoreTerminate(&dest->characters);
	CharacterStoreInit(&dest->characters);
	for (i = 0; i < src->characterCount; i++)
	{
		Character *ch = CharacterStoreAddOther(&dest->characters);
		ConvertCharacter(ch, &src->characters[i]);
	}
}

int LoadCampaignOld(const char *filename, CampaignSettingOld *setting)
{
	FILE *f = NULL;
	int32_t i;
	int err = 0;

	f = fopen(filename, "rb");
	if (f == NULL)
	{
		err = -1;
		goto bail;
	}

	f_read32(f, &i, sizeof(i));
	if (i != CAMPAIGN_MAGIC)
	{
		err = -1;
		goto bail;
	}

	f_read32(f, &i, sizeof(i));
	if (i != CAMPAIGN_VERSION)
	{
		err = -1;
		goto bail;
	}

	f_read(f, setting->title, sizeof(setting->title));
	f_read(f, setting->author, sizeof(setting->author));
	f_read(f, setting->description, sizeof(setting->description));

	f_read32(f, &setting->missionCount, sizeof(int32_t));
	CCALLOC(
		setting->missions,
		setting->missionCount * sizeof *setting->missions);
	for (i = 0; i < setting->missionCount; i++)
	{
		load_mission(f, &setting->missions[i]);
	}

	f_read32(f, &setting->characterCount, sizeof(int32_t));
	CCALLOC(
		setting->characters,
		setting->characterCount * sizeof *setting->characters);
	for (i = 0; i < setting->characterCount; i++)
	{
		load_character(f, &setting->characters[i]);
	}

bail:
	if (f != NULL)
	{
		fclose(f);
	}
	return err;
}

/* GetHomeDirectory ()
 *
 * Uses environment variables to determine the users home directory.
 * returns some sort of path (C string)
 *
 * It's an ugly piece of sh*t... :/
 */
char *cdogs_homepath = NULL;
const char *GetHomeDirectory(void)
{
#ifdef __EMSCRIPTEN__
    return "/persistent_data/";
#endif

	const char *p;

	if (cdogs_homepath != NULL)
	{
		return cdogs_homepath;
	}

	p = getenv("CDOGS_CONFIG_DIR");
	if (p != NULL && strlen(p) != 0)
	{
		CSTRDUP(cdogs_homepath, p);
		return cdogs_homepath;
	}

	p = getenv(HOME_DIR_ENV);
	if (p != NULL && strlen(p) != 0)
	{
		CCALLOC(cdogs_homepath, strlen(p) + 2);
		strcpy(cdogs_homepath, p);
		cdogs_homepath[strlen(p)] = '/';
		return cdogs_homepath;
	}

	fprintf(stderr,"%s%s%s%s",
	"##############################################################\n",
	"# You don't have the environment variables HOME or USER set. #\n",
	"# It is suggested you get a better shell. :D                 #\n",
	"##############################################################\n");
	return "";
}


/* GetConfigFilePath()
 *
 * returns a full path to a data file...
 */
char cfpath[CDOGS_PATH_MAX];
const char *GetConfigFilePath(const char *name)
{
	const char *homedir = GetHomeDirectory();

	strcpy(cfpath, homedir);

#ifndef __EMSCRIPTEN__
	strcat(cfpath, CDOGS_CFG_DIR);
#endif
	strcat(cfpath, name);

	return cfpath;
}

static bool doMkdir(const char *path)
{
	if (mkdir(path, MKDIR_MODE) != -1)
	{
		return true;
	}
	int e = errno;
	(void)e;
	// Mac OS X 10.4 returns EISDIR instead of EEXIST
	// if a dir already exists...
	return
		errno == EEXIST ||
		errno == EISDIR ||
		errno == EACCES;
}
bool mkdir_deep(const char *path)
{
	for (int i = 0; i < (int)strlen(path); i++)
	{
		if (path[i] == '\0') break;
		if (path[i] == '/')
		{
			char buf[CDOGS_PATH_MAX];
			strncpy(buf, path, i + 1);
			buf[i + 1] = '\0';
			if (!doMkdir(buf))
			{
				return false;
			}
		}
	}
	if (path[strlen(path) - 1] != '/' && !doMkdir(path))
	{
		return false;
	}
	return true;
}

void SetupConfigDir(void)
{
	const char *cfg_p = GetConfigFilePath("");

	LOG(LM_MAIN, LL_INFO, "Creating config dir %s... ", cfg_p);

	if (mkdir_deep(cfg_p))
	{
		if (errno != EEXIST) LOG(LM_MAIN, LL_INFO, "Config dir created.");
		else LOG(LM_MAIN, LL_INFO, "Config dir already exists.");
	}
	else
	{
		switch (errno)
		{
			case EACCES:
				LOG(LM_MAIN, LL_WARN, "Permission denied");
				break;
			default:
				perror("Error creating config directory:");
				break;
		}
	}

	return;
}


color_t RangeToColor(const int range)
{
	switch (range)
	{
	case 0: return colorMaroon;
	case 1: return colorLonestar;
	case 2: return colorRusticRed;
	case 3: return colorOfficeGreen;
	case 4: return colorPakistanGreen;
	case 5: return colorDarkFern;
	case 6: return colorNavyBlue;
	case 7: return colorArapawa;
	case 8: return colorStratos;
	case 9: return colorPatriarch;
	case 10: return colorPompadour;
	case 11: return colorLoulou;
	case 12: return colorBattleshipGrey;
	case 13: return colorDoveGray;
	case 14: return colorGravel;
	case 15: return colorComet;
	case 16: return colorFiord;
	case 17: return colorTuna;
	case 18: return colorHacienda;
	case 19: return colorKumera;
	case 20: return colorHimalaya;
	case 21: return colorChocolate;
	case 22: return colorNutmeg;
	case 23: return colorBracken;
	case 24: return colorTeal;
	case 25: return colorSkobeloff;
	case 26: return colorDeepJungleGreen;
	default:
		CASSERT(false, "unknown colour range");
		return colorBlack;
	};
}
