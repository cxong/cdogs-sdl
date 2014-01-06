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

    Copyright (c) 2013-2014, Cong Xu
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

#include "map_new.h"
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


int ScanCampaign(const char *filename, char *title, int *missions)
{
	FILE *f;
	int i;
	CampaignSettingOld setting;

	debug(D_NORMAL, "filename: %s\n", filename);

	f = fopen(filename, "rb");
	if (f != NULL)
	{
		f_read32(f, &i, sizeof(i));

		if (i != CAMPAIGN_MAGIC) {
			fclose(f);
			debug(D_NORMAL, "Filename: %s\n", filename);
			debug(D_NORMAL, "Magic: %d FileM: %d\n", CAMPAIGN_MAGIC, i);
			debug(D_NORMAL, "ScanCampaign - bad file!\n");
			return CAMPAIGN_BADFILE;
		}

		f_read32(f, &i, sizeof(i));
		if (i != CAMPAIGN_VERSION) {
			fclose(f);
			debug(
				D_NORMAL,
				"ScanCampaign - version mismatch (expected %d, read %d)\n",
				CAMPAIGN_VERSION, i);
			return CAMPAIGN_VERSIONMISMATCH;
		}

		f_read(f, setting.title, sizeof(setting.title));
		f_read(f, setting.author, sizeof(setting.author));
		f_read(f, setting.description, sizeof(setting.description));
		f_read32(f, &setting.missionCount, sizeof(setting.missionCount));
		strcpy(title, setting.title);
		*missions = setting.missionCount;

		fclose(f);

		return CAMPAIGN_OK;
	}
	perror("ScanCampaign - couldn't read file:");
	return CAMPAIGN_BADPATH;
}

#define R32(v) { int32_t _n; f_read32(f, &_n, sizeof _n); (v) = _n; }

static void load_mission_objective(FILE *f, MissionObjective *o)
{
	char buf[128];
	f_read(f, buf, sizeof(((struct MissionObjectiveOld *)0)->description));
	CFREE(o->Description);
	CSTRDUP(o->Description, buf);
	R32(o->Type);
	R32(o->Index);
	R32(o->Count);
	R32(o->Required);
	R32(o->Flags);
	debug(D_VERBOSE, " >> Objective: %s data: %d %d %d %d %d\n",
		o->Description, o->Type, o->Index, o->Count, o->Required, o->Flags);
}

static void load_mission(FILE *f, Mission *m)
{
	int i;
	char buf[512];
	int32_t c;

	f_read(f, buf, sizeof(((struct MissionOld *)0)->title));
	CFREE(m->Title);
	CSTRDUP(m->Title, buf);
	f_read(f, buf, sizeof(((struct MissionOld *)0)->description));
	CFREE(m->Description);
	CSTRDUP(m->Description, buf);

	m->Type = MAPTYPE_CLASSIC;

	debug(D_NORMAL, "== MISSION ==\n");
	debug(D_NORMAL, "t: %s\n", m->Title);
	debug(D_NORMAL, "d: %s\n", m->Description);

	R32(m->WallStyle);
	R32(m->FloorStyle);
	R32(m->RoomStyle);
	R32(m->ExitStyle);
	R32(m->KeyStyle);
	R32(m->DoorStyle);

	R32(m->Size.x); R32(m->Size.y);
	R32(m->u.Classic.Walls); R32(m->u.Classic.WallLength);
	R32(m->u.Classic.Rooms);
	R32(m->u.Classic.Squares);

	// Read exit fields
	R32(c); R32(c); R32(c); R32(c);

	R32(c);
	debug(D_NORMAL, "number of objectives: %d\n", c);
	for (i = 0; i < OBJECTIVE_MAX_OLD; i++)
	{
		MissionObjective mo;
		memset(&mo, 0, sizeof mo);
		load_mission_objective(f, &mo);
		if (i >= c)
		{
			continue;
		}
		CArrayPushBack(&m->Objectives, &mo);
	}

	R32(c);
	debug(D_VERBOSE, "number of baddies: %d\n", c);
	for (i = 0; i < BADDIE_MAX; i++)
	{
		int idx;
		R32(idx);
		if (i >= c)
		{
			continue;
		}
		CArrayPushBack(&m->Enemies, &idx);
	}

	R32(c);
	for (i = 0; i < SPECIAL_MAX; i++)
	{
		int idx;
		R32(idx);
		if (i >= c)
		{
			continue;
		}
		CArrayPushBack(&m->SpecialChars, &idx);
	}

	R32(c);
	for (i = 0; i < ITEMS_MAX; i++)
	{
		int idx;
		R32(idx);
		if (i >= c)
		{
			continue;
		}
		CArrayPushBack(&m->Items, &idx);
	}
	for (i = 0; i < ITEMS_MAX; i++)
	{
		int idx;
		R32(idx);
		if (i >= c)
		{
			continue;
		}
		CArrayPushBack(&m->ItemDensities, &idx);
	}

	R32(m->EnemyDensity);
	R32(c);
	for (i = 0; i < WEAPON_MAX; i++)
	{
		if ((c & (1 << i)) || !c)
		{
			CArrayPushBack(&m->Weapons, &i);
		}
	}

	f_read(f, buf, sizeof(((struct MissionOld *)0)->song));
	strcpy(m->Song, buf);
	f_read(f, buf, sizeof(((struct MissionOld *)0)->map));

	R32(m->WallColor);
	R32(m->FloorColor);
	R32(m->RoomColor);
	R32(m->AltColor);
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
	c->looks.armedBody = b->armedBodyPic;
	c->looks.unarmedBody = b->unarmedBodyPic;
	c->looks.face = b->facePic;
	c->speed = b->speed;
	c->bot.probabilityToMove = b->probabilityToMove;
	c->bot.probabilityToTrack = b->probabilityToTrack;
	c->bot.probabilityToShoot = b->probabilityToShoot;
	c->bot.actionDelay = b->actionDelay;
	c->gun = (gun_e)b->gun;
	c->looks.skin = b->skinColor;
	c->looks.arm = b->armColor;
	c->looks.body = b->bodyColor;
	c->looks.leg = b->legColor;
	c->looks.hair = b->hairColor;
	c->maxHealth = b->health;
	c->flags = b->flags;
}
TBadGuy ConvertTBadGuy(Character *e)
{
	TBadGuy b;
	b.armedBodyPic = e->looks.armedBody;
	b.unarmedBodyPic = e->looks.unarmedBody;
	b.facePic = e->looks.face;
	b.speed = e->speed;
	b.probabilityToMove = e->bot.probabilityToMove;
	b.probabilityToTrack = e->bot.probabilityToTrack;
	b.probabilityToShoot = e->bot.probabilityToShoot;
	b.actionDelay = e->bot.actionDelay;
	b.gun = e->gun;
	b.skinColor = e->looks.skin;
	b.armColor = e->looks.arm;
	b.bodyColor = e->looks.body;
	b.legColor = e->looks.leg;
	b.hairColor = e->looks.hair;
	b.health = e->maxHealth;
	b.flags = e->flags;
	return b;
}
static void ConvertMissionObjective(
	MissionObjective *dest, struct MissionObjectiveOld *src)
{
	CFREE(dest->Description);
	CSTRDUP(dest->Description, src->description);
	dest->Type = src->type;
	dest->Index = src->index;
	dest->Count = src->count;
	dest->Required = src->required;
	dest->Flags = src->flags;
}
static void ConvertMission(Mission *dest, struct MissionOld *src)
{
	int i;
	CFREE(dest->Title);
	CSTRDUP(dest->Title, src->title);
	CFREE(dest->Description);
	CSTRDUP(dest->Title, src->title);
	dest->Type = MAPTYPE_CLASSIC;
	dest->Size = Vec2iNew(src->mapWidth, src->mapHeight);
	dest->WallStyle = src->wallStyle;
	dest->FloorStyle = src->floorStyle;
	dest->RoomStyle = src->roomStyle;
	dest->ExitStyle = src->exitStyle;
	dest->KeyStyle = src->keyStyle;
	dest->DoorStyle = src->doorStyle;
	for (i = 0; i < src->objectiveCount; i++)
	{
		MissionObjective mo;
		memset(&mo, 0, sizeof mo);
		ConvertMissionObjective(&mo, &src->objectives[i]);
		CArrayPushBack(&dest->Objectives, &mo);
	}
	for (i = 0; i < src->baddieCount; i++)
	{
		int n = src->baddies[i];
		CArrayPushBack(&dest->Enemies, &n);
	}
	for (i = 0; i < src->specialCount; i++)
	{
		int n = src->specials[i];
		CArrayPushBack(&dest->SpecialChars, &n);
	}
	for (i = 0; i < src->itemCount; i++)
	{
		int n = src->items[i];
		CArrayPushBack(&dest->Items, &n);
	}
	for (i = 0; i < src->itemCount; i++)
	{
		int n = src->itemDensity[i];
		CArrayPushBack(&dest->ItemDensities, &n);
	}
	dest->EnemyDensity = src->baddieDensity;
	for (i = 0; i < WEAPON_MAX; i++)
	{
		if ((src->weaponSelection & (1 << i)) || !src->weaponSelection)
		{
			CArrayPushBack(&dest->Weapons, &i);
		}
	}
	strcpy(dest->Song, src->song);
	dest->WallColor = src->wallRange;
	dest->FloorColor = src->floorRange;
	dest->RoomColor = src->roomRange;
	dest->AltColor = src->altRange;

	dest->u.Classic.Walls = src->wallCount;
	dest->u.Classic.WallLength = src->wallLength;
	dest->u.Classic.Rooms = src->roomCount;
	dest->u.Classic.Squares = src->squareCount;
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
		ConvertMission(&m, &src->missions[i]);
		CArrayPushBack(&dest->Missions, &m);
	}
	CharacterStoreTerminate(&dest->characters);
	CharacterStoreInit(&dest->characters);
	for (i = 0; i < src->characterCount; i++)
	{
		Character *ch = CharacterStoreAddOther(&dest->characters);
		ConvertCharacter(ch, &src->characters[i]);
		CharacterSetLooks(ch, &ch->looks);
	}
}

int LoadCampaignOld(const char *filename, CampaignSetting *setting)
{
	FILE *f = NULL;
	int32_t i;
	int err = 0;
	int32_t numMissions;
	int numCharacters;
	char buf[256];

	debug(D_NORMAL, "f: %s\n", filename);
	f = fopen(filename, "rb");
	if (f == NULL)
	{
		err = CAMPAIGN_BADPATH;
		goto bail;
	}

	f_read32(f, &i, sizeof(i));
	if (i != CAMPAIGN_MAGIC)
	{
		debug(D_NORMAL, "LoadCampaignOld - bad file!\n");
		err = CAMPAIGN_BADFILE;
		goto bail;
	}

	f_read32(f, &i, sizeof(i));
	if (i != CAMPAIGN_VERSION)
	{
		debug(D_NORMAL, "LoadCampaignOld - version mismatch!\n");
		err = CAMPAIGN_VERSIONMISMATCH;
		goto bail;
	}

	f_read(f, buf, sizeof(((CampaignSettingOld *)0)->title));
	CFREE(setting->Title);
	CSTRDUP(setting->Title, buf);
	f_read(f, buf, sizeof(((CampaignSettingOld *)0)->author));
	CFREE(setting->Author);
	CSTRDUP(setting->Author, buf);
	f_read(f, buf, sizeof(((CampaignSettingOld *)0)->description));
	CFREE(setting->Description);
	CSTRDUP(setting->Description, buf);

	R32(numMissions);
	debug(D_NORMAL, "No. missions: %d\n", numMissions);
	for (i = 0; i < numMissions; i++)
	{
		Mission m;
		MissionInit(&m);
		load_mission(f, &m);
		CArrayPushBack(&setting->Missions, &m);
	}

	R32(numCharacters);
	debug(D_NORMAL, "No. characters: %d\n", numCharacters);
	for (i = 0; i < numCharacters; i++)
	{
		TBadGuy b;
		Character *ch;
		load_character(f, &b);
		ch = CharacterStoreAddOther(&setting->characters);
		ConvertCharacter(ch, &b);
		CharacterSetLooks(ch, &ch->looks);
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
	const char *p;

	if (cdogs_homepath != NULL)
	{
		return cdogs_homepath;
	}

	p = getenv("CDOGS_CONFIG_DIR");
	if (p != NULL && strlen(p) != 0)
	{
		cdogs_homepath = strdup(p);
		return cdogs_homepath;
	}

	p = getenv(HOME_DIR_ENV);
	if (p != NULL && strlen(p) != 0)
	{
		cdogs_homepath = calloc(strlen(p) + 1, sizeof(char));
		strncpy(cdogs_homepath, p, strlen(p));
		strncat(cdogs_homepath, "/", 1);
		return cdogs_homepath;
	}

	fprintf(stderr,"%s%s%s%s",
	"##############################################################\n",
	"# You don't have the environment variables HOME or USER set. #\n",
	"# It is suggested you get a better shell. :D                 #\n",
	"##############################################################\n");
	return "";
}


char *GetDataFilePath(const char *path)
{
	static char buf[CDOGS_PATH_MAX];
	strcpy(buf, CDOGS_DATA_DIR);
	strcat(buf, path);
	return buf;
}


/* GetConfigFilePath()
 *
 * returns a full path to a data file...
 */
char cfpath[512];
const char *GetConfigFilePath(const char *name)
{
	const char *homedir = GetHomeDirectory();

	strcpy(cfpath, homedir);

	strcat(cfpath, CDOGS_CFG_DIR);
	strcat(cfpath, name);

	return cfpath;
}

int mkdir_deep(const char *path)
{
	int i;
	char part[255];

	debug(D_NORMAL, "mkdir_deep path: %s\n", path);

	for (i = 0; i < (int)strlen(path); i++)
	{
		if (path[i] == '\0') break;
		if (path[i] == '/') {
			strncpy(part, path, i + 1);
			part[i+1] = '\0';

			if (mkdir(part, MKDIR_MODE) == -1)
			{
				/* Mac OS X 10.4 returns EISDIR instead of EEXIST
				 * if a dir already exists... */
				if (errno == EEXIST || errno == EISDIR) continue;
				else return 1;
			}
		}
	}

	return 0;
}

void SetupConfigDir(void)
{
	const char *cfg_p = GetConfigFilePath("");

	printf("Creating Config dir... ");

	if (mkdir_deep(cfg_p) == 0)
	{
		if (errno != EEXIST)
			printf("Config dir created.\n");
		else
			printf("No need. Already exists!\n");
	} else {
		switch (errno) {
			case EACCES:
				printf("Permission denied!\n");
				break;
			default:
				perror("Error creating config directory:");
		}
	}

	return;
}
