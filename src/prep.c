/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (C) 1995 Ronny Wester
    Copyright (C) 2003 Jeremy Chin
    Copyright (C) 2003-2007 Lucas Martin-King

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    This file incorporates work covered by the following copyright and
    permission notice:

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
#include "prep.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <SDL_timer.h>

#include <cdogs/actors.h>
#include <cdogs/blit.h>
#include <cdogs/config.h>
#include <cdogs/draw.h>
#include <cdogs/files.h>
#include <cdogs/grafx.h>
#include <cdogs/input.h>
#include <cdogs/joystick.h>
#include <cdogs/keyboard.h>
#include <cdogs/pic_manager.h>
#include <cdogs/sounds.h>
#include <cdogs/text.h>
#include <cdogs/utils.h>

#include "menu.h"


#define MODE_MAIN           0
#define MODE_SELECTNAME     1
#define MODE_SELECTFACE     2
#define MODE_SELECTSKIN     3
#define MODE_SELECTHAIR     4
#define MODE_SELECTARMS     5
#define MODE_SELECTBODY     6
#define MODE_SELECTLEGS     7
#define MODE_LOADTEMPLATE   8
#define MODE_SAVETEMPLATE   9
#define MODE_DONE           10

#define MENU_COUNT  11

#define PLAYER_FACE_COUNT   7
#define PLAYER_BODY_COUNT   9
#define PLAYER_SKIN_COUNT   3
#define PLAYER_HAIR_COUNT   8


#define AVAILABLE_FACES PLAYER_FACE_COUNT


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

static const char *mainMenu[MENU_COUNT] = {
	"",
	"Change name",
	"Select face",
	"Select skin",
	"Select hair",
	"Select arms",
	"Select body",
	"Select legs",
	"Use template",
	"Save template",
	"Done"
};

const char *endChoice = "(End)";


struct PlayerTemplate {
	char name[20];
	int head;
	int body;
	int arms;
	int legs;
	int skin;
	int hair;
};

#define MAX_TEMPLATE  10
struct PlayerTemplate templates[MAX_TEMPLATE] = {
	{"-- empty --", 0, 0, 0, 0, 0, 0},
	{"-- empty --", 0, 0, 0, 0, 0, 0},
	{"-- empty --", 0, 0, 0, 0, 0, 0},
	{"-- empty --", 0, 0, 0, 0, 0, 0},
	{"-- empty --", 0, 0, 0, 0, 0, 0},
	{"-- empty --", 0, 0, 0, 0, 0, 0},
	{"-- empty --", 0, 0, 0, 0, 0, 0},
	{"-- empty --", 0, 0, 0, 0, 0, 0},
	{"-- empty --", 0, 0, 0, 0, 0, 0},
	{"-- empty --", 0, 0, 0, 0, 0, 0}
};

void LoadTemplates(void)
{
	FILE *f;
	int i, count;

	f = fopen(GetConfigFilePath("players.cnf"), "r");
	if (f) {
		int fscanfres;
		i = 0;
		fscanfres = fscanf(f, "%d\n", &count);
		if (fscanfres < 1) {
			printf("Error reading players.cnf count\n");
			fclose(f);
			return;
		}
		while (i < MAX_TEMPLATE && i < count) {
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
}

void SaveTemplates(void)
{
	FILE *f;
	int i;

	debug(D_NORMAL, "begin\n");

	f = fopen(GetConfigFilePath("players.cnf"), "w");
	if (f) {
		fprintf(f, "%d\n", MAX_TEMPLATE);
		for (i = 0; i < MAX_TEMPLATE; i++) {
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
}

static void ShowPlayerControls(int x, KeyConfig *config)
{
	char s[256];
	int y = gGraphicsDevice.cachedConfig.ResolutionHeight - (gGraphicsDevice.cachedConfig.ResolutionHeight / 6);

	if (config->Device == INPUT_DEVICE_KEYBOARD)
	{
		sprintf(s, "(%s, %s, %s, %s, %s and %s)",
			SDL_GetKeyName(config->Keys.left),
			SDL_GetKeyName(config->Keys.right),
			SDL_GetKeyName(config->Keys.up),
			SDL_GetKeyName(config->Keys.down),
			SDL_GetKeyName(config->Keys.button1),
			SDL_GetKeyName(config->Keys.button2));
		if (TextGetStringWidth(s) < 125)
		{
			CDogsTextStringAt(x, y, s);
		}
		else
		{
			sprintf(s, "(%s, %s, %s,",
				SDL_GetKeyName(config->Keys.left),
				SDL_GetKeyName(config->Keys.right),
				SDL_GetKeyName(config->Keys.up));
			CDogsTextStringAt(x, y - 10, s);
			sprintf(s, "%s, %s and %s)",
				SDL_GetKeyName(config->Keys.down),
				SDL_GetKeyName(config->Keys.button1),
				SDL_GetKeyName(config->Keys.button2));
			CDogsTextStringAt(x, y, s);
		}
	}
	else if (config->Device == INPUT_DEVICE_MOUSE)
	{
		sprintf(s, "(mouse wheel to scroll, left and right click)");
		CDogsTextStringAt(x, y, s);
	}
	else
	{
		sprintf(s, "(%s)", InputDeviceName(config->Device));
		CDogsTextStringAt(x, y, s);
	}
}

static void ShowSelection(int x, struct PlayerData *data, Character *ch)
{
	DisplayPlayer(x, data->name, ch, 0);

	if (data->weaponCount == 0)
	{
		CDogsTextStringAt(
			x + 40,
			(gGraphicsDevice.cachedConfig.ResolutionHeight / 10) + 20,
			"None selected...");
	}
	else
	{
		int i;
		for (i = 0; i < data->weaponCount; i++)
		{
			CDogsTextStringAt(
				x + 40,
				(gGraphicsDevice.cachedConfig.ResolutionHeight / 10) + 20 + i * CDogsTextHeight(),
				gGunDescriptions[data->weapons[i]].gunName);
		}
	}
}

static int NameSelection(int x, int idx, struct PlayerData *data, int cmd)
{
	int i;
	int y;

	//char s[2];
	static char letters[] =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ !#?:.-0123456789";
	static char smallLetters[] =
		"abcdefghijklmnopqrstuvwxyz !#?:.-0123456789";
	static int selection[2] = { -1, -1 };

	// Kludge since Watcom won't let me initialize selection with a strlen()
	if (selection[0] < 0)
	{
		selection[0] = selection[1] = (int)strlen(letters);
	}

	if (cmd & CMD_BUTTON1)
	{
		if (selection[idx] == (int)strlen(letters))
		{
			SoundPlay(&gSoundDevice, SND_LAUNCH);
			return 0;
		}

		if (strlen(data->name) < sizeof(data->name) - 1)
		{
			size_t l = strlen(data->name);
			data->name[l + 1] = 0;
			if (l > 0 && data->name[l - 1] != ' ')
			{
				data->name[l] = smallLetters[selection[idx]];
			}
			else
			{
				data->name[l] = letters[selection[idx]];
			}
			SoundPlay(&gSoundDevice, SND_MACHINEGUN);
		}
		else
		{
			SoundPlay(&gSoundDevice, SND_KILL);
		}
	}
	else if (cmd & CMD_BUTTON2)
	{
		if (data->name[0]) {
			data->name[strlen(data->name) - 1] = 0;
			SoundPlay(&gSoundDevice, SND_BANG);
		}
		else
		{
			SoundPlay(&gSoundDevice, SND_KILL);
		}
	}
	else if (cmd & CMD_LEFT)
	{
		if (selection[idx] > 0)
		{
			selection[idx]--;
			SoundPlay(&gSoundDevice, SND_DOOR);
		}
	}
	else if (cmd & CMD_RIGHT)
	{
		if (selection[idx] < (int)strlen(letters))
		{
			selection[idx]++;
			SoundPlay(&gSoundDevice, SND_DOOR);
		}
	}
	else if (cmd & CMD_UP)
	{
		if (selection[idx] > 9)
		{
			selection[idx] -= 10;
			SoundPlay(&gSoundDevice, SND_DOOR);
		}
	}
	else if (cmd & CMD_DOWN)
	{
		if (selection[idx] < (int)strlen(letters) - 9)
		{
			selection[idx] += 10;
			SoundPlay(&gSoundDevice, SND_DOOR);
		}
		else if (selection[idx] < (int)strlen(letters))
		{
			selection[idx] = (int)strlen(letters);
			SoundPlay(&gSoundDevice, SND_DOOR);
		}
	}

	#define ENTRY_COLS	10
	#define	ENTRY_SPACING	12

	y = (int)CenterY(((CDogsTextHeight() * ((strlen(letters) - 1) / ENTRY_COLS) )));

	// TODO: support up to 4 player screens
	if (gOptions.numPlayers == 2 && idx == CHARACTER_PLAYER1)
	{
		x = CenterOf(
			0,
			gGraphicsDevice.cachedConfig.ResolutionWidth / 2
			,
			(ENTRY_SPACING * (ENTRY_COLS - 1)) + CDogsTextCharWidth('a'));
	}
	else if (gOptions.numPlayers == 2 && idx == CHARACTER_PLAYER2)
	{
		x = CenterOf(
			gGraphicsDevice.cachedConfig.ResolutionWidth / 2,
			gGraphicsDevice.cachedConfig.ResolutionWidth,
			(ENTRY_SPACING * (ENTRY_COLS - 1)) + CDogsTextCharWidth('a'));
	}
	else
	{
		x = CenterX((ENTRY_SPACING * (ENTRY_COLS - 1)) + CDogsTextCharWidth('a'));
	}

	// Draw selection

	//s[1] = 0;
	for (i = 0; i < (int)strlen(letters); i++)
	{
		//s[0] = letters[i];

		CDogsTextGoto(x + (i % ENTRY_COLS) * ENTRY_SPACING,
			 y + (i / ENTRY_COLS) * CDogsTextHeight());

		if (i == selection[idx])
		{
			CDogsTextCharWithTable(letters[i], &tableFlamed);
		}
		else
			CDogsTextChar(letters[i]);
/*
		DisplayMenuItem(x + (i % 10) * 12,
				80 + (i / 10) * CDogsTextHeight(), s,
				i == selection[idx]);
*/
	}

	DisplayMenuItem(
		x + (i % ENTRY_COLS) * ENTRY_SPACING,
		y + (i / ENTRY_COLS) * CDogsTextHeight(),
		endChoice, i == selection[idx]);

	return 1;
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

static int AppearanceSelection(
	const char **menu, int menuCount,
	int x, int idx,
	struct PlayerData *data, int *property, int (*func)(int), int cmd, int *selection)
{
	int y;
	int i;
	int hasChanged = 0;

	debug(D_NORMAL, "\n");

	if (cmd & (CMD_BUTTON1 | CMD_BUTTON2))
	{
		return 0;
	}
	else if (cmd & (CMD_LEFT | CMD_UP))
	{
		if (selection[idx] > 0)
		{
			selection[idx]--;
			*property = func(selection[idx]);
		}
		else if (selection[idx] == 0)
		{
			selection[idx] = menuCount - 1;
			*property = func(selection[idx]);
		}
		hasChanged = 1;
	}
	else if (cmd & (CMD_RIGHT | CMD_DOWN))
	{
		if (selection[idx] < menuCount - 1)
		{
			selection[idx]++;
			*property = func(selection[idx]);
		}
		else if (selection[idx] == menuCount - 1)
		{
			selection[idx] = 0;
			*property = func(selection[idx]);
		}
		hasChanged = 1;
	}

	if (hasChanged)
	{
		SetPlayer(&gCampaign.Setting.characters.players[idx], data);
	}

	y = CenterY((menuCount * CDogsTextHeight()));

	for (i = 0; i < menuCount; i++)
	{
		DisplayMenuItem(
			x, y + i * CDogsTextHeight(), menu[i], i == selection[idx]);
	}

	return 1;
}

static int FaceSelection(int x, int idx, struct PlayerData *data, int cmd)
{
	static int selection[2] = { 0, 1 };

	return AppearanceSelection(
		faceNames,
		AVAILABLE_FACES,
		x, idx, data, &data->looks.face, IndexToHead, cmd, selection);
}

static int SkinSelection(int x, int idx, struct PlayerData *data, int cmd)
{
	static int selection[2] = { 0, 1 };

	return AppearanceSelection(
		skinNames, PLAYER_SKIN_COUNT,
		x, idx, data, &data->looks.skin, IndexToSkin, cmd, selection);
}

static int HairSelection(int x, int idx, struct PlayerData *data, int cmd)
{
	static int selection[2] = { 0, 1 };

	return AppearanceSelection(
		hairNames, PLAYER_HAIR_COUNT,
		x, idx, data, &data->looks.hair, IndexToHair, cmd, selection);
}

static int BodyPartSelection(
	int x, int idx,
	struct PlayerData *data, int cmd, int *property, int *selection)
{
	int i;
	int y;

	if (cmd & (CMD_BUTTON1 | CMD_BUTTON2))
	{
		return 0;
	} else if (cmd & (CMD_LEFT | CMD_UP)) {
		if (*selection > 0) {
			(*selection)--;
			*property = IndexToShade(*selection);
		}
		else if (*selection == 0)
		{
			(*selection) = PLAYER_BODY_COUNT - 1;
			*property = IndexToShade(*selection);
		}
	} else if (cmd & (CMD_RIGHT | CMD_DOWN)) {
		if (*selection < PLAYER_BODY_COUNT - 1) {
			(*selection)++;
			*property = IndexToShade(*selection);
		}
		else if (*selection == PLAYER_BODY_COUNT - 1)
		{
			(*selection) = 0;
			*property = IndexToShade(*selection);
		}
	}

	y = CenterY((PLAYER_BODY_COUNT * CDogsTextHeight()));

	SetPlayer(&gCampaign.Setting.characters.players[idx], data);
	for (i = 0; i < PLAYER_BODY_COUNT; i++)
		DisplayMenuItem(x, y + i * CDogsTextHeight(), shadeNames[i],
				i == *selection);

	return 1;
}

static int ArmSelection(int x, int idx, struct PlayerData *data, int cmd)
{
	static int selection[2] = { 0, 0 };

	return BodyPartSelection(
		x, idx, data, cmd, &data->looks.arm, &selection[idx]);
}

static int BodySelection(int x, int idx, struct PlayerData *data,
			 int cmd)
{
	static int selection[2] = { 0, 0 };

	return BodyPartSelection(
		x, idx, data, cmd, &data->looks.body, &selection[idx]);
}

static int LegSelection(int x, int idx, struct PlayerData *data, int cmd)
{
	static int selection[2] = { 0, 0 };

	return BodyPartSelection(
		x, idx, data, cmd, &data->looks.leg, &selection[idx]);
}

static int WeaponSelection(
	int x, int idx, struct PlayerData *data, int cmd, int done)
{
	int i;
	int y;
	static int selection[2] = { 0, 0 };

	debug(D_VERBOSE, "\n");

	if (selection[idx] > gMission.weaponCount)
	{
		selection[idx] = gMission.weaponCount;
	}

	if (cmd & CMD_BUTTON1)
	{
		if (selection[idx] == gMission.weaponCount)
		{
			SoundPlay(&gSoundDevice, SND_KILL2);
			return data->weaponCount > 0 ? 0 : 1;
		}

		if (data->weaponCount < MAX_WEAPONS)
		{
			for (i = 0; i < data->weaponCount; i++)
			{
				if ((int)data->weapons[i] ==
						gMission.availableWeapons[selection[idx]])
				{
					return 1;
				}
			}

			data->weapons[data->weaponCount] =
				gMission.availableWeapons[selection[idx]];
			data->weaponCount++;

			SoundPlay(&gSoundDevice, SND_SHOTGUN);
		}
		else
		{
			SoundPlay(&gSoundDevice, SND_KILL);
		}
	} else if (cmd & CMD_BUTTON2) {
		if (data->weaponCount) {
			data->weaponCount--;
			SoundPlay(&gSoundDevice, SND_PICKUP);
			done = 0;
		}
	}
	else if (cmd & (CMD_LEFT | CMD_UP))
	{
		if (selection[idx] > 0)
		{
			selection[idx]--;
			SoundPlay(&gSoundDevice, SND_SWITCH);
		}
		else if (selection[idx] == 0)
		{
			selection[idx] = gMission.weaponCount;
			SoundPlay(&gSoundDevice, SND_SWITCH);
		}
		done = 0;
	}
	else if (cmd & (CMD_RIGHT | CMD_DOWN))
	{
		if (selection[idx] < gMission.weaponCount)
		{
			selection[idx]++;
			SoundPlay(&gSoundDevice, SND_SWITCH);
		}
		else if (selection[idx] == gMission.weaponCount)
		{
			selection[idx] = 0;
			SoundPlay(&gSoundDevice, SND_SWITCH);
		}
		done = 0;
	}

	if (!done) {
		y = CenterY((CDogsTextHeight() * gMission.weaponCount));

		for (i = 0; i < gMission.weaponCount; i++)
		{
			DisplayMenuItem(
				x,
				y + i * CDogsTextHeight(),
				gGunDescriptions[gMission.availableWeapons[i]].gunName,
				i == selection[idx]);
		}

		DisplayMenuItem(x, y + i * CDogsTextHeight(), endChoice, i == selection[idx]);
	}

	return !done;
}

void UseTemplate(int character, struct PlayerData *data,
		 struct PlayerTemplate *t)
{
	memset(data->name, 0, sizeof(data->name));
	strncpy(data->name, t->name, sizeof(data->name) - 1);

	data->looks.face = IndexToHead(t->head < AVAILABLE_FACES ? t->head : 0);
	data->looks.body = t->body;
	data->looks.arm = t->arms;
	data->looks.leg = t->legs;
	data->looks.skin = IndexToSkin(t->skin);
	data->looks.hair = IndexToHair(t->hair);

	SetPlayer(&gCampaign.Setting.characters.players[character], data);
}

void SaveTemplate(struct PlayerData *data, struct PlayerTemplate *t)
{
	memset(t->name, 0, sizeof(t->name));
	strncpy(t->name, data->name, sizeof(t->name) - 1);

	t->head = data->looks.face - FACE_JONES;
	t->body = data->looks.body;
	t->arms = data->looks.arm;
	t->legs = data->looks.leg;
	t->skin = data->looks.skin - SHADE_SKIN;
	t->hair = data->looks.hair - SHADE_RED;
}

static int TemplateSelection(
	int loadFlag, int x, int idx, struct PlayerData *data, int cmd)
{
	int i;
	int y;
	static int selection[2] = { 0, 0 };

	if (cmd & CMD_BUTTON1)
	{
		if (loadFlag)
		{
			UseTemplate(idx, data, &templates[selection[idx]]);
		}
		else
		{
			SaveTemplate(data, &templates[selection[idx]]);
		}
		SoundPlay(&gSoundDevice, rand() % SND_COUNT);
		return 0;
	}
	else if (cmd & CMD_BUTTON2)
	{
		SoundPlay(&gSoundDevice, rand() % SND_COUNT);
		return 0;
	}
	else if (cmd & (CMD_LEFT | CMD_UP))
	{
		if (selection[idx] > 0)
		{
			selection[idx]--;
			SoundPlay(&gSoundDevice, SND_SWITCH);
		}
		else if (selection[idx] == 0)
		{
			selection[idx] = MAX_TEMPLATE - 1;
			SoundPlay(&gSoundDevice, SND_SWITCH);
		}
	}
	else if (cmd & (CMD_RIGHT | CMD_DOWN))
	{
		if (selection[idx] < MAX_TEMPLATE - 1)
		{
			selection[idx]++;
			SoundPlay(&gSoundDevice, SND_SWITCH);
		}
		else if (selection[idx] == MAX_TEMPLATE - 1)
		{
			selection[idx] = 0;
			SoundPlay(&gSoundDevice, SND_SWITCH);
		}
	}

	y = CenterY((CDogsTextHeight() * MAX_TEMPLATE));

	if (!loadFlag) {
		CDogsTextStringAt(x, y - 4 - CDogsTextHeight(), "Save ");
		CDogsTextString(data->name);
		CDogsTextString("...");
	}

	for (i = 0; i < MAX_TEMPLATE; i++)
	{
		DisplayMenuItem(
			x, y + i * CDogsTextHeight(),
			templates[i].name, i == selection[idx]);
	}

	return 1;
}

static int MainMenu(int x, int idx, int cmd)
{
	int i;
	int y;
	static int selection[2] = { MODE_DONE, MODE_DONE };

	if (cmd & (CMD_BUTTON1 | CMD_BUTTON2))
	{
		return selection[idx];
	}
	else if (cmd & (CMD_LEFT | CMD_UP))
	{
		if (selection[idx] > MODE_SELECTNAME)
		{
			selection[idx]--;
		}
		else if (selection[idx] == MODE_SELECTNAME)
		{
			selection[idx] = MODE_DONE;
		}
	}
	else if (cmd & (CMD_RIGHT | CMD_DOWN))
	{
		if (selection[idx] < MODE_DONE)
		{
			selection[idx]++;
		}
		else if (selection[idx] == MODE_DONE)
		{
			selection[idx] = MODE_SELECTNAME;
		}
	}

	y = CenterY((CDogsTextHeight() * MENU_COUNT));

	for (i = 1; i < MENU_COUNT; i++)
	{
		DisplayMenuItem(
			x, y + i * CDogsTextHeight(), mainMenu[i], selection[idx] == i);
	}

	return MODE_MAIN;
}

static int MakeSelection(
	int mode, int x, int character, struct PlayerData *data, int cmd)
{
	switch (mode) {
		case MODE_MAIN:
			mode = MainMenu(x, character, cmd);
			break;

		case MODE_SELECTNAME:
			if (!NameSelection(x, character, data, cmd))
				mode = MODE_MAIN;
			break;

		case MODE_SELECTFACE:
			if (!FaceSelection(x, character, data, cmd))
				mode = MODE_MAIN;
			break;

		case MODE_SELECTSKIN:
			if (!SkinSelection(x, character, data, cmd))
				mode = MODE_MAIN;
			break;

		case MODE_SELECTHAIR:
			if (!HairSelection(x, character, data, cmd))
				mode = MODE_MAIN;
			break;

		case MODE_SELECTARMS:
			if (!ArmSelection(x, character, data, cmd))
				mode = MODE_MAIN;
			break;

		case MODE_SELECTBODY:
			if (!BodySelection(x, character, data, cmd))
				mode = MODE_MAIN;
			break;

		case MODE_SELECTLEGS:
			if (!LegSelection(x, character, data, cmd))
				mode = MODE_MAIN;
			break;

		case MODE_LOADTEMPLATE:
			if (!TemplateSelection(1, x, character, data, cmd))
				mode = MODE_MAIN;
			break;

		case MODE_SAVETEMPLATE:
			if (!TemplateSelection(0, x, character, data, cmd))
				mode = MODE_MAIN;
			break;
	}
	DisplayPlayer(
		x,
		data->name,
		&gCampaign.Setting.characters.players[character],
		mode == MODE_SELECTNAME);

	return mode;
}

static menu_t *MenuCreateName(const char *name)
{
	menu_t *menu = MenuCreateNormal(
		name,
		"",
		MENU_TYPE_NORMAL,
		0);
	MenuAddSubmenu(menu, MenuCreateBack("(End)"));
	return menu;
}

static void MenuCreatePlayerSelection(
	MenuSystem *ms,
	int numPlayers, int player, InputDevices *input, GraphicsDevice *graphics)
{
	Vec2i pos = Vec2iZero();
	Vec2i size = Vec2iZero();
	int w = graphics->cachedConfig.ResolutionWidth;
	int h = graphics->cachedConfig.ResolutionHeight;
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
	ms->root = ms->current = MenuCreateNormal(
		"",
		"",
		MENU_TYPE_NORMAL,
		0);
	MenuAddSubmenu(
		ms->root,
		MenuCreateName("Name"));
	MenuAddSubmenu(ms->root, MenuCreateSeparator(""));
	MenuAddSubmenu(ms->root, MenuCreateBack("Done"));
}

int PlayerSelection(int numPlayers, GraphicsDevice *graphics)
{
	int modes[MAX_PLAYERS];
	int i;
	MenuSystem ms[MAX_PLAYERS];
	for (i = 0; i < numPlayers; i++)
	{
		MenuCreatePlayerSelection(&ms[i], numPlayers, i, &gInputDevices, graphics);
	}

	for (i = 0; i < MAX_PLAYERS; i++)
	{
		if (i < numPlayers)
		{
			modes[i] = MODE_MAIN;
		}
		else
		{
			modes[i] = MODE_DONE;
		}
	}
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		SetPlayer(&gCampaign.Setting.characters.players[i], &gPlayerDatas[i]);
	}

	KeyInit(&gInputDevices.keyboard);
	for (;;)
	{
		int cmds[MAX_PLAYERS];
		int isDone = 1;
		InputPoll(&gInputDevices, SDL_GetTicks());
		GraphicsBlitBkg(graphics);
		GetPlayerCmds(&cmds);

		/*for (i = 0; i < numPlayers; i++)
		{
			MenuDisplay(&ms[i]);
		}*/

		if (KeyIsPressed(&gInputDevices.keyboard, SDLK_ESCAPE))
		{
			return 0; // hack to allow exit
		}

		for (i = 0; i < numPlayers; i++)
		{
			MenuProcessCmd(&ms[i], cmds[i]);
		}

		switch (numPlayers)
		{
			case 1:
				modes[0] = MakeSelection(modes[0], CenterX(50), CHARACTER_PLAYER1, &gPlayerDatas[0], cmds[0]);
				break;
			case 2:
				modes[0] = MakeSelection(modes[0], CenterOfLeft(50), CHARACTER_PLAYER1, &gPlayerDatas[0], cmds[0]);
				modes[1] = MakeSelection(modes[1], CenterOfRight(50), CHARACTER_PLAYER2, &gPlayerDatas[1], cmds[1]);
				break;
			default:
				assert(0 && "not implemented");
				break;
		}
		
		for (i = 0; i < MAX_PLAYERS; i++)
		{
			if (modes[i] != MODE_DONE)
			{
				isDone = 0;
			}
		}
		if (isDone)
		{
			break;
		}

		BlitFlip(graphics, &gConfig.Graphics);
		SDL_Delay(10);
	}

	return 1;
}

int PlayerEquip(GraphicsDevice *graphics)
{
	int dones[MAX_PLAYERS];
	int i;

	debug(D_NORMAL, "\n");

	for (i = 0; i < MAX_PLAYERS; i++)
	{
		if (i < gOptions.numPlayers)
		{
			dones[i] = 0;
		}
		else
		{
			dones[i] = 1;
		}
	}

	for (;;)
	{
		int cmds[MAX_PLAYERS];
		int isDone = 1;
		InputPoll(&gInputDevices, SDL_GetTicks());
		GraphicsBlitBkg(graphics);
		GetPlayerCmds(&cmds);

		if (KeyIsPressed(&gInputDevices.keyboard, SDLK_ESCAPE))
		{
			return 0; // hack to exit from menu
		}

		switch (gOptions.numPlayers)
		{
			case 1:
				dones[0] = !WeaponSelection(CenterX(80), CHARACTER_PLAYER1, &gPlayerDatas[0], cmds[0], dones[0]);
				ShowSelection(
					CenterX(80),
					&gPlayerDatas[0],
					&gCampaign.Setting.characters.players[0]);
				ShowPlayerControls(CenterX(100), &gConfig.Input.PlayerKeys[0]);
				break;
			case 2:
				dones[0] = !WeaponSelection(CenterOfLeft(50), CHARACTER_PLAYER1, &gPlayerDatas[0], cmds[0], dones[0]);
				ShowSelection(
					CenterOfLeft(50),
					&gPlayerDatas[0],
					&gCampaign.Setting.characters.players[0]);
				ShowPlayerControls(CenterOfLeft(100), &gConfig.Input.PlayerKeys[0]);

				dones[1] = !WeaponSelection(CenterOfRight(50), CHARACTER_PLAYER2, &gPlayerDatas[1], cmds[1], dones[1]);
				ShowSelection(
					CenterOfRight(50),
					&gPlayerDatas[1],
					&gCampaign.Setting.characters.players[1]);
				ShowPlayerControls(CenterOfRight(100), &gConfig.Input.PlayerKeys[1]);
				break;
			default:
				assert(0 && "not implemented");
				break;
		}

		for (i = 0; i < MAX_PLAYERS; i++)
		{
			if (!dones[i])
			{
				isDone = 0;
			}
		}
		if (isDone)
		{
			break;
		}

		BlitFlip(graphics, &gConfig.Graphics);
		SDL_Delay(10);
	}

	return 1;
}
