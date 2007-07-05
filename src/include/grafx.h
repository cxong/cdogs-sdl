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
 
 Author: $Author$
 Rev:    $Revision$
 URL:    $HeadURL$
 ID:     $Id$
 
*/

#ifndef __grafx
#define __grafx


struct RGB {
	unsigned char red, green, blue;
};
typedef struct RGB color;
typedef color TPalette[256];
typedef unsigned char TranslationTable[256];

typedef enum {
	HINT_FULLSCREEN,
	HINT_WINDOW,
	HINT_SCALEFACTOR,
	HINT_WIDTH,
	HINT_HEIGHT,
	HINT_FORCEMODE,
	HINT_END
} GFX_Hint;

void SetColorZero(int r, int g, int b);

typedef struct {
	unsigned int w, h;
} GFX_Mode;

GFX_Mode * Gfx_ModePrev(void);
GFX_Mode * Gfx_ModeNext(void);

void Gfx_SetHint(const GFX_Hint h, const int val);
#define Gfx_HintOn(h)	Gfx_SetHint(h, 1)
#define Gfx_HintOff(h)	Gfx_SetHint(h, 0)
#define Gfx_HintToggle(h) Gfx_SetHint(h, !Gfx_GetHint(h))
int Gfx_GetHint(const GFX_Hint h);

int InitVideo(void);
void ShutDownVideo(void);

int ReadPics(const char *filename, void **pics, int maxPics,
	     color * palette);
int AppendPics(const char *filename, void **pics, int startIndex,
	       int maxPics);
int CompilePics(int picCount, void **pics, void **compiledPics);
int RLEncodePics(int picCount, void **pics, void **rlePics);

#ifndef _MSC_VER
inline int PicWidth(void *pic);
inline int PicHeight(void *pic);
#else
	int PicWidth(void *pic);
	int PicHeight(void *pic);
#endif

extern int screen_w;
extern int screen_h;

#define Screen_GetWidth()	(screen_w)
#define Screen_GetHeight()	(screen_h)
#define Screen_GetMemSize() (screen_w * screen_h)

#define	SCREEN_WIDTH	Screen_GetWidth()
#define	SCREEN_HEIGHT	Screen_GetHeight()
#define SCREEN_MEMSIZE	Screen_GetMemSize()

#define CenterX(w)		((SCREEN_WIDTH - w) / 2)
#define CenterY(h)		((SCREEN_HEIGHT - h) / 2)

#define CenterOf(a, b, w)	((a + (((b - a) - w) / 2)))

#define CenterOfRight(w)	CenterOf((SCREEN_WIDTH / 2), (SCREEN_WIDTH), w)
#define CenterOfLeft(w)		CenterOf(0, (SCREEN_WIDTH / 2), w)

#endif
