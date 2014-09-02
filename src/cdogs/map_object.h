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

    This file incorporates work covered by the following copyright and
    permission notice:

    Copyright (c) 2013-2014, Cong Xu
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.
    Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
    ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
    LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
    INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
    CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
    ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGE.
*/
#ifndef __MAP_OBJECT
#define __MAP_OBJECT

#include "pic_manager.h"

#define MAPOBJ_EXPLOSIVE    (1 << 0)
#define MAPOBJ_IMPASSABLE   (1 << 1)
#define MAPOBJ_CANBESHOT    (1 << 2)
#define MAPOBJ_CANBETAKEN   (1 << 3)
#define MAPOBJ_ROOMONLY     (1 << 4)
#define MAPOBJ_NOTINROOM    (1 << 5)
#define MAPOBJ_FREEINFRONT  (1 << 6)
#define MAPOBJ_ONEWALL      (1 << 7)
#define MAPOBJ_ONEWALLPLUS  (1 << 8)
#define MAPOBJ_NOWALLS      (1 << 9)
#define MAPOBJ_HIDEINSIDE   (1 << 10)
#define MAPOBJ_INTERIOR     (1 << 11)
#define MAPOBJ_FLAMMABLE    (1 << 12)
#define MAPOBJ_POISONOUS    (1 << 13)
#define MAPOBJ_QUAKE        (1 << 14)
#define MAPOBJ_ON_WALL      (1 << 15)

#define MAPOBJ_OUTSIDE (MAPOBJ_IMPASSABLE | MAPOBJ_CANBESHOT | \
                        MAPOBJ_NOTINROOM | MAPOBJ_ONEWALL)
#define MAPOBJ_INOPEN (MAPOBJ_IMPASSABLE | MAPOBJ_CANBESHOT | \
                        MAPOBJ_NOTINROOM | MAPOBJ_NOWALLS)
#define MAPOBJ_INSIDE (MAPOBJ_IMPASSABLE | MAPOBJ_CANBESHOT | MAPOBJ_ROOMONLY)

// A static map object, taking up an entire tile
typedef struct
{
	int pic, wreckedPic;
	const char *picName;
	int width, height;
	int structure;
	int flags;
} MapObject;

MapObject *MapObjectGet(int item);
MapObject *MapObjectFindByPicId(const int picId);
int MapObjectGetCount(void);
int MapObjectGetDestructibleCount(void);
Pic *MapObjectGetPic(MapObject *mo, PicManager *pm, Vec2i *offset);

int MapObjectIsTileOK(
	MapObject *obj, unsigned short tile, int isEmpty, unsigned short tileAbove);
int MapObjectIsTileOKStrict(
	MapObject *obj, unsigned short tile, int isEmpty,
	unsigned short tileAbove, unsigned short tileBelow,
	int numWallsAdjacent, int numWallsAround);

#endif
