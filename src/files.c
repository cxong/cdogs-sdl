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
*/
#include "files.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

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
	if (bendian) {
		*((int *)d) = SDL_Swap32(*((int *)d));
	}
}

ssize_t f_read(FILE *f, void *buf, size_t size)
{
	return fread(buf, size, 1, f);
}

ssize_t f_read32(FILE *f, void *buf, size_t size)
{
	ssize_t ret = 0;
	if (buf) {
		ret = f_read(f, buf, size);
		swap32((int *)buf);
	}
	return ret;
}

void swap16 (void *d)
{
	if (bendian) {
		*((short int *)d) = SDL_Swap16(*((short int *)d));
	}
}

ssize_t f_read16(FILE *f, void *buf, size_t size)
{
	ssize_t ret = 0;
	if (buf) {
		ret = f_read(f, buf, size);
		swap16((short int*)buf);
	}
	return ret;
}


#define Is_Writable(n)		(access(n, W_OK) == 0)
#define Is_Readable(n)		(access(n, R_OK) == 0)
#define Is_Executable(n)	(access(n, X_OK) == 0)
#define Is_Browsable(n)		(access(n, X_OK) == 0)

int ScanCampaign(const char *filename, char *title, int *missions)
{
	FILE *f;
	int i;
	TCampaignSetting setting;

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
			debug(D_NORMAL, "ScanCampaign - version mismatch!\n");
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

void load_mission_objective(FILE *f, struct MissionObjective *o)
{
		int offset = 0;
		f_read(f, o->description, sizeof(o->description)); offset += sizeof(o->description);
		f_read32(f, &o->type, sizeof(o->type));
		f_read32(f, &o->index, sizeof(o->index));
		f_read32(f, &o->count, sizeof(o->count));
		f_read32(f, &o->required, sizeof(o->required));
		f_read32(f, &o->flags, sizeof(o->flags));
		debug(D_VERBOSE, " >> Objective: %s data: %d %d %d %d %d\n",
		o->description, o->type, o->index, o->count, o->required, o->flags);
}

#define R32(s,e)	f_read32(f, &s->e, sizeof(s->e))

void load_mission(FILE *f, struct Mission *m)
{
		int i;
		int o = 0;

		f_read(f, m->title, sizeof(m->title));				o += sizeof(m->title);
		f_read(f, m->description, sizeof(m->description));	o += sizeof(m->description);

		debug(D_NORMAL, "== MISSION ==\n");
		debug(D_NORMAL, "t: %s\n", m->title);
		debug(D_NORMAL, "d: %s\n", m->description);

		R32(m,  wallStyle);
		R32(m,  floorStyle);
		R32(m,  roomStyle);
		R32(m,  exitStyle);
		R32(m,  keyStyle);
		R32(m,  doorStyle);

		R32(m,  mapWidth); R32(m, mapHeight);
		R32(m,  wallCount); R32(m, wallLength);
		R32(m,  roomCount);
		R32(m,  squareCount);

		R32(m,  exitLeft); R32(m, exitTop); R32(m, exitRight); R32(m, exitBottom);

		R32(m, objectiveCount);

		debug(D_NORMAL, "number of objectives: %d\n", m->objectiveCount);
 		for (i = 0; i < OBJECTIVE_MAX; i++) {
			load_mission_objective(f, &m->objectives[i]);
		}

		R32(m, baddieCount);
		for (i = 0; i < BADDIE_MAX; i++) {
			f_read32(f, &m->baddies[i], sizeof(int));
		}

		R32(m, specialCount);
		for (i = 0; i < SPECIAL_MAX; i++) {
			f_read32(f, &m->specials[i], sizeof(int));
		}

		R32(m, itemCount);
		for (i = 0; i < ITEMS_MAX; i++) {
			f_read32(f, &m->items[i], sizeof(int));
		}
		for (i = 0; i < ITEMS_MAX; i++) {
			f_read32(f, &m->itemDensity[i], sizeof(int));
		}

		R32(m, baddieDensity);
		R32(m, weaponSelection);

		f_read(f, m->song, sizeof(m->song));
		f_read(f, m->map, sizeof(m->map));

		R32(m, wallRange);
		R32(m, floorRange);
		R32(m, roomRange);
		R32(m, altRange);

		debug(D_VERBOSE, "number of baddies: %d\n", m->baddieCount);

		return;
}



void load_character(FILE *f, TBadGuy *c)
{
		R32(c, armedBodyPic);
		R32(c, unarmedBodyPic);
		R32(c, facePic);
		R32(c, speed);
		R32(c, probabilityToMove);
		R32(c, probabilityToTrack);
		R32(c, probabilityToShoot);
		R32(c, actionDelay);
		R32(c, gun);
		R32(c, skinColor);
		R32(c, armColor);
		R32(c, bodyColor);
		R32(c, legColor);
		R32(c, hairColor);
		R32(c, health);
		R32(c, flags);

//		fprintf(stderr, " speed: %d gun: %d\n", c->speed, c->gun);
}

int LoadCampaign(const char *filename, TCampaignSetting * setting,
		 int max_missions, int max_characters)
{
	FILE *f;
	int i;

	debug(D_NORMAL, "f: %s\n", filename);
	f = fopen(filename, "rb");

	if (f) {
		f_read32(f, &i, sizeof(i));
		if (i != CAMPAIGN_MAGIC) {
			fclose(f);
			debug(D_NORMAL, "LoadCampaign - bad file!\n");
			return CAMPAIGN_BADFILE;
		}

		f_read32(f, &i, sizeof(i));
		if (i != CAMPAIGN_VERSION) {
			fclose(f);
			debug(D_NORMAL, "LoadCampaign - version mismatch!\n");
			return CAMPAIGN_VERSIONMISMATCH;
		}

		f_read(f, setting->title, sizeof(setting->title));
		f_read(f, setting->author, sizeof(setting->author));
		f_read(f, setting->description, sizeof(setting->description));

		f_read32(f, &setting->missionCount, sizeof(setting->missionCount));

		if (max_missions <= 0) {
			i = setting->missionCount * sizeof(struct Mission);
			setting->missions = sys_mem_alloc(i);
			memset(setting->missions, 0, i);
			max_missions = setting->missionCount;
		} else if (setting->missionCount < max_missions)
			max_missions = setting->missionCount;

		debug(D_NORMAL, "No. missions: %d\n", max_missions);

		for (i = 0; i < max_missions; i++) {
			load_mission(f, &setting->missions[i]);
		}

		f_read32(f, &setting->characterCount, sizeof(setting->characterCount));

		if (max_characters <= 0) {
			i = setting->characterCount * sizeof(TBadGuy);
			setting->characters = sys_mem_alloc(i);
			memset(setting->characters, 0, i);
			max_characters = setting->characterCount;
		} else if (setting->characterCount < max_characters)
			max_characters = setting->characterCount;

		debug(D_NORMAL, "No. characters: %d\n", max_characters);

		for (i = 0; i < max_characters; i++) {
			load_character(f, &setting->characters[i]);
		}

		fclose(f);

		return CAMPAIGN_OK;
	}
	return CAMPAIGN_BADPATH;
}

int SaveCampaign(const char *filename, TCampaignSetting * setting)
{
	int f;
	int i;

	f = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (f >= 0) {
		ssize_t writeres;
	#define CHECK_WRITE()\
		if (writeres != 0) {\
			perror("SaveCampaign - couldn't write to file: ");\
			close(f);\
			return CAMPAIGN_BADFILE;\
		}
		i = CAMPAIGN_MAGIC;
		writeres = write(f, &i, sizeof(i));
		CHECK_WRITE()

		i = CAMPAIGN_VERSION;
		writeres = write(f, &i, sizeof(i));
		CHECK_WRITE()

		writeres = write(f, setting->title, sizeof(setting->title));
		CHECK_WRITE()
		writeres = write(f, setting->author, sizeof(setting->author));
		CHECK_WRITE()
		writeres = write(f, setting->description,
				 sizeof(setting->description));
		CHECK_WRITE()

		writeres = write(f, &setting->missionCount,
				 sizeof(setting->missionCount));
		CHECK_WRITE()

		for (i = 0; i < setting->missionCount; i++) {
			writeres = write(f, &setting->missions[i],
					 sizeof(struct Mission));
			CHECK_WRITE()
		}

		writeres = write(f, &setting->characterCount,
				 sizeof(setting->characterCount));
		CHECK_WRITE()
		for (i = 0; i < setting->characterCount; i++) {
			writeres = write(f, &setting->characters[i],
					 sizeof(TBadGuy));
			CHECK_WRITE()
		}
		//fchmod(f, S_IRUSR | S_IRGRP | S_IROTH);
		close(f);
		return CAMPAIGN_OK;
	}
	perror("SaveCampaign - couldn't write to file: ");
	return CAMPAIGN_BADPATH;
}

static void OutputCString(FILE * f, const char *s, int indentLevel)
{
	int length;

	for (length = 0; length < indentLevel; length++)
		fputc(' ', f);

	fputc('\"', f);
	while (*s) {
		switch (*s) {
		case '\"':
		case '\\':
			fputc('\\', f);
		default:
			fputc(*s, f);
		}
		s++;
		length++;
		if (length > 75) {
			fputs("\"\n", f);
			length = 0;
			for (length = 0; length < indentLevel; length++)
				fputc(' ', f);
			fputc('\"', f);
		}
	}
	fputc('\"', f);
}

void SaveCampaignAsC(const char *filename, const char *name,
		     TCampaignSetting * setting)
{
	FILE *f;
	int i, j;

	f = fopen(filename, "w");
	if (f) {
		fprintf(f, "TBadGuy %s_badguys[ %d] =\n{\n", name,
			setting->characterCount);
		for (i = 0; i < setting->characterCount; i++) {
			fprintf(f,
				"  {%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,0x%x}%s\n",
				setting->characters[i].armedBodyPic,
				setting->characters[i].unarmedBodyPic,
				setting->characters[i].facePic,
				setting->characters[i].speed,
				setting->characters[i].probabilityToMove,
				setting->characters[i].probabilityToTrack,
				setting->characters[i].probabilityToShoot,
				setting->characters[i].actionDelay,
				setting->characters[i].gun,
				setting->characters[i].skinColor,
				setting->characters[i].armColor,
				setting->characters[i].bodyColor,
				setting->characters[i].legColor,
				setting->characters[i].hairColor,
				setting->characters[i].health,
				setting->characters[i].flags,
				i <
				setting->characterCount - 1 ? "," : "");
		}
		fprintf(f, "};\n\n");

		fprintf(f, "struct Mission %s_missions[ %d] =\n{\n", name,
			setting->missionCount);
		for (i = 0; i < setting->missionCount; i++) {
			fprintf(f, "  {\n");
			OutputCString(f, setting->missions[i].title, 4);
			fprintf(f, ",\n");
			OutputCString(f, setting->missions[i].description,
				      4);
			fprintf(f, ",\n");
			fprintf(f,
				"    %d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,\n",
				setting->missions[i].wallStyle,
				setting->missions[i].floorStyle,
				setting->missions[i].roomStyle,
				setting->missions[i].exitStyle,
				setting->missions[i].keyStyle,
				setting->missions[i].doorStyle,
				setting->missions[i].mapWidth,
				setting->missions[i].mapHeight,
				setting->missions[i].wallCount,
				setting->missions[i].wallLength,
				setting->missions[i].roomCount,
				setting->missions[i].squareCount);
			fprintf(f, "    %d,%d,%d,%d,\n",
				setting->missions[i].exitLeft,
				setting->missions[i].exitTop,
				setting->missions[i].exitRight,
				setting->missions[i].exitBottom);
			fprintf(f, "    %d,\n",
				setting->missions[i].objectiveCount);
			fprintf(f, "    {\n");
			for (j = 0; j < OBJECTIVE_MAX; j++) {
				fprintf(f, "      {\n");
				OutputCString(f,
					      setting->missions[i].
					      objectives[j].description,
					      8);
				fprintf(f, ",\n");
				fprintf(f, "        %d,%d,%d,%d,0x%x\n",
					setting->missions[i].objectives[j].
					type,
					setting->missions[i].objectives[j].
					index,
					setting->missions[i].objectives[j].
					count,
					setting->missions[i].objectives[j].
					required,
					setting->missions[i].objectives[j].
					flags);
				fprintf(f, "      }%s\n",
					j < OBJECTIVE_MAX - 1 ? "," : "");
			}
			fprintf(f, "    },\n");

			fprintf(f, "    %d,\n",
				setting->missions[i].baddieCount);
			fprintf(f, "    {");
			for (j = 0; j < BADDIE_MAX; j++) {
				fprintf(f, "%d%s",
					setting->missions[i].baddies[j],
					j < BADDIE_MAX - 1 ? "," : "");
			}
			fprintf(f, "},\n");

			fprintf(f, "    %d,\n",
				setting->missions[i].specialCount);
			fprintf(f, "    {");
			for (j = 0; j < SPECIAL_MAX; j++) {
				fprintf(f, "%d%s",
					setting->missions[i].specials[j],
					j < SPECIAL_MAX - 1 ? "," : "");
			}
			fprintf(f, "},\n");

			fprintf(f, "    %d,\n",
				setting->missions[i].itemCount);
			fprintf(f, "    {");
			for (j = 0; j < ITEMS_MAX; j++) {
				fprintf(f, "%d%s",
					setting->missions[i].items[j],
					j < ITEMS_MAX - 1 ? "," : "");
			}
			fprintf(f, "},\n");

			fprintf(f, "    {");
			for (j = 0; j < ITEMS_MAX; j++) {
				fprintf(f, "%d%s",
					setting->missions[i].
					itemDensity[j],
					j < ITEMS_MAX - 1 ? "," : "");
			}
			fprintf(f, "},\n");

			fprintf(f, "    %d,0x%x,\n",
				setting->missions[i].baddieDensity,
				setting->missions[i].weaponSelection);
			OutputCString(f, setting->missions[i].song, 4);
			fprintf(f, ",\n");
			OutputCString(f, setting->missions[i].map, 4);
			fprintf(f, ",\n");
			fprintf(f, "    %d,%d,%d,%d\n",
				setting->missions[i].wallRange,
				setting->missions[i].floorRange,
				setting->missions[i].roomRange,
				setting->missions[i].altRange);
			fprintf(f, "  }%s\n",
				i < setting->missionCount ? "," : "");
		}
		fprintf(f, "};\n\n");

		fprintf(f, "struct CampaignSetting %s_campaign =\n{\n",
			name);
		OutputCString(f, setting->title, 2);
		fprintf(f, ",\n");
		OutputCString(f, setting->author, 2);
		fprintf(f, ",\n");
		OutputCString(f, setting->description, 2);
		fprintf(f, ",\n");
		fprintf(f, "  %d, %s_missions, %d, %s_badguys\n};\n",
			setting->missionCount, name,
			setting->characterCount, name);

		fclose(f);
	}
};

int Is_Dir(const char *name)
{
	struct stat s;

	debug(D_NORMAL, "name: '%s'\n", name);

	if ((stat(name, &s) == 0)) {
	//	if ((s.st_mode & S_IFMT) == S_IFDIR) {
		switch (s.st_mode & S_IFMT) {
			case S_IFDIR:
				debug(D_VERBOSE, "is a dir...\n");
				return 1;
			case S_IFLNK:
				{
					char lnk_buf[512];

					debug(D_VERBOSE, "is a symlink...\n");

					if (readlink(name, lnk_buf, (size_t) 512) != -1) {
						debug(D_VERBOSE, "resolved to '%s'\n", lnk_buf);
						return Is_Dir(lnk_buf);
					} else {
						return 0;
					}
				}
			default:
				return 0;
		}
	}
	return 0;
}

void AddFileEntry(struct FileEntry **list, const char *name,
		  const char *info, int data)
{
	struct FileEntry *entry;

	if (strcmp(name, "..") == 0)	return;
	if (strcmp(name, ".") == 0)	return;

	if (Is_Dir(name)) return;

	while (*list && strcmp((*list)->name, name) < 0)
		list = &(*list)->next;

	if (strcmp(name, "") == 0)
		printf(" -> Adding [builtin]\n");
	else
		printf(" -> Adding [%s]\n", name);

	entry = sys_mem_alloc(sizeof(struct FileEntry));
	strcpy(entry->name, name);
	strcpy(entry->info, info);
	entry->data = data;
	entry->next = *list;
	*list = entry;
}

struct FileEntry *GetFilesFromDirectory(const char *directory)
{
	DIR *dir;
	struct dirent *d;
	struct FileEntry *list = NULL;

	dir = opendir(directory);
	if (dir != NULL) {
		while ((d = readdir(dir)) != NULL)
			AddFileEntry(&list, d->d_name, "", 0);
		closedir(dir);
	}
	return list;
}

void FreeFileEntries(struct FileEntry *entries)
{
	struct FileEntry *tmp;

	while (entries) {
		tmp = entries;
		entries = entries->next;
		free(tmp);
	}
}

void GetCampaignTitles(struct FileEntry **entries)
{
	int i;
	struct FileEntry *tmp;
	char s[10];

	while (*entries) {
		if (ScanCampaign(join(GetDataFilePath(CDOGS_CAMPAIGN_DIR),
				(*entries)->name),
				(*entries)->info,
				&i) == CAMPAIGN_OK) {
			sprintf(s, " (%d)", i);
			strcat((*entries)->info, s);
			entries = &((*entries)->next);
		} else		// Bad campaign
		{
			tmp = *entries;
			*entries = tmp->next;
			free(tmp);
		}
	}
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


/* GetDataFilePath()
 *
 * returns a full path to a data file...
 *
 * XXX: Use default dir as fallback?
 */
static char *data_path = NULL;
static char data_pbuf[255];
char * GetDataFilePath(const char *path)
{
	if (!data_path) {
		char *tmp;

		if ((tmp = getenv("CDOGS_DATA_DIR")) != NULL && strlen(tmp) != 0
				&& Is_Dir(tmp) && Is_Readable(tmp)) {
			data_path = strdup(tmp);
		} else {
			tmp = calloc(strlen(CDOGS_DATA_DIR)+strlen(path)+1,sizeof(char));

			strcpy(tmp, CDOGS_DATA_DIR);
			strcat(tmp, path);

			data_path = strdup(tmp);
			free(tmp);
		}
	}

	strcpy(data_pbuf, data_path);
	strcat(data_pbuf, "/");
	strcat(data_pbuf, path);
	return data_pbuf;
}

char * join(const char *s1, const char *s2)
{
	char *tmp;

	tmp = calloc(strlen(s1)+strlen(s2)+1,sizeof(char));

	strcpy(tmp, s1);
	strcat(tmp, s2);

	return tmp;
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

int mkdir_deep(const char *path, mode_t m)
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

			if (mkdir(part, m) == -1) {
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

	if (mkdir_deep(cfg_p, MKDIR_MODE) == 0)
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

char dir_buf[512];
char * GetPWD(void)
{
	if (getcwd(dir_buf, 511) == NULL) {
		printf("Error getting PWD\n");
	}
	return dir_buf;
}
