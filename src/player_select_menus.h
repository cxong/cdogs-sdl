/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.

	Copyright (c) 2013-2016, 2020 Cong Xu
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

#include <cdogs/character.h>

#include "menu.h"
#include "menu_utils.h"
#include "namegen.h"

typedef struct
{
	int PlayerUID;
	HeadPart HP;
} HeadPartMenuData;
typedef struct
{
	CharColorType Type;
	int PlayerUID;
	const MenuSystem *ms;
	const Pic *palette;
	struct vec2i selectedColor;
} ColorMenuData;
typedef struct
{
	MenuDisplayPlayerData display;
	const MenuSystem *ms;
	int nameMenuSelection;
	HeadPartMenuData headPartData[HEAD_PART_COUNT];
	ColorMenuData skinData;
	ColorMenuData hairData;
	ColorMenuData armsData;
	ColorMenuData bodyData;
	ColorMenuData legsData;
	ColorMenuData feetData;
	ColorMenuData facehairData;
	ColorMenuData hatData;
	ColorMenuData glassesData;
	const NameGen *nameGenerator;
} PlayerSelectMenuData;
typedef struct
{
	MenuSystem ms;
	PlayerSelectMenuData data;
} PlayerSelectMenu;

void PlayerSelectMenusCreate(
	PlayerSelectMenu *menu, int numPlayers, int player, const int playerUID,
	EventHandlers *handlers, GraphicsDevice *graphics, const NameGen *ng);
