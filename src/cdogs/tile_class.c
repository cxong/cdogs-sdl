/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (c) 2018-2019 Cong Xu
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
#include "tile_class.h"

#include "log.h"


TileClasses gTileClasses;
TileClass gTileFloor = {
	"tile", NULL, true, false, false, false, true, false,
};
TileClass gTileWall = {
	"wall", NULL, false, true, true, true, false, false,
};
TileClass gTileNothing = {
	NULL, NULL, false, false, false, false, false, false,
};
TileClass gTileExit = {
	"tile", NULL, true, false, false, false, true, false,
};
TileClass gTileDoor = {
	"door", NULL, false, true, true, false, false, true,
};

void TileClassesInit(TileClasses *c)
{
	c->classes = hashmap_new();
	c->customClasses = hashmap_new();
}
void TileClassesClearCustom(TileClasses *c)
{
	TileClassesTerminate(c);
	TileClassesInit(c);
}
static void TileClassDestroy(any_t data);
void TileClassesTerminate(TileClasses *c)
{
	hashmap_destroy(c->classes, TileClassDestroy);
	hashmap_destroy(c->customClasses, TileClassDestroy);
}
static void TileClassDestroy(any_t data)
{
	TileClass *c = data;
	CFREE(c->Name);
	CFREE(c);
}

const TileClass *StrTileClass(const char *name)
{
	if (name == NULL || strlen(name) == 0)
	{
		return &gTileNothing;
	}
	TileClass *t;
	int error = hashmap_get(gTileClasses.customClasses, name, (any_t *)&t);
	if (error == MAP_OK)
	{
		return t;
	}
	error = hashmap_get(gTileClasses.classes, name, (any_t *)&t);
	if (error == MAP_OK)
	{
		return t;
	}
	LOG(LM_MAIN, LL_ERROR, "failed to get tile class %s: %d",
		name, error);
	return &gTileNothing;
}

static void GetMaskedName(
	char *buf, const char *name, const char *style, const char *type,
	const color_t mask, const color_t maskAlt);
const TileClass *TileClassesGetMaskedTile(
	const TileClass *baseClass, const char *style, const char *type,
	const color_t mask, const color_t maskAlt)
{
	char buf[256];
	GetMaskedName(buf, baseClass->Name, style, type, mask, maskAlt);
	return StrTileClass(buf);
}
void TileClassesAddMaskedTile(
	TileClasses *c, const PicManager *pm, const TileClass *baseClass,
	const char *style, const char *type,
	const color_t mask, const color_t maskAlt)
{
	char buf[256];
	GetMaskedName(buf, baseClass->Name, style, type, mask, maskAlt);
	TileClassAdd(c->customClasses, pm, baseClass, buf);
}
static void GetMaskedName(
	char *buf, const char *name, const char *style, const char *type,
	const color_t mask, const color_t maskAlt)
{
	char maskName[16];
	ColorStr(maskName, mask);
	char maskAltName[16];
	ColorStr(maskAltName, maskAlt);
	sprintf(buf, "%s/%s/%s/%s/%s", name, style, type, maskName, maskAltName);
}

const TileClass *TileClassesGetExit(
	TileClasses *c, const PicManager *pm,
	const char *style, const bool isShadow)
{
	char buf[256];
	sprintf(buf, "exits/%s/%s", style, isShadow ? "shadow" : "normal");
	const TileClass *t = StrTileClass(buf);
	if (t != &gTileNothing)
	{
		return t;
	}

	// tile class not found; create it
	return TileClassAdd(c->customClasses, pm, &gTileExit, buf);
}

TileClass *TileClassAdd(
	map_t classes, const PicManager *pm, const TileClass *base,
	const char *name)
{
	TileClass *t;
	CCALLOC(t, sizeof *t);
	if (base != NULL)
	{
		memcpy(t, base, sizeof *t);
	}
	CSTRDUP(t->Name, name);
	t->Pic = PicManagerGetPic(pm, name);

	const int error = hashmap_put(classes, name, t);
	if (error != MAP_OK)
	{
		LOG(LM_MAIN, LL_ERROR, "failed to add tile class %s: %d",
			name, error);
		return NULL;
	}
	return t;
}
