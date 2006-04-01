/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (C) 1995 Ronny Wester
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

 sprcomp.c - sprite stuff

*/

#include <stdio.h>

#define MOV_off_0_dw	0x07C7
#define MOV_off_1_dw	0x47C7
#define MOV_off_4_dw	0x87C7

#define WORD_OVERRIDE	0x66
#define MOV_off_0_w		0x07C7
#define MOV_off_1_w		0x47C7
#define MOV_off_4_w		0x87C7

#define MOV_off_0_b		0x07C6
#define MOV_off_1_b		0x47C6
#define MOV_off_4_b		0x87C6

#define RET				0xC3

#define ADD_EDI_4		0xC781

#define SCREEN_WIDTH	320

#define DWORD(x)		(*((unsigned int *)x))
#define WORD(x)			(*((unsigned short *)x))
#define BYTE(x)			(*((unsigned char *)x))


int SizeDWMov(int x)
{
	if (x == 0)
		return 6;
	else if (x < 256)
		return 7;
	else
		return 10;
}

void EmitDWMov(int x, unsigned char *p, unsigned char **storage)
{
	if (!(*storage))
		return;

	if (x == 0) {
		WORD(*storage) = MOV_off_0_dw;
		*storage += 2;
	} else if (x < 256) {
		WORD(*storage) = MOV_off_1_dw;
		*storage += 2;
		BYTE(*storage) = (unsigned char) x;
		(*storage)++;
	} else {
		WORD(*storage) = MOV_off_4_dw;
		(*storage) += 2;
		DWORD(*storage) = x;
		(*storage) += 4;
	}
	DWORD(*storage) = DWORD(p);
	*storage += 4;
}

int SizeWMov(int x)
{
	if (x == 0)
		return 5;
	else if (x < 256)
		return 6;
	else
		return 9;
}

void EmitWMov(int x, unsigned char *p, unsigned char **storage)
{
	if (!(*storage))
		return;

	BYTE(*storage) = WORD_OVERRIDE;
	(*storage)++;

	if (x == 0) {
		WORD(*storage) = MOV_off_0_w;
		*storage += 2;
	} else if (x < 256) {
		WORD(*storage) = MOV_off_1_w;
		*storage += 2;
		BYTE(*storage) = (unsigned char) x;
		(*storage)++;
	} else {
		WORD(*storage) = MOV_off_4_w;
		*storage += 2;
		DWORD(*storage) = x;
		*storage += 4;
	}
	WORD(*storage) = WORD(p);
	*storage += 2;
}

int SizeBMov(int x)
{
	if (x == 0)
		return 3;
	else if (x < 256)
		return 4;
	else
		return 7;
}

void EmitBMov(int x, unsigned char *p, unsigned char **storage)
{
	if (!(*storage))
		return;

	if (x == 0) {
		WORD(*storage) = MOV_off_0_b;
		*storage += 2;
	} else if (x < 256) {
		WORD(*storage) = MOV_off_1_b;
		*storage += 2;
		BYTE(*storage) = (unsigned char) x;
		(*storage)++;
	} else {
		WORD(*storage) = MOV_off_4_b;
		*storage += 2;
		DWORD(*storage) = x;
		*storage += 4;
	}
	BYTE(*storage) = *p;
	(*storage)++;
}

void EmitAddEDI(int inc, unsigned char **storage)
{
	if (!(*storage))
		return;

	WORD(*storage) = ADD_EDI_4;
	*storage += 2;
	DWORD(*storage) = inc;
	*storage += 4;
}

void EmitRet(unsigned char **storage)
{
	if (!(*storage))
		return;

	BYTE(*storage) = RET;
	(*storage)++;
}

int compileSprite(void *sprite, unsigned char *ml)
{
	int x, y, w, h;
	unsigned char *p;
	unsigned short *s;
	int size = 0;
	int tCount = 0;

	s = sprite;
	w = *s++;
	h = *s++;
	p = (unsigned char *) s;

	for (y = 0; y < h; y++) {
		x = 0;
		while (x < w) {
			if (x + 3 < w && *p && *(p + 1) && *(p + 2)
			    && *(p + 3)) {
				EmitDWMov(x, p, &ml);
				size += SizeDWMov(x);
				x += 4;
				p += 4;
			} else if (x + 1 < w && *p && *(p + 1)) {
				EmitWMov(x, p, &ml);
				size += SizeWMov(x);
				x += 2;
				p += 2;
			} else if (*p) {
				EmitBMov(x, p, &ml);
				size += SizeBMov(x);
				x++;
				p++;
			} else {
				tCount++;
				x++;
				p++;
			}
		}
		if (y + 1 < h) {
			EmitAddEDI(SCREEN_WIDTH, &ml);
			size += 6;
		}
	}
	EmitRet(&ml);
	size++;

	return tCount ? size : 0;
}

int RLEncodeSprite(void *sprite, unsigned char *rle)
{
	int x, y, w, h;
	unsigned char *p;
	unsigned short *s;
	int size = 0;
	int l = 0;
	int transparent;
	unsigned char *lengthLoc;
	int tCount = 0;

	s = sprite;
	w = *s++;
	h = *s++;
	p = (unsigned char *) s;

	if (*p) {
		lengthLoc = rle;
		if (rle)
			rle++;
		transparent = 0;
	} else {
		if (rle)
			*rle++ = 0;
		transparent = 1;
	}
	size++;

	y = 0;
	while (y < h) {
		x = 0;
		while (x < w) {
			if (*p) {
				if (transparent) {
					if (rle) {
						*rle++ = (l & 0xFF);
						*rle++ = (l >> 8);
						lengthLoc = rle;
						rle++;
					}
					size += 3;
					transparent = 0;
					l = 0;
				}
				if (rle)
					*rle++ = *p;
				size++;
				l++;
			} else {
				tCount++;
				if (!transparent) {
					if (l > 255) {
						printf
						    ("Non-transparent run exceeds 255 pixels.\n");
						return 0;
					}
					if (rle)
						*lengthLoc = (l & 0xFF);
					transparent = 1;
					l = 0;
				}
				l++;
			}
			x++;
			p++;
		}
		if (!transparent) {
			if (l > 255) {
				printf
				    ("Non-transparent run exceeds 255 pixels.\n");
				return 0;
			}
			if (rle)
				*lengthLoc = (l & 0xFF);
			transparent = 1;
			l = SCREEN_WIDTH - w;
		} else
			l += SCREEN_WIDTH - w;
		y++;
	}
	if (rle) {
		*rle++ = 0;
		*rle++ = 0;
	}
	size += 2;

	return tCount ? size : 0;
}
