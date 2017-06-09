/*
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
#include "namegen.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static void LoadFile(CArray *strings, const char *filename);
void NameGenInit(
	NameGen *g,
	const char *prefixFile, const char *suffixFile, const char *suffixNameFile)
{
	CArrayInit(&g->prefixes, sizeof(char *));
	CArrayInit(&g->suffixes, sizeof(char *));
	CArrayInit(&g->suffixNames, sizeof(char *));
	LoadFile(&g->prefixes, prefixFile);
	LoadFile(&g->suffixes, suffixFile);
	LoadFile(&g->suffixNames, suffixNameFile);
}
static void LoadFile(CArray *strings, const char *filename)
{
	FILE *file = fopen(filename, "r");
	char buf[256];
	while (fgets(buf, 256, file) != NULL)
	{
		char *str = malloc(strlen(buf) + 1);
		strncpy(str, buf, strlen(buf) - 1);
		str[strlen(buf) - 1] = '\0';
		CArrayPushBack(strings, &str);
	}
	fclose(file);
}
static void UnloadStrings(CArray *strings);
void NameGenTerminate(NameGen *g)
{
	UnloadStrings(&g->prefixes);
	UnloadStrings(&g->suffixes);
	UnloadStrings(&g->suffixNames);
}
static void UnloadStrings(CArray *strings)
{
	for (int i = 0; i < (int)strings->size; i++)
	{
		char **str = CArrayGet(strings, i);
		free(*str);
	}
	CArrayTerminate(strings);
}

static bool IsSameWord(const char *l, const char *r);
void NameGenMake(const NameGen *g, char *buf)
{
	for (;;)
	{
		char **prefix = CArrayGet(&g->prefixes, rand() % g->prefixes.size);
		int suffixIndex = rand() % (g->suffixes.size + g->suffixNames.size);
		char **suffix;
		if (suffixIndex < (int)g->suffixes.size)
		{
			suffix = CArrayGet(&g->suffixes, suffixIndex);
		}
		else
		{
			suffix = CArrayGet(
				&g->suffixNames, suffixIndex - (int)g->suffixes.size);
		}
		// Don't generate same prefix/suffix
		if (IsSameWord(*prefix, *suffix))
		{
			continue;
		}
		strcpy(buf, *prefix);
		// Add a space if we're using a suffix name
		if (suffixIndex >= (int)g->suffixes.size)
		{
			strcat(buf, " ");
		}
		strcat(buf, *suffix);
		break;
	}
}
static char LowerCase(const char c);
static bool IsSameWord(const char *l, const char *r)
{
	while (*l && *r)
	{
		if (LowerCase(*l) != LowerCase(*r))
		{
			return false;
		}
		l++;
		r++;
	}
	if (*l || *r)
	{
		return false;
	}
	return true;
}
static char LowerCase(const char c)
{
	if (c >= 'A' && c <= 'Z')
	{
		return c - 32;
	}
	return c;
}
