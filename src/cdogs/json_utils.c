/*
 C-Dogs SDL
 A port of the legendary (and fun) action/arcade cdogs.
 
 Copyright (c) 2013-2016, Cong Xu
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
#include "json_utils.h"

#include <errno.h>
#include <stdlib.h>

#include "config.h"
#include "log.h"
#include "weapon.h"
#include "pic_manager.h"
#include "sys_config.h"


void AddIntPair(json_t *parent, const char *name, int number)
{
	char buf[32];
	sprintf(buf, "%d", number);
	json_insert_pair_into_object(parent, name, json_new_number(buf));
}
void AddBoolPair(json_t *parent, const char *name, int value)
{
	json_insert_pair_into_object(parent, name, json_new_bool(value));
}
void AddStringPair(json_t *parent, const char *name, const char *s)
{
	if (!s)
	{
		json_insert_pair_into_object(parent, name, json_new_string(""));
	}
	else
	{
		json_insert_pair_into_object(
			parent, name, json_new_string(json_escape(s)));
	}
}
void AddColorPair(json_t *parent, const char *name, const color_t c)
{
	char buf[8];
	ColorStr(buf, c);
	AddStringPair(parent, name, buf);
}

int TryLoadValue(json_t **node, const char *name)
{
	if (*node == NULL || (*node)->type != JSON_OBJECT)
	{
		return 0;
	}
	*node = json_find_first_label(*node, name);
	if (*node == NULL)
	{
		return 0;
	}
	*node = (*node)->child;
	if (*node == NULL)
	{
		return 0;
	}
	return 1;
}
void LoadBool(bool *value, json_t *node, const char *name)
{
	if (!TryLoadValue(&node, name))
	{
		return;
	}
	*value = node->type == JSON_TRUE;
}
void LoadInt(int *value, json_t *node, const char *name)
{
	if (!TryLoadValue(&node, name))
	{
		return;
	}
	*value = atoi(node->text);
}
void LoadDouble(double *value, json_t *node, const char *name)
{
	if (!TryLoadValue(&node, name))
	{
		return;
	}
	*value = atof(node->text);
}
void LoadVec2i(Vec2i *value, json_t *node, const char *name)
{
	if (!TryLoadValue(&node, name))
	{
		return;
	}
	node = node->child;
	value->x = atoi(node->text);
	node = node->next;
	value->y = atoi(node->text);
}
void LoadStr(char **value, json_t *node, const char *name)
{
	if (!TryLoadValue(&node, name))
	{
		return;
	}
	*value = json_unescape(node->text);
}
char *GetString(json_t *node, const char *name)
{
	return json_unescape(json_find_first_label(node, name)->child->text);
}
void LoadSoundFromNode(Mix_Chunk **value, json_t *node, const char *name)
{
	if (json_find_first_label(node, name) == NULL)
	{
		return;
	}
	if (!TryLoadValue(&node, name))
	{
		return;
	}
	*value = StrSound(node->text);
}
void LoadPic(const Pic **value, json_t *node, const char *name)
{
	if (json_find_first_label(node, name))
	{
		char *tmp = GetString(node, name);
		*value = PicManagerGetPic(&gPicManager, tmp);
		CFREE(tmp);
	}
}
void LoadBulletGuns(CArray *guns, json_t *node, const char *name)
{
	node = json_find_first_label(node, name);
	if (node == NULL || node->child == NULL)
	{
		return;
	}
	CArrayInit(guns, sizeof(const GunDescription *));
	for (json_t *gun = node->child->child; gun; gun = gun->next)
	{
		const GunDescription *g = StrGunDescription(gun->text);
		CArrayPushBack(guns, &g);
	}
}
void LoadColor(color_t *c, json_t *node, const char *name)
{
	if (json_find_first_label(node, name) == NULL)
	{
		return;
	}
	if (!TryLoadValue(&node, name))
	{
		return;
	}
	*c = StrColor(node->text);
}

json_t *JSONFindNode(json_t *node, const char *path)
{
	char *pathCopy;
	CSTRDUP(pathCopy, path);
	char *pch = strtok(pathCopy, "/");
	while (pch != NULL)
	{
		node = json_find_first_label(node, pch);
		if (node == NULL)
		{
			goto bail;
		}
		node = node->child;
		if (node == NULL)
		{
			goto bail;
		}
		pch = strtok(NULL, "/");
	}
bail:
	CFREE(pathCopy);
	return node;
}

bool TrySaveJSONFile(json_t *node, const char *filename)
{
	bool res = true;
	char *text;
	json_tree_to_string(node, &text);
	char *ftext = json_format_string(text);
	FILE *f = fopen(filename, "w");
	if (f == NULL)
	{
		LOG(LM_MAIN, LL_ERROR, "failed to open file(%s) for saving: %s",
			filename, strerror(errno));
		res = false;
		goto bail;
	}
	size_t writeLen = strlen(ftext);
	const size_t rc = fwrite(ftext, 1, writeLen, f);
	if (rc != writeLen)
	{
		LOG(LM_MAIN, LL_ERROR, "Wrote (%d) of (%d) bytes: %s",
			(int)rc, (int)writeLen, strerror(errno));
		res = false;
		goto bail;
	}

bail:
	CFREE(text);
	CFREE(ftext);
	if (f != NULL) fclose(f);
	return res;
}
