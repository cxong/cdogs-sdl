/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.

    Copyright (c) 2013-2015, Cong Xu
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
#include <cdogs/log.h>
#include <cdogs/map_new.h>
#include <cdogs/mission.h>
#include <cdogs/utils.h>


void CampaignInit(CampaignOptions *campaign)
{
	memset(campaign, 0, sizeof *campaign);
	CampaignSettingInit(&campaign->Setting);
	campaign->seed = ConfigGetInt(&gConfig, "Game.RandomSeed");
}
void CampaignTerminate(CampaignOptions *campaign)
{
	campaign->IsLoaded = false;
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
static void LoadCampaignsFromFolder(
	campaign_list_t *list, const char *name, const char *path,
	const GameMode mode);
static void LoadQuickPlayEntry(CampaignEntry *entry);

void LoadAllCampaigns(custom_campaigns_t *campaigns)
{
	char buf[CDOGS_PATH_MAX];

	CampaignListInit(&campaigns->campaignList);
	CampaignListInit(&campaigns->dogfightList);

	LOG(LM_MAIN, LL_INFO, "Load campaigns");
	GetDataFilePath(buf, CDOGS_CAMPAIGN_DIR);
	LoadCampaignsFromFolder(
		&campaigns->campaignList,
		"",
		buf,
		GAME_MODE_NORMAL);

	LOG(LM_MAIN, LL_INFO, "Load dogfights");
	GetDataFilePath(buf, CDOGS_DOGFIGHT_DIR);
	LoadCampaignsFromFolder(
		&campaigns->dogfightList,
		"",
		buf,
		GAME_MODE_DOGFIGHT);

	debug(D_NORMAL, "Load quick play\n");
	LoadQuickPlayEntry(&campaigns->quickPlayEntry);
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
	list->Name = NULL;
	CArrayInit(&list->subFolders, sizeof(campaign_list_t));
	CArrayInit(&list->list, sizeof(CampaignEntry));
}
static void CampaignListTerminate(campaign_list_t *list)
{
	CFREE(list->Name);
	for (int i = 0; i < (int)list->subFolders.size; i++)
	{
		campaign_list_t *sublist = CArrayGet(&list->subFolders, i);
		CampaignListTerminate(sublist);
	}
	CArrayTerminate(&list->subFolders);
	for (int i = 0; i < (int)list->list.size; i++)
	{
		CampaignEntryTerminate(CArrayGet(&list->list, i));
	}
	CArrayTerminate(&list->list);
}

static void LoadQuickPlayEntry(CampaignEntry *entry)
{
	entry->Filename = NULL;
	entry->Path = NULL;
	entry->Info = NULL;
	entry->Mode = GAME_MODE_QUICK_PLAY;
}

static void LoadCampaignsFromFolder(
	campaign_list_t *list, const char *name, const char *path,
	const GameMode mode)
{
	tinydir_dir dir;
	int i;

	CSTRDUP(list->Name, name);
	if (tinydir_open_sorted(&dir, path) == -1)
	{
		printf("Cannot load campaigns from path %s\n", path);
		return;
	}

	for (i = 0; i < (int)dir.n_files; i++)
	{
		tinydir_file file;
		tinydir_readfile_n(&dir, &file, i);

		// Ignore campaigns that start with a ~
		// These are autosaved

		const bool isArchive =
			strcmp(file.extension, "cdogscpn") == 0 ||
			strcmp(file.extension, "CDOGSCPN") == 0;
		if (file.is_dir && !isArchive &&
			strcmp(file.name, ".") != 0 && strcmp(file.name, "..") != 0)
		{
			campaign_list_t subFolder;
			CampaignListInit(&subFolder);
			LoadCampaignsFromFolder(&subFolder, file.name, file.path, mode);
			CArrayPushBack(&list->subFolders, &subFolder);
		}
		else if ((file.is_reg || isArchive) && file.name[0] != '~')
		{
			CampaignEntry entry;
			if (CampaignEntryTryLoad(&entry, file.path, mode))
			{
				CArrayPushBack(&list->list, &entry);
			}
		}
	}

	tinydir_close(&dir);
}

Mission *CampaignGetCurrentMission(CampaignOptions *campaign)
{
	if (campaign->MissionIndex >= (int)campaign->Setting.Missions.size)
	{
		return NULL;
	}
	return CArrayGet(&campaign->Setting.Missions, campaign->MissionIndex);
}

void CampaignSeedRandom(const CampaignOptions *campaign)
{
	const unsigned int seed = 10 * campaign->MissionIndex + campaign->seed;
	debug(D_NORMAL, "Seeding with %u\n", seed);
	srand(seed);
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
