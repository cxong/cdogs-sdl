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

#include <cdogs/utils.h>


void AutosaveInit(Autosave *autosave)
{
	strcpy(autosave->LastMission.CampaignPath, "");
	strcpy(autosave->LastMission.Password, "");
}

static void LoadLastMissionNode(MissionSave *lm, json_t *node)
{
	strcpy(lm->CampaignPath, json_find_first_label(node, "CampaignPath")->child->text);
	strcpy(lm->Password, json_find_first_label(node, "Password")->child->text);
}
static void AddLastMissionNode(MissionSave *lm, json_t *root)
{
	json_t *subConfig = json_new_object();
	json_insert_pair_into_object(
								 subConfig, "CampaignPath", json_new_string(lm->CampaignPath));
	json_insert_pair_into_object(
								 subConfig, "Password", json_new_string(lm->Password));
	json_insert_pair_into_object(root, "LastMission", subConfig);
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
	LoadLastMissionNode(&autosave->LastMission, json_find_first_label(root, "LastMission")->child);

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
	AddLastMissionNode(&autosave->LastMission, root);

	json_tree_to_string(root, &text);
	fputs(json_format_string(text), f);
	
	// clean up
	free(text);
	json_free_value(&root);
	
	fclose(f);}
