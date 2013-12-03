/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.

	Copyright (c) 2013, Cong Xu
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

#include <cdogs/text.h>


static void WeaponSelect(menu_t *menu, int cmd, void *data)
{
	WeaponMenuData *d = data;
	struct PlayerData *p = d->display.pData;
	int i;

	// Don't process if we're not selecting a weapon
	if ((cmd & CMD_BUTTON1) && menu->u.normal.index < gMission.weaponCount)
	{
		// Add the selected weapon

		// Check that the weapon hasn't been chosen yet
		gun_e selectedWeapon = gMission.availableWeapons[menu->u.normal.index];
		for (i = 0; i < p->weaponCount; i++)
		{
			if (p->weapons[i] == selectedWeapon)
			{
				return;
			}
		}

		// Check that there are empty slots to add weapons
		if (p->weaponCount == MAX_WEAPONS)
		{
			return;
		}

		p->weapons[p->weaponCount] = selectedWeapon;
		p->weaponCount++;

		// Note: need to enable before disabling otherwise
		// menu index is not updated properly

		// Enable "Done" menu item
		MenuEnableSubmenu(menu, menu->u.normal.numSubMenus - 1);

		// Disable this menu entry
		MenuDisableSubmenu(menu, menu->u.normal.index);
	}
	else if (cmd & CMD_BUTTON2)
	{
		// Remove a weapon
		if (p->weaponCount > 0)
		{
			gun_e removedWeapon;
			p->weaponCount--;

			// Re-enable the menu entry for this weapon
			removedWeapon = p->weapons[p->weaponCount];
			for (i = 0; i < gMission.weaponCount; i++)
			{
				if (gMission.availableWeapons[i] == removedWeapon)
				{
					MenuEnableSubmenu(menu, i);
					break;
				}
			}
		}

		// Disable "Done" if no weapons selected
		if (p->weaponCount == 0)
		{
			MenuDisableSubmenu(menu, menu->u.normal.numSubMenus - 1);
		}
	}
}

static void DisplayEquippedWeapons(
	GraphicsDevice *g, Vec2i pos, Vec2i size, void *data)
{
	WeaponMenuData *d = data;
	Vec2i weaponsPos;
	Vec2i maxTextSize = TextGetSize("LongestWeaponName");
	pos.x -= size.x;	// move to left half of screen
	weaponsPos = Vec2iNew(
		pos.x + size.x * 3 / 4 - maxTextSize.x / 2,
		CENTER_Y(pos, size, 0) + 14);
	if (d->display.pData->weaponCount == 0)
	{
		DrawTextString("None selected...", g, weaponsPos);
	}
	else
	{
		int i;
		for (i = 0; i < d->display.pData->weaponCount; i++)
		{
			DrawTextString(
				gGunDescriptions[d->display.pData->weapons[i]].name,
				g,
				Vec2iAdd(weaponsPos, Vec2iNew(0, i * CDogsTextHeight())));
		}
	}
}

void WeaponMenuCreate(
	WeaponMenu *menu,
	int numPlayers, int player, Character *c, struct PlayerData *pData,
	InputDevices *input, GraphicsDevice *graphics, InputConfig *inputConfig)
{
	MenuSystem *ms = &menu->ms;
	WeaponMenuData *data = &menu->data;
	Vec2i pos = Vec2iZero();
	Vec2i size = Vec2iZero();
	int w = graphics->cachedConfig.ResolutionWidth;
	int h = graphics->cachedConfig.ResolutionHeight;
	int i;

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
	MenuSystemInit(ms, input, graphics, pos, size);
	ms->align = MENU_ALIGN_LEFT;
	ms->root = ms->current = MenuCreateNormal(
		"",
		"",
		MENU_TYPE_NORMAL,
		0);
	for (i = 0; i < gMission.weaponCount; i++)
	{
		const char *gunName =
			gGunDescriptions[gMission.availableWeapons[i]].name;
		MenuAddSubmenu(ms->root, MenuCreate(gunName, MENU_TYPE_BASIC));
	}
	MenuSetPostInputFunc(ms->root, WeaponSelect, &data->display);
	// Disable menu items where the player already has the weapon
	for (i = 0; i < pData->weaponCount; i++)
	{
		int j;
		for (j = 0; j < gMission.weaponCount; j++)
		{
			if (pData->weapons[i] == gMission.availableWeapons[j])
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
		MenuDisableSubmenu(ms->root, ms->root->u.normal.numSubMenus - 1);
	}

	MenuAddExitType(ms, MENU_TYPE_RETURN);
	MenuSystemAddCustomDisplay(ms, MenuDisplayPlayer, &data->display);
	MenuSystemAddCustomDisplay(ms, DisplayEquippedWeapons, data);
	MenuSystemAddCustomDisplay(ms, MenuDisplayPlayerControls, &data->controls);
}
