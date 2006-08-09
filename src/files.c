/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (C) 1995 Ronny Wester
    Copyright (C) 2003 Jeremy Chin 
    Copyright (C) 2003 Lucas Martin-King 

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

-------------------------------------------------------------------------------

 files.c - file handling functions

*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
// #include <io.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
// #include <direct.h>
#include "files.h"

#define MAX_STRING_LEN 1000

#define CAMPAIGN_MAGIC    690304
#define CAMPAIGN_VERSION  6

#if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
void swap32 (void *d)
{
	*((int *)d) = SDL_Swap32(*((int *)d));
}

ssize_t read32(int fd, void *buf, size_t size)
{
	ssize_t ret = 0;	
	if (buf) {
		ret = read(fd, buf, size);
		swap32((int *)buf);
	}
	return ret;
}

ssize_t readarray32(int fd, void *buf, size_t size)
{
	int i;

	fprintf(stderr, "readarray32(%d, %x, %d)\n", fd, buf, size);

	if (buf) {
		for (i = 0; i < (size/4); i++) {
		//	fprintf(stderr, " i: %d\n", i);
			read32(fd, buf, sizeof(int));
			buf += sizeof(int);
		}
	}
	return size;
}

void swap16 (void *d)
{
	*((short int *)d) = SDL_Swap16(*((short int *)d));
}

ssize_t read16(int fd, void *buf, size_t size)
{
	ssize_t ret = 0;	
	if (buf) {
		ret = read(fd, buf, size);
		swap16((short int*)buf);
	}
	return ret;
}
#endif /* SDL_BYTEORDER == SDL_BIG_ENDIAN */

int ScanCampaign(const char *filename, char *title, int *missions)
{
	int f;
	int i;
	TCampaignSetting setting;

	f = open(filename, O_RDONLY);
	if (f >= 0) {
		read32(f, &i, sizeof(i));
		
		if (i != CAMPAIGN_MAGIC) {
			close(f);
			fprintf(stderr, "Filename: %s\n", filename);
			fprintf(stderr, "Magic: %d FileM: %d\n", CAMPAIGN_MAGIC, i);
			fprintf(stderr, "ScanCampaign - bad file!\n");
			return CAMPAIGN_BADFILE;
		}

		read32(f, &i, sizeof(i));
		if (i != CAMPAIGN_VERSION) {
			close(f);
			printf("ScanCampaign - version mismatch!\n");
			return CAMPAIGN_VERSIONMISMATCH;
		}

		read(f, setting.title, sizeof(setting.title));
		read(f, setting.author, sizeof(setting.author));
		read(f, setting.description, sizeof(setting.description));
		read(f, &setting.missionCount,
		     sizeof(setting.missionCount));
		strcpy(title, setting.title);
		*missions = setting.missionCount;
		close(f);

		return CAMPAIGN_OK;
	}
	printf("ScanCampaign - bad path!?\n");
	return CAMPAIGN_BADPATH;
}

void load_mission_objective(int fd, struct MissionObjective *o)
{
		int offset = 0;
		read(fd, o->description, sizeof(o->description)); offset += sizeof(o->description);
		read32(fd, &o->type, sizeof(o->type));
		read32(fd, &o->index, sizeof(o->index));
		read32(fd, &o->count, sizeof(o->count));
		read32(fd, &o->required, sizeof(o->required));
		read32(fd, &o->flags, sizeof(o->flags));
		//readarray32(fd, o + offset, 5 * sizeof(int));
		fprintf(stderr, " >> Objective: %s data: %d %d %d %d %d\n", o->description,
												o->type, o->index, o->count, o->required, o->flags);
}

#define R32(s,e)	read32(fd, &s->e, sizeof(s->e))

void load_mission(int fd, struct Mission *m)
{
		int i;
		int o = 0;

		read(fd, m->title, sizeof(m->title));				o += sizeof(m->title);
		read(fd, m->description, sizeof(m->description));	o += sizeof(m->description);

		fprintf(stderr, "== MISSION ==\n");
		fprintf(stderr, "t: %s\n", m->title);
		fprintf(stderr, "d: %s\n", m->description);  
	
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
	
	//	readarray32(fd, m + o, 17 * sizeof(int));		
		o += 17 * sizeof(int);
		
		fprintf(stderr, " number of objectives: %d\n", m->objectiveCount);	
 		for (i = 0; i < OBJECTIVE_MAX; i++) {
			load_mission_objective(fd, &m->objectives[i]);
		}
	
		R32(m, baddieCount);
		for (i = 0; i < BADDIE_MAX; i++) {
			read32(fd, &m->baddies[i], sizeof(int));
		}
		
		R32(m, specialCount);
		for (i = 0; i < SPECIAL_MAX; i++) {
			read32(fd, &m->specials[i], sizeof(int));
		}
		
		R32(m, itemCount);
		for (i = 0; i < ITEMS_MAX; i++) {
			read32(fd, &m->items[i], sizeof(int));
		}
		for (i = 0; i < ITEMS_MAX; i++) {
			read32(fd, &m->itemDensity[i], sizeof(int));
		}
	
		R32(m, baddieDensity);	
		R32(m, weaponSelection);					
		
		read(fd, m->song, sizeof(m->song));
		read(fd, m->map, sizeof(m->map));
		
		R32(m, wallRange);						
		R32(m, floorRange);
		R32(m, roomRange);
		R32(m, altRange);			
																																																																							
		//o += OBJECTIVE_MAX * sizeof(struct MissionObjective);
		
		//readarray32(fd, m + o, sizeof(struct Mission) - o);
		fprintf(stderr, " number of baddies: %d\n", m->baddieCount);	
		
		//read(fd, m + o, sizeof(struct Mission) - o);
}



void load_character(int fd, TBadGuy *c)
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

//		readarray32(fd, c, sizeof(TBadGuy));
//		fprintf(stderr, " speed: %d gun: %d\n", c->speed, c->gun);
}

int LoadCampaign(const char *filename, TCampaignSetting * setting,
		 int max_missions, int max_characters)
{
	int f;
	int i;

	f = open(filename, O_RDONLY);
	if (f >= 0) {
		read32(f, &i, sizeof(i));
		if (i != CAMPAIGN_MAGIC) {
			close(f);
			printf("LoadCampaign - bad file!\n");
			return CAMPAIGN_BADFILE;
		}

		read32(f, &i, sizeof(i));
		if (i != CAMPAIGN_VERSION) {
			close(f);
			printf("LoadCampaign - version mismatch!\n");
			return CAMPAIGN_VERSIONMISMATCH;
		}

		read(f, setting->title, sizeof(setting->title));
		read(f, setting->author, sizeof(setting->author));
		read(f, setting->description, sizeof(setting->description));

		read32(f, &setting->missionCount,
		     sizeof(setting->missionCount));

		if (max_missions <= 0) {
			i = setting->missionCount * sizeof(struct Mission);
			setting->missions = malloc(i);
			memset(setting->missions, 0, i);
			max_missions = setting->missionCount;
		} else if (setting->missionCount < max_missions)
			max_missions = setting->missionCount;

		fprintf(stderr, "No. missions: %d\n", max_missions);

		for (i = 0; i < max_missions; i++) {
			load_mission(f, &setting->missions[i]);
		}

		read32(f, &setting->characterCount, sizeof(setting->characterCount));

		if (max_characters <= 0) {
			i = setting->characterCount * sizeof(TBadGuy);
			setting->characters = malloc(i);
			memset(setting->characters, 0, i);
			max_characters = setting->characterCount;
		} else if (setting->characterCount < max_characters)
			max_characters = setting->characterCount;

		fprintf(stderr, "No. characters: %d\n", max_characters);

		for (i = 0; i < max_characters; i++) {
			load_character(f, &setting->characters[i]);
		}
		close(f);

		return CAMPAIGN_OK;
	}
	return CAMPAIGN_BADPATH;
}

int SaveCampaign(const char *filename, TCampaignSetting * setting)
{
	int f;
	int i;

	f = open(filename, O_WRONLY | O_CREAT | O_TRUNC);
	if (f >= 0) {
		i = CAMPAIGN_MAGIC;
		write(f, &i, sizeof(i));

		i = CAMPAIGN_VERSION;
		write(f, &i, sizeof(i));

		write(f, setting->title, sizeof(setting->title));
		write(f, setting->author, sizeof(setting->author));
		write(f, setting->description,
		      sizeof(setting->description));

		write(f, &setting->missionCount,
		      sizeof(setting->missionCount));
		for (i = 0; i < setting->missionCount; i++) {
			write(f, &setting->missions[i],
			      sizeof(struct Mission));
		}

		write(f, &setting->characterCount,
		      sizeof(setting->characterCount));
		for (i = 0; i < setting->characterCount; i++) {
			write(f, &setting->characters[i], sizeof(TBadGuy));
		}
		//fchmod(f, S_IRUSR | S_IRGRP | S_IROTH);
		close(f);
		return CAMPAIGN_OK;
	}
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

void AddFileEntry(struct FileEntry **list, const char *name,
		  const char *info, int data)
{
	struct FileEntry *entry;

	if (strcmp(name, "..") == 0)	return;
	if (strcmp(name, ".") == 0)	return;

	while (*list && strcmp((*list)->name, name) < 0)
		list = &(*list)->next;

	if (strcmp(name, "") == 0)
		printf(" -> Adding [builtin]\n");
	else 
		printf(" -> Adding [%s]\n", name);

	entry = malloc(sizeof(struct FileEntry));
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
		if (ScanCampaign(join(GetDataFilePath("missions/"),
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

char *cdogs_homepath;
char * GetHomeDirectory(void)
{
	char *home;
	char *user;
	char *tmp1; /* excessive buffer */
	char *tmp2; /* dynamic buffer */

	if (cdogs_homepath == NULL) {
		home = getenv("HOME");

		if (home == NULL || strlen(home) == 0) { /* no HOME var, try to get USER */
			user = getenv("USER");

			if (user == NULL || strlen(user) == 0) { /* someone has a dodgy shell (or windows) */
				fprintf(stderr,"%s%s%s%s",
				"##############################################################\n",
				"# You don't have the environment variables HOME or USER set. #\n",
				"# It is suggested you get a better shell. :D                 #\n",
				"##############################################################\n");
				
				tmp1 = calloc(MAX_STRING_LEN, sizeof(char));	/* lots of chars... */ 
				strcpy(tmp1, "/tmp/unknown.user/");	
				tmp2 = calloc(strlen(tmp1)+1, 1);
				strcpy(tmp2, tmp1);
				free(tmp1);
				
				return tmp2;
			} else {				/* hopefully they have USER set... */
				tmp1 = calloc(MAX_STRING_LEN, 1);	/* lots of chars... */
				strcpy(tmp1, "/home/");
				strcat(tmp1, user);
				strcat(tmp1, "/");
				tmp2 = calloc(strlen(tmp1)+1, 1); 
				strcpy(tmp2, tmp1);	
				free(tmp1);
				//setenv("HOME", tmp1, 0);	/* set the HOME var anyway :) */
			
				return tmp2;
			}
		}

		tmp1 = calloc(MAX_STRING_LEN, sizeof(char));	/* lots of chars... */ 
		strcpy(tmp1, home);
		strcat(tmp1, "/");

		tmp2 = calloc(strlen(tmp1)+1, 1);
		strcpy(tmp2, tmp1);
		free(tmp1);
		cdogs_homepath = tmp2;
	}

	return cdogs_homepath;
}

#if 0
int IsWritable(char *name)
{
	struct stat s;
	int tmp;
	int ret;

	tmp=stat(name, &s);

	if (tmp == -1 && errno == ENOENT) {
		printf("Error: '%s' does not exist\n", name);
		ret = FS_OBJ_NEXIST;
	} else if (ret == 1) {
		uid_t u;
		
		u = getuid();
	
		/* Check if it's a file and rw */
		if (((s.s_mode & S_IFMT) == S_IFREG) &&
			)
				
	}

	return ret;
}
#endif


/* GetDataFilePath()
 *
 * returns a full path to a data file...
 */
char * GetDataFilePath(const char *path)
{
	char *tmp;

	tmp = calloc(strlen(CDOGS_DATA_DIR)+strlen(path)+1,sizeof(char));

	strcpy(tmp, CDOGS_DATA_DIR);
	strcat(tmp, path);

	return strdup(tmp);
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
char * GetConfigFilePath(const char *name)
{
	char *tmp;
	char *homedir;

	homedir = GetHomeDirectory();

	tmp = calloc(strlen(homedir) + strlen(name) + strlen(CDOGS_CFG_DIR) + 1, sizeof(char));

	strcpy(tmp, homedir);

	strcat(tmp, CDOGS_CFG_DIR);
	strcat(tmp, name);

	return tmp;
}


#ifdef SYS_WIN
	#define mkdir(p, a)     mkdir(p)
#endif

void SetupConfigDir(void)
{
	char *cfg_p = GetConfigFilePath("");

	printf("Creating Config dir... ");
	if (mkdir(cfg_p, S_IRUSR | S_IXUSR | S_IWUSR) == -1)
		switch (errno) {
			case EACCES:
				printf("Permission denied!\n");
				break;
			case EEXIST:
				printf("No need. Already exists\n");
				break;
			default:
				printf("Error: (%d)\n", errno);
		}
	else
		printf("Config dir created.\n");
	
	return;
}

char * GetPWD(void)
{
	char dir_buf[250];	
	getcwd(dir_buf, 250);
	return dir_buf;
}
