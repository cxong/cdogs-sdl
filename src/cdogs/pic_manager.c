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

#include "files.h"

PicManager gPicManager;

int PicManagerTryInit(
	PicManager *pm, const char *oldGfxFile1, const char *oldGfxFile2)
{
	int i;
	memset(pm, 0, sizeof *pm);
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

void PicManagerGenerateOldPics(PicManager *pm, GraphicsDevice *g)
{
	int i;
	// Convert old pics into new format ones
	// TODO: this is wasteful; better to eliminate old pics altogether
	if (pm->IsLoaded)
	{
		return;
	}
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
	pm->IsLoaded = 1;
}

void PicManagerTerminate(PicManager *pm)
{
	int i;
	for (i = 0; i < PIC_MAX; i++)
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
}

PicPaletted *PicManagerGetOldPic(PicManager *pm, int idx)
{
	return pm->oldPics[idx];
}
Pic *PicManagerGetFromOld(PicManager *pm, int idx)
{
	return &pm->picsFromOld[idx];
}
