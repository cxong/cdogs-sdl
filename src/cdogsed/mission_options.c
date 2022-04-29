/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.
	Copyright (c) 2020-2021 Cong Xu
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
#include "mission_options.h"

#include "nk_window.h"

#define WIDTH 800
#define HEIGHT 200
#define ROW_HEIGHT 25

typedef struct
{
	Mission *m;
	char Title[256];
	char Description[1024];
	char MusicFilename[256];
	EditorResult result;
} MissionOptionsData;

static bool Draw(SDL_Window *win, struct nk_context *ctx, void *data);
static void CheckTextChanged(
	char **dst, const char *src, EditorResult *result);
EditorResult EditMissionOptions(EventHandlers *handlers, Mission *m)
{
	NKWindowConfig cfg;
	memset(&cfg, 0, sizeof cfg);
	cfg.Title = "Mission Options";
	cfg.Size = svec2i(WIDTH, HEIGHT);
	color_t bg = {41, 26, 26, 255};
	cfg.BG = bg;
	cfg.Handlers = handlers;
	cfg.Draw = Draw;

	NKWindowInit(&cfg);

	MissionOptionsData data;
	memset(&data, 0, sizeof data);
	data.m = m;
	if (m->Title)
	{
		strcpy(data.Title, m->Title);
	}
	if (m->Description)
	{
		strcpy(data.Description, m->Description);
	}
	if (m->Music.Type == MUSIC_SRC_DYNAMIC && m->Music.Data.Filename)
	{
		strcpy(data.MusicFilename, m->Music.Data.Filename);
	}
	cfg.DrawData = &data;

	NKWindow(cfg);

	CheckTextChanged(&m->Title, data.Title, &data.result);
	CheckTextChanged(&m->Description, data.Description, &data.result);
	if (m->Music.Type == MUSIC_SRC_DYNAMIC)
	{
		CheckTextChanged(&m->Music.Data.Filename, data.MusicFilename, &data.result);
	}
	else
	{
		CFREE(m->Music.Data.Filename);
		CSTRDUP(m->Music.Data.Filename, "");
	}
	return data.result;
}
static void CheckTextChanged(char **dst, const char *src, EditorResult *result)
{
	if (*dst == NULL || strcmp(src, *dst) != 0)
	{
		CFREE(*dst);
		CSTRDUP(*dst, src);
		*result = EDITOR_RESULT_CHANGED;
	}
}

static bool Draw(SDL_Window *win, struct nk_context *ctx, void *data)
{
	UNUSED(win);
	bool changed = false;
	MissionOptionsData *mData = data;

	if (nk_begin(
			ctx, "", nk_rect(0, 0, WIDTH, HEIGHT),
			NK_WINDOW_BORDER))
	{
		nk_layout_row_dynamic(ctx, ROW_HEIGHT, 1);
		DrawTextbox(ctx, mData->Title, 256, "Title", NK_EDIT_FIELD);
		nk_layout_row_dynamic(ctx, ROW_HEIGHT * 3, 1);
		DrawTextbox(ctx, mData->Description, 1024, "Description", NK_EDIT_BOX);
		nk_layout_row_dynamic(ctx, ROW_HEIGHT, 1);
		bool dynamicMusic = mData->m->Music.Type == MUSIC_SRC_DYNAMIC;
		if (DrawCheckbox(ctx, "Custom Music", NULL, &dynamicMusic))
		{
			changed = true;
			mData->m->Music.Type = dynamicMusic ? MUSIC_SRC_DYNAMIC : MUSIC_SRC_GENERAL;
		}
		if (dynamicMusic)
		{
			DrawTextbox(ctx, mData->MusicFilename, 256, "Music Filename", NK_EDIT_FIELD);
		}
		if (DrawCheckbox(
				ctx, "Persist weapons & ammo",
				"Whether weapons picked up and extra ammo are "
				"available in this mission",
				&mData->m->WeaponPersist))
		{
			changed = true;
		}
		if (DrawCheckbox(
				ctx, "Skip debrief",
				"Skip the end-of-mission summary screen",
				&mData->m->SkipDebrief))
		{
			changed = true;
		}
		nk_end(ctx);
	}
	if (changed)
	{
		mData->result = EDITOR_RESULT_CHANGED;
	}
	return true;
}
