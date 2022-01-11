/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.
	Copyright (c) 2013-2020 Cong Xu
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

#ifdef __EMSCRIPTEN__
#include <SDL2/SDL_image.h>
#else
#include <SDL_image.h>
#endif

#include <tinydir/tinydir.h>

#include "files.h"
#include "log.h"

#define GRAPHICS_DIR "graphics"

PicManager gPicManager;

void PicManagerInit(PicManager *pm)
{
	if (!IMG_Init(IMG_INIT_PNG))
	{
		perror("Cannot initialise SDL_Image");
		return;
	}
	memset(pm, 0, sizeof *pm);
	pm->pics = hashmap_new();
	pm->sprites = hashmap_new();
	pm->customPics = hashmap_new();
	pm->customSprites = hashmap_new();
	CArrayInit(&pm->hairstyleNames, sizeof(char *));
	CArrayInit(&pm->wallStyleNames, sizeof(char *));
	CArrayInit(&pm->tileStyleNames, sizeof(char *));
	CArrayInit(&pm->exitStyleNames, sizeof(char *));
	CArrayInit(&pm->doorStyleNames, sizeof(char *));
	CArrayInit(&pm->keyStyleNames, sizeof(char *));
}

static NamedPic *AddNamedPic(map_t pics, const char *name, const Pic *p);
static NamedSprites *AddNamedSprites(map_t sprites, const char *name);
static void AfterAdd(PicManager *pm);
static void PicManagerAdd(
	map_t pics, map_t sprites, const char *name, SDL_Surface *imageIn)
{
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
	// Special case: if the file name is in the form foobar_WxH.ext,
	// this is a spritesheet where each sprite is W wide by H high
	// Load multiple images from this single sheet
	struct vec2i size = svec2i(imageIn->w, imageIn->h);
	bool isSpritesheet = false;
	char *underscore = strrchr(buf, '_');
	const char *x = strrchr(buf, 'x');
	if (underscore != NULL && x != NULL && underscore + 1 < x &&
		x + 1 < buf + strlen(buf))
	{
		if (sscanf(underscore, "_%dx%d", &size.x, &size.y) != 2)
		{
			size = svec2i(imageIn->w, imageIn->h);
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
		nsp = AddNamedSprites(sprites, buf);
	}
	else
	{
		np = AddNamedPic(pics, buf, NULL);
	}
	// Use 32-bit image
	SDL_Surface *image =
		SDL_ConvertSurfaceFormat(imageIn, SDL_PIXELFORMAT_RGBA8888, 0);
	SDL_LockSurface(image);
	struct vec2i offset;
	for (offset.y = 0; offset.y < image->h; offset.y += size.y)
	{
		for (offset.x = 0; offset.x < image->w; offset.x += size.x)
		{
			Pic *pic;
			if (isSpritesheet)
			{
				Pic p;
				memset(&p, 0, sizeof p);
				CArrayPushBack(&nsp->pics, &p);
				pic = CArrayGet(&nsp->pics, nsp->pics.size - 1);
			}
			else
			{
				pic = &np->pic;
			}
			PicLoad(pic, size, offset, image);

			if (strncmp("chars/", buf, strlen("chars/")) == 0)
			{
				// Convert char pics to multichannel version
				for (int i = 0; i < pic->size.x * pic->size.y; i++)
				{
					color_t c = PIXEL2COLOR(pic->Data[i]);
					// Don't bother if the alpha has already been modified; it
					// means we have already processed this pixel
					if (c.a != 255)
					{
						continue;
					}
					// Convert character color keyed color to
					// greyscale + special alpha
					const CharColorType colorType = CharColorTypeFromColor(c);
					color_t converted = c;
					if (colorType != CHAR_COLOR_COUNT)
					{
						const uint8_t value = MAX(MAX(c.r, c.g), c.b);
						converted.r = converted.g = converted.b = value;
						converted.a = CharColorTypeAlpha(colorType);
					}
					pic->Data[i] = COLOR2PIXEL(converted);
				}
			}
		}
	}
	SDL_UnlockSurface(image);
	SDL_FreeSurface(image);

	AfterAdd(&gPicManager);
}

void PicManagerLoadDir(
	PicManager *pm, const char *path, const char *prefix, map_t pics,
	map_t sprites)
{
	tinydir_dir dir;
	if (tinydir_open(&dir, path) == -1)
	{
		if (errno != ENOENT)
		{
			LOG(LM_MAIN, LL_ERROR, "Error opening image dir '%s': %s", path,
				strerror(errno));
		}
		goto bail;
	}

	for (; dir.has_next; tinydir_next(&dir))
	{
		tinydir_file file;
		if (tinydir_readfile(&dir, &file) == -1)
		{
			LOG(LM_MAIN, LL_ERROR, "Cannot read file '%s': %s", file.path,
				strerror(errno));
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
					LOG(LM_MAIN, LL_ERROR, "Cannot load image IMG_Load: %s",
						IMG_GetError());
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
					PicManagerAdd(pics, sprites, buf, data);
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
				PicManagerLoadDir(pm, file.path, buf, pics, sprites);
			}
			else
			{
				PicManagerLoadDir(pm, file.path, file.name, pics, sprites);
			}
		}
	}

bail:
	tinydir_close(&dir);
}
void PicManagerLoad(PicManager *pm)
{
	char buf[CDOGS_PATH_MAX];
	GetDataFilePath(buf, GRAPHICS_DIR);
	PicManagerLoadDir(pm, buf, NULL, pm->pics, pm->sprites);
}

static void FindStylePics(
	PicManager *pm, CArray *styleNames, PFany hashmapFunc);
static void FindStyleSprites(
	PicManager *pm, CArray *styleNames, PFany hashmapFunc);
static int MaybeAdHairSpriteName(any_t data, any_t item);
static int MaybeAddWallPicName(any_t data, any_t item);
static int MaybeAddTilePicName(any_t data, any_t item);
static int MaybeAddExitPicName(any_t data, any_t item);
static int MaybeAddKeyPicName(any_t data, any_t item);
static int MaybeAddDoorPicName(any_t data, any_t item);
static void AfterAdd(PicManager *pm)
{
	FindStyleSprites(pm, &pm->hairstyleNames, MaybeAdHairSpriteName);
	FindStylePics(pm, &pm->wallStyleNames, MaybeAddWallPicName);
	FindStylePics(pm, &pm->tileStyleNames, MaybeAddTilePicName);
	FindStylePics(pm, &pm->exitStyleNames, MaybeAddExitPicName);
	FindStylePics(pm, &pm->doorStyleNames, MaybeAddDoorPicName);
	FindStylePics(pm, &pm->keyStyleNames, MaybeAddKeyPicName);
}
static int CompareStyleNames(const void *v1, const void *v2);
static void StylesClear(CArray *styles)
{
	CA_FOREACH(char *, styleName, *styles)
	CFREE(*styleName);
	CA_FOREACH_END()
	CArrayClear(styles);
}
static void FindStylePics(
	PicManager *pm, CArray *styleNames, PFany hashmapFunc)
{
	// Scan all pics for style pics
	StylesClear(styleNames);
	hashmap_iterate(pm->customPics, hashmapFunc, pm);
	hashmap_iterate(pm->pics, hashmapFunc, pm);
	// Sort the style names alphabetically
	// This prevents the list from reordering unpredictably, when the editor
	// is used and masked pics get added
	if (styleNames->data != NULL)
	{
		qsort(
			styleNames->data, styleNames->size, styleNames->elemSize,
			CompareStyleNames);
	}
}
static int CompareStyleNames(const void *v1, const void *v2)
{
	const char *const *s1 = v1;
	const char *const *s2 = v2;
	return strcmp(*s1, *s2);
}
static void FindStyleSprites(
	PicManager *pm, CArray *styleNames, PFany hashmapFunc)
{
	// Scan all pics for style sprites
	StylesClear(styleNames);
	hashmap_iterate(pm->customSprites, hashmapFunc, pm);
	hashmap_iterate(pm->sprites, hashmapFunc, pm);
	// Sort the style names alphabetically
	// This prevents the list from reordering unpredictably, when the editor
	// is used and masked pics get added
	if (styleNames->data != NULL)
	{
		qsort(
			styleNames->data, styleNames->size, styleNames->elemSize,
			CompareStyleNames);
	}
}
static void MaybeAddStyleName(
	const char *picName, const char *prefix, CArray *styleNames)
{
	// Look for style names within a full name of the form:
	// prefix/style/suffix
	// NOTE: prefix should include trailing slash
	if (strncmp(picName, prefix, strlen(prefix)) != 0)
	{
		return;
	}
	const char *nextSlash = strchr(picName + strlen(prefix), '/');
	if (nextSlash == NULL)
	{
		nextSlash = picName + strlen(picName);
	}
	char buf[CDOGS_PATH_MAX];
	const size_t len = nextSlash - picName - strlen(prefix);
	strncpy(buf, picName + strlen(prefix), len);
	buf[len] = '\0';
	// Check if we already have the style name
	// This can happen if a custom pic uses the same name as a built in one
	CA_FOREACH(char *, styleName, *styleNames)
	if (strcmp(*styleName, buf) == 0)
	{
		return;
	}
	CA_FOREACH_END()

	char *s;
	CSTRDUP(s, buf);
	CArrayPushBack(styleNames, &s);
}
static int MaybeAdHairSpriteName(any_t data, any_t item)
{
	PicManager *pm = data;
	MaybeAddStyleName(
		((const NamedSprites *)item)->name, "chars/hairs/",
		&pm->hairstyleNames);
	return MAP_OK;
}
static int MaybeAddExitPicName(any_t data, any_t item)
{
	// Exit pics should be like:
	// exits/style/shadow
	// where style is the style name to be stored, and
	// shadow is normal/shadow
	PicManager *pm = data;
	MaybeAddStyleName(
		((const NamedPic *)item)->name, "exits/", &pm->exitStyleNames);
	return MAP_OK;
}
static int MaybeAddDoorPicName(any_t data, any_t item)
{
	// Door pics should be like:
	// door/style/type
	// where style is the style name to be stored
	PicManager *pm = data;
	MaybeAddStyleName(
		((const NamedPic *)item)->name, "door/", &pm->doorStyleNames);
	return MAP_OK;
}
static int MaybeAddKeyPicName(any_t data, any_t item)
{
	// Key pics should be like:
	// keys/style/colour
	// where style is the style name to be stored, and
	// colour is yellow/green/blue/red
	// TODO: more colours
	PicManager *pm = data;
	MaybeAddStyleName(
		((const NamedPic *)item)->name, "keys/", &pm->keyStyleNames);
	return MAP_OK;
}
static int MaybeAddWallPicName(any_t data, any_t item)
{
	// Wall pics should be like:
	// wall/style/type
	// where style is the style name to be stored
	PicManager *pm = data;
	MaybeAddStyleName(
		((const NamedPic *)item)->name, "wall/", &pm->wallStyleNames);
	return MAP_OK;
}
static int MaybeAddTilePicName(any_t data, any_t item)
{
	// Tile pics should be like:
	// tile/style/type
	// where style is the style name to be stored, and
	// type is normal/shadow/alt1/alt2
	PicManager *pm = data;
	MaybeAddStyleName(
		((const NamedPic *)item)->name, "tile/", &pm->tileStyleNames);
	return MAP_OK;
}

// Need to free the pics and the memory since hashmap stores on heap
static void NamedPicDestroy(any_t data);
static void NamedSpritesDestroy(any_t data);
void PicManagerClearCustom(PicManager *pm)
{
	hashmap_clear(pm->customPics, NamedPicDestroy);
	hashmap_clear(pm->customSprites, NamedSpritesDestroy);
	AfterAdd(pm);
}
static void PicManagerUnload(PicManager *pm)
{
	hashmap_clear(pm->pics, NamedPicDestroy);
	hashmap_clear(pm->sprites, NamedSpritesDestroy);
	hashmap_clear(pm->customPics, NamedPicDestroy);
	hashmap_clear(pm->customSprites, NamedSpritesDestroy);
	AfterAdd(pm);
}
static void StyleNamesDestroy(CArray *a)
{
	CA_FOREACH(char, n, *a)
	CFREE(n);
	CA_FOREACH_END()
	CArrayTerminate(a);
}
void PicManagerTerminate(PicManager *pm)
{
	PicManagerUnload(pm);
	StyleNamesDestroy(&pm->hairstyleNames);
	StyleNamesDestroy(&pm->wallStyleNames);
	StyleNamesDestroy(&pm->tileStyleNames);
	StyleNamesDestroy(&pm->exitStyleNames);
	StyleNamesDestroy(&pm->doorStyleNames);
	StyleNamesDestroy(&pm->keyStyleNames);
	IMG_Quit();
}
static void NamedPicDestroy(any_t data)
{
	NamedPic *n = data;
	NamedPicFree(n);
	CFREE(n);
}
static void NamedSpritesDestroy(any_t data)
{
	NamedSprites *n = data;
	NamedSpritesFree(n);
	CFREE(n);
}
static int ReloadTexture(any_t data, any_t item);
static int ReloadSpriteTexture(any_t data, any_t item);
void PicManagerReloadTextures(PicManager *pm)
{
	hashmap_iterate(pm->pics, ReloadTexture, pm);
	hashmap_iterate(pm->customPics, ReloadTexture, pm);
	hashmap_iterate(pm->sprites, ReloadSpriteTexture, pm);
	hashmap_iterate(pm->customSprites, ReloadSpriteTexture, pm);
}
static int ReloadTexture(any_t data, any_t item)
{
	UNUSED(data);
	NamedPic *n = item;
	if (!PicTryMakeTex(&n->pic))
	{
		LOG(LM_MAIN, LL_ERROR, "failed to reload pic texture");
		n->pic.Tex = NULL;
	}
	return MAP_OK;
}
static int ReloadSpriteTexture(any_t data, any_t item)
{
	UNUSED(data);
	NamedSprites *n = item;
	CA_FOREACH(Pic, op, n->pics)
	if (!PicTryMakeTex(op))
	{
		LOG(LM_MAIN, LL_ERROR, "failed to reload pic texture");
		op->Tex = NULL;
	}
	CA_FOREACH_END()
	return MAP_OK;
}

NamedPic *PicManagerGetNamedPic(const PicManager *pm, const char *name)
{
	NamedPic *n;
	int error = hashmap_get(pm->customPics, name, (any_t *)&n);
	if (error == MAP_OK)
	{
		return n;
	}
	error = hashmap_get(pm->pics, name, (any_t *)&n);
	if (error == MAP_OK)
	{
		return n;
	}
	return NULL;
}
Pic *PicManagerGetPic(const PicManager *pm, const char *name)
{
	NamedPic *n = PicManagerGetNamedPic(pm, name);
	if (n != NULL)
		return &n->pic;
	return NULL;
}
const NamedSprites *PicManagerGetSprites(
	const PicManager *pm, const char *name)
{
	NamedSprites *n;
	int error = hashmap_get(pm->customSprites, name, (any_t *)&n);
	if (error == MAP_OK)
	{
		return n;
	}
	error = hashmap_get(pm->sprites, name, (any_t *)&n);
	if (error == MAP_OK)
	{
		return n;
	}
	return NULL;
}

static void GetMaskedName(
	char *buf, const char *name, const color_t mask, const color_t maskAlt);

// Get a pic that is colour-masked.
// The name of the pic will be <name>/<mask>/<maskAlt>
// Used for dynamic map tile pic colours
static NamedPic *PicManagerGetMaskedPic(
	const PicManager *pm, const char *name, const color_t mask,
	const color_t maskAlt)
{
	char maskedName[256];
	GetMaskedName(maskedName, name, mask, maskAlt);
	return PicManagerGetNamedPic(pm, maskedName);
}
NamedPic *PicManagerGetMaskedStylePic(
	const PicManager *pm, const char *name, const char *style,
	const char *type, const color_t mask, const color_t maskAlt)
{
	char buf[256];
	sprintf(buf, "%s/%s/%s", name, style, type);
	return PicManagerGetMaskedPic(pm, buf, mask, maskAlt);
}

static void PicManagerGenerateMaskedPic(
	PicManager *pm, const char *name, const color_t mask,
	const color_t maskAlt, const bool noAltMask)
{
	char maskedName[256];
	GetMaskedName(maskedName, name, mask, maskAlt);
	// Check if the masked pic already exists
	if (PicManagerGetPic(pm, maskedName) != NULL)
		return;

	// Check if the original pic is available; if not then it's impossible to
	// create the masked version
	Pic *original = PicManagerGetPic(pm, name);
	CASSERT(original != NULL, "Cannot find original pic for masking");

	// Create the new pic by masking the original pic
	Pic p = PicCopy(original);
	for (int i = 0; i < p.size.x * p.size.y; i++)
	{
		color_t c = PIXEL2COLOR(original->Data[i]);
		// Apply mask based on which channel each pixel is
		if (c.g <= 2 && c.b <= 2 && !noAltMask)
		{
			// Restore to white before masking
			c.g = c.r;
			c.b = c.r;
			c = ColorMult(c, maskAlt);
		}
		else if (c.r == c.g && c.g == c.b)
		{
			c = ColorMult(c, mask);
		}
		p.Data[i] = COLOR2PIXEL(c);
		// TODO: more channels
	}
	if (!PicTryMakeTex(&p))
	{
		p.Tex = NULL;
	}
	AddNamedPic(pm->customPics, maskedName, &p);

	AfterAdd(pm);
}
void PicManagerGenerateMaskedStylePic(
	PicManager *pm, const char *name, const char *style, const char *type,
	const color_t mask, const color_t maskAlt, const bool noAltMask)
{
	char buf[256];
	sprintf(buf, "%s/%s/%s", name, style, type);
	PicManagerGenerateMaskedPic(pm, buf, mask, maskAlt, noAltMask);
}

const NamedSprites *PicManagerGetCharSprites(
	PicManager *pm, const char *name, const CharColors *colors)
{
	char buf[CDOGS_PATH_MAX];
	CharColorsGetMaskedName(buf, name, colors);
	// Get or generate masked sprites
	const NamedSprites *ns = PicManagerGetSprites(&gPicManager, buf);
	if (ns != NULL)
	{
		return ns;
	}
	const NamedSprites *ons = PicManagerGetSprites(pm, name);
	if (ons == NULL)
	{
		return NULL;
	}
	NamedSprites *nsp = AddNamedSprites(pm->customSprites, buf);
	CA_FOREACH(Pic, op, ons->pics)
	Pic p = PicCopy(op);
	p.Tex = NULL;
	for (int i = 0; i < p.size.x * p.size.y; i++)
	{
		if (op->Data[i] == 0)
		{
			continue;
		}
		const color_t c = PIXEL2COLOR(op->Data[i]);
		p.Data[i] =
			COLOR2PIXEL(ColorMult(c, CharColorsGetChannelMask(colors, c.a)));
	}
	if (!PicTryMakeTex(&p))
	{
		p.Tex = NULL;
	}
	CArrayPushBack(&nsp->pics, &p);
	CA_FOREACH_END()
	AfterAdd(pm);
	return nsp;
}

static void GetMaskedName(
	char *buf, const char *name, const color_t mask, const color_t maskAlt)
{
	char maskName[COLOR_STR_BUF];
	ColorStr(maskName, mask);
	char maskAltName[COLOR_STR_BUF];
	ColorStr(maskAltName, maskAlt);
	sprintf(buf, "%s/%s/%s", name, maskName, maskAltName);
}

static NamedPic *AddNamedPic(map_t pics, const char *name, const Pic *p)
{
	NamedPic *n;
	CMALLOC(n, sizeof *n);
	if (p != NULL)
		n->pic = *p;
	CSTRDUP(n->name, name);
	const int error = hashmap_put(pics, name, n);
	if (error != MAP_OK)
	{
		LOG(LM_MAIN, LL_ERROR, "failed to add named pic %s: %d", name, error);
		CFREE(n);
		return NULL;
	}
	return n;
}
static NamedSprites *AddNamedSprites(map_t sprites, const char *name)
{
	NamedSprites *ns;
	CMALLOC(ns, sizeof *ns);
	NamedSpritesInit(ns, name);
	const int error = hashmap_put(sprites, name, ns);
	if (error != MAP_OK)
	{
		LOG(LM_MAIN, LL_ERROR, "failed to add named sprites %s: %d", name,
			error);
		CFREE(ns);
		return NULL;
	}
	return ns;
}

int PicManagerGetWallStyleIndex(PicManager *pm, const char *style)
{
	CA_FOREACH(const char *, styleName, pm->wallStyleNames)
	if (strcmp(style, *styleName) == 0)
	{
		return _ca_index;
	}
	CA_FOREACH_END()
	return 0;
}
int PicManagerGetTileStyleIndex(PicManager *pm, const char *style)
{
	CA_FOREACH(const char *, styleName, pm->tileStyleNames)
	if (strcmp(style, *styleName) == 0)
	{
		return _ca_index;
	}
	CA_FOREACH_END()
	return 0;
}
int PicManagerGetExitStyleIndex(PicManager *pm, const char *style)
{
	CA_FOREACH(const char *, styleName, pm->exitStyleNames)
	if (strcmp(style, *styleName) == 0)
	{
		return _ca_index;
	}
	CA_FOREACH_END()
	return 0;
}
int PicManagerGetDoorStyleIndex(PicManager *pm, const char *style)
{
	CA_FOREACH(const char *, doorStyleName, pm->doorStyleNames)
	if (strcmp(style, *doorStyleName) == 0)
	{
		return _ca_index;
	}
	CA_FOREACH_END()
	return 0;
}
int PicManagerGetKeyStyleIndex(PicManager *pm, const char *style)
{
	CA_FOREACH(const char *, keyStyleName, pm->keyStyleNames)
	if (strcmp(style, *keyStyleName) == 0)
	{
		return _ca_index;
	}
	CA_FOREACH_END()
	return 0;
}
