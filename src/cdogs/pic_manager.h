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
#pragma once

#include "pic.h"
#include "pics.h"

typedef struct
{
	PicPaletted *oldPics[PIC_MAX];
	Pic picsFromOld[PIC_MAX];
	TPalette palette;
	CArray pics;	// of NamedPic
	CArray sprites;	// of NamedSprites
	CArray customPics;	// of NamedPic
	CArray customSprites;	// of NamedSprites

	CArray drainPics;	// of NamedPic *

	CArray doorStyleNames;	// of char *, for editor
} PicManager;

extern PicManager gPicManager;

bool PicManagerTryInit(
	PicManager *pm, const char *oldGfxFile1, const char *oldGfxFile2);
void PicManagerLoadDir(PicManager *pm, const char *path);
void PicManagerAdd(
	CArray *pics, CArray *sprites, const char *name, SDL_Surface *image);
void PicManagerClearCustom(PicManager *pm);
void PicManagerTerminate(PicManager *pm);

PicPaletted *PicManagerGetOldPic(PicManager *pm, int idx);
Pic *PicManagerGetFromOld(PicManager *pm, int idx);
// Note: return ptr to NamedPic so we can store that instead of the name
NamedPic *PicManagerGetNamedPic(const PicManager *pm, const char *name);
Pic *PicManagerGetPic(const PicManager *pm, const char *name);
Pic *PicManagerGet(PicManager *pm, const char *name, const int oldIdx);
const NamedSprites *PicManagerGetSprites(
	const PicManager *pm, const char *name);

// Get a masked pic for the styled tiles: walls, floors, rooms
// Simply calls GetMaskedPic but the name contains the relevant
// style/type names
NamedPic *PicManagerGetMaskedStylePic(
	const PicManager *pm, const char *name, const int style, const int type,
	const color_t mask, const color_t maskAlt);
// To support dynamic colours, generate pics on request.
void PicManagerGenerateMaskedPic(
	PicManager *pm, const char *name,
	const color_t mask, const color_t maskAlt);
void PicManagerGenerateMaskedStylePic(
	PicManager *pm, const char *name, const int style, const int type,
	const color_t mask, const color_t maskAlt);

NamedPic *PicManagerGetRandomDrain(PicManager *pm);
int PicManagerGetDoorStyleIndex(PicManager *pm, const char *style);

// Conversion
Pic PicFromTOffsetPic(PicManager *pm, TOffsetPic op);
