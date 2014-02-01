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

#include "map_object.h"


void MissionConvertToType(Mission *m, Map *map, MapType type)
{
	memset(&m->u, 0, sizeof m->u);
	switch (type)
	{
	case MAPTYPE_CLASSIC:
		break;
	case MAPTYPE_STATIC:
		{
			Vec2i v;
			// Take all the tiles from the current map
			// and save them in the static map
			CArrayInit(&m->u.Static.Tiles, sizeof(unsigned short));
			for (v.y = 0; v.y < m->Size.y; v.y++)
			{
				for (v.x = 0; v.x < m->Size.x; v.x++)
				{
					unsigned short tile = IMapGet(map, v);
					CArrayPushBack(&m->u.Static.Tiles, &tile);
				}
			}
			CArrayInit(&m->u.Static.Items, sizeof(MapObjectPositions));
		}
		break;
	}
	m->Type = type;
}

static int IsClear(unsigned short tile)
{
	tile &= MAP_MASKACCESS;
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
	return *(unsigned short *)CArrayGet(&m->u.Static.Tiles, index);
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
	*(unsigned short *)CArrayGet(&m->u.Static.Tiles, index) = tile;
}

unsigned short MissionGetTile(Mission *m, Vec2i pos)
{
	if (pos.x < 0 || pos.x >= m->Size.x || pos.y < 0 || pos.y >= m->Size.y)
	{
		return MAP_NOTHING;
	}
	int index = pos.y * m->Size.x + pos.x;
	return *(unsigned short *)CArrayGet(&m->u.Static.Tiles, index);
}

void MissionStaticLayout(Mission *m, Vec2i oldSize)
{
	assert(m->Type == MAPTYPE_STATIC && "invalid map type");
	// re-layout the static map after a resize
	// The mission contains the new size; the old dimensions are oldSize
	// Simply try to "paint" the old tiles to the new mission
	Vec2i v;
	CArray oldTiles;
	CArrayInit(&oldTiles, m->u.Static.Tiles.elemSize);
	CArrayCopy(&oldTiles, &m->u.Static.Tiles);

	// Clear the tiles first
	CArrayTerminate(&m->u.Static.Tiles);
	CArrayInit(&m->u.Static.Tiles, oldTiles.elemSize);
	for (v.y = 0; v.y < m->Size.y; v.y++)
	{
		for (v.x = 0; v.x < m->Size.x; v.x++)
		{
			unsigned short tile = MAP_FLOOR;
			CArrayPushBack(&m->u.Static.Tiles, &tile);
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

	if (m->u.Static.Start.x >= m->Size.x || m->u.Static.Start.y >= m->Size.y)
	{
		m->u.Static.Start = Vec2iZero();
	}
}

bool MissionStaticTryAddItem(Mission *m, int item, Vec2i pos)
{
	assert(m->Type == MAPTYPE_STATIC && "invalid map type");
	unsigned short tile = MissionGetTile(m, pos);
	MapObject *obj = MapObjectGet(item);

	// Remove any items already there
	MissionStaticTryRemoveItemAt(m, pos);

	if (MapObjectIsTileOK(
		obj, tile, 1, MissionGetTile(m, Vec2iNew(pos.x, pos.y - 1))))
	{
		// Check if the item already has an entry, and add to its list
		// of positions
		int hasAdded = 0;
		for (int i = 0; i < (int)m->u.Static.Items.size; i++)
		{
			MapObjectPositions *mop = CArrayGet(&m->u.Static.Items, i);
			if (i == item)
			{
				CArrayPushBack(&mop->Positions, &pos);
				hasAdded = 1;
				break;
			}
		}
		// If not, create a new entry
		if (!hasAdded)
		{
			MapObjectPositions mop;
			mop.Index = item;
			CArrayInit(&mop.Positions, sizeof(Vec2i));
			CArrayPushBack(&mop.Positions, &pos);
			CArrayPushBack(&m->u.Static.Items, &mop);
		}
		return true;
	}
	return false;
}
bool MissionStaticTryRemoveItemAt(Mission *m, Vec2i pos)
{
	for (int i = 0; i < (int)m->u.Static.Items.size; i++)
	{
		MapObjectPositions *mop = CArrayGet(&m->u.Static.Items, i);
		for (int j = 0; j < (int)mop->Positions.size; j++)
		{
			Vec2i *mopPos = CArrayGet(&mop->Positions, j);
			if (Vec2iEqual(*mopPos, pos))
			{
				CArrayDelete(&mop->Positions, j);
				return true;
			}
		}
	}
	return false;
}

bool MissionStaticTryAddCharacter(Mission *m, int ch, Vec2i pos)
{
	assert(m->Type == MAPTYPE_STATIC && "invalid map type");
	unsigned short tile = MissionGetTile(m, pos);

	// Remove any characters already there
	MissionStaticTryRemoveCharacterAt(m, pos);

	if (IsClear(tile))
	{
		// Check if the character already has an entry, and add to its list
		// of positions
		int hasAdded = 0;
		for (int i = 0; i < (int)m->u.Static.Characters.size; i++)
		{
			CharacterPositions *cp = CArrayGet(&m->u.Static.Characters, i);
			if (i == ch)
			{
				CArrayPushBack(&cp->Positions, &pos);
				hasAdded = 1;
				break;
			}
		}
		// If not, create a new entry
		if (!hasAdded)
		{
			CharacterPositions cp;
			cp.Index = ch;
			CArrayInit(&cp.Positions, sizeof(Vec2i));
			CArrayPushBack(&cp.Positions, &pos);
			CArrayPushBack(&m->u.Static.Characters, &cp);
		}
		return true;
	}
	return false;
}
bool MissionStaticTryRemoveCharacterAt(Mission *m, Vec2i pos)
{
	for (int i = 0; i < (int)m->u.Static.Characters.size; i++)
	{
		CharacterPositions *cp = CArrayGet(&m->u.Static.Characters, i);
		for (int j = 0; j < (int)cp->Positions.size; j++)
		{
			Vec2i *cpPos = CArrayGet(&cp->Positions, j);
			if (Vec2iEqual(*cpPos, pos))
			{
				CArrayDelete(&cp->Positions, j);
				return true;
			}
		}
	}
	return false;
}

bool MissionStaticTryAddObjective(Mission *m, int idx, int idx2, Vec2i pos)
{
	assert(m->Type == MAPTYPE_STATIC && "invalid map type");
	unsigned short tile = MissionGetTile(m, pos);
	
	// Remove any objectives already there
	MissionStaticTryRemoveObjectiveAt(m, pos);
	
	if (IsClear(tile))
	{
		// Check if the objective already has an entry, and add to its list
		// of positions
		int hasAdded = 0;
		int objectiveIndex = -1;
		ObjectivePositions *op = NULL;
		for (int i = 0; i < (int)m->u.Static.Objectives.size; i++)
		{
			op = CArrayGet(&m->u.Static.Objectives, i);
			if (i == idx)
			{
				CArrayPushBack(&op->Positions, &pos);
				CArrayPushBack(&op->Indices, &idx2);
				objectiveIndex = i;
				hasAdded = 1;
				break;
			}
		}
		// If not, create a new entry
		if (!hasAdded)
		{
			ObjectivePositions newOp;
			newOp.Index = idx;
			CArrayInit(&newOp.Positions, sizeof(Vec2i));
			CArrayInit(&newOp.Indices, sizeof(int));
			objectiveIndex = (int)newOp.Positions.size;
			CArrayPushBack(&newOp.Positions, &pos);
			CArrayPushBack(&newOp.Indices, &idx2);
			CArrayPushBack(&m->u.Static.Objectives, &newOp);
			op = CArrayGet(&m->u.Static.Objectives, objectiveIndex);
		}
		// If we've added too many, remove the first entry
		MissionObjective *mobj = CArrayGet(&m->Objectives, objectiveIndex);
		while (mobj->Count < (int)op->Positions.size)
		{
			CArrayDelete(&op->Positions, 0);
			CArrayDelete(&op->Indices, 0);
		}
		return true;
	}
	return false;
}
bool MissionStaticTryRemoveObjectiveAt(Mission *m, Vec2i pos)
{
	for (int i = 0; i < (int)m->u.Static.Objectives.size; i++)
	{
		ObjectivePositions *op = CArrayGet(&m->u.Static.Objectives, i);
		for (int j = 0; j < (int)op->Positions.size; j++)
		{
			Vec2i *opPos = CArrayGet(&op->Positions, j);
			if (Vec2iEqual(*opPos, pos))
			{
				CArrayDelete(&op->Positions, j);
				CArrayDelete(&op->Indices, j);
				return true;
			}
		}
	}
	return false;
}

bool MissionStaticTryAddKey(Mission *m, int k, Vec2i pos)
{
	assert(m->Type == MAPTYPE_STATIC && "invalid map type");
	unsigned short tile = MissionGetTile(m, pos);
	
	// Remove any keys already there
	MissionStaticTryRemoveKeyAt(m, pos);
	
	if (IsClear(tile))
	{
		// Check if the item already has an entry, and add to its list
		// of positions
		bool hasAdded = false;
		for (int i = 0; i < (int)m->u.Static.Keys.size; i++)
		{
			KeyPositions *kp = CArrayGet(&m->u.Static.Keys, i);
			if (i == k)
			{
				CArrayPushBack(&kp->Positions, &pos);
				hasAdded = true;
				break;
			}
		}
		// If not, create a new entry
		if (!hasAdded)
		{
			KeyPositions kp;
			kp.Index = k;
			CArrayInit(&kp.Positions, sizeof(Vec2i));
			CArrayPushBack(&kp.Positions, &pos);
			CArrayPushBack(&m->u.Static.Keys, &kp);
		}
		return true;
	}
	return false;
}
bool MissionStaticTryRemoveKeyAt(Mission *m, Vec2i pos)
{
	for (int i = 0; i < (int)m->u.Static.Items.size; i++)
	{
		MapObjectPositions *mop = CArrayGet(&m->u.Static.Items, i);
		for (int j = 0; j < (int)mop->Positions.size; j++)
		{
			Vec2i *mopPos = CArrayGet(&mop->Positions, j);
			if (Vec2iEqual(*mopPos, pos))
			{
				CArrayDelete(&mop->Positions, j);
				return true;
			}
		}
	}
	return false;
}

static bool FloodFill(Mission *m, Vec2i v, unsigned short mask);
bool MissionStaticTrySetKey(Mission *m, int k, Vec2i pos)
{
	assert(m->Type == MAPTYPE_STATIC && "invalid map type");
	unsigned short mask = GetAccessMask(k);
	return FloodFill(m, pos, mask);
}
bool MissionStaticTryUnsetKeyAt(Mission *m, Vec2i pos)
{
	// -1 for no access level
	return MissionStaticTrySetKey(m, -1, pos);
}
// Use flood fill to set access levels for doors
// Set access level for all contiguous door tiles
static bool FloodFill(Mission *m, Vec2i v, unsigned short mask)
{
	unsigned short tile = MissionGetTile(m, v);
	if ((tile & MAP_MASKACCESS) == MAP_DOOR &&
		(tile & MAP_ACCESSBITS) != mask)
	{
		MissionSetTile(m, v, MAP_DOOR | mask);
		FloodFill(m, Vec2iNew(v.x - 1, v.y), mask);
		FloodFill(m, Vec2iNew(v.x + 1, v.y), mask);
		FloodFill(m, Vec2iNew(v.x, v.y - 1), mask);
		FloodFill(m, Vec2iNew(v.x, v.y + 1), mask);
		return true;
	}
	return false;
}
