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

#include "blit.h"
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

int FontW(const char c)
{
	const Pic *p = CArrayGet(&gFont.Chars, (int)c - FIRST_CHAR);
	return p->size.x;
}
int FontH(void)
{
	return gFont.Size.y + 1;
}
int FontStrW(const char *s)
{
	return FontSubstrW(s, strlen(s));
}
int FontSubstrW(const char *s, int len)
{
	int w = 0;
	if (len > (int)strlen(s))
	{
		len = (int)strlen(s);
	}
	for (int i = 0; i < len; i++)
	{
		w += FontW(*s++);
	}
	return w;
}
int FontStrH(const char *s)
{
	int lines;
	for (lines = 0; s != NULL; lines++)
	{
		s = strchr(s, '\n');
		if (s)
		{
			s++;
		}
	}
	return lines * (FontH() + 1);
}
Vec2i FontStrSize(const char *s)
{
	Vec2i size = Vec2iZero();
	while (*s)
	{
		char *lineEnd = strchr(s, '\n');
		size.y += FontH();
		if (lineEnd)
		{
			size.x = MAX(size.x, FontSubstrW(s, lineEnd - s));
			s = lineEnd + 1;
		}
		else
		{
			size.x = MAX(size.x, FontStrW(s));
			s += strlen(s);
		}
	}
	return size;
}

Vec2i FontCh(const char c, const Vec2i pos)
{
	return FontChMask(c, pos, colorWhite);
}
Vec2i FontChMask(const char c, const Vec2i pos, const color_t mask)
{
	CASSERT((int)c >= FIRST_CHAR && (int)c <= LAST_CHAR, "invalid char");
	const int idx = (int)c - FIRST_CHAR;
	BlitMasked(
		&gGraphicsDevice, CArrayGet(&gFont.Chars, idx), pos, mask, true);
	// Add 1px of padding between characters
	return Vec2iNew(pos.x + gFont.Size.x + 1, pos.y);
}
Vec2i FontStr(const char *s, Vec2i pos)
{
	return FontStrMask(s, pos, colorWhite);
}
Vec2i FontStrMask(const char *s, Vec2i pos, const color_t mask)
{
	int left = pos.x;
	while (*s)
	{
		if (*s == '\n')
		{
			pos.x = left;
			pos.y += FontH();
		}
		else
		{
			pos = FontChMask(*s, pos, mask);
		}
		s++;
	}
	return pos;
}
Vec2i FontStrMaskWrap(const char *s, Vec2i pos, color_t mask, const int width)
{
	char buf[1024];
	CASSERT(strlen(s) < 1024, "string too long to wrap");
	FontSplitLines(s, buf, width);
	return FontStrMask(buf, pos, mask);
}

void FontSplitLines(const char *text, char *buf, const int width)
{
	int ix, x;
	const char *ws, *word, *s;

	ix = x = CenterX(width);
	s = ws = word = text;
	
	while (*s)
	{
		// Skip spaces
		ws = s;
		while (*s == ' ' || *s == '\n')
		{
			s++;
			*buf++ = ' ';
		}

		// Find word
		word = s;
		while (*s != 0 && *s != ' ' && *s != '\n')
		{
			s++;
		}
		// Calculate width of word
		int w;
		const char *p;
		for (w = 0, p = ws; p < s; p++)
		{
			w += FontW(*p) + 1;
		}

		// Create new line if text too wide
		if (x + w > width + ix && w < width)
		{
			x = ix;
			ws = word;
			*buf++ = '\n';
		}
		
		for (p = ws; p < word; p++)
		{
			x += FontW(*p) + 1;
		}

		for (p = word; p < s; p++)
		{
			*buf++ = *p;
			x += FontW(*p) + 1;
		}
	}
	*buf = '\0';
}
