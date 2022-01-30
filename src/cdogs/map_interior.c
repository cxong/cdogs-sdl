/*
	C-Dogs SDL
	A port of the legendary (and fun) action/arcade cdogs.
	Copyright (c) 2021 Cong Xu
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
#include "map_interior.h"

#include "algorithms.h"
#include "log.h"
#include "map_build.h"

#define CORRIDOR_LEVEL_DIFF_BLOCK 1

// Which side of the critical path this area is on
// This is important for key placement, as the start is placed on the left,
// and exit on the right.
typedef enum
{
	CRIT_PATH_NONE,
	CRIT_PATH_LEFT,
	CRIT_PATH_RIGHT,
} CriticalPath;

typedef struct
{
	Rect2i r;
	int level;
	int parent;
	int child1;
	int child2;
	bool horizontal;
	bool isCorridor;
	CriticalPath criticalPath;
} BSPArea;

static BSPArea BSPAreaRoot(const struct vec2i size)
{
	BSPArea b;
	memset(&b, 0, sizeof b);
	b.r = Rect2iNew(svec2i_zero(), size);
	b.parent = -1;
	b.child1 = -1;
	b.child2 = -1;
	return b;
}
static bool BSPAreaIsLeaf(const BSPArea *b)
{
	return b->child1 == -1 && b->child2 == -1;
}

typedef struct
{
	CArray a;
	size_t dim;
} Adjacency;

static Adjacency AdjacencyNew(const size_t dim)
{
	Adjacency am;
	CArrayInitFillZero(&am.a, sizeof(bool), dim * dim);
	am.dim = dim;
	return am;
}
static void AdjacencyTerminate(Adjacency *am)
{
	CArrayTerminate(&am->a);
}
static void AdjacencyConnect(Adjacency *am, const int i, const int j)
{
	CArraySet(&am->a, i + j * am->dim, &gTrue);
	CArraySet(&am->a, j + i * am->dim, &gTrue);
}
static bool AdjacencyIsConnected(const Adjacency *am, const int i, const int j)
{
	const bool *isAdjacent = CArrayGet(&am->a, i + j * am->dim);
	return *isAdjacent;
}
static bool AdjacencyHasConnections(const Adjacency *am, const int i)
{
	for (int j = 0; j < (int)am->dim; j++)
	{
		if (AdjacencyIsConnected(am, i, j))
		{
			return true;
		}
	}
	return false;
}
static void AdjacencyDisconnectAll(Adjacency *am, const int i)
{
	for (int j = 0; j < (int)am->dim; j++)
	{
		CArraySet(&am->a, i + j * am->dim, &gFalse);
		CArraySet(&am->a, j + i * am->dim, &gFalse);
	}
}

static void SplitAreas(MapBuilder *mb, CArray *areas);
static void SplitLeafRooms(MapBuilder *mb, CArray *areas);
static void FillRooms(MapBuilder *mb, const CArray *areas);
static Adjacency SetupAdjacencyMatrix(const CArray *areas);
static void AddDoorsToClosestCorridors(
	MapBuilder *mb, CArray *areas, Adjacency *am);
static void ConnectUnconnectedRooms(
	MapBuilder *mb, CArray *areas, Adjacency *am);
static void FindAndMarkCriticalPath(
	MapBuilder *mb, const CArray *areas, const int missionIndex);
static void FillCorridors(MapBuilder *mb, const CArray *areas);
static CArray CalcDistanceToCriticalPath(
	const CArray *areas, const Adjacency *am);
static void PlaceKeys(
	MapBuilder *mb, const CArray *areas, const Adjacency *am,
	CArray *dCriticalPath);
static void AddPillars(
	MapBuilder *mb, const CArray *areas, Adjacency *am,
	const CArray *dCriticalPath);
static void AddRoomWalls(MapBuilder *mb, const CArray *areas);
void MapInteriorLoad(MapBuilder *mb, const int missionIndex)
{
	// TODO: multiple tile types
	MissionSetupTileClasses(
		mb->Map, &gPicManager, &mb->mission->u.Interior.TileClasses);

	CArray areas;
	CArrayInit(&areas, sizeof(BSPArea));
	BSPArea a = BSPAreaRoot(mb->Map->Size);

	// Split the map for a number of iterations, choosing alternating axis and
	// random location
	CArrayPushBack(&areas, &a);
	SplitAreas(mb, &areas);

	// Try to split leaf rooms into more rooms, by longest axis
	SplitLeafRooms(mb, &areas);

	FillRooms(mb, &areas);

	if (areas.size > 1)
	{
		Adjacency am = SetupAdjacencyMatrix(&areas);

		// Add doors leading to the closest corridor in the hierarchy
		AddDoorsToClosestCorridors(mb, &areas, &am);

		// For every room, connect it to a random shallower room
		// Keep going until all rooms are connected
		ConnectUnconnectedRooms(mb, &areas, &am);

		// Find deepest leaf going down both branches; place start/end
		// This represents longest/critical path
		FindAndMarkCriticalPath(mb, &areas, missionIndex);

		FillCorridors(mb, &areas);

		CArray dCriticalPath = CalcDistanceToCriticalPath(&areas, &am);

		if (mb->mission->u.Interior.Doors.Enabled)
		{
			// For each locked street (street on critical path), place a key in
			// a non-critical leaf Do so by following a child away from the
			// critical path
			PlaceKeys(mb, &areas, &am, &dCriticalPath);
		}

		// Fill random leaf rooms
		AddPillars(mb, &areas, &am, &dCriticalPath);

		// Add rooms after placing keys to avoid overlaps
		AddRoomWalls(mb, &areas);

		CArrayTerminate(&dCriticalPath);
		AdjacencyTerminate(&am);
	}

	CArrayTerminate(&areas);
}

static bool BSPAreaTrySplit(
	const CArray *areas, const bool horizontal, const int idx,
	const int minSize, BSPArea *r1, BSPArea *r2);
static void SplitAreas(MapBuilder *mb, CArray *areas)
{
	const int hcount = RAND_INT(0, 2);

	// Need to allow at least one split
	const int minSize =
		MIN(mb->mission->u.Interior.Rooms.Min +
				mb->mission->u.Interior.CorridorWidth / 2,
			MIN(mb->Map->Size.x, mb->Map->Size.y));
	CA_FOREACH(BSPArea, a, *areas)
	// Alternate splitting direction per level
	const bool horizontal = ((hcount + a->level) % 2) == 1;
	BSPArea a1 = BSPAreaRoot(svec2i_zero());
	BSPArea a2 = BSPAreaRoot(svec2i_zero());
	if (BSPAreaTrySplit(areas, horizontal, _ca_index, minSize, &a1, &a2))
	{
		// Resize rooms to allow space for street
		for (int i = 0; i < mb->mission->u.Interior.CorridorWidth; i++)
		{
			if (horizontal)
			{
				if ((i % 2) == 0)
				{
					a1.r.Size.x--;
				}
				else
				{
					a2.r.Pos.x++;
					a2.r.Size.x--;
				}
			}
			else
			{
				if ((i % 2) == 0)
				{
					a1.r.Size.y--;
				}
				else
				{
					a2.r.Pos.y++;
					a2.r.Size.y--;
				}
			}
		}
		// Replace current area with a corridor
		a->isCorridor = true;
		if (horizontal)
		{
			a->r = Rect2iNew(
				svec2i(a1.r.Pos.x + a1.r.Size.x, a1.r.Pos.y),
				svec2i(mb->mission->u.Interior.CorridorWidth, a1.r.Size.y));
		}
		else
		{
			a->r = Rect2iNew(
				svec2i(a1.r.Pos.x, a1.r.Pos.y + a1.r.Size.y),
				svec2i(a1.r.Size.x, mb->mission->u.Interior.CorridorWidth));
		}
		a->horizontal = !horizontal;

		// Add children
		a->child1 = (int)areas->size;
		a->child2 = (int)areas->size + 1;
		CArrayPushBack(areas, &a1);
		CArrayPushBack(areas, &a2);
	}
	CA_FOREACH_END()
}
static bool BSPAreaTrySplit(
	const CArray *areas, const bool horizontal, const int idx,
	const int minSize, BSPArea *a1, BSPArea *a2)
{
	const BSPArea *area = CArrayGet(areas, idx);
	const int r = horizontal ? area->r.Size.x - minSize * 2
							 : area->r.Size.y - minSize * 2;
	if (r < 0)
	{
		// Too small; can't split
		return false;
	}
	a1->level = a2->level = area->level + 1;
	a1->parent = a2->parent = idx;
	a1->horizontal = a2->horizontal = horizontal;
	if (horizontal)
	{
		// Left/right children
		const int x = RAND_INT(0, r) + minSize;
		a1->r = Rect2iNew(area->r.Pos, svec2i(x, area->r.Size.y));
		a2->r = Rect2iNew(
			svec2i(area->r.Pos.x + x, area->r.Pos.y),
			svec2i(area->r.Size.x - x, area->r.Size.y));
	}
	else
	{
		// Top/bottom children
		const int y = RAND_INT(0, r) + minSize;
		a1->r = Rect2iNew(area->r.Pos, svec2i(area->r.Size.x, y));
		a2->r = Rect2iNew(
			svec2i(area->r.Pos.x, area->r.Pos.y + y),
			svec2i(area->r.Size.x, area->r.Size.y - y));
	}
	return true;
}

static void SplitLeafRooms(MapBuilder *mb, CArray *areas)
{
	CA_FOREACH(BSPArea, a, *areas)
	if (a->isCorridor)
	{
		continue;
	}
	// Don't split if we're under max size
	if (a->r.Size.x <= mb->mission->u.Interior.Rooms.Max &&
		a->r.Size.y <= mb->mission->u.Interior.Rooms.Max)
	{
		continue;
	}
	const bool horizontal = a->r.Size.x > a->r.Size.y;
	BSPArea a1 = BSPAreaRoot(svec2i_zero());
	BSPArea a2 = BSPAreaRoot(svec2i_zero());
	if (BSPAreaTrySplit(
			areas, horizontal, _ca_index, mb->mission->u.Interior.Rooms.Min,
			&a1, &a2))
	{
		// Resize rooms so they share a splitting wall
		if (a1.horizontal)
		{
			a1.r.Size.x++;
		}
		else
		{
			a1.r.Size.y++;
		}

		// Add children
		a->child1 = (int)areas->size;
		CArrayPushBack(areas, &a1);
		a->child2 = (int)areas->size;
		CArrayPushBack(areas, &a2);
	}
	CA_FOREACH_END()
}

static void FillRooms(MapBuilder *mb, const CArray *areas)
{
	CA_FOREACH(BSPArea, a, *areas)
	// Skip non-leaves
	if (!BSPAreaIsLeaf(a))
	{
		continue;
	}
	MapMakeRoom(
		mb, a->r.Pos, a->r.Size, true,
		&mb->mission->u.Interior.TileClasses.Wall,
		&mb->mission->u.Interior.TileClasses.Room, false);
	CA_FOREACH_END()
}

static Adjacency SetupAdjacencyMatrix(const CArray *areas)
{
	Adjacency am = AdjacencyNew(areas->size);
	CA_FOREACH(const BSPArea, a, *areas)
	if (!a->isCorridor || a->parent == -1)
	{
		continue;
	}
	AdjacencyConnect(&am, _ca_index, a->parent);
	CA_FOREACH_END()
	return am;
}

static void AddDoorsToClosestCorridors(
	MapBuilder *mb, CArray *areas, Adjacency *am)
{
	CA_FOREACH(BSPArea, a, *areas)
	// Skip non-leaves
	if (!BSPAreaIsLeaf(a))
	{
		continue;
	}
	// Add doors leading to corridors
	const BSPArea *corridor = a;
	int corridorI = _ca_index;
	for (; !corridor->isCorridor && corridor->parent >= 0;)
	{
		corridorI = corridor->parent;
		corridor = CArrayGet(areas, corridorI);
	}
	// Four walls/directions
	for (int i = 0; i < 4; i++)
	{
		struct vec2i doorPos =
			svec2i(a->r.Pos.x + a->r.Size.x / 2, a->r.Pos.y + a->r.Size.y / 2);
		struct vec2i outsideDoor = doorPos;
		switch (i)
		{
		case 0:
			// left
			doorPos.x = a->r.Pos.x;
			outsideDoor = svec2i(doorPos.x - 1, doorPos.y);
			break;
		case 1:
			// right
			doorPos.x = a->r.Pos.x + a->r.Size.x - 1;
			outsideDoor = svec2i(doorPos.x + 1, doorPos.y);
			break;
		case 2:
			// top
			doorPos.y = a->r.Pos.y;
			outsideDoor = svec2i(doorPos.x, doorPos.y - 1);
			break;
		case 3:
			// bottom
			doorPos.y = a->r.Pos.y + a->r.Size.y - 1;
			outsideDoor = svec2i(doorPos.x, doorPos.y + 1);
			break;
		default:
			CASSERT(false, "unexpected");
			break;
		}
		if (Rect2iIsInside(corridor->r, outsideDoor))
		{
			bool doors[4];
			memset(&doors, 0, sizeof doors);
			doors[i] = true;
			MapPlaceDoors(
				mb, a->r, mb->mission->u.Interior.Doors.Enabled, doors,
				mb->mission->u.Interior.Doors.Min,
				mb->mission->u.Interior.Doors.Max, 0,
				mb->mission->u.Interior.Doors.RandomPos,
				&mb->mission->u.Interior.TileClasses.Door,
				&mb->mission->u.Interior.TileClasses.Room);
			AdjacencyConnect(am, _ca_index, corridorI);
			// Change parentage
			a->parent = corridorI;
			break;
		}
	}
	CA_FOREACH_END()
}

static bool TryConnectRooms(
	MapBuilder *mb, CArray *areas, Adjacency *am, BSPArea *a1, const int aIdx);
static void ConnectUnconnectedRooms(
	MapBuilder *mb, CArray *areas, Adjacency *am)
{
	int numUnconnected = 0;
	do
	{
		numUnconnected = 0;
		CA_FOREACH(BSPArea, a, *areas)
		if (AdjacencyHasConnections(am, _ca_index) || a->isCorridor ||
			!BSPAreaIsLeaf(a))
		{
			continue;
		}
		if (!TryConnectRooms(mb, areas, am, a, _ca_index))
		{
			numUnconnected++;
		}
		CA_FOREACH_END()
	} while (numUnconnected > 0);
}
static bool RectIsAdjacent(
	const Rect2i r1, const Rect2i r2, const int overlapSize);
static bool TryConnectRooms(
	MapBuilder *mb, CArray *areas, Adjacency *am, BSPArea *a1, const int aIdx)
{
	CA_FOREACH(BSPArea, a2, *areas)
	// Only connect to a room that is also connected
	if (!BSPAreaIsLeaf(a2) || a1 == a2 ||
		!AdjacencyHasConnections(am, _ca_index))
	{
		continue;
	}
	// Shrink rectangles by 1 to determine
	// overlap
	Rect2i r1 = a1->r;
	r1.Size.x--;
	r1.Size.y--;
	Rect2i r2 = a2->r;
	r2.Size.x--;
	r2.Size.y--;
	const int overlapSize = 1;
	if (!RectIsAdjacent(r1, r2, overlapSize))
	{
		continue;
	}
	// Rooms are adjacent; add a door in the adjacent area
	bool doors[4];
	memset(&doors, 0, sizeof doors);
	if (a1->r.Pos.x > a2->r.Pos.x)
	{
		// left
		doors[0] = true;
	}
	else if (a1->r.Pos.x < a2->r.Pos.x)
	{
		// right
		doors[1] = true;
	}
	else if (a1->r.Pos.y > a2->r.Pos.y)
	{
		// top
		doors[2] = true;
	}
	else
	{
		// botttom
		doors[3] = true;
	}
	MapPlaceDoors(
		mb, a1->r, mb->mission->u.Interior.Doors.Enabled, doors,
		mb->mission->u.Interior.Doors.Min, mb->mission->u.Interior.Doors.Max,
		0, mb->mission->u.Interior.Doors.RandomPos,
		&mb->mission->u.Interior.TileClasses.Door,
		&mb->mission->u.Interior.TileClasses.Room);
	AdjacencyConnect(am, aIdx, _ca_index);
	// Change parentage
	a1->parent = _ca_index;
	return true;
	CA_FOREACH_END()
	return false;
}
static bool RectIsAdjacent(
	const Rect2i r1, const Rect2i r2, const int overlapSize)
{
	// If left/right edges adjacent
	if (r1.Pos.x - (r2.Pos.x + r2.Size.x) == 0 ||
		r2.Pos.x - (r1.Pos.x + r1.Size.x) == 0)
	{
		return r1.Pos.y + overlapSize < r2.Pos.y + r2.Size.y &&
			   r2.Pos.y + overlapSize < r1.Pos.y + r1.Size.y;
	}
	if (r1.Pos.y - (r2.Pos.y + r2.Size.y) == 0 ||
		r2.Pos.y - (r1.Pos.y + r1.Size.y) == 0)
	{
		return r1.Pos.x + overlapSize < r2.Pos.x + r2.Size.x &&
			   r2.Pos.x + overlapSize < r1.Pos.x + r1.Size.x;
	}
	return false;
}

static int FindDeepestRoomFrom(const CArray *areas, const int idx);
static void MarkParentCorridors(
	const CArray *areas, const int idx, const CriticalPath cp);
static void FindAndMarkCriticalPath(
	MapBuilder *mb, const CArray *areas, const int missionIndex)
{
	const BSPArea *root = CArrayGet(areas, 0);
	const int deepestRoom1 = FindDeepestRoomFrom(areas, root->child1);
	const BSPArea *a1 = CArrayGet(areas, deepestRoom1);
	mb->Map->start =
		svec2i_add(a1->r.Pos, svec2i_divide(a1->r.Size, svec2i(2, 2)));
	const int deepestRoom2 = FindDeepestRoomFrom(areas, root->child2);
	const BSPArea *a2 = CArrayGet(areas, deepestRoom2);
	if (mb->mission->u.Interior.ExitEnabled)
	{
		Exit exit;
		exit.Mission = missionIndex + 1;
		exit.Hidden = false;
		exit.R.Pos = svec2i_add(a2->r.Pos, svec2i_one());
		exit.R.Size = svec2i_subtract(a2->r.Size, svec2i(3, 3));
		CArrayPushBack(&mb->Map->exits, &exit);
	}
	MarkParentCorridors(areas, deepestRoom1, CRIT_PATH_LEFT);
	MarkParentCorridors(areas, deepestRoom2, CRIT_PATH_RIGHT);
}
static int FindDeepestRoomFrom(const CArray *areas, const int idx)
{
	if (idx == -1)
	{
		return 0;
	}
	CArray pathStack;
	CArrayInit(&pathStack, sizeof(int));
	CArrayPushBack(&pathStack, &idx);
	int deepestChild = -1;
	int maxDepth = 0;
	while (pathStack.size > 0)
	{
		const int i = *(int *)CArrayGet(&pathStack, pathStack.size - 1);
		const BSPArea *a = CArrayGet(areas, i);
		CArrayPopBack(&pathStack);
		if (BSPAreaIsLeaf(a))
		{
			if (a->level > maxDepth)
			{
				maxDepth = a->level;
				deepestChild = i;
			}
		}
		if (a->child1 >= 0)
		{
			CArrayPushBack(&pathStack, &a->child1);
		}
		if (a->child2 >= 0)
		{
			CArrayPushBack(&pathStack, &a->child2);
		}
	}
	CArrayTerminate(&pathStack);
	return deepestChild;
}
static void MarkParentCorridors(
	const CArray *areas, const int idx, const CriticalPath cp)
{
	for (int i = idx; i >= 0;)
	{
		BSPArea *corridor = CArrayGet(areas, i);
		corridor->criticalPath = cp;
		i = corridor->parent;
	}
}

static void CapCorridor(
	MapBuilder *mb, const CArray *areas, const BSPArea *a,
	const struct vec2i end, const struct vec2i dAcross,
	const struct vec2i dAlong);
static struct vec2i CorridorDAlong(const BSPArea *a);
static struct vec2i CorridorDAcross(const BSPArea *a);
static void FillCorridors(MapBuilder *mb, const CArray *areas)
{
	CA_FOREACH(const BSPArea, a, *areas)
	// Iterate in reverse order so we cap child corridors first,
	// to correctly shrink corridor ends
	a = CArrayGet(areas, areas->size - _ca_index - 1);
	if (!a->isCorridor)
	{
		continue;
	}
	RECT_FOREACH(a->r)
	MapBuilderSetTile(mb, _v, &mb->mission->u.Interior.TileClasses.Floor);
	RECT_FOREACH_END()
	// Check ends of corridors - cap or place door
	const struct vec2i end1 = a->r.Pos;
	const struct vec2i end2 =
		svec2i_add(end1, svec2i_subtract(a->r.Size, svec2i_one()));
	CapCorridor(mb, areas, a, end1, CorridorDAcross(a), CorridorDAlong(a));
	CapCorridor(
		mb, areas, a, end2, svec2i_scale(CorridorDAcross(a), -1),
		svec2i_scale(CorridorDAlong(a), -1));
	CA_FOREACH_END()
}
static void CapCorridor(
	MapBuilder *mb, const CArray *areas, const BSPArea *a,
	const struct vec2i end, const struct vec2i dAcross,
	const struct vec2i dAlong)
{
	// Check ends of corridor - if outside map, or next to much older corridor,
	// block off with wall
	const struct vec2i outside = svec2i_subtract(end, dAlong);
	const TileClass *capTile = &mb->mission->u.Interior.TileClasses.Floor;
	if (!MapIsTileIn(mb->Map, outside))
	{
		capTile = &mb->mission->u.Interior.TileClasses.Wall;
	}
	else
	{
		CA_FOREACH(const BSPArea, a2, *areas)
		if (Rect2iIsInside(a2->r, outside))
		{
			if (a->level - a2->level > CORRIDOR_LEVEL_DIFF_BLOCK)
			{
				capTile = &mb->mission->u.Interior.TileClasses.Wall;
			}
			else if (mb->mission->u.Interior.Doors.Enabled)
			{
				capTile = &mb->mission->u.Interior.TileClasses.Door;
			}
			break;
		}
		CA_FOREACH_END()
	}
	for (int j = 0;; j++)
	{
		const struct vec2i endJ =
			svec2i_add(end, svec2i_multiply(dAlong, svec2i(j, j)));
		// Keep filling the end of the corridor unless there's
		// a door in the way
		if (MapBuilderGetTile(
				mb, svec2i_add(endJ, svec2i_multiply(dAcross, svec2i(-1, -1))))
					->Type != TILE_CLASS_WALL ||
			MapBuilderGetTile(
				mb, svec2i_add(
						endJ, svec2i_multiply(
								  dAcross,
								  svec2i(
									  mb->mission->u.Interior.CorridorWidth,
									  mb->mission->u.Interior.CorridorWidth))))
					->Type != TILE_CLASS_WALL)
		{
			break;
		}
		for (int i = 0; i < mb->mission->u.Interior.CorridorWidth; i++)
		{
			MapBuilderSetTile(
				mb, svec2i_add(endJ, svec2i_multiply(dAcross, svec2i(i, i))),
				capTile);
		}
		if (capTile->Type != TILE_CLASS_WALL)
		{
			break;
		}
	}
}
static struct vec2i CorridorDAlong(const BSPArea *a)
{
	// Unit vector going along the corridor
	return a->horizontal ? svec2i(1, 0) : svec2i(0, 1);
}
static struct vec2i CorridorDAcross(const BSPArea *a)
{
	// Unit vector going across the corridor
	return a->horizontal ? svec2i(0, 1) : svec2i(1, 0);
}

static int FindAdjacentCriticalPath(
	const CArray *areas, const Adjacency *am, const CArray *dCriticalPath,
	const int idx);
static CArray CalcDistanceToCriticalPath(
	const CArray *areas, const Adjacency *am)
{
	CArray dCriticalPath;
	CArrayInitFillZero(&dCriticalPath, sizeof(int), areas->size);
	CA_FOREACH(const BSPArea, a, *areas)
	if (a->criticalPath != CRIT_PATH_NONE)
	{
		const int d = 1;
		CArraySet(&dCriticalPath, _ca_index, &d);
	}
	CA_FOREACH_END()
	int newConnections;
	do
	{
		newConnections = 0;
		CA_FOREACH(const BSPArea, a, *areas)
		if (*(int *)CArrayGet(&dCriticalPath, _ca_index) > 0)
		{
			continue;
		}
		if (!a->isCorridor && !BSPAreaIsLeaf(a))
		{
			continue;
		}
		const int d =
			FindAdjacentCriticalPath(areas, am, &dCriticalPath, _ca_index);
		if (d > 0)
		{
			CArraySet(&dCriticalPath, _ca_index, &d);
			newConnections++;
		}
		CA_FOREACH_END()
	} while (newConnections > 0);
	return dCriticalPath;
}
static int FindAdjacentCriticalPath(
	const CArray *areas, const Adjacency *am, const CArray *dCriticalPath,
	const int idx)
{
	for (int i = 0; i < (int)areas->size; i++)
	{
		const int *d = CArrayGet(dCriticalPath, i);
		if (*d == 0)
		{
			continue;
		}
		if (AdjacencyIsConnected(am, idx, i))
		{
			return *d + 1;
		}
	}
	return 0;
}

static CArray FindRoomsFurthestFromCriticalPath(
	const CArray *areas, const Adjacency *am, const CArray *dCriticalPath,
	const int fromIdx);
static void AddLockedRooms(
	MapBuilder *mb, const CArray *areas, const CArray *rooms,
	const uint16_t accessMask);
static void PlaceKey(
	MapBuilder *mb, const CArray *areas, const Adjacency *am,
	CArray *dCriticalPath, const int idx, const int keyIndex,
	const CriticalPath cp);
static void PlaceKeys(
	MapBuilder *mb, const CArray *areas, const Adjacency *am,
	CArray *dCriticalPath)
{
	CArray lockedRoomCandidates;
	CArrayInit(&lockedRoomCandidates, sizeof(int));
	CA_FOREACH(const BSPArea, a, *areas)
	if (!a->isCorridor || a->criticalPath == CRIT_PATH_NONE || _ca_index == 0)
	{
		continue;
	}

	// If we're on the right critical path, we need to lock the child corridor
	// that is on the critical path instead
	if (a->criticalPath == CRIT_PATH_RIGHT)
	{
		const BSPArea *child1 = CArrayGet(areas, a->child1);
		const BSPArea *child2 = CArrayGet(areas, a->child2);
		a = child1->criticalPath != CRIT_PATH_NONE ? child1 : child2;
		if (!a->isCorridor)
		{
			continue;
		}
	}
	CArrayPushBack(&lockedRoomCandidates, &_ca_index);
	CA_FOREACH_END()

	// Randomly choose rooms to lock
	while (lockedRoomCandidates.size > KEY_COUNT)
	{
		CArrayDelete(
			&lockedRoomCandidates, RAND_INT(0, lockedRoomCandidates.size));
	}

	CA_FOREACH(const int, idx, lockedRoomCandidates)
	const BSPArea *a = CArrayGet(areas, *idx);
	if (a->criticalPath == CRIT_PATH_RIGHT)
	{
		const BSPArea *child1 = CArrayGet(areas, a->child1);
		const BSPArea *child2 = CArrayGet(areas, a->child2);
		a = child1->criticalPath != CRIT_PATH_NONE ? child1 : child2;
	}
	// Lock corridor
	const uint16_t accessMask = GetAccessMask(_ca_index);
	const struct vec2i end1 = a->r.Pos;
	const struct vec2i end2 =
		svec2i_add(end1, svec2i_subtract(a->r.Size, svec2i_one()));
	for (int i = 0; i < mb->mission->u.Interior.CorridorWidth; i++)
	{
		if (MapBuilderGetTile(mb, end1)->Type != TILE_CLASS_WALL)
		{
			MapSetRoomAccessMask(
				mb,
				Rect2iNew(
					svec2i_add(
						end1,
						svec2i_multiply(CorridorDAcross(a), svec2i(i, i))),
					svec2i_one()),
				accessMask);
		}
		if (MapBuilderGetTile(mb, end2)->Type != TILE_CLASS_WALL)
		{
			MapSetRoomAccessMask(
				mb,
				Rect2iNew(
					svec2i_add(
						end2,
						svec2i_multiply(CorridorDAcross(a), svec2i(-i, -i))),
					svec2i_one()),
				accessMask);
		}
	}

	// Place key in a child room before the locked corridor, but far away
	CArray furthestChildren =
		FindRoomsFurthestFromCriticalPath(areas, am, dCriticalPath, *idx);
	CArrayShuffle(&furthestChildren);
	CASSERT(furthestChildren.size > 0, "Cannot find child for locked street");

	const int child = *(int *)CArrayGet(&furthestChildren, 0);
	PlaceKey(mb, areas, am, dCriticalPath, child, _ca_index, a->criticalPath);

	// If there are more children, mark some of them as locked rooms
	// So that special objective items can be placed there
	AddLockedRooms(mb, areas, &furthestChildren, accessMask);

	CArrayTerminate(&furthestChildren);
	CA_FOREACH_END()
}
static CArray FindRoomsFurthestFromCriticalPath(
	const CArray *areas, const Adjacency *am, const CArray *dCriticalPath,
	const int fromIdx)
{
	CArray furthest;
	CArrayInit(&furthest, sizeof(int));
	CArrayPushBack(&furthest, &fromIdx);
	CA_FOREACH(const int, idxp, furthest)
	const int idx = *idxp;
	bool hasChildren = false;
	for (int i = 0; i < (int)areas->size; i++)
	{
		if (i == idx || !AdjacencyIsConnected(am, idx, i) ||
			*(int *)CArrayGet(dCriticalPath, i) <=
				*(int *)CArrayGet(dCriticalPath, idx))
		{
			continue;
		}
		CArrayPushBack(&furthest, &i);
		hasChildren = true;
	}
	const BSPArea *a = CArrayGet(areas, idx);
	if (hasChildren || a->criticalPath != CRIT_PATH_NONE)
	{
		CArrayDelete(&furthest, _ca_index);
		_ca_index--;
	}
	CA_FOREACH_END()
	return furthest;
}
static void AddLockedRooms(
	MapBuilder *mb, const CArray *areas, const CArray *rooms,
	const uint16_t accessMask)
{
	CA_FOREACH(const int, idx, *rooms)
	if (_ca_index == 0)
	{
		continue;
	}
	if (RAND_BOOL())
	{
		const BSPArea *room = CArrayGet(areas, *idx);
		MapSetRoomAccessMask(
			mb,
			Rect2iNew(
				svec2i_add(room->r.Pos, svec2i_one()),
				svec2i_subtract(room->r.Size, svec2i(2, 2))),
			accessMask);
	}
	CA_FOREACH_END()
}
static void PlaceKey(
	MapBuilder *mb, const CArray *areas, const Adjacency *am,
	CArray *dCriticalPath, const int idx, const int keyIndex,
	const CriticalPath cp)
{
	BSPArea *room = CArrayGet(areas, idx);
	const struct vec2i keyPos =
		svec2i_add(room->r.Pos, svec2i_divide(room->r.Size, svec2i(2, 2)));
	MapPlaceKey(mb, keyPos, keyIndex);
	// Prevent items being placed over the key
	MapBuilderSetLeaveFree(mb, keyPos, true);
	// Add room to critical path to avoid room walls here
	room->criticalPath = cp;
	int dNext = *(int *)CArrayGet(dCriticalPath, idx);
	int d = 1;
	CArraySet(dCriticalPath, idx, &d);
	// Path find until we're back on the critical path, marking the path
	// as critical too
	int idxNext = idx;
	for (dNext--; dNext > 1; dNext--)
	{
		CA_FOREACH(BSPArea, a, *areas)
		if (AdjacencyIsConnected(am, _ca_index, idxNext) &&
			*(int *)CArrayGet(dCriticalPath, _ca_index) == dNext)
		{
			idxNext = _ca_index;
			a->criticalPath = cp;
			CArraySet(dCriticalPath, idxNext, &d);
			break;
		}
		CA_FOREACH_END()
	}
}
static bool AllTilesAroundUnwalkable(
	const MapBuilder *mb, const struct vec2i v)
{
	RECT_FOREACH(Rect2iNew(svec2i_subtract(v, svec2i_one()), svec2i(3, 3)))
	const TileClass *tc = MapBuilderGetTile(mb, _v);
	if (tc != NULL && tc->canWalk)
	{
		return false;
	}
	RECT_FOREACH_END()
	return true;
}
static void AddPillars(
	MapBuilder *mb, const CArray *areas, Adjacency *am,
	const CArray *dCriticalPath)
{
	// Repeatedly add pillars because more candidates can be found
	// after placing some
	for (int count = 0; count < mb->mission->u.Interior.Pillars.Count;)
	{
		// Find all the children furthest away from the critical path
		CArray allChildren;
		memset(&allChildren, 0, sizeof allChildren);
		CA_FOREACH(const int, d, *dCriticalPath)
		if (*d == 1)
		{
			// Is on critical path; find furthest children
			CArray furthestChildren = FindRoomsFurthestFromCriticalPath(
				areas, am, dCriticalPath, _ca_index);
			CArrayConcat(&allChildren, &furthestChildren);
			CArrayTerminate(&furthestChildren);
			// Sort and remove duplicates
			qsort(
				allChildren.data, allChildren.size, allChildren.elemSize,
				CompareIntsDesc);
			CArrayUnique(&allChildren, IntsEqual);
		}
		CA_FOREACH_END()
		if (allChildren.size == 0)
		{
			break;
		}
		CArrayShuffle(&allChildren);
		CA_FOREACH(const int, idx, allChildren)
		if (count == mb->mission->u.Interior.Pillars.Count)
		{
			break;
		}
		const BSPArea *a = CArrayGet(areas, *idx);
		MapFillRect(
			mb, a->r, &mb->mission->u.Interior.TileClasses.Wall,
			&gTileNothing);
		MapSetRoomAccessMask(mb, a->r, 0);
		AdjacencyDisconnectAll(am, *idx);
		count++;
		CA_FOREACH_END()
		CArrayTerminate(&allChildren);
	}

	// Clean up orphaned walls (walls with no surrounding walkable tiles)
	RECT_FOREACH(Rect2iNew(svec2i_zero(), mb->Map->Size))
	if (AllTilesAroundUnwalkable(mb, _v))
	{
		MapBuilderSetTile(mb, _v, &gTileNothing);
	}
	RECT_FOREACH_END()
}

static void AddRoomWalls(MapBuilder *mb, const CArray *areas)
{
	CA_FOREACH(const BSPArea, a, *areas)
	// Skip non-leaves
	if (!BSPAreaIsLeaf(a))
	{
		continue;
	}

	if (a->criticalPath == CRIT_PATH_NONE)
	{
		MapMakeRoomWalls(
			mb, mb->mission->u.Interior.Rooms,
			&mb->mission->u.Interior.TileClasses.Wall, a->r);
	}
	CA_FOREACH_END()
}
