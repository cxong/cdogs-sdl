/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (c) 2014, Cong Xu
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
#include "mission_convert.h"

#include <assert.h>


void MissionConvertToType(Mission *m, Map *map, MapType type)
{
	memset(&m->u, 0, sizeof m->u);
	switch (type)
	{
	case MAPTYPE_STATIC:
		{
			Vec2i v;
			// Take all the tiles from the current map
			// and save them in the static map
			CArrayInit(&m->u.StaticTiles, sizeof(unsigned short));
			for (v.y = 0; v.y < m->Size.y; v.y++)
			{
				for (v.x = 0; v.x < m->Size.x; v.x++)
				{
					Vec2i mapPos = Vec2iNew(
						v.x + (XMAX - m->Size.x) / 2,
						v.y + (YMAX - m->Size.y) / 2);
					unsigned short tile = IMapGet(map, mapPos);
					CArrayPushBack(&m->u.StaticTiles, &tile);
				}
			}
		}
		break;
	}
	m->Type = type;
}

static int IsClear(unsigned short tile)
{
	return tile == MAP_FLOOR || tile == MAP_ROOM || tile == MAP_SQUARE;
}
static unsigned short GetTileAt(Mission *m, Vec2i pos)
{
	int index = pos.y * m->Size.x + pos.x;
	// check for out-of-bounds
	if (pos.x < 0 || pos.x >= m->Size.x || pos.y < 0 || pos.y >= m->Size.y)
	{
		return MAP_NOTHING;
	}
	return *(unsigned short *)CArrayGet(&m->u.StaticTiles, index);
}
// See if the tile located at a position is a door and also needs
// to be oriented in a certain way
// If there are walls or doors in the neighbourhood, they can force a certain
// orientation of the door
static int HasDoorOrientedAt(Mission *m, Vec2i pos,int isHorizontal)
{
	unsigned short tile = GetTileAt(m, pos);
	if (tile != MAP_DOOR)
	{
		return 0;
	}
	// Check for walls and doors that force the orientation of the door
	if (GetTileAt(m, Vec2iNew(pos.x - 1, pos.y)) == MAP_WALL ||
		GetTileAt(m, Vec2iNew(pos.x - 1, pos.y)) == MAP_DOOR ||
		GetTileAt(m, Vec2iNew(pos.x + 1, pos.y)) == MAP_WALL ||
		GetTileAt(m, Vec2iNew(pos.x + 1, pos.y)) == MAP_DOOR)
	{
		// There is a horizontal door
		return isHorizontal;
	}
	else if (GetTileAt(m, Vec2iNew(pos.x, pos.y - 1)) == MAP_WALL ||
		GetTileAt(m, Vec2iNew(pos.x, pos.y - 1)) == MAP_DOOR ||
		GetTileAt(m, Vec2iNew(pos.x, pos.y + 1)) == MAP_WALL ||
		GetTileAt(m, Vec2iNew(pos.x, pos.y + 1)) == MAP_DOOR)
	{
		// There is a vertical door
		return !isHorizontal;
	}
	// There is a door but it is free to be oriented in any way
	return 0;
}
void MissionSetTile(Mission *m, Vec2i pos, unsigned short tile)
{
	int index = pos.y * m->Size.x + pos.x;
	assert(m->Type == MAPTYPE_STATIC && "cannot set tile for map type");
	switch (tile)
	{
	case MAP_WALL:
		// Check that there are no incompatible doors
		if (HasDoorOrientedAt(m, Vec2iNew(pos.x - 1, pos.y), 0) ||
			HasDoorOrientedAt(m, Vec2iNew(pos.x + 1, pos.y), 0) ||
			HasDoorOrientedAt(m, Vec2iNew(pos.x, pos.y - 1), 1) ||
			HasDoorOrientedAt(m, Vec2iNew(pos.x, pos.y + 1), 1))
		{
			// Can't place this wall
			return;
		}
		break;
	case MAP_DOOR:
		{
			// Check that there is a clear passage through this door
			int isHClear =
				IsClear(GetTileAt(m, Vec2iNew(pos.x - 1, pos.y))) &&
				IsClear(GetTileAt(m, Vec2iNew(pos.x + 1, pos.y)));
			int isVClear =
				IsClear(GetTileAt(m, Vec2iNew(pos.x, pos.y - 1))) &&
				IsClear(GetTileAt(m, Vec2iNew(pos.x, pos.y + 1)));
			if (!isHClear && !isVClear)
			{
				return;
			}
			// Check that there are no incompatible doors
			if (HasDoorOrientedAt(m, Vec2iNew(pos.x - 1, pos.y), 0) ||
				HasDoorOrientedAt(m, Vec2iNew(pos.x + 1, pos.y), 0) ||
				HasDoorOrientedAt(m, Vec2iNew(pos.x, pos.y - 1), 1) ||
				HasDoorOrientedAt(m, Vec2iNew(pos.x, pos.y + 1), 1))
			{
				// Can't place this door
				return;
			}
		}
		break;
	}
	*(unsigned short *)CArrayGet(&m->u.StaticTiles, index) = tile;
}

void MissionStaticLayout(Mission *m, Vec2i oldSize)
{
	// re-layout the static map after a resize
	// The mission contains the new size; the old dimensions are oldSize
	// Simply try to "paint" the old tiles to the new mission
	Vec2i v;
	CArray oldTiles;
	CArrayInit(&oldTiles, m->u.StaticTiles.elemSize);
	CArrayCopy(&oldTiles, &m->u.StaticTiles);

	// Clear the tiles first
	CArrayTerminate(&m->u.StaticTiles);
	CArrayInit(&m->u.StaticTiles, oldTiles.elemSize);
	for (v.y = 0; v.y < m->Size.y; v.y++)
	{
		for (v.x = 0; v.x < m->Size.x; v.x++)
		{
			unsigned short tile = MAP_NOTHING;
			CArrayPushBack(&m->u.StaticTiles, &tile);
		}
	}

	// Paint the old tiles back
	for (v.y = 0; v.y < m->Size.y; v.y++)
	{
		for (v.x = 0; v.x < m->Size.x; v.x++)
		{
			if (v.x >= oldSize.x || v.y >= oldSize.y)
			{
				MissionSetTile(m, v, MAP_NOTHING);
			}
			else
			{
				int index = v.y * oldSize.x + v.x;
				unsigned short *tile = CArrayGet(&oldTiles, index);
				MissionSetTile(m, v, *tile);
			}
		}
	}

	CArrayTerminate(&oldTiles);
}
