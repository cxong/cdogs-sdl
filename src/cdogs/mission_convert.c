/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (c) 2014-2017 Cong Xu
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

#include "algorithms.h"
#include "map_object.h"
#include "mission.h"


void MissionConvertToType(Mission *m, Map *map, MapType type)
{
	memset(&m->u, 0, sizeof m->u);
	switch (type)
	{
	case MAPTYPE_CLASSIC:
		// Setup default parameters
		m->u.Classic.Walls = 10;
		m->u.Classic.WallLength = 5;
		m->u.Classic.CorridorWidth = 2;
		m->u.Classic.Rooms.Count = 10;
		m->u.Classic.Rooms.Min = 5;
		m->u.Classic.Rooms.Max = 8;;
		m->u.Classic.Rooms.Edge = true;
		m->u.Classic.Rooms.Overlap = true;
		m->u.Classic.Rooms.Walls = 1;
		m->u.Classic.Rooms.WallLength = 1;
		m->u.Classic.Rooms.WallPad = 1;
		m->u.Classic.Squares = 1;
		m->u.Classic.Doors.Enabled = true;
		m->u.Classic.Doors.Min = 1;
		m->u.Classic.Doors.Max = 2;
		m->u.Classic.Pillars.Count = 1;
		m->u.Classic.Pillars.Min = 2;
		m->u.Classic.Pillars.Max = 3;
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
			CArrayInit(&m->u.Static.Characters, sizeof(CharacterPositions));
			CArrayInit(&m->u.Static.Objectives, sizeof(ObjectivePositions));
			CArrayInit(&m->u.Static.Keys, sizeof(KeyPositions));
		}
		break;
	case MAPTYPE_CAVE:
		// Setup default parameters
		m->u.Cave.FillPercent = 40;
		m->u.Cave.Repeat = 4;
		m->u.Cave.R1 = 5;
		m->u.Cave.R2 = 2;
		m->u.Cave.CorridorWidth = 2;
		m->u.Cave.Squares = 1;
		break;
	default:
		CASSERT(false, "unknown map type");
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
	int idx = pos.y * m->Size.x + pos.x;
	// check for out-of-bounds
	if (pos.x < 0 || pos.x >= m->Size.x || pos.y < 0 || pos.y >= m->Size.y)
	{
		return MAP_NOTHING;
	}
	return *(unsigned short *)CArrayGet(&m->u.Static.Tiles, idx);
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
bool MissionTrySetTile(Mission *m, Vec2i pos, unsigned short tile)
{
	if (pos.x < 0 || pos.x >= m->Size.x || pos.y < 0 || pos.y >= m->Size.y)
	{
		return false;
	}
	CASSERT(m->Type == MAPTYPE_STATIC, "cannot set tile for map type");
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
			return false;
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
				return false;
			}
			// Check that there are no incompatible doors
			if (HasDoorOrientedAt(m, Vec2iNew(pos.x - 1, pos.y), 0) ||
				HasDoorOrientedAt(m, Vec2iNew(pos.x + 1, pos.y), 0) ||
				HasDoorOrientedAt(m, Vec2iNew(pos.x, pos.y - 1), 1) ||
				HasDoorOrientedAt(m, Vec2iNew(pos.x, pos.y + 1), 1))
			{
				// Can't place this door
				return false;
			}
		}
		break;
	}
	const int idx = pos.y * m->Size.x + pos.x;
	*(unsigned short *)CArrayGet(&m->u.Static.Tiles, idx) = tile;
	return true;
}

unsigned short MissionGetTile(Mission *m, Vec2i pos)
{
	if (pos.x < 0 || pos.x >= m->Size.x || pos.y < 0 || pos.y >= m->Size.y)
	{
		return MAP_UNSET;
	}
	int idx = pos.y * m->Size.x + pos.x;
	return *(unsigned short *)CArrayGet(&m->u.Static.Tiles, idx);
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
				MissionTrySetTile(m, v, MAP_NOTHING);
			}
			else
			{
				int idx = v.y * oldSize.x + v.x;
				unsigned short *tile = CArrayGet(&oldTiles, idx);
				MissionTrySetTile(m, v, *tile);
			}
		}
	}

	CArrayTerminate(&oldTiles);

	if (m->u.Static.Start.x >= m->Size.x || m->u.Static.Start.y >= m->Size.y)
	{
		m->u.Static.Start = Vec2iZero();
	}
}

static bool TryAddMapObject(
	Mission *m, const MapObject *mo, const Vec2i pos, CArray *objs);
static bool TryRemoveMapObjectAt(const Vec2i pos, CArray *objs);
bool MissionStaticTryAddItem(Mission *m, const MapObject *mo, const Vec2i pos)
{
	return TryAddMapObject(m, mo, pos, &m->u.Static.Items);
}
bool MissionStaticTryRemoveItemAt(Mission *m, const Vec2i pos)
{
	return TryRemoveMapObjectAt(pos, &m->u.Static.Items);
}
static bool TryAddMapObject(
	Mission *m, const MapObject *mo, const Vec2i pos, CArray *objs)
{
	CASSERT(m->Type == MAPTYPE_STATIC, "invalid map type");
	const unsigned short tile = MissionGetTile(m, pos);

	// Remove any items already there
	TryRemoveMapObjectAt(pos, objs);

	if (MapObjectIsTileOK(
		mo, tile, 1, MissionGetTile(m, Vec2iNew(pos.x, pos.y - 1))))
	{
		// Check if the item already has an entry, and add to its list
		// of positions
		bool hasAdded = false;
		for (int i = 0; i < (int)objs->size; i++)
		{
			MapObjectPositions *mop = CArrayGet(objs, i);
			if (mop->M == mo)
			{
				CArrayPushBack(&mop->Positions, &pos);
				hasAdded = true;
				break;
			}
		}
		// If not, create a new entry
		if (!hasAdded)
		{
			MapObjectPositions mop;
			mop.M = mo;
			CArrayInit(&mop.Positions, sizeof(Vec2i));
			CArrayPushBack(&mop.Positions, &pos);
			CArrayPushBack(objs, &mop);
		}
		return true;
	}
	return false;
}
static bool TryRemoveMapObjectAt(const Vec2i pos, CArray *objs)
{
	for (int i = 0; i < (int)objs->size; i++)
	{
		MapObjectPositions *mop = CArrayGet(objs, i);
		for (int j = 0; j < (int)mop->Positions.size; j++)
		{
			const Vec2i *mopPos = CArrayGet(&mop->Positions, j);
			if (Vec2iEqual(*mopPos, pos))
			{
				CArrayDelete(&mop->Positions, j);
				if (mop->Positions.size == 0)
				{
					CArrayTerminate(&mop->Positions);
					CArrayDelete(objs, i);
				}
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
		bool hasAdded = false;
		CA_FOREACH(CharacterPositions, cp, m->u.Static.Characters)
			if (cp->Index == ch)
			{
				CArrayPushBack(&cp->Positions, &pos);
				hasAdded = true;
				break;
			}
		CA_FOREACH_END()
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
	CA_FOREACH(CharacterPositions, cp, m->u.Static.Characters)
		for (int j = 0; j < (int)cp->Positions.size; j++)
		{
			Vec2i *cpPos = CArrayGet(&cp->Positions, j);
			if (Vec2iEqual(*cpPos, pos))
			{
				CArrayDelete(&cp->Positions, j);
				if (cp->Positions.size == 0)
				{
					CArrayTerminate(&cp->Positions);
					CArrayDelete(&m->u.Static.Characters, _ca_index);
				}
				return true;
			}
		}
	CA_FOREACH_END()
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
			if (op->Index == idx)
			{
				CArrayPushBack(&op->Positions, &pos);
				CArrayPushBack(&op->Indices, &idx2);
				objectiveIndex = op->Index;
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
			CArrayPushBack(&newOp.Positions, &pos);
			CArrayPushBack(&newOp.Indices, &idx2);
			objectiveIndex = (int)newOp.Positions.size - 1;
			CArrayPushBack(&m->u.Static.Objectives, &newOp);
			op = CArrayGet(
				&m->u.Static.Objectives, (int)m->u.Static.Objectives.size - 1);
		}
		// Increase number of objectives
		Objective *o = CArrayGet(&m->Objectives, objectiveIndex);
		o->Count++;
		return true;
	}
	return false;
}
bool MissionStaticTryRemoveObjectiveAt(Mission *m, Vec2i pos)
{
	CA_FOREACH(ObjectivePositions, op, m->u.Static.Objectives)
		for (int j = 0; j < (int)op->Positions.size; j++)
		{
			Vec2i *opPos = CArrayGet(&op->Positions, j);
			if (Vec2iEqual(*opPos, pos))
			{
				CArrayDelete(&op->Positions, j);
				CArrayDelete(&op->Indices, j);
				// Decrease number of objectives
				Objective *o = CArrayGet(&m->Objectives, op->Index);
				o->Count--;
				CASSERT(o->Count >= 0, "removing unknown objective");
				if (op->Positions.size == 0)
				{
					CArrayTerminate(&op->Positions);
					CArrayTerminate(&op->Indices);
					CArrayDelete(&m->u.Static.Objectives, _ca_index);
				}
				return true;
			}
		}
	CA_FOREACH_END()
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
		CA_FOREACH(KeyPositions, kp, m->u.Static.Keys)
			if (kp->Index == k)
			{
				CArrayPushBack(&kp->Positions, &pos);
				hasAdded = true;
				break;
			}
		CA_FOREACH_END()
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
	CA_FOREACH(KeyPositions, kp, m->u.Static.Keys)
		for (int j = 0; j < (int)kp->Positions.size; j++)
		{
			Vec2i *kpPos = CArrayGet(&kp->Positions, j);
			if (Vec2iEqual(*kpPos, pos))
			{
				CArrayDelete(&kp->Positions, j);
				if (kp->Positions.size == 0)
				{
					CArrayTerminate(&kp->Positions);
					CArrayDelete(&m->u.Static.Keys, _ca_index);
				}
				return true;
			}
		}
	CA_FOREACH_END()
	return false;
}

typedef struct
{
	Mission *m;
	unsigned short mask;
} MissionFloodFillData;
static void MissionFillTile(void *data, Vec2i v);
static bool MissionIsTileSame(void *data, Vec2i v);
bool MissionStaticTrySetKey(Mission *m, int k, Vec2i pos)
{
	assert(m->Type == MAPTYPE_STATIC && "invalid map type");
	unsigned short mask = GetAccessMask(k);
	FloodFillData data;
	data.Fill = MissionFillTile;
	data.IsSame = MissionIsTileSame;
	MissionFloodFillData mData;
	mData.m = m;
	mData.mask = mask;
	data.data = &mData;
	return CFloodFill(pos, &data);
}
static void MissionFillTile(void *data, Vec2i v)
{
	MissionFloodFillData *mData = data;
	MissionTrySetTile(mData->m, v, MAP_DOOR | mData->mask);
}
static bool MissionIsTileSame(void *data, Vec2i v)
{
	MissionFloodFillData *mData = data;
	unsigned short tile = MissionGetTile(mData->m, v);
	return (tile & MAP_MASKACCESS) == MAP_DOOR &&
		(tile & MAP_ACCESSBITS) != mData->mask;
}

bool MissionStaticTryUnsetKeyAt(Mission *m, Vec2i pos)
{
	// -1 for no access level
	return MissionStaticTrySetKey(m, -1, pos);
}
