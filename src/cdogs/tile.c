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

	Copyright (c) 2013-2014, 2016-2020, 2022 Cong Xu
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

#include "thing.h"
#include "triggers.h"

void DoorStateInit(DoorState *d, const bool isOpen)
{
	d->Count = DOORSTATE_COUNT;
	d->IsOpen = isOpen;
}

Tile TileNone(void)
{
	Tile t;
	TileInit(&t);
	t.Class = &gTileNothing;
	return t;
}
void TileInit(Tile *t)
{
	memset(t, 0, sizeof *t);
	CArrayInit(&t->triggers, sizeof(Trigger *));
	CArrayInit(&t->things, sizeof(ThingId));
}
void TileDestroy(Tile *t)
{
	CArrayTerminate(&t->triggers);
	CArrayTerminate(&t->things);
}

void TileUpdate(Tile *t)
{
	if (t->Door.Count > 0)
	{
		t->Door.Count--;
	}
}

// t->ClassAlt->Name == NULL for nothing tiles
bool TileIsOpaque(const Tile *t)
{
	return t != NULL && ((t->Class->Type == TILE_CLASS_DOOR && t->Door.Class &&
						  !t->Door.IsOpen && t->Door.Class->Name)
							 ? t->Door.Class->isOpaque
							 : t->Class->isOpaque);
}

bool TileIsShootable(const Tile *t)
{
	return (t->Class->Type == TILE_CLASS_DOOR && t->Door.Class &&
			!t->Door.IsOpen && t->Door.Class->Name)
			   ? t->Door.Class->shootable
			   : t->Class->shootable;
}

bool TileCanWalk(const Tile *t)
{
	if (t == NULL)
	{
		return false;
	}
	return (t->Class->Type == TILE_CLASS_DOOR && t->Door.Class &&
			!t->Door.IsOpen && t->Door.Class->Name)
			   ? t->Door.Class->canWalk
			   : t->Class->canWalk;
}

bool TileIsClear(const Tile *t)
{
	if (t == NULL)
	{
		return false;
	}
	if (!TileCanWalk(t))
	{
		return false;
	}
	// Check if tile has no things on it, excluding particles and pickups
	CA_FOREACH(const ThingId, tid, t->things)
	if (tid->Kind != KIND_PARTICLE && tid->Kind != KIND_PICKUP)
		return false;
	CA_FOREACH_END()
	return true;
}

bool TileHasCharacter(Tile *t)
{
	CA_FOREACH(const ThingId, tid, t->things)
	if (tid->Kind == KIND_CHARACTER)
	{
		return true;
	}
	CA_FOREACH_END()
	return false;
}
