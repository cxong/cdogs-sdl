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
	CArrayInit(&pm->customPics, sizeof(NamedPic));
	CArrayInit(&pm->customSprites, sizeof(NamedSprites));
	char buf[CDOGS_PATH_MAX];
	GetDataFilePath(buf, oldGfxFile1);
	i = ReadPics(buf, pm->oldPics, PIC_COUNT1, pm->palette);
	if (!i)
	{
		printf("Unable to read %s\n", buf);
		return 0;
	}
	GetDataFilePath(buf, oldGfxFile2);
	if (!AppendPics(buf, pm->oldPics, PIC_COUNT1, PIC_MAX))
	{
		printf("Unable to read %s\n", buf);
		return 0;
	}
	pm->palette[0].r = pm->palette[0].g = pm->palette[0].b = 0;
	return 1;
}
void PicManagerAdd(
	CArray *pics, CArray *sprites, const char *name, SDL_Surface *image)
{
	if (image->format->BytesPerPixel != 4)
	{
		perror("Cannot load non-32-bit image");
		fprintf(stderr, "Only 32-bit depth images supported (%s)\n", name);
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
		CArrayPushBack(sprites, &ns);
		nsp = CArrayGet(sprites, sprites->size - 1);
	}
	else
	{
		NamedPic n;
		CSTRDUP(n.name, buf);
		CArrayPushBack(pics, &n);
		np = CArrayGet(pics, pics->size - 1);
	}
	SDL_LockSurface(image);
	SDL_Surface *s = SDL_ConvertSurface(
		image, gGraphicsDevice.screen->format, SDL_SWSURFACE);
	CASSERT(s, "image convert failed");
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
			PicLoad(pic, size, offset, image, s);
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
			const bool isPng = IMG_isPNG(rwops);
			if (isPng)
			{
				SDL_Surface *data = IMG_Load_RW(rwops, 0);
				if (!data)
				{
					perror("Cannot load image");
					fprintf(stderr, "IMG_Load: %s\n", IMG_GetError());
				}
				else
				{
					char buf[CDOGS_PATH_MAX];
					if (prefix)
					{
						char buf1[CDOGS_PATH_MAX];
						sprintf(buf1, "%s/%s", prefix, file.name);
						PathGetWithoutExtension(buf, buf1);
					}
					else
					{
						PathGetBasenameWithoutExtension(buf, file.name);
					}
					PicManagerAdd(&pm->pics, &pm->sprites, buf, data);
				}
			}
			rwops->close(rwops);
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
	LoadOldSprites(pm, "fireball", cFireBallPics, FIREBALL_MAX);
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

void PicManagerClear(CArray *pics, CArray *sprites)
{
	for (int i = 0; i < (int)pics->size; i++)
	{
		NamedPic *n = CArrayGet(pics, i);
		CFREE(n->name);
		PicFree(&n->pic);
	}
	CArrayClear(pics);
	for (int i = 0; i < (int)sprites->size; i++)
	{
		NamedSpritesFree(CArrayGet(sprites, i));
	}
	CArrayClear(sprites);
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
	PicManagerClear(&pm->pics, &pm->sprites);
	CArrayTerminate(&pm->pics);
	CArrayTerminate(&pm->sprites);
	PicManagerClear(&pm->customPics, &pm->customSprites);
	CArrayTerminate(&pm->customPics);
	CArrayTerminate(&pm->customSprites);
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
	for (int i = 0; i < (int)pm->customPics.size; i++)
	{
		NamedPic *n = CArrayGet(&pm->customPics, i);
		if (strcmp(n->name, name) == 0)
		{
			return &n->pic;
		}
	}
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
Pic *PicManagerGet(PicManager *pm, const char *name, const int oldIdx)
{
	Pic *pic;
	if (!name || name[0] == '\0' ||
		ConfigGetBool(&gConfig, "Graphics.OriginalPics"))
	{
		goto defaultPic;
	}
	pic = PicManagerGetPic(pm, name);
	if (!pic)
	{
		goto defaultPic;
	}
	return pic;

defaultPic:
	pic = PicManagerGetFromOld(pm, oldIdx);
	CASSERT(pic != NULL, "Cannot find pic");
	if (pic == NULL)
	{
		pic = PicManagerGetFromOld(pm, PIC_UZIBULLET);
	}
	return pic;
}
const NamedSprites *PicManagerGetSprites(
	const PicManager *pm, const char *name)
{
	for (int i = 0; i < (int)pm->customSprites.size; i++)
	{
		const NamedSprites *n = CArrayGet(&pm->customSprites, i);
		if (strcmp(n->name, name) == 0)
		{
			return n;
		}
	}
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
