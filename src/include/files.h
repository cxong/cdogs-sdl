/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (C) 1995 Ronny Webster
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

 files.h - <description here>

*/

#include "gamedata.h"
#include "config.h"

#define CAMPAIGN_OK                0
#define CAMPAIGN_BADFILE          -1
#define CAMPAIGN_VERSIONMISMATCH  -2
#define CAMPAIGN_BADPATH          -3

#define FS_OBJ_WRITE	1	/* File/dir is writable */
#define FS_OBJ_NWRITE 	0	/* File/dir isn't writable */
#define FS_OBJ_NEXIST  -1	/* File/dir doesn't exist */

struct FileEntry {
	char name[13];
	char info[80];
	int data;
	struct FileEntry *next;
};

int LoadCampaign(const char *filename, TCampaignSetting * setting,
		 int max_missions, int max_characters);
int SaveCampaign(const char *filename, TCampaignSetting * setting);
void SaveCampaignAsC(const char *filename, const char *name,
		     TCampaignSetting * setting);

void AddFileEntry(struct FileEntry **list, const char *name,
		  const char *info, int data);
struct FileEntry *GetFilesFromDirectory(const char *directory);
void FreeFileEntries(struct FileEntry *entries);
void GetCampaignTitles(struct FileEntry **entries);

char * GetHomeDirectory(void);
char * GetConfigFilePath(const char *name);
char * GetDataFilePath(const char *path);

char * join(const char *s1, const char *s2);
