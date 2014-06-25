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
	CArrayInit(&pm->sprites, sizeof(NamedSprites));
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
	if (image->format->BytesPerPixel != 4)
	{
		perror("Cannot load non-32-bit image");
		fprintf(stderr, "Only 32-bit depth images supported (%s)\n", path);
		SDL_FreeSurface(image);
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
	// Special case: if the file name is in the form foobar_WxH.ext,
	// this is a spritesheet where each sprite is W wide by H high
	// Load multiple images from this single sheet
	Vec2i size = Vec2iNew(image->w, image->h);
	bool isSpritesheet = false;
	char *underscore = strrchr(buf, '_');
	const char *x = strrchr(buf, 'x');
	if (underscore != NULL && x != NULL &&
		underscore + 1 < x && x + 1 < buf + strlen(buf))
	{
		if (sscanf(underscore, "_%dx%d", &size.x, &size.y) != 2)
		{
			size = Vec2iNew(image->w, image->h);
		}
		else
		{
			*underscore = '\0';
			isSpritesheet = true;
		}
	}
	NamedSprites *nsp = NULL;
	NamedPic *np = NULL;
	if (isSpritesheet)
	{
		NamedSprites ns;
		NamedSpritesInit(&ns, buf);
		CArrayPushBack(&pm->sprites, &ns);
		nsp = CArrayGet(&pm->sprites, pm->sprites.size - 1);
	}
	else
	{
		NamedPic n;
		CSTRDUP(n.name, buf);
		CArrayPushBack(&pm->pics, &n);
		np = CArrayGet(&pm->pics, pm->pics.size - 1);
	}
	SDL_LockSurface(image);
	SDL_Surface *s = SDL_ConvertSurface(
		image, gGraphicsDevice.screen->format, SDL_SWSURFACE);
	if (!s)
	{
		assert(0 && "image convert failed");
		return;
	}
	const int picSize = size.x * size.y * sizeof(((Pic *)0)->Data);
	SDL_LockSurface(s);
	Vec2i offset;
	for (offset.y = 0; offset.y < image->h; offset.y += size.y)
	{
		for (offset.x = 0; offset.x < image->w; offset.x += size.x)
		{
			Pic *pic;
			if (isSpritesheet)
			{
				Pic p;
				CArrayPushBack(&nsp->pics, &p);
				pic = CArrayGet(&nsp->pics, nsp->pics.size - 1);
			}
			else
			{
				pic = &np->pic;
			}
			pic->size = size;
			pic->offset = Vec2iZero();
			CMALLOC(pic->Data, picSize);
			// Manually copy the pixels and replace the alpha component,
			// since our gfx device format has no alpha
			int srcI = offset.y*image->w + offset.x;
			for (int i = 0; i < size.x * size.y; i++, srcI++)
			{
				const Uint32 alpha =
					((Uint32 *)image->pixels)[srcI] >> image->format->Ashift;
				// If completely transparent, replace rgb with black (0) too
				// This is because transparency blitting checks entire pixel
				if (alpha == 0)
				{
					pic->Data[i] = 0;
				}
				else
				{
					const Uint32 pixel = ((Uint32 *)s->pixels)[srcI];
					const Uint32 rgbMask =
						s->format->Rmask | s->format->Gmask | s->format->Bmask;
					pic->Data[i] =
						(pixel & rgbMask) | (alpha << gGraphicsDevice.Ashift);
				}
				if ((i + 1) % size.x == 0)
				{
					srcI += image->w - size.x;
				}
			}
		}
	}
	SDL_UnlockSurface(s);
	SDL_FreeSurface(s);
	SDL_UnlockSurface(image);
	SDL_FreeSurface(image);
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
		if (file.is_reg)
		{
			SDL_RWops *rwops = SDL_RWFromFile(file.path, "rb");
			bool isPng = IMG_isPNG(rwops);
			rwops->close(rwops);
			if (isPng)
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
static void LoadOldPic(
	PicManager *pm, const char *name, const TOffsetPic *pic);
static void LoadOldSprites(
	PicManager *pm, const char *name, const TOffsetPic *pics, const int count);
void PicManagerLoadDir(PicManager *pm, const char *path)
{
	if (!IMG_Init(IMG_INIT_PNG))
	{
		perror("Cannot initialise SDL_Image");
		return;
	}
	PicManagerLoadDirImpl(pm, path, NULL);

	// Load the old pics anyway;
	// even though they will be palette swapped later,
	// this allows us to initialise the data structures so that sprites can be
	// made.
	PicManagerGenerateOldPics(pm, &gGraphicsDevice);

	// Load old pics and sprites
	LoadOldPic(pm, "bullet", &cGeneralPics[OFSPIC_BULLET]);
	LoadOldPic(pm, "molotov", &cGeneralPics[OFSPIC_MOLOTOV]);
	LoadOldPic(pm, "mine_inactive", &cGeneralPics[OFSPIC_MINE]);
	LoadOldPic(pm, "mine_active", &cGeneralPics[OFSPIC_MINE]);
	LoadOldPic(pm, "dynamite", &cGeneralPics[OFSPIC_DYNAMITE]);
	LoadOldSprites(
		pm, "grenade", cGrenadePics,
		sizeof cGrenadePics / sizeof *cGrenadePics);
	LoadOldSprites(
		pm, "flame", cFlamePics, sizeof cFlamePics / sizeof *cFlamePics);
	LoadOldSprites(pm, "gas_cloud", cFireBallPics + 8, 4);
	LoadOldSprites(pm, "beam", cBeamPics[0], DIRECTION_COUNT);
	LoadOldSprites(pm, "beam_bright", cBeamPics[1], DIRECTION_COUNT);
}
static void LoadOldPic(
	PicManager *pm, const char *name, const TOffsetPic *pic)
{
	// Don't use old pics if new ones are available
	if (PicManagerGetPic(pm, name) != NULL)
	{
		return;
	}
	NamedPic p;
	CSTRDUP(p.name, name);
	const Pic *original = PicManagerGetFromOld(pm, pic->picIndex);
	PicCopy(&p.pic, original);
	CArrayPushBack(&pm->pics, &p);
}
static void LoadOldSprites(
	PicManager *pm, const char *name, const TOffsetPic *pics, const int count)
{
	// Don't use old sprites if new ones are available
	if (PicManagerGetSprites(pm, name) != NULL)
	{
		return;
	}
	NamedSprites ns;
	NamedSpritesInit(&ns, name);
	const TOffsetPic *pic = pics;
	for (int i = 0; i < count; i++, pic++)
	{
		Pic p;
		const Pic *original = PicManagerGetFromOld(pm, pic->picIndex);
		PicCopy(&p, original);
		CArrayPushBack(&ns.pics, &p);
	}
	CArrayPushBack(&pm->sprites, &ns);
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
	for (int i = 0; i < (int)pm->sprites.size; i++)
	{
		NamedSpritesFree(CArrayGet(&pm->sprites, i));
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
Pic *PicManagerGetPic(const PicManager *pm, const char *name)
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
const NamedSprites *PicManagerGetSprites(
	const PicManager *pm, const char *name)
{
	for (int i = 0; i < (int)pm->sprites.size; i++)
	{
		const NamedSprites *n = CArrayGet(&pm->sprites, i);
		if (strcmp(n->name, name) == 0)
		{
			return n;
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
