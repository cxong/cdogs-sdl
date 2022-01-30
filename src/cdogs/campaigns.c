/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.

	Copyright (c) 2013-2018, 2020-2021 Cong Xu
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
#include <cdogs/map_wolf.h>
#include <cdogs/mission.h>
#include <cdogs/player_template.h>
#include <cdogs/utils.h>

void CampaignInit(Campaign *campaign)
{
	memset(campaign, 0, sizeof *campaign);
	CampaignSettingInit(&campaign->Setting);
}
void CampaignTerminate(Campaign *campaign)
{
	CampaignUnload(&gCampaign);
	CampaignSettingTerminateAll(&campaign->Setting);
}
void CampaignSettingInit(CampaignSetting *setting)
{
	memset(setting, 0, sizeof *setting);
	CSTRDUP(setting->Title, "");
	CSTRDUP(setting->Author, "");
	CSTRDUP(setting->Description, "");
	CArrayInit(&setting->Missions, sizeof(Mission));
	CharacterStoreInit(&setting->characters);

	PickupClassesLoadKeys(&gPickupClasses.KeyClasses);
}
void CampaignSettingTerminate(CampaignSetting *c)
{
	CFREE(c->Title);
	CFREE(c->Author);
	CFREE(c->Description);
	CA_FOREACH(Mission, m, c->Missions)
	MissionTerminate(m);
	CA_FOREACH_END()
	CArrayTerminate(&c->Missions);
	CharacterStoreTerminate(&c->characters);
	for (int i = 0; i < MUSIC_COUNT; i++)
	{
		MusicChunkTerminate(&c->CustomSongs[i]);
	}
	if (c->CustomDataTerminate)
	{
		c->CustomDataTerminate(c->CustomData);
	}
	memset(c, 0, sizeof *c);
}
void CampaignSettingTerminateAll(CampaignSetting *setting)
{
	CampaignSettingTerminate(setting);

	// Unload previous custom data
	SoundClear(gSoundDevice.customSounds);
	PicManagerClearCustom(&gPicManager);
	ParticleClassesClear(&gParticleClasses.CustomClasses);
	AmmoClassesClear(&gAmmo.CustomAmmo);
	PlayerTemplatesClear(&gPlayerTemplates.CustomClasses);
	CharacterClassesClear(&gCharacterClasses.CustomClasses);
	BulletClassesClear(&gBulletClasses.CustomClasses);
	// HACK: assign temp variable for custom guns to avoid aliasing
	CArray *customGuns = &gWeaponClasses.CustomGuns;
	WeaponClassesClear(customGuns);
	PickupClassesClear(&gPickupClasses.CustomClasses);
	PickupClassesClear(&gPickupClasses.KeyClasses);
	MapObjectsClear(&gMapObjects.CustomClasses);
}

bool CampaignListIsEmpty(const CampaignList *c)
{
	return c->list.size == 0 && c->subFolders.size == 0;
}

static void CampaignListInit(CampaignList *list);
static void CampaignListTerminate(CampaignList *list);
static void LoadCampaignsFromFolder(
	CampaignList *list, const char *name, const char *path,
	const GameMode mode);
static void LoadQuickPlayEntry(CampaignEntry *entry);

void LoadAllCampaigns(CustomCampaigns *campaigns)
{
	MapWolfInit();

	char buf[CDOGS_PATH_MAX];

	CampaignListInit(&campaigns->campaignList);
	CampaignListInit(&campaigns->dogfightList);

	LOG(LM_MAIN, LL_INFO, "Load campaigns from system...");
	MapWolfLoadCampaignsFromSystem(&campaigns->campaignList);

	GetDataFilePath(buf, CDOGS_CAMPAIGN_DIR);
	LOG(LM_MAIN, LL_INFO, "Load campaigns from dir %s...", buf);
	LoadCampaignsFromFolder(
		&campaigns->campaignList, "", buf, GAME_MODE_NORMAL);

	GetDataFilePath(buf, CDOGS_DOGFIGHT_DIR);
	LOG(LM_MAIN, LL_INFO, "Load dogfights from dir %s...", buf);
	LoadCampaignsFromFolder(
		&campaigns->dogfightList, "", buf, GAME_MODE_DOGFIGHT);

	LOG(LM_MAIN, LL_INFO, "Load quick play...");
	LoadQuickPlayEntry(&campaigns->quickPlayEntry);
}

void UnloadAllCampaigns(CustomCampaigns *campaigns)
{
	MapWolfTerminate();
	if (campaigns)
	{
		CampaignListTerminate(&campaigns->campaignList);
		CampaignListTerminate(&campaigns->dogfightList);
	}
}

static void CampaignListInit(CampaignList *list)
{
	list->Name = NULL;
	CArrayInit(&list->subFolders, sizeof(CampaignList));
	CArrayInit(&list->list, sizeof(CampaignEntry));
}
static void CampaignListTerminate(CampaignList *list)
{
	CFREE(list->Name);
	CA_FOREACH(CampaignList, sublist, list->subFolders)
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
	CampaignList *list, const char *name, const char *path,
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
		// Ignore campaigns that start with a ~; these are autosaved
		// Also ignore special folders
		if (file.name[0] == '~' || file.name[0] == '.')
		{
			continue;
		}
		const bool isArchive = strcmp(file.extension, "cdogscpn") == 0 ||
							   strcmp(file.extension, "CDOGSCPN") == 0;
		if (file.is_dir && !isArchive)
		{
			CampaignList subFolder;
			CampaignListInit(&subFolder);
			LoadCampaignsFromFolder(&subFolder, file.name, file.path, mode);
			if (CampaignListIsEmpty(&subFolder))
			{
				CampaignListTerminate(&subFolder);
			}
			else
			{
				CArrayPushBack(&list->subFolders, &subFolder);
			}
		}
		CampaignEntry entry;
		if (CampaignEntryTryLoad(&entry, file.path, mode))
		{
			CArrayPushBack(&list->list, &entry);
		}
	}

	tinydir_close(&dir);
}

Mission *CampaignGetCurrentMission(Campaign *campaign)
{
	if (campaign->MissionIndex >= (int)campaign->Setting.Missions.size)
	{
		return NULL;
	}
	return CArrayGet(&campaign->Setting.Missions, campaign->MissionIndex);
}

void CampaignSeedRandom(const Campaign *campaign)
{
	const int seed = 10 * campaign->MissionIndex +
					 ConfigGetInt(&gConfig, "Game.RandomSeed");
	LOG(LM_MAIN, LL_INFO, "Seeding with %d", seed);
	srand((unsigned int)seed);
}

void CampaignAndMissionSetup(Campaign *campaign, struct MissionOptions *mo)
{
	Mission *m = CampaignGetCurrentMission(campaign);
	if (m == NULL)
	{
		return;
	}
	CampaignSeedRandom(campaign);
	SetupMission(m, mo, campaign->MissionIndex);
}

void CampaignDeleteMission(Campaign *c, const size_t idx)
{
	CASSERT(idx < c->Setting.Missions.size, "invalid mission index");
	Mission *m = CArrayGet(&c->Setting.Missions, idx);
	MissionTerminate(m);
	CArrayDelete(&c->Setting.Missions, idx);
	if (c->MissionIndex >= (int)c->Setting.Missions.size)
	{
		c->MissionIndex = MAX(0, (int)c->Setting.Missions.size - 1);
	}
}
