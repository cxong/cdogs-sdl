/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.

    Copyright (c) 2013, Cong Xu
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
#include "credits.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <cdogs/files.h>
#include <cdogs/text.h>
#include <cdogs/utils.h>


void LoadCredits(
	credits_displayer_t *displayer,
	TranslationTable *nameTranslationTable,
	TranslationTable *textTranslationTable)
{
	char buf[1024];
	char nameBuf[256];
	int nameOrMessageCounter = 0;
	FILE *file;

	debug(D_NORMAL, "Reading CREDITS...\n");

	file = fopen(GetDataFilePath("doc/CREDITS"), "r");
	if (file == NULL)
	{
		printf("Error: cannot load %s\n", GetDataFilePath("doc/CREDITS"));
		return;
	}

	assert(displayer != NULL);
	assert(nameTranslationTable != NULL);
	assert(textTranslationTable != NULL);

	displayer->credits = NULL;
	displayer->creditsCount = 0;
	displayer->creditsIndex = 0;
	displayer->nameTranslationTable = nameTranslationTable;
	displayer->textTranslationTable = textTranslationTable;
	while (fgets(buf, 1024, file) != NULL)
	{
		if (strlen(buf) > 0 && buf[strlen(buf) - 1] == '\n')
		{
			buf[strlen(buf) - 1] = '\0';
		}
		if (strlen(buf) == 0)
		{
			nameOrMessageCounter = 0;
			continue;
		}
		if (nameOrMessageCounter == 0)
		{
			strcpy(nameBuf, buf);
			nameOrMessageCounter = 1;
		}
		else
		{
			int index = displayer->creditsCount;
			displayer->creditsCount++;
			displayer->credits = sys_mem_realloc(
				displayer->credits, sizeof(credit_t)*displayer->creditsCount);
			displayer->credits[index].name = sys_mem_alloc(strlen(nameBuf) + 1);
			displayer->credits[index].message = sys_mem_alloc(strlen(buf));
			if (displayer->credits == NULL ||
				displayer->credits[index].name == NULL ||
				displayer->credits[index].message == NULL)
			{
				printf("Error: out of memory\n");
				goto bail;
			}
			strcpy(displayer->credits[index].name, nameBuf);
			strcpy(displayer->credits[index].message, buf + 1);
			nameOrMessageCounter = 0;

			debug(D_VERBOSE, "Read credits for \"%s\"\n", displayer->credits[index].name);
		}
	}

	debug(D_NORMAL, "%d credits read\n", displayer->creditsCount);

bail:
	fclose(file);
}

void UnloadCredits(credits_displayer_t *displayer)
{
	int i;
	assert(displayer != NULL);
	for (i = 0; i < displayer->creditsCount; i++)
	{
		sys_mem_free(displayer->credits[i].name);
		sys_mem_free(displayer->credits[i].message);
	}
	sys_mem_free(displayer->credits);
	displayer->credits = NULL;
	displayer->creditsCount = 0;
	displayer->creditsIndex = 0;
}

void ShowCredits(credits_displayer_t *displayer)
{
	assert(displayer != NULL);
	if (displayer->creditsCount > 0)
	{
		time_t now = time(NULL);
		credit_t *credits = &displayer->credits[displayer->creditsIndex];

		CDogsTextStringWithTableAt(16, SCREEN_HEIGHT - 50, "Credits:", displayer->textTranslationTable);
		CDogsTextStringWithTableAt(20, SCREEN_HEIGHT - 40, credits->name, displayer->nameTranslationTable);
		CDogsTextStringWithTableAt(20, SCREEN_HEIGHT - 40 + CDogsTextHeight(), credits->message, displayer->textTranslationTable);

		if (difftime(now, displayer->lastUpdateTime) > CREDIT_DISPLAY_PERIOD_SECONDS)
		{
			displayer->creditsIndex++;
			if (displayer->creditsIndex >= displayer->creditsCount)
			{
				displayer->creditsIndex = 0;
			}
			displayer->lastUpdateTime = now;
		}
	}
}
