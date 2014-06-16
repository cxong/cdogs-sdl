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

    Copyright (c) 2013, Cong Xu
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
#include "collision.h"

#include "actors.h"
#include "config.h"

CollisionTeam CalcCollisionTeam(int isActor, TActor *actor)
{
	// Need to have prisoners collide with everything otherwise they will not
	// be "rescued"
	// Also need victims to collide with everyone
	if (!isActor || (actor->flags & (FLAGS_PRISONER | FLAGS_VICTIM)) ||
		gCampaign.Entry.Mode == CAMPAIGN_MODE_DOGFIGHT)
	{
		return COLLISIONTEAM_NONE;
	}
	if (actor->pData || (actor->flags & FLAGS_GOOD_GUY))
	{
		return COLLISIONTEAM_GOOD;
	}
	return COLLISIONTEAM_BAD;
}

bool IsCollisionWithWall(Vec2i pos, Vec2i size)
{
	if (pos.x - size.x < 0 ||
		pos.y - size.y < 0 ||
		pos.x + size.x >= gMap.Size.x * TILE_WIDTH ||
		pos.y + size.y >= gMap.Size.y * TILE_HEIGHT)
	{
		return true;
	}
	if (HitWall(pos.x - size.x,	pos.y - size.y) ||
		HitWall(pos.x - size.x,	pos.y) ||
		HitWall(pos.x - size.x,	pos.y + size.y) ||
		HitWall(pos.x,			pos.y + size.y) ||
		HitWall(pos.x + size.x,	pos.y + size.y) ||
		HitWall(pos.x + size.x,	pos.y) ||
		HitWall(pos.x + size.x,	pos.y - size.y) ||
		HitWall(pos.x,			pos.y - size.y))
	{
		return true;
	}
	return false;
}

bool IsCollisionWallOrEdge(Map *map, Vec2i pos, Vec2i size)
{
	Vec2i mapSize =
		Vec2iNew(map->Size.x * TILE_WIDTH, map->Size.y * TILE_HEIGHT);
	if (pos.x < 0 || pos.x + size.x >= mapSize.x ||
		pos.y < 0 || pos.y + size.y >= mapSize.y)
	{
		return true;
	}
	return IsCollisionWithWall(pos, size);
}

int ItemsCollide(TTileItem *item1, TTileItem *item2, Vec2i pos)
{
	int dx = abs(pos.x - item2->x);
	int dy = abs(pos.y - item2->y);
	int rx = item1->w + item2->w;
	int ry = item1->h + item2->h;

	if (dx < rx && dy < ry)
	{
		int odx = abs(item1->x - item2->x);
		int ody = abs(item1->y - item2->y);

		if (dx <= odx || dy <= ody)
		{
			return 1;
		}
	}
	return 0;
}

bool CollisionIsOnSameTeam(
	const TTileItem *i, const CollisionTeam team, const bool isDogfight)
{
	if (gConfig.Game.AllyCollision != ALLYCOLLISION_NORMAL)
	{
		CollisionTeam itemTeam = COLLISIONTEAM_NONE;
		if (i->kind == KIND_CHARACTER)
		{
			TActor *a = CArrayGet(&gActors, i->id);
			itemTeam = CalcCollisionTeam(1, a);
		}
		return
			team != COLLISIONTEAM_NONE &&
			itemTeam != COLLISIONTEAM_NONE &&
			team == itemTeam &&
			!isDogfight;
	}
	return 0;
}

TTileItem *GetItemOnTileInCollision(
	TTileItem *item, Vec2i pos, int mask, CollisionTeam team, int isDogfight)
{
	const Vec2i tv = Vec2iToTile(pos);
	Vec2i dv;
	// Check collisions with all other items on this tile, in all 8 directions
	for (dv.y = -1; dv.y <= 1; dv.y++)
	{
		for (dv.x = -1; dv.x <= 1; dv.x++)
		{
			const Vec2i dtv = Vec2iAdd(tv, dv);
			if (!MapIsTileIn(&gMap, dtv))
			{
				continue;
			}
			CArray *tileThings = &MapGetTile(&gMap, dtv)->things;
			for (int i = 0; i < (int)tileThings->size; i++)
			{
				TTileItem *ti = ThingIdGetTileItem(CArrayGet(tileThings, i));
				// Don't collide if items are on the same team
				if (!CollisionIsOnSameTeam(ti, team, isDogfight))
				{
					if (item != ti &&
						(ti->flags & mask) &&
						ItemsCollide(item, ti, pos))
					{
						return ti;
					}
				}
			}
		}
	}

	return NULL;
}
