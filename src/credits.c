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
#include "credits.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <cdogs/config.h>
#include <cdogs/files.h>
#include <cdogs/text.h>
#include <cdogs/utils.h>


void LoadCredits(
	credits_displayer_t *displayer, color_t nameColor, color_t textColor)
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

	displayer->credits = NULL;
	displayer->creditsCount = 0;
	displayer->lastUpdateTime = time(NULL);
	displayer->creditsIndex = 0;
	displayer->nameColor = nameColor;
	displayer->textColor = textColor;
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
			int idx = displayer->creditsCount;
			displayer->creditsCount++;
			CREALLOC(displayer->credits, sizeof(credit_t)*displayer->creditsCount);
			CMALLOC(displayer->credits[idx].name, strlen(nameBuf) + 1);
			CMALLOC(displayer->credits[idx].message, strlen(buf));
			strcpy(displayer->credits[idx].name, nameBuf);
			strcpy(displayer->credits[idx].message, buf + 1);
			nameOrMessageCounter = 0;

			debug(D_VERBOSE, "Read credits for \"%s\"\n", displayer->credits[idx].name);
		}
	}

	debug(D_NORMAL, "%d credits read\n", displayer->creditsCount);

	fclose(file);
}

void UnloadCredits(credits_displayer_t *displayer)
{
	int i;
	assert(displayer != NULL);
	for (i = 0; i < displayer->creditsCount; i++)
	{
		CFREE(displayer->credits[i].name);
		CFREE(displayer->credits[i].message);
	}
	CFREE(displayer->credits);
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
		int y = gGraphicsDevice.cachedConfig.Res.y - 50;

		TextStringMasked(
			&gTextManager, "Credits:",
			&gGraphicsDevice, Vec2iNew(16, y),
			displayer->textColor);
		y += 10;
		TextStringMasked(
			&gTextManager, credits->name,
			&gGraphicsDevice, Vec2iNew(20, y),
			displayer->nameColor);
		y += CDogsTextHeight();
		TextStringMasked(
			&gTextManager, credits->message,
			&gGraphicsDevice, Vec2iNew(20, y),
			displayer->textColor);

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
