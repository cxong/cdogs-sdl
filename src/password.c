/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.
	Copyright (c) 2013-2014, 2017, 2021 Cong Xu
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
} LevelSelectionData;
static void LevelSelectionTerminate(GameLoopData *data);
static void LevelSelectionOnEnter(GameLoopData *data);
static GameLoopResult LevelSelectionUpdate(GameLoopData *data, LoopRunner *l);
static void LevelSelectionDraw(GameLoopData *data);
static void MenuCreateStart(MenuSystem *ms, const CampaignSave *save);
GameLoopData *LevelSelection(GraphicsDevice *graphics)
{
	LevelSelectionData *data;
	CCALLOC(data, sizeof *data);
	data->save = AutosaveGetCampaign(&gAutosave, gCampaign.Entry.Path);
	MenuSystemInit(
		&data->ms, &gEventHandlers, graphics, svec2i_zero(),
		graphics->cachedConfig.Res);
	MenuCreateStart(&data->ms, data->save);
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
static void MenuCreateStart(MenuSystem *ms, const CampaignSave *save)
{
	ms->root = ms->current = MenuCreateNormal("", "", MENU_TYPE_NORMAL, 0);

	menu_t *menuContinue = MenuCreateReturn("Continue", RETURN_CODE_CONTINUE);
	// Note: mission can be -1
	menuContinue->isDisabled =
		save == NULL || save->NextMission <= 0 ||
		save->NextMission == (int)gCampaign.Setting.Missions.size;
	MenuAddSubmenu(ms->root, menuContinue);

	// Create level select menus
	menu_t *levelSelect = MenuCreateNormal(
		"Level select...", "Select Level", MENU_TYPE_NORMAL, 0);
	levelSelect->u.normal.maxItems = 20;
	if (save)
	{
		CArray levels;
		CArrayInitFillZero(
			&levels, sizeof(bool), gCampaign.Setting.Missions.size);
		CA_FOREACH(const int, missionIndex, save->MissionsCompleted)
		if (*missionIndex >= (int)gCampaign.Setting.Missions.size)
		{
			continue;
		}
		MenuCreateLevelSelect(levelSelect, &gCampaign, *missionIndex);
		CArraySet(&levels, *missionIndex, &gTrue);
		CA_FOREACH_END()
		if (save->NextMission < (int)gCampaign.Setting.Missions.size &&
			!*(bool *)CArrayGet(&levels, save->NextMission))
		{
			MenuCreateLevelSelect(levelSelect, &gCampaign, save->NextMission);
		}
		CArrayTerminate(&levels);
	}
	levelSelect->isDisabled =
		save == NULL || save->MissionsCompleted.size == 0;
	MenuAddSubmenu(ms->root, levelSelect);

	MenuAddSubmenu(
		ms->root, MenuCreateReturn("Start campaign", RETURN_CODE_START));

	MenuAddExitType(ms, MENU_TYPE_RETURN);
}
static void LevelSelectionTerminate(GameLoopData *data)
{
	LevelSelectionData *pData = data->Data;

	MenuSystemTerminate(&pData->ms);
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
				LoopRunnerChange(l, GameOptions(gCampaign.Entry.Mode));
				break;
			case RETURN_CODE_START:
				LoopRunnerChange(l, GameOptions(gCampaign.Entry.Mode));
				break;
			default:
				// Return code represents the mission to start on
				CASSERT(
					returnCode >= 0, "Invalid return code for password menu");
				gCampaign.MissionIndex = returnCode;
				LoopRunnerChange(l, GameOptions(gCampaign.Entry.Mode));
				break;
			}

			// Load autosaved player guns/ammo
			const Mission *m = CampaignGetCurrentMission(&gCampaign);
			if (pData->save != NULL && m->WeaponPersist &&
				gCampaign.MissionIndex > 0)
			{
				for (int i = 0, idx = 0; i < (int)gPlayerDatas.size &&
										 idx < (int)pData->save->Players.size;
					 i++, idx++)
				{
					PlayerData *p = CArrayGet(&gPlayerDatas, i);
					if (!p->IsLocal)
					{
						idx--;
						continue;
					}
					const PlayerSave *ps =
						CArrayGet(&pData->save->Players, idx);
					for (int j = 0; j < MAX_WEAPONS; j++)
					{
						if (ps->Guns[j] != NULL)
						{
							p->guns[j] = StrWeaponClass(ps->Guns[j]);
						}
					}
					CArrayCopy(&p->ammo, &ps->ammo);
				}
			}
		}
	}
	return result;
}
static void LevelSelectionDraw(GameLoopData *data)
{
	const LevelSelectionData *pData = data->Data;

	MenuDraw(&pData->ms);
}
