/*
	Copyright (c) 2025 Cong Xu
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
#include "weapon_usage.h"

WeaponUsages WeaponUsagesNew(void)
{
	return hashmap_new();
}

void WeaponUsagesTerminate(WeaponUsages wu)
{
	hashmap_free(wu);
}

void WeaponUsagesUpdate(
	WeaponUsages wu, const WeaponClass *wc, const int dShot, const int dHit)
{
	// TODO: accuracy for explosives?
	if (!wc || !wc->IsRealGun || WeaponClassIsHighDPS(wc))
	{
		return;
	}
	NWeaponUsage *w = NULL;
	if (hashmap_get(wu, wc->name, (any_t *)&w) == MAP_MISSING)
	{
		CCALLOC(w, sizeof *w);
		strcpy(w->Weapon, wc->name);
		hashmap_put(wu, wc->name, w);
	}
	w->Shots += dShot;
	w->Hits += dHit;
}

typedef struct
{
	float accuracy;
	int weight;
} GetAccuracyData;
static int GetAccuracyItem(any_t data, any_t item);
float WeaponUsagesGetAccuracy(const WeaponUsages wu)
{
	CArray accuracies;
	CArrayInit(&accuracies, sizeof(GetAccuracyData));
	hashmap_iterate(wu, GetAccuracyItem, &accuracies);
	float cumSum = 0;
	int totalWeight = 0;
	CA_FOREACH(const GetAccuracyData, gData, accuracies)
	cumSum += gData->accuracy * gData->weight;
	totalWeight += gData->weight;
	CA_FOREACH_END()
	return totalWeight > 0 ? cumSum / totalWeight : 0;
}
static int GetAccuracyItem(any_t data, any_t item)
{
	NWeaponUsage *w = item;
	if (w->Shots > 0)
	{
		GetAccuracyData gData;
		gData.accuracy = (float)w->Hits / w->Shots;
		const WeaponClass *wc = StrWeaponClass(w->Weapon);
		gData.weight = MAX(wc->Lock, 1);
		CArray *accuracies = data;
		CArrayPushBack(accuracies, &gData);
	}
	return MAP_OK;
}

typedef struct
{
	const WeaponClass *wc;
	int ticks;
} GetFavoriteData;
static int GetFavoriteItem(any_t data, any_t item);
const WeaponClass *WeaponUsagesGetFavorite(const WeaponUsages wu)
{
	// Find the weapon that spent the most time shooting
	GetFavoriteData gData = {NULL, 0};
	hashmap_iterate(wu, GetFavoriteItem, &gData);
	return gData.wc;
}
static int GetFavoriteItem(any_t data, any_t item)
{
	GetFavoriteData *gData = data;
	NWeaponUsage *w = item;
	const WeaponClass *wc = StrWeaponClass(w->Weapon);
	const int ticks = w->Shots * MAX(wc->Lock, 1);
	if (ticks > gData->ticks)
	{
		gData->wc = wc;
		gData->ticks = ticks;
	}
	return MAP_OK;
}
