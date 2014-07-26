/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.

	Copyright (c) 2013-2014, Cong Xu
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

#include <cdogs/actors.h>	// for shades
#include <cdogs/draw.h>
#include <cdogs/player_template.h>
#include <cdogs/text.h>


static char letters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ !#?:.-0123456789";
static char smallLetters[] = "abcdefghijklmnopqrstuvwxyz !#?:.-0123456789";


static const char *shadeNames[] = {
	"Blue",
	"Skin",
	"Brown",
	"Green",
	"Yellow",
	"Purple",
	"Red",
	"Light Gray",
	"Gray",
	"Dark Gray",
	"Asian",
	"Dark Skin",
	"Black",
	"Golden"
};
const char *IndexToShadeStr(int idx)
{
	if (idx >= 0 && idx < SHADE_COUNT)
	{
		return shadeNames[idx];
	}
	return shadeNames[0];
}

static void SetPlayer(Character *c, struct PlayerData *data)
{
	CharacterSetLooks(c, &data->looks);
	c->speed = 256;
	c->maxHealth = 200;
}


static void DrawNameMenu(
	menu_t *menu, GraphicsDevice *g, Vec2i pos, Vec2i size, void *data)
{
	int i;
	PlayerSelectMenuData *d = data;

#define ENTRY_COLS	8
#define	ENTRY_SPACING	7

	int x = pos.x;
	int y = CENTER_Y(
		pos, size,
		CDogsTextHeight() * ((strlen(letters) - 1) / ENTRY_COLS));

	UNUSED(menu);

	for (i = 0; i < (int)strlen(letters); i++)
	{
		Vec2i menuPos = Vec2iNew(
			x + (i % ENTRY_COLS) * ENTRY_SPACING,
			y + (i / ENTRY_COLS) * CDogsTextHeight());

		if (i == d->nameMenuSelection)
		{
			TextCharMasked(&gTextManager, letters[i], g, menuPos, colorRed);
		}
		else
		{
			TextCharMasked(&gTextManager, letters[i], g, menuPos, colorWhite);
		}
	}

	DisplayMenuItem(
		Vec2iNew(
			x + (i % ENTRY_COLS) * ENTRY_SPACING,
			y + (i / ENTRY_COLS) * CDogsTextHeight()),
		"(End)", i == d->nameMenuSelection, 0, colorBlack);
}

static int HandleInputNameMenu(int cmd, void *data)
{
	PlayerSelectMenuData *d = data;
	struct PlayerData *p = d->display.pData;

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

static void PostInputAppearanceMenu(menu_t *menu, int cmd, void *data)
{
	AppearanceMenuData *d = data;
	UNUSED(cmd);
	*d->property = menu->u.normal.index;
	SetPlayer(d->c, d->pData);
}

static menu_t *CreateAppearanceMenu(
	const char *name, AppearanceMenuData *data)
{
	menu_t *menu = MenuCreateNormal(name, "", MENU_TYPE_NORMAL, 0);
	int i;
	menu->u.normal.maxItems = 11;
	for (i = 0; i < data->menuCount; i++)
	{
		MenuAddSubmenu(menu, MenuCreateBack(data->strFunc(i)));
	}
	MenuSetPostInputFunc(menu, PostInputAppearanceMenu, data);
	return menu;
}

static void PostInputLoadTemplate(menu_t *menu, int cmd, void *data)
{
	if (cmd & CMD_BUTTON1)
	{
		PlayerSelectMenuData *d = data;
		struct PlayerData *p = d->display.pData;
		PlayerTemplate *t = &gPlayerTemplates[menu->u.normal.index];
		memset(p->name, 0, sizeof p->name);
		strncpy(p->name, t->name, sizeof p->name - 1);

		p->looks.face = t->face;
		p->looks.body = t->body;
		p->looks.arm = t->arms;
		p->looks.leg = t->legs;
		p->looks.skin = t->skin;
		p->looks.hair = t->hair;

		SetPlayer(d->display.c, p);
	}
}

// Load all the template names to the menu entries
static void PostEnterLoadTemplateNames(menu_t *menu, void *data)
{
	bool *isSave = (bool *)data;
	int numTemplates = PlayerTemplatesGetCount(gPlayerTemplates);
	for (int i = 0; i < numTemplates; i++)
	{
		// Add menu if necessary
		if (i == (int)menu->u.normal.subMenus.size)
		{
			MenuAddSubmenu(menu, MenuCreateBack(""));
		}
		menu_t *subMenu = CArrayGet(&menu->u.normal.subMenus, i);
		strcpy(subMenu->name, gPlayerTemplates[i].name);
	}
	if (*isSave && (int)menu->u.normal.subMenus.size == numTemplates)
	{
		MenuAddSubmenu(menu, MenuCreateBack("(new)"));
	}
}

static menu_t *CreateUseTemplateMenu(
	const char *name, PlayerSelectMenuData *data)
{
	menu_t *menu = MenuCreateNormal(name, "", MENU_TYPE_NORMAL, 0);
	menu->u.normal.maxItems = 11;
	MenuSetPostEnterFunc(menu, PostEnterLoadTemplateNames, &gFalse);
	MenuSetPostInputFunc(menu, PostInputLoadTemplate, data);
	return menu;
}

static void PostInputSaveTemplate(menu_t *menu, int cmd, void *data)
{
	if (cmd & CMD_BUTTON1)
	{
		PlayerSelectMenuData *d = data;
		struct PlayerData *p = d->display.pData;
		PlayerTemplate *t = &gPlayerTemplates[menu->u.normal.index];
		memset(t->name, 0, sizeof t->name);
		strncpy(t->name, p->name, sizeof t->name - 1);

		t->face = p->looks.face;
		t->body = p->looks.body;
		t->arms = p->looks.arm;
		t->legs = p->looks.leg;
		t->skin = p->looks.skin;
		t->hair = p->looks.hair;
	}
}

static void SaveTemplateDisplayTitle(
	menu_t *menu, GraphicsDevice *g, Vec2i pos, Vec2i size, void *data)
{
	PlayerSelectMenuData *d = data;
	char buf[256];

	UNUSED(menu);
	UNUSED(size);

	// Display "Save <template>..." title
	sprintf(buf, "Save %s...", d->display.pData->name);
	TextString(&gTextManager, buf, g, Vec2iAdd(pos, Vec2iNew(0, 0)));
}

static menu_t *CreateSaveTemplateMenu(
	const char *name, PlayerSelectMenuData *data)
{
	menu_t *menu = MenuCreateNormal(name, "", MENU_TYPE_NORMAL, 0);
	menu->u.normal.maxItems = 11;
	MenuSetPostEnterFunc(menu, PostEnterLoadTemplateNames, &gTrue);
	MenuSetPostInputFunc(menu, PostInputSaveTemplate, data);
	MenuSetCustomDisplay(menu, SaveTemplateDisplayTitle, data);
	return menu;
}

static void CheckReenableLoadMenu(menu_t *menu, void *data)
{
	menu_t *loadMenu = MenuGetSubmenuByName(menu, "Load");
	UNUSED(data);
	assert(loadMenu);
	loadMenu->isDisabled = PlayerTemplatesGetCount(gPlayerTemplates) == 0;
}
static menu_t *CreateCustomizeMenu(
	const char *name, PlayerSelectMenuData *data,
	Character *c, struct PlayerData *p);
static void ShuffleAppearance(void *data);
void PlayerSelectMenusCreate(
	PlayerSelectMenu *menu,
	int numPlayers, int player, Character *c, struct PlayerData *pData,
	EventHandlers *handlers, GraphicsDevice *graphics,
	InputConfig *inputConfig, const NameGen *ng)
{
	MenuSystem *ms = &menu->ms;
	PlayerSelectMenuData *data = &menu->data;
	struct PlayerData *p = pData;
	Vec2i pos = Vec2iZero();
	Vec2i size = Vec2iZero();
	int w = graphics->cachedConfig.Res.x;
	int h = graphics->cachedConfig.Res.y;

	data->nameMenuSelection = (int)strlen(letters);
	data->display.c = c;
	data->display.currentMenu = &ms->current;
	data->display.pData = pData;
	data->controls.inputConfig = inputConfig;
	data->controls.pData = pData;
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
		assert(0 && "not implemented");
		break;
	}
	MenuSystemInit(ms, handlers, graphics, pos, size);
	ms->align = MENU_ALIGN_LEFT;
	ms->root = ms->current = MenuCreateNormal(
		"",
		"",
		MENU_TYPE_NORMAL,
		0);
	MenuAddSubmenu(
		ms->root,
		MenuCreateCustom(
		"Name", DrawNameMenu, HandleInputNameMenu, data));

	MenuAddSubmenu(ms->root, CreateCustomizeMenu("Customize...", data, c, p));
	MenuAddSubmenu(
		ms->root,
		MenuCreateVoidFunc("Shuffle", ShuffleAppearance, data));

	MenuAddSubmenu(ms->root, CreateUseTemplateMenu("Load", data));
	MenuAddSubmenu(ms->root, CreateSaveTemplateMenu("Save", data));

	MenuAddSubmenu(ms->root, MenuCreateSeparator(""));
	MenuAddSubmenu(
		ms->root, MenuCreateNormal("Done", "", MENU_TYPE_NORMAL, 0));
	MenuAddExitType(ms, MENU_TYPE_RETURN);
	MenuSystemAddCustomDisplay(ms, MenuDisplayPlayer, data);
	MenuSystemAddCustomDisplay(ms, MenuDisplayPlayerControls, &data->controls);

	// Detect when there have been new player templates created,
	// to re-enable the load menu
	CheckReenableLoadMenu(ms->root, NULL);
	MenuSetPostEnterFunc(ms->root, CheckReenableLoadMenu, NULL);

	SetPlayer(c, pData);
}
static menu_t *CreateCustomizeMenu(
	const char *name, PlayerSelectMenuData *data,
	Character *c, struct PlayerData *p)
{
	menu_t *menu = MenuCreateNormal(name, "", MENU_TYPE_NORMAL, 0);

	data->faceData.c = c;
	data->faceData.pData = p;
	data->faceData.menuCount = FACE_COUNT;
	data->faceData.strFunc = IndexToFaceStr;
	data->faceData.property = &p->looks.face;
	MenuAddSubmenu(menu, CreateAppearanceMenu("Face", &data->faceData));

	data->skinData.c = c;
	data->skinData.pData = p;
	data->skinData.menuCount = SHADE_COUNT;
	data->skinData.strFunc = IndexToShadeStr;
	data->skinData.property = &p->looks.skin;
	MenuAddSubmenu(menu, CreateAppearanceMenu("Skin", &data->skinData));

	data->hairData.c = c;
	data->hairData.pData = p;
	data->hairData.menuCount = SHADE_COUNT;
	data->hairData.strFunc = IndexToShadeStr;
	data->hairData.property = &p->looks.hair;
	MenuAddSubmenu(menu, CreateAppearanceMenu("Hair", &data->hairData));

	data->armsData.c = c;
	data->armsData.pData = p;
	data->armsData.menuCount = SHADE_COUNT;
	data->armsData.strFunc = IndexToShadeStr;
	data->armsData.property = &p->looks.arm;
	MenuAddSubmenu(menu, CreateAppearanceMenu("Arms", &data->armsData));

	data->bodyData.c = c;
	data->bodyData.pData = p;
	data->bodyData.menuCount = SHADE_COUNT;
	data->bodyData.strFunc = IndexToShadeStr;
	data->bodyData.property = &p->looks.body;
	MenuAddSubmenu(menu, CreateAppearanceMenu("Body", &data->bodyData));

	data->legsData.c = c;
	data->legsData.pData = p;
	data->legsData.menuCount = SHADE_COUNT;
	data->legsData.strFunc = IndexToShadeStr;
	data->legsData.property = &p->looks.leg;
	MenuAddSubmenu(menu, CreateAppearanceMenu("Legs", &data->legsData));

	MenuAddSubmenu(menu, MenuCreateSeparator(""));
	MenuAddSubmenu(menu, MenuCreateBack("Back"));

	return menu;
}
static void ShuffleOne(AppearanceMenuData *data);
static void ShuffleAppearance(void *data)
{
	PlayerSelectMenuData *pData = data;
	struct PlayerData *p = pData->display.pData;
	char buf[512];
	NameGenMake(pData->nameGenerator, buf);
	strncpy(p->name, buf, 20);
	ShuffleOne(&pData->faceData);
	ShuffleOne(&pData->skinData);
	ShuffleOne(&pData->hairData);
	ShuffleOne(&pData->armsData);
	ShuffleOne(&pData->bodyData);
	ShuffleOne(&pData->legsData);
}
static void ShuffleOne(AppearanceMenuData *data)
{
	*data->property = rand() % data->menuCount;
	SetPlayer(data->c, data->pData);
}