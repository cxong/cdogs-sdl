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
*/
#include "grafx.h"

void CDogsTextInit(const char *filename, int offset);
void CDogsTextChar(char c);
void CDogsTextString(const char *s);
void CDogsTextGoto(int x, int y);
void CDogsTextStringAt(int x, int y, const char *s);
void CDogsTextIntAt(int x, int y, int i);
int CDogsTextCharWidth(int c);
int CDogsTextWidth(const char *s);
int CDogsTextHeight(void);
void CDogsTextCharWithTable(char c, TranslationTable * table);
void CDogsTextStringWithTable(const char *s, TranslationTable * table);
void CDogsTextStringWithTableAt(int x, int y, const char *s,
			   TranslationTable * table);
			   
#define TEXT_XCENTER		1
#define TEXT_YCENTER		2
#define TEXT_LEFT		4
#define TEXT_RIGHT		8
#define TEXT_TOP		16
#define TEXT_BOTTOM		32
#define TEXT_FLAMED		64
#define TEXT_PURPLE		128

void CDogsTextStringSpecial(const char *s, unsigned int opts, unsigned int xpad, unsigned int ypad);
#define CDogsTextStringAtCenter(s)	CDogsTextStringSpecial(s, TEXT_XCENTER | TEXT_YCENTER, 0, 0)
