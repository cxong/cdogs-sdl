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
} PicManager;

extern PicManager gPicManager;

bool PicManagerTryInit(
	PicManager *pm, const char *oldGfxFile1, const char *oldGfxFile2);
void PicManagerLoadDir(PicManager *pm, const char *path);
void PicManagerAdd(
	CArray *pics, CArray *sprites, const char *name, SDL_Surface *image);
void PicManagerClear(CArray *pics, CArray *sprites);
void PicManagerTerminate(PicManager *pm);

PicPaletted *PicManagerGetOldPic(PicManager *pm, int idx);
Pic *PicManagerGetFromOld(PicManager *pm, int idx);
Pic *PicManagerGetPic(const PicManager *pm, const char *name);
Pic *PicManagerGet(PicManager *pm, const char *name, const int oldIdx);
const NamedSprites *PicManagerGetSprites(
	const PicManager *pm, const char *name);


// Conversion
Pic PicFromTOffsetPic(PicManager *pm, TOffsetPic op);
