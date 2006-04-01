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

 grafx.h - <description here>

*/

#ifndef __grafx
#define __grafx


struct RGB {
	unsigned char red, green, blue;
};
typedef struct RGB color;
typedef color TPalette[256];
typedef unsigned char TranslationTable[256];

#define VID_FULLSCREEN	1
#define VID_WIN_NORMAL	2
#define VID_WIN_SCALE	3

void SetColorZero(int r, int g, int b);

int InitVideo(int mode);
void TextMode(void);
int ReadPics(const char *filename, void **pics, int maxPics,
	     color * palette);
int AppendPics(const char *filename, void **pics, int startIndex,
	       int maxPics);
int CompilePics(int picCount, void **pics, void **compiledPics);
int RLEncodePics(int picCount, void **pics, void **rlePics);
void vsync(void);
int PicWidth(void *pic);
int PicHeight(void *pic);

#endif
