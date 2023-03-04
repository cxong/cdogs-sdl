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
	MenuSystem ms;
	bool Active;
	int PlayerUID;
	WeaponMenuResult SelectResult;
	int slot;
	const NamedSprites *menuBGSprites;
	int idx;
	CArray weaponIndices;	   // of int
	const CArray *weapons;	   // of const WeaponClass *
	const CArray *weaponIsNew; // of bool
	CArray weaponMeta;  // of DrawGunMeta
	struct vec2i size;
	int cols;
	int scroll;
} WeaponMenu;

void WeaponMenuCreate(
	WeaponMenu *menu, const CArray *weapons, const CArray *weaponIsNew,
	const int playerUID, const int slot, const struct vec2i pos,
	const struct vec2i size, EventHandlers *handlers,
	GraphicsDevice *graphics);
void WeaponMenuTerminate(WeaponMenu *menu);

void WeaponMenuReset(WeaponMenu *menu);
void WeaponMenuActivate(WeaponMenu *menu);
void WeaponMenuUpdate(WeaponMenu *menu, const int cmd);
void WeaponMenuDraw(const WeaponMenu *menu);

int WeaponMenuSelectedCostDiff(const WeaponMenu *menu);

void DrawWeaponAmmo(
	GraphicsDevice *g, const PlayerData *p, const WeaponClass *wc,
	const color_t mask, const struct vec2i pos, const struct vec2i slotSize);
