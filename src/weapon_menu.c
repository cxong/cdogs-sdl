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

#define END_MENU_LABEL "(End)"


static void WeaponSetEnabled(
	menu_t *menu, const GunDescription *g, const bool enable)
{
	if (g == NULL)
	{
		return;
	}
	CA_FOREACH(menu_t, subMenu, menu->u.normal.subMenus)
		if (strcmp(subMenu->name, g->name) == 0)
		{
			MenuSetDisabled(subMenu, !enable);
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

	if (cmd & CMD_BUTTON1)
	{
		// Add the selected weapon to the slot
		d->SelectedGun = GetSelectedGun(menu);
		d->SelectResult = WEAPON_MENU_SELECT;
		if (d->SelectedGun == NULL)
		{
			MenuPlaySound(MENU_SOUND_SWITCH);
			return;
		}
		SoundPlay(&gSoundDevice, d->SelectedGun->SwitchSound);
	}
	else if (cmd & CMD_BUTTON2)
	{
		d->SelectedGun = GetSelectedGun(menu);
		d->SelectResult = WEAPON_MENU_CANCEL;
		MenuPlaySound(MENU_SOUND_BACK);
	}
}

static void AddEquippedMenuItem(
	menu_t *menu, const PlayerData *p, const int slot);
static void CreateEquippedWeaponsMenu(
	MenuSystem *ms, EventHandlers *handlers, GraphicsDevice *g,
	const struct vec2i pos, const struct vec2i size, const PlayerData *p)
{
	const struct vec2i maxTextSize = FontStrSize("LongestWeaponName");
	struct vec2i dPos = pos;
	dPos.x -= size.x;	// move to left half of screen
	const struct vec2i weaponsPos = svec2i(
		dPos.x + size.x * 3 / 4 - maxTextSize.x / 2,
		CENTER_Y(dPos, size, 0) + 2);
	const int numRows = MAX_GUNS + 1 + MAX_GRENADES + 1 + 1;
	const struct vec2i weaponsSize = svec2i(size.x, FontH() * numRows);

	MenuSystemInit(ms, handlers, g, weaponsPos, weaponsSize);
	ms->align = MENU_ALIGN_LEFT;
	ms->root = ms->current = MenuCreateNormal(
		"",
		"",
		MENU_TYPE_NORMAL,
		0);
	MenuAddExitType(ms, MENU_TYPE_RETURN);
	int i;
	for (i = 0; i < MAX_GUNS; i++)
	{
		AddEquippedMenuItem(ms->root, p, i);
	}
	MenuAddSubmenu(ms->root, MenuCreateSeparator("--Grenades--"));
	for (; i < MAX_GUNS + MAX_GRENADES; i++)
	{
		AddEquippedMenuItem(ms->root, p, i);
	}
	MenuAddSubmenu(
		ms->root, MenuCreateNormal(END_MENU_LABEL, x"", MENU_TYPE_NORMAL, 0));
}
static void SetEquippedMenuItemName(
	menu_t *menu, const PlayerData *p, const int slot)
{
	CFREE(menu->name);
	if (p->guns[slot] != NULL)
	{
		CSTRDUP(menu->name, p->guns[slot]->name);
	}
	else
	{
		CSTRDUP(menu->name, "(none)");
	}
}
static void AddEquippedMenuItem(
	menu_t *menu, const PlayerData *p, const int slot)
{
	menu_t *submenu = MenuCreateReturn("", slot);
	SetEquippedMenuItemName(submenu, p, slot);
	MenuAddSubmenu(menu, submenu);
}

static menu_t *CreateGunMenu(
	const CArray *weapons, const struct vec2i menuSize, const bool isGrenade,
	MenuDisplayPlayerData *display);
static void DisplayGunIcon(
	const menu_t *menu, GraphicsDevice *g, const struct vec2i pos,
	const struct vec2i size, const void *data);
static void DisplayDescriptionGunIcon(
	const menu_t *menu, GraphicsDevice *g, const struct vec2i pos,
	const struct vec2i size, const void *data);
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
	data->display.currentMenu = NULL;
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
	PlayerData *pData = PlayerDataGetByUID(playerUID);
	menu->gunMenu = CreateGunMenu(
		&gMission.Weapons, size, false, &data->display);
	menu->grenadeMenu = CreateGunMenu(
		&gMission.Weapons, size, true, &data->display);
	MenuSystemAddCustomDisplay(ms, MenuDisplayPlayer, &data->display);
	MenuSystemAddCustomDisplay(
		ms, MenuDisplayPlayerControls, &data->PlayerUID);

	// Create equipped weapons menu
	CreateEquippedWeaponsMenu(
		&menu->msEquip, handlers, graphics, pos, size, pData);

	// For AI players, pre-pick their weapons and go straight to menu end
	if (pData->inputDevice == INPUT_DEVICE_AI)
	{
		menu->msEquip.current =
			MenuGetSubmenuByName(menu->msEquip.root, END_MENU_LABEL);
		AICoopSelectWeapons(pData, player, &gMission.Weapons);
	}
}
static menu_t *CreateGunMenu(
	const CArray *weapons, const struct vec2i menuSize, const bool isGrenade,
	MenuDisplayPlayerData *display)
{
	menu_t *menu = MenuCreateNormal("", "", MENU_TYPE_NORMAL, 0);
	menu->u.normal.maxItems = 11;
	MenuAddSubmenu(menu, MenuCreate("(none)", MENU_TYPE_BASIC));
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
		MenuAddSubmenu(menu, gunMenu);
	CA_FOREACH_END()

	MenuSetPostInputFunc(menu, WeaponSelect, display);

	MenuSetCustomDisplay(menu, DisplayGunIcon, NULL);

	return menu;
}
static void DisplayGunIcon(
	const menu_t *menu, GraphicsDevice *g, const struct vec2i pos,
	const struct vec2i size, const void *data)
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
	const menu_t *menu, GraphicsDevice *g, const struct vec2i pos,
	const struct vec2i size, const void *data)
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
	PlayerData *p = PlayerDataGetByUID(menu->data.PlayerUID);
	if (menu->equipping)
	{
		MenuProcessCmd(&menu->ms, cmd);
		switch (menu->data.SelectResult)
		{
			case WEAPON_MENU_NONE:
				break;
			case WEAPON_MENU_SELECT:
				// Enable menu items due to changed equipment
				if (menu->data.SelectedGun == NULL)
				{
					WeaponSetEnabled(
						menu->data.EquipSlot < MAX_GUNS ?
						menu->gunMenu : menu->grenadeMenu,
						menu->data.SelectedGun, true);
				}
				WeaponSetEnabled(
					menu->data.EquipSlot < MAX_GUNS ?
					menu->gunMenu : menu->grenadeMenu,
					p->guns[menu->data.EquipSlot], true);
				p->guns[menu->data.EquipSlot] = menu->data.SelectedGun;
				// fallthrough
			case WEAPON_MENU_CANCEL:
				// Switch back to equip menu
				menu->equipping = false;
				menu->ms.current = NULL;
				// Update menu name based on new weapon equipped
				CA_FOREACH(
					menu_t, submenu, menu->msEquip.root->u.normal.subMenus)
					if (submenu->type == MENU_TYPE_RETURN &&
						submenu->u.returnCode == menu->data.EquipSlot)
					{
						SetEquippedMenuItemName(
							submenu, p, menu->data.EquipSlot);
						break;
					}
				CA_FOREACH_END()
				break;
			default:
				CASSERT(false, "unhandled case");
				break;
		}
		// Disable menu items where the player already has the weapon
		for (int i = 0; i < MAX_WEAPONS; i++)
		{
			WeaponSetEnabled(
				menu->data.EquipSlot < MAX_GUNS ?
				menu->gunMenu : menu->grenadeMenu,
				p->guns[i], false);
		}
	}
	else
	{
		MenuProcessCmd(&menu->msEquip, cmd);
		if (MenuIsExit(&menu->msEquip) &&
			strcmp(menu->msEquip.current->name, END_MENU_LABEL) != 0)
		{
			// Open weapon selection menu
			menu->equipping = true;
			menu->data.EquipSlot = menu->msEquip.current->u.returnCode;
			menu->ms.current =
				menu->data.EquipSlot < MAX_GUNS ?
				menu->gunMenu : menu->grenadeMenu;
			menu->msEquip.current = menu->msEquip.root;
			menu->data.SelectResult = WEAPON_MENU_NONE;
		}
	}

	// Disable "Done" if no weapons selected
	menu_t *endMenuItem =
		MenuGetSubmenuByName(menu->msEquip.root, END_MENU_LABEL);
	MenuSetDisabled(endMenuItem, PlayerGetNumWeapons(p) == 0);
}

bool WeaponMenuIsDone(const WeaponMenu *menu)
{
	return strcmp(menu->msEquip.current->name, END_MENU_LABEL) == 0;
}

void WeaponMenuDraw(const WeaponMenu *menu)
{
	MenuDisplay(&menu->ms);
	MenuDisplay(&menu->msEquip);
}
