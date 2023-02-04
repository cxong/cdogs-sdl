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
#include "weapon_menu.h"

typedef struct
{
	MenuDisplayPlayerData display;
	int PlayerUID;
	AnimatedCounter Cash;
	int slot;
	bool equipping;
	bool EquipEnabled[MAX_WEAPONS];
	const NamedSprites *slotBGSprites;
	CArray weapons;		// of const WeaponClass *
	CArray weaponIsNew; // of bool
	bool SlotHasNew[MAX_WEAPONS];
	struct vec2i size;
	int ammoSlot;
	int endSlot;
	MenuSystem ms;
	WeaponMenu weaponMenus[MAX_WEAPONS];
	WeaponMenu ammoMenu;
} EquipMenu;

void EquipMenuCreate(
	EquipMenu *menu, const CArray *weapons, const CArray *prevWeapons,
	const int numPlayers, const int player, const int playerUID,
	EventHandlers *handlers, GraphicsDevice *graphics);
void EquipMenuTerminate(EquipMenu *menu);

void EquipMenuUpdate(EquipMenu *menu, const int cmd);
bool EquipMenuIsDone(const EquipMenu *menu);
void EquipMenuDraw(const EquipMenu *menu);
