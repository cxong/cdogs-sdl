/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.

	Copyright (c) 2023 Cong Xu
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
#include "util_menu.h"

#include <assert.h>

#include <cdogs/draw/draw_actor.h>
#include <cdogs/draw/drawtools.h>
#include <cdogs/draw/nine_slice.h>
#include <cdogs/font.h>

#define MENU_WIDTH 80
#define EQUIP_MENU_SLOT_HEIGHT 40
#define MENU_MAX_ROWS 4
#define BUTTON_BG_W 20
#define SCROLL_H 12
#define SLOT_BORDER 3
#define LEVEL_W 2
#define ROW_H 10
#define HP_DELTA 5
#define HP_PRICE 2000
#define LIFE_PRICE 4000

typedef enum
{
	OPTION_HP,
	OPTION_LIVES,
	OPTION_COUNT
} Option;

static Option GetSelectedOption(const UtilMenu *menu)
{
	return MIN(menu->idx / 2, OPTION_COUNT);
}

int UtilMenuSelectedCostDiff(const UtilMenu *menu)
{
	const PlayerData *pData = PlayerDataGetByUID(menu->PlayerUID);
	const Option option = GetSelectedOption(menu);
	if (option < OPTION_COUNT)
	{
		int amount = 0;
		int max = 0;
		int price = 0;
		switch (option)
		{
		case OPTION_HP:
			amount = pData->HP;
			max = CampaignGetMaxHP(&gCampaign);
			price = HP_PRICE;
			break;
		case OPTION_LIVES:
			amount = pData->Lives;
			max = CampaignGetMaxLives(&gCampaign);
			price = LIFE_PRICE;
			break;
		default:
			CASSERT(false, "unknown option");
			break;
		}
		const bool buy = (menu->idx & 1) == 0;
		if (buy && max > amount)
		{
			return price;
		}
		else if (amount > 0)
		{
			return -price;
		}
	}
	return 0;
}

static void OnSelect(menu_t *menu, int cmd, void *data)
{
	UNUSED(menu);
	UtilMenu *d = data;
	const PlayerData *pData = PlayerDataGetByUID(d->PlayerUID);

	d->SelectResult = UTIL_MENU_NONE;
	if (Button1(cmd))
	{
		const Option option = GetSelectedOption(d);
		const bool buy = (d->idx & 1) == 0;
		int amount = 0;
		int max = 0;
		int delta = 0;
		int price = 0;
		const char *sound = NULL;
		switch (option)
		{
		case OPTION_HP:
			amount = pData->HP;
			max = CampaignGetMaxHP(&gCampaign);
			delta = HP_DELTA;
			price = HP_PRICE;
			sound = "health";
			break;
		case OPTION_LIVES:
			amount = pData->Lives;
			max = CampaignGetMaxLives(&gCampaign);
			delta = 1;
			price = LIFE_PRICE;
			sound = "spawn";
			break;
		case OPTION_COUNT:
			d->SelectResult = UTIL_MENU_CANCEL;
			MenuPlaySound(MENU_SOUND_BACK);
			break;
		default:
			CASSERT(false, "unknown option");
			break;
		}
		if (buy && amount < max && price <= pData->Totals.Score)
		{
			SoundPlay(&gSoundDevice, StrSound(sound));
			d->SelectResult = UTIL_MENU_SELECT;
		}
		else if (!buy && amount > delta)
		{
			SoundPlay(&gSoundDevice, StrSound(sound));
			d->SelectResult = UTIL_MENU_SELECT;
		}
		else if (option != OPTION_COUNT)
		{
			SoundPlay(&gSoundDevice, StrSound("ammo_none"));
		}
	}
	else if (Button2(cmd))
	{
		d->SelectResult = UTIL_MENU_CANCEL;
		MenuPlaySound(MENU_SOUND_BACK);
	}
}

static menu_t *CreateMenu(UtilMenu *data);
void UtilMenuCreate(
	UtilMenu *menu, const int playerUID, const struct vec2i pos,
	const struct vec2i size, EventHandlers *handlers, GraphicsDevice *graphics)
{
	menu->PlayerUID = playerUID;
	menu->buttonBG = PicManagerGetPic(&gPicManager, "hud/button_bg");
	menu->idx = -1;
	UtilMenuReset(menu);

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
static menu_t *CreateMenu(UtilMenu *data)
{
	menu_t *menu = MenuCreateCustom("", DrawMenu, HandleInputMenu, data);

	MenuSetPostInputFunc(menu, OnSelect, data);

	return menu;
}

static void DrawUtilMenuItem(
	const UtilMenu *data, GraphicsDevice *g, const Option option,
	const struct vec2i pos, const struct vec2i bgSize);
static void DrawMenu(
	const menu_t *menu, GraphicsDevice *g, const struct vec2i pos,
	const struct vec2i size, const void *data)
{
	UNUSED(menu);
	const UtilMenu *d = data;
	const int ammoHeight = EQUIP_MENU_SLOT_HEIGHT * 2 + FontH();
	const int ammoY = CENTER_Y(pos, size, ammoHeight) - 12;
	const struct vec2i bgSize =
		svec2i(CLAMP(d->size.x * 3 / 4, 40 * 2, 40 * 4), FontH() * 3 + 4);

	for (Option option = OPTION_HP; option < OPTION_COUNT; option++)
	{
		DrawUtilMenuItem(d, g, option, svec2i(pos.x, pos.y + ammoY), bgSize);
	}

	// Draw back item
	DrawUtilMenuItem(d, g, OPTION_COUNT, svec2i(pos.x, pos.y + ammoY), bgSize);
}
static void DrawUtilMenuItem(
	const UtilMenu *data, GraphicsDevice *g, const Option option,
	const struct vec2i pos, const struct vec2i bgSize)
{
	const bool selected = data->idx / 2 == (int)option;
	const PlayerData *pData = PlayerDataGetByUID(data->PlayerUID);
	int amount = 0;
	int max = 0;
	int delta = 0;
	int price = 0;
	const char *name = NULL;
	const Pic *pic = NULL;
	switch (option)
	{
	case OPTION_HP:
		amount = pData->HP;
		max = CampaignGetMaxHP(&gCampaign);
		delta = HP_DELTA;
		price = HP_PRICE;
		name = "Armor";
		pic = PicManagerGetPic(&gPicManager, "health");
		break;
	case OPTION_LIVES:
		amount = pData->Lives;
		max = CampaignGetMaxLives(&gCampaign);
		delta = 1;
		price = LIFE_PRICE;
		name = "Life";
		pic = GetHeadPic(
			pData->Char.Class, DIRECTION_DOWN, false, &pData->Char.Colors);
		break;
	default:
		name = "Back";
		break;
	}
	const struct vec2i bgPos = svec2i(pos.x, pos.y + option * bgSize.y);
	color_t color = colorWhite;
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
	int y = bgPos.y + ROW_H * 2;

	// Amount BG
	if (max > 0 && amount > 0)
	{
		DrawRectangle(
			g, svec2i_add(svec2i(x, y), svec2i_one()),
			svec2i(amount * (bgSize.x - 8) / max, FontH()), colorBlue, true);
	}

	y = bgPos.y + ROW_H;

	// Sell/buy buttons
	if (max > 0)
	{
		const bool sellSelected =
			selected && data->Active && (data->idx & 1) == 1;
		const bool canSell = amount > delta;
		const FontOpts foptsSell = {
			ALIGN_CENTER, ALIGN_START, svec2i(BUTTON_BG_W, FontH()),
			svec2i(2, 2), sellSelected ? colorRed : colorGray};
		x = bgPos.x + bgSize.x - BUTTON_BG_W - 2;
		const struct vec2i sellPos = svec2i(x, y);
		Draw9Slice(
			g, data->buttonBG,
			Rect2iNew(sellPos, svec2i(BUTTON_BG_W, FontH() + 4)), 3, 3, 3, 3,
			true,
			canSell ? (sellSelected ? colorRed : colorMaroon) : colorGray,
			SDL_FLIP_NONE);
		FontStrOpt("Sell", sellPos, foptsSell);

		x -= BUTTON_BG_W + 3;
		const bool buySelected =
			selected && data->Active && (data->idx & 1) == 0;
		const bool canBuy = amount < max && price <= pData->Totals.Score;
		const FontOpts foptsBuy = {
			ALIGN_CENTER, ALIGN_START, svec2i(BUTTON_BG_W, FontH()),
			svec2i(2, 2), buySelected ? colorGreen : colorGray};
		const struct vec2i buyPos = svec2i(x, y);
		Draw9Slice(
			g, data->buttonBG,
			Rect2iNew(buyPos, svec2i(BUTTON_BG_W, FontH() + 4)), 3, 3, 3, 3,
			true,
			canBuy ? (buySelected ? colorGreen : colorOfficeGreen) : colorGray,
			SDL_FLIP_NONE);
		FontStrOpt("Buy", buyPos, foptsBuy);
	}

	y = bgPos.y;
	x = bgPos.x + 4;

	const FontOpts fopts = {
		ALIGN_START, ALIGN_START, bgSize, svec2i(2, 2), color};

	// Name
	FontStrOpt(name, svec2i(x, y), fopts);

	// Price
	if (price > 0)
	{
		const FontOpts foptsP = {
			ALIGN_END, ALIGN_START, bgSize, svec2i(8, 2),
			selected ? colorRed : colorGray};
		char buf[256];
		sprintf(buf, "$%d", price);
		FontStrOpt(buf, svec2i(x, y), foptsP);
	}

	y += ROW_H * 2;
	x = bgPos.x + 4;

	// Amount BG
	if (max > 0 && amount > 0)
	{
		DrawRectangle(
			g, svec2i_add(svec2i(x, y), svec2i_one()),
			svec2i(amount * (bgSize.x - 8) / max, FontH()), colorBlue, true);
	}

	// Amount
	if (max > 0)
	{
		char buf[256];
		sprintf(buf, "%d/%d", amount, max);
		const FontOpts foptsA = {
			ALIGN_END, ALIGN_START, bgSize, svec2i(8, 2), color};
		FontStrOpt(buf, svec2i(x, y), foptsA);
	}

	y -= ROW_H;

	// Icon
	if (pic != NULL)
	{
		x = bgPos.x + 12;
		const struct vec2i picPos = svec2i_subtract(
			svec2i(x, bgPos.y + bgSize.y / 2),
			svec2i_scale_divide(pic->size, 2));
		PicRender(
			pic, g->gameWindow.renderer, picPos, colorWhite, 0, svec2_one(),
			SDL_FLIP_NONE, Rect2iZero());
	}
}
static int HandleInputMenu(int cmd, void *data)
{
	UtilMenu *d = data;

	if (Button1(cmd))
	{
		// Do nothing; don't switch away from menu
	}
	else if (Button2(cmd))
	{
		return 1;
	}
	else if (Left(cmd))
	{
		if ((d->idx % 2) == 1)
		{
			d->idx--;
			MenuPlaySound(MENU_SOUND_SWITCH);
		}
	}
	else if (Right(cmd))
	{
		if (d->idx < OPTION_COUNT * 2 && (d->idx % 2) == 0)
		{
			d->idx++;
			MenuPlaySound(MENU_SOUND_SWITCH);
		}
	}
	else if (Up(cmd))
	{
		if (d->idx >= 2)
		{
			d->idx -= 2;
			MenuPlaySound(MENU_SOUND_SWITCH);
		}
	}
	else if (Down(cmd))
	{
		if (d->idx + 2 < OPTION_COUNT * 2 + 2)
		{
			d->idx = MIN(OPTION_COUNT * 2 + 2, d->idx + 2);
			MenuPlaySound(MENU_SOUND_SWITCH);
		}
	}

	return 0;
}

void UtilMenuTerminate(UtilMenu *menu)
{
	MenuSystemTerminate(&menu->ms);
}

void UtilMenuReset(UtilMenu *menu)
{
	menu->idx = CLAMP(menu->idx, 0, OPTION_COUNT * 2);
}

void UtilMenuActivate(UtilMenu *menu)
{
	menu->Active = true;
	menu->SelectResult = UTIL_MENU_NONE;
}

bool UtilMenuUpdate(UtilMenu *menu, const int cmd)
{
	PlayerData *p = PlayerDataGetByUID(menu->PlayerUID);
	MenuProcessCmd(&menu->ms, cmd);
	switch (menu->SelectResult)
	{
	case UTIL_MENU_NONE:
		break;
	case UTIL_MENU_SELECT: {
		const Option option = GetSelectedOption(menu);
		const bool buy = (menu->idx & 1) == 0;
		switch (option)
		{
		case OPTION_HP:
			PlayerSetHP(p, p->HP + (buy ? HP_DELTA : -HP_DELTA));
			PlayerScore(p, buy ? -HP_PRICE : HP_PRICE);
			break;
		case OPTION_LIVES:
			PlayerSetLives(p, p->Lives + (buy ? 1 : -1));
			PlayerScore(p, buy ? -LIFE_PRICE : LIFE_PRICE);
			break;
		default:
			break;
		}
		return true;
	}
	break;
	case UTIL_MENU_CANCEL:
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

void UtilMenuDraw(const UtilMenu *menu)
{
	MenuDisplay(&menu->ms);
}
