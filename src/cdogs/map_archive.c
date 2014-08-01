/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (c) 2014, Cong Xu
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

#include <locale.h>

#include <SDL_image.h>

#include "json_utils.h"
#include "map_new.h"
#include "physfs/physfs.h"

#define VERSION 3


static json_t *ReadPhysFSJSON(const char *archive, const char *filename);
int MapNewScanArchive(
	const char *filename, char **title, int *numMissions)
{
	int err = 0;
	json_t *root = ReadPhysFSJSON(filename, "campaign.json");
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

static void LoadArchiveSounds(
	SoundDevice *device, const char *archive, const char *dirname);
static void LoadArchivePics(
	PicManager *pm, const char *archive, const char *dirname);
int MapNewLoadArchive(const char *filename, CampaignSetting *c)
{
	int err = 0;
	json_t *root = ReadPhysFSJSON(filename, "campaign.json");
	if (root == NULL)
	{
		err = -1;
		goto bail;
	}
	int version;
	LoadInt(&version, root, "Version");
	if (version > VERSION || version <= 2)
	{
		err = -1;
		goto bail;
	}
	MapNewLoadCampaignJSON(root, c);
	json_free_value(&root);


	// Unload previous custom data
	SoundClear(&gSoundDevice.customSounds);
	PicManagerClear(&gPicManager.customPics, &gPicManager.customSprites);
	ParticleClassesClear(&gParticleClasses.CustomClasses);
	BulletClassesClear(&gBulletClasses.CustomClasses);
	WeaponClassesClear(&gGunDescriptions.CustomGuns);

	// Load any custom data
	LoadArchiveSounds(&gSoundDevice, filename, "sounds");

	LoadArchivePics(&gPicManager, filename, "graphics");

	root = ReadPhysFSJSON(filename, "particles.json");
	if (root != NULL)
	{
		ParticleClassesLoadJSON(&gParticleClasses.CustomClasses, root);
	}

	root = ReadPhysFSJSON(filename, "bullets.json");
	if (root != NULL)
	{
		BulletLoadJSON(
			&gBulletClasses, &gBulletClasses.CustomClasses, root);
	}

	root = ReadPhysFSJSON(filename, "guns.json");
	if (root != NULL)
	{
		WeaponLoadJSON(
			&gGunDescriptions, &gGunDescriptions.CustomGuns, root);
		json_free_value(&root);
	}

	BulletLoadWeapons(&gBulletClasses);


	root = ReadPhysFSJSON(filename, "missions.json");
	if (root == NULL)
	{
		err = -1;
		goto bail;
	}
	LoadMissions(
		&c->Missions, json_find_first_label(root, "Missions")->child, version);
	json_free_value(&root);

	root = ReadPhysFSJSON(filename, "characters.json");
	if (root == NULL)
	{
		err = -1;
		goto bail;
	}
	LoadCharacters(
		&c->characters, json_find_first_label(root, "Characters")->child);

bail:
	json_free_value(&root);
	return err;
}

static json_t *ReadPhysFSJSON(const char *archive, const char *filename)
{
	json_t *root = NULL;
	PHYSFS_File *f = NULL;
	char *buf = NULL;

	if (!PHYSFS_addToSearchPath(archive, 0))
	{
		printf("Failed to add to search path. reason: %s.\n",
			PHYSFS_getLastError());
		goto bail;
	}

	f = PHYSFS_openRead(filename);
	if (f == NULL)
	{
		printf("failed to open. Reason: [%s].\n", PHYSFS_getLastError());
		goto bail;
	}

	// Read into buffer
	int len = (int)PHYSFS_fileLength(f);
	CCALLOC(buf, len + 1);
	if (PHYSFS_read(f, buf, 1, len) < len)
	{
		printf("PHYSFS_read() failed: %s.\n", PHYSFS_getLastError());
		goto bail;
	}
	if (json_parse_document(&root, buf) != JSON_OK)
	{
		root = NULL;
		goto bail;
	}

bail:
	CFREE(buf);
	if (f != NULL && !PHYSFS_close(f))
	{
		printf("PHYSFS failure. reason: %s.\n", PHYSFS_getLastError());
	}
	if (!PHYSFS_removeFromSearchPath(archive))
	{
		printf("PHYSFS failure. reason: %s.\n", PHYSFS_getLastError());
	}
	return root;
}

static void LoadArchiveSounds(
	SoundDevice *device, const char *archive, const char *dirname)
{
	char **rc = NULL;
	PHYSFS_File *f = NULL;
	char *buf = NULL;

	if (!PHYSFS_addToSearchPath(archive, 0))
	{
		printf("Failed to add to search path. reason: %s.\n",
			PHYSFS_getLastError());
		goto bail;
	}
	
	rc = PHYSFS_enumerateFiles(dirname);
	if (rc == NULL)
	{
		return;
	}

	for (char **i = rc; *i != NULL; i++)
	{
		char path[CDOGS_PATH_MAX];
		sprintf(path, "%s/%s", dirname, *i);
		f = PHYSFS_openRead(path);
		if (f == NULL)
		{
			printf("failed to open %s. Reason: [%s].\n",
				path, PHYSFS_getLastError());
			goto bail;
		}
		int len = (int)PHYSFS_fileLength(f);
		CCALLOC(buf, len + 1);
		if (PHYSFS_read(f, buf, 1, len) < len)
		{
			printf("PHYSFS_read() %s failed: %s.\n",
				path, PHYSFS_getLastError());
			goto bail;
		}
		SDL_RWops *rwops = SDL_RWFromMem(buf, len);
		Mix_Chunk *data = Mix_LoadWAV_RW(rwops, 0);
		if (data != NULL)
		{
			char nameBuf[CDOGS_FILENAME_MAX];
			strcpy(nameBuf, *i);
			// Remove extension
			char *dot = strrchr(nameBuf, '.');
			if (dot != NULL)
			{
				*dot = '\0';
			}
			SoundAdd(&device->customSounds, nameBuf, data);
		}
		rwops->close(rwops);
		CFREE(buf);
		buf = NULL;
		PHYSFS_close(f);
		f = NULL;
	}

bail:
	CFREE(buf);
	if (f != NULL && !PHYSFS_close(f))
	{
		printf("PHYSFS failure. reason: %s.\n", PHYSFS_getLastError());
	}
	if (!PHYSFS_removeFromSearchPath(archive))
	{
		printf("PHYSFS failure. reason: %s.\n", PHYSFS_getLastError());
	}
	PHYSFS_freeList(rc);
}
static void LoadArchivePics(
	PicManager *pm, const char *archive, const char *dirname)
{
	char **rc = NULL;
	PHYSFS_File *f = NULL;
	char *buf = NULL;

	if (!PHYSFS_addToSearchPath(archive, 0))
	{
		printf("Failed to add to search path. reason: %s.\n",
			PHYSFS_getLastError());
		goto bail;
	}

	rc = PHYSFS_enumerateFiles(dirname);
	if (rc == NULL)
	{
		return;
	}

	for (char **i = rc; *i != NULL; i++)
	{
		char path[CDOGS_PATH_MAX];
		sprintf(path, "%s/%s", dirname, *i);
		f = PHYSFS_openRead(path);
		if (f == NULL)
		{
			printf("failed to open %s. Reason: [%s].\n",
				path, PHYSFS_getLastError());
			goto bail;
		}
		int len = (int)PHYSFS_fileLength(f);
		CCALLOC(buf, len + 1);
		if (PHYSFS_read(f, buf, 1, len) < len)
		{
			printf("PHYSFS_read() %s failed: %s.\n",
				path, PHYSFS_getLastError());
			goto bail;
		}
		SDL_RWops *rwops = SDL_RWFromMem(buf, len);
		bool isPng = IMG_isPNG(rwops);
		if (isPng)
		{
			SDL_Surface *data = IMG_Load_RW(rwops, 0);
			if (data != NULL)
			{
				char nameBuf[CDOGS_FILENAME_MAX];
				PathGetBasenameWithoutExtension(nameBuf, *i);
				PicManagerAdd(&pm->customPics, &pm->customSprites, nameBuf, data);
			}
		}
		rwops->close(rwops);
		CFREE(buf);
		buf = NULL;
		PHYSFS_close(f);
		f = NULL;
	}

bail:
	CFREE(buf);
	if (f != NULL && !PHYSFS_close(f))
	{
		printf("PHYSFS failure. reason: %s.\n", PHYSFS_getLastError());
	}
	if (!PHYSFS_removeFromSearchPath(archive))
	{
		printf("PHYSFS failure. reason: %s.\n", PHYSFS_getLastError());
	}
	PHYSFS_freeList(rc);
}

static json_t *SaveMissions(CArray *a);
static json_t *SaveCharacters(CharacterStore *s);
bool TrySaveJSONFile(json_t *node, const char *filename);
int MapArchiveSave(const char *filename, CampaignSetting *c)
{
	int res = 1;

	// Save separate files via physfs
	if (!PHYSFS_setWriteDir("."))
	{
		printf("Failed to set write dir. reason: %s.\n",
			PHYSFS_getLastError());
		res = 0;
		goto bail;
	}
	char buf[CDOGS_PATH_MAX];
	if (strcmp(StrGetFileExt(filename), "cdogscpn") == 0 ||
		strcmp(StrGetFileExt(filename), "CDOGSCPN") == 0)
	{
		strcpy(buf, filename);
	}
	else
	{
		sprintf(buf, "%s.cdogscpn", filename);
	}
	if (!PHYSFS_mkdir(buf))
	{
		printf("Failed to make dir. reason: %s.\n", PHYSFS_getLastError());
		res = 0;
		goto bail;
	}
	setlocale(LC_ALL, "");

	// Campaign
	json_t *root = json_new_object();
	AddIntPair(root, "Version", VERSION);
	AddStringPair(root, "Title", c->Title);
	AddStringPair(root, "Author", c->Author);
	AddStringPair(root, "Description", c->Description);
	AddIntPair(root, "Missions", c->Missions.size);
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

	json_free_value(&root);
	root = json_new_object();
	json_insert_pair_into_object(
		root, "Characters", SaveCharacters(&c->characters));
	sprintf(buf2, "%s/characters.json", buf);
	if (!TrySaveJSONFile(root, buf2))
	{
		res = 0;
		goto bail;
	}

bail:
	json_free_value(&root);
	return res;
}
bool TrySaveJSONFile(json_t *node, const char *filename)
{
	bool res = true;
	char *text;
	json_tree_to_string(node, &text);
	char *ftext = json_format_string(text);
	PHYSFS_File *pf = PHYSFS_openWrite(filename);
	if (pf == NULL)
	{
		printf("failed to open. Reason: [%s].\n", PHYSFS_getLastError());
		res = false;
		goto bail;
	}
	size_t writeLen = strlen(ftext);
	PHYSFS_sint64 rc = PHYSFS_write(pf, ftext, 1, writeLen);
	if (rc != (int)writeLen)
	{
		printf("Wrote (%d) of (%d) bytes. Reason: [%s].\n",
			(int)rc, (int)writeLen, PHYSFS_getLastError());
		res = false;
		goto bail;
	}

bail:
	CFREE(text);
	CFREE(ftext);
	PHYSFS_close(pf);
	return res;
}

static json_t *SaveObjectives(CArray *a);
static json_t *SaveIntArray(CArray *a);
static json_t *SaveVec2i(Vec2i v);
static json_t *SaveWeapons(const CArray *weapons);
static json_t *SaveClassicRooms(Mission *m);
static json_t *SaveClassicDoors(Mission *m);
static json_t *SaveClassicPillars(Mission *m);
static json_t *SaveStaticTiles(Mission *m);
static json_t *SaveStaticItems(Mission *m);
static json_t *SaveStaticWrecks(Mission *m);
static json_t *SaveStaticCharacters(Mission *m);
static json_t *SaveStaticObjectives(Mission *m);
static json_t *SaveStaticKeys(Mission *m);
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

		AddIntPair(node, "WallStyle", mission->WallStyle);
		AddIntPair(node, "FloorStyle", mission->FloorStyle);
		AddIntPair(node, "RoomStyle", mission->RoomStyle);
		AddIntPair(node, "ExitStyle", mission->ExitStyle);
		AddIntPair(node, "KeyStyle", mission->KeyStyle);
		AddIntPair(node, "DoorStyle", mission->DoorStyle);

		json_insert_pair_into_object(
			node, "Objectives", SaveObjectives(&mission->Objectives));
		json_insert_pair_into_object(
			node, "Enemies", SaveIntArray(&mission->Enemies));
		json_insert_pair_into_object(
			node, "SpecialChars", SaveIntArray(&mission->SpecialChars));
		json_insert_pair_into_object(
			node, "Items", SaveIntArray(&mission->Items));
		json_insert_pair_into_object(
			node, "ItemDensities", SaveIntArray(&mission->ItemDensities));

		AddIntPair(node, "EnemyDensity", mission->EnemyDensity);
		json_insert_pair_into_object(
			node, "Weapons", SaveWeapons(&mission->Weapons));

		json_insert_pair_into_object(
			node, "Song", json_new_string(mission->Song));

		AddIntPair(node, "WallColor", mission->WallColor);
		AddIntPair(node, "FloorColor", mission->FloorColor);
		AddIntPair(node, "RoomColor", mission->RoomColor);
		AddIntPair(node, "AltColor", mission->AltColor);

		switch (mission->Type)
		{
		case MAPTYPE_CLASSIC:
			AddIntPair(node, "Walls", mission->u.Classic.Walls);
			AddIntPair(node, "WallLength", mission->u.Classic.WallLength);
			AddIntPair(
				node, "CorridorWidth", mission->u.Classic.CorridorWidth);
			json_insert_pair_into_object(
				node, "Rooms", SaveClassicRooms(mission));
			AddIntPair(node, "Squares", mission->u.Classic.Squares);
			json_insert_pair_into_object(
				node, "Doors", SaveClassicDoors(mission));
			json_insert_pair_into_object(
				node, "Pillars", SaveClassicPillars(mission));
			break;
		case MAPTYPE_STATIC:
			{
				json_insert_pair_into_object(
					node, "Tiles", SaveStaticTiles(mission));
				json_insert_pair_into_object(
					node, "StaticItems", SaveStaticItems(mission));
				json_insert_pair_into_object(
					node, "StaticWrecks", SaveStaticWrecks(mission));
				json_insert_pair_into_object(
					node, "StaticCharacters", SaveStaticCharacters(mission));
				json_insert_pair_into_object(
					node, "StaticObjectives", SaveStaticObjectives(mission));
				json_insert_pair_into_object(
					node, "StaticKeys", SaveStaticKeys(mission));

				json_insert_pair_into_object(
					node, "Start", SaveVec2i(mission->u.Static.Start));
				json_t *exitNode = json_new_object();
				json_insert_pair_into_object(
					exitNode, "Start",
					SaveVec2i(mission->u.Static.Exit.Start));
				json_insert_pair_into_object(
					exitNode, "End",
					SaveVec2i(mission->u.Static.Exit.End));
				json_insert_pair_into_object(node, "Exit", exitNode);
			}
			break;
		default:
			assert(0 && "unknown map type");
			break;
		}

		json_insert_child(missionsNode, node);
	}
	return missionsNode;
}
static json_t *SaveCharacters(CharacterStore *s)
{
	json_t *charNode = json_new_array();
	int i;
	for (i = 0; i < (int)s->OtherChars.size; i++)
	{
		json_t *node = json_new_object();
		Character *c = CArrayGet(&s->OtherChars, i);
		AddIntPair(node, "face", c->looks.face);
		AddIntPair(node, "skin", c->looks.skin);
		AddIntPair(node, "arm", c->looks.arm);
		AddIntPair(node, "body", c->looks.body);
		AddIntPair(node, "leg", c->looks.leg);
		AddIntPair(node, "hair", c->looks.hair);
		AddIntPair(node, "speed", c->speed);
		json_insert_pair_into_object(
			node, "Gun", json_new_string(c->Gun->name));
		AddIntPair(node, "maxHealth", c->maxHealth);
		AddIntPair(node, "flags", c->flags);
		AddIntPair(node, "probabilityToMove", c->bot->probabilityToMove);
		AddIntPair(node, "probabilityToTrack", c->bot->probabilityToTrack);
		AddIntPair(node, "probabilityToShoot", c->bot->probabilityToShoot);
		AddIntPair(node, "actionDelay", c->bot->actionDelay);
		json_insert_child(charNode, node);
	}
	return charNode;
}
static json_t *SaveClassicRooms(Mission *m)
{
	json_t *node = json_new_object();
	AddIntPair(node, "Count", m->u.Classic.Rooms.Count);
	AddIntPair(node, "Min", m->u.Classic.Rooms.Min);
	AddIntPair(node, "Max", m->u.Classic.Rooms.Max);
	AddBoolPair(node, "Edge", m->u.Classic.Rooms.Edge);
	AddBoolPair(node, "Overlap", m->u.Classic.Rooms.Overlap);
	AddIntPair(node, "Walls", m->u.Classic.Rooms.Walls);
	AddIntPair(node, "WallLength", m->u.Classic.Rooms.WallLength);
	AddIntPair(node, "WallPad", m->u.Classic.Rooms.WallPad);
	return node;
}
static json_t *SaveClassicPillars(Mission *m)
{
	json_t *node = json_new_object();
	AddIntPair(node, "Count", m->u.Classic.Pillars.Count);
	AddIntPair(node, "Min", m->u.Classic.Pillars.Min);
	AddIntPair(node, "Max", m->u.Classic.Pillars.Max);
	return node;
}
static json_t *SaveClassicDoors(Mission *m)
{
	json_t *node = json_new_object();
	AddBoolPair(node, "Enabled", m->u.Classic.Doors.Enabled);
	AddIntPair(node, "Min", m->u.Classic.Doors.Min);
	AddIntPair(node, "Max", m->u.Classic.Doors.Max);
	return node;
}

static json_t *SaveStaticTiles(Mission *m)
{
	// Create a text buffer for CSV
	// The buffer will contain n*5 chars (tiles, allow 5 chars each),
	// and n - 1 commas, so 6n total
	int size = (int)m->u.Static.Tiles.size;
	char *bigbuf;
	CCALLOC(bigbuf, size * 6);
	char *pBuf = bigbuf;
	for (int i = 0; i < size; i++)
	{
		char buf[32];
		sprintf(buf, "%d", *(unsigned short *)CArrayGet(
			&m->u.Static.Tiles, i));
		strcpy(pBuf, buf);
		pBuf += strlen(buf);
		if (i < size - 1)
		{
			*pBuf = ',';
			pBuf++;
		}
	}
	json_t *node = json_new_string(bigbuf);
	CFREE(bigbuf);
	return node;
}
static json_t *SaveStaticItems(Mission *m)
{
	json_t *items = json_new_array();
	for (int i = 0; i < (int)m->u.Static.Items.size; i++)
	{
		MapObjectPositions *mop =
			CArrayGet(&m->u.Static.Items, i);
		json_t *itemNode = json_new_object();
		AddIntPair(itemNode, "Index", mop->Index);
		json_t *positions = json_new_array();
		for (int j = 0; j < (int)mop->Positions.size; j++)
		{
			Vec2i *pos = CArrayGet(&mop->Positions, j);
			json_insert_child(positions, SaveVec2i(*pos));
		}
		json_insert_pair_into_object(
			itemNode, "Positions", positions);
		json_insert_child(items, itemNode);
	}
	return items;
}
static json_t *SaveStaticWrecks(Mission *m)
{
	json_t *wrecks = json_new_array();
	for (int i = 0; i < (int)m->u.Static.Wrecks.size; i++)
	{
		MapObjectPositions *mop =
			CArrayGet(&m->u.Static.Wrecks, i);
		json_t *wreckNode = json_new_object();
		AddIntPair(wreckNode, "Index", mop->Index);
		json_t *positions = json_new_array();
		for (int j = 0; j < (int)mop->Positions.size; j++)
		{
			Vec2i *pos = CArrayGet(&mop->Positions, j);
			json_insert_child(positions, SaveVec2i(*pos));
		}
		json_insert_pair_into_object(
			wreckNode, "Positions", positions);
		json_insert_child(wrecks, wreckNode);
	}
	return wrecks;
}
static json_t *SaveStaticCharacters(Mission *m)
{
	json_t *chars = json_new_array();
	for (int i = 0; i < (int)m->u.Static.Characters.size; i++)
	{
		CharacterPositions *cp =
			CArrayGet(&m->u.Static.Characters, i);
		json_t *charNode = json_new_object();
		AddIntPair(charNode, "Index", cp->Index);
		json_t *positions = json_new_array();
		for (int j = 0; j < (int)cp->Positions.size; j++)
		{
			Vec2i *pos = CArrayGet(&cp->Positions, j);
			json_insert_child(positions, SaveVec2i(*pos));
		}
		json_insert_pair_into_object(
			charNode, "Positions", positions);
		json_insert_child(chars, charNode);
	}
	return chars;
}
static json_t *SaveStaticObjectives(Mission *m)
{
	json_t *objs = json_new_array();
	for (int i = 0; i < (int)m->u.Static.Objectives.size; i++)
	{
		ObjectivePositions *op =
		CArrayGet(&m->u.Static.Objectives, i);
		json_t *objNode = json_new_object();
		AddIntPair(objNode, "Index", op->Index);
		json_t *positions = json_new_array();
		for (int j = 0; j < (int)op->Positions.size; j++)
		{
			Vec2i *pos = CArrayGet(&op->Positions, j);
			json_insert_child(positions, SaveVec2i(*pos));
		}
		json_insert_pair_into_object(
			objNode, "Positions", positions);
		json_insert_pair_into_object(
			objNode, "Indices", SaveIntArray(&op->Indices));
		json_insert_child(objs, objNode);
	}
	return objs;
}
static json_t *SaveStaticKeys(Mission *m)
{
	json_t *keys = json_new_array();
	for (int i = 0; i < (int)m->u.Static.Keys.size; i++)
	{
		KeyPositions *kp =
		CArrayGet(&m->u.Static.Keys, i);
		json_t *keyNode = json_new_object();
		AddIntPair(keyNode, "Index", kp->Index);
		json_t *positions = json_new_array();
		for (int j = 0; j < (int)kp->Positions.size; j++)
		{
			Vec2i *pos = CArrayGet(&kp->Positions, j);
			json_insert_child(positions, SaveVec2i(*pos));
		}
		json_insert_pair_into_object(
			keyNode, "Positions", positions);
		json_insert_child(keys, keyNode);
	}
	return keys;
}

static json_t *SaveObjectives(CArray *a)
{
	json_t *objectivesNode = json_new_array();
	int i;
	for (i = 0; i < (int)a->size; i++)
	{
		json_t *objNode = json_new_object();
		MissionObjective *mo = CArrayGet(a, i);
		AddStringPair(objNode, "Description", mo->Description);
		AddStringPair(objNode, "Type", ObjectiveTypeStr(mo->Type));
		AddIntPair(objNode, "Index", mo->Index);
		AddIntPair(objNode, "Count", mo->Count);
		AddIntPair(objNode, "Required", mo->Required);
		AddIntPair(objNode, "Flags", mo->Flags);
		json_insert_child(objectivesNode, objNode);
	}
	return objectivesNode;
}

static json_t *SaveIntArray(CArray *a)
{
	json_t *node = json_new_array();
	int i;
	for (i = 0; i < (int)a->size; i++)
	{
		char buf[32];
		sprintf(buf, "%d", *(int *)CArrayGet(a, i));
		json_insert_child(node, json_new_number(buf));
	}
	return node;
}
static json_t *SaveVec2i(Vec2i v)
{
	json_t *node = json_new_array();
	char buf[32];
	sprintf(buf, "%d", v.x);
	json_insert_child(node, json_new_number(buf));
	sprintf(buf, "%d", v.y);
	json_insert_child(node, json_new_number(buf));
	return node;
}

static json_t *SaveWeapons(const CArray *weapons)
{
	json_t *node = json_new_array();
	for (int i = 0; i < (int)weapons->size; i++)
	{
		const GunDescription **g = CArrayGet(weapons, i);
		json_insert_child(node, json_new_string((*g)->name));
	}
	return node;
}
