/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (c) 2013-2016, Cong Xu
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

#include "c_hashmap/hashmap.h"
#include "cpic.h"
#include "pics.h"

typedef struct
{
	map_t pics;	// of NamedPic
	map_t sprites;	// of NamedSprites
	map_t customPics;	// of NamedPic
	map_t customSprites;	// of NamedSprites

	CArray wallStyleNames;	// of char *
	CArray tileStyleNames;	// of char *
	CArray exitStyleNames;	// of char *
	CArray doorStyleNames;	// of char *
	CArray keyStyleNames;	// of char *
} PicManager;

extern PicManager gPicManager;

void PicManagerInit(PicManager *pm);
void PicManagerLoad(PicManager *pm, const char *path);
void PicManagerLoadDir(
	PicManager *pm, const char *path, const char *prefix,
	map_t pics, map_t sprites);
void PicManagerClearCustom(PicManager *pm);
void PicManagerTerminate(PicManager *pm);

// Note: return ptr to NamedPic so we can store that instead of the name
NamedPic *PicManagerGetNamedPic(const PicManager *pm, const char *name);
Pic *PicManagerGetPic(const PicManager *pm, const char *name);
const NamedSprites *PicManagerGetSprites(
	const PicManager *pm, const char *name);

// Get a masked pic for the styled tiles: walls, floors, rooms
// Simply calls GetMaskedPic but the name contains the relevant
// style/type names
NamedPic *PicManagerGetMaskedStylePic(
	const PicManager *pm,
	const char *name, const char *style, const char *type,
	const color_t mask, const color_t maskAlt);
// To support dynamic colours, generate pics on request.
void PicManagerGenerateMaskedPic(
	PicManager *pm, const char *name,
	const color_t mask, const color_t maskAlt);
void PicManagerGenerateMaskedStylePic(
	PicManager *pm, const char *name, const char *style, const char *type,
	const color_t mask, const color_t maskAlt);

NamedPic *PicManagerGetExitPic(
	PicManager *pm, const char *style, const bool isShadow);
int PicManagerGetWallStyleIndex(PicManager *pm, const char *style);
int PicManagerGetTileStyleIndex(PicManager *pm, const char *style);
int PicManagerGetExitStyleIndex(PicManager *pm, const char *style);
int PicManagerGetDoorStyleIndex(PicManager *pm, const char *style);
int PicManagerGetKeyStyleIndex(PicManager *pm, const char *style);
