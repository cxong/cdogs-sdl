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

 draw-tools.h - various drawing utilities
 
 Author: $Author$
 Rev:    $Revision$
 URL:    $HeadURL$
 ID:     $Id$
 
*/

#ifndef __DRAW_TOOLS
#define __DRAW_TOOLS

void	Draw_Point (const int x, const int y, const unsigned char c);
void	Draw_Line  (const int x1, const int y1, const int x2, const int y2, const unsigned char c);

#define PixelIndex(x, y, w, h)		(y * w + x)

#define Draw_Box(x1, y1, x2, y2, c)		\
		Draw_Line(x1, y1, x2, y1, c);	\
		Draw_Line(x2, y1, x2, y2, c);	\
		Draw_Line(x1, y2, x2, y2, c);	\
		Draw_Line(x1, y1, x1, y2, c);

#define Draw_Rect(x, y, w, h, c)	Draw_Box(x,y,((x + (w - 1))),((y + (h - 1))),c)

#endif /* __DRAW_TOOLS */
