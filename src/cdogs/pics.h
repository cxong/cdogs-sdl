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
#pragma once

#define WALL_TYPE_COUNT 16
const char *IntWallType(const int i);

#define WALL_STYLE_COUNT 7
// Legacy wall type int to str
const char *IntWallStyle(const int i);

#define FLOOR_TYPES 4
#define ROOMFLOOR_TYPES 2
// Legacy floor/room tile type int to str
const char *IntTileType(const int i);

#define FLOOR_STYLE_COUNT 10
const char *IntFloorStyle(const int i);

#define ROOM_STYLE_COUNT 11
const char *IntRoomStyle(const int i);
