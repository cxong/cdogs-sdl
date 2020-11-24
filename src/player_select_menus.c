/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.

	Copyright (c) 2013-2016, 2018-2020 Cong Xu
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

#include <cdogs/character_class.h>
#include <cdogs/draw/draw.h>
#include <cdogs/draw/drawtools.h>
#include <cdogs/font.h>
#include <cdogs/player_template.h>

static char letters[] = "1234567890-QWERTYUIOP!ASDFGHJKL:#ZXCVBNM  .?";
static char smallLetters[] = "1234567890-qwertyuiop!asdfghjkl:#zxcvbnm  .?";

static void DrawNameMenu(
	const menu_t *menu, GraphicsDevice *g, const struct vec2i pos,
	const struct vec2i size, const void *data)
{
	const PlayerSelectMenuData *d = data;

#define ENTRY_COLS 11
#define ENTRY_SPACING 7

	int x = pos.x;
	int y = CENTER_Y(
		pos, size, FontH() * (((int)strlen(letters) - 1) / ENTRY_COLS));

	UNUSED(menu);
	UNUSED(g);

	int i;
	for (i = 0; i < (int)strlen(letters); i++)
	{
		struct vec2i menuPos = svec2i(
			x + (i % ENTRY_COLS) * ENTRY_SPACING,
			y + (i / ENTRY_COLS) * FontH());
		FontChMask(
			letters[i], menuPos,
			i == d->nameMenuSelection ? colorRed : colorWhite);
	}

	DisplayMenuItem(
		svec2i(
			x + (i % ENTRY_COLS) * ENTRY_SPACING,
			y + (i / ENTRY_COLS) * FontH()),
		"(End)", i == d->nameMenuSelection, 0, colorWhite);
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

static void PostInputRotatePlayer(menu_t *menu, int cmd, void *data);
static void CheckReenableHairHatMenu(menu_t *menu, void *data);

static void PostInputFaceMenu(menu_t *menu, int cmd, void *data);
static menu_t *CreateFaceMenu(MenuDisplayPlayerData *data)
{
	menu_t *menu = MenuCreateNormal("Face", "", MENU_TYPE_NORMAL, 0);
	menu->u.normal.maxItems = 11;
	CA_FOREACH(const CharacterClass, c, gCharacterClasses.Classes)
	MenuAddSubmenu(menu, MenuCreateBack(c->Name));
	CA_FOREACH_END()
	CA_FOREACH(const CharacterClass, c, gCharacterClasses.CustomClasses)
	MenuAddSubmenu(menu, MenuCreateBack(c->Name));
	CA_FOREACH_END()
	MenuSetPostInputFunc(menu, PostInputFaceMenu, data);
	return menu;
}
static void PostInputFaceMenu(menu_t *menu, int cmd, void *data)
{
	const MenuDisplayPlayerData *d = data;
	// Change player face based on current menu selection
	PlayerData *p = PlayerDataGetByUID(d->PlayerUID);
	Character *c = &p->Char;
	if (menu->u.normal.index < (int)gCharacterClasses.Classes.size)
	{
		c->Class = CArrayGet(&gCharacterClasses.Classes, menu->u.normal.index);
	}
	else
	{
		c->Class = CArrayGet(
			&gCharacterClasses.CustomClasses,
			menu->u.normal.index - (int)gCharacterClasses.Classes.size);
	}
	PostInputRotatePlayer(menu, cmd, data);
}

static void PostInputHairMenu(menu_t *menu, int cmd, void *data);
static menu_t *CreateHairMenu(MenuDisplayPlayerData *data)
{
	menu_t *menu = MenuCreateNormal("Hair/hat", "", MENU_TYPE_NORMAL, 0);
	menu->u.normal.maxItems = 11;
	MenuAddSubmenu(menu, MenuCreateBack("(None)"));
	CA_FOREACH(const char *, h, gPicManager.hairstyleNames)
	MenuAddSubmenu(menu, MenuCreateBack(*h));
	CA_FOREACH_END()
	MenuSetPostInputFunc(menu, PostInputHairMenu, data);
	return menu;
}
static void PostInputHairMenu(menu_t *menu, int cmd, void *data)
{
	const MenuDisplayPlayerData *d = data;
	// Change player hairstyle based on current menu selection
	PlayerData *p = PlayerDataGetByUID(d->PlayerUID);
	Character *c = &p->Char;
	CFREE(c->Hair);
	c->Hair = NULL;
	if (menu->u.normal.index > 0)
	{
		const char **hair =
			CArrayGet(&gPicManager.hairstyleNames, menu->u.normal.index - 1);
		CSTRDUP(c->Hair, *hair);
	}
	PostInputRotatePlayer(menu, cmd, data);
}

static void DrawColorMenu(
	const menu_t *menu, GraphicsDevice *g, const struct vec2i pos,
	const struct vec2i size, const void *data);
static int HandleInputColorMenu(int cmd, void *data);
static menu_t *CreateColorMenu(
	const char *name, ColorMenuData *data, const CharColorType type,
	const int playerUID)
{
	data->Type = type;
	data->PlayerUID = playerUID;
	data->palette = PicManagerGetPic(&gPicManager, "palette");
	// Find the closest palette colour available, and select it
	// Use least squares method
	int errorLowest = -1;
	PlayerData *p = PlayerDataGetByUID(playerUID);
	const color_t currentColour = *CharColorGetByType(&p->Char.Colors, type);
	struct vec2i v;
	for (v.y = 0; v.y < data->palette->size.y; v.y++)
	{
		for (v.x = 0; v.x < data->palette->size.x; v.x++)
		{
			const color_t colour = PIXEL2COLOR(
				data->palette->Data[v.x + v.y * data->palette->size.x]);
			if (colour.a == 0)
			{
				continue;
			}
			const int er = (int)colour.r - currentColour.r;
			const int eg = (int)colour.g - currentColour.g;
			const int eb = (int)colour.b - currentColour.b;
			const int error = er * er + eg * eg + eb * eb;
			if (errorLowest == -1 || error < errorLowest)
			{
				errorLowest = error;
				data->selectedColor = v;
			}
		}
	}
	return MenuCreateCustom(name, DrawColorMenu, HandleInputColorMenu, data);
}
static void DrawColorMenu(
	const menu_t *menu, GraphicsDevice *g, const struct vec2i pos,
	const struct vec2i size, const void *data)
{
	UNUSED(menu);
	const ColorMenuData *d = data;
	// Draw colour squares from the palette
	const struct vec2i swatchSize = svec2i(4, 4);
	const struct vec2i drawPos =
		svec2i(pos.x, CENTER_Y(pos, size, d->palette->size.y * swatchSize.y));
	struct vec2i v;
	for (v.y = 0; v.y < d->palette->size.y; v.y++)
	{
		for (v.x = 0; v.x < d->palette->size.x; v.x++)
		{
			const color_t colour =
				PIXEL2COLOR(d->palette->Data[v.x + v.y * d->palette->size.x]);
			if (colour.a == 0)
			{
				continue;
			}
			DrawRectangle(
				g, svec2i_add(drawPos, svec2i_multiply(v, swatchSize)),
				swatchSize, colour, true);
		}
	}
	// Draw a highlight around the selected colour
	DrawRectangle(
		g,
		svec2i_add(
			drawPos,
			svec2i_subtract(
				svec2i_multiply(d->selectedColor, swatchSize), svec2i(1, 1))),
		svec2i_add(swatchSize, svec2i(2, 2)), colorWhite, false);
}
static int HandleInputColorMenu(int cmd, void *data)
{
	ColorMenuData *d = data;
	struct vec2i selected = d->selectedColor;
	switch (cmd)
	{
	case CMD_LEFT:
		selected.x--;
		break;
	case CMD_RIGHT:
		selected.x++;
		break;
	case CMD_UP:
		selected.y--;
		break;
	case CMD_DOWN:
		selected.y++;
		break;
	case CMD_BUTTON1:
	case CMD_BUTTON2: // fallthrough
		MenuPlaySound(MENU_SOUND_ENTER);
		return cmd;
	default:
		break;
	}
	if (selected.x >= 0 && selected.x < d->palette->size.x &&
		selected.y >= 0 && selected.y < d->palette->size.y)
	{
		const color_t colour = PIXEL2COLOR(
			d->palette->Data[selected.x + selected.y * d->palette->size.x]);
		if (colour.a != 0)
		{
			d->selectedColor = selected;
			PlayerData *p = PlayerDataGetByUID(d->PlayerUID);
			*CharColorGetByType(&p->Char.Colors, d->Type) = colour;
			MenuPlaySound(MENU_SOUND_SWITCH);
		}
	}
	return 0;
}

static void PostInputLoadTemplate(menu_t *menu, int cmd, void *data)
{
	if (cmd & CMD_BUTTON1)
	{
		PlayerSelectMenuData *d = data;
		PlayerData *p = PlayerDataGetByUID(d->display.PlayerUID);
		const PlayerTemplate *t =
			PlayerTemplateGetById(&gPlayerTemplates, menu->u.normal.index);
		memset(p->name, 0, sizeof p->name);
		strcpy(p->name, t->name);
		p->Char.Class = StrCharacterClass(t->CharClassName);
		if (p->Char.Class == NULL)
		{
			p->Char.Class = StrCharacterClass("Jones");
		}
		CFREE(p->Char.Hair);
		p->Char.Hair = NULL;
		if (t->Hair)
		{
			CSTRDUP(p->Char.Hair, t->Hair);
		}
		p->Char.Colors = t->Colors;
	}
}

// Load all the template names to the menu entries
static void PostEnterLoadTemplateNames(menu_t *menu, void *data)
{
	bool *isSave = (bool *)data;
	MenuClearSubmenus(menu);
	for (int i = 0;; i++)
	{
		const PlayerTemplate *pt = PlayerTemplateGetById(&gPlayerTemplates, i);
		if (pt == NULL)
		{
			break;
		}
		MenuAddSubmenu(menu, MenuCreateBack(pt->name));
	}
	if (*isSave)
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
	PlayerTemplate *t =
		PlayerTemplateGetById(&gPlayerTemplates, menu->u.normal.index);
	if (t == NULL)
	{
		PlayerTemplate nt;
		memset(&nt, 0, sizeof nt);
		CArrayPushBack(&gPlayerTemplates.Classes, &nt);
		t = CArrayGet(
			&gPlayerTemplates.Classes, gPlayerTemplates.Classes.size - 1);
	}
	memset(t->name, 0, sizeof t->name);
	strcpy(t->name, p->name);
	CFREE(t->CharClassName);
	CSTRDUP(t->CharClassName, p->Char.Class->Name);
	if (p->Char.Hair)
	{
		CFREE(t->Hair);
		CSTRDUP(t->Hair, p->Char.Hair);
	}
	t->Colors = p->Char.Colors;
	PlayerTemplatesSave(&gPlayerTemplates);
}

static void SaveTemplateDisplayTitle(
	const menu_t *menu, GraphicsDevice *g, const struct vec2i pos,
	const struct vec2i size, const void *data)
{
	UNUSED(g);
	const PlayerSelectMenuData *d = data;
	char buf[256];

	UNUSED(menu);
	UNUSED(size);

	// Display "Save <template>..." title
	const PlayerData *p = PlayerDataGetByUID(d->display.PlayerUID);
	sprintf(buf, "Save %s...", p->name);
	FontStr(buf, svec2i_add(pos, svec2i(0, 0)));
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
	loadMenu->isDisabled = PlayerTemplateGetById(&gPlayerTemplates, 0) == NULL;
}
static menu_t *CreateCustomizeMenu(
	const char *name, PlayerSelectMenuData *data);
static void ShuffleAppearance(void *data);
void PlayerSelectMenusCreate(
	PlayerSelectMenu *menu, int numPlayers, int player, const int playerUID,
	EventHandlers *handlers, GraphicsDevice *graphics, const NameGen *ng)
{
	MenuSystem *ms = &menu->ms;
	PlayerSelectMenuData *data = &menu->data;
	struct vec2i pos, size;
	int w = graphics->cachedConfig.Res.x;
	int h = graphics->cachedConfig.Res.y;

	data->nameMenuSelection = (int)strlen(letters);
	data->display.PlayerUID = playerUID;
	data->display.currentMenu = &ms->current;
	data->display.Dir = DIRECTION_DOWN;
	data->nameGenerator = ng;

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
	ms->root = ms->current = MenuCreateNormal("", "", MENU_TYPE_NORMAL, 0);
	MenuSetPostInputFunc(ms->root, PostInputRotatePlayer, &data->display);
	MenuAddSubmenu(
		ms->root,
		MenuCreateCustom("Name", DrawNameMenu, HandleInputNameMenu, data));

	MenuAddSubmenu(ms->root, CreateCustomizeMenu("Customize...", data));
	MenuAddSubmenu(
		ms->root, MenuCreateVoidFunc("Shuffle", ShuffleAppearance, data));

	MenuAddSubmenu(ms->root, CreateUseTemplateMenu("Load", data));
	MenuAddSubmenu(ms->root, CreateSaveTemplateMenu("Save", data));

	MenuAddSubmenu(ms->root, MenuCreateSeparator(""));
	MenuAddSubmenu(
		ms->root, MenuCreateNormal("Done", "", MENU_TYPE_NORMAL, 0));
	// Select "Done"
	ms->root->u.normal.index = (int)ms->root->u.normal.subMenus.size - 1;
	MenuAddExitType(ms, MENU_TYPE_RETURN);
	MenuSystemAddCustomDisplay(ms, MenuDisplayPlayer, &data->display);
	MenuSystemAddCustomDisplay(
		ms, MenuDisplayPlayerControls, &data->display.PlayerUID);

	// Detect when there have been new player templates created,
	// to re-enable the load menu
	CheckReenableLoadMenu(ms->root, NULL);
	MenuSetPostEnterFunc(ms->root, CheckReenableLoadMenu, NULL, false);
}
static menu_t *CreateCustomizeMenu(
	const char *name, PlayerSelectMenuData *data)
{
	menu_t *menu = MenuCreateNormal(name, "", MENU_TYPE_NORMAL, 0);

	MenuAddSubmenu(menu, CreateFaceMenu(&data->display));
	MenuAddSubmenu(menu, CreateHairMenu(&data->display));

	MenuAddSubmenu(
		menu, CreateColorMenu(
				  "Skin", &data->skinData, CHAR_COLOR_SKIN,
				  data->display.PlayerUID));
	MenuAddSubmenu(
		menu, CreateColorMenu(
				  "Hair", &data->hairData, CHAR_COLOR_HAIR,
				  data->display.PlayerUID));
	MenuAddSubmenu(
		menu, CreateColorMenu(
				  "Arms", &data->armsData, CHAR_COLOR_ARMS,
				  data->display.PlayerUID));
	MenuAddSubmenu(
		menu, CreateColorMenu(
				  "Body", &data->bodyData, CHAR_COLOR_BODY,
				  data->display.PlayerUID));
	MenuAddSubmenu(
		menu, CreateColorMenu(
				  "Legs", &data->legsData, CHAR_COLOR_LEGS,
				  data->display.PlayerUID));
	MenuAddSubmenu(
		menu, CreateColorMenu(
				  "Feet", &data->feetData, CHAR_COLOR_FEET,
				  data->display.PlayerUID));

	MenuAddSubmenu(menu, MenuCreateSeparator(""));
	MenuAddSubmenu(menu, MenuCreateBack("Back"));

	MenuSetPostInputFunc(menu, PostInputRotatePlayer, &data->display);
	MenuSetPostEnterFunc(
		menu, CheckReenableHairHatMenu, &data->display, false);

	return menu;
}
static void ShuffleAppearance(void *data)
{
	PlayerSelectMenuData *pData = data;
	char buf[512];
	NameGenMake(pData->nameGenerator, buf);
	PlayerData *p = PlayerDataGetByUID(pData->display.PlayerUID);
	strcpy(p->name, buf);
	Character *c = &p->Char;
	CharacterShuffleAppearance(c);
}

static void PostInputRotatePlayer(menu_t *menu, int cmd, void *data)
{
	UNUSED(menu);
	MenuDisplayPlayerData *d = data;
	// Rotate player using left/right keys
	const int dx = (cmd & CMD_LEFT) ? 1 : ((cmd & CMD_RIGHT) ? -1 : 0);
	if (dx != 0)
	{
		d->Dir = (direction_e)CLAMP_OPPOSITE(
			(int)d->Dir + dx, DIRECTION_UP, DIRECTION_UPLEFT);
		SoundPlay(&gSoundDevice, StrSound("footsteps"));
	}
}
static void CheckReenableHairHatMenu(menu_t *menu, void *data)
{
	menu_t *hairMenu = MenuGetSubmenuByName(menu, "Hair/hat");
	CASSERT(hairMenu, "cannot find menu");
	MenuDisplayPlayerData *d = data;
	const PlayerData *p = PlayerDataGetByUID(d->PlayerUID);
	hairMenu->isDisabled = !p->Char.Class->HasHair;
}
