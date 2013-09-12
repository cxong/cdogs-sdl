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

CollisionTeam CalcCollisionTeam(int isActor, int actorFlags)
{
	// Need to have prisoners collide with everything otherwise they will not
	// be "rescued"
	if (!isActor || (actorFlags & FLAGS_PRISONER) ||
		gCampaign.Entry.mode == CAMPAIGN_MODE_DOGFIGHT)
	{
		return COLLISIONTEAM_NONE;
	}
	if (actorFlags & (FLAGS_PLAYERS | FLAGS_GOOD_GUY))
	{
		return COLLISIONTEAM_GOOD;
	}
	return COLLISIONTEAM_BAD;
}

int IsCollisionWithWall(Vec2i pos, Vec2i size)
{
	if (HitWall(pos.x - size.x,	pos.y - size.y) ||
		HitWall(pos.x - size.x,	pos.y) ||
		HitWall(pos.x - size.x,	pos.y + size.y) ||
		HitWall(pos.x,			pos.y + size.y) ||
		HitWall(pos.x + size.x,	pos.y + size.y) ||
		HitWall(pos.x + size.x,	pos.y) ||
		HitWall(pos.x + size.x,	pos.y - size.y) ||
		HitWall(pos.x,			pos.y - size.y))
	{
		return 1;
	}
	return 0;
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

TTileItem *GetItemOnTileInCollision(
	TTileItem *item, Vec2i pos, int mask, CollisionTeam team)
{
	int dy;
	int tx = pos.x / TILE_WIDTH;
	int ty = pos.y / TILE_HEIGHT;
	if (tx == 0 || ty == 0 || tx >= XMAX - 1 || ty >= YMAX - 1)
	{
		return NULL;
	}

	// Check collisions with all other items on this tile, in all 8 directions
	for (dy = -1; dy <= 1; dy++)
	{
		int dx;
		for (dx = -1; dx <= 1; dx++)
		{
			TTileItem *i = Map(tx + dx, ty + dy).things;
			while (i)
			{
				// Don't collide if items are on the same team
				CollisionTeam itemTeam = COLLISIONTEAM_NONE;
				if (i->kind == KIND_CHARACTER)
				{
					TActor *a = i->actor;
					itemTeam = CalcCollisionTeam(1, a->flags);
				}
				if (team == COLLISIONTEAM_NONE ||
					itemTeam == COLLISIONTEAM_NONE ||
					team != itemTeam)
				{
					if (item != i &&
					(i->flags & mask) &&
					ItemsCollide(item, i, pos))
					{
						return i;
					}
				}
				i = i->next;
			}
		}
	}

	return NULL;
}
