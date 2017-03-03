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

#include "pics.h"


const char *IntWallType(const int i)
{
	static const char *wallTypes[] = {
		"o", "w", "e", "n", "s", "nw", "ne", "sw", "se",
		"wt", "et", "nt", "st", "v", "h", "x"
	};
	return wallTypes[i];
}
const char *IntWallStyle(const int i)
{
	static const char *wallStyles[] = {
		"steel", "brick", "carbon", "steelwood", "stone", "plasteel",
		"granite"
	};
	return wallStyles[abs(i) % WALL_STYLE_COUNT];
}

const char *IntTileType(const int i)
{
	static const char *tileTypes[] = { "shadow", "normal", "alt1", "alt2" };
	return tileTypes[abs(i) % FLOOR_TYPES];
}
const char *IntFloorStyle(const int i)
{
	static const char *floorStyles[] = {
		"recessed", "tile", "dirt", "flat", "striped", "smallsquare", "stone",
		"wood", "biggrid", "grid"
	};
	return floorStyles[abs(i) % FLOOR_STYLE_COUNT];
}
// Note: room styles subtly different from floor styles
const char *IntRoomStyle(const int i)
{
	static const char *roomStyles[] = {
		"recessed", "tile", "dirt", "flat", "striped", "smallsquare", "stone",
		"wood", "grid", "biggrid", "checker"
	};
	return roomStyles[abs(i) % ROOM_STYLE_COUNT];
}
