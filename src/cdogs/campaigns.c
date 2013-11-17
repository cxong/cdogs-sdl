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

#include <tinydir/tinydir.h>

#include <cdogs/files.h>
#include <cdogs/mission.h>
#include <cdogs/utils.h>


void CampaignInit(CampaignOptions *campaign)
{
	memset(campaign, 0, sizeof *campaign);
}
void CampaignTerminate(CampaignOptions *campaign)
{
	CampaignSettingTerminate(&campaign->Setting);
	memset(campaign, 0, sizeof *campaign);
}
void CampaignSettingInit(CampaignSettingNew *setting)
{
	memset(setting, 0, sizeof *setting);
	CharacterStoreInit(&setting->characters);
}
void CampaignSettingTerminate(CampaignSettingNew *setting)
{
	CFREE(setting->missions);
	CharacterStoreTerminate(&setting->characters);
	memset(setting, 0, sizeof *setting);
}

void CampaignListInit(campaign_list_t *list);
void LoadBuiltinCampaigns(campaign_list_t *list);
void LoadBuiltinDogfights(campaign_list_t *list);
void LoadCampaignsFromFolder(
	campaign_list_t *list, const char *name, const char *path, campaign_mode_e mode);
void LoadQuickPlayEntry(campaign_entry_t *entry);

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

	LoadQuickPlayEntry(&campaigns->quickPlayEntry);

	printf("\n");
}

void UnloadAllCampaigns(custom_campaigns_t *campaigns)
{
	if (campaigns)
	{
		if (campaigns->campaignList.subFolders)
		{
			CFREE(campaigns->campaignList.subFolders);
		}
		if (campaigns->campaignList.list)
		{
			CFREE(campaigns->campaignList.list);
		}
		if (campaigns->dogfightList.subFolders)
		{
			CFREE(campaigns->dogfightList.subFolders);
		}
		if (campaigns->dogfightList.list)
		{
			CFREE(campaigns->dogfightList.list);
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
	campaign_list_t *list, const char *title, campaign_mode_e mode, int builtinIndex);

void LoadBuiltinCampaigns(campaign_list_t *list)
{
	int i = 0;
	for (i = 0; SetupBuiltinCampaign(i); i++)
	{
		AddBuiltinCampaignEntry(
			list, gCampaign.Setting.title, CAMPAIGN_MODE_NORMAL, i);
	}
}
void LoadBuiltinDogfights(campaign_list_t *list)
{
	int i = 0;
	for (i = 0; SetupBuiltinDogfight(i); i++)
	{
		AddBuiltinCampaignEntry(
			list, gCampaign.Setting.title, CAMPAIGN_MODE_DOGFIGHT, i);
	}
}

void LoadQuickPlayEntry(campaign_entry_t *entry)
{
	strcpy(entry->filename, "");
	strcpy(entry->path, "");
	strcpy(entry->info, "");
	entry->isBuiltin = 1;
	entry->mode = CAMPAIGN_MODE_QUICK_PLAY;
	entry->builtinIndex = 0;
}

int IsCampaignOK(const char *path, char *title);
void AddCustomCampaignEntry(
	campaign_list_t *list,
	const char *filename,
	const char *path,
	const char *title,
	campaign_mode_e mode);

void LoadCampaignsFromFolder(
	campaign_list_t *list, const char *name, const char *path, campaign_mode_e mode)
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
			CREALLOC(list->subFolders, sizeof(campaign_list_t)*list->numSubFolders);
			subFolder = &list->subFolders[list->numSubFolders-1];
			CampaignListInit(subFolder);
			LoadCampaignsFromFolder(subFolder, file.name, file.path, mode);
		}
		else if (file.is_reg)
		{
			char title[256];
			if (IsCampaignOK(file.path, title))
			{
				AddCustomCampaignEntry(list, file.name, file.path, title, mode);
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
	campaign_list_t *list, const char *title, campaign_mode_e mode);

void AddBuiltinCampaignEntry(
	campaign_list_t *list, const char *title, campaign_mode_e mode, int builtinIndex)
{
	campaign_entry_t *entry = AddAndGetCampaignEntry(list, title, mode);
	entry->isBuiltin = 1;
	entry->builtinIndex = builtinIndex;
}
void AddCustomCampaignEntry(
	campaign_list_t *list,
	const char *filename,
	const char *path,
	const char *title,
	campaign_mode_e mode)
{
	campaign_entry_t *entry = AddAndGetCampaignEntry(list, title, mode);
	strcpy(entry->filename, filename);
	strcpy(entry->path, path);
	entry->isBuiltin = 0;
}

campaign_entry_t *AddAndGetCampaignEntry(
	campaign_list_t *list, const char *title, campaign_mode_e mode)
{
	campaign_entry_t *entry;
	list->num++;
	CREALLOC(list->list, sizeof(campaign_entry_t)*list->num);
	entry = &list->list[list->num-1];
	memset(entry, 0, sizeof *entry);
	strcpy(entry->info, title);
	entry->mode = mode;
	return entry;
}
