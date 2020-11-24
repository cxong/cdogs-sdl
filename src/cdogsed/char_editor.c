/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.
	Copyright (c) 2017-2020 Cong Xu
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
#include "char_editor.h"

#include "editor_ui_common.h"
#include "nk_window.h"
#include <cdogs/draw/draw_actor.h>

#define ROW_HEIGHT 25
const float colRatios[] = {0.25f, 0.75f};
#define CHAR_STORE_BOTTOM 490
#define PAD 10.0f

typedef struct
{
	Character *Char;
	CampaignSetting *Setting;
	bool *FileChanged;
	char *CharacterClassNames;
	char *HairNames;
	char *GunNames;
	CArray texidsChars; // of GLuint[BODY_PART_COUNT]
	GLuint texidsPreview[BODY_PART_COUNT];
	CArray texIdsCharClasses; // of GLuint
	CArray texIdsHairs;		  // of GLuint
	CArray texIdsGuns;		  // of GLuint
	Animation anim;
	direction_e previewDir;
	Animation animSelection;
} EditorContext;

static const char *IndexCharacterClassName(const int i);
static const char *IndexHairName(const int i);
static int NumCharacterClasses(void);
static const char *IndexGunName(const int i);
static int NumGuns(void);
static int GunIndex(const WeaponClass *wc);
static void AddCharacterTextures(EditorContext *ec);
static bool Draw(SDL_Window *win, struct nk_context *ctx, void *data);
void CharEditor(
	GraphicsDevice *g, CampaignSetting *setting, EventHandlers *handlers,
	bool *fileChanged)
{
	NKWindowConfig cfg;
	memset(&cfg, 0, sizeof cfg);
	cfg.Title = "Character Editor";
	cfg.Size = svec2i(800, 800);
	cfg.MinSize = svec2i(800, 500);
	cfg.WindowFlags = SDL_WINDOW_RESIZABLE;
	color_t bg = {41, 26, 26, 255};
	cfg.BG = bg;
	cfg.Icon = g->icon;
	cfg.Handlers = handlers;
	cfg.Draw = Draw;

	NKWindowInit(&cfg);

	// Initialise editor context
	EditorContext ec;
	ec.Char = NULL;
	ec.Setting = setting;
	ec.FileChanged = fileChanged;
	ec.CharacterClassNames =
		GetClassNames(NumCharacterClasses(), IndexCharacterClassName);
	ec.HairNames =
		GetClassNames(gPicManager.hairstyleNames.size, IndexHairName);
	ec.GunNames = GetClassNames(NumGuns(), IndexGunName);

	CArrayInit(&ec.texidsChars, sizeof(GLuint) * BODY_PART_COUNT);
	for (int i = 0; i < (int)setting->characters.OtherChars.size; i++)
	{
		AddCharacterTextures(&ec);
	}
	glGenTextures(BODY_PART_COUNT, ec.texidsPreview);

	CharColors cc;
	cc.Skin = colorSkin;
	cc.Hair = colorRed;

	TexArrayInit(&ec.texIdsCharClasses, NumCharacterClasses());
	CA_FOREACH(const GLuint, texid, ec.texIdsCharClasses)
	const CharacterClass *c = IndexCharacterClass(_ca_index);
	LoadTexFromPic(*texid, GetHeadPic(c, DIRECTION_DOWN, false, &cc));
	CA_FOREACH_END()

	TexArrayInit(&ec.texIdsHairs, gPicManager.hairstyleNames.size);
	CA_FOREACH(const GLuint, texid, ec.texIdsHairs)
	const char *hair = IndexHairName(_ca_index);
	LoadTexFromPic(*texid, GetHairPic(hair, DIRECTION_DOWN, false, &cc));
	CA_FOREACH_END()

	TexArrayInit(&ec.texIdsGuns, NumGuns());
	CA_FOREACH(const GLuint, texid, ec.texIdsGuns)
	const WeaponClass *wc = IndexWeaponClassReal(_ca_index);
	LoadTexFromPic(*texid, wc->Icon);
	CA_FOREACH_END()

	ec.anim = AnimationGetActorAnimation(ACTORANIMATION_WALKING);
	ec.previewDir = DIRECTION_DOWN;
	ec.animSelection = AnimationGetActorAnimation(ACTORANIMATION_IDLE);

	cfg.DrawData = &ec;

	NKWindow(cfg);

	CFREE(ec.CharacterClassNames);
	CFREE(ec.HairNames);
	CFREE(ec.GunNames);
	glDeleteTextures(
		(GLsizei)(BODY_PART_COUNT * ec.texidsChars.size), ec.texidsChars.data);
	CArrayTerminate(&ec.texidsChars);
	glDeleteTextures(BODY_PART_COUNT, ec.texidsPreview);
	TexArrayTerminate(&ec.texIdsCharClasses);
	TexArrayTerminate(&ec.texIdsHairs);
	TexArrayTerminate(&ec.texIdsGuns);
}

static const char *IndexCharacterClassName(const int i)
{
	const CharacterClass *c = IndexCharacterClass(i);
	return c->Name;
}
static const char *IndexHairName(const int i)
{
	return *(char **)CArrayGet(&gPicManager.hairstyleNames, i);
}
static int NumCharacterClasses(void)
{
	return (
		int)(gCharacterClasses.Classes.size + gCharacterClasses.CustomClasses.size);
}
static const char *IndexGunName(const int i)
{
	const WeaponClass *wc = IndexWeaponClassReal(i);
	return wc ? wc->name : NULL;
}
static int NumGuns(void)
{
	int totalWeapons = 0;
	CA_FOREACH(const WeaponClass, wc, gWeaponClasses.Guns)
	if (wc->IsRealGun)
	{
		totalWeapons++;
	}
	CA_FOREACH_END()
	CA_FOREACH(const WeaponClass, wc, gWeaponClasses.CustomGuns)
	if (wc->IsRealGun)
	{
		totalWeapons++;
	}
	CA_FOREACH_END()
	return totalWeapons;
}
static int GunIndex(const WeaponClass *wc)
{
	int j = 0;
	CA_FOREACH(const WeaponClass, wc2, gWeaponClasses.Guns)
	if (!wc2->IsRealGun)
	{
		continue;
	}
	if (wc == wc2)
	{
		return j;
	}
	j++;
	CA_FOREACH_END()
	CA_FOREACH(const WeaponClass, wc2, gWeaponClasses.CustomGuns)
	if (!wc2->IsRealGun)
	{
		continue;
	}
	if (wc == wc2)
	{
		return j;
	}
	j++;
	CA_FOREACH_END()
	CASSERT(false, "cannot find gun");
	return -1;
}

static void AddCharacter(EditorContext *ec, const int cloneIdx);
static int MoveCharacter(
	EditorContext *ec, const int selectedIndex, const int d);
static void DeleteCharacter(EditorContext *ec, const int selectedIndex);
static int DrawClassSelection(
	struct nk_context *ctx, EditorContext *ec, const char *label,
	const GLuint *texids, const char *items, const int selected,
	const size_t len);
static int HairIndex(const char *hair);
static void DrawFlag(
	struct nk_context *ctx, EditorContext *ec, const char *label,
	const int flag, const char *tooltip);
static void DrawCharacter(
	struct nk_context *ctx, Character *c, GLuint *texids,
	const struct vec2i pos, const Animation *anim, const direction_e d);
static bool Draw(SDL_Window *win, struct nk_context *ctx, void *data)
{
	EditorContext *ec = data;
	// Stretch char store with window
	struct vec2i windowSize;
	SDL_GetWindowSize(win, &windowSize.x, &windowSize.y);
	const struct vec2i charStoreSize = svec2i(
		MAX(400, windowSize.x - 100),
		MAX(200, windowSize.y - CHAR_STORE_BOTTOM));
	if (nk_begin(
			ctx, "Character Store",
			nk_rect(
				PAD, PAD, (float)charStoreSize.x - PAD,
				(float)charStoreSize.y - PAD),
			NK_WINDOW_BORDER | NK_WINDOW_TITLE))
	{
		int selectedIndex = -1;
		CA_FOREACH(Character, c, ec->Setting->characters.OtherChars)
		if (ec->Char == c)
		{
			selectedIndex = _ca_index;
		}
		CA_FOREACH_END()

		// TODO: keep buttons from scrolling off
		nk_layout_row_dynamic(ctx, ROW_HEIGHT, 5);
		if (nk_button_label(ctx, "Add"))
		{
			AddCharacter(ec, -1);
			selectedIndex =
				MAX((int)ec->Setting->characters.OtherChars.size - 1, 0);
		}
		if (nk_button_label(ctx, "Move Up"))
		{
			selectedIndex = MoveCharacter(ec, selectedIndex, -1);
		}
		if (nk_button_label(ctx, "Move Down"))
		{
			selectedIndex = MoveCharacter(ec, selectedIndex, 1);
		}
		if (selectedIndex >= 0 && nk_button_label(ctx, "Duplicate"))
		{
			AddCharacter(ec, selectedIndex);
			selectedIndex =
				MAX((int)ec->Setting->characters.OtherChars.size - 1, 0);
		}
		if (selectedIndex >= 0 && nk_button_label(ctx, "Remove"))
		{
			DeleteCharacter(ec, selectedIndex);
			selectedIndex =
				MIN(selectedIndex,
					(int)ec->Setting->characters.OtherChars.size - 1);
		}
		if (selectedIndex >= 0)
		{
			ec->Char =
				CArrayGet(&ec->Setting->characters.OtherChars, selectedIndex);
		}
		else
		{
			ec->Char = NULL;
		}

		// Show existing characters
		nk_layout_row_dynamic(ctx, 32 * PIC_SCALE, (int)charStoreSize.x / 75);
		CA_FOREACH(Character, c, ec->Setting->characters.OtherChars)
		const int selected = ec->Char == c;
		// show both label and full character
		char label[256];
		sprintf(
			label, "%s%s", c->PlayerTemplateName != NULL ? "*" : "",
			c->Gun->name);
		if (nk_select_label(
				ctx, label, NK_TEXT_ALIGN_BOTTOM | NK_TEXT_ALIGN_CENTERED,
				selected))
		{
			ec->Char = c;
		}
		DrawCharacter(
			ctx, c, CArrayGet(&ec->texidsChars, _ca_index), svec2i(-34, 5),
			&ec->animSelection, DIRECTION_DOWN);
		CA_FOREACH_END()
		nk_end(ctx);
	}

	if (ec->Char != NULL)
	{
		const float previewWidth = 80;
		if (nk_begin(
				ctx, "Preview",
				nk_rect(
					charStoreSize.x + PAD, PAD, previewWidth,
					charStoreSize.y - PAD),
				NK_WINDOW_BORDER | NK_WINDOW_TITLE))
		{
			// Preview direction
			nk_layout_row_dynamic(ctx, ROW_HEIGHT, 2);
			if (nk_button_label(ctx, "<"))
			{
				ec->previewDir = (direction_e)CLAMP_OPPOSITE(
					(int)ec->previewDir + 1, 0, DIRECTION_UPLEFT);
			}
			if (nk_button_label(ctx, ">"))
			{
				ec->previewDir = (direction_e)CLAMP_OPPOSITE(
					(int)ec->previewDir - 1, 0, DIRECTION_UPLEFT);
			}
			// Preview
			nk_layout_row_dynamic(ctx, 32 * PIC_SCALE, 1);
			DrawCharacter(
				ctx, ec->Char, ec->texidsPreview, svec2i(0, 5), &ec->anim,
				ec->previewDir);
			// Animation
			nk_layout_row_dynamic(ctx, ROW_HEIGHT, 1);
			const int isWalking = ec->anim.Type == ACTORANIMATION_WALKING;
			if (nk_select_label(ctx, "Run", NK_TEXT_ALIGN_LEFT, isWalking) &&
				!isWalking)
			{
				ec->anim = AnimationGetActorAnimation(ACTORANIMATION_WALKING);
			}
			const int isIdle = ec->anim.Type == ACTORANIMATION_IDLE;
			if (nk_select_label(ctx, "Idle", NK_TEXT_ALIGN_LEFT, isIdle) &&
				!isIdle)
			{
				ec->anim = AnimationGetActorAnimation(ACTORANIMATION_IDLE);
			}
			nk_end(ctx);
		}

		if (nk_begin(
				ctx, "Appearance",
				nk_rect(
					PAD, (float)charStoreSize.y + PAD, 260,
					CHAR_STORE_BOTTOM - PAD * 2),
				NK_WINDOW_BORDER | NK_WINDOW_TITLE))
		{
			nk_layout_row(ctx, NK_DYNAMIC, ROW_HEIGHT, 2, colRatios);

			const int selectedClass = DrawClassSelection(
				ctx, ec, "Class:", ec->texIdsCharClasses.data,
				ec->CharacterClassNames,
				(int)CharacterClassIndex(ec->Char->Class),
				NumCharacterClasses());
			ec->Char->Class = IndexCharacterClass(selectedClass);

			nk_layout_row_dynamic(ctx, ROW_HEIGHT, 1);
			int hasHair = ec->Char->Hair != NULL;
			nk_checkbox_label(ctx, "Has Hair", &hasHair);

			if (hasHair)
			{
				nk_layout_row(ctx, NK_DYNAMIC, ROW_HEIGHT, 2, colRatios);
				const int currentHair = (int)HairIndex(ec->Char->Hair);
				int selectedHair = DrawClassSelection(
					ctx, ec, "Hair:", ec->texIdsHairs.data, ec->HairNames,
					currentHair, gPicManager.hairstyleNames.size);
				if (selectedHair == -1)
				{
					selectedHair = 0;
				}
				if (currentHair != selectedHair)
				{
					CFREE(ec->Char->Hair);
					CSTRDUP(ec->Char->Hair, IndexHairName(selectedHair));
				}
			}
			else
			{
				CFREE(ec->Char->Hair);
				ec->Char->Hair = NULL;
			}

			// Character colours
			nk_layout_row(ctx, NK_DYNAMIC, ROW_HEIGHT, 2, colRatios);
			if (ColorPicker(ctx, ROW_HEIGHT, "Skin:", &ec->Char->Colors.Skin))
			{
				*ec->FileChanged = true;
			}
			if (ColorPicker(ctx, ROW_HEIGHT, "Hair:", &ec->Char->Colors.Hair))
			{
				*ec->FileChanged = true;
			}
			if (ColorPicker(ctx, ROW_HEIGHT, "Arms:", &ec->Char->Colors.Arms))
			{
				*ec->FileChanged = true;
			}
			if (ColorPicker(ctx, ROW_HEIGHT, "Body:", &ec->Char->Colors.Body))
			{
				*ec->FileChanged = true;
			}
			if (ColorPicker(ctx, ROW_HEIGHT, "Legs:", &ec->Char->Colors.Legs))
			{
				*ec->FileChanged = true;
			}
			if (ColorPicker(ctx, ROW_HEIGHT, "Feet:", &ec->Char->Colors.Feet))
			{
				*ec->FileChanged = true;
			}
			nk_end(ctx);
		}

		if (nk_begin(
				ctx, "Attributes",
				nk_rect(280, (float)charStoreSize.y + PAD, 250, 250),
				NK_WINDOW_BORDER | NK_WINDOW_TITLE))
		{
			// Speed (256 = 100%)
			nk_layout_row_dynamic(ctx, ROW_HEIGHT, 1);
			int isPlayerTemplate = ec->Char->PlayerTemplateName != NULL;
			nk_checkbox_label(ctx, "Player Template", &isPlayerTemplate);
			if (isPlayerTemplate)
			{
				char buf[256];
				buf[0] = '\0';
				if (ec->Char->PlayerTemplateName != NULL)
				{
					strcpy(buf, ec->Char->PlayerTemplateName);
				}
				DrawTextbox(ctx, buf, 256, "Name", NK_EDIT_FIELD);
				if (ec->Char->PlayerTemplateName == NULL ||
					strcmp(buf, ec->Char->PlayerTemplateName) != 0)
				{
					*ec->FileChanged = true;
					CFREE(ec->Char->PlayerTemplateName);
					CSTRDUP(ec->Char->PlayerTemplateName, buf);
				}
			}
			else
			{
				CFREE(ec->Char->PlayerTemplateName);
				ec->Char->PlayerTemplateName = NULL;
			}
			nk_property_float(
				ctx, "Speed:", 0, &ec->Char->speed, 4, 0.05f, 0.01f);

			nk_layout_row(ctx, NK_DYNAMIC, ROW_HEIGHT, 2, colRatios);
			const int selectedGun = DrawClassSelection(
				ctx, ec, "Gun:", ec->texIdsGuns.data, ec->GunNames,
				GunIndex(ec->Char->Gun), NumGuns());
			ec->Char->Gun = IndexWeaponClassReal(selectedGun);

			nk_layout_row_dynamic(ctx, ROW_HEIGHT, 1);
			nk_property_int(
				ctx, "Max Health:", 10, &ec->Char->maxHealth, 1000, 10, 1);

			nk_layout_row_dynamic(ctx, ROW_HEIGHT, 2);
			DrawFlag(ctx, ec, "Asbestos", FLAGS_ASBESTOS, "Immune to fire");
			DrawFlag(ctx, ec, "Immunity", FLAGS_IMMUNITY, "Immune to poison");
			DrawFlag(ctx, ec, "See-through", FLAGS_SEETHROUGH, NULL);
			DrawFlag(ctx, ec, "Invulnerable", FLAGS_INVULNERABLE, NULL);
			DrawFlag(
				ctx, ec, "Penalty", FLAGS_PENALTY,
				"Large score penalty when shot");
			DrawFlag(
				ctx, ec, "Victim", FLAGS_VICTIM, "Takes damage from everyone");
			nk_end(ctx);
		}

		if (nk_begin(
				ctx, "AI",
				nk_rect(
					540, (float)charStoreSize.y + PAD, 250,
					CHAR_STORE_BOTTOM - PAD * 2),
				NK_WINDOW_BORDER | NK_WINDOW_TITLE))
		{
			nk_layout_row_dynamic(ctx, ROW_HEIGHT, 1);
			nk_property_int(
				ctx, "Move (%):", 0, &ec->Char->bot->probabilityToMove, 100, 5,
				1);
			nk_property_int(
				ctx, "Track (%):", 0, &ec->Char->bot->probabilityToTrack, 100,
				5, 1);
			nk_property_int(
				ctx, "Shoot (%):", 0, &ec->Char->bot->probabilityToShoot, 100,
				5, 1);
			nk_property_int(
				ctx, "Action delay:", 0, &ec->Char->bot->actionDelay, 50, 5,
				1);

			nk_layout_row_dynamic(ctx, ROW_HEIGHT, 2);
			DrawFlag(
				ctx, ec, "Runs away", FLAGS_RUNS_AWAY,
				"Runs away from player");
			DrawFlag(
				ctx, ec, "Sneaky", FLAGS_SNEAKY,
				"Shoots back when player shoots");
			DrawFlag(
				ctx, ec, "Good guy", FLAGS_GOOD_GUY, "Same team as players");
			DrawFlag(
				ctx, ec, "Sleeping", FLAGS_SLEEPING,
				"Doesn't move unless seen");
			DrawFlag(
				ctx, ec, "Prisoner", FLAGS_PRISONER,
				"Doesn't move until touched");
			DrawFlag(ctx, ec, "Follower", FLAGS_FOLLOWER, "Follows players");
			DrawFlag(
				ctx, ec, "Awake", FLAGS_AWAKEALWAYS,
				"Don't go to sleep after players leave");
			nk_end(ctx);
		}
	}

	AnimationUpdate(&ec->anim, 1);
	AnimationUpdate(&ec->animSelection, 1);

	return true;
}

static void AddCharacter(EditorContext *ec, const int cloneIdx)
{
	ec->Char = CharacterStoreAddOther(&ec->Setting->characters);
	if (cloneIdx >= 0)
	{
		const Character *clone =
			CArrayGet(&ec->Setting->characters.OtherChars, cloneIdx);
		CFREE(ec->Char->bot);
		memcpy(ec->Char, clone, sizeof *ec->Char);
		if (clone->PlayerTemplateName)
		{
			CSTRDUP(ec->Char->PlayerTemplateName, clone->PlayerTemplateName);
		}
		CSTRDUP(ec->Char->Hair, clone->Hair);
		CMALLOC(ec->Char->bot, sizeof *ec->Char->bot);
		memcpy(ec->Char->bot, clone->bot, sizeof *ec->Char->bot);
	}
	else
	{
		// set up character template
		ec->Char->Class = StrCharacterClass("Ogre");
		ec->Char->Colors.Skin = colorGreen;
		const color_t darkGray = {64, 64, 64, 255};
		ec->Char->Colors.Arms = darkGray;
		ec->Char->Colors.Body = darkGray;
		ec->Char->Colors.Legs = darkGray;
		ec->Char->Colors.Hair = colorBlack;
		ec->Char->Colors.Feet = darkGray;
		ec->Char->speed = 1;
		ec->Char->Gun = StrWeaponClass("Machine gun");
		ec->Char->maxHealth = 40;
		ec->Char->flags = FLAGS_IMMUNITY;
		ec->Char->bot->probabilityToMove = 50;
		ec->Char->bot->probabilityToTrack = 25;
		ec->Char->bot->probabilityToShoot = 2;
		ec->Char->bot->actionDelay = 15;
	}

	AddCharacterTextures(ec);

	*ec->FileChanged = true;
}

static void AddCharacterTextures(EditorContext *ec)
{
	GLuint texids[BODY_PART_COUNT];
	glGenTextures(BODY_PART_COUNT, texids);
	CArrayPushBack(&ec->texidsChars, &texids);
}

static int MoveCharacter(
	EditorContext *ec, const int selectedIndex, const int d)
{
	const int moveIndex = selectedIndex + d;
	CArray *chars = &ec->Setting->characters.OtherChars;
	if (moveIndex < 0 || moveIndex >= (int)chars->size)
	{
		return selectedIndex;
	}

	Character tmp;
	Character *selected = CArrayGet(chars, selectedIndex);
	Character *move = CArrayGet(chars, moveIndex);
	memcpy(&tmp, selected, chars->elemSize);
	memcpy(selected, move, chars->elemSize);
	memcpy(move, &tmp, chars->elemSize);
	return moveIndex;
}

static void DeleteCharacter(EditorContext *ec, const int selectedIndex)
{
	CharacterStoreDeleteOther(&ec->Setting->characters, selectedIndex);
	const int indexClamped =
		MIN(selectedIndex, (int)ec->Setting->characters.OtherChars.size - 1);
	if (indexClamped >= 0)
	{
		ec->Char =
			CArrayGet(&ec->Setting->characters.OtherChars, indexClamped);
	}
	else
	{
		ec->Char = NULL;
	}

	// Delete character textures
	GLuint *texids = CArrayGet(&ec->texidsChars, selectedIndex);
	glDeleteTextures(BODY_PART_COUNT, texids);
	CArrayDelete(&ec->texidsChars, selectedIndex);

	*ec->FileChanged = true;
}

static int DrawClassSelection(
	struct nk_context *ctx, EditorContext *ec, const char *label,
	const GLuint *texids, const char *items, const int selected,
	const size_t len)
{
	nk_label(ctx, label, NK_TEXT_LEFT);
	const int selectedNew = nk_combo_separator_image(
		ctx, texids, items, '\0', selected, (int)len, ROW_HEIGHT,
		nk_vec2(nk_widget_width(ctx), 8 * ROW_HEIGHT));
	if (selectedNew != selected)
	{
		*ec->FileChanged = true;
	}
	return selectedNew;
}

static int HairIndex(const char *hair)
{
	if (hair == NULL)
	{
		return -1;
	}
	CA_FOREACH(const char *, hairstyleName, gPicManager.hairstyleNames)
	if (strcmp(*hairstyleName, hair) == 0)
	{
		return _ca_index;
	}
	CA_FOREACH_END()
	return -1;
}

static void DrawFlag(
	struct nk_context *ctx, EditorContext *ec, const char *label,
	const int flag, const char *tooltip)
{
	struct nk_rect bounds = nk_widget_bounds(ctx);
	nk_checkbox_flags_label(ctx, label, &ec->Char->flags, flag);
	if (tooltip && nk_input_is_mouse_hovering_rect(&ctx->input, bounds))
	{
		nk_tooltip(ctx, tooltip);
	}
}

static void DrawCharacter(
	struct nk_context *ctx, Character *c, GLuint *texids,
	const struct vec2i pos, const Animation *anim, const direction_e d)
{
	const int frame = AnimationGetFrame(anim);
	ActorPics pics = GetCharacterPics(
		c, d, d, anim->Type, frame, c->Gun->Sprites, GUNSTATE_READY, false,
		colorTransparent, NULL, NULL, 0);
	for (int i = 0; i < BODY_PART_COUNT; i++)
	{
		const Pic *pic = pics.OrderedPics[i];
		if (pic == NULL)
		{
			continue;
		}
		const struct vec2i drawPos = svec2i_add(
			svec2i_add(pos, pics.OrderedOffsets[i]), svec2i(16, 16));
		DrawPic(ctx, pic, texids[i], drawPos, PIC_SCALE);
	}
}
