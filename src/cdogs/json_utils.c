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
#include "json_utils.h"

#include <stdlib.h>

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
