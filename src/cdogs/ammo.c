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
#include "ammo.h"

#include <string.h>

#include "json_utils.h"
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
	for (int i = 0; i < (int)gAmmo.CustomAmmo.size; i++)
	{
		Ammo *a = CArrayGet(&gAmmo.CustomAmmo, i);
		if (strcmp(s, a->Name) == 0)
		{
			return i + (int)gAmmo.Ammo.size;
		}
	}
	for (int i = 0; i < (int)gAmmo.Ammo.size; i++)
	{
		Ammo *a = CArrayGet(&gAmmo.Ammo, i);
		if (strcmp(s, a->Name) == 0)
		{
			return i;
		}
	}
	CASSERT(false, "cannot parse ammo name");
	return 0;
}

#define VERSION 1
static void LoadAmmo(Ammo *a, json_t *node);
void AmmoInitialize(AmmoClasses *ammo, const char *path)
{
	memset(ammo, 0, sizeof *ammo);
	CArrayInit(&ammo->Ammo, sizeof(Ammo));
	CArrayInit(&ammo->CustomAmmo, sizeof(Ammo));

	json_t *root = NULL;
	enum json_error e;

	FILE *f = fopen(path, "r");
	if (f == NULL)
	{
		printf("Error: cannot load ammo file %s\n", path);
		goto bail;
	}
	e = json_stream_parse(f, &root);
	if (e != JSON_OK)
	{
		printf("Error parsing ammo file %s [error %d]\n", path, (int)e);
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
		LoadAmmo(&a, child);
		CArrayPushBack(ammo, &a);
	}
}
static void LoadAmmo(Ammo *a, json_t *node)
{
	a->Name = GetString(node, "Name");
	char *tmp;
	tmp = GetString(node, "Pic");
	a->Pic = PicManagerGetPic(&gPicManager, tmp);
	CFREE(tmp);
	a->Sound = GetString(node, "Sound");
	LoadInt(&a->Amount, node, "Amount");
	LoadInt(&a->Max, node, "Max");
}
void AmmoClassesClear(CArray *ammo)
{
	for (int i = 0; i < (int)ammo->size; i++)
	{
		Ammo *a = CArrayGet(ammo, i);
		CFREE(a->Name);
		CFREE(a->Sound);
	}
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
