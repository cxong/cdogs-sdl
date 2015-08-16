/*
    Copyright (c) 2013-2015, Cong Xu
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

		for (int i = 0; i < (int)gPlayerDatas.size; i++)
		{
			PlayerData *p = CArrayGet(&gPlayerDatas, i);
			ApplyBonuses(p, bonus);
		}
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
	for (int i = 0; i < (int)gPlayerDatas.size; i++)
	{
		const PlayerData *p = CArrayGet(&gPlayerDatas, i);
		if (p->survived)
		{
			return true;
		}
	}
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

	p->totalScore += bonus;

	// Other per-player bonuses
	p->totalScore += GetHealthBonus(p);
	p->totalScore += GetResurrectionFee(p);
	p->totalScore += GetButcherPenalty(p);
	p->totalScore += GetNinjaBonus(p);
	p->totalScore += GetFriendlyBonus(p);
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
	if (p->friendlies > 5 && p->friendlies > p->kills / 2)
	{
		return -100 * p->friendlies;
	}
	return 0;
}
static int GetNinjaBonus(const PlayerData *p)
{
	if (p->weaponCount == 1 && !p->weapons[0]->CanShoot &&
		p->friendlies == 0 && p->kills > 5)
	{
		return 50 * p->kills;
	}
	return 0;
}
static int GetFriendlyBonus(const PlayerData *p)
{
	return (p->kills == 0 && p->friendlies == 0) ? 500 : 0;
}
static void DrawPlayerSummary(
	const Vec2i pos, const Vec2i size, const PlayerData *data);
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
	const PlayerData *pds[MAX_LOCAL_PLAYERS];
	idx = 0;
	for (int i = 0; i < (int)gPlayerDatas.size; i++, idx++)
	{
		const PlayerData *pd = CArrayGet(&gPlayerDatas, i);
		if (!pd->IsLocal)
		{
			idx--;
			continue;
		}
		pds[idx] = pd;
	}
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
	const Vec2i pos, const Vec2i size, const PlayerData *data)
{
	char s[50];
	const int totalTextHeight = FontH() * 7;
	// display text on right half
	Vec2i textPos = Vec2iNew(
		pos.x + size.x / 2, CENTER_Y(pos, size, totalTextHeight));

	DisplayCharacterAndName(
		Vec2iAdd(pos, Vec2iNew(size.x / 4, size.y / 2)),
		&data->Char, data->name);

	if (data->survived)
	{
		FontStr("Completed mission", textPos);
	}
	else
	{
		FontStrMask("Failed mission", textPos, colorRed);
	}

	textPos.y += 2 * FontH();
	sprintf(s, "Score: %d", data->score);
	FontStr(s, textPos);
	textPos.y += FontH();
	sprintf(s, "Total: %d", data->totalScore);
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
			const int i = cd->looks.Face;
			TOffsetPic pic;
			pic.picIndex = cHeadPic[i][DIRECTION_DOWN][STATE_IDLE];
			pic.dx = cHeadOffset[i][DIRECTION_DOWN].dx;
			pic.dy = cHeadOffset[i][DIRECTION_DOWN].dy;
			DrawTTPic(
				pos.x + pic.dx, pos.y + pic.dy,
				PicManagerGetOldPic(&gPicManager, pic.picIndex), &cd->table);
		}
		break;
	case OBJECTIVE_RESCUE:
		{
			const Character *cd = CArrayGet(
				&store->OtherChars, CharacterStoreGetPrisonerId(store, 0));
			const int i = cd->looks.Face;
			TOffsetPic pic;
			pic.picIndex = cHeadPic[i][DIRECTION_DOWN][STATE_IDLE];
			pic.dx = cHeadOffset[i][DIRECTION_DOWN].dx;
			pic.dy = cHeadOffset[i][DIRECTION_DOWN].dy;
			DrawTTPic(
				pos.x + pic.dx, pos.y + pic.dy,
				PicManagerGetOldPic(&gPicManager, pic.picIndex), &cd->table);
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

static void VictoryDraw(void *data);
void ScreenVictory(CampaignOptions *c)
{
	SoundPlay(&gSoundDevice, StrSound("victory"));
	GameLoopData gData = GameLoopDataNew(
		NULL, GameLoopWaitForAnyKeyOrButtonFunc,
		c, VictoryDraw);
	GameLoop(&gData);
	SoundPlay(&gSoundDevice, StrSound("hahaha"));
}
static void VictoryDraw(void *data)
{
	// This will only draw once
	const CampaignOptions *c = data;

	GraphicsBlitBkg(&gGraphicsDevice);
	const int w = gGraphicsDevice.cachedConfig.Res.x;
	const int h = gGraphicsDevice.cachedConfig.Res.y;
	FontOpts opts = FontOptsNew();
	opts.HAlign = ALIGN_CENTER;
	opts.Area = gGraphicsDevice.cachedConfig.Res;
	int y = 100;

	// Congratulations text
#define CONGRATULATIONS "Congratulations, you have completed "
	FontStrOpt(CONGRATULATIONS, Vec2iNew(0, y), opts);
	y += 15;
	opts.Mask = colorRed;
	FontStrOpt(c->Setting.Title, Vec2iNew(0, y), opts);

	// Display players
	const PlayerData *pds[MAX_LOCAL_PLAYERS];
	for (int i = 0, idx = 0; i < (int)gPlayerDatas.size; i++, idx++)
	{
		const PlayerData *pd = CArrayGet(&gPlayerDatas, i);
		if (!pd->IsLocal)
		{
			idx--;
			continue;
		}
		pds[idx] = pd;
	}
	const int numLocalPlayers = GetNumPlayers(PLAYER_ANY, false, true);
	switch (numLocalPlayers)
	{
	case 1:
		DisplayCharacterAndName(
			Vec2iNew(w / 4, h / 4), &pds[0]->Char, pds[0]->name);
		break;
	case 2:
		// side by side
		DisplayCharacterAndName(
			Vec2iNew(w / 8, h / 4), &pds[0]->Char, pds[0]->name);
		DisplayCharacterAndName(
			Vec2iNew(w / 8 + w / 2, h / 4), &pds[1]->Char, pds[1]->name);
		break;
	case 3:	// fallthrough
	case 4:
		// 2x2
		DisplayCharacterAndName(
			Vec2iNew(w / 8, h / 8), &pds[0]->Char, pds[0]->name);
		DisplayCharacterAndName(
			Vec2iNew(w / 8 + w / 2, h / 8), &pds[1]->Char, pds[1]->name);
		DisplayCharacterAndName(
			Vec2iNew(w / 8, h / 8 + h / 4), &pds[2]->Char, pds[2]->name);
		if (numLocalPlayers == 4)
		{
			DisplayCharacterAndName(
				Vec2iNew(w / 8 + w / 2, h / 8 + h / 4),
				&pds[3]->Char, pds[3]->name);
		}
		break;
	default:
		CASSERT(false, "not implemented");
		break;
	}

	// Final words
	const char *finalWordsSingle[] = {
		"Ha, next time I'll use my good hand",
		"Over already? I was just warming up...",
		"There's just no good opposition to be found these days!",
		"Well, maybe I'll just do my monthly reload then",
		"Woof woof",
		"I'll just bury the bones in the back yard, he-he",
		"I just wish they'd let me try bare-handed",
		"Rambo? Who's Rambo?",
		"<in Austrian accent:> I'll be back",
		"Gee, my trigger finger is sore",
		"I need more practice. I think I missed a few shots at times"
	};
	const char *finalWordsMulti[] = {
		"United we stand, divided we conquer",
		"Nothing like good teamwork, is there?",
		"Which way is the camera?",
		"We eat bullets for breakfast and have grenades as dessert",
		"We're so cool we have to wear mittens",
	};
	const char *finalWords;
	if (GetNumPlayers(PLAYER_ANY, false, true) == 1)
	{
		const int numWords = sizeof finalWordsSingle / sizeof(char *);
		finalWords = finalWordsSingle[rand() % numWords];
	}
	else
	{
		const int numWords = sizeof finalWordsMulti / sizeof(char *);
		finalWords = finalWordsMulti[rand() % numWords];
	}
	Vec2i pos = Vec2iNew((w - FontStrW(finalWords)) / 2, h / 2 + 20);
	pos = FontChMask('"', pos, colorDarker);
	pos = FontStrMask(finalWords, pos, colorPurple);
	FontChMask('"', pos, colorDarker);
}

static void DogfightScoresDraw(void *data);
void ScreenDogfightScores(void)
{
	GameLoopData gData = GameLoopDataNew(
		NULL, GameLoopWaitForAnyKeyOrButtonFunc, NULL, DogfightScoresDraw);
	GameLoop(&gData);
	SoundPlay(&gSoundDevice, StrSound("mg"));
}
static void ShowPlayerScore(const Vec2i pos, const int score);
static void DogfightScoresDraw(void *data)
{
	UNUSED(data);

	// This will only draw once
	const int w = gGraphicsDevice.cachedConfig.Res.x;
	const int h = gGraphicsDevice.cachedConfig.Res.y;

	GraphicsBlitBkg(&gGraphicsDevice);

	const PlayerData *pds[MAX_LOCAL_PLAYERS];
	for (int i = 0, idx = 0; i < (int)gPlayerDatas.size; i++, idx++)
	{
		const PlayerData *pd = CArrayGet(&gPlayerDatas, i);
		if (!pd->IsLocal)
		{
			idx--;
			continue;
		}
		pds[idx] = pd;
	}
	const int numLocalPlayers = GetNumPlayers(PLAYER_ANY, false, true);
	CASSERT(
		numLocalPlayers >= 2 && numLocalPlayers <= 4,
		"Unimplemented number of players for dogfight");
	for (int i = 0; i < numLocalPlayers; i++)
	{
		const Vec2i pos = Vec2iNew(
			w / 4 + (i & 1) * w / 2,
			numLocalPlayers == 2 ? h / 2 : h / 4 + (i / 2) * h / 2);
		DisplayCharacterAndName(pos, &pds[i]->Char, pds[i]->name);
		ShowPlayerScore(pos, pds[i]->RoundsWon);
	}
}
static void ShowPlayerScore(const Vec2i pos, const int score)
{
	char s[16];
	sprintf(s, "Score: %d", score);
	const Vec2i scorePos = Vec2iNew(pos.x - FontStrW(s) / 2, pos.y + 20);
	FontStr(s, scorePos);
}

static void DogfightFinalScoresDraw(void *data);
void ScreenDogfightFinalScores(void)
{
	SoundPlay(&gSoundDevice, StrSound("victory"));
	GameLoopData gData = GameLoopDataNew(
		NULL, GameLoopWaitForAnyKeyOrButtonFunc,
		NULL, DogfightFinalScoresDraw);
	GameLoop(&gData);
	SoundPlay(&gSoundDevice, StrSound("mg"));
}
static void DogfightFinalScoresDraw(void *data)
{
	UNUSED(data);

	// This will only draw once
	const int w = gGraphicsDevice.cachedConfig.Res.x;
	const int h = gGraphicsDevice.cachedConfig.Res.y;

	GraphicsBlitBkg(&gGraphicsDevice);

	// Work out who's the winner, or if it's a tie
	int maxScore = 0;
	int playersWithMaxScore = 0;
	for (int i = 0; i < (int)gPlayerDatas.size; i++)
	{
		const PlayerData *p = CArrayGet(&gPlayerDatas, i);
		if (p->RoundsWon > maxScore)
		{
			maxScore = p->RoundsWon;
			playersWithMaxScore = 1;
		}
		else if (p->RoundsWon == maxScore)
		{
			playersWithMaxScore++;
		}
	}
	const bool isTie = playersWithMaxScore == (int)gPlayerDatas.size;

	// Draw players and their names spread evenly around the screen.
	// If it's a tie, display the message in the centre,
	// otherwise display the winner just below the winning player
#define DRAW_TEXT	"It's a draw!"
#define WINNER_TEXT	"Winner!"
	const PlayerData *pds[MAX_LOCAL_PLAYERS];
	for (int i = 0, idx = 0; i < (int)gPlayerDatas.size; i++, idx++)
	{
		const PlayerData *pd = CArrayGet(&gPlayerDatas, i);
		if (!pd->IsLocal)
		{
			idx--;
			continue;
		}
		pds[idx] = pd;
	}
	const int numLocalPlayers = GetNumPlayers(PLAYER_ANY, false, true);
	CASSERT(
		numLocalPlayers >= 2 && numLocalPlayers <= 4,
		"Unimplemented number of players for dogfight");
	for (int i = 0; i < numLocalPlayers; i++)
	{
		const Vec2i pos = Vec2iNew(
			w / 4 + (i & 1) * w / 2,
			numLocalPlayers == 2 ? h / 2 : h / 4 + (i / 2) * h / 2);
		DisplayCharacterAndName(pos, &pds[i]->Char, pds[i]->name);
		ShowPlayerScore(pos, pds[i]->RoundsWon);
		if (!isTie && maxScore == pds[i]->RoundsWon)
		{
			FontStrMask(
				WINNER_TEXT,
				Vec2iNew(pos.x - FontStrW(WINNER_TEXT) / 2, pos.y + 30),
				colorGreen);
		}
	}
	if (isTie)
	{
		FontStrCenter(DRAW_TEXT);
	}
}

static void DeathmatchFinalScoresDraw(void *data);
void ScreenDeathmatchFinalScores(void)
{
	SoundPlay(&gSoundDevice, StrSound("victory"));
	GameLoopData gData = GameLoopDataNew(
		NULL, GameLoopWaitForAnyKeyOrButtonFunc,
		NULL, DeathmatchFinalScoresDraw);
	GameLoop(&gData);
	SoundPlay(&gSoundDevice, StrSound("mg"));
}
static void DeathmatchFinalScoresDraw(void *data)
{
	UNUSED(data);

	// This will only draw once
	const int w = gGraphicsDevice.cachedConfig.Res.x;
	const int h = gGraphicsDevice.cachedConfig.Res.y;

	GraphicsBlitBkg(&gGraphicsDevice);

	// Work out the highest kills
	int maxKills = 0;
	for (int i = 0; i < (int)gPlayerDatas.size; i++)
	{
		const PlayerData *p = CArrayGet(&gPlayerDatas, i);
		if (p->kills > maxKills)
		{
			maxKills = p->kills;
		}
	}

	// Draw players and their names spread evenly around the screen.
	const PlayerData *pds[MAX_LOCAL_PLAYERS];
	for (int i = 0, idx = 0; i < (int)gPlayerDatas.size; i++, idx++)
	{
		const PlayerData *pd = CArrayGet(&gPlayerDatas, i);
		if (!pd->IsLocal)
		{
			idx--;
			continue;
		}
		pds[idx] = pd;
	}
	const int numLocalPlayers = GetNumPlayers(PLAYER_ANY, false, true);
	CASSERT(
		numLocalPlayers >= 2 && numLocalPlayers <= 4,
		"Unimplemented number of players for deathmatch");
#define LAST_MAN_TEXT	"Last man standing!"
	for (int i = 0; i < numLocalPlayers; i++)
	{
		const Vec2i pos = Vec2iNew(
			w / 4 + (i & 1) * w / 2,
			numLocalPlayers == 2 ? h / 2 : h / 4 + (i / 2) * h / 2);
		DisplayCharacterAndName(pos, &pds[i]->Char, pds[i]->name);
		
		// Kills
		char s[16];
		sprintf(s, "Kills: %d", pds[i]->kills);
		FontStrMask(
			s, Vec2iNew(pos.x - FontStrW(s) / 2, pos.y + 20),
			pds[i]->kills == maxKills ? colorGreen : colorWhite);

		// Last man standing?
		if (pds[i]->Lives > 0)
		{
			FontStrMask(
				LAST_MAN_TEXT,
				Vec2iNew(pos.x - FontStrW(LAST_MAN_TEXT) / 2, pos.y + 30),
				colorGreen);
		}
	}
}
