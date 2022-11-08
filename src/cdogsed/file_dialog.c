/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.
	Copyright (c) 2022 Cong Xu
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
#include "file_dialog.h"

#include <tinydir/tinydir.h>

#include <cdogs/log.h>

#include "nk_window.h"

#define WIDTH 400
#define HEIGHT 400
#define ROW_HEIGHT_SMALL 18
#define ROW_HEIGHT 25

typedef struct
{
	struct nk_context *ctx;
	CArray files; // of char[CDOGS_FILENAME_MAX]
	int selected;
	char filename[CDOGS_FILENAME_MAX];
	char dir[CDOGS_PATH_MAX];
	bool mustExist;
	bool result;
} FileData;

static void OpenDir(SDL_Window *win, FileData *data, const char *path);
static bool DrawDialog(SDL_Window *win, struct nk_context *ctx, void *data);

bool TryFileDialog(
	char *out, EventHandlers *handlers, const char *dir, const char *filename,
	const char *title, const bool mustExist)
{
	NKWindowConfig cfg;
	memset(&cfg, 0, sizeof cfg);
	cfg.Title = title;
	cfg.Size = svec2i(WIDTH, HEIGHT);
	color_t bg = {41, 26, 26, 255};
	cfg.BG = bg;
	cfg.Handlers = handlers;
	cfg.Draw = DrawDialog;

	NKWindowInit(&cfg);

	FileData data;
	memset(&data, 0, sizeof data);
	data.ctx = cfg.ctx;
	strcpy(data.filename, filename);
	data.mustExist = mustExist;
	OpenDir(NULL, &data, dir);
	cfg.DrawData = &data;

	NKWindow(cfg);

	if (data.result)
	{
		sprintf(out, "%s/%s", data.dir, data.filename);
	}
	return data.result;
}

static void OpenDir(SDL_Window *win, FileData *data, const char *path)
{
	// Fill FileData with the file listings of a dir
	tinydir_dir dir;

	if (tinydir_open_sorted(&dir, path) == -1)
	{
		LOG(LM_EDIT, LL_ERROR, "Cannot load dir %s", path);
		if (win)
		{
			char msgbuf[CDOGS_PATH_MAX];
			sprintf(msgbuf, "Cannot open %s", path);
			SDL_ShowSimpleMessageBox(
				SDL_MESSAGEBOX_ERROR, "Open Dir Error", msgbuf, win);
		}
		return;
	}

	RealPath(path, data->dir);

	CArrayTerminate(&data->files);
	CArrayInit(&data->files, CDOGS_FILENAME_MAX);
	char buf[CDOGS_FILENAME_MAX];
	if (strcmp(path, "/") != 0)
	{
		CArrayPushBack(&data->files, "../");
	}
	for (int i = 0; i < (int)dir.n_files; i++)
	{
		tinydir_file file;
		tinydir_readfile_n(&dir, &file, i);
		// Ignore campaigns that start with a ~; these are autosaved
		if (file.name[0] == '~' || file.name[0] == '.')
		{
			continue;
		}
		if ((file.is_dir && Stricmp(file.extension, "cdogscpn") == 0) ||
			(file.is_reg && Stricmp(file.extension, "cpn") == 0))
		{
			strcpy(buf, file.name);
			CArrayPushBack(&data->files, buf);
		}
		else if (file.is_dir)
		{
			sprintf(buf, "%s/", file.name);
			CArrayPushBack(&data->files, buf);
		}
	}

	tinydir_close(&dir);
}

bool TryOpenDir(
	char *out, EventHandlers *handlers, const char *dir, const char *filename)
{
	return TryFileDialog(out, handlers, dir, filename, "Open File", true);
}
bool TrySaveFile(
	char *out, EventHandlers *handlers, const char *dir, const char *filename)
{
	return TryFileDialog(out, handlers, dir, filename, "Save File", false);
}

static bool OnSelect(SDL_Window *win, FileData *fData, const char *filename);
static bool DrawDialog(SDL_Window *win, struct nk_context *ctx, void *data)
{
	FileData *fData = data;
	char buf[CDOGS_PATH_MAX];

	bool done = false;
	float y = 0;
	if (nk_begin(
			ctx, "Dir", nk_rect(0, y, WIDTH, ROW_HEIGHT),
			NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR))
	{
		const float colRatios[] = {0.1f, 0.9f};
		nk_layout_row(ctx, NK_DYNAMIC, ROW_HEIGHT, 2, colRatios);
		nk_label(ctx, "Dir:", NK_TEXT_ALIGN_LEFT);
		if (strlen(fData->dir) > 44)
		{
			sprintf(buf, "... %s", strrchr(fData->dir, '/'));
			nk_label(ctx, buf, NK_TEXT_ALIGN_LEFT);
		}
		else
		{
			nk_label(ctx, fData->dir, NK_TEXT_ALIGN_LEFT);
		}
		nk_end(ctx);
	}
	y += ROW_HEIGHT;

	if (nk_begin(
			ctx, "Picker", nk_rect(0, y, WIDTH, HEIGHT - y - ROW_HEIGHT),
			NK_WINDOW_BORDER))
	{
		nk_layout_row_dynamic(ctx, ROW_HEIGHT_SMALL, 1);

		CA_FOREACH(const char, filename, fData->files)
		const bool selected = fData->selected == _ca_index;
		nk_select_label(
			fData->ctx, filename, NK_TEXT_ALIGN_CENTERED | NK_TEXT_ALIGN_LEFT,
			selected);
		if (nk_widget_is_mouse_clicked(
				ctx, NK_BUTTON_LEFT))
		{
			done = OnSelect(win, fData, filename);
		}
		else if (nk_widget_is_hovered(ctx))
		{
			fData->selected = _ca_index;
		}
		CA_FOREACH_END()

		if (nk_input_is_key_pressed(&ctx->input, NK_KEY_DOWN))
		{
			fData->selected = CLAMP_OPPOSITE(
				fData->selected + 1, 0, (int)fData->files.size - 1);
		}
		else if (nk_input_is_key_pressed(&ctx->input, NK_KEY_UP))
		{
			fData->selected = CLAMP_OPPOSITE(
				fData->selected - 1, 0, (int)fData->files.size - 1);
		}
		else if (
			nk_input_is_key_pressed(&ctx->input, NK_KEY_ENTER) &&
			fData->selected >= 0)
		{
			const char *filename = CArrayGet(&fData->files, fData->selected);
			done = OnSelect(win, fData, filename);
		}

		nk_end(ctx);
	}
	y = HEIGHT - ROW_HEIGHT;

	if (nk_begin(
			ctx, "Controls", nk_rect(0, y, WIDTH, ROW_HEIGHT),
			NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR))
	{
		nk_layout_row_dynamic(ctx, ROW_HEIGHT, 2);
		if (nk_button_label(ctx, "OK") && fData->selected >= 0)
		{
			const char *filename = CArrayGet(&fData->files, fData->selected);
			done = OnSelect(win, fData, filename);
		}
		if (nk_button_label(ctx, "Cancel"))
		{
			fData->result = false;
			done = true;
		}
		nk_end(ctx);
	}

	return !done;
}
static bool OnSelect(SDL_Window *win, FileData *fData, const char *filename)
{
	if (filename[strlen(filename) - 1] == '/')
	{
		char buf[CDOGS_PATH_MAX];
		fData->selected = 0;
		sprintf(buf, "%s/%s", fData->dir, filename);
		char buf2[CDOGS_PATH_MAX];
		RealPath(buf, buf2);
		OpenDir(win, fData, buf2);
		fData->filename[0] = '\0';
		return false;
	}
	else
	{
		fData->result = true;
		strcpy(fData->filename, filename);
		return true;
	}
}
