/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (c) 2017 Cong Xu
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

#include <SDL_opengl.h>
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_BUTTON_TRIGGER_ON_RELEASE
#define NK_IMPLEMENTATION
#define NK_SDL_GL2_IMPLEMENTATION
#ifdef _MSC_VER
// Guard against compile time constant in nk_memset
#pragma warning(push)
#pragma warning(disable: 4127)
#endif
#include <nuklear/nuklear.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#include <nuklear/nuklear_sdl_gl2.h>
#include <cdogs/actors.h>
#include <cdogs/draw/draw_actor.h>

#define MAX_VERTEX_MEMORY 512 * 1024
#define MAX_ELEMENT_MEMORY 128 * 1024

#define ROW_HEIGHT 25
const float colRatios[] = { 0.25f, 0.75f };
#define PIC_SCALE 2

typedef struct
{
	struct nk_context *ctx;
	Vec2i WindowSize;
	Character *Char;
	CampaignSetting *Setting;
	EventHandlers *Handlers;
	bool *FileChanged;
	char *CharacterClassNames;
	char *GunNames;
	CArray texidsChars;	// of GLuint[BODY_PART_COUNT]
	GLuint texidsPreview[BODY_PART_COUNT];
	CArray texIdsCharClasses;	// of GLuint
	CArray texIdsGuns;	// of GLuint
	Animation anim;
	direction_e previewDir;
	Animation animSelection;
} EditorContext;

const float bg[4] = { 0.16f, 0.1f, 0.1f, 1.f };

// Util functions
static void LoadTexFromPic(const GLuint texid, const Pic *pic);
static void LoadMultiChannelTexFromPic(
	const GLuint texid, const Pic *pic, const CharColors *colors);
static void BeforeDrawTex(const GLuint texid);


static char *GetClassNames(const int len, const char *(*indexNameFunc)(int));
static const char *IndexCharacterClassName(const int i);
static int NumCharacterClasses(void);
static const char *IndexGunName(const int i);
static int NumGuns(void);
static int GunIndex(const GunDescription *g);
static void AddCharacterTextures(EditorContext *ec);
static bool HandleEvents(EditorContext *ec);
static void Draw(SDL_Window *win, EditorContext *ec);
void CharEditor(
	GraphicsDevice *g, CampaignSetting *setting, EventHandlers *handlers,
	bool *fileChanged)
{
	SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "1");
	SDL_Init(SDL_INIT_VIDEO);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

	SDL_Window *win = SDL_CreateWindow("Character Editor",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		800, 600,
		SDL_WINDOW_OPENGL|SDL_WINDOW_SHOWN|SDL_WINDOW_RESIZABLE);
	SDL_SetWindowMinimumSize(win, 800, 500);
	SDL_SetWindowIcon(win, g->icon);

	SDL_GLContext glContext = SDL_GL_CreateContext(win);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Initialise editor context
	EditorContext ec;
	ec.ctx = nk_sdl_init(win);
	ec.ctx->style.checkbox.hover.data.color = nk_rgb(96, 96, 96);
	ec.ctx->style.checkbox.normal.data.color = nk_rgb(64, 64, 64);
	ec.ctx->style.checkbox.cursor_hover.data.color = nk_rgb(255, 255, 255);
	ec.ctx->style.checkbox.cursor_normal.data.color = nk_rgb(200, 200, 200);
	ec.Char = NULL;
	ec.Setting = setting;
	ec.Handlers = handlers;
	ec.FileChanged = fileChanged;
	ec.CharacterClassNames = GetClassNames(
		NumCharacterClasses(), IndexCharacterClassName);
	ec.GunNames = GetClassNames(NumGuns(), IndexGunName);

	CArrayInit(&ec.texidsChars, sizeof(GLuint) * BODY_PART_COUNT);
	for (int i = 0; i < (int)setting->characters.OtherChars.size; i++)
	{
		AddCharacterTextures(&ec);
	}
	glGenTextures(BODY_PART_COUNT, ec.texidsPreview);
	CArrayInit(&ec.texIdsCharClasses, sizeof(GLuint));
	CArrayResize(&ec.texIdsCharClasses, NumCharacterClasses(), NULL);
	glGenTextures(NumCharacterClasses(), (GLuint *)ec.texIdsCharClasses.data);
	CharColors cc;
	cc.Skin = colorSkin;
	cc.Hair = colorRed;
	for (int i = 0; i < NumCharacterClasses(); i++)
	{
		const GLuint *texid = CArrayGet(&ec.texIdsCharClasses, i);
		const CharacterClass *c = IndexCharacterClass(i);
		LoadMultiChannelTexFromPic(
			*texid, GetHeadPic(c, DIRECTION_DOWN, GUNSTATE_READY), &cc);
	}
	CArrayInit(&ec.texIdsGuns, sizeof(GLuint));
	CArrayResize(&ec.texIdsGuns, NumGuns(), NULL);
	glGenTextures(NumGuns(), (GLuint *)ec.texIdsGuns.data);
	for (int i = 0; i < NumGuns(); i++)
	{
		const GLuint *texid = CArrayGet(&ec.texIdsGuns, i);
		const GunDescription *gd = IndexGunDescriptionReal(i);
		LoadTexFromPic(*texid, gd->Icon);
	}

	ec.anim = AnimationGetActorAnimation(ACTORANIMATION_WALKING);
	ec.previewDir = DIRECTION_DOWN;
	ec.animSelection = AnimationGetActorAnimation(ACTORANIMATION_IDLE);

	// Initialise fonts
	struct nk_font_atlas *atlas;
	nk_sdl_font_stash_begin(&atlas);
	nk_sdl_font_stash_end();

	Uint32 ticksNow = SDL_GetTicks();
	Uint32 ticksElapsed = 0;
	for (;;)
	{
		Uint32 ticksThen = ticksNow;
		ticksNow = SDL_GetTicks();
		ticksElapsed += ticksNow - ticksThen;
		if (ticksElapsed < 1000 / FPS_FRAMELIMIT)
		{
			SDL_Delay(1);
			continue;
		}

		// Note: drawing contains input processing too
		nk_input_begin(ec.ctx);
		if (!HandleEvents(&ec))
		{
			goto bail;
		}
		Draw(win, &ec);
		nk_input_end(ec.ctx);

		ticksElapsed = 0;
	}

bail:
	nk_sdl_shutdown();
	CFREE(ec.CharacterClassNames);
	CFREE(ec.GunNames);
	glDeleteTextures(
		BODY_PART_COUNT * ec.texidsChars.size, ec.texidsChars.data);
	CArrayTerminate(&ec.texidsChars);
	glDeleteTextures(BODY_PART_COUNT, ec.texidsPreview);
	glDeleteTextures(
		ec.texIdsCharClasses.size, (const GLuint *)ec.texIdsCharClasses.data);
	CArrayTerminate(&ec.texIdsCharClasses);
	glDeleteTextures(ec.texIdsGuns.size, (const GLuint *)ec.texIdsGuns.data);
	CArrayTerminate(&ec.texIdsGuns);
	SDL_GL_DeleteContext(glContext);
	SDL_DestroyWindow(win);
}

static char *GetClassNames(const int len, const char *(*indexNameFunc)(int))
{
	int classLen = 0;
	for (int i = 0; i < (int)len; i++)
	{
		const char *name = indexNameFunc(i);
		classLen += strlen(name) + 1;
	}
	char *names;
	CMALLOC(names, classLen);
	char *cp = names;
	for (int i = 0; i < (int)len; i++)
	{
		const char *name = indexNameFunc(i);
		strcpy(cp, name);
		cp += strlen(name) + 1;
	}
	return names;
}

static const char *IndexCharacterClassName(const int i)
{
	const CharacterClass *c = IndexCharacterClass(i);
	return c->Name;
}
static int NumCharacterClasses(void)
{
	return
		gCharacterClasses.Classes.size + gCharacterClasses.CustomClasses.size;
}
static const char *IndexGunName(const int i)
{
	const GunDescription *g = IndexGunDescriptionReal(i);
	return g ? g->name : NULL;
}
static int NumGuns(void)
{
	int totalWeapons = 0;
	CA_FOREACH(const GunDescription, g, gGunDescriptions.Guns)
		if (g->IsRealGun)
		{
			totalWeapons++;
		}
	CA_FOREACH_END()
	CA_FOREACH(const GunDescription, g, gGunDescriptions.CustomGuns)
		if (g->IsRealGun)
		{
			totalWeapons++;
		}
	CA_FOREACH_END()
	return totalWeapons;
}
static int GunIndex(const GunDescription *g)
{
	int j = 0;
	CA_FOREACH(const GunDescription, gg, gGunDescriptions.Guns)
		if (!gg->IsRealGun)
		{
			continue;
		}
		if (g == gg)
		{
			return j;
		}
		j++;
	CA_FOREACH_END()
	CA_FOREACH(const GunDescription, gg, gGunDescriptions.CustomGuns)
		if (!g->IsRealGun)
		{
			continue;
		}
		if (g == gg)
		{
			return j;
		}
		j++;
	CA_FOREACH_END()
	CASSERT(false, "cannot find gun");
	return -1;
}

static bool HandleEvents(EditorContext *ec)
{
	SDL_Event e;
	bool run = true;
	while (SDL_PollEvent(&e))
	{
		switch (e.type)
		{
			case SDL_KEYDOWN:
				if (e.key.repeat)
				{
					break;
				}
				KeyOnKeyDown(&ec->Handlers->keyboard, e.key.keysym);
				break;
			case SDL_KEYUP:
				KeyOnKeyUp(&ec->Handlers->keyboard, e.key.keysym);
				break;
			case SDL_QUIT:
				run = false;
				break;
			case SDL_WINDOWEVENT:
				switch (e.window.event)
				{
					case SDL_WINDOWEVENT_CLOSE:
						run = false;
						break;
					default:
						break;
				}
				break;
			default:
				break;
		}
		nk_sdl_handle_event(&e);
	}

	EventPoll(ec->Handlers, 1);
	const SDL_Scancode sc = KeyGetPressed(&gEventHandlers.keyboard);
	if (sc == SDL_SCANCODE_ESCAPE)
	{
		run = false;
	}

	return run;
}

static void AddCharacter(EditorContext *ec, const int cloneIdx);
static int MoveCharacter(
	EditorContext *ec, const int selectedIndex, const int d);
static void DeleteCharacter(EditorContext *ec, const int selectedIndex);
static int DrawClassSelection(
	EditorContext *ec, const char *label, const GLuint *texids,
	const char *items, const int selected, const size_t len);
static void DrawCharColor(EditorContext *ec, const char *label, color_t *c);
static void DrawFlag(
	EditorContext *ec, const char *label, const int flag, const char *tooltip);
static void DrawCharacter(
	EditorContext *ec, Character *c, GLuint *texids, const Vec2i pos,
	const Animation *anim, const direction_e d);
static void Draw(SDL_Window *win, EditorContext *ec)
{
	// Stretch char store with window
	SDL_GetWindowSize(win, &ec->WindowSize.x, &ec->WindowSize.y);
	const Vec2i charStoreSize = Vec2iNew(
		MAX(400, ec->WindowSize.x - 100),
		MAX(200, ec->WindowSize.y - 300));
	const float pad = 10;
	if (nk_begin(ec->ctx, "Character Store",
		nk_rect(pad, pad, (float)charStoreSize.x - pad, (float)charStoreSize.y - pad),
		NK_WINDOW_BORDER|NK_WINDOW_TITLE))
	{
		int selectedIndex = -1;
		CA_FOREACH(Character, c, ec->Setting->characters.OtherChars)
			if (ec->Char == c)
			{
				selectedIndex = _ca_index;
			}
		CA_FOREACH_END()

		// TODO: keep buttons from scrolling off
		nk_layout_row_dynamic(ec->ctx, ROW_HEIGHT, 5);
		if (nk_button_label(ec->ctx, "Add"))
		{
			AddCharacter(ec, -1);
			selectedIndex = MAX(
				(int)ec->Setting->characters.OtherChars.size - 1, 0);
		}
		if (nk_button_label(ec->ctx, "Move Up"))
		{
			selectedIndex = MoveCharacter(ec, selectedIndex, -1);
		}
		if (nk_button_label(ec->ctx, "Move Down"))
		{
			selectedIndex = MoveCharacter(ec, selectedIndex, 1);
		}
		if (selectedIndex >= 0 && nk_button_label(ec->ctx, "Duplicate"))
		{
			AddCharacter(ec, selectedIndex);
			selectedIndex = MAX(
				(int)ec->Setting->characters.OtherChars.size - 1, 0);
		}
		if (selectedIndex >= 0 && nk_button_label(ec->ctx, "Remove"))
		{
			DeleteCharacter(ec, selectedIndex);
			selectedIndex = MIN(
				selectedIndex,
				(int)ec->Setting->characters.OtherChars.size - 1);
		}
		if (selectedIndex >= 0)
		{
			ec->Char = CArrayGet(
				&ec->Setting->characters.OtherChars, selectedIndex);
		}
		else
		{
			ec->Char = NULL;
		}

		// Show existing characters
		nk_layout_row_dynamic(
			ec->ctx, 32 * PIC_SCALE, (int)charStoreSize.x / 75);
		CA_FOREACH(Character, c, ec->Setting->characters.OtherChars)
			const int selected = ec->Char == c;
			// show both label and full character
			if (nk_select_label(ec->ctx, c->Gun->name,
				NK_TEXT_ALIGN_BOTTOM|NK_TEXT_ALIGN_CENTERED, selected))
			{
				ec->Char = c;
				selectedIndex = _ca_index;
			}
			DrawCharacter(
				ec, c, CArrayGet(&ec->texidsChars, _ca_index),
				Vec2iNew(-34, 5), &ec->animSelection, DIRECTION_DOWN);
		CA_FOREACH_END()
	}
	nk_end(ec->ctx);

	if (ec->Char != NULL)
	{
		const float previewWidth = 80;
		if (nk_begin(ec->ctx, "Preview",
			nk_rect(charStoreSize.x + pad, pad,
				previewWidth, charStoreSize.y - pad),
			NK_WINDOW_BORDER|NK_WINDOW_TITLE))
		{
			// Preview direction
			nk_layout_row_dynamic(ec->ctx, ROW_HEIGHT, 2);
			if (nk_button_label(ec->ctx, "<"))
			{
				ec->previewDir = (direction_e)CLAMP_OPPOSITE(
					(int)ec->previewDir + 1, 0, DIRECTION_UPLEFT);
			}
			if (nk_button_label(ec->ctx, ">"))
			{
				ec->previewDir = (direction_e)CLAMP_OPPOSITE(
					(int)ec->previewDir - 1, 0, DIRECTION_UPLEFT);
			}
			// Preview
			nk_layout_row_dynamic(ec->ctx, 32 * PIC_SCALE, 1);
			DrawCharacter(
				ec, ec->Char, ec->texidsPreview, Vec2iNew(0, 5), &ec->anim,
				ec->previewDir);
			// Animation
			nk_layout_row_dynamic(ec->ctx, ROW_HEIGHT, 1);
			const int isWalking = ec->anim.Type == ACTORANIMATION_WALKING;
			if (nk_select_label(
				ec->ctx, "Run", NK_TEXT_ALIGN_LEFT, isWalking) &&
				!isWalking)
			{
				ec->anim = AnimationGetActorAnimation(ACTORANIMATION_WALKING);
			}
			const int isIdle = ec->anim.Type == ACTORANIMATION_IDLE;
			if (nk_select_label(ec->ctx, "Idle", NK_TEXT_ALIGN_LEFT, isIdle) &&
				!isIdle)
			{
				ec->anim = AnimationGetActorAnimation(ACTORANIMATION_IDLE);
			}
		}
		nk_end(ec->ctx);

		if (nk_begin(ec->ctx, "Appearance",
			nk_rect(pad, (float)charStoreSize.y + pad, 260, 225),
			NK_WINDOW_BORDER|NK_WINDOW_TITLE))
		{
			nk_layout_row(ec->ctx, NK_DYNAMIC, ROW_HEIGHT, 2, colRatios);
			const int selectedClass = DrawClassSelection(
				ec, "Class:", ec->texIdsCharClasses.data,
				ec->CharacterClassNames,
				CharacterClassIndex(ec->Char->Class), NumCharacterClasses());
			ec->Char->Class = IndexCharacterClass(selectedClass);

			// Character colours
			nk_layout_row(ec->ctx, NK_DYNAMIC, ROW_HEIGHT, 2, colRatios);
			DrawCharColor(ec, "Skin:", &ec->Char->Colors.Skin);
			DrawCharColor(ec, "Hair:", &ec->Char->Colors.Hair);
			DrawCharColor(ec, "Arms:", &ec->Char->Colors.Arms);
			DrawCharColor(ec, "Body:", &ec->Char->Colors.Body);
			DrawCharColor(ec, "Legs:", &ec->Char->Colors.Legs);
		}
		nk_end(ec->ctx);

		if (nk_begin(ec->ctx, "Attributes",
			nk_rect(280, (float)charStoreSize.y + pad, 250, 225),
			NK_WINDOW_BORDER|NK_WINDOW_TITLE))
		{
			// Speed (256 = 100%)
			nk_layout_row_dynamic(ec->ctx, ROW_HEIGHT, 1);
			nk_property_int(
				ec->ctx, "Speed:", 0, &ec->Char->speed, 1024, 16, 1);

			nk_layout_row(ec->ctx, NK_DYNAMIC, ROW_HEIGHT, 2, colRatios);
			const int selectedGun = DrawClassSelection(
				ec, "Gun:", ec->texIdsGuns.data, ec->GunNames,
				GunIndex(ec->Char->Gun), NumGuns());
			ec->Char->Gun = IndexGunDescriptionReal(selectedGun);

			nk_layout_row_dynamic(ec->ctx, ROW_HEIGHT, 1);
			nk_property_int(
				ec->ctx, "Max Health:", 10, &ec->Char->maxHealth, 1000, 10, 1);

			nk_layout_row_dynamic(ec->ctx, ROW_HEIGHT, 2);
			DrawFlag(ec, "Asbestos", FLAGS_ASBESTOS, "Immune to fire");
			DrawFlag(ec, "Immunity", FLAGS_IMMUNITY, "Immune to poison");
			DrawFlag(ec, "See-through", FLAGS_SEETHROUGH, NULL);
			DrawFlag(ec, "Invulnerable", FLAGS_INVULNERABLE, NULL);
			DrawFlag(
				ec, "Penalty", FLAGS_PENALTY, "Large score penalty when shot");
			DrawFlag(ec, "Victim", FLAGS_VICTIM, "Takes damage from everyone");
		}
		nk_end(ec->ctx);

		if (nk_begin(ec->ctx, "AI",
			nk_rect(540, (float)charStoreSize.y + pad, 250, 280),
			NK_WINDOW_BORDER|NK_WINDOW_TITLE))
		{
			nk_layout_row_dynamic(ec->ctx, ROW_HEIGHT, 1);
			nk_property_int(
				ec->ctx, "Move (%):", 0, &ec->Char->bot->probabilityToMove,
				100, 5, 1);
			nk_property_int(
				ec->ctx, "Track (%):", 0, &ec->Char->bot->probabilityToTrack,
				100, 5, 1);
			nk_property_int(
				ec->ctx, "Shoot (%):", 0, &ec->Char->bot->probabilityToShoot,
				100, 5, 1);
			nk_property_int(
				ec->ctx, "Action delay:", 0, &ec->Char->bot->actionDelay,
				50, 5, 1);

			nk_layout_row_dynamic(ec->ctx, ROW_HEIGHT, 2);
			DrawFlag(ec, "Runs away", FLAGS_RUNS_AWAY, "Runs away from player");
			DrawFlag(
				ec, "Sneaky", FLAGS_SNEAKY, "Shoots back when player shoots");
			DrawFlag(ec, "Good guy", FLAGS_GOOD_GUY, "Same team as players");
			DrawFlag(
				ec, "Sleeping", FLAGS_SLEEPING, "Doesn't move unless seen");
			DrawFlag(
				ec, "Prisoner", FLAGS_PRISONER, "Doesn't move until touched");
			DrawFlag(ec, "Follower", FLAGS_FOLLOWER, "Follows players");
			DrawFlag(
				ec, "Awake", FLAGS_AWAKEALWAYS,
				"Don't go to sleep after players leave");
		}
		nk_end(ec->ctx);
	}

	AnimationUpdate(&ec->anim, 1);
	AnimationUpdate(&ec->animSelection, 1);

	int winWidth, winHeight;
	SDL_GetWindowSize(win, &winWidth, &winHeight);
	glViewport(0, 0, winWidth, winHeight);
	glClear(GL_COLOR_BUFFER_BIT);
	glClearColor(bg[0], bg[1], bg[2], bg[3]);

	nk_sdl_render(NK_ANTI_ALIASING_ON);
	SDL_GL_SwapWindow(win);
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
		ec->Char->speed = 256;
		ec->Char->Gun = StrGunDescription("Machine gun");
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
	const int indexClamped = MIN(
		selectedIndex,
		(int)ec->Setting->characters.OtherChars.size - 1);
	if (indexClamped >= 0)
	{
		ec->Char = CArrayGet(
			&ec->Setting->characters.OtherChars, indexClamped);
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

static int nk_combo_separator_image(struct nk_context *ctx,
	const GLuint *img_ids, const char *items_separated_by_separator,
    int separator, int selected, int count, int item_height,
	struct nk_vec2 size);
static int DrawClassSelection(
	EditorContext *ec, const char *label, const GLuint *texids,
	const char *items, const int selected, const size_t len)
{
	nk_label(ec->ctx, label, NK_TEXT_LEFT);
	const int selectedNew = nk_combo_separator_image(
		ec->ctx, texids, items, '\0', selected, len,
		ROW_HEIGHT, nk_vec2(nk_widget_width(ec->ctx), 10 * ROW_HEIGHT));
	if (selectedNew != selected)
	{
		*ec->FileChanged = true;
	}
	return selectedNew;
}
static int nk_combo_separator_image(struct nk_context *ctx,
	const GLuint *img_ids, const char *items_separated_by_separator,
    int separator, int selected, int count, int item_height,
	struct nk_vec2 size)
{
    int i;
    int max_height;
    struct nk_vec2 item_spacing;
    struct nk_vec2 window_padding;
    const char *current_item;
    const char *iter;
    int length = 0;

    NK_ASSERT(ctx);
    NK_ASSERT(items_separated_by_separator);
    if (!ctx || !items_separated_by_separator)
        return selected;

    /* calculate popup window */
    item_spacing = ctx->style.window.spacing;
    window_padding = nk_panel_get_padding(&ctx->style, ctx->current->layout->type);
    max_height = count * item_height + count * (int)item_spacing.y;
    max_height += (int)item_spacing.y * 2 + (int)window_padding.y * 2;
    size.y = NK_MIN(size.y, (float)max_height);

    /* find selected item */
    current_item = items_separated_by_separator;
    for (i = 0; i < count; ++i) {
        iter = current_item;
        while (*iter && *iter != separator) iter++;
        length = (int)(iter - current_item);
        if (i == selected) break;
        current_item = iter + 1;
    }

	// Get widget bounds for drawing currently selected item image later
	struct nk_rect bounds;
	nk_layout_widget_space(&bounds, ctx, ctx->current, nk_false);
	bounds.x += size.x - 56;
	bounds.y += 2;
	bounds.w = (float)12 * PIC_SCALE;
	bounds.h = (float)12 * PIC_SCALE;

    if (nk_combo_begin_text(ctx, current_item, length, size)) {
        current_item = items_separated_by_separator;
        nk_layout_row_dynamic(ctx, (float)item_height, 1);
        for (i = 0; i < count; ++i) {
			const struct nk_image img = nk_image_id(img_ids[i]);
			// TODO: image size
			BeforeDrawTex(img_ids[i]);
            iter = current_item;
            while (*iter && *iter != separator) iter++;
            length = (int)(iter - current_item);
            if (nk_contextual_item_image_text(ctx, img, current_item, length, NK_TEXT_LEFT))
                selected = i;
            current_item = current_item + length + 1;
        }
        nk_combo_end(ctx);
    }

	// Also draw currently selected image
	const struct nk_image comboImg = nk_image_id(img_ids[selected]);
	BeforeDrawTex(img_ids[selected]);
	nk_draw_image(&ctx->current->buffer, bounds, &comboImg, nk_white);

    return selected;
}

static void DrawCharColor(EditorContext *ec, const char *label, color_t *c)
{
	nk_label(ec->ctx, label, NK_TEXT_LEFT);
	struct nk_color color = { c->r, c->g, c->b, 255 };
	const struct nk_color colorOriginal = color;
	if (nk_combo_begin_color(
		ec->ctx, color, nk_vec2(nk_widget_width(ec->ctx), 400)))
	{
		nk_layout_row_dynamic(ec->ctx, 110, 1);
		color = nk_color_picker(ec->ctx, color, NK_RGB);
		nk_layout_row_dynamic(ec->ctx, ROW_HEIGHT, 1);
		color.r = (nk_byte)nk_propertyi(ec->ctx, "#R:", 0, color.r, 255, 1, 1);
		color.g = (nk_byte)nk_propertyi(ec->ctx, "#G:", 0, color.g, 255, 1, 1);
		color.b = (nk_byte)nk_propertyi(ec->ctx, "#B:", 0, color.b, 255, 1, 1);
		nk_combo_end(ec->ctx);
		c->r = color.r;
		c->g = color.g;
		c->b = color.b;
		if (memcmp(&color, &colorOriginal, sizeof color))
		{
			*ec->FileChanged = true;
		}
	}
}

static void DrawFlag(
	EditorContext *ec, const char *label, const int flag, const char *tooltip)
{
	struct nk_rect bounds = nk_widget_bounds(ec->ctx);
	nk_checkbox_flags_label(ec->ctx, label, &ec->Char->flags, flag);
	if (tooltip && nk_input_is_mouse_hovering_rect(&ec->ctx->input, bounds))
	{
		nk_tooltip(ec->ctx, tooltip);
	}
}


static void LoadTexFromPic(const GLuint texid, const Pic *pic)
{
	glBindTexture(GL_TEXTURE_2D, texid);
	glTexImage2D(
		GL_TEXTURE_2D, 0, GL_RGBA, pic->size.x, pic->size.y, 0, GL_BGRA,
		GL_UNSIGNED_BYTE, pic->Data);
}
static void LoadMultiChannelTexFromPic(
	const GLuint texid, const Pic *pic, const CharColors *colors)
{
	glBindTexture(GL_TEXTURE_2D, texid);
	Uint32 *data;
	CMALLOC(data, pic->size.x * pic->size.y * sizeof *data);
	for (int i = 0; i < pic->size.x * pic->size.y; i++)
	{
		const Uint32 pixel = pic->Data[i];
		const color_t color = PIXEL2COLOR(pixel);
		if (pixel == 0)
		{
			data[i] = 0;
		}
		else
		{
			data[i] = PixelMult(
				pixel, COLOR2PIXEL(CharColorsGetChannelMask(colors, color.a)));
		}
	}
	glTexImage2D(
		GL_TEXTURE_2D, 0, GL_RGBA, pic->size.x, pic->size.y, 0, GL_BGRA,
		GL_UNSIGNED_BYTE, data);
	CFREE(data);
}

static void BeforeDrawTex(const GLuint texid)
{
	glBindTexture(GL_TEXTURE_2D, texid);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

static void DrawCharacter(
	EditorContext *ec, Character *c, GLuint *texids, const Vec2i pos,
	const Animation *anim, const direction_e d)
{
	const int frame = AnimationGetFrame(anim);
	ActorPics pics = GetCharacterPics(
		c, d, anim->Type, frame,
		c->Gun->Pic, GUNSTATE_READY, false, NULL, NULL, 0);
	for (int i = 0; i < BODY_PART_COUNT; i++)
	{
		const Pic *pic = pics.OrderedPics[i];
		if (pic == NULL)
		{
			continue;
		}
		const Vec2i drawPos = Vec2iAdd(pos, pics.OrderedOffsets[i]);
		LoadMultiChannelTexFromPic(texids[i], pic, pics.Colors);
		struct nk_image tex = nk_image_id((int)texids[i]);
		glTexParameteri(
			GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(
			GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		struct nk_rect bounds;
		nk_layout_widget_space(
			&bounds, ec->ctx, ec->ctx->current, nk_true);
		bounds.x += drawPos.x * PIC_SCALE + 32;
		bounds.y += drawPos.y * PIC_SCALE + 32;
		bounds.w = (float)pic->size.x * PIC_SCALE;
		bounds.h = (float)pic->size.y * PIC_SCALE;
		nk_draw_image(
			&ec->ctx->current->buffer, bounds, &tex, nk_white);
	}
}
