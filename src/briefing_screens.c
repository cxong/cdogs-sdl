/*
	Copyright (c) 2013-2020 Cong Xu
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
#include "briefing_screens.h"

#include <cdogs/draw/draw.h>
#include <cdogs/draw/draw_actor.h>
#include <cdogs/events.h>
#include <cdogs/files.h>
#include <cdogs/font.h>
#include <cdogs/grafx_bg.h>
#include <cdogs/music.h>
#include <cdogs/objective.h>

#include "animated_counter.h"
#include "autosave.h"
#include "hiscores.h"
#include "menu_utils.h"
#include "password.h"
#include "prep.h"
#include "prep_equip.h"
#include "screens_end.h"

static void DrawObjectiveInfo(const Objective *o, const struct vec2i pos);

typedef struct
{
	EventWaitResult waitResult;
	const CampaignSetting *c;
} ScreenCampaignIntroData;
static void CampaignIntroTerminate(GameLoopData *data);
static void CampaignIntroOnEnter(GameLoopData *data);
static void CampaignIntroOnExit(GameLoopData *data);
static void CampaignIntroInput(GameLoopData *data);
static GameLoopResult CampaignIntroUpdate(GameLoopData *data, LoopRunner *l);
static void CampaignIntroDraw(GameLoopData *data);
GameLoopData *ScreenCampaignIntro(CampaignSetting *c)
{
	ScreenCampaignIntroData *data;
	CMALLOC(data, sizeof *data);
	data->c = c;
	return GameLoopDataNew(
		data, CampaignIntroTerminate, CampaignIntroOnEnter,
		CampaignIntroOnExit, CampaignIntroInput, CampaignIntroUpdate,
		CampaignIntroDraw);
}
static void CampaignIntroTerminate(GameLoopData *data)
{
	ScreenCampaignIntroData *mData = data->Data;
	CFREE(mData);
}
static void CampaignIntroOnEnter(GameLoopData *data)
{
	UNUSED(data);
	MusicPlay(&gSoundDevice, MUSIC_BRIEFING, NULL, NULL);
}
static void CampaignIntroOnExit(GameLoopData *data)
{
	const ScreenCampaignIntroData *sData = data->Data;
	if (sData->waitResult != EVENT_WAIT_CANCEL)
	{
		MenuPlaySound(MENU_SOUND_ENTER);
	}
	else
	{
		CampaignUnload(&gCampaign);
	}
}
static void CampaignIntroInput(GameLoopData *data)
{
	ScreenCampaignIntroData *sData = data->Data;
	sData->waitResult = EventWaitForAnyKeyOrButton();
}
static GameLoopResult CampaignIntroUpdate(GameLoopData *data, LoopRunner *l)
{
	const ScreenCampaignIntroData *sData = data->Data;

	if (!IsIntroNeeded(gCampaign.Entry.Mode) ||
		sData->waitResult == EVENT_WAIT_OK)
	{
		// Switch to num players selection
		LoopRunnerChange(
			l, NumPlayersSelection(&gGraphicsDevice, &gEventHandlers));
	}
	else if (sData->waitResult == EVENT_WAIT_CANCEL)
	{
		LoopRunnerPop(l);
	}
	return UPDATE_RESULT_OK;
}
static void CampaignIntroDraw(GameLoopData *data)
{
	const ScreenCampaignIntroData *sData = data->Data;

	BlitClearBuf(&gGraphicsDevice);
	const int w = gGraphicsDevice.cachedConfig.Res.x;
	const int h = gGraphicsDevice.cachedConfig.Res.y;
	const int y = h / 4;

	// Display title + author
	char *buf;
	CMALLOC(buf, strlen(sData->c->Title) + strlen(sData->c->Author) + 16);
	sprintf(buf, "%s by %s", sData->c->Title, sData->c->Author);
	FontOpts opts = FontOptsNew();
	opts.HAlign = ALIGN_CENTER;
	opts.Area = gGraphicsDevice.cachedConfig.Res;
	opts.Pad.y = y - 25;
	FontStrOpt(buf, svec2i_zero(), opts);
	CFREE(buf);

	// Display campaign description
	// allow some slack for newlines
	if (strlen(sData->c->Description) > 0)
	{
		CMALLOC(buf, strlen(sData->c->Description) * 2);
		// Pad about 1/6th of the screen width total (1/12th left and right)
		FontSplitLines(sData->c->Description, buf, w * 5 / 6);
		FontStr(buf, svec2i(w / 12, y));
		CFREE(buf);
	}

	BlitUpdateFromBuf(&gGraphicsDevice, gGraphicsDevice.screen);
}

typedef struct
{
	char *Title;
	FontOpts TitleOpts;
	char Password[32];
	FontOpts PasswordOpts;
	int TypewriterCount;
	char *Description;
	char *TypewriterBuf;
	struct vec2i DescriptionPos;
	struct vec2i ObjectiveDescPos;
	struct vec2i ObjectiveInfoPos;
	int ObjectiveHeight;
	const struct MissionOptions *MissionOptions;
	EventWaitResult waitResult;
} MissionBriefingData;
static void MissionBriefingTerminate(GameLoopData *data);
static void MissionBriefingOnExit(GameLoopData *data);
static void MissionBriefingInput(GameLoopData *data);
static GameLoopResult MissionBriefingUpdate(GameLoopData *data, LoopRunner *l);
static void MissionBriefingDraw(GameLoopData *data);
GameLoopData *ScreenMissionBriefing(const struct MissionOptions *m)
{
	const int w = gGraphicsDevice.cachedConfig.Res.x;
	const int h = gGraphicsDevice.cachedConfig.Res.y;
	const int y = h / 4;
	MissionBriefingData *mData;
	CCALLOC(mData, sizeof *mData);
	mData->waitResult = EVENT_WAIT_CONTINUE;

	// Title
	if (m->missionData->Title)
	{
		CMALLOC(mData->Title, strlen(m->missionData->Title) + 32);
		sprintf(
			mData->Title, "Mission %d: %s", m->index + 1,
			m->missionData->Title);
		mData->TitleOpts = FontOptsNew();
		mData->TitleOpts.HAlign = ALIGN_CENTER;
		mData->TitleOpts.Area = gGraphicsDevice.cachedConfig.Res;
		mData->TitleOpts.Pad.y = y - 25;
	}

	// Password
	if (m->index > 0)
	{
		sprintf(
			mData->Password, "Password: %s", gAutosave.LastMission.Password);
		mData->PasswordOpts = FontOptsNew();
		mData->PasswordOpts.HAlign = ALIGN_CENTER;
		mData->PasswordOpts.Area = gGraphicsDevice.cachedConfig.Res;
		mData->PasswordOpts.Pad.y = y - 15;
	}

	// Description
	if (m->missionData->Description)
	{
		// Split the description, and prepare it for typewriter effect
		// allow some slack for newlines
		CMALLOC(
			mData->Description, strlen(m->missionData->Description) * 2 + 1);
		CCALLOC(
			mData->TypewriterBuf, strlen(m->missionData->Description) * 2 + 1);
		// Pad about 1/6th of the screen width total (1/12th left and right)
		FontSplitLines(
			m->missionData->Description, mData->Description, w * 5 / 6);
		mData->DescriptionPos = svec2i(w / 12, y);

		// Objectives
		mData->ObjectiveDescPos =
			svec2i(w / 6, y + FontStrH(mData->Description) + h / 10);
		mData->ObjectiveInfoPos =
			svec2i(w - (w / 6), mData->ObjectiveDescPos.y + FontH());
		mData->ObjectiveHeight = h / 12;
	}
	mData->MissionOptions = m;

	return GameLoopDataNew(
		mData, MissionBriefingTerminate, NULL, MissionBriefingOnExit,
		MissionBriefingInput, MissionBriefingUpdate, MissionBriefingDraw);
}
static void MissionBriefingTerminate(GameLoopData *data)
{
	MissionBriefingData *mData = data->Data;

	CFREE(mData->Title);
	CFREE(mData->Description);
	CFREE(mData->TypewriterBuf);
	CFREE(mData);
}
static void MissionBriefingOnExit(GameLoopData *data)
{
	const MissionBriefingData *mData = data->Data;

	if (mData->waitResult == EVENT_WAIT_OK)
	{
		MenuPlaySound(MENU_SOUND_ENTER);
	}
	else
	{
		CampaignUnload(&gCampaign);
	}
}
static void MissionBriefingInput(GameLoopData *data)
{
	MissionBriefingData *mData = data->Data;

	int cmds[MAX_LOCAL_PLAYERS];
	memset(cmds, 0, sizeof cmds);
	GetPlayerCmds(&gEventHandlers, &cmds);
	if (mData->Description)
	{
		// Check for player input; if any then skip to the end of the briefing
		for (int i = 0; i < MAX_LOCAL_PLAYERS; i++)
		{
			if (AnyButton(cmds[i]))
			{
				// If the typewriter is still going, skip to end
				if (mData->TypewriterCount <= (int)strlen(mData->Description))
				{
					strcpy(mData->TypewriterBuf, mData->Description);
					mData->TypewriterCount = (int)strlen(mData->Description);
					return;
				}
				// Otherwise, exit out of loop
				mData->waitResult = EVENT_WAIT_OK;
			}
		}
	}
	// Check if anyone pressed escape
	if (EventIsEscape(&gEventHandlers, cmds, GetMenuCmd(&gEventHandlers)))
	{
		mData->waitResult = EVENT_WAIT_CANCEL;
	}
}
static GameLoopResult MissionBriefingUpdate(GameLoopData *data, LoopRunner *l)
{
	MissionBriefingData *mData = data->Data;

	if (!IsMissionBriefingNeeded(gCampaign.Entry.Mode, mData->Description))
	{
		mData->waitResult = EVENT_WAIT_OK;
		goto bail;
	}

	// Check exit conditions from input
	if (mData->waitResult != EVENT_WAIT_CONTINUE)
	{
		goto bail;
	}

	// Update the typewriter effect
	if (mData->TypewriterCount <= (int)strlen(mData->Description))
	{
		mData->TypewriterBuf[mData->TypewriterCount] =
			mData->Description[mData->TypewriterCount];
		mData->TypewriterCount++;
		return UPDATE_RESULT_DRAW;
	}

    // Auto skip if on demo mode
    if (gEventHandlers.DemoQuitTimer > 0)
    {
        mData->waitResult = EVENT_WAIT_OK;
        goto bail;
    }

    return UPDATE_RESULT_OK;

bail:
	if (mData->waitResult == EVENT_WAIT_OK)
	{
		LoopRunnerChange(l, PlayerEquip());
	}
	else
	{
		LoopRunnerPop(l);
	}
	return UPDATE_RESULT_OK;
}
static void MissionBriefingDraw(GameLoopData *data)
{
	const MissionBriefingData *mData = data->Data;

	BlitClearBuf(&gGraphicsDevice);

	// Mission title
	FontStrOpt(mData->Title, svec2i_zero(), mData->TitleOpts);
	// Display password
	FontStrOpt(mData->Password, svec2i_zero(), mData->PasswordOpts);
	// Display description with typewriter effect
	FontStr(mData->TypewriterBuf, mData->DescriptionPos);
	// Display objectives
	CA_FOREACH(
		const Objective, o, mData->MissionOptions->missionData->Objectives)
	// Do not brief optional objectives
	if (o->Required == 0)
	{
		continue;
	}
	struct vec2i offset = svec2i(0, _ca_index * mData->ObjectiveHeight);
	FontStr(o->Description, svec2i_add(mData->ObjectiveDescPos, offset));
	// Draw the icons slightly offset so that tall icons don't overlap each
	// other
	offset.x = -16 * (_ca_index & 1);
	DrawObjectiveInfo(o, svec2i_add(mData->ObjectiveInfoPos, offset));
	CA_FOREACH_END()

	BlitUpdateFromBuf(&gGraphicsDevice, gGraphicsDevice.screen);
}

#define PERFECT_BONUS 500

typedef struct
{
	const PlayerData *Pd;
	AnimatedCounter Score;
	AnimatedCounter Total;
	AnimatedCounter HealthResurrection;
	AnimatedCounter ButcherNinjaFriendly;
} PlayerSummaryDrawData;
typedef struct
{
	MenuSystem ms;
	const Campaign *c;
	struct MissionOptions *m;
	bool completed;
	AnimatedCounter AccessBonus;
	AnimatedCounter TimeBonus;
	PlayerSummaryDrawData pDatas[MAX_LOCAL_PLAYERS];
} MissionSummaryData;
static void MissionSummaryTerminate(GameLoopData *data);
static void MissionSummaryOnEnter(GameLoopData *data);
static GameLoopResult MissionSummaryUpdate(GameLoopData *data, LoopRunner *l);
static void MissionSummaryDraw(GameLoopData *data);
static void MissionSummaryMenuDraw(
	const menu_t *menu, GraphicsDevice *g, const struct vec2i p,
	const struct vec2i size, const void *data);
GameLoopData *ScreenMissionSummary(
	const Campaign *c, struct MissionOptions *m, const bool completed)
{
	MissionSummaryData *mData;
	CCALLOC(mData, sizeof *mData);

	const int h = FontH() * 10;
	MenuSystemInit(
		&mData->ms, &gEventHandlers, &gGraphicsDevice,
		svec2i(0, gGraphicsDevice.cachedConfig.Res.y - h),
		svec2i(gGraphicsDevice.cachedConfig.Res.x, h));
	mData->ms.current = mData->ms.root =
		MenuCreateNormal("", "", MENU_TYPE_NORMAL, 0);
	// Use return code 0 for whether to continue the game
	if (completed)
	{
		MenuAddSubmenu(mData->ms.root, MenuCreateReturn("Continue", 0));
	}
	else
	{
		MenuAddSubmenu(mData->ms.root, MenuCreateReturn("Replay mission", 0));
		MenuAddSubmenu(mData->ms.root, MenuCreateReturn("Back to menu", 1));
	}
	mData->ms.allowAborts = true;
	MenuAddExitType(&mData->ms, MENU_TYPE_RETURN);
	MenuSystemAddCustomDisplay(&mData->ms, MissionSummaryMenuDraw, mData);

	mData->c = c;
	mData->m = m;
	mData->completed = completed;

	return GameLoopDataNew(
		mData, MissionSummaryTerminate, MissionSummaryOnEnter, NULL, NULL,
		MissionSummaryUpdate, MissionSummaryDraw);
}
static void MissionSummaryTerminate(GameLoopData *data)
{
	MissionSummaryData *mData = data->Data;

	MenuSystemTerminate(&mData->ms);
	AnimatedCounterTerminate(&mData->AccessBonus);
	AnimatedCounterTerminate(&mData->TimeBonus);
	for (int i = 0; i < MAX_LOCAL_PLAYERS; i++)
	{
		AnimatedCounterTerminate(&mData->pDatas[i].Score);
		AnimatedCounterTerminate(&mData->pDatas[i].Total);
		AnimatedCounterTerminate(&mData->pDatas[i].HealthResurrection);
		AnimatedCounterTerminate(&mData->pDatas[i].ButcherNinjaFriendly);
	}
	CFREE(mData);
}
static bool AreAnySurvived(void);
static int GetAccessBonus(const struct MissionOptions *m);
static int GetTimeBonus(const struct MissionOptions *m, int *secondsOut);
static void ApplyBonuses(PlayerData *p, const int bonus);
static int GetHealthBonus(const PlayerData *p);
static int GetResurrectionFee(const PlayerData *p);
static int GetButcherPenalty(const PlayerData *p);
static int GetNinjaBonus(const PlayerData *p);
static int GetFriendlyBonus(const PlayerData *p);
static void MissionSummaryOnEnter(GameLoopData *data)
{
	MissionSummaryData *mData = data->Data;

	MusicPlay(&gSoundDevice, MUSIC_BRIEFING, NULL, NULL);

	if (mData->completed && IsPasswordAllowed(mData->c->Entry.Mode))
	{
		// Save password
		MissionSave ms;
		MissionSaveInit(&ms);
		ms.Campaign = mData->c->Entry;
		// Don't make password for next level if there is none
		int passwordIndex = mData->m->index + 1;
		if (passwordIndex == mData->c->Entry.NumMissions)
		{
			passwordIndex--;
		}
		strcpy(ms.Password, MakePassword(passwordIndex, 0));
		ms.MissionsCompleted = mData->m->index + 1;
		AutosaveAddMission(&gAutosave, &ms);
		AutosaveSave(&gAutosave, GetConfigFilePath(AUTOSAVE_FILE));
	}

	// Calculate bonus scores
	// Bonuses only apply if at least one player has lived
	const int accessBonus = GetAccessBonus(mData->m);
	if (AreAnySurvived())
	{
		int bonus = 0;
		// Objective bonuses
		CA_FOREACH(const Objective, o, mData->m->missionData->Objectives)
		if (ObjectiveIsPerfect(o))
		{
			bonus += PERFECT_BONUS;
		}
		CA_FOREACH_END()
		bonus += accessBonus;
		bonus += GetTimeBonus(mData->m, NULL);

		CA_FOREACH(PlayerData, p, gPlayerDatas)
		ApplyBonuses(p, bonus);
		CA_FOREACH_END()
	}

	// Init mission bonuses
	if (AreAnySurvived())
	{
		if (accessBonus > 0)
		{
			mData->AccessBonus =
				AnimatedCounterNew("Access bonus: ", accessBonus);
		}
		int seconds;
		const int timeBonus = GetTimeBonus(mData->m, &seconds);
		char buf[256];
		sprintf(buf, "Time bonus: %d secs x 25 = ", seconds);
		mData->TimeBonus = AnimatedCounterNew(buf, timeBonus);
	}

	// Init per-player summaries
	int idx = 0;
	CA_FOREACH(PlayerData, pd, gPlayerDatas)
	if (!pd->IsLocal)
	{
		continue;
	}
	mData->pDatas[idx].Pd = pd;
	mData->pDatas[idx].Score = AnimatedCounterNew("Score: ", pd->Stats.Score);
	mData->pDatas[idx].Total = AnimatedCounterNew("Total: ", pd->Totals.Score);
	if (pd->survived)
	{
		const int healthBonus = GetHealthBonus(pd);
		if (healthBonus != 0)
		{
			mData->pDatas[idx].HealthResurrection =
				AnimatedCounterNew("Health bonus: ", healthBonus);
		}
		const int resurrectionFee = GetResurrectionFee(pd);
		if (resurrectionFee != 0)
		{
			mData->pDatas[idx].HealthResurrection =
				AnimatedCounterNew("Resurrection fee: ", resurrectionFee);
		}

		const int butcherPenalty = GetButcherPenalty(pd);
		if (butcherPenalty != 0)
		{
			mData->pDatas[idx].ButcherNinjaFriendly =
				AnimatedCounterNew("Butcher penalty: ", butcherPenalty);
		}
		const int ninjaBonus = GetNinjaBonus(pd);
		if (ninjaBonus != 0)
		{
			mData->pDatas[idx].ButcherNinjaFriendly =
				AnimatedCounterNew("Ninja bonus: ", ninjaBonus);
		}
		const int friendlyBonus = GetFriendlyBonus(pd);
		if (friendlyBonus != 0)
		{
			mData->pDatas[idx].ButcherNinjaFriendly =
				AnimatedCounterNew("Friendly bonus: ", friendlyBonus);
		}
	}
	idx++;
	CA_FOREACH_END()
}
static GameLoopResult MissionSummaryUpdate(GameLoopData *data, LoopRunner *l)
{
	MissionSummaryData *mData = data->Data;

    GameLoopResult result = MenuUpdate(&mData->ms);
	if (result == UPDATE_RESULT_DRAW)
	{
        bool done = true;
        done = AnimatedCounterUpdate(&mData->AccessBonus, 1) && done;
        done = AnimatedCounterUpdate(&mData->TimeBonus, 1) && done;
        for (int i = 0; i < MAX_LOCAL_PLAYERS; i++)
        {
            done = AnimatedCounterUpdate(&mData->pDatas[i].Score, 1) && done;
            done = AnimatedCounterUpdate(&mData->pDatas[i].Total, 1) && done;
            done = AnimatedCounterUpdate(&mData->pDatas[i].HealthResurrection, 1) && done;
            done = AnimatedCounterUpdate(&mData->pDatas[i].ButcherNinjaFriendly, 1) && done;
        }

        // Skip after animations are done if in demo mode
        if (done && gEventHandlers.DemoQuitTimer > 0)
        {
            result = UPDATE_RESULT_OK;
        }
    }

    if (result == UPDATE_RESULT_OK)
    {
		gCampaign.IsComplete =
			mData->completed &&
			mData->m->NextMission == (int)gCampaign.Setting.Missions.size;
		if (gCampaign.IsComplete)
		{
			LoopRunnerChange(l, ScreenVictory(&gCampaign));
		}
		else if (!mData->completed)
		{
			// Check if we want to return to menu or replay mission
			gCampaign.IsQuit = mData->ms.current->u.returnCode == 1;
			LoopRunnerPop(l);
		}
		else
		{
			LoopRunnerChange(
				l, HighScoresScreen(&gCampaign, &gGraphicsDevice));
		}
	}
	return result;
}
static void MissionSummaryDraw(GameLoopData *data)
{
	const MissionSummaryData *mData = data->Data;

	MenuDraw(&mData->ms);
}
static bool AreAnySurvived(void)
{
	CA_FOREACH(const PlayerData, p, gPlayerDatas)
	if (p->survived)
	{
		return true;
	}
	CA_FOREACH_END()
	return false;
}
static int GetAccessBonus(const struct MissionOptions *m)
{
	return KeycardCount(m->KeyFlags) * 50;
}
static int GetTimeBonus(const struct MissionOptions *m, int *secondsOut)
{
	int seconds = 60 + (int)m->missionData->Objectives.size * 30 -
				  m->time / FPS_FRAMELIMIT;
	if (seconds < 0)
	{
		seconds = 0;
	}
	if (secondsOut)
	{
		*secondsOut = seconds;
	}
	return seconds * 25;
}
static void ApplyBonuses(PlayerData *p, const int bonus)
{
	// Apply bonuses to surviving players only
	if (!p->survived)
	{
		return;
	}

	p->Totals.Score += bonus;

	// Other per-player bonuses
	p->Totals.Score += GetHealthBonus(p);
	p->Totals.Score += GetResurrectionFee(p);
	p->Totals.Score += GetButcherPenalty(p);
	p->Totals.Score += GetNinjaBonus(p);
	p->Totals.Score += GetFriendlyBonus(p);
}
static int GetHealthBonus(const PlayerData *p)
{
	const int maxHealth = ModeMaxHealth(gCampaign.Entry.Mode);
	return p->hp > maxHealth - 50 ? (p->hp + 50 - maxHealth) * 10 : 0;
}
static int GetResurrectionFee(const PlayerData *p)
{
	return p->hp <= 0 ? -500 : 0;
}
static int GetButcherPenalty(const PlayerData *p)
{
	if (p->Stats.Friendlies > 5 && p->Stats.Friendlies > p->Stats.Kills / 2)
	{
		return -100 * p->Stats.Friendlies;
	}
	return 0;
}
// TODO: amend ninja bonus to check if no shots fired
static int GetNinjaBonus(const PlayerData *p)
{
	if (PlayerGetNumWeapons(p) == 1)
	{
		const WeaponClass *wc = NULL;
		for (int i = 0; i < MAX_GUNS; i++)
		{
			if (p->guns[i] != NULL)
			{
				wc = p->guns[i];
				break;
			}
		}
		if (wc != NULL && !wc->CanShoot && p->Stats.Friendlies == 0 &&
			p->Stats.Kills > 5)
		{
			return 50 * p->Stats.Kills;
		}
	}
	return 0;
}
static int GetFriendlyBonus(const PlayerData *p)
{
	return (p->Stats.Kills == 0 && p->Stats.Friendlies == 0) ? 500 : 0;
}
static void DrawPlayerSummary(
	const struct vec2i pos, const struct vec2i size,
	const PlayerSummaryDrawData *data);
static void MissionSummaryMenuDraw(
	const menu_t *menu, GraphicsDevice *g, const struct vec2i p,
	const struct vec2i size, const void *data)
{
	UNUSED(menu);
	UNUSED(p);
	UNUSED(size);
	const MissionSummaryData *mData = data;

	const int w = gGraphicsDevice.cachedConfig.Res.x;
	const int h = gGraphicsDevice.cachedConfig.Res.y;

	// Display password
	if (strlen(gAutosave.LastMission.Password) > 0)
	{
		char s[64];
		sprintf(s, "Last password: %s", gAutosave.LastMission.Password);
		FontOpts opts = FontOptsNew();
		opts.HAlign = ALIGN_CENTER;
		opts.VAlign = ALIGN_END;
		opts.Area = g->cachedConfig.Res;
		opts.Pad.y = opts.Area.y / 12;
		FontStrOpt(s, svec2i_zero(), opts);
	}

	// Display objectives and bonuses
	struct vec2i pos = svec2i(w / 6, h / 2 + h / 10);
	int idx = 1;
	CA_FOREACH(const Objective, o, mData->m->missionData->Objectives)
	// Do not mention optional objectives with none completed
	if (o->done == 0 && !ObjectiveIsRequired(o))
	{
		continue;
	}

	// Objective icon
	DrawObjectiveInfo(o, svec2i_add(pos, svec2i(-26, FontH())));

	// Objective completion text
	char s[100];
	sprintf(
		s, "Objective %d: %d of %d, %d required", idx, o->done, o->Count,
		o->Required);
	FontOpts opts = FontOptsNew();
	opts.Area = gGraphicsDevice.cachedConfig.Res;
	opts.Pad = pos;
	if (!ObjectiveIsRequired(o))
	{
		// Show optional objectives in purple
		opts.Mask = colorPurple;
	}
	FontStrOpt(s, svec2i_zero(), opts);

	// Objective status text
	opts = FontOptsNew();
	opts.HAlign = ALIGN_END;
	opts.Area = gGraphicsDevice.cachedConfig.Res;
	opts.Pad = pos;
	if (!ObjectiveIsComplete(o))
	{
		opts.Mask = colorRed;
		FontStrOpt("Failed", svec2i_zero(), opts);
	}
	else if (ObjectiveIsPerfect(o) && AreAnySurvived())
	{
		opts.Mask = colorGreen;
		char buf[16];
		sprintf(buf, "Perfect: %d", PERFECT_BONUS);
		FontStrOpt(buf, svec2i_zero(), opts);
	}
	else if (ObjectiveIsRequired(o))
	{
		FontStrOpt("Done", svec2i_zero(), opts);
	}
	else
	{
		FontStrOpt("Bonus!", svec2i_zero(), opts);
	}

	pos.y += 15;
	idx++;
	CA_FOREACH_END()

	// Draw other bonuses
	if (mData->AccessBonus.prefix)
	{
		AnimatedCounterDraw(&mData->AccessBonus, pos);
		pos.y += FontH() + 1;
	}
	if (mData->TimeBonus.prefix)
	{
		AnimatedCounterDraw(&mData->TimeBonus, pos);
	}

	// Draw per-player summaries
	struct vec2i playerSize;
	switch (GetNumPlayers(PLAYER_ANY, false, true))
	{
	case 1:
		playerSize = svec2i(w, h / 2);
		DrawPlayerSummary(svec2i_zero(), playerSize, &mData->pDatas[0]);
		break;
	case 2:
		// side by side
		playerSize = svec2i(w / 2, h / 2);
		DrawPlayerSummary(svec2i_zero(), playerSize, &mData->pDatas[0]);
		DrawPlayerSummary(svec2i(w / 2, 0), playerSize, &mData->pDatas[1]);
		break;
	case 3: // fallthrough
	case 4:
		// 2x2
		playerSize = svec2i(w / 2, h / 4);
		DrawPlayerSummary(svec2i_zero(), playerSize, &mData->pDatas[0]);
		DrawPlayerSummary(svec2i(w / 2, 0), playerSize, &mData->pDatas[1]);
		DrawPlayerSummary(svec2i(0, h / 4), playerSize, &mData->pDatas[2]);
		if (idx == 4)
		{
			DrawPlayerSummary(
				svec2i(w / 2, h / 4), playerSize, &mData->pDatas[3]);
		}
		break;
	default:
		CASSERT(false, "not implemented");
		break;
	}
}
// Display compact player summary, with player on left half and score summaries
// on right half
static void DrawPlayerSummary(
	const struct vec2i pos, const struct vec2i size,
	const PlayerSummaryDrawData *data)
{
	char s[50];
	const int totalTextHeight = FontH() * 7;
	// display text on right half
	struct vec2i textPos =
		svec2i(pos.x + size.x / 2, CENTER_Y(pos, size, totalTextHeight));

	DisplayCharacterAndName(
		svec2i_add(pos, svec2i(size.x / 4, size.y / 2)), &data->Pd->Char,
		DIRECTION_DOWN, data->Pd->name, colorWhite);

	if (data->Pd->survived)
	{
		FontStr("Completed mission", textPos);
	}
	else
	{
		FontStrMask("Failed mission", textPos, colorRed);
	}

	textPos.y += 2 * FontH();
	AnimatedCounterDraw(&data->Score, textPos);
	textPos.y += FontH();
	AnimatedCounterDraw(&data->Total, textPos);
	textPos.y += FontH();
	sprintf(
		s, "Missions: %d", data->Pd->missions + (data->Pd->survived ? 1 : 0));
	FontStr(s, textPos);
	textPos.y += FontH();

	if (data->HealthResurrection.prefix)
	{
		AnimatedCounterDraw(&data->HealthResurrection, textPos);
		textPos.y += FontH();
	}
	if (data->ButcherNinjaFriendly.prefix)
	{
		AnimatedCounterDraw(&data->ButcherNinjaFriendly, textPos);
	}
}

static void DrawObjectiveInfo(const Objective *o, const struct vec2i pos)
{
	const CharacterStore *store = &gCampaign.Setting.characters;

	switch (o->Type)
	{
	case OBJECTIVE_KILL: {
		const Character *cd = CArrayGet(
			&store->OtherChars, CharacterStoreGetSpecialId(store, 0));
		DrawHead(gGraphicsDevice.gameWindow.renderer, cd, DIRECTION_DOWN, pos);
	}
	break;
	case OBJECTIVE_RESCUE: {
		const Character *cd = CArrayGet(
			&store->OtherChars, CharacterStoreGetPrisonerId(store, 0));
		DrawHead(gGraphicsDevice.gameWindow.renderer, cd, DIRECTION_DOWN, pos);
	}
	break;
	case OBJECTIVE_COLLECT:
		CPicDraw(
			&gGraphicsDevice, &o->u.Pickup->Pic,
			svec2i_subtract(pos, svec2i(-4, -4)), NULL);
		break;
	case OBJECTIVE_DESTROY: {
		struct vec2i picOffset;
		const Pic *p = MapObjectGetPic(o->u.MapObject, &picOffset);
		PicRender(
			p, gGraphicsDevice.gameWindow.renderer, svec2i_add(pos, picOffset),
			colorWhite, 0, svec2_one(), SDL_FLIP_NONE, Rect2iZero());
	}
	break;
	case OBJECTIVE_INVESTIGATE:
		// Don't draw
		return;
	default:
		CASSERT(false, "Unknown objective type");
		return;
	}
}
