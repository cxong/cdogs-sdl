/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.

	Copyright (c) 2013-2016, Cong Xu
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
#include "player_select_menus.h"

#include <assert.h>
#include <stddef.h>

#include <cdogs/actors.h>	// for shades
#include <cdogs/draw.h>
#include <cdogs/font.h>
#include <cdogs/player_template.h>


static char letters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ !#?:.-0123456789";
static char smallLetters[] = "abcdefghijklmnopqrstuvwxyz !#?:.-0123456789";


static void DrawNameMenu(
	const menu_t *menu, GraphicsDevice *g,
	const Vec2i pos, const Vec2i size, const void *data)
{
	const PlayerSelectMenuData *d = data;

#define ENTRY_COLS	8
#define	ENTRY_SPACING	7

	int x = pos.x;
	int y = CENTER_Y(
		pos, size, FontH() * ((strlen(letters) - 1) / ENTRY_COLS));

	UNUSED(menu);
	UNUSED(g);

	int i;
	for (i = 0; i < (int)strlen(letters); i++)
	{
		Vec2i menuPos = Vec2iNew(
			x + (i % ENTRY_COLS) * ENTRY_SPACING,
			y + (i / ENTRY_COLS) * FontH());
		FontChMask(
			letters[i], menuPos,
			i == d->nameMenuSelection ? colorRed : colorWhite);
	}

	DisplayMenuItem(
		Vec2iNew(
			x + (i % ENTRY_COLS) * ENTRY_SPACING,
			y + (i / ENTRY_COLS) * FontH()),
		"(End)", i == d->nameMenuSelection, 0, colorBlack);
}

static int HandleInputNameMenu(int cmd, void *data)
{
	PlayerSelectMenuData *d = data;
	PlayerData *p = PlayerDataGetByUID(d->display.PlayerUID);

	if (cmd & CMD_BUTTON1)
	{
		if (d->nameMenuSelection == (int)strlen(letters))
		{
			MenuPlaySound(MENU_SOUND_ENTER);
			return 1;
		}

		if (strlen(p->name) < sizeof p->name - 1)
		{
			size_t l = strlen(p->name);
			p->name[l + 1] = 0;
			if (l > 0 && p->name[l - 1] != ' ')
			{
				p->name[l] = smallLetters[d->nameMenuSelection];
			}
			else
			{
				p->name[l] = letters[d->nameMenuSelection];
			}
			MenuPlaySound(MENU_SOUND_ENTER);
		}
		else
		{
			MenuPlaySound(MENU_SOUND_ERROR);
		}
	}
	else if (cmd & CMD_BUTTON2)
	{
		if (p->name[0])
		{
			p->name[strlen(p->name) - 1] = 0;
			MenuPlaySound(MENU_SOUND_BACK);
		}
		else
		{
			MenuPlaySound(MENU_SOUND_ERROR);
		}
	}
	else if (cmd & CMD_LEFT)
	{
		if (d->nameMenuSelection > 0)
		{
			d->nameMenuSelection--;
			MenuPlaySound(MENU_SOUND_SWITCH);
		}
	}
	else if (cmd & CMD_RIGHT)
	{
		if (d->nameMenuSelection < (int)strlen(letters))
		{
			d->nameMenuSelection++;
			MenuPlaySound(MENU_SOUND_SWITCH);
		}
	}
	else if (cmd & CMD_UP)
	{
		if (d->nameMenuSelection >= ENTRY_COLS)
		{
			d->nameMenuSelection -= ENTRY_COLS;
			MenuPlaySound(MENU_SOUND_SWITCH);
		}
	}
	else if (cmd & CMD_DOWN)
	{
		if (d->nameMenuSelection <= (int)strlen(letters) - ENTRY_COLS)
		{
			d->nameMenuSelection += ENTRY_COLS;
			MenuPlaySound(MENU_SOUND_SWITCH);
		}
		else if (d->nameMenuSelection < (int)strlen(letters))
		{
			d->nameMenuSelection = (int)strlen(letters);
			MenuPlaySound(MENU_SOUND_SWITCH);
		}
	}

	return 0;
}

static void PostInputFaceMenu(menu_t *menu, int cmd, void *data);
static menu_t *CreateFaceMenu(int *playerUID)
{
	menu_t *menu = MenuCreateNormal("Face", "", MENU_TYPE_NORMAL, 0);
	menu->u.normal.maxItems = 11;
	for (int i = 0; i < FACE_COUNT; i++)
	{
		MenuAddSubmenu(menu, MenuCreateBack(IndexToFaceStr(i)));
	}
	MenuSetPostInputFunc(menu, PostInputFaceMenu, playerUID);
	return menu;
}
static void PostInputFaceMenu(menu_t *menu, int cmd, void *data)
{
	int *playerUID = data;
	UNUSED(cmd);
	PlayerData *p = PlayerDataGetByUID(*playerUID);
	Character *c = &p->Char;
	c->Face = menu->u.normal.index;
}

// TODO: colour picker
static void PostInputColorMenu(menu_t *menu, int cmd, void *data);
static menu_t *CreateColorMenu(
	const char *name, AppearanceMenuData *data)
{
	menu_t *menu = MenuCreateNormal(name, "", MENU_TYPE_NORMAL, 0);
	menu->u.normal.maxItems = 11;
	for (int i = 0; i < 5; i++)
	{
		char buf[256];
		sprintf(buf, "Color %d", i);
		MenuAddSubmenu(menu, MenuCreateBack(buf));
	}
	MenuSetPostInputFunc(menu, PostInputColorMenu, data);
	return menu;
}
static void PostInputColorMenu(menu_t *menu, int cmd, void *data)
{
	AppearanceMenuData *d = data;
	UNUSED(cmd);
	PlayerData *p = PlayerDataGetByUID(d->PlayerUID);
	Character *c = &p->Char;
	// TODO: colour picker
	color_t *prop = (color_t *)((char *)&c->Colors + d->propertyOffset);
	prop->r = (uint8_t)(menu->u.normal.index * 0x30);
	prop->g = (uint8_t)(menu->u.normal.index * 0x30);
	prop->b = (uint8_t)(menu->u.normal.index * 0x30);
}

static void PostInputLoadTemplate(menu_t *menu, int cmd, void *data)
{
	if (cmd & CMD_BUTTON1)
	{
		PlayerSelectMenuData *d = data;
		PlayerData *p = PlayerDataGetByUID(d->display.PlayerUID);
		const PlayerTemplate *t =
			CArrayGet(&gPlayerTemplates, menu->u.normal.index);
		memset(p->name, 0, sizeof p->name);
		strncpy(p->name, t->name, sizeof p->name - 1);
		p->Char.Face = t->Face;
		p->Char.Colors = t->Colors;
	}
}

// Load all the template names to the menu entries
static void PostEnterLoadTemplateNames(menu_t *menu, void *data)
{
	bool *isSave = (bool *)data;
	for (int i = 0; i < (int)gPlayerTemplates.size; i++)
	{
		// Add menu if necessary
		if (i == (int)menu->u.normal.subMenus.size)
		{
			MenuAddSubmenu(menu, MenuCreateBack(""));
		}
		menu_t *subMenu = CArrayGet(&menu->u.normal.subMenus, i);
		const PlayerTemplate *pt = CArrayGet(&gPlayerTemplates, i);
		CFREE(subMenu->name);
		CSTRDUP(subMenu->name, pt->name);
	}
	if (*isSave && menu->u.normal.subMenus.size == gPlayerTemplates.size)
	{
		MenuAddSubmenu(menu, MenuCreateBack("(new)"));
	}
}

static menu_t *CreateUseTemplateMenu(
	const char *name, PlayerSelectMenuData *data)
{
	menu_t *menu = MenuCreateNormal(name, "", MENU_TYPE_NORMAL, 0);
	menu->u.normal.maxItems = 11;
	MenuSetPostEnterFunc(menu, PostEnterLoadTemplateNames, &gFalse, false);
	MenuSetPostInputFunc(menu, PostInputLoadTemplate, data);
	return menu;
}

static void PostInputSaveTemplate(menu_t *menu, int cmd, void *data)
{
	if (!(cmd & CMD_BUTTON1))
	{
		return;
	}
	PlayerSelectMenuData *d = data;
	PlayerData *p = PlayerDataGetByUID(d->display.PlayerUID);
	while (menu->u.normal.index >= (int)gPlayerTemplates.size)
	{
		PlayerTemplate empty;
		memset(&empty, 0, sizeof empty);
		CArrayPushBack(&gPlayerTemplates, &empty);
	}
	PlayerTemplate *t =
		CArrayGet(&gPlayerTemplates, menu->u.normal.index);
	memset(t->name, 0, sizeof t->name);
	strncpy(t->name, p->name, sizeof t->name - 1);
	t->Face = p->Char.Face;
	t->Colors = p->Char.Colors;
}

static void SaveTemplateDisplayTitle(
	const menu_t *menu, GraphicsDevice *g,
	const Vec2i pos, const Vec2i size, const void *data)
{
	UNUSED(g);
	const PlayerSelectMenuData *d = data;
	char buf[256];

	UNUSED(menu);
	UNUSED(size);

	// Display "Save <template>..." title
	const PlayerData *p = PlayerDataGetByUID(d->display.PlayerUID);
	sprintf(buf, "Save %s...", p->name);
	FontStr(buf, Vec2iAdd(pos, Vec2iNew(0, 0)));
}

static menu_t *CreateSaveTemplateMenu(
	const char *name, PlayerSelectMenuData *data)
{
	menu_t *menu = MenuCreateNormal(name, "", MENU_TYPE_NORMAL, 0);
	menu->u.normal.maxItems = 11;
	MenuSetPostEnterFunc(menu, PostEnterLoadTemplateNames, &gTrue, false);
	MenuSetPostInputFunc(menu, PostInputSaveTemplate, data);
	MenuSetCustomDisplay(menu, SaveTemplateDisplayTitle, data);
	return menu;
}

static void CheckReenableLoadMenu(menu_t *menu, void *data)
{
	menu_t *loadMenu = MenuGetSubmenuByName(menu, "Load");
	UNUSED(data);
	assert(loadMenu);
	loadMenu->isDisabled = gPlayerTemplates.size == 0;
}
static menu_t *CreateCustomizeMenu(
	const char *name, PlayerSelectMenuData *data);
static void ShuffleAppearance(void *data);
void PlayerSelectMenusCreate(
	PlayerSelectMenu *menu,
	int numPlayers, int player, const int playerUID,
	EventHandlers *handlers, GraphicsDevice *graphics,
	const NameGen *ng)
{
	MenuSystem *ms = &menu->ms;
	PlayerSelectMenuData *data = &menu->data;
	Vec2i pos, size;
	int w = graphics->cachedConfig.Res.x;
	int h = graphics->cachedConfig.Res.y;

	data->nameMenuSelection = (int)strlen(letters);
	data->display.PlayerUID = playerUID;
	data->display.currentMenu = &ms->current;
	data->PlayerUID = playerUID;
	data->nameGenerator = ng;

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
	MenuAddSubmenu(ms->root, MenuCreateCustom(
		"Name", DrawNameMenu, HandleInputNameMenu, data));

	MenuAddSubmenu(ms->root, CreateCustomizeMenu("Customize...", data));
	MenuAddSubmenu(
		ms->root,
		MenuCreateVoidFunc("Shuffle", ShuffleAppearance, data));

	MenuAddSubmenu(ms->root, CreateUseTemplateMenu("Load", data));
	MenuAddSubmenu(ms->root, CreateSaveTemplateMenu("Save", data));

	MenuAddSubmenu(ms->root, MenuCreateSeparator(""));
	MenuAddSubmenu(
		ms->root, MenuCreateNormal("Done", "", MENU_TYPE_NORMAL, 0));
	// Select "Done"
	ms->root->u.normal.index = (int)ms->root->u.normal.subMenus.size - 1;
	MenuAddExitType(ms, MENU_TYPE_RETURN);
	MenuSystemAddCustomDisplay(ms, MenuDisplayPlayer, data);
	MenuSystemAddCustomDisplay(
		ms, MenuDisplayPlayerControls, &data->PlayerUID);

	// Detect when there have been new player templates created,
	// to re-enable the load menu
	CheckReenableLoadMenu(ms->root, NULL);
	MenuSetPostEnterFunc(ms->root, CheckReenableLoadMenu, NULL, false);
}
static menu_t *CreateCustomizeMenu(
	const char *name, PlayerSelectMenuData *data)
{
	menu_t *menu = MenuCreateNormal(name, "", MENU_TYPE_NORMAL, 0);

	MenuAddSubmenu(menu, CreateFaceMenu(&data->PlayerUID));

	data->skinData.PlayerUID = data->PlayerUID;
	data->skinData.propertyOffset = offsetof(CharColors, Skin);
	MenuAddSubmenu(menu, CreateColorMenu("Skin", &data->skinData));

	data->hairData.PlayerUID = data->PlayerUID;
	data->hairData.propertyOffset = offsetof(CharColors, Hair);
	MenuAddSubmenu(menu, CreateColorMenu("Hair", &data->hairData));

	data->armsData.PlayerUID = data->PlayerUID;
	data->armsData.propertyOffset = offsetof(CharColors, Arms);
	MenuAddSubmenu(menu, CreateColorMenu("Arms", &data->armsData));

	data->bodyData.PlayerUID = data->PlayerUID;
	data->bodyData.propertyOffset = offsetof(CharColors, Body);
	MenuAddSubmenu(menu, CreateColorMenu("Body", &data->bodyData));

	data->legsData.PlayerUID = data->PlayerUID;
	data->legsData.propertyOffset = offsetof(CharColors, Legs);
	MenuAddSubmenu(menu, CreateColorMenu("Legs", &data->legsData));

	MenuAddSubmenu(menu, MenuCreateSeparator(""));
	MenuAddSubmenu(menu, MenuCreateBack("Back"));

	return menu;
}
static void ShuffleAppearance(void *data)
{
	PlayerSelectMenuData *pData = data;
	char buf[512];
	NameGenMake(pData->nameGenerator, buf);
	PlayerData *p = PlayerDataGetByUID(pData->display.PlayerUID);
	strncpy(p->name, buf, 20);
	Character *c = &p->Char;
	CharacterShuffleAppearance(c);
}
