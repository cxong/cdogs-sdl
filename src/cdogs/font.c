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
#include "font.h"

#include <SDL_image.h>

#include "pic.h"
#include "json_utils.h"

#define FIRST_CHAR 32
#define LAST_CHAR 126

Font gFont;


void FontLoad(Font *f, const char *imgPath, const char *jsonPath)
{
	FILE *file = NULL;
	json_t *root = NULL;

	SDL_RWops *rwops = SDL_RWFromFile(imgPath, "rb");
	CASSERT(IMG_isPNG(rwops), "Error: font file is not PNG");
	SDL_Surface *data = IMG_Load_RW(rwops, 0);
	if (!data)
	{
		fprintf(stderr, "Cannot load font image: %s\n", IMG_GetError());
		goto bail;
	}

	file = fopen(jsonPath, "r");
	if (file == NULL)
	{
		printf("Error loading font JSON file '%s'\n", jsonPath);
		goto bail;
	}
	if (json_stream_parse(file, &root) != JSON_OK)
	{
		printf("Error parsing font JSON '%s'\n", jsonPath);
		goto bail;
	}

	FontFromImage(f, data, root);

bail:
	if (file)
	{
		fclose(file);
	}
	SDL_FreeSurface(data);
	rwops->close(rwops);
}
void FontFromImage(Font *f, SDL_Surface *image, json_t *data)
{
	memset(f, 0, sizeof *f);
	CArrayInit(&f->Chars, sizeof(Pic));

	if (image->format->BytesPerPixel != 4)
	{
		perror("Cannot load non-32-bit image");
		fprintf(stderr, "Only 32-bit depth images supported\n");
		return;
	}

	// Load definitions from JSON data
	LoadVec2i(&f->Size, data, "Size");
	CASSERT(!Vec2iIsZero(f->Size), "Cannot load font size");
	LoadInt(&f->Stride, data, "Stride");
	LoadVec2i(&f->Padding, data, "Padding");

	// Check that the image is big enough for the dimensions
	const Vec2i step =
		Vec2iNew(f->Size.x + 2 * f->Padding.x, f->Size.y + 2 * f->Padding.y);
	if (step.x * f->Stride > image->w || step.y > image->h)
	{
		printf("Error: font image not big enough for font data "
			"Image %dx%d Size %dx%d Stride %d Padding %dx%d\n",
			image->w, image->h,
			f->Size.x, f->Size.y, f->Stride, f->Padding.x, f->Padding.y);
		return;
	}

	// Load letters from image file
	SDL_LockSurface(image);
	SDL_Surface *s = SDL_ConvertSurface(
		image, gGraphicsDevice.screen->format, SDL_SWSURFACE);
	CASSERT(s, "image convert failed");
	SDL_LockSurface(s);
	int chars = 0;
	for (Vec2i pos = Vec2iZero();
		pos.y + step.y <= image->h && chars < LAST_CHAR - FIRST_CHAR + 1;
		pos.y += step.y)
	{
		int x = 0;
		for (pos.x = 0;
			x < f->Stride && chars < LAST_CHAR - FIRST_CHAR + 1;
			pos.x += step.x, x++, chars++)
		{
			Pic p;
			p.size = f->Size;
			p.offset = Vec2iZero();
			PicLoad(&p, f->Size, Vec2iAdd(pos, f->Padding), image, s);
			CArrayPushBack(&f->Chars, &p);
		}
	}
	SDL_UnlockSurface(s);
	SDL_FreeSurface(s);
	SDL_UnlockSurface(image);
}
void FontTerminate(Font *f)
{
	for (int i = 0; i < (int)f->Chars.size; i++)
	{
		Pic *p = CArrayGet(&f->Chars, i);
		PicFree(p);
	}
	CArrayTerminate(&f->Chars);
}
