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
#include "weapon_menu.h"

#include <assert.h>

#include <cdogs/ai_coop.h>
#include <cdogs/draw/draw_actor.h>
#include <cdogs/draw/drawtools.h>
#include <cdogs/draw/nine_slice.h>
#include <cdogs/font.h>

#define NO_GUN_LABEL "(None)"
#define EQUIP_MENU_SLOT_HEIGHT 40
#define WEAPON_MENU_MAX_ROWS 4
#define GUN_BG_W 40
#define GUN_BG_H 25
#define SCROLL_H 12
#define SLOT_BORDER 3
#define AMMO_LEVEL_W 2

static bool InSlot(const WeaponClass *wc, const int slot)
{
	if (slot < MELEE_SLOT)
	{
		return wc->Type == GUNTYPE_NORMAL || wc->Type == GUNTYPE_MULTI;
	}
	else if (slot == MELEE_SLOT)
	{
		return wc->Type == GUNTYPE_MELEE;
	}
	return wc->Type == GUNTYPE_GRENADE;
}

static const WeaponClass *GetSelectedGun(const WeaponMenu *menu)
{
	if (menu->idx >= (int)menu->weaponIndices.size)
	{
		return NULL;
	}
	const int idx = *(int *)CArrayGet(&menu->weaponIndices, menu->idx);
	return *(const WeaponClass **)CArrayGet(menu->weapons, idx);
}

static int EquipCostDiff(
	const WeaponClass *wc, const PlayerData *p, const int slot)
{
	if (!gCampaign.Setting.BuyAndSell)
		return 0;
	if (PlayerHasWeapon(p, wc))
	{
		return 0;
	}
	const int cost1 = wc ? wc->Price : 0;
	const WeaponClass *wc2 = p->guns[slot];
	const int cost2 = wc2 ? wc2->Price : 0;
	return cost1 - cost2;
}

int WeaponMenuSelectedCostDiff(const WeaponMenu *menu)
{
	const PlayerData *pData = PlayerDataGetByUID(menu->PlayerUID);
	const WeaponClass *wc = GetSelectedGun(menu);
	if (wc)
	{
		return EquipCostDiff(wc, pData, menu->slot);
	}
	return 0;
}

static void WeaponSelect(menu_t *menu, int cmd, void *data)
{
	UNUSED(menu);
	WeaponMenu *d = data;

	if (cmd & CMD_BUTTON1)
	{
		// Add the selected item
		const PlayerData *p = PlayerDataGetByUID(d->PlayerUID);
		const WeaponClass *selectedGun = GetSelectedGun(d);
		if (gCampaign.Setting.BuyAndSell &&
			WeaponMenuSelectedCostDiff(d) > p->Totals.Score)
		{
			// Can't afford
			SoundPlay(&gSoundDevice, StrSound("ammo_none"));
			return;
		}
		d->SelectResult = WEAPON_MENU_SELECT;
		if (selectedGun != NULL)
		{
			SoundPlay(&gSoundDevice, selectedGun->SwitchSound);
		}
		else
		{
			MenuPlaySound(MENU_SOUND_SWITCH);
		}
	}
	else if (cmd & CMD_BUTTON2)
	{
		d->SelectResult = WEAPON_MENU_CANCEL;
		MenuPlaySound(MENU_SOUND_BACK);
	}
}

static int ClampScroll(const WeaponMenu *menu)
{
	// Update menu scroll based on selected gun
	const int selectedRow = menu->idx / menu->cols;
	const int lastRow = (int)menu->weaponIndices.size / menu->cols;
	int minRow = MAX(0, selectedRow - WEAPON_MENU_MAX_ROWS + 1);
	int maxRow = MIN(
		selectedRow,
		MAX(0, DIV_ROUND_UP((int)menu->weaponIndices.size + 1, menu->cols) -
				   WEAPON_MENU_MAX_ROWS));
	// If the selected row is the last row on screen, and we can still
	// scroll down (i.e. show the scroll down button), scroll down
	if (selectedRow - menu->scroll == WEAPON_MENU_MAX_ROWS - 1 &&
		selectedRow < lastRow)
	{
		minRow++;
	}
	else if (selectedRow == menu->scroll && selectedRow > 0)
	{
		maxRow--;
	}
	return CLAMP(menu->scroll, minRow, maxRow);
}

void DrawWeaponAmmo(
	GraphicsDevice *g, const PlayerData *p, const WeaponClass *wc,
	const color_t mask, const struct vec2i pos, const struct vec2i slotSize)
{
	if (!gCampaign.Setting.Ammo || !wc)
	{
		return;
	}

	CPicDrawContext c = CPicDrawContextNew();
	c.Mask = mask;
	const int numBarrels = WeaponClassNumBarrels(wc);
	for (int i = 0; i < numBarrels; i++)
	{
		const int ammoId = WC_BARREL_ATTR(*wc, AmmoId, i);
		if (ammoId < 0)
		{
			continue;
		}
		const Ammo *a = AmmoGetById(&gAmmo, ammoId);
		// Draw ammo level
		const int amount = PlayerGetAmmoAmount(p, ammoId);
		const int ammoMax = a->Max ? a->Max : amount;
		if (ammoMax > 0)
		{
			const int dx =
				slotSize.x - SLOT_BORDER - AMMO_LEVEL_W * (numBarrels - i);
			const int h = amount * (slotSize.y - 2 * SLOT_BORDER) / ammoMax;
			if (AmmoIsLow(a, amount))
			{
				DrawRectangle(
					g, svec2i_add(pos, svec2i(dx, SLOT_BORDER)),
					svec2i(AMMO_LEVEL_W, slotSize.y - 2 * SLOT_BORDER),
					ColorMult(colorRed, mask), true);
			}
			if (amount > 0)
			{
				DrawRectangle(
					g,
					svec2i_add(pos, svec2i(dx, slotSize.y - SLOT_BORDER - h)),
					svec2i(AMMO_LEVEL_W, h), ColorMult(colorBlue, mask), true);
			}
		}

		// Draw ammo icon
		CPicDraw(
			g, &a->Pic,
			svec2i_subtract(
				svec2i_add(pos, slotSize),
				svec2i(16 - (numBarrels - i) * 4, 18)),
			&c);
	}
}

static menu_t *CreateMenu(WeaponMenu *data);
void WeaponMenuCreate(
	WeaponMenu *menu, const CArray *weapons, const CArray *weaponIsNew,
	const int playerUID, const int slot, const struct vec2i pos,
	const struct vec2i size, EventHandlers *handlers, GraphicsDevice *graphics)
{
	menu->PlayerUID = playerUID;
	menu->menuBGSprites = PicManagerGetSprites(&gPicManager, "hud/gun_bg");
	menu->weapons = weapons;
	menu->weaponIsNew = weaponIsNew;
	menu->slot = slot;
	WeaponMenuReset(menu);

	const Pic *bg = CArrayGet(&menu->menuBGSprites->pics, 0);
	menu->cols = CLAMP(menu->size.x * 3 / (bg->size.x + 2) / 4, 2, 4);

	MenuSystemInit(&menu->ms, handlers, graphics, pos, size);
	menu->ms.align = MENU_ALIGN_LEFT;
	menu->ms.root = menu->ms.current = CreateMenu(menu);
	MenuSystemAddCustomDisplay(
		&menu->ms, MenuDisplayPlayerControls, &menu->PlayerUID);
}
static void DrawMenu(
	const menu_t *menu, GraphicsDevice *g, const struct vec2i pos,
	const struct vec2i size, const void *data);
static int HandleInputMenu(int cmd, void *data);
static menu_t *CreateMenu(WeaponMenu *data)
{
	menu_t *menu = MenuCreateCustom("", DrawMenu, HandleInputMenu, data);

	MenuSetPostInputFunc(menu, WeaponSelect, data);

	return menu;
}
static void DrawGun(
	const WeaponMenu *data, GraphicsDevice *g, const int idx,
	const WeaponClass *wc, const bool isNew, const struct vec2i pos,
	const struct vec2i bgSize);
static void DrawMenu(
	const menu_t *menu, GraphicsDevice *g, const struct vec2i pos,
	const struct vec2i size, const void *data)
{
	UNUSED(menu);
	const WeaponMenu *d = data;
	const int weaponsHeight = EQUIP_MENU_SLOT_HEIGHT * 2 + FontH();
	const int weaponsY = CENTER_Y(pos, size, weaponsHeight) - 12;
	const color_t color = d->Active ? colorWhite : colorGray;
	// Allow space for price if buy/sell enabled
	const int h = GUN_BG_H + (gCampaign.Setting.BuyAndSell ? FontH() : 0);
	const struct vec2i bgSize = svec2i(GUN_BG_W, h);
	const struct vec2i scrollSize = svec2i(d->cols * GUN_BG_W - 2, SCROLL_H);
	bool scrollDown = false;

	// Draw guns: red if selected, yellow if equipped
	CA_FOREACH(const int, idx, d->weaponIndices)
	const int row = _ca_index / d->cols;
	if (row >= d->scroll && row < d->scroll + WEAPON_MENU_MAX_ROWS)
	{
		const bool *isNew = CArrayGet(d->weaponIsNew, *idx);
		const WeaponClass **wc = CArrayGet(d->weapons, *idx);
		DrawGun(
			d, g, _ca_index, *wc, *isNew, svec2i(pos.x, pos.y + weaponsY), bgSize);
	}
	if (_ca_index / d->cols == d->scroll + WEAPON_MENU_MAX_ROWS)
	{
		scrollDown = true;
		break;
	}
	CA_FOREACH_END()

	// Draw scroll buttons
	const Pic *gradient = PicManagerGetPic(&gPicManager, "hud/gradient");
	if (d->scroll > 0)
	{
		const Pic *scrollPic = CArrayGet(&d->menuBGSprites->pics, 0);
		const Rect2i scrollRect =
			Rect2iNew(svec2i(pos.x + 3, pos.y + 1 + weaponsY), scrollSize);
		PicRender(
			gradient, g->gameWindow.renderer,
			svec2i(
				scrollRect.Pos.x + scrollSize.x / 2,
				pos.y + gradient->size.y / 2 + weaponsY + scrollSize.y - 1),
			colorBlack, 0, svec2((float)scrollSize.x, 1), SDL_FLIP_NONE,
			Rect2iZero());
		Draw9Slice(
			g, scrollPic, scrollRect, 3, 3, 3, 3, true, color, SDL_FLIP_NONE);
		FontOpts fopts = FontOptsNew();
		fopts.Area = scrollRect.Size;
		fopts.HAlign = ALIGN_CENTER;
		fopts.VAlign = ALIGN_CENTER;
		fopts.Mask = color;
		FontStrOpt(ARROW_UP, scrollRect.Pos, fopts);
	}
	if (scrollDown)
	{
		const Pic *scrollPic = CArrayGet(&d->menuBGSprites->pics, 0);
		const Rect2i scrollRect = Rect2iNew(
			svec2i(
				pos.x + 3, pos.y - 1 + weaponsY +
							   bgSize.y * WEAPON_MENU_MAX_ROWS - SCROLL_H),
			scrollSize);
		PicRender(
			gradient, g->gameWindow.renderer,
			svec2i(
				scrollRect.Pos.x + scrollSize.x / 2,
				pos.y - gradient->size.y / 2 + weaponsY +
					bgSize.y * WEAPON_MENU_MAX_ROWS - SCROLL_H -
					gradient->size.y / 2 + 1),
			colorBlack, 0, svec2((float)scrollSize.x, 1), SDL_FLIP_VERTICAL,
			Rect2iZero());
		Draw9Slice(
			g, scrollPic, scrollRect, 3, 3, 3, 3, true, color, SDL_FLIP_NONE);
		FontOpts fopts = FontOptsNew();
		fopts.Area = scrollRect.Size;
		fopts.HAlign = ALIGN_CENTER;
		fopts.VAlign = ALIGN_CENTER;
		fopts.Mask = color;
		FontStrOpt(ARROW_DOWN, scrollRect.Pos, fopts);
	}
	else
	{
		// Draw "none" gun which can be used to unequip this slot
		DrawGun(
			d, g, (int)d->weaponIndices.size, NULL, false, svec2i(pos.x, pos.y + weaponsY), bgSize);
	}
}
static void DrawGun(
	const WeaponMenu *data, GraphicsDevice *g, const int idx,
	const WeaponClass *wc, const bool isNew, const struct vec2i pos,
	const struct vec2i bgSize)
{
	const bool selected = data->idx == idx;
	const PlayerData *pData = PlayerDataGetByUID(data->PlayerUID);
	const bool equipped = wc && PlayerHasWeapon(pData, wc);
	const int bgSpriteIndex = (int)selected;
	const Pic *bg = CArrayGet(&data->menuBGSprites->pics, bgSpriteIndex);
	const struct vec2i bgPos = svec2i(
		pos.x + 2 + (idx % data->cols) * GUN_BG_W,
		pos.y + (idx / data->cols - data->scroll) * bgSize.y);
	const int costDiff = EquipCostDiff(wc, pData, data->slot);
	const bool enabled = data->Active && (!gCampaign.Setting.BuyAndSell ||
										  costDiff <= pData->Totals.Score);
	color_t color = enabled ? colorWhite : colorGray;
	const color_t mask = color;
	if (selected && data->Active)
	{
		const color_t cbg = {0, 255, 255, 64};
		DrawRectangle(g, bgPos, bgSize, cbg, true);
		color = colorRed;
	}
	else if (equipped)
	{
		color = colorYellow;
	}

	Draw9Slice(
		g, bg,
		Rect2iNew(
			svec2i(bgPos.x + 1, bgPos.y + 1),
			svec2i(GUN_BG_W - 2, bgSize.y - 2)),
		3, 3, 3, 3, true, mask, SDL_FLIP_NONE);

	// Draw icon at center of slot
	const Pic *gunIcon =
		wc ? wc->Icon : PicManagerGetPic(&gPicManager, "peashooter");
	const struct vec2i gunPos = svec2i_subtract(
		svec2i_add(bgPos, svec2i_scale_divide(bgSize, 2)),
		svec2i_scale_divide(gunIcon->size, 2));
	PicRender(
		gunIcon, g->gameWindow.renderer, gunPos, wc ? mask : colorBlack, 0,
		svec2_one(), SDL_FLIP_NONE, Rect2iZero());

	DrawWeaponAmmo(g, pData, wc, mask, bgPos, svec2i(bgSize.x - 1, bgSize.y));

	// Draw price
	if (gCampaign.Setting.BuyAndSell && wc && wc->Price != 0)
	{
		const FontOpts foptsP = {
			ALIGN_CENTER, ALIGN_START, bgSize, svec2i(2, 2),
			enabled ? (selected ? colorRed : colorGray) : colorDarkGray};
		char buf[256];
		sprintf(buf, "$%d", wc->Price);
		FontStrOpt(buf, bgPos, foptsP);
	}

	if (isNew)
	{
		DrawCross(g, svec2i(bgPos.x + GUN_BG_W - 6, bgPos.y + 5), colorGreen);
	}

	const FontOpts fopts = {
		ALIGN_CENTER, ALIGN_END, bgSize, svec2i(2, 2), color};
	const char *gunName = wc ? wc->name : NO_GUN_LABEL;
	FontStrOpt(gunName, bgPos, fopts);
}

static int HandleInputMenu(int cmd, void *data)
{
	WeaponMenu *d = data;
	PlayerData *p = PlayerDataGetByUID(d->PlayerUID);

	const int numGuns = (int)d->weaponIndices.size;

	if (cmd & CMD_BUTTON1)
	{
		if (gCampaign.Setting.BuyAndSell)
		{
			const int costDiff = WeaponMenuSelectedCostDiff(d);
			if (costDiff > 0 && costDiff > p->Totals.Score)
			{
				// Can't afford
				return 0;
			}
		}
		return 1;
	}
	else if (cmd & CMD_BUTTON2)
	{
		return 1;
	}
	else if (cmd & CMD_LEFT)
	{
		if ((d->idx % d->cols) > 0)
		{
			d->idx--;
			MenuPlaySound(MENU_SOUND_SWITCH);
		}
	}
	else if (cmd & CMD_RIGHT)
	{
		if ((d->idx % d->cols) < d->cols - 1 && numGuns > 0)
		{
			d->idx++;
			if (d->idx > numGuns)
			{
				d->idx = MAX(0, d->idx - d->cols);
			}
			MenuPlaySound(MENU_SOUND_SWITCH);
		}
	}
	else if (cmd & CMD_UP)
	{
		if (d->idx >= d->cols)
		{
			d->idx -= d->cols;
			MenuPlaySound(MENU_SOUND_SWITCH);
		}
	}
	else if (cmd & CMD_DOWN)
	{
		if ((d->idx + d->cols) < DIV_ROUND_UP(numGuns + 1, d->cols) * d->cols)
		{
			d->idx = MIN(numGuns, d->idx + d->cols);
			MenuPlaySound(MENU_SOUND_SWITCH);
		}
	}

	d->scroll = ClampScroll(d);

	return 0;
}

void WeaponMenuTerminate(WeaponMenu *menu)
{
	MenuSystemTerminate(&menu->ms);
	CArrayTerminate(&menu->weaponIndices);
}

void WeaponMenuReset(WeaponMenu *menu)
{
	const PlayerData *pData = PlayerDataGetByUID(menu->PlayerUID);
	menu->idx = 0;

	// Get the weapon indices available for this slot
	CArrayTerminate(&menu->weaponIndices);
	CArrayInit(&menu->weaponIndices, sizeof(int));
	
	// Add the equipped weapon's upgrades and downgrades first
	const WeaponClass *equipped = pData->guns[menu->slot];
	if (equipped != NULL)
	{
		CA_FOREACH(const WeaponClass *, wc, *menu->weapons)
		if (WeaponClassGetPrerequisite(*wc) == equipped)
		{
			CArrayPushBack(&menu->weaponIndices, &_ca_index);
		}
		CA_FOREACH_END()
		CA_FOREACH(const WeaponClass *, wc, *menu->weapons)
		if (WeaponClassGetPrerequisite(equipped) == *wc)
		{
			CArrayPushBack(&menu->weaponIndices, &_ca_index);
		}
		CA_FOREACH_END()
	}

	CA_FOREACH(const WeaponClass *, wc, *menu->weapons)
	if (!InSlot(*wc, menu->slot))
	{
		continue;
	}
	// Pre-select the equipped gun for the slot
	if (*wc == equipped)
	{
		menu->idx = (int)menu->weaponIndices.size;
	}
	else if (WeaponClassesAreRelated(*wc, equipped))
	{
		// Don't add other related weapons
		continue;
	}
	else if (WeaponClassGetPrerequisite(*wc) != NULL &&
		 !PlayerHasWeapon(pData, *wc))
	{
		// Don't add weapons that are upgrades and the player doesn't have
		continue;
	}
	CArrayPushBack(&menu->weaponIndices, &_ca_index);
	CA_FOREACH_END()
}
void WeaponMenuActivate(WeaponMenu *menu)
{
	menu->Active = true;
	menu->SelectResult = WEAPON_MENU_NONE;
}

void WeaponMenuUpdate(WeaponMenu *menu, const int cmd)
{
	PlayerData *p = PlayerDataGetByUID(menu->PlayerUID);
	MenuProcessCmd(&menu->ms, cmd);
	switch (menu->SelectResult)
	{
	case WEAPON_MENU_NONE:
		break;
	case WEAPON_MENU_SELECT: {
		const WeaponClass *selectedGun = GetSelectedGun(menu);
		PlayerAddWeaponToSlot(p, selectedGun, menu->slot);
	}
		// fallthrough
	case WEAPON_MENU_CANCEL:
		menu->ms.current = menu->ms.root;
		// Switch back to equip menu
		menu->Active = false;
		break;
	default:
		CASSERT(false, "unhandled case");
		break;
	}
}

void WeaponMenuDraw(const WeaponMenu *menu)
{
	MenuDisplay(&menu->ms);
}
