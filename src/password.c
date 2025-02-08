/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.
	Copyright (c) 2013-2014, 2017, 2021-2022, 2024-2025 Cong Xu
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
#include "password.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cdogs/actors.h>
#include <cdogs/blit.h>
#include <cdogs/config.h>
#include <cdogs/defs.h>
#include <cdogs/font.h>
#include <cdogs/gamedata.h>
#include <cdogs/grafx.h>
#include <cdogs/grafx_bg.h>
#include <cdogs/joystick.h>
#include <cdogs/keyboard.h>
#include <cdogs/sounds.h>

#include "autosave.h"
#include "game_loop.h"
#include "hiscores.h"
#include "menu.h"
#include "prep.h"

typedef enum
{
	RETURN_CODE_CONTINUE = -1,
	RETURN_CODE_START = -2,
} ReturnCode;

typedef struct
{
	MenuSystem ms;
	const CampaignSave *save;
	HighScoresData hData;
} LevelSelectionData;
static void LevelSelectionTerminate(GameLoopData *data);
static void LevelSelectionOnEnter(GameLoopData *data);
static GameLoopResult LevelSelectionUpdate(GameLoopData *data, LoopRunner *l);
static void LevelSelectionDraw(GameLoopData *data);
static void MenuCreateStart(LevelSelectionData *data);
GameLoopData *LevelSelection(GraphicsDevice *graphics)
{
	LevelSelectionData *data;
	CCALLOC(data, sizeof *data);
	data->save = AutosaveGetCampaign(&gAutosave, gCampaign.Entry.Path);
	MenuSystemInit(
		&data->ms, &gEventHandlers, graphics, svec2i_zero(),
		graphics->cachedConfig.Res);
	MenuCreateStart(data);
	return GameLoopDataNew(
		data, LevelSelectionTerminate, LevelSelectionOnEnter, NULL, NULL,
		LevelSelectionUpdate, LevelSelectionDraw);
}
static void MenuCreateLevelSelect(
	menu_t *levelSelect, const Campaign *co, const int i)
{
	const Mission *m = CArrayGet(&co->Setting.Missions, i);
	char buf[CDOGS_FILENAME_MAX];
	sprintf(buf, "%d: %s", i + 1, m->Title);
	menu_t *l = MenuCreateReturn(buf, i);
	MenuAddSubmenu(levelSelect, l);
}
static void HighScoresDrawFunc(
	const menu_t *menu, GraphicsDevice *g, const struct vec2i pos,
	const struct vec2i size, const void *data);
static void MenuCreateStart(LevelSelectionData *data)
{
	MenuSystem *ms = &data->ms;
	ms->root = ms->current = MenuCreateNormal("", "", MENU_TYPE_NORMAL, 0);

	menu_t *menuContinue = MenuCreateReturn("Continue", RETURN_CODE_CONTINUE);
	// Note: mission can be -1
	menuContinue->isDisabled =
		data->save == NULL || data->save->NextMission <= 0 ||
		data->save->NextMission == (int)gCampaign.Setting.Missions.size;
	MenuAddSubmenu(ms->root, menuContinue);

	// Create level select menus
	menu_t *levelSelect = MenuCreateNormal(
		"Level select...", "Select Level", MENU_TYPE_NORMAL, 0);
	levelSelect->u.normal.maxItems = 20;
	if (data->save)
	{
		CArray levels;
		CArrayInitFillZero(
			&levels, sizeof(bool), gCampaign.Setting.Missions.size);
		CA_FOREACH(const int, missionIndex, data->save->MissionsCompleted)
		if (*missionIndex >= (int)gCampaign.Setting.Missions.size)
		{
			continue;
		}
		MenuCreateLevelSelect(levelSelect, &gCampaign, *missionIndex);
		CArraySet(&levels, *missionIndex, &gTrue);
		CA_FOREACH_END()
		if (data->save->NextMission < (int)gCampaign.Setting.Missions.size &&
			!*(bool *)CArrayGet(&levels, data->save->NextMission))
		{
			MenuCreateLevelSelect(
				levelSelect, &gCampaign, data->save->NextMission);
		}
		CArrayTerminate(&levels);
	}
	levelSelect->isDisabled = data->save == NULL ||
							  data->save->MissionsCompleted.size == 0 ||
							  gCampaign.Setting.Missions.size == 1;
	MenuAddSubmenu(ms->root, levelSelect);

	MenuAddSubmenu(
		ms->root, MenuCreateReturn("Start campaign", RETURN_CODE_START));

	data->hData = HighScoresDataLoad(&gCampaign, &gGraphicsDevice);
	menu_t *highScoresMenu = MenuCreateCustom(
		"High Scores", HighScoresDrawFunc, NULL, &data->hData);
	highScoresMenu->isDisabled = data->hData.scores.size == 0;
	MenuAddSubmenu(ms->root, highScoresMenu);

	MenuAddExitType(ms, MENU_TYPE_RETURN);
}
static void LevelSelectionTerminate(GameLoopData *data)
{
	LevelSelectionData *pData = data->Data;

	MenuSystemTerminate(&pData->ms);
	HighScoresDataTerminate(&pData->hData);
	CFREE(data->Data);
}
static void LevelSelectionOnEnter(GameLoopData *data)
{
	LevelSelectionData *pData = data->Data;

	// TODO: re-detect mission saves on enter
	MenuReset(&pData->ms);
	gCampaign.MissionIndex = 0;
}
static GameLoopResult LevelSelectionUpdate(GameLoopData *data, LoopRunner *l)
{
	LevelSelectionData *pData = data->Data;

	const GameLoopResult result = MenuUpdate(&pData->ms);
	if (result == UPDATE_RESULT_OK)
	{
		if (pData->ms.hasAbort)
		{
			LoopRunnerPop(l);
		}
		else
		{
			// Check valid password
			const int returnCode = pData->ms.current->u.returnCode;
			switch (returnCode)
			{
			case RETURN_CODE_CONTINUE:
				gCampaign.MissionIndex = pData->save->NextMission;
				break;
			case RETURN_CODE_START:
				break;
			default:
				// Return code represents the mission to start on
				CASSERT(
					returnCode >= 0, "Invalid return code for password menu");
				gCampaign.MissionIndex = returnCode;
				break;
			}

			// Load autosaved player data
			if (pData->save != NULL && gCampaign.MissionIndex > 0)
			{
				const Mission *m = CampaignGetCurrentMission(&gCampaign);
				PlayerSavesApply(&pData->save->Players, m->WeaponPersist);
			}
			// IMPORTANT: change state after applying player save to avoid
			// reading freed data
			LoopRunnerChange(l, GameOptions(gCampaign.Entry.Mode));
		}
	}
	return result;
}
static void LevelSelectionDraw(GameLoopData *data)
{
	const LevelSelectionData *pData = data->Data;

	MenuDraw(&pData->ms);
}

static void HighScoresDrawFunc(
	const menu_t *menu, GraphicsDevice *g, const struct vec2i pos,
	const struct vec2i size, const void *data)
{
	UNUSED(menu);
	UNUSED(g);
	UNUSED(pos);
	UNUSED(size);
	const HighScoresData *hData = data;
	HighScoresDraw(hData);
}
