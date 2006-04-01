/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (C) 1995 Ronny Webster
    Copyright (C) 2003 Jeremy Chin 
    Copyright (C) 2003 Lucas Martin-King 

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

 text.h - <description here>

*/

void TextInit(const char *filename, int offset, int compile, int rle);
void TextChar(char c);
void TextString(const char *s);
void TextGoto(int x, int y);
void TextStringAt(int x, int y, const char *s);
int TextCharWidth(int c);
int TextWidth(const char *s);
int TextHeight(void);
void TextCharWithTable(char c, TranslationTable * table);
void TextStringWithTable(const char *s, TranslationTable * table);
void TextStringWithTableAt(int x, int y, const char *s,
			   TranslationTable * table);
