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

 draw-tools.c - miscellaneous drawing functions 
 
 Author: $Author$
 Rev:    $Revision$
 URL:    $HeadURL$
 ID:     $Id$
 
*/ 

#include <stdio.h>

#include <drawtools.h>
#include "blit.h"
#include "grafx.h"


void	Draw_Point (const int x, const int y, const unsigned char c)
{
	unsigned char *screen = GetDstScreen();
	screen[PixelIndex(x, y, SCREEN_WIDTH, SCREEN_HEIGHT)] = c;
}

static
void
Draw_StraightLine (const int x1, const int y1, const int x2, const int y2, const unsigned char c)
{
	register int i;
	
	if (x1 == x2) {						/* vertical line */
		for (i = y1; i <= y2; i++) {
			Draw_Point(x1, i, c);
		}
	} else if (y1 == y2) {				/* horizontal line */
		for (i = x1; i <= x2; i++) {
			Draw_Point(i, y1, c);
		}
	}
}

void	Draw_Line  (const int x1, const int y1, const int x2, const int y2, const unsigned char c)
{
	register int i;
	
	if (x1 == x2 || y1 == y2) 
		Draw_StraightLine(x1, y1, x2, y2, c);
	else
		printf("### XXX: Diagonal lines not implemented yet! ###\n");
}
