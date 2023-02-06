/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.

	Copyright (c) 2013-2015, 2018, 2020-2023 Cong Xu
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
#include "equip_menu.h"

#include <assert.h>

#include <cdogs/ai_coop.h>
#include <cdogs/draw/draw_actor.h>
#include <cdogs/draw/drawtools.h>
#include <cdogs/draw/nine_slice.h>
#include <cdogs/font.h>

#include "material.h"

#define NO_GUN_LABEL "(None)"
#define AMMO_LABEL "Ammo"
#define END_MENU_LABEL "(End)"
#define EQUIP_MENU_WIDTH 80
#define EQUIP_MENU_SLOT_HEIGHT 40
#define UTIL_MENU_SLOT_HEIGHT 12
#define EQUIP_MENU_MAX_ROWS 4
#define SLOT_BORDER 3
#define AMMO_LEVEL_W 2

static GunType SlotType(const int slot)
{
	if (slot < MELEE_SLOT)
	{
		return GUNTYPE_NORMAL;
	}
	else if (slot == MELEE_SLOT)
	{
		return GUNTYPE_MELEE;
	}
	return GUNTYPE_GRENADE;
}

static int CountNumGuns(const EquipMenu *data, const int slot)
{
	if (slot >= MAX_WEAPONS)
	{
		return 0;
	}
	// Count total guns
	int numGuns = 0;
	CA_FOREACH(const WeaponClass *, wc, data->weapons)
	if ((*wc)->Type != SlotType(slot))
	{
		continue;
	}
	numGuns++;
	CA_FOREACH_END()
	return numGuns;
}

static bool IsSlotDisabled(const EquipMenu *data, const int slot)
{
	if (slot < 0 || slot > data->endSlot)
	{
		return true;
	}
	if (slot < MAX_WEAPONS)
	{
		return CountNumGuns(data, slot) == 0;
	}
	const PlayerData *pData = PlayerDataGetByUID(data->PlayerUID);
	// Disable end option if nothing equipped
	if (slot == data->endSlot)
	{
		return PlayerGetNumWeapons(pData) == 0;
	}
	if (slot == data->ammoSlot)
	{
		return !PlayerUsesAnyAmmo(pData);
	}
	// TODO: util menus
	return false;
}

static void DrawEquipSlot(
	const EquipMenu *data, GraphicsDevice *g, const int slot,
	const char *label, const struct vec2i pos, const FontAlign align)
{
	const bool selected = data->slot == slot;
	color_t color = data->equipping ? colorDarkGray : colorWhite;
	color_t mask = color;
	// Allow space for price if buy/sell enabled
	const int h =
		EQUIP_MENU_SLOT_HEIGHT + (gCampaign.Setting.BuyAndSell ? FontH() : 0);
	if (selected)
	{
		if (data->equipping)
		{
			color = colorYellow;
			mask = colorWhite;
		}
		else
		{
			const color_t bg = {0, 255, 255, 64};
			// Add 1px padding
			const struct vec2i bgPos = svec2i_subtract(pos, svec2i_one());
			const struct vec2i bgSize =
				svec2i(EQUIP_MENU_WIDTH / 2 + 2, h + 2);
			DrawRectangle(g, bgPos, bgSize, bg, true);

			color = colorRed;
		}
	}
	if (IsSlotDisabled(data, slot))
	{
		color = mask = colorDarkGray;
	}
	const PlayerData *pData = PlayerDataGetByUID(data->PlayerUID);

	int y = pos.y;

	const int bgSpriteIndex = (slot & 1) + (int)selected * 2;
	const Pic *slotBG = CArrayGet(&data->slotBGSprites->pics, bgSpriteIndex);
	const struct vec2i bgPos = svec2i(pos.x, y);
	Draw9Slice(
		g, slotBG,
		Rect2iNew(
			svec2i(bgPos.x + 1, bgPos.y + 1),
			svec2i(EQUIP_MENU_WIDTH / 2, h - 2)),
		11, (slot & 1) ? 4 : 13, 12, (slot & 1) ? 13 : 4, true, mask,
		SDL_FLIP_NONE);

	const FontOpts fopts = {
		align, ALIGN_START, svec2i(EQUIP_MENU_WIDTH / 2, FontH()),
		svec2i(3, 1), color};
	FontStrOpt(label, pos, fopts);

	const Pic *gunIcon = pData->guns[slot]
							 ? pData->guns[slot]->Icon
							 : PicManagerGetPic(&gPicManager, "peashooter");
	// Draw icon at center of slot
	const struct vec2i slotSize = svec2i(EQUIP_MENU_WIDTH / 2, h);
	const struct vec2i gunPos = svec2i_subtract(
		svec2i_add(bgPos, svec2i_scale_divide(slotSize, 2)),
		svec2i_scale_divide(gunIcon->size, 2));
	PicRender(
		gunIcon, g->gameWindow.renderer, gunPos,
		pData->guns[slot] ? mask : colorBlack, 0, svec2_one(), SDL_FLIP_NONE,
		Rect2iZero());

	DrawWeaponAmmo(
		g, pData, pData->guns[slot], mask, svec2i(bgPos.x, bgPos.y + FontH()),
		svec2i(slotSize.x, slotSize.y - 2 * FontH() - 1));

	if (data->SlotHasNew[slot])
	{
		DrawCross(
			g, svec2i(pos.x + EQUIP_MENU_WIDTH / 2 - 6, y + 13), colorGreen);
	}

	// Draw price
	if (gCampaign.Setting.BuyAndSell && data->equipping && pData->guns[slot] &&
		pData->guns[slot]->Price != 0)
	{
		const FontOpts foptsP = {
			ALIGN_CENTER, ALIGN_START, svec2i(EQUIP_MENU_WIDTH / 2, FontH()),
			svec2i(2, 2), selected ? colorGray : colorDarkGray};
		char buf[256];
		sprintf(buf, "$%d", pData->guns[slot]->Price);
		FontStrOpt(buf, svec2i_add(pos, svec2i(0, FontH() + 2)), foptsP);
	}

	y += h - FontH() - 1;
	const char *gunName =
		pData->guns[slot] ? pData->guns[slot]->name : NO_GUN_LABEL;
	const FontOpts fopts2 = {
		align, ALIGN_START, svec2i(EQUIP_MENU_WIDTH / 2, FontH()),
		svec2i(3, 0), color};
	FontStrOpt(gunName, svec2i(pos.x, y), fopts2);
}
static void DrawEquipMenu(
	const menu_t *menu, GraphicsDevice *g, const struct vec2i pos,
	const struct vec2i size, const void *data)
{
	UNUSED(menu);
	const EquipMenu *d = data;
	const PlayerData *pData = PlayerDataGetByUID(d->PlayerUID);
	// Allow space for price if buy/sell enabled
	const int h =
		EQUIP_MENU_SLOT_HEIGHT + (gCampaign.Setting.BuyAndSell ? FontH() : 0);
	int y = pos.y;

	// Draw player cash
	if (gCampaign.Setting.BuyAndSell)
	{
		const struct vec2i fpos = AnimatedCounterDraw(
			&d->Cash, svec2i_add(pos, svec2i(0, -FontH() - 2)));
		// Draw cost difference to buy selected item
		if (d->equipping)
		{
			int costDiff = 0;
			if (d->slot < MAX_WEAPONS)
			{
				const WeaponMenu *weaponMenu = &d->weaponMenus[d->slot];
				costDiff = WeaponMenuSelectedCostDiff(weaponMenu);
			}
			else if (d->slot == d->ammoSlot)
			{
				costDiff = AmmoMenuSelectedCostDiff(&d->ammoMenu);
			}
			if (costDiff != 0)
			{
				// Draw price diff
				const FontOpts foptsD = {
					ALIGN_START, ALIGN_START, svec2i_zero(), svec2i_zero(),
					costDiff > 0 ? colorRed : colorGreen};
				char buf[256];
				sprintf(buf, "%s%d", costDiff < 0 ? "+" : "", -costDiff);
				FontStrOpt(buf, fpos, foptsD);
			}
		}
	}

	DrawEquipSlot(d, g, 0, "I", svec2i(pos.x, y), ALIGN_START);
	DrawEquipSlot(
		d, g, 1, "II", svec2i(pos.x + EQUIP_MENU_WIDTH / 2, y), ALIGN_END);
	y += h;
	DrawEquipSlot(d, g, MELEE_SLOT, "Melee", svec2i(pos.x, y), ALIGN_START);
	DrawEquipSlot(
		d, g, 3, "Bombs", svec2i(pos.x + EQUIP_MENU_WIDTH / 2, y), ALIGN_END);
	y += h + 8;
	if (d->ammoSlot >= 0)
	{
		const bool selected = d->slot == d->ammoSlot;
		const bool disabled = !PlayerUsesAnyAmmo(pData);
		DisplayMenuItem(
			g,
			Rect2iNew(
				svec2i(CENTER_X(pos, size, FontStrSize(AMMO_LABEL).x), y),
				FontStrSize(AMMO_LABEL)),
			AMMO_LABEL, selected, disabled, colorWhite);
		y += UTIL_MENU_SLOT_HEIGHT;
	}

	y += 8;
	const WeaponClass *gun = NULL;
	if (d->display.GunIdx >= 0 && d->display.GunIdx < MAX_WEAPONS)
	{
		gun = pData->guns[d->display.GunIdx];
	}
	DrawCharacterSimple(
		&pData->Char, svec2i(pos.x + EQUIP_MENU_WIDTH / 2, pos.y + h + 8),
		DIRECTION_DOWN, false, false, gun);

	const bool endDisabled = IsSlotDisabled(d, d->endSlot) || d->equipping;
	const bool endSelected = d->slot == d->endSlot;
	DisplayMenuItem(
		g,
		Rect2iNew(
			svec2i(CENTER_X(pos, size, FontStrSize(END_MENU_LABEL).x), y),
			FontStrSize(END_MENU_LABEL)),
		END_MENU_LABEL, endSelected, endDisabled, colorWhite);
}
static int HandleInputEquipMenu(int cmd, void *data)
{
	EquipMenu *d = data;
	PlayerData *p = PlayerDataGetByUID(d->PlayerUID);

	int newSlot = d->slot;
	if (cmd & CMD_BUTTON1)
	{
		MenuPlaySound(MENU_SOUND_ENTER);
		return 1;
	}
	else if (cmd & CMD_BUTTON2)
	{
		if (d->slot < MAX_WEAPONS)
		{
			PlayerRemoveWeapon(p, d->slot);
			MenuPlaySound(MENU_SOUND_SWITCH);
			AnimatedCounterReset(&d->Cash, p->Totals.Score);
		}
	}
	else if (cmd & CMD_LEFT)
	{
		if (d->slot < MAX_WEAPONS && (d->slot & 1) == 1)
		{
			newSlot--;
			if (IsSlotDisabled(data, newSlot))
			{
				// Try switching up-left or down-left
				if (!IsSlotDisabled(data, newSlot - 2))
				{
					newSlot -= 2;
				}
				else if (!IsSlotDisabled(data, newSlot + 2))
				{
					newSlot += 2;
				}
			}
		}
	}
	else if (cmd & CMD_RIGHT)
	{
		if (d->slot < MAX_WEAPONS && (d->slot & 1) == 0)
		{
			newSlot++;
			if (IsSlotDisabled(data, newSlot))
			{
				// Try switching up-right or down-right
				if (!IsSlotDisabled(data, newSlot - 2))
				{
					newSlot -= 2;
				}
				else if (!IsSlotDisabled(data, newSlot + 2))
				{
					newSlot += 2;
				}
			}
		}
	}
	else if (cmd & CMD_UP)
	{
		if (d->slot >= MAX_WEAPONS)
		{
			// Keep going back until we find an enabled slot
			do
			{
				newSlot--;
			} while (newSlot > 0 && IsSlotDisabled(data, newSlot));
		}
		else if (d->slot >= 2)
		{
			newSlot -= 2;
			if (IsSlotDisabled(data, newSlot))
			{
				// Try switching to the other slot above (up-left or up-right),
				// or skip the row and keep going back until an enabled slot
				if (!IsSlotDisabled(data, newSlot ^ 1))
				{
					newSlot ^= 1;
				}
				else
				{
					do
					{
						newSlot--;
					} while (newSlot > 0 && IsSlotDisabled(data, newSlot));
				}
			}
		}
	}
	else if (cmd & CMD_DOWN)
	{
		if (d->slot < d->endSlot)
		{
			if (d->slot < MAX_WEAPONS)
			{
				newSlot = MIN(d->endSlot, d->slot + 2);
				if (IsSlotDisabled(data, newSlot))
				{
					// Try switching to the other slot below (down-left or
					// down-right), or skip the row and keep going forward
					// until an enabled slot
					if (!IsSlotDisabled(data, newSlot ^ 1))
					{
						newSlot ^= 1;
					}
					else
					{
						do
						{
							newSlot++;
						} while (newSlot < d->endSlot &&
								 IsSlotDisabled(data, newSlot));
					}
				}
			}
			else
			{
				do
				{
					newSlot++;
				} while (newSlot < d->endSlot &&
						 IsSlotDisabled(data, newSlot));
			}
		}
	}

	if (newSlot != d->slot && !IsSlotDisabled(d, newSlot))
	{
		d->slot = newSlot;
		MenuPlaySound(MENU_SOUND_SWITCH);
	}

	// Display gun based on menu index
	if (d->slot < MAX_WEAPONS)
	{
		d->display.GunIdx = d->slot;
	}

	return 0;
}

static bool HasWeapon(const CArray *weapons, const WeaponClass *wc);
void EquipMenuCreate(
	EquipMenu *menu, const CArray *weapons, const CArray *prevWeapons,
	const int numPlayers, const int player, const int playerUID,
	EventHandlers *handlers, GraphicsDevice *graphics)
{
	struct vec2i pos;
	int w = graphics->cachedConfig.Res.x;
	int h = graphics->cachedConfig.Res.y;

	menu->display.PlayerUID = playerUID;
	menu->display.currentMenu = NULL;
	menu->display.Dir = DIRECTION_DOWN;
	menu->PlayerUID = playerUID;
	PlayerData *pData = PlayerDataGetByUID(playerUID);
	menu->Cash = AnimatedCounterNew("Cash: $", pData->Totals.Score);
	menu->Cash.incRatio = 0.2f;
	menu->slotBGSprites =
		PicManagerGetSprites(&gPicManager, "hud/gun_slot_bg");
	CArrayCopy(&menu->weapons, weapons);
	CArrayInit(&menu->weaponIsNew, sizeof(bool));
	CA_FOREACH(const WeaponClass *, wc, menu->weapons)
	const bool isNew = !HasWeapon(prevWeapons, *wc);
	CArrayPushBack(&menu->weaponIsNew, &isNew);
	if (isNew)
	{
		if ((*wc)->Type == GUNTYPE_GRENADE)
		{
			menu->SlotHasNew[MAX_GUNS] = true;
		}
		else if ((*wc)->Type == GUNTYPE_MELEE)
		{
			menu->SlotHasNew[MELEE_SLOT] = true;
		}
		else
		{
			for (int i = 0; i < MELEE_SLOT; i++)
			{
				menu->SlotHasNew[i] = true;
			}
		}
	}
	CA_FOREACH_END()

	switch (numPlayers)
	{
	case 1:
		// Single menu, entire screen
		pos = svec2i(w / 2, 0);
		menu->size = svec2i(w / 2, h);
		break;
	case 2:
		// Two menus, side by side
		pos = svec2i(player * w / 2 + w / 4, 0);
		menu->size = svec2i(w / 4, h);
		break;
	case 3:
	case 4:
		// Four corners
		pos = svec2i((player & 1) * w / 2 + w / 4, (player / 2) * h / 2);
		menu->size = svec2i(w / 4, h / 2);
		break;
	default:
		CASSERT(false, "not implemented");
		pos = svec2i(w / 2, 0);
		menu->size = svec2i(w / 2, h);
		break;
	}
	// Check how many util menu items there are
	menu->endSlot = MAX_WEAPONS;
	if (gCampaign.Setting.BuyAndSell)
	{
		if (gCampaign.Setting.Ammo)
		{
			menu->ammoSlot = menu->endSlot;
			menu->endSlot++;
			// Auto-refill free ammo
			for (int i = 0; i < AmmoGetNumClasses(&gAmmo); i++)
			{
				const Ammo *ammo = AmmoGetById(&gAmmo, i);
				if (ammo->Price == 0)
				{
					PlayerAddAmmo(pData, i, ammo->Max, true);
				}
			}
		}
	}

	// Create equipped weapons menu
	const struct vec2i weaponsSize =
		svec2i(EQUIP_MENU_WIDTH, EQUIP_MENU_SLOT_HEIGHT * 2 + FontH());
	const struct vec2i weaponsPos = svec2i(
		pos.x - EQUIP_MENU_WIDTH,
		CENTER_Y(pos, menu->size, weaponsSize.y) - 12);

	MenuSystemInit(&menu->ms, handlers, graphics, weaponsPos, weaponsSize);
	menu->ms.align = MENU_ALIGN_LEFT;
	MenuAddExitType(&menu->ms, MENU_TYPE_RETURN);

	// Count number of each gun type, and disable extra menu items
	int numGuns = 0;
	int numMelee = 0;
	int numGrenades = 0;
	CA_FOREACH(const WeaponClass *, wc, menu->weapons)
	if ((*wc)->Type == GUNTYPE_GRENADE)
	{
		numGrenades++;
	}
	else if ((*wc)->Type == GUNTYPE_MELEE)
	{
		numMelee++;
	}
	else
	{
		numGuns++;
	}
	CA_FOREACH_END()
	for (int i = 0; i < MELEE_SLOT; i++)
	{
		menu->EquipEnabled[i] = i < numGuns;
	}
	menu->EquipEnabled[MELEE_SLOT] = numMelee > 0;
	for (int i = 0; i < MAX_GRENADES; i++)
	{
		menu->EquipEnabled[i + MAX_GUNS] = i < numGrenades;
	}

	// Pre-select the End menu
	menu->slot = menu->endSlot;
	menu->ms.root = menu->ms.current =
		MenuCreateCustom("", DrawEquipMenu, HandleInputEquipMenu, menu);

	// For AI players, pre-pick their weapons and go straight to menu end
	if (pData->inputDevice == INPUT_DEVICE_AI)
	{
		menu->slot = menu->endSlot;
		menu->equipping = false;
		AICoopSelectWeapons(pData, player, weapons);
	}
	else
	{
		for (int i = 0; i < MAX_WEAPONS; i++)
		{
			WeaponMenuCreate(
				&menu->weaponMenus[i], &menu->weapons, &menu->weaponIsNew,
				playerUID, i, pos, menu->size, handlers, graphics);
		}
		AmmoMenuCreate(
			&menu->ammoMenu, playerUID, pos, menu->size, handlers, graphics);
	}
}
static bool HasWeapon(const CArray *weapons, const WeaponClass *wc)
{
	if (weapons == NULL)
	{
		return true;
	}
	CA_FOREACH(const WeaponClass *, wc2, *weapons)
	if (*wc2 == wc)
	{
		return true;
	}
	CA_FOREACH_END()
	return false;
}

void EquipMenuTerminate(EquipMenu *menu)
{
	for (int i = 0; i < MAX_WEAPONS; i++)
	{
		WeaponMenuTerminate(&menu->weaponMenus[i]);
	}
	AmmoMenuTerminate(&menu->ammoMenu);
	MenuSystemTerminate(&menu->ms);
	CArrayTerminate(&menu->weapons);
	CArrayTerminate(&menu->weaponIsNew);
	AnimatedCounterTerminate(&menu->Cash);
}

void EquipMenuUpdate(EquipMenu *menu, const int cmd)
{
	AnimatedCounterUpdate(&menu->Cash, 1);
	PlayerData *p = PlayerDataGetByUID(menu->PlayerUID);
	if (menu->slot < MAX_WEAPONS && menu->weaponMenus[menu->slot].Active)
	{
		WeaponMenuUpdate(&menu->weaponMenus[menu->slot], cmd);
		menu->equipping = menu->weaponMenus[menu->slot].Active;
		// If weapons changed, reset ammo menu
		if (!menu->equipping)
		{
			AmmoMenuReset(&menu->ammoMenu);
		}
	}
	else if (menu->slot == menu->ammoSlot && menu->ammoMenu.Active)
	{
		AmmoMenuUpdate(&menu->ammoMenu, cmd);
		menu->equipping = menu->ammoMenu.Active;
	}
	else
	{
		MenuProcessCmd(&menu->ms, cmd);
		if (menu->slot < menu->endSlot)
		{
			if (MenuIsExit(&menu->ms))
			{
				// Open weapon selection menu
				menu->equipping = true;
				if (menu->slot < MAX_WEAPONS)
				{
					WeaponMenuActivate(&menu->weaponMenus[menu->slot]);
				}
				else if (menu->slot == menu->ammoSlot)
				{
					AmmoMenuActivate(&menu->ammoMenu);
				}
				else
				{
					CASSERT(false, "unknown menu slot");
				}
				menu->ms.current = menu->ms.root;
			}
		}
	}

	// Disable menu based on equipping state
	MenuSetDisabled(menu->ms.root, menu->equipping);
	if (!menu->equipping)
	{
		AnimatedCounterReset(&menu->Cash, p->Totals.Score);
	}
}

bool EquipMenuIsDone(const EquipMenu *menu)
{
	return menu->ms.current == NULL && !menu->equipping &&
		   menu->slot == menu->endSlot;
}

void EquipMenuDraw(const EquipMenu *menu)
{
	MenuDisplay(&menu->ms);
	if (menu->slot < MAX_WEAPONS)
	{
		WeaponMenuDraw(&menu->weaponMenus[menu->slot]);
	}
	else if (menu->slot == menu->ammoSlot)
	{
		AmmoMenuDraw(&menu->ammoMenu);
	}
}
