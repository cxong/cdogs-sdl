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
#include "config.h"

#include <stdio.h>

#include <cdogs/files.h>
#include <cdogs/grafx.h>
#include <cdogs/sounds.h>
#include <cdogs/utils.h>


void LoadConfig(void)
{
	FILE *f;
	int fx, music, channels, musicChannels;

	f = fopen(GetConfigFilePath("options.cnf"), "r");

	if (f) {
		int fscanfres;
	#define CHECK_FSCANF(count)\
		if (fscanfres < count) {\
			printf("Error loading config\n");\
			fclose(f);\
			return;\
		}
		fscanfres = fscanf(f, "%d %d %d %d %d %d %d\n",
			           &gOptions.displayFPS,
			           &gOptions.displayTime,
			           &gOptions.playersHurt,
			           &gOptions.brightness,
			           &gOptions.swapButtonsJoy1,
			           &gOptions.swapButtonsJoy2,
			           &gOptions.splitScreenAlways);
		CHECK_FSCANF(7)
		fscanfres = fscanf(f, "%d\n%d %d %d %d %d %d\n",
			(int *)&gPlayer1Data.inputDevice,
			&gPlayer1Data.keys.left,
			&gPlayer1Data.keys.right,
			&gPlayer1Data.keys.up,
			&gPlayer1Data.keys.down,
			&gPlayer1Data.keys.button1,
			&gPlayer1Data.keys.button2);
		CHECK_FSCANF(7)
		fscanfres = fscanf(f, "%d\n%d %d %d %d %d %d\n",
			(int *)&gPlayer2Data.inputDevice,
			&gPlayer2Data.keys.left,
			&gPlayer2Data.keys.right,
			&gPlayer2Data.keys.up,
			&gPlayer2Data.keys.down,
			&gPlayer2Data.keys.button1,
			&gPlayer2Data.keys.button2);
		CHECK_FSCANF(7)
		fscanfres = fscanf(f, "%d\n", &gOptions.mapKey);
		CHECK_FSCANF(1)
		fscanfres = fscanf(f, "%d %d %d %d\n",
			           &fx, &music, &channels, &musicChannels);
		CHECK_FSCANF(4)
		SetFXVolume(fx);
		SetMusicVolume(music);
		SetFXChannels(channels);
		SetMinMusicChannels(musicChannels);

		fscanfres = fscanf(f, "%u\n", &gCampaign.seed);
		CHECK_FSCANF(1)
		fscanfres = fscanf(f, "%d %d\n", (int *)&gOptions.difficulty,
			           &gOptions.slowmotion);
		CHECK_FSCANF(2)

		fscanfres = fscanf(f, "%d\n", &gOptions.density);
		CHECK_FSCANF(1)
		if (gOptions.density < 25 || gOptions.density > 200)
			gOptions.density = 100;
		fscanfres = fscanf(f, "%d\n", &gOptions.npcHp);
		CHECK_FSCANF(1)
		if (gOptions.npcHp < 25 || gOptions.npcHp > 200)
			gOptions.npcHp = 100;
		fscanfres = fscanf(f, "%d\n", &gOptions.playerHp);
		CHECK_FSCANF(1)
		if (gOptions.playerHp < 25 || gOptions.playerHp > 200)
			gOptions.playerHp = 100;

		{
			int w, h, scaleFactor, fs;

			if (fscanf(f, "%dx%d:%d:%d\n", &w, &h, &fs, &scaleFactor) == 4)
			{
				Gfx_SetHint(HINT_WIDTH, w);
				Gfx_SetHint(HINT_HEIGHT, h);

				if (fs != 0)
					Gfx_HintOn(HINT_FULLSCREEN);
				if (scaleFactor > 1)
					Gfx_SetHint(HINT_SCALEFACTOR, scaleFactor);
			}
		}

		fclose(f);
	}

	return;
}

void SaveConfig(void)
{
	FILE *f;

	debug(D_NORMAL, "begin\n");

	f = fopen(GetConfigFilePath("options.cnf"), "w");

	if (f) {
		fprintf(f, "%d %d %d %d %d %d %d\n",
			gOptions.displayFPS,
			gOptions.displayTime,
			gOptions.playersHurt,
			gOptions.brightness,
			gOptions.swapButtonsJoy1,
			gOptions.swapButtonsJoy2,
			gOptions.splitScreenAlways);
		fprintf(f, "%d\n%d %d %d %d %d %d\n",
			gPlayer1Data.inputDevice,
			gPlayer1Data.keys.left,
			gPlayer1Data.keys.right,
			gPlayer1Data.keys.up,
			gPlayer1Data.keys.down,
			gPlayer1Data.keys.button1,
			gPlayer1Data.keys.button2);
		fprintf(f, "%d\n%d %d %d %d %d %d\n",
			gPlayer2Data.inputDevice,
			gPlayer2Data.keys.left,
			gPlayer2Data.keys.right,
			gPlayer2Data.keys.up,
			gPlayer2Data.keys.down,
			gPlayer2Data.keys.button1,
			gPlayer2Data.keys.button2);
		fprintf(f, "%d\n", gOptions.mapKey);
		fprintf(f, "%d %d %d %d\n",
			FXVolume(),
			MusicVolume(), FXChannels(), MinMusicChannels());
		fprintf(f, "%u\n", gCampaign.seed);
		fprintf(f, "%d %d\n", gOptions.difficulty,
			gOptions.slowmotion);
		fprintf(f, "%d\n", gOptions.density);
		fprintf(f, "%d\n", gOptions.npcHp);
		fprintf(f, "%d\n", gOptions.playerHp);
		fprintf(f, "%dx%d:%d:%d\n",
			Gfx_GetHint(HINT_WIDTH),
			Gfx_GetHint(HINT_HEIGHT),
			Gfx_GetHint(HINT_FULLSCREEN),
			GrafxGetScale());
		fclose(f);

		debug(D_NORMAL, "saved config\n");
	}

	return;
}
