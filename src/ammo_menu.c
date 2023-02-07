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
#define AMMO_ROW_H 10

static int GetSelectedAmmo(const AmmoMenu *menu)
{
	if (menu->idx / 2 >= (int)menu->ammoIds.size)
	{
		return -1;
	}
	return *(int *)CArrayGet(&menu->ammoIds, menu->idx / 2);
}

static bool CanBuy(const AmmoMenu *menu, const int ammoId)
{
	const PlayerData *pData = PlayerDataGetByUID(menu->PlayerUID);
	const Ammo *ammo = AmmoGetById(&gAmmo, ammoId);
	const int amount = PlayerGetAmmoAmount(pData, ammoId);
	return ammo->Max > amount && ammo->Price <= pData->Totals.Score;
}
static bool CanSell(const AmmoMenu *menu, const int ammoId)
{
	const PlayerData *pData = PlayerDataGetByUID(menu->PlayerUID);
	const int amount = PlayerGetAmmoAmount(pData, ammoId);
	return amount > 0;
}

int AmmoMenuSelectedCostDiff(const AmmoMenu *menu)
{
	const PlayerData *pData = PlayerDataGetByUID(menu->PlayerUID);
	const int ammoId = GetSelectedAmmo(menu);
	if (ammoId >= 0)
	{
		const Ammo *ammo = AmmoGetById(&gAmmo, ammoId);
		const int amount = PlayerGetAmmoAmount(pData, ammoId);
		const bool buy = (menu->idx & 1) == 0;
		if (buy && ammo->Max > amount)
		{
			return ammo->Price;
		}
		else if (amount > 0)
		{
			return -ammo->Price;
		}
	}
	return 0;
}

static void AmmoSelect(menu_t *menu, int cmd, void *data)
{
	UNUSED(menu);
	AmmoMenu *d = data;

	d->SelectResult = AMMO_MENU_NONE;
	if (cmd & CMD_BUTTON1)
	{
		const int ammoId = GetSelectedAmmo(d);
		const Ammo *ammo = AmmoGetById(&gAmmo, ammoId);
		const bool buy = (d->idx & 1) == 0;
		if (buy && CanBuy(d, ammoId))
		{
			SoundPlay(&gSoundDevice, StrSound(ammo->Sound));
			d->SelectResult = AMMO_MENU_SELECT;
		}
		else if (!buy && CanSell(d, ammoId))
		{
			// TODO: sell sound
			SoundPlay(&gSoundDevice, StrSound(ammo->Sound));
			d->SelectResult = AMMO_MENU_SELECT;
		}
		else
		{
			// TODO: can't buy/sell sound
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

static menu_t *CreateMenu(AmmoMenu *data);
void AmmoMenuCreate(
	AmmoMenu *menu, const int playerUID, const struct vec2i pos,
	const struct vec2i size, EventHandlers *handlers, GraphicsDevice *graphics)
{
	menu->PlayerUID = playerUID;
	menu->menuBGSprites = PicManagerGetSprites(&gPicManager, "hud/gun_bg");
	menu->idx = -1;
	AmmoMenuReset(menu);

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

	CA_FOREACH(const int, ammoId, d->ammoIds)
	const Ammo *ammo = AmmoGetById(&gAmmo, *ammoId);
	if (_ca_index >= d->scroll && _ca_index < d->scroll + AMMO_MENU_MAX_ROWS)
	{
		DrawAmmoMenuItem(
			d, g, _ca_index, ammo, svec2i(pos.x, pos.y + ammoY), bgSize);
	}
	if (_ca_index == d->scroll + AMMO_MENU_MAX_ROWS)
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
	const bool selected = data->idx / 2 == idx;
	const PlayerData *pData = PlayerDataGetByUID(data->PlayerUID);
	const int ammoId = *(int *)CArrayGet(&data->ammoIds, idx);
	const int ammoAmount = PlayerGetAmmoAmount(pData, ammoId);
	const int bgSpriteIndex = (int)selected;
	const Pic *bg = CArrayGet(&data->menuBGSprites->pics, bgSpriteIndex);
	const struct vec2i bgPos =
		svec2i(pos.x, pos.y + (idx - data->scroll) * bgSize.y);
	// Disallow buy/sell if ammo is free
	const bool enabled =
		data->Active && a->Price > 0 && a->Price <= pData->Totals.Score;
	color_t color = enabled ? colorWhite : colorGray;
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
	int y = bgPos.y + AMMO_ROW_H * 2;

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

	y = bgPos.y + AMMO_ROW_H;

	// Sell/buy buttons
	const bool sellSelected = selected && (data->idx & 1) == 1;
	const FontOpts foptsSell = {
		ALIGN_CENTER, ALIGN_START, svec2i(AMMO_BUTTON_BG_W, FontH()),
		svec2i(2, 2), sellSelected ? colorRed : colorGray};
	x = bgPos.x + bgSize.x - AMMO_BUTTON_BG_W - 2;
	const struct vec2i sellPos = svec2i(x, y);
	Draw9Slice(
		g, bg, Rect2iNew(sellPos, svec2i(AMMO_BUTTON_BG_W, FontH() + 4)), 3, 3,
		3, 3, true, CanSell(data, ammoId) ? colorRed : colorGray, SDL_FLIP_NONE);
	FontStrOpt("Sell", sellPos, foptsSell);

	x -= AMMO_BUTTON_BG_W + 3;
	const bool buySelected = selected && (data->idx & 1) == 0;
	const FontOpts foptsBuy = {
		ALIGN_CENTER, ALIGN_START, svec2i(AMMO_BUTTON_BG_W, FontH()),
		svec2i(2, 2), buySelected ? colorGreen : colorGray};
	const struct vec2i buyPos = svec2i(x, y);
	Draw9Slice(
		g, bg, Rect2iNew(buyPos, svec2i(AMMO_BUTTON_BG_W, FontH() + 4)), 3, 3,
		3, 3, true, CanBuy(data, ammoId) ? colorGreen : colorGray, SDL_FLIP_NONE);
	FontStrOpt("Buy", buyPos, foptsBuy);

	y = bgPos.y;
	x = bgPos.x + 4;

	const FontOpts fopts = {
		ALIGN_START, ALIGN_START, bgSize, svec2i(2, 2), color};

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

	y += AMMO_ROW_H * 2;
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

	y -= AMMO_ROW_H;

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

	const int numAmmo = (int)d->ammoIds.size;

	if (cmd & CMD_BUTTON1)
	{
		// Do nothing; don't switch away from menu
	}
	else if (cmd & CMD_BUTTON2)
	{
		return 1;
	}
	else if (cmd & CMD_LEFT)
	{
		if ((d->idx % 2) == 1)
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
		if (d->idx + 2 < numAmmo * 2 + 1)
		{
			d->idx = MIN(numAmmo * 2 + 1, d->idx + 2);
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

void AmmoMenuReset(AmmoMenu *menu)
{
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

void AmmoMenuActivate(AmmoMenu *menu)
{
	menu->Active = true;
	menu->SelectResult = AMMO_MENU_NONE;
}

bool AmmoMenuUpdate(AmmoMenu *menu, const int cmd)
{
	PlayerData *p = PlayerDataGetByUID(menu->PlayerUID);
	MenuProcessCmd(&menu->ms, cmd);
	switch (menu->SelectResult)
	{
	case AMMO_MENU_NONE:
		break;
	case AMMO_MENU_SELECT: {
		const int ammoId = GetSelectedAmmo(menu);
		const bool buy = (menu->idx & 1) == 0;
		const int amount = AmmoGetById(&gAmmo, ammoId)->Amount;
		PlayerAddAmmo(p, ammoId, buy ? amount : -amount, false);
		return true;
	}
	break;
	case AMMO_MENU_CANCEL:
		menu->ms.current = menu->ms.root;
		// Switch back to equip menu
		menu->Active = false;
		break;
	default:
		CASSERT(false, "unhandled case");
		break;
	}
	return false;
}

void AmmoMenuDraw(const AmmoMenu *menu)
{
	MenuDisplay(&menu->ms);
}
