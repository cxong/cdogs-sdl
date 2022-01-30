/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.
	Copyright (c) 2018-2022 Cong Xu
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
#pragma once

#include <stdbool.h>

#include "c_hashmap/hashmap.h"
#include "pic_manager.h"

#define TILE_WIDTH      16
#define TILE_HEIGHT     12
#define TILE_SIZE		svec2i(TILE_WIDTH, TILE_HEIGHT)

#define X_TILES			((gGraphicsDevice.cachedConfig.Res.x + TILE_WIDTH - 1) / TILE_WIDTH + 1)

#define X_TILES_HALF    ((X_TILES + 1) / 2)

// + 1 because walls from bottom row show up one row above
#define Y_TILES			((gGraphicsDevice.cachedConfig.Res.y + TILE_HEIGHT - 1) / TILE_HEIGHT + 2)
#define Y_TILES_HALF    ((Y_TILES + 1 / 2)

typedef enum
{
	TILE_CLASS_FLOOR,
	TILE_CLASS_WALL,
	TILE_CLASS_DOOR,
	TILE_CLASS_NOTHING,
	TILE_CLASS_COUNT,
} TileClassType;

const char *TileClassTypeStr(const TileClassType t);
TileClassType StrTileClassType(const char *s);

typedef struct
{
	char *Name;
	const Pic *Pic;
	char *Style;
	char *StyleType;
	color_t Mask;
	color_t MaskAlt;
	bool canWalk;	// can walk on tile
	bool isOpaque;	// cannot see through
	bool shootable;	// blocks bullets
	bool IsRoom;	// affects random placement of indoor/outdoor map objects
	TileClassType Type;
	char *DamageBullet;
} TileClass;

extern TileClass gTileFloor;
extern TileClass gTileRoom;
extern TileClass gTileWall;
extern TileClass gTileNothing;
extern TileClass gTileExit;
extern TileClass gTileDoor;

map_t TileClassesNew(void);
void TileClassesTerminate(map_t c);
void TileClassDestroy(any_t data);
void TileClassTerminate(TileClass *tc);
void TileClassLoadJSON(TileClass *tc, json_t *node);
json_t *TileClassSaveJSON(const TileClass *tc);

void TileClassInit(
	TileClass *t, PicManager *pm, const TileClass *base,
	const char *style, const char *type,
	const color_t mask, const color_t maskAlt);
void TileClassInitDefault(
	TileClass *t, PicManager *pm, const TileClass *base,
	const char *forceStyle, const color_t *forceMask);
void TileClassReloadPic(TileClass *t, PicManager *pm);
const char *TileClassBaseStyleType(const TileClassType type);
void TileClassCopy(TileClass *dst, const TileClass *src);
const TileClass *StrTileClass(map_t c, const char *name);
const TileClass *TileClassesGetMaskedTile(
	map_t c, const TileClass *baseClass, const char *style, const char *type,
	const color_t mask, const color_t maskAlt);
TileClass *TileClassesAdd(
	map_t c, PicManager *pm, const TileClass *baseClass,
	const char *style, const char *type,
	const color_t mask, const color_t maskAlt);
const Pic *TileClassGetPic(const PicManager *pm, const TileClass *tc);
void TileClassGetName(
	char *buf, const TileClass *base, const char *style, const char *type,
	const color_t mask, const color_t maskAlt);
void TileClassGetBaseName(char *buf, const TileClass *tc);
const TileClass *TileClassesGetExit(
	map_t c, PicManager *pm, const char *style, const bool isShadow);
