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
#include "autosave.h"

#include <locale.h>
#include <stdio.h>
#include <string.h>

#include <json/json.h>

#include <cdogs/json_utils.h>
#include <cdogs/utils.h>
#include <cdogs/sys_specifics.h>

Autosave gAutosave;


void AutosaveInit(Autosave *autosave)
{
	memset(&autosave->LastMission.Campaign, 0, sizeof autosave->LastMission.Campaign);
	autosave->LastMission.Campaign.mode = CAMPAIGN_MODE_NORMAL;
	strcpy(autosave->LastMission.Password, "");
	autosave->Missions = NULL;
	autosave->NumMissions = 0;
}

static void LoadCampaignNode(campaign_entry_t *c, json_t *node)
{
	strcpy(c->path, json_find_first_label(node, "Path")->child->text);
	LoadBool(&c->isBuiltin, node, "IsBuiltin");
	c->mode = CAMPAIGN_MODE_NORMAL;
	c->builtinIndex = atoi(json_find_first_label(node, "BuiltinIndex")->child->text);
	LoadBool(&c->is_two_player, node, "IsTwoPlayer");
}
static void AddCampaignNode(campaign_entry_t *c, json_t *root)
{
	json_t *subConfig = json_new_object();
	json_insert_pair_into_object(subConfig, "Path", json_new_string(c->path));
	json_insert_pair_into_object(subConfig, "IsBuiltin", json_new_bool(c->isBuiltin));
	AddIntPair(subConfig, "BuiltinIndex", c->builtinIndex);
	json_insert_pair_into_object(subConfig, "IsTwoPlayer", json_new_bool(c->is_two_player));
	json_insert_pair_into_object(root, "Campaign", subConfig);
}

static void LoadMissionNode(MissionSave *m, json_t *node)
{
	LoadCampaignNode(&m->Campaign, json_find_first_label(node, "Campaign")->child);
	strcpy(m->Password, json_find_first_label(node, "Password")->child->text);
	m->IsValid = 1;
	// If the campaign is from a file, check that file exists
	if (!m->Campaign.isBuiltin)
	{
		m->IsValid = access(m->Campaign.path, F_OK | R_OK) != -1;
	}
}
static json_t *CreateMissionNode(MissionSave *m)
{
	json_t *subConfig = json_new_object();
	AddCampaignNode(&m->Campaign, subConfig);
	json_insert_pair_into_object(subConfig, "Password", json_new_string(m->Password));
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
		AutosaveAddMission(a, &m);
		child = child->next;
	}
}
static void AddMissionNodes(Autosave *a, json_t *root, const char *nodeName)
{
	json_t *missions = json_new_array();
	size_t i;
	for (i = 0; i < a->NumMissions; i++)
	{
		json_insert_child(missions, CreateMissionNode(&a->Missions[i]));
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
	LoadMissionNode(&autosave->LastMission, json_find_first_label(root, "LastMission")->child);
	LoadMissionNodes(autosave, root, "Missions");

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
		printf("Error saving config '%s'\n", filename);
		return;
	}
	
	setlocale(LC_ALL, "");
	
	root = json_new_object();
	json_insert_pair_into_object(root, "Version", json_new_number("1"));
	json_insert_pair_into_object(
		root, "LastMission", CreateMissionNode(&autosave->LastMission));
	AddMissionNodes(autosave, root, "Missions");

	json_tree_to_string(root, &text);
	fputs(json_format_string(text), f);
	
	// clean up
	free(text);
	json_free_value(&root);
	
	fclose(f);
}

MissionSave *AutosaveFindMission(Autosave *autosave, const char *path)
{
	size_t i;
	for (i = 0; i < autosave->NumMissions; i++)
	{
		if (strcmp(autosave->Missions[i].Campaign.path, path) == 0)
		{
			return &autosave->Missions[i];
		}
	}
	return NULL;
}

void AutosaveAddMission(Autosave *autosave, MissionSave *mission)
{
	MissionSave *existingMission = AutosaveFindMission(autosave, mission->Campaign.path);
	if (existingMission != NULL)
	{
		memcpy(existingMission, mission, sizeof *existingMission);
	}
	else
	{
		autosave->NumMissions++;
		CREALLOC(
			autosave->Missions, autosave->NumMissions * sizeof *autosave->Missions);
		memcpy(
			&autosave->Missions[autosave->NumMissions - 1],
			mission,
			sizeof *mission);
	}
	memcpy(&autosave->LastMission, mission, sizeof autosave->LastMission);
}

void AutosaveLoadMission(Autosave *autosave, MissionSave *mission, const char *path)
{
	MissionSave *existingMission = AutosaveFindMission(autosave, path);
	if (existingMission != NULL)
	{
		memcpy(mission, existingMission, sizeof *mission);
	}
	else
	{
		memset(mission, 0, sizeof *mission);
	}
}