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
#include "pic_manager.h"

#include <assert.h>

#include <SDL_image.h>

#include <tinydir/tinydir.h>

#include "files.h"

PicManager gPicManager;

int PicManagerTryInit(
	PicManager *pm, const char *oldGfxFile1, const char *oldGfxFile2)
{
	int i;
	memset(pm, 0, sizeof *pm);
	CArrayInit(&pm->pics, sizeof(NamedPic));
	i = ReadPics(
		GetDataFilePath(oldGfxFile1), pm->oldPics, PIC_COUNT1, pm->palette);
	if (!i)
	{
		printf("Unable to read %s\n", GetDataFilePath(oldGfxFile1));
		return 0;
	}
	if (!AppendPics(
		GetDataFilePath(oldGfxFile2), pm->oldPics, PIC_COUNT1, PIC_MAX))
	{
		printf("Unable to read %s\n", GetDataFilePath(oldGfxFile2));
		return 0;
	}
	pm->palette[0].r = pm->palette[0].g = pm->palette[0].b = 0;
	return 1;
}
static void AddPic(PicManager *pm, const char *name, const char *path)
{
	SDL_Surface *image = IMG_Load(path);
	if (!image)
	{
		perror("Cannot load image");
		fprintf(stderr, "IMG_Load: %s\n", IMG_GetError());
		return;
	}
	char buf[CDOGS_FILENAME_MAX];
	const char *dot = strrchr(name, '.');
	if (dot)
	{
		strncpy(buf, name, dot - name);
		buf[dot - name] = '\0';
	}
	else
	{
		strcpy(buf, name);
	}
	// TODO: check if name already exists
	// TODO: use efficient data structure like trie
	NamedPic n;
	CSTRDUP(n.name, buf);
	n.pic.size = Vec2iNew(image->w, image->h);
	n.pic.offset = Vec2iZero();
	SDL_Surface *s = SDL_ConvertSurface(
		image, gGraphicsDevice.screen->format, SDL_SWSURFACE);
	if (!s)
	{
		assert(0 && "image convert failed");
		return;
	}
	int picSize = n.pic.size.x * n.pic.size.y * sizeof *n.pic.Data;
	CMALLOC(n.pic.Data, picSize);
	SDL_LockSurface(s);
	// Manually copy the pixels and replace the alpha component,
	// since our gfx device format has no alpha
	for (int i = 0; i < n.pic.size.x * n.pic.size.y; i++)
	{
		Uint32 pixel = ((Uint32 *)s->pixels)[i];
		Uint32 alpha = ((Uint32 *)image->pixels)[i] >> image->format->Ashift;
		Uint32 rgbMask = s->format->Rmask | s->format->Gmask | s->format->Bmask;
		n.pic.Data[i] = (pixel & rgbMask) | (alpha << gGraphicsDevice.Ashift);
	}
	SDL_UnlockSurface(s);
	SDL_FreeSurface(s);
	SDL_FreeSurface(image);
	CArrayPushBack(&pm->pics, &n);
}
static void PicManagerLoadDirImpl(
	PicManager *pm, const char *path, const char *prefix)
{
	tinydir_dir dir;
	if (tinydir_open(&dir, path) == -1)
	{
		perror("Cannot open image dir");
		goto bail;
	}

	for (; dir.has_next; tinydir_next(&dir))
	{
		tinydir_file file;
		if (tinydir_readfile(&dir, &file) == -1)
		{
			perror("Cannot read image file");
			goto bail;
		}
		if (file.is_reg &&
			IMG_isPNG(SDL_RWFromFile(file.path, "rb")))
		{
			if (prefix)
			{
				char buf[CDOGS_PATH_MAX];
				sprintf(buf, "%s/%s", prefix, file.name);
				AddPic(pm, buf, file.path);
			}
			else
			{
				AddPic(pm, file.name, file.path);
			}
		}
		else if (file.is_dir && file.name[0] != '.')
		{
			if (prefix)
			{
				char buf[CDOGS_PATH_MAX];
				sprintf(buf, "%s/%s", prefix, file.name);
				PicManagerLoadDirImpl(pm, file.path, buf);
			}
			else
			{
				PicManagerLoadDirImpl(pm, file.path, file.name);
			}
		}
	}

bail:
	tinydir_close(&dir);
}
void PicManagerLoadDir(PicManager *pm, const char *path)
{
	if (!IMG_Init(IMG_INIT_PNG))
	{
		perror("Cannot initialise SDL_Image");
		return;
	}
	PicManagerLoadDirImpl(pm, path, NULL);
}

void PicManagerGenerateOldPics(PicManager *pm, GraphicsDevice *g)
{
	int i;
	// Convert old pics into new format ones
	// TODO: this is wasteful; better to eliminate old pics altogether
	// Note: always need to reload in editor since colours could change,
	// requiring an updating of palettes
	for (i = 0; i < PIC_MAX; i++)
	{
		PicPaletted *oldPic = PicManagerGetOldPic(pm, i);
		if (PicIsNotNone(&pm->picsFromOld[i]))
		{
			PicFree(&pm->picsFromOld[i]);
		}
		if (oldPic == NULL)
		{
			memcpy(&pm->picsFromOld[i], &picNone, sizeof picNone);
		}
		else
		{
			PicFromPicPaletted(g, &pm->picsFromOld[i], oldPic);
		}
	}
}

void PicManagerTerminate(PicManager *pm)
{
	for (int i = 0; i < PIC_MAX; i++)
	{
		if (pm->oldPics[i] != NULL)
		{
			CFREE(pm->oldPics[i]);
		}
		if (PicIsNotNone(&pm->picsFromOld[i]))
		{
			PicFree(&pm->picsFromOld[i]);
		}
	}
	for (int i = 0; i < (int)pm->pics.size; i++)
	{
		NamedPic *n = CArrayGet(&pm->pics, i);
		CFREE(n->name);
		PicFree(&n->pic);
	}
	CArrayTerminate(&pm->pics);
	IMG_Quit();
}

PicPaletted *PicManagerGetOldPic(PicManager *pm, int idx)
{
	if (idx < 0)
	{
		return NULL;
	}
	return pm->oldPics[idx];
}
Pic *PicManagerGetFromOld(PicManager *pm, int idx)
{
	if (idx < 0)
	{
		return NULL;
	}
	return &pm->picsFromOld[idx];
}
Pic *PicManagerGetPic(PicManager *pm, const char *name)
{
	for (int i = 0; i < (int)pm->pics.size; i++)
	{
		NamedPic *n = CArrayGet(&pm->pics, i);
		if (strcmp(n->name, name) == 0)
		{
			return &n->pic;
		}
	}
	return NULL;
}


Pic PicFromTOffsetPic(PicManager *pm, TOffsetPic op)
{
	Pic *opPic = PicManagerGetFromOld(pm, op.picIndex);
	Pic pic;
	if (opPic == NULL)
	{
		return picNone;
	}
	pic.size = opPic->size;
	pic.offset = Vec2iNew(op.dx, op.dy);
	pic.Data = opPic->Data;
	return pic;
}
