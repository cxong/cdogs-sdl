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
#ifndef __TEXT
#define __TEXT

#include "grafx.h"
#include "pic.h"
#include "pic_file.h"
#include "vector.h"

#define FIRST_CHAR      0
#define LAST_CHAR       153
#define CHARS_IN_FONT   (LAST_CHAR - FIRST_CHAR + 1)

typedef struct
{
	PicPaletted *oldPics[CHARS_IN_FONT];
	Pic picsFromOld[CHARS_IN_FONT];
} TextManager;

extern TextManager gTextManager;

void TextManagerInit(TextManager *tm, const char *filename);
// Old paletted pics need the palette to be set before using
void TextManagerGenerateOldPics(TextManager *tm, GraphicsDevice *g);
void TextManagerTerminate(TextManager *tm);
void CDogsTextGoto(int x, int y);

#define TEXT_XCENTER		1
#define TEXT_YCENTER		2
#define TEXT_LEFT		4
#define TEXT_RIGHT		8
#define TEXT_TOP		16
#define TEXT_BOTTOM		32
#define TEXT_FLAMED		64
#define TEXT_PURPLE		128

void DrawTextStringSpecialBlend(
	TextManager *tm, const char *s,
	GraphicsDevice *device, unsigned int opts,
	Vec2i pos, Vec2i size, Vec2i padding,
	color_t blend);
void DrawTextStringSpecial(
	const char *s, unsigned int opts, Vec2i pos, Vec2i size, Vec2i padding);
void CDogsTextStringSpecial(const char *s, unsigned int opts, unsigned int xpad, unsigned int ypad);
#define CDogsTextStringAtCenter(s)	CDogsTextStringSpecial(s, TEXT_XCENTER | TEXT_YCENTER, 0, 0)

char *PercentStr(int p);
char *Div8Str(int i);

#endif
