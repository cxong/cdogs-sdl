/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.

    Copyright (c) 2013-2017 Cong Xu
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

	PickupClassesLoadKeys(&gPickupClasses.KeyClasses);
}
void CampaignSettingTerminate(CampaignSetting *setting)
{
	CFREE(setting->Title);
	CFREE(setting->Author);
	CFREE(setting->Description);
	CA_FOREACH(Mission, m, setting->Missions)
		MissionTerminate(m);
	CA_FOREACH_END()
	CArrayTerminate(&setting->Missions);
	CharacterStoreTerminate(&setting->characters);
	memset(setting, 0, sizeof *setting);

	// Unload previous custom data
	SoundClear(gSoundDevice.customSounds);
	PicManagerClearCustom(&gPicManager);
	ParticleClassesClear(&gParticleClasses.CustomClasses);
	AmmoClassesClear(&gAmmo.CustomAmmo);
	CharacterClassesClear(&gCharacterClasses.CustomClasses);
	BulletClassesClear(&gBulletClasses.CustomClasses);
	WeaponClassesClear(&gGunDescriptions.CustomGuns);
	PickupClassesClear(&gPickupClasses.CustomClasses);
	PickupClassesClear(&gPickupClasses.KeyClasses);
	MapObjectsClear(&gMapObjects.CustomClasses);
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

	GetDataFilePath(buf, CDOGS_CAMPAIGN_DIR);
	LOG(LM_MAIN, LL_INFO, "Load campaigns from dir %s...", buf);
	LoadCampaignsFromFolder(
		&campaigns->campaignList,
		"",
		buf,
		GAME_MODE_NORMAL);

	GetDataFilePath(buf, CDOGS_DOGFIGHT_DIR);
	LOG(LM_MAIN, LL_INFO, "Load dogfights from dir %s...", buf);
	LoadCampaignsFromFolder(
		&campaigns->dogfightList,
		"",
		buf,
		GAME_MODE_DOGFIGHT);

	LOG(LM_MAIN, LL_INFO, "Load quick play...");
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
	CA_FOREACH(campaign_list_t, sublist, list->subFolders)
		CampaignListTerminate(sublist);
	CA_FOREACH_END()
	CArrayTerminate(&list->subFolders);
	CA_FOREACH(CampaignEntry, e, list->list)
		CampaignEntryTerminate(e);
	CA_FOREACH_END()
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
	const int seed =
		10 * campaign->MissionIndex + ConfigGetInt(&gConfig, "Game.RandomSeed");
	LOG(LM_MAIN, LL_INFO, "Seeding with %d", seed);
	srand((unsigned int)seed);
}

void CampaignAndMissionSetup(
	CampaignOptions *campaign, struct MissionOptions *mo)
{
	CampaignSeedRandom(campaign);
	SetupMission(
		CampaignGetCurrentMission(campaign), mo, campaign->MissionIndex);
}
