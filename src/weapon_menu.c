/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.

	Copyright (c) 2013-2015, 2018, 2020-2021 Cong Xu
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

#include "material.h"

#define NO_GUN_LABEL "(None)"
#define END_MENU_LABEL "(End)"

static void WeaponSetSelected(
	menu_t *menu, const WeaponClass *wc, const bool selected)
{
	if (wc == NULL)
	{
		return;
	}
	CA_FOREACH(menu_t, subMenu, menu->u.normal.subMenus)
	if (strcmp(subMenu->name, wc->name) == 0)
	{
		subMenu->color = selected ? colorYellow : colorWhite;
		break;
	}
	CA_FOREACH_END()
}

static const WeaponClass *GetSelectedGun(const menu_t *menu)
{
	const menu_t *subMenu =
		CArrayGet(&menu->u.normal.subMenus, menu->u.normal.index);
	if (strcmp(subMenu->name, NO_GUN_LABEL) == 0)
	{
		return NULL;
	}
	return StrWeaponClass(subMenu->name);
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

static void DisplayGunIcon(
	const menu_t *menu, GraphicsDevice *g, const struct vec2i pos,
	const struct vec2i size, const void *data);
static void AddEquippedMenuItem(
	menu_t *menu, const PlayerData *p, const int slot, const bool enabled);
static void PostInputEquipMenu(menu_t *menu, int cmd, void *data)
{
	MenuDisplayPlayerData *d = data;

	// Rotate player using left/right keys
	const int dx = (cmd & CMD_LEFT) ? 1 : ((cmd & CMD_RIGHT) ? -1 : 0);
	if (dx != 0)
	{
		d->Dir = (direction_e)CLAMP_OPPOSITE(
			(int)d->Dir + dx, DIRECTION_UP, DIRECTION_UPLEFT);
		char buf[CDOGS_PATH_MAX];
		const PlayerData *p = PlayerDataGetByUID(d->PlayerUID);
		MatGetFootstepSound(p->Char.Class, NULL, buf);
		SoundPlay(&gSoundDevice, StrSound(buf));
	}

	// Display gun based on menu index
	d->GunIdx = MIN(menu->u.normal.index, MAX_WEAPONS);
}
static void CreateEquippedWeaponsMenu(
	MenuSystem *ms, EventHandlers *handlers, GraphicsDevice *g,
	const struct vec2i pos, const struct vec2i size, const PlayerData *p,
	const CArray *weapons, MenuDisplayPlayerData *display)
{
	const struct vec2i maxTextSize = FontStrSize("LongestWeaponName");
	struct vec2i dPos = pos;
	dPos.x -= size.x; // move to left half of screen
	const struct vec2i weaponsPos = svec2i(
		dPos.x + size.x * 3 / 4 - maxTextSize.x / 2,
		CENTER_Y(dPos, size, 0) + 2);
	const int numRows = MAX_GUNS + 1 + MAX_GRENADES + 1 + 1;
	const struct vec2i weaponsSize = svec2i(size.x, FontH() * numRows);

	MenuSystemInit(ms, handlers, g, weaponsPos, weaponsSize);
	ms->align = MENU_ALIGN_LEFT;
	ms->root = ms->current = MenuCreateNormal("", "", MENU_TYPE_NORMAL, 0);
	MenuAddExitType(ms, MENU_TYPE_RETURN);

	// Count number of guns/grenades, and disable extra menu items
	int numGuns = 0;
	int numGrenades = 0;
	CA_FOREACH(const WeaponClass *, wc, *weapons)
	if ((*wc)->Type == GUNTYPE_GRENADE)
	{
		numGrenades++;
	}
	else
	{
		numGuns++;
	}
	CA_FOREACH_END()
	int i;
	for (i = 0; i < MAX_GUNS; i++)
	{
		const bool submenuEnabled = i < numGuns;
		AddEquippedMenuItem(ms->root, p, i, submenuEnabled);
	}
	MenuAddSubmenu(ms->root, MenuCreateSeparator("--Grenades--"));
	for (; i < MAX_GUNS + MAX_GRENADES; i++)
	{
		const bool submenuEnabled = i - MAX_GUNS < numGrenades;
		AddEquippedMenuItem(ms->root, p, i, submenuEnabled);
	}
	MenuAddSubmenu(
		ms->root, MenuCreateNormal(END_MENU_LABEL, "", MENU_TYPE_NORMAL, 0));

	MenuSetCustomDisplay(ms->root, DisplayGunIcon, NULL);
	MenuSetPostInputFunc(ms->root, PostInputEquipMenu, display);

	// Pre-select the End menu
	ms->root->u.normal.index = (int)(ms->root->u.normal.subMenus.size - 1);
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
		CSTRDUP(menu->name, NO_GUN_LABEL);
	}
}
static void AddEquippedMenuItem(
	menu_t *menu, const PlayerData *p, const int slot, const bool enabled)
{
	menu_t *submenu = MenuCreateReturn("", slot);
	SetEquippedMenuItemName(submenu, p, slot);
	menu_t *addedMenu = MenuAddSubmenu(menu, submenu);
	addedMenu->isDisabled = !enabled;
}

static menu_t *CreateGunMenu(
	const CArray *weapons, const struct vec2i menuSize, const bool isGrenade,
	WeaponMenuData *data);
static void DisplayDescriptionGunIcon(
	const menu_t *menu, GraphicsDevice *g, const struct vec2i pos,
	const struct vec2i size, const void *data);
void WeaponMenuCreate(
	WeaponMenu *menu, const CArray *weapons, const int numPlayers,
	const int player, const int playerUID, EventHandlers *handlers,
	GraphicsDevice *graphics)
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
	menu->gunMenu = CreateGunMenu(weapons, size, false, data);
	menu->grenadeMenu = CreateGunMenu(weapons, size, true, data);
	MenuSystemAddCustomDisplay(ms, MenuDisplayPlayer, &data->display);
	MenuSystemAddCustomDisplay(
		ms, MenuDisplayPlayerControls, &data->PlayerUID);

	// Create equipped weapons menu
	CreateEquippedWeaponsMenu(
		&menu->msEquip, handlers, graphics, pos, size, pData, weapons,
		&data->display);

	// For AI players, pre-pick their weapons and go straight to menu end
	if (pData->inputDevice == INPUT_DEVICE_AI)
	{
		menu->msEquip.current =
			MenuGetSubmenuByName(menu->msEquip.root, END_MENU_LABEL);
		AICoopSelectWeapons(pData, player, weapons);
	}
}
static menu_t *CreateGunMenu(
	const CArray *weapons, const struct vec2i menuSize, const bool isGrenade,
	WeaponMenuData *data)
{
	menu_t *menu = MenuCreateNormal("", "", MENU_TYPE_NORMAL, 0);
	menu->u.normal.maxItems = 11;
	MenuAddSubmenu(menu, MenuCreate(NO_GUN_LABEL, MENU_TYPE_BASIC));
	CA_FOREACH(const WeaponClass *, wc, *weapons)
	if (((*wc)->Type == GUNTYPE_GRENADE) != isGrenade)
	{
		continue;
	}
	menu_t *gunMenu;
	if ((*wc)->Description != NULL)
	{
		// Gun description menu
		gunMenu = MenuCreateNormal((*wc)->name, "", MENU_TYPE_NORMAL, 0);
		char *buf;
		CMALLOC(buf, strlen((*wc)->Description) * 2);
		FontSplitLines((*wc)->Description, buf, menuSize.x * 5 / 6);
		MenuAddSubmenu(gunMenu, MenuCreateBack(buf));
		CFREE(buf);
		gunMenu->u.normal.isSubmenusAlt = true;
		MenuSetCustomDisplay(gunMenu, DisplayDescriptionGunIcon, *wc);
	}
	else
	{
		gunMenu = MenuCreate((*wc)->name, MENU_TYPE_BASIC);
	}
	MenuAddSubmenu(menu, gunMenu);
	CA_FOREACH_END()

	MenuSetPostInputFunc(menu, WeaponSelect, data);

	MenuSetCustomDisplay(menu, DisplayGunIcon, NULL);

	return menu;
}
static void DisplayGunIcon(
	const menu_t *menu, GraphicsDevice *g, const struct vec2i pos,
	const struct vec2i size, const void *data)
{
	UNUSED(data);
	if (menu->isDisabled)
	{
		return;
	}
	// Display a gun icon next to the currently selected weapon
	const WeaponClass *wc = GetSelectedGun(menu);
	if (wc == NULL)
	{
		return;
	}
	const int menuItems = MenuGetNumMenuItemsShown(menu);
	const int textScroll =
		-menuItems * FontH() / 2 +
		(menu->u.normal.index - menu->u.normal.scroll) * FontH();
	const struct vec2i iconPos = svec2i(
		pos.x - wc->Icon->size.x - 4,
		pos.y + size.y / 2 + textScroll + (FontH() - wc->Icon->size.y) / 2);
	PicRender(
		wc->Icon, g->gameWindow.renderer, iconPos, colorWhite, 0, svec2_one(),
		SDL_FLIP_NONE, Rect2iZero());
}
static void DisplayDescriptionGunIcon(
	const menu_t *menu, GraphicsDevice *g, const struct vec2i pos,
	const struct vec2i size, const void *data)
{
	UNUSED(menu);
	UNUSED(size);
	const WeaponClass *wc = data;
	// Display the gun just to the left of the description text
	const struct vec2i iconPos =
		svec2i(pos.x - wc->Icon->size.x - 4, pos.y + size.y / 2);
	PicRender(
		wc->Icon, g->gameWindow.renderer, iconPos, colorWhite, 0, svec2_one(),
		SDL_FLIP_NONE, Rect2iZero());
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
		menu_t *equipMenu = menu->data.EquipSlot < MAX_GUNS
								? menu->gunMenu
								: menu->grenadeMenu;
		switch (menu->data.SelectResult)
		{
		case WEAPON_MENU_NONE:
			break;
		case WEAPON_MENU_SELECT:
			// Deselect menu items due to changed equipment
			WeaponSetSelected(equipMenu, p->guns[menu->data.EquipSlot], false);
			// See if the selected gun is already equipped; if so swap it with
			// the current slot
			for (int i = 0; i < MAX_WEAPONS; i++)
			{
				if (p->guns[i] == menu->data.SelectedGun)
				{
					p->guns[i] = p->guns[menu->data.EquipSlot];
					break;
				}
			}
			p->guns[menu->data.EquipSlot] = menu->data.SelectedGun;
			// fallthrough
		case WEAPON_MENU_CANCEL:
			// Switch back to equip menu
			menu->equipping = false;
			// Update menu names based on weapons
			CA_FOREACH(menu_t, submenu, menu->msEquip.root->u.normal.subMenus)
			if (submenu->type == MENU_TYPE_RETURN)
			{
				SetEquippedMenuItemName(submenu, p, submenu->u.returnCode);
			}
			CA_FOREACH_END()
			break;
		default:
			CASSERT(false, "unhandled case");
			break;
		}
		// Select menu items where the player already has the weapon
		for (int i = 0; i < MAX_WEAPONS; i++)
		{
			WeaponSetSelected(equipMenu, p->guns[i], true);
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
			menu->msEquip.current = menu->msEquip.root;
			menu->data.SelectResult = WEAPON_MENU_NONE;
		}
	}

	// Display the gun/grenade menu based on which submenu is hovered
	if (!menu->equipping)
	{
		const menu_t *hoveredEquipMenu = CArrayGet(
			&menu->msEquip.root->u.normal.subMenus,
			menu->msEquip.root->u.normal.index);
		if (hoveredEquipMenu->type == MENU_TYPE_RETURN)
		{
			const int equipSlot = hoveredEquipMenu->u.returnCode;
			menu->ms.current =
				equipSlot < MAX_GUNS ? menu->gunMenu : menu->grenadeMenu;
		}
		else
		{
			menu->ms.current = NULL;
		}
	}

	// Disable the equip/weapon menus based on equipping state
	MenuSetDisabled(menu->msEquip.root, menu->equipping);
	if (menu->ms.current)
	{
		MenuSetDisabled(menu->ms.current, !menu->equipping);
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
