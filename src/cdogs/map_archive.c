/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.
	Copyright (c) 2014-2021 Cong Xu
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
#include "map_archive.h"

#include <tinydir/tinydir.h>

#include "ammo.h"
#include "character_class.h"
#include "files.h"
#include "json_utils.h"
#include "log.h"
#include "map_new.h"
#include "pickup.h"
#include "player_template.h"

static char *ReadFileIntoBuf(const char *path, const char *mode, long *len);

static json_t *ReadArchiveJSON(const char *archive, const char *filename);
int MapNewScanArchive(const char *filename, char **title, int *numMissions)
{
	int err = 0;
	json_t *root = ReadArchiveJSON(filename, "campaign.json");
	if (root == NULL)
	{
		err = -1;
		goto bail;
	}
	err = MapNewScanJSON(root, title, numMissions);
	if (err < 0)
	{
		goto bail;
	}

bail:
	json_free_value(&root);
	return err;
}

int MapLoadCampaignJSON(const char *filename, CampaignSetting *c, int *version)
{
	int err = 0;
	json_t *root = ReadArchiveJSON(filename, "campaign.json");
	if (root == NULL)
	{
		err = -1;
		goto bail;
	}
	if (version)
	{
		LoadInt(version, root, "Version");
		if (*version > MAP_VERSION || *version <= 2)
		{
			err = -1;
			goto bail;
		}
	}
	MapNewLoadCampaignJSON(root, c);
	
bail:
	json_free_value(&root);
	return err;
}

static void LoadArchiveSounds(
	SoundDevice *device, const char *archive, const char *dirname);
static void LoadArchivePics(PicManager *pm, map_t cc, const char *archive);
int MapNewLoadArchive(const char *filename, CampaignSetting *c)
{
	LOG(LM_MAP, LL_DEBUG, "Loading archive map %s", filename);
	int err = 0;
	json_t *root = NULL;
	int version = 0;
	err = MapLoadCampaignJSON(filename, c, &version);
	if (err != 0)
	{
		goto bail;
	}

	// Load any custom data
	LoadArchiveSounds(&gSoundDevice, filename, "sounds");

	LoadArchivePics(&gPicManager, gCharSpriteClasses.customClasses, filename);

	root = ReadArchiveJSON(filename, "particles.json");
	if (root != NULL)
	{
		ParticleClassesLoadJSON(&gParticleClasses.CustomClasses, root);
	}

	root = ReadArchiveJSON(filename, "character_classes.json");
	if (root != NULL)
	{
		CharacterClassesLoadJSON(&gCharacterClasses.CustomClasses, root);
	}

	root = ReadArchiveJSON(filename, "bullets.json");
	if (root != NULL)
	{
		BulletLoadJSON(&gBulletClasses, &gBulletClasses.CustomClasses, root);
	}

	bool hasCustomAmmo = false;
	root = ReadArchiveJSON(filename, "ammo.json");
	if (root != NULL)
	{
		AmmoLoadJSON(&gAmmo.CustomAmmo, root);
		json_free_value(&root);
		hasCustomAmmo = true;
	}

	bool hasCustomGuns = false;
	root = ReadArchiveJSON(filename, "guns.json");
	if (root != NULL)
	{
		WeaponClassesLoadJSON(
			&gWeaponClasses, &gWeaponClasses.CustomGuns, root);
		json_free_value(&root);
		hasCustomGuns = true;
	}

	BulletLoadWeapons(&gBulletClasses);

	root = ReadArchiveJSON(filename, "pickups.json");
	if (root != NULL)
	{
		PickupClassesLoadJSON(&gPickupClasses.CustomClasses, root);
	}
	if (hasCustomAmmo)
	{
		PickupClassesLoadAmmo(
			&gPickupClasses.CustomClasses, &gAmmo.CustomAmmo);
	}
	if (hasCustomGuns)
	{
		PickupClassesLoadGuns(
			&gPickupClasses.CustomClasses, &gWeaponClasses.CustomGuns);
	}
	PickupClassesLoadKeys(&gPickupClasses.KeyClasses);

	root = ReadArchiveJSON(filename, "map_objects.json");
	if (root != NULL)
	{
		MapObjectsLoadJSON(&gMapObjects.CustomClasses, root);
	}
	MapObjectsLoadAmmoAndGunSpawners(
		&gMapObjects, &gAmmo, &gWeaponClasses, true);

	root = ReadArchiveJSON(filename, "missions.json");
	if (root == NULL)
	{
		err = -1;
		goto bail;
	}
	LoadMissions(
		&c->Missions, json_find_first_label(root, "Missions")->child, version);
	json_free_value(&root);

	// Note: some campaigns don't have characters (e.g. dogfights)
	root = ReadArchiveJSON(filename, "characters.json");
	if (root != NULL)
	{
		CharacterLoadJSON(
			&c->characters, &gPlayerTemplates.CustomClasses, root, version);
	}

bail:
	json_free_value(&root);
	return err;
}

static json_t *ReadArchiveJSON(const char *archive, const char *filename)
{
	json_t *root = NULL;
	char path[CDOGS_PATH_MAX];
	sprintf(path, "%s/%s", archive, filename);
	long len;
	char *buf = ReadFileIntoBuf(path, "rb", &len);
	if (buf == NULL)
		goto bail;
	const enum json_error e = json_parse_document(&root, buf);
	if (e != JSON_OK)
	{
		LOG(LM_MAP, LL_ERROR, "Invalid syntax in JSON file (%s) error(%d)",
			filename, (int)e);
		root = NULL;
		goto bail;
	}

bail:
	CFREE(buf);
	return root;
}

static void LoadArchiveSounds(
	SoundDevice *device, const char *archive, const char *dirname)
{
	char path[CDOGS_PATH_MAX];
	sprintf(path, "%s/%s", archive, dirname);
	SoundLoadDir(device->customSounds, path, NULL);
}
static void LoadArchivePics(PicManager *pm, map_t cc, const char *archive)
{
	char path[CDOGS_PATH_MAX];
	sprintf(path, "%s/graphics", archive);
	PicManagerLoadDir(pm, path, NULL, pm->customPics, pm->customSprites);
	CharSpriteClassesLoadDir(cc, archive);
}

static char *ReadFileIntoBuf(const char *path, const char *mode, long *len)
{
	char *buf = NULL;
	FILE *f = fopen(path, mode);
	if (f == NULL)
	{
		goto bail;
	}

	// Read into buffer
	if (fseek(f, 0L, SEEK_END) != 0)
	{
		goto bail;
	}
	*len = ftell(f);
	if (*len == -1)
	{
		goto bail;
	}
	CCALLOC(buf, *len + 1);
	if (fseek(f, 0L, SEEK_SET) != 0)
	{
		goto bail;
	}
	if (fread(buf, 1, *len, f) == 0)
	{
		goto bail;
	}

	goto end;

bail:
	CFREE(buf);
	buf = NULL;

end:
	if (f != NULL && fclose(f) != 0)
	{
		LOG(LM_MAP, LL_ERROR, "Cannot close file %s: %s", path,
			strerror(errno));
	}
	return buf;
}

static json_t *SaveMissions(CArray *a);
int MapArchiveSave(const char *filename, CampaignSetting *c)
{
	int res = 1;
	json_t *root = NULL;

	char relbuf[CDOGS_PATH_MAX];
	if (strcmp(StrGetFileExt(filename), "cdogscpn") == 0 ||
		strcmp(StrGetFileExt(filename), "CDOGSCPN") == 0)
	{
		strcpy(relbuf, filename);
	}
	else
	{
		sprintf(relbuf, "%s.cdogscpn", filename);
	}
	char buf[CDOGS_PATH_MAX];
	RealPath(relbuf, buf);
	// Make dir but ignore error, as we may be saving over an existing dir
	mkdir_deep(buf);

	// Campaign
	root = json_new_object();
	AddIntPair(root, "Version", MAP_VERSION);
	AddStringPair(root, "Title", c->Title);
	AddStringPair(root, "Author", c->Author);
	AddStringPair(root, "Description", c->Description);
	AddBoolPair(root, "Ammo", c->Ammo);
	AddBoolPair(root, "SkipWeaponMenu", c->SkipWeaponMenu);
	AddBoolPair(root, "RandomPickups", c->RandomPickups);
	AddIntPair(root, "DoorOpenTicks", c->DoorOpenTicks);
	AddIntPair(root, "Missions", (int)c->Missions.size);
	char buf2[CDOGS_PATH_MAX];
	sprintf(buf2, "%s/campaign.json", buf);
	if (!TrySaveJSONFile(root, buf2))
	{
		res = 0;
		goto bail;
	}

	json_free_value(&root);
	root = json_new_object();
	json_insert_pair_into_object(root, "Missions", SaveMissions(&c->Missions));
	sprintf(buf2, "%s/missions.json", buf);
	if (!TrySaveJSONFile(root, buf2))
	{
		res = 0;
		goto bail;
	}

	if (!CharacterSave(&c->characters, buf))
	{
		res = 0;
		goto bail;
	}

bail:
	json_free_value(&root);
	return res;
}

static json_t *SaveObjectives(CArray *a);
static json_t *SaveWeapons(const CArray *weapons);
static json_t *SaveMissionTileClasses(const MissionTileClasses *mtc);
static json_t *SaveRooms(const RoomParams r);
static json_t *SaveDoors(const DoorParams d);
static json_t *SavePillars(const PillarParams p);
static json_t *SaveMissions(CArray *a)
{
	json_t *missionsNode = json_new_array();
	for (int i = 0; i < (int)a->size; i++)
	{
		json_t *node = json_new_object();
		Mission *mission = CArrayGet(a, i);
		AddStringPair(node, "Title", mission->Title);
		AddStringPair(node, "Description", mission->Description);
		AddStringPair(node, "Type", MapTypeStr(mission->Type));
		AddIntPair(node, "Width", mission->Size.x);
		AddIntPair(node, "Height", mission->Size.y);

		AddStringPair(node, "ExitStyle", mission->ExitStyle);
		AddStringPair(node, "KeyStyle", mission->KeyStyle);

		json_insert_pair_into_object(
			node, "Objectives", SaveObjectives(&mission->Objectives));
		AddIntArray(node, "Enemies", &mission->Enemies);
		AddIntArray(node, "SpecialChars", &mission->SpecialChars);
		json_t *modsNode = json_new_array();
		for (int j = 0; j < (int)mission->MapObjectDensities.size; j++)
		{
			const MapObjectDensity *mod =
				CArrayGet(&mission->MapObjectDensities, j);
			json_t *modNode = json_new_object();
			AddStringPair(modNode, "MapObject", mod->M->Name);
			AddIntPair(modNode, "Density", mod->Density);
			json_insert_child(modsNode, modNode);
		}
		json_insert_pair_into_object(node, "MapObjectDensities", modsNode);

		AddIntPair(node, "EnemyDensity", mission->EnemyDensity);
		json_insert_pair_into_object(
			node, "Weapons", SaveWeapons(&mission->Weapons));
		AddBoolPair(node, "WeaponPersist", mission->WeaponPersist);
		AddBoolPair(node, "SkipDebrief", mission->SkipDebrief);

		if (mission->Music.Type == MUSIC_SRC_DYNAMIC &&
			mission->Music.Data.Filename &&
			strlen(mission->Music.Data.Filename) > 0)
		{
			json_insert_pair_into_object(
				node, "Song", json_new_string(mission->Music.Data.Filename));
		}

		switch (mission->Type)
		{
		case MAPTYPE_CLASSIC:
			json_insert_pair_into_object(
				node, "TileClasses",
				SaveMissionTileClasses(&mission->u.Classic.TileClasses));
			AddIntPair(node, "Walls", mission->u.Classic.Walls);
			AddIntPair(node, "WallLength", mission->u.Classic.WallLength);
			AddIntPair(
				node, "CorridorWidth", mission->u.Classic.CorridorWidth);
			json_insert_pair_into_object(
				node, "Rooms", SaveRooms(mission->u.Classic.Rooms));
			AddIntPair(node, "Squares", mission->u.Classic.Squares);
			AddBoolPair(node, "ExitEnabled", mission->u.Classic.ExitEnabled);
			json_insert_pair_into_object(
				node, "Doors", SaveDoors(mission->u.Classic.Doors));
			json_insert_pair_into_object(
				node, "Pillars", SavePillars(mission->u.Classic.Pillars));
			break;
		case MAPTYPE_STATIC:
			MissionStaticSaveJSON(&mission->u.Static, mission->Size, node);
			break;
		case MAPTYPE_CAVE:
			json_insert_pair_into_object(
				node, "TileClasses",
				SaveMissionTileClasses(&mission->u.Cave.TileClasses));
			AddIntPair(node, "FillPercent", mission->u.Cave.FillPercent);
			AddIntPair(node, "Repeat", mission->u.Cave.Repeat);
			AddIntPair(node, "R1", mission->u.Cave.R1);
			AddIntPair(node, "R2", mission->u.Cave.R2);
			json_insert_pair_into_object(
				node, "Rooms", SaveRooms(mission->u.Cave.Rooms));
			AddIntPair(node, "Squares", mission->u.Cave.Squares);
			AddBoolPair(node, "ExitEnabled", mission->u.Cave.ExitEnabled);
			AddBoolPair(node, "DoorsEnabled", mission->u.Cave.DoorsEnabled);
			break;
		case MAPTYPE_INTERIOR:
			json_insert_pair_into_object(
				node, "TileClasses",
				SaveMissionTileClasses(&mission->u.Interior.TileClasses));
			AddIntPair(
				node, "CorridorWidth", mission->u.Interior.CorridorWidth);
			json_insert_pair_into_object(
				node, "Rooms", SaveRooms(mission->u.Interior.Rooms));
			AddBoolPair(node, "ExitEnabled", mission->u.Interior.ExitEnabled);
			json_insert_pair_into_object(
				node, "Doors", SaveDoors(mission->u.Interior.Doors));
			json_insert_pair_into_object(
				node, "Pillars", SavePillars(mission->u.Interior.Pillars));
			break;
		default:
			CASSERT(false, "unknown map type");
			break;
		}

		json_insert_child(missionsNode, node);
	}
	return missionsNode;
}
static json_t *SaveRooms(const RoomParams r)
{
	json_t *node = json_new_object();
	AddIntPair(node, "Count", r.Count);
	AddIntPair(node, "Min", r.Min);
	AddIntPair(node, "Max", r.Max);
	AddBoolPair(node, "Edge", r.Edge);
	AddBoolPair(node, "Overlap", r.Overlap);
	AddIntPair(node, "Walls", r.Walls);
	AddIntPair(node, "WallLength", r.WallLength);
	AddIntPair(node, "WallPad", r.WallPad);
	return node;
}
static json_t *SaveWeapons(const CArray *weapons)
{
	json_t *node = json_new_array();
	for (int i = 0; i < (int)weapons->size; i++)
	{
		const WeaponClass **wc = CArrayGet(weapons, i);
		json_insert_child(node, json_new_string((*wc)->name));
	}
	return node;
}
static json_t *SaveMissionTileClasses(const MissionTileClasses *mtc)
{
	json_t *node = json_new_object();
	json_insert_pair_into_object(
		node, "Wall", TileClassSaveJSON(&mtc->Wall));
	json_insert_pair_into_object(
		node, "Floor", TileClassSaveJSON(&mtc->Floor));
	json_insert_pair_into_object(
		node, "Room", TileClassSaveJSON(&mtc->Room));
	json_insert_pair_into_object(
		node, "Door", TileClassSaveJSON(&mtc->Door));
	return node;
}
static json_t *SavePillars(const PillarParams p)
{
	json_t *node = json_new_object();
	AddIntPair(node, "Count", p.Count);
	AddIntPair(node, "Min", p.Min);
	AddIntPair(node, "Max", p.Max);
	return node;
}
static json_t *SaveDoors(const DoorParams d)
{
	json_t *node = json_new_object();
	AddBoolPair(node, "Enabled", d.Enabled);
	AddIntPair(node, "Min", d.Min);
	AddIntPair(node, "Max", d.Max);
	AddBoolPair(node, "RandomPos", d.RandomPos);
	return node;
}

static json_t *SaveObjectives(CArray *a)
{
	json_t *objectivesNode = json_new_array();
	CA_FOREACH(const Objective, o, *a)
	json_t *objNode = json_new_object();
	AddStringPair(objNode, "Description", o->Description);
	AddStringPair(objNode, "Type", ObjectiveTypeStr(o->Type));
	switch (o->Type)
	{
	case OBJECTIVE_COLLECT:
		AddStringPair(objNode, "Pickup", o->u.Pickup->Name);
		break;
	case OBJECTIVE_DESTROY:
		AddStringPair(objNode, "MapObject", o->u.MapObject->Name);
		break;
	default:
		AddIntPair(objNode, "Index", o->u.Index);
		break;
	}
	AddIntPair(objNode, "Count", o->Count);
	AddIntPair(objNode, "Required", o->Required);
	AddIntPair(objNode, "Flags", o->Flags);
	json_insert_child(objectivesNode, objNode);
	CA_FOREACH_END()
	return objectivesNode;
}
