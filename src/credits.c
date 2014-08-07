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
#include <cdogs/font.h>
#include <cdogs/utils.h>


void LoadCredits(
	credits_displayer_t *displayer, color_t nameColor, color_t textColor)
{
	char buf[CDOGS_PATH_MAX];
	int nameOrMessageCounter = 0;
	FILE *file;

	debug(D_NORMAL, "Reading CREDITS...\n");

	GetDataFilePath(buf, "doc/CREDITS");
	file = fopen(buf, "r");
	if (file == NULL)
	{
		printf("Error: cannot load %s\n", buf);
		return;
	}

	assert(displayer != NULL);

	CArrayInit(&displayer->credits, sizeof(credit_t));
	displayer->lastUpdateTime = time(NULL);
	displayer->creditsIndex = 0;
	displayer->nameColor = nameColor;
	displayer->textColor = textColor;
	while (fgets(buf, CDOGS_PATH_MAX, file) != NULL)
	{
		char nameBuf[256];
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
			credit_t credit;
			CSTRDUP(credit.name, nameBuf);
			CSTRDUP(credit.message, buf + 1);
			CArrayPushBack(&displayer->credits, &credit);
			nameOrMessageCounter = 0;

			debug(D_VERBOSE, "Read credits for \"%s\"\n", credit.name);
		}
	}

	debug(D_NORMAL, "%d credits read\n", (int)displayer->credits.size);

	fclose(file);
}

void UnloadCredits(credits_displayer_t *displayer)
{
	CASSERT(displayer != NULL, "null pointer");
	for (int i = 0; i < (int)displayer->credits.size; i++)
	{
		credit_t *credit = CArrayGet(&displayer->credits, i);
		CFREE(credit->name);
		CFREE(credit->message);
	}
	CArrayTerminate(&displayer->credits);
	displayer->creditsIndex = 0;
}

#define CREDIT_DISPLAY_PERIOD_SECONDS 10.0
void ShowCredits(credits_displayer_t *displayer)
{
	CASSERT(displayer != NULL, "null pointer");
	if ((int)displayer->credits.size > 0)
	{
		time_t now = time(NULL);
		credit_t *credits =
			CArrayGet(&displayer->credits, displayer->creditsIndex);
		int y = gGraphicsDevice.cachedConfig.Res.y - 50;

		FontStrMask("Credits:", Vec2iNew(16, y), displayer->textColor);
		y += FontH() + 2;
		FontStrMask(credits->name, Vec2iNew(20, y), displayer->nameColor);
		y += FontH();
		FontStrMask(credits->message, Vec2iNew(20, y), displayer->textColor);

		if (difftime(now, displayer->lastUpdateTime) > CREDIT_DISPLAY_PERIOD_SECONDS)
		{
			displayer->creditsIndex++;
			if (displayer->creditsIndex >= (int)displayer->credits.size)
			{
				displayer->creditsIndex = 0;
			}
			displayer->lastUpdateTime = now;
		}
	}
}
