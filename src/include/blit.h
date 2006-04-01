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

 blit.h - <description here>

*/

// blit.h (blit.asm)

#ifndef __blit
#define __blit

#define BLIT_TRANSPARENT 1
#define BLIT_BACKGROUND 2

void Blit(int x, int y, void *pic, void *table, int mode);
/* DrawPic - simply draws a rectangular picture to screen. I do not
 * remember if this is the one that ignores zero source-pixels or not, but
 * that much should be obvious.
 */
#define DrawPic(x, y, pic, code) (Blit(x, y, pic, NULL, 0))
/* 
 * DrawTPic - I think the T here stands for transparent, ie ignore zero
 * source pixels when copying data.
 */
#define DrawTPic(x, y, pic, code) (Blit(x, y, pic, NULL, BLIT_TRANSPARENT))
/*
 * DrawTTPic - I think this stands for translated transparent. What this
 * does is that for each source pixel that would be copied it will first
 * translate the value by looking it up in the provided table. This means
 * that you can provide a 256 byte table to change any or all colors of
 * the source image. This feature is used heavily in the game.
 */
#define DrawTTPic(x, y, pic, table, rle) (Blit(x, y, pic, table, BLIT_TRANSPARENT))
/* 
 * DrawBTPic - I think the B stands for background here. If I remember
 * correctly this one uses the sourc eimage only as a mask. If a pixel in
 * the image is non-zero, look at the value at the destination and
 * translate that value through the table and put it back. This is used to
 * do the "invisible" guys as well as the gas clouds.
 */
#define DrawBTPic(x, y, pic, table, rle) (Blit(x, y, pic, table, BLIT_TRANSPARENT | BLIT_BACKGROUND))

/*
void DrawPic(int x, int y, void *pic, void *code);
void DrawTPic(int x, int y, void *pic, void *code);
void DrawTTPic(int x, int y, void *pic, void *table, void *rle);
void DrawBTPic(int x, int y, void *pic, void *table, void *rle); */
void SetClip(int left, int top, int right, int bottom);
void SetDstScreen(void *the_screen);
void *GetDstScreen(void);
void CopyToScreen(void);
void AltScrCopy(void);
void SetPalette(void *palette);
#endif
