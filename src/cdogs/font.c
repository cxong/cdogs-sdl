/*
    Copyright (c) 2014-2016, Cong Xu
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

#include <stdio.h>
#include <string.h>

#include <SDL_image.h>

#include "blit.h"
#include "pic.h"
#include "sys_config.h"
#include "utils.h"


#define FIRST_CHAR 0
#define LAST_CHAR 255

Font gFont;


FontOpts FontOptsNew(void)
{
	FontOpts opts;
	memset(&opts, 0, sizeof opts);
	opts.Mask = colorWhite;
	return opts;
}

void FontLoad(Font *f, const char *imgPath, const bool isProportional)
{
	char buf[CDOGS_PATH_MAX];
	GetDataFilePath(buf, imgPath);
	SDL_RWops *rwops = SDL_RWFromFile(buf, "rb");
	CASSERT(IMG_isPNG(rwops), "Error: font file is not PNG");
	SDL_Surface *image = IMG_Load_RW(rwops, 0);
	if (!image)
	{
		fprintf(stderr, "Cannot load font image: %s\n", IMG_GetError());
		goto bail;
	}
	if (image->format->BytesPerPixel != 4)
	{
		perror("Cannot load non-32-bit image");
		fprintf(stderr, "Only 32-bit depth images supported\n");
		goto bail;
	}

	CArrayInit(&f->Chars, sizeof(Pic));

	// Check that the image is big enough for the dimensions
	const Vec2i step = Vec2iNew(
		f->Size.x + f->Padding.Left + f->Padding.Right,
		f->Size.y + f->Padding.Top + f->Padding.Bottom);
	if (step.x * f->Stride > image->w || step.y > image->h)
	{
		fprintf(stderr,
			"Error: font image not big enough for font data "
			"Image %dx%d Size %dx%d Stride %d Padding %d,%d,%d,%d\n",
			image->w, image->h,
			f->Size.x, f->Size.y, f->Stride,
			f->Padding.Left, f->Padding.Top,
			f->Padding.Right, f->Padding.Bottom);
		goto bail;
	}

	// Load letters from image file
	SDL_LockSurface(image);
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
			PicLoad(
				&p, f->Size,
				Vec2iAdd(pos, Vec2iNew(f->Padding.Left, f->Padding.Top)),
				image);
			if (isProportional)
			{
				PicTrim(&p, true, false);
			}
			CArrayPushBack(&f->Chars, &p);
		}
	}
	SDL_UnlockSurface(image);

bail:
	SDL_FreeSurface(image);
	rwops->close(rwops);
}
void FontTerminate(Font *f)
{
	CA_FOREACH(Pic, p, f->Chars)
		PicFree(p);
	CA_FOREACH_END()
	CArrayTerminate(&f->Chars);
}

int FontW(const char c)
{
	const Pic *p = CArrayGet(&gFont.Chars, (int)c - FIRST_CHAR);
	return p->size.x + gFont.Gap.x;
}
int FontH(void)
{
	return gFont.Size.y + gFont.Gap.y;
}
int FontStrW(const char *s)
{
	return FontSubstrW(s, strlen(s));
}
int FontSubstrW(const char *s, int len)
{
	if (len > (int)strlen(s))
	{
		len = (int)strlen(s);
	}
	// Find the width of the longest line, if this is a multi-line
	int maxWidth = 0;
	int w = 0;
	for (int i = 0; i < len; i++, s++)
	{
		if (*s == '\n')
		{
			maxWidth = MAX(maxWidth, w);
			w = 0;
		}
		else
		{
			w += FontW(*s);
		}
	}
	maxWidth = MAX(maxWidth, w);
	return maxWidth;
}
int FontStrH(const char *s)
{
	return FontStrNumLines(s) * FontH();
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
int FontStrNumLines(const char *s)
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
	return lines;
}

Vec2i FontCh(const char c, const Vec2i pos)
{
	return FontChMask(c, pos, colorWhite);
}
static Vec2i FontChColor(
	const char c, const Vec2i pos, const color_t color, const bool blend);
Vec2i FontChMask(const char c, const Vec2i pos, const color_t mask)
{
	return FontChColor(c, pos, mask, false);
}
static Vec2i FontChColor(
	const char c, const Vec2i pos, const color_t color, const bool blend)
{
	int idx = (int)c - FIRST_CHAR;
	if (idx < 0)
	{
		idx += 256;
	}
	if (idx < FIRST_CHAR || idx > LAST_CHAR)
	{
		fprintf(stderr, "invalid char %d\n", idx);
		idx = FIRST_CHAR;
	}
	const Pic *pic = CArrayGet(&gFont.Chars, idx);
	if (blend)
	{
		BlitBlend(&gGraphicsDevice, pic, pos, color);
	}
	else
	{
		BlitMasked(&gGraphicsDevice, pic, pos, color, true);
	}
	// Add gap between characters
	return Vec2iNew(pos.x + pic->size.x + gFont.Gap.x, pos.y);
}
Vec2i FontStr(const char *s, Vec2i pos)
{
	return FontStrMask(s, pos, colorWhite);
}
static Vec2i FontStrColor(
	const char *s, Vec2i pos, const color_t c, const bool blend);
Vec2i FontStrMask(const char *s, Vec2i pos, const color_t mask)
{
	return FontStrColor(s, pos, mask, false);
}
static Vec2i FontStrColor(
	const char *s, Vec2i pos, const color_t c, const bool blend)
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
			pos = FontChColor(*s, pos, c, blend);
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
static Vec2i GetStrPos(const char *s, Vec2i pos, const FontOpts opts);
void FontStrOpt(const char *s, Vec2i pos, const FontOpts opts)
{
	pos = GetStrPos(s, pos, opts);
	FontStrMask(s, pos, opts.Mask);
}
static int GetAlign(
	const FontAlign align,
	const int pos, const int pad, const int area, const int size);
static Vec2i GetStrPos(const char *s, Vec2i pos, const FontOpts opts)
{
	const Vec2i textSize = FontStrSize(s);
	return Vec2iNew(
		GetAlign(opts.HAlign, pos.x, opts.Pad.x, opts.Area.x, textSize.x),
		GetAlign(opts.VAlign, pos.y, opts.Pad.y, opts.Area.y, textSize.y));
}
static int GetAlign(
	const FontAlign align,
	const int pos, const int pad, const int area, const int size)
{
	switch (align)
	{
	case ALIGN_START:
		return pos + pad;
		break;
	case ALIGN_CENTER:
		return pos + (area - size) / 2;
		break;
	case ALIGN_END:
		return pos + area - size - pad;
		break;
	default:
		CASSERT(false, "unknown align");
		return pos;
	}
}

void FontStrCenter(const char *s)
{
	FontOpts opts = FontOptsNew();
	opts.HAlign = ALIGN_CENTER;
	opts.VAlign = ALIGN_CENTER;
	opts.Area = gGraphicsDevice.cachedConfig.Res;
	FontStrOpt(s, Vec2iZero(), opts);
}

void FontSplitLines(const char *text, char *buf, const int width)
{
	if (width == 0)
	{
		strcpy(buf, text);
		return;
	}
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
			*buf++ = *s;
			if (*s == '\n')
			{
				// We've already skipped a word, so reset the word count
				ws = s + 1;
				x = ix;
			}
			s++;
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
			w += FontW(*p);
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
			x += FontW(*p);
		}

		for (p = word; p < s; p++)
		{
			*buf++ = *p;
			x += FontW(*p);
		}
	}
	*buf = '\0';
}

Vec2i Vec2iAligned(
	const Vec2i v, const Vec2i size,
	const FontAlign hAlign, const FontAlign vAlign, const Vec2i area)
{
	Vec2i vAligned = v;
	switch (hAlign)
	{
	case ALIGN_CENTER:
		vAligned.x += (area.x - size.x) / 2;
		break;
	case ALIGN_END:
		vAligned.x = -v.x + area.x - size.x;
		break;
	default:
		break;
	}
	switch (vAlign)
	{
	case ALIGN_CENTER:
		vAligned.y += (area.y - size.y) / 2;
		break;
	case ALIGN_END:
		vAligned.y = -v.y + area.y - size.y;
		break;
	default:
		break;
	}
	return vAligned;
}
