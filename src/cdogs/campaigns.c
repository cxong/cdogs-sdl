/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.

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
#include "campaigns.h"

#include <stdio.h>

#include <tinydir/tinydir.h>

#include <cdogs/files.h>
#include <cdogs/map_new.h>
#include <cdogs/mission.h>
#include <cdogs/utils.h>


void CampaignInit(CampaignOptions *campaign)
{
	memset(campaign, 0, sizeof *campaign);
	CampaignSettingInit(&campaign->Setting);
}
void CampaignTerminate(CampaignOptions *campaign)
{
	CampaignSettingTerminate(&campaign->Setting);
}
void CampaignSettingInit(CampaignSetting *setting)
{
	memset(setting, 0, sizeof *setting);
	CArrayInit(&setting->Missions, sizeof(Mission));
	CharacterStoreInit(&setting->characters);
}
void CampaignSettingTerminate(CampaignSetting *setting)
{
	CFREE(setting->Title);
	CFREE(setting->Author);
	CFREE(setting->Description);
	for (int i = 0; i < (int)setting->Missions.size; i++)
	{
		MissionTerminate(CArrayGet(&setting->Missions, i));
	}
	CArrayTerminate(&setting->Missions);
	CharacterStoreTerminate(&setting->characters);
	memset(setting, 0, sizeof *setting);
}

static void CampaignListInit(campaign_list_t *list);
static void CampaignListTerminate(campaign_list_t *list);
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
		CampaignListTerminate(&campaigns->campaignList);
		CampaignListTerminate(&campaigns->dogfightList);
	}
}

static void CampaignListInit(campaign_list_t *list)
{
	strcpy(list->name, "");
	CArrayInit(&list->subFolders, sizeof(campaign_list_t));
	CArrayInit(&list->list, sizeof(campaign_entry_t));
}
static void CampaignListTerminate(campaign_list_t *list)
{
	for (int i = 0; i < (int)list->subFolders.size; i++)
	{
		campaign_list_t *sublist = CArrayGet(&list->subFolders, i);
		CampaignListTerminate(sublist);
	}
	CArrayTerminate(&list->subFolders);
	CArrayTerminate(&list->list);
}

void AddBuiltinCampaignEntry(
	campaign_list_t *list,
	const char *title,
	campaign_mode_e mode,
	int numMissions,
	int builtinIndex);

void LoadBuiltinCampaigns(campaign_list_t *list)
{
	int i = 0;
	for (i = 0; SetupBuiltinCampaign(i); i++)
	{
		AddBuiltinCampaignEntry(
			list,
			gCampaign.Setting.Title,
			CAMPAIGN_MODE_NORMAL,
			gCampaign.Setting.Missions.size,
			i);
	}
}
void LoadBuiltinDogfights(campaign_list_t *list)
{
	int i = 0;
	for (i = 0; SetupBuiltinDogfight(i); i++)
	{
		AddBuiltinCampaignEntry(
			list, gCampaign.Setting.Title, CAMPAIGN_MODE_DOGFIGHT, 1, i);
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

int IsCampaignOK(const char *path, char **buf, int *numMissions);
void AddCustomCampaignEntry(
	campaign_list_t *list,
	const char *filename,
	const char *path,
	const char *title,
	campaign_mode_e mode,
	int numMissions);

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

	for (i = 0; i < (int)dir.n_files; i++)
	{
		tinydir_file file;
		tinydir_readfile_n(&dir, &file, i);

		if (file.is_dir &&
			strcmp(file.name, ".") != 0 && strcmp(file.name, "..") != 0)
		{
			campaign_list_t subFolder;
			CampaignListInit(&subFolder);
			LoadCampaignsFromFolder(&subFolder, file.name, file.path, mode);
			CArrayPushBack(&list->subFolders, &subFolder);
		}
		else if (file.is_reg)
		{
			char title[256];
			char *buf;
			int numMissions;
			if (IsCampaignOK(file.path, &buf, &numMissions))
			{
				// cap length of title
				size_t maxLen = sizeof ((campaign_entry_t *)0)->info - 10;
				if (strlen(buf) > maxLen)
				{
					buf[maxLen] = '\0';
				}
				sprintf(title, "%s (%d)", buf, numMissions);
				AddCustomCampaignEntry(
					list, file.name, file.path, title, mode, numMissions);
				CFREE(buf);
			}
		}
	}

	tinydir_close(&dir);
}

int IsCampaignOK(const char *path, char **buf, int *numMissions)
{
	return MapNewScan(path, buf, numMissions) == 0;
}

campaign_entry_t *AddAndGetCampaignEntry(
	campaign_list_t *list, const char *title, campaign_mode_e mode);

void AddBuiltinCampaignEntry(
	campaign_list_t *list,
	const char *title,
	campaign_mode_e mode,
	int numMissions,
	int builtinIndex)
{
	campaign_entry_t *entry = AddAndGetCampaignEntry(list, title, mode);
	entry->isBuiltin = 1;
	entry->builtinIndex = builtinIndex;
	entry->numMissions = numMissions;
}
void AddCustomCampaignEntry(
	campaign_list_t *list,
	const char *filename,
	const char *path,
	const char *title,
	campaign_mode_e mode,
	int numMissions)
{
	campaign_entry_t *entry = AddAndGetCampaignEntry(list, title, mode);
	strcpy(entry->filename, filename);
	strcpy(entry->path, path);
	entry->isBuiltin = 0;
	entry->numMissions = numMissions;
}

campaign_entry_t *AddAndGetCampaignEntry(
	campaign_list_t *list, const char *title, campaign_mode_e mode)
{
	campaign_entry_t entry;
	memset(&entry, 0, sizeof entry);
	strncpy(entry.info, title, sizeof entry.info - 1);
	entry.mode = mode;
	CArrayPushBack(&list->list, &entry);
	return CArrayGet(&list->list, (int)list->list.size - 1);
}

Mission *CampaignGetCurrentMission(CampaignOptions *campaign)
{
	if (campaign->MissionIndex >= (int)campaign->Setting.Missions.size)
	{
		return NULL;
	}
	return CArrayGet(&campaign->Setting.Missions, campaign->MissionIndex);
}

void CampaignSeedRandom(CampaignOptions *campaign)
{
	srand(10 * campaign->MissionIndex + campaign->seed);
}

void CampaignAndMissionSetup(
	int buildTables, CampaignOptions *campaign, struct MissionOptions *mo)
{
	CampaignSeedRandom(campaign);
	SetupMission(
		buildTables,
		CampaignGetCurrentMission(campaign), mo,
		campaign->MissionIndex);
}
