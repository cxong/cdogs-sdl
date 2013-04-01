/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.

    Copyright (c) 2013, Cong Xu
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
#include "campaigns.h"

#include <stdio.h>

#include <tinydir.h>

#include "files.h"
#include "mission.h"
#include "utils.h"


void CampaignListInit(campaign_list_t *list);
void LoadBuiltinCampaigns(campaign_list_t *list);
void LoadBuiltinDogfights(campaign_list_t *list);
void LoadCampaignsFromFolder(
	campaign_list_t *list, const char *name, const char *path, int isDogfight);

void LoadAllCampaigns(custom_campaigns_t *campaigns)
{
	CampaignListInit(&campaigns->campaignList);
	CampaignListInit(&campaigns->dogfightList);

	printf("\nCampaigns:\n");

	LoadBuiltinCampaigns(&campaigns->campaignList);
	LoadCampaignsFromFolder(
		&campaigns->campaignList,
		"",
		GetDataFilePath(CDOGS_CAMPAIGN_DIR),
		0);

	printf("\nDogfights:\n");

	LoadBuiltinDogfights(&campaigns->dogfightList);
	LoadCampaignsFromFolder(
		&campaigns->dogfightList,
		"",
		GetDataFilePath(CDOGS_DOGFIGHT_DIR),
		1);

	printf("\n");
}

void UnloadAllCampaigns(custom_campaigns_t *campaigns)
{
	if (campaigns)
	{
		if (campaigns->campaignList.subFolders)
		{
			sys_mem_free(campaigns->campaignList.subFolders);
		}
		if (campaigns->campaignList.list)
		{
			sys_mem_free(campaigns->campaignList.list);
		}
		if (campaigns->dogfightList.subFolders)
		{
			sys_mem_free(campaigns->dogfightList.subFolders);
		}
		if (campaigns->dogfightList.list)
		{
			sys_mem_free(campaigns->dogfightList.list);
		}
	}
}

void CampaignListInit(campaign_list_t *list)
{
	strcpy(list->name, "");
	list->subFolders = NULL;
	list->list = NULL;
	list->numSubFolders = 0;
	list->num = 0;
}

void AddBuiltinCampaignEntry(
	campaign_list_t *list, const char *title, int isDogfight, int builtinIndex);

void LoadBuiltinCampaigns(campaign_list_t *list)
{
	int i = 0;
	for (i = 0; SetupBuiltinCampaign(i); i++)
	{
		AddBuiltinCampaignEntry(list, gCampaign.setting->title, 0, i);
	}
}
void LoadBuiltinDogfights(campaign_list_t *list)
{
	int i = 0;
	for (i = 0; SetupBuiltinDogfight(i); i++)
	{
		AddBuiltinCampaignEntry(list, gCampaign.setting->title, 1, i);
	}
}

int IsCampaignOK(const char *path, char *title);
void AddCustomCampaignEntry(
	campaign_list_t *list,
	const char *filename,
	const char *path,
	const char *title,
	int isDogfight);

void LoadCampaignsFromFolder(
	campaign_list_t *list, const char *name, const char *path, int isDogfight)
{
	tinydir_dir dir;
	int i;

	strcpy(list->name, name);
	if (tinydir_open_sorted(&dir, path) == -1)
	{
		printf("Cannot load campaigns from path %s\n", path);
		return;
	}

	for (i = 0; i < dir.n_files; i++)
	{
		tinydir_file file;
		tinydir_readfile_n(&dir, &file, i);

		if (file.is_dir &&
			strcmp(file.name, ".") != 0 && strcmp(file.name, "..") != 0)
		{
			campaign_list_t *subFolder;
			list->numSubFolders++;
			list->subFolders = sys_mem_realloc(
				list->subFolders, sizeof(campaign_list_t)*list->numSubFolders);
			subFolder = &list->subFolders[list->numSubFolders-1];
			CampaignListInit(subFolder);
			LoadCampaignsFromFolder(subFolder, file.name, file.path, isDogfight);
		}
		else if (file.is_reg)
		{
			char title[256];
			if (IsCampaignOK(file.path, title))
			{
				AddCustomCampaignEntry(
					list, file.name, file.path, title, isDogfight);
			}
		}
	}

	tinydir_close(&dir);
}

int IsCampaignOK(const char *path, char *title)
{
	char buf[256];
	int numMissions;
	if (ScanCampaign(path, buf, &numMissions) == CAMPAIGN_OK)
	{
		sprintf(title, "%s (%d)", buf, numMissions);
		return 1;
	}
	return 0;
}

campaign_entry_t *AddAndGetCampaignEntry(
	campaign_list_t *list, const char *title, int isDogfight);

void AddBuiltinCampaignEntry(
	campaign_list_t *list, const char *title, int isDogfight, int builtinIndex)
{
	campaign_entry_t *entry = AddAndGetCampaignEntry(list, title, isDogfight);
	entry->isBuiltin = 1;
	entry->builtinIndex = builtinIndex;
}
void AddCustomCampaignEntry(
	campaign_list_t *list,
	const char *filename,
	const char *path,
	const char *title,
	int isDogfight)
{
	campaign_entry_t *entry = AddAndGetCampaignEntry(list, title, isDogfight);
	strcpy(entry->filename, filename);
	strcpy(entry->path, path);
	entry->isBuiltin = 0;
}

campaign_entry_t *AddAndGetCampaignEntry(
	campaign_list_t *list, const char *title, int isDogfight)
{
	campaign_entry_t *entry;
	list->num++;
	list->list = sys_mem_realloc(list->list, sizeof(campaign_entry_t)*list->num);
	entry = &list->list[list->num-1];
	strcpy(entry->info, title);
	entry->isDogfight = isDogfight;
	return entry;
}
