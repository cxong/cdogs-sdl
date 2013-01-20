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

 menu.c - menu misc functions 
 
 Author: $Author$
 Rev:    $Revision$
 URL:    $HeadURL$
 ID:     $Id$
 
*/

#include <string.h>

#include "grafx.h"
#include "text.h"
#include "actors.h"


void ShowControls(void)
{
	CDogsTextStringSpecial("(use player 1 controls or arrow keys + Enter/Backspace)", TEXT_BOTTOM | TEXT_XCENTER, 0, 10);
}

void DisplayMenuItem(int x, int y, const char *s, int selected)
{
	if (selected)
		CDogsTextStringWithTableAt(x, y, s, &tableFlamed);
	else
		CDogsTextStringAt(x, y, s);
		
	return;
}

int  MenuWidth(const char **table, int count)
{
	int i;
	int len, max;
	
	len = max = 0;
	
	for (i = 0; i < count; i++) {
		if ( ( len = CDogsTextWidth(table[i]) ) > max)
			max = len;
	}
	
	return max;
}

int MenuHeight(int count)
{
	return count * CDogsTextHeight();
}

void DisplayMenuAt(int x, int y, const char **table, int count, int index)
{
	int i;

	for (i = 0; i < count; i++) {
		DisplayMenuItem(x, y + i * CDogsTextHeight(), table[i], i == index);
	}
	
	return;
}

#define MENU_OFFSET_Y	50

void DisplayMenu(int x, const char **table, int count, int index)
{
	DisplayMenuAt(x, MENU_OFFSET_Y, table, count, index);
	return;
}

