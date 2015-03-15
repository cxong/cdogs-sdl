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
#include "autosave.h"

#include <errno.h>
#include <locale.h>
#include <stdio.h>
#include <string.h>

#include <json/json.h>

#include <cdogs/campaign_entry.h>
#include <cdogs/json_utils.h>
#include <cdogs/utils.h>
#include <cdogs/sys_specifics.h>

Autosave gAutosave;


void MissionSaveInit(MissionSave *ms)
{
	memset(ms, 0, sizeof *ms);
	ms->IsValid = 1;
}


void AutosaveInit(Autosave *autosave)
{
	memset(&autosave->LastMission.Campaign, 0, sizeof autosave->LastMission.Campaign);
	autosave->LastMission.Campaign.Mode = GAME_MODE_NORMAL;
	strcpy(autosave->LastMission.Password, "");
	CArrayInit(&autosave->Missions, sizeof(MissionSave));
}
void AutosaveTerminate(Autosave *autosave)
{
	for (int i = 0; i < (int)autosave->Missions.size; i++)
	{
		MissionSave *m = CArrayGet(&autosave->Missions, i);
		CampaignEntryTerminate(&m->Campaign);
	}
	CArrayTerminate(&autosave->Missions);
}

static void LoadCampaignNode(CampaignEntry *c, json_t *node)
{
	CSTRDUP(c->Path, json_find_first_label(node, "Path")->child->text);
	LoadBool(&c->IsBuiltin, node, "IsBuiltin");
	c->Mode = GAME_MODE_NORMAL;
	c->BuiltinIndex = atoi(json_find_first_label(node, "BuiltinIndex")->child->text);
}
static void AddCampaignNode(CampaignEntry *c, json_t *root)
{
	json_t *subConfig = json_new_object();
	// Save relative path so that save files are portable across installs
	char path[CDOGS_PATH_MAX] = "";
	if (c->Path != NULL)
	{
		char cwd[CDOGS_PATH_MAX];
		if (getcwd(cwd, CDOGS_PATH_MAX) == NULL)
		{
			printf("Error getting CWD; %s\n", strerror(errno));
			strcpy(path, c->Path);
		}
		else
		{
			RelPath(path, c->Path, cwd);
		}
	}
	json_insert_pair_into_object(
		subConfig, "Path", json_new_string(path));
	json_insert_pair_into_object(
		subConfig, "IsBuiltin", json_new_bool(c->IsBuiltin));
	AddIntPair(subConfig, "BuiltinIndex", c->BuiltinIndex);
	json_insert_pair_into_object(root, "Campaign", subConfig);
}

static void LoadMissionNode(MissionSave *m, json_t *node)
{
	MissionSaveInit(m);
	LoadCampaignNode(&m->Campaign, json_find_first_label(node, "Campaign")->child);
	strcpy(m->Password, json_find_first_label(node, "Password")->child->text);
	LoadInt(&m->MissionsCompleted, node, "MissionsCompleted");
	m->IsValid = 1;
	// If the campaign is from a file, check that file exists
	if (!m->Campaign.IsBuiltin)
	{
		m->IsValid = access(m->Campaign.Path, F_OK | R_OK) != -1;
	}
}
static json_t *CreateMissionNode(MissionSave *m)
{
	json_t *subConfig = json_new_object();
	AddCampaignNode(&m->Campaign, subConfig);
	json_insert_pair_into_object(subConfig, "Password", json_new_string(m->Password));
	AddIntPair(subConfig, "MissionsCompleted", m->MissionsCompleted);
	return subConfig;
}

static void LoadMissionNodes(Autosave *a, json_t *root, const char *nodeName)
{
	json_t *child;
	if (json_find_first_label(root, nodeName) == NULL)
	{
		return;
	}
	child = json_find_first_label(root, nodeName)->child->child;
	while (child != NULL)
	{
		MissionSave m;
		LoadMissionNode(&m, child);
		AutosaveAddMission(a, &m, m.Campaign.BuiltinIndex);
		child = child->next;
	}
}
static void AddMissionNodes(Autosave *a, json_t *root, const char *nodeName)
{
	json_t *missions = json_new_array();
	for (int i = 0; i < (int)a->Missions.size; i++)
	{
		json_insert_child(
			missions, CreateMissionNode(CArrayGet(&a->Missions, i)));
	}
	json_insert_pair_into_object(root, nodeName, missions);
}

void AutosaveLoad(Autosave *autosave, const char *filename)
{
	FILE *f = fopen(filename, "r");
	json_t *root = NULL;
	
	if (f == NULL)
	{
		printf("Error loading autosave '%s'\n", filename);
		goto bail;
	}
	
	if (json_stream_parse(f, &root) != JSON_OK)
	{
		printf("Error parsing autosave '%s'\n", filename);
		goto bail;
	}
	// Note: need to load missions before LastMission because the former
	// will overwrite the latter, since AutosaveAddMission also
	// writes to LastMission
	LoadMissionNodes(autosave, root, "Missions");
	if (json_find_first_label(root, "LastMission"))
	{
		LoadMissionNode(
			&autosave->LastMission,
			json_find_first_label(root, "LastMission")->child);
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
	FILE *f = fopen(filename, "w");
	char *text = NULL;
	json_t *root;
	
	if (f == NULL)
	{
		printf("Error saving autosave '%s'\n", filename);
		return;
	}
	
	setlocale(LC_ALL, "");
	
	root = json_new_object();
	json_insert_pair_into_object(root, "Version", json_new_number("2"));
	json_insert_pair_into_object(
		root, "LastMission", CreateMissionNode(&autosave->LastMission));
	AddMissionNodes(autosave, root, "Missions");

	json_tree_to_string(root, &text);
	char *formatText = json_format_string(text);
	fputs(formatText, f);
	
	// clean up
	free(formatText);
	free(text);
	json_free_value(&root);
	
	fclose(f);
}

MissionSave *AutosaveFindMission(
	Autosave *autosave, const char *path, int builtinIndex)
{
	for (int i = 0; i < (int)autosave->Missions.size; i++)
	{
		MissionSave *m = CArrayGet(&autosave->Missions, i);
		const char *campaignPath = m->Campaign.Path;
		if (path == NULL || strlen(path) == 0)
		{
			// builtin campaign
			if (m->Campaign.IsBuiltin &&
				m->Campaign.BuiltinIndex == builtinIndex)
			{
				return m;
			}
		}
		else if (campaignPath != NULL && strcmp(campaignPath, path) == 0)
		{
			if (!m->Campaign.IsBuiltin)
			{
				return m;
			}
		}
	}
	return NULL;
}

void AutosaveAddMission(
	Autosave *autosave, MissionSave *mission, int builtinIndex)
{
	MissionSave *existingMission = AutosaveFindMission(
		autosave, mission->Campaign.Path, builtinIndex);
	if (existingMission != NULL)
	{
		CampaignEntryTerminate(&existingMission->Campaign);
	}
	else
	{
		CArrayPushBack(&autosave->Missions, mission);
		existingMission =
			CArrayGet(&autosave->Missions, autosave->Missions.size - 1);
	}
	memcpy(existingMission, mission, sizeof *existingMission);
	CampaignEntryCopy(&existingMission->Campaign, &mission->Campaign);
	memcpy(&autosave->LastMission, mission, sizeof autosave->LastMission);
}

void AutosaveLoadMission(
	Autosave *autosave, MissionSave *mission, const char *path, int builtinIndex)
{
	MissionSave *existingMission = AutosaveFindMission(
		autosave, path, builtinIndex);
	MissionSaveInit(mission);
	if (existingMission != NULL)
	{
		memcpy(mission, existingMission, sizeof *mission);
	}
}