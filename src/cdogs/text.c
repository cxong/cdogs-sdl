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
#include "font.h"
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
Vec2i TextCharBlend(
	TextManager *tm, char c, GraphicsDevice *device, Vec2i pos, color_t blend)
{
	Pic *fontPic = &tm->picsFromOld[GetFontPicIndex(c)];
	BlitBlend(device, fontPic, pos, blend);
	pos.x += 1 + fontPic->size.x + dxCDogsText;
	CDogsTextGoto(pos.x, pos.y);
	return pos;
}


static Vec2i TextStringFunc(
	TextManager *tm, const char *s,
	GraphicsDevice *device, Vec2i pos, color_t color,
	Vec2i (*textCharFunc)(TextManager *, char, GraphicsDevice *, Vec2i, color_t))
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
			pos = textCharFunc(tm, *s, device, pos, color);
		}
		s++;
	}
	return pos;
}
Vec2i TextStringBlend(
	TextManager *tm, const char *s,
	GraphicsDevice *device, Vec2i pos, color_t blend)
{
	return TextStringFunc(tm, s, device, pos, blend, TextCharBlend);
}

void CDogsTextGoto(int x, int y)
{
	xCDogsText = x;
	yCDogsText = y;
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
		w += FontW(*s++);
	}
	return w;
}

#define FLAG_SET(a, b)	((a & b) != 0)

static Vec2i GetSpecialTextPos(
	const char *s, unsigned int opts, Vec2i pos, Vec2i size, Vec2i padding);
void DrawTextStringSpecialBlend(
	TextManager *tm, const char *s,
	GraphicsDevice *device, unsigned int opts,
	Vec2i pos, Vec2i size, Vec2i padding,
	color_t blend)
{
	pos = TextStringBlend(
		tm, s, device, GetSpecialTextPos(s, opts, pos, size, padding), blend);
	CDogsTextGoto(pos.x, pos.y);
}
static void DrawTextStringSpecialMasked(
	const char *s, unsigned int opts,
	Vec2i pos, Vec2i size, Vec2i padding, color_t mask)
{
	pos = FontStrMask(s, GetSpecialTextPos(s, opts, pos, size, padding), mask);
	CDogsTextGoto(pos.x, pos.y);
}
static Vec2i GetSpecialTextPos(
	const char *s, unsigned int opts, Vec2i pos, Vec2i size, Vec2i padding)
{
	int x = 0;
	int y = 0;
	const int w = FontStrW(s);
	const int h = FontH();

	if (FLAG_SET(opts, TEXT_XCENTER))	{ x = pos.x + (size.x - w) / 2; }
	if (FLAG_SET(opts, TEXT_YCENTER))	{ y = pos.y + (size.y - h) / 2; }

	if (FLAG_SET(opts, TEXT_LEFT))		{ x = pos.x + padding.x; }
	if (FLAG_SET(opts, TEXT_RIGHT))		{ x = pos.x + size.x - w - padding.x; }

	if (FLAG_SET(opts, TEXT_TOP))		{ y = pos.y + padding.y; }
	if (FLAG_SET(opts, TEXT_BOTTOM))	{ y = pos.y + size.y - h - padding.y; }

	return Vec2iNew(x, y);
}

void DrawTextStringSpecial(
	const char *s, unsigned int opts, Vec2i pos, Vec2i size, Vec2i padding)
{
	if (FLAG_SET(opts, TEXT_FLAMED))
	{
		DrawTextStringSpecialMasked(s, opts, pos, size, padding, colorRed);
	}
	else if (FLAG_SET(opts, TEXT_PURPLE))
	{
		DrawTextStringSpecialMasked(s, opts, pos, size, padding, colorPurple);
	}
	else
	{
		DrawTextStringSpecialMasked(s, opts, pos, size, padding, colorWhite);
	}
}

void CDogsTextStringSpecial(const char *s, unsigned int opts, unsigned int xpad, unsigned int ypad)
{
	int scrw = gGraphicsDevice.cachedConfig.Res.x;
	int scrh = gGraphicsDevice.cachedConfig.Res.y;
	DrawTextStringSpecial(
		s, opts, Vec2iZero(), Vec2iNew(scrw, scrh), Vec2iNew(xpad, ypad));
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
