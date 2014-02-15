/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (C) 1995 Ronny Wester
    Copyright (C) 2003 Jeremy Chin
    Copyright (C) 2003-2007 Lucas Martin-King

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    This file incorporates work covered by the following copyright and
    permission notice:

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
#include "text.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "grafx.h"
#include "blit.h"
#include "actors.h" /* for tableFlamed */

#define CHAR_INDEX(c) ((int)c - FIRST_CHAR)


static int dxCDogsText = 0;
static int xCDogsText = 0;
static int yCDogsText = 0;
static int hCDogsText = 0;

TextManager gTextManager;


void TextManagerInit(TextManager *tm, const char *filename)
{
	memset(tm, 0, sizeof *tm);
	dxCDogsText = -2;
	ReadPics(filename, tm->oldPics, CHARS_IN_FONT, NULL);

	for (int i = 0; i < CHARS_IN_FONT; i++)
	{
		if (tm->oldPics[i] != NULL)
		{
			hCDogsText = MAX(hCDogsText, tm->oldPics[i]->h);
		}
	}
}
void TextManagerGenerateOldPics(TextManager *tm, GraphicsDevice *g)
{
	// Convert old pics into new format ones
	// TODO: this is wasteful; better to eliminate old pics altogether
	// Note: always need to reload in editor since colours could change,
	// requiring an updating of palettes
	for (int i = 0; i < CHARS_IN_FONT; i++)
	{
		PicPaletted *oldPic = tm->oldPics[i];
		if (PicIsNotNone(&tm->picsFromOld[i]))
		{
			PicFree(&tm->picsFromOld[i]);
		}
		if (oldPic == NULL)
		{
			memcpy(&tm->picsFromOld[i], &picNone, sizeof picNone);
		}
		else
		{
			PicFromPicPaletted(g, &tm->picsFromOld[i], oldPic);
		}
	}
}

void TextManagerTerminate(TextManager *tm)
{
	for (int i = 0; i < CHARS_IN_FONT; i++)
	{
		if (tm->oldPics[i] != NULL)
		{
			CFREE(tm->oldPics[i]);
		}
		if (PicIsNotNone(&tm->picsFromOld[i]))
		{
			PicFree(&tm->picsFromOld[i]);
		}
	}
}

void CDogsTextChar(char c)
{
	int i = CHAR_INDEX(c);
	if (i >= 0 && i <= CHARS_IN_FONT && gTextManager.oldPics[i])
	{
		DrawTPic(xCDogsText, yCDogsText, gTextManager.oldPics[i]);
		xCDogsText += 1 + gTextManager.oldPics[i]->w + dxCDogsText;
	}
	else
	{
		i = CHAR_INDEX('.');
		DrawTPic(xCDogsText, yCDogsText, gTextManager.oldPics[i]);
		xCDogsText += 1 + gTextManager.oldPics[i]->w + dxCDogsText;
	}
}

void CDogsTextCharWithTable(char c, TranslationTable * table)
{
	int i = CHAR_INDEX(c);
	if (i >= 0 && i <= CHARS_IN_FONT && gTextManager.oldPics[i])
	{
		DrawTTPic(xCDogsText, yCDogsText, gTextManager.oldPics[i], table);
		xCDogsText += 1 + gTextManager.oldPics[i]->w + dxCDogsText;
	}
	else
	{
		i = CHAR_INDEX('.');
		DrawTTPic(xCDogsText, yCDogsText, gTextManager.oldPics[i], table);
		xCDogsText += 1 + gTextManager.oldPics[i]->w + dxCDogsText;
	}
}

void CDogsTextString(const char *s)
{
	while (*s)
		CDogsTextChar(*s++);
}

void CDogsTextStringWithTable(const char *s, TranslationTable * table)
{
	while (*s)
		CDogsTextCharWithTable(*s++, table);
}

static int GetFontPicIndex(char c)
{
	int i = CHAR_INDEX(c);
	if (i < 0 || i > CHARS_IN_FONT || !gTextManager.oldPics[i])
	{
		i = CHAR_INDEX('.');
	}
	assert(gTextManager.oldPics[i]);
	return i;
}
Vec2i TextCharMasked(
	TextManager *tm, char c, GraphicsDevice *device, Vec2i pos, color_t mask)
{
	Pic *fontPic = &tm->picsFromOld[GetFontPicIndex(c)];
	BlitMasked(device, fontPic, pos, mask, 1);
	pos.x += 1 + fontPic->size.x + dxCDogsText;
	CDogsTextGoto(pos.x, pos.y);
	return pos;
}

Vec2i TextStringMasked(
	TextManager *tm, const char *s,
	GraphicsDevice *device, Vec2i pos, color_t mask)
{
	int left = pos.x;
	while (*s)
	{
		if (*s == '\n')
		{
			pos.x = left;
			pos.y += CDogsTextHeight();
		}
		else
		{
			pos = TextCharMasked(tm, *s, device, pos, mask);
		}
		s++;
	}
	return pos;
}

Vec2i TextString(
	TextManager *tm, const char *s, GraphicsDevice *device, Vec2i pos)
{
	return TextStringMasked(tm, s, device, pos, colorWhite);
}

Vec2i TextStringMaskedWrapped(
	TextManager *tm, const char *s,
	GraphicsDevice *device, Vec2i pos, color_t mask, int width)
{
	char buf[1024];
	assert(strlen(s) < 1024);
	TextSplitLines(s, buf, width);
	return TextStringMasked(tm, buf, device, pos, mask);
}

Vec2i TextGetSize(const char *s)
{
	Vec2i size = Vec2iZero();
	while (*s)
	{
		char *lineEnd = strchr(s, '\n');
		size.y += CDogsTextHeight();
		if (lineEnd)
		{
			size.x = MAX(size.x, TextGetSubstringWidth(s, lineEnd - s));
			s = lineEnd + 1;
		}
		else
		{
			size.x = MAX(size.x, TextGetStringWidth(s));
			s += strlen(s);
		}
	}
	return size;
}

void CDogsTextGoto(int x, int y)
{
	xCDogsText = x;
	yCDogsText = y;
}

void CDogsTextStringAt(int x, int y, const char *s)
{
	CDogsTextGoto(x, y);
	CDogsTextString(s);
}

void CDogsTextIntAt(int x, int y, int i)
{
	char s[32];
	CDogsTextGoto(x, y);
	sprintf(s, "%d", i);
	CDogsTextString(s);
}

void CDogsTextFormatAt(int x, int y, const char *fmt, ...)
{
	char s[256];
	va_list argptr;
	va_start(argptr, fmt);
	vsprintf(s, fmt, argptr);
	va_end(argptr);
	CDogsTextGoto(x, y);
	CDogsTextString(s);
}

void CDogsTextStringWithTableAt(int x, int y, const char *s,
			   TranslationTable * table)
{
	CDogsTextGoto(x, y);
	CDogsTextStringWithTable(s, table);
}

int CDogsTextCharWidth(int c)
{
	if (c >= FIRST_CHAR && c <= LAST_CHAR && gTextManager.oldPics[CHAR_INDEX(c)])
	{
		return 1 + gTextManager.oldPics[CHAR_INDEX(c)]->w + dxCDogsText;
	}
	else
	{
		return 1 + gTextManager.oldPics[CHAR_INDEX('.')]->w + dxCDogsText;
	}
}

int TextGetSubstringWidth(const char *s, int len)
{
	int w = 0;
	int i;
	if (len > (int)strlen(s))
	{
		len = (int)strlen(s);
	}
	for (i = 0; i < len; i++)
	{
		w += CDogsTextCharWidth(*s++);
	}
	return w;
}

int TextGetStringWidth(const char *s)
{
	return TextGetSubstringWidth(s, (int)strlen(s));
}

#define FLAG_SET(a, b)	((a & b) != 0)

void DrawTextStringSpecialMasked(
	TextManager *tm, const char *s,
	GraphicsDevice *device, unsigned int opts,
	Vec2i pos, Vec2i size, Vec2i padding,
	color_t mask)
{
	int x = 0;
	int y = 0;
	int w = TextGetStringWidth(s);
	int h = CDogsTextHeight();
	
	if (FLAG_SET(opts, TEXT_XCENTER))	{ x = pos.x + (size.x - w) / 2; }
	if (FLAG_SET(opts, TEXT_YCENTER))	{ y = pos.y + (size.y - h) / 2; }
	
	if (FLAG_SET(opts, TEXT_LEFT))		{ x = pos.x + padding.x; }
	if (FLAG_SET(opts, TEXT_RIGHT))		{ x = pos.x + size.x - w - padding.x; }
	
	if (FLAG_SET(opts, TEXT_TOP))		{ y = pos.y + padding.y; }
	if (FLAG_SET(opts, TEXT_BOTTOM))	{ y = pos.y + size.y - h - padding.y; }

	pos = TextStringMasked(tm, s, device, Vec2iNew(x, y), mask);
	CDogsTextGoto(pos.x, pos.y);
}
void DrawTextStringSpecial(
	const char *s, unsigned int opts, Vec2i pos, Vec2i size, Vec2i padding)
{
	if (FLAG_SET(opts, TEXT_FLAMED))
	{
		DrawTextStringSpecialMasked(
			&gTextManager, s, &gGraphicsDevice, opts, pos, size, padding,
			colorRed);
	}
	else if (FLAG_SET(opts, TEXT_PURPLE))
	{
		DrawTextStringSpecialMasked(
			&gTextManager, s, &gGraphicsDevice, opts, pos, size, padding,
			colorPurple);
	}
	else
	{
		DrawTextStringSpecialMasked(
			&gTextManager, s, &gGraphicsDevice, opts, pos, size, padding,
			colorWhite);
	}
}

void CDogsTextStringSpecial(const char *s, unsigned int opts, unsigned int xpad, unsigned int ypad)
{
	int scrw = gGraphicsDevice.cachedConfig.ResolutionWidth;
	int scrh = gGraphicsDevice.cachedConfig.ResolutionHeight;
	DrawTextStringSpecial(
		s, opts, Vec2iZero(), Vec2iNew(scrw, scrh), Vec2iNew(xpad, ypad));
}

int CDogsTextHeight(void)
{
	assert(hCDogsText && "text not initialised");
	return hCDogsText;
}

void TextSplitLines(const char *text, char *buf, int width)
{
	int w, ix, x;
	const char *ws, *word, *p, *s;

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
		for (w = 0, p = ws; p < s; p++)
		{
			w += CDogsTextCharWidth(*p);
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
			x += CDogsTextCharWidth(*p);
		}

		for (p = word; p < s; p++)
		{
			*buf++ = *p;
			x += CDogsTextCharWidth(*p);
		}
	}
	*buf = '\0';
}

char *PercentStr(int p)
{
	static char buf[8];
	sprintf(buf, "%d%%", p);
	return buf;
}
char *Div8Str(int i)
{
	static char buf[8];
	sprintf(buf, "%d", i/8);
	return buf;
}
