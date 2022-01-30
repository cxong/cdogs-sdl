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
#include "tile_class.h"

#include "door.h"
#include "log.h"
#include "pics.h"
#include "sys_config.h"


TileClass gTileFloor = {
	"tile", NULL, NULL, NULL, { 255, 255, 255, 255 }, { 255, 255, 255, 255 },
	true, false, false, false, TILE_CLASS_FLOOR, NULL,
};
TileClass gTileRoom = {
	"tile", NULL, NULL, NULL, { 255, 255, 255, 255 }, { 255, 255, 255, 255 },
	true, false, false, true, TILE_CLASS_FLOOR, NULL,
};
TileClass gTileWall = {
	"wall", NULL, NULL, NULL, { 255, 255, 255, 255 }, { 255, 255, 255, 255 },
	false, true, true, false, TILE_CLASS_WALL, NULL,
};
TileClass gTileNothing = {
	NULL, NULL, NULL, NULL, { 255, 255, 255, 255 }, { 255, 255, 255, 255 },
	false, false, false, false, TILE_CLASS_NOTHING, NULL,
};
TileClass gTileExit = {
	"exits", NULL, NULL, NULL, { 255, 255, 255, 255 }, { 255, 255, 255, 255 },
	true, false, false, false, TILE_CLASS_FLOOR, NULL,
};
TileClass gTileDoor = {
	"door", NULL, NULL, NULL, { 255, 255, 255, 255 }, { 255, 255, 255, 255 },
	false, true, true, true, TILE_CLASS_DOOR, NULL,
};

const char *TileClassTypeStr(const TileClassType t)
{
	switch (t)
	{
		T2S(TILE_CLASS_FLOOR, "Floor");
		T2S(TILE_CLASS_WALL, "Wall");
		T2S(TILE_CLASS_DOOR, "Door");
	default:
		return "";
	}
}
TileClassType StrTileClassType(const char *s)
{
	S2T(TILE_CLASS_FLOOR, "Floor");
	S2T(TILE_CLASS_WALL, "Wall");
	S2T(TILE_CLASS_DOOR, "Door");
	S2T(TILE_CLASS_NOTHING, "");
	CASSERT(false, "unknown tile class type");
	return TILE_CLASS_NOTHING;
}

map_t TileClassesNew(void)
{
	return hashmap_new();
}
void TileClassesTerminate(map_t c)
{
	hashmap_destroy(c, TileClassDestroy);
}
void TileClassDestroy(any_t data)
{
	TileClass *tc = data;
	TileClassTerminate(tc);
	CFREE(tc);
}
void TileClassTerminate(TileClass *tc)
{
	if (tc->Type == TILE_CLASS_NOTHING)
	{
		return;
	}
	CFREE(tc->Name);
	CFREE(tc->Style);
	CFREE(tc->StyleType);
	CFREE(tc->DamageBullet);
}

void TileClassLoadJSON(TileClass *tc, json_t *node)
{
	memset(tc, 0, sizeof *tc);
	LoadStr(&tc->Name, node, "Name");
	JSON_UTILS_LOAD_ENUM(tc->Type, node, "Type", StrTileClassType);
	LoadStr(&tc->Style, node, "Style");
	LoadColor(&tc->Mask, node, "Mask");
	LoadColor(&tc->MaskAlt, node, "MaskAlt");
	LoadBool(&tc->canWalk, node, "CanWalk");
	LoadBool(&tc->isOpaque, node, "IsOpaque");
	LoadBool(&tc->shootable, node, "Shootable");
	LoadBool(&tc->IsRoom, node, "IsRoom");
	LoadStr(&tc->DamageBullet, node, "DamageBullet");
}

json_t *TileClassSaveJSON(const TileClass *tc)
{
	json_t *itemNode = json_new_object();
	AddStringPair(itemNode, "Name", tc->Name);
	AddStringPair(itemNode, "Type", TileClassTypeStr(tc->Type));
	AddStringPair(itemNode, "Style", tc->Style);
	AddColorPair(itemNode, "Mask", tc->Mask);
	AddColorPair(itemNode, "MaskAlt", tc->MaskAlt);
	AddBoolPair(itemNode, "CanWalk", tc->canWalk);
	AddBoolPair(itemNode, "IsOpaque", tc->isOpaque);
	AddBoolPair(itemNode, "Shootable", tc->shootable);
	AddBoolPair(itemNode, "IsRoom", tc->IsRoom);
	AddStringPair(itemNode, "DamageBullet", tc->DamageBullet);
	return itemNode;
}

const char *TileClassBaseStyleType(const TileClassType type)
{
	switch (type)
	{
	case TILE_CLASS_FLOOR:
		return "normal";
	case TILE_CLASS_WALL:
		return "o";
	case TILE_CLASS_DOOR:
		return "normal_h";
	default:
		return "";
	}
}

void TileClassCopy(TileClass *dst, const TileClass *src)
{
	memcpy(dst, src, sizeof *dst);
	if (src->Name) CSTRDUP(dst->Name, src->Name);
	if (src->Style) CSTRDUP(dst->Style, src->Style);
	if (src->StyleType) CSTRDUP(dst->StyleType, TileClassBaseStyleType(src->Type));
	if (src->DamageBullet) CSTRDUP(dst->DamageBullet, src->DamageBullet);
}
const TileClass *StrTileClass(map_t c, const char *name)
{
	if (name == NULL || strlen(name) == 0)
	{
		return &gTileNothing;
	}
	LOG(LM_MAIN, LL_TRACE, "get tile class %s", name);
	TileClass *t;
	const int error = hashmap_get(c, name, (any_t *)&t);
	if (error == MAP_OK)
	{
		return t;
	}
	LOG(LM_MAIN, LL_ERROR, "failed to get tile class %s: %d",
		name, error);
	return &gTileNothing;
}

void TileClassInit(
	TileClass *t, PicManager *pm, const TileClass *base,
	const char *style, const char *type,
	const color_t mask, const color_t maskAlt)
{
	memcpy(t, base, sizeof *t);
	if (base->Name) CSTRDUP(t->Name, base->Name);
	if (style) CSTRDUP(t->Style, style);
	if (type && strlen(type))
	{
		CSTRDUP(t->StyleType, type);
	}
	if (base->DamageBullet) CSTRDUP(t->DamageBullet, base->DamageBullet);
	t->Mask = mask;
	t->MaskAlt = maskAlt;
	TileClassReloadPic(t, pm);
}
void TileClassInitDefault(
	TileClass *t, PicManager *pm, const TileClass *base,
	const char *forceStyle, const color_t *forceMask)
{
	const char *style = IntFloorStyle(0);
	color_t mask = colorBattleshipGrey;
	color_t maskAlt = colorOfficeGreen;
	switch (base->Type)
	{
	case TILE_CLASS_FLOOR:
		break;
	case TILE_CLASS_WALL:
		style = IntWallStyle(0);
		mask = colorGravel;
		break;
	case TILE_CLASS_DOOR:
		style = IntDoorStyle(0);
		mask = colorWhite;
		break;
	case TILE_CLASS_NOTHING:
		break;
	default:
		CASSERT(false, "unknown tile class");
		break;
	}
	if (forceStyle != NULL)
	{
		style = forceStyle;
	}
	if (forceMask != NULL)
	{
		mask = *forceMask;
	}
	TileClassInit(
		t, pm, base, style, TileClassBaseStyleType(base->Type),
		mask, maskAlt);
}
void TileClassReloadPic(TileClass *t, PicManager *pm)
{
	if (t->Name != NULL && strlen(t->Name) != 0)
	{
		// Generate the pic in case it doesn't exist
		PicManagerGenerateMaskedStylePic(
			pm, t->Name, t->Style, t->StyleType, t->Mask, t->MaskAlt, false);
		t->Pic = TileClassGetPic(pm, t);
		CASSERT(t->Pic != NULL, "cannot find tile pic");
	}
}
const TileClass *TileClassesGetMaskedTile(
	map_t c, const TileClass *baseClass, const char *style, const char *type,
	const color_t mask, const color_t maskAlt)
{
	char buf[256];
	TileClassGetName(buf, baseClass, style, type, mask, maskAlt);
	return StrTileClass(c, buf);
}
TileClass *TileClassesAdd(
	map_t c, PicManager *pm, const TileClass *baseClass,
	const char *style, const char *type,
	const color_t mask, const color_t maskAlt)
{
	TileClass *t;
	CMALLOC(t, sizeof *t);
	TileClassInit(t, pm, baseClass, style, type, mask, maskAlt);

	char buf[CDOGS_PATH_MAX];
	TileClassGetName(buf, t, style, type, mask, maskAlt);
	const int error = hashmap_put(c, buf, t);
	if (error != MAP_OK)
	{
		LOG(LM_MAIN, LL_ERROR, "failed to add tile class %s: %d", buf, error);
		TileClassDestroy(t);
		return NULL;
	}
	LOG(LM_MAIN, LL_DEBUG, "add tile class %s", buf);
	return t;
}
void TileClassGetName(
	char *buf, const TileClass *base, const char *style, const char *type,
	const color_t mask, const color_t maskAlt)
{
	char maskName[COLOR_STR_BUF];
	ColorStr(maskName, mask);
	char maskAltName[COLOR_STR_BUF];
	ColorStr(maskAltName, maskAlt);
	sprintf(
		buf, "%s%s/%s/%s/%s/%s",
		TileClassTypeStr(base->Type), base->IsRoom ? "room" : "",
		style, type, maskName, maskAltName);
}
void TileClassGetBaseName(char *buf, const TileClass *tc)
{
	TileClassGetName(
		buf, tc, tc->Style, TileClassBaseStyleType(tc->Type),
		tc->Mask, tc->MaskAlt);
}
const Pic *TileClassGetPic(const PicManager *pm, const TileClass *tc)
{
	char buf[CDOGS_PATH_MAX];
	char maskName[COLOR_STR_BUF];
	ColorStr(maskName, tc->Mask);
	char maskAltName[COLOR_STR_BUF];
	ColorStr(maskAltName, tc->MaskAlt);
	sprintf(
		buf, "%s/%s/%s/%s/%s",
		tc->Name, tc->Style, tc->StyleType, maskName, maskAltName);
	const Pic *pic = PicManagerGetPic(pm, buf);
	return pic;
}

const TileClass *TileClassesGetExit(
	map_t c, PicManager *pm, const char *style, const bool isShadow)
{
	char buf[256];
	const char *type = isShadow ? "shadow" : "normal";
	TileClassGetName(buf, &gTileExit, style, type, colorWhite, colorWhite);
	const TileClass *t = StrTileClass(c, buf);
	if (t != &gTileNothing)
	{
		return t;
	}

	// tile class not found; create it
	PicManagerGenerateMaskedStylePic(
		pm, "exits", style, type, colorWhite, colorWhite, true);
	return TileClassesAdd(
		c, pm, &gTileExit, style, type, colorWhite, colorWhite);
}
