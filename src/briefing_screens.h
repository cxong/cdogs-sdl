/*
	Copyright (c) 2014-2017, 2021-2022 Cong Xu
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
#pragma once

#include <cdogs/campaigns.h>
#include <cdogs/events.h>
#include <cdogs/font.h>

#include "game_loop.h"

typedef struct
{
	char *Title;
	FontOpts TitleOpts;
	char *Description;
	struct vec2i DescriptionPos;
	struct vec2i ObjectiveDescPos;
	struct vec2i ObjectiveInfoPos;
	int ObjectiveHeight;
	const struct MissionOptions *MissionOptions;
} MissionBriefingData;

void DrawObjectiveInfo(const Objective *o, const struct vec2i pos);

GameLoopData *ScreenCampaignIntro(CampaignSetting *c, const GameMode gameMode, const CampaignEntry *entry);
GameLoopData *ScreenMissionBriefing(
	CampaignSetting *c, const struct MissionOptions *m);
// Display a summary page at the end of a mission
// Returns true if the game is to continue
GameLoopData *ScreenMissionSummary(
	const Campaign *c, struct MissionOptions *m, const bool completed);

void MissionBriefingTitle(MissionBriefingData *mData,
    const struct MissionOptions *m, const int y);
void MissionBriefingDescription(MissionBriefingData *mData,
    const struct MissionOptions *m, 
    const int w, const int h, const int y);

void MissionBriefingDrawData(const MissionBriefingData *mData);
