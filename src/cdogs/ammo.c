/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (c) 2014, 2016-2019, 2023 Cong Xu
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
#include "ammo.h"

#include <string.h>

#include "json_utils.h"
#include "log.h"
#include "pic_manager.h"
#include "utils.h"


AmmoClasses gAmmo;


// TODO: use map structure?
Ammo *StrAmmo(const char *s)
{
	return AmmoGetById(&gAmmo, StrAmmoId(s));
}
int StrAmmoId(const char *s)
{
	if (s == NULL || strlen(s) == 0)
	{
		return 0;
	}
	CA_FOREACH(Ammo, a, gAmmo.CustomAmmo)
		if (strcmp(s, a->Name) == 0)
		{
			return _ca_index + (int)gAmmo.Ammo.size;
		}
	CA_FOREACH_END()
	CA_FOREACH(Ammo, a, gAmmo.Ammo)
		if (strcmp(s, a->Name) == 0)
		{
			return _ca_index;
		}
	CA_FOREACH_END()
	CASSERT(false, "cannot parse ammo name");
	return 0;
}

#define VERSION 2
static void LoadAmmo(Ammo *a, json_t *node, const int version);
void AmmoInitialize(AmmoClasses *ammo, const char *path)
{
	memset(ammo, 0, sizeof *ammo);
	CArrayInit(&ammo->Ammo, sizeof(Ammo));
	CArrayInit(&ammo->CustomAmmo, sizeof(Ammo));

	json_t *root = NULL;
	enum json_error e;

	char buf[CDOGS_PATH_MAX];
	GetDataFilePath(buf, path);
	FILE *f = fopen(buf, "r");
	if (f == NULL)
	{
		LOG(LM_MAIN, LL_ERROR, "Error: cannot load ammo file %s", buf);
		goto bail;
	}
	e = json_stream_parse(f, &root);
	if (e != JSON_OK)
	{
		LOG(LM_MAIN, LL_ERROR, "Error parsing ammo file %s [error %d]",
			buf, (int)e);
		goto bail;
	}
	AmmoLoadJSON(&ammo->Ammo, root);

bail:
	if (f)
	{
		fclose(f);
	}
	json_free_value(&root);
}
void AmmoLoadJSON(CArray *ammo, json_t *node)
{
	int version;
	LoadInt(&version, node, "Version");
	if (version > VERSION || version <= 0)
	{
		CASSERT(false, "cannot read ammo file version");
		return;
	}

	json_t *ammoNode = json_find_first_label(node, "Ammo")->child;
	for (json_t *child = ammoNode->child; child; child = child->next)
	{
		Ammo a;
		LoadAmmo(&a, child, version);
		CArrayPushBack(ammo, &a);
	}
}
static void LoadAmmo(Ammo *a, json_t *node, const int version)
{
	memset(a, 0, sizeof *a);
	a->Name = GetString(node, "Name");
	json_t *picNode = json_find_first_label(node, "Pic")->child;
	if (version < 2)
	{
		CPicLoadNormal(&a->Pic, picNode);
	}
	else
	{
		CPicLoadJSON(&a->Pic, picNode);
	}
	a->Sound = GetString(node, "Sound");
	LoadInt(&a->Amount, node, "Amount");
	LoadInt(&a->Max, node, "Max");
	LoadInt(&a->Price, node, "Price");
}
void AmmoClassesClear(CArray *ammo)
{
	CA_FOREACH(Ammo, a, *ammo)
		CFREE(a->Name);
		CFREE(a->Sound);
		CFREE(a->DefaultGun);
	CA_FOREACH_END()
	CArrayClear(ammo);
}
void AmmoTerminate(AmmoClasses *ammo)
{
	AmmoClassesClear(&ammo->Ammo);
	CArrayTerminate(&ammo->Ammo);
	AmmoClassesClear(&ammo->CustomAmmo);
	CArrayTerminate(&ammo->CustomAmmo);
}

Ammo *AmmoGetById(AmmoClasses *ammo, const int id)
{
	if (id < (int)ammo->Ammo.size)
	{
		return CArrayGet(&ammo->Ammo, id);
	}
	else
	{
		return CArrayGet(&ammo->CustomAmmo, id - (int)ammo->Ammo.size);
	}
}

int AmmoGetNumClasses(const AmmoClasses *ammo)
{
	return (int)ammo->Ammo.size + (int)ammo->CustomAmmo.size;
}


bool AmmoIsLow(const Ammo *a, const int amount)
{
	// Low ammo if less than or equal to half the default amount
	return amount <= (a->Amount / 2);
}
