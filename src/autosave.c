/*
 C-Dogs SDL
 A port of the legendary (and fun) action/arcade cdogs.

 Copyright (c) 2013-2016, 2019, 2021-2024 Cong Xu
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
#include "autosave.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <json/json.h>

#include <cdogs/campaign_entry.h>
#include <cdogs/json_utils.h>
#include <cdogs/log.h>
#include <cdogs/sys_specifics.h>
#include <cdogs/utils.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#define VERSION 3

Autosave gAutosave;

static void PlayerSaveInit(PlayerSave *ps)
{
	memset(ps, 0, sizeof *ps);
	CArrayInit(&ps->ammo, sizeof(int));
}
static void PlayerSaveTerminate(PlayerSave *ps)
{
	for (int i = 0; i < MAX_WEAPONS; i++)
	{
		CFREE(ps->Guns[i]);
	}
	CArrayTerminate(&ps->ammo);
}
static void PlayerSavesTerminate(CArray *a)
{
	CA_FOREACH(PlayerSave, ps, *a)
	PlayerSaveTerminate(ps);
	CA_FOREACH_END()
	CArrayTerminate(a);
}

void CampaignSaveInit(CampaignSave *ms)
{
	memset(ms, 0, sizeof *ms);
	ms->IsValid = true;
	CArrayInit(&ms->MissionsCompleted, sizeof(int));
	CArrayInit(&ms->Players, sizeof(PlayerSave));
}

bool CampaignSaveIsValid(const CampaignSave *cs)
{
	return cs != NULL && cs->IsValid && strlen(cs->Campaign.Path) > 0;
}

void AutosaveInit(Autosave *autosave)
{
	memset(autosave, 0, sizeof *autosave);
	autosave->LastCampaignIndex = -1;
	CArrayInit(&autosave->Campaigns, sizeof(CampaignSave));
}
void AutosaveTerminate(Autosave *autosave)
{
	CA_FOREACH(CampaignSave, m, autosave->Campaigns)
	CampaignEntryTerminate(&m->Campaign);
	CArrayTerminate(&m->MissionsCompleted);
	PlayerSavesTerminate(&m->Players);
	CA_FOREACH_END()
	CArrayTerminate(&autosave->Campaigns);
}

static void LoadCampaignNode(CampaignEntry *c, json_t *node)
{
	const char *path = json_find_first_label(node, "Path")->child->text;
	if (path != NULL)
	{
		c->Path = json_unescape(path);
	}
	c->Mode = GAME_MODE_NORMAL;
}
static void AddCampaignNode(CampaignEntry *c, json_t *root)
{
	json_t *subConfig = json_new_object();
	// Save relative path so that save files are portable across installs
	char path[CDOGS_PATH_MAX] = "";
	RelPathFromCWD(path, c->Path);
	json_insert_pair_into_object(
		subConfig, "Path", json_new_string(json_escape(path)));
	json_insert_pair_into_object(root, "Campaign", subConfig);
}

static void LoadPlayersNode(CArray *players, json_t *node)
{
	json_t *playersNode = json_find_first_label(node, "Players");
	if (playersNode == NULL)
	{
		return;
	}
	for (json_t *child = playersNode->child->child; child; child = child->next)
	{
		PlayerSave ps;
		PlayerSaveInit(&ps);

		json_t *gunsNode = json_find_first_label(child, "Guns")->child;
		int i = 0;
		for (json_t *gunNode = gunsNode->child;
			 gunNode != NULL && i < MAX_WEAPONS; gunNode = gunNode->next, i++)
		{
			char *gun = json_unescape(gunNode->text);
			if (strlen(gun) > 0)
			{
				ps.Guns[i] = gun;
			}
			else
			{
				CFREE(gun);
			}
		}

		LoadIntArray(&ps.ammo, child, "Ammo");

		LoadInt(&ps.Lives, child, "Lives");
		LoadInt(&ps.HP, child, "HP");

		CArrayPushBack(players, &ps);
	}
}
static void AddPlayersNode(CArray *players, json_t *root)
{
	json_t *playersNode = json_new_array();

	CA_FOREACH(const PlayerSave, ps, *players)
	json_t *playerNode = json_new_object();

	json_t *gunsNode = json_new_array();
	for (int i = 0; i < MAX_WEAPONS; i++)
	{
		json_insert_child(
			gunsNode, json_new_string(ps->Guns[i] != NULL ? ps->Guns[i] : ""));
	}
	json_insert_pair_into_object(playerNode, "Guns", gunsNode);
	AddIntArray(playerNode, "Ammo", &ps->ammo);
	AddIntPair(playerNode, "Lives", ps->Lives);
	AddIntPair(playerNode, "HP", ps->HP);

	json_insert_child(playersNode, playerNode);
	CA_FOREACH_END()

	json_insert_pair_into_object(root, "Players", playersNode);
}

static void LoadMissionNode(CampaignSave *m, json_t *node, const int version)
{
	CampaignSaveInit(m);
	LoadCampaignNode(
		&m->Campaign, json_find_first_label(node, "Campaign")->child);
	LoadInt(&m->NextMission, node, "NextMission");
	if (version < 3)
	{
		int missionsCompleted = 0;
		LoadInt(&missionsCompleted, node, "MissionsCompleted");
		for (int i = 0; i < missionsCompleted; i++)
		{
			CArrayPushBack(&m->MissionsCompleted, &i);
		}
		m->NextMission = missionsCompleted;
	}
	else
	{
		LoadIntArray(&m->MissionsCompleted, node, "MissionsCompleted");
	}
	// Check that file exists
	char buf[CDOGS_PATH_MAX];
	GetDataFilePath(buf, m->Campaign.Path);
	m->IsValid = access(buf, F_OK | R_OK) != -1;
	LoadPlayersNode(&m->Players, node);
}
static json_t *CreateMissionNode(CampaignSave *m)
{
	json_t *subConfig = json_new_object();
	AddCampaignNode(&m->Campaign, subConfig);
	AddIntPair(subConfig, "NextMission", m->NextMission);
	AddIntArray(subConfig, "MissionsCompleted", &m->MissionsCompleted);
	AddPlayersNode(&m->Players, subConfig);
	return subConfig;
}

static void LoadMissionNodes(
	Autosave *a, json_t *root, const char *nodeName, const int version)
{
	json_t *child;
	if (json_find_first_label(root, nodeName) == NULL)
	{
		return;
	}
	child = json_find_first_label(root, nodeName)->child->child;
	while (child != NULL)
	{
		CampaignSave m;
		LoadMissionNode(&m, child, version);
		AutosaveAddCampaign(a, &m);
		child = child->next;
	}
}
static void AddMissionNodes(Autosave *a, json_t *root, const char *nodeName)
{
	json_t *missions = json_new_array();
	CA_FOREACH(CampaignSave, m, a->Campaigns)
	json_insert_child(missions, CreateMissionNode(m));
	CA_FOREACH_END()
	json_insert_pair_into_object(root, nodeName, missions);
}

static CampaignSave *FindCampaign(
	Autosave *autosave, const char *path, int *missionIndex);

void AutosaveLoad(Autosave *autosave, const char *filename)
{
	FILE *f = fopen(filename, "r");
	json_t *root = NULL;

	if (f == NULL)
	{
		LOG(LM_MAIN, LL_WARN, "Error loading autosave '%s'", filename);
		goto bail;
	}

	if (json_stream_parse(f, &root) != JSON_OK)
	{
		LOG(LM_MAIN, LL_ERROR, "Error parsing autosave '%s'", filename);
		goto bail;
	}
	int version = 2;
	LoadInt(&version, root, "Version");
	LoadMissionNodes(autosave, root, "Missions", version);
	if (version < 3)
	{
		if (json_find_first_label(root, "LastMission"))
		{
			json_t *lastMission =
				json_find_first_label(root, "LastMission")->child;
			json_t *campaign =
				json_find_first_label(lastMission, "Campaign")->child;
			char *path = NULL;
			LoadStr(&path, campaign, "Path");
			if (path != NULL)
			{
				FindCampaign(autosave, path, &autosave->LastCampaignIndex);
				CFREE(path);
			}
		}
	}
	else
	{
		LoadInt(&autosave->LastCampaignIndex, root, "LastCampaignIndex");
	}

bail:
	json_free_value(&root);
	if (f != NULL)
	{
		fclose(f);
	}
}

void AutosaveSave(Autosave *autosave, const char *filename)
{
	char *text = NULL;
	json_t *root;

	root = json_new_object();
	AddIntPair(root, "Version", VERSION);
	AddIntPair(root, "LastCampaignIndex", autosave->LastCampaignIndex);
	AddMissionNodes(autosave, root, "Missions");

	json_tree_to_string(root, &text);
	char *formatText = json_format_string(text);

	FILE *f = fopen(filename, "w");
	if (f == NULL)
	{
		printf("Error saving autosave '%s'\n", filename);
		return;
	}
	fputs(formatText, f);
	fclose(f);

	// clean up
	CFREE(formatText);
	CFREE(text);
	json_free_value(&root);

#ifdef __EMSCRIPTEN__
	EM_ASM(
		// persist changes
		FS.syncfs(
			false, function(err) { assert(!err); }););
#endif
}

static CampaignSave *FindCampaign(
	Autosave *autosave, const char *path, int *missionIndex)
{
	if (path == NULL || strlen(path) == 0)
	{
		return NULL;
	}

	// Turn the path into a relative one, since autosave paths are all stored
	// in this form
	char relPath[CDOGS_PATH_MAX] = "";
	RelPathFromCWD(relPath, path);
	CA_FOREACH(CampaignSave, m, autosave->Campaigns)
	const char *campaignPath = m->Campaign.Path;
	if (campaignPath != NULL && strcmp(campaignPath, relPath) == 0)
	{
		if (missionIndex != NULL)
		{
			*missionIndex = _ca_index;
		}
		return m;
	}
	CA_FOREACH_END()
	return NULL;
}

void AutosaveAdd(
	Autosave *a, const CampaignEntry *ce, const int missionIndex,
	const int nextMission, const CArray *playerDatas)
{
	CampaignSave ms;
	CampaignSaveInit(&ms);
	CampaignEntryCopy(&ms.Campaign, ce);
	CArrayPushBack(&ms.MissionsCompleted, &missionIndex);
	ms.NextMission = nextMission;
	CA_FOREACH(const PlayerData, pd, *playerDatas)
	PlayerSave ps;
	PlayerSaveInit(&ps);
	for (int i = 0; i < MAX_WEAPONS; i++)
	{
		if (pd->guns[i])
		{
			CSTRDUP(ps.Guns[i], pd->guns[i]->name);
		}
	}
	CArrayCopy(&ps.ammo, &pd->ammo);
	ps.Lives = pd->Lives;
	ps.HP = pd->HP;
	CArrayPushBack(&ms.Players, &ps);
	CA_FOREACH_END()
	AutosaveAddCampaign(a, &ms);
}

void AutosaveAddCampaign(Autosave *autosave, CampaignSave *cs)
{
	CampaignSave *existing = FindCampaign(
		autosave, cs->Campaign.Path, &autosave->LastCampaignIndex);
	if (existing != NULL)
	{
		CampaignEntryTerminate(&existing->Campaign);
	}
	else
	{
		CArrayPushBack(&autosave->Campaigns, cs);
		autosave->LastCampaignIndex = (int)autosave->Campaigns.size - 1;
		existing =
			CArrayGet(&autosave->Campaigns, autosave->LastCampaignIndex);
		CampaignSaveInit(existing);
	}

	existing->NextMission = cs->NextMission;
	// Update missions completed
	CA_FOREACH(const int, missionIndex, cs->MissionsCompleted)
	bool found = false;
	for (int i = 0; i < (int)existing->MissionsCompleted.size; i++)
	{
		if (*(int *)CArrayGet(&existing->MissionsCompleted, i) ==
			*missionIndex)
		{
			found = true;
			break;
		}
	}
	if (!found)
	{
		CArrayPushBack(&existing->MissionsCompleted, missionIndex);
	}
	CA_FOREACH_END()
	// Sort and remove duplicates
	qsort(
		existing->MissionsCompleted.data, existing->MissionsCompleted.size,
		existing->MissionsCompleted.elemSize, CompareIntsAsc);
	CArrayUnique(&existing->MissionsCompleted, IntsEqual);
	CArrayTerminate(&cs->MissionsCompleted);

	PlayerSavesTerminate(&existing->Players);
	memcpy(&existing->Players, &cs->Players, sizeof cs->Players);

	memcpy(&existing->Campaign, &cs->Campaign, sizeof existing->Campaign);
}

const CampaignSave *AutosaveGetCampaign(Autosave *autosave, const char *path)
{
	return FindCampaign(autosave, path, NULL);
}

const CampaignSave *AutosaveGetLastCampaign(const Autosave *a)
{
	if (a->LastCampaignIndex < 0)
	{
		return NULL;
	}
	return CArrayGet(&a->Campaigns, a->LastCampaignIndex);
}

void PlayerSavesApply(const CArray *playerSaves, const bool weaponPersist)
{
	for (int i = 0, idx = 0;
		 i < (int)gPlayerDatas.size && idx < (int)playerSaves->size;
		 i++, idx++)
	{
		PlayerData *p = CArrayGet(&gPlayerDatas, i);
		if (!p->IsLocal)
		{
			idx--;
			continue;
		}
		const PlayerSave *ps = CArrayGet(playerSaves, idx);
		if (weaponPersist)
		{
			for (int j = 0; j < MAX_WEAPONS; j++)
			{
				if (ps->Guns[j] != NULL)
				{
					PlayerAddWeapon(p, StrWeaponClass(ps->Guns[j]));
				}
			}
			PlayerAddMinimalWeapons(p);
			CArrayCopy(&p->ammo, &ps->ammo);
		}
		if (ps->Lives > 0)
		{
			PlayerSetLives(p, ps->Lives);
		}
		if (ps->HP > 0)
		{
			p->HP = ps->HP;
		}
	}
}
