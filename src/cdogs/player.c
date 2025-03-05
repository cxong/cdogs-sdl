/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.
	Copyright (c) 2014-2016, 2018-2020, 2022-2023, 2025 Cong Xu
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
#include "player.h"

#include "actors.h"
#include "events.h"
#include "log.h"
#include "net_client.h"
#include "player_template.h"

#define STARTING_CASH 10000

CArray gPlayerDatas;

void PlayerDataInit(CArray *p)
{
	CArrayInit(p, sizeof(PlayerData));
}

void PlayerDataAddOrUpdate(const NPlayerData pd)
{
	PlayerData *p = PlayerDataGetByUID(pd.UID);
	if (p == NULL)
	{
		PlayerData pNew;
		memset(&pNew, 0, sizeof pNew);
		CArrayPushBack(&gPlayerDatas, &pNew);
		p = CArrayGet(&gPlayerDatas, (int)gPlayerDatas.size - 1);

		// Set defaults
		p->ActorUID = -1;
		p->IsLocal =
			(int)pd.UID >= gNetClient.FirstPlayerUID &&
			(int)pd.UID < gNetClient.FirstPlayerUID + MAX_LOCAL_PLAYERS;
		CArrayInitFillZero(&p->ammo, sizeof(int), pd.Ammo_count);
		p->inputDevice = INPUT_DEVICE_UNSET;

		p->Char.speed = 1;

		p->WeaponUsages = WeaponUsagesNew();

		LOG(LM_MAIN, LL_INFO, "add default player UID(%u) local(%s)", pd.UID,
			p->IsLocal ? "true" : "false");
	}

	p->UID = pd.UID;

	strcpy(p->name, pd.Name);
	p->Char.Class = StrCharacterClass(pd.CharacterClass);
	if (p->Char.Class == NULL)
	{
		p->Char.Class = StrCharacterClass("Jones");
	}
#define ADDHEADPART(_hp, _pdPart)                                             \
	CFREE(p->Char.HeadParts[_hp]);                                            \
	p->Char.HeadParts[_hp] = NULL;                                            \
	if (strlen(_pdPart) > 0)                                                  \
	{                                                                         \
		CSTRDUP(p->Char.HeadParts[_hp], _pdPart);                             \
	}
	ADDHEADPART(HEAD_PART_HAIR, pd.Hair);
	ADDHEADPART(HEAD_PART_FACEHAIR, pd.Facehair);
	ADDHEADPART(HEAD_PART_HAT, pd.Hat);
	ADDHEADPART(HEAD_PART_GLASSES, pd.Glasses);
	p->Char.Colors = Net2CharColors(pd.Colors);
	for (int i = 0; i < (int)pd.Weapons_count; i++)
	{
		p->guns[i] = NULL;
		if (strlen(pd.Weapons[i]) > 0)
		{
			const WeaponClass *wc = StrWeaponClass(pd.Weapons[i]);
			p->guns[i] = wc;
		}
	}
	CArrayFillZero(&p->ammo);
	for (int i = 0; i < (int)pd.Ammo_count; i++)
	{
		CArraySet(&p->ammo, pd.Ammo[i].Id, &pd.Ammo[i].Amount);
	}
	p->Lives = pd.Lives;
	p->Stats = pd.Stats;
	p->Totals = pd.Totals;
	p->Char.maxHealth = pd.MaxHealth;
	p->Char.excessHealth = pd.ExcessHealth;
	p->HP = pd.HP;
	p->lastMission = pd.LastMission;

	// Ready players as well
	p->Ready = true;

	LOG(LM_MAIN, LL_INFO,
		"update player UID(%d) maxHealth(%d) excessHealth(%d)  HP(%d)", p->UID,
		p->Char.maxHealth, p->Char.excessHealth, p->HP);
}

static void PlayerTerminate(PlayerData *p);
void PlayerRemove(const int uid)
{
	// Find the player so we can remove by index
	PlayerData *p = NULL;
	int i;
	for (i = 0; i < (int)gPlayerDatas.size; i++)
	{
		PlayerData *pi = CArrayGet(&gPlayerDatas, i);
		if (pi->UID == uid)
		{
			p = pi;
			break;
		}
	}
	if (p == NULL)
	{
		return;
	}
	if (p->ActorUID >= 0)
	{
		ActorDestroy(ActorGetByUID(p->ActorUID));
	}
	PlayerTerminate(p);
	CArrayDelete(&gPlayerDatas, i);

	LOG(LM_MAIN, LL_INFO, "remove player UID(%d)", uid);
}

static void PlayerTerminate(PlayerData *p)
{
	CFREE(p->Char.bot);
	WeaponUsagesTerminate(p->WeaponUsages);
}

NPlayerData PlayerDataDefault(const int idx)
{
	NPlayerData pd = NPlayerData_init_default;

	pd.has_Colors = pd.has_Stats = pd.has_Totals = true;

	// load from template if available
	const PlayerTemplate *t = PlayerTemplateGetById(&gPlayerTemplates, idx);
	if (t != NULL)
	{
		strcpy(pd.Name, t->name);
		strcpy(pd.CharacterClass, t->CharClassName);
		if (t->HeadParts[HEAD_PART_HAIR] != NULL)
		{
			strcpy(pd.Hair, t->HeadParts[HEAD_PART_HAIR]);
		}
		if (t->HeadParts[HEAD_PART_FACEHAIR] != NULL)
		{
			strcpy(pd.Facehair, t->HeadParts[HEAD_PART_FACEHAIR]);
		}
		if (t->HeadParts[HEAD_PART_HAT] != NULL)
		{
			strcpy(pd.Hat, t->HeadParts[HEAD_PART_HAT]);
		}
		if (t->HeadParts[HEAD_PART_GLASSES] != NULL)
		{
			strcpy(pd.Glasses, t->HeadParts[HEAD_PART_GLASSES]);
		}
		pd.Colors = CharColors2Net(t->Colors);
	}
	else
	{
		switch (idx)
		{
		case 0:
			strcpy(pd.Name, "Jones");
			strcpy(pd.CharacterClass, "Jones");
			pd.Colors.Skin = Color2Net(colorSkin);
			pd.Colors.Arms = Color2Net(colorLightBlue);
			pd.Colors.Body = Color2Net(colorLightBlue);
			pd.Colors.Legs = Color2Net(colorLightBlue);
			pd.Colors.Feet = Color2Net(colorLightBlue);
			break;
		case 1:
			strcpy(pd.Name, "Ice");
			strcpy(pd.CharacterClass, "Jones");
			strcpy(pd.Glasses, "shades");
			pd.Colors.Skin = Color2Net(colorDarkSkin);
			pd.Colors.Arms = Color2Net(colorRed);
			pd.Colors.Body = Color2Net(colorRed);
			pd.Colors.Legs = Color2Net(colorRed);
			pd.Colors.Feet = Color2Net(colorRed);
			pd.Colors.Glasses = Color2Net(colorBlack);
			break;
		case 2:
			strcpy(pd.Name, "Warbaby");
			strcpy(pd.CharacterClass, "Jones");
			strcpy(pd.Hat, "beret");
			pd.Colors.Skin = Color2Net(colorSkin);
			pd.Colors.Arms = Color2Net(colorGreen);
			pd.Colors.Body = Color2Net(colorGreen);
			pd.Colors.Legs = Color2Net(colorGreen);
			pd.Colors.Feet = Color2Net(colorGreen);
			pd.Colors.Hat = Color2Net(colorRed);
			break;
		case 3:
			strcpy(pd.Name, "Han");
			strcpy(pd.CharacterClass, "Jones");
			strcpy(pd.Hair, "rattail");
			strcpy(pd.Facehair, "handlebar");
			pd.Colors.Skin = Color2Net(colorAsianSkin);
			pd.Colors.Arms = Color2Net(colorYellow);
			pd.Colors.Body = Color2Net(colorYellow);
			pd.Colors.Legs = Color2Net(colorYellow);
			pd.Colors.Hair = Color2Net(colorYellow);
			pd.Colors.Feet = Color2Net(colorYellow);
			pd.Colors.Facehair = Color2Net(colorYellow);
			break;
		default:
			// Set up player N template
			sprintf(pd.Name, "Player %d", idx);
			strcpy(pd.CharacterClass, "Jones");
			pd.Colors.Skin = Color2Net(colorSkin);
			pd.Colors.Arms = Color2Net(colorLightBlue);
			pd.Colors.Body = Color2Net(colorLightBlue);
			pd.Colors.Legs = Color2Net(colorLightBlue);
			pd.Colors.Hair = Color2Net(colorLightBlue);
			pd.Colors.Feet = Color2Net(colorLightBlue);
			break;
		}
	}

	pd.HP = CampaignGetHP(&gCampaign);
	pd.MaxHealth = CampaignGetMaxHP(&gCampaign);
	pd.ExcessHealth = CampaignGetExcessHP(&gCampaign);
	pd.Lives = CampaignGetLives(&gCampaign);
	if (gCampaign.Setting.BuyAndSell)
	{
		pd.Totals.Score = STARTING_CASH;
	}

	pd.Ammo_count = (pb_size_t)AmmoGetNumClasses(&gAmmo);

	return pd;
}

NPlayerData PlayerDataMissionReset(const PlayerData *p)
{
	NPlayerData pd = NMakePlayerData(p);
	if (gCampaign.Setting.PlayerHP == 0)
	{
		pd.HP = CampaignGetHP(&gCampaign);
	}

	memset(&pd.Stats, 0, sizeof pd.Stats);

	pd.LastMission = gCampaign.MissionIndex;
	pd.MaxHealth = CampaignGetMaxHP(&gCampaign);
	pd.ExcessHealth = CampaignGetExcessHP(&gCampaign);
	return pd;
}

void PlayerDataTerminate(CArray *p)
{
	for (int i = 0; i < (int)p->size; i++)
	{
		PlayerTerminate(CArrayGet(p, i));
	}
	CArrayTerminate(p);
}

PlayerData *PlayerDataGetByUID(const int uid)
{
	if (uid == -1)
	{
		return NULL;
	}
	CA_FOREACH(PlayerData, p, gPlayerDatas)
	if (p->UID == uid)
		return p;
	CA_FOREACH_END()
	return NULL;
}

int FindLocalPlayerIndex(const int uid)
{
	const PlayerData *p = PlayerDataGetByUID(uid);
	if (p == NULL || !p->IsLocal)
	{
		// This update was for a non-local player; abort
		return -1;
	}
	// Note: player UIDs divided by MAX_LOCAL_PLAYERS per client
	return uid % MAX_LOCAL_PLAYERS;
}

int GetNumPlayers(
	const PlayerAliveOptions alive, const bool human, const bool local)
{
	int numPlayers = 0;
	CA_FOREACH(const PlayerData, p, gPlayerDatas)
	bool life = false;
	switch (alive)
	{
	case PLAYER_ANY:
		life = true;
		break;
	case PLAYER_ALIVE:
		life = IsPlayerAlive(p);
		break;
	case PLAYER_ALIVE_OR_DYING:
		life = IsPlayerAliveOrDying(p);
		break;
	}
	if (life && (!human || p->inputDevice != INPUT_DEVICE_AI) &&
		(!local || p->IsLocal))
	{
		numPlayers++;
	}
	CA_FOREACH_END()
	return numPlayers;
}

bool AreAllPlayersDeadAndNoLives(void)
{
	CA_FOREACH(const PlayerData, p, gPlayerDatas)
	if (IsPlayerAlive(p) || p->Lives > 0)
	{
		return false;
	}
	CA_FOREACH_END()
	return true;
}

const PlayerData *GetFirstPlayer(
	const bool alive, const bool human, const bool local)
{
	CA_FOREACH(const PlayerData, p, gPlayerDatas)
	if ((!alive || IsPlayerAliveOrDying(p)) &&
		(!human || p->inputDevice != INPUT_DEVICE_AI) &&
		(!local || p->IsLocal))
	{
		return p;
	}
	CA_FOREACH_END()
	return NULL;
}

bool IsPlayerAlive(const PlayerData *player)
{
	if (player == NULL || player->ActorUID == -1)
	{
		return false;
	}
	const TActor *p = ActorGetByUID(player->ActorUID);
	return !p->dead;
}
bool IsPlayerHuman(const PlayerData *player)
{
	return player->inputDevice != INPUT_DEVICE_AI;
}
bool IsPlayerHumanAndAlive(const PlayerData *player)
{
	return IsPlayerAlive(player) && IsPlayerHuman(player);
}
bool IsPlayerAliveOrDying(const PlayerData *player)
{
	if (player->ActorUID == -1)
	{
		return false;
	}
	const TActor *p = ActorGetByUID(player->ActorUID);
	const NamedSprites *deathSprites = CharacterClassGetDeathSprites(
		ActorGetCharacter(p)->Class, &gPicManager);
	return p->dead <= (int)deathSprites->pics.size;
}
bool IsPlayerScreen(const PlayerData *p)
{
	const bool humanOnly = IsPVP(gCampaign.Entry.Mode) ||
						   !ConfigGetBool(&gConfig, "Interface.SplitscreenAI");
	const bool humanOrScreen = !humanOnly || p->inputDevice != INPUT_DEVICE_AI;
	return p->IsLocal && humanOrScreen && IsPlayerAliveOrDying(p);
}

struct vec2 PlayersGetMidpoint(void)
{
	// for all surviving players, find bounding rectangle, and get center
	struct vec2 min, max;
	PlayersGetBoundingRectangle(&min, &max);
	return svec2_scale(svec2_add(min, max), 0.5f);
}

void PlayersGetBoundingRectangle(struct vec2 *min, struct vec2 *max)
{
	bool isFirst = true;
	*min = svec2_zero();
	*max = svec2_zero();
	const bool humansOnly =
		GetNumPlayers(PLAYER_ALIVE_OR_DYING, true, false) > 0;
	CA_FOREACH(const PlayerData, p, gPlayerDatas)
	if (!p->IsLocal)
	{
		continue;
	}
	if (humansOnly ? IsPlayerHumanAndAlive(p) : IsPlayerAlive(p))
	{
		const TActor *player = ActorGetByUID(p->ActorUID);
		const Thing *ti = &player->thing;
		if (isFirst)
		{
			*min = *max = ti->Pos;
		}
		else
		{
			min->x = MIN(ti->Pos.x, min->x);
			min->y = MIN(ti->Pos.y, min->y);
			max->x = MAX(ti->Pos.x, max->x);
			max->y = MAX(ti->Pos.y, max->y);
		}
		isFirst = false;
	}
	CA_FOREACH_END()
}

// Get the number of players that use this ammo
int PlayersNumUseAmmo(const int ammoId)
{
	int numPlayersWithAmmo = 0;
	CA_FOREACH(const PlayerData, p, gPlayerDatas)
	if (!IsPlayerAlive(p))
	{
		continue;
	}
	const TActor *player = ActorGetByUID(p->ActorUID);
	for (int j = 0; j < MAX_WEAPONS; j++)
	{
		const Weapon *w = &player->guns[j];
		if (w->Gun == NULL)
			continue;
		for (int i = 0; i < WeaponClassNumBarrels(w->Gun); i++)
		{
			if (WC_BARREL_ATTR(*(w->Gun), AmmoId, i) == ammoId)
			{
				numPlayersWithAmmo++;
			}
		}
	}
	CA_FOREACH_END()
	return numPlayersWithAmmo;
}

bool PlayerIsLocal(const int uid)
{
	const PlayerData *p = PlayerDataGetByUID(uid);
	return p != NULL && p->IsLocal;
}

void PlayerScore(PlayerData *p, const int points)
{
	if (p == NULL)
	{
		return;
	}
	p->Stats.Score += points;
	p->Totals.Score += points;
}

bool PlayerTrySetInputDevice(
	PlayerData *p, const input_device_e d, const int idx)
{
	if (p->inputDevice == d && p->deviceIndex == idx)
	{
		return false;
	}
	p->inputDevice = d;
	p->deviceIndex = idx;
	LOG(LM_MAIN, LL_DEBUG, "playerUID(%d) assigned input device(%d %d)",
		p->UID, (int)d, idx);
	return true;
}

bool PlayerTrySetUnusedInputDevice(
	PlayerData *p, const input_device_e d, const int idx)
{
	// Check that player's input device is unassigned
	if (p->inputDevice != INPUT_DEVICE_UNSET)
		return false;
	// Check that no players use this input device
	CA_FOREACH(const PlayerData, pOther, gPlayerDatas)
	if (pOther->inputDevice == d && pOther->deviceIndex == idx)
	{
		return false;
	}
	CA_FOREACH_END()
	return PlayerTrySetInputDevice(p, d, idx);
}

int PlayerGetNumWeapons(const PlayerData *p)
{
	int count = 0;
	for (int i = 0; i < MAX_WEAPONS; i++)
	{
		if (p->guns[i] != NULL)
		{
			count++;
		}
	}
	return count;
}

bool PlayerHasGrenadeButton(const PlayerData *p)
{
	return p && InputHasGrenadeButton(p->inputDevice, p->deviceIndex);
}

bool PlayerHasWeapon(const PlayerData *p, const WeaponClass *wc)
{
	if (wc == NULL)
	{
		return false;
	}
	for (int i = 0; i < MAX_WEAPONS; i++)
	{
		if (p->guns[i] == wc)
		{
			return true;
		}
	}
	return false;
}

bool PlayerHasWeaponUpgrade(const PlayerData *p, const WeaponClass *wc)
{
	if (wc == NULL)
	{
		return false;
	}
	for (int i = 0; i < MAX_WEAPONS; i++)
	{
		const WeaponClass *prereq = WeaponClassGetPrerequisite(p->guns[i]);
		if (prereq == wc)
		{
			return true;
		}
	}
	return false;
}

static void AddWeaponToSlot(
	PlayerData *p, const WeaponClass *wc, const int slot)
{
	p->guns[slot] = wc;
	// Add minimal ammo
	const int numBarrels = WeaponClassNumBarrels(wc);
	for (int i = 0; i < numBarrels; i++)
	{
		const int ammoId = WC_BARREL_ATTR(*wc, AmmoId, i);
		if (ammoId < 0)
		{
			continue;
		}
		const Ammo *a = AmmoGetById(&gAmmo, ammoId);
		const int startingAmount = a->Amount * AMMO_STARTING_MULTIPLE;
		if (startingAmount > PlayerGetAmmoAmount(p, ammoId))
		{
			CArraySet(&p->ammo, ammoId, &startingAmount);
		}
	}
}

void PlayerAddWeaponToSlot(
	PlayerData *p, const WeaponClass *wc, const int slot)
{
	if (wc)
	{
		// See if the weapon or its pre-/post-requisite is already
		// equipped; if so swap it with the slot
		for (int i = 0; i < MAX_WEAPONS; i++)
		{
			if (i == slot)
			{
				continue;
			}
			if (p->guns[i] && (p->guns[i] == wc ||
							   WeaponClassGetPrerequisite(p->guns[i]) == wc ||
							   p->guns[i] == WeaponClassGetPrerequisite(wc)))
			{
				p->guns[i] = p->guns[slot];
				PlayerAddWeaponToSlot(p, wc, i);
				return;
			}
		}
	}
	// Remove old weapon
	if (p->guns[slot])
	{
		PlayerRemoveWeapon(p, slot);
	}
	// Subtract gun price
	if (gCampaign.Setting.BuyAndSell)
	{
		PlayerScore(p, -WeaponClassFullPrice(wc));
	}
	AddWeaponToSlot(p, wc, slot);
}

void PlayerAddWeapon(PlayerData *p, const WeaponClass *wc)
{
	if (PlayerHasWeapon(p, wc))
	{
		return;
	}
	// Add the weapon to the next empty slot, based on type
	int first = 0;
	int max = MELEE_SLOT;
	if (wc->Type == GUNTYPE_MELEE)
	{
		first = MELEE_SLOT;
		max = MELEE_SLOT + 1;
	}
	else if (wc->Type == GUNTYPE_GRENADE)
	{
		first = MAX_GUNS;
		max = first + MAX_GRENADES;
	}
	for (int i = first; i < max; i++)
	{
		if (p->guns[i] == NULL)
		{
			AddWeaponToSlot(p, wc, i);
			break;
		}
	}
}

void PlayerRemoveWeapon(PlayerData *p, const int slot)
{
	// Refund gun price
	if (gCampaign.Setting.BuyAndSell)
	{
		PlayerScore(p, WeaponClassFullPrice(p->guns[slot]));
	}
	// Refund ammo if no guns use this ammo
	int ammoIds[MAX_BARRELS] = {-1, -1};
	const int numBarrels = WeaponClassNumBarrels(p->guns[slot]);
	for (int i = 0; i < numBarrels; i++)
	{
		ammoIds[i] = WC_BARREL_ATTR(*p->guns[slot], AmmoId, i);
	}
	p->guns[slot] = NULL;
	PlayerAddMinimalWeapons(p);
	for (int i = 0; i < MAX_BARRELS; i++)
	{
		if (ammoIds[i] >= 0 && !PlayerUsesAmmo(p, ammoIds[i]))
		{
			PlayerAddAmmo(
				p, ammoIds[i], -PlayerGetAmmoAmount(p, ammoIds[i]), false);
		}
	}
}

void PlayerAddMinimalWeapons(PlayerData *p)
{
	// Always have fists
	PlayerAddWeapon(p, StrWeaponClass("Fists"));
}

bool PlayerUsesAmmo(const PlayerData *p, const int ammoId)
{
	for (int i = 0; i < MAX_WEAPONS; i++)
	{
		if (p->guns[i] == NULL)
		{
			continue;
		}
		const int numBarrels = WeaponClassNumBarrels(p->guns[i]);
		for (int j = 0; j < numBarrels; j++)
		{
			const int ammoId2 = WC_BARREL_ATTR(*p->guns[i], AmmoId, j);
			if (ammoId == ammoId2)
			{
				return true;
			}
		}
	}
	return false;
}

bool PlayerUsesAnyAmmo(const PlayerData *p)
{
	for (int i = 0; i < MAX_WEAPONS; i++)
	{
		if (p->guns[i] == NULL)
		{
			continue;
		}
		const int numBarrels = WeaponClassNumBarrels(p->guns[i]);
		for (int j = 0; j < numBarrels; j++)
		{
			const int ammoId2 = WC_BARREL_ATTR(*p->guns[i], AmmoId, j);
			if (ammoId2 >= 0)
			{
				return true;
			}
		}
	}
	return false;
}

int PlayerGetAmmoAmount(const PlayerData *p, const int ammoId)
{
	return (ammoId >= 0 && (int)p->ammo.size >= ammoId)
			   ? *(int *)CArrayGet(&p->ammo, ammoId)
			   : 0;
}

void PlayerAddAmmo(
	PlayerData *p, const int ammoId, const int amount, const bool isFree)
{
	int *ammoAmount = CArrayGet(&p->ammo, ammoId);
	const int oldAmount = *ammoAmount;
	const Ammo *a = AmmoGetById(&gAmmo, ammoId);
	*ammoAmount = CLAMP(*ammoAmount + amount, 0, a->Max);
	if (!isFree && a->Price && gCampaign.Setting.BuyAndSell)
	{
		const int dLots = (oldAmount - *ammoAmount) / a->Amount;
		PlayerScore(p, dLots * a->Price);
	}
}

void PlayerSetHP(PlayerData *p, const int hp)
{
	p->HP = CLAMP(hp, 1, CampaignGetMaxHP(&gCampaign));
}

void PlayerSetLives(PlayerData *p, const int lives)
{
	p->Lives = CLAMP(lives, 1, CampaignGetMaxLives(&gCampaign));
}

int CharacterGetStartingHealth(const Character *c, const PlayerData *p)
{
	if (p == NULL)
	{
		return MAX(
			(c->maxHealth * ConfigGetInt(&gConfig, "Game.NonPlayerHP")) / 100,
			1);
	}
	else
	{
		return p->HP > 0 ? p->HP : c->maxHealth;
	}
}
