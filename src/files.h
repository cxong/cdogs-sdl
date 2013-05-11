/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (C) 1995 Ronny Wester
    Copyright (C) 2003 Jeremy Chin
    Copyright (C) 2003-2007 Lucas Martin-King

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    This file incorporates work covered by the following copyright and
    permission notice:

    Copyright (c) 2013, Cong Xu
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
#ifndef __FILES
#define __FILES

#include <SDL_endian.h>

#include "sys_config.h"
#include "gamedata.h"
#include "sys_specifics.h"

#define CAMPAIGN_OK                0
#define CAMPAIGN_BADFILE          -1
#define CAMPAIGN_VERSIONMISMATCH  -2
#define CAMPAIGN_BADPATH          -3

int ScanCampaign(const char *filename, char *title, int *missions);
int LoadCampaign(
	const char *filename, CampaignSetting *setting,
	int max_missions, int max_characters);
int SaveCampaign(const char *filename, CampaignSetting *setting);
void SaveCampaignAsC(
	const char *filename, const char *name,
	CampaignSetting *setting);

const char *GetHomeDirectory(void);
const char *GetConfigFilePath(const char *name);
char *GetDataFilePath(const char *path);

char * GetPWD(void);
void SetupConfigDir(void);

ssize_t f_read(FILE *f, void *buf, size_t size);
#define f_read8(f, b, s)	f_read(f, b, 1)
ssize_t f_read32(FILE *f, void *buf, size_t size);
ssize_t f_read16(FILE *f, void *buf, size_t size);

void swap32 (void *d);
void swap16 (void *d);

#endif
