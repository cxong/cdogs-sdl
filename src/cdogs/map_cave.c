/*
    C-Dogs SDL
    A port of the legendary (and fun) action/arcade cdogs.
    Copyright (c) 2016-2017 Cong Xu
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
#include "map_cave.h"

#include "algorithms.h"
#include "log.h"
#include "map_build.h"


static void CaveRep(Map *map, const int r1, const int r2);
static void LinkDisconnectedAreas(Map *map);
static void FixCorridors(Map *map, const int corridorWidth);
static void PlaceSquares(Map *map, const int squares);
static void PlaceRooms(Map *map, const Mission *m);
void MapCaveLoad(
	Map *map, const struct MissionOptions *mo, const CampaignOptions* co)
{
	// Re-seed RNG so results are consistent
	CampaignSeedRandom(co);

	const Mission *m = mo->missionData;

	// Randomly set a percentage of the tiles as walls
	for (int i = 0;
		i < m->u.Cave.FillPercent*map->Size.x*map->Size.y / 100;
		i++)
	{
		const struct vec2i pos = svec2i(i % map->Size.x, i / map->Size.x);
		IMapSet(map, pos, MAP_WALL);
	}
	// Shuffle
	CArrayShuffle(&map->iMap);
	// Repetitions
	for (int i = 0; i < m->u.Cave.Repeat; i++)
	{
		CaveRep(map, m->u.Cave.R1, m->u.Cave.R2);
	}

	LinkDisconnectedAreas(map);

	FixCorridors(map, m->u.Cave.CorridorWidth);

	PlaceSquares(map, m->u.Cave.Squares);

	PlaceRooms(map, m);
}

// Perform one generation of cellular automata
// If the number of walls within 1 distance is at least R1, OR
// if the number of walls within 2 distance is at most R2, then the tile
// becomes a wall; otherwise it is a floor
static int CountTilesAround(
	const Map *map, const struct vec2i pos, const int radius,
	const unsigned short tile);
static void CaveRep(Map *map, const int r1, const int r2)
{
	CArray buf;
	CArrayInit(&buf, map->iMap.elemSize);
	const unsigned short floor = MAP_FLOOR;
	CArrayResize(&buf, map->iMap.size, &floor);
	struct vec2i v;
	for (v.y = 0; v.y < map->Size.y; v.y++)
	{
		for (v.x = 0; v.x < map->Size.x; v.x++)
		{
			const int idx = v.x + v.y * map->Size.x;
			unsigned short *tile = CArrayGet(&buf, idx);
			if (CountTilesAround(map, v, 1, MAP_WALL) >= r1 ||
				CountTilesAround(map, v, 2, MAP_WALL) <= r2)
			{
				*tile = MAP_WALL;
			}
			else
			{
				*tile = MAP_FLOOR;
			}
		}
	}
	CArrayCopy(&map->iMap, &buf);
	CArrayTerminate(&buf);
}
static int CountTilesAround(
	const Map *map, const struct vec2i pos, const int radius,
	const unsigned short tile)
{
	int c = 0;
	for (int x = pos.x - radius; x <= pos.x + radius; x++)
	{
		for (int y = pos.y - radius; y <= pos.y + radius; y++)
		{
			// Also count edge of maps
			if (x < 0 || x >= map->Size.x || y < 0 || y >= map->Size.y ||
				IMapGet(map, svec2i(x, y)) == tile)
			{
				c++;
			}
		}
	}
	return c;
}

static void MapFloodFill(
	CArray *fl, const struct vec2i size, const int idx, const int elem);
static void AddCorridor(
	Map *map, const struct vec2i v1, const struct vec2i v2, const struct vec2i dInit,
	const unsigned short tile);
static void LinkDisconnectedAreas(Map *map)
{
	// Use flood fill to identify disconnected areas
	CArray fl;
	CArrayInit(&fl, sizeof(int));
	const int zero = 0;
	CArrayResize(&fl, map->iMap.size, &zero);
	// First copy across the wall tiles (as -1)
	CA_FOREACH(const unsigned short, tile, map->iMap)
		if (*tile == MAP_WALL)
		{
			*(int *)CArrayGet(&fl, _ca_index) = -1;
		}
	CA_FOREACH_END()
	// Then perform flood fill conditionally on the flood layer
	int idx = 1;
	CA_FOREACH(int, i, fl)
		if (*i == 0)
		{
			MapFloodFill(&fl, map->Size, _ca_index, idx);
			idx++;
		}
	CA_FOREACH_END()

	const int numAreas = idx - 1;
	// Connect the disconnected areas, first to second, second to third etc.
	// Select random tile from each area, using index shuffle
	CArray areaTiles;
	CArrayInit(&areaTiles, sizeof(int));
	CA_FOREACH(int, i, fl)
		UNUSED(i);
		CArrayPushBack(&areaTiles, &_ca_index);
	CA_FOREACH_END()
	CArrayShuffle(&areaTiles);
	CArray areaStarts;
	CArrayInit(&areaStarts, sizeof(int));
	CArrayResize(&areaStarts, numAreas, &zero);
	CA_FOREACH(int, areaIdx, areaTiles)
		const int tile = *(int *)CArrayGet(&fl, *areaIdx) - 1;
		if (tile >= 0 && tile < numAreas)
		{
			int *areaStart = CArrayGet(&areaStarts, tile);
			if (*areaStart == 0)
			{
				*areaStart = *areaIdx;
			}
		}
	CA_FOREACH_END()
	CArrayTerminate(&fl);
	CArrayTerminate(&areaTiles);
	// Connect consecutive pairs of tiles
	for (int i = 0; i < (int)areaStarts.size - 1; i++)
	{
		const int *a1 = CArrayGet(&areaStarts, i);
		const int *a2 = CArrayGet(&areaStarts, i + 1);
		const struct vec2i v1 = svec2i(*a1 % map->Size.x, *a1 / map->Size.x);
		const struct vec2i v2 = svec2i(*a2 % map->Size.x, *a2 / map->Size.x);
		const struct vec2i delta = svec2i(abs(v1.x - v2.x), abs(v1.y - v2.y));
		const int dx = delta.x > delta.y ? 1 : 0;
		const int dy = 1 - dx;
		AddCorridor(map, v1, v2, svec2i(dx, dy), MAP_FLOOR);
	}
	CArrayTerminate(&areaStarts);
}

static void MapFloodFill(
	CArray *fl, const struct vec2i size, const int idx, const int elem)
{
	CArray indices;
	CArrayInit(&indices, sizeof(int));
	CArrayPushBack(&indices, &idx);
	const int floodTile = *(int *)CArrayGet(fl, idx);
	*(int *)CArrayGet(fl, idx) = elem;
	CA_FOREACH(int, i, indices)
		const int x = *i % size.x;
		const int y = *i / size.x;
		int next;
		// top
		next = (y - 1)*size.x + x;
		if (y > 0 && *(int *)CArrayGet(fl, next) == floodTile)
		{
			CArrayPushBack(&indices, &next);
			*(int *)CArrayGet(fl, next) = elem;
		}
		// bottom
		next = (y + 1)*size.x + x;
		if (y < size.y - 1 && *(int *)CArrayGet(fl, next) == floodTile)
		{
			CArrayPushBack(&indices, &next);
			*(int *)CArrayGet(fl, next) = elem;
		}
		// left
		next = y*size.x + x - 1;
		if (x > 0 && *(int *)CArrayGet(fl, next) == floodTile)
		{
			CArrayPushBack(&indices, &next);
			*(int *)CArrayGet(fl, next) = elem;
		}
		// right
		next = y*size.x + x + 1;
		if (x < size.x - 1 && *(int *)CArrayGet(fl, next) == floodTile)
		{
			CArrayPushBack(&indices, &next);
			*(int *)CArrayGet(fl, next) = elem;
		}
	CA_FOREACH_END()
	CArrayTerminate(&indices);
}

// Add an S-shaped corridor from one point to another, filling it with a
// certain tile value. The corridor starts in a specific direction d, then
// makes a turn in the middle, then turns back to the original direction.
static void AddCorridor(
	Map *map, const struct vec2i v1, const struct vec2i v2, const struct vec2i dInit,
	const unsigned short tile)
{
	struct vec2i dAlt;
	// Location of the turn
	struct vec2i half;
	struct vec2i start = v1;
	struct vec2i end = v2;
	struct vec2i d = dInit;
	if (d.x > 0)
	{
		// horizontal
		d = svec2i(1, 0);
		if (start.x > end.x)
		{
			// Swap
			const struct vec2i tmp = start;
			start = end;
			end = tmp;
		}
		dAlt = svec2i(0, 1);
		half = svec2i((end.x - start.x) / 2 + start.x, end.y + 1);
		if (end.y < start.y)
		{
			dAlt.y = -1;
			half.y = end.y - 1;
		}
	}
	else
	{
		// vertical
		d = svec2i(0, 1);
		if (start.y > end.y)
		{
			// Swap
			const struct vec2i tmp = start;
			start = end;
			end = tmp;
		}
		dAlt = svec2i(1, 0);
		half = svec2i(end.x + 1, (end.y - start.y) / 2 + start.y);
		if (end.x < start.x)
		{
			dAlt.x = -1;
			half.x = end.x - 1;
		}
	}
	// Initial direction
	struct vec2i v = start;
	for (; v.x != half.x && v.y != half.y; v = svec2i_add(v, d))
	{
		IMapSet(map, v, tile);
	}
	// Turn
	for (; v.x != end.x && v.y != end.y; v = svec2i_add(v, dAlt))
	{
		IMapSet(map, v, tile);
	}
	// Finish
	for (; v.x != end.x || v.y != end.y; v = svec2i_add(v, d))
	{
		IMapSet(map, v, tile);
	}
	IMapSet(map, end, tile);
}

static bool CheckCorridorsAroundTile(
	const Map *map, const int corridorWidth, const struct vec2i v);
// Make sure corridors are wide enough
static void FixCorridors(Map *map, const int corridorWidth)
{
	// Find all instances where a corridor is too narrow
	// Do this by drawing a line from each wall tile to its surrounding square
	// area, and that this line of tiles that each wall tile is either
	// - blocked by a neighbouring wall tile, or
	// - only consists of floor tiles
	// If not, then replace that wall with a floor tile
	struct vec2i v;
	for (v.y = corridorWidth; v.y < map->Size.y - corridorWidth; v.y++)
	{
		for (v.x = corridorWidth; v.x < map->Size.x - corridorWidth; v.x++)
		{
			const int idx = v.x + v.y * map->Size.x;
			unsigned short *tile = CArrayGet(&map->iMap, idx);
			// Make sure that the tile is a wall
			if (*tile != MAP_WALL)
			{
				continue;
			}
			if (!CheckCorridorsAroundTile(map, corridorWidth, v))
			{
				// Corridor checks failed; replace with a floor tile
				*tile = MAP_FLOOR;
			}
		}
	}
}
typedef struct
{
	const Map *M;
	int Counter;
	bool IsFirstWall;
	bool AreAllFloors;
} FixCorridorOnTileData;
static void FixCorridorOnTile(void *data, struct vec2i v);
static bool CheckCorridorsAroundTile(
	const Map *map, const int corridorWidth, const struct vec2i v)
{
	AlgoLineDrawData data;
	data.Draw = FixCorridorOnTile;
	FixCorridorOnTileData onTileData;
	onTileData.M = map;
	data.data = &onTileData;
	struct vec2i v1;
	for (v1.y = v.y - corridorWidth; v1.y <= v.y + corridorWidth; v1.y++)
	{
		for (v1.x = v.x - corridorWidth; v1.x <= v.x + corridorWidth; v1.x++)
		{
			// only draw out to edges
			if (v1.y != v.y - corridorWidth && v1.y != v.y + corridorWidth &&
				v1.x != v.x - corridorWidth && v1.x != v.x + corridorWidth)
			{
				continue;
			}
			onTileData.Counter = 0;
			onTileData.IsFirstWall = false;
			onTileData.AreAllFloors = true;
			BresenhamLineDraw(v, v1, &data);
			if (!onTileData.IsFirstWall && !onTileData.AreAllFloors)
			{
				return false;
			}
		}
	}
	return true;
}
static void FixCorridorOnTile(void *data, struct vec2i v)
{
	FixCorridorOnTileData *onTileData = data;
	if (onTileData->Counter == 1)
	{
		// This is the first tile after the starting tile
		// If this tile is a wall, result is good, and we don't care about
		// the rest of the tiles anymore
		onTileData->IsFirstWall = IMapGet(onTileData->M, v) == MAP_WALL;
	}
	if (onTileData->Counter > 0)
	{
		if (IMapGet(onTileData->M, v) != MAP_FLOOR)
		{
			onTileData->AreAllFloors = false;
		}
	}
	onTileData->Counter++;
}

static bool MapIsAreaClearForCaveSquare(
	const Map *map, const struct vec2i pos, const struct vec2i size);
static void PlaceSquares(Map *map, const int squares)
{
	// Place empty square areas on the map
	// This can only be done if at least one tile in the square is a floor type
	int count = 0;
	for (int i = 0; i < 1000 && count < squares; i++)
	{
		const struct vec2i v = MapGetRandomTile(map);
		const struct vec2i size = svec2i(rand() % 9 + 8, rand() % 9 + 8);
		if (!MapIsAreaClearForCaveSquare(map, v, size))
		{
			continue;
		}
		MapMakeSquare(map, v, size);
		count++;
	}
}
static bool MapIsAreaClearForCaveSquare(
	const Map *map, const struct vec2i pos, const struct vec2i size)
{
	if (!MapIsAreaInside(map, pos, size))
	{
		return false;
	}

	// For area to be clear, it must have:
	// - At least one floor tile
	// - No square tiles
	struct vec2i v;
	bool hasFloor = false;
	for (v.y = pos.y; v.y < pos.y + size.y; v.y++)
	{
		for (v.x = pos.x; v.x < pos.x + size.x; v.x++)
		{
			switch (IMapGet(map, v))
			{
				case MAP_FLOOR:
					hasFloor = true;
					break;
				case MAP_WALL:
					break;
				default:
					// Any other tile type is disallowed
					return false;
			}
		}
	}
	return hasFloor;
}

static bool MapIsAreaClearForCaveRoom(
	const Map *map, const Rect2i room, const Mission *m);
static void MapBuildRoom(Map *map, const Rect2i room, const Mission *m);
static void PlaceRooms(Map *map, const Mission *m)
{
	CArray rooms;	// of Rect2i
	CArrayInit(&rooms, sizeof(Rect2i));
	for (int i = 0; i < 1000 && (int)rooms.size < m->u.Cave.Rooms.Count; i++)
	{
		Rect2i room;
		room.Pos = MapGetRandomTile(map);
		room.Size = MapGetRoomSize(m->u.Cave.Rooms, 0);
		if (!MapIsAreaClearForCaveRoom(map, room, m))
		{
			continue;
		}
		MapBuildRoom(map, room, m);
		CArrayPushBack(&rooms, &room);
		LOG(LM_MAP, LL_TRACE, "Room %d, %d (%dx%d)",
			room.Pos.x, room.Pos.y, room.Size.x, room.Size.y);
	}
	// Set keys for rooms
	if (AreKeysAllowed(gCampaign.Entry.Mode) && m->u.Cave.DoorsEnabled)
	{
		while (rooms.size > 0)
		{
			// generate an access level for this room
			const unsigned short accessMask =
				GenerateAccessMask(&map->keyAccessCount);
			if (map->keyAccessCount < 1)
			{
				map->keyAccessCount = 1;
			}
			MapSetRoomAccessMaskOverlap(map, &rooms, accessMask);
		}
	}
	CArrayTerminate(&rooms);
}

static bool CaveRoomOutsideOk(const Map *map, const struct vec2i v);
static bool MapIsAreaClearForCaveRoom(
	const Map *map, const Rect2i room, const Mission *m)
{
	if (!MapIsAreaInside(map, room.Pos, room.Size))
	{
		return false;
	}

	// For area to be clear, it must have:
	// - Area has at least one floor tile
	// - Can only contain floor, wall or room tiles
	// - Not be entirely surrounded by walls
	bool hasFloor = false;
	bool hasFloorAroundEdge = false;
	bool isOverlapRoom = false;
	RECT_FOREACH(room)
		switch (IMapGet(map, _v))
		{
			case MAP_FLOOR:
				hasFloor = true;
				if (Rect2iIsAtEdge(room, _v))
				{
					hasFloorAroundEdge = true;
				}
				break;
			case MAP_WALL:
				break;
			case MAP_ROOM:	// passthrough
			case MAP_DOOR:
				isOverlapRoom = true;
				break;
			default:
				// Any other tile type is disallowed
				return false;
		}
	RECT_FOREACH_END()
	if (!hasFloor || !hasFloorAroundEdge)
	{
		return false;
	}

	// For edges:
	// - Either the edge is a wall, or
	// - Edge is floor/room, and outside is floor/room/square
	// This is to prevent rooms from cutting off areas of the map
	RECT_FOREACH(room)
		if (!Rect2iIsAtEdge(room, _v))
		{
			continue;
		}
		const bool isTop = _v.y == room.Pos.y;
		const bool isBottom = _v.y == room.Pos.y + room.Size.y - 1;
		const bool isLeft = _v.x == room.Pos.x;
		const bool isRight = _v.x == room.Pos.x + room.Size.x - 1;
		const struct vec2i outside = svec2i(
			isLeft ? room.Pos.x - 1 :
			(isRight ? room.Pos.x + room.Size.x : _v.x),
			isTop ? room.Pos.y - 1 :
			(isBottom ? room.Pos.y + room.Size.y : _v.y));
		const struct vec2i outsideX = svec2i(
			(isLeft || isRight) ? outside.x : _v.x, _v.y);
		const struct vec2i outsideY = svec2i(
			_v.x, (isTop || isBottom) ? outside.y : _v.y);
		switch (IMapGet(map, _v))
		{
			case MAP_WALL:	// passthrough
			case MAP_DOOR:
				// Note: also need to check outside to see if we overlap
				// but just along the edge
				if (IMapGet(map, outside) == MAP_ROOM ||
					IMapGet(map, outsideX) == MAP_ROOM ||
					IMapGet(map, outsideY) == MAP_ROOM)
				{
					isOverlapRoom = true;
				}
				break;
			case MAP_FLOOR:	// passthrough
			case MAP_ROOM:
				// Check outside tiles
				if (!CaveRoomOutsideOk(map, outside) ||
					!CaveRoomOutsideOk(map, outsideX) ||
					!CaveRoomOutsideOk(map, outsideY))
				{
					return false;
				}
				break;
			default:
				CASSERT(false, "unexpected tile type");
				return false;
		}
	RECT_FOREACH_END()

	// Check if room overlaps with another room and the overlap is valid
	if (isOverlapRoom)
	{
		if (!m->u.Cave.Rooms.Overlap)
		{
			// Overlapping disabled
			return false;
		}
		// Now check if the overlapping rooms will create a passage
		// large enough
		const int roomOverlapSize = MapGetRoomOverlapSize(
			map, room.Pos, room.Size, NULL);
		if (roomOverlapSize < m->u.Cave.CorridorWidth)
		{
			return false;
		}
	}

	return true;
}
static bool CaveRoomOutsideOk(const Map *map, const struct vec2i v)
{
	const unsigned short t = IMapGet(map, v);
	return t == MAP_FLOOR || t == MAP_ROOM || t == MAP_SQUARE;
}

static void MapBuildRoom(Map *map, const Rect2i room, const Mission *m)
{
	// For edges, any tile that is next to a floor must be turned into
	// a door, unless it was a corner - then it must be a wall
	// This is to prevent generating inaccessible rooms, or blocking off
	// sections of the map
	// TODO: if this is a locked room, can still cause the level to be
	// blocked off
	RECT_FOREACH(room)
		if (!Rect2iIsAtEdge(room, _v))
		{
			continue;
		}
		const bool isTop = _v.y == room.Pos.y;
		const bool isBottom = _v.y == room.Pos.y + room.Size.y - 1;
		const bool isLeft = _v.x == room.Pos.x;
		const bool isRight = _v.x == room.Pos.x + room.Size.x - 1;
		const struct vec2i outside = svec2i(
			isLeft ? room.Pos.x - 1 :
			(isRight ? room.Pos.x + room.Size.x : _v.x),
			isTop ? room.Pos.y - 1 :
			(isBottom ? room.Pos.y + room.Size.y : _v.y));
		const struct vec2i outsideX = svec2i(
			(isLeft || isRight) ? outside.x : _v.x, _v.y);
		const struct vec2i outsideY = svec2i(
			_v.x, (isTop || isBottom) ? outside.y : _v.y);
		const bool atEdgeOfMap =
			_v.y == 0 || _v.y == map->Size.y - 1 ||
			_v.x == 0 || _v.x == map->Size.x - 1;
		switch (IMapGet(map, _v) & MAP_MASKACCESS)
		{
			case MAP_DOOR:
				if (!CaveRoomOutsideOk(map, outside) &&
					!CaveRoomOutsideOk(map, outsideX) &&
					!CaveRoomOutsideOk(map, outsideY))
				{
					// This door would become a corner
					IMapSet(map, _v, MAP_WALL);
				}
				break;
			default:
				// Check outside tiles
				if (!atEdgeOfMap &&
					(svec2i_is_equal(outside, _v) || CaveRoomOutsideOk(map, outside)) &&
					(svec2i_is_equal(outsideX, _v) || CaveRoomOutsideOk(map, outsideX)) &&
					(svec2i_is_equal(outsideY, _v) || CaveRoomOutsideOk(map, outsideY)))
				{
					IMapSet(
						map, _v,
						m->u.Cave.DoorsEnabled ? MAP_DOOR : MAP_ROOM);
				}
				else
				{
					IMapSet(map, _v, MAP_WALL);
				}
				break;
		}

		const bool leftOrRightEdge = isLeft || isRight;
		const bool topOrBottomEdge = isTop || isBottom;
		if (leftOrRightEdge && topOrBottomEdge)
		{
			// corner
			IMapSet(map, _v, MAP_WALL);
		}
	RECT_FOREACH_END()

	MapMakeRoom(map, room.Pos, room.Size, false);

	MapMakeRoomWalls(map, m->u.Cave.Rooms);
}
