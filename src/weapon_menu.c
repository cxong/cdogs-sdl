/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.

	Copyright (c) 2013-2015, 2018 Cong Xu
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

#include <cdogs/ai_coop.h>
#include <cdogs/font.h>


static void WeaponSetEnabled(
	menu_t *menu, const GunDescription *g, const bool enable)
{
	CA_FOREACH(menu_t, subMenu, menu->u.normal.subMenus)
		if (strcmp(subMenu->name, g->name) == 0)
		{
			subMenu->isDisabled = !enable;
			break;
		}
	CA_FOREACH_END()
}

static const GunDescription *GetSelectedGun(const menu_t *menu)
{
	const menu_t *subMenu =
		CArrayGet(&menu->u.normal.subMenus, menu->u.normal.index);
	return StrGunDescription(subMenu->name);
}

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
		const GunDescription *selectedWeapon = GetSelectedGun(menu);
		for (int i = 0; i < p->weaponCount; i++)
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
		SoundPlay(&gSoundDevice, selectedWeapon->SwitchSound);

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
			WeaponSetEnabled(menu, removedWeapon, true);
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
	const struct vec2i pos, const struct vec2i size, const void *data)
{
	UNUSED(g);
	const WeaponMenuData *d = data;
	struct vec2i weaponsPos;
	struct vec2i maxTextSize = FontStrSize("LongestWeaponName");
	UNUSED(menu);
	struct vec2i dPos = pos;
	dPos.x -= size.x;	// move to left half of screen
	weaponsPos = svec2i(
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
				svec2i_add(weaponsPos, svec2i(0, i * FontH())));
		}
	}
}

static void AddGunMenuItems(
	MenuSystem *ms, const CArray *weapons, const struct vec2i menuSize,
	const bool isGrenade);
static void DisplayGunIcon(
	const menu_t *menu, GraphicsDevice *g, const struct vec2i pos, const struct vec2i size,
	const void *data);
static void DisplayDescriptionGunIcon(
	const menu_t *menu, GraphicsDevice *g, const struct vec2i pos, const struct vec2i size,
	const void *data);
void WeaponMenuCreate(
	WeaponMenu *menu,
	int numPlayers, int player, const int playerUID,
	EventHandlers *handlers, GraphicsDevice *graphics)
{
	MenuSystem *ms = &menu->ms;
	WeaponMenuData *data = &menu->data;
	struct vec2i pos, size;
	int w = graphics->cachedConfig.Res.x;
	int h = graphics->cachedConfig.Res.y;

	data->display.PlayerUID = playerUID;
	data->display.currentMenu = &ms->current;
	data->display.Dir = DIRECTION_DOWN;
	data->PlayerUID = playerUID;

	switch (numPlayers)
	{
	case 1:
		// Single menu, entire screen
		pos = svec2i(w / 2, 0);
		size = svec2i(w / 2, h);
		break;
	case 2:
		// Two menus, side by side
		pos = svec2i(player * w / 2 + w / 4, 0);
		size = svec2i(w / 4, h);
		break;
	case 3:
	case 4:
		// Four corners
		pos = svec2i((player & 1) * w / 2 + w / 4, (player / 2) * h / 2);
		size = svec2i(w / 4, h / 2);
		break;
	default:
		CASSERT(false, "not implemented");
		pos = svec2i(w / 2, 0);
		size = svec2i(w / 2, h);
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
	AddGunMenuItems(ms, &gMission.Weapons, size, false);
	MenuAddSubmenu(ms->root, MenuCreateSeparator("----------------"));
	AddGunMenuItems(ms, &gMission.Weapons, size, true);
	MenuSetPostInputFunc(ms->root, WeaponSelect, &data->display);
	// Disable menu items where the player already has the weapon
	PlayerData *pData = PlayerDataGetByUID(playerUID);
	for (int i = 0; i < pData->weaponCount; i++)
	{
		WeaponSetEnabled(ms->root, pData->weapons[i], false);
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

	// For AI players, pre-pick their weapons and go straight to menu end
	if (pData->inputDevice == INPUT_DEVICE_AI)
	{
		const int lastMenuIndex = (int)ms->root->u.normal.subMenus.size - 1;
		ms->current = CArrayGet(&ms->root->u.normal.subMenus, lastMenuIndex);
		AICoopSelectWeapons(pData, player, &gMission.Weapons);
	}
}
static void AddGunMenuItems(
	MenuSystem *ms, const CArray *weapons, const struct vec2i menuSize,
	const bool isGrenade)
{
	CA_FOREACH(const GunDescription *, g, *weapons)
		if ((*g)->IsGrenade != isGrenade)
		{
			continue;
		}
		menu_t *gunMenu;
		if ((*g)->Description != NULL)
		{
			// Gun description menu
			gunMenu = MenuCreateNormal((*g)->name, "", MENU_TYPE_NORMAL, 0);
			char *buf;
			CMALLOC(buf, strlen((*g)->Description) * 2);
			FontSplitLines((*g)->Description, buf, menuSize.x * 5 / 6);
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
	CA_FOREACH_END()
}
static void DisplayGunIcon(
	const menu_t *menu, GraphicsDevice *g, const struct vec2i pos, const struct vec2i size,
	const void *data)
{
	UNUSED(data);
	// Display a gun icon next to the currently selected weapon
	const GunDescription *gun = GetSelectedGun(menu);
	if (gun == NULL)
	{
		return;
	}
	const int menuItems = MIN(
		menu->u.normal.maxItems, (int)menu->u.normal.subMenus.size);
	const int textScroll =
		-menuItems * FontH() / 2 +
		(menu->u.normal.index - menu->u.normal.scroll) * FontH();
	const struct vec2i iconPos = svec2i(
		pos.x - gun->Icon->size.x - 4,
		pos.y + size.y / 2 + textScroll +
		(FontH() - gun->Icon->size.y) / 2);
	Blit(g, gun->Icon, iconPos);
}
static void DisplayDescriptionGunIcon(
	const menu_t *menu, GraphicsDevice *g, const struct vec2i pos, const struct vec2i size,
	const void *data)
{
	UNUSED(menu);
	UNUSED(size);
	const GunDescription *gun = data;
	// Display the gun just to the left of the description text
	const struct vec2i iconPos = svec2i(
		pos.x - gun->Icon->size.x - 4, pos.y + size.y / 2);
	Blit(g, gun->Icon, iconPos);
}

void WeaponMenuTerminate(WeaponMenu *menu)
{
	MenuSystemTerminate(&menu->ms);
}

void WeaponMenuUpdate(WeaponMenu *menu, const int cmd)
{
	const PlayerData *p = PlayerDataGetByUID(menu->data.PlayerUID);
	if (!MenuIsExit(&menu->ms))
	{
		MenuProcessCmd(&menu->ms, cmd);
	}
	else if (p->weaponCount == 0)
	{
		// Check exit condition; must have selected at least one weapon
		// Otherwise reset the current menu
		menu->ms.current = menu->ms.root;
	}
}

bool WeaponMenuIsDone(const WeaponMenu *menu)
{
	return strcmp(menu->ms.current->name, "(End)") == 0;
}

void WeaponMenuDraw(const WeaponMenu *menu)
{
	MenuDisplay(&menu->ms);
}
