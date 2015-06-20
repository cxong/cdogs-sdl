/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.

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
#include "weapon_menu.h"

#include <assert.h>

#include <cdogs/font.h>


static void WeaponSelect(menu_t *menu, int cmd, void *data)
{
	WeaponMenuData *d = data;
	PlayerData *p = PlayerDataGetByUID(d->display.PlayerUID);
	const CArray *weapons = &gMission.Weapons;

	// Don't process if we're not selecting a weapon
	if ((cmd & CMD_BUTTON1) && menu->u.normal.index < (int)weapons->size)
	{
		// Add the selected weapon

		// Check that the weapon hasn't been chosen yet
		const GunDescription **selectedWeapon =
			CArrayGet(weapons, menu->u.normal.index);
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
		SoundPlay(&gSoundDevice, (*selectedWeapon)->SwitchSound);

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
			for (int i = 0; i < (int)weapons->size; i++)
			{
				const GunDescription **g = CArrayGet(weapons, i);
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
	const menu_t *menu, GraphicsDevice *g,
	const Vec2i pos, const Vec2i size, const void *data)
{
	UNUSED(g);
	const WeaponMenuData *d = data;
	Vec2i weaponsPos;
	Vec2i maxTextSize = FontStrSize("LongestWeaponName");
	UNUSED(menu);
	Vec2i dPos = pos;
	dPos.x -= size.x;	// move to left half of screen
	weaponsPos = Vec2iNew(
		dPos.x + size.x * 3 / 4 - maxTextSize.x / 2,
		CENTER_Y(dPos, size, 0) + 14);
	const PlayerData *p = PlayerDataGetByUID(d->display.PlayerUID);
	if (p->weaponCount == 0)
	{
		FontStr("None selected...", weaponsPos);
	}
	else
	{
		for (int i = 0; i < p->weaponCount; i++)
		{
			FontStr(
				p->weapons[i]->name,
				Vec2iAdd(weaponsPos, Vec2iNew(0, i * FontH())));
		}
	}
}

static void DisplayGunIcon(
	const menu_t *menu, GraphicsDevice *g, const Vec2i pos, const Vec2i size,
	const void *data);
static void DisplayDescriptionGunIcon(
	const menu_t *menu, GraphicsDevice *g, const Vec2i pos, const Vec2i size,
	const void *data);
void WeaponMenuCreate(
	WeaponMenu *menu,
	int numPlayers, int player, const int playerUID,
	EventHandlers *handlers, GraphicsDevice *graphics)
{
	MenuSystem *ms = &menu->ms;
	WeaponMenuData *data = &menu->data;
	Vec2i pos, size;
	int w = graphics->cachedConfig.Res.x;
	int h = graphics->cachedConfig.Res.y;

	data->display.PlayerUID = playerUID;
	data->display.currentMenu = &ms->current;
	data->PlayerUID = playerUID;

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
		CASSERT(false, "not implemented");
		pos = Vec2iNew(w / 2, 0);
		size = Vec2iNew(w / 2, h);
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
	const CArray *weapons = &gMission.Weapons;
	for (int i = 0; i < (int)weapons->size; i++)
	{
		const GunDescription **g = CArrayGet(weapons, i);
		menu_t *gunMenu;
		if ((*g)->Description != NULL)
		{
			// Gun description menu
			gunMenu = MenuCreateNormal((*g)->name, "", MENU_TYPE_NORMAL, 0);
			char *buf;
			CMALLOC(buf, strlen((*g)->Description) * 2);
			FontSplitLines((*g)->Description, buf, size.x * 5 / 6);
			MenuAddSubmenu(gunMenu, MenuCreateBack(buf));
			CFREE(buf);
			gunMenu->u.normal.isSubmenusAlt = true;
			MenuSetCustomDisplay(gunMenu, DisplayDescriptionGunIcon, *g);
		}
		else
		{
			gunMenu = MenuCreate((*g)->name, MENU_TYPE_BASIC);
		}
		MenuAddSubmenu(ms->root, gunMenu);
	}
	MenuSetPostInputFunc(ms->root, WeaponSelect, &data->display);
	// Disable menu items where the player already has the weapon
	PlayerData *pData = PlayerDataGetByUID(playerUID);
	for (int i = 0; i < pData->weaponCount; i++)
	{
		for (int j = 0; j < (int)weapons->size; j++)
		{
			const GunDescription **g = CArrayGet(weapons, j);
			if (pData->weapons[i] == *g)
			{
				MenuDisableSubmenu(ms->root, j);
			}
		}
	}
	MenuAddSubmenu(ms->root, MenuCreateSeparator(""));
	MenuAddSubmenu(
		ms->root, MenuCreateNormal("(End)", "", MENU_TYPE_NORMAL, 0));
	// Select "(End)"
	ms->root->u.normal.index = (int)ms->root->u.normal.subMenus.size - 1;

	// Disable "Done" if no weapons selected
	if (pData->weaponCount == 0)
	{
		MenuDisableSubmenu(ms->root, (int)ms->root->u.normal.subMenus.size - 1);
	}

	MenuSetCustomDisplay(ms->root, DisplayGunIcon, NULL);
	MenuSystemAddCustomDisplay(ms, MenuDisplayPlayer, &data->display);
	MenuSystemAddCustomDisplay(ms, DisplayEquippedWeapons, data);
	MenuSystemAddCustomDisplay(
		ms, MenuDisplayPlayerControls, &data->PlayerUID);
}
static void DisplayGunIcon(
	const menu_t *menu, GraphicsDevice *g, const Vec2i pos, const Vec2i size,
	const void *data)
{
	UNUSED(data);
	if (menu->u.normal.index >= (int)gMission.Weapons.size)
	{
		return;
	}
	// Display a gun icon next to the currently selected weapon
	const GunDescription **gun =
		CArrayGet(&gMission.Weapons, menu->u.normal.index);
	const int menuItems = MIN(
		menu->u.normal.maxItems, (int)menu->u.normal.subMenus.size);
	const int textScroll =
		-menuItems * FontH() / 2 +
		(menu->u.normal.index - menu->u.normal.scroll) * FontH();
	const Vec2i iconPos = Vec2iNew(
		pos.x - (*gun)->Icon->size.x - 4,
		pos.y + size.y / 2 + textScroll + (FontH() - (*gun)->Icon->size.y) / 2);
	Blit(g, (*gun)->Icon, iconPos);
}
static void DisplayDescriptionGunIcon(
	const menu_t *menu, GraphicsDevice *g, const Vec2i pos, const Vec2i size,
	const void *data)
{
	UNUSED(menu);
	UNUSED(size);
	const GunDescription *gun = data;
	// Display the gun just to the left of the description text
	const Vec2i iconPos = Vec2iNew(
		pos.x - gun->Icon->size.x - 4, pos.y + size.y / 2);
	Blit(g, gun->Icon, iconPos);
}
