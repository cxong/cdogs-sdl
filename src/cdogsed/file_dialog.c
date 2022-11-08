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
#define ROW_HEIGHT 35
#define FILE_LIST_HEIGHT (HEIGHT - ROW_HEIGHT_SMALL - 2 * ROW_HEIGHT)

typedef struct
{
	struct nk_context *ctx;
	CArray files; // of char[CDOGS_FILENAME_MAX]
	int selected;
	char pathOrig[CDOGS_PATH_MAX];
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
	char buf[CDOGS_PATH_MAX];
	sprintf(buf, "%s/%s", dir, filename);
	RealPath(buf, data.pathOrig);

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

static void OnSelectChange(FileData *fData, const int selected);
static bool OnSelect(SDL_Window *win, FileData *fData);
static bool DrawDialog(SDL_Window *win, struct nk_context *ctx, void *data)
{
	FileData *fData = data;
	char buf[CDOGS_PATH_MAX];

	bool done = false;
	float y = 0;
	if (nk_begin(
			ctx, "Dir", nk_rect(0, y, WIDTH, ROW_HEIGHT_SMALL),
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
	y += ROW_HEIGHT_SMALL;

	if (nk_begin(
			ctx, "Picker", nk_rect(0, y, WIDTH, FILE_LIST_HEIGHT),
			NK_WINDOW_BORDER))
	{
		nk_layout_row_dynamic(ctx, ROW_HEIGHT_SMALL, 1);

		CA_FOREACH(const char, filename, fData->files)
		const bool selected = fData->selected == _ca_index;
		nk_select_label(
			fData->ctx, filename, NK_TEXT_ALIGN_CENTERED | NK_TEXT_ALIGN_LEFT,
			selected);
		if (nk_widget_is_mouse_clicked(ctx, NK_BUTTON_LEFT))
		{
			OnSelectChange(fData, _ca_index);
			done = OnSelect(win, fData);
		}
		else if (nk_widget_is_hovered(ctx))
		{
			OnSelectChange(fData, _ca_index);
		}
		CA_FOREACH_END()

		if (nk_input_is_key_pressed(&ctx->input, NK_KEY_DOWN))
		{
			OnSelectChange(
				fData,
				CLAMP_OPPOSITE(
					fData->selected + 1, 0, (int)fData->files.size - 1));
		}
		else if (nk_input_is_key_pressed(&ctx->input, NK_KEY_UP))
		{
			OnSelectChange(
				fData,
				CLAMP_OPPOSITE(
					fData->selected - 1, 0, (int)fData->files.size - 1));
		}
		else if (
			nk_input_is_key_pressed(&ctx->input, NK_KEY_ENTER) &&
			fData->selected >= 0)
		{
			done = OnSelect(win, fData);
		}

		nk_end(ctx);
	}
	y += FILE_LIST_HEIGHT;

	if (nk_begin(
			ctx, "Filename", nk_rect(0, y, WIDTH, ROW_HEIGHT),
			NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR))
	{
		nk_layout_row_dynamic(ctx, ROW_HEIGHT - 10, 1);
		DrawTextbox(
			ctx, fData->filename, CDOGS_FILENAME_MAX, NULL, NK_EDIT_FIELD);
		nk_end(ctx);
	}
	y += ROW_HEIGHT;

	if (nk_begin(
			ctx, "Controls", nk_rect(0, y, WIDTH, ROW_HEIGHT),
			NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR))
	{
		nk_layout_row_dynamic(ctx, ROW_HEIGHT - 10, 2);
		if (nk_button_label(ctx, "OK") && fData->selected >= 0)
		{
			done = OnSelect(win, fData);
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
static void OnSelectChange(FileData *fData, const int selected)
{
	if (fData->selected == selected || selected < 0 ||
		selected >= (int)fData->files.size)
		return;

	const char *filename = CArrayGet(&fData->files, selected);
	strcpy(fData->filename, filename);
	fData->selected = selected;
}
static bool OnSelect(SDL_Window *win, FileData *fData)
{
	char buf[CDOGS_PATH_MAX];
	sprintf(buf, "%s/%s", fData->dir, fData->filename);
	char buf2[CDOGS_PATH_MAX];
	RealPath(buf, buf2);
	if (fData->filename[strlen(fData->filename) - 1] == '/')
	{
		fData->selected = 0;
		OpenDir(win, fData, buf2);
		fData->filename[0] = '\0';
		return false;
	}
	else
	{
		if (strcmp(buf2, fData->pathOrig) != 0 && !fData->mustExist)
		{
			// Overwriting file; confirm whether to overwrite
			const SDL_MessageBoxButtonData buttons[] = {
				{SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 0, "OK"},
				{SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 1, "Cancel"},
			};
			const SDL_MessageBoxData messageboxdata = {
				SDL_MESSAGEBOX_INFORMATION | SDL_MESSAGEBOX_BUTTONS_LEFT_TO_RIGHT,
				win,
				"Overwrite File", // title
				"File already exists. Do you want to overwrite?",
				SDL_arraysize(buttons),
				buttons,
				NULL,
			};
			int buttonid;
			if (SDL_ShowMessageBox(&messageboxdata, &buttonid) < 0)
			{
				LOG(LM_EDIT, LL_ERROR, "error displaying message box");
			}
			if (buttonid != 0)
			{
				return false;
			}
		}
		fData->result = true;
		return true;
	}
}
