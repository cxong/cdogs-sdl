/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.
	Copyright (c) 2020 Cong Xu
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
#include "map_wolf.h"

#include "cwolfmap/cwolfmap.h"
#include "map_archive.h"

static void GetCampaignPath(const CWMapType type, char *buf)
{
	switch (type)
	{
	case CWMAPTYPE_WL1:
		GetDataFilePath(buf, "missions/.wolf3d/WL1.cdogscpn");
	case CWMAPTYPE_WL6:
		GetDataFilePath(buf, "missions/.wolf3d/WL6.cdogscpn");
		break;
	case CWMAPTYPE_SOD:
		GetDataFilePath(buf, "missions/.wolf3d/SOD.cdogscpn");
		break;
	default:
		CASSERT(false, "unknown map type");
		break;
	}
}

int MapWolfScan(const char *filename, char **title, int *numMissions)
{
	CWolfMap map;
	if (CWLoad(&map, filename) != 0)
	{
		return -1;
	}
	char buf[CDOGS_PATH_MAX];
	GetCampaignPath(map.type, buf);
	if (MapNewScanArchive(buf, title, NULL) != 0)
	{
		return -1;
	}
	*numMissions = map.nLevels;
	return 0;
}

static void LoadCampaign(const CWolfMap *map, CampaignSetting *c);
int MapWolfLoad(const char *filename, CampaignSetting *c)
{
	CWolfMap map;
	if (CWLoad(&map, filename) != 0)
	{
		return -1;
	}

	char buf[CDOGS_PATH_MAX];
	GetCampaignPath(map.type, buf);
	if (MapNewLoadArchive(buf, c) != 0)
	{
		return -1;
	}

	return 0;
}
