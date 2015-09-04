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
#include "tile.h"

#include "actors.h"
#include "objs.h"
#include "pickup.h"
#include "triggers.h"


Tile TileNone(void)
{
	Tile t;
	memset(&t, 0, sizeof t);
	t.pic = NULL;
	t.flags = MAPTILE_NO_WALK | MAPTILE_IS_NOTHING;
	return t;
}
void TileInit(Tile *t)
{
	memset(t, 0, sizeof *t);
	// lazy initialise the arrays of triggers
	// it's very slow to do 128x128 mallocs!
	t->pic = NULL;
	t->picAlt = NULL;
}
void TileDestroy(Tile *t)
{
	if (t->triggers.elemSize > 0)
	{
		CArrayTerminate(&t->triggers);
	}
	if (t->things.elemSize > 0)
	{
		CArrayTerminate(&t->things);
	}
}

bool IsTileItemInsideTile(TTileItem *i, Vec2i tilePos)
{
	return
		i->x - i->size.x / 2 >= tilePos.x * TILE_WIDTH &&
		i->x + i->size.x / 2 < (tilePos.x + 1) * TILE_WIDTH &&
		i->y - i->size.y / 2 >= tilePos.y * TILE_HEIGHT &&
		i->y + i->size.y / 2 < (tilePos.y + 1) * TILE_HEIGHT;
}

bool TileCanSee(Tile *t)
{
	return !(t->flags & MAPTILE_NO_SEE);
}
bool TileCanWalk(const Tile *t)
{
	return !(t->flags & MAPTILE_NO_WALK);
}
bool TileIsNormalFloor(Tile *t)
{
	return t->flags & MAPTILE_IS_NORMAL_FLOOR;
}
bool TileIsClear(Tile *t)
{
	// Check if tile is normal floor
	const int normalFloorFlags =
		MAPTILE_IS_NORMAL_FLOOR | MAPTILE_IS_DRAINAGE | MAPTILE_OFFSET_PIC;
	if (t->flags & ~normalFloorFlags) return false;
	// Check if tile has no things on it, excluding particles
	for (int i = 0; i < (int)t->things.size; i++)
	{
		const ThingId *tid = CArrayGet(&t->things, i);
		if (tid->Kind != KIND_PARTICLE) return false;
	}
	return true;
}
bool TileHasCharacter(Tile *t)
{
	for (int i = 0; i < (int)t->things.size; i++)
	{
		ThingId *tid = CArrayGet(&t->things, i);
		if (tid->Kind == KIND_CHARACTER)
		{
			return true;
		}
	}
	return false;
}

void TileSetAlternateFloor(Tile *t, NamedPic *p)
{
	t->pic = p;
	t->flags &= ~MAPTILE_IS_NORMAL_FLOOR;
}


TTileItem *ThingIdGetTileItem(ThingId *tid)
{
	TTileItem *ti = NULL;
	switch (tid->Kind)
	{
	case KIND_CHARACTER:
		ti = &((TActor *)CArrayGet(&gActors, tid->Id))->tileItem;
		break;
	case KIND_PARTICLE:
		ti = &((Particle *)CArrayGet(&gParticles, tid->Id))->tileItem;
		break;
	case KIND_MOBILEOBJECT:
		ti = &((TMobileObject *)CArrayGet(
			&gMobObjs, tid->Id))->tileItem;
		break;
	case KIND_OBJECT:
		ti = &((TObject *)CArrayGet(&gObjs, tid->Id))->tileItem;
		break;
	case KIND_PICKUP:
		ti = &((Pickup *)CArrayGet(&gPickups, tid->Id))->tileItem;
		break;
	default:
		CASSERT(false, "unknown tile item to get");
		break;
	}
	return ti;
}

bool TileItemIsDebris(const TTileItem *t)
{
	return t->flags & TILEITEM_IS_WRECK;
}
