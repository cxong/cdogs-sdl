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

#include "material.h"

#define NO_GUN_LABEL "(None)"
#define AMMO_LABEL "Ammo"
#define END_MENU_LABEL "(End)"
#define WEAPON_MENU_WIDTH 80
#define EQUIP_MENU_SLOT_HEIGHT 40
#define UTIL_MENU_SLOT_HEIGHT 12
#define WEAPON_MENU_MAX_ROWS 4
#define GUN_BG_W 40
#define GUN_BG_H 25
#define AMMO_BUTTON_BG_W 20
#define SCROLL_H 12
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

static const WeaponClass *GetGun(
	const CArray *weapons, const int idx, const int slot)
{
	if (slot >= MAX_WEAPONS)
	{
		return NULL;
	}
	int idx2 = 0;
	CA_FOREACH(const WeaponClass *, wc, *weapons)
	if ((*wc)->Type != SlotType(slot))
	{
		continue;
	}
	if (idx == idx2)
	{
		return *wc;
	}
	idx2++;
	CA_FOREACH_END()
	return NULL;
}

static const WeaponClass *GetSelectedGun(const WeaponMenu *menu)
{
	return GetGun(menu->weapons, menu->idx, menu->slot);
}
static int GetSelectedAmmo(const WeaponMenu *menu)
{
	const PlayerData *p = PlayerDataGetByUID(menu->PlayerUID);
	int idx = 0;
	for (int i = 0; i < AmmoGetNumClasses(&gAmmo); i++)
	{
		if (!PlayerUsesAmmo(p, i))
		{
			continue;
		}
		if (idx == menu->idx)
		{
			return idx;
		}
		idx++;
	}
	return -1;
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
	const PlayerData *pData = PlayerDataGetByUID(
		menu->PlayerUID);
	const WeaponClass *wc = GetSelectedGun(menu);
	if (wc)
	{
		return EquipCostDiff(wc, pData, menu->slot);
	}
	const int ammoId = GetSelectedAmmo(menu);
	if (ammoId >= 0)
	{
		const Ammo *ammo = AmmoGetById(&gAmmo, ammoId);
		const int amount = PlayerGetAmmoAmount(pData, ammoId);
		if (ammo->Max > amount)
		{
			return ammo->Price;
		}
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
		const int ammoId = GetSelectedAmmo(d);
		if (gCampaign.Setting.BuyAndSell && WeaponMenuSelectedCostDiff(d) > p->Totals.Score)
		{
			// Can't afford
			return;
		}
		d->SelectResult = WEAPON_MENU_SELECT;
		if (selectedGun != NULL)
		{
			SoundPlay(&gSoundDevice, selectedGun->SwitchSound);
		}
		else if (ammoId >= 0)
		{
			const Ammo *ammo = AmmoGetById(&gAmmo, ammoId);
			SoundPlay(&gSoundDevice, StrSound(ammo->Sound));
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
	int maxRow =
		MIN(selectedRow, MAX(0, DIV_ROUND_UP((int)menu->weaponIndices.size + 1, menu->cols) -
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

static void DrawAmmo(
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

static menu_t *CreateGunMenu(WeaponMenu *data);
void WeaponMenuCreate(
					  WeaponMenu *menu, const CArray *weapons, const CArray *weaponIsNew,
	const int playerUID,
							const int slot, const struct vec2i pos, const struct vec2i size,
	EventHandlers *handlers, GraphicsDevice *graphics)
{
	menu->PlayerUID = playerUID;
	menu->gunBGSprites = PicManagerGetSprites(&gPicManager, "hud/gun_bg");
	menu->idx = -1;
	// Get the weapon indices available for this slot
	CArrayInit(&menu->weaponIndices, sizeof(int));
	int idx = 0;
	CA_FOREACH(const WeaponClass *, wc, *weapons)
	if ((*wc)->Type != SlotType(slot))
	{
		continue;
	}
	CArrayPushBack(&menu->weaponIndices, &idx);
	idx++;
	CA_FOREACH_END()
	menu->weapons = weapons;
	menu->weaponIsNew = weaponIsNew;
	menu->slot = slot;

	const Pic *gunBG = CArrayGet(&menu->gunBGSprites->pics, 0);
	menu->cols = CLAMP(menu->size.x * 3 / (gunBG->size.x + 2) / 4, 2, 4);

	MenuSystemInit(&menu->ms, handlers, graphics, pos, size);
	menu->ms.align = MENU_ALIGN_LEFT;
	menu->ms.root = menu->ms.current = CreateGunMenu(menu);
	MenuSystemAddCustomDisplay(
							   &menu->ms, MenuDisplayPlayerControls, &menu->PlayerUID);
}
static void DrawAvailableMenu(
	const menu_t *menu, GraphicsDevice *g, const struct vec2i pos,
	const struct vec2i size, const void *data);
static int HandleInputGunMenu(int cmd, void *data);
static menu_t *CreateGunMenu(WeaponMenu *data)
{
	menu_t *menu =
		MenuCreateCustom("", DrawAvailableMenu, HandleInputGunMenu, data);

	MenuSetPostInputFunc(menu, WeaponSelect, data);

	return menu;
}
static void DrawGunMenu(
	const WeaponMenu *d, GraphicsDevice *g, const struct vec2i pos,
	const struct vec2i size);
static void DrawAmmoMenu(
	const WeaponMenu *d, GraphicsDevice *g, const struct vec2i pos,
	const struct vec2i size);
static void DrawAvailableMenu(
	const menu_t *menu, GraphicsDevice *g, const struct vec2i pos,
	const struct vec2i size, const void *data)
{
	UNUSED(menu);
	const WeaponMenu *d = data;
	if (d->slot < MAX_WEAPONS)
	{
		DrawGunMenu(d, g, pos, size);
	}
	else
	{
		// TODO: separate ammo menu module
		DrawAmmoMenu(d, g, pos, size);
	}
}
static void DrawGun(
	const WeaponMenu *data, GraphicsDevice *g, const int idx,
	const WeaponClass *wc, const bool isNew, const struct vec2i pos);
static void DrawGunMenu(
	const WeaponMenu *d, GraphicsDevice *g, const struct vec2i pos,
	const struct vec2i size)
{
	const int weaponsHeight = EQUIP_MENU_SLOT_HEIGHT * 2 + FontH();
	const int weaponsY = CENTER_Y(pos, size, weaponsHeight) - 12;
	const color_t color = d->Active ? colorWhite : colorGray;
	const struct vec2i scrollSize = svec2i(d->cols * GUN_BG_W - 2, SCROLL_H);
	bool scrollDown = false;

	// Draw guns: red if selected, yellow if equipped
	int idx = 0;
	CA_FOREACH(const WeaponClass *, wc, *d->weapons)
	if ((*wc)->Type != SlotType(d->slot))
	{
		continue;
	}
	const int row = idx / d->cols;
	if (row >= d->scroll && row < d->scroll + WEAPON_MENU_MAX_ROWS)
	{
		const bool *isNew = CArrayGet(d->weaponIsNew, _ca_index);
		DrawGun(d, g, idx, *wc, *isNew, svec2i(pos.x, pos.y + weaponsY));
	}
	idx++;
	if (idx / d->cols == d->scroll + WEAPON_MENU_MAX_ROWS)
	{
		scrollDown = true;
		break;
	}
	CA_FOREACH_END()

	// Draw scroll buttons
	const Pic *gradient = PicManagerGetPic(&gPicManager, "hud/gradient");
	if (d->scroll > 0)
	{
		const Pic *scrollPic = CArrayGet(&d->gunBGSprites->pics, 0);
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
		const Pic *scrollPic = CArrayGet(&d->gunBGSprites->pics, 0);
		const Rect2i scrollRect = Rect2iNew(
			svec2i(
				pos.x + 3, pos.y - 1 + weaponsY +
							   GUN_BG_H * WEAPON_MENU_MAX_ROWS - SCROLL_H),
			scrollSize);
		PicRender(
			gradient, g->gameWindow.renderer,
			svec2i(
				scrollRect.Pos.x + scrollSize.x / 2,
				pos.y - gradient->size.y / 2 + weaponsY +
					GUN_BG_H * WEAPON_MENU_MAX_ROWS - SCROLL_H -
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
		DrawGun(d, g, idx, NULL, false, svec2i(pos.x, pos.y + weaponsY));
	}
}
static void DrawGun(
	const WeaponMenu *data, GraphicsDevice *g, const int idx,
	const WeaponClass *wc, const bool isNew, const struct vec2i pos)
{
	const bool selected = data->idx == idx;
	const PlayerData *pData = PlayerDataGetByUID(data->PlayerUID);
	const bool equipped = wc && PlayerHasWeapon(pData, wc);
	const int bgSpriteIndex = (int)selected;
	const Pic *gunBG = CArrayGet(&data->gunBGSprites->pics, bgSpriteIndex);
	// Allow space for price if buy/sell enabled
	const int h = GUN_BG_H + (gCampaign.Setting.BuyAndSell ? FontH() : 0);
	const struct vec2i bgSize = svec2i(GUN_BG_W, h);
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
		const color_t bg = {0, 255, 255, 64};
		DrawRectangle(g, bgPos, bgSize, bg, true);
		color = colorRed;
	}
	else if (equipped)
	{
		color = colorYellow;
	}

	Draw9Slice(
		g, gunBG,
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

	DrawAmmo(g, pData, wc, mask, bgPos, svec2i(bgSize.x - 1, bgSize.y));

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

static void DrawAmmoMenuItem(
	const WeaponMenu *data, GraphicsDevice *g, const int idx,
	const Ammo *a, const struct vec2i pos);
static void DrawAmmoMenu(
	const WeaponMenu *d, GraphicsDevice *g, const struct vec2i pos,
	const struct vec2i size)
{
	const int ammoHeight = EQUIP_MENU_SLOT_HEIGHT * 2 + FontH();
	const int ammoY = CENTER_Y(pos, size, ammoHeight) - 12;
	const color_t color = d->Active ? colorWhite : colorGray;
	const struct vec2i scrollSize = svec2i(d->cols * GUN_BG_W - 2, SCROLL_H);
	bool scrollDown = false;
	const PlayerData *pData = PlayerDataGetByUID(d->PlayerUID);

	int idx = 0;
	for (int i = 0; i < AmmoGetNumClasses(&gAmmo); i++)
	{
		if (!PlayerUsesAmmo(pData, i))
		{
			continue;
		}
		const Ammo *ammo = AmmoGetById(&gAmmo, i);
		const int row = idx / d->cols;
		if (row >= d->scroll && row < d->scroll + WEAPON_MENU_MAX_ROWS)
		{
			DrawAmmoMenuItem(d, g, idx, ammo, svec2i(pos.x, pos.y + ammoY));
		}
		idx++;
		if (idx / d->cols == d->scroll + WEAPON_MENU_MAX_ROWS)
		{
			scrollDown = true;
			break;
		}
	}

	// Draw scroll buttons
	const Pic *gradient = PicManagerGetPic(&gPicManager, "hud/gradient");
	if (d->scroll > 0)
	{
		const Pic *scrollPic = CArrayGet(&d->gunBGSprites->pics, 0);
		const Rect2i scrollRect =
			Rect2iNew(svec2i(pos.x + 3, pos.y + 1 + ammoY), scrollSize);
		PicRender(
			gradient, g->gameWindow.renderer,
			svec2i(
				scrollRect.Pos.x + scrollSize.x / 2,
				pos.y + gradient->size.y / 2 + ammoY + scrollSize.y - 1),
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
		const Pic *scrollPic = CArrayGet(&d->gunBGSprites->pics, 0);
		const Rect2i scrollRect = Rect2iNew(
			svec2i(
				pos.x + 3, pos.y - 1 + ammoY +
							   GUN_BG_H * WEAPON_MENU_MAX_ROWS - SCROLL_H),
			scrollSize);
		PicRender(
			gradient, g->gameWindow.renderer,
			svec2i(
				scrollRect.Pos.x + scrollSize.x / 2,
				pos.y - gradient->size.y / 2 + ammoY +
					GUN_BG_H * WEAPON_MENU_MAX_ROWS - SCROLL_H -
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
}
static void DrawAmmoMenuItem(
	const WeaponMenu *data, GraphicsDevice *g, const int idx,
	const Ammo *a, const struct vec2i pos)
{
	const bool selected = data->idx == idx;
	const PlayerData *pData = PlayerDataGetByUID(data->PlayerUID);
	const int ammoAmount = PlayerGetAmmoAmount(pData, idx);
	const int bgSpriteIndex = (int)selected;
	const Pic *gunBG = CArrayGet(&data->gunBGSprites->pics, bgSpriteIndex);
	const struct vec2i bgSize = svec2i(CLAMP(data->size.x * 3 / 4, 40 * 2, 40 * 4), FontH() * 3 + 4);
	const struct vec2i bgPos = svec2i(
		pos.x, pos.y + (idx - data->scroll) * bgSize.y);
	// Disallow buy/sell if ammo is free
	const bool enabled =
		data->Active && a->Price > 0 && a->Price <= pData->Totals.Score;
	color_t color = enabled ? colorWhite : colorGray;
	const color_t mask = color;
	if (selected && data->Active)
	{
		const color_t bg = {0, 255, 255, 64};
		DrawRectangle(g, bgPos, bgSize, bg, true);
		color = colorRed;
	}
	
	// Draw:
	// <name>              <price>
	// <icon>         <buy> <sell>
	//              <amount>/<max>
	// With: amount/max coloured rectangle
	
	int x = bgPos.x + 4;
	int y = bgPos.y + FontH();
	const FontOpts fopts = {
		ALIGN_START, ALIGN_START, bgSize, svec2i(2, 2), color};
	
	// Sell/buy buttons
	const FontOpts foptsB = {
		ALIGN_CENTER, ALIGN_START, svec2i(AMMO_BUTTON_BG_W, FontH()), svec2i(2, 2), color};
	x = bgPos.x + bgSize.x - AMMO_BUTTON_BG_W - 2;
	const struct vec2i sellPos = svec2i(x, y);
	Draw9Slice(
		g, gunBG,
		Rect2iNew(
				  sellPos,
			svec2i(AMMO_BUTTON_BG_W, FontH() + 4)),
		3, 3, 3, 3, true, mask, SDL_FLIP_NONE);
	FontStrOpt("Sell", sellPos, foptsB);

	x -= AMMO_BUTTON_BG_W - 3;
	const struct vec2i buyPos = svec2i(x, y);
	Draw9Slice(
		g, gunBG,
		Rect2iNew(
				  buyPos,
			svec2i(AMMO_BUTTON_BG_W, FontH() + 4)),
		3, 3, 3, 3, true, mask, SDL_FLIP_NONE);
	FontStrOpt("Buy", buyPos, foptsB);

	y = bgPos.y;
	x = bgPos.x + 4;

	// Name
	FontStrOpt(a->Name, svec2i(x, y), fopts);

	// Price
	if (a->Price > 0)
	{
		const FontOpts foptsP = {
			ALIGN_END, ALIGN_START, bgSize, svec2i(8, 2),
			enabled ? (selected ? colorRed : colorGray) : colorDarkGray};
		char buf[256];
		sprintf(buf, "$%d", a->Price);
		FontStrOpt(buf, svec2i(x, y), foptsP);
	}

	y += FontH() * 2;
	x = bgPos.x + 4;

	// Ammo amount BG
	if (a->Max > 0 && ammoAmount > 0)
	{
		const color_t gaugeBG = AmmoIsLow(a, ammoAmount) ? colorRed : colorBlue;
		DrawRectangle(g, svec2i_add(svec2i(x, y), svec2i_one()), svec2i(ammoAmount * (bgSize.x - 8) / a->Max, FontH()), gaugeBG, true);
	}
	
	// Amount
	char buf[256];
	if (a->Max > 0)
	{
		sprintf(buf, "%d/%d", ammoAmount, a->Max);
	}
	else
	{
		sprintf(buf, "%d", ammoAmount);
	}
	const FontOpts foptsA = {
		ALIGN_END, ALIGN_START, bgSize, svec2i(8, 2), color};
	FontStrOpt(buf, svec2i(x, y), foptsA);
	
	y -= FontH();

	// Icon
	x = bgPos.x + 12;
	CPicDrawContext c = CPicDrawContextNew();
	const struct vec2i ammoPos = svec2i_subtract(
												 svec2i(x, bgPos.y + bgSize.y / 2),
		svec2i_scale_divide(CPicGetPic(&a->Pic, 0)->size, 2));
	CPicDraw(g, &a->Pic, ammoPos, &c);
}
static int HandleInputGunMenu(int cmd, void *data)
{
	WeaponMenu *d = data;
	PlayerData *p = PlayerDataGetByUID(d->PlayerUID);

	// Pre-select the equipped gun for the slot
	bool hasSelected = d->idx >= 0;
	if (!hasSelected)
	{
		d->idx = 0;
	}

	// Count total guns
	int numGuns = 0;
	CA_FOREACH(const WeaponClass *, wc, *d->weapons)
	if ((*wc)->Type != SlotType(d->slot))
	{
		continue;
	}
	if (*wc == p->guns[d->slot])
	{
		hasSelected = true;
	}
	if (!hasSelected)
	{
		d->idx++;
	}
	numGuns++;
	CA_FOREACH_END()

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
		if ((d->idx + d->cols) <
			DIV_ROUND_UP(numGuns + 1, d->cols) * d->cols)
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
	case WEAPON_MENU_SELECT:
		if (menu->slot < MAX_WEAPONS)
		{
			const WeaponClass *selectedGun = GetSelectedGun(menu);
			PlayerAddWeaponToSlot(p, selectedGun, menu->slot);
		}
		else	// TODO: ammo module
		{
			const int ammoId = GetSelectedAmmo(menu);
			PlayerAddAmmo(p, ammoId, AmmoGetById(&gAmmo, ammoId)->Amount, false);
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
