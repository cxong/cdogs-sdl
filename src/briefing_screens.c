/*
    Copyright (c) 2013-2016, Cong Xu
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

#include <cdogs/draw.h>
#include <cdogs/events.h>
#include <cdogs/files.h>
#include <cdogs/font.h>
#include <cdogs/game_loop.h>
#include <cdogs/grafx_bg.h>
#include <cdogs/objective.h>

#include "autosave.h"
#include "menu_utils.h"
#include "password.h"


static void DrawObjectiveInfo(
	const struct MissionOptions *mo, const int idx, const Vec2i pos);

static void CampaignIntroDraw(void *data);
bool ScreenCampaignIntro(CampaignSetting *c)
{
	GameLoopWaitForAnyKeyOrButtonData wData;
	GameLoopData gData = GameLoopDataNew(
		&wData, GameLoopWaitForAnyKeyOrButtonFunc,
		c, CampaignIntroDraw);
	GameLoop(&gData);
	if (wData.IsOK)
	{
		SoundPlay(&gSoundDevice, StrSound("mg"));
	}
	return wData.IsOK;
}
static void CampaignIntroDraw(void *data)
{
	// This will only draw once
	const CampaignSetting *c = data;

	GraphicsBlitBkg(&gGraphicsDevice);
	const int w = gGraphicsDevice.cachedConfig.Res.x;
	const int h = gGraphicsDevice.cachedConfig.Res.y;
	const int y = h / 4;

	// Display title + author
	char *buf;
	CMALLOC(buf, strlen(c->Title) + strlen(c->Author) + 16);
	sprintf(buf, "%s by %s", c->Title, c->Author);
	FontOpts opts = FontOptsNew();
	opts.HAlign = ALIGN_CENTER;
	opts.Area = gGraphicsDevice.cachedConfig.Res;
	opts.Pad.y = y - 25;
	FontStrOpt(buf, Vec2iZero(), opts);
	CFREE(buf);

	// Display campaign description
	// allow some slack for newlines
	if (strlen(c->Description) > 0)
	{
		CMALLOC(buf, strlen(c->Description) * 2);
		// Pad about 1/6th of the screen width total (1/12th left and right)
		FontSplitLines(c->Description, buf, w * 5 / 6);
		FontStr(buf, Vec2iNew(w / 12, y));
		CFREE(buf);
	}
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
	Vec2i DescriptionPos;
	Vec2i ObjectiveDescPos;
	Vec2i ObjectiveInfoPos;
	int ObjectiveHeight;
	const struct MissionOptions *MissionOptions;
	bool IsOK;
} MissionBriefingData;
static GameLoopResult MissionBriefingUpdate(void *data);
static void MissionBriefingDraw(void *data);
bool ScreenMissionBriefing(const struct MissionOptions *m)
{
	const int w = gGraphicsDevice.cachedConfig.Res.x;
	const int h = gGraphicsDevice.cachedConfig.Res.y;
	const int y = h / 4;
	MissionBriefingData mData;
	memset(&mData, 0, sizeof mData);
	mData.IsOK = true;

	// Title
	CMALLOC(mData.Title, strlen(m->missionData->Title) + 32);
	sprintf(mData.Title, "Mission %d: %s",
		m->index + 1, m->missionData->Title);
	mData.TitleOpts = FontOptsNew();
	mData.TitleOpts.HAlign = ALIGN_CENTER;
	mData.TitleOpts.Area = gGraphicsDevice.cachedConfig.Res;
	mData.TitleOpts.Pad.y = y - 25;

	// Password
	if (m->index > 0)
	{
		sprintf(
			mData.Password, "Password: %s", gAutosave.LastMission.Password);
		mData.PasswordOpts = FontOptsNew();
		mData.PasswordOpts.HAlign = ALIGN_CENTER;
		mData.PasswordOpts.Area = gGraphicsDevice.cachedConfig.Res;
		mData.PasswordOpts.Pad.y = y - 15;
	}

	// Split the description, and prepare it for typewriter effect
	mData.TypewriterCount = 0;
	// allow some slack for newlines
	CMALLOC(mData.Description, strlen(m->missionData->Description) * 2 + 1);
	CCALLOC(mData.TypewriterBuf, strlen(m->missionData->Description) * 2 + 1);
	// Pad about 1/6th of the screen width total (1/12th left and right)
	FontSplitLines(m->missionData->Description, mData.Description, w * 5 / 6);
	mData.DescriptionPos = Vec2iNew(w / 12, y);

	// Objectives
	mData.ObjectiveDescPos =
		Vec2iNew(w / 6, y + FontStrH(mData.Description) + h / 10);
	mData.ObjectiveInfoPos =
		Vec2iNew(w - (w / 6), mData.ObjectiveDescPos.y + FontH());
	mData.ObjectiveHeight = h / 12;
	mData.MissionOptions = m;

	GameLoopData gData = GameLoopDataNew(
		&mData, MissionBriefingUpdate,
		&mData, MissionBriefingDraw);
	GameLoop(&gData);
	if (mData.IsOK)
	{
		SoundPlay(&gSoundDevice, StrSound("mg"));
	}

	CFREE(mData.Title);
	CFREE(mData.Description);
	CFREE(mData.TypewriterBuf);
	return mData.IsOK;
}
static GameLoopResult MissionBriefingUpdate(void *data)
{
	MissionBriefingData *mData = data;

	// Check for player input; if any then skip to the end of the briefing
	int cmds[MAX_LOCAL_PLAYERS];
	memset(cmds, 0, sizeof cmds);
	GetPlayerCmds(&gEventHandlers, &cmds);
	for (int i = 0; i < MAX_LOCAL_PLAYERS; i++)
	{
		if (AnyButton(cmds[i]))
		{
			// If the typewriter is still going, skip to end
			if (mData->TypewriterCount <= (int)strlen(mData->Description))
			{
				strcpy(mData->TypewriterBuf, mData->Description);
				mData->TypewriterCount = strlen(mData->Description);
				return UPDATE_RESULT_DRAW;
			}
			// Otherwise, exit out of loop
			return UPDATE_RESULT_EXIT;
		}
	}
	// Check if anyone pressed escape
	if (EventIsEscape(&gEventHandlers, cmds, GetMenuCmd(&gEventHandlers)))
	{
		mData->IsOK = false;
		return UPDATE_RESULT_EXIT;
	}

	// Update the typewriter effect
	if (mData->TypewriterCount <= (int)strlen(mData->Description))
	{
		mData->TypewriterBuf[mData->TypewriterCount] =
			mData->Description[mData->TypewriterCount];
		mData->TypewriterCount++;
		return UPDATE_RESULT_DRAW;
	}

	return UPDATE_RESULT_OK;
}
static void MissionBriefingDraw(void *data)
{
	const MissionBriefingData *mData = data;

	GraphicsBlitBkg(&gGraphicsDevice);

	// Mission title
	FontStrOpt(mData->Title, Vec2iZero(), mData->TitleOpts);
	// Display password
	FontStrOpt(mData->Password, Vec2iZero(), mData->PasswordOpts);
	// Display description with typewriter effect
	FontStr(mData->TypewriterBuf, mData->DescriptionPos);
	// Display objectives
	for (int i = 0;
		i < (int)mData->MissionOptions->missionData->Objectives.size;
		i++)
	{
		const MissionObjective *o =
			CArrayGet(&mData->MissionOptions->missionData->Objectives, i);
		// Do not brief optional objectives
		if (o->Required == 0)
		{
			continue;
		}
		const Vec2i yInc = Vec2iNew(0, i * mData->ObjectiveHeight);
		FontStr(o->Description, Vec2iAdd(mData->ObjectiveDescPos, yInc));
		DrawObjectiveInfo(
			mData->MissionOptions, i, Vec2iAdd(mData->ObjectiveInfoPos, yInc));
	}
}

#define PERFECT_BONUS 500

static bool AreAnySurvived(void);
static int GetAccessBonus(const struct MissionOptions *m);
static int GetTimeBonus(const struct MissionOptions *m, int *secondsOut);
static void ApplyBonuses(PlayerData *p, const int bonus);
static void MissionSummaryDraw(void *data);
void ScreenMissionSummary(CampaignOptions *c, struct MissionOptions *m)
{
	// Save password
	MissionSave ms;
	MissionSaveInit(&ms);
	ms.Campaign = c->Entry;
	// Don't make password for next level if there is none
	int passwordIndex = m->index + 1;
	if (passwordIndex == c->Entry.NumMissions)
	{
		passwordIndex--;
	}
	strcpy(ms.Password, MakePassword(passwordIndex, 0));
	ms.MissionsCompleted = m->index + 1;
	AutosaveAddMission(&gAutosave, &ms);
	AutosaveSave(&gAutosave, GetConfigFilePath(AUTOSAVE_FILE));

	// Calculate bonus scores
	// Bonuses only apply if at least one player has lived
	if (AreAnySurvived())
	{
		int bonus = 0;
		// Objective bonuses
		for (int i = 0; i < (int)m->missionData->Objectives.size; i++)
		{
			const ObjectiveDef *o = CArrayGet(&m->Objectives, i);
			const MissionObjective *mo = CArrayGet(&m->missionData->Objectives, i);
			if (o->done == mo->Count && o->done > mo->Required)
			{
				// Perfect
				bonus += PERFECT_BONUS;
			}
		}
		bonus += GetAccessBonus(m);
		bonus += GetTimeBonus(m, NULL);

		CA_FOREACH(PlayerData, p, gPlayerDatas)
			ApplyBonuses(p, bonus);
		CA_FOREACH_END()
	}
	GameLoopWaitForAnyKeyOrButtonData wData;
	GameLoopData gData = GameLoopDataNew(
		&wData, GameLoopWaitForAnyKeyOrButtonFunc,
		m, MissionSummaryDraw);
	GameLoop(&gData);
	SoundPlay(&gSoundDevice, StrSound("mg"));
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
	int seconds =
		60 +
		(int)m->missionData->Objectives.size * 30 -
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
static int GetHealthBonus(const PlayerData *p);
static int GetResurrectionFee(const PlayerData *p);
static int GetButcherPenalty(const PlayerData *p);
static int GetNinjaBonus(const PlayerData *p);
static int GetFriendlyBonus(const PlayerData *p);
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
	if (p->weaponCount == 1 && !p->weapons[0]->CanShoot &&
		p->Stats.Friendlies == 0 && p->Stats.Kills > 5)
	{
		return 50 * p->Stats.Kills;
	}
	return 0;
}
static int GetFriendlyBonus(const PlayerData *p)
{
	return (p->Stats.Kills == 0 && p->Stats.Friendlies == 0) ? 500 : 0;
}
static void DrawPlayerSummary(
	const Vec2i pos, const Vec2i size, PlayerData *data);
static void MissionSummaryDraw(void *data)
{
	// This will only draw once
	const struct MissionOptions *m = data;

	GraphicsBlitBkg(&gGraphicsDevice);
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
		opts.Area = gGraphicsDevice.cachedConfig.Res;
		opts.Pad.y = opts.Area.y / 12;
		FontStrOpt(s, Vec2iZero(), opts);
	}

	// Display objectives and bonuses
	Vec2i pos = Vec2iNew(w / 6, h / 2 + h / 10);
	int idx = 1;
	for (int i = 0; i < (int)m->missionData->Objectives.size; i++)
	{
		const ObjectiveDef *o = CArrayGet(&m->Objectives, i);
		const MissionObjective *mo = CArrayGet(&m->missionData->Objectives, i);

		// Do not mention optional objectives with none completed
		if (o->done == 0 && mo->Required == 0)
		{
			continue;
		}

		// Objective icon
		DrawObjectiveInfo(m, i, Vec2iAdd(pos, Vec2iNew(-26, FontH())));

		// Objective completion text
		char s[100];
		sprintf(s, "Objective %d: %d of %d, %d required",
			idx, o->done, mo->Count, mo->Required);
		FontOpts opts = FontOptsNew();
		opts.Area = gGraphicsDevice.cachedConfig.Res;
		opts.Pad = pos;
		if (mo->Required == 0)
		{
			// Show optional objectives in purple
			opts.Mask = colorPurple;
		}
		FontStrOpt(s, Vec2iZero(), opts);

		// Objective status text
		opts = FontOptsNew();
		opts.HAlign = ALIGN_END;
		opts.Area = gGraphicsDevice.cachedConfig.Res;
		opts.Pad = pos;
		if (o->done < mo->Required)
		{
			opts.Mask = colorRed;
			FontStrOpt("Failed", Vec2iZero(), opts);
		}
		else if (
			o->done == mo->Count && o->done > mo->Required && AreAnySurvived())
		{
			opts.Mask = colorGreen;
			char buf[16];
			sprintf(buf, "Perfect: %d", PERFECT_BONUS);
			FontStrOpt(buf, Vec2iZero(), opts);
		}
		else if (mo->Required > 0)
		{
			FontStrOpt("Done", Vec2iZero(), opts);
		}
		else
		{
			FontStrOpt("Bonus!", Vec2iZero(), opts);
		}

		pos.y += 15;
		idx++;
	}

	// Draw other bonuses
	if (AreAnySurvived())
	{
		char s[64];

		sprintf(s, "Access bonus: %d", GetAccessBonus(m));
		FontStr(s, pos);

		pos.y += FontH() + 1;
		int seconds;
		const int timeBonus = GetTimeBonus(m, &seconds);
		sprintf(s, "Time bonus: %d secs x 25 = %d", seconds, timeBonus);
		FontStr(s, pos);
	}

	// Draw per-player summaries
	Vec2i size;
	PlayerData *pds[MAX_LOCAL_PLAYERS];
	idx = 0;
	CA_FOREACH(PlayerData, pd, gPlayerDatas)
		if (!pd->IsLocal)
		{
			idx--;
			continue;
		}
		pds[idx] = pd;
		idx++;
	CA_FOREACH_END()
	const int numLocalPlayers = GetNumPlayers(PLAYER_ANY, false, true);
	switch (numLocalPlayers)
	{
	case 1:
		size = Vec2iNew(w, h / 2);
		DrawPlayerSummary(Vec2iZero(), size, pds[0]);
		break;
	case 2:
		// side by side
		size = Vec2iNew(w / 2, h / 2);
		DrawPlayerSummary(Vec2iZero(), size, pds[0]);
		DrawPlayerSummary(Vec2iNew(w / 2, 0), size, pds[1]);
		break;
	case 3:	// fallthrough
	case 4:
		// 2x2
		size = Vec2iNew(w / 2, h / 4);
		DrawPlayerSummary(Vec2iZero(), size, pds[0]);
		DrawPlayerSummary(Vec2iNew(w / 2, 0), size, pds[1]);
		DrawPlayerSummary(Vec2iNew(0, h / 4), size, pds[2]);
		if (numLocalPlayers == 4)
		{
			DrawPlayerSummary(Vec2iNew(w / 2, h / 4), size, pds[3]);
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
	const Vec2i pos, const Vec2i size, PlayerData *data)
{
	char s[50];
	const int totalTextHeight = FontH() * 7;
	// display text on right half
	Vec2i textPos = Vec2iNew(
		pos.x + size.x / 2, CENTER_Y(pos, size, totalTextHeight));

	DisplayCharacterAndName(
		Vec2iAdd(pos, Vec2iNew(size.x / 4, size.y / 2)),
		&data->Char, data->name, colorWhite);

	if (data->survived)
	{
		FontStr("Completed mission", textPos);
	}
	else
	{
		FontStrMask("Failed mission", textPos, colorRed);
	}

	textPos.y += 2 * FontH();
	sprintf(s, "Score: %d", data->Stats.Score);
	FontStr(s, textPos);
	textPos.y += FontH();
	sprintf(s, "Total: %d", data->Totals.Score);
	FontStr(s, textPos);
	textPos.y += FontH();
	sprintf(s, "Missions: %d", data->missions + (data->survived ? 1 : 0));
	FontStr(s, textPos);
	textPos.y += FontH();

	// Display bonuses if the player has survived
	if (data->survived)
	{
		const int healthBonus = GetHealthBonus(data);
		if (healthBonus != 0)
		{
			sprintf(s, "Health bonus: %d", healthBonus);
		}
		const int resurrectionFee = GetResurrectionFee(data);
		if (resurrectionFee != 0)
		{
			sprintf(s, "Resurrection fee: %d", resurrectionFee);
		}
		if (healthBonus != 0 || resurrectionFee != 0)
		{
			FontStr(s, textPos);
			textPos.y += FontH();
		}

		const int butcherPenalty = GetButcherPenalty(data);
		if (butcherPenalty != 0)
		{
			sprintf(s, "Butcher penalty: %d", butcherPenalty);
		}
		const int ninjaBonus = GetNinjaBonus(data);
		if (ninjaBonus != 0)
		{
			sprintf(s, "Ninja bonus: %d", ninjaBonus);
		}
		const int friendlyBonus = GetFriendlyBonus(data);
		if (friendlyBonus != 0)
		{
			sprintf(s, "Friendly bonus: %d", friendlyBonus);
		}
		if (butcherPenalty != 0 || ninjaBonus != 0 || friendlyBonus != 0)
		{
			FontStr(s, textPos);
		}
	}
}

static void DrawObjectiveInfo(
	const struct MissionOptions *mo, const int idx, const Vec2i pos)
{
	const MissionObjective *mobj =
		CArrayGet(&mo->missionData->Objectives, idx);
	const ObjectiveDef *o = CArrayGet(&mo->Objectives, idx);
	const CharacterStore *store = &gCampaign.Setting.characters;

	switch (mobj->Type)
	{
	case OBJECTIVE_KILL:
		{
			const Character *cd = CArrayGet(
				&store->OtherChars, CharacterStoreGetSpecialId(store, 0));
			DrawHead(BODY_UNARMED, DIRECTION_DOWN, cd->Face, STATE_IDLE, pos);
		}
		break;
	case OBJECTIVE_RESCUE:
		{
			const Character *cd = CArrayGet(
				&store->OtherChars, CharacterStoreGetPrisonerId(store, 0));
			DrawHead(BODY_UNARMED, DIRECTION_DOWN, cd->Face, STATE_IDLE, pos);
		}
		break;
	case OBJECTIVE_COLLECT:
		{
			const Pic *p = o->pickupClass->Pic;
			Blit(&gGraphicsDevice, p,
				Vec2iMinus(pos, Vec2iScaleDiv(p->size, 2)));
		}
		break;
	case OBJECTIVE_DESTROY:
		{
			Vec2i picOffset;
			const Pic *p =
				MapObjectGetPic(IntMapObject(mobj->Index), &picOffset, false);
			Blit(&gGraphicsDevice, p, Vec2iAdd(pos, picOffset));
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
