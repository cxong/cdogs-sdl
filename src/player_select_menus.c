/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.

	Copyright (c) 2013, Cong Xu
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
#include <cdogs/files.h>
#include <cdogs/text.h>


static char letters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ !#?:.-0123456789";
static char smallLetters[] = "abcdefghijklmnopqrstuvwxyz !#?:.-0123456789";

#define PLAYER_FACE_COUNT   7
#define PLAYER_BODY_COUNT   9
#define PLAYER_SKIN_COUNT   3
#define PLAYER_HAIR_COUNT   8

static const char *faceNames[PLAYER_FACE_COUNT] = {
	"Jones",
	"Ice",
	"WarBaby",
	"Dragon",
	"Smith",
	"Lady",
	"Wolf"
};


static const char *shadeNames[PLAYER_BODY_COUNT] = {
	"Blue",
	"Green",
	"Red",
	"Silver",
	"Brown",
	"Purple",
	"Black",
	"Yellow",
	"White"
};

static const char *skinNames[PLAYER_SKIN_COUNT] = {
	"Caucasian",
	"Asian",
	"Black"
};

static const char *hairNames[PLAYER_HAIR_COUNT] = {
	"Red",
	"Silver",
	"Brown",
	"Gray",
	"Blonde",
	"White",
	"Golden",
	"Black"
};

PlayerTemplate gPlayerTemplates[MAX_TEMPLATE] =
{
	{ "-- empty --", 0, 0, 0, 0, 0, 0 },
	{ "-- empty --", 0, 0, 0, 0, 0, 0 },
	{ "-- empty --", 0, 0, 0, 0, 0, 0 },
	{ "-- empty --", 0, 0, 0, 0, 0, 0 },
	{ "-- empty --", 0, 0, 0, 0, 0, 0 },
	{ "-- empty --", 0, 0, 0, 0, 0, 0 },
	{ "-- empty --", 0, 0, 0, 0, 0, 0 },
	{ "-- empty --", 0, 0, 0, 0, 0, 0 },
	{ "-- empty --", 0, 0, 0, 0, 0, 0 },
	{ "-- empty --", 0, 0, 0, 0, 0, 0 }
};

void LoadPlayerTemplates(PlayerTemplate templates[MAX_TEMPLATE])
{
	int i, count;
	int fscanfres;
	FILE *f = fopen(GetConfigFilePath(PLAYER_TEMPLATE_FILENAME), "r");
	if (!f)
	{
		return;
	}
	i = 0;
	fscanfres = fscanf(f, "%d\n", &count);
	if (fscanfres < 1)
	{
		printf("Error reading " PLAYER_TEMPLATE_FILENAME " count\n");
		fclose(f);
		return;
	}
	while (i < MAX_TEMPLATE && i < count)
	{
		fscanfres = fscanf(f, "[%[^]]] %d %d %d %d %d %d\n",
			templates[i].name,
			&templates[i].head,
			&templates[i].body,
			&templates[i].arms,
			&templates[i].legs,
			&templates[i].skin,
			&templates[i].hair);
		if (fscanfres < 7)
		{
			printf("Error reading player %d\n", i);
			fclose(f);
			return;
		}
		i++;
	}
	fclose(f);
}

void SavePlayerTemplates(PlayerTemplate templates[MAX_TEMPLATE])
{
	FILE *f;
	int i;

	debug(D_NORMAL, "begin\n");

	f = fopen(GetConfigFilePath(PLAYER_TEMPLATE_FILENAME), "w");
	if (!f)
	{
		return;
	}

	fprintf(f, "%d\n", MAX_TEMPLATE);
	for (i = 0; i < MAX_TEMPLATE; i++)
	{
		fprintf(f, "[%s] %d %d %d %d %d %d\n",
			templates[i].name,
			templates[i].head,
			templates[i].body,
			templates[i].arms,
			templates[i].legs,
			templates[i].skin, templates[i].hair);
	}
	fclose(f);

	debug(D_NORMAL, "saved templates\n");
}


static int IndexToHead(int idx)
{
	switch (idx)
	{
	case 0:
		return FACE_JONES;
	case 1:
		return FACE_ICE;
	case 2:
		return FACE_WARBABY;
	case 3:
		return FACE_HAN;
	case 4:
		return FACE_BLONDIE;
	case 5:
		return FACE_LADY;
	case 6:
		return FACE_PIRAT2;
	}
	return FACE_BLONDIE;
}

static int IndexToSkin(int idx)
{
	switch (idx)
	{
	case 0:
		return SHADE_SKIN;
	case 1:
		return SHADE_ASIANSKIN;
	case 2:
		return SHADE_DARKSKIN;
	}
	return SHADE_SKIN;
}

static int IndexToHair(int idx)
{
	switch (idx)
	{
	case 0:
		return SHADE_RED;
	case 1:
		return SHADE_GRAY;
	case 2:
		return SHADE_BROWN;
	case 3:
		return SHADE_DKGRAY;
	case 4:
		return SHADE_YELLOW;
	case 5:
		return SHADE_LTGRAY;
	case 6:
		return SHADE_GOLDEN;
	case 7:
		return SHADE_BLACK;
	}
	return SHADE_SKIN;
}

static int IndexToShade(int idx)
{
	switch (idx)
	{
	case 0:
		return SHADE_BLUE;
	case 1:
		return SHADE_GREEN;
	case 2:
		return SHADE_RED;
	case 3:
		return SHADE_GRAY;
	case 4:
		return SHADE_BROWN;
	case 5:
		return SHADE_PURPLE;
	case 6:
		return SHADE_DKGRAY;
	case 7:
		return SHADE_YELLOW;
	case 8:
		return SHADE_LTGRAY;
	}
	return SHADE_BLUE;
};

static void SetPlayer(Character *c, struct PlayerData *data)
{
	data->looks.armedBody = BODY_ARMED;
	data->looks.unarmedBody = BODY_UNARMED;
	CharacterSetLooks(c, &data->looks);
	c->speed = 256;
	c->maxHealth = 200;
}


static void DrawNameMenu(GraphicsDevice *g, Vec2i pos, Vec2i size, void *data)
{
	int i;
	PlayerSelectMenuData *d = data;

#define ENTRY_COLS	8
#define	ENTRY_SPACING	7

	int x = pos.x;
	int y = CENTER_Y(
		pos, size,
		CDogsTextHeight() * ((strlen(letters) - 1) / ENTRY_COLS));

	for (i = 0; i < (int)strlen(letters); i++)
	{
		Vec2i menuPos = Vec2iNew(
			x + (i % ENTRY_COLS) * ENTRY_SPACING,
			y + (i / ENTRY_COLS) * CDogsTextHeight());

		if (i == d->nameMenuSelection)
		{
			DrawTextCharMasked(letters[i], g, menuPos, colorRed);
		}
		else
		{
			DrawTextCharMasked(letters[i], g, menuPos, colorWhite);
		}
	}

	DisplayMenuItem(
		x + (i % ENTRY_COLS) * ENTRY_SPACING,
		y + (i / ENTRY_COLS) * CDogsTextHeight(),
		"(End)", i == d->nameMenuSelection);
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
	*d->property = d->func(menu->u.normal.index);
	SetPlayer(d->c, d->pData);
}

static menu_t *CreateAppearanceMenu(
	const char *name, AppearanceMenuData *faceData)
{
	menu_t *menu = MenuCreateNormal(name, "", MENU_TYPE_NORMAL, 0);
	int i;
	for (i = 0; i < faceData->menuCount; i++)
	{
		MenuAddSubmenu(menu, MenuCreateBack(faceData->menu[i]));
	}
	MenuSetPostInputFunc(menu, PostInputAppearanceMenu, faceData);
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

		p->looks.face = t->head;
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
	int i;
	UNUSED(data);
	for (i = 0; i < MAX_TEMPLATE; i++)
	{
		strcpy(menu->u.normal.subMenus[i].name, gPlayerTemplates[i].name);
	}
}

static menu_t *CreateUseTemplateMenu(
	const char *name, PlayerSelectMenuData *data)
{
	menu_t *menu = MenuCreateNormal(name, "", MENU_TYPE_NORMAL, 0);
	int i;
	for (i = 0; i < MAX_TEMPLATE; i++)
	{
		MenuAddSubmenu(menu, MenuCreateBack(""));
	}
	MenuSetPostEnterFunc(menu, PostEnterLoadTemplateNames, data);
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

		t->head = p->looks.face;
		t->body = p->looks.body;
		t->arms = p->looks.arm;
		t->legs = p->looks.leg;
		t->skin = p->looks.skin;
		t->hair = p->looks.hair;
	}
}

static void SaveTemplateDisplayTitle(
	GraphicsDevice *g, Vec2i pos, Vec2i size, void *data)
{
	PlayerSelectMenuData *d = data;
	char buf[256];

	UNUSED(size);

	// Display "Save <template>..." title
	sprintf(buf, "Save %s...", d->display.pData->name);
	DrawTextString(buf, g, Vec2iAdd(pos, Vec2iNew(0, 0)));
}

static menu_t *CreateSaveTemplateMenu(
	const char *name, PlayerSelectMenuData *data)
{
	menu_t *menu = MenuCreateNormal(name, "", MENU_TYPE_NORMAL, 0);
	int i;
	for (i = 0; i < MAX_TEMPLATE; i++)
	{
		MenuAddSubmenu(menu, MenuCreateBack(""));
	}
	MenuSetPostEnterFunc(menu, PostEnterLoadTemplateNames, data);
	MenuSetPostInputFunc(menu, PostInputSaveTemplate, data);
	MenuSetCustomDisplay(menu, SaveTemplateDisplayTitle, data);
	return menu;
}

void PlayerSelectMenusCreate(
	PlayerSelectMenu *menu,
	int numPlayers, int player, Character *c, struct PlayerData *pData,
	InputDevices *input, GraphicsDevice *graphics, KeyConfig *key)
{
	MenuSystem *ms = &menu->ms;
	PlayerSelectMenuData *data = &menu->data;
	struct PlayerData *p = pData;
	Vec2i pos = Vec2iZero();
	Vec2i size = Vec2iZero();
	int w = graphics->cachedConfig.ResolutionWidth;
	int h = graphics->cachedConfig.ResolutionHeight;

	data->nameMenuSelection = (int)strlen(letters);
	data->display.c = c;
	data->display.currentMenu = &ms->current;
	data->display.pData = pData;
	data->controls.keys = key;
	data->controls.pData = pData;

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
	default:
		assert(0 && "not implemented");
		break;
	}
	MenuSystemInit(ms, input, graphics, pos, size);
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

	data->faceData.c = c;
	data->faceData.pData = p;
	data->faceData.menu = faceNames;
	data->faceData.menuCount = PLAYER_FACE_COUNT;
	data->faceData.property = &p->looks.face;
	data->faceData.func = IndexToHead;
	MenuAddSubmenu(ms->root, CreateAppearanceMenu("Face", &data->faceData));

	data->skinData.c = c;
	data->skinData.pData = p;
	data->skinData.menu = skinNames;
	data->skinData.menuCount = PLAYER_SKIN_COUNT;
	data->skinData.property = &p->looks.skin;
	data->skinData.func = IndexToSkin;
	MenuAddSubmenu(ms->root, CreateAppearanceMenu("Skin", &data->skinData));

	data->hairData.c = c;
	data->hairData.pData = p;
	data->hairData.menu = hairNames;
	data->hairData.menuCount = PLAYER_HAIR_COUNT;
	data->hairData.property = &p->looks.hair;
	data->hairData.func = IndexToHair;
	MenuAddSubmenu(ms->root, CreateAppearanceMenu("Hair", &data->hairData));

	data->armsData.c = c;
	data->armsData.pData = p;
	data->armsData.menu = shadeNames;
	data->armsData.menuCount = PLAYER_BODY_COUNT;
	data->armsData.property = &p->looks.arm;
	data->armsData.func = IndexToShade;
	MenuAddSubmenu(ms->root, CreateAppearanceMenu("Arms", &data->armsData));

	data->bodyData.c = c;
	data->bodyData.pData = p;
	data->bodyData.menu = shadeNames;
	data->bodyData.menuCount = PLAYER_BODY_COUNT;
	data->bodyData.property = &p->looks.body;
	data->bodyData.func = IndexToShade;
	MenuAddSubmenu(ms->root, CreateAppearanceMenu("Body", &data->bodyData));

	data->legsData.c = c;
	data->legsData.pData = p;
	data->legsData.menu = shadeNames;
	data->legsData.menuCount = PLAYER_BODY_COUNT;
	data->legsData.property = &p->looks.leg;
	data->legsData.func = IndexToShade;
	MenuAddSubmenu(ms->root, CreateAppearanceMenu("Legs", &data->legsData));

	MenuAddSubmenu(ms->root, CreateUseTemplateMenu("Load", data));
	MenuAddSubmenu(ms->root, CreateSaveTemplateMenu("Save", data));

	MenuAddSubmenu(ms->root, MenuCreateSeparator(""));
	MenuAddSubmenu(ms->root, MenuCreateReturn("Done", 0));
	MenuAddExitType(ms, MENU_TYPE_RETURN);
	MenuSystemAddCustomDisplay(ms, MenuDisplayPlayer, data);
	MenuSystemAddCustomDisplay(ms, MenuDisplayPlayerControls, &data->controls);

	SetPlayer(c, pData);
}
