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

-------------------------------------------------------------------------------

 text.c - text rendering and related functions
 
 Author: $Author$
 Rev:    $Revision$
 URL:    $HeadURL$
 ID:     $Id$
 
*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "grafx.h"
#include "blit.h"
#include "text.h"
#include "actors.h" /* for tableFlamed */

#define FIRST_CHAR      0
#define LAST_CHAR       153
#define CHARS_IN_FONT   (LAST_CHAR - FIRST_CHAR + 1)

#define CHAR_INDEX(c) ((int)c - FIRST_CHAR)


static int dxCDogsText = 0;
static int xCDogsText = 0;
static int yCDogsText = 0;
static int hCDogsText = 0;
static void *font[CHARS_IN_FONT];
static void *compiledFont[CHARS_IN_FONT];
static void *rleFont[CHARS_IN_FONT];


void CDogsTextInit(const char *filename, int offset)
{
	int i, h;

	dxCDogsText = offset;
	memset(font, 0, sizeof(font));
	memset(compiledFont, 0, sizeof(compiledFont));
	memset(rleFont, 0, sizeof(rleFont));
	ReadPics(filename, font, CHARS_IN_FONT, NULL);

	for (i = 0; i < CHARS_IN_FONT; i++) {
		h = PicHeight(font[i]);
		if (h > hCDogsText)
			hCDogsText = h;
	}
}

void CDogsTextChar(char c)
{
	int i = CHAR_INDEX(c);
	if (i >= 0 && i <= CHARS_IN_FONT && font[i]) {
		DrawTPic(xCDogsText, yCDogsText, font[i], compiledFont[i]);
		xCDogsText += 1 + PicWidth(font[i]) + dxCDogsText;
	} else {
		i = CHAR_INDEX('.');
		DrawTPic(xCDogsText, yCDogsText, font[i], compiledFont[i]);
		xCDogsText += 1 + PicWidth(font[i]) + dxCDogsText;
	}
}

void CDogsTextCharWithTable(char c, TranslationTable * table)
{
	int i = CHAR_INDEX(c);
	if (i >= 0 && i <= CHARS_IN_FONT && font[i]) {
		DrawTTPic(xCDogsText, yCDogsText, font[i], table, rleFont[i]);
		xCDogsText += 1 + PicWidth(font[i]) + dxCDogsText;
	} else {
		i = CHAR_INDEX('.');
		DrawTTPic(xCDogsText, yCDogsText, font[i], table, rleFont[i]);
		xCDogsText += 1 + PicWidth(font[i]) + dxCDogsText;
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

void CDogsTextStringWithTableAt(int x, int y, const char *s,
			   TranslationTable * table)
{
	CDogsTextGoto(x, y);
	CDogsTextStringWithTable(s, table);
}

int CDogsTextCharWidth(int c)
{
	if (c >= FIRST_CHAR && c <= LAST_CHAR && font[CHAR_INDEX(c)])
		return 1 + PicWidth(font[CHAR_INDEX(c)]) + dxCDogsText;
	else
		return 1 + PicWidth(font[CHAR_INDEX('.')]) + dxCDogsText;
}

int CDogsTextWidth(const char *s)
{
	int w = 0;

	while (*s)
		w += CDogsTextCharWidth(*s++);
	return w;
}

#define FLAG_SET(a, b)	((a & b) != 0)

void CDogsTextStringSpecial(const char *s, unsigned int opts, unsigned int xpad, unsigned int ypad)
{
	int scrw = SCREEN_WIDTH;
	int scrh = SCREEN_HEIGHT;
	int x, y, w, h;
	
	x = y = w = h = 0;
	w = CDogsTextWidth(s);
	h = CDogsTextHeight();
	
	if (FLAG_SET(opts, TEXT_XCENTER))	{ x = (scrw - w) / 2; }
	if (FLAG_SET(opts, TEXT_YCENTER))	{ y = (scrh - h) / 2; }
	
	if (FLAG_SET(opts, TEXT_LEFT))		{ x = 0 + xpad; }
	if (FLAG_SET(opts, TEXT_RIGHT))		{ x = scrw - w - xpad; }
	
	if (FLAG_SET(opts, TEXT_TOP))		{ y = 0 + ypad; }
	if (FLAG_SET(opts, TEXT_BOTTOM))	{ y = scrh - h - ypad; }

	if (FLAG_SET(opts, TEXT_FLAMED)) {
		CDogsTextStringWithTableAt(x, y, s, &tableFlamed);
	} else if (FLAG_SET(opts, TEXT_PURPLE)) {
		CDogsTextStringWithTableAt(x, y, s, &tablePurple);
	} else {
		CDogsTextStringAt(x, y, s);
	}
}

int CDogsTextHeight(void)
{
	return hCDogsText;
}
