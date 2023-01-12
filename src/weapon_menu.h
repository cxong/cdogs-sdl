/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.

	Copyright (c) 2013-2015, 2018, 2020, 2022-2023 Cong Xu
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

#include "animated_counter.h"
#include "menu.h"
#include "menu_utils.h"

typedef enum
{
	WEAPON_MENU_NONE,
	WEAPON_MENU_SELECT,
	WEAPON_MENU_CANCEL
} WeaponMenuResult;
typedef struct
{
	MenuDisplayPlayerData display;
	int PlayerUID;
	AnimatedCounter Cash;
	int EquipSlot;
	bool equipping;
	bool EquipEnabled[MAX_WEAPONS];
	WeaponMenuResult SelectResult;
	const NamedSprites *slotBGSprites;
	const NamedSprites *gunBGSprites;
	int gunIdx;
	CArray weapons;		// of const WeaponClass *
	CArray weaponIsNew; // of bool
	bool SlotHasNew[MAX_WEAPONS];
	int cols;
	int scroll;
	int ammoSlot;
	int endSlot;
} WeaponMenuData;
typedef struct
{
	MenuSystem ms;
	MenuSystem msEquip;
	WeaponMenuData data;
} WeaponMenu;

void WeaponMenuCreate(
	WeaponMenu *menu, const CArray *weapons, const CArray *prevWeapons,
	const int numPlayers, const int player, const int playerUID,
	EventHandlers *handlers, GraphicsDevice *graphics);
void WeaponMenuTerminate(WeaponMenu *menu);

void WeaponMenuUpdate(WeaponMenu *menu, const int cmd);
bool WeaponMenuIsDone(const WeaponMenu *menu);
void WeaponMenuDraw(const WeaponMenu *menu);
