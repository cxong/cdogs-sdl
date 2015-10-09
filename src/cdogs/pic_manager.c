/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (c) 2013-2015, Cong Xu
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

#include <SDL_image.h>

#include <tinydir/tinydir.h>

#include "files.h"
#include "log.h"

PicManager gPicManager;

// +--------------------+
// |  Color range info  |
// +--------------------+
#define WALL_COLORS       208
#define FLOOR_COLORS      216
#define ROOM_COLORS       232
#define ALT_COLORS        224

static uint8_t cWhiteValues[] = { 64, 56, 46, 36, 30, 24, 20, 16 };


static NamedPic *AddNamedPic(CArray *pics, const char *name, const Pic *p);

static void SetupPalette(TPalette palette);
bool PicManagerTryInit(
	PicManager *pm, const char *oldGfxFile1, const char *oldGfxFile2)
{
	memset(pm, 0, sizeof *pm);
	CArrayInit(&pm->pics, sizeof(NamedPic));
	CArrayInit(&pm->sprites, sizeof(NamedSprites));
	CArrayInit(&pm->customPics, sizeof(NamedPic));
	CArrayInit(&pm->customSprites, sizeof(NamedSprites));
	CArrayInit(&pm->drainPics, sizeof(NamedPic *));
	CArrayInit(&pm->doorStyleNames, sizeof(char *));

	char buf[CDOGS_PATH_MAX];
	GetDataFilePath(buf, oldGfxFile1);
	int i = ReadPics(buf, pm->oldPics, PIC_COUNT1, pm->palette);
	if (!i)
	{
		printf("Unable to read %s\n", buf);
		return false;
	}
	GetDataFilePath(buf, oldGfxFile2);
	if (!AppendPics(buf, pm->oldPics, PIC_COUNT1, PIC_MAX))
	{
		printf("Unable to read %s\n", buf);
		return false;
	}
	SetupPalette(pm->palette);
	return true;
}
static void SetPaletteRange(
	TPalette palette, const int start, const color_t mask);
static void SetupPalette(TPalette palette)
{
	palette[0].r = palette[0].g = palette[0].b = 0;

	// Set the coloured palette ranges
	// Note: alpha used as "channel"
	// These pics will be recoloured on demand by the PicManager based on
	// mission-specific colours requested during map load. Some pics will
	// have an "alt" colour in the same pic, so to differentiate and mask
	// each colour individually, those converted pics will use a different
	// colour mask, to signify different recolouring channels.
	SetPaletteRange(palette, WALL_COLORS, colorWhite);
	SetPaletteRange(palette, FLOOR_COLORS, colorWhite);
	SetPaletteRange(palette, ROOM_COLORS, colorWhite);
	SetPaletteRange(palette, ALT_COLORS, colorRed);
}
static void SetPaletteRange(
	TPalette palette, const int start, const color_t mask)
{
	for (int i = 0; i < 8; i++)
	{
		color_t c;
		c.r = c.g = c.b = cWhiteValues[i];
		c.a = 255;
		palette[start + i] = ColorMult(c, mask);
	}
}

static void AfterAdd(PicManager *pm);
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
		np = AddNamedPic(pics, buf, NULL);
	}
	SDL_LockSurface(image);
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
			PicLoad(pic, size, offset, image);
		}
	}
	SDL_UnlockSurface(image);
	SDL_FreeSurface(image);

	AfterAdd(&gPicManager);
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
					LOG(LM_MAIN, LL_ERROR, "Cannot load image");
					LOG(LM_MAIN, LL_ERROR, "IMG_Load: %s", IMG_GetError());
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
static void GenerateOldPics(PicManager *pm);
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
	GenerateOldPics(pm);

	// Load old pics and sprites
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
		Pic p = PicCopy(PicManagerGetFromOld(pm, pic->picIndex));
		CArrayPushBack(&ns.pics, &p);
	}
	CArrayPushBack(&pm->sprites, &ns);
}
static void AddMaskBasePic(
	PicManager *pm, const char *name,
	const char *styleName, const char *typeName, const int picIdx);
static void GenerateOldPics(PicManager *pm)
{
	// Convert old pics into new format ones
	for (int i = 0; i < PIC_MAX; i++)
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
			PicFromPicPaletted(&pm->picsFromOld[i], oldPic);
		}
	}

	// For the tile pics, generate named pics from them, since we will be
	// using them with masks later
	for (int i = 0; i < WALL_STYLE_COUNT; i++)
	{
		for (int j = 0; j < WALL_TYPES; j++)
		{
			AddMaskBasePic(
				pm, "wall", WallStyleStr(i), WallTypeStr(j), cWallPics[i][j]);
		}
	}
	for (int i = 0; i < FLOOR_STYLE_COUNT; i++)
	{
		for (int j = 0; j < FLOOR_TYPES; j++)
		{
			AddMaskBasePic(
				pm, "floor", FloorStyleStr(i), FloorTypeStr(j),
				cFloorPics[i][j]);
		}
	}
	for (int i = 0; i < ROOM_STYLE_COUNT; i++)
	{
		for (int j = 0; j < ROOMFLOOR_TYPES; j++)
		{
			AddMaskBasePic(
				pm, "room", RoomStyleStr(i), RoomTypeStr(j), cRoomPics[i][j]);
		}
	}
}
static void AddMaskBasePic(
	PicManager *pm, const char *name,
	const char *styleName, const char *typeName, const int picIdx)
{
	char buf[256];
	sprintf(buf, "%s/%s_%s", name, styleName, typeName);
	const PicPaletted *old = PicManagerGetOldPic(pm, picIdx);
	Pic p = PicCopy(PicManagerGetFromOld(pm, picIdx));
	// Detect alt pixels and modify their channel
	for (int i = 0; i < p.size.x * p.size.y; i++)
	{
		if (old->data[i] >= ALT_COLORS && old->data[i] < ALT_COLORS + 8)
		{
			color_t c = PIXEL2COLOR(p.Data[i]);
			c.a = 254;
			p.Data[i] = COLOR2PIXEL(c);
		}
	}
	AddNamedPic(&pm->pics, buf, &p);

	AfterAdd(pm);
}


static void FindDrainPics(PicManager *pm);
static void FindDoorPics(PicManager *pm);
static void AfterAdd(PicManager *pm)
{
	FindDrainPics(pm);
	FindDoorPics(pm);
}
static void FindDrainPics(PicManager *pm)
{
	// Scan all pics for drainage pics
	CArrayClear(&pm->drainPics);
	for (int i = 0;; i++)
	{
		char buf[CDOGS_FILENAME_MAX];
		sprintf(buf, "drains/%d", i);
		NamedPic *p = PicManagerGetNamedPic(pm, buf);
		if (p == NULL) break;
		CArrayPushBack(&pm->drainPics, &p);
	}
}
static void MaybeAddDoorPicName(PicManager *pm, const char *picName);
static void FindDoorPics(PicManager *pm)
{
	// Scan all pics for door pics
	CA_FOREACH(char *, doorStyleName, pm->doorStyleNames)
		CFREE(*doorStyleName);
	CA_FOREACH_END()
	CArrayClear(&pm->doorStyleNames);
	CA_FOREACH(NamedPic, p, pm->customPics)
		MaybeAddDoorPicName(pm, p->name);
	CA_FOREACH_END()
	CA_FOREACH(NamedPic, p, pm->pics)
		MaybeAddDoorPicName(pm, p->name);
	CA_FOREACH_END()
}
static void MaybeAddDoorPicName(PicManager *pm, const char *picName)
{
	// Use the "wall" pic name
	if (strncmp(picName, "door/", strlen("door/")) != 0 ||
		strcmp(picName + strlen(picName) - strlen("_wall"), "_wall") != 0)
	{
		return;
	}
	char buf[CDOGS_FILENAME_MAX];
	const size_t len = strlen(picName) - strlen("door/") - strlen("_wall");
	strncpy(buf, picName + strlen("door/"), len);
	buf[len] = '\0';
	// Check if we already have the door pic name
	// This can happen if a custom door pic uses the same name as a built in
	// one
	CA_FOREACH(char *, doorStyleName, pm->doorStyleNames)
		if (strcmp(*doorStyleName, buf) == 0)
		{
			return;
		}
	CA_FOREACH_END()

	char *s;
	CSTRDUP(s, buf);
	CArrayPushBack(&pm->doorStyleNames, &s);
}

static void PicManagerClear(CArray *pics, CArray *sprites);
void PicManagerClearCustom(PicManager *pm)
{
	PicManagerClear(&pm->customPics, &pm->customSprites);
	FindDrainPics(pm);
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
	PicManagerClearCustom(pm);
	CArrayTerminate(&pm->customPics);
	CArrayTerminate(&pm->customSprites);
	CArrayTerminate(&pm->drainPics);
	CA_FOREACH(char *, doorStyleName, pm->doorStyleNames)
		CFREE(*doorStyleName);
	CA_FOREACH_END()
	CArrayTerminate(&pm->doorStyleNames);
	IMG_Quit();
}
static void PicManagerClear(CArray *pics, CArray *sprites)
{
	for (int i = 0; i < (int)pics->size; i++)
	{
		NamedPic *n = CArrayGet(pics, i);
		PicFree(&n->pic);
		CFREE(n->name);
	}
	CArrayClear(pics);
	for (int i = 0; i < (int)sprites->size; i++)
	{
		NamedSpritesFree(CArrayGet(sprites, i));
	}
	CArrayClear(sprites);
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
NamedPic *PicManagerGetNamedPic(const PicManager *pm, const char *name)
{
	for (int i = 0; i < (int)pm->customPics.size; i++)
	{
		NamedPic *n = CArrayGet(&pm->customPics, i);
		if (strcmp(n->name, name) == 0)
		{
			return n;
		}
	}
	for (int i = 0; i < (int)pm->pics.size; i++)
	{
		NamedPic *n = CArrayGet(&pm->pics, i);
		if (strcmp(n->name, name) == 0)
		{
			return n;
		}
	}
	return NULL;
}
Pic *PicManagerGetPic(const PicManager *pm, const char *name)
{
	NamedPic *n = PicManagerGetNamedPic(pm, name);
	if (n != NULL) return &n->pic;
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

static void GetMaskedName(
	char *buf, const char *name, const color_t mask, const color_t maskAlt);
static void GetMaskedStyleName(
	char *buf, const char *name, const int style, const int type);

// Get a pic that is colour-masked.
// The name of the pic will be <name>_<mask>_<maskAlt>
// Used for dynamic map tile pic colours
static NamedPic *PicManagerGetMaskedPic(
	const PicManager *pm, const char *name,
	const color_t mask, const color_t maskAlt)
{
	char maskedName[256];
	GetMaskedName(maskedName, name, mask, maskAlt);
	return PicManagerGetNamedPic(pm, maskedName);
}
NamedPic *PicManagerGetMaskedStylePic(
	const PicManager *pm, const char *name, const int style, const int type,
	const color_t mask, const color_t maskAlt)
{
	char buf[256];
	GetMaskedStyleName(buf, name, style, type);
	return PicManagerGetMaskedPic(pm, buf, mask, maskAlt);
}

void PicManagerGenerateMaskedPic(
	PicManager *pm, const char *name,
	const color_t mask, const color_t maskAlt)
{
	char maskedName[256];
	GetMaskedName(maskedName, name, mask, maskAlt);
	// Check if the masked pic already exists
	if (PicManagerGetPic(pm, maskedName) != NULL) return;

	// Check if the original pic is available; if not then it's impossible to
	// create the masked version
	Pic *original = PicManagerGetPic(pm, name);
	CASSERT(original != NULL, "Cannot find original pic for masking\n");

	// Create the new pic by masking the original pic
	Pic p = PicCopy(original);
	debug(D_VERBOSE, "Creating new masked pic %s (%d x %d)\n",
		maskedName, p.size.x, p.size.y);
	for (int i = 0; i < p.size.x * p.size.y; i++)
	{
		color_t o = PIXEL2COLOR(original->Data[i]);
		color_t c;
		// Apply mask based on which channel each pixel is
		if (o.g == 0 && o.b == 0)
		{
			// Restore to white before masking
			o.g = o.r;
			o.b = o.r;
			c = ColorMult(o, maskAlt);
		}
		else
		{
			c = ColorMult(o, mask);
		}
		p.Data[i] = COLOR2PIXEL(c);
		// TODO: more channels
	}
	AddNamedPic(&pm->customPics, maskedName, &p);

	AfterAdd(pm);
}
void PicManagerGenerateMaskedStylePic(
	PicManager *pm, const char *name, const int style, const int type,
	const color_t mask, const color_t maskAlt)
{
	char buf[256];
	GetMaskedStyleName(buf, name, style, type);
	PicManagerGenerateMaskedPic(pm, buf, mask, maskAlt);
}

static void GetMaskedName(
	char *buf, const char *name, const color_t mask, const color_t maskAlt)
{
	char maskName[8];
	ColorStr(maskName, mask);
	char maskAltName[8];
	ColorStr(maskAltName, maskAlt);
	sprintf(buf, "%s_%s_%s", name, maskName, maskAltName);
}
static void GetMaskedStyleName(
	char *buf, const char *name, const int style, const int type)
{
	const char *styleName;
	const char *typeName;
	if (strcmp(name, "wall") == 0)
	{
		styleName = WallStyleStr(style);
		typeName = WallTypeStr(type);
	}
	else if (strcmp(name, "floor") == 0)
	{
		styleName = FloorStyleStr(style);
		typeName = FloorTypeStr(type);
	}
	else if (strcmp(name, "room") == 0)
	{
		styleName = RoomStyleStr(style);
		typeName = RoomTypeStr(type);
	}
	else
	{
		CASSERT(false, "Invalid masked style name");
		return;
	}
	sprintf(buf, "%s/%s_%s", name, styleName, typeName);
}

static NamedPic *AddNamedPic(CArray *pics, const char *name, const Pic *p)
{
	NamedPic n;
	if (p != NULL) n.pic = *p;
	CSTRDUP(n.name, name);
	CArrayPushBack(pics, &n);
	return CArrayGet(pics, pics->size - 1);
}

NamedPic *PicManagerGetRandomDrain(PicManager *pm)
{
	NamedPic **p = CArrayGet(&pm->drainPics, rand() % pm->drainPics.size);
	return *p;
}

int PicManagerGetDoorStyleIndex(PicManager *pm, const char *style)
{
	int idx = 0;
	CA_FOREACH(const char *, doorStyleName, pm->doorStyleNames)
		if (strcmp(style, *doorStyleName) == 0)
		{
			break;
		}
		idx++;
	CA_FOREACH_END()
	return idx;
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
