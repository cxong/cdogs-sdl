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

/*
C-Dogs (classic) file format:
Sizes in bytes unless otherwise stated

Priviledged types (types that are used directly in the file format)
Changing these types will break the game!
- CampaignSetting
- struct Mission
- TBadGuy
- struct MissionObjective

Campaign:
- CAMPAIGN_MAGIC (4)
- CAMPAIGN_VERSION (4)
- Title (40, char *)
- Author (40, char *)
- Description (200, char *)
- MissionCount (4)
- <Missions> (MissionCount * sizeof(struct Mission))
- CharacterCount (4)
- <Characters> (CharacterCount * sizeof(TBadGuy))
*/
#ifdef _MSC_VER
#pragma pack(push, 1)
#endif
typedef struct
{
	int32_t armedBodyPic;
	int32_t unarmedBodyPic;
	int32_t facePic;
	int32_t speed;
	int32_t probabilityToMove;
	int32_t probabilityToTrack;
	int32_t probabilityToShoot;
	int32_t actionDelay;
	int32_t gun;
	int32_t skinColor;
	int32_t armColor;
	int32_t bodyColor;
	int32_t legColor;
	int32_t hairColor;
	int32_t health;
	int32_t flags;
} TBadGuy
#ifdef _MSC_VER
;
#pragma pack(pop)
#else
__attribute__((packed));
#endif

// WARNING: data type used in C format (builtin campaigns)
typedef struct
{
	char title[40];
	char author[40];
	char description[200];
	int missionCount;
	struct Mission *missions;
	int characterCount;
	TBadGuy *characters;
} CampaignSetting;

void ConvertCampaignSetting(CampaignSettingNew *dest, CampaignSetting *src);

#define CAMPAIGN_OK                0
#define CAMPAIGN_BADFILE          -1
#define CAMPAIGN_VERSIONMISMATCH  -2
#define CAMPAIGN_BADPATH          -3

int ScanCampaign(const char *filename, char *title, int *missions);
int LoadCampaign(const char *filename, CampaignSettingNew *setting);
int SaveCampaign(const char *filename, CampaignSettingNew *setting);
void SaveCampaignAsC(
	const char *filename, const char *name,
	CampaignSettingNew *setting);

const char *GetHomeDirectory(void);
const char *GetConfigFilePath(const char *name);
char *GetDataFilePath(const char *path);

char * GetPWD(void);
void SetupConfigDir(void);

size_t f_read(FILE *f, void *buf, size_t size);
#define f_read8(f, b, s)	f_read(f, b, 1)
size_t f_read32(FILE *f, void *buf, size_t size);
size_t f_read16(FILE *f, void *buf, size_t size);

void swap32 (void *d);
void swap16 (void *d);

#endif
