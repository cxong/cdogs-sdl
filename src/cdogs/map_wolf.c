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

static const char *GetTitle(const CWMapType type)
{
	switch (type)
	{
	case CWMAPTYPE_WL1:
		return "Wolfenstein 2D (shareware)";
	case CWMAPTYPE_WL6:
		return "Wolfenstein 2D";
	case CWMAPTYPE_SOD:
		return "Spear of Destiny";
	default:
		CASSERT(false, "unknown map type");
		return "Unknown";
	}
}

int MapWolfScan(const char *filename, char **title, int *numMissions)
{
	CWolfMap map;
	if (CWLoad(&map, filename) != 0)
	{
		return -1;
	}
	CSTRDUP(*title, GetTitle(map.type));
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

	LoadCampaign(&map, c);

	return 0;
}
static const char *GetDescription(const CWMapType type);
static void LoadCampaign(const CWolfMap *map, CampaignSetting *c)
{
	CFREE(c->Title);
	CSTRDUP(c->Title, GetTitle(map->type));
	CFREE(c->Author);
	CSTRDUP(c->Author, "id Software");
	CFREE(c->Description);
	CSTRDUP(c->Description, GetDescription(map->type));
	c->Ammo = true;
	c->WeaponPersist = true;
	c->SkipWeaponMenu = true;
}
static const char *GetDescription(const CWMapType type)
{
	switch (type)
	{
	case CWMAPTYPE_WL1: // fallthrough
	case CWMAPTYPE_WL6:
		return "You're William J. \"B.J.\" Blazkowicz, the Allies' bad boy of "
			   "espionage and a terminal action seeker. Your mission was to "
			   "infiltrate the Nazi fortress Castle Hollehammer and find the "
			   "plans for Operation Eisenfaust, the Nazi's blueprint for "
			   "building the perfect army. Rumors are that deep within the "
			   "castle the diabolical Dr. Schabbs has perfected a technique "
			   "for building a fierce army from the bodies of the dead. It's "
			   "so far removed from reality that it would seem silly if it "
			   "wasn't so sick. But what if it were true?";
	case CWMAPTYPE_SOD:
		return "It's World War II and you are B.J. Blazkowicz, the Allies' "
			   "most valuable agent. In the midst of the German Blitzkrieg, "
			   "the Spear that pierced the side of Christ is taken from "
			   "Versailles by the Nazis and secured in the impregnable Castle "
			   "Wolfenstein. According to legend, no man can be defeated when "
			   "he has the Spear. Hitler believes himself to be invincible "
			   "with the power of the Spear as his brutal army sweeps across "
			   "Europe.\n\nYour mission is to infiltrate the heavily guarded "
			   "Nazi stronghold and recapture the Spear from an already "
			   "unbalanced Hitler. The loss of his most coveted weapon could "
			   "push him over the edge. It could also get you ripped to "
			   "pieces.";
	default:
		CASSERT(false, "unknown map type");
		return "Unknown";
	}
}
