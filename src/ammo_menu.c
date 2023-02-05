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
#include "ammo_menu.h"

#include <assert.h>

#include <cdogs/ai_coop.h>
#include <cdogs/draw/draw_actor.h>
#include <cdogs/draw/drawtools.h>
#include <cdogs/draw/nine_slice.h>
#include <cdogs/font.h>

#define AMMO_MENU_WIDTH 80
#define EQUIP_MENU_SLOT_HEIGHT 40
#define AMMO_MENU_MAX_ROWS 4
#define AMMO_BUTTON_BG_W 20
#define SCROLL_H 12
#define SLOT_BORDER 3
#define AMMO_LEVEL_W 2

static int GetSelectedAmmo(const AmmoMenu *menu)
{
	if (menu->idx >= menu->ammoIds.size)
	{
		return -1;
	}
	return *(int *)CArrayGet(&menu->ammoIds, menu->idx);
}

int AmmoMenuSelectedCostDiff(const AmmoMenu *menu)
{
	const PlayerData *pData = PlayerDataGetByUID(menu->PlayerUID);
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

static void AmmoSelect(menu_t *menu, int cmd, void *data)
{
	UNUSED(menu);
	AmmoMenu *d = data;

	if (cmd & CMD_BUTTON1)
	{
		// Add the selected item
		const int ammoId = GetSelectedAmmo(d);
		d->SelectResult = AMMO_MENU_SELECT;
		if (ammoId >= 0)
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
		d->SelectResult = AMMO_MENU_CANCEL;
		MenuPlaySound(MENU_SOUND_BACK);
	}
}

static int ClampScroll(const AmmoMenu *menu)
{
	// Update menu scroll based on selected ammo
	const int selectedRow = menu->idx / 2;
	const int lastRow = (int)menu->ammoIds.size / 2;
	int minRow = MAX(0, selectedRow - AMMO_MENU_MAX_ROWS + 1);
	int maxRow = MIN(selectedRow, MAX(0, lastRow - AMMO_MENU_MAX_ROWS));
	// If the selected row is the last row on screen, and we can still
	// scroll down (i.e. show the scroll down button), scroll down
	if (selectedRow - menu->scroll == AMMO_MENU_MAX_ROWS - 1 &&
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

static menu_t *CreateMenu(AmmoMenu *data);
void AmmoMenuCreate(
	AmmoMenu *menu, const int playerUID, const struct vec2i pos,
	const struct vec2i size, EventHandlers *handlers, GraphicsDevice *graphics)
{
	menu->PlayerUID = playerUID;
	const PlayerData *pData = PlayerDataGetByUID(playerUID);
	menu->menuBGSprites = PicManagerGetSprites(&gPicManager, "hud/gun_bg");
	menu->idx = -1;
	// Get the ammo indices available for this slot
	CArrayInit(&menu->ammoIds, sizeof(int));
	for (int i = 0; i < AmmoGetNumClasses(&gAmmo); i++)
	{
		if (!PlayerUsesAmmo(pData, i))
		{
			continue;
		}
		CArrayPushBack(&menu->ammoIds, &i);
	}

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
static menu_t *CreateMenu(AmmoMenu *data)
{
	menu_t *menu = MenuCreateCustom("", DrawMenu, HandleInputMenu, data);

	MenuSetPostInputFunc(menu, AmmoSelect, data);

	return menu;
}

static void DrawAmmoMenuItem(
	const AmmoMenu *data, GraphicsDevice *g, const int idx, const Ammo *a,
	const struct vec2i pos, const struct vec2i bgSize);
static void DrawMenu(
	const menu_t *menu, GraphicsDevice *g, const struct vec2i pos,
	const struct vec2i size, const void *data)
{
	UNUSED(menu);
	const AmmoMenu *d = data;
	const int ammoHeight = EQUIP_MENU_SLOT_HEIGHT * 2 + FontH();
	const int ammoY = CENTER_Y(pos, size, ammoHeight) - 12;
	const color_t color = d->Active ? colorWhite : colorGray;
	const struct vec2i bgSize =
		svec2i(CLAMP(d->size.x * 3 / 4, 40 * 2, 40 * 4), FontH() * 3 + 4);
	const struct vec2i scrollSize = svec2i(bgSize.x, SCROLL_H);
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
		const int row = idx / 2;
		if (row >= d->scroll && row < d->scroll + AMMO_MENU_MAX_ROWS)
		{
			DrawAmmoMenuItem(
				d, g, idx, ammo, svec2i(pos.x, pos.y + ammoY), bgSize);
		}
		idx++;
		if (idx / 2 == d->scroll + AMMO_MENU_MAX_ROWS)
		{
			scrollDown = true;
			break;
		}
	}

	// Draw scroll buttons
	const Pic *gradient = PicManagerGetPic(&gPicManager, "hud/gradient");
	if (d->scroll > 0)
	{
		const Pic *scrollPic = CArrayGet(&d->menuBGSprites->pics, 0);
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
		const Pic *scrollPic = CArrayGet(&d->menuBGSprites->pics, 0);
		const Rect2i scrollRect = Rect2iNew(
			svec2i(
				pos.x + 3,
				pos.y - 1 + ammoY + bgSize.y * AMMO_MENU_MAX_ROWS - SCROLL_H),
			scrollSize);
		PicRender(
			gradient, g->gameWindow.renderer,
			svec2i(
				scrollRect.Pos.x + scrollSize.x / 2,
				pos.y - gradient->size.y / 2 + ammoY +
					bgSize.y * AMMO_MENU_MAX_ROWS - SCROLL_H -
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
	const AmmoMenu *data, GraphicsDevice *g, const int idx, const Ammo *a,
	const struct vec2i pos, const struct vec2i bgSize)
{
	const bool selected = data->idx == idx;
	const PlayerData *pData = PlayerDataGetByUID(data->PlayerUID);
	const int ammoAmount = PlayerGetAmmoAmount(pData, idx);
	const int bgSpriteIndex = (int)selected;
	const Pic *bg = CArrayGet(&data->menuBGSprites->pics, bgSpriteIndex);
	const struct vec2i bgPos =
		svec2i(pos.x, pos.y + (idx - data->scroll) * bgSize.y);
	// Disallow buy/sell if ammo is free
	const bool enabled =
		data->Active && a->Price > 0 && a->Price <= pData->Totals.Score;
	color_t color = enabled ? colorWhite : colorGray;
	const color_t mask = color;
	if (selected && data->Active)
	{
		const color_t cbg = {0, 255, 255, 64};
		DrawRectangle(g, bgPos, bgSize, cbg, true);
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
		ALIGN_CENTER, ALIGN_START, svec2i(AMMO_BUTTON_BG_W, FontH()),
		svec2i(2, 2), color};
	x = bgPos.x + bgSize.x - AMMO_BUTTON_BG_W - 2;
	const struct vec2i sellPos = svec2i(x, y);
	Draw9Slice(
		g, bg, Rect2iNew(sellPos, svec2i(AMMO_BUTTON_BG_W, FontH() + 4)), 3, 3,
		3, 3, true, mask, SDL_FLIP_NONE);
	FontStrOpt("Sell", sellPos, foptsB);

	x -= AMMO_BUTTON_BG_W - 3;
	const struct vec2i buyPos = svec2i(x, y);
	Draw9Slice(
		g, bg, Rect2iNew(buyPos, svec2i(AMMO_BUTTON_BG_W, FontH() + 4)), 3, 3,
		3, 3, true, mask, SDL_FLIP_NONE);
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
		const color_t gaugeBG =
			AmmoIsLow(a, ammoAmount) ? colorRed : colorBlue;
		DrawRectangle(
			g, svec2i_add(svec2i(x, y), svec2i_one()),
			svec2i(ammoAmount * (bgSize.x - 8) / a->Max, FontH()), gaugeBG,
			true);
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
static int HandleInputMenu(int cmd, void *data)
{
	AmmoMenu *d = data;
	PlayerData *p = PlayerDataGetByUID(d->PlayerUID);

	// Count total ammo
	int numAmmo = 0;
	for (int i = 0; i < AmmoGetNumClasses(&gAmmo); i++)
	{
		if (!PlayerUsesAmmo(p, i))
		{
			continue;
		}
		numAmmo++;
	}

	if (cmd & CMD_BUTTON1)
	{
		if (gCampaign.Setting.BuyAndSell)
		{
			// TODO: check buy/sell button
			const int costDiff = AmmoMenuSelectedCostDiff(d);
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
		if ((d->idx % 2) > 0)
		{
			d->idx--;
			MenuPlaySound(MENU_SOUND_SWITCH);
		}
	}
	else if (cmd & CMD_RIGHT)
	{
		if ((d->idx % 2) == 0)
		{
			d->idx++;
			MenuPlaySound(MENU_SOUND_SWITCH);
		}
	}
	else if (cmd & CMD_UP)
	{
		if (d->idx >= 2)
		{
			d->idx -= 2;
			MenuPlaySound(MENU_SOUND_SWITCH);
		}
	}
	else if (cmd & CMD_DOWN)
	{
		if ((d->idx + 2) < numAmmo + 2)
		{
			d->idx = MIN(numAmmo, d->idx + 2);
			MenuPlaySound(MENU_SOUND_SWITCH);
		}
	}

	d->scroll = ClampScroll(d);

	return 0;
}

void AmmoMenuTerminate(AmmoMenu *menu)
{
	MenuSystemTerminate(&menu->ms);
	CArrayTerminate(&menu->ammoIds);
}

void AmmoMenuActivate(AmmoMenu *menu)
{
	menu->Active = true;
	menu->SelectResult = AMMO_MENU_NONE;
	const PlayerData *pData = PlayerDataGetByUID(menu->PlayerUID);
	// Get the ammo indices available for this slot
	CArrayTerminate(&menu->ammoIds);
	CArrayInit(&menu->ammoIds, sizeof(int));
	for (int i = 0; i < AmmoGetNumClasses(&gAmmo); i++)
	{
		if (!PlayerUsesAmmo(pData, i))
		{
			continue;
		}
		CArrayPushBack(&menu->ammoIds, &i);
	}
	menu->idx = CLAMP(menu->idx, 0, (int)menu->ammoIds.size);
}

void AmmoMenuUpdate(AmmoMenu *menu, const int cmd)
{
	PlayerData *p = PlayerDataGetByUID(menu->PlayerUID);
	MenuProcessCmd(&menu->ms, cmd);
	switch (menu->SelectResult)
	{
	case AMMO_MENU_NONE:
		break;
	case AMMO_MENU_SELECT: {
		const int ammoId = GetSelectedAmmo(menu);
		PlayerAddAmmo(p, ammoId, AmmoGetById(&gAmmo, ammoId)->Amount, false);
	}
		// fallthrough
	case AMMO_MENU_CANCEL:
		menu->ms.current = menu->ms.root;
		// Switch back to equip menu
		menu->Active = false;
		break;
	default:
		CASSERT(false, "unhandled case");
		break;
	}
}

void AmmoMenuDraw(const AmmoMenu *menu)
{
	MenuDisplay(&menu->ms);
}
