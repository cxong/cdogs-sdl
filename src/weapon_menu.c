/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.

	Copyright (c) 2013-2014, Cong Xu
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
#include "weapon_menu.h"

#include <assert.h>

#include <cdogs/font.h>
#include <cdogs/text.h>


static void WeaponSelect(menu_t *menu, int cmd, void *data)
{
	WeaponMenuData *d = data;
	struct PlayerData *p = d->display.pData;

	// Don't process if we're not selecting a weapon
	if ((cmd & CMD_BUTTON1) &&
		menu->u.normal.index < (int)gMission.missionData->Weapons.size)
	{
		// Add the selected weapon

		// Check that the weapon hasn't been chosen yet
		const GunDescription **selectedWeapon = CArrayGet(
			&gMission.missionData->Weapons, menu->u.normal.index);
		for (int i = 0; i < p->weaponCount; i++)
		{
			if (p->weapons[i] == *selectedWeapon)
			{
				return;
			}
		}

		// Check that there are empty slots to add weapons
		if (p->weaponCount == MAX_WEAPONS)
		{
			return;
		}

		p->weapons[p->weaponCount] = *selectedWeapon;
		p->weaponCount++;
		MenuPlaySound(MENU_SOUND_ENTER);

		// Note: need to enable before disabling otherwise
		// menu index is not updated properly

		// Enable "Done" menu item
		MenuEnableSubmenu(menu, (int)menu->u.normal.subMenus.size - 1);

		// Disable this menu entry
		MenuDisableSubmenu(menu, menu->u.normal.index);
	}
	else if (cmd & CMD_BUTTON2)
	{
		// Remove a weapon
		if (p->weaponCount > 0)
		{
			p->weaponCount--;
			MenuPlaySound(MENU_SOUND_BACK);

			// Re-enable the menu entry for this weapon
			const GunDescription *removedWeapon = p->weapons[p->weaponCount];
			for (int i = 0; i < (int)gMission.missionData->Weapons.size; i++)
			{
				const GunDescription **g = CArrayGet(
					&gMission.missionData->Weapons, i);
				if (*g == removedWeapon)
				{
					MenuEnableSubmenu(menu, i);
					break;
				}
			}
		}

		// Disable "Done" if no weapons selected
		if (p->weaponCount == 0)
		{
			MenuDisableSubmenu(menu, (int)menu->u.normal.subMenus.size - 1);
		}
	}
}

static void DisplayEquippedWeapons(
	menu_t *menu, GraphicsDevice *g, Vec2i pos, Vec2i size, void *data)
{
	UNUSED(g);
	WeaponMenuData *d = data;
	Vec2i weaponsPos;
	Vec2i maxTextSize = TextGetSize("LongestWeaponName");
	UNUSED(menu);
	pos.x -= size.x;	// move to left half of screen
	weaponsPos = Vec2iNew(
		pos.x + size.x * 3 / 4 - maxTextSize.x / 2,
		CENTER_Y(pos, size, 0) + 14);
	if (d->display.pData->weaponCount == 0)
	{
		FontStr("None selected...", weaponsPos);
	}
	else
	{
		for (int i = 0; i < d->display.pData->weaponCount; i++)
		{
			FontStr(
				d->display.pData->weapons[i]->name,
				Vec2iAdd(weaponsPos, Vec2iNew(0, i * FontH())));
		}
	}
}

void WeaponMenuCreate(
	WeaponMenu *menu,
	int numPlayers, int player, Character *c, struct PlayerData *pData,
	EventHandlers *handlers, GraphicsDevice *graphics,
	InputConfig *inputConfig)
{
	MenuSystem *ms = &menu->ms;
	WeaponMenuData *data = &menu->data;
	Vec2i pos = Vec2iZero();
	Vec2i size = Vec2iZero();
	int w = graphics->cachedConfig.Res.x;
	int h = graphics->cachedConfig.Res.y;

	data->display.c = c;
	data->display.currentMenu = &ms->current;
	data->display.pData = pData;
	data->controls.inputConfig = inputConfig;
	data->controls.pData = pData;

	switch (numPlayers)
	{
	case 1:
		// Single menu, entire screen
		pos = Vec2iNew(w / 2, 0);
		size = Vec2iNew(w / 2, h);
		break;
	case 2:
		// Two menus, side by side
		pos = Vec2iNew(player * w / 2 + w / 4, 0);
		size = Vec2iNew(w / 4, h);
		break;
	case 3:
	case 4:
		// Four corners
		pos = Vec2iNew((player & 1) * w / 2 + w / 4, (player / 2) * h / 2);
		size = Vec2iNew(w / 4, h / 2);
		break;
	default:
		assert(0 && "not implemented");
		break;
	}
	MenuSystemInit(ms, handlers, graphics, pos, size);
	ms->align = MENU_ALIGN_LEFT;
	ms->root = ms->current = MenuCreateNormal(
		"",
		"",
		MENU_TYPE_NORMAL,
		0);
	ms->root->u.normal.maxItems = 11;
	for (int i = 0; i < (int)gMission.missionData->Weapons.size; i++)
	{
		const GunDescription **g = CArrayGet(
			&gMission.missionData->Weapons, i);
		MenuAddSubmenu(ms->root, MenuCreate((*g)->name, MENU_TYPE_BASIC));
	}
	MenuSetPostInputFunc(ms->root, WeaponSelect, &data->display);
	// Disable menu items where the player already has the weapon
	for (int i = 0; i < pData->weaponCount; i++)
	{
		for (int j = 0; j < (int)gMission.missionData->Weapons.size; j++)
		{
			const GunDescription **g = CArrayGet(
				&gMission.missionData->Weapons, j);
			if (pData->weapons[i] == *g)
			{
				MenuDisableSubmenu(ms->root, j);
			}
		}
	}
	MenuAddSubmenu(ms->root, MenuCreateSeparator(""));
	MenuAddSubmenu(
		ms->root, MenuCreateNormal("(End)", "", MENU_TYPE_NORMAL, 0));

	// Disable "Done" if no weapons selected
	if (pData->weaponCount == 0)
	{
		MenuDisableSubmenu(ms->root, (int)ms->root->u.normal.subMenus.size - 1);
	}

	MenuSystemAddCustomDisplay(ms, MenuDisplayPlayer, &data->display);
	MenuSystemAddCustomDisplay(ms, DisplayEquippedWeapons, data);
	MenuSystemAddCustomDisplay(ms, MenuDisplayPlayerControls, &data->controls);
}
