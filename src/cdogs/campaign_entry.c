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
#include "campaign_entry.h"

#include <stdio.h>

#include <cdogs/files.h>
#include <cdogs/map_new.h>
#include <cdogs/mission.h>
#include <cdogs/utils.h>


static bool IsCampaignOK(const char *path, char **buf, int *numMissions)
{
	return MapNewScan(path, buf, numMissions) == 0;
}

void CampaignEntryInit(CampaignEntry *entry, const char *title, GameMode mode)
{
	memset(entry, 0, sizeof *entry);
	CSTRDUP(entry->Info, title);
	entry->Mode = mode;
}
void CampaignEntryCopy(CampaignEntry *dst, CampaignEntry *src)
{
	memcpy(dst, src, sizeof *dst);
	if (src->Filename) CSTRDUP(dst->Filename, src->Filename);
	if (src->Path) CSTRDUP(dst->Path, src->Path);
	if (src->Info) CSTRDUP(dst->Info, src->Info);
}
bool CampaignEntryTryLoad(
	CampaignEntry *entry, const char *path, GameMode mode)
{
	char *buf;
	int numMissions;
	if (!IsCampaignOK(path, &buf, &numMissions))
	{
		return false;
	}
	// cap length of title
	size_t maxLen = 70;
	if (strlen(buf) > maxLen)
	{
		buf[maxLen] = '\0';
	}
	char title[256];
	sprintf(title, "%s (%d)", buf, numMissions);
	CampaignEntryInit(entry, title, mode);
	CSTRDUP(entry->Filename, PathGetBasename(path));
	CSTRDUP(entry->Path, path);
	entry->NumMissions = numMissions;
	CFREE(buf);
	return true;
}
void CampaignEntryTerminate(CampaignEntry *entry)
{
	CFREE(entry->Filename);
	CFREE(entry->Path);
	CFREE(entry->Info);
}
